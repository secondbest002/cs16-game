# Permanent Agent Memory Contract — CS 1.6 / Xash3D

> **Doc ID:** `AGENTS.md` | **Target:** `D:\cs16\cs16-game` (Paired Client Contract: `../cs16-client/AGENTS.md`)  
> **Source of Truth:** Selaras dengan `ARCHITECTURE.md` (7 Plugin Kustom Aktif + `cs2_sdk`).  
> **Rules:** Wajib dibaca sebelum modifikasi kode. Dilarang membuat folder `.agents/`.

---

## 1. Runtime Topology & 7 Active Custom Plugins

* **Engine Stack:** Xash3D FWGS 0.21 (`xash3d.exe`) + Patched `client.dll` (`../cs16-client/cl_dll/`) + Metamod 1.21-am + AMX Mod X 1.10 + YaPB (`addons/yapb/bin/yapb.dll`).
* **Launchers & Profiles:**
  * Competitive: `!Start_CS16_Xash3D.bat` -> salin atomik `plugins-competitive.ini` -> `plugins.ini` + load `competitive.cfg`.
  * Deathmatch: `!Start_Deathmatch_Xash3D.bat` -> salin atomik `plugins-deathmatch.ini` -> `plugins.ini` + load `deathmatch.cfg`.
* **7 Target Plugin Aktif (Build via `scripting/build_all.bat` -> `..\plugins\`):**
  1. `cs2_arsenal.amxx` (**Pillar 1**): `cs2_arsenal.sma` + `cs2_arsenal/{core,modules}/` (Skins, Loadouts, M4A4/M4A1-S Split, Gunshot FX, Weapon Variants).
  2. `cs2_modern_engine.amxx` (**Pillar 2**): `cs2_modern_engine.sma` + `cs2_engine/` (Kill feedback, Kill rewards, Utility 3-way, E-pickup, Switchhands, Ammo refunder, USPS silencer, Knife sounds, 3D smart ping).
  3. `bot_savage_ai.amxx` (**Pillar 3**): `bot_savage_ai.sma` + `bot_savage/{core,modules}/` (10 modul `.inl`: YaPB Bridge, Hazard/Smoke Extinguish, Tactical Molotov, Roles, IGL Strats, Combat tick, Reflex ESP, Scavenge/drop, Esports chat).
  4. `cs2_client_studio.amxx` (**Pillar 4**): `cs2_client_studio.sma` + `cs2_studio/{core,modules}/` (Crosshair HUD, Scope, Viewmodel studio, Zero `PreThink`).
  5. `csgo_molotov.amxx` (**Standalone**): `csgo_molotov.sma` ($500 buy, burn/extinguish logic).
  6. `csgo_competitive.amxx` (**Gamemode Comp**): `csgo_competitive.sma` + `cs2_competitive/{core,modules}/` (Warmup DM, `.ready`, Knife round, MR12, Overtime MR3).
  7. `redm_listen_engine.amxx` (**Gamemode DM**): `redm_listen_engine.sma` (Listen Deathmatch, JSON spawns, instant respawn).
* **Shared SDK (`scripting/include/cs2_sdk/`):**
  * `cs2_api.inc`: Typed natives, library guards, dan native filter `cs2_sdk_native_filter` (**0 `callfunc_begin`**).
  * `cs2_offsets.inc`, `cs2_weapon_registry.inc`, `cs2_state_cvars.inc`.

---

## 2. Feature -> Source File Lookup & Task ID Registry

| Feature / Sub-System | Source Entrypoint / Submodule | Task ID Range | Key Task Bases |
|---|---|---|---|
| Skin Models & M4 Split | `cs2_arsenal/modules/mod_skin_models.inl`, `mod_m4_split.inl` | `10000–10999` | `10100` ResetHUD, `10200` TeamChange, `10300` BotSync |
| Gunshot FX & Variants | `cs2_arsenal/modules/mod_gunshot_fx.inl`, `mod_weapon_variants.inl` | `11000–11999` | `11100` VariantCheck, `11200` BulletFX |
| Molotov Grenade | `csgo_molotov.sma` | `18000–18999` | `18100` BotBuy, `18200` Spawn |
| Kill Feedback & Rewards | `cs2_engine/modules/mod_kill_feedback.inl`, `mod_kill_reward.inl` | `2000–2999` | `2001` C4_10s, `2100+id` HudClear |
| Utility 3-Way & Smart Ping | `cs2_engine/modules/mod_modern_utility.inl`, `mod_smart_ping.inl` | `3000–4999` | `3001` Freeze, `3100+id` Buy, `4001` PingHud, `4100+id` PingExpire |
| E-Pickup, Switchhands, USPS | `cs2_engine/modules/mod_weapon_pickup.inl`, `mod_arm_switcher.inl`, `mod_usps_silencer.inl` | `5000–8999` | `5000` ArmBase, `7000` PickupBase, `8100+id` UspsSpawn |
| Ammo Refill & Knife Sounds | `cs2_engine/modules/mod_ammo_refill.inl`, `mod_knife_sounds.inl` | `6000–9999` | `6001` RoundRefill, `6100/6200+id` Refill, `9100–9400+id` KnifeInspect |
| Savage AI Roles, IGL & Bridge | `bot_savage/modules/savage_roles_loadout.inl`, `savage_igl_strats.inl`, `savage_yapb_bridge.inl` | `30000–31999` | `30009` MidScan, `30010/11` CallT/CT, `30014` RetakeSync, `31000+id` RoleMenu, `31200+id` RefillAmmo |
| Savage Combat, Hazard & Molotov | `bot_savage/modules/savage_combat_tick.inl`, `savage_hazard_ai.inl`, `savage_molotov_tactics.inl`, `savage_scavenge_utils.inl`, `savage_drop_economy.inl`, `savage_reflex_esp.inl`, `savage_esports_chat.inl` | `30001–30013` | `30001–30008` CoreLoops, `30012` HazardScan, `30013` MolotovScan |
| Studio HUD & Crosshair | `cs2_studio/modules/mod_crosshair_hud.inl` | `40000–44999` | `40100` CapabilityProbe, `44100` Sync, `44500` Save |
| Studio Viewmodel | `cs2_studio/modules/mod_viewmodel_studio.inl` | `45000–49999` | `46200` VmDefer, `46300` VmSpawn |
| Competitive Gamemode | `cs2_competitive/modules/comp_*.inl` | `50000–54999` | `50009–50300+id` Halftime, OT, AutoPick, Knife |
| ReDM Listen Gamemode | `redm_listen_engine.sma` | `55000–59999` | `55001–55300` ReDM Spawns & Respawn Timers |

---

## 3. Hard Invariants (DO NOT BREAK)

1. **Engine Command Trap:** DILARANG memakai `register_clcmd("m4a1", ...)` karena membajak menu buy bawaan engine (`B-4-3`). Gunakan alias internal atau hook buy command safely.
2. **nVault Immutability:** Vault `cs2_loadout` (dibuka `cs2_arsenal`), serta `cs2_crosshair` & `cs2_viewmodel` (keduanya dibuka terpisah oleh `cs2_client_studio`) tidak boleh diubah nama vault maupun format key-nya agar kompatibel dengan data pemain.
3. **External Sync Contract:** File `configs/skin_inventory.ini` & `configs/active_loadout.ini` dikelola oleh desktop app `CS2 Loadout Manager`. Parser `arsenal_loadout.inc` membaca single-pass via `cs2_reload_loadout` (`ADMIN_CFG`) / `/reloadloadout`.
4. **Client DLL Sync (lihat juga `../cs16-client/AGENTS.md`):**
   * Flag `CS2_KILL_*` wajib sinkron dengan `../cs16-client/cl_dll/death.cpp:28-34`.
   * Batasan clamp `viewmodel_fov [50..75]` & `offset [-3.5..3.5]` wajib sinkron dengan `../cs16-client/cl_dll/view.cpp`.
   * Telemetri scoreboard `BP-04` wajib memakai custom UserMsg terpisah (`CS2PlayerStats` & `CS2RoundHist`), dilarang menumpang byte ekstra pada `ScoreInfo`.
5. **Knife Detection Order:** Cek substring `"m9"` WAJIB sebelum `"bayonet"` (terpusat di `include/cs2_sdk/cs2_weapon_registry.inc`).
6. **YaPB Boundary & Zero PreThink:**
   * `bot_savage_micro_override` default `0`.
   * `cs2_client_studio.amxx` WAJIB **Zero `PreThink`** (gunakan HamHook atau client command dispatch).
7. **Cross-Plugin Resilience & No callfunc_begin:**
   * Dilarang keras memakai `callfunc_begin`.
   * Seluruh komunikasi antar-plugin wajib melewati `include/cs2_sdk/cs2_api.inc` dengan perlindungan `set_native_filter("cs2_sdk_native_filter")` dan pengecekan `cs2_api_has_*()`.
   * Sub-modul dalam satu plugin jika gagal memuat aset wajib soft-disable (dilarang `set_fail_state` yang mematikan seluruh pilar).

---

## 4. Standard Developer Workflow & Safety Gates

1. **Sebelum Merestart Game / Pengujian:**
   Jalankan verifikasi integritas penuh 4-Gate:
   ```powershell
   powershell -ExecutionPolicy Bypass -File tools\verify_workspace.ps1
   ```
   *Wajib exit code `0` (PASS 4/4 Gate: Compile 7/7, Asset integrity, CVar drift, Profile parity).*
2. **Setelah Menguji Game (Post-Test Diagnostics):**
   Rangkum log sesi terakhir dan deteksi runtime error:
   ```powershell
   powershell -ExecutionPolicy Bypass -File tools\check_logs.ps1
   ```
3. **Commit & Rollback Hygiene:**
   Gunakan `git diff` untuk memeriksa perubahan hanya pada file source/config. Jika terjadi regresi darurat, kembalikan ke baseline:
   ```powershell
   git checkout baseline-modular-7plugin -- .
   ```
