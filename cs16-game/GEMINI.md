# PENGINGAT ARSITEKTUR & WORKFLOW PROYEK CS 1.6 (XASH3D ENGINE)

> [!CRITICAL]
> **ATURAN UTAMA PENYIMPANAN**:
> **DILARANG KERAS MEMBUAT FOLDER `.agents` DI DALAM DIREKTORI INI!**
> Seluruh workflow, tindakan, konteks, dan hasil riset dikelola secara eksklusif di dalam folder `gemini/` dan file panduan `GEMINI.md` ini.

---

## 1. STRUKTUR DIREKTORI WORKFLOW (`gemini/`)
- `gemini/context/`: Tempat menyimpan file konteks teknis per fitur (`context_<nama_fitur>.md`).
- `gemini/argon/`: Tempat User menaruh file dokumen hasil riset dari Gemini 4 Argon.
- `gemini/workflow/WORKFLOW.md`: Spesifikasi lengkap alur kerja (Lifecycle & State Machine).
- `gemini/actions/ACTIONS.md`: Spesifikasi aksi dan peran Action Swarm (`pawn-coder`, `amxx-compiler`, `config-integrator`).

---

## 2. STANDARD OPERATING PROCEDURE (SOP) UNTUK MODEL

### A. Saat User Memicu `/gemini-plan ...`
Format pemicu user:
`"/gemini-plan buat kan context dan prompt untuk gemini argon. aku ingin fitur <nama/deskripsi fitur>"`

**Langkah Wajib Model**:
1. Bedah fitur yang diminta, identifikasi engine hooks, direktori, dan plugin terkait.
2. Buat file konteks teknis di `gemini/context/context_<nama_fitur>.md` yang memuat detail arsitektur game yang harus diketahui oleh Gemini Argon.
3. Berikan **Curated Prompt** langsung di dalam balasan chat (code block markdown) agar user mudah menyalinnya. **JANGAN membuat file prompt dalam format `.md`**.
4. Arahkan user untuk menaruh hasil riset Gemini Argon ke folder `gemini/argon/`.

### B. Saat Hasil Riset Gemini Argon Tersedia di `gemini/argon/`
1. Baca dan telaah dokumen riset dari `gemini/argon/`.
2. Validasi kelayakan teknis terhadap batasan AMXX 1.10 dan Xash3D engine.
3. Buat **Implementation Plan Artifact** (`/plan`) dengan metadata `request_feedback: true` dan `user_facing: true`.
4. **Tunggu persetujuan eksplisit dari User**. Dilarang mengubah kode sebelum disetujui.

### C. Saat User Memberikan Persetujuan (Approval)
1. Aktifkan **Action Swarm** menggunakan tool `swarm(action='add', ...)`:
   - Worker `pawn-coder`: Menulis atau mengedit file `.sma` / `.inl`.
   - Worker `amxx-compiler`: Memverifikasi kompilasi dengan `cstrike/addons/amxmodx/scripting/amxxpc.exe`.
   - Worker `config-integrator`: Menyinkronkan `.ini`, `.cfg`, dan precache aset.
2. Validasi bahwa tidak ada error/warning kompilasi.
3. Buat **Walkthrough Artifact** (`walkthrough.md`) dan laporkan ke user.

### D. Saat User Memicu `/clean-code-session`
Format pemicu user:
`"/clean-code-session"` atau `"/clean-code-session <target_file>"`

**Langkah Wajib Model**:
1. Lakukan audit teknis mendalam terhadap kode yang baru saja dimodifikasi atau modul terkait:
   - **Dead Code & Unused Elements**: Hapus macro (#define), task ID, cvar, variabel, stock, dan header (`#include`) yang tidak lagi digunakan.
   - **Deduplikasi (DRY)**: Satukan blok logika atau loop repetitif (misal multiple while-loops pencarian entitas) ke dalam static array-driven loop atau fungsi sentral.
   - **Throttling & Resource Safety**: Terapkan pencegahan task duplikat / multiple map entity scans dengan `!task_exists(...)` dan pembersihan task defensif di `client_putinserver` serta `client_disconnected`.
   - **Log Sanitization**: Hapus sisa debugging verbose (`log_amx`, print spam) terutama pada *hot execution paths* agar tidak memicu *disk I/O stutter*.
2. **Invariant Fungsionalitas**: Dilarang keras mengubah logic atau fungsionalitas yang sudah dinyatakan bekerja sempurna (*zero functional regression*).
3. Jalankan kompilasi menggunakan `cstrike/addons/amxmodx/scripting/amxxpc.exe` dan pastikan 0 error, 0 warning.
4. Sinkronisasikan file `.amxx` hasil kompilasi ke direktori plugin runtime dan berikan laporan detail yang transparan kepada user.

---

## 3. LINGKUNGAN TEKNIS PROYEK
- **Engine**: Xash3D FWGS 32-bit (OpenGL `ref_gl.dll`, SDL2, kompatibilitas penuh biner GoldSrc).
- **Mod Framework**: Metamod (`addons/metamod/dlls/metamod.dll`) + AMX Mod X 1.10 (`amxmodx_mm.dll`).
- **Compiler & Build Gate**: `cstrike/addons/amxmodx/scripting/amxxpc.exe` + `build_all.bat` (wajib 7/7 lolos tanpa error/warning).
- **Shared SDK**: `cstrike/addons/amxmodx/scripting/include/cs2_sdk/` (`cs2_offsets.inc`, `cs2_weapon_registry.inc`, `cs2_state_cvars.inc`, `cs2_api.inc` dengan `cs2_sdk_native_filter` + `LibraryExists`). Peta arsitektur lengkap ada di `ARCHITECTURE.md`.
- **Sistem Aktif Utama (7 Plugin Kustom AMXX)**:
  - `cs2_arsenal.sma` (`cs2_arsenal/`): Pilar 1 — Skin inventory, CT/T loadouts, M4A4/M4A1-S split, gunshot FX, dan pricing/clip weapon variants (`cs2_loadout` nVault).
  - `cs2_modern_engine.sma` (`cs2_engine/`): Pilar 2 — Engine gameplay CS2 (Kill feedback, Kill rewards, Utility 3-way, E-pickup, Switchhands, Ammo refunder, USPS silencer, Knife sounds, 3D smart ping).
  - `bot_savage_ai.sma` (`bot_savage/`): Pilar 3 — YaPB Tactical Esports AI (`bot_savage_ai.inc` wrapper -> `cs2_sdk`), roles, IGL strats, combat tick, reflex ESP, scavenge/drop economy.
  - `cs2_client_studio.sma` (`cs2_studio/`): Pilar 4 — Client UX (`cs2_crosshair` & `cs2_viewmodel` dual-vault, zero `PreThink`, `/studio` hub, `/crosshair`, `/scope`, `/hud`, `/vm`).
  - `csgo_molotov.sma`: Standalone — Molotov & Incendiary Grenade (`csgo_molotov` library).
  - `csgo_competitive.sma` (`cs2_competitive/`): Gamemode Competitive — Warmup DM, `.ready`, Knife round + side pick, MR12, Halftime swap, Overtime MR3.
  - `redm_listen_engine.sma`: Gamemode Deathmatch — ReDM Listen DM, JSON spawns, weapon menus, instant respawn (`plugins-deathmatch.ini`).
