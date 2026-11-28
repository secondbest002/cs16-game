# BP-02: Kesadaran Zona Api Molotov & Refleks Smoke-Extinguish

* **Author:** Antigravity
* **Status:** Implemented (Pending Runtime Verification)
* **Date:** 2026-11-27
* **Series:** Bot Tactical Utility & Molotov Integration (`BP-02` dari `3`)
* **Depends On:** `BP-01` (`savage_yapb_bridge.inl`)

--------------------------------------------------------------------------------

## Overview

Blueprint ini memberikan bot "kesadaran spasial" terhadap zona api Molotov/Incendiary yang sedang aktif di map, serta refleks kompetitif Tier 1 untuk:
1. Menghindari berdiri di dalam atau berlari menembus kobaran api (`LEASE_HAZARD_EVADE`), dan
2. Memadamkan api Molotov musuh yang memblokir jalur rotasi/eksekusi tim menggunakan **Smoke Grenade** (`LEASE_UTILITY_THROW`).

Dengan adanya kesadaran bahaya yang nyata ini, pengurangan *damage* buatan sebesar `0.35x` khusus bot di `csgo_molotov.sma:443-444` dapat dihapus sehingga aturan *fire damage* berlaku adil antara pemain manusia dan bot.

---

## The Problem

Dari hasil audit `csgo_molotov.sma` dan `bot_savage/`, terdapat empat celah teknis yang membuat bot buta terhadap api:

1. **YaPB C++ Tidak Mengenali Entitas Api Kustom**:
   * `csgo_molotov.sma:361-374` membuat dua entitas terpisah saat botol pecah: entitas pusat ber-classname `"molotov"` (`molotovClassName`) dan entitas efek `"molotov_fire"` (`info_target`).
   * Mesin YaPB C++ hanya menghindari entitas `"grenade"` bawaan sebelum meledak (`yb_avoid_grenades "1"`). Begitu Molotov berubah menjadi `"molotov"` + `"molotov_fire"`, YaPB menganggap area tersebut aman untuk dilewati atau dijadikan posisi *camp*.
2. **Perangkap Koordinat pada Entitas `"molotov_fire"`**:
   * Di `csgo_molotov.sma:409` dan `:614-634`, fungsi `random_fire()` memanggil `ground_z(origin, entFire)` yang memindahkan `pev_origin` milik `"molotov_fire"` secara acak di sekitar radius api setiap `0.1` detik.
   * Jika `bot_savage_ai` mencari `"molotov_fire"` dan membaca `pev_origin`-nya, titik pusat api akan melompat-lompat tidak akurat. Titik pusat api yang sebenarnya dan stabil disimpan di entitas `"molotov"` (`pev(entMolotov, pev_origin, origin)` pada baris `407`), lengkap dengan `pev_team` dan sisa waktu bakar `THINK_COUNT`.
3. **Workaround Damage Buatan (`0.35x`)**:
   * Karena bot tidak bisa menghindar, `csgo_molotov.sma:443-444` mengurangi *self-damage* bot menjadi `35%`. Akibatnya bot terbiasa berjalan santai di atas api sendiri tanpa konsekuensi taktis.
4. **Tidak Ada Refleks Smoke-Extinguish**:
   * `csgo_molotov.sma:463-490` (`extinguish_molotov`) sudah mendukung pemadaman api otomatis apabila ada entitas `"grenade"` bermodel `smokegrenade` berada dalam jarak `molotovRadius * 0.75` (`112.5` unit) dari pusat `"molotov"`. Namun bot tidak pernah memanfaatkan mekanik ini saat jalur masuk tim diblokir api.

---

## Goals

* Mengekspos registry api Molotov aktif dari `csgo_molotov.amxx` ke `cs2_api.inc` secara aman tanpa pemindaian entitas berulang di setiap tick.
* Membuat bot mendeteksi apakah posisinya saat ini atau node rute YaPB berikutnya (`yb_get_bot_path_node`) bersinggungan dengan zona api (`molotovRadius + 48.0` unit safety margin), lalu mengalihkan rute ke node aman terdekat melalui `Savage_Bridge_RequestEvade` (`BP-01`).
* Membuat bot Support / Entry di dekat titik api musuh (yang menghalangi jalur tim dan tersisa durasi bakar `>= 2.5s`) otomatis melempar Smoke Grenade ke titik pusat api untuk memadamkannya.
* Menghapus *workaround* `tempDamange *= 0.35` di `csgo_molotov.sma` agar *damage* api konsisten untuk semua pemain.

---

## Non-Goals

* **Tidak mengubah logika visual sprite api maupun rumus radius bakar** di `csgo_molotov.sma`.
* **Belum mengatur pembelian Molotov oleh bot maupun lemparan Molotov ofensif/defensif** (cakupan `BP-03`).
* **Tidak memaksa bot memadamkan api di belakang mereka** yang tidak menghalangi rute/pergerakan tim (mencegah pemborosan Smoke).

---

## Open Questions

* Tidak ada pertanyaan terbuka untuk `BP-02`; seluruh mekanisme fisik pemadaman dan struktur entitas di `csgo_molotov.sma` telah terverifikasi langsung dari kode sumber.

---

## Critical User Journeys (CUJs)

### CUJ 1: Menghindar Saat Berdiri di Dalam atau Mendekati Zona Api

* **Given**: Sebuah Molotov meledak di `de_dust2` B Tunnels dan menciptakan zona api beradius `150.0` unit (`molotovRadius`). Seorang bot berada di jarak `120.0` unit dari pusat api atau jalur node berikutnya (`yb_get_bot_path_node(id, 0..2)`) melintasi lingkaran `198.0` unit (`150.0 + 48.0` margin).
* **When**: `Task_HazardMonitor` (berjalan setiap `0.25s`) mengevaluasi posisi bot dan 3 node rute terdekatnya terhadap daftar api aktif.
* **Then**:
  1. Sistem mencari node graph YaPB terdekat di luar radius bahaya (`distance > molotovRadius + 64.0`) yang terhubung dan dapat dijangkau (`yb_is_node_reachable`).
  2. Sistem memanggil `Savage_Bridge_RequestEvade(id, vecSafeNodeOrigin, 1.25)` (`LEASE_HAZARD_EVADE`, prioritas `3`).
  3. Bot langsung berbelok menjauh dari pusat api tanpa terjebak berdiri di dalam api maupun terjebak *loop* bolak-balik.

### CUJ 2: Memadamkan Molotov Musuh yang Memblokir Jalur Push

* **Given**: Tim T sedang mengeksekusi masuk ke *bombsite*, lalu CT melempar Incendiary/Molotov yang memblokir pintu masuk tepat di depan rute bot (`sisa durasi >= 2.5s`, pemilik adalah tim lawan), dan salah satu bot rekan setim di jarak `<= 950.0` unit memiliki `CSW_SMOKEGRENADE` serta tidak sedang berduel (`yb_get_bot_enemy(id) == YB_INVALID_ENTITY_INDEX`).
* **When**: `Task_HazardMonitor` mendeteksi jalur tim terhambat oleh api milik tim lawan dan belum ada granat Smoke yang sedang melayang menuju titik api tersebut.
* **Then**:
  1. Sistem memilih 1 bot terbaik (prioritas `ROLE_SUPPORT`, lalu `ROLE_ENTRY`) yang memiliki garis pandang/lempar valid ke titik pusat api.
  2. Sistem memanggil `Savage_Bridge_RequestUtilityThrow(iBot, CSW_SMOKEGRENADE, vecFireCenter, 0.65)` dan menandai api tersebut sebagai *claimed* (`g_bFireExtinguishClaimed`) agar 4 bot tidak melempar Smoke bersamaan ke 1 api.
  3. Smoke mendarat dalam radius `112.5` unit (`molotovRadius * 0.75`), memicu `extinguish_molotov()` di `csgo_molotov.sma`, dan tim melanjutkan push.

---

## Technical Plan (Architecture)

### 1. Kontrak Data Lintas Plugin (`csgo_molotov` → `cs2_api.inc` → `bot_savage_ai`)

Daripada memindai seluruh entitas `info_target` dari `bot_savage_ai`, `csgo_molotov.sma` memelihara array ringkas berisi entitas `"molotov"` (`MAX_ACTIVE_MOLOTOVS = 16`) dan mengekspor 2 native baru ke library `"csgo_molotov"`:

```pawn
// Deklarasi di cs2_api.inc (dengan wrapper guarded cs2_api_*)
native csgo_get_active_molotov_count();
native bool:csgo_get_active_molotov(index, Float:origin[3], &Float:radius, &team, &Float:remainingTime);
```

Keuntungan desain ini:
* **Zero Entity Scan Overhead**: `bot_savage_ai` membaca langsung dari array memori `csgo_molotov` dalam $O(N)$ di mana $N \le 16$.
* **Koordinat Pusat Akurat**: `origin` diambil dari entitas induk `"molotov"` (bukan `"molotov_fire"` yang digeser acak oleh `ground_z()`).
* **Resiliensi Plugin**: Jika `csgo_molotov.amxx` dimatikan pada mode tertentu, `cs2_api_has_molotov()` mengembalikan `false` dan scanner hazard otomatis nonaktif tanpa error.

### 2. Arsitektur Deteksi Bahaya & Penghindaran (`savage_hazard_ai.inl`)

```text
[csgo_molotov.amxx]  -- native array (origin, radius, team, remTime) -->  [Task_HazardMonitor @ 0.25s]
                                                                                  |
                    +-------------------------------------------------------------+
                    |                                                             |
                    v                                                             v
     [Cek Posisi & Path Nodes Bot]                                [Cek Blokade Jalur Tim (Enemy Fire)]
     (dist <= radius + 48.0?)                                     (remTime >= 2.5s && belum di-smoke?)
                    |                                                             |
                    v                                                             v
     Cari Safe Graph Node Terdekat                                Pilih 1 Bot (Support > Entry) + Trace
     -> Savage_Bridge_RequestEvade()                              -> Savage_Bridge_RequestUtilityThrow(SMOKE)
```

#### Algoritma Pemilihan Node Aman (Anti-Oscillation):
1. Jika bot sudah memiliki `g_iBotLeaseType[id] == LEASE_HAZARD_EVADE`, jangan timpa target evakuasi sebelum masa *lease* habis atau bot sudah berada di jarak `> molotovRadius + 72.0` dari semua api aktif (mencegah bot bergetar mengubah arah setiap `0.25s`).
2. Untuk mencari titik evakuasi terbaik:
   * Periksa terlebih dahulu node pada rute mundur bot (`yb_get_bot_node(id)`).
   * Jika node saat ini berada di dalam zona api, iterasi kandidat node di radius `220.0..420.0` unit sekitar bot menggunakan `yb_find_nearest_node_in_radius` / vektor menjauh dari pusat api (`vecSafeDir = normalize(vecBot - vecFire)`), lalu validasi bahwa titik tujuan bebas dari semua api aktif dan dapat dijangkau (`yb_is_node_reachable`).

#### Filter Keputusan Smoke-Extinguish:
Agar bot tidak membuang Smoke secara sia-sia, seluruh syarat berikut harus terpenuhi:
1. `teamFire != get_user_team(id)` (hanya padamkan api musuh, kecuali api tersebut mengurung pembawa C4).
2. `flRemainingTime >= 2.5` detik (jika api tinggal 1 detik padam sendiri, lebih hemat menunggu daripada membuang Smoke `$300`).
3. Belum ada granat Smoke aktif di sekitar `vecFireOrigin` dalam radius `200.0` unit dan belum di-*claim* bot lain pada siklus yang sama.
4. Ada minimal 1 bot hidup di jarak `180.0..950.0` unit yang sedang bergerak ke arah zona api (`dot(vecBotVel, vecToFire) > 0.2` atau `yb_get_bot_path_node` melewati zona api) dan memiliki `CSW_SMOKEGRENADE`.

---

## Alternatives Considered

### Alternative A: Memodifikasi `yb_avoid_grenades` di C++ YaPB untuk Mengenali Classname `"molotov"`

* **Pros**: Penanganan internal langsung di engine *pathfinder* YaPB.
* **Cons**: Memerlukan perubahan dan kompilasi ulang `yapb.dll`, serta tidak menyelesaikan kebutuhan keputusan taktis tingkat tinggi seperti kapan harus memadamkan api dengan Smoke berdasarkan *role* (`Support` vs `AWPer`).
* **Reason for Rejection**: Sesuai kesepakatan arsitektur *Pawn-First* (`BP-01`), eksposur native registry dari `csgo_molotov.sma` + `savage_yapb_bridge.inl` memberikan kontrol penuh dan jauh lebih mudah di-tuning.

### Alternative B: Memindai Classname `"molotov_fire"` Langsung dari `bot_savage_ai`

* **Pros**: Tidak perlu menyentuh `csgo_molotov.sma`.
* **Cons**: `pev_origin` entitas `"molotov_fire"` terus digeser secara acak oleh `ground_z()` di `csgo_molotov.sma:623`, dan tidak membawa informasi sisa durasi bakar (`THINK_COUNT`) maupun `molotovRadius` dinamis dari cvar `csgo_molotov_radius`.
* **Reason for Rejection**: Mengekspos data langsung dari `csgo_molotov.sma` jauh lebih akurat, hemat CPU, dan tetap menjaga batas kepemilikan data.

---

## Non-Functional Requirements

* **Beban CPU Terkendali**: `Task_HazardMonitor` berjalan pada interval `0.25s` (4 Hz), bukan per-frame (`FM_CmdStart`), dan langsung *early-return* apabila `cs2_api_get_active_molotov_count() == 0`.
* **Graceful Degradation**: Jika `yb_has_graph()` bernilai `false` pada map custom tanpa waypoint, fallback menghitung titik evakuasi menggunakan `engfunc(EngFunc_TraceHull)` ke arah berlawanan dari pusat api.

---

## Detailed Implementation Plan

### Target Files to Create/Modify

1. **`cs16-game/cstrike/addons/amxmodx/scripting/csgo_molotov.sma`** (**Modify**)
   * **Type / Scope**: Molotov Core Plugin & Native Provider.
   * **Proposed Changes**:
     * Tambahkan registry array internal `g_iActiveMolotovEnts[16]` yang mencatat entitas `"molotov"` saat `molotov_explode()` dipanggil dan menghapusnya di `fire_stop()` / `event_new_round()`.
     * Daftarkan native `csgo_get_active_molotov_count` dan `csgo_get_active_molotov` di `plugin_natives()`.
     * Hapus pengurangan *damage* buatan `if (attacker == i && is_user_bot(i)) tempDamange *= 0.35;` pada `radius_damage2()` (baris `443-444`).

2. **`cs16-game/cstrike/addons/amxmodx/scripting/include/cs2_sdk/cs2_api.inc`** (**Modify**)
   * **Type / Scope**: Shared Cross-Plugin SDK.
   * **Proposed Changes**: Deklarasikan `csgo_get_active_molotov_count()` dan `csgo_get_active_molotov(...)` beserta wrapper aman `cs2_api_get_active_molotov_count()` dan `cs2_api_get_active_molotov(...)` yang terlindung `cs2_api_has_molotov()`.

3. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage/core/savage_taskids.inc`** (**Modify**)
   * **Type / Scope**: Task ID Partition.
   * **Proposed Changes**: Tambahkan `#define TASK_SV_HAZARD_SCAN 30012`.

4. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage/modules/savage_hazard_ai.inl`** (**Create**)
   * **Type / Scope**: Fire Hazard Evasion & Smoke Extinguish Module.
   * **Proposed Changes**: Implementasikan `Savage_Hazard_ResetRound()` (pembersihan klaim api per ronde), konstanta kalibrasi (`HAZARD_MAX_FIRES = 16`, `HAZARD_EVADE_MARGIN = 48.0`, `HAZARD_SAFE_CLEARANCE = 72.0`, `HAZARD_SMOKE_CHECK_RADIUS = 200.0`, `HAZARD_MIN_REM_FOR_SMOKE = 2.5`, `HAZARD_MIN_SMOKE_THROW_DIST = 180.0`, `HAZARD_MAX_SMOKE_THROW_DIST = 950.0`), `Task_HazardMonitor()`, pengecekan interseksi segmen rute bot terhadap lingkaran api, pencarian node aman via `yapb.inc`, serta pemilihan bot untuk melempar Smoke pemadam api via `Savage_Bridge_RequestUtilityThrow`.

5. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage_ai.sma`** (**Modify**)
   * **Type / Scope**: Plugin Entrypoint.
   * **Proposed Changes**: Tambahkan `#include "bot_savage/modules/savage_hazard_ai.inl"` dan daftarkan `set_task(0.25, "Task_HazardMonitor", TASK_SV_HAZARD_SCAN, _, _, "b")` di `plugin_init()`.

### Verification Plan

* **Compilation / Build**: Jalankan `build_all.bat` dan pastikan `csgo_molotov.amxx` serta `bot_savage_ai.amxx` terkompilasi tanpa error (`7/7 exit 0`).
* **Fire Evasion Test**: Lempar Molotov tepat di depan jalur gerak bot pada `de_dust2` (misalnya Long A atau B Tunnels); verifikasi bot berhenti sebelum masuk ke pusat api, berputar menjauh keluar dari `molotovRadius`, dan tidak mati terbakar.
* **Smoke Extinguish Test**: Beri tim bot `weapon_smokegrenade`, lempar Molotov musuh di pintu masuk B saat tim T melakukan push; verifikasi 1 bot Support/Entry melempar Smoke ke titik api dan api langsung padam disertai suara `molotov_extinguish.wav`.

--------------------------------------------------------------------------------

*Drafted with EGM*
