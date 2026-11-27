# TEKNIKAL KONTEKS: KUSTOMISASI HUD (CROSSHAIR & SCOPE) - CS 1.6 / XASH3D ENGINE

Dokumen ini disusun sebagai baseline arsitektur game dan teknikal konteks untuk **Gemini 4 Argon**. Dokumen ini merinci arsitektur mesin game saat ini, file-file konfigurasi UI yang telah terdeteksi, serta batasan teknis antara client-side rendering dan server-side AMX Mod X logic.

---

## 1. DESKRIPSI FITUR & REFERENSI VISUAL PENGGUNA

Pengguna meminta implementasi fitur **Kustomisasi HUD (Crosshair & Scope)** yang persis seperti pengaturan HUD client-side pada CS 1.6 / GoldSrc (sebagaimana terlihat pada gambar dialog Options -> tab `HUD`):

### A. Sub-Tab 1: `Crosshair - Basic`
- **Interactive Preview Box**: Menampilkan preview real-time crosshair berlatar belakang tekstur map (misal: de_dust2 wall).
- **Style Selector**: Dropdown pilihan style (Classic Static, Classic Dynamic, Default, Dot, T-Shape, dll.).
- **Slider Parameter Utama**:
  1. `Size` (Panjang garis): Rentang `0` hingga `20` (default ~`7`).
  2. `Thickness` (Ketebalan garis): Rentang `0` hingga `20` (default ~`0` / `1`).
  3. `Gap` (Jarak celah tengah): Rentang `-30` hingga `30` (default ~`-1`).
  4. `Opacity / Alpha` (Transparansi): Rentang `0%` hingga `100%` (0 - 255).
  5. `Outline` (Garis tepi hitam): Rentang `0` hingga `3` (ketebalan outline 0.5 - 3.0 px) untuk kontras di latar terang.
- **Toggles / Checkboxes**:
  - `Dot`: Titik tengah crosshair.
  - `T-Shape`: Menghilangkan garis vertikal bagian atas crosshair (gaya T-crosshair modern).
  - `Gap based on current weapon`: Celah menyesuaikan recoil/spread senjata yang sedang dipegang.
  - `Recovery time depends on FPS`: Sinkronisasi waktu pemulihan crosshair terhadap frame rate engine.
- **Color Selector**: Dropdown / RGB color picker / preset warna (`#vgui_select`, Green, Red, Blue, Yellow, Cyan, Custom RGB).
- **Reset Button**: `Use Defaults`.

### B. Sub-Tab 2: `Crosshair - Sniper Rifles` (Scope Customization)
- **Scope Line Thickness**: Slider ketebalan garis crosshair teropong sniper (`1` hingga `10`).
- **Scope Style**: Pilihan style scope (Classic Static, Modern CS2 style, Dynamic Blur saat bergerak, Center Dot Only).
- **Sniper Bobcycle**: Toggle/slider efek goyangan (sway/bob) reticle saat berjalan sambil membuka scope.

### C. Sub-Tab 3: `Misc`
- **HUD Color**: Color picker global untuk warna teks dan elemen HUD (`hud_color` / `con_color`).
- **HUD Radar Options**: Bentuk radar (lingkaran vs kotak modern `hud_radar_square`), orientasi (rotasi pemain vs utara statis), dan transparansi.

---

## 2. ARSITEKTUR ENGINE & DETEKSI LOKAL PROYEK

Audit terhadap direktori kerja (`D:\Counter-Strike 1.6\Counter-Strike 1.6 Original`) menghasilkan temuan penting berikut:

### A. Environment Aktif Saat Ini
- **Engine**: Xash3D FWGS 0.21 (32-bit `xash3d.exe`, `xash.dll`, `ref_gl.dll`, SDL2).
- **Server Module**: Metamod-r v1.3.0.149 + ReGameDLL 5.30.0.1-dev + AMX Mod X 1.10.0.5217.
- **Bot Engine**: YaPB v4.4.957 + `bot_savage_ai.sma` (v3.4.0 Esports AI).
- **Client DLL Aktif**: `cstrike/cl_dlls/client.dll` (CS16Client ver. 4027).
- **Menu DLL Aktif**: `menu.dll` (Xash3D FWGS Extended Menu API).

### B. Asset VGUI2 & Binary Eksisting yang Sudah Tersedia
Di dalam workspace telah ditemukan file-file layout resource dialog VGUI2 yang **identik 100% dengan screenshot pengguna**:
- `cstrike/resource/OptionsSubHudCrosshairPage.res`: Form lengkap Slider Size, Thickness, Gap, Alpha, Outline, ComboBox Style, Checkbox Dot, T-Shape, FPS, Weapon Gap.
- `cstrike/resource/OptionsSubHudCrosshairSniperPage.res`: Form Scope Thickness, Scope Style, Bobcycle Checkbox.
- `cstrike/resource/OptionsSubHudMiscPage.res`: Form HUD Color picker dan Radar config.

Selain itu, di dalam `valve/cl_dlls/GameUI.dll` dan backup GoldSrc client (`_backup_goldsrc_binaries/cstrike/cl_dlls/client.dll`), terdaftar cvar-cvar native client berikut:
```c
// Crosshair CVars
cl_crosshairsize                   // Panjang crosshair (float)
cl_crosshairthickness              // Ketebalan crosshair (float)
cl_crosshairgap                    // Gap/jarak antar garis (float)
cl_crosshairusealpha               // Toggle transparansi (0/1)
cl_crosshairalpha                  // Nilai transparansi (0-255)
cl_crosshair_drawoutline           // Toggle outline hitam (0/1)
cl_crosshair_outlinethickness      // Ketebalan outline (float 0.5 - 3.0)
cl_crosshairdot                    // Titik tengah (0/1)
cl_crosshair_t                     // T-shape crosshair (0/1)
cl_crosshairstyle                  // Style dynamic/static (0-5)
cl_crosshaircolor                  // Index warna atau format RGB
cl_crosshairgap_useweaponvalue     // Dynamic gap berbasis akurasi senjata (0/1)
cl_crosshair_recoverytime_on_fps   // Recovery time fps scale (0/1)

// Sniper Rifle Scope CVars
cl_crosshair_sniper_width          // Ketebalan garis scope sniper (int 1-10)
cl_crosshair_sniper_style          // Gaya scope (classic/dynamic)

// HUD CVars
hud_color                          // RGB HUD color string "R G B"
hud_radar_square                   // Radar kotak (0/1)
hud_radar_scale                    // Skala zoom radar
```

---

## 3. KONTRAKSI ARSITEKTUR & DILEMA TEKNIS

1. **Pemisahan Client-Side vs Server-Side**:
   - Pada CS 1.6 standar, crosshair digambar oleh fungsi internal `client.dll` (`CHudAmmo::DrawCrosshair` dan `CHudSniperScope::Draw`).
   - Server-side AMX Mod X (Pawn) tidak memiliki akses render primitif DirectX/OpenGL client secara langsung selain mengirim pesan jaringan (`SVC_DIRECTOR`, `show_hudmessage`, `CreateHudSyncObj`, screen fade, atau sprite `TE_SPRITE`).
   - Penggambaran crosshair via `show_hudmessage` server-side memiliki keterbatasan: batas 4-8 channel HUD bersamaan, potensi kedip (flicker) jika tickrate tidak 1:1, dan latency delay jaringan.

2. **Dua Mode Menjalankan Game**:
   - Mode 1: **Xash3D Engine** (`!Start_CS16_Xash3D.bat`) -> Menjalankan `xash3d.exe` dengan `CS16Client` (`cl_dlls/client.dll`). Saat ini `client.dll` milik CS16Client belum tentu mengekspos semua cvar kustom milik GoldSrc NextClient/GSClient secara default.
   - Mode 2: **GoldSrc Engine** (`_backup_goldsrc_binaries/!Start_Competitive_GoldSrc.bat`) -> Menjalankan binary asli GoldSrc di mana `GameUI.dll` dan `client.dll` sudah memiliki dukungan VGUI2 dialog HUD dan cvar lengkap.

---

## 4. FOKUS RISET YANG DIBUTUHKAN DARI GEMINI 4 ARGON

Gemini 4 Argon diminta untuk menganalisis dan merancang solusi menyeluruh yang mencakup:

1. **Analisis Komparatif Jalur Implementasi**:
   - **Opsi A (Native Client / VGUI2 Bridge)**: Bagaimana menghubungkan VGUI resource files yang sudah ada (`OptionsSubHud*.res`) ke dalam GameUI / CS16Client / Xash3D agar menu options tersebut aktif dan mengatur cvar yang benar.
   - **Opsi B (Server-Side AMXX Smart In-Game System)**: Pembuatan plugin AMXX (`cs2_custom_hud.sma` / `cs2_crosshair_manager.sma`) yang menyediakan menu in-game interaktif (`say /crosshair`, `say /scope`, `say /hud` atau via console) dengan penyimpanan profil per-pemain menggunakan **nvault**. Plugin ini mengeksekusi `query_client_cvar` dan `client_cmd` untuk mengatur cvar kustom client secara real-time, serta memberikan visual fallback jika client belum mendukung cvar tertentu.
   - **Opsi C (Hybrid Solution)**: Kombinasi terbaik di mana pengguna bisa mengatur HUD baik melalui dialog Options bawaan maupun menu server AMXX.

2. **Spesifikasi Algoritma & Formula Kustomisasi**:
   - Bagaimana rumus matematika scaling ukuran crosshair (`size`, `thickness`, `gap`) dan kalkulasi dynamic spread saat menembak, bergerak, atau diam.
   - Mekanisme rendering reticle sniper scope: opsi mengubah sprite scope (`scope_arc_*.spr`), memodifikasi ketebalan garis crosshair sniper, atau menggambar reticle CS2 modern tanpa sudut hitam.

3. **Manajemen Persistensi & UX**:
   - Desain skema nvault (`cs2_crosshair.nvault`) untuk menyimpan konfigurasi per SteamID / AuthID / IP pemain.
   - Auto-load preferensi saat pemain `client_putinserver`.
   - Command cepat (`.crosshair`, `/crosshair`, `bind F4 "crosshair_menu"`).

4. **Kompatibilitas & Safety Guardrails**:
   - Menjaga nol dampak buruk terhadap `bot_savage_ai.sma` dan `csgo_competitive.sma`.
   - Menghindari channel overflow atau buffer overrun pada pengiriman cvar ke client.
