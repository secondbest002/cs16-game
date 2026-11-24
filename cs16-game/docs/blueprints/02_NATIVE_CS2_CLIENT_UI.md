# Blueprint 02 — Native CS2-Style Client UI (`client.dll` & `mainui_cpp`)

**Document ID:** `BP-02-NATIVE-CS2-CLIENT-UI`  
**Target Workspace:** `D:\Counter-Strike 1.6\cs16-client` (`cl_dll/` + `3rdparty/mainui_cpp/`) & `D:\Counter-Strike 1.6\cs16-game` (`cs2_client_studio.amxx`, `cs2_modern_engine.amxx`, `csgo_competitive.amxx`, `cs2_arsenal.amxx`)  
**Output Binaries:** `cs16-game/cstrike/cl_dlls/client.dll` & `cs16-game/menu.dll`  
**Architecture Baseline:** `cs16-game/ARCHITECTURE.md` (Option 1 + Option 2-Lite — 7 Active Custom Plugins + `include/cs2_sdk/`)  
**Primary Objective:** Menggantikan menu teks angka 1–9 dan DHUD AMXX yang berkedip dengan antarmuka **Native C++ CS2-Style** berlatensi nol (Live Studio Slider Panel, Single-Screen Grid Buy Menu + Right-Click Refund, Native Bottom Kill Cards, dan Post-Round Damage Strip) sambil mempertahankan **100% kompatibilitas mundur** untuk klien vanilla/tanpa patch.

---

## 1. Architectural Strategy: Dual-Layer Server–Client Co-Design

Workspace memiliki keunggulan struktural karena source penuh **`client.dll`** (`cs16-client/cl_dll/`) dan **`menu.dll`** (`cs16-client/3rdparty/mainui_cpp/`) berada tepat di sebelah `cs16-game/`. Daripada membangun ulang sistem input mouse mentah di `client.dll`, blueprint ini memisahkan tanggung jawab secara tegas ke dua lapisan yang memanfaatkan komponen yang **sudah tersedia** di Xash3D FWGS:

| Lapisan | Binary & Source Root | Basis Komponen Eksisting | Fitur yang Dimiliki | Karakteristik Input & Render |
| :--- | :--- | :--- | :--- | :--- |
| **Layer 1: Interactive Modal UI** | `menu.dll` (`cs16-client/3rdparty/mainui_cpp/`) | `CClientWindow` (`menus/client/ClientWindow.h`), `CMenuSlider`, `CMenuCheckBox`, `CMenuSpinControl`, `CMenuColorPicker`, `CMenuAction` | **1. Live Viewmodel & Crosshair Studio Panel**<br>**2. CS2 5-Column Grid Buy Menu + Right-Click Refund** | Native OS/SDL cursor (`EngFuncs::KEY_SetDest(KEY_MENU)`), semi-transparent overlay di atas viewport 3D live (tanpa freeze kamera saat preview!), otomatis kembalikan fokus ke `KEY_GAME` saat ditutup. |
| **Layer 2: Zero-Latency Gameplay HUD** | `client.dll` (`cs16-client/cl_dll/`) | `CHudBase` (`hud.h`), `CHudDeathNotice` (`death.cpp`), `FillRGBA`/`FillRGBABlend`/`DrawUtils::Draw2DQuad` (`draw_util.h`), `gEngfuncs.pTriAPI` | **3. Native CS2 Bottom Kill Cards (1K–5K ACE + Modifier Badges)**<br>**4. Native Post-Round Damage Strip** | Dirender langsung tiap frame HUD (`HUD_Redraw` -> `CHud::Draw`), 0 interaksi mouse, 0 server `PreThink`, 0 konflik channel HUD/DHUD AMXX. |

---

## 2. Server–Client Capability Handshake & Graceful Fallback

Agar plugin AMXX (`cs2_client_studio.amxx`, `cs2_modern_engine.amxx`, `csgo_competitive.amxx`) tidak pernah mengirim protokol baru ke klien lama atau merusak pengalaman klien GoldSrc/vanilla:

### 2.1 Client Version CVar (`cl_dll/hud.cpp`)
Daftarkan satu CVar read-only/archive ringan di `CHud::Init()`:
```cpp
// cs16-client/cl_dll/hud.cpp
CVAR_CREATE( "cl_cs2_ui_ver", "1", FCVAR_USERINFO );
CVAR_CREATE( "cl_cs2_killcards", "1", FCVAR_ARCHIVE );
CVAR_CREATE( "cl_cs2_dmgstrip", "1", FCVAR_ARCHIVE );
CVAR_CREATE( "cl_cs2_buymenu", "1", FCVAR_ARCHIVE );
```

### 2.2 Server Capability Probe (`cs2_studio/core/studio_client_probe.inc`)
`cs2_client_studio.amxx` sudah menjalankan single unified probe (`TASK_STU_PROBE = 40100 + id`) pada `client_putinserver()`. Perluas probe tersebut agar memeriksa `cl_cs2_ui_ver`:
* Tambahkan global flag `g_iClientUiVer[MAX_PLAYERS + 1]` (default `0`).
* Ekspos native baru pada library `"cs2_studio"` (atau `cs2_sdk/cs2_api.inc`):
  * `native cs2_get_client_ui_ver(id);` -> mengembalikan versi protokol native UI (`0` = vanilla/fallback, `>= 1` = Patched CS2 Client).
* **Aturan Fallback Otomatis:**
  * Jika `cs2_get_client_ui_ver(id) >= 1`:
    * `/studio`, `/vm`, `/crosshair` mengeksekusi `client_cmd(id, "ui_cs2_studio <tab>")` (membuka panel slider native `menu.dll`). Pemain tetap bisa membuka menu teks lama via `/studio text` atau `/vm text`.
    * `mod_kill_feedback.inl` otomatis menonaktifkan DHUD skull card (`amx_cs2_hud_cards`) hanya untuk player tersebut (audio chimes `1K-5K` & `headshot_dink.wav` tetap aktif!).
    * `comp_dmg_mvp_hud.inl` mengirim UserMessage kompak `CS2DmgStrip` alih-alih DHUD damage card yang penuh teks.
  * Jika `cs2_get_client_ui_ver(id) == 0`: **100% perilaku lama** (menu angka 1–9, chat damage summary, dan fallback GoldSrc) berjalan tanpa perubahan sedikit pun.

---

## 3. Feature 1 — Live Viewmodel & Crosshair Studio Panel (`menu.dll`)

### 3.1 Struktur File & Integrasi Build
* **File Baru:**
  * `cs16-client/3rdparty/mainui_cpp/menus/client/StudioPanel.cpp`
  * `cs16-client/3rdparty/mainui_cpp/menus/client/StudioPanel.h`
* **Registrasi Command (`mainui_cpp/BaseMenu.cpp`):**
  * Daftarkan command engine `ui_cs2_studio [crosshair|viewmodel|hud]` yang memanggil `UI_StudioPanel_Menu(int initialTab)`.
* **Referensi Pola Eksisting:**
  * `cs16-client/3rdparty/mainui_cpp/menus/Crosshair.cpp` (`CMenuXhair` dengan `CMenuSlider`, `CMenuCheckBox`, `CMenuSpinControl`, dan live preview box).
  * `cs16-client/3rdparty/mainui_cpp/menus/client/ClientWindow.cpp` (`CClientWindow` dengan `KEY_MENU` focus & `touch_setclientonly`).

### 3.2 Layout Docked Right-Side Overlay (Live 3D Preview)
Berbeda dengan `CClientWindow` standar yang menutup hampir seluruh layar (`1024x768`), `CClientStudioPanel` menggunakan **panel docked di sisi kanan (lebar 38% layar, latar RGBA `14, 18, 24, 215`)** sehingga **62% layar kiri tetap menampilkan senjata 3D dan crosshair aktual secara real-time** saat pemain menggeser slider!

| Tab Panel | Kontrol UI (`mainui_cpp/controls/`) | CVar Client yang Dimutasi Langsung (Real-Time) | Batas Clamp Wajib (Sinkron dengan Engine) |
| :--- | :--- | :--- | :--- |
| **Tab 1: Crosshair** | `CMenuCheckBox` (`xhair_enable`, `xhair_dot`, `xhair_t`, `xhair_dynamic_move`, `xhair_gap_useweaponvalue`, `xhair_additive`)<br>`CMenuSlider` (Size, Thickness, Gap, Outline/Pad, Dynamic Scale, Alpha)<br>`CMenuColorPicker` / RGB Sliders (`xhair_color`) | `xhair_enable`, `xhair_size`, `xhair_thick`, `xhair_gap`, `xhair_pad`, `xhair_dot`, `xhair_t`, `xhair_dynamic_move`, `xhair_dynamic_scale`, `xhair_gap_useweaponvalue`, `xhair_color`, `xhair_additive` (`cl_dll/ammo.cpp:305-317`) | `size [0..20]`, `thick [0..10]`, `gap [-15..15]`, `pad [0..3]`, `RGBA [0..255]` |
| **Tab 2: Viewmodel** | `CMenuSlider` (FOV, Offset X, Offset Y, Offset Z)<br>`CMenuSpinControl` (Preset: *Classic*, *Couch*, *Desktop*, *CS2 Pro*, *Left/Right Hand*)<br>`CMenuCheckBox` (`cl_righthand`, `cl_viewbob`, `cl_weaponlag`)<br>`CMenuAction` (**"Apply to Current Weapon Slot"** & **"Apply to All Weapons"**) | `viewmodel_fov`, `viewmodel_offset_x`, `viewmodel_offset_y`, `viewmodel_offset_z` (`cl_dll/view.cpp:1942-1945`), `cl_righthand`, `cl_viewbob`, `cl_weaponlag` | **HARD INVARIANT:** `viewmodel_fov [50.0..75.0]`, `offset_x/y/z [-3.5..3.5]` (wajib identik dengan `cl_dll/view.cpp:970-985` & `cs2_studio/core/studio_defs.inc:94-100`) |
| **Tab 3: Scope & HUD** | `CMenuSpinControl` (Sniper Style: *Classic / CS2 Frameless / Dot*; HUD Color Preset)<br>`CMenuSlider` (Sniper Width, Radar Scale)<br>`CMenuCheckBox` (Sniper Blur, Sniper Bob, Square Radar, Locked Radar, CS2 Kill Cards, CS2 Damage Strip) | `cl_crosshair_sniper_style`, `cl_crosshair_sniper_width`, `cl_crosshair_sniper_show_normal_inaccuracy`, `cl_bob_sniper`, `hud_radar_square`, `hud_radar_locked`, `hud_radar_scale`, `hud_color`, `cl_cs2_killcards`, `cl_cs2_dmgstrip` | `sniper_width [1..10]`, `radar_scale [0.5..2.0]` |

### 3.3 Persistence Bridge ke Dual-Vault Server (`cs2_client_studio.amxx`)
Karena `cs2_client_studio.amxx` menyimpan profil **per-weapon** (49 profil: 30 CSW + M4A4 + 18 custom knife) ke `data/vault/cs2_viewmodel.vault` dan profil HUD ke `data/vault/cs2_crosshair.vault` tanpa mengubah skema nVault:
* Saat pemain menekan tombol **"Save Profile"** di `CClientStudioPanel`:
  * **Tab Viewmodel:** Client mengeksekusi `cs2_studio_save_vm <fov> <x> <y> <z> <scope_all:0|1>` -> diterima oleh `cs2_studio/modules/mod_viewmodel_studio.inl`, divalidasi ulang terhadap `VM_FOV_MIN..MAX` & `VM_OFS_MIN..MAX`, lalu disimpan ke `g_hVmVault` (`cs2_viewmodel`) untuk weapon slot aktif (atau semua slot jika `scope_all=1`).
  * **Tab Crosshair / Scope & HUD:** Client mengeksekusi `cs2_studio_save_hud` -> memicu alur import eksisting (`TASK_HUD_SAVE = 44500 + id`) di `cs2_studio/modules/mod_crosshair_hud.inl` yang menarik nilai CVar aktual klien dan menulisnya ke `g_hHudVault` (`cs2_crosshair`).
* **Zero `PreThink` Guarantee:** Slider tidak pernah memanggil server saat sedang digeser (100% lokal di `client.dll`/`menu.dll`); komunikasi server terjadi **hanya 1 kali** saat tombol **"Save Profile"** ditekan.

---

## 4. Feature 2 — CS2 Single-Screen Grid Buy Menu + Right-Click Refund (`menu.dll`)

### 4.1 Masalah pada `BuyMenu.cpp` Saat Ini
`cs16-client/3rdparty/mainui_cpp/menus/client/BuyMenu.cpp` saat ini masih memakai struktur hierarki bertingkat klasik CS 1.6 (`CClientBuyMenu` -> sub-menu Pistols/Rifles/SMGs/Equipment, harus tekan `B -> 4 -> 3` atau klik 2 layar), belum menampilkan Molotov (`$500`), belum menampilkan label/harga varian CS2 (M4A4 vs M4A1-S, MAG-7, Negev, Bizon, CZ75, Tec-9, MP7, Sawed-Off), dan belum mendukung **Right-Click Refund**.

### 4.2 Arsitektur `CClientCS2BuyMenu` (`menus/client/BuyMenuCS2.cpp`)
Dibangun sebagai kelas saudara dari `CClientWindow` yang aktif saat `cl_cs2_buymenu 1` (tetap bisa fallback ke `BuyMenu.cpp` klasik dengan `cl_cs2_buymenu 0`):

```text
+---------------------------------------------------------------------------------------------------+
|  BUY MENU  |  CASH: $4,750   |   MIN NEXT ROUND: $1,900   |   [RMB = REFUND]   [ESC / B = CLOSE]  |
+-------------------+-------------------+-------------------+-------------------+-------------------+
| 1. EQUIPMENT      | 2. PISTOLS        | 3. MID-TIER       | 4. RIFLES         | 5. GRENADES       |
|-------------------|-------------------|-------------------|-------------------|-------------------|
| [1] Kevlar  $650  | [1] USP/Glock $200| [1] Nova/M3 $1200 | [1] FAMAS/Galil   | [1] Flash   $200  |
| [2] Vest+H $1000  | [2] P250/P228 $300| [2] MAG-7*  $1300 | [2] M4A4/AK-47    | [2] Smoke   $300  |
| [3] Defuser $400* | [3] Dualies   $300| [3] MP7*    $1500 | [3] M4A1-S* $2900 | [3] HE      $300  |
| [4] Zeus    (OFF) | [4] 5-7/Tec9* $500| [4] UMP-45  $1200 | [4] SSG08   $1700 | [4] Molotov $500  |
|                   | [5] Deagle    $700| [5] P90/Biz*$1400 | [5] AUG/SG  $3300 |                   |
|                   |                   | [6] Negev*  $1700 | [6] AWP     $4750 | [Slot Inventory]  |
+-------------------+-------------------+-------------------+-------------------+-------------------+
|  Keyboard Quick-Buy tetap aktif: B -> 4 -> 2 (AK/M4), B -> 8 -> 2 (Vest+Helm), B -> 8 -> 6 (Def)  |
+---------------------------------------------------------------------------------------------------+
```

### 4.3 Mekanisme Klik Kiri (Buy) vs Klik Kanan (Refund)
1. **Left-Click (`K_MOUSE1`):**
   * Mengirim command buy engine standar via `EngFuncs::ClientCmd(false, info.command)` — contoh: `"m4a1"`, `"ak47"`, `"awp"`, `"vesthelm"`, `"defuser"`, `"flash"`, `"sgren"`, `"hegren"`.
   * **Hard Invariant Terjaga:** Tidak pernah mendaftarkan `register_clcmd("m4a1")` di server; command `m4a1` tetap melalui jalur buy engine asli + interception harga/clip di `cs2_arsenal/modules/mod_weapon_variants.inl` dan `mod_m4_split.inl`.
   * Untuk Molotov (`$500`), mengirim `"say /molotov"` (atau command console `csgo_buy_molotov` yang ditambahkan di `csgo_molotov.sma` agar tidak membanjiri chat).
   * Untuk **M4A4 vs M4A1-S** pada sisi CT: tombol slot menampilkan varian yang aktif dari `CS2BuySync`, atau jika kedua slot diizinkan, mengirim `cs2_buy_m4a4` / `cs2_buy_m4a1s` (hanya mengubah preferensi slot lalu mengeksekusi `m4a1`).
2. **Right-Click (`K_MOUSE2` pada kartu item yang dimiliki & refundable):**
   * `CClientCS2BuyMenu::KeyDown(int key)` menangkap `key == K_MOUSE2`, mencari `CMenuBuyTile *tile = TileAtCursor()`.
   * Jika item memiliki flag `bOwned && bRefundable`, client mengirim `cs2_refund_weapon <csw_id>` ke `cs2_modern_engine.amxx` (`cs2_engine/modules/mod_modern_utility.inl`).
   * Server memvalidasi ulang `cs2_can_player_refund(id)` (masih di buyzone & dalam buytime & item dibeli ronde ini), mengembalikan 100% harga beli (termasuk harga varian via `cs2_get_variant_price()`), mencabut senjata, dan memutar suara refund ringan.
3. **Sync Metadata Varian & Status Kepemilikan (`CS2BuySync`):**
   * Agar label kartu di Grid Buy Menu otomatis menampilkan **"MAG-7 ($1300)"** (bukan "XM1014 $3000") atau **"M4A4 ($3100)"** sesuai `active_loadout.ini`, `cs2_arsenal.amxx` mengirim 1 string CVar/UserMessage `CS2BuyMeta` saat spawn / `cs2_reload_loadout` berisi bitmask 7 varian aktif + flag M4A4 (`bit0=M4A4, bit1=MAG7, bit2=NEGEV, bit3=BIZON, bit4=CZ75, bit5=TEC9, bit6=MP7, bit7=SAWEDOFF`). Total payload hanya **2 byte**!

---

## 5. Feature 3 — Native CS2 Bottom Kill Cards (`client.dll`)

### 5.1 Mengapa Pindah dari AMXX DHUD ke `client.dll`?
Pada `cs2_engine/modules/mod_kill_feedback.inl`, kartu kill AMXX (`amx_cs2_hud_cards`) sebelumnya terpaksa dimatikan (`0`) karena batas 8 channel DHUD GoldSrc membuat teks berkedip/tertimpa oleh header `csgo_competitive.amxx` dan tidak bisa merender ikon/geometri tajam.

### 5.2 Implementasi `CHudCS2KillCards` (`cl_dll/hud/cs2_killcards.cpp`)
* **Zero Extra Network Traffic:** `CHudDeathNotice::MsgFunc_DeathMsg()` di `cl_dll/death.cpp` **sudah menerima** byte ke-3 berisi bitmask `CS2_KILL_*` (`death.cpp:28-34`):
  * `CS2_KILL_HS (1<<0)`
  * `CS2_KILL_WALLBANG (1<<1)`
  * `CS2_KILL_NOSCOPE (1<<2)`
  * `CS2_KILL_SMOKE (1<<3)`
  * `CS2_KILL_BLIND (1<<4)`
  * `CS2_KILL_INAIR (1<<5)`
* Ketika `iKiller == gEngfuncs.GetLocalPlayer()->index && iKiller != iVictim` dan `cl_cs2_killcards->value > 0`:
  * `CHudDeathNotice` memanggil `gHUD.m_CS2KillCards.OnLocalPlayerKill( iFlags, szVictimName, szWeaponName )`.
* **State & Visual Design:**
  * Menyimpan hingga `MAX_ROUND_KILLS = 5` kartu (`struct KillCardEntry { int flags; float spawnTime; bool isAce; }`).
  * Reset otomatis pada `MsgFunc_ResetHUD` dan event `RoundTime` baru (`CHud::m_Timer`).
  * **Posisi Render:** Bawah-tengah layar (`y = ScreenHeight - ScaleY(88)`), tepat di atas bar HP/Ammo klasik (atau sedikit di atas tengah bawah jika HUD minimal aktif), berjajar simetris ke kiri-kanan seiring jumlah kill (`1` sampai `5` kartu).
  * **Geometri & Warna (CS2 Authentic):**
    * Kartu miring ringan (parallelogram menggunakan `gEngfuncs.pTriAPI->Begin(TRI_QUADS)` dengan warna dasar gelap `RGBA(16, 20, 28, 200)` dan garis aksen bawah:
      * Kill biasa: Putih-Biru (`#DCE6F2`)
      * Headshot (`CS2_KILL_HS`): Merah Crimson (`#E84141`) + ikon skull/headshot (`m_HUD_d_headshot`)
      * 5K ACE: Emas Prestige (`#F5C542`) + animasi glow pulse `0.6s`
    * **Animasi Pop-In:** `t = clamp((flTime - entry.spawnTime) / 0.18f, 0.0f, 1.0f)` -> `easeOutBack` scale dari `1.35x -> 1.0x` + slide-up `12px`.
    * **Modifier Strip:** Di bawah deretan kartu, tampilkan ikon mini untuk modifier spesial kill terakhir (`m_HUD_d_wallbang`, `m_HUD_d_noscope`, `m_HUD_d_smoke_kill`, `m_HUD_d_blind_kill`, `m_HUD_d_inairkill` — sprite index **sudah di-load** di `death.cpp:90-96`!).

---

## 6. Feature 4 — Native Post-Round Damage Strip (`client.dll`)

### 6.1 Wire Protocol: Custom UserMessage `CS2DmgStrip`
Didaftarkan di server (`cs2_competitive/modules/comp_dmg_mvp_hud.inl`) via `engfunc(EngFunc_RegUserMsg, "CS2DmgStrip", -1)` dan di client (`cl_dll/hud/cs2_damagestrip.cpp`) via `HOOK_MESSAGE(gHUD.m_CS2DamageStrip, CS2DmgStrip)`.

**Format Paket (Binary Compact — Maks 100 byte per player per akhir ronde):**
| Field | Tipe | Deskripsi |
| :--- | :--- | :--- |
| `u8 count` | `WRITE_BYTE` | Jumlah lawan yang terlibat interaksi damage (`0..5`, max 5 kartu) |
| `f32 duration` | `WRITE_BYTE` | Durasi tampil dalam per-sepuluh detik (misal `65` = `6.5s`, otomatis hilang saat freeze time berakhir) |
| *Loop `i = 0 .. count - 1`:* | | |
| `u8 enemySlot` | `WRITE_BYTE` | Entity index lawan (`1..32`) -> client mengambil nama & tim langsung dari `g_PlayerInfoList[enemySlot].name` (hemat bandwidth!) |
| `u16 dmgGiven` | `WRITE_SHORT` | Total damage diberikan ke lawan ronde ini |
| `u8 hitsGiven` | `WRITE_BYTE` | Total hit diberikan |
| `u16 dmgTaken` | `WRITE_SHORT` | Total damage diterima dari lawan ronde ini |
| `u8 hitsTaken` | `WRITE_BYTE` | Total hit diterima |
| `u8 flags` | `WRITE_BYTE` | `bit0 = bEnemyKilledByMe`, `bit1 = bEnemyAlive`, `bit2 = bEnemyKilledMe` |

### 6.2 Visual Layout (`cl_dll/hud/cs2_damagestrip.cpp`)
* Dirender di **bawah-tengah** (`y = ScreenHeight - ScaleY(155)`) selama `round_end -> next_freeze_end` (otomatis fade-out saat player mulai bergerak setelah freeze time selesai atau saat scoreboard `TAB` ditekan).
* Setiap kartu (lebar `124px` x tinggi `56px`, jarak `8px`):
  * **Baris 1 (Header):** Nama lawan (dipotong max 12 char) + titik status (Merah = Mati / Hijau = Hidup).
  * **Baris 2 (Given):** Panah kanan hijau `-> 84 (3)` (teks emas terang jika `bEnemyKilledByMe`).
  * **Baris 3 (Taken):** Panah kiri merah `<- 100 (4)` (border merah tipis jika `bEnemyKilledMe`).
* **Keuntungan vs AMXX DHUD:** Zero channel collision, tampilan identik CS2 Premier post-round card, dan pemain bisa tetap membaca chat/radar tanpa terhalang blok teks besar.

---

## 7. Build, Packaging & Zero-Regression Safeguards

### 7.1 Perubahan File di `cs16-client`
* `cs16-client/cl_dll/CMakeLists.txt`: **Otomatis mendeteksi** file baru di `cl_dll/hud/*.cpp` (`file(GLOB CS_HUD_SRC "hud/*.cpp")` di baris 10) — cukup tambah `hud/cs2_killcards.cpp` & `hud/cs2_damagestrip.cpp` dan deklarasikan member di `cl_dll/hud.h` + `Init()` di `cl_dll/hud.cpp`.
* `cs16-client/3rdparty/mainui_cpp/CMakeLists.txt`: Tambahkan `menus/client/StudioPanel.cpp` dan `menus/client/BuyMenuCS2.cpp`.

### 7.2 Backup & Deploy Gate (Wajib)
Sebelum menimpa binary aktif:
1. Salin `cs16-game/cstrike/cl_dlls/client.dll` dan `cs16-game/menu.dll` ke `D:\Counter-Strike 1.6\_archive_amxx\client_binaries_backup\<YYYYMMDD>\`.
2. Build menggunakan preset CMake yang ada di `cs16-client/CMakePresets.json`.
3. Copy hasil build `client.dll` -> `cs16-game/cstrike/cl_dlls/client.dll` dan `menu.dll` -> `cs16-game/menu.dll`.
4. Jalankan `build_all.bat` di `cs16-game/cstrike/addons/amxmodx/scripting/` untuk mengompilasi perubahan handshake pada `cs2_client_studio.amxx`, `cs2_modern_engine.amxx`, `cs2_arsenal.amxx`, dan `csgo_competitive.amxx`.

---

## 8. Acceptance Criteria & Verification Plan

* [ ] **Live Studio Panel:** Mengetik `/studio` atau `ui_cs2_studio` membuka panel docked kanan dengan cursor mouse aktif; menggeser `viewmodel_fov` (`50..75`) dan `viewmodel_offset_x/y/z` (`-3.5..3.5`) langsung memindahkan model senjata secara real-time di 62% viewport kiri; klik **Save** menulis nilai ke `cs2_viewmodel.vault` & `cs2_crosshair.vault` yang persisten setelah restart map.
* [ ] **CS2 Grid Buy Menu + Refund:** Menekan `B` menampilkan 5 kolom tunggal lengkap dengan harga varian dari `active_loadout.ini` dan Molotov `$500`; klik kiri membeli item tanpa membajak `B-4-3`; klik kanan mengembalikan uang 100% melalui `cs2_modern_engine.amxx`.
* [ ] **Native Kill Cards:** Membunuh 1–5 musuh dalam satu ronde memunculkan 1–5 kartu CS2 crisp di bawah-tengah dengan ikon HS/Wallbang/Noscope/Smoke/Blind/InAir sesuai `death.cpp:28-34`, reset bersih tiap ronde baru, dan zero DHUD flicker.
* [ ] **Post-Round Damage Strip:** Di akhir ronde Competitive, strip kartu damage muncul tepat waktu dan hilang saat freeze time berakhir atau saat TAB ditekan.
* [ ] **Fallback Compatibility:** Dengan mensimulasikan `cl_cs2_ui_ver 0`, seluruh fitur `/studio`, `/vm`, `/crosshair`, buy menu klasik, dan laporan damage chat bekerja 100% tanpa error di `error_*.log`.
