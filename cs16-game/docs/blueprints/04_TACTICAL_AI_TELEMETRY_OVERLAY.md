# Blueprint 04 — Tactical AI Live Telemetry Overlay (`amx_savage_debug`)

**Document ID:** `BP-04-TACTICAL-AI-TELEMETRY`  
**Target Plugin:** `bot_savage_ai.amxx` (Pillar 3 — sub-modul baru `bot_savage/modules/savage_telemetry.inl`)  
**SDK & Cross-Pillar Integration:** `include/cs2_sdk/cs2_api.inc`, `include/cs2_sdk/cs2_state_cvars.inc`, `include/cs2_sdk/cs2_offsets.inc` (read-only query ke `cs2_arsenal.amxx` & `csgo_molotov.amxx`)  
**Architecture Baseline:** `cs16-game/ARCHITECTURE.md` (Option 1 + Option 2-Lite — 7 Active Custom Plugins + Task ID Partition `30000 – 39999`)  
**Primary Objective:** Menyediakan instrumen observabilitas real-time untuk `bot_savage_ai.amxx`—berupa HUD diagnostik saat spectate/aim-lock, ring-buffer jejak keputusan AI (*why did the bot do that?*), garis vektor 3D (`TE_BEAMPOINTS`) privat per viewer, serta snapshot konsol terstruktur untuk analisis Agent—dengan **0% overhead runtime saat nonaktif (`amx_savage_debug 0`)** dan **0% regresi** ke Competitive maupun Deathmatch.

---

## 1. Architectural Overview & Zero-Overhead Design

`bot_savage_ai.amxx` mengoordinasikan 15+ subsistem taktis (5-Role, IGL Strat, Team Economy, Holding Angle, Sound ESP, Trade-Frag, Anti-Flash, Double-Peek, Fake Defuse, Weapon Scavenge, Danger/Care Zones, Mid Lineups), tetapi seluruh state tersebut saat ini bersifat *black box* di memori Pawn.

### 1.1 Empat Prinsip Keras (Hard Invariants)
1. **Zero Idle CPU Cost:** Tidak ada `FM_CmdStart` atau `PreThink` tambahan. Satu-satunya loop adalah `set_task(flRate, "Task_SavageTelemetryTick", TASK_SV_TELEMETRY_HUD, _, _, "b")` (default `0.25s` / **4 Hz**) yang **hanya berjalan jika minimal 1 pemain mengaktifkan telemetry** (`g_iTelemetryViewerCount > 0`) dan langsung berhenti via `remove_task()` saat semua viewer menonaktifkan debug atau disconnect.
2. **Strict `MSG_ONE_UNRELIABLE` Network Isolation:** Seluruh garis vektor 3D (`TE_BEAMPOINTS`) dan pesan HUD hanya dikirim ke viewer yang meminta menggunakan `MSG_ONE_UNRELIABLE` (tidak pernah `MSG_BROADCAST`), dengan batas anggaran maksimal **8 beam per tick per viewer** untuk mencegah `SZ_GetSpace` overflow di Xash3D FWGS.
3. **YaPB Separation-of-Concerns Clarity (`bot_savage_micro_override`):** Di `bot_savage_ai.sma:141`, `bot_savage_micro_override` ber-default `0` (YaPB C++ memegang penuh micro aim/move/fire, sehingga `OnCmdStart`, `Task_CheckGrenades`, dan `Task_SoundESPScanner` melakukan early-return). HUD telemetry **wajib** menampilkan indikator eksplisit **`[MICRO: OFF — YaPB Native]`** vs **`[MICRO: ON — Savage Override]`** agar pengembang/Agent tidak keliru mengira fitur Holding Angle / Sound ESP / Anti-Flash sedang rusak padahal memang di-gate oleh CVar tersebut.
4. **Compile-Time Switchboard:** Di `bot_savage_ai.sma`, modul dikontrol oleh `#define ENABLE_SAVAGE_TELEMETRY 1` sehingga dapat dilepas bersih kapan saja tanpa menyentuh modul lain.

### 1.2 Peta File yang Ditambahkan / Dimodifikasi
```text
cs16-game/cstrike/addons/amxmodx/
├── configs/
│   └── bot_savage_roles.cfg                  # Tambah default CVar amx_savage_debug* (semua default 0 / safe)
└── scripting/
    ├── include/cs2_sdk/
    │   └── cs2_api.inc                       # Tambah optional native read-only: savage_get_bot_state_flags, savage_get_team_strat, savage_get_team_economy
    ├── bot_savage_ai.sma                     # Include savage_telemetry.inl, register CVars/cmds/natives, precache laserbeam.spr, cleanup di client_disconnected
    └── bot_savage/
        ├── core/
        │   ├── savage_taskids.inc            # Alokasi TASK_SV_TELEMETRY_HUD (31500) dalam blok 30000–39999
        │   └── savage_defs.inc               # State bitflags, decision ring-buffer globals, HUD sync handle (+ hapus sisa macro yatim TASK_GRENADE_CHECK 2048 di baris 9)
        └── modules/
            ├── savage_telemetry.inl          # [BARU] Engine HUD, Spectator/Aim-Lock resolver, 3D Beam renderer, Console dump, /aidebug menu
            └── savage_*.inl                  # Sisipkan 1-baris SAVAGE_TRACE(bot, ...) pada titik transisi keputusan (zero-cost saat idle)
```

---

## 2. Internal Data Model, Bitflags & Task ID Allocation

### 2.1 Task ID Registry (`bot_savage/core/savage_taskids.inc`)
Berada aman di dalam partisi resmi `bot_savage_ai` (`30000 – 39999`), bebas bentrok dengan task eksisting (`30001..30011`, `31000..31200+id`) maupun reservasi BP-06 (`TASK_SV_ROSTER_APPLY = 31400`):

```pawn
// BP-04 Tactical AI Telemetry Overlay (Block 31500 - 31599)
#define TASK_SV_TELEMETRY_HUD  31500  // Global 4 Hz dynamic ticker (active only when g_iTelemetryViewerCount > 0)
```

### 2.2 Viewer Mode & Target Selection (`bot_savage/core/savage_defs.inc`)
```pawn
enum _:SavageTelemMode
{
    TELEM_OFF = 0,
    TELEM_SPECTATE,       // Default: auto-inspect bot yang sedang di-spectate (pev_iuser2) atau crosshair aim saat hidup
    TELEM_LOCKED_BOT,     // Kunci inspeksi pada 1 bot spesifik meski viewer sedang hidup / roaming
    TELEM_SQUAD_OVERVIEW  // Ringkasan kompak seluruh 5 bot dalam 1 tim + beam taktis tim
};

// Telemetry State Bitflags (32-bit bitmask — juga diekspos lewat cs2_sdk/cs2_api.inc)
#define SAVAGE_STATE_HOLD_ANGLE     (1 << 0)  // g_bHoldingAngle[id]
#define SAVAGE_STATE_SOUND_ESP      (1 << 1)  // g_iSoundTarget[id] > 0 && active
#define SAVAGE_STATE_TRADE_FRAG     (1 << 2)  // g_iTradeTarget[id] > 0 && active
#define SAVAGE_STATE_ANTI_FLASH     (1 << 3)  // flGameTime < g_flFlashDodgeUntil[id]
#define SAVAGE_STATE_DOUBLE_PEEK    (1 << 4)  // g_iDoublePeekPartner[id] > 0 && active
#define SAVAGE_STATE_FAKE_DEFUSE    (1 << 5)  // flGameTime < g_flFakeDefuseAmbushUntil[id]
#define SAVAGE_STATE_SCAVENGE       (1 << 6)  // g_iScavengeTarget[id] > 0 && pev_valid
#define SAVAGE_STATE_LINEUP_THROW   (1 << 7)  // g_bLineupActive[id]
#define SAVAGE_STATE_REAR_FLANK     (1 << 8)  // flGameTime < g_flRearFlankUntil[id]
#define SAVAGE_STATE_SILENT_WALK    (1 << 9)  // flGameTime < g_flSilentWalkUntil[team]
#define SAVAGE_STATE_KNIFE_BRAWL    (1 << 10) // cs2_state_is_knife_round()
#define SAVAGE_STATE_IN_COMBAT      (1 << 11) // (flGameTime - g_flLastCombatTime[id]) <= 2.0
#define SAVAGE_STATE_TRIGGER_BURST  (1 << 12) // flGameTime < g_flTriggerBurstUntil[id]
#define SAVAGE_STATE_TRADE_FOLLOW   (1 << 13) // g_iTradeLeader[id] > 0 && flGameTime < g_flTradeFollowUntil[id]

// Economy & Danger Zone Display Labels (sinkron dengan enum di savage_defs.inc:82-92 & 120-126)
new const g_szEconNames[][] = {
    "AUTO / NORMAL", "ECO (SAVE)", "FORCE BUY", "HALF BUY", "FULL BUY"
};

new const g_szZoneNames[][] = {
    "None", "Bombsite A", "Bombsite B", "Mid", "Short/Cat", "Long A", "Flank/Rear", "Enemy AWP", "Enemy Push"
};
```

### 2.3 Lightweight Decision Ring-Buffer (Zero-Alloc, 4 Slot/Bot)
Untuk menjawab *"mengapa bot melakukan hal tersebut 3 detik lalu?"*, simpan 4 keputusan terakhir per bot dalam array statis kecil (`33 * 4 * 64` = ~8,4 KB total):

```pawn
#define SAVAGE_TRACE_SLOTS  4
#define SAVAGE_TRACE_LEN    64

new g_szBotTrace[33][SAVAGE_TRACE_SLOTS][SAVAGE_TRACE_LEN];
new Float:g_flBotTraceTime[33][SAVAGE_TRACE_SLOTS];
new g_iBotTraceHead[33];

// Per-viewer telemetry control
new g_iViewerTelemMode[33];     // SavageTelemMode
new g_iViewerLockedBot[33];     // 1..32 jika TELEM_LOCKED_BOT
new bool:g_bViewerBeams[33];    // Toggle 3D TE_BEAMPOINTS per viewer
new CsTeams:g_iViewerOverviewTeam[33]; // Filter tim untuk TELEM_SQUAD_OVERVIEW
new g_iTelemetryViewerCount;    // Counter aktif; 0 = task dimatikan total
new g_HudSyncTelemetry;         // Dedicated CreateHudSyncObj() handle
new g_iSprTelemetryBeam;        // Precache index sprites/laserbeam.spr
```

**Fast-Path Trace Helper (`savage_telemetry.inl`):**
```pawn
stock Savage_RecordDecision(id, const szFmt[], any:...)
{
    // Guard #1: Zero string formatting cost saat tidak ada debug viewer & console trace off
    if (g_iTelemetryViewerCount <= 0 && !get_pcvar_num(g_pCvarDebugConsole))
        return;

    if (id < 1 || id > MaxClients)
        return;

    new slot = g_iBotTraceHead[id] % SAVAGE_TRACE_SLOTS;
    vformat(g_szBotTrace[id][slot], SAVAGE_TRACE_LEN - 1, szFmt, 3);
    g_flBotTraceTime[id][slot] = get_gametime();
    g_iBotTraceHead[id] = (slot + 1) % SAVAGE_TRACE_SLOTS;

    if (get_pcvar_num(g_pCvarDebugConsole) >= 2)
    {
        new szName[32];
        get_user_name(id, szName, charsmax(szName));
        server_print("[SAVAGE_TRACE %.1fs] #%d %s (%s): %s",
            g_flBotTraceTime[id][slot], id, szName, g_szRoleTags[g_iPlayerRole[id]], g_szBotTrace[id][slot]);
    }
}
```
> **Catatan Pawn:** Dengan `if (g_iTelemetryViewerCount <= 0 && !get_pcvar_num(g_pCvarDebugConsole)) return;` di baris pertama, saat mode normal (`amx_savage_debug 0`), fungsi langsung keluar dalam 2 cek integer tanpa menyentuh `vformat()` atau alokasi stack.

### 2.4 CVars Baru (`bot_savage_ai.sma` + `configs/bot_savage_roles.cfg`)

| CVar | Default | Tipe / Range | Fungsi |
| :--- | :--- | :--- | :--- |
| `amx_savage_debug` | `"0"` | Int `0..3` | Global master switch & default mode untuk Listen Server host (`0=Off`, `1=Spectate/Aim`, `2=Squad Overview`, `3=Spectate + Full Tactical Map Anchors`). |
| `amx_savage_debug_beams` | `"1"` | Bool `0\|1` | Mengaktifkan render garis 3D `TE_BEAMPOINTS` untuk viewer yang menyalakan telemetry. |
| `amx_savage_debug_rate` | `"0.25"` | Float `0.15..1.0` | Interval refresh telemetry HUD & beam (di-clamp aman `>= 0.15s` agar tidak membanjiri netchan). |
| `amx_savage_debug_console` | `"0"` | Int `0..2` | `0=Off`, `1=Manual dump only (amx_savage_dump)`, `2=Live stream [SAVAGE_TRACE] ke qconsole.log` (siap di-parse `check_logs.ps1`). |
| `amx_savage_debug_admin_only` | `"0"` | Bool `0\|1` | `0` untuk Listen Server solo (semua human bisa akses `/aidebug`); `1` membatasi hanya `ADMIN_CVAR`. |

---

## 3. HUD Layout, Channel Audit & Visual Schema

### 3.1 Audit Koordinat HUD Lintas 7 Plugin Aktif (Zero Overlap Guarantee)
Berdasarkan inspeksi langsung terhadap seluruh plugin aktif di `cs16-game`:

| Zona Layar | Koordinat `(x, y)` | Pemilik Eksisting | Konflik dengan Telemetry? |
| :--- | :--- | :--- | :--- |
| Top-Center | `(-1.0, 0.02..0.08)` | `comp_dmg_mvp_hud.inl` (Scoreboard / Warmup / C4) | Tidak |
| Top-Left Radar | `(0.00..0.18, 0.00..0.22)` | CS 1.6 / `cs2_client_studio` Radar (hanya saat hidup) | Dihindari saat hidup |
| Center DHUD | `(-1.0, 0.24..0.30)` | `comp_match_flow.inl` & `comp_dmg_mvp_hud.inl` (MVP / Live) | Tidak |
| Right-Mid | `(0.68, 0.32)` | `comp_dmg_mvp_hud.inl` (Damage Card) | Tidak |
| Left-Mid Upper | `(0.03, 0.38)` | `mod_smart_ping.inl` (Smart Ping HUD) | Tidak |
| Bottom-Center | `(-1.0, 0.72..0.80)` | `mod_weapon_variants.inl` & `mod_kill_feedback.inl` | Tidak |
| Bottom-Right | `(0.78, 0.82)` | `mod_kill_reward.inl` (`+$300`) | Tidak |
| **Telemetry Zone (Baru)** | **Spectate/Dead: `(0.02, 0.16)`**<br>**Alive: `(0.02, 0.48)`** | **`bot_savage_ai.amxx` (`g_HudSyncTelemetry`)** | **100% Bebas Tumpang Tindih** |

* **Channel Safety:** Menggunakan `g_HudSyncTelemetry = CreateHudSyncObj()` dengan `channel = -1` (auto-sync AMXX) dan `holdtime = flRate + 0.15` (`0.40s`, `fadein = 0.0`, `fadeout = 0.05`) sehingga tampilan stabil tanpa kedip dan tidak pernah menimpa `g_HudSyncHeader` atau `ping_HudSyncPing`.
* **Batasan Panjang Buffer GoldSrc/Xash3D:** Maksimal payload `SVC_TEMPENTITY` / `ShowSyncHudMsg` adalah `511` byte. Seluruh format string di `savage_telemetry.inl` wajib dibatasi `static szHud[480]` dengan truncation aman.

### 3.2 Target Resolution Logic (`ResolveTelemetryTarget(viewer)`)
1. Jika `g_iViewerTelemMode[viewer] == TELEM_LOCKED_BOT`: pakai `g_iViewerLockedBot[viewer]` jika masih terhubung dan merupakan bot.
2. Jika `!is_user_alive(viewer)` (mati atau tim Spectator):
   * Baca `iSpecMode = pev(viewer, pev_iuser1)` dan `iSpecTarget = pev(viewer, pev_iuser2)`.
   * Jika `iSpecMode` adalah `OBS_IN_EYE (4)`, `OBS_CHASE_LOCKED (1)`, atau `OBS_CHASE_FREE (2)` dan `1 <= iSpecTarget <= MaxClients` serta `is_user_bot(iSpecTarget)` -> tampilkan telemetry `iSpecTarget`.
   * Jika `OBS_ROAMING (3)` (free-look camera) -> fallback ke `get_user_aiming(viewer, iAimEnt, iBody, 2500)` atau otomatis tampilkan ringkasan tim (`Squad Overview`).
3. Jika `is_user_alive(viewer)` dan mode `TELEM_SPECTATE`:
   * Cek `get_user_aiming(viewer, iAimEnt, iBody, 2048)` -> jika membidik bot, tampilkan telemetry bot tersebut secara instan; jika tidak membidik bot, tampilkan status ringkas **Tim Sendiri** (IGL Strat, Economy Call, Danger Zone, jumlah bot aktif per state).

### 3.3 Format Tampilan HUD — Mode Single-Bot (`TELEM_SPECTATE` / `TELEM_LOCKED_BOT`)
```text
[SAVAGE AI TELEMETRY] R08 LIVE | MICRO: OFF (YaPB)
Bot: [NV] s1mple (CT) | Role: AWPer | HP:100 AP:100 $4850
Loadout: AWP [10/30] + Deagle | Util: 1SM 1FL 0HE 1MOL
Strat: Default 1-3-1 | Econ: FULL BUY | Danger: Mid (11s)
--------------------------------------------------
ACTIVE STATES: [HOLD_ANGLE 2.1s] [DBL_PEEK -> b1t]
Sensory: SoundESP=NONE | Trade=NONE | Threat=12.4s ago
Move: 0 u/s (HOLDING) | Weapon: AWP | SavedWpn: None
--------------------------------------------------
TRACE:
 -0.8s: HOLD_ANGLE lock (2.9s, maxspd=1.0)
 -4.2s: DBL_PEEK paired w/ b1t (STAND)
-14.5s: EQUIP AWPer -> AWP + Deagle (FULL BUY)
```
*Catatan Dinamis:*
* Jika bot CT memegang `CSW_M4A1`, resolver senjata memanggil `cs2_api_is_m4a4(bot)` untuk menampilkan **`M4A4`** vs **`M4A1-S`** secara akurat.
* Jika bot memegang varian CS2 (`MAG-7`, `MP7`, `CZ75`, `Tec-9`, `PP-Bizon`, `Negev`, `Sawed-Off`), resolver memanggil `cs2_get_user_weapon_variant()` (guarded oleh `cs2_api_has_variants()`) agar nama varian yang tampil sesuai ekonomi sebenarnya.
* Warna HUD menyesuaikan kondisi:
  * **Cyan `(110, 220, 255)`:** Idle / Normal / Holding.
  * **Amber `(255, 200, 60)`:** Active Tactical Trigger (Sound ESP / Trade-Frag / Scavenge / Lineup).
  * **Red `(255, 90, 90)`:** Active Combat (`<= 2.0s`) / Anti-Flash Reflex / Fake Defuse Ambush.

### 3.4 Format Tampilan HUD — Mode Squad Overview (`TELEM_SQUAD_OVERVIEW`)
```text
[SAVAGE SQUAD TELEMETRY — CT] R08 | MICRO: OFF (YaPB)
Strat: Split Mid to Site | Econ: FORCE BUY | Danger: Site A (7s)
--------------------------------------------------
#2  karrigan [IGL]     HP:100 $1200 M4A1-S  [LINEUP]
#3  rain     [Entry]   HP:82  $450  FAMAS   [COMBAT]
#4  broky    [AWPer]   HP:100 $1900 SCOUT   [HOLD 1.8s]
#5  frozen   [Support] HP:100 $800  MP7     [TRADE->#3]
#6  ropz     [Lurker]  HP:100 $2100 M4A1-S  [FLANK 4.2s]
```

---

## 4. 3D Vector Beam Overlay (`TE_BEAMPOINTS` Privat)

### 4.1 Asset Precache (Zero-Failure Contract)
Di `bot_savage_ai.sma` -> `plugin_precache()`:
```pawn
#if ENABLE_SAVAGE_TELEMETRY
    g_iSprTelemetryBeam = precache_model("sprites/laserbeam.spr");
#endif
```
`sprites/laserbeam.spr` adalah aset standar GoldSrc/Xash3D yang sudah diprecache juga oleh `cs2_engine/modules/mod_modern_utility.inl:146`, sehingga menambah `0` slot precache baru.

### 4.2 Semantik Warna & Prioritas Beam (Budget Cap: Maks 8 Beam / Tick / Viewer)
Semua beam dirender dengan `life = floatround(flRate * 10.0) + 1` (0.3–0.4 detik, hilang mulus tanpa artefak sisa) dan hanya dikirim via `message_begin(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, _, viewer)`.

| Prioritas | Kondisi Aktif pada Bot Target | Titik Awal -> Titik Akhir | Warna `(R, G, B)` | Lebar | Makna Diagnostik |
| :---: | :--- | :--- | :--- | :---: | :--- |
| **1** | **Aim / Holding Angle Ray** (`g_bHoldingAngle[bot]` atau selalu saat Single-Bot) | Mata bot (`origin + view_ofs`) -> `TR_vecEndPos` (trace ke depan maks `600u`, atau `900u` jika Holding/AWP) | Cyan `(0, 220, 255)` (Normal)<br>Biru Terang `(60, 140, 255)` (Holding Angle) | `8` / `14` | Menunjukkan sudut persis yang sedang di-hold / pre-aim oleh bot. |
| **2** | **Sound ESP Lock** (`g_iSoundTarget[bot] > 0` & aktif) | Mata bot -> `g_vecSoundTargetOrigin[bot]` | Kuning `(255, 230, 40)` | `16` | Menunjukkan target suara musuh yang dideteksi `Task_SoundESPScanner()` (tampak menembus dinding bagi spectator). |
| **3** | **Trade-Frag Snap** (`g_iTradeTarget[bot] > 0` & aktif) | Mata bot -> `origin` dari `g_iTradeTarget[bot]` | Merah `(255, 50, 50)` | `18` | Menunjukkan musuh yang sedang dikunci untuk instant trade-frag. |
| **4** | **Weapon Scavenge Target** (`g_iScavengeTarget[bot]` valid) | Dada bot -> `origin` entity `weaponbox` | Hijau `(60, 255, 110)` | `12` | Menunjukkan senjata jatuh yang sedang dikejar bot untuk upgrade tier. |
| **5** | **Double-Peek Tether** (`g_iDoublePeekPartner[bot] > 0`) | Dada bot -> dada partner (`g_iDoublePeekPartner[bot]`) | Magenta `(230, 80, 255)` | `10` | Menunjukkan pasangan anti-AWP double-peek (`STAND` vs `CROUCH`). |
| **6** | **Mid Utility Lineup Vector** (`g_bLineupActive[bot]`) | Mata bot -> vektor `g_vecLineupAngles[bot]` (`400u`) | Oranye `(255, 150, 30)` | `14` | Menunjukkan arah lemparan smoke/flash lineup otomatis. |
| **7** | **Team Hive-Mind Threat** (`flGameTime - g_flTeamThreatTime[team] <= 6.0`) | Pillar vertikal `120u` di `g_vecTeamThreatOrigin[team]` | Merah Tua `(220, 40, 80)` | `20` | Lokasi ancaman terakhir yang dibagikan ke seluruh tim via Hive-Mind. |
| **8** | **Active Care / Danger Zone** (`g_iActiveDangerZone[team] != ZONE_NONE`) | Pillar vertikal `160u` di `g_vecDangerPoint[team]` | Oranye Terang `(255, 110, 0)` | `24` | Titik bahaya yang ditandai via perintah chat `care a / b / mid / awp / flank`. |

* **Mode 3 Extra (`amx_savage_debug 3`):** Saat viewer dalam mode roaming/spectate level 3, tambahkan pillar referensi statis untuk `g_vecBombsiteA` (Hijau), `g_vecBombsiteB` (Biru), dan `g_vecMapMidPoint` (Putih) guna memverifikasi kalibrasi titik tengah map (`InitMapMidPoint()` & `InitBombsites()`).

---

## 5. Decision Instrumentation Map (`bot_savage/modules/*.inl`)

Titik penyisipan tepat 1 baris `Savage_RecordDecision(id, ...)` pada modul eksisting (nol perubahan logika AI):

| Modul Sumber | Fungsi / Baris Referensi | Event yang Dicatat | Contoh Pesan Trace |
| :--- | :--- | :--- | :--- |
| `savage_roles_loadout.inl` | `AssignBotRoleDeterministic()` & `EquipBotByRole()` | Penetapan role & hasil equip senjata | `"EQUIP AWPer -> AWP+Deagle (FULL)"` |
| `savage_igl_strats.inl` | `Event_FreezeTimeStart()` / `Task_AnnounceTeamStrat()` | Pemilihan strategi tim ronde baru | `"IGL CALL -> Split Mid to Site"` |
| `savage_drop_economy.inl` | `Cmd_SetTeamEconomy()` & weapon drop handler | Perubahan mode ekonomi & drop rifle ke teammate | `"ECON -> FORCE BUY ($2150)"` / `"DROP AK47 -> Player"` |
| `savage_esports_chat.inl` | `Execute_CareZoneCall()` (`:24`), `Execute_DropC4Call()` (`:85`) | Respon panggilan taktis human | `"CARE ZONE -> Mid (18.0s)"` |
| `savage_combat_tick.inl` | Hold Angle lock (`:215`, `:730`), Sound Wallbang (`:488`), Trade-Frag (`:524`), Double-Peek (`:554`), Scavenge pickup (`:659`) | Transisi state combat & movement | `"HOLD_ANGLE lock (2.7s)"`, `"WALLBANG burst -> #7"`, `"SCAVENGE pickup AK47"` |
| `savage_reflex_esp.inl` | `TriggerAntiFlashReflex()` (`:46`), `Task_SoundESPScanner()` (`:85`), Fake Defuse ambush | Trigger refleks defensif & deteksi langkah | `"ANTI_FLASH 180° (0.75s)"`, `"SOUND_ESP lock #4 (410u)"`, `"FAKE_DEFUSE ambush"` |
| `savage_scavenge_utils.inl` | `Task_ScavengeGroundWeapons()` | Penargetan senjata jatuh | `"SCAVENGE target M4A1 (320u)"` |

---

## 6. Cross-Pillar `cs2_sdk` Integration & Forward Compatibility

### 6.1 Konsumsi SDK Eksisting (Zero `callfunc_begin`)
`savage_telemetry.inl` langsung memanfaatkan header yang sudah di-include oleh `bot_savage_ai.sma`:
* **`<cs2_sdk/cs2_state_cvars>`:**
  * `cs2_state_is_warmup()`, `cs2_state_is_timeout()`, `cs2_state_is_deathmatch()`, `cs2_state_is_knife_round()`, `cs2_state_get_match_round()` -> menampilkan badge fase match akurat (`R08 LIVE`, `WARMUP`, `KNIFE`, `TIMEOUT`, `DM`).
* **`<cs2_sdk/cs2_api>`:**
  * `cs2_api_is_m4a4(bot, bAvail)` -> membedakan `M4A4` vs `M4A1-S` saat CT bot memegang `CSW_M4A1`.
  * `cs2_api_has_variants()` + `cs2_get_user_weapon_variant(bot, iWpn, szVar, charsmax(szVar))` -> menampilkan nama varian (`MAG-7`, `MP7`, `CZ75`, dll.).
  * `cs2_api_has_molotov()` + `csgo_get_user_molotov(bot)` -> menampilkan indikator kepemilikan Molotov (`1MOL` / `0MOL`).
  * Seluruh panggilan ini 100% aman meski `cs2_arsenal.amxx` atau `csgo_molotov.amxx` dimatikan sementara berkat `set_native_filter("cs2_sdk_native_filter")` di `bot_savage_ai.sma:92`.

### 6.2 Native Baru yang Diekspor (`bot_savage_ai.sma` + `include/cs2_sdk/cs2_api.inc`)
Agar **BP-02 (Native `client.dll` HUD)** dan **BP-05 (Post-Match Analytics)** di masa depan dapat membaca state taktis bot tanpa duplikasi kode:

```pawn
// include/cs2_sdk/cs2_api.inc (Provider: bot_savage_ai)
native savage_get_bot_state_flags(id);              // Mengembalikan bitmask SAVAGE_STATE_* (0 jika bukan bot / idle)
native savage_get_team_strat(CsTeams:team);         // Mengembalikan STRAT_* (0..5)
native savage_get_team_economy(CsTeams:team);       // Mengembalikan ECON_* (0..4)

// Guarded wrappers:
stock cs2_api_get_bot_state_flags(id)
{
    if (!cs2_api_has_savage_ai())
        return 0;
    return savage_get_bot_state_flags(id);
}

stock cs2_api_get_team_strat(CsTeams:team)
{
    if (!cs2_api_has_savage_ai())
        return 0;
    return savage_get_team_strat(team);
}

stock cs2_api_get_team_economy(CsTeams:team)
{
    if (!cs2_api_has_savage_ai())
        return 0;
    return savage_get_team_economy(team);
}
```
Didaftarkan di `bot_savage_ai.sma` -> `plugin_natives()` tepat di bawah `savage_equip_bot_role` (baris 94).

### 6.3 Interoperabilitas Opsional dengan BP-06 (`savage_rosters.inl`)
Gunakan guard kompilasi kondisional agar BP-04 tidak bergantung pada BP-06 (dan sebaliknya):
```pawn
#if defined _savage_rosters_included
    if (g_iBotRosterPack[bot] >= 0)
    {
        // Tampilkan baris tambahan: "Roster: [NV] NAVI Structured (Slot #3 | Tone: PRO_CALM)"
    }
#endif
```

---

## 7. Commands, Interactive Menu & Agent Console Dump

### 7.1 Referensi Perintah Chat & Console

| Command (Chat / Console) | Parameter | Fungsi |
| :--- | :--- | :--- |
| `/aidebug` atau `savage_debug_menu` | — | Membuka menu cepat AMXX: Toggle Mode (Off / Spectate / Lock Bot / Squad CT / Squad T), Toggle 3D Beams, Toggle Micro Override Info, Dump Snapshot ke Console. |
| `amx_savage_debug <0\|1\|2\|3>` | `0..3` | Shortcut CVar / console untuk menghidupkan/mematikan overlay secara instan (langsung menyinkronkan mode host Listen Server). |
| `/debugbot [arg]` | `name \| #id \| aim \| off` | Mengunci HUD telemetry ke 1 bot tertentu (`TELEM_LOCKED_BOT`) saat pemain masih hidup, atau `off` untuk kembali ke mode auto-spectate. |
| `amx_savage_beams <0\|1>` | `0 \| 1` | Toggle pribadi garis vektor 3D `TE_BEAMPOINTS` untuk viewer tersebut. |
| `amx_savage_dump [ct\|t\|all]` | Opsional (`all`) | Mencetak tabel snapshot deterministik seluruh bot & state tim ke `server_print` / `qconsole.log`. |

### 7.2 Agent-Readable Console Snapshot (`amx_savage_dump`)
Dirancang agar sinkron dengan `tools/check_logs.ps1` (BP-01) saat melakukan sesi tuning AI bersama AI Agent:
```text
[SAVAGE_DUMP] Round=8 Phase=LIVE MicroOverride=0 Freeze=0
[SAVAGE_TEAM CT] Strat=STRAT_DEFAULT Econ=ECON_FULL Danger=ZONE_MID(9.4s) ThreatAge=11.2s
[SAVAGE_TEAM T ] Strat=STRAT_SPLIT   Econ=ECON_FORCE Danger=ZONE_NONE(0.0s) ThreatAge=3.1s
[SAVAGE_BOT] id=2 team=CT name="s1mple" role=AWPER hp=100 money=4850 wpn=AWP flags=0x0011 spd=0 last="HOLD_ANGLE lock (2.9s)"
[SAVAGE_BOT] id=3 team=CT name="b1t"    role=ENTRY hp=82  money=1200 wpn=M4A4 flags=0x0810 spd=210 last="DBL_PEEK paired w/ s1mple"
```
Satu baris per bot dengan format `key=value` memudahkan regex parsing maupun pembacaan cepat oleh Agent tanpa membakar context token.

---

## 8. Build Gate, Isolation Tests & Acceptance Criteria

### 8.1 Langkah Implementasi Bertahap
1. **Step 1 — Core Definitions & Cleanup:** Tambahkan `TASK_SV_TELEMETRY_HUD = 31500` ke `savage_taskids.inc`; tambahkan state bitflags, globals ring-buffer, dan label ekonomi/zona ke `savage_defs.inc` (sekaligus hapus `#define TASK_GRENADE_CHECK 2048` yatim di baris 9).
2. **Step 2 — SDK Contract Update:** Tambahkan 3 native read-only + guarded wrappers ke `include/cs2_sdk/cs2_api.inc` dan daftarkan di `bot_savage_ai.sma` (`plugin_natives()`).
3. **Step 3 — Implement `savage_telemetry.inl`:** Bangun viewer counter lifecycle, `Task_SavageTelemetryTick`, HUD renderer (`TELEM_SPECTATE` / `TELEM_LOCKED_BOT` / `TELEM_SQUAD_OVERVIEW`), `TE_BEAMPOINTS` privat (cap 8 beam/tick), `/aidebug` menu, dan `amx_savage_dump`.
4. **Step 4 — Wire Switchboard & Trace Hooks:** Include `savage_telemetry.inl` di `bot_savage_ai.sma` di bawah `#if ENABLE_SAVAGE_TELEMETRY`, precache `sprites/laserbeam.spr`, panggil `Savage_Telemetry_OnDisconnect(id)` di `client_disconnected()`, dan pasang `Savage_RecordDecision()` pada 7 titik transisi utama.
5. **Step 5 — Compile & Verify:** Jalankan `cs16-game\cstrike\addons\amxmodx\scripting\build_all.bat` dan pastikan **7/7 plugin exit code `0`** dengan `0` warning baru dan `0` file `.amxx` nyasar di `scripting/`.

### 8.2 Acceptance Criteria & Verification Checklist
* [ ] **Compile Gate:** `build_all.bat` menghasilkan `7/7` sukses tanpa error/warning di `bot_savage_ai.sma` maupun plugin lain yang meng-include `cs2_sdk/cs2_api.inc`.
* [ ] **Zero-Overhead Idle Test:** Saat `amx_savage_debug 0`, `task_exists(TASK_SV_TELEMETRY_HUD)` bernilai `false`, dan tidak ada `TE_BEAMPOINTS` maupun HUD sync yang dikirim.
* [ ] **Spectator & Live Aim-Lock Test:** Saat mati & spectate bot di Competitive, HUD telemetry muncul rapi di `(0.02, 0.16)` menampilkan Role, Strat, Economy, status `MICRO: OFF/ON`, senjata akurat (`M4A4` vs `M4A1-S` & varian), serta 3 trace keputusan terakhir tanpa menimpa Scoreboard (`y=0.02`) maupun Smart Ping (`y=0.38`).
* [ ] **3D Vector Beam & Netchan Safety Test:** Dengan 9 bot YaPB aktif dan `amx_savage_debug 1` + `amx_savage_debug_beams 1`, garis aim/hold-angle/sound-esp/scavenge tampil mulus khusus ke viewer debug tanpa muncul warning `SZ_GetSpace` di `qconsole.log` maupun error di `error_*.log`.
* [ ] **Cross-Pillar Isolation Test:** Nonaktifkan sementara `cs2_arsenal.amxx` dan `csgo_molotov.amxx` di `plugins.ini` — `bot_savage_ai.amxx` + telemetry tetap berjalan dengan **0 runtime error** (fallback otomatis menampilkan nama senjata base GoldSrc).
* [ ] **Mode Switch Cleanliness:** Berpindah antara `!Start_CS16_Xash3D.bat` (Competitive) dan `!Start_Deathmatch_Xash3D.bat` (Deathmatch) tetap menampilkan fase yang tepat (`LIVE` vs `DM`) tanpa kebocoran task atau crash saat disconnect/reconnect.
