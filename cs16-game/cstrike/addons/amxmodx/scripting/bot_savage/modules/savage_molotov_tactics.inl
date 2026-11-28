#if defined _savage_molotov_tactics_included
    #endinput
#endif
#define _savage_molotov_tactics_included

// ==============================================================================
// Map-Agnostic Tactical Molotov Evaluator & Ballistic Solver (BP-03)
// Uses YaPB navigation graph metadata (NARROW, CAMP, SNIPER, GOAL) & C4/bombsite
// entities to execute:
//   1. Post-Plant / Retake Denial (burn C4 defuse zone or post-plant camp spots)
//   2. Site Execute Corner Clear (flush CAMP/SNIPER defenders on bombsite entry)
//   3. Chokepoint Delay / Threat Denial (deny narrow choke pushes early/mid round)
// ==============================================================================

#define MOLO_MAX_ACTIVE_FIRES       16
#define MOLO_TEAM_COOLDOWN          4.0
#define MOLO_ACTIVE_FIRE_GUARD      260.0
#define MOLO_TEAMMATE_SAFE_RADIUS   220.0
#define MOLO_POSTPLANT_RANGE        950.0
#define MOLO_SITE_EXEC_RANGE        900.0
#define MOLO_SITE_SEARCH_RADIUS     420.0
#define MOLO_CHOKE_MIN_RANGE        350.0
#define MOLO_CHOKE_MAX_RANGE        850.0
#define MOLO_MIN_THROW_DIST         210.0
#define MOLO_MAX_THROW_DIST         920.0

enum MolotovTacticType
{
    MOLO_TACTIC_NONE = 0,
    MOLO_TACTIC_POSTPLANT,
    MOLO_TACTIC_SITE_CLEAR,
    MOLO_TACTIC_CHOKE_DELAY
};

stock Savage_MolotovTactics_ResetRound()
{
    g_flNextTeamMolotovTime[CS_TEAM_T] = 0.0;
    g_flNextTeamMolotovTime[CS_TEAM_CT] = 0.0;
}

public Task_TacticalMolotovScanner(taskid)
{
    if (!get_pcvar_num(g_pCvarEnable))
        return;

    if (g_bInFreezeTime || IsWarmupActive() || IsKnifeRoundActive() || IsDeathmatchActive())
        return;

    if (!cs2_api_has_molotov())
        return;

    if (!g_bYapbAvailable || !yb_has_graph())
        return;

    new iNodeCount = yb_get_node_count();
    if (iNodeCount <= 0)
        return;

    new Float:flGameTime = get_gametime();
    if (flGameTime < g_flNextTeamMolotovTime[CS_TEAM_T] && flGameTime < g_flNextTeamMolotovTime[CS_TEAM_CT])
        return;

    // Snapshot active Molotov fires once so no bot throws onto an already burning zone
    new Float:vecActiveFires[MOLO_MAX_ACTIVE_FIRES][3];
    new iActiveFireCount = 0;
    new iRawFires = cs2_api_get_active_molotov_count();
    if (iRawFires > 0)
    {
        new iLimit = (iRawFires > MOLO_MAX_ACTIVE_FIRES) ? MOLO_MAX_ACTIVE_FIRES : iRawFires;
        for (new idx = 0; idx < iLimit; idx++)
        {
            new Float:vecTmp[3], Float:flRad, iTm, Float:flRem;
            if (cs2_api_get_active_molotov(idx, vecTmp, flRad, iTm, flRem))
            {
                xs_vec_copy(vecTmp, vecActiveFires[iActiveFireCount++]);
            }
        }
    }

    // Detect planted C4 location if active
    new Float:vecC4Origin[3];
    new bool:bBombPlanted = MolotovTactics_FindPlantedC4(vecC4Origin);
    new Float:flRoundElapsed = flGameTime - g_flRoundStartTime;

    for (new id = 1; id <= MaxClients; id++)
    {
        if (!is_user_alive(id) || !is_user_bot(id))
            continue;

        new CsTeams:team = cs_get_user_team(id);
        if (team != CS_TEAM_T && team != CS_TEAM_CT)
            continue;

        if (flGameTime < g_flNextTeamMolotovTime[team])
            continue;

        if (!cs2_api_get_user_molotov(id) || !user_has_weapon(id, CSW_HEGRENADE))
            continue;

        if (g_iBotLeaseType[id] != LEASE_NONE)
            continue;

        if (Savage_Bridge_HasActiveEnemy(id) || (flGameTime - g_flLastCombatTime[id] < 2.0))
            continue;

        new Float:vecBotOrigin[3];
        pev(id, pev_origin, vecBotOrigin);

        new Float:vecTarget[3], Float:vecAimPoint[3];
        new MolotovTacticType:eTactic = MolotovTactics_FindBestTarget(
            id, team, vecBotOrigin, flGameTime, flRoundElapsed,
            bBombPlanted, vecC4Origin, vecActiveFires, iActiveFireCount,
            iNodeCount, vecTarget, vecAimPoint
        );

        if (eTactic == MOLO_TACTIC_NONE)
            continue;

        if (Savage_Bridge_RequestUtilityThrow(id, CSW_HEGRENADE, vecAimPoint, 0.65))
        {
            g_flNextTeamMolotovTime[team] = flGameTime + MOLO_TEAM_COOLDOWN;

            new szBotName[32];
            get_user_name(id, szBotName, charsmax(szBotName));

            switch (eTactic)
            {
                case MOLO_TACTIC_POSTPLANT:
                {
                    BroadcastTeamCall(team, "^4[Utility]^3 %s %s^1: Molotov out — burning bomb zone!",
                        g_szRoleTags[g_iPlayerRole[id]], szBotName);
                }
                case MOLO_TACTIC_SITE_CLEAR:
                {
                    BroadcastTeamCall(team, "^4[Utility]^3 %s %s^1: Molotov out — clearing site corner!",
                        g_szRoleTags[g_iPlayerRole[id]], szBotName);
                }
                case MOLO_TACTIC_CHOKE_DELAY:
                {
                    BroadcastTeamCall(team, "^4[Utility]^3 %s %s^1: Molotov out — delaying choke push!",
                        g_szRoleTags[g_iPlayerRole[id]], szBotName);
                }
            }
            PlayTeamAudio(team, "spk radio/fireinhole");
        }
    }
}

stock bool:MolotovTactics_FindPlantedC4(Float:vecOutOrigin[3])
{
    new ent = -1;
    new szModel[48];
    while ((ent = find_ent_by_class(ent, "grenade")))
    {
        if (!pev_valid(ent))
            continue;

        pev(ent, pev_model, szModel, charsmax(szModel));
        if (containi(szModel, "c4") != -1)
        {
            pev(ent, pev_origin, vecOutOrigin);
            return true;
        }
    }
    return false;
}

stock MolotovTacticType:MolotovTactics_FindBestTarget(
    id,
    CsTeams:team,
    const Float:vecBotOrigin[3],
    Float:flGameTime,
    Float:flRoundElapsed,
    bool:bBombPlanted,
    const Float:vecC4Origin[3],
    const Float:vecActiveFires[][3],
    iActiveFireCount,
    iNodeCount,
    Float:vecOutTarget[3],
    Float:vecOutAim[3]
)
{
    // --------------------------------------------------------------------------
    // Mode 1: Post-Plant / Retake Denial (C4 planted & bot within 950u)
    // --------------------------------------------------------------------------
    if (bBombPlanted)
    {
        new Float:flDistToC4 = get_distance_f(vecBotOrigin, vecC4Origin);
        if (flDistToC4 <= MOLO_POSTPLANT_RANGE)
        {
            // T defending planted C4: deny defuse directly on C4 if safe from teammates
            if (team == CS_TEAM_T)
            {
                if (MolotovTactics_ValidateAndComputeAim(id, team, vecBotOrigin, vecC4Origin, vecActiveFires, iActiveFireCount, vecOutAim))
                {
                    xs_vec_copy(vecC4Origin, vecOutTarget);
                    return MOLO_TACTIC_POSTPLANT;
                }
            }

            // CT Retake or T flushing post-plant corner near C4
            if (MolotovTactics_FindSiteCornerNode(id, team, vecBotOrigin, vecC4Origin, MOLO_SITE_SEARCH_RADIUS, iNodeCount, vecActiveFires, iActiveFireCount, vecOutTarget, vecOutAim))
            {
                return MOLO_TACTIC_POSTPLANT;
            }
        }
    }

    // --------------------------------------------------------------------------
    // Mode 2: Site Execute / Corner Clear (Approaching Bombsite A or B <= 900u)
    // --------------------------------------------------------------------------
    if (g_bBombsitesFound)
    {
        new Float:flDistA = get_distance_f(vecBotOrigin, g_vecBombsiteA);
        new Float:flDistB = get_distance_f(vecBotOrigin, g_vecBombsiteB);
        new Float:vecNearSite[3];
        new bool:bNearSite = false;

        if (flDistA <= MOLO_SITE_EXEC_RANGE && flDistA <= flDistB)
        {
            xs_vec_copy(g_vecBombsiteA, vecNearSite);
            bNearSite = true;
        }
        else if (flDistB <= MOLO_SITE_EXEC_RANGE)
        {
            xs_vec_copy(g_vecBombsiteB, vecNearSite);
            bNearSite = true;
        }

        if (bNearSite)
        {
            if (MolotovTactics_FindSiteCornerNode(id, team, vecBotOrigin, vecNearSite, MOLO_SITE_SEARCH_RADIUS, iNodeCount, vecActiveFires, iActiveFireCount, vecOutTarget, vecOutAim))
            {
                return MOLO_TACTIC_SITE_CLEAR;
            }
        }
    }

    // --------------------------------------------------------------------------
    // Mode 3: Chokepoint Delay / Threat Denial (Early round <= 35s or Intel Threat)
    // --------------------------------------------------------------------------
    new Float:vecThreatDir[3];
    new bool:bHasThreatDir = false;

    if (g_iSoundTarget[id] > 0 && flGameTime < g_flSoundTargetExpire[id])
    {
        xs_vec_sub(g_vecSoundTargetOrigin[id], vecBotOrigin, vecThreatDir);
        vecThreatDir[2] = 0.0;
        bHasThreatDir = true;
    }
    else if (g_iTeamThreatTarget[team] > 0 && (flGameTime - g_flTeamThreatTime[team] <= 8.0))
    {
        xs_vec_sub(g_vecTeamThreatOrigin[team], vecBotOrigin, vecThreatDir);
        vecThreatDir[2] = 0.0;
        bHasThreatDir = true;
    }
    else if (flRoundElapsed >= 6.0 && flRoundElapsed <= 35.0)
    {
        // Use bot's upcoming path direction or forward heading during early round
        new iPathLen = yb_get_bot_path_length(id);
        if (iPathLen > 2)
        {
            new iLookAhead = (iPathLen > 5) ? 5 : (iPathLen - 1);
            new iPathNode = yb_get_bot_path_node(id, iLookAhead);
            if (iPathNode != YB_INVALID_NODE_INDEX && yb_get_node_origin(iPathNode, vecThreatDir))
            {
                xs_vec_sub(vecThreatDir, vecBotOrigin, vecThreatDir);
                vecThreatDir[2] = 0.0;
                bHasThreatDir = true;
            }
        }

        if (!bHasThreatDir)
        {
            new Float:vecAngles[3];
            pev(id, pev_v_angle, vecAngles);
            angle_vector(vecAngles, ANGLEVECTOR_FORWARD, vecThreatDir);
            vecThreatDir[2] = 0.0;
            bHasThreatDir = true;
        }
    }

    if (bHasThreatDir)
    {
        new Float:flLen = floatsqroot(vecThreatDir[0] * vecThreatDir[0] + vecThreatDir[1] * vecThreatDir[1]);
        if (flLen > 1.0)
        {
            vecThreatDir[0] /= flLen;
            vecThreatDir[1] /= flLen;

            if (MolotovTactics_FindChokepointNode(id, team, vecBotOrigin, vecThreatDir, iNodeCount, vecActiveFires, iActiveFireCount, vecOutTarget, vecOutAim))
            {
                return MOLO_TACTIC_CHOKE_DELAY;
            }
        }
    }

    return MOLO_TACTIC_NONE;
}

stock bool:MolotovTactics_FindSiteCornerNode(
    id,
    CsTeams:team,
    const Float:vecBotOrigin[3],
    const Float:vecSiteCenter[3],
    Float:flSiteRadius,
    iNodeCount,
    const Float:vecActiveFires[][3],
    iActiveFireCount,
    Float:vecOutTarget[3],
    Float:vecOutAim[3]
)
{
    new Float:flBestScore = -999999.0;
    new Float:vecBestNode[3];
    new Float:vecBestAim[3];
    new bool:bFound = false;

    for (new n = 0; n < iNodeCount; n++)
    {
        if (!yb_is_node_valid(n))
            continue;

        new iFlags = _:yb_get_node_flags(n);
        if (!(iFlags & (_:YB_NODE_FLAG_CAMP | _:YB_NODE_FLAG_SNIPER)))
            continue;

        new Float:vecNode[3];
        if (!yb_get_node_origin(n, vecNode))
            continue;

        if (floatabs(vecNode[2] - vecSiteCenter[2]) > 140.0)
            continue;

        new Float:flDistSite = get_distance_f(vecNode, vecSiteCenter);
        if (flDistSite > flSiteRadius)
            continue;

        new Float:flDistBot = get_distance_f(vecBotOrigin, vecNode);
        if (flDistBot < MOLO_MIN_THROW_DIST || flDistBot > MOLO_MAX_THROW_DIST)
            continue;

        new Float:vecCandAim[3];
        if (!MolotovTactics_ValidateAndComputeAim(id, team, vecBotOrigin, vecNode, vecActiveFires, iActiveFireCount, vecCandAim))
            continue;

        // Prefer CAMP/SNIPER nodes closest to bombsite center at comfortable throw distance (~500u)
        new Float:flScore = (flSiteRadius - flDistSite) - floatabs(flDistBot - 520.0) * 0.25;
        if (iFlags & _:YB_NODE_FLAG_SNIPER)
            flScore += 45.0;

        if (flScore > flBestScore)
        {
            flBestScore = flScore;
            xs_vec_copy(vecNode, vecBestNode);
            xs_vec_copy(vecCandAim, vecBestAim);
            bFound = true;
        }
    }

    if (bFound)
    {
        xs_vec_copy(vecBestNode, vecOutTarget);
        xs_vec_copy(vecBestAim, vecOutAim);
        return true;
    }

    return false;
}

stock bool:MolotovTactics_FindChokepointNode(
    id,
    CsTeams:team,
    const Float:vecBotOrigin[3],
    const Float:vecDir[3],
    iNodeCount,
    const Float:vecActiveFires[][3],
    iActiveFireCount,
    Float:vecOutTarget[3],
    Float:vecOutAim[3]
)
{
    new Float:flBestScore = -999999.0;
    new Float:vecBestNode[3];
    new Float:vecBestAim[3];
    new bool:bFound = false;

    for (new n = 0; n < iNodeCount; n++)
    {
        if (!yb_is_node_valid(n))
            continue;

        new iFlags = _:yb_get_node_flags(n);
        new bool:bIsNarrow = ((iFlags & _:YB_NODE_FLAG_NARROW) != 0);
        new bool:bIsCrossing = ((iFlags & _:YB_NODE_FLAG_CROSSING) != 0);

        if (!bIsNarrow && !bIsCrossing)
            continue;

        new Float:vecNode[3];
        if (!yb_get_node_origin(n, vecNode))
            continue;

        if (floatabs(vecNode[2] - vecBotOrigin[2]) > 140.0)
            continue;

        new Float:dx = vecNode[0] - vecBotOrigin[0];
        new Float:dy = vecNode[1] - vecBotOrigin[1];
        new Float:flDist = floatsqroot(dx * dx + dy * dy);
        if (flDist < MOLO_CHOKE_MIN_RANGE || flDist > MOLO_CHOKE_MAX_RANGE)
            continue;

        // Require forward cone alignment (> ~45 deg) toward threat/choke direction
        new Float:flDot = (dx * vecDir[0] + dy * vecDir[1]) / flDist;
        if (flDot < 0.70)
            continue;

        new Float:vecCandAim[3];
        if (!MolotovTactics_ValidateAndComputeAim(id, team, vecBotOrigin, vecNode, vecActiveFires, iActiveFireCount, vecCandAim))
            continue;

        new Float:flScore = (flDot * 400.0) - floatabs(flDist - 580.0) * 0.30;
        if (bIsNarrow)
            flScore += 120.0;

        if (flScore > flBestScore)
        {
            flBestScore = flScore;
            xs_vec_copy(vecNode, vecBestNode);
            xs_vec_copy(vecCandAim, vecBestAim);
            bFound = true;
        }
    }

    if (bFound)
    {
        xs_vec_copy(vecBestNode, vecOutTarget);
        xs_vec_copy(vecBestAim, vecOutAim);
        return true;
    }

    return false;
}

stock bool:MolotovTactics_ValidateAndComputeAim(
    id,
    CsTeams:team,
    const Float:vecBotOrigin[3],
    const Float:vecTarget[3],
    const Float:vecActiveFires[][3],
    iActiveFireCount,
    Float:vecOutAim[3]
)
{
    // 1. Active Fire Guard: do not stack Molotovs within 260u of an existing fire
    for (new f = 0; f < iActiveFireCount; f++)
    {
        if (floatabs(vecTarget[2] - vecActiveFires[f][2]) <= 120.0
            && get_distance_f(vecTarget, vecActiveFires[f]) <= MOLO_ACTIVE_FIRE_GUARD)
        {
            return false;
        }
    }

    // 2. Anti-Teamtrap Guard: never burn an area with an alive teammate within 220u
    for (new p = 1; p <= MaxClients; p++)
    {
        if (p == id || !is_user_alive(p))
            continue;

        if (cs_get_user_team(p) != team)
            continue;

        new Float:vecMate[3];
        pev(p, pev_origin, vecMate);
        if (floatabs(vecTarget[2] - vecMate[2]) <= 110.0
            && get_distance_f(vecTarget, vecMate) <= MOLO_TEAMMATE_SAFE_RADIUS)
        {
            return false;
        }
    }

    // 3. Ballistic Gravity Compensation & Ceiling/Doorframe Clearance Trace
    new Float:vecEyes[3];
    xs_vec_copy(vecBotOrigin, vecEyes);
    vecEyes[2] += 17.0;

    new Float:dx = vecTarget[0] - vecEyes[0];
    new Float:dy = vecTarget[1] - vecEyes[1];
    new Float:flHorizDist = floatsqroot(dx * dx + dy * dy);
    if (flHorizDist < MOLO_MIN_THROW_DIST || flHorizDist > MOLO_MAX_THROW_DIST)
        return false;

    // GoldSrc HE/Molotov throw v0 ~= 750 u/s, sv_gravity = 800 u/s^2
    new Float:flFlightTime = flHorizDist / 750.0;
    new Float:flArcLift = (0.5 * 800.0 * flFlightTime * flFlightTime) / 1.15;
    if (flArcLift > 220.0)
        flArcLift = 220.0;

    xs_vec_copy(vecTarget, vecOutAim);
    vecOutAim[2] += flArcLift;

    // Check initial launch ray (first 220u along aim vector) for low tunnel ceiling or wall
    new Float:vecAimDir[3];
    xs_vec_sub(vecOutAim, vecEyes, vecAimDir);
    new Float:flAimLen = vector_length(vecAimDir);
    if (flAimLen <= 1.0)
        return false;

    xs_vec_div_scalar(vecAimDir, flAimLen, vecAimDir);

    new Float:flCheckDist = (flAimLen < 220.0) ? flAimLen : 220.0;
    new Float:vecLaunchCheck[3];
    xs_vec_mul_scalar(vecAimDir, flCheckDist, vecLaunchCheck);
    xs_vec_add(vecEyes, vecLaunchCheck, vecLaunchCheck);

    engfunc(EngFunc_TraceLine, vecEyes, vecLaunchCheck, IGNORE_MONSTERS, id, 0);
    new Float:flFracLaunch;
    get_tr2(0, TR_flFraction, flFracLaunch);
    if (flFracLaunch < 0.95)
        return false;

    // Also verify line-of-sight clearance to target point (+28u above floor)
    new Float:vecTargetLOS[3];
    xs_vec_copy(vecTarget, vecTargetLOS);
    vecTargetLOS[2] += 28.0;

    engfunc(EngFunc_TraceLine, vecEyes, vecTargetLOS, IGNORE_MONSTERS, id, 0);
    new Float:flFracTarget;
    get_tr2(0, TR_flFraction, flFracTarget);
    if (flFracTarget < 0.88)
        return false;

    return true;
}
