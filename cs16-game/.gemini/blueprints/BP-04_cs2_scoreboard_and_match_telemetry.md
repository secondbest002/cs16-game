# BP-04: CS2 Modern Scoreboard & Match Telemetry Bridge

* **Author:** Antigravity
* **Status:** Draft
* **Date:** 2026-11-27
* **Series:** CS2 Native HUD Overhaul (`BP-04`)
* **Target Repos:** `cs16-client/cl_dll/` (C++ Client DLL) & `cs16-game/cstrike/addons/amxmodx/scripting/` (AMXX 1.10)

--------------------------------------------------------------------------------

## Overview

Blueprint ini merancang perombakan penuh *scoreboard* (`TAB`) di `cs16-client/cl_dll/hud/scoreboard.cpp` ke gaya **Counter-Strike 2** yang selaras dengan elemen HUD CS2 yang sudah terpasang (`death.cpp` Killfeed & `radar.cpp` Square Radar), sekaligus membangun jembatan telemetri biner yang aman dari `csgo_competitive.amxx` ke `client.dll` untuk metrik kompetitif (**Assists**, **MVP Stars**, **Total Damage**, dan **Round History Bar**).

---

## The Problem

Hasil inspeksi mendalam pada `cs16-client/cl_dll/hud/scoreboard.cpp`, `hud.h`, `spectator_gui.cpp`, dan `cs2_competitive/` menemukan keterbatasan visual dan bug laten berikut:

1. **Tampilan Klasik 1999 & Konflik Ruang Visual**:
   * `scoreboard.cpp` merender satu kotak statis `DrawUtils::DrawRectangle` dengan header oranye `(255, 140, 0)`, `ROW_GAP = 15`, dan tinggi tetap `ScreenHeight - 180` yang meninggalkan ruang kosong besar pada match 5v5, namun terpotong pada server 32 pemain di resolusi `1280×720`.
   * Belum memperhitungkan bar sinematik `CHudSpectatorGui` (`~55px` atas/bawah saat `g_iUser1 != 0`).
2. **Destructive Sorting & Bug `ownteam`**:
   * `DrawPlayers()` (baris `606`) mengosongkan `pl_info->name = NULL` saat menggambar agar pemain tidak terduplikasi, sehingga tidak aman untuk render multi-pass (background row, zebra striping, local highlight, lalu teks/ikon).
   * `DrawTeams()` (baris `368–371`) menimpa `g_TeamInfo[j].ownteam = FALSE` pada setiap iterasi pemain non-lokal.
3. **Bug Laten 1-Based Indexing di `MsgFunc_TeamScore`**:
   * `scoreboard.cpp:757–767` mengiterasi `for ( i = 0; i < m_iNumTeams; i++ )` sementara `g_TeamInfo` berindeks `1..m_iNumTeams`, menyebabkan tim kedua tidak pernah ter-update dan pesan tak cocok menimpa slot terakhir.
   * Selain itu, `sv_restart 1` pada `StartLiveMatch()`, `TriggerHalftime()`, dan `TriggerOvertime()` (`comp_match_flow.inl:63, 148, 199`) me-reset `TeamScore` bawaan engine menjadi `0 : 0`.
4. **Data Kompetitif CS2 Belum Tersinkronisasi ke Client**:
   * `g_DamageDealt[33][33]` di `comp_economy_round.inl:647` di-reset setiap ronde baru tanpa akumulator match, `AnnounceMVP()` (`comp_dmg_mvp_hud.inl:104`) tidak menyimpan jumlah bintang MVP per pemain, Assist (`>= 40` damage) belum dilacak, dan riwayat kemenangan tiap ronde belum direkam maupun dikirim ke klien.

---

## Goals

* **Fase 1 (Pure `client.dll` — Kompatibel Semua Mode):**
  * Menghadirkan layout CS2 modern yang dikontrol CVar `hud_scoreboard_cs2` (default `1`), mempertahankan fallback klasik saat `hud_scoreboard_cs2 0` atau `m_bForceDraw` (`showscoreboard2` touch menu).
  * Mengganti *destructive sorting* dengan array indeks tersortir non-destruktif (`frags DESC`, `deaths ASC`, `clientIdx ASC`), memperbaiki bug `MsgFunc_TeamScore` & `ownteam`, serta melindungi margin terhadap expanded radar (`0.185f * ScreenWidth`) dan `CHudSpectatorGui`.
  * Merender ikon status `16x16` (`DEAD`, `C4`, `DEFUSE`, `VIP`) secara prosedural via `gRenderAPI.GL_CreateTexture` dengan scaling `gHUD.m_flScale` dan cleanup penuh di `Shutdown()`.
* **Fase 2 (Telemetry Bridge AMXX → `client.dll` — Khusus Competitive):**
  * Melacak **Assists (`>= 40` dmg)**, **MVP Stars (`★`)**, **Total Match Damage (`DMG`)**, dan **Round History** (`ELIM`, `BOMB`, `DEFUSE`, `TIME`) lengkap dengan *Halftime Flip* di `csgo_competitive`.
  * Mengirim data melalui 2 custom UserMsg terpisah (**`CS2PlayerStats`** & **`CS2RoundHist`**) agar **0 konflik** dengan `ScoreInfo` standar ReGameDLL maupun `csgo_molotov.sma:618`.

---

## Technical Architecture & Design

### 1. Geometri Piksel & Adaptive Density (`scoreboard.cpp`)

* **Horizontal Safe-Zone (Anti-Tabrakan Expanded Radar `radar.cpp:113`):**
  * `ScreenWidth >= 800`: `xstart = max(185, (int)(0.185f * ScreenWidth))` (untuk `ScreenWidth >= 960`), `panelW = ScreenWidth - 2 * xstart`.
  * `ScreenWidth < 800`: `xstart = (int)(0.08f * ScreenWidth)`, `panelW = ScreenWidth - 2 * xstart`.
  * Clamp ke `GetSafeAreaRect(safe)`.
* **Vertikal Dinamis & Kompresi 32-Pemain:**
  * `topMargin = (g_iUser1 != 0) ? 68 : 48` (menghindari bar `CHudSpectatorGui`).
  * `availableH = ScreenHeight - 2 * topMargin`.
  * Ukuran default: `headerH = 42`, `teamHdrH = 24`, `rowH = 20`, `histH = (m_iRoundHistMaxHalf > 0 && gHUD.m_Teamplay) ? 34 : 6`, `specFooterH = (specCount > 0) ? 20 : 0`.
  * Jika `totalH > availableH`, kompres `rowH` secara proporsional hingga minimum `15px` dan `teamHdrH` ke `20px` agar seluruh 32 pemain (Deathmatch) tetap muat pada `1280×720`.
  * Posisi vertikal: `ystart = max(topMargin, (ScreenHeight - totalH) / 2)`.

### 2. Anchor Kolom Horizontal (Right-to-Left dari `xend - 12`)

| Kolom | Lebar (`px`) | Alignment | Kondisi Tampil |
| :--- | :---: | :---: | :--- |
| `PING` | `46` | Right | Selalu tampil (`ms` atau `"BOT"` jika `*bot > 0`). |
| `DMG` | `46` | Right | Hanya saat `m_bHasCS2Stats == true` (Competitive). |
| `MVP (★)` | `34` | Center | Hanya saat `m_bHasCS2Stats == true`. |
| `DEATHS` | `38` | Right | Disembunyikan untuk Spectator. |
| `ASSISTS` | `32` | Right | Hanya saat `m_bHasCS2Stats == true`. |
| `KILLS` | `38` | Right | Disembunyikan untuk Spectator. |
| `MONEY` | `64` | Right | Jika `sb_account >= 0 && gHUD.m_pShowMoney->value`. |
| `HP` | `38` | Right | Jika `!dead && sb_health >= 0 && gHUD.m_pShowHealth->value`. |
| `ATTRIB` | `24` | Center | Ikon `16x16` (`DEAD`, `C4`, `VIP`, `DEFUSE`) atau `cscl_ver` jika `cl_showplayerversion 1`. |
| `NAME` | Sisa lebar | Left | `xstart + 12` s.d. `colAttribLeft - 8` (potong aman UTF-8 via `DrawUtils::DrawHudString`). |

### 3. Palet Warna Token `RGBA` (Selaras `death.cpp` & `radar.cpp`)

| Token | `RGBA` | Penggunaan |
| :--- | :--- | :--- |
| `CS2_BG_MAIN` | `(13, 16, 22, 225)` | Latar utama panel (`FillRGBABlend`). |
| `CS2_BG_HEADER` | `(20, 25, 34, 240)` | Latar Top Match Header (`42px`). |
| `CS2_BORDER_OUTER` | `(140, 175, 210, 180)` | Accent border atas `2px` & frame luar `1px`. |
| `CS2_CT_PRIMARY` / `BANNER` | `(93, 137, 196, 255)` / `(32, 52, 82, 165)` | Header, skor, strip `3px` & banner tim CT. |
| `CS2_T_PRIMARY` / `BANNER` | `(226, 168, 62, 255)` / `(84, 60, 24, 165)` | Header, skor, strip `3px` & banner tim T. |
| `CS2_ROW_EVEN` / `ODD` | `(22, 27, 36, 150)` / `(17, 21, 29, 150)` | Zebra striping baris pemain. |
| `CS2_ROW_LOCAL` | `(255, 255, 255, 36)` | Highlight baris pemain lokal (`thisplayer`). |
| `CS2_TEXT_ALIVE` / `DEAD` | `(236, 240, 245, 255)` / `(130, 138, 150, 255)` | Teks pemain hidup vs mati (dimmed ~45%). |
| `CS2_MONEY_GREEN` | `(102, 204, 128, 255)` | Uang pemain hidup (`(72, 130, 90)` saat mati). |
| `CS2_PING_GOOD / MID / BAD` | `(110, 215, 130)` / `(235, 195, 75)` / `(235, 85, 85)` | `< 50ms`, `50–100ms`, `> 100ms`. |

### 4. Ikon Prosedural `gRenderAPI` & Aturan `m_flScale`

* Buat tekstur RGBA prosedural saat `VidInit()` (setelah membebaskan handle lama) dan bebaskan seluruhnya pada `CHudScoreboard::Shutdown()`:
  * Status (`16x16`): `sb_cs2_dead`, `sb_cs2_c4`, `sb_cs2_defuse`, `sb_cs2_vip`, `sb_cs2_mvp`.
  * Round History (`14x14`): `sb_cs2_rh_elim`, `sb_cs2_rh_bomb`, `sb_cs2_rh_defuse`, `sb_cs2_rh_time`.
* **Wajib `m_flScale`:** Di dalam `DrawCS2Icon(hTex, x, y, size, r, g, b, a)`, seluruh koordinat `DrawUtils::Draw2DQuad` wajib dikalikan `gHUD.m_flScale` (mengikuti `spectator_gui.cpp:159–162`). Jika `hTex == 0` (software renderer), fallback otomatis ke teks singkat (`DEAD`, `C4`, `KIT`, `VIP`, `*`).

### 5. Protokol Jaringan Biner (AMXX → `client.dll`)

Daftarkan pada `plugin_precache()` di `csgo_competitive.sma` menggunakan `engfunc(EngFunc_RegUserMsg, ..., -1)`:

1. **`CS2PlayerStats`** (Sinkronisasi Assists, MVP, Damage):
   * Payload: `byte count` diikuti `count` kali `{ byte clientIdx, short assists, short mvps, short totalDamage }`.
   * Jika `count == 0` (dikirim saat `amx_csgo_mode 0` / Deathmatch), client menyetel `m_bHasCS2Stats = false` dan menyembunyikan kolom `A`, `★`, `DMG`.
2. **`CS2RoundHist`** (Sinkronisasi Skor Resmi Match + Round History Bar):
   * Payload: `byte maxRoundsHalf` (`0` = sembunyikan bar), `byte scoreCT`, `byte scoreT`, `byte count` (`0..36`), diikuti `count` kali `byte packed` dengan `packed = (winner << 4) | (reason & 0x0F)` (`winner`: `1=CT, 2=T`; `reason`: `1=ELIM, 2=BOMB, 3=DEFUSE, 4=TIME`).
   * **Urutan Wajib di `HandleRoundWin()`:** Rekam histori **tepat setelah** `g_ScoreCT++` / `g_ScoreT++` (sebelum early-return `EndMatch`, `TriggerHalftime`, `TriggerOvertime`).
   * **Halftime Flip & Post-Restart Resync:** Pada `TriggerHalftime()` dan `TriggerOTHalftime()`, balik `g_RoundHistWinner[0..count-1]` (`1 <-> 2`) bersamaan dengan swap `g_ScoreCT <-> g_ScoreT`, lalu broadcast `CS2RoundHist` lagi di dalam `Task_RestartSecondHalf()` / `Task_RestartOTSecondHalf()` setelah `sv_restart 1`.

---

## Detailed Implementation Plan

### Target Files to Modify

1. **`cs16-client/cl_dll/hud.h`** (**Modify**)
   * Tambahkan `short sb_assists`, `short sb_mvps`, `int sb_damage` pada `struct extra_player_info_t`.
   * Tambahkan deklarasi `Shutdown()`, `CHudMsgFunc(CS2PlayerStats)`, `CHudMsgFunc(CS2RoundHist)`, struct `RoundHistEntry`, CVar `hud_scoreboard_cs2`, handle ikon prosedural, serta helper render CS2 pada `class CHudScoreboard`.
2. **`cs16-client/cl_dll/hud/scoreboard.cpp`** (**Modify**)
   * Perbaiki loop 1-based di `MsgFunc_TeamScore()`.
   * Implementasikan `InitCS2Icons()`, `Shutdown()`, `MsgFunc_CS2PlayerStats()`, `MsgFunc_CS2RoundHist()`, `BuildSortedTeamPlayers()` (non-destruktif), dan seluruh pipeline `DrawCS2Scoreboard()` (Header, Team Section, Player Row, Round History Bar, Spectator Footer).
3. **`cs16-game/cstrike/addons/amxmodx/scripting/cs2_competitive/core/comp_defs.inc`** (**Modify**)
   * Tambahkan enum `WIN_REASON_*`, `MAX_ROUND_HISTORY (36)`, `g_PlayerAssists[33]`, `g_PlayerMVPs[33]`, `g_TotalMatchDamage[33]`, `g_PendingWinReason`, `g_RoundHistCount`, `g_RoundHistWinner[36]`, `g_RoundHistReason[36]`, `g_MsgPlayerStats`, dan `g_MsgRoundHist`.
4. **`cs16-game/cstrike/addons/amxmodx/scripting/csgo_competitive.sma`** (**Modify**)
   * Registrasi `CS2PlayerStats` & `CS2RoundHist` di `plugin_precache()`.
   * Daftarkan `TextMsg` hook untuk `#Target_Saved`, `#Hostages_Not_Rescued`, `#All_Hostages_Rescued`.
   * Reset & sync stats/history pada `client_putinserver()`, `client_disconnected()`, dan `SyncCompetitiveLifecycle(false)`.
5. **`cs16-game/cstrike/addons/amxmodx/scripting/cs2_competitive/modules/comp_economy_round.inl`, `comp_dmg_mvp_hud.inl`, & `comp_match_flow.inl`** (**Modify**)
   * Akumulasi `g_TotalMatchDamage` di `OnTakeDamagePost()`, kalkulasi Assist (`>= 40` dmg) di `OnPlayerKilledPost()`, increment `g_PlayerMVPs[best_player]++` di `AnnounceMVP()`, pencatatan & flip `g_RoundHist*` di `HandleRoundWin()` / `TriggerHalftime()` / `TriggerOTHalftime()`, serta helper `BroadcastCS2PlayerStats()` & `BroadcastCS2RoundHistory()`.

### Build & Verification Plan

1. **Client DLL Build (LLVM-MinGW `i686`):**
   * Refresh stale CMake cache (`cs16-client/build/CMakeCache.txt` yang masih menunjuk `D:/Counter-Strike 1.6/cs16-client`), compile `cmake --build cs16-client/build --target client -j`, dan salin `cs16-client\build\cl_dll\client.dll` → `cs16-game\cstrike\cl_dlls\client.dll`.
2. **AMXX Build & 4-Gate Integrity Check:**
   * Jalankan `cs16-game\cstrike\addons\amxmodx\scripting\build_all.bat` (`7/7 exit 0`) dan `powershell -ExecutionPolicy Bypass -File cs16-game\tools\verify_workspace.ps1` (`PASS 4/4 Gate`).
3. **In-Game Runtime Verification:**
   * **Competitive MR12:** Verifikasi panel CS2 tidak menimpa expanded radar maupun spectator bar, skor `CT : T` tetap utuh setelah `sv_restart 1` Halftime, kolom `A`/`★`/`DMG` terisi, dan Round History Bar menampilkan ikon `ELIM`/`BOMB`/`DEFUSE`/`TIME` serta terbalik benar saat Halftime.
   * **Deathmatch (`redm_active 1`):** Verifikasi kolom `A`/`★`/`DMG` & Round History Bar otomatis tersembunyi, dan seluruh 24–32 pemain muat rapi lewat adaptive row density.
   * **Toggle Fallback:** Verifikasi `hud_scoreboard_cs2 0` kembali bersih ke scoreboard klasik.

--------------------------------------------------------------------------------

*Drafted with EGM*
