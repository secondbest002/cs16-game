# Blueprint 01 — Agent DevEx & Automated Guardrails Suite

**Document ID:** `BP-01-DEVEX-GUARDRAILS`  
**Target Workspace:** `D:\Counter-Strike 1.6` (Root + `cs16-game/` + `cs16-client/`)  
**Architecture Baseline:** `cs16-game/ARCHITECTURE.md` (Option 1 + Option 2-Lite — 7 Active Custom Plugins + `include/cs2_sdk/`)  
**Scope:** Git Source-Control Hygiene, Permanent Agent Memory (`AGENTS.md`), Pre-Flight Validation Pipeline (`verify_workspace.ps1`), and Automated Runtime Log Digest (`check_logs.ps1`).  
**Primary Objective:** Mengurangi waktu orientasi AI Agent hingga >75%, mencegah regresi diam-diam (CVar drift, missing precache asset, accidental engine command hijack), dan menyediakan audit `git diff` + rollback deterministik.

---

## 1. Executive Problem Statement

Hasil audit workspace saat ini mengungkap 4 risiko operasional utama saat AI Agent mengedit proyek:
1. **Zero Version Control:** `D:\Counter-Strike 1.6` bukan Git repository (`fatal: not a git repository`). Agent tidak dapat menjalankan `git diff`, `git status`, atau `git checkout` untuk memverifikasi ruang lingkup edit.
2. **Repeated Context Discovery Tax:** Setiap sesi baru, Agent harus membaca ulang puluhan file untuk mengetahui aturan kritis (misal: larangan `clcmd("m4a1")`, sinkronisasi `CS2_KILL_*` dengan `client.dll/death.cpp`, dan 3 file nVault yang imutabel).
3. **Silent Pre-Flight Defects:**
   * Asset hilang di `models/` atau `sound/` baru meledak saat map load (contoh aktual di `error_20261123.log`: `csgo_zeus.amxx` gagal precache dan memicu `Run time error 1`).
   * **CVar Drift** antara `.cfg` dan `.sma` terjadi tanpa error compile (contoh aktual di `csgo_competitive.cfg`: baris 96 `csgo_comp_max_rounds 24` tidak terdaftar di mana pun; baris 67 `amx_csgo_c4_timer 40` padahal di `csgo_competitive.sma:44` CVar tersebut boolean `1`/`0`).
4. **Raw Log Noise:** `qconsole.log` dan `addons/amxmodx/logs/L*.log` + `error_*.log` mengandung ribuan baris dari sesi-sesi lama, memboroskan context token Agent saat debugging.

---

## 2. Pillar A — Lightweight Git Repository & `.gitignore` Strategy

### 2.1 Architectural Policy: "Source & Config Only"
Git diinisialisasi di root `D:\Counter-Strike 1.6\.git`, **hanya melacak artefak teks/source** (`.sma`, `.inc`, `.inl`, `.cfg`, `.ini`, `.json`, `.bat`, `.ps1`, `.md`, `.cpp`, `.h`, `CMakeLists.txt`). Seluruh binary engine, aset game, log, dan build output dikecualikan agar repo tetap <15 MB dan `git status` berjalan <100 ms.

### 2.2 Production `.gitignore` Specification (`D:\Counter-Strike 1.6\.gitignore`)
```gitignore
# ==============================================================================
# 1. ARCHIVE & BACKUPS
# ==============================================================================
_archive_amxx/
**/*.bak
**/*.orig*
**/*.pre_*
**/*_backup*/

# ==============================================================================
# 2. GAME ASSETS & HEAVY BINARIES (Models, Maps, Audio, Textures, Waypoints)
# ==============================================================================
*.bsp
*.mdl
*.spr
*.wav
*.mp3
*.wad
*.bmp
*.tga
*.png
*.jpg
*.dem
*.graph
*.pmx
*.prc
*.vis
*.dat
*.vault
*.nvault

# ==============================================================================
# 3. COMPILED BINARIES & DEBUG SYMBOLS
# ==============================================================================
*.exe
*.dll
*.so
*.dylib
*.lib
*.exp
*.pdb
*.ilk
*.obj
*.o
*.amxx
*.amx
build/
out/
.vs/
CMakeCache.txt
CMakeFiles/

# ==============================================================================
# 4. RUNTIME LOGS, USER STATE & EPHEMERAL FILES
# ==============================================================================
**/logs/*.log
**/logs/*.txt
**/qconsole.log
**/console.log
cs16-game/cstrike/mode.cfg
cs16-game/cstrike/addons/amxmodx/configs/plugins.ini
cs16-game/cstrike/addons/amxmodx/data/matches/*.json
cs16-game/cstrike/addons/amxmodx/scripting/compile.dat

# ==============================================================================
# 5. EXPLICITLY TRACKED EXCEPTIONS (Never Ignore)
# ==============================================================================
!cs16-game/cstrike/addons/amxmodx/configs/plugins-competitive.ini
!cs16-game/cstrike/addons/amxmodx/configs/plugins-deathmatch.ini
!cs16-game/cstrike/addons/amxmodx/configs/plugins-practice.ini
!cs16-game/cstrike/addons/amxmodx/data/redm/*.spawns.json
```

> **Catatan Kritis `plugins.ini` & `mode.cfg`:** Kedua file ini selalu ditimpa secara atomik oleh `!Start_*_Xash3D.bat` setiap kali game diluncurkan, sehingga wajib di-ignore agar tidak mengotori `git status`. Sumber kebenaran (Source of Truth) yang di-track adalah `plugins-competitive.ini`, `plugins-deathmatch.ini`, dan (jika ada) `plugins-practice.ini`.

### 2.3 Git Initialization Runbook
1. Tulis `D:\Counter-Strike 1.6\.gitignore`.
2. Jalankan `git init` di `D:\Counter-Strike 1.6`.
3. Dry-run `git status --short` dan verifikasi **tidak ada** file >500 KB atau ekstensi binary di daftar untracked.
4. Buat baseline commit: `git add . && git commit -m "chore: initial source & config baseline (post Option 1 + Option 2-Lite 7-plugin modular refactor)"`.
5. Buat git tag penanda rollback: `git tag baseline-modular-7plugin`.

---

## 3. Pillar B — Permanent Agent Memory Contract (`cs16-game/AGENTS.md`)

File `cs16-game/AGENTS.md` dirancang agar muat di bawah **180 baris** (~2.500 token), menjadi file pertama yang wajib dibaca Agent sebelum menyentuh kode apa pun.

### 3.1 Struktur Wajib `cs16-game/AGENTS.md`
1. **Runtime Topology, 7 Active Custom Plugins & Launchers (Sinkron dengan `cs16-game/ARCHITECTURE.md`):**
   * Engine: Xash3D FWGS 0.21 (`xash3d.exe`) + Patched `client.dll` (source di `../cs16-client/cl_dll/`) + Metamod + AMXX 1.10 + YaPB (`addons/yapb/bin/yapb.dll`).
   * **4 Pilar + Standalone + 2 Gamemode Slot (Total 7 Plugin Kustom Aktif):**
     * Pillar 1: `cs2_arsenal.amxx` (`cs2_arsenal.sma` + `cs2_arsenal/{core,modules}/` — gabungan skin inventory + weapon variants)
     * Pillar 2: `cs2_modern_engine.amxx` (`cs2_modern_engine.sma` + `cs2_engine/`)
     * Pillar 3: `bot_savage_ai.amxx` (`bot_savage_ai.sma` + `bot_savage/{core,modules}/`)
     * Pillar 4: `cs2_client_studio.amxx` (`cs2_client_studio.sma` + `cs2_studio/{core,modules}/` — gabungan crosshair HUD + viewmodel studio)
     * Standalone: `csgo_molotov.amxx` (`csgo_molotov.sma`)
     * Gamemode Slot: `csgo_competitive.amxx` (`csgo_competitive.sma` + `cs2_competitive/{core,modules}/`) **atau** `redm_listen_engine.amxx` (`redm_listen_engine.sma`)
   * **Shared SDK (`scripting/include/cs2_sdk/`):** `cs2_api.inc` (kontrak native terproteksi `cs2_sdk_native_filter` + `LibraryExists`, **0 `callfunc_begin`**), `cs2_offsets.inc`, `cs2_state_cvars.inc`, `cs2_weapon_registry.inc`.
   * Mode Competitive: `!Start_CS16_Xash3D.bat` -> `plugins-competitive.ini` + `competitive.cfg`.
   * Mode Deathmatch: `!Start_Deathmatch_Xash3D.bat` -> `plugins-deathmatch.ini` + `deathmatch.cfg`.
2. **Feature -> Source File Lookup Table & Task ID Partitions:**
   * Tabel ringkas 1 baris per fitur mengarah langsung ke sub-modul `.inl` aktual (`cs2_arsenal/modules/*`, `cs2_engine/modules/*`, `bot_savage/modules/*`, `cs2_studio/modules/*`, `cs2_competitive/modules/*`, `csgo_molotov.sma`, `redm_listen_engine.sma`) beserta rentang Task ID resmi (`10000–59999`, plus `60000–60999` untuk BP-03).
3. **Hard Invariants (DO NOT BREAK):**
   * **Engine Command Trap:** Dilarang `register_clcmd("m4a1", ...)` (membajak `B-4-3` engine buy).
   * **nVault Immutability:** `cs2_loadout` (`cs2_arsenal.amxx`), `cs2_crosshair` & `cs2_viewmodel` (keduanya dibuka terpisah oleh `cs2_client_studio.amxx`) tidak boleh ubah nama vault maupun skema key.
   * **External Sync:** `skin_inventory.ini` & `active_loadout.ini` dikelola desktop app `CS2 Loadout Manager` dan di-parse single-pass oleh `cs2_arsenal/core/arsenal_loadout.inc` (`cs2_reload_loadout`).
   * **Client Sync:** `CS2_KILL_*` flags harus cocok dengan `../cs16-client/cl_dll/death.cpp:28-34`; clamp `viewmodel_fov [50..75]` & `offset [-3.5..3.5]` harus cocok dengan `../cs16-client/cl_dll/view.cpp`.
   * **Knife Detection Order:** Cek substring `"m9"` **sebelum** `"bayonet"` (terpusat di `include/cs2_sdk/cs2_weapon_registry.inc`).
   * **YaPB Boundary:** `bot_savage_micro_override` default `0`; `cs2_client_studio.amxx` wajib **Zero `PreThink`**.
   * **Cross-Plugin Resilience:** Dilarang memakai `callfunc_begin`; semua komunikasi lintas pilar wajib lewat `include/cs2_sdk/cs2_api.inc` + `set_native_filter("cs2_sdk_native_filter")`. Sub-modul dalam plugin terpadu wajib soft-disable (dilarang `set_fail_state`).
4. **Standard Verification Workflow:**
   * Setiap perubahan `.sma`/`.inc`/`.inl`/`.cfg`/`.ini` WAJIB diakhiri dengan menjalankan `cs16-game\cstrike\addons\amxmodx\scripting\build_all.bat` (atau `powershell -ExecutionPolicy Bypass -File cs16-game\tools\verify_workspace.ps1`).
   * Setiap selesai uji in-game, jalankan `powershell -ExecutionPolicy Bypass -File cs16-game\tools\check_logs.ps1`.

---

## 4. Pillar C — Pre-Flight Validator (`cs16-game/tools/verify_workspace.ps1`)

Skrip PowerShell self-contained (zero external dependencies) dengan **exit code `0` (PASS)** atau **`1` (FAIL)** agar Agent dapat menggunakannya sebagai hard verification gate.

### 4.1 Empat Gate Validasi

| Gate | Target Pemeriksaan | Mekanisme Teknis | Kriteria FAIL / WARN |
| :--- | :--- | :--- | :--- |
| **Gate 1: Strict AMXX Compile** | 7 `.sma` aktif (`cs2_arsenal`, `cs2_modern_engine`, `bot_savage_ai`, `cs2_client_studio`, `csgo_molotov`, `csgo_competitive`, `redm_listen_engine` — selaras `scripting/build_all.bat` & `plugins-*.ini`) | Eksekusi `amxxpc.exe -iinclude -o..\plugins\<name>.amxx <name>.sma`. Tangkap stdout/stderr. | **FAIL** jika exit code != 0 atau ada kata `Error:` / `Fatal error:`. **WARN** jika jumlah `Warning:` meningkat. |
| **Gate 2: Asset & Precache Integrity** | `skin_inventory.ini`, `active_loadout.ini`, `cs2_arsenal/{core,modules}/*`, `cs2_engine/modules/{mod_knife_sounds,mod_kill_feedback,mod_modern_utility}.inl`, `bot_savage_ai.sma`, `csgo_molotov.sma` | Ekstrak regex seluruh literal string `models/...\.mdl`, `sprites/...\.spr`, `weapons/...\.wav`, `sound/...\.wav`. Verifikasi eksistensi file relatif terhadap `cs16-game/cstrike/` (dan `cs16-game/valve/` untuk aset base HL). | **FAIL** jika ada aset di plugin aktif atau `active_loadout.ini` yang tidak ditemukan di disk (mencegah `Run time error 1`). |
| **Gate 3: CVar Drift & Type Audit** | `configs/cs2_modern_engine.cfg`, `configs/bot_savage_roles.cfg`, `configs/csgo_competitive.cfg`, `cstrike/competitive.cfg`, `cstrike/deathmatch.cfg` (+ `cstrike/practice.cfg` jika ada) | Parse semua `register_cvar("X", ...)` dan `create_cvar("X", ...)` dari seluruh `.sma`/`.inl`/`.inc` aktif -> bangun HashSet valid custom CVars (prefix `amx_`, `cs2_`, `csgo_`, `bot_savage_`, `redm_`). Bandingkan dengan setiap baris non-komentar di `.cfg`. | **WARN/FAIL** jika ada CVar berprefix custom di `.cfg` yang tidak pernah didaftarkan di kode (seperti `csgo_comp_max_rounds 24`), atau nilai boolean CVar di luar `{0,1}` (seperti `amx_csgo_c4_timer 40`). |
| **Gate 4: Profile Parity & Orphan Check** | `plugins-competitive.ini`, `plugins-deathmatch.ini` (+ `plugins-practice.ini` jika ada), `plugins/`, `scripting/` | Pastikan setiap `.amxx` aktif (uncommented) di semua profil benar-benar ada di `plugins/`, memiliki timestamp lebih baru dari `.sma` serta sub-modul `.inl`/`.inc` terkait (deteksi stale binary!), tidak ada `.amxx` nyasar di `scripting/`, dan **0 pemanggilan `callfunc_begin`** di plugin kustom. | **FAIL** jika `.amxx` missing/stale, ada `.amxx` nyasar di `scripting/`, atau ditemukan regresi `callfunc_begin`. |

### 4.2 CLI Interface & Output Contract
```powershell
# Default: jalankan semua 4 gate (mode CI / Agent)
.\cs16-game\tools\verify_workspace.ps1

# Mode cepat tanpa compile ulang (hanya cek config, asset, & parity)
.\cs16-game\tools\verify_workspace.ps1 -SkipCompile

# Mode strict: jadikan semua Warning sebagai Fail
.\cs16-game\tools\verify_workspace.ps1 -Strict
```
Output dicetak dalam format ringkas (<40 baris saat PASS) agar hemat token untuk AirLock/Agent:
```text
[GATE 1] Compile Active Plugins (7/7) ........... PASS (0 errors, 0 warnings)
[GATE 2] Asset & Precache Integrity (214 refs) .. PASS (0 missing)
[GATE 3] CVar Drift Detector .................... FAIL (2 anomalies)
  - csgo_competitive.cfg:96 -> Unknown CVar 'csgo_comp_max_rounds'
  - csgo_competitive.cfg:67 -> Boolean CVar 'amx_csgo_c4_timer' has value '40' (expected 0|1)
[GATE 4] Profile Parity & SDK Guard ............. PASS (0 stale, 0 stray, 0 callfunc_begin)
RESULT: FAILED (1 gate failed, 2 anomalies)
```

---

## 5. Pillar D — Auto Runtime Log-Digest (`cs16-game/tools/check_logs.ps1`)

### 5.1 Sumber Log yang Dipindai
1. `cs16-game/cstrike/addons/amxmodx/logs/error_*.log` (ambil file dengan tanggal/LastWriteTime terbaru).
2. `cs16-game/cstrike/addons/amxmodx/logs/L*.log` (file terbaru).
3. `cs16-game/cstrike/qconsole.log` atau `cs16-game/qconsole.log` (dari flag `-condebug` pada launcher `.bat`).

### 5.2 Logika Sesi & Filtering
* **Session Slicing:** Hanya baca setelah marker sesi terakhir (`Start of error session.` untuk AMXX error log; `Log file started` / `Map:` / `Server logging data` terakhir untuk console log) sehingga error lama dari minggu lalu tidak membingungkan Agent.
* **Noise Suppression & Grouping:**
  * Deduplikasi error berulang dengan counter `(x14 occurrences)`.
  * Kelompokkan output menjadi 4 bucket:
    1. `[FATAL / RUNTIME ERRORS]` (`Run time error`, `bad load`, `Invalid plugin`, `set_fail_state`, `Native error`, `Stack error`)
    2. `[PRECACHE / ASSET WARNINGS]` (`not exist`, `not precached`, `can't load`, `missing`)
    3. `[PLUGIN INIT STATUS]` (baris `[CS2 ...]`, `[Bot Savage ...]`, `[CS:GO Competitive]`, `[ReDM]`)
    4. `[CLIENT / ENGINE WARNINGS]` (`SV_StartSound`, `SZ_GetSpace`, `Host_Error`, `Cbuf_AddText`)
* **Parameter Opsional:** `-Sessions <N>` (default `1` sesi terakhir), `-RawErrors` (tampilkan stack trace penuh jika `debug` aktif di `plugins.ini`).

---

## 6. Known Config Anomalies to Fix Immediately Upon Deployment

Berdasarkan inspeksi aktual, skrip Gate 3 akan langsung mendeteksi 2 anomali nyata yang harus diperbaiki segera:
1. **`cs16-game/cstrike/addons/amxmodx/configs/csgo_competitive.cfg:96`**: Hapus `csgo_comp_max_rounds 24` (redundant & tidak terdaftar; `amx_csgo_maxrounds 12` sudah ada di baris 13).
2. **`cs16-game/cstrike/addons/amxmodx/configs/csgo_competitive.cfg:67`**: Ubah `amx_csgo_c4_timer 40` -> `amx_csgo_c4_timer 1` (toggle HUD timer; durasi 40 detik sudah diatur dengan benar oleh `mp_c4timer 40` di baris 85).

---

## 7. Acceptance Criteria & Verification Checklist

* [ ] `git status` di `D:\Counter-Strike 1.6` bersih dan hanya melacak file source/config (<20 MB `.git`).
* [ ] `cs16-game/AGENTS.md` selesai ditulis (<180 baris) dan mencerminkan struktur plugin aktif terkini.
* [ ] `cs16-game/tools/verify_workspace.ps1` berjalan <5 detik, sukses mendeteksi dummy missing asset & CVar drift saat diuji, dan return exit code `0` setelah 2 anomali `csgo_competitive.cfg` diperbaiki.
* [ ] `cs16-game/tools/check_logs.ps1` menampilkan ringkasan bersih dari sesi terbaru `error_*.log` dan `qconsole.log` dalam <50 baris.
