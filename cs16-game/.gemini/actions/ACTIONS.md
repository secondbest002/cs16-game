# SPESIFIKASI ACTIONS & ACTION SWARM: GEMINI MODDING PIPELINE

Dokumen ini mendefinisikan aksi otomatis (Actions) dan peran tim pekerja otonom (Action Swarm Workers) yang dijalankan oleh Model saat memproses alur kerja modifikasi CS 1.6 / Xash3D.

---

## 1. DAFTAR AKSI INTI (CORE ACTIONS)

### Action 1: `Action_GenerateContextAndPrompt`
- **Trigger**: Deteksi pola chat `^/gemini-plan\b` atau permintaan pengguna untuk membuat context & prompt Gemini Argon.
- **Input**: Deskripsi fitur dari pengguna.
- **Prosedur**:
  1. Parsing nama fitur dan ekstrak kata kunci (contoh: "c4 defuse mini-game" -> `c4_defuse_minigame`).
  2. Lakukan audit cepat terhadap subsistem proyek terkait:
     - Header/forward AMXX (`cstrike`, `fakemeta`, `hamsandwich`, `engine`, `xs`).
     - Plugin relevan yang sudah ada (`cs2_modern_engine.sma`, `bot_savage_ai.sma`, `csgo_competitive.sma`, dll.).
  3. Tulis file konteks spesifik:
     - Path: `gemini/context/context_<nama_fitur>.md`
     - Isi: Direktori relevan, batas memory/entitas, network channel limits, hooks yang tersedia, dan modul yang sedang aktif.
  4. Cetak **Curated Prompt** langsung pada balasan chat (formatted in markdown codeblock). JANGAN menulis file prompt `.md`.
  5. Ingatkan pengguna untuk menaruh hasil riset di `gemini/argon/`.

---

### Action 2: `Action_IngestArgonResearch`
- **Trigger**: Pengguna menginfokan bahwa dokumen riset Gemini Argon telah tersedia di folder `gemini/argon/`.
- **Prosedur**:
  1. Pindai folder `gemini/argon/` untuk membaca file riset terbaru.
  2. Validasi struktur kelayakan riset:
     - Spesifikasi hook (Hamsandwich / Fakemeta).
     - Alokasi Task ID & Entity budget.
     - Kompatibilitas dengan YaPB Bot / Bot Savage AI.
     - Kebutuhan aset (sound, model, sprite).
  3. Lakukan verifikasi silang (cross-reference) terhadap file lokal di `cstrike/addons/amxmodx/scripting/`.

---

### Action 3: `Action_CreateImplementationPlan`
- **Trigger**: Selesai menganalisis dokumen riset dari `gemini/argon/`.
- **Prosedur**:
  1. Buat implementation plan artifact di Artifact Directory (`<appDataDir>/brain/<conversation-id>/<plan_name>.md`).
  2. Metadata: `RequestFeedback: true`, `UserFacing: true`, `Summary: ...`.
  3. Struktur Plan:
     - **Goal Description**: Deskripsi perubahan & fitur.
     - **User Review Required**: Keputusan desain, breaking changes, resiko memori/channel overflow.
     - **Open Questions**: Pertanyaan konfirmasi ke user.
     - **Proposed Changes**: Komponen, file [NEW] / [MODIFY], cuplikan kode Pawn.
     - **Verification Plan**: Perintah kompilasi `amxxpc.exe` & langkah uji coba dalam game.
  4. Hentikan pemanggilan tool dan tunggu user menekan tombol "Proceed" / memberikan persetujuan chat.

---

### Action 4: `Action_SwarmExecution`
- **Trigger**: User memberikan persetujuan eksplisit terhadap Implementation Plan.
- **Teknologi**: Native Swarm Tool (`swarm(action='add', name='...', brief='...')`).
- **Pembagian Roster & Role Swarm**:

| Worker Name | Model | Brief / Tanggung Jawab |
| :--- | :--- | :--- |
| `pawn-coder` | Inherit | Bertanggung jawab menulis atau mengedit file source code Pawn (`.sma` atau `.inl` di `cstrike/addons/amxmodx/scripting/`). Mematuhi standar sintaks AMXX 1.10 dan partisi Task ID aman. |
| `amxx-compiler` | Inherit | Bertanggung jawab memvalidasi sintaks dengan menjalankan compiler `cstrike/addons/amxmodx/scripting/amxxpc.exe`, menangkap baris warning/error, dan memastikan file `.amxx` terkompilasi bersih tanpa cacat. |
| `config-integrator` | Inherit | Bertanggung jawab mendaftarkan plugin di `plugins-competitive.ini` / `plugins-deathmatch.ini`, mengkonfigurasi file `.cfg`, serta memeriksa jalur precache aset (`models/`, `sound/`, `sprites/`). |

- **Prosedur Koordinasi Swarm**:
  1. Koordinator (Antigravity) menamai swarm via `swarm(action='title', title='CS 1.6 Feature Implementation Swarm')`.
  2. Koordinator menambahkan worker dengan brief yang jelas dan terisolasi.
  3. Koordinator memantau dan memvalidasi pekerjaan seluruh worker hingga kompilasi 100% sukses.
  4. Koordinator membubarkan worker (`swarm(action='kill', ...)`).
  5. Koordinator membuat dokumen akhir `walkthrough.md`.

---

### Action 5: `Action_CleanCodeSession`
- **Trigger**: Deteksi pola chat `^/clean-code-session\b` atau permintaan pengguna untuk refaktorisasi akhir sesi / eliminasi technical debt.
- **Input**: Target file atau konteks file yang baru saja dimodifikasi di sesi aktif.
- **Tujuan**: Mengubah kode menjadi modular, ramping, efisien, bebas dari redundansi, serta membasmi potensi technical debt tanpa mengubah fungsionalitas yang sudah berjalan (*zero behavioral regression*).
- **Prosedur**:
  1. **Audit Dead Code & Redundansi**:
     - Memeriksa macro usang, task ID sisa, header `#include` yang tidak lagi dipanggil, dan variabel global yang tidak diperlukan.
     - Mengubah pola duplikasi logika (misal multiple `while`-loops entity query, repetitive switch-cases) menjadi array-driven loops atau fungsi tersentralisasi.
  2. **Throttling & Resource Protection**:
     - Menerapkan pencegahan task tumpang-tindih dengan `!task_exists(...)` untuk aksi periodik atau operasi peta yang berat.
     - Memastikan pembersihan task defensif pada `client_putinserver` dan `client_disconnected`.
  3. **Sanitasi Logging**:
     - Menghapus panggilan debugging `log_amx` atau console print berulang pada *hot execution paths* (spawn, death, attack, damage, think) untuk mencegah lag disk I/O.
  4. **Modularitas & Arsitektur Bersih**:
     - Mengelompokkan helper functions / stock secara logis di bagian bawah file atau memindahkannya ke sub-modul `.inl` jika kode utama terlalu panjang.
  5. **Verifikasi Kompilasi & Sinkronisasi**:
     - Menjalankan `cstrike/addons/amxmodx/scripting/amxxpc.exe` dari direktori scripting.
     - Memastikan 0 error dan 0 warning.
     - Menyalin file `.amxx` hasil build ke direktori plugins runtime (`cstrike/addons/amxmodx/plugins/` dan root).
