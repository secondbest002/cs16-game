# Architectural Blueprint — Option 2: Domain-Unified 4-Pillar Modular Suite

**Target Workspace:** `D:\Counter-Strike 1.6\cs16-game`  
**Runtime Stack:** Xash3D FWGS 0.21 (Patched `client.dll`) + Metamod + AMX Mod X 1.10 + YaPB  
**Strategy:** **Unifying by Domain + Strictly Modular Inside** — menyatukan 7 plugin kustom aktif menjadi **4 Pilar `.amxx` tetap + 1 Slot Gamemode (Swap)** dengan 1 pola arsitektur identik di seluruh codebase (`<pillar>.sma` tipis + `core/` + `modules/*.inl` + compile-time switchboard `#define ENABLE_*`), sembari menjaga **100% Zero Regression** terhadap nVault, Desktop Companion, `client.dll`, YaPB, dan plugin/mod AMXX eksternal lainnya.  
**Handoff Target:** Antigravity Agent  

---

## 1. Non-Negotiable Zero-Regression Invariants

Antigravity **WAJIB** menegakkan seluruh invariant berikut di setiap tahap migrasi:

1. **Zero Data Loss pada Seluruh nVault:**
   * Pilar 1 (`cs2_arsenal.amxx`) tetap membuka `nvault_open("cs2_loadout")` dengan format key & data identik `cs2_skin_inventory.sma`.
   * Pilar 3 (`cs2_client_studio.amxx`) membuka **dua handle nVault terpisah secara paralel**:
     * `g_hHudVault = nvault_open("cs2_crosshair")` (identik 100% dengan `cs2_custom_hud.sma`)
     * `g_hVmVault  = nvault_open("cs2_viewmodel")` (identik 100% dengan `cs2_viewmodel_studio.sma`)
     * *Dilarang menggabungkan kedua file `.vault` agar profil pemain yang sudah tersimpan tidak hilang!*
2. **External Desktop Companion Sync Contract (`CS2 Loadout Manager`):**
   * `configs/skin_inventory.ini` dan `configs/active_loadout.ini` tidak boleh berubah path maupun sintaks.
   * `cs2_reload_loadout` (`ADMIN_CFG`) dan `say /reloadloadout` sekarang langsung me-refresh **baik skin maupun weapon variants dalam 1 pass atomik** (otomatis memperbaiki bug dual-registration yang sebelumnya ada di antara `cs2_skin_inventory` dan `cs2_weapon_variants`).
3. **100% Backward-Compatible Native & Public Function Shims (Zero Regression untuk Mod Lain):**
   * Semua native & fungsi `public` yang pernah diekspos oleh plugin lama **WAJIB tetap terdaftar** di pilar barunya agar plugin pihak ketiga / mod AMXX lain tidak rusak:
     * `cs2_arsenal.amxx` mendaftarkan: `cs2_get_active_skin_name`, `cs2_get_active_skin_vmodel`, `cs2_get_active_submodel`, `cs2_is_m4a4` (native + public), `cs2_is_user_holding_variant`, `cs2_get_user_weapon_variant`, `cs2_get_variant_price`, `csgo_get_user_molotov` (jika modul molotov aktif).
     * `cs2_modern_engine.amxx` mendaftarkan: `cs2_refund_player_primary`, `cs2_refund_player_all`, `cs2_can_player_refund`.
     * `bot_savage_ai.amxx` mendaftarkan: `savage_get_player_role`, `savage_equip_bot_role` (+ public `savage_get_bot_role` shim).
4. **Patched `client.dll` Sync & Zero `PreThink` Contract:**
   * `CS2_KILL_*` bitmask flags (`CS2_KILL_HS`..`CS2_KILL_INAIR`) tetap identik.
   * Clamped bounds `viewmodel_fov` `[50.0..75.0]`, `viewmodel_offset_x/y/z` `[-3.5..3.5]`, `cl_bob` `[0.0..0.030]`, `cl_weaponlag` `[0.0..1.50]` serta dual-sync `xhair_*` / `cl_crosshair*` tetap 100% identik dan **Zero `PreThink`**.
5. **Engine Buy Command Safety Trap:**
   * **JANGAN PERNAH** mendaftarkan raw `register_clcmd("m4a1", ...)` (membajak pembelian `B-4-3` engine).
6. **Pawn Memory & Heap Safety (`#pragma dynamic`):**
   * Karena Pilar 1 (`cs2_arsenal.amxx`) menggabungkan tabel skin 3D (`g_SkinVModel[TOTAL_SLOTS][16][128]`), tabel variant, dan molotov, pastikan kompilasi dicek ukuran stack/heap dan tambahkan `#pragma dynamic 32768` bila diperlukan agar tidak pernah memicu `AMX_ERR_STACKERR` / `AMX_ERR_MEMACCESS`.
7. **Strict Plugin Load Order di `plugins-competitive.ini`, `plugins-deathmatch.ini`, dan `plugins.ini`:**
   * Urutan pilar wajib: `cs2_arsenal.amxx` -> `cs2_modern_engine.amxx` -> `bot_savage_ai.amxx` -> `cs2_client_studio.amxx` -> `[Gamemode Slot]`.
   * Urutan ini memastikan saat senjata di-deploy: Skin & Variant diaplikasikan lebih dulu -> lalu Knife Sounds / USP-S Silencer / Kill Feedback membaca model akhir yang benar -> lalu Viewmodel Studio menerapkan FOV/Offset untuk model pisau/M4A4 yang tepat.

---

## 2. Target 4-Pillar Architecture Map

### 2.1 Konsolidasi Runtime (Dari 8 Plugin Custom -> 4 Pilar + 1 Gamemode Slot)

| Slot | Target Binary (`.amxx`) | Gabungan Dari Plugin Lama | Konfigurasi & Storage |
| :--- | :--- | :--- | :--- |
| **Pilar 1: Weapons & Skins** | `cs2_arsenal.amxx` | `cs2_skin_inventory` + `cs2_weapon_variants` + `csgo_molotov` *(switchable via `#define`)* | `skin_inventory.ini`, `active_loadout.ini`, nVault `cs2_loadout` |
| **Pilar 2: Core Mechanics** | `cs2_modern_engine.amxx` | Tetap (`9 modul aktif`; 3 modul retired diarsipkan) | `cs2_modern_engine.cfg` |
| **Pilar 3: Client UX Studio** | `cs2_client_studio.amxx` | `cs2_custom_hud` + `cs2_viewmodel_studio` | nVault `cs2_crosshair` + nVault `cs2_viewmodel` |
| **Pilar 4: Tactical AI** | `bot_savage_ai.amxx` | Tetap (dipecah dari 5.869 LOC -> 7 modul `.inl`) | `bot_savage_roles.cfg` |
| **Gamemode Slot (Swap)** | `gm_competitive.amxx` **ATAU** `gm_deathmatch.amxx` *(opsional pertahankan nama lama `csgo_competitive.amxx` & `redm_listen_engine.amxx` agar 0 perubahan habit)* | `csgo_competitive` (dipecah -> 6 `.inl`) / `redm_listen_engine` | `csgo_competitive.cfg` / `data/redm/*.spawns.json` |

> **Rekomendasi Penamaan Gamemode Slot:** Pertahankan nama binary `csgo_competitive.amxx` dan `redm_listen_engine.amxx` (atau gunakan alias jelas di `plugins-*.ini`), karena semua dokumentasi (`PANDUAN_CSGO_COMPETITIVE.txt`) dan launcher `.bat` sudah terbiasa dengannya — sedangkan 4 pilar utama memakai prefix `cs2_*` / `bot_savage_ai`.

### 2.2 Target Directory Tree

```text
D:\Counter-Strike 1.6\
├── _archive_amxx\                              # Rollback-safe archive (DI LUAR cs16-game)
│   └── [Identik dengan Manifest Fase 0]
│
└── cs16-game\
    ├── ARCHITECTURE.md                         # Master Agent Index (4-Pillar Lookup Table)
    ├── !Start_CS16_Xash3D.bat                  # Copy plugins-competitive.ini -> plugins.ini
    ├── !Start_Deathmatch_Xash3D.bat            # Copy plugins-deathmatch.ini -> plugins.ini
    └── cstrike\addons\amxmodx\
        ├── configs\
        │   ├── plugins-competitive.ini         # Updated -> 4 Pillars + csgo_competitive.amxx
        │   ├── plugins-deathmatch.ini          # Updated -> 4 Pillars + redm_listen_engine.amxx
        │   ├── plugins.ini                     # Synced with active profile
        │   ├── skin_inventory.ini              # UNCHANGED (Desktop Companion)
        │   ├── active_loadout.ini              # UNCHANGED (Desktop Companion)
        │   ├── cs2_modern_engine.cfg           # UNCHANGED
        │   ├── bot_savage_roles.cfg            # UNCHANGED
        │   └── csgo_competitive.cfg            # UNCHANGED
        └── scripting\
            ├── build_all.bat                   # Builds 4 Pillars + 2 Gamemodes -> ../plugins/
            ├── include\
            │   └── cs2_sdk\                    # Shared Single-Source-of-Truth SDK
            │       ├── cs2_offsets.inc
            │       ├── cs2_weapon_registry.inc
            │       ├── cs2_state_cvars.inc
            │       └── cs2_api.inc
            │
            ├── cs2_arsenal.sma                 # [PILAR 1] Master Orchestrator (~260 LOC)
            ├── cs2_arsenal\
            │   ├── core\
            │   │   ├── arsenal_defs.inc        # Shared skin/variant arrays, CVars
            │   │   ├── arsenal_taskids.inc     # 10000 - 10999
            │   │   └── arsenal_loadout.inc     # Single-pass parser skin_inventory.ini + active_loadout.ini
            │   └── modules\
            │       ├── mod_skin_models.inl     # Deploy, CurWeapon, ResetHUD, TeamInfo, nVault "cs2_loadout"
            │       ├── mod_m4_split.inl        # M4A4 vs M4A1-S + Bot M4 preferences (/botm4)
            │       ├── mod_gunshot_fx.inl      # Custom gunshot suppression & decals
            │       ├── mod_weapon_variants.inl # 7 CS2 Variants (pricing, authentic clips, buy intercepts)
            │       ├── mod_loadout_menu.inl    # /loadout, /skin, /knife, /m4, direct variant commands
            │       └── mod_molotov.inl         # Molotov & Incendiary ($500, /molotov) — decoupled from csgomod
            │
            ├── cs2_modern_engine.sma           # [PILAR 2] Master Orchestrator (9 active modules)
            ├── cs2_engine\
            │   ├── core\
            │   │   ├── cs2_engine_defs.inc
            │   │   └── cs2_engine_taskids.inc  # 20000 - 29999 (or keep 1000 - 12999)
            │   └── modules\                    # 9 active .inl modules
            │
            ├── cs2_client_studio.sma           # [PILAR 3] Master Orchestrator (~200 LOC)
            ├── cs2_studio\
            │   ├── core\
            │   │   ├── studio_defs.inc         # HUD + VM enums, bounds, dual-vault handles
            │   │   ├── studio_taskids.inc      # 40000 - 49999
            │   │   └── studio_client_probe.inc # Unified non-blocking capability probe (xhair + viewmodel_fov)
            │   └── modules\
            │       ├── mod_crosshair_hud.inl   # Crosshair, Sniper Scope, Misc HUD/Radar (nVault: cs2_crosshair)
            │       ├── mod_viewmodel_studio.inl# 49 Per-Weapon FOV & 3D Offset (nVault: cs2_viewmodel)
            │       └── mod_studio_hub.inl      # Unified /studio entry menu (routes to Crosshair/Scope/HUD/VM)
            │
            ├── bot_savage_ai.sma               # [PILAR 4] Master Orchestrator (~300 LOC)
            ├── bot_savage\
            │   ├── core\
            │   │   ├── savage_defs.inc
            │   │   └── savage_taskids.inc      # 30000 - 39999
            │   └── modules\                    # 7 modular .inl files
            │
            ├── csgo_competitive.sma            # [GAMEMODE A] Master Orchestrator (~350 LOC)
            ├── cs2_competitive\
            │   ├── core\   (comp_defs.inc, comp_taskids.inc)
            │   └── modules\ (6 modular .inl files)
            │
            └── redm_listen_engine.sma          # [GAMEMODE B] Clean standalone (~825 LOC)
```

---

## 3. Spesifikasi Detail Setiap Pilar Baru

### 3.1 Pilar 1: `cs2_arsenal.sma` (Weapons, Skins, Variants & Molotov)

#### Compile-Time Switchboard (`cs2_arsenal.sma`)
```pawn
#pragma dynamic 32768

#define ENABLE_ARSENAL_SKINS        // Skin Inventory, nVault "cs2_loadout", /loadout UI
#define ENABLE_ARSENAL_M4_SPLIT     // M4A4 vs M4A1-S & Bot M4 management
#define ENABLE_ARSENAL_GUNSHOT_FX   // Custom gunshot audio suppression & decals
#define ENABLE_ARSENAL_VARIANTS     // MAG-7, Negev, Sawed-Off, PP-Bizon, CZ75-Auto, Tec-9, MP7
#define ENABLE_ARSENAL_MOLOTOV      // Molotov & Incendiary Grenade ($500, /molotov)
```

#### Keuntungan Arsitektural Spesifik Pilar 1
1. **Single-Pass `active_loadout.ini` Parser (`core/arsenal_loadout.inc`):**
   * Sebelumnya `cs2_skin_inventory.sma` dan `cs2_weapon_variants.sma` membaca `active_loadout.ini` dua kali secara terpisah (dan `cs2_weapon_variants` me-reparse file dari disk di setiap `HLTV` NewRound!).
   * Sekarang `arsenal_loadout.inc` menyimpan **1 cache in-memory** untuk CT & T active loadout yang langsung dipakai oleh `mod_skin_models.inl` DAN `mod_weapon_variants.inl`. `cs2_reload_loadout` me-refresh kedua subsistem secara instan dan atomik.
2. **Penghapusan `FM_SetModel` / `Ham_Item_Deploy` Race Condition:**
   * Master dispatcher di `cs2_arsenal.sma` mengatur urutan eksekusi secara deterministik:
     * `OnItemDeploy_Post(iWpn)` -> (1) `Arsenal_SkinModels_DeployPost(iWpn)` (set `pev_viewmodel2` & submodel) -> (2) `Arsenal_Variants_DeployPost(iWpn)` (sinkronisasi clip varian) -> (3) `Arsenal_Molotov_DeployPost(iWpn)` (override model jika player memegang molotov di slot HE).
     * `OnSetModel_Pre/Post(iEnt, szModel[])` -> `Arsenal_Molotov_SetModelPre` (PRE, tetap return `FMRES_IGNORED` jika bukan molotov) dan `Arsenal_M4Split_SetModelPost` (POST, world model `w_m4a4.mdl` / custom `w_*`).
3. **Preservasi Total Seluruh CVar, Command, & Native:**
   * **CVars dipertahankan 100%:** `cs2_loadout_enable`, `cs2_m4_auto_silence`, `cs2_m4_block_silencer`, `cs2_loadout_notify`, `cs2_bot_m4_mode`, `amx_cs2_variants`, `amx_cs2_variant_pricing`, `amx_cs2_variant_clips`, `amx_cs2_variant_notify`, `csgo_molotov_enabled`, `csgo_molotov_price`, `csgo_molotov_radius`, `csgo_molotov_firetime`, `csgo_molotov_firedamage`.
   * **Lifecycle Warning untuk `mod_molotov.inl`:** Di `csgo_molotov.sma` lama (baris 102), jika 1 file model/sound molotov hilang, plugin memanggil `set_fail_state(...)` yang mematikan seluruh plugin! **Di `cs2_arsenal.amxx`, DILARANG keras memanggil `set_fail_state` dari dalam `mod_molotov.inl`!** Ubah menjadi `g_bMolotovAssetsReady = false; log_amx(...)` agar jika asset molotov hilang, hanya modul molotov yang nonaktif sementara skin & variants tetap berjalan 100% normal!
   * **Natives terdaftar di `plugin_natives()`:**
     * `register_library("cs2_arsenal");`
     * `cs2_get_active_skin_name`, `cs2_get_active_skin_vmodel`, `cs2_get_active_submodel`, `cs2_is_m4a4`
     * `cs2_is_user_holding_variant`, `cs2_get_user_weapon_variant`, `cs2_get_variant_price`
     * `csgo_get_user_molotov`
     * *(Semua juga tetap mengekspor fungsi `public` dengan nama identik untuk compat.)*

---

### 3.2 Pilar 2: `cs2_modern_engine.sma` (Core CS2 Gameplay Mechanics)

* Sudah memiliki arsitektur orchestrator + `.inl` yang matang.
* **Perubahan terfokus:**
  1. Arsipkan 3 modul mati (`mod_cs2_buymenu.inl`, `mod_cs2_hud.inl`, `mod_silent_reload.inl`) ke `_archive_amxx/` dan hapus blok `#if defined` mati dari `cs2_modern_engine.sma`.
  2. Ganti `cs2_engine/core/cs2_engine_defs.inc` agar meng-include `<cs2_sdk/cs2_offsets>`.
  3. Migrasi 2 `callfunc_begin` di dalam `cs2_engine/modules/`:
     * `mod_kill_reward.inl:58`: `callfunc_begin("cs2_is_user_holding_variant", "cs2_weapon_variants.amxx")` -> panggil native `cs2_is_user_holding_variant()` dari `cs2_sdk/cs2_api.inc` (karena provider-nya sekarang `cs2_arsenal.amxx`!).
     * `mod_smart_ping.inl:703`: `callfunc_begin("savage_get_bot_role", "bot_savage_ai.amxx")` -> panggil native `savage_get_player_role()` dari `cs2_sdk/cs2_api.inc`.
  4. Migrasi deteksi 18 pisau di `mod_knife_sounds.inl` & `mod_kill_feedback.inl` ke `cs2_detect_knife_type()` dari `cs2_sdk/cs2_weapon_registry.inc`.
  5. Ganti lookup CVar `amx_csgo_is_warmup` di `mod_kill_feedback.inl:918` ke `cs2_state_is_warmup()`.

---

### 3.3 Pilar 3: `cs2_client_studio.sma` (HUD, Crosshair, Scope & Viewmodel Studio)

#### Compile-Time Switchboard (`cs2_client_studio.sma`)
```pawn
#define ENABLE_STUDIO_CROSSHAIR_HUD  // Crosshair, Sniper Scope, Misc HUD & Radar (nVault: cs2_crosshair)
#define ENABLE_STUDIO_VIEWMODEL      // 49 Per-Weapon FOV & 3D Offset Live Studio (nVault: cs2_viewmodel)
#define ENABLE_STUDIO_UNIFIED_HUB    // /studio hub menu combining both studios
```

#### Keuntungan Arsitektural Spesifik Pilar 3
1. **Shared Capability Probe (`core/studio_client_probe.inc`):**
   * Saat `client_putinserver(id)`, 1 task terpadu (`TASK_STU_PROBE + id`, delay `1.0s`) melakukan non-blocking probe `xhair_enable`, `cl_crosshairthickness`, dan `viewmodel_fov` sekaligus — mengurangi timer ganda per pemain.
2. **Dual-Vault Zero-Migration Persistence:**
   * `mod_crosshair_hud.inl` tetap baca/tulis ke `g_hHudVault` (`nvault_open("cs2_crosshair")`) dengan key sanitizer identik.
   * `mod_viewmodel_studio.inl` tetap baca/tulis ke `g_hVmVault` (`nvault_open("cs2_viewmodel")`) dengan format `%s#%s` (`steamid#profilekey`) identik.
   * `plugin_end()` menutup keduanya secara aman setelah flush profil yang `dirty`.
3. **Zero `PreThink` & Unified Knife Registry:**
   * Hanya event-driven `CurWeapon` + `Ham_Spawn` (post) untuk viewmodel sync.
   * `mod_viewmodel_studio.inl` menggunakan `cs2_detect_knife_type()` dari `cs2_sdk/cs2_weapon_registry.inc` (menghapus duplikasi 50 baris deteksi pisau).
   * Untuk mendeteksi M4A4 saat `CurWeapon`, gunakan native `cs2_is_m4a4()` dari `cs2_arsenal.amxx` dengan fallback `pev_viewmodel2` substring `"m4a4"` jika `cs2_arsenal` sedang nonaktif.
4. **Commands Preserved 100% + Hub Baru:**
   * Semua command lama tetap aktif: `/crosshair`, `.crosshair`, `crosshair_menu`, `/savecrosshair`, `/crosshair_save`, `save_crosshair`, `/scope`, `.scope`, `/hud`, `.hud`, `/vm`, `.vm`, `/viewmodel`, `/fov`, `.fov`, `vm_menu`.
   * Command hub baru (non-breaking): `say /studio`, `say_team /studio`, `say .studio`, `cs2_studio`.

---

### 3.4 Pilar 4: `bot_savage_ai.sma` (Tactical AI Pillar — Isolated Blast Radius)

* **Mengapa TIDAK digabung dengan `cs2_modern_engine`?**
  * `bot_savage_ai` memiliki `FM_CmdStart` per-tick untuk seluruh bot, 4 periodic scanner task, dan ~5.869 LOC logika AI. Memisahkannya sebagai pilar mandiri menjaga isolasi performa, kemudahan profiling CPU/debug, serta memungkinkan server owner mematikan AI override tanpa menyentuh mekanik CS2.
* **Struktur Internal:** Identik dengan Opsi 1 — dipecah menjadi 7 modul `.inl` di bawah `scripting/bot_savage/modules/`:
  1. `savage_roles_loadout.inl`
  2. `savage_igl_strats.inl`
  3. `savage_esports_chat.inl`
  4. `savage_drop_economy.inl`
  5. `savage_combat_tick.inl`
  6. `savage_reflex_esp.inl`
  7. `savage_scavenge_utils.inl`
* **Migrasi Native:**
  * Ganti `callfunc_begin("cs2_is_m4a4", "cs2_skin_inventory.amxx")` (baris 1559) -> panggil `cs2_is_m4a4()` via `cs2_sdk/cs2_api.inc` (provider `cs2_arsenal.amxx`).
  * Ganti `#include <cs2_engine>` (yang punya hard `#pragma reqlib`) -> `#include <cs2_sdk/cs2_api>` + `set_native_filter` sehingga `bot_savage_ai.amxx` tetap bisa load walaupun `cs2_modern_engine.amxx` dimatikan!

---

### 3.5 Gamemode Slot (Swap via Launcher `.bat`)
* **`csgo_competitive.amxx`**: Dipecah menjadi 6 modul `.inl` di `scripting/cs2_competitive/modules/` (`comp_admin_chat`, `comp_warmup_ready`, `comp_knife_sidepick`, `comp_match_flow`, `comp_economy_round`, `comp_dmg_mvp_hud`).
* **`redm_listen_engine.amxx`**: Tetap clean standalone (~825 LOC), memakai `cs2_sdk/cs2_offsets.inc` dan Task ID eksplisit.

---

## 4. Shared SDK (`include/cs2_sdk/`) & Global Task ID Map

### 4.1 `include/cs2_sdk/` (4 File)
1. `cs2_offsets.inc` — Offsets GoldSrc/Xash3D, bit helpers (`cs2_get_bit/set_bit/rem_bit`), primary/secondary bitmasks, `CS2_KILL_*` flags, dan semua stock `xash_bot_utils`.
2. `cs2_weapon_registry.inc` — Enum `CS2KnifeType` (18 custom knives + default), `cs2_detect_knife_type()`, token & display name lookup.
3. `cs2_state_cvars.inc` — Cached O(1) state queries (`cs2_state_is_warmup`, `is_timeout`, `is_deathmatch`, `is_ffa`, `is_knife_round`, `get_match_round`).
4. `cs2_api.inc` — Seluruh native deklarasi lintas 4 pilar + `cs2_sdk_native_filter` (optional native guard). File `.inc` lama (`cs2_engine.inc`, `cs2_weapon_variants.inc`, `bot_savage_ai.inc`, `xash_bot_utils.inc`) dijadikan thin wrapper agar 0 breaking change bagi plugin lain.

### 4.2 Global Task ID Partition Table (Non-Overlapping Across Entire Suite)

| Pilar / Gamemode | Block Range | Alokasi Sub-Blok |
| :--- | :--- | :--- |
| **Pilar 1: `cs2_arsenal`** | `10000 – 19999` | `10100 (+id)` ResetHUD, `10200 (+id)` TeamChange, `10300` SyncBots, `11000+` Variants, `12000+` Molotov |
| **Pilar 2: `cs2_modern_engine`** | `20000 – 29999` *(atau pertahankan `1000–9999`)* | 1.000-slot per sub-modul (`KF`, `UTIL`, `PING`, `ARM`, `AMMO`, `WP`, `USPS`, `KS`) |
| **Pilar 4: `bot_savage_ai`** | `30000 – 39999` | `30001..30011` global scanners/timers *(termasuk fix `MidLineupScanner` & pisah `CallT`/`CallCT`)*, `31000 (+id)` RoleMenu, `31100 (+id)` SpawnEquip, `31200 (+id)` RefillAmmo *(fix raw id)* |
| **Pilar 3: `cs2_client_studio`** | `40000 – 49999` | `40100 (+id)` UnifiedProbe, `44100 (+id)` HudBatchSync, `44500 (+id)` HudDeferredSave *(fix magic 8888)*, `46200 (+id)` VmDeferSync, `46300 (+id)` VmSpawnSync |
| **Gamemode: `csgo_competitive`** | `50000 – 54999` | `50001..50010` match state timers *(termasuk fix `Half2` & `OT2`)*, `50100 (+id)` Respawn, `50200 (+id)` BotEquip |
| **Gamemode: `redm_listen_engine`** | `55000 – 59999` | `55001` Cleanup, `55002` NewRoundSweep *(fix unassigned)*, `55100 (+id)` Respawn, `55200 (+id)` Protect, `55300 (+id)` EquipMenu |

---

## 5. Perubahan Konfigurasi `plugins-*.ini` (Atomic Lockstep)

Bagian custom di **kedua** `plugins-competitive.ini` dan `plugins-deathmatch.ini` (serta `plugins.ini`) diperbarui secara serentak menjadi:

```ini
; ==============================================================================
; CS2 UNIFIED 4-PILLAR MODULAR SUITE
; ==============================================================================
cs2_arsenal.amxx debug          ; Pilar 1: Skin Inventory, M4A4/M4A1-S, Weapon Variants, Molotov (nVault: cs2_loadout)
cs2_modern_engine.amxx          ; Pilar 2: Core Mechanics (Kill Feedback, Rewards, Utility, Pickup, Switchhands, Ammo, USPS, Knife Sounds, Smart Ping)
bot_savage_ai.amxx              ; Pilar 4: Bot Savage & Predator AI Engine (5-Role, IGL, Chat v3.4, Reflexes)
cs2_client_studio.amxx          ; Pilar 3: Unified HUD, Crosshair, Sniper Scope & Per-Weapon Viewmodel Studio (nVault: cs2_crosshair + cs2_viewmodel)

; ==============================================================================
; GAMEMODE SEPARATION (SWAP VIA LAUNCHER .BAT)
; ==============================================================================
; [Di plugins-competitive.ini]:
csgo_competitive.amxx           ; Active: Warmup DM -> .ready -> Knife Round -> MR12 Match Live
;redm_listen_engine.amxx debug  ; Dormant in Competitive mode

; [Di plugins-deathmatch.ini]:
;csgo_competitive.amxx          ; Dormant in Deathmatch mode
redm_listen_engine.amxx debug   ; Active: Listen DM (Instant Respawn, JSON Spawns, Weapon Menus)
```

> `!Start_CS16_Xash3D.bat` dan `!Start_Deathmatch_Xash3D.bat` **tidak perlu diubah sama sekali** karena keduanya sudah menyalin `plugins-competitive.ini` / `plugins-deathmatch.ini` -> `plugins.ini`.

---

## 6. Antigravity Handoff Execution Runbook (Incremental Migration — Zero Big-Bang Risk)

Jangan menyatukan semua pilar dalam satu commit/sesi sekaligus. Jalankan bertahap dengan verifikasi compile + smoke test di setiap gate:

### Gate 0 — Repository Hygiene & Golden Baseline (0% Risk)
1. Cek referensi `modern_pov*.cfg` di `cstrike/config.cfg` & `userconfig.cfg`.
2. Pindahkan seluruh file mati/duplikat (~80% isi `scripting/`, `backup_standalone/`, `testsuite/`, `ReDeathmatch/`, legacy `csgo_*`, `.amxx` nyasar, `.bak`, backup `client.dll.*`, duplikat `cstrike/dlls/yapb.dll`) ke `D:\Counter-Strike 1.6\_archive_amxx\` (manifest identik dengan Blueprint Opsi 1 Bagian 4).
3. Verifikasi semua plugin aktif saat ini tetap compile bersih (0 error).

### Gate 1 — Shared SDK (`include/cs2_sdk/`)
1. Buat `cs2_offsets.inc`, `cs2_weapon_registry.inc`, `cs2_state_cvars.inc`, `cs2_api.inc`.
2. Jadikan `xash_bot_utils.inc`, `cs2_engine.inc`, `cs2_weapon_variants.inc`, `bot_savage_ai.inc` sebagai thin backward-compat wrappers.

### Gate 2 — Migrasi Pilar 3 (`cs2_client_studio.amxx`) — *Paling Independen*
1. Bangun `cs2_client_studio.sma` + `cs2_studio/` dari `cs2_custom_hud.sma` + `cs2_viewmodel_studio.sma`.
2. Gunakan dual nVault (`cs2_crosshair` & `cs2_viewmodel`) dan `cs2_detect_knife_type()`.
3. Compile -> ganti 2 baris lama di `plugins-competitive.ini` & `plugins-deathmatch.ini` -> `cs2_client_studio.amxx`.
4. **Smoke Test Gate 2:** Masuk game, pastikan crosshair & profil viewmodel lama dari nVault langsung ter-load tanpa reset; uji `/crosshair`, `/scope`, `/hud`, `/vm`, `/studio`.

### Gate 3 — Migrasi Pilar 1 (`cs2_arsenal.amxx`) + Update Pilar 2 (`cs2_modern_engine.amxx`)
1. Bangun `cs2_arsenal.sma` + `cs2_arsenal/` dari `cs2_skin_inventory.sma` + `cs2_weapon_variants.sma` + `csgo_molotov.sma` (lepas `csgomod.inc`; ganti `set_fail_state` molotov jadi soft-disable flag).
2. Pastikan urutan hook `OnItemDeploy_Post` & `OnSetModel` sesuai Bagian 3.1.
3. Update `cs2_modern_engine`: arsipkan 3 modul mati, ganti `callfunc_begin` di `mod_kill_reward.inl` & `mod_smart_ping.inl` ke `cs2_api.inc`, gunakan `cs2_weapon_registry.inc` di `mod_knife_sounds.inl` & `mod_kill_feedback.inl`.
4. Compile `cs2_arsenal.sma` & `cs2_modern_engine.sma` (cek 0 error & stack/heap aman) -> update `plugins-*.ini` (ganti `cs2_skin_inventory`, `cs2_weapon_variants`, `csgo_molotov` -> `cs2_arsenal.amxx debug`).
5. Arsipkan `csgomod.inc` dan 3 `.sma`/`.amxx` lama ke `_archive_amxx/pre_pillar_migration/`.
6. **Smoke Test Gate 3:** Uji `/loadout` (data lama `cs2_loadout.vault` utuh), `/m4a4` vs `/m4a1`, `/botm4`, beli MAG-7/Bizon/CZ75/Tec-9/Negev/Sawed-Off/MP7 (harga refund + clip + kill reward `$900/$600/$100`), beli `/molotov` & lempar, uji `cs2_reload_loadout`, uji suara 18 knife & killfeed custom knife.

### Gate 4 — Modularisasi Pilar 4 (`bot_savage_ai.amxx`) & Gamemode (`csgo_competitive.amxx` + `redm_listen_engine.sma`)
1. Pecah `bot_savage_ai.sma` (5.869 LOC) -> 7 modul `.inl` di `bot_savage/modules/`, migrasikan `callfunc_begin("cs2_is_m4a4")` -> `cs2_is_m4a4()` via `cs2_api.inc`, dan perbaiki Task IDs.
2. Pecah `csgo_competitive.sma` (2.702 LOC) -> 6 modul `.inl` di `cs2_competitive/modules/` dan perbaiki Task IDs.
3. Update `redm_listen_engine.sma` agar memakai `cs2_sdk/cs2_offsets.inc` dan Task ID eksplisit.
4. Buat `scripting/build_all.bat` dan `cs16-game/ARCHITECTURE.md`.
5. **Smoke Test Gate 4 (Full End-to-End):**
   * `error_*.log` kosong (0 runtime errors).
   * Competitive full cycle: Warmup DM -> `.ready` -> Knife Round -> Side Pick -> MR12 Live -> Tactical Timeout -> Halftime -> Bot tactical chat (`drop`, `care a`, `flash me`, `utility?`) -> Refund `R` oleh human & bot.
   * Deathmatch full cycle: Instant respawn, JSON spawns, `/guns`, `/ffa`, grenade blocked.
   * **Modularity Isolation Test:** Comment sementara `ENABLE_ARSENAL_MOLOTOV` atau matikan 1 pilar (`cs2_client_studio.amxx` / `cs2_modern_engine.amxx`) -> seluruh pilar lain wajib tetap boot & berjalan 0 error.

### Rollback Procedure
Simpan snapshot binary lama (`*.amxx`) dan `plugins-*.ini` lama di `D:\Counter-Strike 1.6\_archive_amxx\pre_pillar_migration\`. Karena seluruh nVault (`cs2_loadout`, `cs2_crosshair`, `cs2_viewmodel`) dan seluruh `.ini`/`.cfg` tetap 100% kompatibel mundur, rollback ke 8 plugin lama cukup dengan menyalin ulang `.amxx` & `plugins-*.ini` dalam <10 detik.
