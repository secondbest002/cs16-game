# CS 1.6 / Xash3D FWGS — AMX Mod X 1.10 Architecture

Arsitektur modular Option 1 + Option 2-Lite menyatukan modul kohesif menjadi **7 plugin kustom aktif**, menghilangkan seluruh ketergantungan `callfunc_begin` / `csgomod.inc`, menstandarkan SDK bersama, dan mempartisi Task ID agar bebas tabrakan.

---

## 1. Active Custom Plugins (7 Targets)

| Plugin Binary | Role / Pillar | Source Entrypoint & Submodules | Mode Config |
|---|---|---|---|
| `cs2_arsenal.amxx` | **Pillar 1** — Skins, CT/T Loadout, M4A4/M4A1-S Split, Gunshot FX, Weapon Variants Pricing & Clips | `cs2_arsenal.sma` + `cs2_arsenal/{core,modules}/` | Comp + DM |
| `cs2_modern_engine.amxx` | **Pillar 2** — Kill Feedback/Rewards, Utility 3-Way, E-Pickup, Switchhands, Refunder, USPS, Knife Sounds, Ping | `cs2_modern_engine.sma` + `cs2_engine/` | Comp + DM |
| `bot_savage_ai.amxx` | **Pillar 3** — YaPB Esports AI (Roles, IGL Strats, Combat Tick, Reflex ESP, Economy Drop, Scavenge) | `bot_savage_ai.sma` + `bot_savage/{core,modules}/` | Comp + DM |
| `cs2_client_studio.amxx` | **Pillar 4** — Client UX Hub (`/studio`, `/crosshair`, `/scope`, `/hud`, `/vm`; zero `PreThink`) | `cs2_client_studio.sma` + `cs2_studio/{core,modules}/` | Comp + DM |
| `csgo_molotov.amxx` | **Standalone** — Molotov & Incendiary Grenade (`$500`, `/molotov`, burn/extinguish) | `csgo_molotov.sma` | Comp + DM |
| `csgo_competitive.amxx` | **Gamemode (Comp)** — Warmup DM, `.ready`, Knife Round, MR12, Halftime Swap, Overtime MR3 | `csgo_competitive.sma` + `cs2_competitive/{core,modules}/` | Competitive only |
| `redm_listen_engine.amxx` | **Gamemode (DM)** — Listen Deathmatch (Instant Respawn, JSON Spawns, Weapon Menus, YaPB DM) | `redm_listen_engine.sma` | Deathmatch only |

Catatan aktivasi: `!Start_CS16_Xash3D.bat` menyalin `plugins-competitive.ini` -> `plugins.ini`; `!Start_Deathmatch_Xash3D.bat` menyalin `plugins-deathmatch.ini` -> `plugins.ini`.

---

## 2. Scripting Directory Layout (`cstrike/addons/amxmodx/scripting/`)

```text
scripting/
├── build_all.bat               # Fail-fast build gate untuk 7 plugin aktif (-> ..\plugins\)
├── include/
│   ├── cs2_sdk/                # Single Source of Truth SDK (#pragma semicolon 1 safe)
│   │   ├── cs2_offsets.inc     # GoldSrc / ReGameDLL m_* & extra_offset_* pdata constants
│   │   ├── cs2_weapon_registry.inc # 18 custom knives (CS2_KNIFE_*), sprites, tokens, detect helpers
│   │   ├── cs2_state_cvars.inc # CVar & warmup/competitive state helpers (cs2_is_match_warmup)
│   │   └── cs2_api.inc         # Typed natives + cs2_sdk_native_filter + LibraryExists guards
│   ├── bot_savage_ai.inc       # Thin compatibility wrapper -> cs2_sdk
│   ├── cs2_engine.inc          # Thin compatibility wrapper -> cs2_sdk
│   ├── cs2_weapon_variants.inc # Thin compatibility wrapper -> cs2_sdk
│   └── xash_bot_utils.inc      # Thin compatibility wrapper -> cs2_sdk
├── cs2_arsenal.sma             # + cs2_arsenal/core/{arsenal_taskids,arsenal_defs,arsenal_loadout,arsenal_vault}.inc
│                               # + cs2_arsenal/modules/{mod_skin_models,mod_m4_split,mod_gunshot_fx,mod_weapon_variants,mod_loadout_menu}.inl
├── cs2_modern_engine.sma       # + cs2_engine/{cs2_engine_taskids,cs2_engine_defs}.inc + 10 mod_*.inl
├── bot_savage_ai.sma           # + bot_savage/core/{savage_taskids,savage_defs}.inc + 7 savage_*.inl
├── cs2_client_studio.sma       # + cs2_studio/core/{studio_taskids,studio_defs,studio_client_probe}.inc + 3 mod_*.inl
├── cs2_competitive/            # core/{comp_taskids,comp_defs}.inc + 6 comp_*.inl (included by csgo_competitive.sma)
├── csgo_molotov.sma
├── csgo_competitive.sma
└── redm_listen_engine.sma
```

---

## 3. Shared SDK & Inter-Plugin Native Contracts (`cs2_sdk`)

Seluruh plugin yang memanggil native lintas-plugin memanggil `set_native_filter("cs2_sdk_native_filter")` di `plugin_natives()` dan memeriksa ketersediaan library lewat helper `cs2_api_has_*()` sebelum memanggil native (bebas crash meski satu plugin dimatikan sementara):

- **Pillar 1 (`register_library("cs2_arsenal")`)**: `cs2_is_m4a4_native`, `cs2_get_active_skin_name`, `cs2_get_active_skin_vmodel`, `cs2_get_active_submodel`, `cs2_is_user_holding_variant`, `cs2_get_user_weapon_variant`, `cs2_get_variant_price` (serta shim `public` untuk kompatibilitas). `cs2_api_has_loadout()` & `cs2_api_has_variants()` menerima `"cs2_arsenal"` maupun legacy `"cs2_loadout"`/`"cs2_variants"`.
- **Pillar 2 (`register_library("cs2_engine")`)**: `cs2_refund_player_primary`, `cs2_refund_player_all`, `cs2_can_player_refund`.
- **Pillar 3 (`register_library("bot_savage_ai")`)**: `savage_get_player_role`, `savage_equip_bot_role`.
- **Standalone (`register_library("csgo_molotov")`)**: `csgo_get_user_molotov`.

---

## 4. Task ID Partition Registry

| Partition Range | Owner Plugin | Key Bases / Purpose |
|---|---|---|
| `10000 – 19999` | `cs2_arsenal` + `csgo_molotov` | Arsenal (`10100` ResetHUD, `10200` TeamChange, `10300` BotSync); Molotov (`18100` BotBuy, `18200` Spawn) |
| `20000 – 29999` | `cs2_modern_engine` | Modular gameplay timers (`cs2_engine_taskids.inc`: ping decay, pickup hint, rewards, switchhands, knife, etc.) |
| `30000 – 39999` | `bot_savage_ai` | AI tactical loops (`30009` MidScan, `30010/30011` Calls, `31200+id` RefillAmmo, combat/utility tasks) |
| `40000 – 49999` | `cs2_client_studio` | `40100` CapabilityProbe, `44100` HudSync, `44500` HudSave, `46200` VmDefer, `46300` VmSpawn |
| `50000 – 59999` | Gamemode Slot | Competitive (`50009–50300+id` Half2/OT2/Draw/AutoPick/StripKnife); Deathmatch (`55001–55300` ReDM timers) |

---

## 5. Persistence & External Desktop Companion Contracts

- **nVault Databases** (`cstrike/addons/amxmodx/data/vault/`):
  - `cs2_loadout` (dibuka oleh `cs2_arsenal.amxx`)
  - `cs2_crosshair` & `cs2_viewmodel` (keduanya dibuka terpisah oleh `cs2_client_studio.amxx` tanpa migrasi skema)
- **External Configs & Reload Command**:
  - `configs/skin_inventory.ini` & `configs/active_loadout.ini` dibaca dalam single-pass oleh `arsenal_loadout.inc` saat precache maupun via `cs2_reload_loadout` (`ADMIN_CFG`) / `/reloadloadout` (kini mengembalikan `PLUGIN_HANDLED` dan memperbarui skin + variant pricing/clip sekaligus).
- **Guardrails**: Tidak ada raw `register_clcmd("m4a1", ...)` (mencegah hijacking B-4-3/buy menu); zero `PreThink` pada `cs2_client_studio.amxx`.

---

## 6. Build & Rollback Reference

- **Compile semua plugin**: jalankan `cstrike\addons\amxmodx\scripting\build_all.bat` (wajib 7/7 exit 0, zero `.amxx` di `scripting/`).
- **Arsip & Rollback** (di luar folder repo `cs16-game`):
  - `D:\Counter-Strike 1.6\_archive_amxx\snapshot_option1\`: Snapshot penuh 9 `.amxx` + `plugins*.ini` pasca Option 1.
  - `D:\Counter-Strike 1.6\_archive_amxx\pre_option2lite\`: Source & binary lama sebelum penggabungan Pillar 1 (`cs2_skin_inventory`, `cs2_weapon_variants`) & Pillar 4 (`cs2_custom_hud`, `cs2_viewmodel_studio`).
  - `D:\Counter-Strike 1.6\_archive_amxx\`: Arsip dead/duplicate scripts Stage I.
