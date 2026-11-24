# Blueprint 06 — Hot-Swappable Bot Roster & Persona Packs

**Document ID:** `BP-06-BOT-ROSTER-PERSONAS`  
**Target Plugins:** `bot_savage_ai.amxx` (Pillar 3 — `bot_savage/modules/savage_rosters.inl`) + Cross-Pillar SDK Bridge ke `cs2_arsenal.amxx` (`cs2_arsenal/modules/mod_m4_split.inl`) & `csgo_competitive.amxx` (`comp_match_flow.inl`)  
**New Config Asset:** `cs16-game/cstrike/addons/amxmodx/configs/bot_rosters.ini`  
**Architecture Baseline:** `cs16-game/ARCHITECTURE.md` (Shared `include/cs2_sdk/cs2_api.inc` + Task ID Partition `30000 – 39999`)  
**Primary Objective:** Mengubah bot YaPB yang sebelumnya menggunakan nama acak (`yapb/conf/lang/en_names.cfg`) dan perilaku seragam menjadi **tim esports berkarakter yang dapat diganti secara instan (hot-swappable)**—lengkap dengan roster nama tetap, role terkunci, bobot strategi IGL, preferensi senjata (M4A4 vs M4A1-S, AWP vs Rifle, Pistol), serta gaya komunikasi chat—dengan **0% regresi** ke mode Competitive dan Deathmatch.

---

## 1. Roster Configuration Schema (`configs/bot_rosters.ini`)

Format INI datar yang ramah edit manual maupun AI Agent, dibaca 1x saat `plugin_cfg()` dan dapat di-reload live via `amx_bot_roster_reload` (`ADMIN_CFG`):

```ini
; ==============================================================================
; CS2 BOT ROSTER & PERSONA PACKS (bot_savage_ai + cs2_arsenal)
; Format Slot:
;   slot = <Name> | <Role> | <M4Pref> | <PrimaryPref> | <PistolPref> | <ChatTone>
;
; Role       : ENTRY, AWPER, SUPPORT, LURKER, IGL
; M4Pref     : M4A4, M4A1S, AUTO
; PrimaryPref: AK47, M4, AWP, SCOUT, AUG, SG552, GALIL, FAMAS, MP7, P90
; PistolPref : DEAGLE, USP, GLOCK, FIVESEVEN, TEC9, CZ75
; ChatTone   : PRO_CALM, HYPE_AGGRO, TACTICAL_SHORT, BANTER
; ==============================================================================

[ROSTER:FAZE_AGGRO]
display_name      = FaZe Aggro Pack (Fast Execute & Entry)
team_tag          = [FZ]
aggression        = 0.85
; Bobot: rush_a, rush_b, default, split, eco (total tidak harus 100, dinormalisasi)
strat_weights     = 30, 30, 15, 20, 5
force_buy_chance  = 55
yapb_skill_min    = 90
yapb_skill_max    = 98
chat_style        = HYPE_AGGRO
slot_1            = karrigan | IGL     | M4A1S | M4   | DEAGLE    | HYPE_AGGRO
slot_2            = rain     | ENTRY   | M4A4  | AK47 | DEAGLE    | PRO_CALM
slot_3            = broky    | AWPER   | M4A1S | AWP  | DEAGLE    | PRO_CALM
slot_4            = frozen   | SUPPORT | M4A4  | M4   | FIVESEVEN | TACTICAL_SHORT
slot_5            = ropz     | LURKER  | M4A1S | AK47 | USP       | TACTICAL_SHORT

[ROSTER:NAVI_TACTICAL]
display_name      = NAVI Structured Pack (Default & Trade)
team_tag          = [NV]
aggression        = 0.45
strat_weights     = 10, 10, 45, 30, 5
force_buy_chance  = 25
yapb_skill_min    = 92
yapb_skill_max    = 99
chat_style        = PRO_CALM
slot_1            = Aleksib  | IGL     | M4A1S | M4   | FIVESEVEN | PRO_CALM
slot_2            = jL       | ENTRY   | M4A4  | AK47 | DEAGLE    | HYPE_AGGRO
slot_3            = w0nderful| AWPER   | M4A1S | AWP  | DEAGLE    | PRO_CALM
slot_4            = iM       | SUPPORT | M4A4  | AK47 | TEC9      | TACTICAL_SHORT
slot_5            = b1t      | LURKER  | M4A4  | AK47 | USP       | PRO_CALM

[ROSTER:GOLDEN_16]
display_name      = 2006 CS 1.6 Legends Pack
team_tag          = [1.6]
aggression        = 0.65
strat_weights     = 20, 20, 35, 20, 5
force_buy_chance  = 35
yapb_skill_min    = 88
yapb_skill_max    = 96
chat_style        = BANTER
slot_1            = cArn     | IGL     | M4A4  | M4   | DEAGLE    | TACTICAL_SHORT
slot_2            = f0rest   | ENTRY   | M4A4  | AK47 | DEAGLE    | BANTER
slot_3            = markeloff| AWPER   | M4A1S | AWP  | DEAGLE    | PRO_CALM
slot_4            = NEO      | SUPPORT | M4A4  | AK47 | USP       | PRO_CALM
slot_5            = GeT_RiGhT| LURKER  | M4A1S | AK47 | GLOCK     | BANTER

[ROSTER:DOUBLE_AWP_META]
display_name      = Double-AWP Hold & Retake Pack
team_tag          = [2AWP]
aggression        = 0.35
strat_weights     = 8, 8, 54, 25, 5
force_buy_chance  = 20
yapb_skill_min    = 90
yapb_skill_max    = 98
chat_style        = TACTICAL_SHORT
slot_1            = Jame     | IGL     | M4A1S | AWP  | DEAGLE    | TACTICAL_SHORT
slot_2            = YEKINDAR | ENTRY   | M4A4  | AK47 | TEC9      | HYPE_AGGRO
slot_3            = sh1ro    | AWPER   | M4A1S | AWP  | DEAGLE    | PRO_CALM
slot_4            = FL1T     | SUPPORT | M4A4  | M4   | FIVESEVEN | TACTICAL_SHORT
slot_5            = fame     | LURKER  | M4A1S | AK47 | USP       | TACTICAL_SHORT
```

---

## 2. Internal Data Architecture (`bot_savage/modules/savage_rosters.inl`)

### 2.1 Memory Budget & Limits (Pawn-Safe Static Structs)
Untuk menghindari overhead heap besar (`#pragma dynamic`), batas statis dikunci secara deterministik:
* `#define MAX_ROSTER_PACKS    12`
* `#define MAX_ROSTER_SLOTS    5` (mendukung hingga 5v5 kompetitif; jika bot > 5 per tim, bot ke-6+ otomatis fallback ke nama YaPB + `AssignBotRoleDeterministic()`).
* **Struct Pack (`RosterPackData`):** `szId[24]`, `szDisplay[48]`, `szTag[12]`, `Float:flAggression`, `iStratWeights[5]`, `iForceChance`, `iSkillMin`, `iSkillMax`, `iChatStyle`, `iSlotCount`.
* **Struct Slot (`RosterSlotData`):** `szName[24]`, `iRole`, `iM4Pref` (`-1=AUTO, 0=M4A4, 1=M4A1S`), `iPrimaryCsw`, `iPistolCsw`, `iChatTone`.
* **Active Team Binding:**
  * `new g_iActiveRoster[CsTeams] = { -1, -1, -1 };` (`-1` = Vanilla YaPB behavior — 100% zero regression jika tidak dipilih!).
  * `new g_iBotRosterPack[33] = { -1, ... };` & `new g_iBotRosterSlot[33] = { -1, ... };` (melekat pada **entity bot**, bukan indeks tim, sehingga tahan saat Halftime Side Swap!).
* **Task ID (`bot_savage/core/savage_taskids.inc`):**
  * `#define TASK_SV_ROSTER_APPLY   31400` (berada di dalam blok resmi `30000 – 39999`).

### 2.2 CVars Baru (`bot_savage_ai.sma`)
* `bot_savage_roster_ct ""` — ID roster default untuk tim CT saat map load (misal `"NAVI_TACTICAL"`; kosong = nonaktif/vanilla).
* `bot_savage_roster_t ""` — ID roster default untuk tim T saat map load (misal `"FAZE_AGGRO"`).
* `bot_savage_roster_tag "1"` — Tampilkan prefix `team_tag` pada nama bot (contoh: `[NV] b1t` vs `b1t`).
* `bot_savage_roster_respawn "0"` — `0` = Rename in-place (0 reset skor/round), `1` = Full YaPB respawn via `yb add` saat di Warmup agar skill YaPB ikut disesuaikan dengan `yapb_skill_min..max`.

---

## 3. Cross-Pillar Integration (Zero Coupling, SDK-First)

Seluruh komunikasi lintas plugin menggunakan **`include/cs2_sdk/cs2_api.inc`** (nol `callfunc_begin`, aman jika satu plugin dimatikan).

### 3.1 Role & Loadout Engine (`savage_roles_loadout.inl` & `savage_igl_strats.inl`)
1. **Deterministic Role Lock:**
   * Di `AssignBotRoleDeterministic(id)` (`savage_roles_loadout.inl:65`) dan `BalanceTeamRoles(CsTeams:team)` (`savage_igl_strats.inl:65`):
     * Jika bot terikat ke slot roster (`g_iBotRosterPack[id] != -1`), langsung kunci `g_iPlayerRole[id] = slot.iRole` dan lewati rotasi role otomatis.
     * Jika human player memilih role yang sama lewat `/role` (misal human memilih `AWPer`), hanya bot *non-signature* atau slot cadangan yang digeser—atau jika human mengambil `AWPer`, `bot_savage` menampilkan notifikasi bahwa role tersebut dishare (mendukung meta Double-AWP!).
2. **Weighted IGL Strat Selection (`savage_igl_strats.inl`):**
   * Saat IGL bot memilih strategi ronde otomatis (ketika human tidak memanggil `/rusha`, `/rushb`, `/default`, `/split`), gunakan `WeightedRandom(pack.iStratWeights[0..4])` alih-alih random seragam.
   * **Efek Nyata In-Game:** Melawan `FAZE_AGGRO` terasa penuh eksekusi cepat A/B dan force-buy berbahaya (`force_buy_chance 55%`), sedangkan melawan `NAVI_TACTICAL` terasa disiplin di Default 1-3-1 + Split Mid dengan ekonomi ketat!
3. **Weapon & Pistol Preference Override (`EquipBotByRole()`):**
   * Saat membeli/melengkapi senjata ronde (`savage_roles_loadout.inl:103`), hormati `slot.iPrimaryCsw` dan `slot.iPistolCsw` selama uang tim mencukupi (termasuk opsi `Jame` IGL membawa AWP di pack `DOUBLE_AWP_META`).

### 3.2 CT Bot M4A4 vs M4A1-S Bridge (`cs2_arsenal` Pillar 1)
Tambahkan 1 native opsional pada `bot_savage_ai.sma` & `include/cs2_sdk/cs2_api.inc`:
```pawn
// include/cs2_sdk/cs2_api.inc (Provider: bot_savage_ai)
native savage_get_bot_m4_pref(id); // returns -1 (auto), 0 (force M4A4), 1 (force M4A1-S)

stock cs2_api_get_bot_m4_pref(id)
{
    if (!cs2_api_has_savage_ai())
        return -1;

    return savage_get_bot_m4_pref(id);
}
```
Pada `cs2_arsenal/modules/mod_m4_split.inl:72` (`AssignBotM4Loadout(id)`), sisipkan prioritas **di bawah** override manual `/botm4` (`g_iBotM4Override[id]`) dan **di atas** fallback `pCvar_BotM4Mode`:
1. `g_iBotM4Override[id]` (override manual admin via `/botm4` — prioritas tertinggi)
2. **`cs2_api_get_bot_m4_pref(id)`** (preferensi persona roster: misal `rain` -> M4A4, `ropz` -> M4A1-S)
3. `pCvar_BotM4Mode` (fallback global eksisting: Mirror / Role / Mix 50-50 / All M4A4 / All M4A1-S)

### 3.3 Persona Chat Engine (`savage_esports_chat.inl`)
Tambahkan fungsi pemilih template berbasis `slot.iChatTone` (4 gaya terukur) pada respons `Execute_CareZoneCall()`, callout IGL, ekonomi, dan clutch/ace reactions:
* **`PRO_CALM`:** Ringkas, disiplin, bahasa comms LAN (*"Copy, anchoring A. Don't dry peek mid."*).
* **`HYPE_AGGRO`:** Energik, tempo cepat (*"Let's go! Flashing over, swing with me on contact!"*).
* **`TACTICAL_SHORT`:** Ultra-padat ala comms Eropa (*"A hot. Utility first. Playing trade."*).
* **`BANTER`:** Klasik 1.6 pub/scrim (*"Easy mid control, watch the flank boys!"*).
* Semua trigger chat v3.4 (`care a`, `flash me`, `drop c4`, `eco`, `force`, `drop awp`, dll.) tetap 100% kompatibel tanpa mengubah parser `HandleSay()`.

### 3.4 Competitive Halftime Side-Swap Preservation (`csgo_competitive`)
* Karena `g_iBotRosterPack[id]` dan `g_iBotRosterSlot[id]` disimpan per entity index bot (`1..32`), ketika `csgo_competitive` (`comp_match_flow.inl`) menukar tim CT <-> T pada ronde 12:
  * Identitas nama, role, M4 preference, dan gaya chat setiap bot **otomatis ikut berpindah sisi bersama timnya**.
  * Hook `Event_FreezeTimeStart()` di `savage_igl_strats.inl` cukup menyinkronkan `g_iActiveRoster[CS_TEAM_CT]` & `g_iActiveRoster[CS_TEAM_T]` dari mayoritas `g_iBotRosterPack` anggota tim saat ini (1 loop ringan `1..MaxClients`), sehingga bobot strategi IGL ikut berpindah secara akurat ke sisi baru!

---

## 4. Hot-Swap Execution Flow & YaPB Safety

### 4.1 Safe In-Place Hot-Swap (Default — Zero Disconnect)
Saat host/admin memilih roster dari menu `/roster` atau menjalankan `amx_bot_roster NAVI_TACTICAL FAZE_AGGRO`:
1. Kumpulkan seluruh bot di tim target (`is_user_bot(i) && safe_cs_get_user_team(i) == targetTeam`).
2. Petakan berurutan ke `slot_1 .. slot_5` dari pack yang dipilih.
3. Untuk setiap bot:
   * Set `g_iBotRosterPack[bot] = packIdx`, `g_iBotRosterSlot[bot] = slotIdx`.
   * Bangun nama baru (`[TAG] Name` jika `bot_savage_roster_tag 1`, jaga agar unik dan `< 31` char).
   * Panggil `set_user_info(bot, "name", szFinalName)` — mengubah nama bot seketika di scoreboard & killfeed **tanpa mengeluarkan bot dari server** dan **tanpa mengganggu quota YaPB (`yb_quota`)**!
   * Kunci `g_iPlayerRole[bot] = slot.iRole`.
4. Jadwalkan `TASK_SV_ROSTER_APPLY` (`0.2s`) untuk memicu re-sync M4/loadout di Freeze Time atau Warmup berikutnya.
5. Cetak banner ringkas di chat: `^4[Bot Roster]^1 CT -> ^3NAVI Structured Pack^1 | T -> ^3FaZe Aggro Pack^1 (Active immediately!)`.

### 4.2 Full Warmup Skill Re-Roll Mode (`bot_savage_roster_respawn 1`)
Hanya diizinkan jika `cs2_is_match_warmup()` bernilai `true` (dicegah keras saat match live agar tidak membatalkan skor!):
* Menjalankan `yb kickall`, lalu men-spawn ulang bot per tim memakai parameter skill spesifik roster: `yb add <random(skillMin, skillMax)> <personality> <team> 0 "<name>"`, lalu menautkan `g_iBotRosterPack` & `g_iBotRosterSlot` saat `client_putinserver`.

---

## 5. Interactive Menu & Commands

| Command (Chat / Console) | Akses | Fungsi |
| :--- | :--- | :--- |
| `/roster`, `.roster`, `/rosters` | Host / `ADMIN_CFG` | Buka menu AMXX 3 opsi: **Set CT Roster**, **Set T Roster**, **Preset Matchup (e.g. NAVI vs FaZe)**, **Reset to Vanilla YaPB** |
| `amx_bot_roster <ct_id> [t_id]` | `ADMIN_CFG` | Terapkan roster langsung via console/config (gunakan `"none"` untuk reset ke vanilla) |
| `amx_bot_roster_reload` | `ADMIN_CFG` | Muat ulang `configs/bot_rosters.ini` tanpa restart map |
| `amx_bot_roster_list` | Semua / Console | Cetak daftar semua roster pack yang valid beserta ringkasan 5 slot & strategi |

---

## 6. Acceptance Criteria & Verification Plan

* [ ] Dengan `bot_savage_roster_ct ""` dan `bot_savage_roster_t ""`, perilaku bot 100% identik dengan kondisi sebelum fitur ini (zero regression).
* [ ] Memilih preset `NAVI_TACTICAL` (CT) vs `FAZE_AGGRO` (T) lewat `/roster` langsung mengganti nama 9–10 bot di scoreboard tanpa kick/reconnect, tanpa bug `yb_quota`, dan tanpa error di `error_*.log`.
* [ ] Role setiap bot terkunci persis sesuai INI (terverifikasi lewat Blueprint 04 `savage_dump all` dan HUD telemetry).
* [ ] CT bots memakai M4A4 vs M4A1-S persis sesuai kolom `M4Pref` melalui native `cs2_api_get_bot_m4_pref()`, namun tetap menghormati override manual `/botm4`.
* [ ] Saat Halftime Side-Swap di `csgo_competitive.amxx` (Ronde 12), seluruh nama roster, role, dan profil strategi ikut berpindah tim dengan benar.
* [ ] Jika `bot_savage_ai.amxx` dimatikan sementara (isolation test), `cs2_arsenal.amxx` tetap berjalan dengan `0` runtime error berkat `cs2_sdk_native_filter` dan `cs2_api_has_savage_ai()`.
