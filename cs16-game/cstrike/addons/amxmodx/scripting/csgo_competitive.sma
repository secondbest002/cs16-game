#include <amxmodx>
#include <amxmisc>
#include <cstrike>
#include <fakemeta>
#include <hamsandwich>
#include <fun>
#include <cs2_sdk/cs2_offsets>
#include <cs2_sdk/cs2_api>


#include "cs2_competitive/core/comp_taskids.inc"
#include "cs2_competitive/core/comp_defs.inc"

#include "cs2_competitive/modules/comp_admin_chat.inl"
#include "cs2_competitive/modules/comp_warmup_ready.inl"
#include "cs2_competitive/modules/comp_knife_sidepick.inl"
#include "cs2_competitive/modules/comp_match_flow.inl"
#include "cs2_competitive/modules/comp_economy_round.inl"
#include "cs2_competitive/modules/comp_dmg_mvp_hud.inl"

public plugin_natives()
{
    set_native_filter("cs2_sdk_native_filter");
}

public plugin_init()
{
    register_plugin(PLUGIN, VERSION, AUTHOR);

    // Cvars
    pCvar_Mode            = register_cvar("amx_csgo_mode", "1");
    bind_pcvar_num(pCvar_Mode, g_iCsgoMode);
    hook_cvar_change(pCvar_Mode, "OnCvarModeChanged");
    pCvar_MaxRounds       = register_cvar("amx_csgo_maxrounds", "12"); // MR12 (halftime at 12, win at 13)
    pCvar_KnifeRound      = register_cvar("amx_csgo_kniferound", "1");
    pCvar_Warmup          = register_cvar("amx_csgo_warmup", "1");
    pCvar_Timeouts        = register_cvar("amx_csgo_timeouts", "1"); // 1 timeout per team per half
    pCvar_TimeoutDuration = register_cvar("amx_csgo_timeout_duration", "60"); // 60 seconds
    pCvar_OTMoney         = register_cvar("amx_csgo_ot_money", "10000"); // Start money in OT
    pCvar_BotAutoReady    = register_cvar("amx_csgo_bot_autoready", "1"); // Bots ready automatically with humans
    pCvar_LossBonus       = register_cvar("amx_csgo_lossbonus", "1"); // CS:GO loss bonus decrement rule
    pCvar_HudStyle        = register_cvar("amx_csgo_hud_style", "1"); // 1 = CS:GO Modern Header, 0 = Classic Simple
    pCvar_MvpCard         = register_cvar("amx_csgo_mvp_card", "1"); // 1 = Show Center MVP Card, 0 = Chat only
    pCvar_C4Timer         = register_cvar("amx_csgo_c4_timer", "1"); // 1 = Show C4 Countdown in Top Bar, 0 = Disabled
    pCvar_ScreenFade      = register_cvar("amx_csgo_screenfade", "1"); // 1 = Enable cinematic screenfade transitions
    pCvar_DmgCard         = register_cvar("amx_csgo_dmg_card", "1"); // 1 = Show on-screen damage report card
    pCvar_FreezeTime      = register_cvar("amx_csgo_freezetime", "20"); // 20 seconds freeze time (CS2 / FACEIT rule)
    pCvar_CurrentRound    = register_cvar("amx_csgo_current_round", "0"); // Current live match round
    pCvar_WarmupTime      = register_cvar("amx_csgo_warmuptime", "55"); // Warmup DM limit in minutes (default 55)
    register_cvar("bot_savage_kniferound", "0"); // Savage AI Knife Round Signal
    register_cvar("amx_csgo_is_warmup", "1"); // Signal whether Warmup DM is currently active
    register_cvar("amx_csgo_is_timeout", "0"); // Signal whether Tactical Timeout is currently active
    register_message(get_user_msgid("RoundTime"), "Msg_RoundTime");

    // Chat & Console Commands
    register_clcmd("say", "HandleSay");
    register_clcmd("say_team", "HandleSay");
    register_clcmd("ready", "Cmd_Ready");
    register_clcmd(".ready", "Cmd_Ready");
    register_clcmd("!ready", "Cmd_Ready");
    register_clcmd("unready", "Cmd_Unready");
    register_clcmd(".unready", "Cmd_Unready");
    register_clcmd("!unready", "Cmd_Unready");

    // Buy & drop blocking during knife round & warmup (All aliases + grenades)
    for (new i = 0; i < sizeof(g_BuyCommands); i++)
    {
        register_clcmd(g_BuyCommands[i], "BlockBuyCmd");
    }
    register_clcmd("buy_hegrenade", "BlockBuyCmd");
    register_clcmd("buy_flashbang", "BlockBuyCmd");
    register_clcmd("buy_smokegrenade", "BlockBuyCmd");
    register_clcmd("drop", "BlockDropCmd");

    // Admin Commands
    register_concmd("amx_csgo_start", "AdminCmd_StartMatch", ADMIN_CFG, "- Force start live match");
    register_concmd("amx_csgo_warmup", "AdminCmd_RestartWarmup", ADMIN_CFG, "- Return to warmup");

    // Ham Hooks
    g_hHamSpawnPost   = RegisterHam(Ham_Spawn, "player", "OnPlayerSpawnPost", 1);
    RegisterHam(Ham_Killed, "player", "OnPlayerKilledPre", 0);
    g_hHamKilledPost  = RegisterHam(Ham_Killed, "player", "OnPlayerKilledPost", 1);
    g_hHamDeployGlock = RegisterHam(Ham_Item_Deploy, "weapon_glock18", "OnDefaultPistolDeploy_Post", 1);
    g_hHamDeployUsp   = RegisterHam(Ham_Item_Deploy, "weapon_usp", "OnDefaultPistolDeploy_Post", 1);
    RegisterHam(Ham_TakeDamage, "player", "OnTakeDamagePost", 1);
    RegisterHam(Ham_Touch, "weaponbox", "OnWeaponTouchPre");
    RegisterHam(Ham_Touch, "armoury_entity", "OnWeaponTouchPre");
    RegisterHam(Ham_Touch, "weapon_shield", "OnWeaponTouchPre");
    RegisterHam(Ham_Item_AddToPlayer, "weapon_c4", "OnC4AddToPlayer");

    // Game Events
    register_event("HLTV", "Event_FreezeTimeStart", "a", "1=0", "2=0");
    register_event("TextMsg", "Event_GameRestart", "a", "2=#Game_will_restart_in", "2=#Game_Commencing");
    register_event("CurWeapon", "Event_CurWeapon", "be", "1=1");
    register_event("SendAudio", "Event_TWin", "a", "2=%!MRAD_terwin");
    register_event("SendAudio", "Event_CTWin", "a", "2=%!MRAD_ctwin");
    register_event("SendAudio", "Event_RoundDraw", "a", "2=%!MRAD_rounddraw");
    register_logevent("LogEvent_RoundStart", 2, "1=Round_Start");
    register_logevent("LogEvent_RoundEnd", 2, "1=Round_End");
    register_logevent("LogEvent_Plant", 3, "2=Planted_The_Bomb");
    register_logevent("LogEvent_Defuse", 3, "2=Defused_The_Bomb");
    register_logevent("LogEvent_TargetBombed", 3, "2=Target_Bombed");
    register_logevent("LogEvent_TargetBombed", 2, "1=Target_Bombed");

    // Multiple HUD Channels
    g_HudSyncHeader    = CreateHudSyncObj();
    g_HudSyncSubHeader = CreateHudSyncObj();
    g_HudSyncDamage    = CreateHudSyncObj();

    // Start HUD Task
    set_task(1.0, "Task_UpdateHUD", TASK_COMP_HUD, _, _, "b");

    server_print("[CS:GO Competitive] Plugin v%s initialized successfully.", VERSION);
}

public OnConfigsExecuted()
{
    // Deteksi silang: Jika redm_active bernilai 1, paksa amx_csgo_mode = 0
    pCvar_RedmActive = get_cvar_pointer("redm_active");
    if (pCvar_RedmActive != 0 && get_pcvar_num(pCvar_RedmActive) == 1)
    {
        set_pcvar_num(pCvar_Mode, 0);
        g_iCsgoMode = 0;
    }
    else
    {
        g_iCsgoMode = get_pcvar_num(pCvar_Mode);
    }

    SyncCompetitiveLifecycle(g_iCsgoMode > 0);
}

public OnCvarModeChanged(pcvar, const old_value[], const new_value[])
{
    new bool:bEnable = (str_to_num(new_value) > 0);
    g_iCsgoMode = bEnable ? 1 : 0;
    SyncCompetitiveLifecycle(bEnable);
}

stock SyncCompetitiveLifecycle(bool:bEnable)
{
    if (!bEnable)
    {
        g_iCsgoMode = 0;
        // Matikan Ham Forwards di level C++ Hamsandwich
        if (g_hHamSpawnPost)   DisableHamForward(g_hHamSpawnPost);
        if (g_hHamKilledPost)  DisableHamForward(g_hHamKilledPost);
        if (g_hHamDeployGlock) DisableHamForward(g_hHamDeployGlock);
        if (g_hHamDeployUsp)   DisableHamForward(g_hHamDeployUsp);

        // Bersihkan seluruh task global dan task spesifik pemain
        remove_task(TASK_COMP_WARMUP);
        remove_task(TASK_COMP_SWEEP1);
        remove_task(TASK_COMP_SWEEP2);
        remove_task(TASK_COMP_HUD);
        remove_task(TASK_COMP_COUNTDOWN);
        remove_task(TASK_COMP_SIDEPICK);
        remove_task(TASK_COMP_TIMEOUT);
        remove_task(TASK_COMP_HALF2);
        remove_task(TASK_COMP_OT2);
        remove_task(TASK_COMP_DRAW_KNIFE);
        remove_task(TASK_COMP_AUTOPICK);

        for (new i = 1; i <= MaxClients; i++)
        {
            remove_task(i + TASK_COMP_RESPAWN);
            remove_task(i + TASK_COMP_BOTEQUIP);
            remove_task(i + TASK_COMP_STRIPKNIFE);
            g_bEquippingBot[i] = false;
            g_iBotEquipRetries[i] = 0;
        }

        // Hapus HUD pesan agar layar bersih seketika (menghilangkan banner .ready dan timer 55 menit)
        ShowSyncHudMsg(0, g_HudSyncHeader, "");
        ShowSyncHudMsg(0, g_HudSyncSubHeader, "");

        log_amx("[CSGO-COMP] Mode Competitive dinonaktifkan (amx_csgo_mode 0). Seluruh hook dan task telah dibekukan.");
    }
    else
    {
        if (g_hHamSpawnPost)   EnableHamForward(g_hHamSpawnPost);
        if (g_hHamKilledPost)  EnableHamForward(g_hHamKilledPost);
        if (g_hHamDeployGlock) EnableHamForward(g_hHamDeployGlock);
        if (g_hHamDeployUsp)   EnableHamForward(g_hHamDeployUsp);

        log_amx("[CSGO-COMP] Mode Competitive diaktifkan (amx_csgo_mode 1).");
    }
}

UpdateMatchFormat()
{
    new rounds = get_pcvar_num(pCvar_MaxRounds);
    // Support both MR format (12 = MR12, 15 = MR15, 8 = MR8)
    // and total match format (24 -> MR12, 30 -> MR15, 16 -> MR8)
    if (rounds >= 16)
    {
        g_MaxRoundsHalf = rounds / 2;
    }
    else if (rounds >= 1)
    {
        g_MaxRoundsHalf = rounds;
    }
    else
    {
        g_MaxRoundsHalf = 12; // CS2 standard MR12 default
    }
    g_TargetWinScore = g_MaxRoundsHalf + 1;
}

public plugin_cfg()
{
    pCvar_RedmActive = get_cvar_pointer("redm_active");
    if ((pCvar_RedmActive != 0 && get_pcvar_num(pCvar_RedmActive) == 1) || get_pcvar_num(pCvar_Mode) == 0)
    {
        g_iCsgoMode = 0;
        SyncCompetitiveLifecycle(false);
        return;
    }

    new config_file[128];
    get_configsdir(config_file, charsmax(config_file));
    format(config_file, charsmax(config_file), "%s/csgo_competitive.cfg", config_file);

    if (file_exists(config_file))
    {
        server_cmd("exec %s", config_file);
        server_exec();
    }

    UpdateMatchFormat();

    ResetToWarmup();
}

public client_putinserver(id)
{
    g_PlayerReady[id] = false;
    g_bEquippingBot[id] = false;
    g_iBotEquipRetries[id] = 0;
    ResetPlayerDamage(id);
}

public client_disconnected(id)
{
    g_PlayerReady[id] = false;
    g_bEquippingBot[id] = false;
    g_iBotEquipRetries[id] = 0;
    ResetPlayerDamage(id);
    remove_task(id + TASK_COMP_RESPAWN);
    remove_task(id + TASK_COMP_BOTEQUIP);
    remove_task(id + TASK_COMP_STRIPKNIFE);
}


public plugin_end()
{
    SetBotKnifeOnly(false);
}
