#if defined _savage_scavenge_utils_included
    #endinput
#endif
#define _savage_scavenge_utils_included

// ==============================================================================
// 12. UTILITY HELPERS & AUDIO CALLS
// ==============================================================================
GetTeamIgl(CsTeams:team)
{
    // First check human IGL
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_connected(i) && !is_user_bot(i) && cs_get_user_team(i) == team && g_iPlayerRole[i] == ROLE_IGL)
        {
            return i;
        }
    }

    // Then check bot IGL
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_connected(i) && is_user_bot(i) && cs_get_user_team(i) == team && g_iPlayerRole[i] == ROLE_IGL)
        {
            return i;
        }
    }

    return 0;
}

BroadcastTeamCall(CsTeams:team, const szFormat[], any:...)
{
    new szBuffer[192];
    vformat(szBuffer, charsmax(szBuffer), szFormat, 3);

    for (new i = 1; i <= 32; i++)
    {
        if (is_user_connected(i) && cs_get_user_team(i) == team)
        {
            client_print_color(i, print_team_default, "%s", szBuffer);
        }
    }
}

PlayTeamAudio(CsTeams:team, const szCommand[])
{
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_connected(i) && !is_user_bot(i) && cs_get_user_team(i) == team)
        {
            client_cmd(i, szCommand);
        }
    }
}

InitMapMidPoint()
{
    new szMap[32];
    get_mapname(szMap, charsmax(szMap));

    if (containi(szMap, "dust2") != -1)
    {
        g_vecMapMidPoint[0] = -380.0;
        g_vecMapMidPoint[1] = -800.0;
        g_vecMapMidPoint[2] = -120.0;
        g_bMidPointCalculated = true;
        return;
    }
    else if (containi(szMap, "inferno") != -1)
    {
        g_vecMapMidPoint[0] = 350.0;
        g_vecMapMidPoint[1] = 1200.0;
        g_vecMapMidPoint[2] = 160.0;
        g_bMidPointCalculated = true;
        return;
    }
    else if (containi(szMap, "mirage") != -1)
    {
        g_vecMapMidPoint[0] = -650.0;
        g_vecMapMidPoint[1] = -600.0;
        g_vecMapMidPoint[2] = -160.0;
        g_bMidPointCalculated = true;
        return;
    }
    else if (containi(szMap, "nuke") != -1)
    {
        g_vecMapMidPoint[0] = 200.0;
        g_vecMapMidPoint[1] = -600.0;
        g_vecMapMidPoint[2] = -220.0;
        g_bMidPointCalculated = true;
        return;
    }
    else if (containi(szMap, "train") != -1)
    {
        g_vecMapMidPoint[0] = 100.0;
        g_vecMapMidPoint[1] = 400.0;
        g_vecMapMidPoint[2] = -200.0;
        g_bMidPointCalculated = true;
        return;
    }
    else if (containi(szMap, "tuscan") != -1)
    {
        g_vecMapMidPoint[0] = -150.0;
        g_vecMapMidPoint[1] = 300.0;
        g_vecMapMidPoint[2] = -80.0;
        g_bMidPointCalculated = true;
        return;
    }

    new Float:vecCtSum[3], Float:vecTSum[3], Float:vecOrigin[3];
    new iCtCount = 0, iTCount = 0;

    new ent = -1;
    while ((ent = find_ent_by_class(ent, "info_player_start")))
    {
        if (pev_valid(ent))
        {
            pev(ent, pev_origin, vecOrigin);
            xs_vec_add(vecCtSum, vecOrigin, vecCtSum);
            iCtCount++;
        }
    }

    ent = -1;
    while ((ent = find_ent_by_class(ent, "info_player_deathmatch")))
    {
        if (pev_valid(ent))
        {
            pev(ent, pev_origin, vecOrigin);
            xs_vec_add(vecTSum, vecOrigin, vecTSum);
            iTCount++;
        }
    }

    if (iCtCount > 0 && iTCount > 0)
    {
        xs_vec_div_scalar(vecCtSum, float(iCtCount), vecCtSum);
        xs_vec_div_scalar(vecTSum, float(iTCount), vecTSum);

        xs_vec_add(vecCtSum, vecTSum, g_vecMapMidPoint);
        xs_vec_div_scalar(g_vecMapMidPoint, 2.0, g_vecMapMidPoint);

        new Float:vecStart[3], Float:vecEnd[3], Float:vecHit[3];
        xs_vec_copy(g_vecMapMidPoint, vecStart);
        vecStart[2] += 64.0;
        xs_vec_copy(vecStart, vecEnd);
        vecEnd[2] -= 1000.0;

        trace_line(0, vecStart, vecEnd, vecHit);
        g_vecMapMidPoint[2] = vecHit[2] + 36.0;
        g_bMidPointCalculated = true;
    }
    else
    {
        g_vecMapMidPoint[0] = 0.0;
        g_vecMapMidPoint[1] = 0.0;
        g_vecMapMidPoint[2] = 0.0;
    }
}

stock InitBombsites()
{
    g_bBombsitesFound = false;
    new ent = -1;
    new iFound = 0;
    while ((ent = find_ent_by_class(ent, "func_bomb_target")))
    {
        if (!pev_valid(ent)) continue;

        new Float:vecOrigin[3];
        pev(ent, pev_origin, vecOrigin);
        if (vecOrigin[0] == 0.0 && vecOrigin[1] == 0.0 && vecOrigin[2] == 0.0)
        {
            new Float:absmin[3], Float:absmax[3];
            pev(ent, pev_absmin, absmin);
            pev(ent, pev_absmax, absmax);
            xs_vec_add(absmin, absmax, vecOrigin);
            xs_vec_div_scalar(vecOrigin, 2.0, vecOrigin);
        }

        if (iFound == 0)
        {
            xs_vec_copy(vecOrigin, g_vecBombsiteA);
            iFound++;
        }
        else if (iFound == 1)
        {
            xs_vec_copy(vecOrigin, g_vecBombsiteB);
            iFound++;
            g_bBombsitesFound = true;
            break;
        }
    }

    if (!g_bBombsitesFound)
    {
        ent = -1;
        while ((ent = find_ent_by_class(ent, "info_bomb_target")))
        {
            if (!pev_valid(ent)) continue;

            new Float:vecOrigin[3];
            pev(ent, pev_origin, vecOrigin);
            if (iFound == 0)
            {
                xs_vec_copy(vecOrigin, g_vecBombsiteA);
                iFound++;
            }
            else if (iFound == 1)
            {
                xs_vec_copy(vecOrigin, g_vecBombsiteB);
                iFound++;
                g_bBombsitesFound = true;
                break;
            }
        }
    }
}

bool:IsKnifeRoundActive()
{
    if (get_pcvar_num(g_pCvarKnifeRoundActive) == 1)
        return true;

    return false;
}

public Task_TacticalMidLineupScanner()
{
    if (!get_pcvar_num(g_pCvarEnable) || g_bInFreezeTime || IsKnifeRoundActive() || IsWarmupActive())
        return;

    new Float:flGameTime = get_gametime();
    // Only run during opening mid clash phase (first 25 seconds)
    if (flGameTime - g_flRoundStartTime > 25.0)
        return;

    for (new id = 1; id <= 32; id++)
    {
        if (!is_user_alive(id) || !is_user_bot(id))
            continue;

        new CsTeams:team = cs_get_user_team(id);
        if (team != CS_TEAM_T && team != CS_TEAM_CT)
            continue;

        if (flGameTime - g_flLastCombatTime[id] < 3.0 || g_bLineupActive[id])
            continue;

        new iRole = g_iPlayerRole[id];
        new Float:vecBotOrigin[3];
        pev(id, pev_origin, vecBotOrigin);

        // 1. MID SMOKE LINEUP (Support / IGL / Entry / AWPer)
        if (!g_bMidSmokedThisRound[team] && (iRole == ROLE_SUPPORT || iRole == ROLE_IGL || iRole == ROLE_ENTRY || iRole == ROLE_AWPER))
        {
            if (user_has_weapon(id, CSW_SMOKEGRENADE))
            {
                new Float:flDistToMid = get_distance_f(vecBotOrigin, g_vecMapMidPoint);
                if (flDistToMid >= 350.0 && flDistToMid <= 1800.0)
                {
                    TriggerMidLineup(id, true);
                    continue;
                }
            }
        }

        // 2. MID FLASHBANG LINEUP (Entry / Support)
        if (!g_bMidFlashedThisRound[team] && (iRole == ROLE_ENTRY || iRole == ROLE_SUPPORT))
        {
            if (user_has_weapon(id, CSW_FLASHBANG))
            {
                new Float:flDistToMid = get_distance_f(vecBotOrigin, g_vecMapMidPoint);
                if (flDistToMid >= 300.0 && flDistToMid <= 1300.0)
                {
                    TriggerMidLineup(id, false);
                    continue;
                }
            }
        }
    }
}

TriggerMidLineup(id, bool:isSmoke)
{
    new CsTeams:team = cs_get_user_team(id);
    new Float:flGameTime = get_gametime();

    new Float:vecBotEyes[3], Float:vecTarget[3];
    pev(id, pev_origin, vecBotEyes);
    vecBotEyes[2] += 17.0;

    new szMapName[32];
    get_mapname(szMapName, charsmax(szMapName));

    if (containi(szMapName, "dust2") != -1)
    {
        if (team == CS_TEAM_CT)
        {
            // Smoke CT Cross & Mid Doors to cut off sniper sightline from T spawn
            vecTarget[0] = -380.0;
            vecTarget[1] = 1750.0;
            vecTarget[2] = -100.0;
        }
        else
        {
            // Smoke Mid Doors / Xbox from T side
            vecTarget[0] = -360.0;
            vecTarget[1] = 1600.0;
            vecTarget[2] = -100.0;
        }
    }
    else
    {
        xs_vec_copy(g_vecMapMidPoint, vecTarget);
    }

    new Float:vecDiff[3], Float:vecAimAngles[3];
    xs_vec_sub(vecTarget, vecBotEyes, vecDiff);
    vector_to_angle(vecDiff, vecAimAngles);

    // Set upward arc pitch for realistic esports lob throw (-28° to -38°)
    vecAimAngles[0] = isSmoke ? -28.0 : -38.0;
    ClampAngles(vecAimAngles);

    xs_vec_copy(vecAimAngles, g_vecLineupAngles[id]);
    g_bLineupActive[id] = true;
    g_flLineupThrowUntil[id] = flGameTime + 0.45;

    new szName[32];
    get_user_name(id, szName, charsmax(szName));

    if (isSmoke)
    {
        engclient_cmd(id, "weapon_smokegrenade");
        g_bMidSmokedThisRound[team] = true;
        BroadcastTeamCall(team, "^4[Tactical Lineup]^3 %s %s^1: Smoking Mid! Sightline blocked.",
            g_szRoleTags[g_iPlayerRole[id]], szName);
        PlayTeamAudio(team, "spk radio/stormfront");
    }
    else
    {
        engclient_cmd(id, "weapon_flashbang");
        g_bMidFlashedThisRound[team] = true;
        BroadcastTeamCall(team, "^4[Tactical Lineup]^3 %s %s^1: Pop-flashing Mid! Turn away!",
            g_szRoleTags[g_iPlayerRole[id]], szName);
        PlayTeamAudio(team, "spk radio/takingfire");
    }
}

bool:IsWarmupActive()
{
    static pCvarWarmup;
    if (!pCvarWarmup)
        pCvarWarmup = get_cvar_pointer("amx_csgo_is_warmup");

    return (pCvarWarmup != 0 && get_pcvar_num(pCvarWarmup) == 1);
}

bool:IsTimeoutActive()
{
    static pCvarTimeout;
    if (!pCvarTimeout)
        pCvarTimeout = get_cvar_pointer("amx_csgo_is_timeout");

    return (pCvarTimeout != 0 && get_pcvar_num(pCvarTimeout) == 1);
}

stock bool:CanSeeDirectlyToPoint(Float:vecStart[3], Float:vecTarget[3])
{
    new Float:vecHitEnd[3];
    new iHit = trace_line(0, vecStart, vecTarget, vecHitEnd);
    if (iHit <= 0)
    {
        new Float:flDistTotal = get_distance_f(vecStart, vecTarget);
        new Float:flDistHit = get_distance_f(vecStart, vecHitEnd);
        return (floatabs(flDistTotal - flDistHit) < 40.0);
    }
    return false;
}

stock bool:IsGunWeapon(weapon_id)
{
    switch (weapon_id)
    {
        case CSW_P228, CSW_SCOUT, CSW_XM1014, CSW_MAC10, CSW_AUG,
             CSW_ELITE, CSW_FIVESEVEN, CSW_UMP45, CSW_SG550, CSW_GALIL,
             CSW_FAMAS, CSW_USP, CSW_GLOCK18, CSW_AWP, CSW_MP5NAVY,
             CSW_M249, CSW_M3, CSW_M4A1, CSW_TMP, CSW_G3SG1,
             CSW_DEAGLE, CSW_SG552, CSW_AK47, CSW_P90:
        {
            return true;
        }
    }
    return false;
}

stock bool:IsPrimaryRifle(weapon_id)
{
    switch (weapon_id)
    {
        case CSW_AK47, CSW_M4A1, CSW_AWP, CSW_GALIL, CSW_FAMAS, CSW_AUG, CSW_SG552, CSW_SCOUT:
        {
            return true;
        }
    }
    return false;
}

stock bool:CanWallbang(weapon_id)
{
    switch (weapon_id)
    {
        case CSW_AK47, CSW_M4A1, CSW_AWP, CSW_DEAGLE, CSW_SG552, CSW_AUG, CSW_G3SG1, CSW_SG550:
        {
            return true;
        }
    }
    return false;
}

stock bool:CanSeeDirectly(ent1, ent2)
{
    new Float:vecStart[3], Float:vecEnd[3], Float:vecHitEnd[3];
    pev(ent1, pev_origin, vecStart);
    vecStart[2] += 17.0;
    pev(ent2, pev_origin, vecEnd);
    vecEnd[2] += 17.0;

    new iHit = trace_line(ent1, vecStart, vecEnd, vecHitEnd);
    return (iHit == ent2);
}

stock ClampAngles(Float:angles[3])
{
    if (angles[0] < -89.0) angles[0] = -89.0;
    if (angles[0] > 89.0)  angles[0] = 89.0;

    while (angles[1] < -180.0) angles[1] += 360.0;
    while (angles[1] > 180.0)  angles[1] -= 360.0;

    angles[2] = 0.0;
}

stock GetGroundWeaponType(ent)
{
    if (!pev_valid(ent))
        return 0;

    new szClass[32];
    pev(ent, pev_classname, szClass, charsmax(szClass));

    if (equal(szClass, "cs2_dropped_rifle"))
    {
        return pev(ent, pev_iuser1);
    }

    if (equal(szClass, "weaponbox"))
    {
        new szModel[64];
        pev(ent, pev_model, szModel, charsmax(szModel));

        if (containi(szModel, "w_awp") != -1)     return CSW_AWP;
        if (containi(szModel, "w_ak47") != -1)    return CSW_AK47;
        if (containi(szModel, "w_m4a1") != -1 || containi(szModel, "w_m4a4") != -1)    return CSW_M4A1;
        if (containi(szModel, "w_sg552") != -1)   return CSW_SG552;
        if (containi(szModel, "w_aug") != -1)     return CSW_AUG;
        if (containi(szModel, "w_scout") != -1)   return CSW_SCOUT;
        if (containi(szModel, "w_galil") != -1)   return CSW_GALIL;
        if (containi(szModel, "w_famas") != -1)   return CSW_FAMAS;
        if (containi(szModel, "w_mp5") != -1)     return CSW_MP5NAVY;
        if (containi(szModel, "w_ump45") != -1)   return CSW_UMP45;
        if (containi(szModel, "w_p90") != -1)     return CSW_P90;
        if (containi(szModel, "w_m3") != -1)      return CSW_M3;
        if (containi(szModel, "w_xm1014") != -1)  return CSW_XM1014;
        if (containi(szModel, "w_m249") != -1)    return CSW_M249;
        if (containi(szModel, "w_g3sg1") != -1)   return CSW_G3SG1;
        if (containi(szModel, "w_sg550") != -1)   return CSW_SG550;
        if (containi(szModel, "w_deagle") != -1)  return CSW_DEAGLE;
    }

    return 0;
}

public Cmd_PlayerDropWeaponPre(id)
{
    if (g_bInFreezeTime && is_user_alive(id) && !is_user_bot(id))
    {
        // Player dropped a weapon in freeze time: scan spawn in 0.1s to allow entity to touch ground
        set_task(0.1, "Task_ScanSpawnDroppedWeapons", TASK_SV_SCAN_WPNS);
    }
    return PLUGIN_CONTINUE;
}

public Cmd_PlayerPingPointPre(id)
{
    if (g_bInFreezeTime && is_user_alive(id) && !is_user_bot(id))
    {
        // Player pinged during freeze time: instantly scan spawn for handoff
        set_task(0.05, "Task_ScanSpawnDroppedWeapons", TASK_SV_SCAN_WPNS);
    }
    return PLUGIN_CONTINUE;
}

public Task_ScanSpawnDroppedWeapons()
{
    if (IsWarmupActive() || IsKnifeRoundActive())
        return;

    ScanAndClaimSpawnDroppedWeapons(CS_TEAM_T);
    ScanAndClaimSpawnDroppedWeapons(CS_TEAM_CT);
}

stock ScanAndClaimSpawnDroppedWeapons(CsTeams:team)
{
    if (!g_bInFreezeTime || IsKnifeRoundActive() || IsWarmupActive())
        return;

    // 1. Scan dropped weaponbox entities in spawn
    new ent = -1;
    while ((ent = find_ent_by_class(ent, "weaponbox")))
    {
        if (!pev_valid(ent))
            continue;

        new Float:vecWpnOrigin[3];
        pev(ent, pev_origin, vecWpnOrigin);

        // Verify the dropped weapon is in spawn / near team players (within 1400 units)
        new bool:bNearTeam = false;
        for (new p = 1; p <= 32; p++)
        {
            if (is_user_alive(p) && cs_get_user_team(p) == team)
            {
                new Float:vecPOrigin[3];
                pev(p, pev_origin, vecPOrigin);
                if (get_distance_f(vecPOrigin, vecWpnOrigin) <= 1400.0)
                {
                    bNearTeam = true;
                    break;
                }
            }
        }

        if (!bNearTeam)
            continue;

        new iWpn = GetGroundWeaponType(ent);
        if (!iWpn || !IsPrimaryWeapon(iWpn))
            continue;

        // A. If AWP: Claim for team's AWPer bot if they need it!
        if (iWpn == CSW_AWP)
        {
            new iAwper = GetTeamAwperBot(team);
            if (iAwper > 0 && is_user_alive(iAwper) && GetUserPrimaryWeapon(iAwper) != CSW_AWP)
            {
                ClaimGroundWeaponForBot(iAwper, ent, CSW_AWP, team);
                continue;
            }
        }
        // B. If AK-47 / M4A1: Claim for Entry Fragger or broke bot without a rifle
        else if (iWpn == CSW_AK47 || iWpn == CSW_M4A1)
        {
            new iRecipient = GetTeamBestRifleRecipient(team);
            if (iRecipient > 0)
            {
                ClaimGroundWeaponForBot(iRecipient, ent, iWpn, team);
                continue;
            }
        }
    }

    // 2. Scan custom cs2_dropped_rifle in spawn
    ent = -1;
    while ((ent = find_ent_by_class(ent, "cs2_dropped_rifle")))
    {
        if (!pev_valid(ent))
            continue;

        new Float:vecWpnOrigin[3];
        pev(ent, pev_origin, vecWpnOrigin);

        new bool:bNearTeam = false;
        for (new p = 1; p <= 32; p++)
        {
            if (is_user_alive(p) && cs_get_user_team(p) == team)
            {
                new Float:vecPOrigin[3];
                pev(p, pev_origin, vecPOrigin);
                if (get_distance_f(vecPOrigin, vecWpnOrigin) <= 1400.0)
                {
                    bNearTeam = true;
                    break;
                }
            }
        }

        if (!bNearTeam)
            continue;

        new iWpn = pev(ent, pev_iuser1);
        if (!iWpn || !IsPrimaryWeapon(iWpn))
            continue;

        if (iWpn == CSW_AWP)
        {
            new iAwper = GetTeamAwperBot(team);
            if (iAwper > 0 && is_user_alive(iAwper) && GetUserPrimaryWeapon(iAwper) != CSW_AWP)
            {
                ClaimGroundWeaponForBot(iAwper, ent, CSW_AWP, team);
                continue;
            }
        }
        else if (iWpn == CSW_AK47 || iWpn == CSW_M4A1)
        {
            new iRecipient = GetTeamBestRifleRecipient(team);
            if (iRecipient > 0)
            {
                ClaimGroundWeaponForBot(iRecipient, ent, iWpn, team);
                continue;
            }
        }
    }
}

stock ClaimGroundWeaponForBot(bot, ent, weaponId, CsTeams:team)
{
    new szBotName[32];
    get_user_name(bot, szBotName, charsmax(szBotName));

    // Strip previous inferior primary weapon if holding one
    new iCurPrim = GetUserPrimaryWeapon(bot);
    if (iCurPrim > 0)
    {
        StripUserPrimary(bot);
    }

    // Remove ground entity cleanly
    if (pev_valid(ent))
    {
        engfunc(EngFunc_RemoveEntity, ent);
    }

    // Equip the bot
    if (weaponId == CSW_AWP)
    {
        give_item(bot, "weapon_awp");
        cs_set_user_bpammo(bot, CSW_AWP, 30);
        give_item(bot, "weapon_deagle");
        cs_set_user_bpammo(bot, CSW_DEAGLE, 35);
        give_item(bot, "item_assaultsuit");

        BroadcastTeamCall(team, "^4[Freeze Time Drop]^3 %s (AWPer)^1: Claimed dropped ^4AWP^1 from spawn! Kept $4750 team economy!", szBotName);
        PlayTeamAudio(team, "spk radio/roger");
    }
    else if (weaponId == CSW_AK47)
    {
        give_item(bot, "weapon_ak47");
        cs_set_user_bpammo(bot, CSW_AK47, 90);
        give_item(bot, "item_assaultsuit");

        BroadcastTeamCall(team, "^4[Freeze Time Drop]^3 %s %s^1: Claimed dropped ^4AK-47^1 from spawn!",
            g_szRoleTags[g_iPlayerRole[bot]], szBotName);
        PlayTeamAudio(team, "spk radio/roger");
    }
    else if (weaponId == CSW_M4A1)
    {
        new iWpn = give_item(bot, "weapon_m4a1");
        cs_set_user_bpammo(bot, CSW_M4A1, 90);
        if (pev_valid(iWpn))
        {
            new bool:bM4A4 = IsPlayerM4A4(bot);
            set_pev(iWpn, pev_iuser1, bM4A4 ? 1 : 2);
            if (bM4A4 && cs_get_weapon_silen(iWpn))
                cs_set_weapon_silen(iWpn, 0, 0);
            else if (!bM4A4 && !cs_get_weapon_silen(iWpn))
                cs_set_weapon_silen(iWpn, 1, 0);
        }
        give_item(bot, "item_assaultsuit");

        BroadcastTeamCall(team, "^4[Freeze Time Drop]^3 %s %s^1: Claimed dropped ^4%s^1 from spawn!",
            g_szRoleTags[g_iPlayerRole[bot]], szBotName, IsPlayerM4A4(bot) ? "M4A4" : "M4A1-S");
        PlayTeamAudio(team, "spk radio/roger");
    }

    emit_sound(bot, CHAN_ITEM, "items/gunpickup2.wav", 0.9, ATTN_NORM, 0, PITCH_NORM);
    g_flLastWeaponSwapTime[bot] = get_gametime();
}

stock GetTeamAwperBot(CsTeams:team)
{
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == team && g_iPlayerRole[i] == ROLE_AWPER)
            return i;
    }
    return 0;
}

stock GetTeamBestRifleRecipient(CsTeams:team)
{
    // Priority 1: Entry Fragger without primary
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == team && g_iPlayerRole[i] == ROLE_ENTRY)
        {
            if (GetUserPrimaryWeapon(i) == 0)
                return i;
        }
    }
    // Priority 2: Any non-AWP bot without primary
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == team && g_iPlayerRole[i] != ROLE_AWPER)
        {
            if (GetUserPrimaryWeapon(i) == 0)
                return i;
        }
    }
    return 0;
}

stock GetRoleWeaponTier(id, weapon_id, bool:bRoundEnded)
{
    if (weapon_id <= 0)
        return 0;

    new iRole = (id >= 1 && id <= 32) ? g_iPlayerRole[id] : ROLE_NONE;

    // For AWPer: AWP is always Tier 4 (pinnacle weapon)
    if (iRole == ROLE_AWPER)
    {
        switch (weapon_id)
        {
            case CSW_AWP: return 4;
            case CSW_AK47, CSW_M4A1, CSW_SG552, CSW_AUG: return 3;
            case CSW_GALIL, CSW_FAMAS, CSW_SCOUT, CSW_G3SG1, CSW_SG550: return 2;
            case CSW_MP5NAVY, CSW_UMP45, CSW_P90, CSW_MAC10, CSW_TMP, CSW_XM1014, CSW_M3, CSW_M249: return 1;
        }
        return 0;
    }

    // For non-AWPers (Entry, Support, Lurker, IGL):
    // When the round has ended, any bot can scavenge an AWP to save it for the AWPer next round!
    if (bRoundEnded)
    {
        switch (weapon_id)
        {
            case CSW_AWP: return 4;
            case CSW_AK47, CSW_M4A1, CSW_SG552, CSW_AUG: return 3;
            case CSW_GALIL, CSW_FAMAS, CSW_SCOUT, CSW_G3SG1, CSW_SG550: return 2;
            case CSW_MP5NAVY, CSW_UMP45, CSW_P90, CSW_MAC10, CSW_TMP, CSW_XM1014, CSW_M3, CSW_M249: return 1;
        }
        return 0;
    }

    // During active combat (!bRoundEnded):
    // AK-47 & M4A1 are Tier 4 (Pinnacle Assault Rifles) for Entry, Support, Lurker, IGL!
    // AWP is Tier 2 (only picked up in emergency if bot has NO primary or only a pistol/SMG, never replacing AK/M4!)
    switch (weapon_id)
    {
        case CSW_AK47, CSW_M4A1: return 4;
        case CSW_SG552, CSW_AUG, CSW_GALIL, CSW_FAMAS: return 3;
        case CSW_AWP, CSW_SCOUT, CSW_G3SG1, CSW_SG550: return 2;
        case CSW_MP5NAVY, CSW_UMP45, CSW_P90, CSW_MAC10, CSW_TMP, CSW_XM1014, CSW_M3, CSW_M249: return 1;
    }
    return 0;
}

stock GetWeaponTier(weapon_id)
{
    switch (weapon_id)
    {
        case CSW_AWP: return 4;
        case CSW_AK47, CSW_M4A1, CSW_SG552, CSW_AUG: return 3;
        case CSW_GALIL, CSW_FAMAS, CSW_SCOUT, CSW_G3SG1, CSW_SG550: return 2;
        case CSW_MP5NAVY, CSW_UMP45, CSW_P90, CSW_MAC10, CSW_TMP, CSW_XM1014, CSW_M3, CSW_M249: return 1;
    }
    return 0;
}

stock GetUserPrimaryWeapon(id)
{
    if (!is_user_alive(id))
        return 0;

    new weapons[32], numWeapons;
    get_user_weapons(id, weapons, numWeapons);
    for (new i = 0; i < numWeapons; i++)
    {
        if (IsPrimaryWeapon(weapons[i]))
            return weapons[i];
    }
    return 0;
}

stock bool:IsPrimaryWeapon(weapon_id)
{
    switch (weapon_id)
    {
        case CSW_AK47, CSW_M4A1, CSW_AWP, CSW_GALIL, CSW_FAMAS,
             CSW_AUG, CSW_SG552, CSW_SCOUT, CSW_MP5NAVY, CSW_UMP45,
             CSW_P90, CSW_MAC10, CSW_TMP, CSW_XM1014, CSW_M3,
             CSW_M249, CSW_G3SG1, CSW_SG550:
        {
            return true;
        }
    }
    return false;
}

stock GetWeaponCleanName(weapon_id, szBuffer[], iMaxLen)
{
    switch (weapon_id)
    {
        case CSW_AK47:      copy(szBuffer, iMaxLen, "AK-47");
        case CSW_M4A1:      copy(szBuffer, iMaxLen, "M4A1");
        case CSW_AWP:       copy(szBuffer, iMaxLen, "AWP");
        case CSW_GALIL:     copy(szBuffer, iMaxLen, "Galil");
        case CSW_FAMAS:     copy(szBuffer, iMaxLen, "FAMAS");
        case CSW_SCOUT:     copy(szBuffer, iMaxLen, "Scout");
        case CSW_SG552:     copy(szBuffer, iMaxLen, "SG552");
        case CSW_AUG:       copy(szBuffer, iMaxLen, "AUG");
        case CSW_DEAGLE:    copy(szBuffer, iMaxLen, "Deagle");
        case CSW_MP5NAVY:   copy(szBuffer, iMaxLen, "MP7");
        case CSW_P228:      copy(szBuffer, iMaxLen, "Tec-9");
        case CSW_TMP:       copy(szBuffer, iMaxLen, "CZ75-Auto");
        case CSW_MAC10:     copy(szBuffer, iMaxLen, "CZ75-Auto");
        case CSW_P90:       copy(szBuffer, iMaxLen, "PP-Bizon");
        case CSW_XM1014:    copy(szBuffer, iMaxLen, "MAG-7");
        case CSW_M3:        copy(szBuffer, iMaxLen, "Sawed-Off");
        case CSW_M249:      copy(szBuffer, iMaxLen, "Negev");
        case CSW_UMP45:     copy(szBuffer, iMaxLen, "UMP-45");
        case CSW_G3SG1:     copy(szBuffer, iMaxLen, "G3SG1");
        case CSW_SG550:     copy(szBuffer, iMaxLen, "SG550");
        case CSW_USP:       copy(szBuffer, iMaxLen, "USP");
        case CSW_GLOCK18:   copy(szBuffer, iMaxLen, "Glock");
        case CSW_ELITE:     copy(szBuffer, iMaxLen, "Dual Berettas");
        case CSW_FIVESEVEN: copy(szBuffer, iMaxLen, "Five-SeveN");
        default:            copy(szBuffer, iMaxLen, "Weapon");
    }
}

stock DropUserPrimary(id)
{
    if (!is_user_alive(id))
        return 0;

    new iWeapons[32], iNum;
    get_user_weapons(id, iWeapons, iNum);
    for (new i = 0; i < iNum; i++)
    {
        if (IsPrimaryWeapon(iWeapons[i]))
        {
            new szWeapon[32];
            get_weaponname(iWeapons[i], szWeapon, charsmax(szWeapon));
            engclient_cmd(id, szWeapon);
            engclient_cmd(id, "drop", szWeapon);
            return iWeapons[i];
        }
    }
    return 0;
}

stock StripUserPrimary(id)
{
    if (!is_user_alive(id))
        return;

    new iWeapons[32], iNum;
    get_user_weapons(id, iWeapons, iNum);
    for (new i = 0; i < iNum; i++)
    {
        if (IsPrimaryWeapon(iWeapons[i]))
        {
            new szWeapon[32];
            get_weaponname(iWeapons[i], szWeapon, charsmax(szWeapon));
            ham_strip_weapon(id, szWeapon);
            break;
        }
    }
}

stock ham_strip_weapon(id, const weapon[])
{
    if (!equal(weapon, "weapon_", 7)) return 0;
    new wId = get_weaponid(weapon);
    if (!wId) return 0;
    new wEnt;
    while ((wEnt = engfunc(EngFunc_FindEntityByString, wEnt, "classname", weapon)) && pev(wEnt, pev_owner) != id) {}
    if (!pev_valid(wEnt)) return 0;
    
    if (get_user_weapon(id) == wId)
    {
        ExecuteHamB(Ham_Item_Holster, wEnt, 0);

        new activeItem = get_pdata_cbase(id, 373, 5); // m_pActiveItem
        if (activeItem == wEnt)
        {
            new knife = -1;
            while ((knife = engfunc(EngFunc_FindEntityByString, knife, "classname", "weapon_knife")) > 0)
            {
                if (pev(knife, pev_owner) == id && knife != wEnt)
                    break;
            }

            if (pev_valid(knife) && knife != wEnt)
            {
                set_pdata_cbase(id, 373, knife, 5);
                ExecuteHamB(Ham_Item_Deploy, knife);
            }
            else
            {
                set_pdata_cbase(id, 373, 0, 5);
            }
        }
    }
    ExecuteHamB(Ham_RemovePlayerItem, id, wEnt);
    set_pev(id, pev_weapons, pev(id, pev_weapons) & ~(1<<wId));
    set_pev(wEnt, pev_flags, pev(wEnt, pev_flags) | FL_KILLME);
    dllfunc(DLLFunc_Think, wEnt);
    return 1;
}

stock bool:IsSecondaryWeapon(weapon_id)
{
    switch (weapon_id)
    {
        case CSW_P228, CSW_ELITE, CSW_FIVESEVEN, CSW_USP, CSW_GLOCK18, CSW_DEAGLE:
            return true;
    }
    return false;
}

stock bool:IsPistolWeapon(weapon_id)
{
    return IsSecondaryWeapon(weapon_id);
}

stock GetUserSecondaryWeapon(id)
{
    if (!is_user_alive(id))
        return 0;

    new weapons[32], num;
    get_user_weapons(id, weapons, num);
    for (new i = 0; i < num; i++)
    {
        if (IsSecondaryWeapon(weapons[i]))
            return weapons[i];
    }
    return 0;
}

stock DropUserSecondary(id)
{
    if (!is_user_alive(id))
        return 0;

    new iWeapons[32], iNum;
    get_user_weapons(id, iWeapons, iNum);
    for (new i = 0; i < iNum; i++)
    {
        if (IsSecondaryWeapon(iWeapons[i]))
        {
            new szWeapon[32];
            get_weaponname(iWeapons[i], szWeapon, charsmax(szWeapon));
            engclient_cmd(id, szWeapon);
            engclient_cmd(id, "drop", szWeapon);
            return iWeapons[i];
        }
    }
    return 0;
}