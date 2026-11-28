# BP-03: Ekonomi & Eksekusi Taktis Molotov Berbasis Role (Map-Agnostic)

* **Author:** Antigravity
* **Status:** Implemented (Pending Runtime Verification)
* **Date:** 2026-11-27
* **Series:** Bot Tactical Utility & Molotov Integration (`BP-03` dari `3`)
* **Depends On:** `BP-01` (`savage_yapb_bridge.inl`), `BP-02` (`savage_hazard_ai.inl` & native `csgo_molotov`)

--------------------------------------------------------------------------------

## Overview

Blueprint ini menyatukan **siklus hidup Molotov/Incendiary** bagi bot ke dalam sistem ekonomi dan taktik `bot_savage_ai.amxx` tanpa bergantung pada koordinat yang di-*hardcode* per map (*map-agnostic*):
1. **Unified Role Economy**: Mematikan pembelian acak di `csgo_molotov.sma` saat `bot_savage_ai` aktif, dan memindahkan keputusan pembelian Molotov ke `savage_roles_loadout.inl` sesuai *economy call* tim (`ECON_FULL` / `ECON_FORCE` vs `ECON_ECO`) serta *role* bot (`ROLE_SUPPORT`, `ROLE_ENTRY`, `ROLE_LURKER`, `ROLE_IGL`), sekaligus menyelesaikan konflik slot `CSW_HEGRENADE`.
2. **Map-Agnostic Tactical Molotov Execution**: Memanfaatkan metadata *navigation graph* YaPB (`YB_NODE_FLAG_GOAL`, `YB_NODE_FLAG_CAMP`, `YB_NODE_FLAG_SNIPER`, `YB_NODE_FLAG_NARROW`), entitas `func_bomb_target`, dan info posisi musuh (`yb_get_bot_last_enemy_origin` / `g_vecTeamThreatOrigin`) untuk mengeksekusi lemparan Molotov di 3 fase taktis Tier 1 CS:
   * **CT Chokepoint Delay** (awal ronde menahan push *narrow choke*).
   * **T Site Execute / Corner Clear** (membakar sudut berlindung `CAMP`/`SNIPER` saat tim masuk ke *bombsite*).
   * **Post-Plant / Retake Denial** (membakar area *default plant* C4 saat menghalangi *defuse* atau memaksa pembela keluar saat *retake*).

---

## The Problem

Hasil inspeksi terhadap `csgo_molotov.sma` dan `bot_savage/` mengungkap tiga masalah utama pada sisi pembelian dan penggunaan Molotov oleh bot:

1. **Pembelian Acak Menabrak Ekonomi Tim & Bentrok Slot HE Grenade**:
   * `csgo_molotov.sma:270-292` (`Task_BotBuyMolotov`) menjalankan peluang acak **35%** bagi setiap bot yang memiliki uang `>= $2200` untuk memanggil `buy_molotov(id)`.
   * Karena `buy_molotov(id)` (`csgo_molotov.sma:201-204`) menggunakan entitas `weapon_hegrenade` dan menandai kepemilikan lewat bitmask internal `molotov`, pemanggilan ini:
     * Mengabaikan perintah ekonomi IGL (`g_iTeamEconomyCall[team] == ECON_ECO` / `ECON_HALF` di `savage_drop_economy.inl`).
     * Menimpa `weapon_hegrenade` yang baru saja diberikan oleh `EquipBotByRole()` (`savage_roles_loadout.inl:217, 276, 335`) sambil tetap memotong uang `$500`.
2. **Lemparan Molotov Tidak Punya Otak Taktis (*Area Denial*)**:
   * Karena entitas di tangan bot adalah `weapon_hegrenade`, YaPB C++ memperlakukan Molotov sebagai granat ledak (`YB_TASK_THROW_EXPLOSIVE`)—hanya dilempar ketika musuh terlihat langsung atau malah tidak pernah dipakai untuk menahan *chokepoint* dan membersihkan sudut buta.
3. **Ketergantungan Koordinat Hardcode Tidak Skalabel**:
   * Pendekatan lama seperti `Task_TacticalMidLineupScanner` (`savage_scavenge_utils.inl:297-313`) memakai koordinat spesifik `de_dust2`, sehingga tidak berfungsi di `de_inferno`, `de_mirage`, `de_nuke`, atau map workshop/custom lainnya.

---

## Goals

* Mengekspos native pemberian/pembelian Molotov terprogram dari `csgo_molotov.amxx` (`csgo_give_user_molotov`) ke `cs2_api.inc` agar `savage_roles_loadout.inl` memegang kendali tunggal atas inventaris utility bot.
* Menyelesaikan konflik slot `CSW_HEGRENADE` vs Molotov secara eksplisit: bot yang dialokasikan Molotov tidak diberikan `weapon_hegrenade` ganda pada ronde yang sama, dan status kepemilikan Molotov terbaca akurat via `csgo_get_user_molotov(id)`.
* Membangun **Tactical Molotov Evaluator** (`Task_TacticalMolotovScanner`) berbasis metadata graph YaPB (`yapb.inc`) yang otomatis beradaptasi di semua map (`de_*`) dengan validasi garis lempar (`EngFunc_TraceLine`), perhitungan sudut balistik berdasarkan jarak, dan *friendly-fire guard*.

---

## Non-Goals

* **Tidak mengubah batas engine `MAX_WEAPONS = 32`**; Molotov tetap berjalan di atas entitas `weapon_hegrenade` milik `csgo_molotov.amxx` yang sudah terbukti stabil.
* **Tidak membuat file konfigurasi koordinat lineup per-map** (`.ini` / `.json`); pemilihan target menggunakan properti spasial dinamis dari graph YaPB dan entitas map.
* **Tidak memaksa bot melempar Molotov saat sedang dalam kontak duel langsung** (selalu tunduk pada aturan *Combat Preemption* dari `BP-01`).

---

## Open Questions

* Tidak ada pertanyaan terbuka; seluruh mekanik ekonomi, *role*, dan native graph YaPB telah dipetakan.

---

## Critical User Journeys (CUJs)

### CUJ 1: Alokasi Molotov Sesuai Role & Economy Call di Freeze Time

* **Given**: `bot_savage_ai.amxx` dan `csgo_molotov.amxx` aktif. Tim berada pada fase *Freeze Time* ronde kompetitif MR12.
* **When**:
  * Skenario A: IGL memanggil `ECON_ECO` atau `ECON_HALF`.
  * Skenario B: IGL memanggil `ECON_FULL` (atau uang bot `>= $4200`), dan bot memiliki *role* `ROLE_SUPPORT`, `ROLE_ENTRY`, `ROLE_LURKER`, atau `ROLE_IGL`.
* **Then**:
  * Pada Skenario A (`ECON_ECO`), `Task_BotBuyMolotov` di `csgo_molotov.sma` tidak berjalan (dinonaktifkan saat `cs2_api_has_savage_ai()` aktif), dan `EquipBotByRole()` tidak membeli Molotov sehingga tabungan bot tetap utuh.
  * Pada Skenario B (`ECON_FULL`), bot dengan *role* prioritas (misalnya `ROLE_SUPPORT` dan `ROLE_ENTRY` sebagai Molotov, sementara `ROLE_IGL` / `ROLE_LURKER` membawa HE Grenade agar komposisi utility tim seimbang) menerima Molotov via `cs2_api_give_user_molotov(id, false)` tanpa tabrakan dengan `CSW_HEGRENADE`.

### CUJ 2: CT Chokepoint Delay di Awal Ronde (Map-Agnostic)

* **Given**: Ronde baru dimulai (`12s – 25s` pertama), bot CT (`ROLE_SUPPORT` / `ROLE_ENTRY`) memegang Molotov, bergerak mendekati *bombsite* atau jalur tengah, dan tidak sedang bertempur (`yb_get_bot_enemy(id) == YB_INVALID_ENTITY_INDEX`).
* **When**: `Task_TacticalMolotovScanner` memindai node graph YaPB di depan arah gerak bot (`350.0 – 850.0` unit) yang memiliki flag `YB_NODE_FLAG_NARROW` (lorong sempit / pintu masuk) dan mendekati arah suara langkah musuh (`g_iSoundTarget[id]`) atau *team threat* (`g_vecTeamThreatOrigin[CS_TEAM_CT]`).
* **Then**:
  1. Evaluator memverifikasi tidak ada rekan setim CT dalam radius `220.0` unit dari titik target dan tidak ada api Molotov aktif di sana (`BP-02`).
  2. Evaluator menghitung titik bidik balistik dan memanggil `Savage_Bridge_RequestUtilityThrow(id, CSW_HEGRENADE, vecBallisticAim, 0.65)` (`BP-01`).
  3. Bot melempar Incendiary tepat ke mulut lorong sempit, menunda push T secara alami di map mana pun.

### CUJ 3: T Site Execute & Retake Denial

* **Given**: Bot T membawa Molotov saat mendekati `func_bomb_target` (`distance <= 900.0` unit), ATAU bot CT/T berada dalam fase *post-plant* (C4 sudah terpasang) dan mendekati lokasi C4.
* **When**: `Task_TacticalMolotovScanner` mencari node terdekat dari pusat *bombsite* atau entitas C4 yang bertipe `YB_NODE_TYPE_CAMP` / `YB_NODE_TYPE_SNIPER` (sudut persembunyian) yang tidak terhalang tembok penuh dari posisi lempar bot.
* **Then**: Bot mengunci pandangan melalui `Savage_Bridge_RequestUtilityThrow`, melempar Molotov ke sudut *camp* / area *plant*, dan langsung kembali memegang senjata utama untuk masuk ke *site*.

---

## Technical Plan (Architecture)

### 1. Ekspansi Native `csgo_molotov` & Penyelesaian Konflik Slot

Di `csgo_molotov.sma`:
1. Tambahkan native `csgo_give_user_molotov(id, bool:bDeductMoney = false)`:
   * Memeriksa `molotovEnabled`, `is_user_alive(id)`, dan `!cs2_get_bit(id, molotov)`.
   * Jika `bDeductMoney == true`, memeriksa `cs_get_user_money(id) >= molotovPrice` dan memotong uangnya.
   * Mengatur `cs2_set_bit(id, molotov)`, memberikan `weapon_hegrenade` jika pemain belum memilikinya (atau mengonversi slot HE yang ada menjadi Molotov secara bersih), dan memanggil `ExecuteHamB(Ham_Item_Deploy, ...)` jika sedang memegang `CSW_HEGRENADE`.
2. Pada `player_spawned(id)` (`csgo_molotov.sma:258`), lewati penjadwalan `Task_BotBuyMolotov` apabila `cs2_api_has_savage_ai()` bernilai `true`, sehingga `bot_savage_ai.amxx` menjadi satu-satunya otoritas ekonomi utility bot.

#### Matriks Distribusi Utility per Role (`savage_roles_loadout.inl`)

| Role Bot | Full Buy (`>= $4200` / Warmup) | Force Buy (`$2200 – $4199`) | Eco (`< $2200` / `ECON_ECO`) |
| :--- | :--- | :--- | :--- |
| **`ROLE_ENTRY`** | **Molotov** + 1x Flashbang | 1x Flashbang | Tanpa utility |
| **`ROLE_SUPPORT`** | **Molotov** + 1x Smoke + 2x Flashbang | 1x Smoke + 1x Flashbang | Tanpa utility |
| **`ROLE_LURKER`** | **Molotov** (50%) / HE (50%) + 1x Smoke + 1x Flash | 1x Smoke | Tanpa utility |
| **`ROLE_IGL`** | 1x HE Grenade + 1x Smoke + 2x Flashbang | 1x Flashbang | Tanpa utility |
| **`ROLE_AWPER`** | 1x Smoke + 1x Flashbang | 1x Flashbang | Tanpa utility |

*(Catatan: Karena `CSW_HEGRENADE` hanya menampung 1 item fisik per pemain di engine CS 1.6, pembagian peran di atas memastikan setiap skuad 5-bot selalu membawa kombinasi **2–3 Molotov + 1–2 HE Grenade + 3 Smoke + 5 Flashbang** tanpa saling menimpa.)*

### 2. Arsitektur Map-Agnostic Tactical Molotov Solver (`savage_molotov_tactics.inl`)

```text
[Task_TacticalMolotovScanner @ 0.5s]
   |
   +--> Filter Bot: Hidup, Bot, cs2_api_get_user_molotov(id) == true,
   |                g_iBotLeaseType[id] == LEASE_NONE, yb_get_bot_enemy(id) == INVALID
   |
   +--> Mode Evaluasi Berdasarkan Konteks Ronde:
          |-- [1] Post-Plant / Retake (C4 tertanam, jarak bot ke C4 <= 950u)
          |     -> Target: Node CAMP/GOAL terdekat ke entitas C4
          |-- [2] Site Execute (Jarak bot ke func_bomb_target <= 900u)
          |     -> Target: Node CAMP/SNIPER di dalam radius 420u dari bombsite
          |-- [3] Chokepoint Delay / Threat Denial (flRoundElapsed <= 35s atau ada Sound/TeamThreat)
                -> Target: Node NARROW di arah threat / depan jalur bot (350u..850u)
   |
   +--> Validasi Keamanan & Fisika:
          1. Cek tidak ada api Molotov aktif dalam radius 260u dari target (BP-02).
          2. Cek tidak ada rekan setim hidup dalam radius 220u dari target (Anti-Teamtrap).
          3. Hitung titik aim balistik (kompensasi gravitasi sv_gravity=800 & jarak) + TraceLine cek langit-langit.
          4. Panggil Savage_Bridge_RequestUtilityThrow(id, CSW_HEGRENADE, vecAimTarget, 0.65) (BP-01).
```

#### Perhitungan Sudut Balistik (`ComputeMolotovBallisticAim`)
Agar botol Molotov mendarat tepat di titik target (`vecTarget`) dan tidak memantul terlalu jauh atau membentur kusen pintu atas:
* Hitung jarak horizontal $d = \sqrt{\Delta x^2 + \Delta y^2}$ dan beda tinggi $\Delta z = z_{\text{target}} - z_{\text{eyes}}$.
* Kecepatan awal lemparan granat GoldSrc (`CSW_HEGRENADE`) adalah sekitar $v_0 \approx 750\text{ unit/detik}$ dengan gravitasi standar $g = 800\text{ unit/detik}^2$.
* Gunakan kompensasi ketinggian target:
  $$z_{\text{aim}} = z_{\text{target}} + \frac{0.5 \cdot g \cdot (d / 750)^2}{1.15}$$
* Lakukan `engfunc(EngFunc_TraceLine, vecEyes, vecAimCheck, IGNORE_MONSTERS, id, tr)` untuk memastikan lintasan awal sejauh minimal `220.0` unit bebas dari tembok/langit-langit rendah (misalnya di B Tunnels atau Underpass) sebelum mengizinkan lemparan.

---

## Alternatives Considered

### Alternative A: Membuat File Konfigurasi Koordinat Lineup Per-Map (`configs/molotov_lineups/*.ini`)

* **Pros**: Lemparan sangat presisi pada titik-titik ikonik tertentu di map populer.
* **Cons**: Membutuhkan ratusan koordinat manual untuk setiap map dan tim, tidak berjalan di map baru/custom, dan mudah meleset jika posisi spawn atau kecepatan gerak bot sedikit bergeser.
* **Reason for Rejection**: Tidak sesuai dengan kebutuhan soloplay lintas banyak map; pemanfaatan metadata node graph YaPB (`YB_NODE_FLAG_NARROW`, `YB_NODE_TYPE_CAMP`, `YB_NODE_TYPE_SNIPER`) jauh lebih adaptif dan nol konfigurasi manual.

### Alternative B: Mengubah Source C++ YaPB untuk Menambah Task Baru `YB_TASK_THROW_MOLOTOV`

* **Pros**: Menyatu dengan *decision tree* internal C++ YaPB.
* **Cons**: Membutuhkan modifikasi di beberapa file C++ (`constant.h`, `tasks.cpp`, `combat.cpp`, `yapb_amxx.cpp`) dan kompilasi ulang biner DLL, sementara deteksi status Molotov tetap berada di plugin AMXX `csgo_molotov.amxx`.
* **Reason for Rejection**: Pendekatan Pawn melalui `savage_yapb_bridge.inl` (`BP-01`) + query node graph `yapb.inc` memberikan hasil yang sama bersihnya dengan tetap menjaga seluruh logika taktis dalam satu ekosistem build (`build_all.bat`).

---

## Non-Functional Requirements

* **Rate-Limiting & Anti-Spam**: Maksimal **1 lemparan Molotov taktis per tim setiap `4.0` detik** (`g_flNextTeamMolotovTime[CsTeams]`) agar satu tim tidak membuang 3 Molotov sekaligus ke titik yang sama saat *execute*.
* **Kompatibilitas Gamemode**: `Task_TacticalMolotovScanner` wajib melewati eksekusi saat `IsWarmupActive()`, `IsKnifeRoundActive()`, `IsDeathmatchActive()`, atau `g_bInFreezeTime` bernilai `true`.
* **Keamanan Stack Build**: Tetap menjaga 7 target plugin di `build_all.bat` terkompilasi bersih dengan exit code `0`.

---

## Detailed Implementation Plan

### Target Files to Create/Modify

1. **`cs16-game/cstrike/addons/amxmodx/scripting/csgo_molotov.sma`** (**Modify**)
   * **Type / Scope**: Molotov Provider & Bot Buy Delegation.
   * **Proposed Changes**:
     * Daftarkan native `csgo_give_user_molotov` di `plugin_natives()` untuk pemberian/pembelian Molotov secara terprogram tanpa konflik slot `weapon_hegrenade`.
     * Pada `player_spawned(id)` (baris `258`), lewati penjadwalan `Task_BotBuyMolotov` jika `LibraryExists("bot_savage_ai", LibType_Library)` bernilai `true`.

2. **`cs16-game/cstrike/addons/amxmodx/scripting/include/cs2_sdk/cs2_api.inc`** (**Modify**)
   * **Type / Scope**: Shared Cross-Plugin SDK.
   * **Proposed Changes**: Deklarasikan `csgo_get_user_molotov(id)` dan `csgo_give_user_molotov(id, bool:bDeductMoney = false)` beserta wrapper aman `cs2_api_get_user_molotov(id)` dan `cs2_api_give_user_molotov(id, bool:bDeductMoney = false)`.

3. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage/core/savage_taskids.inc` & `savage_defs.inc`** (**Modify**)
   * **Type / Scope**: Task ID & Tactical State Definitions.
   * **Proposed Changes**: Tambahkan `#define TASK_SV_MOLOTOV_SCAN 30013` di `savage_taskids.inc`, serta variabel cooldown tim `Float:g_flNextTeamMolotovTime[CsTeams]` dan status `bool:g_bBotHasMolotov[33]` di `savage_defs.inc`.

4. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage/modules/savage_roles_loadout.inl` & `savage_drop_economy.inl`** (**Modify**)
   * **Type / Scope**: Role Utility Allocation & Economy Sync.
   * **Proposed Changes**: Perbarui `EquipBotByRole()` (baris `212–338`) agar memberikan Molotov via `cs2_api_give_user_molotov(id, false)` pada `ROLE_SUPPORT`, `ROLE_ENTRY`, dan `ROLE_LURKER` saat `bFullBuy` dan `bAllowUtility` aktif (menggantikan alokasi `weapon_hegrenade` pada role tersebut), serta membatalkan status Molotov saat perintah `ECON_ECO` *full refund* dipanggil di `savage_drop_economy.inl`.

5. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage/modules/savage_molotov_tactics.inl`** (**Create**)
   * **Type / Scope**: Map-Agnostic Tactical Molotov Evaluator & Ballistic Solver.
   * **Proposed Changes**: Implementasikan `Savage_MolotovTactics_ResetRound()`, `Task_TacticalMolotovScanner()`, pemilihan target node dinamis (`NARROW`, `CAMP`, `SNIPER`, `GOAL`) via `yapb.inc`, filter anti-teamfire dan anti-overlap api aktif (`BP-02`), perhitungan titik bidik balistik dengan `TraceLine` clearance check, serta pemicuan `Savage_Bridge_RequestUtilityThrow(id, CSW_HEGRENADE, vecAimTarget, 0.65)` (`BP-01`).

5b. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage/modules/savage_drop_economy.inl`** (**Modify**)
   * **Type / Scope**: Full Eco Refund Sync.
   * **Proposed Changes**: Panggil `cs2_api_clear_user_molotov(b)` tepat setelah `cs2_api_refund_player_all(b, cost)` pada blok `bFullRefund` (baris `245–254`) agar flag kepemilikan Molotov tersinkronisasi bersih saat bot melakukan full eco refund.

6. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage_ai.sma`** (**Modify**)
   * **Type / Scope**: Plugin Entrypoint.
   * **Proposed Changes**: Tambahkan `#include "bot_savage/modules/savage_molotov_tactics.inl"` dan jadwalkan `set_task(0.5, "Task_TacticalMolotovScanner", TASK_SV_MOLOTOV_SCAN, _, _, "b")` di `plugin_init()`.

### Verification Plan

* **Compilation / Build**: Jalankan `cs16-game\cstrike\addons\amxmodx\scripting\build_all.bat` dan pastikan seluruh 7 plugin (`cs2_arsenal`, `cs2_modern_engine`, `bot_savage_ai`, `cs2_client_studio`, `csgo_molotov`, `csgo_competitive`, `redm_listen_engine`) lulus kompilasi (`7/7 exit 0`).
* **Role Economy Audit**: Mulai ronde kompetitif MR12 di `de_dust2` dan `de_inferno`; periksa dengan spectate bahwa bot `Support` dan `Entry` memegang model `v_molotov.mdl` pada ronde *Full Buy*, dan tidak membeli Molotov saat IGL memanggil `eco`.
* **End-to-End Tactical Flow (`BP-01` + `BP-02` + `BP-03`)**:
  1. Verifikasi bot T/CT melempar Molotov ke *chokepoint* / sudut *bombsite* tanpa jitter sudut pandang (`BP-01` + `BP-03`).
  2. Verifikasi bot lawan di sekitar api menghindar keluar dari radius bakar tanpa mati konyol, dan bot Support yang memiliki Smoke memadamkan api musuh yang menghalangi jalur masuk (`BP-02`).

--------------------------------------------------------------------------------

*Drafted with EGM*
