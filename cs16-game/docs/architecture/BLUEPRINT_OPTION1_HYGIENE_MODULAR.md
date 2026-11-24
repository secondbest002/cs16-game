# Architectural Blueprint — Option 1: Clean-Room Hygiene & Strict Modular Decomposition

**Target Workspace:** `D:\Counter-Strike 1.6\cs16-game`  
**Runtime Stack:** Xash3D FWGS 0.21 (Patched `client.dll`) + Metamod + AMX Mod X 1.10 + YaPB  
**Strategy:** Evolusioner, **Zero Binary Consolidation Risk** — mempertahankan 7 plugin kustom aktif + 1 slot gamemode yang sudah terbukti stabil di runtime, sambil membersihkan ~80% file mati/duplikat, memecah 3 monolit raksasa (5.8k, 3.4k, 2.7k LOC) ke pola modular `.inl` seragam, dan menstandarisasi kontrak antar-plugin lewat Shared SDK (`include/cs2_sdk/`).  
**Handoff Target:** Antigravity Agent  

---

## 1. Non-Negotiable Zero-Regression Invariants

Sebelum menyentuh 1 baris kode pun, Antigravity **WAJIB** menjaga seluruh invariant berikut 100% utuh:

1. **External Desktop Companion Sync Contract (`CS2 Loadout Manager`):**
   * Path & format file `cstrike/addons/amxmodx/configs/skin_inventory.ini` dan `cstrike/addons/amxmodx/configs/active_loadout.ini` **TIDAK BOLEH BERUBAH** (termasuk section `[CT_ACTIVE]`, `[T_ACTIVE]`, `[CT_KNIVES]`, `[T_KNIVES]`, `[CT_M4A1]`, dst.).
   * Console command `cs2_reload_loadout` (`ADMIN_CFG`) dan chat command `/reloadloadout` wajib tetap berfungsi dan menyinkronkan baik skin maupun weapon variants.
2. **nVault Database & Key Schema Immutability (Zero Player Data Loss):**
   * `data/vault/cs2_loadout.vault` -> opened via `nvault_open("cs2_loadout")` (`cs2_skin_inventory.amxx`)
   * `data/vault/cs2_crosshair.vault` -> opened via `nvault_open("cs2_crosshair")` (`cs2_custom_hud.amxx`)
   * `data/vault/cs2_viewmodel.vault` -> opened via `nvault_open("cs2_viewmodel")` (`cs2_viewmodel_studio.amxx`)
   * Format key & serialization string tidak boleh diubah sedikit pun.
3. **Patched `client.dll` Sync Contract:**
   * `CS2_KILL_*` bitmask flags (`CS2_KILL_HS`..`CS2_KILL_INAIR`) di `mod_kill_feedback.inl` harus identik bit-per-bit.
   * Client CVar probes & bounds di `cs2_custom_hud.sma` (`xhair_enable`, `xhair_*`, `cl_crosshair*`, `hud_radar_*`) dan `cs2_viewmodel_studio.sma` (`viewmodel_fov` `[50.0..75.0]`, `viewmodel_offset_x/y/z` `[-3.5..3.5]`, `cl_bob` `[0.0..0.030]`, `cl_weaponlag` `[0.0..1.50]`) harus persis sama dengan `client.dll/view.cpp`.
4. **Engine Buy Command Safety Trap:**
   * **JANGAN PERNAH** mendaftarkan raw `register_clcmd("m4a1", ...)` di `cs2_skin_inventory` karena akan membajak pembelian senjata engine CS 1.6 (`B-4-3` / `m4a1`). Gunakan hanya `say /m4a1`, `say_team /m4a1`, `m4a1s`.
5. **YaPB C++ Separation of Concerns:**
   * `bot_savage_micro_override` default `"0"` (YaPB C++ memiliki 100% kendali micro aim/movement/fire pada gun-round).
   * `cs2_custom_hud` dan `cs2_viewmodel_studio` wajib tetap **Zero `PreThink` overhead**.
6. **Atomic Launcher Switch Contract:**
   * `!Start_CS16_Xash3D.bat` menyalin `plugins-competitive.ini` -> `plugins.ini` dan mengeksekusi `competitive.cfg`.
   * `!Start_Deathmatch_Xash3D.bat` menyalin `plugins-deathmatch.ini` -> `plugins.ini` dan mengeksekusi `deathmatch.cfg`.
   * Urutan baris plugin di kedua `.ini` harus tetap dijaga agar urutan hook `Ham_Item_Deploy` & `FM_SetModel` tidak bergeser.
7. **Cross-Mod Resilience (Optional Dependencies):**
   * Setiap plugin harus tetap bisa berjalan **mandiri (graceful fallback)** jika salah satu plugin lain dimatikan di `plugins.ini` (gunakan `MarkNativeAsOptional` / `set_native_filter`, JANGAN hard-fail karena `#pragma reqlib` tanpa fallback).

---

## 2. Audit Temuan Aktual (Baseline Snapshot)

### 2.1 Active Runtime Plugin Topology (Dipertahankan di Opsi 1)
1. `cs2_skin_inventory.amxx debug` — 113 KB / **3.444 LOC** (Monolit #2)
2. `cs2_weapon_variants.amxx` — 31 KB / 904 LOC
3. `cs2_modern_engine.amxx` — 12 KB orchestrator + 9 `.inl` aktif (3 `.inl` nonaktif)
4. `bot_savage_ai.amxx` — 203 KB / **5.869 LOC** (Monolit #1)
5. `cs2_custom_hud.amxx` — 36 KB / 975 LOC
6. `cs2_viewmodel_studio.amxx` — 31 KB / 1.001 LOC
7. `csgo_molotov.amxx` — 16 KB / 664 LOC (masih `#include <csgomod>`)
8. **Gamemode Slot (Mutual Exclusive):**
   * Competitive: `csgo_competitive.amxx` — 86 KB / **2.702 LOC** (Monolit #3)
   * Deathmatch: `redm_listen_engine.amxx debug` — 26 KB / 825 LOC

### 2.2 Defect & Technical Debt yang Harus Dibereskan di Opsi 1
| Kategori | Lokasi Eksak | Masalah | Solusi di Opsi 1 |
| :--- | :--- | :--- | :--- |
| **Triple-Copy Noise** | `scripting/*.sma` + `scripting/backup_standalone/` + `cs2_engine/modules/*.inl` | 102 `.sma` di `scripting/`, >80% mati/duplikat. Pencarian Agent selalu hit 3 salinan berbeda. | Fase 0: Arsipkan semua file non-aktif ke `_archive_amxx/` di luar `cs16-game/`. |
| **Hardcoded `callfunc_begin`** | `bot_savage_ai.sma:1559`, `cs2_skin_inventory.sma:1349`, `mod_smart_ping.inl:703`, `mod_kill_reward.inl:58` | Coupling nama file `.amxx` hardcoded & silent failure jika signature berubah. | Fase 1: Migrasi ke native formal di `include/cs2_sdk/cs2_api.inc` + `MarkNativeAsOptional`. |
| **Dual `cs2_reload_loadout`** | `cs2_skin_inventory.sma:1009` & `cs2_weapon_variants.sma:450` | Keduanya register `concmd("cs2_reload_loadout")`. Jika plugin #1 return `PLUGIN_HANDLED`, plugin #2 tidak terpanggil. | Pastikan `cs2_skin_inventory` return `PLUGIN_CONTINUE` pada reload atau trigger multi-forward `CS2_OnActiveLoadoutReloaded`. |
| **3x Duplikasi Knife Detector** | `mod_knife_sounds.inl:254`, `mod_kill_feedback.inl:467`, `cs2_viewmodel_studio.sma:479` | 18 jenis pisau (`karambit`, `butterfly`, `stiletto`, dll.) di-hardcode 3 kali dengan risiko mismatch. | Fase 1: Konsolidasi ke `include/cs2_sdk/cs2_weapon_registry.inc`. |
| **5x Duplikasi GoldSrc Offsets** | `cs2_engine_defs.inc`, `xash_bot_utils.inc`, `cs2_weapon_variants.sma`, `cs2_viewmodel_studio.sma`, `csgomod.inc` | Definisi `m_pActiveItem`, `m_pPlayer`, `extra_offset_*` tersebar dan beda penamaan. | Fase 1: Konsolidasi ke `include/cs2_sdk/cs2_offsets.inc`. |
| **Legacy `csgomod.inc` Dependency** | `csgo_molotov.sma:7` | Hanya pakai 6 macro (`VERSION`, `get/set/rem_bit`, `VALID_PDATA`, `OFFSET_PLAYER`, `OFFSET_ITEM_LINUX`), tapi narik header CS:GO Mod 388 baris. | Pindahkan macro ke `cs2_offsets.inc` dan hapus `#include <csgomod>`. |
| **Unassigned / Colliding Task IDs** | `bot_savage_ai.sma:810,1051,1054,3329`, `cs2_custom_hud.sma:770`, `cs2_skin_inventory.sma:1294`, `csgo_competitive.sma:1202,1295` | `set_task` tanpa ID atau raw `id` (1..32) dan sharing `TASK_TACTICAL_CALL` untuk CT & T. | Standarisasi enum Task ID per plugin (lihat Bagian 6). |
| **Hard `#pragma reqlib`** | `include/cs2_engine.inc:6-9` | Jika `cs2_modern_engine.amxx` dimatikan, `bot_savage_ai.amxx` gagal load! | Gunakan `plugin_natives()` -> `set_native_filter` / `MarkNativeAsOptional` via `cs2_api.inc`. |
| **Binary/Backup Clutter** | `scripting/*.amxx` (28 file), `scripting/compiled/`, `configs/*.bak`, `cstrike/modern_pov*.cfg`, `cl_dlls/client.dll.*`, `cstrike/dlls/yapb.dll` | Membingungkan Agent dan memicu load binary usang. | Fase 0: Pindahkan ke `_archive_amxx/` & standarisasi `build_all.bat`. |

---

## 3. Target Directory Structure (Option 1)

```text
D:\Counter-Strike 1.6\
├── _archive_amxx\                              # Rollback-safe archive (DI LUAR cs16-game)
│   ├── scripting_legacy_csgo\                  # 20+ csgo_*.sma mati + csgomod.inc
│   ├── scripting_standalone_dupes\             # backup_standalone/ + 10 cs2_*.sma duplikat
│   ├── scripting_redm_reapi\                   # ReDeathmatch.sma, redm_spawns.sma, ReDeathmatch/
│   ├── scripting_testsuite\                    # 24 testsuite/*.sma
│   ├── compiled_artifacts\                     # scripting/*.amxx, scripting/compiled/*, plugins/ mati
│   └── misc_backups\                           # configs/*.bak, modern_pov*.cfg, cl_dlls/*.bak, dlls/yapb.dll
│
└── cs16-game\
    ├── ARCHITECTURE.md                         # Master Agent Index (1-page lookup map)
    ├── !Start_CS16_Xash3D.bat                  # Unchanged
    ├── !Start_Deathmatch_Xash3D.bat            # Unchanged
    └── cstrike\addons\amxmodx\
        ├── configs\                            # Hanya config aktif (0 file .bak / mati)
        ├── plugins\                            # Hanya AMXX base aktif + 8 custom .amxx aktif
        └── scripting\
            ├── build_all.bat                   # Automated compiler -> ../plugins/ (zero-error gate)
            ├── include\
            │   ├── [AMXX 1.10 Standard .inc]
            │   └── cs2_sdk\                    # NEW: Single Source of Truth Shared SDK
            │       ├── cs2_offsets.inc         # Offsets, bitmasks, Xash3D bot helpers
            │       ├── cs2_weapon_registry.inc # 18 Custom Knives + M4A4 + 7 Variants catalog
            │       ├── cs2_state_cvars.inc     # O(1) cached gamemode state queries
            │       └── cs2_api.inc             # Unified Native declarations + Optional Filter
            │
            ├── cs2_skin_inventory.sma          # Thin Orchestrator (~250 LOC)
            ├── cs2_inventory\
            │   ├── core\
            │   │   ├── inv_defs.inc
            │   │   └── inv_taskids.inc
            │   └── modules\
            │       ├── inv_catalog_parser.inl  # Precache, skin_inventory.ini, active_loadout.ini
            │       ├── inv_vault_storage.inl   # nVault "cs2_loadout" load/save/reset
            │       ├── inv_deploy_models.inl   # CurWeapon, Deploy, ResetHUD, TeamInfo
            │       ├── inv_m4_split.inl        # M4A4 vs M4A1-S + Bot M4 preferences (/botm4)
            │       ├── inv_gunshot_fx.inl      # Custom gunshot sound suppression & decals
            │       └── inv_menus_natives.inl   # /loadout UI + Native exports + Compat shims
            │
            ├── cs2_weapon_variants.sma         # Clean standalone (~900 LOC, uses cs2_sdk)
            │
            ├── cs2_modern_engine.sma           # Existing Master Orchestrator (9 active modules)
            ├── cs2_engine\
            │   ├── core\
            │   │   ├── cs2_engine_defs.inc     # Thin wrapper importing cs2_sdk/cs2_offsets.inc
            │   │   └── cs2_engine_taskids.inc
            │   └── modules\                    # HANYA 9 modul aktif (3 retired dipindah ke archive)
            │       ├── mod_kill_feedback.inl
            │       ├── mod_kill_reward.inl
            │       ├── mod_modern_utility.inl
            │       ├── mod_weapon_pickup.inl
            │       ├── mod_arm_switcher.inl
            │       ├── mod_ammo_refill.inl
            │       ├── mod_usps_silencer.inl
            │       ├── mod_knife_sounds.inl
            │       └── mod_smart_ping.inl
            │
            ├── bot_savage_ai.sma               # Thin Orchestrator (~300 LOC)
            ├── bot_savage\
            │   ├── core\
            │   │   ├── savage_defs.inc         # Enums, CVars, global state arrays
            │   │   └── savage_taskids.inc      # Partitioned Task IDs (fix unassigned/collisions)
            │   └── modules\
            │       ├── savage_roles_loadout.inl   # 5-Role assignment, freeze delivery, natives
            │       ├── savage_igl_strats.inl      # IGL strat calls, /role & /strat menus
            │       ├── savage_esports_chat.inl    # v3.4 Chat/Callout engine ('care', utility, intel)
            │       ├── savage_drop_economy.inl    # Interactive drop, refund via cs2_api, spawn claim
            │       ├── savage_combat_tick.inl     # FM_CmdStart, holding angles, counter-strafe, micro
            │       ├── savage_reflex_esp.inl      # Anti-flash, Sound ESP, fake defuse, trade-frag
            │       └── savage_scavenge_utils.inl  # Ground scavenge, mid lineup, weapon/audio helpers
            │
            ├── cs2_custom_hud.sma              # Clean standalone (~975 LOC, fix Task ID 8888)
            ├── cs2_viewmodel_studio.sma        # Clean standalone (~950 LOC, uses cs2_weapon_registry)
            ├── csgo_molotov.sma                # Clean standalone (~660 LOC, decoupled from csgomod.inc)
            │
            ├── csgo_competitive.sma            # Thin Orchestrator (~350 LOC)
            ├── cs2_competitive\
            │   ├── core\
            │   │   ├── comp_defs.inc
            │   │   └── comp_taskids.inc
            │   └── modules\
            │       ├── comp_warmup_ready.inl   # Warmup DM, .ready/.unready, countdown, bot equip
            │       ├── comp_knife_sidepick.inl # Knife round, weapon/buy/drop blocks, stay/switch
            │       ├── comp_match_flow.inl     # MR12/MR15, Halftime, OT MR3, Tactical Timeout
            │       ├── comp_economy_round.inl  # Loss bonus ladder, round start/end, C4 events
            │       ├── comp_dmg_mvp_hud.inl    # Damage summary, MVP card, top HUD, ScreenFade
            │       └── comp_admin_chat.inl     # HandleSay router & admin commands
            │
            ├── redm_listen_engine.sma          # Clean standalone (~825 LOC, uses cs2_sdk)
            └── [AMXX 1.10 Base .sma aktif]     # admin*.sma, cmdmenu, plmenu, mapsmenu, statsx, dll.
```

---

## 4. Phase 0 — Complete Archival & Hygiene Manifest

**Aturan Keras:** Gunakan `move` (pindahkan) ke `D:\Counter-Strike 1.6\_archive_amxx\...`, **JANGAN `delete` permanen**. Verifikasi tidak ada satu pun file yang diarsipkan yang tercantum aktif di `plugins-competitive.ini` atau `plugins-deathmatch.ini`.

### 4.1 File yang Diarsipkan dari `scripting/`
1. **`_archive_amxx/scripting_standalone_dupes/`:**
   * Seluruh folder `scripting/backup_standalone/` (9 file `.sma`)
   * 10 file duplikat root `scripting/` yang sudah masuk `cs2_modern_engine`:
     * `cs2_arm_switcher.sma`, `cs2_kill_feedback.sma`, `cs2_kill_reward.sma`, `cs2_modern_utility.sma`, `cs2_silent_reload.sma`, `cs2_smart_ping.sma`, `cs2_weapon_pickup.sma`, `csgo_ammo_refill.sma`, `csgo_knife_sound_manager.sma`, `csgo_usps_silencer.sma`
   * 3 modul nonaktif (commented out) di `scripting/cs2_engine/modules/`:
     * `mod_cs2_buymenu.inl`, `mod_cs2_hud.inl`, `mod_silent_reload.inl`
   * 3 plugin custom CS2/skin usang tidak aktif:
     * `cs2_faction_loadout.sma`, `cs2_hud_manager.sma`, `team_weapon_skins.sma`
2. **`_archive_amxx/scripting_legacy_csgo/`:**
   * 20 file CS:GO Mod legacy non-aktif:
     * `csgo_accounts.sma`, `csgo_assist_revenge.sma`, `csgo_buymenu.sma`, `csgo_clans.sma`, `csgo_core.sma`, `csgo_inspect.sma`, `csgo_m4_loadout.sma`, `csgo_nades.sma`, `csgo_no_distractions.sma`, `csgo_operations.sma`, `csgo_physics.sma`, `csgo_ranks.sma`, `csgo_runboost.sma`, `csgo_server_menu.sma`, `csgo_smoke.sma`, `csgo_stattrak.sma`, `csgo_transfer.sma`, `csgo_vip_svip.sma`, `csgo_zeus.sma`
   * `scripting/include/csgomod.inc` *(hanya dipindahkan SETELAH `csgo_molotov.sma` selesai dilepas ketergantungannya pada Fase 1)*
   * Config legacy CS:GO Mod mati di `configs/`: `csgo_database_schema.sql`, `csgo_menu.ini`, `csgo_mod.cfg`, `csgo_operations.ini`, `csgo_skins.ini`
   * Vault mati: `data/vault/csgo_operations.vault`
3. **`_archive_amxx/scripting_redm_reapi/`:**
   * `ReDeathmatch.sma`, `redm_spawns.sma`, folder `scripting/ReDeathmatch/` (12 file `.inc`), `include/redm.inc`, `include/redm_version.inc`, `include/rog.inc`, `configs/plugins-redm.ini`, `configs/redm/gamemode_deathmatch.json`
4. **`_archive_amxx/scripting_testsuite/`:**
   * Seluruh folder `scripting/testsuite/` (24 `.sma` + `.ini` + `.cfg`)
5. **`_archive_amxx/compiled_artifacts/`:**
   * Seluruh 28 file `*.amxx` & `cs2_weapon_variants.amx` (0 byte) yang nyasar di root `scripting/`
   * Seluruh isi `scripting/compiled/` (6 file termasuk `test.amxx`)
   * Seluruh `.amxx` non-aktif di `plugins/` yang tidak ada di `plugins-competitive.ini` maupun `plugins-deathmatch.ini` (tetap pertahankan base AMXX opsional seperti `admin_sql.amxx`, `miscstats.amxx`, `restmenu.amxx`, `stats_logging.amxx`, `telemenu.amxx` jika ingin aman, tapi arsipkan semua `csgo_*.amxx` mati, `cs2_*` standalone duplikat, `ReDeathmatch.amxx`, `redm_spawns.amxx`, `team_weapon_skins.amxx`).
6. **`_archive_amxx/misc_backups/`:**
   * `configs/*.bak` (`bot_savage_roles.cfg.bak`, `cs2_modern_engine.cfg.bak`, `csgo_competitive.cfg.bak`, `plugins.ini.bak`)
   * `addons/yapb/conf/yapb.cfg.bak`
   * `cstrike/modern_pov*.cfg` (8 file — sudah diganti `cs2_viewmodel_studio.amxx`) — *cek dulu apakah `config.cfg` atau `userconfig.cfg` mengeksekusi `modern_pov.cfg` sebelum memindahkan!*
   * `cstrike/cl_dlls/client.dll.bak`, `client.dll.orig_vanilla`, `client.dll.pre_hud_patch`, `client.dll.pre_vm_patch`, `menu.dll.bak`
   * `cstrike/dlls/yapb.dll`, `yapb.lib`, `yapb.pdb` *(verifikasi `metamod/plugins.ini` hanya memuat `addons/yapb/bin/yapb.dll` — sudah terverifikasi)*
   * Duplikat `scripting/PANDUAN_CSGO_COMPETITIVE.txt` (sudah ada di `configs/PANDUAN_CSGO_COMPETITIVE.txt`).

---

## 5. Phase 1 — Shared SDK Specification (`include/cs2_sdk/`)

### 5.1 `include/cs2_sdk/cs2_offsets.inc`
Menyatukan semua private data offset GoldSrc/Xash3D, weapon bitmask, bit helper, dan stock `xash_bot_utils.inc`:
* **Offsets:** `XO_WEAPON = 4`, `XO_PLAYER = 5`, `m_pPlayer = 41`, `m_iId = 43`, `m_flNextPrimaryAttack = 46`, `m_flNextSecondaryAttack = 47`, `m_flTimeWeaponIdle = 48`, `m_iClip = 51`, `m_fInReload = 54`, `m_iSpecialReload = 55`, `m_flNextAttack = 83`, `m_bIsDefusing = 99`, `m_pActiveItem = 373`, `VALID_PDATA = 2`, `SVC_WEAPONANIM = 35`.
* **Bit Helpers (untuk `csgo_molotov.sma`):** `cs2_get_bit`, `cs2_set_bit`, `cs2_rem_bit`.
* **Weapon Bitmasks:** `CS2_PRIMARY_WEAPONS_BIT_SUM`, `CS2_SECONDARY_WEAPONS_BIT_SUM`.
* **Killfeed Flags (synced with `client.dll`):** `CS2_KILL_NONE` .. `CS2_KILL_INAIR`.
* **Xash3D Bot Utils:** Serap semua stock dari `xash_bot_utils.inc` (`xash_safe_respawn`, `xash_is_bot_combat_ready`, `xash_find_weapon_ent`, `xash_get_weapon_default_clip`, `xash_destroy_weaponbox`, `xash_force_deploy_weapon`). Pertahankan `include/xash_bot_utils.inc` sebagai thin shim (`#include <cs2_sdk/cs2_offsets>`) demi 100% backward compatibility.

### 5.2 `include/cs2_sdk/cs2_weapon_registry.inc`
Menghilangkan duplikasi 3x deteksi 18 pisau custom & 7 modern weapon variants:
```pawn
enum _:CS2KnifeType
{
    CS2_KNIFE_DEFAULT = 0,
    CS2_KNIFE_KARAMBIT,
    CS2_KNIFE_BUTTERFLY,
    CS2_KNIFE_M9,
    CS2_KNIFE_BAYONET,
    CS2_KNIFE_BOWIE,
    CS2_KNIFE_FALCHION,
    CS2_KNIFE_DAGGERS,
    CS2_KNIFE_STILETTO,
    CS2_KNIFE_TALON,
    CS2_KNIFE_URSUS,
    CS2_KNIFE_SKELETON,
    CS2_KNIFE_FLIP,
    CS2_KNIFE_GUT,
    CS2_KNIFE_HUNTSMAN,
    CS2_KNIFE_NAVAJA,
    CS2_KNIFE_NOMAD,
    CS2_KNIFE_PARACORD,
    CS2_KNIFE_SURVIVAL,
    CS2_KNIFE_MAX
};

// Single canonical detector (urutan cek "m9" SEBELUM "bayonet" wajib dipertahankan!)
stock CS2KnifeType:cs2_detect_knife_type(const szModelPath[]);
stock cs2_get_knife_token(CS2KnifeType:kType, szToken[], maxlen);       // e.g. "knife_karambit"
stock cs2_get_knife_display_name(CS2KnifeType:kType, szName[], maxlen); // e.g. "KARAMBIT"
```
**Konsumen:** `mod_knife_sounds.inl`, `mod_kill_feedback.inl`, `cs2_viewmodel_studio.sma`.

### 5.3 `include/cs2_sdk/cs2_state_cvars.inc`
Helper O(1) dengan static cached CVar pointer untuk query lintas plugin:
* `bool:cs2_state_is_warmup()` -> baca `amx_csgo_is_warmup`
* `bool:cs2_state_is_timeout()` -> baca `amx_csgo_is_timeout`
* `bool:cs2_state_is_deathmatch()` -> baca `redm_active`
* `bool:cs2_state_is_ffa()` -> baca `redm_freeforall`
* `bool:cs2_state_is_knife_round()` -> baca `bot_savage_kniferound`
* `cs2_state_get_match_round()` -> baca `amx_csgo_current_round`

### 5.4 `include/cs2_sdk/cs2_api.inc` (Pengganti Seluruh `callfunc_begin`)
Menyatukan seluruh kontrak native lintas plugin dengan dukungan **Optional Native Resilience** (tidak pakai hard `#pragma reqlib` yang bikin plugin lain crash kalau 1 plugin dimatikan):

| Native Signature | Provider Plugin | Consumer Saat Ini (Migrasi Dari) |
| :--- | :--- | :--- |
| `native bool:cs2_is_m4a4_native(id);` *(+ keep `public cs2_is_m4a4(id)` shim)* | `cs2_skin_inventory.amxx` | `bot_savage_ai.sma:1559` (`callfunc_begin`) |
| `native cs2_get_active_skin_name(id, weapon_id, out[], maxlen);` | `cs2_skin_inventory.amxx` | External / HUD / Variants |
| `native cs2_get_active_skin_vmodel(id, weapon_id, out[], maxlen);` | `cs2_skin_inventory.amxx` | External / Studio |
| `native cs2_get_active_submodel(id, weapon_id);` | `cs2_skin_inventory.amxx` | External |
| `native bool:cs2_is_user_holding_variant(id, weapon_id = 0, const variant[] = "");` | `cs2_weapon_variants.amxx` | `mod_kill_reward.inl:58` (`callfunc_begin`) |
| `native bool:cs2_get_user_weapon_variant(id, weapon_id, buf[], maxlen);` | `cs2_weapon_variants.amxx` | External |
| `native cs2_get_variant_price(weapon_id, const variant[] = "");` | `cs2_weapon_variants.amxx` | External |
| `native cs2_refund_player_primary(id, &cost = 0, name[] = "", maxlen = 0);` | `cs2_modern_engine.amxx` | `bot_savage_ai.sma:2670` |
| `native cs2_refund_player_all(id, &cost = 0);` | `cs2_modern_engine.amxx` | `bot_savage_ai.sma:2660` |
| `native bool:cs2_can_player_refund(id);` | `cs2_modern_engine.amxx` | External |
| `native SavageRole:savage_get_player_role(id);` | `bot_savage_ai.amxx` | `cs2_skin_inventory.sma:1349`, `mod_smart_ping.inl:703` (`callfunc_begin("savage_get_bot_role")`) |
| `native savage_equip_bot_role(id, bool:bFree = true, bool:bUtil = false);` | `bot_savage_ai.amxx` | `csgo_competitive.sma:2068` |
| `native csgo_get_user_molotov(id);` | `csgo_molotov.amxx` | External / BuyMenu |

**Wajib di setiap consumer `plugin_natives()`:**
```pawn
public plugin_natives()
{
    set_native_filter("cs2_sdk_native_filter");
}
// stock cs2_sdk_native_filter(const name[], index, trap) -> return PLUGIN_HANDLED jika !trap untuk native cs2_*/savage_*/csgo_*
```
**Header lama (`cs2_engine.inc`, `cs2_weapon_variants.inc`, `bot_savage_ai.inc`)** tetap dipertahankan sebagai thin wrapper `#include <cs2_sdk/cs2_api>` agar modul eksternal / mod lain tetap bisa di-compile tanpa error.

---

## 6. Phase 2 — Monolith Decomposition Plan (Exact Line-Range Mapping)

### 6.1 Pemecahan `bot_savage_ai.sma` (5.869 LOC -> 7 Modul `.inl`)
Semua modul menggunakan prefix `sv_` / `Savage_` dan compile-time switchboard di `bot_savage_ai.sma` (identik dengan `cs2_modern_engine.sma`).

| Target File | Sumber Baris Saat Ini | Tanggung Jawab Utama | Estimasi LOC |
| :--- | :--- | :--- | :--- |
| `bot_savage/core/savage_defs.inc` | Baris 42–278 | Enum `SavageRole`, `STRAT_*`, semua CVar pointer (`g_pCvar*`), global arrays state | ~260 |
| `bot_savage/core/savage_taskids.inc` | Baris 46–57 (+ fix unassigned) | Enum Task ID terpartisi ketat (lihat Bagian 7) | ~45 |
| `bot_savage_ai.sma` | Baris 1–41, 279–315, 733–885 | Orchestrator: `plugin_natives`, `plugin_init`, `plugin_cfg`, `client_putinserver/disconnected`, event dispatcher | ~280 |
| `modules/savage_roles_loadout.inl` | Baris 316–732, 1268–1288, 1356–1573 | `savage_equip_bot_role`, 5-role loadout enforcement, M4A4/M4A1-S detection via `cs2_is_m4a4_native`, `/role` menu, `OnPlayerSpawnPost` | ~680 |
| `modules/savage_igl_strats.inl` | Baris 886–1267, 1289–1355 | `Event_FreezeTimeStart`, `Event_RoundStart`, `Event_GameRestart`, IGL strat menu (`/strat`) & call announcements | ~460 |
| `modules/savage_esports_chat.inl` | Baris 1574–2422 | v3.4 Esports Chat & Callout Engine (`care [zone]`, popflash/smoke request, objective/C4, squad intel, pacing) | ~850 |
| `modules/savage_drop_economy.inl` | Baris 2423–3794 | `HandleSay` router, interactive weapon drop, buy-period refund via `cs2_refund_player_*`, spawn dropped weapon claim | ~1.350 *(opsional split lagi jadi `savage_drop_request.inl` + `savage_economy_buy.inl` jika >900 LOC)* |
| `modules/savage_combat_tick.inl` | Baris 3795–4556 | `OnCmdStart` (`FM_CmdStart`), holding angles, counter-strafe, jiggle, recoil, triggerbot, knife midbrawl, double-peek | ~760 |
| `modules/savage_reflex_esp.inl` | Baris 4557–4898 | `Task_CheckGrenades` (anti-flash 180°), `Task_SoundESPScanner`, fake defuse ambush, `OnPlayerKilled` trade-frag | ~350 |
| `modules/savage_scavenge_utils.inl` | Baris 4899–5869 | Ground weapon scavenging, mid lineup scanner, bombsite/midpoint init, audio/weapon utility helpers | ~970 |

> **Catatan Krusial Pawn Compiler:** Karena fungsi-fungsi di `bot_savage_ai` saling memanggil (misal `savage_esports_chat.inl` memanggil helper di `savage_scavenge_utils.inl`), letakkan `#include "bot_savage/modules/savage_scavenge_utils.inl"` lebih awal (atau deklarasikan `forward` di `savage_defs.inc`) agar zero compiler warnings/errors.

### 6.2 Pemecahan `cs2_skin_inventory.sma` (3.444 LOC -> 6 Modul `.inl`)

| Target File | Sumber Baris Saat Ini | Tanggung Jawab Utama | Estimasi LOC |
| :--- | :--- | :--- | :--- |
| `cs2_inventory/core/inv_defs.inc` | Baris 29–239 | `TOTAL_SLOTS`, `WeaponCategory`, skin data arrays (`g_Skin*`), CVar pointers, `g_Weapon*Names` | ~220 |
| `cs2_inventory/core/inv_taskids.inc` | Baris 219–222 (+ fix sync bots) | Partisi Task ID `inv_` | ~25 |
| `cs2_skin_inventory.sma` | Baris 1–28, 928–1071 | Orchestrator: `plugin_natives`, `plugin_precache`, `plugin_init`, `plugin_end` | ~220 |
| `modules/inv_catalog_parser.inl` | Baris 240–927 | `skin_inventory.ini` parser, fallback discovery, `active_loadout.ini` ingestion, `FM_PrecacheEvent` | ~680 |
| `modules/inv_vault_storage.inl` | Baris 1072–1209 | `client_putinserver/authorized/disconnected`, nVault `"cs2_loadout"` load/save/reset | ~140 |
| `modules/inv_deploy_models.inl` | Baris 1210–1333 | `OnItemDeploy_Post`, `Event_CurWeapon`, `Event_ResetHUD`, `OnPlayerSpawn_Post`, `Event_TeamInfo`, `Event_RoundStart_Skin` | ~130 |
| `modules/inv_m4_split.inl` | Baris 1334–2106 | CT Bot M4 management (ganti `GetBotRole` -> `savage_get_player_role`), M4A4 vs M4A1-S silencer locking, `FM_SetModel` world drop, `cs2_is_m4a4` | ~770 |
| `modules/inv_gunshot_fx.inl` | Baris 2107–2363 | `FM_UpdateClientData`, `FM_PlaybackEvent`, custom gunfire playback & bullet decals | ~260 |
| `modules/inv_menus_natives.inl` | Baris 2364–3444 | `/loadout`, `/knife`, `/gloves`, `/m4`, `/botm4`, direct variant select cmds, `cs2_reload_loadout` (return `PLUGIN_CONTINUE` + trigger forward), native exports | ~1.080 |

### 6.3 Pemecahan `csgo_competitive.sma` (2.702 LOC -> 6 Modul `.inl`)

| Target File | Sumber Baris Saat Ini | Tanggung Jawab Utama | Estimasi LOC |
| :--- | :--- | :--- | :--- |
| `cs2_competitive/core/comp_defs.inc` | Baris 1–226 | `STATE_*` enum, match score/OT/timeout/damage/MVP globals, CVar pointers, `CHECK_COMPETITIVE_ACTIVE` | ~225 |
| `cs2_competitive/core/comp_taskids.inc` | Baris 14–24 (+ fix restart tasks) | Partisi Task ID `comp_` | ~30 |
| `csgo_competitive.sma` | Baris 227–448, 532–536 | Orchestrator: `plugin_init`, `OnConfigsExecuted`, `OnCvarModeChanged`, `plugin_cfg`, `plugin_end`, client connect/disconnect | ~240 |
| `modules/comp_admin_chat.inl` | Baris 449–531 | `AdminCmd_StartMatch`, `AdminCmd_RestartWarmup`, `HandleSay` router | ~85 |
| `modules/comp_warmup_ready.inl` | Baris 537–1012 | Buy/drop/C4/weaponbox blocks, `SetBotKnifeOnly`, `SafeSetUserTeam`, `Util_ScreenFade`, `.ready`/`.unready`, countdown | ~475 |
| `modules/comp_knife_sidepick.inl` | Baris 1013–1067, 1755–1874 | `StartKnifeRound`, side pick (`stay`/`switch`), bot auto-pick | ~180 |
| `modules/comp_match_flow.inl` | Baris 1068–1437 | `StartLiveMatch`, `TriggerHalftime`, `TriggerOvertime`, `TriggerOTHalftime`, Tactical Timeout & freeze | ~370 |
| `modules/comp_economy_round.inl` | Baris 1438–1754, 1875–2247 | Round events, loss bonus ladder, spawn/killed/damage hooks, Warmup bot equip via `savage_equip_bot_role` | ~690 |
| `modules/comp_dmg_mvp_hud.inl` | Baris 2248–2702 | Damage summary, MVP card, `Task_UpdateHUD` (top bar & C4 timer), `EndMatch` & cleanup | ~455 |

### 6.4 Pembersihan 4 Plugin Standalone Lainnya
* **`cs2_weapon_variants.sma`**: Import `cs2_sdk/cs2_offsets.inc` & `cs2_sdk/cs2_api.inc`. Pastikan `Cmd_ReloadActiveLoadout` juga terpanggil via multi-forward `CS2_OnActiveLoadoutReloaded()` dari `cs2_skin_inventory` (sehingga tidak bergantung pada urutan `PLUGIN_CONTINUE` di `concmd`).
* **`cs2_custom_hud.sma`**: Ganti magic number `id + 8888` (baris 770-771) menjadi `#define TASK_DEFERRED_SAVE 44500`.
* **`cs2_viewmodel_studio.sma`**: Hapus fungsi lokal `DetectKnifeProfileFromModel` (baris 475-520) dan delegasikan ke `cs2_detect_knife_type()` dari `cs2_weapon_registry.inc` (mapping langsung `PROFILE_KNIFE_KARAMBIT = 31 + _:kType`).
* **`csgo_molotov.sma`**: Ganti `#include <csgomod>` dengan `#include <cs2_sdk/cs2_offsets>` dan definisikan `#define MOLOTOV_VERSION "2.5.12"` lokal.
* **`redm_listen_engine.sma`**: Ganti `#include <xash_bot_utils>` -> `#include <cs2_sdk/cs2_offsets>`, beri Task ID eksplisit `TASK_NEWROUND_SWEEP = 5500` untuk `Task_NewRoundSweep` (baris 179).

---

## 7. Global Task ID Registry (Zero Collision Standard)

Meskipun Task ID di AMXX terisolasi per plugin, standarisasi blok global mencegah tabrakan saat refactor atau penyatuan di masa depan:

| Plugin | Range Blok | Daftar Task ID Eksplisit |
| :--- | :--- | :--- |
| `cs2_skin_inventory` | `10000 – 10999` | `TASK_INV_RESET_HUD = 10100 (+id)`, `TASK_INV_TEAM_CHANGE = 10200 (+id)`, `TASK_INV_SYNC_BOTS = 10300` *(fix unassigned line 1294)* |
| `cs2_modern_engine` | `20000 – 29999` *(atau pertahankan `1000–12999` yang sudah rapi)* | Sudah terpartisi ketat di `cs2_engine_taskids.inc` (per 1.000 per modul) |
| `bot_savage_ai` | `30000 – 39999` | `TASK_SV_GRENADE = 30001`, `TASK_SV_SOUND = 30002`, `TASK_SV_ECONOMY = 30003`, `TASK_SV_ENFORCE = 30004`, `TASK_SV_DELIVER = 30005`, `TASK_SV_SCAVENGE = 30006`, `TASK_SV_SCAN_WPNS = 30007`, `TASK_SV_UNFREEZE = 30008`, `TASK_SV_MID_SCAN = 30009` *(fix line 810)*, `TASK_SV_CALL_T = 30010`, `TASK_SV_CALL_CT = 30011` *(fix collision line 1051/1054)*, `TASK_SV_ROLE_MENU = 31000 (+id)`, `TASK_SV_SPAWN_EQUIP = 31100 (+id)`, `TASK_SV_REFILL_AMMO = 31200 (+id)` *(fix raw id line 3329)* |
| `cs2_custom_hud` | `44000 – 44999` | `TASK_HUD_SYNC = 44100 (+id)`, `TASK_HUD_SAVE = 44500 (+id)` *(fix magic 8888)* |
| `cs2_viewmodel_studio` | `46000 – 46999` | `TASK_VM_PROBE = 46100 (+id)`, `TASK_VM_DEFER = 46200 (+id)`, `TASK_VM_SPAWN = 46300 (+id)` |
| `csgo_competitive` | `50000 – 50999` | `TASK_COMP_HUD = 50001`, `TASK_COMP_COUNTDOWN = 50002`, `TASK_COMP_WARMUP = 50003`, `TASK_COMP_TIMEOUT = 50004`, `TASK_COMP_SIDEPICK = 50005`, `TASK_COMP_MATCH_END = 50006`, `TASK_COMP_SWEEP1 = 50007`, `TASK_COMP_SWEEP2 = 50008`, `TASK_COMP_HALF2 = 50009` *(fix line 1202)*, `TASK_COMP_OT2 = 50010` *(fix line 1295)*, `TASK_COMP_RESPAWN = 50100 (+id)`, `TASK_COMP_BOTEQUIP = 50200 (+id)` |
| `redm_listen_engine` | `55000 – 55999` | `TASK_DM_CLEANUP = 55001`, `TASK_DM_NEWROUND = 55002` *(fix line 179)*, `TASK_DM_RESPAWN = 55100 (+id)`, `TASK_DM_PROTECT = 55200 (+id)`, `TASK_DM_EQUIP = 55300 (+id)` |

---

## 8. Build Pipeline (`scripting/build_all.bat`) & `ARCHITECTURE.md`

### 8.1 `scripting/build_all.bat`
* Mengkompilasi seluruh 8 plugin kustom aktif secara eksplisit menggunakan `amxxpc.exe -iinclude -o..\plugins\<name>.amxx <name>.sma`.
* **Fail-fast gate:** Jika `ERRORLEVEL NEQ 0`, langsung stop dan tampilkan plugin yang gagal (mencegah deploy parsial).
* **Cleanliness guarantee:** Tidak pernah meninggalkan `.amxx` di folder `scripting/`.

### 8.2 `cs16-game/ARCHITECTURE.md`
File indeks ringkas (<150 baris) di root `cs16-game/` yang berisi:
* Tabel *Feature -> Plugin -> Sub-module `.inl` -> Config/nVault -> Key Commands*.
* Daftar seluruh Native API di `cs2_sdk/cs2_api.inc`.
* Cara compile (`build_all.bat`) dan cara launch mode Competitive vs Deathmatch.

---

## 9. Antigravity Handoff Execution Runbook & Smoke-Test Matrix

Eksekusi **harus** dijalankan berurutan dan diverifikasi di setiap gate:

1. **Gate 0 — Backup & Hygiene:**
   * Periksa `cstrike/config.cfg` dan `cstrike/userconfig.cfg` terkait referensi `modern_pov*.cfg`.
   * Pindahkan seluruh item Bagian 4 ke `D:\Counter-Strike 1.6\_archive_amxx\`.
   * Buat `scripting/build_all.bat` dan jalankan -> pastikan seluruh 8 plugin aktif saat ini ter-compile dengan **0 error** sebelum refactor dimulai (golden baseline).
2. **Gate 1 — Shared SDK & `callfunc_begin` Removal:**
   * Buat 4 file di `include/cs2_sdk/`.
   * Lepas `csgomod.inc` dari `csgo_molotov.sma`, lalu arsipkan `csgomod.inc`.
   * Ganti 4 `callfunc_begin` di `bot_savage_ai`, `cs2_skin_inventory`, `mod_smart_ping`, `mod_kill_reward` ke native `cs2_api.inc` + `set_native_filter`.
   * Ganti deteksi knife di `mod_knife_sounds`, `mod_kill_feedback`, `cs2_viewmodel_studio` ke `cs2_weapon_registry.inc`.
   * Jalankan `build_all.bat` -> wajib **0 error, 0 tag mismatch**.
3. **Gate 2 — Modularisasi Monolit (Satu per Satu):**
   * Urutan: (a) `cs2_skin_inventory.sma` -> compile & cek; (b) `bot_savage_ai.sma` -> compile & cek; (c) `csgo_competitive.sma` -> compile & cek.
   * Perbaiki unassigned Task IDs di semua plugin.
   * Tulis `cs16-game/ARCHITECTURE.md`.
4. **Gate 3 — Verification & In-Game Smoke Test:**
   * Cek log `cstrike/addons/amxmodx/logs/error_*.log` -> **0 runtime errors, 0 bad load**.
   * **Competitive Mode (`!Start_CS16_Xash3D.bat`):**
     * Warmup DM aktif -> bot spawn dengan role loadout -> tidak ada C4 / weaponbox leak.
     * Ketik `.ready` -> countdown -> Knife Round (hanya pisau, buy/drop terblokir) -> Side Pick -> MR12 Live.
     * Uji `/loadout`, `/m4a4` vs `/m4a1`, beli MAG-7/Bizon/CZ75 (harga & clip benar, kill reward sesuai), beli `/molotov`, tekan `R` refund di buyzone, tekan `E` pickup senjata, switchhands, smart ping, ketik `drop` / `care a` ke bot AI.
     * Uji `/crosshair`, `/hud`, `/vm` (profil tersimpan & bertahan setelah reconnect/restart map).
   * **Deathmatch Mode (`!Start_Deathmatch_Xash3D.bat`):**
     * `redm_listen_engine` aktif, `csgo_competitive` dormant.
     * Instant respawn, JSON spawn points (`de_dust2`), `/guns`, `/ffa`, grenade buy terblokir, bot tidak terjebak pistol default.
   * **Modularity Isolation Test:** Nonaktifkan sementara 1 plugin (misal `cs2_modern_engine.amxx`) di `plugins.ini` -> pastikan `bot_savage_ai.amxx` dan plugin lain tetap load normal tanpa error.

### Rollback Procedure
Jika ditemukan regresi di gate mana pun: kembalikan file `.sma`/`.amxx` dari `D:\Counter-Strike 1.6\_archive_amxx\` dan jalankan ulang `amxxpc.exe` pada sumber asli. Tidak ada database nVault atau config `.ini` yang termodifikasi secara destruktif.
