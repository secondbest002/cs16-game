#if defined _savage_igl_strats_included
    #endinput
#endif
#define _savage_igl_strats_included

// ==============================================================================
// 1. FREEZE TIME EVENT & IGL STRAT CALLS (At 0.0s Freeze Time)
// ==============================================================================
public Event_GameRestart()
{
    g_iRoundCount = 0;
}

public Event_FreezeTimeStart()
{
    if (IsWarmupActive())
    {
        g_bInFreezeTime = false;
        return;
    }

    g_bInFreezeTime = true;
    g_bRoundEnded = false;

    // Fail-safe universal unfreeze timer for both GoldSrc & Xash3D
    remove_task(TASK_SV_UNFREEZE);
    new Float:flFreeze = get_cvar_float("mp_freezetime");
    if (flFreeze > 0.0)
        set_task(flFreeze + 0.1, "Task_ForceReleaseBots", TASK_SV_UNFREEZE);
    else
        set_task(0.5, "Task_ForceReleaseBots", TASK_SV_UNFREEZE);

    // YaPB engine handles freeze time physics natively; clear any lingering freeze flags
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && is_user_bot(i))
        {
            new flags = pev(i, pev_flags);
            if (flags & FL_FROZEN)
                set_pev(i, pev_flags, flags & ~FL_FROZEN);
        }
    }

    new Float:flGameTime = get_gametime();
    g_flNextContactCall[CS_TEAM_T] = flGameTime;
    g_flNextContactCall[CS_TEAM_CT] = flGameTime;
    g_flNextBombCall[CS_TEAM_T] = flGameTime;
    g_flNextBombCall[CS_TEAM_CT] = flGameTime;

    if (!g_bMidPointCalculated)
    {
        InitMapMidPoint();
    }
    if (!g_bBombsitesFound)
    {
        InitBombsites();
    }

    g_bMidSmokedThisRound[CS_TEAM_T] = false;
    g_bMidSmokedThisRound[CS_TEAM_CT] = false;
    g_bMidFlashedThisRound[CS_TEAM_T] = false;
    g_bMidFlashedThisRound[CS_TEAM_CT] = false;
    g_bBombPlantedActive = false;
    g_bRetakeUtilThrown = false;
    g_iRetakeSyncTicks = 0;
    remove_task(TASK_SV_RETAKE_SYNC);
    Savage_Hazard_ResetRound();
    Savage_MolotovTactics_ResetRound();

    // Balance bot roles for both teams
    BalanceTeamRoles(CS_TEAM_T);
    BalanceTeamRoles(CS_TEAM_CT);

    // Reset round-specific states
    g_flSilentWalkUntil[CS_TEAM_T] = 0.0;
    g_flSilentWalkUntil[CS_TEAM_CT] = 0.0;
    g_flHoldAngleUntil[CS_TEAM_T] = 0.0;
    g_flHoldAngleUntil[CS_TEAM_CT] = 0.0;
    g_iActiveDangerZone[CS_TEAM_T] = ZONE_NONE;
    g_iActiveDangerZone[CS_TEAM_CT] = ZONE_NONE;
    g_flDangerZoneExpire[CS_TEAM_T] = 0.0;
    g_flDangerZoneExpire[CS_TEAM_CT] = 0.0;

    for (new i = 1; i <= 32; i++)
    {
        g_bDidFakeDefuse[i] = false;
        g_flFakeDefuseAmbushUntil[i] = 0.0;
        g_iDoublePeekPartner[i] = 0;
        g_bHoldingAngle[i] = false;
        g_flHoldingUntil[i] = 0.0;
        g_flNextHoldCheck[i] = 0.0;
        g_flOriginalMaxSpeed[i] = 0.0;
        g_flLineupThrowUntil[i] = 0.0;
        g_bLineupActive[i] = false;
        Savage_Bridge_ResetBot(i);
        g_flRearFlankUntil[i] = 0.0;
        g_iTradeLeader[i] = 0;
        g_flTradeFollowUntil[i] = 0.0;
        g_flNextTriggerCheck[i] = 0.0;
        g_flTriggerBurstUntil[i] = 0.0;
        g_flNextKnifeGoalPush[i] = 0.0;
        g_flNextMacroGoalPush[i] = 0.0;
    }

    // Check Knife Round Condition
    if (IsKnifeRoundActive())
    {
        g_iRoundCount = 0;
        g_flKnifeRoundStartTime = flGameTime;
        g_bAntiStallTriggered = false;

        // Force YaPB to stop doing bombsite objectives & stop camping/conga-lining!
        set_cvar_num("yb_ignore_objectives", 1);
        set_cvar_num("yb_camping_allowed", 0);
        set_cvar_num("yb_user_follow_percent", 0);
        set_cvar_num("yb_user_max_followers", 0);

        if (get_pcvar_num(g_pCvarKnifeMidBrawl))
        {
            BroadcastTeamCall(CS_TEAM_T, "^4[IGL Knife Call]^3 ⚔ KNIFE ROUND! All units RUSH MID for the brawl! ⚔");
            PlayTeamAudio(CS_TEAM_T, "spk radio/stormfront");

            BroadcastTeamCall(CS_TEAM_CT, "^4[IGL Knife Call]^3 ⚔ KNIFE ROUND! All units RUSH MID for the brawl! ⚔");
            PlayTeamAudio(CS_TEAM_CT, "spk radio/stormfront");
            return;
        }
    }
    else if (IsWarmupActive())
    {
        g_iRoundCount = 0;

        // Force YaPB to stop doing bombsite objectives during Warmup!
        set_cvar_num("yb_ignore_objectives", 1);
        set_cvar_num("yb_camping_allowed", 0);
        return;
    }
    else
    {
        g_iRoundCount++;

        // Restore standard competitive objectives for gun rounds (Native camping disabled per Argon audit)
        set_cvar_num("yb_ignore_objectives", 0);
        set_cvar_num("yb_camping_allowed", 0);
    }

    // LIVE MATCH GUN ROUND: Check if human player is IGL
    new bool:bHumanIsIglT = false;
    new bool:bHumanIsIglCT = false;

    for (new i = 1; i <= 32; i++)
    {
        if (is_user_connected(i) && !is_user_bot(i) && g_iPlayerRole[i] == ROLE_IGL)
        {
            new CsTeams:team = cs_get_user_team(i);
            if (team == CS_TEAM_T)
            {
                bHumanIsIglT = true;
                ShowIglStratMenu(i);
            }
            else if (team == CS_TEAM_CT)
            {
                bHumanIsIglCT = true;
                ShowIglStratMenu(i);
            }
        }
    }

    // If IGL is a bot, schedule callout in Freeze Time (while still in spawn!)
    if (get_pcvar_num(g_pCvarCalls))
    {
        new Float:flCallDelay = 0.8;
        if (IsTimeoutActive())
        {
            // During tactical timeout, delay strat call until near end of freeze time
            new freeze = get_cvar_num("mp_freezetime");
            flCallDelay = floatmax(2.0, float(freeze) - 3.0);
        }

        if (!bHumanIsIglT)
            set_task(flCallDelay, "Task_AnnounceTStrat", TASK_SV_CALL_T);

        if (!bHumanIsIglCT)
            set_task(flCallDelay, "Task_AnnounceCTStrat", TASK_SV_CALL_CT);
    }

    // Schedule Spawn Dropped Weapon Claiming in Freeze Time (0.4s & 0.9s)
    set_task(0.4, "Task_ScanSpawnDroppedWeapons", TASK_SV_SCAN_WPNS);
    set_task(0.9, "Task_ScanSpawnDroppedWeapons", TASK_SV_SCAN_WPNS);

    // Schedule Role Priority Weapon Delivery at 1.0s into Freeze Time
    if (get_pcvar_num(g_pCvarEnable) && get_pcvar_num(g_pCvarRoleDeliver))
    {
        set_task(1.0, "Task_DeliverSavedWeapons", TASK_SV_DELIVER);
    }

    // Schedule Role Loadout Enforcement at 1.4s into Freeze Time
    set_task(1.4, "Task_EnforceRoleLoadouts", TASK_SV_ENFORCE);

    // Schedule Team Economy Weapon Drop at 2.0s into Freeze Time
    if (get_pcvar_num(g_pCvarEnable) && get_pcvar_num(g_pCvarDropGuns))
    {
        set_task(2.0, "Task_EconomyWeaponDrop", TASK_SV_ECONOMY);
    }
}

public Task_AnnounceTStrat()
{
    ExecuteTeamStrat(CS_TEAM_T);
}

public Task_AnnounceCTStrat()
{
    ExecuteTeamStrat(CS_TEAM_CT);
}

ExecuteTeamStrat(CsTeams:team)
{
    new iIgl = GetTeamIgl(team);
    if (iIgl <= 0)
        return;

    new szIglName[32];
    get_user_name(iIgl, szIglName, charsmax(szIglName));

    if (team == CS_TEAM_T)
    {
        new iStrat = random_num(1, 5);
        g_iTeamStrat[team] = iStrat;

        switch (iStrat)
        {
            case STRAT_RUSH_A:
            {
                BroadcastTeamCall(team, "^4[IGL Call]^3 %s^1: Strat: ^4Fast Rush Bombsite A^1! Flash out and take entry!", szIglName);
                PlayTeamAudio(team, "spk radio/stormfront");
            }
            case STRAT_RUSH_B:
            {
                BroadcastTeamCall(team, "^4[IGL Call]^3 %s^1: Strat: ^4Execute Bombsite B^1! Smoke tunnels, stick together!", szIglName);
                PlayTeamAudio(team, "spk radio/go_go_go");
            }
            case STRAT_DEFAULT:
            {
                BroadcastTeamCall(team, "^4[IGL Call]^3 %s^1: Strat: ^4Default 1-3-1 & Hold for Picks^1! Do not rush, play slow!", szIglName);
                PlayTeamAudio(team, "spk radio/sticktog");
            }
            case STRAT_SPLIT:
            {
                BroadcastTeamCall(team, "^4[IGL Call]^3 %s^1: Strat: ^4Split Mid to Bombsite^1! Cut off rotators!", szIglName);
                PlayTeamAudio(team, "spk radio/flankthem");
            }
            case STRAT_ECO:
            {
                BroadcastTeamCall(team, "^4[IGL Call]^3 %s^1: Strat: ^4Eco Stack^1! Group up, hunt for rifle drops!", szIglName);
                PlayTeamAudio(team, "spk radio/regroup");
            }
        }
    }
    else if (team == CS_TEAM_CT)
    {
        new iStrat = random_num(1, 4);
        g_iTeamStrat[team] = iStrat;

        switch (iStrat)
        {
            case 1:
            {
                BroadcastTeamCall(team, "^4[IGL Call]^3 %s^1: Strat: ^4Standard 2-1-2 Crossfire Hold^1! Anchors stay on sites!", szIglName);
                PlayTeamAudio(team, "spk radio/sticktog");
            }
            case 2:
            {
                BroadcastTeamCall(team, "^4[IGL Call]^3 %s^1: Strat: ^4Aggressive Chokepoint Information Push^1!", szIglName);
                PlayTeamAudio(team, "spk radio/stormfront");
            }
            case 3:
            {
                BroadcastTeamCall(team, "^4[IGL Call]^3 %s^1: Strat: ^4Site Stack Defense^1! Fall back and play retake if overwhelmed!", szIglName);
                PlayTeamAudio(team, "spk radio/fallback");
            }
            case 4:
            {
                BroadcastTeamCall(team, "^4[IGL Call]^3 %s^1: Strat: ^4Eco Crossfire Stack^1! Play close angles with Deagles!", szIglName);
                PlayTeamAudio(team, "spk radio/regroup");
            }
        }
    }
}

// ==============================================================================
// 2. ROUND START (FREEZE TIME ENDS - MOVEMENT UNFREEZES)
// ==============================================================================
public Event_RoundStart()
{
    g_bInFreezeTime = false;
    g_flRoundStartTime = get_gametime();
    remove_task(TASK_SV_UNFREEZE);

    g_bMidSmokedThisRound[CS_TEAM_T] = false;
    g_bMidSmokedThisRound[CS_TEAM_CT] = false;
    g_bMidFlashedThisRound[CS_TEAM_T] = false;
    g_bMidFlashedThisRound[CS_TEAM_CT] = false;

    // Unfreeze all bots now that freeze time expired & route Knife Round to Mid via YaPB graph
    new bool:bRouteKnifeMid = (IsKnifeRoundActive() && get_pcvar_num(g_pCvarKnifeMidBrawl) && g_bYapbAvailable && g_bMidPointCalculated);
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && is_user_bot(i))
        {
            new flags = pev(i, pev_flags);
            set_pev(i, pev_flags, flags & ~FL_FROZEN);

            if (bRouteKnifeMid && yb_is_user_bot(i))
            {
                yb_set_bot_goal_origin(i, g_vecMapMidPoint);
                g_flNextKnifeGoalPush[i] = g_flRoundStartTime + 3.0;
            }
        }
    }

    // Dispatch real engine radio command so YaPB bot engine actually obeys!
    DispatchIglEngineCommand(CS_TEAM_T);
    DispatchIglEngineCommand(CS_TEAM_CT);

    // BP-01 Macro Strat Nav-Graph Dispatch (Live Match Terrorist execute/split)
    if (!IsKnifeRoundActive() && !IsWarmupActive())
    {
        Savage_DispatchStratMacroGoals(CS_TEAM_T, false);
    }
}

stock Savage_DispatchStratMacroGoals(CsTeams:team, bool:bForceImmediate = false)
{
    if (!get_pcvar_num(g_pCvarEnable) || !g_bYapbAvailable || g_bInFreezeTime)
        return;

    if (IsKnifeRoundActive() || IsWarmupActive() || IsDeathmatchActive() || !g_bBombsitesFound)
        return;

    new iStrat = g_iTeamStrat[team];
    if (iStrat != STRAT_RUSH_A && iStrat != STRAT_RUSH_B && iStrat != STRAT_SPLIT)
        return;

    new Float:flCooldown = bForceImmediate ? 0.0 : 4.0;
    new Float:vecSplitSite[3];
    xs_vec_copy((g_iRoundCount % 2 == 0) ? g_vecBombsiteA : g_vecBombsiteB, vecSplitSite);

    for (new id = 1; id <= MaxClients; id++)
    {
        if (!is_user_alive(id) || !is_user_bot(id) || safe_cs_get_user_team(id) != team)
            continue;

        if (!Savage_Bridge_CanRouteMacro(id, true))
            continue;

        new iRole = g_iPlayerRole[id];
        if (!bForceImmediate && iStrat != STRAT_SPLIT && iRole == ROLE_LURKER)
            continue;

        if (iStrat == STRAT_RUSH_A)
        {
            Savage_Bridge_RouteMacroGoal(id, g_vecBombsiteA, flCooldown, true);
        }
        else if (iStrat == STRAT_RUSH_B)
        {
            Savage_Bridge_RouteMacroGoal(id, g_vecBombsiteB, flCooldown, true);
        }
        else if (iStrat == STRAT_SPLIT)
        {
            if (g_bMidPointCalculated && (iRole == ROLE_ENTRY || iRole == ROLE_IGL || iRole == ROLE_SUPPORT))
                Savage_Bridge_RouteMacroGoal(id, g_vecMapMidPoint, flCooldown, true);
            else
                Savage_Bridge_RouteMacroGoal(id, vecSplitSite, flCooldown, true);
        }
    }
}

public Task_ForceReleaseBots()
{
    if (g_bInFreezeTime)
    {
        Event_RoundStart();
    }
}

public Event_RoundTime_Universal()
{
    if (g_bInFreezeTime)
    {
        Event_RoundStart();
    }
}

public Event_ResetHUD_Universal(id)
{
    if (is_user_alive(id) && is_user_bot(id) && !g_bInFreezeTime)
    {
        new flags = pev(id, pev_flags);
        if (flags & FL_FROZEN)
        {
            set_pev(id, pev_flags, flags & ~FL_FROZEN);
        }
    }
}

DispatchIglEngineCommand(CsTeams:team)
{
    new iIgl = GetTeamIgl(team);
    if (iIgl <= 0 || !is_user_alive(iIgl))
        return;

    if (IsKnifeRoundActive())
    {
        engclient_cmd(iIgl, "stormfront");
        set_cvar_num("yb_user_follow_percent", 0);
        set_cvar_num("yb_user_max_followers", 0);
        return;
    }

    new iStrat = g_iTeamStrat[team];
    switch (iStrat)
    {
        case STRAT_RUSH_A, STRAT_RUSH_B:
        {
            engclient_cmd(iIgl, "stormfront");
            set_cvar_num("yb_user_follow_percent", 10);
            set_cvar_num("yb_user_max_followers", 1);
        }
        case STRAT_DEFAULT:
        {
            engclient_cmd(iIgl, "sticktog");
            set_cvar_num("yb_user_follow_percent", 10);
            set_cvar_num("yb_user_max_followers", 1);
        }
        case STRAT_SPLIT:
        {
            engclient_cmd(iIgl, "flankthem");
            set_cvar_num("yb_user_follow_percent", 10);
            set_cvar_num("yb_user_max_followers", 1);
        }
        case STRAT_ECO:
        {
            engclient_cmd(iIgl, "regroup");
            set_cvar_num("yb_user_follow_percent", 10);
            set_cvar_num("yb_user_max_followers", 1);
        }
        default:
        {
            engclient_cmd(iIgl, "go");
            set_cvar_num("yb_user_follow_percent", 10);
            set_cvar_num("yb_user_max_followers", 1);
        }
    }
}


// ==============================================================================
// 4. HUMAN PLAYER IGL STRAT SELECTION MENU
// ==============================================================================
public ShowIglStratMenu(id)
{
    new menu = menu_create("\y[IGL Command] Call Strat for this Round:", "HandleIglStratMenu");

    menu_additem(menu, "\w⚡ Fast Rush Bombsite A", "1");
    menu_additem(menu, "\w🔥 Execute Bombsite B", "2");
    menu_additem(menu, "\w🛡️ Default 1-3-1 (Hold for Picks)", "3");
    menu_additem(menu, "\w⚔️ Split Mid to Site", "4");
    menu_additem(menu, "\w💰 Full Eco / Group Save", "5");

    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, menu, 0);
}

public HandleIglStratMenu(id, menu, item)
{
    if (item == MENU_EXIT)
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new szData[6], szName[64], iAccess, iCallback;
    menu_item_getinfo(menu, item, iAccess, szData, charsmax(szData), szName, charsmax(szName), iCallback);
    new iStrat = str_to_num(szData);

    if (iStrat >= 1 && iStrat <= 5)
    {
        new CsTeams:team = cs_get_user_team(id);
        g_iTeamStrat[team] = iStrat;

        new szPlayerName[32];
        get_user_name(id, szPlayerName, charsmax(szPlayerName));

        BroadcastTeamCall(team, "^4[IGL Call]^3 %s^1: Strat: ^4%s^1!", szPlayerName, g_szStratNames[iStrat]);

        DispatchIglEngineCommand(team);
        if (!g_bInFreezeTime)
        {
            Savage_DispatchStratMacroGoals(team, true);
        }

        // Bot teammates acknowledge
        for (new b = 1; b <= 32; b++)
        {
            if (is_user_alive(b) && is_user_bot(b) && cs_get_user_team(b) == team)
            {
                client_cmd(id, "spk radio/roger");
                break;
            }
        }
    }

    menu_destroy(menu);
    return PLUGIN_HANDLED;
}

public Cmd_OpenStratMenu(id)
{
    if (is_user_connected(id) && g_iPlayerRole[id] == ROLE_IGL)
    {
        ShowIglStratMenu(id);
    }
    else
    {
        client_print_color(id, print_team_default, "^4[IGL]^1 You must have the ^3IGL role^1 (/role) to call team strategies!");
    }
    return PLUGIN_HANDLED;
}
