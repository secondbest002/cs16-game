#if defined _savage_hazard_ai_included
    #endinput
#endif
#define _savage_hazard_ai_included

// ==============================================================================
// Molotov Fire Zone Awareness & Tactical Smoke-Extinguish Engine (BP-02)
// Evaluates active Molotov fire zones via cs2_api_get_active_molotov* natives:
//   1. Evacuates endangered bots via YaPB path graph + LEASE_HAZARD_EVADE
//   2. Assigns Support/Entry bots to extinguish blocking enemy fires with Smoke
// ==============================================================================

#define HAZARD_MAX_FIRES                16
#define HAZARD_EVADE_MARGIN             48.0
#define HAZARD_SAFE_CLEARANCE           72.0
#define HAZARD_SMOKE_CHECK_RADIUS       200.0
#define HAZARD_MIN_REM_FOR_SMOKE        2.5
#define HAZARD_MIN_SMOKE_THROW_DIST     180.0
#define HAZARD_MAX_SMOKE_THROW_DIST     950.0

new Float:g_vecClaimedFireOrigin[HAZARD_MAX_FIRES][3];
new Float:g_flClaimedFireExpire[HAZARD_MAX_FIRES];
new g_iClaimedFireTeam[HAZARD_MAX_FIRES];

stock Savage_Hazard_ResetRound()
{
    for (new i = 0; i < HAZARD_MAX_FIRES; i++)
    {
        g_flClaimedFireExpire[i] = 0.0;
        g_iClaimedFireTeam[i] = 0;
        g_vecClaimedFireOrigin[i][0] = 0.0;
        g_vecClaimedFireOrigin[i][1] = 0.0;
        g_vecClaimedFireOrigin[i][2] = 0.0;
    }
}

public Task_HazardMonitor(taskid)
{
    if (!get_pcvar_num(g_pCvarEnable))
        return;

    if (g_bInFreezeTime && !IsWarmupActive())
        return;

    new iRawCount = cs2_api_get_active_molotov_count();
    if (iRawCount <= 0)
        return;

    new Float:flGameTime = get_gametime();

    new Float:vecFireOrigin[HAZARD_MAX_FIRES][3];
    new Float:flFireRadius[HAZARD_MAX_FIRES];
    new iFireTeam[HAZARD_MAX_FIRES];
    new Float:flFireRem[HAZARD_MAX_FIRES];
    new iValidFires = 0;

    new iLimit = (iRawCount > HAZARD_MAX_FIRES) ? HAZARD_MAX_FIRES : iRawCount;
    for (new idx = 0; idx < iLimit; idx++)
    {
        new Float:vecTmp[3], Float:flRad, iTm, Float:flRem;
        if (cs2_api_get_active_molotov(idx, vecTmp, flRad, iTm, flRem))
        {
            xs_vec_copy(vecTmp, vecFireOrigin[iValidFires]);
            flFireRadius[iValidFires] = (flRad > 32.0) ? flRad : 150.0;
            iFireTeam[iValidFires] = iTm;
            flFireRem[iValidFires] = flRem;
            iValidFires++;
        }
    }

    if (iValidFires <= 0)
        return;

    // --------------------------------------------------------------------------
    // 1. Per-Bot Molotov Hazard Detection & Path Evasion (LEASE_HAZARD_EVADE)
    // --------------------------------------------------------------------------
    for (new id = 1; id <= MaxClients; id++)
    {
        if (!is_user_alive(id) || !is_user_bot(id))
            continue;

        new Float:vecBotOrigin[3];
        pev(id, pev_origin, vecBotOrigin);

        // Anti-oscillation guard: release lease once safely clear, otherwise keep current target
        if (g_iBotLeaseType[id] == LEASE_HAZARD_EVADE)
        {
            if (Hazard_IsPointSafeFromAllFires(vecBotOrigin, vecFireOrigin, flFireRadius, iValidFires, HAZARD_SAFE_CLEARANCE))
            {
                Savage_Bridge_ReleaseLease(id);
            }
            else if (flGameTime < g_flBotLeaseUntil[id])
            {
                continue;
            }
        }

        new iDangerFire = Hazard_FindEndangeringFire(id, vecBotOrigin, vecFireOrigin, flFireRadius, iValidFires);
        if (iDangerFire != -1)
        {
            new Float:vecSafeOrigin[3];
            if (Hazard_FindSafeEvadeOrigin(id, vecBotOrigin, iDangerFire, vecFireOrigin, flFireRadius, iValidFires, vecSafeOrigin))
            {
                Savage_Bridge_RequestEvade(id, vecSafeOrigin, 1.25);
            }
        }
    }

    // --------------------------------------------------------------------------
    // 2. Tactical Smoke-Extinguish Reflex for Blocking Enemy Molotovs
    // --------------------------------------------------------------------------
    for (new f = 0; f < iValidFires; f++)
    {
        if (flFireRem[f] < HAZARD_MIN_REM_FOR_SMOKE)
            continue;

        if (Hazard_HasActiveSmokeNear(vecFireOrigin[f], HAZARD_SMOKE_CHECK_RADIUS))
            continue;

        Hazard_TryTeamSmokeExtinguish(CS_TEAM_T, f, vecFireOrigin[f], flFireRadius[f], iFireTeam[f], flGameTime);
        Hazard_TryTeamSmokeExtinguish(CS_TEAM_CT, f, vecFireOrigin[f], flFireRadius[f], iFireTeam[f], flGameTime);
    }
}

stock Hazard_FindEndangeringFire(id, const Float:vecBotOrigin[3], const Float:vecFireOrigin[][3], const Float:flFireRadius[], iFireCount)
{
    new bool:bYapbBot = (g_bYapbAvailable && yb_is_user_bot(id));
    new iPathLen = bYapbBot ? yb_get_bot_path_length(id) : 0;
    new iCheckNodes = (iPathLen > 3) ? 3 : iPathLen;

    for (new f = 0; f < iFireCount; f++)
    {
        new Float:flDangerRad = flFireRadius[f] + HAZARD_EVADE_MARGIN;

        // Check bot's current position
        if (floatabs(vecBotOrigin[2] - vecFireOrigin[f][2]) <= 110.0)
        {
            if (get_distance_f(vecBotOrigin, vecFireOrigin[f]) <= flDangerRad)
                return f;
        }

        // Check next 3 YaPB path nodes (offset 0..2)
        if (iCheckNodes > 0)
        {
            for (new offset = 0; offset < iCheckNodes; offset++)
            {
                new iNode = yb_get_bot_path_node(id, offset);
                if (iNode == YB_INVALID_NODE_INDEX)
                    continue;

                new Float:vecNodeOrigin[3];
                if (yb_get_node_origin(iNode, vecNodeOrigin))
                {
                    if (floatabs(vecNodeOrigin[2] - vecFireOrigin[f][2]) <= 110.0
                        && get_distance_f(vecNodeOrigin, vecFireOrigin[f]) <= flDangerRad)
                    {
                        return f;
                    }
                }
            }
        }
    }

    return -1;
}

stock bool:Hazard_IsPointSafeFromAllFires(const Float:vecPoint[3], const Float:vecFireOrigin[][3], const Float:flFireRadius[], iFireCount, Float:flMargin)
{
    for (new f = 0; f < iFireCount; f++)
    {
        if (floatabs(vecPoint[2] - vecFireOrigin[f][2]) <= 110.0)
        {
            if (get_distance_f(vecPoint, vecFireOrigin[f]) <= flFireRadius[f] + flMargin)
                return false;
        }
    }
    return true;
}

stock bool:Hazard_FindSafeEvadeOrigin(id, const Float:vecBotOrigin[3], iDangerFire, const Float:vecFireOrigin[][3], const Float:flFireRadius[], iFireCount, Float:vecOutSafe[3])
{
    new Float:vecAway[3];
    vecAway[0] = vecBotOrigin[0] - vecFireOrigin[iDangerFire][0];
    vecAway[1] = vecBotOrigin[1] - vecFireOrigin[iDangerFire][1];
    vecAway[2] = 0.0;

    new Float:flLen = floatsqroot(vecAway[0] * vecAway[0] + vecAway[1] * vecAway[1]);
    if (flLen > 1.0)
    {
        vecAway[0] /= flLen;
        vecAway[1] /= flLen;
    }
    else
    {
        new Float:vecAngles[3], Float:vecFwd[3];
        pev(id, pev_v_angle, vecAngles);
        angle_vector(vecAngles, ANGLEVECTOR_FORWARD, vecFwd);
        vecAway[0] = -vecFwd[0];
        vecAway[1] = -vecFwd[1];
    }

    // 1. Prefer reachable YaPB graph nodes outside all fire radii
    if (g_bYapbAvailable && yb_is_user_bot(id) && yb_has_graph())
    {
        new iCurNode = yb_get_bot_node(id);
        if (iCurNode == YB_INVALID_NODE_INDEX)
            iCurNode = yb_get_nearest_node(vecBotOrigin);

        new iBestNode = YB_INVALID_NODE_INDEX;
        new Float:flBestScore = -999999.0;
        new Float:vecBestNodeOrigin[3];

        // Evaluate outgoing links from current node first
        if (iCurNode != YB_INVALID_NODE_INDEX && yb_is_node_valid(iCurNode))
        {
            new iLinks = yb_get_node_link_count(iCurNode);
            for (new k = 0; k < iLinks; k++)
            {
                new iLinkNode = yb_get_node_link(iCurNode, k);
                if (iLinkNode == YB_INVALID_NODE_INDEX || !yb_is_node_valid(iLinkNode))
                    continue;

                new Float:vecCand[3];
                if (!yb_get_node_origin(iLinkNode, vecCand))
                    continue;

                if (!Hazard_IsPointSafeFromAllFires(vecCand, vecFireOrigin, flFireRadius, iFireCount, HAZARD_SAFE_CLEARANCE))
                    continue;

                new Float:flDist = get_distance_f(vecBotOrigin, vecCand);
                new Float:flAlign = ((vecCand[0] - vecBotOrigin[0]) * vecAway[0] + (vecCand[1] - vecBotOrigin[1]) * vecAway[1]);
                new Float:flScore = flAlign - floatabs(flDist - 260.0) * 0.35;

                if (flScore > flBestScore)
                {
                    flBestScore = flScore;
                    iBestNode = iLinkNode;
                    xs_vec_copy(vecCand, vecBestNodeOrigin);
                }
            }
        }

        // Probe directional & flanking graph nodes in 220..420 unit radius
        static const Float:flProbeDist[3] = { 240.0, 340.0, 280.0 };
        static const Float:flProbeAngle[3] = { 0.0, 0.0, 55.0 };

        for (new p = 0; p < 4; p++)
        {
            new Float:flDistStep = (p < 3) ? flProbeDist[p] : 280.0;
            new Float:flYawDeg = (p < 3) ? flProbeAngle[p] : -55.0;
            new Float:flRad = flYawDeg * 0.01745329;
            new Float:flCos = floatcos(flRad);
            new Float:flSin = floatsin(flRad);

            new Float:vecProbe[3];
            vecProbe[0] = vecBotOrigin[0] + (vecAway[0] * flCos - vecAway[1] * flSin) * flDistStep;
            vecProbe[1] = vecBotOrigin[1] + (vecAway[0] * flSin + vecAway[1] * flCos) * flDistStep;
            vecProbe[2] = vecBotOrigin[2];

            new iCandNode = yb_find_nearest_node_in_radius(vecProbe, 180.0);
            if (iCandNode == YB_INVALID_NODE_INDEX || !yb_is_node_valid(iCandNode))
                continue;

            if (iCurNode != YB_INVALID_NODE_INDEX && !yb_is_node_reachable(iCurNode, iCandNode))
                continue;

            new Float:vecCand[3];
            if (!yb_get_node_origin(iCandNode, vecCand))
                continue;

            if (!Hazard_IsPointSafeFromAllFires(vecCand, vecFireOrigin, flFireRadius, iFireCount, HAZARD_SAFE_CLEARANCE))
                continue;

            new Float:flDist = get_distance_f(vecBotOrigin, vecCand);
            if (flDist < 120.0 || flDist > 460.0)
                continue;

            new Float:flAlign = ((vecCand[0] - vecBotOrigin[0]) * vecAway[0] + (vecCand[1] - vecBotOrigin[1]) * vecAway[1]);
            new Float:flScore = flAlign - floatabs(flDist - 280.0) * 0.35;

            if (flScore > flBestScore)
            {
                flBestScore = flScore;
                iBestNode = iCandNode;
                xs_vec_copy(vecCand, vecBestNodeOrigin);
            }
        }

        if (iBestNode != YB_INVALID_NODE_INDEX)
        {
            xs_vec_copy(vecBestNodeOrigin, vecOutSafe);
            return true;
        }
    }

    // 2. Trace-validated geometric fallback when map has no graph or isolated corner
    static const Float:flFallbackYaw[5] = { 0.0, 45.0, -45.0, 90.0, -90.0 };
    new Float:flStepDist = flFireRadius[iDangerFire] + 96.0;

    new Float:vecStart[3];
    xs_vec_copy(vecBotOrigin, vecStart);
    vecStart[2] += 18.0;

    for (new d = 0; d < 5; d++)
    {
        new Float:flRad = flFallbackYaw[d] * 0.01745329;
        new Float:flCos = floatcos(flRad);
        new Float:flSin = floatsin(flRad);

        new Float:vecEnd[3];
        vecEnd[0] = vecStart[0] + (vecAway[0] * flCos - vecAway[1] * flSin) * flStepDist;
        vecEnd[1] = vecStart[1] + (vecAway[0] * flSin + vecAway[1] * flCos) * flStepDist;
        vecEnd[2] = vecStart[2];

        engfunc(EngFunc_TraceHull, vecStart, vecEnd, IGNORE_MONSTERS, HULL_HUMAN, id, 0);

        new Float:flFraction;
        get_tr2(0, TR_flFraction, flFraction);
        if (flFraction >= 0.70)
        {
            new Float:vecHit[3];
            get_tr2(0, TR_vecEndPos, vecHit);
            vecHit[2] = vecBotOrigin[2];

            if (Hazard_IsPointSafeFromAllFires(vecHit, vecFireOrigin, flFireRadius, iFireCount, HAZARD_EVADE_MARGIN))
            {
                xs_vec_copy(vecHit, vecOutSafe);
                return true;
            }
        }
    }

    return false;
}

stock bool:Hazard_HasActiveSmokeNear(const Float:vecFireOrigin[3], Float:flCheckRadius)
{
    new ent = -1;
    new szModel[64];

    while ((ent = engfunc(EngFunc_FindEntityInSphere, ent, vecFireOrigin, flCheckRadius)) > 0)
    {
        if (!pev_valid(ent))
            continue;

        new szClass[16];
        pev(ent, pev_classname, szClass, charsmax(szClass));
        if (!equal(szClass, "grenade"))
            continue;

        pev(ent, pev_model, szModel, charsmax(szModel));
        if (containi(szModel, "smokegrenade") != -1)
            return true;
    }

    return false;
}

stock bool:Hazard_IsFireClaimedByTeam(const Float:vecFireOrigin[3], iTeam, Float:flGameTime)
{
    for (new i = 0; i < HAZARD_MAX_FIRES; i++)
    {
        if (g_flClaimedFireExpire[i] > flGameTime && g_iClaimedFireTeam[i] == iTeam)
        {
            if (get_distance_f(vecFireOrigin, g_vecClaimedFireOrigin[i]) <= 180.0)
                return true;
        }
    }
    return false;
}

stock Hazard_ClaimFireForTeam(const Float:vecFireOrigin[3], iTeam, Float:flExpireTime)
{
    new Float:flNow = get_gametime();
    new iSlot = 0;
    new Float:flEarliest = 999999999.0;

    for (new i = 0; i < HAZARD_MAX_FIRES; i++)
    {
        if (g_flClaimedFireExpire[i] <= flNow)
        {
            iSlot = i;
            break;
        }
        if (g_flClaimedFireExpire[i] < flEarliest)
        {
            flEarliest = g_flClaimedFireExpire[i];
            iSlot = i;
        }
    }

    xs_vec_copy(vecFireOrigin, g_vecClaimedFireOrigin[iSlot]);
    g_flClaimedFireExpire[iSlot] = flExpireTime;
    g_iClaimedFireTeam[iSlot] = iTeam;
}

stock Hazard_TryTeamSmokeExtinguish(CsTeams:team, iFireIdx, const Float:vecFireOrigin[3], Float:flFireRadius, iFireTeam, Float:flGameTime)
{
    #pragma unused iFireIdx

    if (Hazard_IsFireClaimedByTeam(vecFireOrigin, _:team, flGameTime))
        return;

    // Only extinguish enemy Molotovs unless friendly fire is trapping our C4 carrier
    new bool:bEnemyFire = (iFireTeam != _:team);
    new bool:bTrappingCarrier = false;

    if (!bEnemyFire && team == CS_TEAM_T)
    {
        for (new p = 1; p <= MaxClients; p++)
        {
            if (is_user_alive(p) && cs_get_user_team(p) == CS_TEAM_T && user_has_weapon(p, CSW_C4))
            {
                new Float:vecCarrier[3];
                pev(p, pev_origin, vecCarrier);
                if (get_distance_f(vecCarrier, vecFireOrigin) <= flFireRadius + 64.0)
                {
                    bTrappingCarrier = true;
                    break;
                }
            }
        }
    }

    if (!bEnemyFire && !bTrappingCarrier)
        return;

    new iBestBot = 0;
    new Float:flBestScore = 999999.0;

    for (new id = 1; id <= MaxClients; id++)
    {
        if (!is_user_alive(id) || !is_user_bot(id) || cs_get_user_team(id) != team)
            continue;

        if (!user_has_weapon(id, CSW_SMOKEGRENADE))
            continue;

        if (g_iBotLeaseType[id] != LEASE_NONE)
            continue;

        if (Savage_Bridge_HasActiveEnemy(id))
            continue;

        new Float:vecBotOrigin[3];
        pev(id, pev_origin, vecBotOrigin);

        new Float:flDist = get_distance_f(vecBotOrigin, vecFireOrigin);
        if (flDist < HAZARD_MIN_SMOKE_THROW_DIST || flDist > HAZARD_MAX_SMOKE_THROW_DIST)
            continue;

        // Ensure fire is actually blocking this bot's movement or upcoming path (never waste smoke behind)
        if (!Hazard_IsFireBlockingBotAdvance(id, vecBotOrigin, vecFireOrigin, flFireRadius))
            continue;

        // Verify clear throw trajectory from bot eyes to fire center
        new Float:vecEyes[3], Float:vecTarget[3];
        xs_vec_copy(vecBotOrigin, vecEyes);
        vecEyes[2] += 17.0;

        xs_vec_copy(vecFireOrigin, vecTarget);
        vecTarget[2] += 24.0;

        engfunc(EngFunc_TraceLine, vecEyes, vecTarget, IGNORE_MONSTERS, id, 0);
        new Float:flFraction;
        get_tr2(0, TR_flFraction, flFraction);
        if (flFraction < 0.85)
            continue;

        // Role-weighted selection: prioritize Support, then Entry / IGL
        new Float:flScore = flDist;
        new iRole = g_iPlayerRole[id];
        if (iRole == ROLE_SUPPORT)
            flScore -= 250.0;
        else if (iRole == ROLE_ENTRY || iRole == ROLE_IGL)
            flScore -= 100.0;

        if (flScore < flBestScore)
        {
            flBestScore = flScore;
            iBestBot = id;
        }
    }

    if (iBestBot <= 0)
        return;

    new Float:vecBotEyes[3], Float:vecAimPoint[3];
    pev(iBestBot, pev_origin, vecBotEyes);
    vecBotEyes[2] += 17.0;

    new Float:flThrowDist = get_distance_f(vecBotEyes, vecFireOrigin);
    if (flThrowDist > 480.0)
    {
        Savage_Bridge_ComputeLobAimPoint(vecBotEyes, vecFireOrigin, -14.0, vecAimPoint);
    }
    else
    {
        xs_vec_copy(vecFireOrigin, vecAimPoint);
        vecAimPoint[2] += 12.0;
    }

    if (Savage_Bridge_RequestUtilityThrow(iBestBot, CSW_SMOKEGRENADE, vecAimPoint, 0.65))
    {
        Hazard_ClaimFireForTeam(vecFireOrigin, _:team, flGameTime + 3.2);

        new szBotName[32];
        get_user_name(iBestBot, szBotName, charsmax(szBotName));

        BroadcastTeamCall(team, "^4[Utility Support]^3 %s %s^1: Extinguishing Molotov with smoke!",
            g_szRoleTags[g_iPlayerRole[iBestBot]], szBotName);
        PlayTeamAudio(team, "spk radio/fireinhole");
    }
}

stock bool:Hazard_IsFireBlockingBotAdvance(id, const Float:vecBotOrigin[3], const Float:vecFireOrigin[3], Float:flFireRadius)
{
    new Float:vecToFire[3];
    vecToFire[0] = vecFireOrigin[0] - vecBotOrigin[0];
    vecToFire[1] = vecFireOrigin[1] - vecBotOrigin[1];
    vecToFire[2] = 0.0;

    new Float:flLen = floatsqroot(vecToFire[0] * vecToFire[0] + vecToFire[1] * vecToFire[1]);
    if (flLen > 1.0)
    {
        vecToFire[0] /= flLen;
        vecToFire[1] /= flLen;

        new Float:vecVel[3];
        pev(id, pev_velocity, vecVel);
        new Float:flSpeed = floatsqroot(vecVel[0] * vecVel[0] + vecVel[1] * vecVel[1]);
        if (flSpeed > 40.0)
        {
            new Float:flDot = (vecVel[0] * vecToFire[0] + vecVel[1] * vecToFire[1]) / flSpeed;
            if (flDot > 0.25)
                return true;
        }
    }

    // Also check if upcoming YaPB path nodes (0..4) pass near the fire zone
    if (g_bYapbAvailable && yb_is_user_bot(id))
    {
        new iPathLen = yb_get_bot_path_length(id);
        new iCheckNodes = (iPathLen > 5) ? 5 : iPathLen;
        for (new offset = 0; offset < iCheckNodes; offset++)
        {
            new iNode = yb_get_bot_path_node(id, offset);
            if (iNode == YB_INVALID_NODE_INDEX)
                continue;

            new Float:vecNode[3];
            if (yb_get_node_origin(iNode, vecNode))
            {
                if (floatabs(vecNode[2] - vecFireOrigin[2]) <= 110.0
                    && get_distance_f(vecNode, vecFireOrigin) <= flFireRadius + 96.0)
                {
                    return true;
                }
            }
        }
    }

    return false;
}
