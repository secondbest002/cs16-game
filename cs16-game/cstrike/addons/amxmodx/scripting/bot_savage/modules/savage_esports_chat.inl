#if defined _savage_esports_chat_included
    #endinput
#endif
#define _savage_esports_chat_included

// ==============================================================================
// 6. ESPORTS PLAYER-TO-BOT CHAT & CALLOUT ENGINE (v3.4)
// ==============================================================================
stock GetFirstBotTeammate(CsTeams:team)
{
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == team)
            return i;
    }
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_connected(i) && is_user_bot(i) && cs_get_user_team(i) == team)
            return i;
    }
    return 0;
}

bool:Execute_CareZoneCall(id, iZone, const szZoneName[])
{
    new CsTeams:team = cs_get_user_team(id);
    new Float:flGameTime = get_gametime();
    g_iActiveDangerZone[team] = iZone;
    g_flDangerZoneExpire[team] = flGameTime + 18.0;

    new Float:vecPlayerOrigin[3], Float:vecVAngle[3], Float:vecForward[3];
    pev(id, pev_origin, vecPlayerOrigin);
    pev(id, pev_v_angle, vecVAngle);
    angle_vector(vecVAngle, ANGLEVECTOR_FORWARD, vecForward);

    if (iZone == ZONE_SITE_A && g_bBombsitesFound)
        xs_vec_copy(g_vecBombsiteA, g_vecDangerPoint[team]);
    else if (iZone == ZONE_SITE_B && g_bBombsitesFound)
        xs_vec_copy(g_vecBombsiteB, g_vecDangerPoint[team]);
    else if (iZone == ZONE_MID && g_bMidPointCalculated)
        xs_vec_copy(g_vecMapMidPoint, g_vecDangerPoint[team]);
    else
    {
        xs_vec_mul_scalar(vecForward, 500.0, vecForward);
        xs_vec_add(vecPlayerOrigin, vecForward, g_vecDangerPoint[team]);
    }

    new iResponder = GetTeamIgl(team);
    if (iResponder <= 0 || !is_user_alive(iResponder))
        iResponder = GetFirstBotTeammate(team);

    if (iResponder > 0)
    {
        new szBotName[32];
        get_user_name(iResponder, szBotName, charsmax(szBotName));
        new iRole = g_iPlayerRole[iResponder];

        switch (iZone)
        {
            case ZONE_SITE_A:
                BroadcastTeamCall(team, "^4[Danger Intel]^3 %s %s^1: Copy that! Watching angles on ^4Bombsite A^1. Danger flagged, do not dry peek!", g_szRoleTags[iRole], szBotName);
            case ZONE_SITE_B:
                BroadcastTeamCall(team, "^4[Danger Intel]^3 %s %s^1: Understood! ^4Sector B^1 flagged as hot. Clear corners with utility!", g_szRoleTags[iRole], szBotName);
            case ZONE_SHORT:
                BroadcastTeamCall(team, "^4[Danger Intel]^3 %s %s^1: Watching ^4Short / Catwalk^1! Crosshair locked, playing cautious.", g_szRoleTags[iRole], szBotName);
            case ZONE_LONG:
                BroadcastTeamCall(team, "^4[Danger Intel]^3 %s %s^1: Acknowledged, scope locked on ^4Long A^1. Ready for their sniper!", g_szRoleTags[iRole], szBotName);
            case ZONE_MID:
                BroadcastTeamCall(team, "^4[Danger Intel]^3 %s %s^1: ^4Mid is dangerous^1! Hold passive angles, don't give first blood!", g_szRoleTags[iRole], szBotName);
            case ZONE_FLANK:
                BroadcastTeamCall(team, "^4[Danger Intel]^3 %s %s^1: Flank alert confirmed! Anchoring our ^4rear six^1.", g_szRoleTags[iRole], szBotName);
            case ZONE_AWP:
                BroadcastTeamCall(team, "^4[Danger Intel]^3 %s %s^1: ^4Enemy sniper spotted^1! Pop flashes and smokes before challenging!", g_szRoleTags[iRole], szBotName);
            case ZONE_PUSH:
                BroadcastTeamCall(team, "^4[Danger Intel]^3 %s %s^1: ^4Enemy push incoming^1! Fall into defensive crossfire positions!", g_szRoleTags[iRole], szBotName);
            default:
                BroadcastTeamCall(team, "^4[Danger Intel]^3 %s %s^1: Copy, danger flagged at ^4%s^1. Proceeding with caution!", g_szRoleTags[iRole], szBotName, szZoneName);
        }

        PlayTeamAudio(team, (iZone == ZONE_AWP || iZone == ZONE_PUSH) ? "spk radio/takingfire" : "spk radio/spotted");
    }

    // Route / pre-aim up to 2 nearest non-busy teammates toward the danger zone via BP-01 bridge
    if (!g_bInFreezeTime && !IsKnifeRoundActive() && !IsWarmupActive() && g_bYapbAvailable)
    {
        new Float:vecDangerEye[3];
        xs_vec_copy(g_vecDangerPoint[team], vecDangerEye);
        vecDangerEye[2] += 36.0;

        new iAssigned = 0;
        for (new pass = 0; pass < 2 && iAssigned < 2; pass++)
        {
            new iBestBot = 0;
            new Float:flBestDist = 1100.0;

            for (new b = 1; b <= MaxClients; b++)
            {
                if (!is_user_alive(b) || !is_user_bot(b) || safe_cs_get_user_team(b) != team)
                    continue;

                if (!Savage_Bridge_CanRouteMacro(b, false))
                    continue;

                new Float:vecBotOrigin[3];
                pev(b, pev_origin, vecBotOrigin);
                new Float:flDist = get_distance_f(vecBotOrigin, g_vecDangerPoint[team]);
                if (flDist < flBestDist)
                {
                    flBestDist = flDist;
                    iBestBot = b;
                }
            }

            if (iBestBot <= 0)
                break;

            if (flBestDist <= 550.0)
            {
                Savage_Bridge_RequestHold(iBestBot, vecDangerEye, 2.5);
            }
            else
            {
                Savage_Bridge_RouteMacroGoal(iBestBot, g_vecDangerPoint[team], 3.0, false);
            }
            iAssigned++;
        }
    }
    return true;
}

bool:Execute_DropC4Call(id)
{
    new CsTeams:team = cs_get_user_team(id);
    if (team != CS_TEAM_T)
    {
        client_print_color(id, print_team_default, "^4[Objective]^1 Only Terrorists manage C4 explosives!");
        return true;
    }

    if (user_has_weapon(id, CSW_C4))
    {
        client_print_color(id, print_team_default, "^4[Objective]^1 You already carry the ^4C4 Backpack^1! Lead the site execute!");
        return true;
    }

    new iCarrier = 0;
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && cs_get_user_team(i) == CS_TEAM_T && user_has_weapon(i, CSW_C4))
        {
            iCarrier = i;
            break;
        }
    }

    if (iCarrier <= 0)
    {
        client_print_color(id, print_team_default, "^4[Objective]^1 No teammate is currently holding the C4 (dropped or planted).");
        return true;
    }

    if (!is_user_bot(iCarrier))
    {
        new szCarrierName[32];
        get_user_name(iCarrier, szCarrierName, charsmax(szCarrierName));
        client_print_color(id, print_team_default, "^4[Objective]^3 %s^1 is carrying the C4!", szCarrierName);
        return true;
    }

    new Float:vecCarrier[3], Float:vecPlayer[3];
    pev(iCarrier, pev_origin, vecCarrier);
    pev(id, pev_origin, vecPlayer);
    new Float:flDist = get_distance_f(vecCarrier, vecPlayer);

    new szBotName[32];
    get_user_name(iCarrier, szBotName, charsmax(szBotName));
    new iRole = g_iPlayerRole[iCarrier];

    if (flDist <= 350.0)
    {
        new Float:vecDiff[3], Float:vecAimAngles[3];
        xs_vec_sub(vecPlayer, vecCarrier, vecDiff);
        vector_to_angle(vecDiff, vecAimAngles);
        ClampAngles(vecAimAngles);
        set_pev(iCarrier, pev_v_angle, vecAimAngles);
        set_pev(iCarrier, pev_angles, vecAimAngles);

        engclient_cmd(iCarrier, "weapon_c4");
        engclient_cmd(iCarrier, "drop", "weapon_c4");

        BroadcastTeamCall(CS_TEAM_T, "^4[Objective]^3 %s %s^1: Dropping the C4 to you! Plant safe!", g_szRoleTags[iRole], szBotName);
        PlayTeamAudio(CS_TEAM_T, "spk radio/roger");
    }
    else
    {
        g_iTradeLeader[iCarrier] = id;
        g_flTradeFollowUntil[iCarrier] = get_gametime() + 18.0;

        BroadcastTeamCall(CS_TEAM_T, "^4[Objective]^3 %s %s^1: Bringing the C4 to your location! Hold position!", g_szRoleTags[iRole], szBotName);
        PlayTeamAudio(CS_TEAM_T, "spk radio/sticktog");
    }
    return true;
}

bool:Execute_PopFlashCall(id)
{
    new CsTeams:team = cs_get_user_team(id);
    new Float:vecPlayerOrigin[3], Float:vecBotOrigin[3];
    pev(id, pev_origin, vecPlayerOrigin);

    new iBestBot = 0;
    new Float:flBestDist = 1200.0;

    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == team)
        {
            if (user_has_weapon(i, CSW_FLASHBANG))
            {
                pev(i, pev_origin, vecBotOrigin);
                new Float:flDist = get_distance_f(vecBotOrigin, vecPlayerOrigin);
                if (g_iPlayerRole[i] == ROLE_SUPPORT)
                    flDist *= 0.65;

                if (flDist < flBestDist)
                {
                    flBestDist = flDist;
                    iBestBot = i;
                }
            }
        }
    }

    if (iBestBot > 0)
    {
        new szBotName[32];
        get_user_name(iBestBot, szBotName, charsmax(szBotName));

        new Float:vecBotEyes[3], Float:vecForward[3], Float:vecVAngle[3], Float:vecTarget[3], Float:vecAimPoint[3];
        pev(iBestBot, pev_origin, vecBotEyes);
        vecBotEyes[2] += 17.0;

        pev(id, pev_v_angle, vecVAngle);
        angle_vector(vecVAngle, ANGLEVECTOR_FORWARD, vecForward);
        xs_vec_mul_scalar(vecForward, 350.0, vecForward);
        xs_vec_add(vecPlayerOrigin, vecForward, vecTarget);
        vecTarget[2] += 180.0; // High popflash apex

        Savage_Bridge_ComputeLobAimPoint(vecBotEyes, vecTarget, -38.0, vecAimPoint);

        if (Savage_Bridge_RequestUtilityThrow(iBestBot, CSW_FLASHBANG, vecAimPoint, 0.65))
        {
            BroadcastTeamCall(team, "^4[Utility Support]^3 %s %s^1: Flashing high for you! Turn away!",
                g_szRoleTags[g_iPlayerRole[iBestBot]], szBotName);
            PlayTeamAudio(team, "spk radio/fireinhole");
        }
        return true;
    }

    new iAnyBot = GetFirstBotTeammate(team);
    if (iAnyBot > 0)
    {
        new szBotName[32];
        get_user_name(iAnyBot, szBotName, charsmax(szBotName));
        BroadcastTeamCall(team, "^4[Utility Support]^3 %s %s^1: Negative! We have no flashbangs left in reserve.",
            g_szRoleTags[g_iPlayerRole[iAnyBot]], szBotName);
        PlayTeamAudio(team, "spk radio/negative");
    }
    return true;
}

bool:Execute_SmokeCall(id)
{
    new CsTeams:team = cs_get_user_team(id);
    new Float:vecPlayerOrigin[3], Float:vecBotOrigin[3];
    pev(id, pev_origin, vecPlayerOrigin);

    new iBestBot = 0;
    new Float:flBestDist = 1400.0;

    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == team)
        {
            if (user_has_weapon(i, CSW_SMOKEGRENADE))
            {
                pev(i, pev_origin, vecBotOrigin);
                new Float:flDist = get_distance_f(vecBotOrigin, vecPlayerOrigin);
                if (g_iPlayerRole[i] == ROLE_SUPPORT)
                    flDist *= 0.65;

                if (flDist < flBestDist)
                {
                    flBestDist = flDist;
                    iBestBot = i;
                }
            }
        }
    }

    if (iBestBot > 0)
    {
        new szBotName[32];
        get_user_name(iBestBot, szBotName, charsmax(szBotName));

        new Float:vecBotEyes[3], Float:vecForward[3], Float:vecVAngle[3], Float:vecTarget[3], Float:vecAimPoint[3];
        pev(iBestBot, pev_origin, vecBotEyes);
        vecBotEyes[2] += 17.0;

        pev(id, pev_v_angle, vecVAngle);
        angle_vector(vecVAngle, ANGLEVECTOR_FORWARD, vecForward);
        xs_vec_mul_scalar(vecForward, 450.0, vecForward);
        xs_vec_add(vecPlayerOrigin, vecForward, vecTarget);

        Savage_Bridge_ComputeLobAimPoint(vecBotEyes, vecTarget, -28.0, vecAimPoint);

        if (Savage_Bridge_RequestUtilityThrow(iBestBot, CSW_SMOKEGRENADE, vecAimPoint, 0.65))
        {
            BroadcastTeamCall(team, "^4[Utility Support]^3 %s %s^1: Smoking the choke point, 3 seconds!",
                g_szRoleTags[g_iPlayerRole[iBestBot]], szBotName);
            PlayTeamAudio(team, "spk radio/roger");
        }
        return true;
    }

    new iAnyBot = GetFirstBotTeammate(team);
    if (iAnyBot > 0)
    {
        new szBotName[32];
        get_user_name(iAnyBot, szBotName, charsmax(szBotName));
        BroadcastTeamCall(team, "^4[Utility Support]^3 %s %s^1: Negative! Out of smoke grenades.",
            g_szRoleTags[g_iPlayerRole[iAnyBot]], szBotName);
        PlayTeamAudio(team, "spk radio/negative");
    }
    return true;
}

bool:Execute_TradeFragCall(id)
{
    new CsTeams:team = cs_get_user_team(id);
    new Float:vecPlayerOrigin[3], Float:vecBotOrigin[3];
    pev(id, pev_origin, vecPlayerOrigin);

    new iBestBot = 0;
    new Float:flBestDist = 1200.0;

    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == team)
        {
            pev(i, pev_origin, vecBotOrigin);
            new Float:flDist = get_distance_f(vecBotOrigin, vecPlayerOrigin);
            if (g_iPlayerRole[i] == ROLE_ENTRY || g_iPlayerRole[i] == ROLE_SUPPORT)
                flDist *= 0.70;

            if (flDist < flBestDist)
            {
                flBestDist = flDist;
                iBestBot = i;
            }
        }
    }

    if (iBestBot > 0)
    {
        g_iTradeLeader[iBestBot] = id;
        g_flTradeFollowUntil[iBestBot] = get_gametime() + 20.0;

        new szBotName[32];
        get_user_name(iBestBot, szBotName, charsmax(szBotName));

        BroadcastTeamCall(team, "^4[Trade Duo]^3 %s %s^1: Right behind you, I'll trade your contact!",
            g_szRoleTags[g_iPlayerRole[iBestBot]], szBotName);
        PlayTeamAudio(team, "spk radio/go_go_go");
        return true;
    }
    return false;
}

bool:Execute_RearFlankCall(id)
{
    new CsTeams:team = cs_get_user_team(id);
    new Float:vecPlayerOrigin[3], Float:vecBotOrigin[3];
    pev(id, pev_origin, vecPlayerOrigin);

    new iBestBot = 0;
    new Float:flBestDist = 1400.0;

    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == team)
        {
            pev(i, pev_origin, vecBotOrigin);
            new Float:flDist = get_distance_f(vecBotOrigin, vecPlayerOrigin);
            if (g_iPlayerRole[i] == ROLE_LURKER)
                flDist *= 0.60;

            if (flDist < flBestDist)
            {
                flBestDist = flDist;
                iBestBot = i;
            }
        }
    }

    if (iBestBot > 0)
    {
        g_flRearFlankUntil[iBestBot] = get_gametime() + 15.0;

        new Float:vecPlayerAngles[3];
        pev(id, pev_v_angle, vecPlayerAngles);
        vecPlayerAngles[1] += 180.0;
        if (vecPlayerAngles[1] > 180.0)
            vecPlayerAngles[1] -= 360.0;
        vecPlayerAngles[0] = 0.0;

        xs_vec_copy(vecPlayerAngles, g_vecRearFlankAngle[iBestBot]);

        new szBotName[32];
        get_user_name(iBestBot, szBotName, charsmax(szBotName));

        BroadcastTeamCall(team, "^4[Flank Anchor]^3 %s %s^1: Covering our rear flank! Holding the six.",
            g_szRoleTags[g_iPlayerRole[iBestBot]], szBotName);
        PlayTeamAudio(team, "spk radio/sticktog");
        return true;
    }
    return false;
}

bool:Execute_DefuseCoverCall(id)
{
    new CsTeams:team = cs_get_user_team(id);
    if (team != CS_TEAM_CT)
    {
        client_print_color(id, print_team_default, "^4[Objective]^1 Defuse cover is only applicable for Counter-Terrorists!");
        return true;
    }

    new Float:vecPlayerOrigin[3], Float:vecBotOrigin[3];
    pev(id, pev_origin, vecPlayerOrigin);

    new iBestBot = 0;
    new Float:flBestDist = 1200.0;

    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == CS_TEAM_CT)
        {
            pev(i, pev_origin, vecBotOrigin);
            new Float:flDist = get_distance_f(vecBotOrigin, vecPlayerOrigin);
            if (user_has_weapon(i, CSW_SMOKEGRENADE))
                flDist *= 0.60;

            if (flDist < flBestDist)
            {
                flBestDist = flDist;
                iBestBot = i;
            }
        }
    }

    if (iBestBot > 0)
    {
        new szBotName[32];
        get_user_name(iBestBot, szBotName, charsmax(szBotName));

        if (user_has_weapon(iBestBot, CSW_SMOKEGRENADE))
        {
            new Float:vecBotEyes[3], Float:vecAimPoint[3];
            pev(iBestBot, pev_origin, vecBotEyes);
            vecBotEyes[2] += 17.0;

            Savage_Bridge_ComputeLobAimPoint(vecBotEyes, vecPlayerOrigin, -25.0, vecAimPoint);

            if (Savage_Bridge_RequestUtilityThrow(iBestBot, CSW_SMOKEGRENADE, vecAimPoint, 0.65))
            {
                BroadcastTeamCall(CS_TEAM_CT, "^4[Objective]^3 %s %s^1: Laying smoke on bomb and covering your defuse!",
                    g_szRoleTags[g_iPlayerRole[iBestBot]], szBotName);
            }
            else
            {
                BroadcastTeamCall(CS_TEAM_CT, "^4[Objective]^3 %s %s^1: Covering your defuse!",
                    g_szRoleTags[g_iPlayerRole[iBestBot]], szBotName);
            }
        }
        else
        {
            BroadcastTeamCall(CS_TEAM_CT, "^4[Objective]^3 %s %s^1: Providing suppressive body-block & crossfire for your defuse!",
                g_szRoleTags[g_iPlayerRole[iBestBot]], szBotName);
        }

        g_iTradeLeader[iBestBot] = id;
        g_flTradeFollowUntil[iBestBot] = get_gametime() + 12.0;

        PlayTeamAudio(CS_TEAM_CT, "spk radio/coverme");
        return true;
    }
    return false;
}

bool:Execute_RotateCall(id, iStrat)
{
    SetPlayerStrat(id, iStrat);
    return true;
}

bool:Execute_SaveGunsCall(id)
{
    new CsTeams:team = cs_get_user_team(id);
    g_iTeamStrat[team] = STRAT_ECO;

    new iAnyBot = GetFirstBotTeammate(team);
    if (iAnyBot > 0)
    {
        new szBotName[32];
        get_user_name(iAnyBot, szBotName, charsmax(szBotName));
        BroadcastTeamCall(team, "^4[Tactical Order]^3 %s %s^1: Conceding site! Falling back to save weapons for next round.",
            g_szRoleTags[g_iPlayerRole[iAnyBot]], szBotName);
        PlayTeamAudio(team, "spk radio/fallback");
    }
    return true;
}

bool:Execute_UtilityReport(id)
{
    new CsTeams:team = cs_get_user_team(id);
    new bool:bFound = false;

    for (new b = 1; b <= 32; b++)
    {
        if (is_user_alive(b) && is_user_bot(b) && cs_get_user_team(b) == team)
        {
            new iSmokes = cs_get_user_bpammo(b, CSW_SMOKEGRENADE);
            new iFlashes = cs_get_user_bpammo(b, CSW_FLASHBANG);
            new iHE = cs_get_user_bpammo(b, CSW_HEGRENADE);
            new bool:bKit = (team == CS_TEAM_CT && cs_get_user_defuse(b));
            new iMoney = cs_get_user_money(b);
            new szBotName[32];
            get_user_name(b, szBotName, charsmax(szBotName));

            client_print_color(id, print_team_default, "^4[Utility Intel]^3 %s %s^1: %d Smoke, %d Flash, %d HE%s ($%d)",
                g_szRoleTags[g_iPlayerRole[b]], szBotName, iSmokes, iFlashes, iHE, bKit ? ", Defuse Kit" : "", iMoney);
            bFound = true;
        }
    }

    if (!bFound)
    {
        client_print_color(id, print_team_default, "^4[Utility Intel]^1 No living bot teammates to report utility.");
    }
    else
    {
        client_cmd(id, "spk radio/roger");
    }
    return true;
}

bool:Execute_EconomyReport(id)
{
    new CsTeams:team = cs_get_user_team(id);
    new iTotalMoney = 0, iCount = 0;

    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && cs_get_user_team(i) == team)
        {
            iTotalMoney += cs_get_user_money(i);
            iCount++;
        }
    }

    if (iCount <= 0) iCount = 1;
    new iAvg = iTotalMoney / iCount;

    new iIgl = GetTeamIgl(team);
    if (iIgl <= 0 || !is_user_alive(iIgl))
        iIgl = GetFirstBotTeammate(team);

    if (iIgl > 0)
    {
        new szIglName[32];
        get_user_name(iIgl, szIglName, charsmax(szIglName));
        new iRole = g_iPlayerRole[iIgl];

        if (iAvg >= 4200)
            BroadcastTeamCall(team, "^4[Economy Intel]^3 %s %s^1: Team Avg: ^4$%d^1. Recommendation: ^4FULL BUY^1 (Rifles + Full Utility)!", g_szRoleTags[iRole], szIglName, iAvg);
        else if (iAvg >= 2400)
            BroadcastTeamCall(team, "^4[Economy Intel]^3 %s %s^1: Team Avg: ^4$%d^1. Recommendation: ^3HALF BUY / FORCE^1 (Keep $2k savings buffer)!", g_szRoleTags[iRole], szIglName, iAvg);
        else
            BroadcastTeamCall(team, "^4[Economy Intel]^3 %s %s^1: Team Avg: ^4$%d^1. Recommendation: ^1FULL ECO / SAVE^1 (Preserve cash for full rifles next round)!", g_szRoleTags[iRole], szIglName, iAvg);

        PlayTeamAudio(team, "spk radio/regroup");
    }
    return true;
}

bool:Execute_ReconReport(id)
{
    new CsTeams:team = cs_get_user_team(id);
    new Float:flGameTime = get_gametime();
    new bool:bFound = false;

    for (new b = 1; b <= 32; b++)
    {
        if (is_user_alive(b) && is_user_bot(b) && cs_get_user_team(b) == team)
        {
            new Float:vecOrigin[3];
            pev(b, pev_origin, vecOrigin);

            new szLocation[32];
            if (g_bBombsitesFound && get_distance_f(vecOrigin, g_vecBombsiteA) <= 650.0)
                copy(szLocation, charsmax(szLocation), "Bombsite A");
            else if (g_bBombsitesFound && get_distance_f(vecOrigin, g_vecBombsiteB) <= 650.0)
                copy(szLocation, charsmax(szLocation), "Bombsite B");
            else if (g_bMidPointCalculated && get_distance_f(vecOrigin, g_vecMapMidPoint) <= 650.0)
                copy(szLocation, charsmax(szLocation), "Middle Corridor");
            else
                copy(szLocation, charsmax(szLocation), "Transit Chokepoint");

            new bool:bContact = (g_iSoundTarget[b] > 0 && g_flSoundTargetExpire[b] > flGameTime);
            new szBotName[32];
            get_user_name(b, szBotName, charsmax(szBotName));

            client_print_color(id, print_team_default, "^4[Recon Intel]^3 %s %s^1: Holding ^4%s^1 - %s",
                g_szRoleTags[g_iPlayerRole[b]], szBotName, szLocation,
                bContact ? "^1[Heard Enemy Steps Nearby!]" : "^3[Clear, No Immediate Contact]");
            bFound = true;
        }
    }

    if (bFound)
    {
        client_cmd(id, "spk radio/clear");
    }
    return true;
}

bool:Execute_DamageReport(id)
{
    new CsTeams:team = cs_get_user_team(id);
    new Float:flGameTime = get_gametime();
    new bool:bFound = false;

    for (new b = 1; b <= 32; b++)
    {
        if (is_user_connected(b) && is_user_bot(b) && cs_get_user_team(b) == team)
        {
            if (g_iLastDamageTarget[b] > 0 && (flGameTime - g_flLastDamageTime[b] < 12.0))
            {
                new szBotName[32], szEnemyName[32];
                get_user_name(b, szBotName, charsmax(szBotName));
                get_user_name(g_iLastDamageTarget[b], szEnemyName, charsmax(szEnemyName));

                BroadcastTeamCall(team, "^4[Damage Intel]^3 %s %s^1: Tagged ^4%s^1 for -^3%d HP^1! He is low!",
                    g_szRoleTags[g_iPlayerRole[b]], szBotName, szEnemyName, g_iLastDamageDealt[b]);
                bFound = true;
            }
        }
    }

    if (!bFound)
    {
        new iBot = GetFirstBotTeammate(team);
        if (iBot > 0)
        {
            new szBotName[32];
            get_user_name(iBot, szBotName, charsmax(szBotName));
            BroadcastTeamCall(team, "^4[Damage Intel]^3 %s %s^1: No recent combat damage dealt to enemies.",
                g_szRoleTags[g_iPlayerRole[iBot]], szBotName);
        }
    }

    PlayTeamAudio(team, "spk radio/reportin");
    return true;
}

bool:Execute_SilentWalkCall(id)
{
    new CsTeams:team = cs_get_user_team(id);
    g_flSilentWalkUntil[team] = get_gametime() + 12.0;

    new iAnyBot = GetFirstBotTeammate(team);
    if (iAnyBot > 0)
    {
        new szBotName[32];
        get_user_name(iAnyBot, szBotName, charsmax(szBotName));
        BroadcastTeamCall(team, "^4[Squad Pacing]^3 %s %s^1: Going silent. Shift walking contact!",
            g_szRoleTags[g_iPlayerRole[iAnyBot]], szBotName);
        PlayTeamAudio(team, "spk radio/roger");
    }
    return true;
}

bool:Execute_HoldAngleCall(id)
{
    new CsTeams:team = cs_get_user_team(id);
    g_flHoldAngleUntil[team] = get_gametime() + 15.0;

    new iAnyBot = GetFirstBotTeammate(team);
    if (iAnyBot > 0)
    {
        new szBotName[32];
        get_user_name(iAnyBot, szBotName, charsmax(szBotName));
        BroadcastTeamCall(team, "^4[Squad Pacing]^3 %s %s^1: Holding passive off-angles. Waiting for their peek!",
            g_szRoleTags[g_iPlayerRole[iAnyBot]], szBotName);
        PlayTeamAudio(team, "spk radio/sticktog");
    }
    return true;
}

bool:ProcessTacticalChatCall(id, const szMessage[])
{
    new Float:flGameTime = get_gametime();
    new Float:flCooldown = get_pcvar_float(g_pCvarChatCooldown);
    if (flCooldown < 0.5) flCooldown = 0.5;

    // Anti-spam cooldown per player
    if (flGameTime - g_flLastChatCallTime[id] < flCooldown)
        return false;

    new CsTeams:team = cs_get_user_team(id);
    if (team != CS_TEAM_T && team != CS_TEAM_CT)
        return false;

    // -------------------------------------------------------------
    // 1. CARE & DANGER ZONE WARNINGS ("care [zone]")
    // -------------------------------------------------------------
    if (containi(szMessage, "care") != -1 || containi(szMessage, "awas") != -1 || containi(szMessage, "hati-hati") != -1 || containi(szMessage, "watch out") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        if (containi(szMessage, "short") != -1 || containi(szMessage, "cat") != -1)
            return Execute_CareZoneCall(id, ZONE_SHORT, "Short");
        else if (containi(szMessage, "long") != -1 || containi(szMessage, "pit") != -1)
            return Execute_CareZoneCall(id, ZONE_LONG, "Long A");
        else if (containi(szMessage, "mid") != -1 || containi(szMessage, "tengah") != -1)
            return Execute_CareZoneCall(id, ZONE_MID, "Middle");
        else if (containi(szMessage, "flank") != -1 || containi(szMessage, "behind") != -1 || containi(szMessage, "back") != -1 || containi(szMessage, "bokong") != -1 || containi(szMessage, "belakang") != -1)
            return Execute_CareZoneCall(id, ZONE_FLANK, "Rear Flank");
        else if (containi(szMessage, "awp") != -1 || containi(szMessage, "sniper") != -1)
            return Execute_CareZoneCall(id, ZONE_AWP, "Enemy Sniper");
        else if (containi(szMessage, "push") != -1 || containi(szMessage, "rush") != -1 || containi(szMessage, "maju") != -1)
            return Execute_CareZoneCall(id, ZONE_PUSH, "Aggressive Push");
        else if (containi(szMessage, "site a") != -1 || containi(szMessage, " a") != -1)
            return Execute_CareZoneCall(id, ZONE_SITE_A, "Bombsite A");
        else if (containi(szMessage, "site b") != -1 || containi(szMessage, " b") != -1)
            return Execute_CareZoneCall(id, ZONE_SITE_B, "Bombsite B");
        else
            return Execute_CareZoneCall(id, ZONE_MID, "Target Sector");
    }

    // -------------------------------------------------------------
    // 2. C4 / BOMB MANAGEMENT (Drop C4, pass bomb)
    // -------------------------------------------------------------
    if (containi(szMessage, "drop c4") != -1 || containi(szMessage, "drop bomb") != -1 || 
        containi(szMessage, "give bomb") != -1 || containi(szMessage, "bagi c4") != -1 || 
        containi(szMessage, "bagi bom") != -1 || containi(szMessage, "sini bom") != -1 ||
        containi(szMessage, "kasih bom") != -1 || containi(szMessage, "pass bomb") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_DropC4Call(id);
    }

    // -------------------------------------------------------------
    // 3. UTILITY ASSISTANCE (Pop-Flash, Smoke)
    // -------------------------------------------------------------
    if (containi(szMessage, "flash me") != -1 || containi(szMessage, "popflash") != -1 || 
        containi(szMessage, "minta flash") != -1 || containi(szMessage, "lempar flash") != -1 || 
        containi(szMessage, "flash bang") != -1 || equali(szMessage, "flash") || equali(szMessage, "/flash"))
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_PopFlashCall(id);
    }

    if (containi(szMessage, "smoke") != -1 || containi(szMessage, "tutup asap") != -1 || 
        containi(szMessage, "asap sini") != -1 || containi(szMessage, "lempar smoke") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_SmokeCall(id);
    }

    // -------------------------------------------------------------
    // 4. COMBAT COORDINATION (Trade-Frag, Rear Flank)
    // -------------------------------------------------------------
    if (containi(szMessage, "trade") != -1 || containi(szMessage, "push with me") != -1 || 
        containi(szMessage, "double peek") != -1 || containi(szMessage, "ikut aku") != -1 || 
        containi(szMessage, "maju bareng") != -1 || containi(szMessage, "backup me") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_TradeFragCall(id);
    }

    if (containi(szMessage, "watch back") != -1 || containi(szMessage, "watch flank") != -1 || 
        containi(szMessage, "cover rear") != -1 || containi(szMessage, "jaga belakang") != -1 || 
        containi(szMessage, "pantau bokong") != -1 || containi(szMessage, "tengok belakang") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_RearFlankCall(id);
    }

    // -------------------------------------------------------------
    // 5. OBJECTIVE DEFENSE & RETAKE (Defuse Cover, Rotations, Save)
    // -------------------------------------------------------------
    if (containi(szMessage, "cover defuse") != -1 || containi(szMessage, "stick defuse") != -1 || 
        containi(szMessage, "tutup defuse") != -1 || containi(szMessage, "lindungin defuse") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_DefuseCoverCall(id);
    }

    if (containi(szMessage, "rotate a") != -1 || containi(szMessage, "rotasi a") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_RotateCall(id, STRAT_RUSH_A);
    }
    if (containi(szMessage, "rotate b") != -1 || containi(szMessage, "rotasi b") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_RotateCall(id, STRAT_RUSH_B);
    }

    if (containi(szMessage, "save gun") != -1 || containi(szMessage, "save senjata") != -1 || 
        equali(szMessage, "save") || equali(szMessage, "/save") || containi(szMessage, "fallback") != -1)
    {
        if (g_bInFreezeTime)
        {
            Cmd_SetTeamEconomy(id, ECON_ECO, false);
            return true;
        }
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_SaveGunsCall(id);
    }

    // -------------------------------------------------------------
    // 6. SQUAD INTEL & STATUS QUERIES (Utility, Economy, Recon, Damage)
    // -------------------------------------------------------------
    if (containi(szMessage, "utility?") != -1 || containi(szMessage, "nades?") != -1 || 
        containi(szMessage, "granat?") != -1 || containi(szMessage, "ada flash?") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_UtilityReport(id);
    }

    if (containi(szMessage, "money?") != -1 || containi(szMessage, "cash?") != -1 || 
        containi(szMessage, "duit?") != -1 || containi(szMessage, "eco apa buy?") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_EconomyReport(id);
    }

    if (containi(szMessage, "status?") != -1 || containi(szMessage, "where?") != -1 || 
        containi(szMessage, "posisi?") != -1 || containi(szMessage, "clear?") != -1 || 
        containi(szMessage, "laporan?") != -1 || containi(szMessage, "aman ga?") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_ReconReport(id);
    }

    if (containi(szMessage, "damage?") != -1 || containi(szMessage, "hp?") != -1 || 
        containi(szMessage, "musuh kena berapa?") != -1 || containi(szMessage, "hit berapa?") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_DamageReport(id);
    }

    // -------------------------------------------------------------
    // 7. SQUAD PACING & STEALTH (Walk, Shift, Hold, Play Slow)
    // -------------------------------------------------------------
    if (containi(szMessage, "walk") != -1 || containi(szMessage, "shift") != -1 || 
        containi(szMessage, "quiet") != -1 || containi(szMessage, "jalan pelan") != -1 || 
        containi(szMessage, "step") != -1 || containi(szMessage, "jangan lari") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_SilentWalkCall(id);
    }

    if (containi(szMessage, "hold") != -1 || containi(szMessage, "play slow") != -1 || 
        containi(szMessage, "wait for pick") != -1 || containi(szMessage, "tahan") != -1 || 
        containi(szMessage, "jangan intip") != -1)
    {
        g_flLastChatCallTime[id] = flGameTime;
        return Execute_HoldAngleCall(id);
    }

    return false;
}
