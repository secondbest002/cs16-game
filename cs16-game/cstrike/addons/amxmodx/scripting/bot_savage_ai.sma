/**
 * ==============================================================================
 * Plugin: CS 1.6 Bot Savage & Predator AI Engine (Version 3.4 - Tactical Chat & Danger Callouts)
 * Version: 3.4.0
 * Author: Antigravity
 * Description:
 *   Elevates YAPB / CS 1.6 bots into disciplined, coordinated Tier-1 Esports combatants.
 *
 *   New Tactical & Synchronization Systems (v3.4):
 *     1. Esports Player-to-Bot Chat Communication Engine (Full English Esports Persona Responses).
 *     2. Dynamic 'care [zone]' System (care a, care b, care short, care long, care mid, care awp, care flank).
 *     3. On-Demand Tactical Utility Coordination (flash me, popflash, smoke choke/cross).
 *     4. Real-Time Objective & C4 Management (drop c4/bomb, rotate a/b, save guns, cover defuse).
 *     5. Interactive Squad Intel & Economy Queries (utility?, money?, status?, damage?).
 *     6. Squad Pacing & Stealth Synchronization (walk/shift silent pacing, hold passive angles).
 *
 *   Preserved Core Systems:
 *     7. Dynamic Weapon Scavenging & Round-End Opportunity Hunting.
 *     8. Freeze Time Role Priority Delivery & Esports Role Economy.
 *     9. Competitive 5-Role Architecture & Player Menu (/role).
 *    10. Common Angle & Holding Angle Engine (Head-level crosshair lock & pre-scope).
 *    11. Interactive Weapon Drop Engine ('drop', /drop, radio Need backup).
 *    12. Instant Counter-Strafing & Spray Recoil Compensation.
 *    13. Auditory Sound ESP & Predictive Wallbang.
 *    14. Instant Trade-Frag Snapping & Anti-Flash 180° Reflex.
 *    15. High-IQ Fake Defuse Ambush & Synchronized Double-Peek.
 * ==============================================================================
 */

#include <amxmodx>
#include <amxmisc>
#include <cstrike>
#include <fakemeta>
#include <hamsandwich>
#include <engine>
#include <xs>
#include <fun>
#include <cs2_sdk/cs2_offsets>
#include <cs2_sdk/cs2_state_cvars>
#include <cs2_sdk/cs2_api>

#define PLUGIN_NAME    "Bot Savage Predator AI v3"

#include "bot_savage/core/savage_taskids.inc"
#include "bot_savage/core/savage_defs.inc"

#include "bot_savage/modules/savage_scavenge_utils.inl"
#include "bot_savage/modules/savage_roles_loadout.inl"
#include "bot_savage/modules/savage_igl_strats.inl"
#include "bot_savage/modules/savage_esports_chat.inl"
#include "bot_savage/modules/savage_drop_economy.inl"
#include "bot_savage/modules/savage_combat_tick.inl"
#include "bot_savage/modules/savage_reflex_esp.inl"

public plugin_precache()
{
    precache_model("models/w_ak47.mdl");
    precache_model("models/w_m4a1.mdl");
    if (file_exists("models/pw_models/w_m4a4.mdl") || file_exists("models/pw_models/w_m4a4.mdl", true))
        precache_model("models/pw_models/w_m4a4.mdl");
    else if (file_exists("models/csgo_skins/ct/w_m4a4.mdl") || file_exists("models/csgo_skins/ct/w_m4a4.mdl", true))
        precache_model("models/csgo_skins/ct/w_m4a4.mdl");
    precache_model("models/w_awp.mdl");
    precache_model("models/w_deagle.mdl");
    precache_model("models/w_galil.mdl");
    precache_model("models/w_famas.mdl");
    precache_model("models/w_mp5.mdl");
    precache_model("models/w_scout.mdl");

    // Modern CS weapon models
    if (file_exists("models/pw_models/w_tec9.mdl") || file_exists("models/pw_models/w_tec9.mdl", true))
        precache_model("models/pw_models/w_tec9.mdl");
    if (file_exists("models/pw_models/w_cz75.mdl") || file_exists("models/pw_models/w_cz75.mdl", true))
        precache_model("models/pw_models/w_cz75.mdl");
    if (file_exists("models/pw_models/w_mp7.mdl") || file_exists("models/pw_models/w_mp7.mdl", true))
        precache_model("models/pw_models/w_mp7.mdl");
    if (file_exists("models/pw_models/w_bizon.mdl") || file_exists("models/pw_models/w_bizon.mdl", true))
        precache_model("models/pw_models/w_bizon.mdl");
    if (file_exists("models/pw_models/w_mag7.mdl") || file_exists("models/pw_models/w_mag7.mdl", true))
        precache_model("models/pw_models/w_mag7.mdl");
    if (file_exists("models/pw_models/w_sawedoff.mdl") || file_exists("models/pw_models/w_sawedoff.mdl", true))
        precache_model("models/pw_models/w_sawedoff.mdl");
    if (file_exists("models/pw_models/w_negev.mdl") || file_exists("models/pw_models/w_negev.mdl", true))
        precache_model("models/pw_models/w_negev.mdl");

    precache_sound("items/gunpickup2.wav");
}

public plugin_natives()
{
    register_library("bot_savage_ai");
    set_native_filter("cs2_sdk_native_filter");
    register_native("savage_get_player_role", "_native_get_player_role", 0);
    register_native("savage_equip_bot_role", "_native_equip_bot_role", 0);
}

public SavageRole:_native_get_player_role(plugin_id, num_params)
{
    new id = get_param(1);
    if (id < 1 || id > MaxClients)
        return SavageRole:ROLE_NONE;

    if (g_iPlayerRole[id] == ROLE_NONE && is_user_connected(id))
    {
        AssignBotRoleDeterministic(id);
    }
    return SavageRole:g_iPlayerRole[id];
}

public _native_equip_bot_role(plugin_id, num_params)
{
    new id = get_param(1);
    new bool:bFreeLoadout = bool:get_param(2);
    new bool:bAllowUtility = (num_params >= 3 && get_param(3) != 0);

    if (id < 1 || id > MaxClients || !is_user_bot(id) || !xash_is_bot_combat_ready(id))
        return 0;

    return EquipBotByRole(id, bFreeLoadout, bAllowUtility);
}

public savage_get_bot_role(id)
{
    if (id < 1 || id > MaxClients || !is_user_connected(id))
        return ROLE_NONE;

    if (g_iPlayerRole[id] == ROLE_NONE)
    {
        AssignBotRoleDeterministic(id);
    }
    return g_iPlayerRole[id];
}


public plugin_init()
{
    register_plugin(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR);

    // Register Core CVARs
    g_pCvarEnable        = register_cvar("bot_savage_enable", "1");
    g_pCvarMicroOverride = register_cvar("bot_savage_micro_override", "0"); // 0 = Strict Separation of Concerns (YaPB C++ owns gun-round micro aim/move/fire)
    g_pCvarCounterStrafe = register_cvar("bot_savage_counterstrafe", "1");
    g_pCvarJiggle        = register_cvar("bot_savage_jiggle", "1");
    g_pCvarTradeFrag     = register_cvar("bot_savage_tradefrag", "1");
    g_pCvarAntiFlash     = register_cvar("bot_savage_antiflash", "1");
    g_pCvarTriggerbot    = register_cvar("bot_savage_triggerbot", "1");
    g_pCvarHiveMind      = register_cvar("bot_savage_hivemind", "1");
    g_pCvarReloadCover   = register_cvar("bot_savage_reloadcover", "1");

    // Register Combat CVARs
    g_pCvarRecoil        = register_cvar("bot_savage_recoil", "1");
    g_pCvarBhop          = register_cvar("bot_savage_bhop", "0"); // Default 0: Clean smooth movement!
    g_pCvarWallbang      = register_cvar("bot_savage_wallbang", "1");
    g_pCvarDoublePeek    = register_cvar("bot_savage_doublepeek", "1");
    g_pCvarFakeDefuse    = register_cvar("bot_savage_fakedefuse", "1");
    g_pCvarJumpScout     = register_cvar("bot_savage_jumpscout", "1");
    g_pCvarDropGuns      = register_cvar("bot_savage_dropguns", "1");

    // Register v3.2 & v3.3 Competitive CVARs
    g_pCvarRoles            = register_cvar("bot_savage_roles", "1");
    g_pCvarHolding          = register_cvar("bot_savage_holding", "1");
    g_pCvarCalls            = register_cvar("bot_savage_calls", "1");
    g_pCvarKnifeMidBrawl    = register_cvar("bot_savage_knife_midbrawl", "1");
    g_pCvarKnifeRoundActive = register_cvar("bot_savage_kniferound", "0");
    g_pCvarWeaponScavenge   = register_cvar("bot_savage_scavenge", "1");
    g_pCvarRoleDeliver      = register_cvar("bot_savage_roledeliver", "1");

    // Register v3.4 Tactical Chat & Callout CVARs
    g_pCvarChatCalls        = register_cvar("bot_savage_chat_calls", "1");
    g_pCvarChatCooldown     = register_cvar("bot_savage_chat_cooldown", "1.5");

    // Chat & Console Commands
    register_clcmd("say", "HandleSay");
    register_clcmd("say_team", "HandleSay");
    register_clcmd("role", "Cmd_OpenRoleMenu");
    register_clcmd("strat", "Cmd_OpenStratMenu");
    register_clcmd("needbackup", "Cmd_RadioNeedBackup");
    register_clcmd("coverme", "Cmd_RadioNeedBackup");
    register_clcmd("drop", "Cmd_PlayerDropWeaponPre");
    register_clcmd("+ping", "Cmd_PlayerPingPointPre");
    register_clcmd("amx_ping", "Cmd_PlayerPingPointPre");
    register_clcmd("ping_point", "Cmd_PlayerPingPointPre");

    // Hook usercmd processing (Every tick for players and bots)
    register_forward(FM_CmdStart, "OnCmdStart");

    // Ham Hooks
    RegisterHam(Ham_Spawn, "player", "OnPlayerSpawnPost", 1);
    RegisterHam(Ham_TakeDamage, "player", "OnPlayerTakeDamage", 1);
    RegisterHam(Ham_Killed, "player", "OnPlayerKilled", 1);
    RegisterHam(Ham_Touch, "info_target", "OnDroppedRifleTouch");
    RegisterHam(Ham_Think, "info_target", "OnDroppedRifleThink");
    RegisterHam(Ham_Touch, "weaponbox", "OnWeaponBoxTouchPre", 0);

    // True Freeze Time & Round Lifecycle Events
    register_event("HLTV", "Event_FreezeTimeStart", "a", "1=0", "2=0");
    register_event("TextMsg", "Event_GameRestart", "a", "2=#Game_will_restart_in", "2=#Game_Commencing");
    register_logevent("Event_RoundStart", 2, "1=Round_Start");
    register_event("RoundTime", "Event_RoundTime_Universal", "bc");
    register_event("ResetHUD", "Event_ResetHUD_Universal", "b");
    register_logevent("Event_RoundEnd", 2, "1=Round_End");
    register_event("SendAudio", "Event_RoundEndAudio", "a", "2=%!MRAD_terwin");
    register_event("SendAudio", "Event_RoundEndAudio", "a", "2=%!MRAD_ctwin");
    register_event("SendAudio", "Event_RoundEndAudio", "a", "2=%!MRAD_rounddraw");
    register_logevent("Event_PlantTheBomb", 3, "2=Planted_The_Bomb");
    register_logevent("Event_DefusedTheBomb", 3, "2=Defused_The_Bomb");

    // Periodic safe scanners
    set_task(0.10, "Task_CheckGrenades", TASK_SV_GRENADE, _, _, "b");
    set_task(0.15, "Task_SoundESPScanner", TASK_SV_SOUND, _, _, "b");
    set_task(0.35, "Task_ScavengeGroundWeapons", TASK_SV_SCAVENGE, _, _, "b");
    set_task(0.30, "Task_TacticalMidLineupScanner", TASK_SV_MID_SCAN, _, _, "b");

    server_print("[%s v%s] Activated successfully. Tier-1 Esports AI online.", PLUGIN_NAME, PLUGIN_VERSION);
}

public plugin_cfg()
{
    new szCfgPath[128];
    get_configsdir(szCfgPath, charsmax(szCfgPath));
    format(szCfgPath, charsmax(szCfgPath), "%s/bot_savage_roles.cfg", szCfgPath);

    if (file_exists(szCfgPath))
    {
        server_cmd("exec %s", szCfgPath);
        server_exec();
    }

    InitMapMidPoint();
    InitBombsites();
}

public client_putinserver(id)
{
    remove_task(TASK_SV_ROLE_MENU + id);
    remove_task(TASK_SV_SPAWN_EQUIP + id);
    remove_task(TASK_SV_REFILL_AMMO + id);
    ResetBotState(id);
    g_iPlayerRole[id] = ROLE_NONE;
    g_bRoleSelected[id] = false;
}

public client_disconnected(id)
{
    ResetBotState(id);
    g_iPlayerRole[id] = ROLE_NONE;
    g_bRoleSelected[id] = false;
    remove_task(TASK_SV_ROLE_MENU + id);
    remove_task(TASK_SV_SPAWN_EQUIP + id);
    remove_task(TASK_SV_REFILL_AMMO + id);
}

ResetBotState(id)
{
    g_flLastCombatTime[id] = 0.0;
    g_flNextStrafeSwitch[id] = 0.0;
    g_flNextTriggerCheck[id] = 0.0;
    g_flTriggerBurstUntil[id] = 0.0;
    g_bStrafeDirection[id] = false;
    g_bDuckTapActive[id] = false;
    g_flFlashDodgeUntil[id] = 0.0;
    g_iTradeTarget[id] = 0;
    g_flTradeExpire[id] = 0.0;

    g_bHoldingAngle[id] = false;
    g_flHoldingUntil[id] = 0.0;

    g_iSoundTarget[id] = 0;
    g_flSoundTargetExpire[id] = 0.0;
    g_flNextWallbangBurst[id] = 0.0;
    g_bDidFakeDefuse[id] = false;
    g_flFakeDefuseAmbushUntil[id] = 0.0;
    g_iDoublePeekPartner[id] = 0;
    g_bIsCrouchPeeker[id] = false;
    g_flDoublePeekExpire[id] = 0.0;

    g_iSavedWeapon[id] = 0;
    g_bSavedWeaponForRole[id] = false;
    g_iScavengeTarget[id] = 0;
    g_flScavengeExpire[id] = 0.0;
    g_flLastWeaponSwapTime[id] = 0.0;

    // Tactical Chat States Reset
    g_iTradeLeader[id] = 0;
    g_flTradeFollowUntil[id] = 0.0;
    g_flRearFlankUntil[id] = 0.0;
    g_flLastChatCallTime[id] = 0.0;
    g_iLastDamageTarget[id] = 0;
    g_iLastDamageDealt[id] = 0;
    g_flLastDamageTime[id] = 0.0;
}
