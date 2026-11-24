#if defined _comp_knife_sidepick_included
    #endinput
#endif
#define _comp_knife_sidepick_included

// ========================================================================
// KNIFE ROUND
// ========================================================================
public StartKnifeRound()
{
    remove_task(TASK_COMP_WARMUP);
    remove_task(TASK_COMP_COUNTDOWN);

    g_MatchState = STATE_KNIFE_ROUND;
    g_bKnifeRoundLive = false;
    g_KnifeWinner = CS_TEAM_UNASSIGNED;
    g_KnifeChooserId = 0;
    g_BombPlanted = false;

    set_cvar_num("amx_csgo_is_warmup", 0);
    remove_task(TASK_COMP_SWEEP1);
    remove_task(TASK_COMP_SWEEP2);

    for (new i = 1; i <= MaxClients; i++)
    {
        remove_task(i + TASK_COMP_BOTEQUIP);
        remove_task(i + TASK_COMP_RESPAWN);
        g_bEquippingBot[i] = false;
        g_iBotEquipRetries[i] = 0;
        if (is_user_connected(i))
        {
            cs_set_user_money(i, 0);
        }
    }

    // Force bots (YaPB/PodBot) to knife-only & disable buying routines
    ApplyYaPBState(STATE_KNIFE_ROUND);
    set_cvar_num("bot_savage_kniferound", 1);
    set_cvar_float("mp_roundtime", 1.92);
    set_cvar_num("mp_freezetime", 3);
    set_cvar_num("mp_startmoney", 0);
    set_cvar_float("mp_buytime", 0.0);
    set_cvar_num("mp_buy_anywhere", 0);
    set_cvar_string("mp_round_infinite", "0");
    set_cvar_num("sv_restart", 1);

    // Visual Announcement (DHUD & Screen Flash)
    set_dhudmessage(50, 255, 140, -1.0, 0.28, 2, 0.1, 4.0, 0.1, 1.0);
    show_dhudmessage(0, "⚔ KNIFE ROUND ⚔^nFight for Side Choice! (Knives Only)");

    if (get_pcvar_num(pCvar_ScreenFade) == 1)
    {
        Util_ScreenFade(0, 0.6, 0.2, FFADE_IN, 50, 255, 140, 45);
    }

    client_print(0, print_chat, "^x04==================================================");
    client_print(0, print_chat, "^x04[CS:GO Match] KNIFE ROUND! Fight for side choice!");
    client_print(0, print_chat, "^x04==================================================");
}


// ========================================================================
// SIDE PICK (KNIFE WINNER)
// ========================================================================
public Task_BotAutoPickStay()
{
    if (g_MatchState == STATE_SIDE_PICK)
    {
        Cmd_SideChoice(0, 1);
    }
}

public ShowSidePickMenu(id)
{
    new menu = menu_create("\y[Knife Round Winner] Choose Your Side:\w", "HandleMenuSidePick");
    menu_additem(menu, "Stay on Current Side (.stay)", "1");
    menu_additem(menu, "Switch Sides (.switch)", "2");
    menu_setprop(menu, MPROP_EXIT, MEXIT_NEVER);
    menu_display(id, menu, 0);
}

public HandleMenuSidePick(id, menu, item)
{
    if (item == MENU_EXIT || g_MatchState != STATE_SIDE_PICK)
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new access, info[3], name[64], callback;
    menu_item_getinfo(menu, item, access, info, charsmax(info), name, charsmax(name), callback);
    menu_destroy(menu);

    if (info[0] == '1')
    {
        Cmd_SideChoice(id, 1);
    }
    else
    {
        Cmd_SideChoice(id, 2);
    }

    return PLUGIN_HANDLED;
}

public Cmd_SideChoice(id, choice) // 1 = stay, 2 = switch
{
    if (g_MatchState != STATE_SIDE_PICK)
        return;

    new CsTeams:team = (id > 0) ? safe_cs_get_user_team(id) : g_KnifeWinner;
    if (team != g_KnifeWinner && id != 0)
    {
        client_print(id, print_chat, "[CS:GO Match] Only the winning team can choose sides!");
        return;
    }

    remove_task(TASK_COMP_SIDEPICK);

    new chooser_name[32];
    if (id > 0) get_user_name(id, chooser_name, charsmax(chooser_name));
    else format(chooser_name, charsmax(chooser_name), "System");

    if (choice == 2)
    {
        // Switch sides
        client_print(0, print_chat, "^x04[CS:GO Match] %s chose to ^x04SWITCH SIDES^x01!", chooser_name);
        for (new i = 1; i <= MaxClients; i++)
        {
            if (!is_user_connected(i)) continue;
            new CsTeams:t = safe_cs_get_user_team(i);
            if (t == CS_TEAM_CT) SafeSetUserTeam(i, CS_TEAM_T);
            else if (t == CS_TEAM_T) SafeSetUserTeam(i, CS_TEAM_CT);
        }
    }
    else
    {
        client_print(0, print_chat, "^x04[CS:GO Match] %s chose to ^x04STAY^x01 on current side!", chooser_name);
    }

    StartLiveMatch();
}

public Task_SidePickCountdown()
{
    if (g_MatchState != STATE_SIDE_PICK)
    {
        remove_task(TASK_COMP_SIDEPICK);
        return;
    }

    if (g_SidePickTimer > 0)
    {
        client_print(0, print_center, "KNIFE WINNER CHOOSING SIDES^nRemaining: %d s (type .stay or .switch)", g_SidePickTimer);
        g_SidePickTimer--;
    }
    else
    {
        // Timeout -> default to stay
        remove_task(TASK_COMP_SIDEPICK);
        Cmd_SideChoice(0, 1);
    }
}

public FindTeamRepresentative(CsTeams:team)
{
    // Prefer human player
    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_connected(i) && !is_user_bot(i) && safe_cs_get_user_team(i) == team)
            return i;
    }
    // Otherwise bot
    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_connected(i) && safe_cs_get_user_team(i) == team)
            return i;
    }
    return 0;
}
