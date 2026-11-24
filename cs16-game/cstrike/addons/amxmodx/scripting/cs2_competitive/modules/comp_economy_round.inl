#if defined _comp_economy_round_included
    #endinput
#endif
#define _comp_economy_round_included

// ========================================================================
// ROUND EVENTS & ECONOMY
// ========================================================================
public Event_GameRestart()
{
    g_TimeoutQueuedCT = false;
    g_TimeoutQueuedT = false;
    g_IsTimeoutActive = false;
    set_cvar_num("amx_csgo_is_timeout", 0);
    remove_task(TASK_COMP_TIMEOUT);

    if (g_MatchState != STATE_WARMUP)
    {
        new freeze = get_pcvar_num(pCvar_FreezeTime);
        if (freeze <= 0) freeze = 20;
        set_cvar_num("mp_freezetime", freeze);
    }
}

public Event_FreezeTimeStart()
{
    if (g_MatchState == STATE_WARMUP)
    {
        g_bInFreezePeriod = false;
        return;
    }

    g_bInFreezePeriod = true;

    // Freeze all bots in spawn alongside human players during freeze time
    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_alive(i) && is_user_bot(i))
        {
            new flags = pev(i, pev_flags);
            set_pev(i, pev_flags, flags | FL_FROZEN);
            set_pev(i, pev_velocity, Float:{0.0, 0.0, 0.0});
        }
    }

    // Check if Tactical Timeout was queued for this round
    if (g_TimeoutQueuedCT)
    {
        g_TimeoutQueuedCT = false;
        StartTimeout(CS_TEAM_CT);
    }
    else if (g_TimeoutQueuedT)
    {
        g_TimeoutQueuedT = false;
        StartTimeout(CS_TEAM_T);
    }
}

public LogEvent_RoundStart()
{
    if (get_pcvar_num(pCvar_Mode) == 0)
        return;

    g_bInFreezePeriod = false;

    // Restore standard freeze time for subsequent rounds
    if (g_MatchState != STATE_WARMUP)
    {
        new freeze = get_pcvar_num(pCvar_FreezeTime);
        if (freeze <= 0) freeze = 20;
        set_cvar_num("mp_freezetime", freeze);
    }

    // Ensure timeout state is cleared
    g_IsTimeoutActive = false;
    set_cvar_num("amx_csgo_is_timeout", 0);
    remove_task(TASK_COMP_TIMEOUT);

    // Unfreeze all players and bots when freeze time expires
    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_alive(i))
        {
            new flags = pev(i, pev_flags);
            set_pev(i, pev_flags, flags & ~FL_FROZEN);
        }
    }

    // Reset bomb state on every round start
    g_BombPlanted = false;

    if (g_MatchState == STATE_WARMUP)
    {
        CleanWarmupWeapons();

        new warmup_mins = get_pcvar_num(pCvar_WarmupTime);
        if (warmup_mins <= 0) warmup_mins = 55;

        set_cvar_float("mp_roundtime", float(warmup_mins));
        set_cvar_float("mp_buytime", 999.0);
        set_cvar_num("mp_buy_anywhere", 1);
        ApplyYaPBState(STATE_WARMUP);
        set_cvar_string("mp_round_infinite", "abcdef");
        set_cvar_num("mp_freezetime", 0);
        set_cvar_num("bot_savage_kniferound", 0);
        set_cvar_num("amx_csgo_is_warmup", 1);

        remove_task(TASK_COMP_SWEEP1);
        set_task(0.4, "Task_SweepWarmupBots", TASK_COMP_SWEEP1);

        message_begin(MSG_ALL, get_user_msgid("RoundTime"));
        write_short(g_WarmupTimeLeft);
        message_end();
    }
    else
    {
        set_cvar_num("amx_csgo_is_warmup", 0);
        if (g_MatchState == STATE_KNIFE_ROUND)
        {
            g_bKnifeRoundLive = true;
        }
    }

    // Reset damage tracking and MVP kills for new round
    for (new i = 1; i <= MaxClients; i++)
    {
        ResetPlayerDamage(i);
        g_KillsInRound[i] = 0;
    }
    g_BombPlanter = 0;
    g_BombDefuser = 0;

    // Apply CS:GO Loss Bonus notification & adjustment
    if (g_MatchState == STATE_LIVE_HALF_1 || g_MatchState == STATE_LIVE_HALF_2)
    {
        new ct_bonus = 1400 + (g_LossStreakCT * 500);
        new t_bonus = 1400 + (g_LossStreakT * 500);

        client_print(0, print_console, "[Economy] CT Loss Bonus: Level %d ($%d) | T Loss Bonus: Level %d ($%d)",
            g_LossStreakCT, ct_bonus, g_LossStreakT, t_bonus);
    }
}

public LogEvent_RoundEnd()
{
    if (get_pcvar_num(pCvar_Mode) == 0)
        return;

    // Reset bomb state on round end
    g_BombPlanted = false;

    // Print Damage Summary to all players
    PrintRoundDamageSummary();

    // Announce MVP
    AnnounceMVP();

    // Prepare Tactical Timeout for next round if queued
    if (g_TimeoutQueuedCT || g_TimeoutQueuedT)
    {
        if (g_MatchState == STATE_LIVE_HALF_1 || g_MatchState == STATE_LIVE_HALF_2 ||
            g_MatchState == STATE_OVERTIME_1 || g_MatchState == STATE_OVERTIME_2)
        {
            new duration = get_pcvar_num(pCvar_TimeoutDuration);
            if (duration <= 0) duration = 60;

            set_cvar_num("mp_freezetime", duration);

            new CsTeams:queuedTeam = g_TimeoutQueuedCT ? CS_TEAM_CT : CS_TEAM_T;
            new tname[16];
            format(tname, charsmax(tname), (queuedTeam == CS_TEAM_CT) ? "Counter-Terrorist" : "Terrorist");
            client_print(0, print_chat, "^x04[CS:GO Match]^x01 Tactical Timeout (%s) will begin in Freeze Time next round!", tname);
        }
    }
}

public Event_CTWin()
{
    g_BombPlanted = false;
    HandleRoundWin(CS_TEAM_CT);
}

public Event_TWin()
{
    g_BombPlanted = false;
    HandleRoundWin(CS_TEAM_T);
}

public Event_RoundDraw()
{
    g_BombPlanted = false;
    if (g_MatchState == STATE_KNIFE_ROUND && g_bKnifeRoundLive)
    {
        g_bKnifeRoundLive = false;
        client_print(0, print_chat, "^x04[CS:GO Match] Knife round was a DRAW! Restarting knife round...");
        remove_task(TASK_COMP_DRAW_KNIFE);
        set_task(2.0, "StartKnifeRound", TASK_COMP_DRAW_KNIFE);
    }
}

public HandleRoundWin(CsTeams:winning_team)
{
    if (get_pcvar_num(pCvar_Mode) == 0)
        return;

    if (g_MatchState == STATE_KNIFE_ROUND)
    {
        g_bKnifeRoundLive = false;
        set_cvar_num("bot_savage_kniferound", 0);
        g_KnifeWinner = winning_team;
        g_MatchState = STATE_SIDE_PICK;
        g_SidePickTimer = 15;

        new team_name[16];
        format(team_name, charsmax(team_name), (winning_team == CS_TEAM_CT) ? "Counter-Terrorist" : "Terrorist");

        client_print(0, print_chat, "^x04==================================================");
        client_print(0, print_chat, "^x04[CS:GO Match] %s WON the Knife Round!", team_name);
        client_print(0, print_chat, "^x04==================================================");

        // Find a representative
        g_KnifeChooserId = FindTeamRepresentative(winning_team);
        if (g_KnifeChooserId > 0 && !is_user_bot(g_KnifeChooserId))
        {
            client_print(0, print_chat, "^x04Type .stay or .switch in chat (15 seconds) to pick side!");
            ShowSidePickMenu(g_KnifeChooserId);
            set_task(1.0, "Task_SidePickCountdown", TASK_COMP_SIDEPICK, _, _, "b");
        }
        else
        {
            // Bot team won! Auto-pick stay after 2 seconds
            client_print(0, print_chat, "^x04[CS:GO Match] %s Bots won Knife Round and chose to STAY!", team_name);
            remove_task(TASK_COMP_AUTOPICK);
            set_task(2.0, "Task_BotAutoPickStay", TASK_COMP_AUTOPICK);
        }
        return;
    }

    if (g_MatchState == STATE_LIVE_HALF_1 || g_MatchState == STATE_LIVE_HALF_2)
    {
        g_RoundNumber++;
        set_pcvar_num(pCvar_CurrentRound, g_RoundNumber + 1);

        if (winning_team == CS_TEAM_CT)
        {
            g_ScoreCT++;
            // CS:GO Loss ladder: Decrement winner by 1, increment loser by 1
            if (get_pcvar_num(pCvar_LossBonus) == 1)
            {
                if (g_LossStreakCT > 0) g_LossStreakCT--;
                if (g_LossStreakT < 4)  g_LossStreakT++;
            }
        }
        else if (winning_team == CS_TEAM_T)
        {
            g_ScoreT++;
            if (get_pcvar_num(pCvar_LossBonus) == 1)
            {
                if (g_LossStreakT > 0) g_LossStreakT--;
                if (g_LossStreakCT < 4) g_LossStreakCT++;
            }
        }

        // Check Regulation Win Conditions
        if (g_ScoreCT >= g_TargetWinScore)
        {
            EndMatch(CS_TEAM_CT);
            return;
        }
        else if (g_ScoreT >= g_TargetWinScore)
        {
            EndMatch(CS_TEAM_T);
            return;
        }

        // Check Halftime Condition (Half 1 ended)
        if (g_MatchState == STATE_LIVE_HALF_1 && g_RoundNumber == g_MaxRoundsHalf)
        {
            TriggerHalftime();
            return;
        }

        // Check Overtime Condition (Full 24 rounds ended in 12-12 tie)
        if (g_RoundNumber == (g_MaxRoundsHalf * 2) && g_ScoreCT == g_ScoreT)
        {
            TriggerOvertime();
            return;
        }
    }
    else if (g_MatchState == STATE_OVERTIME_1 || g_MatchState == STATE_OVERTIME_2)
    {
        g_OTRoundNumber++;
        set_pcvar_num(pCvar_CurrentRound, (g_MaxRoundsHalf * 2) + g_OTRoundNumber + 1);
        if (winning_team == CS_TEAM_CT) g_ScoreCT++;
        else if (winning_team == CS_TEAM_T) g_ScoreT++;

        if (g_ScoreCT >= g_OTTargetWinScore)
        {
            EndMatch(CS_TEAM_CT);
            return;
        }
        else if (g_ScoreT >= g_OTTargetWinScore)
        {
            EndMatch(CS_TEAM_T);
            return;
        }

        if (g_MatchState == STATE_OVERTIME_1 && g_OTRoundNumber == g_OTMaxRoundsHalf)
        {
            TriggerOTHalftime();
            return;
        }

        if (g_OTRoundNumber == (g_OTMaxRoundsHalf * 2))
        {
            // Another tie or match end
            if (g_ScoreCT == g_ScoreT)
            {
                TriggerOvertime(); // Another OT
            }
        }
    }
}


// ========================================================================
// PLAYER HOOKS (SPAWN, KILL, DAMAGE)
// ========================================================================
public OnPlayerSpawnPost(id)
{
    CHECK_COMPETITIVE_ACTIVE(HAM_IGNORED);

    if (g_MatchState == STATE_WARMUP)
    {
        if (!xash_is_bot_combat_ready(id))
            return HAM_IGNORED;
    }
    else
    {
        if (!is_user_alive(id))
            return HAM_IGNORED;
    }

    new CsTeams:team = safe_cs_get_user_team(id);
    if (team != CS_TEAM_CT && team != CS_TEAM_T)
    {
        if (is_user_bot(id) && team == CS_TEAM_UNASSIGNED && g_MatchState == STATE_WARMUP)
        {
            remove_task(id + TASK_COMP_BOTEQUIP);
            set_task(0.3, "Task_WarmupBotEquip", id + TASK_COMP_BOTEQUIP);
        }
        return HAM_IGNORED;
    }

    if (g_MatchState == STATE_WARMUP)
    {
        cs_set_user_money(id, 16000);
        cs_set_user_armor(id, 100, CS_ARMOR_VESTHELM);

        // Strip default/spawn grenades completely in Warmup DM
        cs_set_user_bpammo(id, CSW_HEGRENADE, 0);
        cs_set_user_bpammo(id, CSW_FLASHBANG, 0);
        cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 0);

        // Disallow C4 and planting during warmup DM
        cs_set_user_plant(id, 0, 0);
        if (user_has_weapon(id, CSW_C4))
        {
            StripUserC4(id);
        }

        // Enforce buytime check disabled and buy anywhere
        set_cvar_float("mp_buytime", 999.0);
        set_cvar_num("mp_buy_anywhere", 1);

        // Show shopping cart buyzone HUD icon and sync round clock for human player
        if (!is_user_bot(id))
        {
            static msgRoundTime;
            if (!msgRoundTime) msgRoundTime = get_user_msgid("RoundTime");
            message_begin(MSG_ONE_UNRELIABLE, msgRoundTime, _, id);
            write_short(g_WarmupTimeLeft);
            message_end();

            static msgStatusIcon;
            if (!msgStatusIcon) msgStatusIcon = get_user_msgid("StatusIcon");
            message_begin(MSG_ONE_UNRELIABLE, msgStatusIcon, _, id);
            write_byte(1);
            write_string("buyzone");
            write_byte(0);
            write_byte(160);
            write_byte(0);
            message_end();
        }

        if (is_user_bot(id))
        {
            g_iBotEquipRetries[id] = 0;
            remove_task(id + TASK_COMP_BOTEQUIP);
            set_task(0.1, "Task_WarmupBotEquip", id + TASK_COMP_BOTEQUIP);

            // Auto-ready bot on spawn if enabled and any human is already ready
            if (get_pcvar_num(pCvar_BotAutoReady) == 1 && IsAnyHumanReady())
            {
                new CsTeams:bteam = safe_cs_get_user_team(id);
                if (bteam == CS_TEAM_CT || bteam == CS_TEAM_T)
                {
                    g_PlayerReady[id] = true;
                    CheckReadyStatus();
                }
            }
        }
    }
    else if (g_MatchState == STATE_KNIFE_ROUND)
    {
        cs_set_user_money(id, 0);
        // 0.1s delay ensures engine default Glock/USP is given first and then cleanly stripped
        remove_task(TASK_COMP_STRIPKNIFE + id);
        set_task(0.1, "Task_StripToKnife", TASK_COMP_STRIPKNIFE + id);
    }

    // Freeze bots during freeze time in live match & knife round
    if (g_MatchState != STATE_WARMUP && g_bInFreezePeriod && is_user_bot(id))
    {
        new flags = pev(id, pev_flags);
        set_pev(id, pev_flags, flags | FL_FROZEN);
        set_pev(id, pev_velocity, Float:{0.0, 0.0, 0.0});
    }

    return HAM_IGNORED;
}

public Task_StripToKnife(taskid)
{
    new id = taskid - TASK_COMP_STRIPKNIFE;
    if (is_user_alive(id) && g_MatchState == STATE_KNIFE_ROUND)
    {
        strip_user_weapons(id);
        give_item(id, "weapon_knife");
        cs_set_user_armor(id, 100, CS_ARMOR_VESTHELM);
        cs_set_user_money(id, 0);
    }
}

public OnPlayerKilledPre(victim, attacker, shouldgib)
{
    CHECK_COMPETITIVE_ACTIVE(HAM_IGNORED);

    if (victim < 1 || victim > MaxClients)
        return HAM_IGNORED;

    return HAM_IGNORED;
}

public OnPlayerKilledPost(victim, attacker, shouldgib)
{
    CHECK_COMPETITIVE_ACTIVE(HAM_IGNORED);

    if (victim < 1 || victim > MaxClients)
        return HAM_IGNORED;

    if (g_MatchState == STATE_WARMUP)
    {
        remove_task(victim + TASK_COMP_BOTEQUIP);
        g_bEquippingBot[victim] = false;
        g_iBotEquipRetries[victim] = 0;

        // Respawn after 1.5 seconds in warmup (mp_item_staytime 0 natively purges dropped weaponboxes)
        set_task(1.5, "Task_WarmupRespawn", victim + TASK_COMP_RESPAWN);
    }
    else if (g_MatchState == STATE_LIVE_HALF_1 || g_MatchState == STATE_LIVE_HALF_2 ||
             g_MatchState == STATE_OVERTIME_1 || g_MatchState == STATE_OVERTIME_2)
    {
        if (1 <= attacker <= MaxClients && attacker != victim)
        {
            g_KillsInRound[attacker]++;
        }
    }

    return HAM_IGNORED;
}

public Task_WarmupRespawn(taskid)
{
    if (!IS_COMPETITIVE_ENABLED())
        return;

    new id = taskid - TASK_COMP_RESPAWN;
    if (is_user_connected(id) && !is_user_alive(id) && g_MatchState == STATE_WARMUP)
    {
        new CsTeams:team = safe_cs_get_user_team(id);
        if (team == CS_TEAM_CT || team == CS_TEAM_T)
        {
            xash_safe_respawn(id);
        }
    }
}

public Task_WarmupBotEquip(taskid)
{
    if (!IS_COMPETITIVE_ENABLED() || g_MatchState != STATE_WARMUP)
        return;

    new id = taskid - TASK_COMP_BOTEQUIP;
    if (!is_user_bot(id) || !xash_is_bot_combat_ready(id))
        return;

    new CsTeams:team = safe_cs_get_user_team(id);
    if (team == CS_TEAM_UNASSIGNED)
    {
        set_task(0.3, "Task_WarmupBotEquip", id + TASK_COMP_BOTEQUIP);
        return;
    }

    if (team != CS_TEAM_CT && team != CS_TEAM_T)
        return;

    g_bEquippingBot[id] = true;

    // Delegate Warmup DM loadout to Unified 5-Role Delivery Engine (Free Loadout, No Utility/Defuse)
    new iEquipped = cs2_api_equip_bot_role(id, true, false);

    g_bEquippingBot[id] = false;

    // Bounded retry if equip failed or bot still lacks a primary rifle
    if ((iEquipped == 0 || !(pev(id, pev_weapons) & XASH_PRIMARY_WEAPONS_BIT_SUM)) && g_iBotEquipRetries[id] < 5)
    {
        g_iBotEquipRetries[id]++;
        set_task(0.25, "Task_WarmupBotEquip", id + TASK_COMP_BOTEQUIP);
    }
    else
    {
        g_iBotEquipRetries[id] = 0;
    }
}

public OnDefaultPistolDeploy_Post(iEnt)
{
    CHECK_COMPETITIVE_ACTIVE(HAM_IGNORED);

    if (g_MatchState != STATE_WARMUP || !pev_valid(iEnt))
        return HAM_IGNORED;

    new id = get_pdata_cbase(iEnt, XASH_OFFSET_WEAPON_OWNER, XASH_EXTRAOFFSET_WEAPONS);
    if (id < 1 || id > MaxClients || !is_user_bot(id) || g_bEquippingBot[id])
        return HAM_IGNORED;

    if (!xash_is_bot_combat_ready(id))
        return HAM_IGNORED;

    // Only intercept if bot has no primary weapon yet (default spawn pistol)
    if (pev(id, pev_weapons) & XASH_PRIMARY_WEAPONS_BIT_SUM)
        return HAM_IGNORED;

    if (!task_exists(id + TASK_COMP_BOTEQUIP))
    {
        set_task(0.1, "Task_WarmupBotEquip", id + TASK_COMP_BOTEQUIP);
    }

    return HAM_IGNORED;
}

public Task_SweepWarmupBots()
{
    if (!IS_COMPETITIVE_ENABLED() || g_MatchState != STATE_WARMUP)
        return;

    for (new i = 1; i <= MaxClients; i++)
    {
        if (!is_user_bot(i) || !xash_is_bot_combat_ready(i))
            continue;

        new CsTeams:team = safe_cs_get_user_team(i);
        if (team != CS_TEAM_CT && team != CS_TEAM_T)
            continue;

        if (!(pev(i, pev_weapons) & XASH_PRIMARY_WEAPONS_BIT_SUM) && !task_exists(i + TASK_COMP_BOTEQUIP))
        {
            g_iBotEquipRetries[i] = 0;
            set_task(0.1, "Task_WarmupBotEquip", i + TASK_COMP_BOTEQUIP);
        }
    }
}

public Task_WarmupTimer()
{
    if (!IS_COMPETITIVE_ENABLED() || (pCvar_RedmActive && get_pcvar_num(pCvar_RedmActive) == 1))
    {
        remove_task(TASK_COMP_WARMUP);
        ShowSyncHudMsg(0, g_HudSyncHeader, "");
        return;
    }

    if (g_MatchState != STATE_WARMUP)
    {
        remove_task(TASK_COMP_WARMUP);
        return;
    }

    if (g_WarmupTimeLeft > 0)
    {
        g_WarmupTimeLeft--;
        if (g_WarmupTimeLeft == 300)
        {
            client_print(0, print_chat, "^x04[CS:GO Match]^x01 Warmup ending in 5 minutes! Type .ready when prepared.");
        }
        else if (g_WarmupTimeLeft == 60)
        {
            client_print(0, print_chat, "^x04[CS:GO Match]^x01 Warmup ending in 1 minute!");
        }
    }
    else
    {
        // 55 minutes finished! Auto-start match countdown
        remove_task(TASK_COMP_WARMUP);
        client_print(0, print_chat, "^x04[CS:GO Match]^x01 Warmup time limit reached! Starting match countdown...");

        for (new i = 1; i <= MaxClients; i++)
        {
            if (is_user_connected(i))
            {
                new CsTeams:team = safe_cs_get_user_team(i);
                if (team == CS_TEAM_CT || team == CS_TEAM_T)
                {
                    g_PlayerReady[i] = true;
                }
            }
        }

        remove_task(TASK_COMP_COUNTDOWN);
        g_MatchState = STATE_COUNTDOWN;
        g_CountdownTimer = 5;
        set_task(1.0, "Task_MatchCountdown", TASK_COMP_COUNTDOWN, _, _, "b");
    }
}

public Msg_RoundTime(msg_id, msg_dest, msg_entity)
{
    CHECK_COMPETITIVE_ACTIVE(PLUGIN_CONTINUE);

    if (g_MatchState == STATE_WARMUP)
    {
        set_msg_arg_int(1, ARG_SHORT, g_WarmupTimeLeft);
    }
    return PLUGIN_CONTINUE;
}

public OnTakeDamagePost(victim, inflictor, attacker, Float:damage, damagebits)
{
    if (attacker < 1 || attacker > MaxClients || victim < 1 || victim > MaxClients || attacker == victim)
        return HAM_IGNORED;

    if (!is_user_connected(attacker) || !is_user_connected(victim))
        return HAM_IGNORED;

    if (safe_cs_get_user_team(attacker) != safe_cs_get_user_team(victim))
    {
        g_DamageDealt[attacker][victim] += floatround(damage);
        g_HitsDealt[attacker][victim]++;
    }

    return HAM_IGNORED;
}

public LogEvent_Plant()
{
    if (g_MatchState == STATE_WARMUP)
        return;

    new loguser[80], name[32];
    read_logargv(0, loguser, charsmax(loguser));
    parse_loguser(loguser, name, charsmax(name));
    g_BombPlanter = get_user_index(name);

    g_BombPlanted = true;
    g_BombPlantedTime = get_gametime();
    g_C4TimerCvar = get_cvar_num("mp_c4timer");
    if (g_C4TimerCvar <= 0) g_C4TimerCvar = 40;

    if (get_pcvar_num(pCvar_ScreenFade) == 1)
    {
        Util_ScreenFade(0, 0.4, 0.1, FFADE_IN, 255, 80, 40, 35);
    }
}

public LogEvent_Defuse()
{
    new loguser[80], name[32];
    read_logargv(0, loguser, charsmax(loguser));
    parse_loguser(loguser, name, charsmax(name));
    g_BombDefuser = get_user_index(name);

    g_BombPlanted = false;
}

public LogEvent_TargetBombed()
{
    g_BombPlanted = false;
}
