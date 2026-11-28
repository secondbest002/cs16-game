#if defined _savage_yapb_bridge_included
    #endinput
#endif
#define _savage_yapb_bridge_included

// ==============================================================================
// YaPB C++ <-> AMXX Arbitration Bridge (BP-01)
// Manages time-bounded Action Leases (LookAt / GoalOrigin / Utility Throw)
// with Combat Preemption and clean handback so AMXX never fights YaPB C++.
// ==============================================================================

const SAVAGE_PRIMARY_WEAPONS_BITSUM = (
    (1 << CSW_SCOUT) | (1 << CSW_XM1014) | (1 << CSW_MAC10) | (1 << CSW_AUG) |
    (1 << CSW_UMP45) | (1 << CSW_SG550) | (1 << CSW_GALIL) | (1 << CSW_FAMAS) |
    (1 << CSW_AWP) | (1 << CSW_MP5NAVY) | (1 << CSW_M249) | (1 << CSW_M3) |
    (1 << CSW_M4A1) | (1 << CSW_TMP) | (1 << CSW_G3SG1) | (1 << CSW_SG552) |
    (1 << CSW_AK47) | (1 << CSW_P90)
);

const SAVAGE_SECONDARY_WEAPONS_BITSUM = (
    (1 << CSW_P228) | (1 << CSW_ELITE) | (1 << CSW_FIVESEVEN) |
    (1 << CSW_USP) | (1 << CSW_GLOCK18) | (1 << CSW_DEAGLE)
);

Savage_Bridge_Init()
{
    g_bYapbAvailable = (LibraryExists("yapb", LibType_Library) && yb_get_api_version() > 0);

    for (new i = 1; i <= 32; i++)
    {
        Savage_Bridge_ResetBot(i);
    }

    server_print("[Bot Savage Bridge] YaPB Native Bridge %s.",
        g_bYapbAvailable ? "ONLINE" : "FALLBACK (yapb library unavailable)");
}

Savage_Bridge_ResetBot(id)
{
    g_iBotLeaseType[id] = LEASE_NONE;
    g_flBotLeaseUntil[id] = 0.0;
    g_flBotLeaseStart[id] = 0.0;
    g_vecBotLeaseTarget[id][0] = 0.0;
    g_vecBotLeaseTarget[id][1] = 0.0;
    g_vecBotLeaseTarget[id][2] = 0.0;
    g_iBotLeaseWeaponCSW[id] = 0;
    g_bBotLeaseAttackSent[id] = false;
}

stock Savage_Bridge_ComputeLobAimPoint(const Float:vecEyes[3], const Float:vecTarget[3], Float:flPitchDeg, Float:vecOutAimPoint[3])
{
    new Float:vecDiff[3], Float:vecAngles[3], Float:vecLobDir[3];
    xs_vec_sub(vecTarget, vecEyes, vecDiff);
    vector_to_angle(vecDiff, vecAngles);
    vecAngles[0] = flPitchDeg;
    ClampAngles(vecAngles);

    angle_vector(vecAngles, ANGLEVECTOR_FORWARD, vecLobDir);
    xs_vec_mul_scalar(vecLobDir, 500.0, vecLobDir);
    xs_vec_add(vecEyes, vecLobDir, vecOutAimPoint);
}

bool:Savage_Bridge_HasActiveEnemy(id)
{
    if (g_bYapbAvailable && yb_is_user_bot(id))
    {
        new iEnemy = yb_get_bot_enemy(id);
        if (iEnemy > 0 && iEnemy <= MaxClients && is_user_alive(iEnemy))
            return true;

        if (yb_get_bot_task(id) == YB_TASK_ATTACK)
            return true;
    }

    if (get_gametime() - g_flLastCombatTime[id] < 0.35)
        return true;

    return false;
}

stock Savage_Bridge_RestoreBestWeapon(id)
{
    if (!is_user_alive(id))
        return;

    new iWeapons = pev(id, pev_weapons);
    new szWpnName[32];

    for (new wpn = CSW_P228; wpn <= CSW_P90; wpn++)
    {
        if ((SAVAGE_PRIMARY_WEAPONS_BITSUM & (1 << wpn)) && (iWeapons & (1 << wpn)))
        {
            get_weaponname(wpn, szWpnName, charsmax(szWpnName));
            engclient_cmd(id, szWpnName);
            return;
        }
    }

    for (new wpn = CSW_P228; wpn <= CSW_P90; wpn++)
    {
        if ((SAVAGE_SECONDARY_WEAPONS_BITSUM & (1 << wpn)) && (iWeapons & (1 << wpn)))
        {
            get_weaponname(wpn, szWpnName, charsmax(szWpnName));
            engclient_cmd(id, szWpnName);
            return;
        }
    }

    engclient_cmd(id, "weapon_knife");
}

Savage_Bridge_ReleaseLease(id)
{
    new SavageLeaseType:ePrevLease = g_iBotLeaseType[id];
    Savage_Bridge_ResetBot(id);

    if (!is_user_connected(id) || !is_user_alive(id))
        return;

    if (ePrevLease == LEASE_UTILITY_THROW || ePrevLease == LEASE_TACTICAL_HOLD || ePrevLease == LEASE_HAZARD_EVADE)
    {
        if (g_bYapbAvailable && yb_is_user_bot(id))
        {
            yb_set_bot_look_at(id, Float:{0.0, 0.0, 0.0});

            if (ePrevLease == LEASE_HAZARD_EVADE)
            {
                new Float:vecCur[3];
                pev(id, pev_origin, vecCur);
                yb_set_bot_goal_origin(id, vecCur);
            }
        }
    }

    if (ePrevLease == LEASE_UTILITY_THROW)
    {
        Savage_Bridge_RestoreBestWeapon(id);
    }
}

bool:Savage_Bridge_RequestUtilityThrow(id, iWeaponCSW, const Float:vecAimPoint[3], Float:flDuration = 0.65)
{
    if (id < 1 || id > MaxClients || !is_user_bot(id) || !is_user_alive(id))
        return false;

    new Float:flNow = get_gametime();

    // Do not preempt higher-priority leases (e.g., LEASE_HAZARD_EVADE)
    if (g_iBotLeaseType[id] > LEASE_UTILITY_THROW && flNow < g_flBotLeaseUntil[id])
        return false;

    // Combat Preemption: never pull a grenade while an enemy is actively engaged
    if (Savage_Bridge_HasActiveEnemy(id))
        return false;

    if (!user_has_weapon(id, iWeaponCSW))
        return false;

    g_iBotLeaseType[id] = LEASE_UTILITY_THROW;
    g_flBotLeaseStart[id] = flNow;
    g_flBotLeaseUntil[id] = flNow + floatclamp(flDuration, 0.45, 1.50);
    xs_vec_copy(vecAimPoint, g_vecBotLeaseTarget[id]);
    g_iBotLeaseWeaponCSW[id] = iWeaponCSW;
    g_bBotLeaseAttackSent[id] = false;

    new szWpnName[32];
    get_weaponname(iWeaponCSW, szWpnName, charsmax(szWpnName));
    engclient_cmd(id, szWpnName);

    if (g_bYapbAvailable && yb_is_user_bot(id))
    {
        yb_set_bot_look_at(id, vecAimPoint);
    }

    return true;
}

stock bool:Savage_Bridge_RequestEvade(id, const Float:vecSafeOrigin[3], Float:flDuration = 1.25)
{
    if (id < 1 || id > MaxClients || !is_user_bot(id) || !is_user_alive(id))
        return false;

    if (g_iBotLeaseType[id] != LEASE_NONE && g_iBotLeaseType[id] != LEASE_HAZARD_EVADE)
    {
        Savage_Bridge_ReleaseLease(id);
    }

    new Float:flNow = get_gametime();
    g_iBotLeaseType[id] = LEASE_HAZARD_EVADE;
    g_flBotLeaseStart[id] = flNow;
    g_flBotLeaseUntil[id] = flNow + floatclamp(flDuration, 0.45, 2.50);
    xs_vec_copy(vecSafeOrigin, g_vecBotLeaseTarget[id]);
    g_iBotLeaseWeaponCSW[id] = 0;
    g_bBotLeaseAttackSent[id] = false;

    if (g_bYapbAvailable && yb_is_user_bot(id))
    {
        yb_set_bot_goal_origin(id, vecSafeOrigin);
    }

    return true;
}

stock bool:Savage_Bridge_RequestHold(id, const Float:vecLookOrigin[3], Float:flDuration = 1.50)
{
    if (id < 1 || id > MaxClients || !is_user_bot(id) || !is_user_alive(id))
        return false;

    new Float:flNow = get_gametime();
    if (g_iBotLeaseType[id] > LEASE_TACTICAL_HOLD && flNow < g_flBotLeaseUntil[id])
        return false;

    if (Savage_Bridge_HasActiveEnemy(id))
        return false;

    g_iBotLeaseType[id] = LEASE_TACTICAL_HOLD;
    g_flBotLeaseStart[id] = flNow;
    g_flBotLeaseUntil[id] = flNow + floatclamp(flDuration, 0.35, 4.00);
    xs_vec_copy(vecLookOrigin, g_vecBotLeaseTarget[id]);
    g_iBotLeaseWeaponCSW[id] = 0;
    g_bBotLeaseAttackSent[id] = false;

    if (g_bYapbAvailable && yb_is_user_bot(id))
    {
        yb_set_bot_look_at(id, vecLookOrigin);
    }

    return true;
}

bool:Savage_Bridge_ProcessTick(id, uc_handle, Float:flGameTime)
{
    // Fast-path early return: zero overhead when bot is under 100% YaPB control
    if (g_iBotLeaseType[id] == LEASE_NONE)
        return false;

    // Lease expiration check
    if (flGameTime >= g_flBotLeaseUntil[id])
    {
        Savage_Bridge_ReleaseLease(id);
        return false;
    }

    // Combat Preemption: abort non-evade lease immediately if enemy spotted before pin release
    if (g_iBotLeaseType[id] < LEASE_HAZARD_EVADE && !g_bBotLeaseAttackSent[id] && Savage_Bridge_HasActiveEnemy(id))
    {
        Savage_Bridge_ReleaseLease(id);
        return false;
    }

    new iButtons = get_uc(uc_handle, UC_Buttons);
    new bool:bYapbBot = (g_bYapbAvailable && yb_is_user_bot(id));

    switch (g_iBotLeaseType[id])
    {
        case LEASE_UTILITY_THROW:
        {
            new iTargetWpn = g_iBotLeaseWeaponCSW[id];

            // Once grenade leaves inventory, immediately restore primary weapon & release lease
            if (!user_has_weapon(id, iTargetWpn))
            {
                Savage_Bridge_ReleaseLease(id);
                return false;
            }

            if (get_user_weapon(id) != iTargetWpn)
            {
                new szWpnName[32];
                get_weaponname(iTargetWpn, szWpnName, charsmax(szWpnName));
                engclient_cmd(id, szWpnName);
            }

            if (bYapbBot)
            {
                yb_set_bot_look_at(id, g_vecBotLeaseTarget[id]);
            }
            else
            {
                new Float:vecEyes[3], Float:vecViewOfs[3], Float:vecDiff[3], Float:vecAngles[3];
                pev(id, pev_origin, vecEyes);
                pev(id, pev_view_ofs, vecViewOfs);
                xs_vec_add(vecEyes, vecViewOfs, vecEyes);
                xs_vec_sub(g_vecBotLeaseTarget[id], vecEyes, vecDiff);
                vector_to_angle(vecDiff, vecAngles);
                ClampAngles(vecAngles);
                set_uc(uc_handle, UC_ViewAngles, vecAngles);
            }

            new Float:flElapsed = flGameTime - g_flBotLeaseStart[id];
            if (flElapsed < 0.25)
            {
                // Phase 1: Aim settle & weapon deploy
                iButtons &= ~IN_ATTACK;
                set_uc(uc_handle, UC_Buttons, iButtons);
            }
            else if (!g_bBotLeaseAttackSent[id])
            {
                // Phase 2: Pull pin briefly (0.12s) then mark ready to release
                iButtons |= IN_ATTACK;
                set_uc(uc_handle, UC_Buttons, iButtons);

                if (flElapsed >= 0.37)
                {
                    g_bBotLeaseAttackSent[id] = true;
                }
            }
            else
            {
                // Phase 3: Release IN_ATTACK to launch grenade
                iButtons &= ~IN_ATTACK;
                set_uc(uc_handle, UC_Buttons, iButtons);
            }

            return true;
        }

        case LEASE_HAZARD_EVADE:
        {
            new Float:vecOrigin[3];
            pev(id, pev_origin, vecOrigin);

            if (get_distance_f(vecOrigin, g_vecBotLeaseTarget[id]) <= 48.0)
            {
                Savage_Bridge_ReleaseLease(id);
                return false;
            }

            if (!bYapbBot)
            {
                new Float:vecDiff[3], Float:vecAngles[3];
                xs_vec_sub(g_vecBotLeaseTarget[id], vecOrigin, vecDiff);
                vector_to_angle(vecDiff, vecAngles);
                vecAngles[0] = 0.0;
                ClampAngles(vecAngles);
                set_uc(uc_handle, UC_ViewAngles, vecAngles);
                set_uc(uc_handle, UC_ForwardMove, 400.0);
            }

            return true;
        }

        case LEASE_TACTICAL_HOLD:
        {
            if (bYapbBot)
            {
                yb_set_bot_look_at(id, g_vecBotLeaseTarget[id]);
            }

            set_uc(uc_handle, UC_ForwardMove, 0.0);
            set_uc(uc_handle, UC_SideMove, 0.0);
            return true;
        }
    }

    return false;
}

stock bool:Savage_Bridge_CanRouteMacro(id, bool:bAllowBombCarrier = true)
{
    if (!g_bYapbAvailable || id < 1 || id > MaxClients || !is_user_alive(id) || !yb_is_user_bot(id))
        return false;

    if (g_iBotLeaseType[id] != LEASE_NONE)
        return false;

    if (Savage_Bridge_HasActiveEnemy(id))
        return false;

    if (!bAllowBombCarrier && user_has_weapon(id, CSW_C4))
        return false;

    new YBTask:task = yb_get_bot_task(id);
    if (task == YB_TASK_ATTACK || task == YB_TASK_PLANT_BOMB || task == YB_TASK_DEFUSE_BOMB ||
        task == YB_TASK_ESCAPE_FROM_BOMB || task == YB_TASK_SEEK_COVER || task == YB_TASK_BLIND)
    {
        return false;
    }

    return true;
}

stock bool:Savage_Bridge_RouteMacroGoal(id, const Float:vecOrigin[3], Float:flCooldown = 2.5, bool:bAllowBombCarrier = true)
{
    if (!Savage_Bridge_CanRouteMacro(id, bAllowBombCarrier))
        return false;

    new Float:flNow = get_gametime();
    if (flCooldown > 0.0 && flNow < g_flNextMacroGoalPush[id])
        return false;

    new Float:vecCur[3];
    pev(id, pev_origin, vecCur);
    if (get_distance_f(vecCur, vecOrigin) <= 140.0)
        return false;

    yb_set_bot_goal_origin(id, vecOrigin);
    g_flNextMacroGoalPush[id] = flNow + flCooldown;
    return true;
}
