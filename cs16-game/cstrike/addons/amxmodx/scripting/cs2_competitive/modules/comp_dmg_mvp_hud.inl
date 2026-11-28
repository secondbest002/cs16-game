#if defined _comp_dmg_mvp_hud_included
    #endinput
#endif
#define _comp_dmg_mvp_hud_included

// ========================================================================
// DAMAGE REPORT & MVP
// ========================================================================
public PrintRoundDamageSummary()
{
    for (new id = 1; id <= MaxClients; id++)
    {
        if (!is_user_connected(id) || is_user_bot(id))
            continue;

        new bool:has_damage = false;
        client_print(id, print_chat, "^x04----------------- ROUND DAMAGE REPORT -----------------");

        new card_buf[256];
        new card_len = formatex(card_buf, charsmax(card_buf), "── COMBAT REPORT ──^n");

        for (new v = 1; v <= MaxClients; v++)
        {
            if (g_DamageDealt[id][v] > 0)
            {
                has_damage = true;
                new vname[32];
                get_user_name(v, vname, charsmax(vname));
                client_print(id, print_chat, "^x01Damage Given to ^"^x03%s^x01^" -> ^x04%d^x01 in ^x04%d^x01 hit%s%s",
                    vname, g_DamageDealt[id][v], g_HitsDealt[id][v],
                    (g_HitsDealt[id][v] > 1) ? "s" : "",
                    (!is_user_alive(v)) ? " (KILLED)" : "");

                if (card_len < (charsmax(card_buf) - 50))
                {
                    card_len += formatex(card_buf[card_len], charsmax(card_buf) - card_len,
                        "to %s: %d dmg (%d hit%s)%s^n",
                        vname, g_DamageDealt[id][v], g_HitsDealt[id][v],
                        (g_HitsDealt[id][v] > 1) ? "s" : "",
                        (!is_user_alive(v)) ? " [KILLED]" : "");
                }
            }
        }

        if (!has_damage)
        {
            client_print(id, print_chat, "^x01No damage dealt this round.");
            copy(card_buf, charsmax(card_buf), "── COMBAT REPORT ──^nNo damage dealt this round.");
        }
        client_print(id, print_chat, "^x04-------------------------------------------------------");

        if (get_pcvar_num(pCvar_DmgCard) == 1)
        {
            set_hudmessage(200, 220, 255, 0.68, 0.32, 0, 0.0, 6.0, 0.1, 0.5, -1);
            ShowSyncHudMsg(id, g_HudSyncDamage, "%s", card_buf);
        }
    }
}

public Cmd_Damage(id)
{
    new bool:has_damage = false;
    client_print(id, print_chat, "^x04----------------- DAMAGE SUMMARY -----------------");

    new card_buf[256];
    new card_len = formatex(card_buf, charsmax(card_buf), "── COMBAT REPORT ──^n");

    for (new v = 1; v <= MaxClients; v++)
    {
        if (g_DamageDealt[id][v] > 0)
        {
            has_damage = true;
            new vname[32];
            get_user_name(v, vname, charsmax(vname));
            client_print(id, print_chat, "^x01To ^"^x03%s^x01^" -> ^x04%d^x01 dmg (%d hit%s)",
                vname, g_DamageDealt[id][v], g_HitsDealt[id][v],
                (g_HitsDealt[id][v] > 1) ? "s" : "");

            if (card_len < (charsmax(card_buf) - 50))
            {
                card_len += formatex(card_buf[card_len], charsmax(card_buf) - card_len,
                    "to %s: %d dmg (%d hit%s)%s^n",
                    vname, g_DamageDealt[id][v], g_HitsDealt[id][v],
                    (g_HitsDealt[id][v] > 1) ? "s" : "",
                    (!is_user_alive(v)) ? " [KILLED]" : "");
            }
        }
    }

    if (!has_damage)
    {
        client_print(id, print_chat, "^x01No damage recorded yet in this round.");
        copy(card_buf, charsmax(card_buf), "── COMBAT REPORT ──^nNo damage recorded yet.");
    }
    client_print(id, print_chat, "^x04-------------------------------------------------");

    if (get_pcvar_num(pCvar_DmgCard) == 1)
    {
        set_hudmessage(200, 220, 255, 0.68, 0.32, 0, 0.0, 5.0, 0.1, 0.5, -1);
        ShowSyncHudMsg(id, g_HudSyncDamage, "%s", card_buf);
    }
}

public AnnounceMVP()
{
    new best_player = 0;
    new max_kills = 0;

    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_connected(i) && g_KillsInRound[i] > max_kills)
        {
            max_kills = g_KillsInRound[i];
            best_player = i;
        }
    }

    new reason[64];
    if (best_player > 0 && max_kills >= 2)
    {
        format(reason, charsmax(reason), "eliminating %d enemies", max_kills);
    }
    else if (g_BombDefuser > 0)
    {
        best_player = g_BombDefuser;
        format(reason, charsmax(reason), "defusing the bomb");
    }
    else if (g_BombPlanter > 0)
    {
        best_player = g_BombPlanter;
        format(reason, charsmax(reason), "planting the bomb");
    }

    if (best_player > 0)
    {
        if (IsLiveMatchState())
        {
            g_PlayerMVPs[best_player]++;
        }

        new mvp_name[32];
        get_user_name(best_player, mvp_name, charsmax(mvp_name));
        client_print(0, print_chat, "^x04★ MVP: %s for %s! ★", mvp_name, reason);

        if (get_pcvar_num(pCvar_MvpCard) == 1)
        {
            set_dhudmessage(255, 215, 0, -1.0, 0.24, 2, 0.1, 4.0, 0.1, 0.8);
            show_dhudmessage(0, "★ ★ ★   ROUND MVP   ★ ★ ★^n%s^n[ %s ]", mvp_name, reason);
            if (get_pcvar_num(pCvar_ScreenFade) == 1)
            {
                Util_ScreenFade(0, 0.4, 0.2, FFADE_IN, 255, 215, 0, 35);
            }
        }
    }
}

public Cmd_Score(id)
{
    client_print(id, print_chat, "^x04[CS:GO Match] Current Score: CT [%d] - [%d] T | Round %d/%d",
        g_ScoreCT, g_ScoreT, g_RoundNumber, g_MaxRoundsHalf * 2);

    new score_str[128];
    formatex(score_str, charsmax(score_str), "SCORE: CT [ %d ] — [ %d ] T^nRound %d of %d",
        g_ScoreCT, g_ScoreT, g_RoundNumber, g_MaxRoundsHalf * 2);
    set_hudmessage(255, 215, 0, -1.0, 0.08, 0, 0.0, 3.0, 0.1, 0.5, -1);
    ShowSyncHudMsg(id, g_HudSyncSubHeader, "%s", score_str);
}

// ========================================================================
// HUD DISPLAY TASK
// ========================================================================
public Task_UpdateHUD()
{
    CHECK_COMPETITIVE_ACTIVE();

    new hud_msg[128];

    switch (g_MatchState)
    {
        case STATE_WARMUP:
        {
            new total = 0, ready = 0;
            for (new i = 1; i <= MaxClients; i++)
            {
                if (is_user_connected(i))
                {
                    new CsTeams:team = safe_cs_get_user_team(i);
                    if (team == CS_TEAM_CT || team == CS_TEAM_T)
                    {
                        total++;
                        if (g_PlayerReady[i]) ready++;
                    }
                }
            }

            new mins = g_WarmupTimeLeft / 60;
            new secs = g_WarmupTimeLeft % 60;
            formatex(hud_msg, charsmax(hud_msg), "★ CS:GO WARMUP LOBBY [ %02d:%02d ] ★^nReady: [ %d / %d ]  —  Type .ready in chat", mins, secs, ready, total);
            set_hudmessage(220, 220, 220, -1.0, 0.02, 0, 0.0, 1.1, 0.1, 0.1, -1);
            ShowSyncHudMsg(0, g_HudSyncHeader, "%s", hud_msg);
        }
        case STATE_COUNTDOWN:
        {
            new display_time = (g_CountdownTimer > 0) ? g_CountdownTimer : 1;
            formatex(hud_msg, charsmax(hud_msg), "★ MATCH STARTING IN %d SECONDS ★^nPrepare for combat!", display_time);
            set_hudmessage(100, 255, 120, -1.0, 0.02, 0, 0.0, 1.1, 0.1, 0.1, -1);
            ShowSyncHudMsg(0, g_HudSyncHeader, "%s", hud_msg);
        }
        case STATE_KNIFE_ROUND:
        {
            formatex(hud_msg, charsmax(hud_msg), "⚔ [ KNIFE ROUND ]   FIGHT FOR SIDE CHOICE ⚔^n(Knives Only)");
            set_hudmessage(50, 255, 140, -1.0, 0.02, 0, 0.0, 1.1, 0.1, 0.1, -1);
            ShowSyncHudMsg(0, g_HudSyncHeader, "%s", hud_msg);
        }
        case STATE_SIDE_PICK:
        {
            new wteam[24];
            format(wteam, charsmax(wteam), (g_KnifeWinner == CS_TEAM_CT) ? "COUNTER-TERRORISTS" : "TERRORISTS");
            formatex(hud_msg, charsmax(hud_msg), "★ %s WON KNIFE ROUND ★^nChoosing Side... [ .stay / .switch ] (%d s)", wteam, g_SidePickTimer);
            set_hudmessage(100, 200, 255, -1.0, 0.02, 0, 0.0, 1.1, 0.1, 0.1, -1);
            ShowSyncHudMsg(0, g_HudSyncHeader, "%s", hud_msg);
        }
        case STATE_LIVE_HALF_1, STATE_LIVE_HALF_2:
        {
            if (g_IsTimeoutActive)
            {
                new tname[16];
                format(tname, charsmax(tname), (g_TimeoutTeam == CS_TEAM_CT) ? "CT" : "T");
                formatex(hud_msg, charsmax(hud_msg), "⏸ [ TACTICAL TIMEOUT: %s ] ⏸^nResuming in: 00:%02d", tname, g_TimeoutRemaining);
                set_hudmessage(255, 80, 80, -1.0, 0.02, 0, 0.0, 1.1, 0.1, 0.1, -1);
                ShowSyncHudMsg(0, g_HudSyncHeader, "%s", hud_msg);
            }
            else
            {
                if (g_BombPlanted && get_pcvar_num(pCvar_C4Timer) == 1)
                {
                    new Float:elapsed = get_gametime() - g_BombPlantedTime;
                    if (elapsed > (float(g_C4TimerCvar) + 1.0))
                    {
                        // Bomb has already detonated/expired - auto clear bomb state immediately
                        g_BombPlanted = false;
                    }
                }

                if (g_BombPlanted && get_pcvar_num(pCvar_C4Timer) == 1)
                {
                    new Float:elapsed = get_gametime() - g_BombPlantedTime;
                    new c4_left = g_C4TimerCvar - floatround(elapsed);
                    if (c4_left < 0) c4_left = 0;

                    if (c4_left <= 10)
                    {
                        static bool:flash_toggle = false;
                        flash_toggle = !flash_toggle;
                        if (flash_toggle)
                        {
                            formatex(hud_msg, charsmax(hud_msg), "[ CT : %02d ]   💣 C4: %02d s [CRITICAL]   [ %02d : T ]",
                                g_ScoreCT, c4_left, g_ScoreT);
                            set_hudmessage(255, 30, 30, -1.0, 0.02, 0, 0.0, 1.1, 0.05, 0.05, -1);
                        }
                        else
                        {
                            formatex(hud_msg, charsmax(hud_msg), "[ CT : %02d ]   💣 [ C4: %02d s ]   [ %02d : T ]",
                                g_ScoreCT, c4_left, g_ScoreT);
                            set_hudmessage(255, 140, 40, -1.0, 0.02, 0, 0.0, 1.1, 0.05, 0.05, -1);
                        }

                        if (c4_left <= 5 && c4_left > 0)
                        {
                            client_cmd(0, "spk buttons/blip1.wav");
                        }
                    }
                    else
                    {
                        formatex(hud_msg, charsmax(hud_msg), "[ CT : %02d ]   💣 C4: %02d s   [ %02d : T ]",
                            g_ScoreCT, c4_left, g_ScoreT);
                        set_hudmessage(255, 180, 50, -1.0, 0.02, 0, 0.0, 1.1, 0.1, 0.1, -1);
                    }
                    ShowSyncHudMsg(0, g_HudSyncHeader, "%s", hud_msg);
                }
                else
                {
                    new match_point[32];
                    match_point[0] = 0;
                    new r = 240, g = 240, b = 240;

                    if (g_ScoreCT == (g_TargetWinScore - 1))
                    {
                        copy(match_point, charsmax(match_point), "^n▲ MATCH POINT (CT) ▲");
                        r = 255; g = 215; b = 0;
                    }
                    else if (g_ScoreT == (g_TargetWinScore - 1))
                    {
                        copy(match_point, charsmax(match_point), "^n▲ MATCH POINT (T) ▲");
                        r = 255; g = 215; b = 0;
                    }

                    if (get_pcvar_num(pCvar_HudStyle) == 1)
                    {
                        formatex(hud_msg, charsmax(hud_msg), "[ CT : %02d ]   —   ROUND %02d/%02d   —   [ %02d : T ]%s",
                            g_ScoreCT, g_RoundNumber, g_MaxRoundsHalf * 2, g_ScoreT, match_point);
                    }
                    else
                    {
                        formatex(hud_msg, charsmax(hud_msg), "CT: %d  |  T: %d  (Round %d/%d)%s",
                            g_ScoreCT, g_ScoreT, g_RoundNumber, g_MaxRoundsHalf * 2, match_point);
                    }

                    set_hudmessage(r, g, b, -1.0, 0.02, 0, 0.0, 1.1, 0.1, 0.1, -1);
                    ShowSyncHudMsg(0, g_HudSyncHeader, "%s", hud_msg);
                }
            }
        }
        case STATE_OVERTIME_1, STATE_OVERTIME_2:
        {
            if (g_IsTimeoutActive)
            {
                new tname[16];
                format(tname, charsmax(tname), (g_TimeoutTeam == CS_TEAM_CT) ? "CT" : "T");
                formatex(hud_msg, charsmax(hud_msg), "⏸ [ TACTICAL TIMEOUT: %s ] ⏸^nResuming in: 00:%02d", tname, g_TimeoutRemaining);
                set_hudmessage(255, 80, 80, -1.0, 0.02, 0, 0.0, 1.1, 0.1, 0.1, -1);
                ShowSyncHudMsg(0, g_HudSyncHeader, "%s", hud_msg);
            }
            else
            {
                formatex(hud_msg, charsmax(hud_msg), "[ CT : %02d ]   ★ OVERTIME MR3 ★   [ %02d : T ]",
                    g_ScoreCT, g_ScoreT);
                set_hudmessage(255, 215, 0, -1.0, 0.02, 0, 0.0, 1.1, 0.1, 0.1, -1);
                ShowSyncHudMsg(0, g_HudSyncHeader, "%s", hud_msg);
            }
        }
        case STATE_HALFTIME, STATE_OVERTIME_HALFTIME:
        {
            formatex(hud_msg, charsmax(hud_msg), "★ [ HALFTIME — SWITCHING SIDES ] ★");
            set_hudmessage(255, 255, 100, -1.0, 0.02, 0, 0.0, 1.1, 0.1, 0.1, -1);
            ShowSyncHudMsg(0, g_HudSyncHeader, "%s", hud_msg);
        }
        case STATE_FINISHED:
        {
            new winner[24];
            if (g_ScoreCT > g_ScoreT) format(winner, charsmax(winner), "COUNTER-TERRORISTS");
            else if (g_ScoreT > g_ScoreCT) format(winner, charsmax(winner), "TERRORISTS");
            else format(winner, charsmax(winner), "NOBODY (DRAW)");

            formatex(hud_msg, charsmax(hud_msg), "🏆 MATCH FINISHED 🏆^nWINNER: %s (%d - %d)", winner, g_ScoreCT, g_ScoreT);
            set_hudmessage(0, 255, 120, -1.0, 0.02, 0, 0.0, 1.1, 0.1, 0.1, -1);
            ShowSyncHudMsg(0, g_HudSyncHeader, "%s", hud_msg);
        }
    }
}

// ========================================================================
// MATCH END & CLEANUP
// ========================================================================
public EndMatch(CsTeams:winner)
{
    g_MatchState = STATE_FINISHED;

    g_TimeoutQueuedCT = false;
    g_TimeoutQueuedT = false;
    g_IsTimeoutActive = false;
    set_cvar_num("amx_csgo_is_timeout", 0);
    remove_task(TASK_COMP_TIMEOUT);

    new freeze = get_pcvar_num(pCvar_FreezeTime);
    if (freeze <= 0) freeze = 20;
    set_cvar_num("mp_freezetime", freeze);

    new team_name[32];
    if (winner == CS_TEAM_CT) format(team_name, charsmax(team_name), "COUNTER-TERRORISTS");
    else if (winner == CS_TEAM_T) format(team_name, charsmax(team_name), "TERRORISTS");
    else format(team_name, charsmax(team_name), "TIE / DRAW");

    set_dhudmessage(0, 255, 120, -1.0, 0.28, 2, 0.1, 6.0, 0.1, 1.5);
    show_dhudmessage(0, "🏆 MATCH FINISHED 🏆^nWINNER: %s^nFinal Score: CT [%d] - [%d] T", team_name, g_ScoreCT, g_ScoreT);

    if (get_pcvar_num(pCvar_ScreenFade) == 1)
    {
        Util_ScreenFade(0, 0.8, 0.4, FFADE_IN, 0, 255, 120, 50);
    }

    set_task(10.0, "ResetToWarmup", TASK_COMP_MATCH_END);
}

public ResetToWarmup()
{
    UpdateMatchFormat();

    if (get_pcvar_num(pCvar_Warmup) == 0)
    {
        StartLiveMatch();
        return;
    }

    g_MatchState = STATE_WARMUP;
    g_ScoreCT = 0;
    g_ScoreT = 0;
    g_RoundNumber = 0;
    set_pcvar_num(pCvar_CurrentRound, 0);
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
    g_bInFreezePeriod = false;

    CleanWarmupWeapons();

    new pcvar_scavenge = get_cvar_pointer("bot_savage_scavenge");
    if (pcvar_scavenge) set_pcvar_num(pcvar_scavenge, 0);

    new warmup_mins = get_pcvar_num(pCvar_WarmupTime);
    if (warmup_mins <= 0) warmup_mins = 55;
    g_WarmupTimeLeft = warmup_mins * 60;

    // Configure Warmup DM YaPB state
    ApplyYaPBState(STATE_WARMUP);
    g_bKnifeRoundLive = false;
    set_cvar_num("bot_savage_kniferound", 0);
    set_cvar_num("amx_csgo_is_warmup", 1);
    set_cvar_num("mp_freezetime", 0);
    set_cvar_float("mp_roundtime", float(warmup_mins));
    set_cvar_num("mp_buy_anywhere", 1);
    set_cvar_float("mp_buytime", 999.0);
    set_cvar_string("mp_round_infinite", "abcdef");

    remove_task(TASK_COMP_WARMUP);
    remove_task(TASK_COMP_SWEEP1);
    remove_task(TASK_COMP_SWEEP2);
    remove_task(TASK_COMP_MATCH_END);
    remove_task(TASK_COMP_SYNC_TELEMETRY);
    set_task(1.0, "Task_WarmupTimer", TASK_COMP_WARMUP, _, _, "b");
    set_task(1.5, "Task_SweepWarmupBots", TASK_COMP_SWEEP1);
    set_task(4.5, "Task_SweepWarmupBots", TASK_COMP_SWEEP2);

    ResetMatchTelemetry();
    BroadcastCS2PlayerStats(0, true);
    BroadcastCS2RoundHistory(0, true);

    for (new i = 1; i <= MaxClients; i++)
    {
        g_PlayerReady[i] = false;
        ResetPlayerDamage(i);
        remove_task(i + TASK_COMP_RESPAWN);
        remove_task(i + TASK_COMP_BOTEQUIP);
        if (is_user_alive(i))
        {
            new flags = pev(i, pev_flags);
            set_pev(i, pev_flags, flags & ~FL_FROZEN);
        }
    }

    set_cvar_num("mp_startmoney", 16000);
    set_cvar_num("sv_restart", 1);

    client_print(0, print_chat, "^x04[CS:GO Match] Warmup DM Active (%d minutes)! Buy anytime & type .ready when prepared.", warmup_mins);
}

public ResetPlayerDamage(id)
{
    for (new v = 1; v <= MaxClients; v++)
    {
        g_DamageDealt[id][v] = 0;
        g_HitsDealt[id][v] = 0;
    }
}

// ========================================================================
// CS2 SCOREBOARD TELEMETRY BRIDGE (BP-04)
// ========================================================================
stock ResetMatchTelemetry()
{
    for (new i = 1; i <= 32; i++)
    {
        g_PlayerAssists[i] = 0;
        g_PlayerMVPs[i] = 0;
        g_TotalMatchDamage[i] = 0;
    }

    g_RoundHistCount = 0;
    g_PendingWinReason = WIN_REASON_NONE;
    g_LastRoundWinTime = 0.0;

    for (new r = 0; r < MAX_ROUND_HISTORY; r++)
    {
        g_RoundHistWinner[r] = 0;
        g_RoundHistReason[r] = 0;
    }
}

stock BroadcastCS2PlayerStats(targetId = 0, bool:bClear = false)
{
    if (!g_MsgPlayerStats)
        g_MsgPlayerStats = get_user_msgid("CS2PlayerStats");
    if (!g_MsgPlayerStats)
        return;

    if (targetId > 0 && (!is_user_connected(targetId) || is_user_bot(targetId)))
        return;

    new dest = (targetId > 0) ? MSG_ONE : MSG_ALL;

    if (bClear || !IS_COMPETITIVE_ENABLED())
    {
        message_begin(dest, g_MsgPlayerStats, _, targetId);
        write_byte(0);
        message_end();
        return;
    }

    new ids[32], n = 0;
    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_connected(i))
        {
            ids[n++] = i;
        }
    }

    if (n == 0)
        return;

    // Chunk at max 24 players per message (1 + 24 * 7 = 169 bytes < 192 GoldSrc limit)
    for (new start = 0; start < n; start += 24)
    {
        new chunk = n - start;
        if (chunk > 24) chunk = 24;

        message_begin(dest, g_MsgPlayerStats, _, targetId);
        write_byte(chunk);
        for (new k = 0; k < chunk; k++)
        {
            new p = ids[start + k];
            write_byte(p);
            write_short(clamp(g_PlayerAssists[p], 0, 32767));
            write_short(clamp(g_PlayerMVPs[p], 0, 32767));
            write_short(clamp(g_TotalMatchDamage[p], 0, 65535));
        }
        message_end();
    }
}

stock BroadcastCS2RoundHistory(targetId = 0, bool:bClear = false)
{
    if (!g_MsgRoundHist)
        g_MsgRoundHist = get_user_msgid("CS2RoundHist");
    if (!g_MsgRoundHist)
        return;

    if (targetId > 0 && (!is_user_connected(targetId) || is_user_bot(targetId)))
        return;

    new dest = (targetId > 0) ? MSG_ONE : MSG_ALL;

    if (bClear || !IS_COMPETITIVE_ENABLED())
    {
        message_begin(dest, g_MsgRoundHist, _, targetId);
        write_byte(0);
        write_byte(0);
        write_byte(0);
        write_byte(0);
        message_end();
        return;
    }

    new count = clamp(g_RoundHistCount, 0, MAX_ROUND_HISTORY);
    message_begin(dest, g_MsgRoundHist, _, targetId);
    write_byte(clamp(g_MaxRoundsHalf, 1, 255));
    write_byte(clamp(g_ScoreCT, 0, 255));
    write_byte(clamp(g_ScoreT, 0, 255));
    write_byte(count);
    for (new i = 0; i < count; i++)
    {
        new packed = ((g_RoundHistWinner[i] & 0x0F) << 4) | (g_RoundHistReason[i] & 0x0F);
        write_byte(packed);
    }
    message_end();
}

public Task_SyncTelemetryPostRestart()
{
    if (!IS_COMPETITIVE_ENABLED())
        return;

    if (IsLiveMatchState() || g_MatchState == STATE_FINISHED)
    {
        BroadcastCS2PlayerStats(0);
        BroadcastCS2RoundHistory(0);
    }
}

public Task_SyncClientTelemetry(taskid)
{
    new id = taskid - TASK_COMP_CLIENT_SYNC;
    if (!is_user_connected(id) || is_user_bot(id))
        return;

    if (!IS_COMPETITIVE_ENABLED() || (!IsLiveMatchState() && g_MatchState != STATE_FINISHED &&
        g_MatchState != STATE_HALFTIME && g_MatchState != STATE_OVERTIME_HALFTIME))
    {
        BroadcastCS2PlayerStats(id, true);
        BroadcastCS2RoundHistory(id, true);
    }
    else
    {
        BroadcastCS2PlayerStats(id);
        BroadcastCS2RoundHistory(id);
    }
}