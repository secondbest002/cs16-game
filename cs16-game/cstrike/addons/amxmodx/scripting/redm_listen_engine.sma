#include <amxmodx>
#include <amxmisc>
#include <hamsandwich>
#include <fakemeta>
#include <cstrike>
#include <fun>
#include <engine>
#include <json>
#include <cs2_sdk/cs2_offsets>

#define PLUGIN "ReDM Listen Engine"
#define VERSION "1.0.2"
#define AUTHOR "Antigravity & ReDeathmatch Community"

#define MAX_PLAYERS 32
#define TASK_RESPAWN    55100
#define TASK_PROTECT    55200
#define TASK_EQUIP_MENU 55300
#define TASK_CLEANUP    55001
#define TASK_DM_NEWROUND 55002

// Spawn structure for JSON spawns
enum _:SpawnPoint_s {
    Float:sp_OriginX,
    Float:sp_OriginY,
    Float:sp_OriginZ,
    Float:sp_AnglesX,
    Float:sp_AnglesY,
    Float:sp_AnglesZ
}

static Array:g_aSpawns = Invalid_Array
static bool:g_bHasCustomSpawns = false

// Player state
static bool:g_bProtected[MAX_PLAYERS + 1]
static bool:g_bAutoEquip[MAX_PLAYERS + 1]
static bool:g_bInMenu[MAX_PLAYERS + 1]
static g_iLastPrimary[MAX_PLAYERS + 1]
static g_iLastSecondary[MAX_PLAYERS + 1]

// CVars
static pCvar_Active
static pCvar_RespawnDelay
static pCvar_ProtectTime
static pCvar_RefillOnKill
static pCvar_FreeForAll
static pCvar_CleanWeapons

// Weapon lists
static const g_szPrimaryNames[][] = {
    "AK-47 Kalashnikov",
    "M4A1 Carbine",
    "AWP Sniper Rifle",
    "Galil AR",
    "FAMAS",
    "MP5 Navy",
    "SSG08 Scout",
    "SG552 Commando",
    "AUG",
    "M249 Para"
}

static const g_szPrimaryEntities[][] = {
    "weapon_ak47",
    "weapon_m4a1",
    "weapon_awp",
    "weapon_galil",
    "weapon_famas",
    "weapon_mp5navy",
    "weapon_scout",
    "weapon_sg552",
    "weapon_aug",
    "weapon_m249"
}

static const g_iPrimaryAmmoTypes[] = {
    CSW_AK47,
    CSW_M4A1,
    CSW_AWP,
    CSW_GALIL,
    CSW_FAMAS,
    CSW_MP5NAVY,
    CSW_SCOUT,
    CSW_SG552,
    CSW_AUG,
    CSW_M249
}

static const g_szSecondaryNames[][] = {
    "Desert Eagle .50 AE",
    "Dual Berettas",
    "FN Five-Seven"
}

static const g_szSecondaryEntities[][] = {
    "weapon_deagle",
    "weapon_elite",
    "weapon_fiveseven"
}

static const g_iSecondaryAmmoTypes[] = {
    CSW_DEAGLE,
    CSW_ELITE,
    CSW_FIVESEVEN
}

public plugin_init() {
    register_plugin(PLUGIN, VERSION, AUTHOR)

    pCvar_Active       = register_cvar("redm_active", "1")
    pCvar_RespawnDelay = register_cvar("redm_respawn_delay", "1.2")
    pCvar_ProtectTime  = register_cvar("redm_protect_time", "2.0")
    pCvar_RefillOnKill = register_cvar("redm_refill_on_kill", "1")
    pCvar_FreeForAll   = register_cvar("redm_freeforall", "1")
    pCvar_CleanWeapons = register_cvar("redm_clean_weapons", "1")

    RegisterHam(Ham_Killed, "player", "OnPlayerKilled_Post", 1)
    RegisterHam(Ham_Spawn, "player", "OnPlayerSpawn_Post", 1)
    RegisterHam(Ham_TakeDamage, "player", "OnPlayerTakeDamage", 0)

    // Option 2 Safety Net: Hook default pistol deploy on bots to eliminate Glock-18 & USP
    RegisterHam(Ham_Item_Deploy, "weapon_glock18", "OnDefaultPistolDeploy_Post", 1)
    RegisterHam(Ham_Item_Deploy, "weapon_usp", "OnDefaultPistolDeploy_Post", 1)

    // Hook primary attack on all standard weapons to cancel spawn protection
    static const szWeapons[][] = {
        "weapon_p228", "weapon_scout", "weapon_hegrenade", "weapon_xm1014",
        "weapon_mac10", "weapon_aug", "weapon_smokegrenade", "weapon_elite",
        "weapon_fiveseven", "weapon_ump45", "weapon_sg550", "weapon_galil",
        "weapon_famas", "weapon_usp", "weapon_glock18", "weapon_awp",
        "weapon_mp5navy", "weapon_m249", "weapon_m3", "weapon_m4a1",
        "weapon_tmp", "weapon_g3sg1", "weapon_flashbang", "weapon_deagle",
        "weapon_sg552", "weapon_ak47", "weapon_knife", "weapon_p90"
    }
    for (new i = 0; i < sizeof(szWeapons); i++) {
        RegisterHam(Ham_Weapon_PrimaryAttack, szWeapons[i], "OnWeaponPrimaryAttack_Post", 1)
    }

    register_event("HLTV", "Event_NewRound", "a", "1=0", "2=0")

    register_clcmd("say /guns", "Cmd_Guns")
    register_clcmd("say guns", "Cmd_Guns")
    register_clcmd("say /menu", "Cmd_Guns")
    register_clcmd("guns", "Cmd_Guns")

    register_clcmd("say /ffa", "Cmd_ToggleFFA")
    register_clcmd("say ffa", "Cmd_ToggleFFA")
    register_concmd("redm_ffa", "Cmd_ConsoleFFA")

    // Block Grenade purchases completely in Deathmatch
    register_clcmd("buy_hegrenade", "Cmd_BlockGrenade")
    register_clcmd("buy_flashbang", "Cmd_BlockGrenade")
    register_clcmd("buy_smokegrenade", "Cmd_BlockGrenade")
    register_clcmd("hegren", "Cmd_BlockGrenade")
    register_clcmd("flash", "Cmd_BlockGrenade")
    register_clcmd("sgren", "Cmd_BlockGrenade")

    // Block round-end when teams are wiped out
    register_message(get_user_msgid("RoundTime"), "Msg_RoundTime")
    register_message(get_user_msgid("SendAudio"), "Msg_SendAudio")

    LoadSpawnsFromJSON()
}

public plugin_cfg() {
    // Re-verify spawns after map entities load
    if (!g_bHasCustomSpawns) {
        LoadSpawnsFromJSON()
    }

    if (get_pcvar_num(pCvar_Active)) {
        SetFFAMode(bool:get_pcvar_num(pCvar_FreeForAll))
        SweepAndEquipAliveBots()
    }
}

public Event_NewRound() {
    if (get_pcvar_num(pCvar_Active)) {
        remove_task(TASK_DM_NEWROUND)
        set_task(0.1, "Task_NewRoundSweep", TASK_DM_NEWROUND)
    }
}

public Task_NewRoundSweep() {
    if (get_pcvar_num(pCvar_Active)) {
        SweepAndEquipAliveBots()
    }
}

SweepAndEquipAliveBots() {
    for (new i = 1; i <= MAX_PLAYERS; i++) {
        if (is_user_connected(i) && is_user_alive(i) && is_user_bot(i)) {
            AssignBotLoadout(i)
            EquipPlayer(i, g_iLastPrimary[i], g_iLastSecondary[i])
        }
    }
}

public client_putinserver(id) {
    remove_task(id + TASK_RESPAWN)
    remove_task(id + TASK_PROTECT)
    remove_task(id + TASK_EQUIP_MENU)
    g_bInMenu[id] = false
    g_bProtected[id] = false
    if (is_user_bot(id)) {
        AssignBotLoadout(id)
    } else {
        g_bAutoEquip[id] = false
        g_iLastPrimary[id] = -1
        g_iLastSecondary[id] = -1
    }
}

public client_disconnected(id) {
    g_bInMenu[id] = false
    g_bProtected[id] = false
    g_bAutoEquip[id] = false
    g_iLastPrimary[id] = -1
    g_iLastSecondary[id] = -1
    remove_task(id + TASK_RESPAWN)
    remove_task(id + TASK_PROTECT)
    remove_task(id + TASK_EQUIP_MENU)
}

public plugin_end() {
    if (g_aSpawns != Invalid_Array) {
        ArrayDestroy(g_aSpawns)
        g_aSpawns = Invalid_Array
    }
}

// -----------------------------------------------------------------------------
// JSON Spawns Loader (Reads ReDeathmatch data/redm/<mapname>.spawns.json)
// -----------------------------------------------------------------------------
LoadSpawnsFromJSON() {
    if (g_aSpawns != Invalid_Array) {
        ArrayDestroy(g_aSpawns)
        g_aSpawns = Invalid_Array
    }

    g_aSpawns = ArrayCreate(SpawnPoint_s)
    g_bHasCustomSpawns = false

    new szMap[32], szPath[128]
    get_mapname(szMap, charsmax(szMap))
    formatex(szPath, charsmax(szPath), "addons/amxmodx/data/redm/%s.spawns.json", szMap)

    if (!file_exists(szPath)) {
        server_print("[ReDM Listen] No custom JSON spawn file found for map %s. Using default map spots.", szMap)
        return
    }

    new JSON:hRoot = json_parse(szPath, true)
    if (hRoot == Invalid_JSON) {
        server_print("[ReDM Listen] Failed to parse JSON spawn file: %s", szPath)
        return
    }

    new JSON:hSpawns = json_object_get_value(hRoot, "spawns")
    if (hSpawns != Invalid_JSON && json_is_array(hSpawns)) {
        new iCount = json_array_get_count(hSpawns)
        new spawnPoint[SpawnPoint_s]

        for (new i = 0; i < iCount; i++) {
            new JSON:hItem = json_array_get_value(hSpawns, i)
            if (hItem == Invalid_JSON) continue

            new JSON:hOrigin = json_object_get_value(hItem, "origin")
            new JSON:hAngle = json_object_get_value(hItem, "vAngle")
            if (hAngle == Invalid_JSON) {
                hAngle = json_object_get_value(hItem, "angle")
            }

            if (hOrigin != Invalid_JSON && json_is_array(hOrigin) && json_array_get_count(hOrigin) >= 3) {
                spawnPoint[sp_OriginX] = json_array_get_real(hOrigin, 0)
                spawnPoint[sp_OriginY] = json_array_get_real(hOrigin, 1)
                spawnPoint[sp_OriginZ] = json_array_get_real(hOrigin, 2)

                if (hAngle != Invalid_JSON && json_is_array(hAngle) && json_array_get_count(hAngle) >= 2) {
                    spawnPoint[sp_AnglesX] = json_array_get_real(hAngle, 0)
                    spawnPoint[sp_AnglesY] = json_array_get_real(hAngle, 1)
                    spawnPoint[sp_AnglesZ] = 0.0
                }

                ArrayPushArray(g_aSpawns, spawnPoint)
            }

            json_free(hOrigin)
            if (hAngle != Invalid_JSON) json_free(hAngle)
            json_free(hItem)
        }
        json_free(hSpawns)

        if (ArraySize(g_aSpawns) > 0) {
            g_bHasCustomSpawns = true
            server_print("[ReDM Listen] Loaded %d custom JSON spawns for %s successfully!", ArraySize(g_aSpawns), szMap)
        }
    }

    json_free(hRoot)
}

// -----------------------------------------------------------------------------
// Respawn & Spawn Hooks
// -----------------------------------------------------------------------------
public OnPlayerKilled_Post(iVictim, iKiller, iShouldGib) {
    if (!get_pcvar_num(pCvar_Active)) return

    // Clear victim menu state and tasks
    remove_task(iVictim + TASK_EQUIP_MENU)
    g_bInMenu[iVictim] = false
    show_menu(iVictim, 0, "^n", 0)

    // Refill killer's active weapon clip
    if (get_pcvar_num(pCvar_RefillOnKill) && is_user_alive(iKiller) && iKiller != iVictim) {
        RefillActiveWeapon(iKiller)
    }

    // Clean dropped weapons on map periodically
    if (get_pcvar_num(pCvar_CleanWeapons)) {
        if (!task_exists(TASK_CLEANUP)) {
            set_task(0.5, "Task_CleanDroppedWeapons", TASK_CLEANUP)
        }
    }

    // Schedule instant respawn
    new Float:flDelay = get_pcvar_float(pCvar_RespawnDelay)
    if (flDelay < 0.5) flDelay = 0.5

    remove_task(iVictim + TASK_RESPAWN)
    set_task(flDelay, "Task_RespawnPlayer", iVictim + TASK_RESPAWN)
}

// CBasePlayer m_iTeam private data offsets
#define OFFSET_TEAM 114
#define EXTRAOFFSET_PLAYER 5

// Bulletproof combatant validation for Deathmatch (FFA & TDM)
stock bool:IsValidCombatant(id) {
    if (id < 1 || id > MAX_PLAYERS || !is_user_connected(id))
        return false

    // Bots created by YaPB in Deathmatch are ALWAYS valid combatants (never spectators)
    if (is_user_bot(id))
        return true

    // For human players, verify they are in T (1) or CT (2) via CBasePlayer::m_iTeam offset
    new iTeam = get_pdata_int(id, OFFSET_TEAM, EXTRAOFFSET_PLAYER)
    return (iTeam == 1 || iTeam == 2)
}

public Task_RespawnPlayer(iTaskID) {
    new id = iTaskID - TASK_RESPAWN
    if (!IsValidCombatant(id)) return

    if (!is_user_alive(id)) {
        xash_safe_respawn(id)
    }
}

public OnPlayerSpawn_Post(id) {
    if (!get_pcvar_num(pCvar_Active)) return
    if (!IsValidCombatant(id)) return

    if (get_user_health(id) > 0) {
        set_pev(id, pev_deadflag, DEAD_NO)
    }

    // Teleport to custom random JSON spawn point
    if (g_bHasCustomSpawns && g_aSpawns != Invalid_Array) {
        TeleportToRandomSpawn(id)
    }

    // Give default armor + helmet
    cs_set_user_armor(id, 100, CS_ARMOR_VESTHELM)

    // Activate spawn protection
    new Float:flProtectTime = get_pcvar_float(pCvar_ProtectTime)
    if (flProtectTime > 0.0) {
        ActivateProtection(id, flProtectTime)
    }

    // Strip default/spawn grenades completely
    cs_set_user_bpammo(id, CSW_HEGRENADE, 0)
    cs_set_user_bpammo(id, CSW_FLASHBANG, 0)
    cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 0)

    // Unified Auto-Equip / Weapon Menu pipeline
    SchedulePlayerEquipOrMenu(id)
}

SchedulePlayerEquipOrMenu(id) {
    // Ensure bot has assigned loadout (Option 1)
    if (is_user_bot(id)) {
        if (!g_bAutoEquip[id] || g_iLastPrimary[id] < 0) {
            AssignBotLoadout(id)
        }
    }

    // Unified Auto-Equip pipeline for both bots & players
    if (g_bAutoEquip[id] && g_iLastPrimary[id] >= 0) {
        remove_task(id + TASK_EQUIP_MENU)
        set_task(0.10, "Task_AutoEquipPlayer", id + TASK_EQUIP_MENU)
    } else if (!is_user_bot(id) && !g_bInMenu[id]) {
        remove_task(id + TASK_EQUIP_MENU)
        set_task(0.25, "Task_ShowPrimaryMenu", id + TASK_EQUIP_MENU)
    }
}

// Teleport player to a valid spawn spot with hull clearance check
TeleportToRandomSpawn(id) {
    new iTotal = ArraySize(g_aSpawns)
    if (iTotal <= 0) return

    new spawnPoint[SpawnPoint_s]
    new Float:vecOrigin[3], Float:vecAngles[3]
    new iStartIdx = random_num(0, iTotal - 1)
    new bool:bFound = false

    for (new i = 0; i < iTotal; i++) {
        new idx = (iStartIdx + i) % iTotal
        ArrayGetArray(g_aSpawns, idx, spawnPoint)

        vecOrigin[0] = spawnPoint[sp_OriginX]
        vecOrigin[1] = spawnPoint[sp_OriginY]
        vecOrigin[2] = spawnPoint[sp_OriginZ]

        if (IsHullVacant(vecOrigin, HULL_HUMAN)) {
            vecAngles[0] = spawnPoint[sp_AnglesX]
            vecAngles[1] = spawnPoint[sp_AnglesY]
            vecAngles[2] = 0.0
            bFound = true
            break
        }
    }

    if (bFound) {
        entity_set_origin(id, vecOrigin)
        entity_set_vector(id, EV_VEC_angles, vecAngles)
        entity_set_vector(id, EV_VEC_velocity, Float:{0.0, 0.0, 0.0})
        entity_set_int(id, EV_INT_fixangle, 1)
    }
}

bool:IsHullVacant(const Float:origin[3], hull) {
    new tr = 0
    engfunc(EngFunc_TraceHull, origin, origin, 0, hull, 0, tr)
    if (!get_tr2(tr, TR_StartSolid) && !get_tr2(tr, TR_AllSolid) && get_tr2(tr, TR_InOpen)) {
        return true
    }
    return false
}

// -----------------------------------------------------------------------------
// Spawn Protection
// -----------------------------------------------------------------------------
ActivateProtection(id, Float:flDuration) {
    g_bProtected[id] = true
    set_user_rendering(id, kRenderFxGlowShell, 0, 255, 128, kRenderNormal, 16)
    
    remove_task(id + TASK_PROTECT)
    set_task(flDuration, "Task_RemoveProtection", id + TASK_PROTECT)
}

public Task_RemoveProtection(iTaskID) {
    new id = iTaskID - TASK_PROTECT
    if (!is_user_connected(id)) return

    g_bProtected[id] = false
    set_user_rendering(id, kRenderFxNone, 0, 0, 0, kRenderNormal, 0)
}

public OnPlayerTakeDamage(iVictim, iInflictor, iAttacker, Float:flDamage, iDamageBits) {
    if (!get_pcvar_num(pCvar_Active)) return HAM_IGNORED

    if (iVictim >= 1 && iVictim <= MAX_PLAYERS && g_bProtected[iVictim]) {
        return HAM_SUPERCEDE // Immune to damage
    }
    return HAM_IGNORED
}

public OnWeaponPrimaryAttack_Post(iEnt) {
    if (!pev_valid(iEnt)) return

    new id = get_pdata_cbase(iEnt, 41, 4) // m_pPlayer
    if (id >= 1 && id <= MAX_PLAYERS && g_bProtected[id]) {
        // Player fired weapon -> remove protection instantly
        remove_task(id + TASK_PROTECT)
        Task_RemoveProtection(id + TASK_PROTECT)
    }
}

// -----------------------------------------------------------------------------
// Auto-Refill Weapon Clip
// -----------------------------------------------------------------------------
RefillActiveWeapon(id) {
    new iClip, iAmmo
    new iWeapon = get_user_weapon(id, iClip, iAmmo)
    if (iWeapon > 0) {
        new szWeapon[32]
        get_weaponname(iWeapon, szWeapon, charsmax(szWeapon))
        new iEnt = xash_find_weapon_ent(id, szWeapon)
        if (pev_valid(iEnt)) {
            // Set max clip depending on weapon
            new iMaxClip = 30
            switch (iWeapon) {
                case CSW_AWP: iMaxClip = 10
                case CSW_SCOUT: iMaxClip = 10
                case CSW_DEAGLE: iMaxClip = 7
                case CSW_USP: iMaxClip = 12
                case CSW_GLOCK18: iMaxClip = 20
                case CSW_P228: iMaxClip = 13
                case CSW_ELITE: iMaxClip = 30
                case CSW_FIVESEVEN: iMaxClip = 20
                case CSW_M3: iMaxClip = 8
                case CSW_XM1014: iMaxClip = 7
                case CSW_M249: iMaxClip = 100
                default: iMaxClip = 30
            }
            cs_set_weapon_ammo(iEnt, iMaxClip)
        }
    }
}

// -----------------------------------------------------------------------------
// Weapon Menus
// -----------------------------------------------------------------------------
public Cmd_Guns(id) {
    if (!get_pcvar_num(pCvar_Active)) return PLUGIN_CONTINUE;

    g_bAutoEquip[id] = false
    g_bInMenu[id] = false
    remove_task(id + TASK_EQUIP_MENU)
    ShowPrimaryMenu(id)
    return PLUGIN_HANDLED
}

public Cmd_ToggleFFA(id) {
    if (!get_pcvar_num(pCvar_Active)) return PLUGIN_CONTINUE;

    new bool:bCurrent = bool:get_pcvar_num(pCvar_FreeForAll)
    SetFFAMode(!bCurrent)
    client_print(0, print_chat, "[ReDM] Free-For-All Mode: %s!", !bCurrent ? "ENABLED (Semua Lawan Semua)" : "DISABLED (Team Deathmatch)")
    return PLUGIN_HANDLED
}

public Cmd_ConsoleFFA(id, level, cid) {
    if (!cmd_access(id, level, cid, 1)) return PLUGIN_HANDLED

    new szArg[8]
    read_argv(1, szArg, charsmax(szArg))

    if (szArg[0] != '^0') {
        SetFFAMode(bool:str_to_num(szArg))
    } else {
        new bool:bCurrent = bool:get_pcvar_num(pCvar_FreeForAll)
        SetFFAMode(!bCurrent)
    }

    console_print(id, "[ReDM] Free-For-All Mode is now: %s", get_pcvar_num(pCvar_FreeForAll) ? "ENABLED" : "DISABLED")
    return PLUGIN_HANDLED
}

public Cmd_BlockGrenade(id) {
    if (get_pcvar_num(pCvar_Active)) {
        client_print(id, print_center, "[ReDM] Grenades are disabled in Deathmatch!")
        return PLUGIN_HANDLED
    }
    return PLUGIN_CONTINUE
}

SetFFAMode(bool:bEnable) {
    set_pcvar_num(pCvar_FreeForAll, bEnable ? 1 : 0)
    if (bEnable) {
        set_cvar_num("mp_freeforall", 1)
        set_cvar_num("mp_friendlyfire", 1)
        set_cvar_num("mp_autokick", 0)
        set_cvar_num("mp_tkpunish", 0)
        set_cvar_num("mp_hostagepenalty", 0)
        server_cmd("yb_csdm_mode 2")
    } else {
        set_cvar_num("mp_freeforall", 0)
        set_cvar_num("mp_friendlyfire", 0)
        server_cmd("yb_csdm_mode 1")
    }
}

public Task_ShowPrimaryMenu(iTaskID) {
    new id = iTaskID - TASK_EQUIP_MENU
    if (!is_user_alive(id)) return
    ShowPrimaryMenu(id)
}

ShowPrimaryMenu(id) {
    if (!is_user_alive(id)) return
    g_bInMenu[id] = true

    new szTitle[64]
    formatex(szTitle, charsmax(szTitle), "\yReDM: Select Primary Weapon\R\w[1/2]")
    new hMenu = menu_create(szTitle, "Handler_PrimaryMenu")

    for (new i = 0; i < sizeof(g_szPrimaryNames); i++) {
        menu_additem(hMenu, g_szPrimaryNames[i], "")
    }

    if (g_iLastPrimary[id] >= 0) {
        menu_additem(hMenu, "\yRe-equip Previous Choice", "re_equip")
    }

    menu_setprop(hMenu, MPROP_EXIT, MEXIT_ALL)
    menu_display(id, hMenu, 0)
}

public Handler_PrimaryMenu(id, hMenu, item) {
    if (item == MENU_EXIT || !is_user_alive(id)) {
        g_bInMenu[id] = false
        menu_destroy(hMenu)
        return PLUGIN_HANDLED
    }

    new szInfo[16], szName[64], iAccess, iCallback
    menu_item_getinfo(hMenu, item, iAccess, szInfo, charsmax(szInfo), szName, charsmax(szName), iCallback)

    if (equal(szInfo, "re_equip")) {
        g_bInMenu[id] = false
        EquipPlayer(id, g_iLastPrimary[id], g_iLastSecondary[id])
        menu_destroy(hMenu)
        return PLUGIN_HANDLED
    }

    g_iLastPrimary[id] = item
    menu_destroy(hMenu)

    ShowSecondaryMenu(id)
    return PLUGIN_HANDLED
}

ShowSecondaryMenu(id) {
    if (!is_user_alive(id)) return
    g_bInMenu[id] = true

    new szTitle[64]
    formatex(szTitle, charsmax(szTitle), "\yReDM: Select Secondary Weapon\R\w[2/2]")
    new hMenu = menu_create(szTitle, "Handler_SecondaryMenu")

    for (new i = 0; i < sizeof(g_szSecondaryNames); i++) {
        menu_additem(hMenu, g_szSecondaryNames[i], "")
    }

    menu_additem(hMenu, "\rSave & Don't Show Again (say /guns to reset)", "auto_save")
    menu_setprop(hMenu, MPROP_EXIT, MEXIT_ALL)
    menu_display(id, hMenu, 0)
}

public Handler_SecondaryMenu(id, hMenu, item) {
    g_bInMenu[id] = false

    if (item == MENU_EXIT || !is_user_alive(id)) {
        menu_destroy(hMenu)
        return PLUGIN_HANDLED
    }

    new szInfo[16], szName[64], iAccess, iCallback
    menu_item_getinfo(hMenu, item, iAccess, szInfo, charsmax(szInfo), szName, charsmax(szName), iCallback)

    if (equal(szInfo, "auto_save")) {
        g_bAutoEquip[id] = true
        client_print(id, print_chat, "[ReDM] Auto-equip enabled! Type 'guns' in console/chat to reopen menu.")
        item = 0 // Default to Deagle
    }

    g_iLastSecondary[id] = item
    menu_destroy(hMenu)

    EquipPlayer(id, g_iLastPrimary[id], g_iLastSecondary[id])
    return PLUGIN_HANDLED
}

// -----------------------------------------------------------------------------
// Equip Logic (Unified for Players & Bots - Option 1)
// -----------------------------------------------------------------------------
AssignBotLoadout(id) {
    g_bAutoEquip[id] = true

    // Randomize primary: 0 = AK-47, 1 = M4A1 (M4A1-S / M4A4), 2 = AWP
    g_iLastPrimary[id] = random_num(0, 2)

    // Randomize secondary: Strictly 3 agreed weapons (0: Deagle, 1: Dual Berettas, 2: Five-Seven)
    g_iLastSecondary[id] = random_num(0, 2)
}

EquipPlayer(id, iPrimaryIdx, iSecondaryIdx) {
    if (!xash_is_bot_combat_ready(id)) return

    // 1. Strip all weapons (including default pistol/knife)
    strip_user_weapons(id)

    // 2. Strip any residual grenades
    cs_set_user_bpammo(id, CSW_HEGRENADE, 0)
    cs_set_user_bpammo(id, CSW_FLASHBANG, 0)
    cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 0)

    // 3. Give Knife
    give_item(id, "weapon_knife")

    // 4. Give Secondary: Strictly 3 agreed weapons
    // Index 0: Desert Eagle (weapon_deagle)
    // Index 1: Dual Berettas (weapon_elite)
    // Index 2: FN Five-Seven (weapon_fiveseven)
    if (iSecondaryIdx >= 0 && iSecondaryIdx < sizeof(g_szSecondaryEntities)) {
        give_item(id, g_szSecondaryEntities[iSecondaryIdx])
        cs_set_user_bpammo(id, g_iSecondaryAmmoTypes[iSecondaryIdx], 250)
    } else {
        give_item(id, "weapon_deagle")
        cs_set_user_bpammo(id, CSW_DEAGLE, 250)
    }

    // 5. Give Primary (AK-47, M4A1, AWP, etc.)
    new szPrimClass[32]
    new iPrimCSW = 0
    new iPrimEnt = 0
    if (iPrimaryIdx >= 0 && iPrimaryIdx < sizeof(g_szPrimaryEntities)) {
        copy(szPrimClass, charsmax(szPrimClass), g_szPrimaryEntities[iPrimaryIdx])
        iPrimCSW = g_iPrimaryAmmoTypes[iPrimaryIdx]
        iPrimEnt = give_item(id, szPrimClass)
        cs_set_user_bpammo(id, iPrimCSW, 250)

        // Custom M4 variant logic for both bots & players
        if (equal(szPrimClass, "weapon_m4a1")) {
            if (!pev_valid(iPrimEnt)) {
                iPrimEnt = xash_find_weapon_ent(id, szPrimClass)
            }
            if (pev_valid(iPrimEnt)) {
                if (random_num(0, 1) == 0) {
                    set_pev(iPrimEnt, pev_iuser1, 2) // M4A1-S
                    cs_set_weapon_silen(iPrimEnt, 1, 0)
                } else {
                    set_pev(iPrimEnt, pev_iuser1, 1) // M4A4
                    cs_set_weapon_silen(iPrimEnt, 0, 0)
                }
            }
        }
    }

    // 6. Give Kevlar + Helmet
    cs_set_user_armor(id, 100, CS_ARMOR_VESTHELM)

    // 7. Force deploy primary weapon (guarded against double-deploy)
    if (szPrimClass[0]) {
        xash_force_deploy_weapon(id, iPrimEnt, szPrimClass, iPrimCSW)
    }
}

public Task_AutoEquipPlayer(iTaskID) {
    new id = iTaskID - TASK_EQUIP_MENU
    if (!is_user_connected(id)) return
    if (get_user_health(id) <= 0) return

    if (g_bAutoEquip[id] && g_iLastPrimary[id] >= 0) {
        EquipPlayer(id, g_iLastPrimary[id], g_iLastSecondary[id])
    }
}

// -----------------------------------------------------------------------------
// Option 2 Safety Net: Strip default Glock-18 & USP on Deploy
// -----------------------------------------------------------------------------
public OnDefaultPistolDeploy_Post(iEnt) {
    if (!pev_valid(iEnt) || !get_pcvar_num(pCvar_Active)) return HAM_IGNORED

    new id = get_pdata_cbase(iEnt, 41, 4) // m_pPlayer
    if (id < 1 || id > MAX_PLAYERS || !is_user_bot(id)) return HAM_IGNORED

    // Bot deployed default Glock-18 or USP (from GameDLL AddDefaultItems)
    // Intercept immediately and replace with full assigned loadout
    if (!g_bAutoEquip[id] || g_iLastPrimary[id] < 0) {
        AssignBotLoadout(id)
    }

    EquipPlayer(id, g_iLastPrimary[id], g_iLastSecondary[id])
    return HAM_IGNORED
}

// -----------------------------------------------------------------------------
// Ground Clean Up & Objective Blocking
// -----------------------------------------------------------------------------
public Task_CleanDroppedWeapons(iTaskID) {
    new iEnt = -1
    while ((iEnt = find_ent_by_class(iEnt, "weaponbox")) > 0) {
        xash_destroy_weaponbox(iEnt)
    }

    static const szCleanClasses[][] = {
        "weapon_shield",
        "weapon_hegrenade",
        "weapon_flashbang",
        "weapon_smokegrenade"
    }

    for (new i = 0; i < sizeof(szCleanClasses); i++) {
        iEnt = -1
        while ((iEnt = find_ent_by_class(iEnt, szCleanClasses[i])) > 0) {
            remove_entity(iEnt)
        }
    }
}

public Msg_RoundTime(iMsgId, iMsgDest, id) {
    if (get_pcvar_num(pCvar_Active)) {
        set_msg_arg_int(1, ARG_SHORT, 99 * 60) // Keep round time high
    }
}

public Msg_SendAudio(iMsgId, iMsgDest, id) {
    if (!get_pcvar_num(pCvar_Active)) return PLUGIN_CONTINUE

    new szAudio[32]
    get_msg_arg_string(2, szAudio, charsmax(szAudio))

    // Block round end & round start radio spam in Deathmatch
    if (equal(szAudio, "%!MRAD_ctwin") || equal(szAudio, "%!MRAD_terwin") || equal(szAudio, "%!MRAD_rounddraw")
        || equal(szAudio, "%!MRAD_GO") || equal(szAudio, "%!MRAD_LETSGO") || equal(szAudio, "%!MRAD_MOVEOUT")
        || equal(szAudio, "%!MRAD_LOCKNLOAD") || equal(szAudio, "%!MRAD_FIREINHOLE")) {
        return PLUGIN_HANDLED
    }
    return PLUGIN_CONTINUE
}
