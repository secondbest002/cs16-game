# Blueprint 05 — Post-Match Analytics Export & Local HLTV/Leetify Scoreboard

**Document ID:** `BP-05-POST-MATCH-ANALYTICS`  
**Target Plugin:** `csgo_competitive.amxx` (`csgo_competitive.sma` + sub-modul baru `cs2_competitive/modules/comp_analytics.inl`)  
**Architecture Baseline:** `cs16-game/ARCHITECTURE.md` (Option 1 + Option 2-Lite — 7 Active Custom Plugins + `include/cs2_sdk/` + Task ID Partition `50000 – 54999`)  
**AMXX Dependency:** `<json>` (`cstrike/addons/amxmodx/modules/json_amxx.dll` — **sudah tersedia** di instalasi AMXX 1.10 kamu!)  
**Output Storage:** `cs16-game/cstrike/addons/amxmodx/data/matches/match_YYYYMMDD_HHMMSS_<map>.json`  
**Local Viewer:** `cs16-game/tools/match_viewer.html` (Single-file zero-backend SPA)  
**Primary Objective:** Mengubah data combat yang sebagian sudah direkam oleh `cs2_competitive/core/comp_defs.inc` & `comp_economy_round.inl` menjadi analitik pasca-pertandingan setara CS2 Premier / HLTV / Leetify yang otomatis diekspor saat `EndMatch()` (`comp_dmg_mvp_hud.inl:350`) dan bisa dibuka instan di browser lokal.

---

## 1. Server-Side Metric Tracking Engine (`comp_analytics.inl`)

### 1.1 Data yang Sudah Ada vs Perlu Ditambahkan di `cs2_competitive/`
* **Sudah ada (`cs2_competitive/core/comp_defs.inc:28-66`):** `g_DamageDealt[33][33]`, `g_HitsDealt[33][33]`, `g_KillsInRound[33]`, `g_ScoreCT`, `g_ScoreT`, `g_RoundNumber`, `g_BombPlanter`, `g_BombDefuser`.
* **Cross-Pillar SDK Bridge (`include/cs2_sdk/cs2_api.inc` — sudah di-include di `csgo_competitive.sma:8` + `set_native_filter("cs2_sdk_native_filter")` di baris 23):**
  * **Role Bot/Pemain:** Gunakan wrapper terproteksi **`cs2_api_get_player_role(id)`** (bukan pemanggilan `savage_get_player_role()` mentah) agar tetap aman jika `bot_savage_ai.amxx` dimatikan.
  * **Akurasi Senjata M4A4 vs M4A1-S (`cs2_arsenal`):** Saat `weapon_id == CSW_M4A1`, panggil **`cs2_api_is_m4a4(attacker, bAvail)`** untuk mencatat kill & damage secara terpisah ke slot `"m4a4"` vs `"m4a1s"`.
  * **Akurasi Varian CS2 (`cs2_arsenal`):** Gunakan `cs2_api_has_variants()` + `cs2_get_user_weapon_variant(attacker, weapon_id, szVar, charsmax(szVar))` agar kill & damage dari 7 varian (`MAG-7`, `MP7`, `CZ75`, `Tec-9`, `PP-Bizon`, `Negev`, `Sawed-Off`) tercatat menggunakan nama varian aslinya, bukan slot base GoldSrc (`xm1014`, `mp5navy`, dst.).
  * **Akurasi Molotov (`csgo_molotov.sma:363`):** Deteksi `inflictor` dengan classname `"molotov_fire"` (atau `"molotov"`) untuk mencatat damage dan kill ke slot `"molotov"` serta akumulator `iUtilityDamage`.
  * **Opsional (BP-04 Integration):** Jika `cs2_api_get_team_strat()` / `cs2_api_get_team_economy()` tersedia di `cs2_sdk/cs2_api.inc`, simpan label strategi & ekonomi ronde pada `g_RoundHistory[r]` untuk memperkaya tooltip timeline.
* **Ditambahkan (Per-Match Accumulator Struct `MatchPlayerStats[33]` di `comp_analytics.inl`):**
  * **Identitas:** `szName[32]`, `szAuthId[35]`, `bool:bIsBot`, `iStartTeam` (CT/T pada Half 1), `iFinalTeam`, `iRole` (via `cs2_api_get_player_role(id)`).
  * **Core Combat:** `iKills`, `iDeaths`, `iAssists` (threshold >= 40 damage pada korban di ronde berjalan), `iHeadshots`, `iTotalDamage` (dibatasi maks HP korban sebelum mati agar overkill AWP tidak menggelembungkan ADR!).
  * **Impact & Clutch:**
    * `iFirstKills`, `iFirstDeaths` (kill pertama setiap ronde live).
    * `iMultiKills[6]` (indeks `2=2K`, `3=3K`, `4=4K`, `5=5K ACE`).
    * `iClutchAttempts[6]`, `iClutchWins[6]` (deteksi otomatis saat `aliveTeam == 1` vs `aliveEnemy >= 1`; catat 1v1..1v5 saat tim si clutch menang ronde).
    * `iKASTRounds` (ronde di mana pemain mendapat minimal 1 dari: **K**ill, **A**ssist, **S**urvived, atau **T**raded dalam `5.0` detik setelah mati).
  * **Utility & Objectives:** `iUtilityDamage` (HE + Molotov damage), `iEnemiesFlashed`, `Float:flTotalBlindTime`, `iPlants`, `iDefuses`, `iMVPs`.
  * **Per-Weapon Breakdown:** `iWeaponKills[33][CSW_ANALYTICS_SLOTS]` & `iWeaponDamage[33][CSW_ANALYTICS_SLOTS]` (30 base CSW + slot khusus `M4A4`, `MOLOTOV`, dan 7 varian CS2).
* **Per-Round Timeline Array (`g_RoundHistory[64]`, mendukung MR12 + hingga 6x Overtime MR3):**
  * `iRoundNum`, `iHalf` (`1`, `2`, `3+=OT`), `iWinnerTeam` (`CT`/`T`), `szWinReason` (`"elimination"`, `"bomb_exploded"`, `"bomb_defused"`, `"time_expired"`), `iScoreCT`, `iScoreT`, `szMvpName[32]`, `szMvpReason[32]`, `iFirstKillerSlot`, `iFirstVictimSlot`.

### 1.2 Aturan Hygiene Data & Titik Hook Aktual
1. **Hanya Rekam Ronde Live & Switchboard:** Kontrol di `csgo_competitive.sma` dengan `#define ENABLE_COMP_ANALYTICS 1` (include `cs2_competitive/modules/comp_analytics.inl`). Tracking aktif hanya pada `STATE_LIVE_HALF_1`, `STATE_LIVE_HALF_2`, `STATE_OVERTIME_1`, `STATE_OVERTIME_2`. Warmup DM dan Knife Round **diabaikan total**.
2. **Damage Clamping (Akurat ADR):** Tambahkan `Ham_TakeDamage` PRE (atau simpan `g_iPreHealth[victim]` sebelum `OnTakeDamagePost` di `cs2_competitive/modules/comp_economy_round.inl:648`), lalu catat `iEffectiveDmg = min(iPreHealth, floatround(flDamage))`. Sinkronkan pencatatan kill/assist/first-kill/clutch pada `OnPlayerKilledPost` (`comp_economy_round.inl:455`).
3. **Zero Memory Leak JSON:** Semua handle `JSON:` (`json_init_object()`, `json_init_array()`) wajib di-`json_free()` setelah `json_serial_to_file(hRoot, szFilePath, true)` dipanggil di **`EndMatch()` (`cs2_competitive/modules/comp_dmg_mvp_hud.inl:350`)**.
4. **Mid-Match Export Command:** Tambahkan command `.stats` / `amx_export_match` di `cs2_competitive/modules/comp_admin_chat.inl` untuk mengekspor snapshot manual jika match dihentikan sebelum selesai.

---

## 2. Canonical Match JSON Schema (`v1.0`)

Path: `cs16-game/cstrike/addons/amxmodx/data/matches/match_20261124_213045_de_dust2.json`

```json
{
  "schema_version": "1.0",
  "match_id": "20261124_213045_de_dust2",
  "timestamp_iso": "2026-11-24T21:30:45+08:00",
  "map": "de_dust2",
  "format": "MR12",
  "total_rounds": 22,
  "overtime_rounds": 0,
  "winner": "CT",
  "score": { "ct": 13, "t": 9, "half1_ct": 7, "half1_t": 5, "half2_ct": 6, "half2_t": 4 },
  "players": [
    {
      "name": "Player",
      "authid": "STEAM_0:0:123456",
      "is_bot": false,
      "role": "Entry Fragger",
      "final_team": "CT",
      "kills": 24,
      "deaths": 14,
      "assists": 5,
      "headshots": 13,
      "hs_pct": 54.2,
      "total_damage": 2380,
      "adr": 108.2,
      "kast_pct": 77.3,
      "first_kills": 6,
      "first_deaths": 2,
      "multi_kills": { "2k": 4, "3k": 2, "4k": 0, "5k": 0 },
      "clutches": { "1v1_win": 1, "1v1_att": 1, "1v2_win": 1, "1v2_att": 2, "1v3_win": 0, "1v3_att": 0 },
      "utility_damage": 185,
      "enemies_flashed": 9,
      "plants": 2,
      "defuses": 1,
      "mvps": 5,
      "weapons": {
        "ak47": { "kills": 11, "damage": 1120 },
        "m4a4": { "kills": 8, "damage": 840 },
        "deagle": { "kills": 3, "damage": 290 },
        "molotov": { "kills": 2, "damage": 130 }
      }
    }
  ],
  "rounds": [
    {
      "round": 1,
      "half": 1,
      "winner": "CT",
      "reason": "bomb_defused",
      "score_ct": 1,
      "score_t": 0,
      "mvp": "Player",
      "mvp_reason": "Defused the Bomb"
    }
  ]
}
```

---

## 3. Local Zero-Backend Viewer (`cs16-game/tools/match_viewer.html`)

Sebuah file HTML/CSS/JS tunggal (self-contained, 0 external CDN wajib, berjalan langsung lewat `file:///` di Chrome/Edge):

### 3.1 Fitur Utama UI
1. **Instant Folder / Multi-File Loader:**
   * Drag-and-drop 1 atau banyak file `match_*.json` sekaligus, ATAU klik tombol **"Open `data/matches/` Folder"** (via Web File System Access API `showDirectoryPicker()`) untuk memuat seluruh riwayat pertandingan sekaligus dengan daftar match di sidebar kiri.
2. **HLTV 2.0-Style Rating Approximation (Client-Side Computed):**
   * Dihitung otomatis dari JSON tanpa membebani Pawn:
     * `KPR = kills / total_rounds`, `DPR = deaths / total_rounds`, `APR = assists / total_rounds`
     * `Impact = 2.13 * KPR + 0.42 * APR - 0.41 + (first_kills - first_deaths) * 0.03`
     * `Rating = 0.0073 * KAST% + 0.3591 * KPR - 0.5329 * DPR + 0.2372 * Impact + 0.0032 * ADR + 0.1587`
3. **Tampilan Dashboard CS2 Premier Dark Theme:**
   * **Header Scoreboard:** Map badge, skor besar `CT 13 — 9 T`, breakdown Half 1 / Half 2 / OT, dan kartu **Match MVP** (Rating tertinggi).
   * **Interactive Round Timeline Strip (24+ ikon):** Baris atas CT, baris bawah T, lengkap dengan ikon kondisi kemenangan (Skull = Elimination, Bomb = Exploded, Pliers = Defused, Clock = Time), tooltip hover menampilkan MVP ronde.
   * **Sortable Team Tables (CT vs T):** Kolom `Player (Role badge)`, `K-D-A`, `+/-`, `ADR`, `HS%`, `KAST%`, `FK/FD`, `Util Dmg`, `Clutch`, `Rating` (highlight hijau >= 1.15, merah <= 0.85).
   * **Player Deep-Dive Drawer:** Klik nama pemain mana pun untuk melihat bar chart **Weapon Breakdown** (AK-47, M4A4, M4A1-S, AWP, MAG-7, Molotov, dst.) dan multi-kill / clutch badges.
   * **Career Trend View (Jika memuat >1 match):** Grafik garis perkembangan Rating, ADR, dan Winrate per map (`de_dust2`, `de_inferno`, `de_mirage`, `de_train`).

---

## 4. Acceptance Criteria & Verification Plan

* [ ] `scripting/build_all.bat` mengompilasi 7/7 plugin aktif termasuk `csgo_competitive.sma` dengan `#include <json>` (`0` error, `0` memory leak, `0` `.amxx` nyasar).
* [ ] Setelah 1 match MR12 selesai (dipicu dari `comp_dmg_mvp_hud.inl:350` `EndMatch()`) atau via `amx_export_match`, file JSON otomatis tercipta di `cstrike/addons/amxmodx/data/matches/` dengan struktur schema `v1.0` valid.
* [ ] Total kills & deaths di JSON 100% sinkron dengan scoreboard akhir, `m4a4` vs `m4a1s` & varian CS2 terpisah akurat via `cs2_sdk/cs2_api.inc`, dan ADR tidak melebihi batas realistis karena overkill clamping.
* [ ] **Cross-Pillar Isolation Test:** Jika `bot_savage_ai.amxx` atau `cs2_arsenal.amxx` dimatikan sementara, ekspor JSON tetap sukses dengan `0` runtime error berkat `cs2_sdk_native_filter` (`role` fallback ke `"Unassigned"`, senjata fallback ke nama base GoldSrc).
* [ ] `tools/match_viewer.html` dapat dibuka offline di browser, memuat seluruh folder `data/matches/`, menampilkan timeline ronde + rating + weapon breakdown tanpa error console browser.
