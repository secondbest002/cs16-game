# CS 1.6 / Xash3D FWGS — AMX Mod X 1.10 Architecture

Arsitektur modular Option 1 + Option 2-Lite menyatukan modul kohesif menjadi **7 plugin kustom aktif**, menghilangkan seluruh ketergantungan `callfunc_begin` / `csgomod.inc`, menstandarkan SDK bersama, dan mempartisi Task ID agar bebas tabrakan.

---

## 1. Active Custom Plugins (7 Targets)

| Plugin Binary | Role / Pillar | Source Entrypoint & Submodules | Mode Config |
|---|---|---|---|
| `cs2_arsenal.amxx` | **Pillar 1** — Skins, CT/T Loadout, M4A4/M4A1-S Split, Gunshot FX, Weapon Variants Pricing & Clips | `cs2_arsenal.sma` + `cs2_arsenal/{core,modules}/` | Comp + DM |
| `cs2_modern_engine.amxx` | **Pillar 2** — Kill Feedback/Rewards, Utility 3-Way, E-Pickup, Switchhands, Refunder, USPS, Knife Sounds, Ping | `cs2_modern_engine.sma` + `cs2_engine/` | Comp + DM |
| `bot_savage_ai.amxx` | **Pillar 3** — YaPB Esports AI (Roles, IGL Strats, YaPB Bridge, Hazard/Smoke Extinguish, Tactical Molotov, Combat Tick, Reflex ESP, Economy Drop, Scavenge) | `bot_savage_ai.sma` + `bot_savage/{core,modules}/` | Comp + DM |
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
├── bot_savage_ai.sma           # + include/yapb.inc + bot_savage/core/{savage_taskids,savage_defs}.inc + 10 savage_*.inl
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
- **Standalone (`register_library("csgo_molotov")`)**: `csgo_get_user_molotov`, `csgo_give_user_molotov`, `csgo_clear_user_molotov`, `csgo_get_active_molotov_count`, `csgo_get_active_molotov` (terlindung `cs2_api_has_molotov()`).

---

## 4. Task ID Partition Registry

| Partition Range | Owner Plugin | Key Bases / Purpose |
|---|---|---|
| `1000 – 9999` | `cs2_modern_engine` | Modular gameplay timers (`cs2_engine_taskids.inc`: `2000` KillFeedback, `3000` Utility, `4000` SmartPing, `5000` ArmSwitch, `6000` AmmoRefill, `7000` WeaponPickup, `8000` USPS, `9000` KnifeSounds) |
| `10000 – 19999` | `cs2_arsenal` + `csgo_molotov` | Arsenal (`10100` ResetHUD, `10200` TeamChange, `10300` BotSync); Molotov (`18100` BotBuy, `18200` Spawn) |
| `30000 – 39999` | `bot_savage_ai` | AI tactical loops (`30001–30014`: Grenade, Sound, Economy, Enforce, Deliver, Scavenge, ScanWpns, Unfreeze, MidScan, CallT/CT, `30012` HazardScan, `30013` MolotovScan, `30014` RetakeSync; `31000+id` RoleMenu, `31200+id` RefillAmmo) |
| `40000 – 49999` | `cs2_client_studio` | `40100` CapabilityProbe, `44100` HudSync, `44500` HudSave, `46200` VmDefer, `46300` VmSpawn |
| `50000 – 59999` | Gamemode Slot | Competitive (`50001–50012` HUD/Countdown/Warmup/Timeout/SidePick/MatchEnd/Sweep/Half2/OT2/DrawKnife/AutoPick, `50100/50200/50300+id` Respawn/BotEquip/StripKnife); Deathmatch (`55001` Cleanup, `55002` NewRound, `55100/55200/55300+id` Respawn/Protect/EquipMenu) |

---

## 5. Persistence & External Desktop Companion Contracts

- **nVault Databases** (`cstrike/addons/amxmodx/data/vault/`):
  - `cs2_loadout` (`arsenal_vault.inc:113`): key `"%s_%s_%d"` (`<sanitized_auth_or_name>_<ct|t>_<csw_id>`), value = skin display name (`g_SkinName`).
  - `cs2_crosshair` (`mod_crosshair_hud.inl:748`): key = `Studio_BuildPlayerKey(id)`, value = 25 field crosshair/scope/radar/HUD terpisah spasi.
  - `cs2_viewmodel` (`mod_viewmodel_studio.inl:625`): key `"%s#%s"` (`<prefix>#<profile_key>`), value = 6 float (`FOV X Y Z BOB LAG`).
- **External Configs & Reload Command**:
  - `configs/skin_inventory.ini`: `[SECTION_HEADER]` → `"Display Name" "v_model" ["p_model" "w_model"]`.
  - `configs/active_loadout.ini`: `[CT_ACTIVE]` & `[T_ACTIVE]` → `WEAPON_KEY = "models/custom_skins/.../v_*.mdl"`.
  - Dibaca single-pass oleh `arsenal_loadout.inc` saat precache maupun via `cs2_reload_loadout` (`ADMIN_CFG`) / `/reloadloadout` (mengembalikan `PLUGIN_HANDLED` dan memperbarui skin + variant pricing/clip sekaligus).
- **Custom Entities & UserMsg Registry (Anti-Collision)**:
  - Custom classnames: `cs2_dropped_nade` (`mod_modern_utility.inl:323`), `cs2_ping_beacon` (`mod_smart_ping.inl:454`), `molotov` & `molotov_fire` (`csgo_molotov.sma`).
  - Intercepted `register_message`: `DeathMsg` & `ScreenFade` (`mod_kill_feedback.inl:83-84`), `RoundTime` (`csgo_competitive.sma:53`), `RoundTime` & `SendAudio` (`redm_listen_engine.sma:160-161`, hanya aktif di profil Deathmatch).
- **Guardrails**: Tidak ada raw `register_clcmd("m4a1", ...)` (mencegah hijacking B-4-3/buy menu); zero `PreThink` pada `cs2_client_studio.amxx`.

---

## 6. Gamemode State Machine & Cross-Isolation (`csgo_competitive` ↔ `redm_listen_engine`)

- **Competitive MR12 State Machine (`comp_defs.inc:11-23`):**
  `STATE_WARMUP` (`0`, 55m DM, `$16000`, `mp_item_staytime 0`) → `STATE_COUNTDOWN` (`1`, 5s setelah semua `.ready` + auto-ready YaPB) → `STATE_KNIFE_ROUND` (`2`, `bot_savage_kniferound 1`, `$0`) → `STATE_SIDE_PICK` (`3`, `.stay`/`.switch` 15s, auto-stay bot 2s) → `STATE_LIVE_HALF_1` (`4`, MR12: 12 ronde, `$800`, `mp_item_staytime 300`) → `STATE_HALFTIME` (`5`, tukar sisi + `g_ScoreCT <-> g_ScoreT` + reset timeout & loss streak) → `STATE_LIVE_HALF_2` (`6`, first to `g_TargetWinScore = 13`) → jika seri `12-12`: `STATE_OVERTIME_1` (`7`, MR3, `$10000`) → `STATE_OVERTIME_HALFTIME` (`8`) → `STATE_OVERTIME_2` (`9`, target `g_ScoreCT + 4`) → `STATE_FINISHED` (`10`).
- **CS:GO / CS2 Loss Bonus Ladder (`comp_economy_round.inl:131, 243-256`):**
  - Level `0..4` (`$1400`, `$1900`, `$2400`, `$2900`, `$3400`). Pemenang ronde turun `1` level (min `0`), tim kalah naik `1` level (maks `4`), reset ke `0` saat Halftime.
- **Isolasi Silang 2-Lapis (Competitive vs Deathmatch):**
  1. **Hard Profile Separation:** `!Start_CS16_Xash3D.bat` menyalin `plugins-competitive.ini` (hanya `csgo_competitive.amxx` aktif) + `competitive.cfg` (`redm_active 0`, `amx_csgo_mode 1`); `!Start_Deathmatch_Xash3D.bat` menyalin `plugins-deathmatch.ini` (hanya `redm_listen_engine.amxx` aktif) + `deathmatch.cfg` (`amx_csgo_mode 0`, `redm_active 1`, `yb_csdm_mode 2`).
  2. **Runtime CVar Guard (`csgo_competitive.sma:116-188, 212`):** Jika `redm_active == 1` atau `amx_csgo_mode == 0`, `SyncCompetitiveLifecycle(false)` langsung mematikan 4 `HamForward` (`SpawnPost`, `KilledPost`, `DeployGlock`, `DeployUsp`), menghapus task competitive, dan mengosongkan channel HUD. *(Catatan audit: tambahkan `remove_task(TASK_COMP_MATCH_END)` dan broadcast `CS2PlayerStats(0)` / `CS2RoundHist(0)` pada `SyncCompetitiveLifecycle(false)` saat implementasi `BP-04`.)*
- **ReDM Listen Engine (`redm_listen_engine.sma`):** Memuat titik spawn kustom dari `addons/amxmodx/data/redm/<mapname>.spawns.json` dengan fallback spawn map bawaan, respawn `1.2s`, spawn protection `2.0s`, dan pemblokiran akhir ronde via `Msg_RoundTime` & `Msg_SendAudio`.

---

## 7. Build, Automated Verification & Rollback Reference

- **AMXX Compile Gate (`cstrike\addons\amxmodx\scripting\build_all.bat`):**
  - Mengkompilasi 7 plugin aktif secara fail-fast via `amxxpc.exe -iinclude -o..\plugins\%%p.amxx %%p.sma` (wajib `7/7 exit 0`, `0` file `.amxx` tertinggal di `scripting/`).
- **Pre-Flight 4-Gate Validation (`tools\verify_workspace.ps1`, mendukung `-SkipCompile` & `-Strict`):**
  1. **Gate 1 — Strict AMXX Compile:** Kompilasi 7/7 plugin aktif, cek `ExitCode`, `"Error:"`, `"Fatal error:"`, dan hitung `"Warning:"` (gagal jika `-Strict` dan `warnings > 0`).
  2. **Gate 2 — Asset & Precache Integrity:** Validasi keberadaan file model/sound/sprite aktif & katalog (`active_loadout.ini`, `skin_inventory.ini`).
  3. **Gate 3 — CVar Drift Detector:** Cek CVar kustom di file `.cfg` terhadap registrasi source + validasi nilai boolean `0|1`.
  4. **Gate 4 — Profile Parity & SDK Guard:** Cek paritas plugin di `plugins-*.ini`, deteksi binary stale, stray `.amxx` di `scripting/`, dan pemblokiran `callfunc_begin` di seluruh folder kustom.
  - *Catatan Audit:* `verify_workspace.ps1` saat ini hanya mencakup sisi AMXX/game dan belum memeriksa paritas/build `../cs16-client/build/cl_dll/client.dll` → `cstrike/cl_dlls/client.dll` (untuk build C++, ikuti prosedur LLVM-MinGW `i686` di `../cs16-client/AGENTS.md`).
- **Post-Test Log Diagnostics (`tools\check_logs.ps1`):** Memindai log AMXX + `qconsole.log` sesi terakhir dan mengelompokkan anomali ke 4 bucket: `Fatal/Runtime`, `Precache/Missing`, `Init`, dan `Engine` (`SV_StartSound`, `SZ_GetSpace`, `Host_Error`, `Mod_LoadModel`).
- **Launchers (`!Start_CS16_Xash3D.bat` & `!Start_Deathmatch_Xash3D.bat`):** Menyalin `plugins-<mode>.ini` → `plugins.ini`, menulis `cstrike\mode.cfg`, lalu meluncurkan `xash3d.exe -game cstrike -dll addons/metamod/dlls/metamod.dll -fullscreen -high -condebug +log on +mp_logfile 1 +exec <mode>.cfg`.
- **Arsip & Rollback** (di luar folder repo `cs16-game`):
  - `D:\Counter-Strike 1.6\_archive_amxx\snapshot_option1\`: Snapshot penuh 9 `.amxx` + `plugins*.ini` pasca Option 1.
  - `D:\Counter-Strike 1.6\_archive_amxx\pre_option2lite\`: Source & binary lama sebelum penggabungan Pillar 1 (`cs2_skin_inventory`, `cs2_weapon_variants`) & Pillar 4 (`cs2_custom_hud`, `cs2_viewmodel_studio`).
  - `D:\Counter-Strike 1.6\_archive_amxx\`: Arsip dead/duplicate scripts Stage I.
