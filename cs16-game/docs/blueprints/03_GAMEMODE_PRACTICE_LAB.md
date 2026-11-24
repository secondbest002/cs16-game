# Blueprint 03 — Gamemode Slot #3: Utility & Aim Practice Lab

**Document ID:** `BP-03-GAMEMODE-PRACTICE-LAB`  
**Target Workspace:** `D:\Counter-Strike 1.6\cs16-game`  
**Architecture Baseline:** `cs16-game/ARCHITECTURE.md` (Option 1 + Option 2-Lite — 7 Active Custom Plugins + `include/cs2_sdk/` + Task ID Partition `60000 – 60999`)  
**New Artifacts:**
* Launcher: `cs16-game/!Start_Practice_Xash3D.bat`
* Plugin Profile: `cs16-game/cstrike/addons/amxmodx/configs/plugins-practice.ini`
* Server Rules: `cs16-game/cstrike/practice.cfg`
* Plugin Source: `cs16-game/cstrike/addons/amxmodx/scripting/gm_practice.sma` + `gm_practice/{core,modules}/`  
**Primary Objective:** Menyediakan lingkungan latihan *utility lineup*, *wallbang*, *pop-flash*, dan *spray/aim* standar CS2 PracServer yang terintegrasi penuh dengan fisika lemparan 3-arah (`cs2_engine/modules/mod_modern_utility.inl`), Molotov standalone (`csgo_molotov.amxx` via `include/cs2_sdk/cs2_api.inc`), dan YaPB bot puppets—dengan **0% regresi** ke mode Competitive dan Deathmatch.

---

## 1. Gamemode Isolation & Launcher Architecture

Mengikuti pola pemisahan atomik yang sudah terbukti pada `!Start_CS16_Xash3D.bat` dan `!Start_Deathmatch_Xash3D.bat`:

### 1.1 `!Start_Practice_Xash3D.bat`
```bat
@echo off
setlocal enabledelayedexpansion
title Counter-Strike 1.6 (Pure Xash3D FWGS) - CS2 Utility & Aim Practice Lab
cd /d "%~dp0"

:: 1. Apply Practice Profile (Atomic switch)
copy /y "cstrike\addons\amxmodx\configs\plugins-practice.ini" "cstrike\addons\amxmodx\configs\plugins.ini" >nul 2>&1
(echo exec practice.cfg)> "cstrike\mode.cfg"

:: 2. Launch Pure Xash3D Engine Standalone
start "" "xash3d.exe" -game cstrike -dll addons/metamod/dlls/metamod.dll -fullscreen -high -condebug +log on +mp_logfile 1 +exec practice.cfg
exit
```

### 1.2 `plugins-practice.ini` (Mutual Exclusion Rules — Sinkron `ARCHITECTURE.md`)
* **Aktif:** Base AMXX + Pilar CS2 Aktif (`cs2_arsenal.amxx`, `cs2_modern_engine.amxx`, `cs2_client_studio.amxx`) + Standalone (`csgo_molotov.amxx`) + **`gm_practice.amxx debug`**.
* **Dormant (Commented Out):**
  * `;csgo_competitive.amxx`
  * `;redm_listen_engine.amxx`
  * `;bot_savage_ai.amxx` *(WAJIB nonaktif di Practice agar bot puppet tidak melakukan counter-strafe, anti-flash turnaround, atau scavenging saat digunakan untuk uji flash/lineup!)*
* **Build Gate (`scripting/build_all.bat`):** Tambahkan `gm_practice` ke variabel `PLUGINS` saat implementasi dimulai sehingga `build_all.bat` memvalidasi 8/8 target aktif langsung ke `..\plugins\`.

### 1.3 `cstrike/practice.cfg`
```cfg
amx_csgo_mode 0
redm_active 0
amx_prac_active 1

// Infinite Practice Round Rules
mp_roundtime 60
mp_timelimit 0
mp_freezetime 0
mp_buytime 999
mp_buy_anywhere 1
mp_startmoney 16000
mp_round_infinite "abcdef"
mp_autoteambalance 0
mp_limitteams 0
mp_item_staytime 30
sv_cheats 1

// YaPB Puppet Mode (Zero autonomous bots on boot; spawned on demand via .place)
yb_quota 0
yb_freeze_bots 1
yb_botbuy 0
yb_radio_mode 0
yb_chat 0
yb_avoid_grenades 0
```

---

## 2. Modular Structure (`gm_practice.sma` + `gm_practice/`)

```text
scripting/
├── gm_practice.sma                     # Master Orchestrator (~200 LOC, switchboard #define)
└── gm_practice/
    ├── core/
    │   ├── prac_defs.inc               # CVars, constants, trajectory structs, puppet slots
    │   └── prac_taskids.inc            # Block 60000 - 60999 (Zero collision)
    └── modules/
        ├── prac_infinite_env.inl       # Infinite ammo/utility, $16k top-up, .noclip, .god, instant respawn
        ├── prac_nade_trajectory.inl    # Real-time parabola preview + actual flight trail + .rethrow + .clearsmoke
        ├── prac_flash_tester.inl       # Blind duration/alpha telemetry on puppets & self (.flash)
        ├── prac_bullet_impacts.inl     # Server-side 3D impact cubes & wallbang penetration readout (.impacts)
        └── prac_bot_puppets.inl        # .place, .crouch, .mimic, .bot_boost, .clearbots (YaPB safe)
```

### Global Task ID Partition (`60000 – 60999`)
* `TASK_PRAC_PREVIEW = 60100 (+id)` — Repeating `0.06s` (~16 Hz) grenade preview ray-march saat pin ditarik / senjata granat dipegang.
* `TASK_PRAC_NADE_RESTOCK = 60200 (+id)` — Delay `0.35s` isi ulang granat yang baru dilempar.
* `TASK_PRAC_RESPAWN = 60300 (+id)` — Instant respawn `0.5s` jika player mati kena Molotov/HE/fall damage.
* `TASK_PRAC_MIMIC_TICK = 60400 (+id)` — Sinkronisasi gerakan bot saat `.mimic` aktif.

### Cross-Pillar SDK Contract (`include/cs2_sdk/`)
`gm_practice.sma` wajib meng-include `<cs2_sdk/cs2_offsets>`, `<cs2_sdk/cs2_state_cvars>`, dan `<cs2_sdk/cs2_api>`, serta memanggil `set_native_filter("cs2_sdk_native_filter")` di `plugin_natives()` (**0 `callfunc_begin`**):
* **Molotov Detection (`csgo_molotov.amxx`):** Gunakan guard `cs2_api_has_molotov() && csgo_get_user_molotov(id)` agar tetap bebas runtime error meski `csgo_molotov.amxx` dinonaktifkan.
* **Weapon Readout (`cs2_arsenal.amxx`):** Pada `.impacts` wallbang readout, panggil `cs2_api_is_m4a4(attacker)` dan `cs2_api_has_variants()` + `cs2_get_user_weapon_variant()` untuk menampilkan nama `M4A4`/`M4A1-S` dan varian CS2 (`MAG-7`, `MP7`, dll.) secara akurat.
* **Gamemode State Signal (`include/cs2_sdk/cs2_state_cvars.inc`):** Opsional tambahkan helper `cs2_state_is_practice()` membaca `amx_prac_active` agar pilar lain dapat mendeteksi mode Practice secara O(1).

---

## 3. Physics-Matched Grenade Trajectory Preview (`prac_nade_trajectory.inl`)

Ini komponen teknis terpenting agar preview akurat 100% dengan engine Xash3D/GoldSrc dan mod CS2 kamu.

### 3.1 Exact Initial Velocity Formula (Synced with `mod_modern_utility.inl:1420-1437`)
1. **Base Throw Velocity (GoldSrc `CGrenade::ShootTimed`):**
   * Ambil `v_angle` + `punchangle`, hitung sudut throw `angThrow[0] = (v_angle[0] < 0) ? -10.0 + v_angle[0] * ((90.0 - 10.0) / 90.0) : -10.0 + v_angle[0] * ((90.0 + 10.0) / 90.0)`.
   * `flVel = min((90.0 - angThrow[0]) * 6.0, 750.0)`.
   * `vecStart = eyePos + forward * 16.0`.
   * `vecThrow = forward * flVel + playerVelocity` *(termasuk jump-throw velocity!)*.
2. **3-Way CS2 Modifier Injection (WAJIB persis dengan `mod_modern_utility.inl`):**
   * Deteksi tombol yang ditahan (`pev_button`):
     * `IN_ATTACK` only (**Full 100%**): `vecThrow` tidak diubah.
     * `IN_ATTACK | IN_ATTACK2` (**Medium 65%**): `vecThrow[0] *= 0.65; vecThrow[1] *= 0.65; vecThrow[2] = (vecThrow[2] * 0.65) + 75.0;`
     * `IN_ATTACK2` only (**Short Underhand 35%**): `vecThrow[0] *= 0.35; vecThrow[1] *= 0.35; vecThrow[2] = (vecThrow[2] * 0.35) + 125.0;`
3. **Iterative Ballistic Simulation (`dt = 0.05s`, max `60` steps = 3.0 detik):**
   * Setiap step:
     * `vecVel[2] -= (sv_gravity * 0.55) * dt` *(GoldSrc `MOVETYPE_BOUNCE` menggunakan gravity scale `0.55` untuk granat!)*.
     * `vecNext = vecPos + vecVel * dt`.
     * `engfunc(EngFunc_TraceLine, vecPos, vecNext, IGNORE_MONSTERS, id, tr)`.
     * Jika `flFraction < 1.0`: hitung refleksi pantulan `v' = v - 2*(v·n)*n`, kalikan koefisien elastisitas `0.45` (CS 1.6 grenade bounce elasticity), gambar marker titik pantul kuning (`TE_GLOWSPRITE` / small cross), lalu lanjutkan simulasi hingga kecepatan < 20 u/s atau `dt` total mencapai fuse time (`1.5s` HE/Flash, `2.0s` Smoke).
   * Render segmen garis menggunakan `TE_BEAMPOINTS` (hanya `MSG_ONE_UNRELIABLE` ke `id` agar 0 lag!) dengan warna per tipe granat:
     * Smoke: Putih/Abu-abu `(210, 220, 230)`
     * Flashbang: Kuning `(255, 230, 60)`
     * HE Grenade: Merah `(255, 70, 70)`
     * Molotov (cek via `cs2_api_has_molotov() && csgo_get_user_molotov(id)`): Oranye Api `(255, 130, 20)`

### 3.2 Actual Flight Trail & `.rethrow` System
* **Flight Trail:** Pada `FM_SetModel` (`w_hegrenade.mdl`, `w_flashbang.mdl`, `w_smokegrenade.mdl`, `w_molotov.mdl`), tempelkan `TE_BEAMFOLLOW` (durasi 12 detik) ke entity granat, dan cetak chat ringkas: `[Prac] Landed in 2.35s | Dist: 1140u | Bounces: 2`.
* **Snapshot & `.rethrow`:**
  * Simpan struct per pemain (`g_LastThrow[33]`): `{ origin[3], angles[3], velocity[3], cswWeapon, bool:wasMolotov, throwMode }`.
  * Ketika pemain mengetik `.rethrow` (atau bind `prac_rethrow`), plugin men-spawn granat persis dari posisi & vektor `g_LastThrow[id]` tanpa memindahkan pemain—sehingga pemain bisa berdiri di posisi musuh (misal di A Site) lalu memicu `.rethrow` untuk melihat apakah smoke menutup celah atau flash membutakan posisi tersebut!
* **`.clearsmoke` & `.clearfire`:** Menghapus instan semua entity `grenade` smoke aktif (termasuk `cs2_dropped_nade` di `mod_modern_utility.inl:323`) dan entity `"molotov"` / `"molotov_fire"` (`csgo_molotov.sma:363,371`) di map.

---

## 4. Flashbang Blindness Telemetry (`prac_flash_tester.inl`)

* Hook `register_message(get_user_msgid("ScreenFade"), "Prac_OnScreenFade")` untuk memantau setiap player & bot puppet yang terkena flash.
* Hitung durasi total blind (`Float:flDuration = float(duration) / 4096.0`, `Float:flHold = float(holdtime) / 4096.0`) dan `alpha` (`0–255`).
* Jika target adalah bot puppet yang ditaruh pemain (atau pemain sendiri saat `.rethrow`):
  * Cetak langsung ke chat pemain:
    * `[Flash] Puppet #1: FULL BLIND 3.42s (Alpha: 255/255, Hold: 1.85s)` -> Hijau jika full blind (`alpha >= 240`), Kuning jika half-blind, Merah jika `NOT BLIND (facing away)`.
  * Tampilkan ikon sementara di atas kepala bot puppet yang terkena blind.

---

## 5. Bullet Impacts & Wallbang Penetration Readout (`prac_bullet_impacts.inl`)

* Toggle via `.impacts` (default ON di Practice).
* Hook `Ham_TraceAttack` pada `worldspawn`, `func_wall`, `func_breakable`, dan `player`.
* Gambar kubus/crosshair 3D merah kecil (`TE_BEAMPOINTS` 3 sumbu X/Y/Z ±4 unit, durasi 4.0s) pada koordinat impact server (`TR_vecEndPos`).
* Jika mengenai bot puppet: cetak detail ke HUD & chat:
  * `[Impact] AK-47 -> HEAD | 108 dmg | Dist: 18.4m | Wallbang: YES (1 wall)`
  * Langsung memulihkan HP bot puppet ke 100 setelah 0.1s (kecuali jika `.godbot 0`) sehingga pemain bisa menguji damage wallbang berkali-kali pada 1 puppet.

---

## 6. YaPB-Safe Bot Puppet Controller (`prac_bot_puppets.inl`)

Karena YaPB berjalan sebagai Metamod DLL dengan sistem quota sendiri, kontrol puppet harus menghindari konflik dengan `yb_quota`:

| Command | Efek Teknis |
| :--- | :--- |
| `.place` / `.bot` | Menjalankan `server_cmd("yb add 100 0 %d", enemyTeam)` (jika puppet belum ada atau <5), menunggu spawn via `Ham_Spawn` post, lalu memindahkan bot tepat ke titik bidik crosshair pemain (`TR_vecEndPos` + Z offset `36.0`) menghadap ke arah pemain dengan `yb_freeze_bots 1` & `FL_FROZEN`. |
| `.crouch` | Toggle posisi jongkok permanen (`FL_DUCKING` + `pev_bInDuck` + adjust hull/origin) pada puppet terakhir yang ditaruh (ideal untuk tes headshot box / boost). |
| `.mimic` | Bot puppet terakhir menyalin input gerak/aim/crouch pemain secara real-time (mirror mode) untuk latihan jiggle peek & pre-fire. |
| `.boost` | Menaruh puppet tepat di kaki pemain (crouched) untuk latihan solo-boost ke posisi tinggi (misal Mirage window / Dust2 short). |
| `.clearbots` | Menjalankan `server_cmd("yb kickall")` dan mereset array slot puppet. |

---

## 7. Complete Chat & Console Command Reference

| Command (Chat / Console) | Fungsi |
| :--- | :--- |
| `.prac` / `/prac` | Buka menu bantuan & toggle cepat Practice Lab |
| `.noclip` / `prac_noclip` | Toggle `MOVETYPE_NOCLIP` terbang bebas (bind-able ke tombol misal `bind v prac_noclip`) |
| `.rethrow` / `prac_rethrow` | Lempar ulang granat terakhir (Smoke/Flash/HE/Molotov) dari koordinat & velocity asli |
| `.clearsmoke` / `.clear` | Bersihkan seluruh smoke, api molotov, dan decal aktif secara instan |
| `.preview` | Toggle garis parabola preview granat |
| `.impacts` | Toggle 3D server bullet impacts & wallbang readout |
| `.place` / `.crouch` / `.mimic` / `.boost` / `.clearbots` | Manajemen Bot Puppet |
| `.savepos <1-5>` & `.loadpos <1-5>` | Simpan & teleport ke 5 posisi lineup favorit (in-memory per map + opsional ekspor JSON `data/practice/<map>_lineups.json`) |

---

## 8. Acceptance Criteria & Verification Plan

* [ ] `scripting/build_all.bat` mengompilasi semua target aktif termasuk `gm_practice.sma` dengan `0` error dan `0` `.amxx` nyasar di `scripting/`.
* [ ] `!Start_Practice_Xash3D.bat` meluncurkan server dengan pilar aktif (`cs2_arsenal`, `cs2_modern_engine`, `cs2_client_studio`, `csgo_molotov`, `gm_practice`) tanpa `csgo_competitive`, `redm_listen_engine`, maupun `bot_savage_ai`, dan `error_*.log` mencatat `0` error.
* [ ] Preview parabola cocok dalam toleransi <24 unit dari titik jatuh asli untuk **ketiga mode lemparan** (`LMB 100%`, `LMB+RMB 65%`, `RMB 35%` — sinkron `mod_modern_utility.inl:1420-1435`) dan **jump-throw**.
* [ ] `.rethrow` mereplikasi Smoke, Flashbang, HE, dan Molotov secara identik sementara pemain berada di posisi target.
* [ ] `.place` + `.crouch` menempatkan bot YaPB diam tanpa bergerak sendiri, dan `.flash` melaporkan durasi blind akurat.
* [ ] **Cross-Pillar Isolation Test:** Jika `csgo_molotov.amxx` atau `cs2_arsenal.amxx` dimatikan sementara, `gm_practice.amxx` tetap berjalan dengan `0` runtime error berkat `cs2_sdk_native_filter` dan guard `cs2_api_has_*()`.
* [ ] Meluncurkan kembali `!Start_CS16_Xash3D.bat` atau `!Start_Deathmatch_Xash3D.bat` langsung mengembalikan profil Competitive/Deathmatch secara bersih (0 CVar practice bocor).
