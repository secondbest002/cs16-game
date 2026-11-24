#if defined _savage_reflex_esp_included
    #endinput
#endif
#define _savage_reflex_esp_included

// ==============================================================================
// 8. DEFENSIVE REFLEX: Anti-Flash 180-Degree Turnaround
// ==============================================================================
public Task_CheckGrenades()
{
    if (!get_pcvar_num(g_pCvarEnable) || !get_pcvar_num(g_pCvarMicroOverride) || !get_pcvar_num(g_pCvarAntiFlash))
        return;

    new ent = -1;
    while ((ent = find_ent_by_class(ent, "grenade")))
    {
        if (!pev_valid(ent) || (pev(ent, pev_flags) & FL_KILLME))
            continue;

        new szModel[32];
        pev(ent, pev_model, szModel, charsmax(szModel));

        if (containi(szModel, "flashbang") == -1)
            continue;

        new Float:vecGrenadeOrigin[3];
        pev(ent, pev_origin, vecGrenadeOrigin);

        for (new i = 1; i <= 32; i++)
        {
            if (is_user_alive(i) && is_user_bot(i))
            {
                new Float:vecBotOrigin[3];
                pev(i, pev_origin, vecBotOrigin);

                new Float:flDist = get_distance_f(vecBotOrigin, vecGrenadeOrigin);
                if (flDist <= 650.0)
                {
                    TriggerAntiFlashReflex(i, vecGrenadeOrigin);
                }
            }
        }
    }
}

TriggerAntiFlashReflex(id, Float:vecGrenadeOrigin[3])
{
    new Float:vecBotOrigin[3], Float:vecViewOfs[3], Float:vecBotEyes[3];
    pev(id, pev_origin, vecBotOrigin);
    pev(id, pev_view_ofs, vecViewOfs);
    xs_vec_add(vecBotOrigin, vecViewOfs, vecBotEyes);

    new Float:vecDirToGrenade[3];
    xs_vec_sub(vecGrenadeOrigin, vecBotEyes, vecDirToGrenade);
    new Float:flDist = xs_vec_len(vecDirToGrenade);
    if (flDist < 1.0)
        return;

    xs_vec_normalize(vecDirToGrenade, vecDirToGrenade);

    new Float:vecVAngle[3], Float:vecForward[3];
    pev(id, pev_v_angle, vecVAngle);
    angle_vector(vecVAngle, ANGLEVECTOR_FORWARD, vecForward);

    new Float:flDot = xs_vec_dot(vecForward, vecDirToGrenade);
    if (flDot > 0.15)
    {
        vecVAngle[1] += 180.0;
        if (vecVAngle[1] > 180.0)
            vecVAngle[1] -= 360.0;
        
        vecVAngle[0] = 35.0;
        ClampAngles(vecVAngle);

        set_pev(id, pev_v_angle, vecVAngle);
        set_pev(id, pev_angles, vecVAngle);

        g_flFlashDodgeUntil[id] = get_gametime() + 0.75;
    }
}

// ==============================================================================
// 9. SOUND ESP SCANNER
// ==============================================================================
public Task_SoundESPScanner()
{
    if (!get_pcvar_num(g_pCvarEnable) || !get_pcvar_num(g_pCvarMicroOverride) || !get_pcvar_num(g_pCvarWallbang))
        return;

    new Float:flGameTime = get_gametime();

    for (new enemy = 1; enemy <= 32; enemy++)
    {
        if (!is_user_alive(enemy))
            continue;

        new Float:vecEnemyVel[3];
        pev(enemy, pev_velocity, vecEnemyVel);
        new Float:flEnemySpeed = floatsqroot(vecEnemyVel[0] * vecEnemyVel[0] + vecEnemyVel[1] * vecEnemyVel[1]);
        new iButtons = pev(enemy, pev_button);

        new bool:bIsMakingSound = (flEnemySpeed > 175.0 && (pev(enemy, pev_flags) & FL_ONGROUND) && !(iButtons & IN_DUCK));

        if (!bIsMakingSound)
            continue;

        new Float:vecEnemyOrigin[3];
        pev(enemy, pev_origin, vecEnemyOrigin);
        vecEnemyOrigin[2] += 17.0;

        new CsTeams:teamEnemy = cs_get_user_team(enemy);

        for (new bot = 1; bot <= 32; bot++)
        {
            if (is_user_alive(bot) && is_user_bot(bot))
            {
                new CsTeams:teamBot = cs_get_user_team(bot);
                if (teamBot != teamEnemy && teamBot != CS_TEAM_UNASSIGNED && teamBot != CS_TEAM_SPECTATOR)
                {
                    new Float:vecBotOrigin[3];
                    pev(bot, pev_origin, vecBotOrigin);

                    new Float:flDist = get_distance_f(vecBotOrigin, vecEnemyOrigin);
                    if (flDist <= 750.0)
                    {
                        if (!CanSeeDirectly(bot, enemy))
                        {
                            g_iSoundTarget[bot] = enemy;
                            g_flSoundTargetExpire[bot] = flGameTime + 1.2;
                            g_vecSoundTargetOrigin[bot] = vecEnemyOrigin;
                        }
                    }
                }
            }
        }
    }
}

// ==============================================================================
// 10. HIGH-IQ FAKE DEFUSE MIND GAME
// ==============================================================================
CheckForPlantedC4FakeDefuse(id)
{
    new ent = -1;
    while ((ent = find_ent_by_class(ent, "grenade")))
    {
        if (!pev_valid(ent))
            continue;

        new szModel[32];
        pev(ent, pev_model, szModel, charsmax(szModel));
        if (containi(szModel, "c4") == -1)
            continue;

        new Float:vecC4Origin[3], Float:vecBotOrigin[3];
        pev(ent, pev_origin, vecC4Origin);
        pev(id, pev_origin, vecBotOrigin);

        if (get_distance_f(vecBotOrigin, vecC4Origin) <= 120.0)
        {
            g_bDidFakeDefuse[id] = true;
            g_flFakeDefuseAmbushUntil[id] = get_gametime() + 2.2;

            new Float:vecAngles[3];
            pev(id, pev_v_angle, vecAngles);
            vecAngles[0] = 0.0;
            g_vecFakeDefuseAmbushAngle[id] = vecAngles;
            break;
        }
    }
}

// ==============================================================================
// 11. TEAMWORK, TRADE-FRAG & EVENT CALLOUTS
// ==============================================================================
public OnPlayerKilled(victim, killer, shouldgib)
{
    if (!get_pcvar_num(g_pCvarEnable))
        return HAM_IGNORED;

    if (!is_user_connected(victim) || !is_user_connected(killer) || victim == killer)
        return HAM_IGNORED;

    new CsTeams:teamVictim = cs_get_user_team(victim);
    new CsTeams:teamKiller = cs_get_user_team(killer);

    if (teamVictim == teamKiller || teamVictim == CS_TEAM_UNASSIGNED || teamVictim == CS_TEAM_SPECTATOR)
        return HAM_IGNORED;

    new Float:vecVicOrigin[3], Float:vecKilOrigin[3];
    pev(victim, pev_origin, vecVicOrigin);
    pev(killer, pev_origin, vecKilOrigin);

    if (get_pcvar_num(g_pCvarHiveMind))
    {
        g_vecTeamThreatOrigin[teamVictim] = vecKilOrigin;
        g_flTeamThreatTime[teamVictim] = get_gametime();
        g_iTeamThreatTarget[teamVictim] = killer;
    }

    if (!get_pcvar_num(g_pCvarTradeFrag))
        return HAM_IGNORED;

    new iBestBot = 0;
    new Float:flBestDist = 1600.0;

    for (new i = 1; i <= 32; i++)
    {
        if (i != victim && is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == teamVictim)
        {
            new Float:vecOrigin[3];
            pev(i, pev_origin, vecOrigin);
            new Float:flDist = get_distance_f(vecOrigin, vecVicOrigin);

            if (g_iPlayerRole[i] == ROLE_SUPPORT)
                flDist *= 0.70;

            if (flDist < flBestDist)
            {
                flBestDist = flDist;
                iBestBot = i;
            }
        }
    }

    if (get_pcvar_num(g_pCvarMicroOverride) && iBestBot > 0 && is_user_alive(killer))
    {
        g_iTradeTarget[iBestBot] = killer;
        g_flTradeExpire[iBestBot] = get_gametime() + 3.5;
        g_flLastCombatTime[iBestBot] = get_gametime();

        new Float:vecBotEyes[3], Float:vecDiff[3], Float:vecAimAngles[3];
        pev(iBestBot, pev_origin, vecBotEyes);
        vecBotEyes[2] += 17.0;
        xs_vec_sub(vecKilOrigin, vecBotEyes, vecDiff);
        new Float:flDist = xs_vec_len(vecDiff);
        if (flDist > 1.0)
        {
            vector_to_angle(vecDiff, vecAimAngles);
            ClampAngles(vecAimAngles);

            set_pev(iBestBot, pev_v_angle, vecAimAngles);
            set_pev(iBestBot, pev_angles, vecAimAngles);
        }

        if (get_pcvar_num(g_pCvarDoublePeek))
        {
            SetupDoublePeekPair(iBestBot, teamVictim);
        }
    }

    return HAM_IGNORED;
}

SetupDoublePeekPair(bot1, CsTeams:team)
{
    new Float:vecBot1Origin[3];
    pev(bot1, pev_origin, vecBot1Origin);

    for (new i = 1; i <= 32; i++)
    {
        if (i != bot1 && is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == team)
        {
            new Float:vecBot2Origin[3];
            pev(i, pev_origin, vecBot2Origin);
            if (get_distance_f(vecBot1Origin, vecBot2Origin) <= 400.0)
            {
                g_iDoublePeekPartner[bot1] = i;
                g_iDoublePeekPartner[i] = bot1;
                g_bIsCrouchPeeker[bot1] = true;
                g_bIsCrouchPeeker[i] = false;
                new Float:flExpire = get_gametime() + 2.0;
                g_flDoublePeekExpire[bot1] = flExpire;
                g_flDoublePeekExpire[i] = flExpire;
                break;
            }
        }
    }
}

public OnPlayerTakeDamage(victim, inflictor, attacker, Float:damage, damage_type)
{
    if (!get_pcvar_num(g_pCvarEnable))
        return HAM_IGNORED;

    if (!is_user_connected(victim) || !is_user_connected(attacker) || victim == attacker)
        return HAM_IGNORED;

    new Float:flGameTime = get_gametime();

    if (is_user_bot(victim))
    {
        g_flLastCombatTime[victim] = flGameTime;
        ReleaseBotHoldAngle(victim);
    }

    if (is_user_bot(attacker))
    {
        g_flLastCombatTime[attacker] = flGameTime;
    }

    if (attacker >= 1 && attacker <= 32 && victim >= 1 && victim <= 32)
    {
        g_iLastDamageTarget[attacker] = victim;
        g_iLastDamageDealt[attacker] = floatround(damage);
        g_flLastDamageTime[attacker] = flGameTime;
    }

    if (get_pcvar_num(g_pCvarHiveMind))
    {
        new CsTeams:teamVictim = cs_get_user_team(victim);
        new CsTeams:teamAttacker = cs_get_user_team(attacker);

        if (teamVictim != teamAttacker)
        {
            new Float:vecAttackerOrigin[3];
            pev(attacker, pev_origin, vecAttackerOrigin);
            g_vecTeamThreatOrigin[teamVictim] = vecAttackerOrigin;
            g_flTeamThreatTime[teamVictim] = flGameTime;
            g_iTeamThreatTarget[teamVictim] = attacker;
        }
    }

    return HAM_IGNORED;
}

public Event_PlantTheBomb()
{
    if (!get_pcvar_num(g_pCvarCalls))
        return;

    BroadcastTeamCall(CS_TEAM_T, "^4[Bomb Intel]^3 Bomb has been planted! Hold defensive crossfires!");
    PlayTeamAudio(CS_TEAM_T, "spk radio/coverme");

    BroadcastTeamCall(CS_TEAM_CT, "^4[Bomb Intel]^1 The bomb is planted! Coordinate retake together!");
    PlayTeamAudio(CS_TEAM_CT, "spk radio/regroup");
}

public Event_DefusedTheBomb()
{
    if (!get_pcvar_num(g_pCvarCalls))
        return;

    BroadcastTeamCall(CS_TEAM_CT, "^4[Bomb Intel]^4 Bomb defused! Sector secured!");
    PlayTeamAudio(CS_TEAM_CT, "spk radio/clear");
}
