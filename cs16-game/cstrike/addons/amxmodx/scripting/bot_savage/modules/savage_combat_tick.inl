#if defined _savage_combat_tick_included
    #endinput
#endif
#define _savage_combat_tick_included

// ==============================================================================
// 7. MAIN COMBAT TICK ENGINE (FM_CmdStart)
// ==============================================================================
ReleaseBotHoldAngle(id)
{
    if (g_bHoldingAngle[id])
    {
        g_bHoldingAngle[id] = false;
        if (g_flOriginalMaxSpeed[id] > 50.0)
        {
            set_user_maxspeed(id, g_flOriginalMaxSpeed[id]);
        }
        else
        {
            ExecuteHamB(Ham_Item_PreFrame, id);
        }
    }
}

public OnCmdStart(id, uc_handle, seed)
{
    if (!get_pcvar_num(g_pCvarEnable))
        return FMRES_IGNORED;

    if (!is_user_bot(id) || !is_user_alive(id))
        return FMRES_IGNORED;

    // Fast-path early return: when micro override is disabled and not Knife Round,
    // YaPB C++ owns 100% of movement/aim/firing — avoid per-tick native overhead.
    if (!get_pcvar_num(g_pCvarMicroOverride) && !IsKnifeRoundActive())
    {
        if (g_bHoldingAngle[id])
        {
            ReleaseBotHoldAngle(id);
        }
        return FMRES_IGNORED;
    }

    // Freeze time is natively handled by YaPB bot engine and server physics
    if (g_bInFreezeTime && !IsWarmupActive())
    {
        return FMRES_IGNORED;
    }

    new Float:flGameTime = get_gametime();
    new iButtons = get_uc(uc_handle, UC_Buttons);
    new iWeapon = get_user_weapon(id);
    new bool:bIsGun = IsGunWeapon(iWeapon);
    new bool:bIsOnGround = bool:(pev(id, pev_flags) & FL_ONGROUND);
    new iRole = g_iPlayerRole[id];

    // -------------------------------------------------------------
    // KNIFE ROUND MID BRAWL & GLADIATOR COMBAT ENGINE
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarKnifeMidBrawl) && IsKnifeRoundActive() && iWeapon == CSW_KNIFE)
    {
        // Anti-Stall Tracker: After 35 seconds, alert remaining survivors
        if (flGameTime - g_flKnifeRoundStartTime > 35.0 && !g_bAntiStallTriggered)
        {
            g_bAntiStallTriggered = true;
            client_print_color(0, print_team_default, "^4[Anti-Stall]^3 Knife round > 35s! Radar beacon active, finish the brawl!");
            PlayTeamAudio(CS_TEAM_T, "spk radio/enemys");
            PlayTeamAudio(CS_TEAM_CT, "spk radio/enemys");
        }

        new CsTeams:teamBot = cs_get_user_team(id);
        new Float:vecBotOrigin[3], Float:vecBotEyes[3];
        pev(id, pev_origin, vecBotOrigin);
        pev(id, pev_view_ofs, vecBotEyes);
        xs_vec_add(vecBotOrigin, vecBotEyes, vecBotEyes);

        // Scan for nearest living enemy
        new iNearestEnemy = 0;
        new Float:flNearestDist = 999999.0;
        new Float:vecEnemyOrigin[3];

        for (new enemy = 1; enemy <= 32; enemy++)
        {
            if (is_user_alive(enemy))
            {
                new CsTeams:teamEnemy = cs_get_user_team(enemy);
                if (teamEnemy != teamBot && teamEnemy != CS_TEAM_UNASSIGNED && teamEnemy != CS_TEAM_SPECTATOR)
                {
                    pev(enemy, pev_origin, vecEnemyOrigin);
                    new Float:flDist = get_distance_f(vecBotOrigin, vecEnemyOrigin);
                    if (flDist < flNearestDist)
                    {
                        flNearestDist = flDist;
                        iNearestEnemy = enemy;
                    }
                }
            }
        }

        // Target Enemy if in pursuit range (< 950u), directly visible, or Anti-Stall is active!
        if (iNearestEnemy > 0 && (flNearestDist < 950.0 || g_bAntiStallTriggered || CanSeeDirectly(id, iNearestEnemy)))
        {
            pev(iNearestEnemy, pev_origin, vecEnemyOrigin);
            vecEnemyOrigin[2] += 12.0;

            new Float:vecDiff[3], Float:vecAimAngles[3];
            xs_vec_sub(vecEnemyOrigin, vecBotEyes, vecDiff);
            vector_to_angle(vecDiff, vecAimAngles);
            ClampAngles(vecAimAngles);

            set_uc(uc_handle, UC_ViewAngles, vecAimAngles);
            set_uc(uc_handle, UC_ForwardMove, 400.0);

            // Close-range Melee Combat
            if (flNearestDist <= 85.0)
            {
                // Heavy Right-Click Stab (195 Backstab / 65 Frontal)
                iButtons |= IN_ATTACK2;

                // Side-strafe around enemy for backstab
                new Float:flSide = (id % 2 == 0) ? 250.0 : -250.0;
                set_uc(uc_handle, UC_SideMove, flSide);
            }
            else if (flNearestDist <= 115.0)
            {
                // Fast Left-Click Slash
                iButtons |= IN_ATTACK;
            }

            if (flNearestDist < 130.0 && flNearestDist > 70.0 && bIsOnGround && random_num(1, 100) <= 25)
            {
                iButtons |= IN_JUMP;
            }

            set_uc(uc_handle, UC_Buttons, iButtons);
            return FMRES_IGNORED;
        }
        else
        {
            // During open navigation: If bot has direct line of sight to Mid, guide forward
            if (CanSeeDirectlyToPoint(vecBotEyes, g_vecMapMidPoint))
            {
                new Float:vecDiff[3], Float:vecAimAngles[3];
                xs_vec_sub(g_vecMapMidPoint, vecBotEyes, vecDiff);
                vector_to_angle(vecDiff, vecAimAngles);
                ClampAngles(vecAimAngles);

                set_uc(uc_handle, UC_ViewAngles, vecAimAngles);
                set_uc(uc_handle, UC_ForwardMove, 400.0);
                set_uc(uc_handle, UC_SideMove, 0.0);
                set_uc(uc_handle, UC_Buttons, iButtons);
                return FMRES_IGNORED;
            }
            // Otherwise, let YaPB's waypoint graph pathfind out of corridors naturally at full speed!
            set_uc(uc_handle, UC_ForwardMove, 400.0);
            return FMRES_IGNORED;
        }
    }

    // -------------------------------------------------------------
    // STRICT SEPARATION OF CONCERNS MASTER GUARD (Option 3)
    // When bot_savage_micro_override == 0 (default), YaPB C++ owns 100%
    // of gun-round movement, aim, recoil, and firing. AMXX only handles
    // macro/meta layer (5-Role loadouts, team economy/drops, IGL callouts).
    // -------------------------------------------------------------
    if (!get_pcvar_num(g_pCvarMicroOverride))
    {
        if (g_bHoldingAngle[id])
        {
            ReleaseBotHoldAngle(id);
        }
        return FMRES_IGNORED;
    }

    // -------------------------------------------------------------
    // Tactical Lineup Throw Execution (Mid Smoke & Pop-Flash)
    // -------------------------------------------------------------
    if (g_bLineupActive[id])
    {
        if (flGameTime < g_flLineupThrowUntil[id])
        {
            set_uc(uc_handle, UC_ViewAngles, g_vecLineupAngles[id]);
            iButtons |= IN_ATTACK;
            set_uc(uc_handle, UC_Buttons, iButtons);
            set_uc(uc_handle, UC_ForwardMove, 150.0);
            return FMRES_HANDLED;
        }
        else
        {
            g_bLineupActive[id] = false;
            engclient_cmd(id, "lastinv");
        }
    }

    // -------------------------------------------------------------
    // Squad Pacing & Stealth Synchronization (v3.4)
    // -------------------------------------------------------------
    new CsTeams:teamPlayer = cs_get_user_team(id);
    if (flGameTime < g_flSilentWalkUntil[teamPlayer])
    {
        iButtons |= IN_RUN;
        iButtons &= ~IN_JUMP;
        new Float:flFwd, Float:flSide;
        get_uc(uc_handle, UC_ForwardMove, flFwd);
        get_uc(uc_handle, UC_SideMove, flSide);
        if (flFwd > 130.0) set_uc(uc_handle, UC_ForwardMove, 130.0);
        else if (flFwd < -130.0) set_uc(uc_handle, UC_ForwardMove, -130.0);
        if (flSide > 130.0) set_uc(uc_handle, UC_SideMove, 130.0);
        else if (flSide < -130.0) set_uc(uc_handle, UC_SideMove, -130.0);
    }

    // Hold Angle & Passive Defense (Engine-Safe Maxspeed)
    if (!IsWarmupActive() && flGameTime < g_flHoldAngleUntil[teamPlayer] && (flGameTime - g_flLastCombatTime[id] > 2.0))
    {
        if (!g_bHoldingAngle[id])
        {
            g_bHoldingAngle[id] = true;
            g_flOriginalMaxSpeed[id] = get_user_maxspeed(id);
        }
        set_user_maxspeed(id, 1.0);
        iButtons |= IN_DUCK;
    }
    // Rear Flank Anchor Protocol (Engine-Safe Maxspeed)
    else if (!IsWarmupActive() && flGameTime < g_flRearFlankUntil[id] && (flGameTime - g_flLastCombatTime[id] > 2.0))
    {
        if (!g_bHoldingAngle[id])
        {
            g_bHoldingAngle[id] = true;
            g_flOriginalMaxSpeed[id] = get_user_maxspeed(id);
        }
        set_uc(uc_handle, UC_ViewAngles, g_vecRearFlankAngle[id]);
        set_user_maxspeed(id, 1.0);
        iButtons |= IN_DUCK;
    }
    else if (g_bHoldingAngle[id] && flGameTime >= g_flHoldingUntil[id])
    {
        ReleaseBotHoldAngle(id);
    }

    // Trade-Frag Buddy Tether
    if (g_iTradeLeader[id] > 0 && is_user_alive(g_iTradeLeader[id]) && flGameTime < g_flTradeFollowUntil[id])
    {
        // Bomb carrier must never be tethered away from objectives!
        if (user_has_weapon(id, CSW_C4))
        {
            g_flTradeFollowUntil[id] = 0.0;
        }
        else
        {
            new Float:vecLeaderOrigin[3], Float:vecCurBotOrigin[3], Float:vecTetherDiff[3], Float:vecTetherAim[3];
            pev(g_iTradeLeader[id], pev_origin, vecLeaderOrigin);
            pev(id, pev_origin, vecCurBotOrigin);
            new Float:flLeaderDist = get_distance_f(vecCurBotOrigin, vecLeaderOrigin);
            if (flLeaderDist > 220.0)
            {
                new Float:vecBotEyes[3], Float:vecLeaderEyes[3];
                pev(id, pev_origin, vecBotEyes);
                vecBotEyes[2] += 17.0;
                pev(g_iTradeLeader[id], pev_origin, vecLeaderEyes);
                vecLeaderEyes[2] += 17.0;

                if (CanSeeDirectlyToPoint(vecBotEyes, vecLeaderEyes))
                {
                    xs_vec_sub(vecLeaderOrigin, vecCurBotOrigin, vecTetherDiff);
                    vector_to_angle(vecTetherDiff, vecTetherAim);
                    ClampAngles(vecTetherAim);
                    set_uc(uc_handle, UC_ViewAngles, vecTetherAim);
                    set_uc(uc_handle, UC_ForwardMove, 350.0);
                }
            }
        }
    }

    // Care & Danger Zone Awareness
    if (flGameTime < g_flDangerZoneExpire[teamPlayer] && (flGameTime - g_flLastCombatTime[id] > 2.0))
    {
        new Float:vecCurBotOrigin[3];
        pev(id, pev_origin, vecCurBotOrigin);
        new Float:flDistToDanger = get_distance_f(vecCurBotOrigin, g_vecDangerPoint[teamPlayer]);
        if (flDistToDanger <= 650.0 && flDistToDanger > 60.0)
        {
            new Float:vecEyes[3], Float:vecDiffToDanger[3], Float:vecDangerAngles[3];
            pev(id, pev_view_ofs, vecEyes);
            xs_vec_add(vecCurBotOrigin, vecEyes, vecEyes);
            xs_vec_sub(g_vecDangerPoint[teamPlayer], vecEyes, vecDiffToDanger);
            vector_to_angle(vecDiffToDanger, vecDangerAngles);
            ClampAngles(vecDangerAngles);
            set_uc(uc_handle, UC_ViewAngles, vecDangerAngles);
            set_uc(uc_handle, UC_ForwardMove, 160.0);
            iButtons |= IN_RUN;
        }
    }

    // -------------------------------------------------------------
    // Bombsite Rush / Strat Objective Guidance (First 25s of gun round)
    // -------------------------------------------------------------
    if (!IsKnifeRoundActive() && !IsWarmupActive() && g_bBombsitesFound && (flGameTime - g_flRoundStartTime < 25.0) && (flGameTime - g_flLastCombatTime[id] > 3.0))
    {
        new CsTeams:teamBot = cs_get_user_team(id);
        new iStrat = g_iTeamStrat[teamBot];
        if (iStrat == STRAT_RUSH_A || iStrat == STRAT_RUSH_B)
        {
            new Float:vecTargetSite[3];
            xs_vec_copy((iStrat == STRAT_RUSH_A) ? g_vecBombsiteA : g_vecBombsiteB, vecTargetSite);

            new Float:vecBotOrigin[3], Float:vecBotEyes[3];
            pev(id, pev_origin, vecBotOrigin);
            pev(id, pev_view_ofs, vecBotEyes);
            xs_vec_add(vecBotOrigin, vecBotEyes, vecBotEyes);

            if (CanSeeDirectlyToPoint(vecBotEyes, vecTargetSite))
            {
                new Float:vecDiff[3], Float:vecAimAngles[3];
                xs_vec_sub(vecTargetSite, vecBotEyes, vecDiff);
                vector_to_angle(vecDiff, vecAimAngles);
                ClampAngles(vecAimAngles);

                set_uc(uc_handle, UC_ViewAngles, vecAimAngles);
                set_uc(uc_handle, UC_ForwardMove, 400.0);
                set_uc(uc_handle, UC_SideMove, 0.0);
                set_uc(uc_handle, UC_Buttons, iButtons);
                return FMRES_IGNORED;
            }
        }
    }

    // -------------------------------------------------------------
    // Anti-Flash Dodge State: Bot is currently turning away & ducking
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarAntiFlash) && g_flFlashDodgeUntil[id] > flGameTime)
    {
        iButtons |= IN_DUCK;
        iButtons &= ~IN_ATTACK;
        set_uc(uc_handle, UC_Buttons, iButtons);
        return FMRES_IGNORED;
    }

    // -------------------------------------------------------------
    // High-IQ Fake Defuse Ambush State
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarFakeDefuse) && g_flFakeDefuseAmbushUntil[id] > flGameTime)
    {
        iButtons &= ~IN_USE;
        iButtons |= IN_DUCK;
        set_uc(uc_handle, UC_Buttons, iButtons);
        set_uc(uc_handle, UC_ViewAngles, g_vecFakeDefuseAmbushAngle[id]);
        set_uc(uc_handle, UC_ForwardMove, 0.0);
        set_uc(uc_handle, UC_SideMove, 0.0);
        return FMRES_IGNORED;
    }

    // -------------------------------------------------------------
    // Tactical Reload Cover: If out of ammo in combat, duck and back away
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarReloadCover) && bIsGun && (flGameTime - g_flLastCombatTime[id] < 3.0))
    {
        new iClip, iAmmo;
        get_user_weapon(id, iClip, iAmmo);
        if (iClip <= 0 && iAmmo > 0)
        {
            iButtons |= IN_DUCK;
            set_uc(uc_handle, UC_Buttons, iButtons);
            set_uc(uc_handle, UC_ForwardMove, -350.0);
            return FMRES_IGNORED;
        }
    }

    // -------------------------------------------------------------
    // Scout Jump-Apex Shot (CS 1.6 100% accuracy midair apex)
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarJumpScout))
    {
        if (iWeapon == CSW_SCOUT && !bIsOnGround)
        {
            new Float:vecVel[3];
            pev(id, pev_velocity, vecVel);
            if (floatabs(vecVel[2]) < 35.0 && (flGameTime - g_flLastCombatTime[id] < 1.5))
            {
                new Float:vecStart[3], Float:vecViewOfs[3], Float:vecVAngle[3], Float:vecForward[3], Float:vecEnd[3], Float:vecHitEnd[3];
                pev(id, pev_origin, vecStart);
                pev(id, pev_view_ofs, vecViewOfs);
                xs_vec_add(vecStart, vecViewOfs, vecStart);

                pev(id, pev_v_angle, vecVAngle);
                angle_vector(vecVAngle, ANGLEVECTOR_FORWARD, vecForward);
                xs_vec_mul_scalar(vecForward, 3500.0, vecForward);
                xs_vec_add(vecStart, vecForward, vecEnd);

                new iHit = trace_line(id, vecStart, vecEnd, vecHitEnd);
                if (iHit > 0 && iHit <= 32 && is_user_alive(iHit) && cs_get_user_team(id) != cs_get_user_team(iHit))
                {
                    iButtons |= IN_ATTACK;
                    set_uc(uc_handle, UC_Buttons, iButtons);
                }
            }
        }
    }

    // -------------------------------------------------------------
    // Zero-Latency Triggerbot & Enemy Contact Callout
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarTriggerbot) && bIsGun)
    {
        new Float:vecStart[3], Float:vecViewOfs[3], Float:vecVAngle[3], Float:vecForward[3], Float:vecEnd[3], Float:vecHitEnd[3];
        pev(id, pev_origin, vecStart);
        pev(id, pev_view_ofs, vecViewOfs);
        xs_vec_add(vecStart, vecViewOfs, vecStart);

        pev(id, pev_v_angle, vecVAngle);
        angle_vector(vecVAngle, ANGLEVECTOR_FORWARD, vecForward);
        xs_vec_mul_scalar(vecForward, 3500.0, vecForward);
        xs_vec_add(vecStart, vecForward, vecEnd);

        new iHit = trace_line(id, vecStart, vecEnd, vecHitEnd);
        if (iHit > 0 && iHit <= 32 && is_user_alive(iHit))
        {
            new CsTeams:teamBot = cs_get_user_team(id);
            new CsTeams:teamHit = cs_get_user_team(iHit);

            if (teamBot != teamHit && teamHit != CS_TEAM_UNASSIGNED && teamHit != CS_TEAM_SPECTATOR)
            {
                new bool:bIsAutoRifle = (!IsPistolWeapon(iWeapon) && iWeapon != CSW_AWP && iWeapon != CSW_SCOUT);
                if (bIsAutoRifle)
                {
                    if (flGameTime < g_flTriggerBurstUntil[id])
                    {
                        // Active burst window (1-2 crisp shots)
                        iButtons |= IN_ATTACK;
                        set_uc(uc_handle, UC_Buttons, iButtons);
                    }
                    else if (flGameTime < g_flNextTriggerCheck[id])
                    {
                        // Controlled throttle interval to prevent wild full-spray recoil overcompensation
                        iButtons &= ~IN_ATTACK;
                        set_uc(uc_handle, UC_Buttons, iButtons);
                    }
                    else
                    {
                        // Initiate new controlled burst/tap (1-2 shots, then throttle)
                        new bool:bTwoShot = (random_num(1, 100) <= 40);
                        new Float:flBurstTime = bTwoShot ? 0.12 : 0.04;
                        g_flTriggerBurstUntil[id] = flGameTime + flBurstTime;
                        g_flNextTriggerCheck[id] = g_flTriggerBurstUntil[id] + random_float(0.08, 0.14);

                        iButtons |= IN_ATTACK;
                        set_uc(uc_handle, UC_Buttons, iButtons);
                    }
                }
                else
                {
                    // Semi-auto pistols & Snipers: immediate crisp single trigger
                    iButtons |= IN_ATTACK;
                    set_uc(uc_handle, UC_Buttons, iButtons);
                }

                g_flLastCombatTime[id] = flGameTime;
                ReleaseBotHoldAngle(id);

                if (get_pcvar_num(g_pCvarCalls) && flGameTime > g_flNextContactCall[teamBot])
                {
                    g_flNextContactCall[teamBot] = flGameTime + 5.0;
                    new szBotName[32];
                    get_user_name(id, szBotName, charsmax(szBotName));
                    BroadcastTeamCall(teamBot, "^4[Intel]^3 %s %s^1: Enemy spotted! Watch the angle!", g_szRoleTags[iRole], szBotName);
                    PlayTeamAudio(teamBot, "spk radio/enemys");
                }

                if (get_pcvar_num(g_pCvarHiveMind))
                {
                    new Float:vecEnemyOrigin[3];
                    pev(iHit, pev_origin, vecEnemyOrigin);
                    g_vecTeamThreatOrigin[teamBot] = vecEnemyOrigin;
                    g_flTeamThreatTime[teamBot] = flGameTime;
                    g_iTeamThreatTarget[teamBot] = iHit;
                }
            }
        }
        else if (flGameTime - g_flLastCombatTime[id] > 0.4)
        {
            g_flNextTriggerCheck[id] = 0.0;
            g_flTriggerBurstUntil[id] = 0.0;
        }
    }

    // -------------------------------------------------------------
    // Auditory Sound Wallbang Burst Engine
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarWallbang) && g_iSoundTarget[id] > 0 && CanWallbang(iWeapon))
    {
        if (flGameTime < g_flSoundTargetExpire[id] && is_user_alive(g_iSoundTarget[id]))
        {
            if (flGameTime - g_flLastCombatTime[id] > 1.0)
            {
                new Float:vecBotEyes[3], Float:vecDiff[3], Float:vecAimAngles[3];
                pev(id, pev_origin, vecBotEyes);
                vecBotEyes[2] += 17.0;

                xs_vec_sub(g_vecSoundTargetOrigin[id], vecBotEyes, vecDiff);
                new Float:flDist = xs_vec_len(vecDiff);
                if (flDist > 1.0)
                {
                    vector_to_angle(vecDiff, vecAimAngles);
                    ClampAngles(vecAimAngles);
                    set_uc(uc_handle, UC_ViewAngles, vecAimAngles);

                    if (flGameTime > g_flNextWallbangBurst[id])
                    {
                        iButtons |= IN_ATTACK;
                        set_uc(uc_handle, UC_Buttons, iButtons);
                        g_flNextWallbangBurst[id] = flGameTime + random_float(0.12, 0.22);
                    }
                }
            }
        }
        else
        {
            g_iSoundTarget[id] = 0;
        }
    }

    // -------------------------------------------------------------
    // Trade-Frag Tracking
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarTradeFrag) && g_iTradeTarget[id] > 0)
    {
        if (flGameTime < g_flTradeExpire[id] && is_user_alive(g_iTradeTarget[id]))
        {
            new Float:vecBotOrigin[3], Float:vecTargetOrigin[3], Float:vecDiff[3], Float:vecAimAngles[3];
            pev(id, pev_origin, vecBotOrigin);
            vecBotOrigin[2] += 17.0;
            pev(g_iTradeTarget[id], pev_origin, vecTargetOrigin);

            xs_vec_sub(vecTargetOrigin, vecBotOrigin, vecDiff);
            new Float:flDist = xs_vec_len(vecDiff);
            if (flDist > 1.0)
            {
                vector_to_angle(vecDiff, vecAimAngles);
                ClampAngles(vecAimAngles);

                set_uc(uc_handle, UC_ViewAngles, vecAimAngles);
                set_uc(uc_handle, UC_ForwardMove, 400.0);
                g_flLastCombatTime[id] = flGameTime;
            }
        }
        else
        {
            g_iTradeTarget[id] = 0;
        }
    }

    // -------------------------------------------------------------
    // Synchronized Double-Peek (Anti-AWP Protocol)
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarDoublePeek) && g_iDoublePeekPartner[id] > 0)
    {
        if (flGameTime < g_flDoublePeekExpire[id] && is_user_alive(g_iDoublePeekPartner[id]))
        {
            if (g_bIsCrouchPeeker[id])
            {
                iButtons |= IN_DUCK;
                set_uc(uc_handle, UC_Buttons, iButtons);
                set_uc(uc_handle, UC_SideMove, 300.0);
            }
            else
            {
                set_uc(uc_handle, UC_SideMove, 400.0);
                set_uc(uc_handle, UC_ForwardMove, 250.0);
            }
        }
        else
        {
            g_iDoublePeekPartner[id] = 0;
        }
    }

    // -------------------------------------------------------------
    // Perfect Recoil Compensation (Laser Spray Transfer)
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarRecoil) && (iButtons & IN_ATTACK) && bIsGun)
    {
        new Float:vecPunch[3];
        pev(id, pev_punchangle, vecPunch);

        if (vecPunch[0] != 0.0 || vecPunch[1] != 0.0)
        {
            new Float:vecAngles[3];
            get_uc(uc_handle, UC_ViewAngles, vecAngles);

            vecAngles[0] -= (vecPunch[0] * 2.0);
            vecAngles[1] -= (vecPunch[1] * 2.0);

            ClampAngles(vecAngles);
            set_uc(uc_handle, UC_ViewAngles, vecAngles);
        }
    }

    // -------------------------------------------------------------
    // Instant Counter-Strafing: Cancel inertia when shooting
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarCounterStrafe) && (iButtons & IN_ATTACK) && bIsGun)
    {
        if (bIsOnGround)
        {
            new Float:vecVel[3];
            pev(id, pev_velocity, vecVel);
            vecVel[0] *= 0.08;
            vecVel[1] *= 0.08;
            set_pev(id, pev_velocity, vecVel);

            set_uc(uc_handle, UC_ForwardMove, 0.0);
            set_uc(uc_handle, UC_SideMove, 0.0);
        }
        g_flLastCombatTime[id] = flGameTime;
    }
    // -------------------------------------------------------------
    // Combat AD-AD Jiggle Strafe & Crouch-Tap during duels
    // -------------------------------------------------------------
    else if (get_pcvar_num(g_pCvarJiggle) && (flGameTime - g_flLastCombatTime[id] < 2.5) && bIsOnGround)
    {
        if (flGameTime > g_flNextStrafeSwitch[id])
        {
            g_bStrafeDirection[id] = !g_bStrafeDirection[id];
            g_flNextStrafeSwitch[id] = flGameTime + random_float(0.20, 0.35);
            g_bDuckTapActive[id] = (random_num(1, 100) <= 45);
        }

        new Float:flSide = g_bStrafeDirection[id] ? 400.0 : -400.0;
        set_uc(uc_handle, UC_SideMove, flSide);

        if (g_bDuckTapActive[id])
        {
            iButtons |= IN_DUCK;
            set_uc(uc_handle, UC_Buttons, iButtons);
        }
    }
    // -------------------------------------------------------------
    // Ground Weapon Scavenge & Upgrade Navigation
    // -------------------------------------------------------------
    else if (get_pcvar_num(g_pCvarWeaponScavenge) && g_iScavengeTarget[id] > 0 && !IsWarmupActive() && !IsKnifeRoundActive())
    {
        if (pev_valid(g_iScavengeTarget[id]) && flGameTime < g_flScavengeExpire[id])
        {
            if (!g_bRoundEnded && (flGameTime - g_flLastCombatTime[id] < 1.0))
            {
                g_iScavengeTarget[id] = 0;
            }
            else
            {
                new Float:vecBotOrigin[3], Float:vecWpnOrigin[3];
                pev(id, pev_origin, vecBotOrigin);
                pev(g_iScavengeTarget[id], pev_origin, vecWpnOrigin);

                new Float:flDist = get_distance_f(vecBotOrigin, vecWpnOrigin);

                if (flDist <= 95.0)
                {
                    new iGroundWpn = GetGroundWeaponType(g_iScavengeTarget[id]);
                    new iCurPrim = GetUserPrimaryWeapon(id);
                    if (GetRoleWeaponTier(id, iGroundWpn, g_bRoundEnded) > GetRoleWeaponTier(id, iCurPrim, g_bRoundEnded))
                    {
                        DropUserPrimary(id);
                        ExecuteHamB(Ham_Touch, g_iScavengeTarget[id], id);
                        g_flLastWeaponSwapTime[id] = flGameTime;
                    }
                    g_iScavengeTarget[id] = 0;
                }
                else if (CanSeeDirectlyToPoint(vecBotOrigin, vecWpnOrigin) || g_bRoundEnded)
                {
                    new Float:vecBotEyes[3], Float:vecDiff[3], Float:vecAimAngles[3];
                    pev(id, pev_view_ofs, vecBotEyes);
                    xs_vec_add(vecBotOrigin, vecBotEyes, vecBotEyes);
                    xs_vec_sub(vecWpnOrigin, vecBotEyes, vecDiff);
                    vector_to_angle(vecDiff, vecAimAngles);
                    ClampAngles(vecAimAngles);

                    set_uc(uc_handle, UC_ViewAngles, vecAimAngles);
                    set_uc(uc_handle, UC_ForwardMove, 400.0);
                    set_uc(uc_handle, UC_SideMove, 0.0);
                    return FMRES_IGNORED;
                }
            }
        }
        else
        {
            g_iScavengeTarget[id] = 0;
        }
    }
    // -------------------------------------------------------------
    // REFACTORED: Engine-Safe Holding Angle & Crossfire State (Argon Framework)
    // -------------------------------------------------------------
    else if (get_pcvar_num(g_pCvarHolding) && !IsWarmupActive())
    {
        if (g_bHoldingAngle[id])
        {
            if (flGameTime < g_flHoldingUntil[id])
            {
                // Engine-Safe Speed Restriction: Halt physically without breaking usercmd pipeline
                // Prevents YaPB Bot::isStuck() panic cascade while preserving A* pathing mesh integrity
                set_user_maxspeed(id, 1.0);

                new Float:vecAngles[3];
                get_uc(uc_handle, UC_ViewAngles, vecAngles);
                vecAngles[0] = 0.0;
                ClampAngles(vecAngles);
                set_uc(uc_handle, UC_ViewAngles, vecAngles);

                if (iWeapon == CSW_AWP || iWeapon == CSW_SCOUT)
                {
                    iButtons |= IN_DUCK;
                }

                set_uc(uc_handle, UC_Buttons, iButtons);
                return FMRES_IGNORED;
            }
            else
            {
                ReleaseBotHoldAngle(id);
            }
        }
        else if (flGameTime >= g_flNextHoldCheck[id])
        {
            // 1Hz Chronological Tick-Gate: Evaluate holding decision once every 1.0 to 1.5 seconds
            g_flNextHoldCheck[id] = flGameTime + random_float(1.0, 1.5);

            // Strict Role Targeting: Only AWPer and Lurker hold passive angles, never blanket CT
            if ((iRole == ROLE_AWPER || iRole == ROLE_LURKER) && bIsOnGround && (flGameTime - g_flLastCombatTime[id] > 6.0))
            {
                if (random_num(1, 100) <= 15)
                {
                    g_bHoldingAngle[id] = true;
                    g_flOriginalMaxSpeed[id] = get_user_maxspeed(id);
                    g_flHoldingUntil[id] = flGameTime + random_float(2.0, 3.5);
                    set_user_maxspeed(id, 1.0);
                }
            }
        }

        // Stealth shift-walking for Lurker
        if (iRole == ROLE_LURKER && bIsOnGround)
        {
            new Float:flFwd;
            get_uc(uc_handle, UC_ForwardMove, flFwd);
            if (flFwd > 120.0)
            {
                set_uc(uc_handle, UC_ForwardMove, 130.0);
            }
        }

        // Tactical Jump-Peek (Only if bot_savage_bhop is explicitly enabled)
        if (get_pcvar_num(g_pCvarBhop) && bIsOnGround && random_num(1, 100) <= 8)
        {
            iButtons |= IN_JUMP;
            set_uc(uc_handle, UC_Buttons, iButtons);
        }
    }

    // -------------------------------------------------------------
    // Fake Defuse Trigger Check (Near planted C4)
    // -------------------------------------------------------------
    if (get_pcvar_num(g_pCvarFakeDefuse) && !g_bDidFakeDefuse[id] && cs_get_user_team(id) == CS_TEAM_CT)
    {
        CheckForPlantedC4FakeDefuse(id);
    }

    return FMRES_IGNORED;
}
