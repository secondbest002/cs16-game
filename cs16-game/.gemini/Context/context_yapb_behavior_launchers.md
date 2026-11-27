# TEKNIKAL KONTEKS: PERILAKU YaPB BOT, KONSOLIDASI LAUNCHER & ISOLASI GAME MODE (DEATHMATCH VS COMPETITIVE)
## XASH3D FWGS ENGINE & AMX MOD X 1.10

Dokumen ini disusun sebagai baseline arsitektur game dan teknikal konteks untuk **Gemini 4 Argon**. Dokumen ini merinci akar masalah kegagalan isolasi gamemode pada launcher Xash3D, redundansi file launcher, serta anomali perilaku YaPB bot yang tidak membeli senjata atau tidak menjalankan sistem role di mode Warmup dan Deathmatch (DM).

---

## 1. RINGKASAN MASALAH & OBSERVASI PENGGUNA

Pengguna mengidentifikasi 3 masalah kritis pada instalasi Counter-Strike 1.6 di atas engine **Xash3D FWGS (OpenGL / SDL2)**:

### A. Masalah 1: Launcher DM Ter-intervensi Sistem Warmup & Competitive
- **Gejala**: Ketika menjalankan launcher Deathmatch (`!Start_Deathmatch_Xash3D.bat`), pemain spawn secara acak (sesuai sistem JSON DM), namun sistem Warmup dari mode Competitive tetap aktif di latar belakang. Pemain dapat mengetik `.ready` dan memulai Competitive Match MR12 di dalam mode DM.
- **Harapan Pengguna**: Launcher DM harus murni dan eksklusif menjalankan Deathmatch (`redm_listen_engine.amxx`). Gamemode Deathmatch tidak boleh ter-intervensi oleh sistem Warmup, `.ready`, knife round, atau match competitive.

### B. Masalah 2: Redundansi Launcher Xash3D vs Competitive
- **Gejala**: Terdapat tiga launcher batch di direktori root:
  1. `!Start_Deathmatch_Xash3D.bat`
  2. `!Start_CS16_Xash3D.bat`
  3. `!Start_Competitive_Xash3D.bat`
  Isi `!Start_CS16_Xash3D.bat` dan `!Start_Competitive_Xash3D.bat` adalah **100% identik** (keduanya menyalin `plugins-competitive.ini` ke `plugins.ini` dan mengeksekusi `competitive.cfg`).
- **Harapan Pengguna**: Hapus `!Start_Competitive_Xash3D.bat`. Pertahankan `!Start_CS16_Xash3D.bat` sebagai launcher utama Counter-Strike 1.6 standar (yang mencakup Warmup DM -> `.ready` -> Knife Round -> Competitive MR12), dan `!Start_Deathmatch_Xash3D.bat` untuk Deathmatch murni. Total hanya ada 2 launcher bersih.

### C. Masalah 3: YaPB Bot Abnormal di Xash3D FWGS (Tidak Membeli Senjata / Tidak Sesuai Role)
- **Gejala**: Di mode Warmup maupun Deathmatch, YaPB bot sama sekali tidak membeli senjata/perlengkapan. Bot tampak seperti tidak 100% berfungsi sesuai kode Savage AI. Padahal di GoldSrc sebelumnya, YaPB bot berjalan normal dan sistem role (senjata & utility per role) berfungsi sebagaimana mestinya.

---

## 2. ARSITEKTUR ENGINE & AUDIT KODE SUMBER

Audit teknis mendalam terhadap kode sumber `.sma`, file konfigurasi `.cfg`, dan profil `.ini` menghasilkan temuan sebagai berikut:

### A. Audit File Launcher & Konfigurasi Gamemode
1. **Redundansi Launcher**:
   - `!Start_CS16_Xash3D.bat`:
     ```bat
     copy /y "cstrike\addons\amxmodx\configs\plugins-competitive.ini" "cstrike\addons\amxmodx\configs\plugins.ini" >nul 2>&1
     echo exec competitive.cfg > "cstrike\mode.cfg"
     start "" "xash3d.exe" -game cstrike -dll addons/metamod/dlls/metamod.dll -fullscreen -high -condebug +log on +mp_logfile 1 +exec competitive.cfg
     ```
   - `!Start_Competitive_Xash3D.bat`: Memiliki baris kode yang persis sama. File ini redundan dan membingungkan pengguna.
2. **Kelemahan Mekanisme Mode Switch**:
   - Batch script menyalin file `.ini` sebelum `xash3d.exe` dijalankan. Namun jika game diluncurkan langsung atau jika pergantian mode terjadi saat server listen berjalan (`listenserver.cfg` mengeksekusi `mode.cfg`), plugin AMXX tidak reload secara otomatis tanpa `restart` atau `changelevel`.
3. **Ketiadaan Guard Clause pada `csgo_competitive.sma`**:
   - Dalam `cstrike/deathmatch.cfg`, baris 7 telah mengeksekusi: `amx_csgo_mode 0`.
   - Namun di dalam `csgo_competitive.sma`, cvar `pCvar_Mode` (`amx_csgo_mode`) **hanya diperiksa di 5 fungsi**: `HandleSay`, `LogEvent_RoundStart`, `LogEvent_RoundEnd`, `HandleRoundWin`, dan `Task_UpdateHUD`.
   - Fungsi penting berikut **TIDAK MEMERIKSA `pCvar_Mode` SAMA SEKALI**:
     - `OnPlayerSpawnPost(id)`: Jika `g_MatchState == STATE_WARMUP` (defaultnya 0), maka otomatis memberikan uang $16000, armor, hegrenade, mematikan C4, dan menjadwalkan `Task_WarmupBotEquip`.
     - `Cmd_Ready(id)`: Pemain manapun yang mengetik `.ready` atau `ready` akan langsung memicu `CheckReadyStatus()` dan memulai match live MR12!
     - `OnPlayerKilledPost(victim, ...)`: Menjadwalkan `Task_WarmupRespawn` (memanggil `ExecuteHamB(Ham_CS_RoundRespawn, id)`).
     - `BlockBuyCmd` & `BlockDropCmd`: Tetap memblokir perintah buy/drop jika state knife round aktif.
     - `Msg_RoundTime`: Mengubah timer menjadi `g_WarmupTimeLeft`.
   - **Kesimpulan Akar Masalah 1**: Selama `csgo_competitive.amxx` aktif di memory atau terdaftar di `plugins.ini`, mematikan `amx_csgo_mode 0` tidak menghentikan fungsi Warmup DM dan command `.ready`. Harus ada isolasi absolut (plugin unpause/pause atau strict master guard).

---

### B. Audit YaPB Bot & Savage AI Role System
Mengapa YaPB Bot tidak membeli senjata dan tidak menjalankan role di Warmup & DM?

1. **Konfigurasi Cvar YaPB di `deathmatch.cfg`**:
   - Di `cstrike/deathmatch.cfg` baris 31:
     ```cfg
     yb_botbuy 0
     ```
   - Cvar ini **mematikan total logika pembelian senjata bawaan binary YaPB DLL (`yapb.dll`)**. YaPB tidak akan pernah mengeluarkan perintah `buy` internal jika cvar ini bernilai 0.

2. **Batasan Logika Beli Internal YaPB di Xash3D**:
   - Logika beli internal C++ YaPB (`yb_botbuy 1`) dirancang untuk match standar Counter-Strike: ia hanya mengevaluasi pembelian saat menerima event `FreezeTime` atau detik pertama setelah `Round_Start`.
   - Pada mode **Warmup** dan **Deathmatch**:
     - `mp_freezetime` diset `0`.
     - Tidak ada freeze period, dan pemain terus menerus respawn (`Ham_CS_RoundRespawn`).
     - Bot YaPB yang respawn di tengah jalan menganggap round sedang berjalan, sehingga YaPB C++ tidak memicu siklus buy.
   - Di engine **Xash3D FWGS**, fakeclient bot yang berada di luar entitas trigger `func_buyzone` (karena random JSON spawns DM atau spawn di luar markas) otomatis ditolak oleh engine jika mengirim perintah buy, karena check buyzone engine Xash3D ketat terhadap fakeclient entity flags.

3. **Keterbatasan Arsitektur Role Loadout pada `bot_savage_ai.sma`**:
   - `bot_savage_ai.sma` memiliki sistem 5-Role Esports yang sangat canggih:
     - `ROLE_ENTRY`: Entry Fragger (AK/M4/Galil/Famas + Flashbang + HE).
     - `ROLE_AWPER`: Sniper (AWP/Scout + Deagle).
     - `ROLE_SUPPORT`: Support Utility (Rifle + Smoke + Flashbang + HE + Defuse Kit).
     - `ROLE_LURKER`: Flanker/Anchor (Silenced M4A1-S/USP/MP5 + Smoke).
     - `ROLE_IGL`: Tactical Leader (Rifle/SMG + Full Utility).
   - **Namun perhatikan kapan role loadout ini dieksekusi di kode sumber**:
     ```pawn
     // bot_savage_ai.sma baris 641:
     set_task(1.4, "Task_EnforceRoleLoadouts", TASK_ENFORCE_LOADOUTS);
     ```
     Fungsi ini dipanggil **hanya saat Freeze Time Round Start** (`Event_FreezeTimeStart`).
   - Di dalam `OnPlayerSpawnPost(id)` pada `bot_savage_ai.sma`:
     ```pawn
     // bot_savage_ai.sma baris 1416:
     if (IsWarmupActive() || IsKnifeRoundActive())
     {
         return HAM_IGNORED;
     }
     ```
     Artinya, saat Warmup aktif, `bot_savage_ai` **SECARA EKSPLISIT MENOLAK MENGURUS SPAWN BOT**.
   - Di mode Deathmatch murni (`redm_active 1`), event freeze time tidak pernah terjadi. Akibatnya, `Task_EnforceRoleLoadouts` **tidak pernah terpanggil sekalipun**!
   - Di `redm_listen_engine.sma`:
     Fungsi bot equip saat ini adalah `AutoEquipBot(id)` yang hanya memilih indeks senjata random secara acak tanpa memanggil `savage_get_player_role(id)`, tanpa utility (grenade), dan tanpa memperhitungkan role loadout.
   - Di `csgo_competitive.sma`:
     Fungsi `Task_WarmupBotEquip(taskid)` juga memberikan senjata random `random_num(1, 10)` tanpa mengindahkan role `bot_savage_ai`.

---

## 3. IDENTIFIKASI DIREKTORI & FILE RELEVAN

| Komponen | Path File | Peran & Masalah |
| :--- | :--- | :--- |
| **Launchers** | `!Start_CS16_Xash3D.bat` | Launcher utama CS 1.6 standar (Warmup + Comp MR12). Perlu dipertahankan. |
| | `!Start_Competitive_Xash3D.bat` | Launcher redundan yang identik. **Harus dihapus**. |
| | `!Start_Deathmatch_Xash3D.bat` | Launcher khusus Deathmatch. Harus mengisolasi mode DM secara mutlak. |
| **Configs** | `cstrike/competitive.cfg` | Config aturan competitive & warmup. |
| | `cstrike/deathmatch.cfg` | Config aturan DM (`yb_botbuy 0`, `redm_active 1`). |
| | `cstrike/listenserver.cfg` | Config listen server, mengeksekusi `mode.cfg`. |
| | `cstrike/mode.cfg` | File penunjuk gamemode aktif (`exec competitive.cfg` atau `exec deathmatch.cfg`). |
| | `cstrike/addons/amxmodx/configs/plugins-competitive.ini` | Profil plugin competitive aktif, redm dormant. |
| | `cstrike/addons/amxmodx/configs/plugins-deathmatch.ini` | Profil plugin deathmatch aktif, competitive dormant. |
| **Plugins** | `cstrike/addons/amxmodx/scripting/csgo_competitive.sma` | Perlu master guard clause pada `OnPlayerSpawnPost`, `Cmd_Ready`, `OnPlayerKilledPost`, dll. |
| | `cstrike/addons/amxmodx/scripting/bot_savage_ai.sma` | Perlu hooks role delivery saat bot spawn di mode Warmup & DM. |
| | `cstrike/addons/amxmodx/scripting/redm_listen_engine.sma` | Perlu integrasi role delivery Savage AI saat bot spawn di DM. |

---

## 4. PERTANYAAN ARSITEKTURAL & KEBUTUHAN RISET UNTUK GEMINI 4 ARGON

Gemini 4 Argon diminta untuk menyusun arsitektur dan spesifikasi solusi menyeluruh:

1. **Gamemode Separation & Master Guard Architecture**:
   - Bagaimana cara memastikan bahwa ketika launcher DM aktif, seluruh fungsi `csgo_competitive.amxx` (Warmup, `.ready`, Knife round, MR12, HUD sync) 100% inert/dormant atau di-pause secara engine-level (`pause_plugin`), sehingga tidak ada kebocoran state atau hook?
   - Bagaimana menyederhanakan launcher menjadi 2 launcher resmi (`!Start_CS16_Xash3D.bat` dan `!Start_Deathmatch_Xash3D.bat`) dengan atomic switch yang handal di Windows / Xash3D?

2. **YaPB Bot Role Loadout Delivery System di Xash3D FWGS**:
   - Mengapa YaPB bot di Xash3D tidak membeli senjata saat `yb_botbuy 0` atau saat spawn di luar buyzone, dan bagaimana arsitektur **Programmatic Role Delivery** via Pawn API (`give_item`, `cs_set_user_bpammo`, `cs_set_user_armor`) yang paling stabil untuk Xash3D?
   - Bagaimana membagi peran role bot (`ROLE_ENTRY`, `ROLE_AWPER`, `ROLE_SUPPORT`, `ROLE_LURKER`, `ROLE_IGL`) agar tetap aktif dan mendapatkan senjata + utility yang sesuai, baik di:
     a) Mode Warmup DM (sebelum match competitive live).
     b) Mode ReDeathmatch murni (`redm_listen_engine.amxx`).
     c) Mode Live Match MR12.
   - Bagaimana native bridge antara `bot_savage_ai.sma` dan gamemode (`csgo_competitive.sma` & `redm_listen_engine.sma`) agar bot loadout terdistribusi secara konsisten tanpa race condition?

3. **YaPB Engine Cvars & Navigation Sync**:
   - Cvar apa saja dari YaPB (`yb_csdm_mode`, `yb_ignore_objectives`, `yb_botbuy`, `yb_walking_allowed`) yang wajib diset pada masing-masing mode agar YaPB bot di Xash3D FWGS bergerak agresif, tidak bingung mencari buyzone, dan memanfaatkan pathing/waypoint dengan presisi tinggi?
