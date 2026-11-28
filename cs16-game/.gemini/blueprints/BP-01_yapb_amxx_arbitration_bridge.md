# BP-01: Jembatan & Arbitrasi Kontrol YaPB C++ ↔ AMXX

* **Author:** Antigravity
* **Status:** Implemented (Pending Runtime Verification)
* **Date:** 2026-11-27
* **Series:** Bot Tactical Utility & Molotov Integration (`BP-01` dari `3`)

--------------------------------------------------------------------------------

## Overview

Blueprint ini menetapkan kontrak pembagian wewenang (*ownership boundary*) dan mekanisme arbitrasi kontrol antara mesin bot **YaPB C++** (`addons/yapb/bin/yapb.dll`) dan plugin **AMX Mod X** (`bot_savage_ai.amxx`). Tujuannya adalah menghilangkan konflik kendali (*input fighting*) pada sudut pandang (`v_angle`), tujuan navigasi, dan tombol aksi bot—serta memperbaiki bug di mana seluruh eksekusi *tactical utility* AMXX saat ini mati total ketika `bot_savage_micro_override` bernilai `0` (default).

Dokumen ini menjadi fondasi wajib sebelum membangun kesadaran bahaya api Molotov (`BP-02`) dan eksekusi taktis Molotov berbasis *role* (`BP-03`).

---

## The Problem

Hasil inspeksi terhadap `cs16-game` dan `cs16-client/3rdparty/yapb` menemukan tiga akar masalah struktural yang membuat perilaku bot sulit dikendalikan:

1. **Header Native YaPB Belum Terpasang di Project Game**:
   * Source YaPB di `cs16-client/3rdparty/yapb/cfg/addons/amxmodx/scripting/include/yapb.inc` sudah menyediakan 40+ native API (`yb_get_bot_task`, `yb_get_bot_enemy`, `yb_set_bot_look_at`, `yb_set_bot_goal_node`, `yb_find_nearest_node_in_radius`, dll.), tetapi file tersebut belum ada di `cs16-game/cstrike/addons/amxmodx/scripting/include/` dan belum di-`#include` oleh `bot_savage_ai.sma`.
   * Akibatnya, `bot_savage_ai` tidak mengetahui *state* internal YaPB (apakah bot sedang berduel `YB_TASK_ATTACK`, menanam/menjinakkan bom, atau sedang melempar granat bawaan).
2. **Dead-Code Gate pada Eksekusi Utility AMXX (`savage_combat_tick.inl`)**:
   * Pada `savage_combat_tick.inl:35` dan `savage_combat_tick.inl:166`, `OnCmdStart` langsung keluar (`return FMRES_IGNORED`) apabila `bot_savage_micro_override == 0` (nilai default di `bot_savage_ai.sma:141`).
   * Blok eksekusi lemparan utility `if (g_bLineupActive[id])` berada di baris `178`—di bawah gerbang `return` tersebut. Ketika `Task_TacticalMidLineupScanner` (`savage_scavenge_utils.inl:328`) atau perintah chat `flash me` / `smoke` (`savage_esports_chat.inl:210, 283, 447`) mengganti senjata bot ke `weapon_smokegrenade` atau `weapon_flashbang` dan mengaktifkan `g_bLineupActive[id]`, bot hanya memegang granat tanpa pernah melemparnya ataupun kembali ke senjata utama.
3. **Tabrakan Arah Hadap (`UC_ViewAngles` vs YaPB `BotThink`)**:
   * Mengubah `UC_ViewAngles` hanya di `FM_CmdStart` tanpa memberi tahu YaPB C++ membuat `BotThink` di `yapb.dll` menimpa kembali `pev->v_angle` dan `pev->angles` pada tick yang sama, menghasilkan gerakan kepala bot yang bergetar (*jitter*) atau lemparan meleset.

---

## Goals

* Mengaktifkan komunikasi dua arah yang resmi antara `bot_savage_ai.amxx` dan `yapb.dll` melalui `yapb.inc` dengan pengecekan ketersediaan modul yang aman (*graceful fallback*).
* Memisahkan *Tactical Action Lease* (ambil alih singkat terukur untuk lempar utility, menghindar bahaya, atau *cover defuse*) dari `bot_savage_micro_override`, sehingga `bot_savage_micro_override "0"` tetap membiarkan YaPB C++ menangani *gunfight* dan *pathfinding* 100%, namun aksi taktis AMXX tetap berjalan mulus.
* Menerapkan aturan **Combat Preemption**: YaPB C++ selalu berhak membatalkan *lease* utility AMXX seketika jika bot melihat musuh aktif (`yb_get_bot_enemy(id)`), agar bot tidak mati konyol sambil memegang granat.
* Menjamin *clean handback* setiap kali *lease* selesai atau dibatalkan (`yb_set_bot_look_at(id, {0.0, 0.0, 0.0})` + restore senjata utama), tanpa meninggalkan bot dalam kondisi macet.

---

## Non-Goals

* **Tidak mengubah atau meng-compile ulang binary C++ `yapb.dll`** pada tahap ini (strategi *Pawn-First* menggunakan native `yapb.inc` yang sudah tersedia).
* **Tidak mengaktifkan `bot_savage_micro_override 1`** sebagai default; kontrol tembak-menembak dan navigasi dasar tetap milik YaPB C++.
* **Belum mengimplementasikan deteksi api Molotov maupun logika lempar Molotov** (cakupan `BP-02` dan `BP-03`).

---

## Open Questions

* Seluruh keputusan arsitektural dasar telah disepakati bersama pengguna (strategi *Pawn-First* melalui `yapb.inc`, penyimpanan blueprint di `cs16-game/.gemini/blueprints/`). Tidak ada pertanyaan teknis terbuka sebelum review `BP-01`.

---

## Critical User Journeys (CUJs)

### CUJ 1: Eksekusi Lempar Utility Taktis Tanpa Rebutan Arah Hadap

* **Given**: `bot_savage_micro_override` bernilai `0`, `yapb.dll` aktif, dan bot Support memiliki `weapon_smokegrenade` serta tidak sedang berduel dengan musuh.
* **When**: Modul taktis AMXX (baik dari `BP-02`, `BP-03`, maupun perintah chat `smoke` / `flash me`) meminta *utility throw lease* ke titik target tertentu.
* **Then**:
  1. `savage_yapb_bridge` mengunci `LEASE_UTILITY_THROW` selama durasi terukur (maksimal `0.65s`), mengganti senjata ke granat target, dan memanggil `yb_set_bot_look_at(id, vecAimTarget)` agar YaPB C++ mengarahkan pandangan secara native tanpa melawan AMXX.
  2. Setelah `0.25s` waktu *aim settle*, `OnCmdStart` menahan `IN_ATTACK` lalu melepaskannya untuk melempar granat.
  3. Begitu granat terlepas atau waktu *lease* habis, bridge memanggil `yb_set_bot_look_at(id, {0.0, 0.0, 0.0})` dan mengembalikan bot ke senjata utama (`slot1` / `slot2`).

### CUJ 2: Combat Preemption Saat Disergap Musuh

* **Given**: Bot sedang berada dalam fase persiapan lempar utility (`LEASE_UTILITY_THROW`) sebelum pin granat dilepas.
* **When**: YaPB C++ mendeteksi musuh dalam jarak pandang (`yb_get_bot_enemy(id) != YB_INVALID_ENTITY_INDEX` atau `yb_get_bot_task(id) == YB_TASK_ATTACK`).
* **Then**: Bridge langsung membatalkan *lease* pada tick yang sama, mereset `yb_set_bot_look_at(id, {0.0, 0.0, 0.0})`, dan mengembalikan kendali penuh kepada YaPB C++ untuk menarik senjata dan membalas tembakan.

### CUJ 3: Pengalihan Navigasi Menghindar Bahaya (`LEASE_HAZARD_EVADE`)

* **Given**: Bot sedang berjalan mengikuti rute YaPB menuju sebuah *bombsite*.
* **When**: Modul bahaya (`BP-02`) memanggil `Savage_Bridge_RequestEvade(id, vecSafeOrigin, flDuration)`.
* **Then**: Bridge mengaktifkan `LEASE_HAZARD_EVADE`, memanggil `yb_set_bot_goal_origin(id, vecSafeOrigin)` (atau node aman terdekat via `yb_find_nearest_node_in_radius`), dan mengembalikan rute normal ke YaPB setelah durasi menghindar selesai.

---

## Technical Plan (Architecture)

### 1. Ownership Matrix (Pembagian Wewenang Layer)

| Domain Perilaku | Pemilik Utama | Mekanisme Koordinasi |
| :--- | :--- | :--- |
| Waypoint pathfinding, lompat/jongkok rintangan, navigasi umum | **YaPB C++** | Default aktif; AMXX hanya memberi *goal override* sementara saat menghindar bahaya (`LEASE_HAZARD_EVADE`). |
| Deteksi musuh, *recoil control*, *aim tracking*, dan tembakan duel | **YaPB C++** | Mutlak milik YaPB saat `bot_savage_micro_override == 0`; otomatis membatalkan *lease* non-darurat AMXX. |
| Ekonomi tim, pembagian *role*, *drop* senjata, dan IGL *strat call* | **AMXX (`bot_savage_ai`)** | Berjalan di *freeze time* / *event-driven* tanpa menyentuh `FM_CmdStart`. |
| Eksekusi *lineup utility* (Smoke, Flash, Molotov) & *hazard evasion* | **AMXX via Bridge** | Menggunakan **Action Lease** berbatas waktu (`0.45s – 1.50s`) dengan API `yb_set_bot_look_at` / `yb_set_bot_goal_origin`. |

### 2. Lease State Machine (`SavageLeaseType`)

Setiap bot (`1..32`) memiliki state arbitrasi tunggal agar dua subsistem AMXX (misalnya `MidLineupScanner` dan `ChatUtility`) tidak saling menimpa:

```text
+---------------+   Request Lease (Priority >= Current)   +---------------------+
|  LEASE_NONE   | --------------------------------------> |  ACTIVE LEASE       |
| (YaPB 100%)   |                                         | - UTILITY_THROW (2) |
+---------------+ <-------------------------------------- | - HAZARD_EVADE  (3) |
        ^            Expire / Complete / Clean Handback   +---------------------+
        |                                                            |
        +------------- Combat Preemption (Enemy Spotted) ------------+
                       (Hanya berlaku jika Priority < HAZARD_EVADE)
```

Prioritas *lease* (angka lebih besar menang):
* `0` — `LEASE_NONE`: Kontrol penuh pada YaPB C++.
* `1` — `LEASE_TACTICAL_HOLD`: *Hold angle* / *silent pacing* perintah IGL (dapat dibatalkan oleh kontak musuh).
* `2` — `LEASE_UTILITY_THROW`: Lempar granat taktis/utility (dapat dibatalkan oleh kontak musuh sebelum lemparan dilepas).
* `3` — `LEASE_HAZARD_EVADE`: Menghindar dari api Molotov / C4 (prioritas tertinggi; tetap berjalan meski ada musuh agar bot tidak mati terbakar).

### 3. Struktur Data & Native Bridge (`savage_yapb_bridge.inl`)

```pawn
enum SavageLeaseType {
    LEASE_NONE = 0,
    LEASE_TACTICAL_HOLD = 1,
    LEASE_UTILITY_THROW = 2,
    LEASE_HAZARD_EVADE = 3
};

new bool:g_bYapbAvailable = false;
new SavageLeaseType:g_iBotLeaseType[33];
new Float:g_flBotLeaseUntil[33];
new Float:g_flBotLeaseStart[33];
new Float:g_vecBotLeaseTarget[33][3];
new g_iBotLeaseWeaponCSW[33];
new bool:g_bBotLeaseAttackSent[33];
```

Alur eksekusi per tick di `OnCmdStart` (`savage_combat_tick.inl`):
1. Jika `!is_user_bot(id) || !is_user_alive(id)`, langsung `return FMRES_IGNORED`.
2. **Jalur Cepat (Fast-Path)**: Jika `g_iBotLeaseType[id] == LEASE_NONE` **dan** `bot_savage_micro_override == 0` **dan** `!IsKnifeRoundActive()`, langsung `return FMRES_IGNORED` (biaya CPU nyaris nol pada kondisi normal).
3. Jika `g_iBotLeaseType[id] != LEASE_NONE`, jalankan `Savage_Bridge_ProcessTick(id, uc_handle, flGameTime)` **sebelum** gerbang `bot_savage_micro_override`:
   * Cek kedaluwarsa (`flGameTime >= g_flBotLeaseUntil[id]`) -> panggil `Savage_Bridge_ReleaseLease(id)`.
   * Cek *Combat Preemption* (untuk `LEASE_UTILITY_THROW` dan `LEASE_TACTICAL_HOLD`): jika `g_bYapbAvailable && yb_get_bot_enemy(id) != YB_INVALID_ENTITY_INDEX` dan belum masuk fase pelepasan pin, panggil `Savage_Bridge_ReleaseLease(id)`.
   * Untuk `LEASE_UTILITY_THROW`: pastikan senjata aktif == `g_iBotLeaseWeaponCSW[id]`, kunci pandangan ke `g_vecBotLeaseTarget[id]` via `yb_set_bot_look_at`, tahan `IN_ATTACK` selama `0.15s..0.35s`, lalu lepas `IN_ATTACK` agar mesin GoldSrc melempar granat secara alami.

---

## Alternatives Considered

### Alternative A: Mengaktifkan `bot_savage_micro_override 1` Secara Global

* **Pros**: Tidak perlu membuat struktur *lease* terpisah di `OnCmdStart`.
* **Cons**: Mengaktifkan seluruh logika *aiming*, *counter-strafe*, *wallbang*, dan *bhop* AMXX di atas YaPB C++, yang selama ini menyebabkan *input fighting*, jitter sudut pandang, dan beban CPU tinggi di setiap frame.
* **Reason for Rejection**: Bertentangan dengan prinsip *Strict Separation of Concerns* yang sudah ditetapkan di `savage_combat_tick.inl:161` dan membuat perilaku YaPB sulit diprediksi.

### Alternative B: Memodifikasi Langsung Source C++ YaPB (`tasks.cpp` / `combat.cpp`)

* **Pros**: Akses langsung ke struktur internal `Bot` dan *pathfinder* C++.
* **Cons**: Membutuhkan kompilasi ulang DLL C++, menyulitkan iterasi cepat dibanding `build_all.bat` AMXX, dan berisiko merusak stabilitas `yapb.dll` jika terjadi bug memori.
* **Reason for Rejection**: Native API di `yapb.inc` sudah mengekspos `yb_set_bot_look_at`, `yb_set_bot_goal_origin`, `yb_get_bot_enemy`, dan `yb_get_bot_task`, sehingga cukup diselesaikan bersih di layer Pawn.

---

## Non-Functional Requirements

* **Performa Per-Frame (`FM_CmdStart`)**: Ketika bot tidak memiliki *active lease* (`g_iBotLeaseType[id] == LEASE_NONE`), `OnCmdStart` wajib keluar pada 3 pengecekan boolean/integer pertama tanpa memanggil native YaPB maupun `engfunc`.
* **Resiliensi Modul (`yapb`)**: Menggunakan pemeriksaan `LibraryExists("yapb", LibType_Library)` / `yb_get_api_version() > 0` saat `plugin_cfg()` sehingga server tidak mengalami *runtime error* jika `yapb.dll` sedang tidak dimuat.
* **Kompatibilitas Build**: Tetap mematuhi `build_all.bat` (wajib 7/7 plugin terkompilasi dengan exit code `0` dan nol `.amxx` tertinggal di `scripting/`).

---

## Detailed Implementation Plan

### Target Files to Create/Modify

1. **`cs16-game/cstrike/addons/amxmodx/scripting/include/yapb.inc`** (**Create / Copy**)
   * **Type / Scope**: AMXX Include Header.
   * **Proposed Changes**: Salin dari `cs16-client/3rdparty/yapb/cfg/addons/amxmodx/scripting/include/yapb.inc` agar seluruh deklarasi native dan enum (`YBTask`, `YBNodeFlag`, `YBNodeType`) tersedia bagi compiler `amxxpc.exe`.

2. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage/core/savage_defs.inc`** (**Modify**)
   * **Type / Scope**: Core Definitions & State Arrays.
   * **Proposed Changes**: Tambahkan enum `SavageLeaseType` serta array per-player `g_bYapbAvailable`, `g_iBotLeaseType[33]`, `g_flBotLeaseUntil[33]`, `g_flBotLeaseStart[33]`, `g_vecBotLeaseTarget[33][3]`, `g_iBotLeaseWeaponCSW[33]`, dan `g_bBotLeaseAttackSent[33]`.

3. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage/modules/savage_yapb_bridge.inl`** (**Create**)
   * **Type / Scope**: Arbitration & YaPB Native Wrapper Module.
   * **Proposed Changes**: Implementasikan `Savage_Bridge_Init()`, `Savage_Bridge_ResetBot(id)`, `Savage_Bridge_ComputeLobAimPoint(vecEyes, vecTarget, flPitchDeg, vecOutAimPoint)`, `Savage_Bridge_HasActiveEnemy(id)`, `Savage_Bridge_RequestUtilityThrow(id, iWeaponCSW, const Float:vecAimPoint[3], Float:flDuration)`, `Savage_Bridge_RequestEvade(id, const Float:vecSafeOrigin[3], Float:flDuration)`, `Savage_Bridge_ReleaseLease(id)`, dan `Savage_Bridge_ProcessTick(id, uc_handle, Float:flGameTime)`.

4. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage/modules/savage_combat_tick.inl`** (**Modify**)
   * **Type / Scope**: Frame Hook Dispatcher (`FM_CmdStart`).
   * **Proposed Changes**: Pindahkan pemrosesan `Savage_Bridge_ProcessTick` ke atas gerbang `if (!get_pcvar_num(g_pCvarMicroOverride))`, dan ganti blok lama `if (g_bLineupActive[id])` (baris `178–193`) agar terpusat melalui bridge.

5. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage/modules/savage_esports_chat.inl` & `savage_scavenge_utils.inl`** (**Modify**)
   * **Type / Scope**: Tactical Utility Callers.
   * **Proposed Changes**: Ganti pengisian manual `g_bLineupActive` / `g_vecLineupAngles` di `Execute_FlashCall`, `Execute_SmokeCall`, `Execute_CoverDefuse`, dan `Task_TacticalMidLineupScanner` dengan pemanggilan `Savage_Bridge_RequestUtilityThrow(...)` menggunakan titik target 3D dunia (bukan sekadar sudut Euler mentah) agar `yb_set_bot_look_at` mengarahkan pandangan bot secara akurat.

6. **`cs16-game/cstrike/addons/amxmodx/scripting/bot_savage_ai.sma`** (**Modify**)
   * **Type / Scope**: Plugin Entrypoint.
   * **Proposed Changes**: Tambahkan `#include <yapb>`, `#include "bot_savage/modules/savage_yapb_bridge.inl"`, inisialisasi `Savage_Bridge_Init()` di `plugin_cfg()`, dan reset state bridge pada `ResetBotCombatStates(id)` serta awal ronde.

### Verification Plan

* **Compilation / Build**: Jalankan `cs16-game\cstrike\addons\amxmodx\scripting\build_all.bat` dan pastikan `bot_savage_ai.amxx` serta ke-6 plugin lainnya terkompilasi tanpa error maupun warning simbol tak terdefinisi (`7/7 exit 0`).
* **Runtime Smoke Test**: Muat `de_dust2` dengan 9 YaPB bot (`bot_savage_micro_override 0`), ketik `smoke` atau `flash me` di team chat, dan verifikasi bahwa bot terdekat beralih ke granat, mengarahkan pandangan ke target tanpa jitter, melempar granat dalam `<= 0.65s`, lalu kembali memegang senjata utama.
* **Combat Preemption Test**: Picu lemparan utility bot saat musuh muncul di garis pandang bot; verifikasi bot langsung membatalkan granat dan menembak musuh.

--------------------------------------------------------------------------------

*Drafted with EGM*
