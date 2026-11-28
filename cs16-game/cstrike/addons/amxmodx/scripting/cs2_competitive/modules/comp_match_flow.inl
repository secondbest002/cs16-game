#if defined _comp_match_flow_included
    #endinput
#endif
#define _comp_match_flow_included

// ========================================================================
// LIVE MATCH (MR12 / MR15)
// ========================================================================
public StartLiveMatch()
{
    UpdateMatchFormat();

    remove_task(TASK_COMP_WARMUP);
    remove_task(TASK_COMP_SWEEP1);
    remove_task(TASK_COMP_SWEEP2);
    remove_task(TASK_COMP_COUNTDOWN);
    remove_task(TASK_COMP_SIDEPICK);
    remove_task(TASK_COMP_MATCH_END);

    ResetMatchTelemetry();

    g_MatchState = STATE_LIVE_HALF_1;
    g_RoundNumber = 0;
    set_pcvar_num(pCvar_CurrentRound, 1);
    g_ScoreCT = 0;
    g_ScoreT = 0;
    g_TimeoutsCT = 0;
    g_TimeoutsT = 0;
    g_TimeoutQueuedCT = false;
    g_TimeoutQueuedT = false;
    g_IsTimeoutActive = false;
    set_cvar_num("amx_csgo_is_timeout", 0);
    remove_task(TASK_COMP_TIMEOUT);
    g_LossStreakCT = 0;
    g_LossStreakT = 0;
    g_BombPlanted = false;

    set_cvar_num("amx_csgo_is_warmup", 0);

    // Restore bot buying & buytime
    ApplyYaPBState(STATE_LIVE_HALF_1);
    g_bKnifeRoundLive = false;
    set_cvar_num("bot_savage_kniferound", 0);
    new freeze = get_pcvar_num(pCvar_FreezeTime);
    if (freeze <= 0) freeze = 20;
    set_cvar_num("mp_freezetime", freeze);
    set_cvar_float("mp_roundtime", 1.92);
    set_cvar_float("mp_buytime", 0.25);
    set_cvar_num("mp_buy_anywhere", 0);
    set_cvar_string("mp_round_infinite", "0");
    set_cvar_num("mp_startmoney", 800);

    new pcvar_scavenge = get_cvar_pointer("bot_savage_scavenge");
    if (pcvar_scavenge) set_pcvar_num(pcvar_scavenge, 1);

    for (new i = 1; i <= MaxClients; i++)
    {
        remove_task(i + TASK_COMP_BOTEQUIP);
        if (is_user_connected(i))
        {
            cs_set_user_money(i, 800);
            remove_task(i + TASK_COMP_RESPAWN);
        }
    }

    set_cvar_num("sv_restart", 1);
    remove_task(TASK_COMP_SYNC_TELEMETRY);
    set_task(1.5, "Task_SyncTelemetryPostRestart", TASK_COMP_SYNC_TELEMETRY);

    // Visual Announcement (DHUD & Screen Flash)
    set_dhudmessage(255, 215, 0, -1.0, 0.28, 2, 0.1, 4.5, 0.1, 1.0);
    show_dhudmessage(0, "★ MATCH IS LIVE ★^nFormat: MR%d  |  First to %d WINS!", g_MaxRoundsHalf, g_TargetWinScore);

    if (get_pcvar_num(pCvar_ScreenFade) == 1)
    {
        Util_ScreenFade(0, 0.6, 0.2, FFADE_IN, 255, 215, 0, 40);
    }

    client_print(0, print_chat, "^x04==================================================");
    client_print(0, print_chat, "^x04[CS:GO Match] MATCH IS LIVE! Format: MR%d", g_MaxRoundsHalf);
    client_print(0, print_chat, "^x04First team to %d points WINS!", g_TargetWinScore);
    client_print(0, print_chat, "^x04==================================================");
}

// ========================================================================
// HALFTIME SWITCH
// ========================================================================
public TriggerHalftime()
{
    g_MatchState = STATE_HALFTIME;
    g_BombPlanted = false;

    // Swap scores
    new temp_score = g_ScoreCT;
    g_ScoreCT = g_ScoreT;
    g_ScoreT = temp_score;

    // Flip recorded round history sides (1 <-> 2)
    for (new r = 0; r < g_RoundHistCount; r++)
    {
        if (g_RoundHistWinner[r] == 1)      g_RoundHistWinner[r] = 2;
        else if (g_RoundHistWinner[r] == 2) g_RoundHistWinner[r] = 1;
    }
    BroadcastCS2RoundHistory(0);

    client_print(0, print_chat, "^x04==================================================");
    client_print(0, print_chat, "^x04[CS:GO Match] HALFTIME! Switching Sides (CT <-> T)");
    client_print(0, print_chat, "^x04Score Half 1: CT [%d] - [%d] T", temp_score, g_ScoreCT);
    client_print(0, print_chat, "^x04==================================================");

    // Visual Announcement & ScreenFade
    set_dhudmessage(255, 255, 100, -1.0, 0.30, 2, 0.1, 3.0, 0.1, 0.8);
    show_dhudmessage(0, "★ HALFTIME ★^nSWITCHING SIDES...^nScore: CT [%d] - [%d] T", temp_score, g_ScoreCT);

    if (get_pcvar_num(pCvar_ScreenFade) == 1)
    {
        Util_ScreenFade(0, 1.2, 1.5, FFADE_OUT, 0, 0, 0, 255);
    }

    // Swap players
    for (new i = 1; i <= MaxClients; i++)
    {
        if (!is_user_connected(i)) continue;

        new CsTeams:team = safe_cs_get_user_team(i);
        if (team == CS_TEAM_CT)
        {
            SafeSetUserTeam(i, CS_TEAM_T);
        }
        else if (team == CS_TEAM_T)
        {
            SafeSetUserTeam(i, CS_TEAM_CT);
        }

        cs_set_user_money(i, 800);
    }

    // Reset half timeouts & loss streaks
    g_TimeoutsCT = 0;
    g_TimeoutsT = 0;
    g_TimeoutQueuedCT = false;
    g_TimeoutQueuedT = false;
    g_IsTimeoutActive = false;
    set_cvar_num("amx_csgo_is_timeout", 0);
    remove_task(TASK_COMP_TIMEOUT);
    new freeze = get_pcvar_num(pCvar_FreezeTime);
    if (freeze <= 0) freeze = 20;
    set_cvar_num("mp_freezetime", freeze);
    g_LossStreakCT = 0;
    g_LossStreakT = 0;

    // Restart in 3 seconds to apply new team spawns
    remove_task(TASK_COMP_HALF2);
    set_task(3.0, "Task_RestartSecondHalf", TASK_COMP_HALF2);
}

public Task_RestartSecondHalf()
{
    g_MatchState = STATE_LIVE_HALF_2;
    set_pcvar_num(pCvar_CurrentRound, g_MaxRoundsHalf + 1);
    set_cvar_num("sv_restart", 1);
    remove_task(TASK_COMP_SYNC_TELEMETRY);
    set_task(1.5, "Task_SyncTelemetryPostRestart", TASK_COMP_SYNC_TELEMETRY);

    set_dhudmessage(100, 255, 120, -1.0, 0.28, 2, 0.1, 3.5, 0.1, 1.0);
    show_dhudmessage(0, "★ SECOND HALF IS LIVE ★^nGood luck, have fun!");

    client_print(0, print_chat, "^x04[CS:GO Match] SECOND HALF IS LIVE! Good luck!");
}

// ========================================================================
// OVERTIME (MR3)
// ========================================================================
public TriggerOvertime()
{
    g_MatchState = STATE_OVERTIME_1;
    g_BombPlanted = false;
    g_OTRoundNumber = 0;
    set_pcvar_num(pCvar_CurrentRound, (g_MaxRoundsHalf * 2) + 1);
    g_OTTargetWinScore = g_ScoreCT + 4; // Need 4 wins out of 6 OT rounds to win (e.g. 16 in MR12)

    g_TimeoutsCT = 0;
    g_TimeoutsT = 0;
    g_TimeoutQueuedCT = false;
    g_TimeoutQueuedT = false;
    g_IsTimeoutActive = false;
    set_cvar_num("amx_csgo_is_timeout", 0);
    remove_task(TASK_COMP_TIMEOUT);

    new ot_money = get_pcvar_num(pCvar_OTMoney);
    new freeze = get_pcvar_num(pCvar_FreezeTime);
    if (freeze <= 0) freeze = 20;
    set_cvar_num("mp_freezetime", freeze);
    set_cvar_float("mp_buytime", 0.45);
    set_cvar_num("mp_startmoney", ot_money);

    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_connected(i))
        {
            cs_set_user_money(i, ot_money);
        }
    }

    set_dhudmessage(255, 215, 0, -1.0, 0.28, 2, 0.1, 4.5, 0.1, 1.0);
    show_dhudmessage(0, "★ MATCH TIED (%d - %d) ★^nOVERTIME (MR3) STARTED!", g_ScoreCT, g_ScoreT);
    client_cmd(0, "spk buttons/bell1.wav");

    client_print(0, print_chat, "^x04==================================================");
    client_print(0, print_chat, "^x04[CS:GO Match] MATCH TIED (%d - %d)! OVERTIME (MR3)", g_ScoreCT, g_ScoreT);
    client_print(0, print_chat, "^x04Starting OT Half 1. Start Money: $%d", ot_money);
    client_print(0, print_chat, "^x04==================================================");

    set_cvar_num("sv_restart", 1);
    remove_task(TASK_COMP_SYNC_TELEMETRY);
    set_task(1.5, "Task_SyncTelemetryPostRestart", TASK_COMP_SYNC_TELEMETRY);
}

public TriggerOTHalftime()
{
    g_MatchState = STATE_OVERTIME_HALFTIME;

    // Swap scores
    new temp_score = g_ScoreCT;
    g_ScoreCT = g_ScoreT;
    g_ScoreT = temp_score;

    // Flip recorded round history sides (1 <-> 2)
    for (new r = 0; r < g_RoundHistCount; r++)
    {
        if (g_RoundHistWinner[r] == 1)      g_RoundHistWinner[r] = 2;
        else if (g_RoundHistWinner[r] == 2) g_RoundHistWinner[r] = 1;
    }
    BroadcastCS2RoundHistory(0);

    set_dhudmessage(255, 255, 100, -1.0, 0.30, 2, 0.1, 3.0, 0.1, 0.8);
    show_dhudmessage(0, "★ OVERTIME HALFTIME ★^nSWITCHING SIDES...");

    if (get_pcvar_num(pCvar_ScreenFade) == 1)
    {
        Util_ScreenFade(0, 1.0, 1.5, FFADE_OUT, 0, 0, 0, 255);
    }

    // Swap players
    for (new i = 1; i <= MaxClients; i++)
    {
        if (!is_user_connected(i)) continue;

        new CsTeams:team = safe_cs_get_user_team(i);
        if (team == CS_TEAM_CT)
            SafeSetUserTeam(i, CS_TEAM_T);
        else if (team == CS_TEAM_T)
            SafeSetUserTeam(i, CS_TEAM_CT);

        cs_set_user_money(i, get_pcvar_num(pCvar_OTMoney));
    }

    client_print(0, print_chat, "^x04[CS:GO Match] OVERTIME HALFTIME! Switching Sides...");
    remove_task(TASK_COMP_OT2);
    set_task(3.0, "Task_RestartOTSecondHalf", TASK_COMP_OT2);
}

public Task_RestartOTSecondHalf()
{
    g_MatchState = STATE_OVERTIME_2;
    set_cvar_num("sv_restart", 1);
    remove_task(TASK_COMP_SYNC_TELEMETRY);
    set_task(1.5, "Task_SyncTelemetryPostRestart", TASK_COMP_SYNC_TELEMETRY);
    set_dhudmessage(255, 215, 0, -1.0, 0.28, 2, 0.1, 3.5, 0.1, 1.0);
    show_dhudmessage(0, "★ OVERTIME SECOND HALF IS LIVE ★");
    client_print(0, print_chat, "^x04[CS:GO Match] OVERTIME SECOND HALF IS LIVE!");
}

// ========================================================================
// TACTICAL TIMEOUT
// ========================================================================
public Cmd_Timeout(id)
{
    if (g_MatchState != STATE_LIVE_HALF_1 && g_MatchState != STATE_LIVE_HALF_2 &&
        g_MatchState != STATE_OVERTIME_1 && g_MatchState != STATE_OVERTIME_2)
    {
        client_print(id, print_chat, "[CS:GO Match] Timeouts can only be called during live rounds!");
        return;
    }

    new CsTeams:team = safe_cs_get_user_team(id);
    new max_timeouts = get_pcvar_num(pCvar_Timeouts);

    if (team == CS_TEAM_CT)
    {
        if (g_TimeoutsCT >= max_timeouts)
        {
            client_print(id, print_chat, "[CS:GO Match] Team CT has used all tactical timeouts for this half!");
            return;
        }
        if (g_TimeoutQueuedCT)
        {
            client_print(id, print_chat, "[CS:GO Match] Timeout is already queued for Team CT!");
            return;
        }
        if (g_TimeoutQueuedT)
        {
            client_print(id, print_chat, "[CS:GO Match] Team T has already queued a timeout for next round! Call again next round.");
            return;
        }
        g_TimeoutQueuedCT = true;
        g_TimeoutsCT++;
        client_print(0, print_chat, "^x04[CS:GO Match]^x01 Team ^x04CT^x01 called a Tactical Timeout! It will start in Freeze Time next round.");
    }
    else if (team == CS_TEAM_T)
    {
        if (g_TimeoutsT >= max_timeouts)
        {
            client_print(id, print_chat, "[CS:GO Match] Team T has used all tactical timeouts for this half!");
            return;
        }
        if (g_TimeoutQueuedT)
        {
            client_print(id, print_chat, "[CS:GO Match] Timeout is already queued for Team T!");
            return;
        }
        if (g_TimeoutQueuedCT)
        {
            client_print(id, print_chat, "[CS:GO Match] Team CT has already queued a timeout for next round! Call again next round.");
            return;
        }
        g_TimeoutQueuedT = true;
        g_TimeoutsT++;
        client_print(0, print_chat, "^x03[CS:GO Match]^x01 Team ^x03TERRORIST^x01 called a Tactical Timeout! It will start in Freeze Time next round.");
    }
}

public StartTimeout(CsTeams:team)
{
    g_IsTimeoutActive = true;
    g_TimeoutTeam = team;
    g_TimeoutRemaining = get_pcvar_num(pCvar_TimeoutDuration);
    if (g_TimeoutRemaining <= 0) g_TimeoutRemaining = 60;

    set_cvar_num("amx_csgo_is_timeout", 1);

    new team_name[16];
    format(team_name, charsmax(team_name), (team == CS_TEAM_CT) ? "Counter-Terrorist" : "Terrorist");
    client_print(0, print_chat, "^x04[CS:GO Match] TACTICAL TIMEOUT (%s) STARTED! (%d seconds in Freeze Time)", team_name, g_TimeoutRemaining);

    set_dhudmessage(255, 80, 80, -1.0, 0.28, 2, 0.1, 4.0, 0.1, 1.0);
    show_dhudmessage(0, "⏸ TACTICAL TIMEOUT ⏸^nTeam: %s^nDuration: %d seconds", team_name, g_TimeoutRemaining);

    if (get_pcvar_num(pCvar_ScreenFade) == 1)
    {
        Util_ScreenFade(0, 0.5, 0.5, FFADE_IN, 30, 30, 30, 140);
    }

    remove_task(TASK_COMP_TIMEOUT);
    set_task(1.0, "Task_TimeoutCountdown", TASK_COMP_TIMEOUT, _, _, "b");
}

public Task_TimeoutCountdown()
{
    if (!g_IsTimeoutActive)
    {
        remove_task(TASK_COMP_TIMEOUT);
        return;
    }

    if (g_TimeoutRemaining > 0)
    {
        if (g_TimeoutRemaining <= 3)
        {
            client_cmd(0, "spk buttons/blip1.wav");
        }
        g_TimeoutRemaining--;
    }
    else
    {
        // Timeout ended
        g_IsTimeoutActive = false;
        set_cvar_num("amx_csgo_is_timeout", 0);
        remove_task(TASK_COMP_TIMEOUT);

        client_cmd(0, "spk buttons/bell1.wav");
        client_print(0, print_chat, "^x04[CS:GO Match] Tactical Timeout ended! Live round commencing.");
    }
}

public FreezeAllPlayers(bool:freeze)
{
    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_alive(i))
        {
            new flags = pev(i, pev_flags);
            if (freeze)
            {
                set_pev(i, pev_flags, flags | FL_FROZEN);
            }
            else
            {
                set_pev(i, pev_flags, flags & ~FL_FROZEN);
            }
        }
    }
}
