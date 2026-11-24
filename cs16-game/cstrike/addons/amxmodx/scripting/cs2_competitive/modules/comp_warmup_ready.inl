#if defined _comp_warmup_ready_included
    #endinput
#endif
#define _comp_warmup_ready_included

public BlockBuyCmd(id)
{
    CHECK_COMPETITIVE_ACTIVE(PLUGIN_CONTINUE);

    if (g_MatchState == STATE_KNIFE_ROUND)
    {
        if (!is_user_bot(id))
        {
            client_print(id, print_center, "Buying is disabled during Knife Round!");
        }
        return PLUGIN_HANDLED;
    }
    else if (g_MatchState == STATE_WARMUP)
    {
        new szCmd[32];
        read_argv(0, szCmd, charsmax(szCmd));
        if (equal(szCmd, "flash") || equal(szCmd, "hegren") || equal(szCmd, "sgren") ||
            equal(szCmd, "buy_hegrenade") || equal(szCmd, "buy_flashbang") || equal(szCmd, "buy_smokegrenade"))
        {
            if (!is_user_bot(id))
            {
                client_print(id, print_center, "[Warmup DM] Grenades are disabled during Warmup!");
            }
            return PLUGIN_HANDLED;
        }
    }
    return PLUGIN_CONTINUE;
}

public BlockDropCmd(id)
{
    CHECK_COMPETITIVE_ACTIVE(PLUGIN_CONTINUE);

    if (g_MatchState == STATE_KNIFE_ROUND)
    {
        return PLUGIN_HANDLED;
    }
    return PLUGIN_CONTINUE;
}

// Low-level engine buy attempt interception (cstrike module)
public CS_OnBuyAttempt(id, item)
{
    CHECK_COMPETITIVE_ACTIVE(PLUGIN_CONTINUE);

    if (g_MatchState == STATE_KNIFE_ROUND)
    {
        return PLUGIN_HANDLED;
    }
    return PLUGIN_CONTINUE;
}

// Low-level purchase execution interception
public CS_OnBuy(id, item)
{
    if (g_MatchState == STATE_KNIFE_ROUND)
    {
        return PLUGIN_HANDLED;
    }
    return PLUGIN_CONTINUE;
}

// Block internal buy commands sent to bots or auto-buy
public CS_InternalCommand(id, const cmd[])
{
    if (g_MatchState == STATE_KNIFE_ROUND)
    {
        return PLUGIN_HANDLED;
    }
    return PLUGIN_CONTINUE;
}

// Continuous check: if any player or bot equips anything other than a knife during knife round, strip it instantly!
// And during warmup DM, immediately strip C4 if player somehow equips it
public Event_CurWeapon(id)
{
    if (g_MatchState == STATE_WARMUP && is_user_alive(id))
    {
        new weapon = read_data(2);
        if (weapon == CSW_C4)
        {
            StripUserC4(id);
        }
    }
    else if (g_MatchState == STATE_KNIFE_ROUND && is_user_alive(id))
    {
        new weapon = read_data(2);
        if (weapon != CSW_KNIFE)
        {
            strip_user_weapons(id);
            give_item(id, "weapon_knife");
            cs_set_user_armor(id, 100, CS_ARMOR_VESTHELM);
            cs_set_user_money(id, 0);
        }
    }
}

// Intercept adding C4 to any player/bot during warmup DM and destroy it
public OnC4AddToPlayer(ent, id)
{
    if (g_MatchState == STATE_WARMUP)
    {
        ExecuteHamB(Ham_Item_Kill, ent);
        return HAM_SUPERCEDE;
    }
    return HAM_IGNORED;
}

// Prevent picking up dropped weapons or C4 during warmup (destroying weaponbox) or any weapons during knife round
public OnWeaponTouchPre(entity, id)
{
    if (g_MatchState == STATE_WARMUP)
    {
        static classname[32];
        pev(entity, pev_classname, classname, charsmax(classname));
        if (equal(classname, "weaponbox"))
        {
            xash_destroy_weaponbox(entity);
            return HAM_SUPERCEDE;
        }
        else if (equal(classname, "cs2_dropped_rifle"))
        {
            set_pev(entity, pev_flags, pev(entity, pev_flags) | FL_KILLME);
            return HAM_SUPERCEDE;
        }
    }
    else if (g_MatchState == STATE_KNIFE_ROUND)
    {
        if (1 <= id <= MaxClients)
        {
            return HAM_SUPERCEDE;
        }
    }
    return HAM_IGNORED;
}

// Safely strip C4 weapon from a player and disable planting capability
stock StripUserC4(id)
{
    if (!is_user_alive(id))
        return;

    cs_set_user_plant(id, 0, 0);

    new ent = -1;
    while ((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", "weapon_c4")) > 0)
    {
        if (pev(ent, pev_owner) == id)
        {
            ExecuteHamB(Ham_Item_Kill, ent);
            set_pev(id, pev_weapons, pev(id, pev_weapons) & ~(1 << CSW_C4));
            break;
        }
    }
}

// Purge all dropped weapons, C4, and custom rifles on the entire map during warmup
stock CleanWarmupWeapons()
{
    new ent = -1;
    while ((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", "weapon_c4")) > 0)
    {
        ExecuteHamB(Ham_Item_Kill, ent);
    }
    ent = -1;
    while ((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", "weaponbox")) > 0)
    {
        xash_destroy_weaponbox(ent);
    }
    ent = -1;
    while ((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", "cs2_dropped_rifle")) > 0)
    {
        set_pev(ent, pev_flags, pev(ent, pev_flags) | FL_KILLME);
    }
    ent = -1;
    while ((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", "weapon_hegrenade")) > 0)
    {
        set_pev(ent, pev_flags, pev(ent, pev_flags) | FL_KILLME);
    }
    ent = -1;
    while ((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", "weapon_flashbang")) > 0)
    {
        set_pev(ent, pev_flags, pev(ent, pev_flags) | FL_KILLME);
    }
    ent = -1;
    while ((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", "weapon_smokegrenade")) > 0)
    {
        set_pev(ent, pev_flags, pev(ent, pev_flags) | FL_KILLME);
    }
}

// Helper to configure YaPB & PodBot knife-only behavior dynamically
stock SetBotKnifeOnly(bool:knife_only)
{
    new pcvar_yb_jason = get_cvar_pointer("yb_jasonmode");
    if (pcvar_yb_jason)
    {
        set_pcvar_num(pcvar_yb_jason, knife_only ? 1 : 0);
    }

    new pcvar_yb_botbuy = get_cvar_pointer("yb_botbuy");
    if (pcvar_yb_botbuy)
    {
        set_pcvar_num(pcvar_yb_botbuy, knife_only ? 0 : 1);
    }

    new pcvar_pb_botbuy = get_cvar_pointer("pb_botbuy");
    if (pcvar_pb_botbuy)
    {
        set_pcvar_num(pcvar_pb_botbuy, knife_only ? 0 : 1);
    }
}

// Centralized YaPB & ReGameDLL cvar synchronization across Match State transitions
stock ApplyYaPBState(iState)
{
    set_cvar_num("yb_think_fps_disable", 0);

    if (iState == STATE_WARMUP)
    {
        SetBotKnifeOnly(false);
        set_cvar_num("yb_botbuy", 0);
        set_cvar_num("yb_csdm_mode", 1);
        set_cvar_num("yb_ignore_objectives", 1);
        set_cvar_num("yb_radio_mode", 0);
        set_cvar_num("yb_chat", 0);
        set_cvar_float("yb_think_fps", 40.0);
        set_cvar_num("mp_item_staytime", 0);
    }
    else if (iState == STATE_KNIFE_ROUND)
    {
        SetBotKnifeOnly(true);
        set_cvar_num("yb_csdm_mode", 3);
        set_cvar_num("yb_ignore_objectives", 0);
        set_cvar_num("yb_radio_mode", 0);
        set_cvar_num("yb_chat", 0);
        set_cvar_float("yb_think_fps", 60.0);
        set_cvar_num("mp_item_staytime", 300);
    }
    else
    {
        SetBotKnifeOnly(false);
        set_cvar_num("yb_csdm_mode", 3);
        set_cvar_num("yb_ignore_objectives", 0);
        set_cvar_num("yb_radio_mode", 2);
        set_cvar_num("yb_chat", 1);
        set_cvar_float("yb_think_fps", 60.0);
        set_cvar_num("mp_item_staytime", 300);
    }
}

// ========================================================================
// SAFE TEAM CHANGER STOCK (Immune to cstrike_amxx sv crash on Build 9947 & bots)
// ========================================================================
stock SafeSetUserTeam(id, CsTeams:team)
{
    if (!is_user_connected(id))
        return;

    // Directly set CBasePlayer m_iTeam offset via fakemeta (prevents cstrike_amxx.dll access violation)
    set_pdata_int(id, OFFSET_TEAM, _:team, EXTRAOFFSET_PLAYER);

    // Dispatch TeamInfo network message so scoreboard, HUD, and radar sync cleanly
    static msgTeamInfo = 0;
    if (!msgTeamInfo)
        msgTeamInfo = get_user_msgid("TeamInfo");

    new team_str[16];
    if (team == CS_TEAM_T)
        copy(team_str, charsmax(team_str), "TERRORIST");
    else if (team == CS_TEAM_CT)
        copy(team_str, charsmax(team_str), "CT");
    else if (team == CS_TEAM_SPECTATOR)
        copy(team_str, charsmax(team_str), "SPECTATOR");
    else
        copy(team_str, charsmax(team_str), "UNASSIGNED");

    message_begin(MSG_ALL, msgTeamInfo);
    write_byte(id);
    write_string(team_str);
    message_end();
}

// ========================================================================
// ENGINE SCREEN FADE STOCK
// ========================================================================

stock Util_ScreenFade(id, Float:duration, Float:holdtime, flags, r, g, b, alpha, bool:reliable = false)
{
    static msgScreenFade = 0;
    if (!msgScreenFade)
    {
        msgScreenFade = get_user_msgid("ScreenFade");
    }

    new dur = floatround(duration * 4096.0);
    new hold = floatround(holdtime * 4096.0);

    if (id)
    {
        if (!is_user_connected(id)) return;
        message_begin(reliable ? MSG_ONE : MSG_ONE_UNRELIABLE, msgScreenFade, _, id);
    }
    else
    {
        message_begin(reliable ? MSG_ALL : MSG_BROADCAST, msgScreenFade);
    }

    write_short(dur);
    write_short(hold);
    write_short(flags);
    write_byte(r);
    write_byte(g);
    write_byte(b);
    write_byte(alpha);
    message_end();
}

// ========================================================================
// READY UP & WARMUP SYSTEM
// ========================================================================
public Cmd_Ready(id)
{
    CHECK_COMPETITIVE_ACTIVE(PLUGIN_CONTINUE);

    if (g_MatchState != STATE_WARMUP && g_MatchState != STATE_COUNTDOWN)
    {
        client_print(id, print_chat, "[CS:GO Match] Match is already in progress!");
        return PLUGIN_HANDLED;
    }

    new CsTeams:team = safe_cs_get_user_team(id);
    if (team != CS_TEAM_CT && team != CS_TEAM_T)
    {
        client_print(id, print_chat, "[CS:GO Match] Please join CT or T before typing .ready!");
        return PLUGIN_HANDLED;
    }

    if (g_PlayerReady[id])
    {
        // If player is already ready, typing ready again rescans and auto-readies any bots
        // that joined or spawned after the player first readied!
        if (get_pcvar_num(pCvar_BotAutoReady) == 1)
        {
            new readied = AutoReadyBots();
            if (readied > 0)
            {
                client_print(0, print_chat, "^x04[CS:GO Match]^x01 Auto-readied %d bot(s) that joined recently!", readied);
            }
            CheckReadyStatus();
        }
        client_print(id, print_chat, "[CS:GO Match] You are already READY!");
        return PLUGIN_HANDLED;
    }

    g_PlayerReady[id] = true;

    new name[32];
    get_user_name(id, name, charsmax(name));
    client_print(0, print_chat, "^x04[CS:GO Match]^x01 %s is now ^x04READY^x01!", name);

    // Auto-ready YaPB bots if enabled
    if (get_pcvar_num(pCvar_BotAutoReady) == 1)
    {
        AutoReadyBots();
    }

    CheckReadyStatus();
    return PLUGIN_HANDLED;
}

public Cmd_Unready(id)
{
    CHECK_COMPETITIVE_ACTIVE(PLUGIN_CONTINUE);

    if (g_MatchState != STATE_WARMUP && g_MatchState != STATE_COUNTDOWN)
        return PLUGIN_HANDLED;

    if (!g_PlayerReady[id])
    {
        client_print(id, print_chat, "[CS:GO Match] You are not ready!");
        return PLUGIN_HANDLED;
    }

    g_PlayerReady[id] = false;

    if (g_MatchState == STATE_COUNTDOWN)
    {
        g_MatchState = STATE_WARMUP;
        remove_task(TASK_COMP_COUNTDOWN);
        client_print(0, print_chat, "^x03[CS:GO Match]^x01 Countdown cancelled because a player unreadied!");
    }

    new name[32];
    get_user_name(id, name, charsmax(name));
    client_print(0, print_chat, "^x03[CS:GO Match]^x01 %s is now ^x03NOT READY^x01.", name);
    return PLUGIN_HANDLED;
}

public CheckReadyStatus()
{
    new total_players = 0;
    new ready_players = 0;
    new ct_count = 0;
    new t_count = 0;

    for (new i = 1; i <= MaxClients; i++)
    {
        if (!is_user_connected(i)) continue;

        new CsTeams:team = safe_cs_get_user_team(i);
        if (team == CS_TEAM_CT)
        {
            total_players++;
            ct_count++;
            if (g_PlayerReady[i]) ready_players++;
        }
        else if (team == CS_TEAM_T)
        {
            total_players++;
            t_count++;
            if (g_PlayerReady[i]) ready_players++;
        }
    }

    if (total_players > 0 && ct_count > 0 && t_count > 0 && ready_players == total_players)
    {
        if (g_MatchState != STATE_COUNTDOWN)
        {
            // Cancel any existing countdown task first
            remove_task(TASK_COMP_COUNTDOWN);

            g_MatchState = STATE_COUNTDOWN;
            g_CountdownTimer = 5;
            // Run as a repeating loop until cancelled or expired
            set_task(1.0, "Task_MatchCountdown", TASK_COMP_COUNTDOWN, _, _, "b");
            client_print(0, print_chat, "^x04[CS:GO Match]^x01 All players READY (%d/%d)! Starting in 5 seconds...", ready_players, total_players);
        }
    }
}

public Task_MatchCountdown()
{
    if (g_MatchState != STATE_COUNTDOWN)
    {
        remove_task(TASK_COMP_COUNTDOWN);
        return;
    }

    if (g_CountdownTimer > 0)
    {
        set_dhudmessage(100, 255, 120, -1.0, 0.32, 0, 0.0, 0.95, 0.05, 0.05);
        show_dhudmessage(0, "★ ALL PLAYERS READY ★^nMatch starts in: %d", g_CountdownTimer);

        new sound[32];
        format(sound, charsmax(sound), "buttons/blip%d.wav", (g_CountdownTimer <= 3) ? 2 : 1);
        client_cmd(0, "spk %s", sound);
        g_CountdownTimer--;
    }
    else
    {
        // Countdown finished! Kill loop task and launch match
        remove_task(TASK_COMP_COUNTDOWN);
        client_cmd(0, "spk buttons/bell1.wav");

        if (get_pcvar_num(pCvar_KnifeRound) == 1)
        {
            StartKnifeRound();
        }
        else
        {
            StartLiveMatch();
        }
    }
}
