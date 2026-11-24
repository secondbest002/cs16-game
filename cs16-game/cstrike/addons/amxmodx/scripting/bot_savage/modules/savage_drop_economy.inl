#if defined _savage_drop_economy_included
    #endinput
#endif
#define _savage_drop_economy_included

// ==============================================================================
// 7. INTERACTIVE WEAPON DROP ENGINE
// ==============================================================================
public HandleSay(id)
{
    new szMessage[64];
    read_args(szMessage, charsmax(szMessage));
    remove_quotes(szMessage);
    trim(szMessage);

    if (szMessage[0] == EOS)
        return PLUGIN_CONTINUE;

    // Process v3.4 Tactical Chat Calls first if enabled
    if (get_pcvar_num(g_pCvarChatCalls) && is_user_connected(id) && !is_user_bot(id))
    {
        if (ProcessTacticalChatCall(id, szMessage))
            return PLUGIN_CONTINUE;
    }

    if (equali(szMessage, "/role") || equali(szMessage, "role") || equali(szMessage, "/roles"))
    {
        ShowRoleMenu(id);
        return PLUGIN_HANDLED;
    }

    if (equali(szMessage, "/strat") || equali(szMessage, "/strats") || equali(szMessage, "strat"))
    {
        Cmd_OpenStratMenu(id);
        return PLUGIN_HANDLED;
    }

    // -------------------------------------------------------------
    // INTERACTIVE TEAM ECONOMY CALLS (Eco, Force Buy, Half Buy, Full Buy)
    // -------------------------------------------------------------
    if (equali(szMessage, "full eco"))
    {
        Cmd_SetTeamEconomy(id, ECON_ECO, true);
        return PLUGIN_HANDLED;
    }
    else if (equali(szMessage, "/eco") || equali(szMessage, "eco") || equali(szMessage, "!eco") || 
             equali(szMessage, "eco round") || equali(szMessage, "lets eco") || equali(szMessage, "go eco") ||
             equali(szMessage, "save") || equali(szMessage, "/save") || equali(szMessage, "save round"))
    {
        Cmd_SetTeamEconomy(id, ECON_ECO, false);
        return PLUGIN_HANDLED;
    }
    else if (equali(szMessage, "/force") || equali(szMessage, "force") || equali(szMessage, "/forcebuy") || 
             equali(szMessage, "force buy") || equali(szMessage, "forcebuy"))
    {
        Cmd_SetTeamEconomy(id, ECON_FORCE);
        return PLUGIN_HANDLED;
    }
    else if (equali(szMessage, "/half") || equali(szMessage, "half") || equali(szMessage, "/halfbuy") || 
             equali(szMessage, "half buy") || equali(szMessage, "halfbuy") || equali(szMessage, "semi buy"))
    {
        Cmd_SetTeamEconomy(id, ECON_HALF);
        return PLUGIN_HANDLED;
    }
    else if (equali(szMessage, "/full") || equali(szMessage, "full buy") || equali(szMessage, "/fullbuy") || equali(szMessage, "buy"))
    {
        Cmd_SetTeamEconomy(id, ECON_FULL);
        return PLUGIN_HANDLED;
    }

    // -------------------------------------------------------------
    // INTERACTIVE TACTICAL STRAT CALLS (Rush A, Rush B, Default, Split)
    // -------------------------------------------------------------
    if (equali(szMessage, "/rusha") || equali(szMessage, "rush a") || equali(szMessage, "rush site a") || equali(szMessage, "a"))
    {
        SetPlayerStrat(id, STRAT_RUSH_A);
        return PLUGIN_HANDLED;
    }
    else if (equali(szMessage, "/rushb") || equali(szMessage, "rush b") || equali(szMessage, "rush site b") || equali(szMessage, "b"))
    {
        SetPlayerStrat(id, STRAT_RUSH_B);
        return PLUGIN_HANDLED;
    }
    else if (equali(szMessage, "/default") || equali(szMessage, "default") || equali(szMessage, "play slow") || equali(szMessage, "hold"))
    {
        SetPlayerStrat(id, STRAT_DEFAULT);
        return PLUGIN_HANDLED;
    }
    else if (equali(szMessage, "/split") || equali(szMessage, "split") || equali(szMessage, "split mid") || equali(szMessage, "mid"))
    {
        SetPlayerStrat(id, STRAT_SPLIT);
        return PLUGIN_HANDLED;
    }

    if (containi(szMessage, "drop") != -1)
    {
        new targetWeapon = 0;
        new m4Variant = 0;
        if (containi(szMessage, "deagle") != -1 || containi(szMessage, "dgl") != -1)
            targetWeapon = CSW_DEAGLE;
        else if (containi(szMessage, "awp") != -1 || containi(szMessage, "sniper") != -1)
            targetWeapon = CSW_AWP;
        else if (containi(szMessage, "ak") != -1 || containi(szMessage, "ak47") != -1 || containi(szMessage, "kalash") != -1)
            targetWeapon = CSW_AK47;
        else if (containi(szMessage, "m4a4") != -1 || containi(szMessage, "a4") != -1)
        {
            targetWeapon = CSW_M4A1;
            m4Variant = 1;
        }
        else if (containi(szMessage, "m4a1") != -1 || containi(szMessage, "a1") != -1)
        {
            targetWeapon = CSW_M4A1;
            m4Variant = 2;
        }
        else if (containi(szMessage, "m4") != -1)
        {
            targetWeapon = CSW_M4A1;
            m4Variant = 0; // Default to requester's preferred variant
        }
        else if (containi(szMessage, "galil") != -1)
            targetWeapon = CSW_GALIL;
        else if (containi(szMessage, "famas") != -1)
            targetWeapon = CSW_FAMAS;
        else if (containi(szMessage, "mp7") != -1 || containi(szMessage, "mp5") != -1)
            targetWeapon = CSW_MP5NAVY;
        else if (containi(szMessage, "tec9") != -1 || containi(szMessage, "tec") != -1)
            targetWeapon = CSW_P228;
        else if (containi(szMessage, "cz75") != -1 || containi(szMessage, "cz") != -1)
            targetWeapon = (cs_get_user_team(id) == CS_TEAM_T) ? CSW_MAC10 : CSW_TMP;
        else if (containi(szMessage, "bizon") != -1 || containi(szMessage, "p90") != -1)
            targetWeapon = CSW_P90;
        else if (containi(szMessage, "mag7") != -1 || containi(szMessage, "mag") != -1 || containi(szMessage, "xm1014") != -1)
            targetWeapon = CSW_XM1014;
        else if (containi(szMessage, "sawedoff") != -1 || containi(szMessage, "sawed") != -1 || containi(szMessage, "m3") != -1)
            targetWeapon = CSW_M3;
        else if (containi(szMessage, "negev") != -1 || containi(szMessage, "m249") != -1)
            targetWeapon = CSW_M249;
        else if (containi(szMessage, "scout") != -1)
            targetWeapon = CSW_SCOUT;
        else
            targetWeapon = 0; // Default team rifle

        RequestWeaponDrop(id, targetWeapon, m4Variant);
        return PLUGIN_CONTINUE;
    }

    return PLUGIN_CONTINUE;
}

SetPlayerStrat(id, iStrat)
{
    new CsTeams:team = cs_get_user_team(id);
    if (team != CS_TEAM_T && team != CS_TEAM_CT)
        return;

    g_iTeamStrat[team] = iStrat;

    new szPlayerName[32];
    get_user_name(id, szPlayerName, charsmax(szPlayerName));

    BroadcastTeamCall(team, "^4[Team Strat]^3 %s^1: Calling Strat: ^4%s^1!", szPlayerName, g_szStratNames[iStrat]);
    client_cmd(id, "spk radio/roger");

    // Squad bot confirmation
    new iBotTeammate = 0;
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_connected(i) && is_user_bot(i) && cs_get_user_team(i) == team)
        {
            iBotTeammate = i;
            break;
        }
    }
    if (iBotTeammate > 0)
    {
        new szBotName[32];
        get_user_name(iBotTeammate, szBotName, charsmax(szBotName));
        BroadcastTeamCall(team, "^4[Squad Bot]^3 %s^1: Roger that! Moving into position for ^4%s^1.", szBotName, g_szStratNames[iStrat]);
    }

    // Immediately dispatch engine radio & follow orders
    DispatchIglEngineCommand(team);
}

stock Cmd_SetTeamEconomy(id, iEconMode, bool:bFullRefund = false)
{
    new CsTeams:team = cs_get_user_team(id);
    if (team != CS_TEAM_T && team != CS_TEAM_CT)
        return;

    g_iTeamEconomyCall[team] = iEconMode;

    new szCallerName[32];
    get_user_name(id, szCallerName, charsmax(szCallerName));

    switch (iEconMode)
    {
        case ECON_ECO:
        {
            if (bFullRefund)
                BroadcastTeamCall(team, "^4[Team Economy]^3 %s^1 called: ^4FULL ECO^1! Bots will sell all gear & save 100%% cash.", szCallerName);
            else
                BroadcastTeamCall(team, "^4[Team Economy]^3 %s^1 called: ^4ECO / SAVE^1! Bots will sell primary rifles & save cash.", szCallerName);
            PlayTeamAudio(team, "spk radio/regroup");
        }
        case ECON_FORCE:
        {
            BroadcastTeamCall(team, "^4[Team Economy]^3 %s^1 called: ^4FORCE BUY^1! Spending all available cash on weapons/armor!", szCallerName);
            PlayTeamAudio(team, "spk radio/stormfront");
        }
        case ECON_HALF:
        {
            BroadcastTeamCall(team, "^4[Team Economy]^3 %s^1 called: ^4HALF BUY^1! Preserving $2,000 savings buffer for next round.", szCallerName);
            PlayTeamAudio(team, "spk radio/sticktog");
        }
        case ECON_FULL:
        {
            BroadcastTeamCall(team, "^4[Team Economy]^3 %s^1 called: ^4FULL BUY^1! Buying rifles and full equipment.", szCallerName);
            PlayTeamAudio(team, "spk radio/go_go_go");
        }
    }

    if (iEconMode == ECON_ECO)
    {
        new iRefundCount = 0;
        new iTotalRefundMoney = 0;

        for (new b = 1; b <= 32; b++)
        {
            if (is_user_alive(b) && is_user_bot(b) && cs_get_user_team(b) == team)
            {
                new szBotName[32];
                get_user_name(b, szBotName, charsmax(szBotName));

                if (cs2_api_can_player_refund(b))
                {
                    new cost = 0;
                    new szWpn[32];
                    new bool:bRefunded = false;

                    if (bFullRefund)
                    {
                        new count = cs2_api_refund_player_all(b, cost);
                        if (count > 0)
                        {
                            bRefunded = true;
                            formatex(szWpn, charsmax(szWpn), "all equipment (%d items)", count);
                        }
                    }
                    else
                    {
                        // Opsi B: Primary Weapon Refund only (preserves vest & sidearm)
                        if (cs2_api_refund_player_primary(b, cost, szWpn, charsmax(szWpn)))
                        {
                            bRefunded = true;
                        }
                    }

                    if (bRefunded)
                    {
                        iRefundCount++;
                        iTotalRefundMoney += cost;
                        new newMoney = cs_get_user_money(b);
                        BroadcastTeamCall(team, "^4[Squad AI]^3 %s^1: Copy eco! Refunding ^3%s^1 (+^4$%d^1). Bank: ^4$%d^1.", szBotName, szWpn, cost, newMoney);
                    }
                    else
                    {
                        new curPrim = GetUserPrimaryWeapon(b);
                        if (curPrim > 0)
                        {
                            BroadcastTeamCall(team, "^4[Squad AI]^3 %s^1: Holding saved weapon from last round (Hero Rifle)!", szBotName);
                        }
                        else
                        {
                            BroadcastTeamCall(team, "^4[Squad AI]^3 %s^1: Understood, already on eco loadout.", szBotName);
                        }
                    }
                }
                else
                {
                    if (!g_bInFreezeTime)
                    {
                        BroadcastTeamCall(team, "^4[Squad AI]^3 %s^1: Negative, buy period has expired! Cannot refund now.", szBotName);
                    }
                }
            }
        }

        if (iRefundCount > 0)
        {
            PlayTeamAudio(team, "spk radio/roger");
        }
    }
    else
    {
        new iBotTeammate = 0;
        for (new i = 1; i <= 32; i++)
        {
            if (is_user_connected(i) && is_user_bot(i) && cs_get_user_team(i) == team)
            {
                iBotTeammate = i;
                break;
            }
        }
        if (iBotTeammate > 0)
        {
            new szBotName[32];
            get_user_name(iBotTeammate, szBotName, charsmax(szBotName));
            if (iEconMode == ECON_FORCE)
                BroadcastTeamCall(team, "^4[Squad Bot]^3 %s^1: Understood! Buying best weapons and armor now.", szBotName);
            else if (iEconMode == ECON_HALF)
                BroadcastTeamCall(team, "^4[Squad Bot]^3 %s^1: Confirmed! Keeping money buffer for next round.", szBotName);
            else
                BroadcastTeamCall(team, "^4[Squad Bot]^3 %s^1: Roger that, full buy ready!", szBotName);
        }
    }

    if (g_bInFreezeTime)
    {
        remove_task(TASK_SV_ENFORCE);
        set_task(0.2, "Task_EnforceRoleLoadouts", TASK_SV_ENFORCE);
    }
}

public Cmd_RadioNeedBackup(id)
{
    if (is_user_alive(id) && !is_user_bot(id))
    {
        new iPrimary = cs_get_user_weapon(id);
        if (!IsPrimaryRifle(iPrimary) && cs_get_user_money(id) < 2500)
        {
            RequestWeaponDrop(id, 0, 0);
        }
    }
    return PLUGIN_CONTINUE;
}

RequestWeaponDrop(requester, targetWeapon = 0, m4Variant = 0)
{
    if (!is_user_alive(requester) || !get_pcvar_num(g_pCvarEnable) || !get_pcvar_num(g_pCvarDropGuns))
        return;

    new CsTeams:teamRequester = cs_get_user_team(requester);
    if (teamRequester != CS_TEAM_T && teamRequester != CS_TEAM_CT)
        return;

    new szReqName[32];
    get_user_name(requester, szReqName, charsmax(szReqName));

    if (targetWeapon <= 0)
    {
        targetWeapon = (teamRequester == CS_TEAM_T) ? CSW_AK47 : CSW_M4A1;
    }

    // Adapt weapon to team allegiance (CTs buy M4/Famas/TMP, Ts buy AK/Galil/MAC-10)
    if (teamRequester == CS_TEAM_CT)
    {
        if (targetWeapon == CSW_AK47)
            targetWeapon = CSW_M4A1;
        else if (targetWeapon == CSW_GALIL)
            targetWeapon = CSW_FAMAS;
        else if (targetWeapon == CSW_MAC10)
            targetWeapon = CSW_TMP;
    }
    else if (teamRequester == CS_TEAM_T)
    {
        if (targetWeapon == CSW_M4A1)
            targetWeapon = CSW_AK47;
        else if (targetWeapon == CSW_FAMAS)
            targetWeapon = CSW_GALIL;
        else if (targetWeapon == CSW_TMP)
            targetWeapon = CSW_MAC10;
    }

    new iCost = 0;
    new szWpnName[32];
    new bool:isSecondary = false;

    switch (targetWeapon)
    {
        case CSW_DEAGLE:
        {
            iCost = 650;
            copy(szWpnName, charsmax(szWpnName), "Desert Eagle");
            isSecondary = true;
        }
        case CSW_P228:
        {
            iCost = 600;
            copy(szWpnName, charsmax(szWpnName), "Tec-9");
            isSecondary = true;
        }
        case CSW_AK47:
        {
            iCost = 2700;
            copy(szWpnName, charsmax(szWpnName), "AK-47");
        }
        case CSW_M4A1:
        {
            iCost = 3100;
            if (m4Variant == 1)
                copy(szWpnName, charsmax(szWpnName), "M4A4");
            else if (m4Variant == 2)
                copy(szWpnName, charsmax(szWpnName), "M4A1-S");
            else
                copy(szWpnName, charsmax(szWpnName), IsPlayerM4A4(requester) ? "M4A4" : "M4A1-S");
        }
        case CSW_AWP:
        {
            iCost = 4750;
            copy(szWpnName, charsmax(szWpnName), "AWP");
        }
        case CSW_GALIL:
        {
            iCost = 2000;
            copy(szWpnName, charsmax(szWpnName), "Galil");
        }
        case CSW_FAMAS:
        {
            iCost = 2250;
            copy(szWpnName, charsmax(szWpnName), "Famas");
        }
        case CSW_MP5NAVY:
        {
            iCost = 1500;
            copy(szWpnName, charsmax(szWpnName), "MP7");
        }
        case CSW_TMP:
        {
            iCost = 1250;
            copy(szWpnName, charsmax(szWpnName), "CZ75-Auto");
        }
        case CSW_MAC10:
        {
            iCost = 1400;
            copy(szWpnName, charsmax(szWpnName), "CZ75-Auto");
        }
        case CSW_P90:
        {
            iCost = 2350;
            copy(szWpnName, charsmax(szWpnName), "PP-Bizon");
        }
        case CSW_XM1014:
        {
            iCost = 3000;
            copy(szWpnName, charsmax(szWpnName), "MAG-7");
        }
        case CSW_M3:
        {
            iCost = 1700;
            copy(szWpnName, charsmax(szWpnName), "Sawed-Off");
        }
        case CSW_M249:
        {
            iCost = 5750;
            copy(szWpnName, charsmax(szWpnName), "Negev");
        }
        case CSW_SCOUT:
        {
            iCost = 2750;
            copy(szWpnName, charsmax(szWpnName), "Scout");
        }
        default:
        {
            targetWeapon = (teamRequester == CS_TEAM_T) ? CSW_AK47 : CSW_M4A1;
            iCost = (teamRequester == CS_TEAM_T) ? 2700 : 3100;
            if (teamRequester == CS_TEAM_T)
                copy(szWpnName, charsmax(szWpnName), "AK-47");
            else
                copy(szWpnName, charsmax(szWpnName), IsPlayerM4A4(requester) ? "M4A4" : "M4A1-S");
        }
    }

    // Slot Conflict Check
    if (isSecondary)
    {
        new iCurSec = GetUserSecondaryWeapon(requester);
        if (iCurSec == targetWeapon)
        {
            client_print_color(requester, print_team_default, "^4[Weapon Drop]^1 You already have a ^4%s^1, ^4%s^1!", szWpnName, szReqName);
            return;
        }
    }
    else
    {
        new iCurPrim = GetUserPrimaryWeapon(requester);
        if (iCurPrim == targetWeapon)
        {
            client_print_color(requester, print_team_default, "^4[Weapon Drop]^1 You already have a ^4%s^1, ^4%s^1!", szWpnName, szReqName);
            return;
        }
    }

    new Float:vecReqOrigin[3];
    pev(requester, pev_origin, vecReqOrigin);

    // Search for the wealthiest bot or Support bot
    new iBestBot = 0;
    new iMaxMoney = iCost + 800; // Minimum money threshold to drop with safety buffer

    for (new i = 1; i <= 32; i++)
    {
        if (i != requester && is_user_alive(i) && is_user_bot(i) && cs_get_user_team(i) == teamRequester)
        {
            new iMoney = cs_get_user_money(i);
            if (g_iPlayerRole[i] == ROLE_SUPPORT)
                iMoney += 1200;

            if (iMoney >= iMaxMoney)
            {
                new Float:vecBotOrigin[3];
                pev(i, pev_origin, vecBotOrigin);
                if (get_distance_f(vecBotOrigin, vecReqOrigin) <= 1200.0)
                {
                    iMaxMoney = iMoney;
                    iBestBot = i;
                }
            }
        }
    }

    if (iBestBot > 0)
    {
        new szBotName[32];
        get_user_name(iBestBot, szBotName, charsmax(szBotName));

        // Bot voice & chat confirmation (broadcast ONLY to teammates)
        client_cmd(requester, "spk radio/roger");
        BroadcastTeamCall(teamRequester, "^4[Weapon Drop]^3 %s %s^1: Dropping you a ^4%s^1, ^4%s^1! Catch!", 
            g_szRoleTags[g_iPlayerRole[iBestBot]], szBotName, szWpnName, szReqName);

        // Deduct money from bot
        new iCurrentMoney = cs_get_user_money(iBestBot);
        cs_set_user_money(iBestBot, max(0, iCurrentMoney - iCost));

        // Turn bot to face player
        new Float:vecBotEyes[3], Float:vecDiff[3], Float:vecAimAngles[3];
        pev(iBestBot, pev_origin, vecBotEyes);
        vecBotEyes[2] += 17.0;
        xs_vec_sub(vecReqOrigin, vecBotEyes, vecDiff);
        vector_to_angle(vecDiff, vecAimAngles);
        ClampAngles(vecAimAngles);

        set_pev(iBestBot, pev_v_angle, vecAimAngles);
        set_pev(iBestBot, pev_angles, vecAimAngles);

        // Spawn realistic dropped weapon toss
        SpawnDroppedWeaponToss(iBestBot, requester, targetWeapon, m4Variant);
    }
    else
    {
        client_print_color(requester, print_team_default, "^4[Weapon Drop]^1 No teammates have enough money ($%d) nearby to drop a %s!", iCost, szWpnName);
    }
}

SpawnDroppedWeaponToss(donor, recipient, weaponId, m4Variant = 0)
{
    new Float:vecBotEyes[3], Float:vecRecOrigin[3], Float:vecDiff[3], Float:vecAimAngles[3];
    pev(donor, pev_origin, vecBotEyes);
    vecBotEyes[2] += 17.0;
    pev(recipient, pev_origin, vecRecOrigin);

    xs_vec_sub(vecRecOrigin, vecBotEyes, vecDiff);
    vector_to_angle(vecDiff, vecAimAngles);
    ClampAngles(vecAimAngles);

    set_pev(donor, pev_v_angle, vecAimAngles);
    set_pev(donor, pev_angles, vecAimAngles);

    new ent = engfunc(EngFunc_CreateNamedEntity, engfunc(EngFunc_AllocString, "info_target"));
    if (pev_valid(ent))
    {
        set_pev(ent, pev_classname, "cs2_dropped_rifle");
        set_pev(ent, pev_origin, vecBotEyes);

        new Float:vecVel[3];
        xs_vec_normalize(vecDiff, vecVel);
        xs_vec_mul_scalar(vecVel, 320.0, vecVel);
        vecVel[2] = 160.0; // slight arc
        set_pev(ent, pev_velocity, vecVel);

        set_pev(ent, pev_movetype, MOVETYPE_TOSS);
        set_pev(ent, pev_solid, SOLID_TRIGGER);
        set_pev(ent, pev_gravity, 1.0);
        set_pev(ent, pev_friction, 0.85);
        set_pev(ent, pev_owner, donor);
        set_pev(ent, pev_iuser1, weaponId);

        new Float:mins[3] = {-16.0, -16.0, -10.0};
        new Float:maxs[3] = {16.0, 16.0, 16.0};
        engfunc(EngFunc_SetSize, ent, mins, maxs);

        if (weaponId == CSW_AWP)
            engfunc(EngFunc_SetModel, ent, "models/w_awp.mdl");
        else if (weaponId == CSW_AK47)
            engfunc(EngFunc_SetModel, ent, "models/w_ak47.mdl");
        else if (weaponId == CSW_DEAGLE)
            engfunc(EngFunc_SetModel, ent, "models/w_deagle.mdl");
        else if (weaponId == CSW_GALIL)
            engfunc(EngFunc_SetModel, ent, "models/w_galil.mdl");
        else if (weaponId == CSW_FAMAS)
            engfunc(EngFunc_SetModel, ent, "models/w_famas.mdl");
        else if (weaponId == CSW_MP5NAVY)
        {
            if (file_exists("models/pw_models/w_mp7.mdl") || file_exists("models/pw_models/w_mp7.mdl", true))
                engfunc(EngFunc_SetModel, ent, "models/pw_models/w_mp7.mdl");
            else
                engfunc(EngFunc_SetModel, ent, "models/w_mp5.mdl");
        }
        else if (weaponId == CSW_SCOUT)
            engfunc(EngFunc_SetModel, ent, "models/w_scout.mdl");
        else if (weaponId == CSW_P228)
        {
            if (file_exists("models/pw_models/w_tec9.mdl") || file_exists("models/pw_models/w_tec9.mdl", true))
                engfunc(EngFunc_SetModel, ent, "models/pw_models/w_tec9.mdl");
            else
                engfunc(EngFunc_SetModel, ent, "models/w_p228.mdl");
        }
        else if (weaponId == CSW_TMP || weaponId == CSW_MAC10)
        {
            if (file_exists("models/pw_models/w_cz75.mdl") || file_exists("models/pw_models/w_cz75.mdl", true))
                engfunc(EngFunc_SetModel, ent, "models/pw_models/w_cz75.mdl");
            else
                engfunc(EngFunc_SetModel, ent, (weaponId == CSW_TMP) ? "models/w_tmp.mdl" : "models/w_mac10.mdl");
        }
        else if (weaponId == CSW_P90)
        {
            if (file_exists("models/pw_models/w_bizon.mdl") || file_exists("models/pw_models/w_bizon.mdl", true))
                engfunc(EngFunc_SetModel, ent, "models/pw_models/w_bizon.mdl");
            else
                engfunc(EngFunc_SetModel, ent, "models/w_p90.mdl");
        }
        else if (weaponId == CSW_XM1014)
        {
            if (file_exists("models/pw_models/w_mag7.mdl") || file_exists("models/pw_models/w_mag7.mdl", true))
                engfunc(EngFunc_SetModel, ent, "models/pw_models/w_mag7.mdl");
            else
                engfunc(EngFunc_SetModel, ent, "models/w_xm1014.mdl");
        }
        else if (weaponId == CSW_M3)
        {
            if (file_exists("models/pw_models/w_sawedoff.mdl") || file_exists("models/pw_models/w_sawedoff.mdl", true))
                engfunc(EngFunc_SetModel, ent, "models/pw_models/w_sawedoff.mdl");
            else
                engfunc(EngFunc_SetModel, ent, "models/w_m3.mdl");
        }
        else if (weaponId == CSW_M249)
        {
            if (file_exists("models/pw_models/w_negev.mdl") || file_exists("models/pw_models/w_negev.mdl", true))
                engfunc(EngFunc_SetModel, ent, "models/pw_models/w_negev.mdl");
            else
                engfunc(EngFunc_SetModel, ent, "models/w_m249.mdl");
        }
        else if (weaponId == CSW_M4A1)
        {
            new bool:bM4A4 = false;
            if (m4Variant == 1)
                bM4A4 = true;
            else if (m4Variant == 2)
                bM4A4 = false;
            else
                bM4A4 = (recipient >= 1 && recipient <= 32 && is_user_connected(recipient)) ? IsPlayerM4A4(recipient) : IsPlayerM4A4(donor);

            if (bM4A4)
            {
                if (file_exists("models/pw_models/w_m4a4.mdl") || file_exists("models/pw_models/w_m4a4.mdl", true))
                    engfunc(EngFunc_SetModel, ent, "models/pw_models/w_m4a4.mdl");
                else if (file_exists("models/csgo_skins/ct/w_m4a4.mdl") || file_exists("models/csgo_skins/ct/w_m4a4.mdl", true))
                    engfunc(EngFunc_SetModel, ent, "models/csgo_skins/ct/w_m4a4.mdl");
                else
                    engfunc(EngFunc_SetModel, ent, "models/w_m4a1.mdl");
                set_pev(ent, pev_iuser2, 1);
            }
            else
            {
                engfunc(EngFunc_SetModel, ent, "models/w_m4a1.mdl");
                set_pev(ent, pev_iuser2, 0);
            }
        }
        else
            engfunc(EngFunc_SetModel, ent, "models/w_m4a1.mdl");

        // Despawn after 60 seconds if untouched
        set_pev(ent, pev_nextthink, get_gametime() + 60.0);
    }
}

public OnDroppedRifleTouch(ent, id)
{
    if (!pev_valid(ent))
        return HAM_IGNORED;

    new szClass[32];
    pev(ent, pev_classname, szClass, charsmax(szClass));
    if (!equal(szClass, "cs2_dropped_rifle"))
        return HAM_IGNORED;

    if (!is_user_alive(id))
        return HAM_IGNORED;

    new weaponId = pev(ent, pev_iuser1);
    new iGroundTier = GetRoleWeaponTier(id, weaponId, g_bRoundEnded);

    new bool:isSecondary = IsSecondaryWeapon(weaponId);
    if (isSecondary)
    {
        new iCurSec = GetUserSecondaryWeapon(id);
        if (iCurSec > 0 && iCurSec != weaponId)
        {
            DropUserSecondary(id);
        }
    }
    else
    {
        new iCurPrim = GetUserPrimaryWeapon(id);
        new iCurTier = GetRoleWeaponTier(id, iCurPrim, g_bRoundEnded);
        if (iCurPrim > 0)
        {
            if (iGroundTier > iCurTier || !is_user_bot(id))
            {
                DropUserPrimary(id);
                g_flLastWeaponSwapTime[id] = get_gametime();
            }
            else
            {
                return HAM_IGNORED;
            }
        }
    }

    if (weaponId == CSW_AK47)
    {
        give_item(id, "weapon_ak47");
        cs_set_user_bpammo(id, CSW_AK47, 90);
    }
    else if (weaponId == CSW_M4A1)
    {
        new iWpn = give_item(id, "weapon_m4a1");
        cs_set_user_bpammo(id, CSW_M4A1, 90);
        if (pev_valid(iWpn))
        {
            new iIsM4A4 = pev(ent, pev_iuser2);
            set_pev(iWpn, pev_iuser1, (iIsM4A4 == 1) ? 1 : 2);
            if (iIsM4A4 == 1 && cs_get_weapon_silen(iWpn))
                cs_set_weapon_silen(iWpn, 0, 0);
            else if (iIsM4A4 == 0 && !cs_get_weapon_silen(iWpn))
                cs_set_weapon_silen(iWpn, 1, 0);

            if (is_user_alive(id) && cs_get_user_weapon(id) == CSW_M4A1)
            {
                ExecuteHamB(Ham_Item_Deploy, iWpn);
            }
        }
    }
    else if (weaponId == CSW_AWP)
    {
        give_item(id, "weapon_awp");
        cs_set_user_bpammo(id, CSW_AWP, 30);
    }
    else if (weaponId == CSW_DEAGLE)
    {
        give_item(id, "weapon_deagle");
        cs_set_user_bpammo(id, CSW_DEAGLE, 35);
    }
    else if (weaponId == CSW_GALIL)
    {
        give_item(id, "weapon_galil");
        cs_set_user_bpammo(id, CSW_GALIL, 90);
    }
    else if (weaponId == CSW_FAMAS)
    {
        give_item(id, "weapon_famas");
        cs_set_user_bpammo(id, CSW_FAMAS, 90);
    }
    else if (weaponId == CSW_MP5NAVY)
    {
        give_item(id, "weapon_mp5navy");
        cs_set_user_bpammo(id, CSW_MP5NAVY, 120);
    }
    else if (weaponId == CSW_SCOUT)
    {
        give_item(id, "weapon_scout");
        cs_set_user_bpammo(id, CSW_SCOUT, 90);
    }
    else if (weaponId == CSW_P228)
    {
        give_item(id, "weapon_p228");
        cs_set_user_bpammo(id, CSW_P228, 52);
    }
    else if (weaponId == CSW_TMP)
    {
        give_item(id, "weapon_tmp");
        cs_set_user_bpammo(id, CSW_TMP, 90);
    }
    else if (weaponId == CSW_MAC10)
    {
        give_item(id, "weapon_mac10");
        cs_set_user_bpammo(id, CSW_MAC10, 100);
    }
    else if (weaponId == CSW_P90)
    {
        give_item(id, "weapon_p90");
        cs_set_user_bpammo(id, CSW_P90, 100);
    }
    else if (weaponId == CSW_XM1014)
    {
        give_item(id, "weapon_xm1014");
        cs_set_user_bpammo(id, CSW_XM1014, 32);
    }
    else if (weaponId == CSW_M3)
    {
        give_item(id, "weapon_m3");
        cs_set_user_bpammo(id, CSW_M3, 32);
    }
    else if (weaponId == CSW_M249)
    {
        give_item(id, "weapon_m249");
        cs_set_user_bpammo(id, CSW_M249, 200);
    }

    emit_sound(id, CHAN_ITEM, "items/gunpickup2.wav", 0.9, ATTN_NORM, 0, PITCH_NORM);

    new szGunName[16];
    if (weaponId == CSW_M4A1)
    {
        copy(szGunName, charsmax(szGunName), (pev(ent, pev_iuser2) == 1) ? "M4A4" : "M4A1-S");
    }
    else
    {
        GetWeaponCleanName(weaponId, szGunName, charsmax(szGunName));
    }
    client_print(id, print_center, "Picked up %s", szGunName);

    if (g_bRoundEnded && is_user_bot(id))
    {
        g_iSavedWeapon[id] = weaponId;
        g_bSavedWeaponForRole[id] = true;
    }

    engfunc(EngFunc_RemoveEntity, ent);
    return HAM_HANDLED;
}

public OnDroppedRifleThink(ent)
{
    if (!pev_valid(ent))
        return HAM_IGNORED;

    new szClass[32];
    pev(ent, pev_classname, szClass, charsmax(szClass));
    if (equal(szClass, "cs2_dropped_rifle"))
    {
        engfunc(EngFunc_RemoveEntity, ent);
        return HAM_HANDLED;
    }
    return HAM_IGNORED;
}

public OnWeaponBoxTouchPre(ent, id)
{
    if (!pev_valid(ent) || !is_user_alive(id) || !is_user_bot(id))
        return HAM_IGNORED;

    if (g_bInFreezeTime || !get_pcvar_num(g_pCvarWeaponScavenge) || IsWarmupActive() || IsKnifeRoundActive())
        return HAM_IGNORED;

    new Float:flGameTime = get_gametime();
    // Anti-thrashing cooldown: prevent bot from fluttering/swapping repeatedly within 4 seconds!
    if (flGameTime - g_flLastWeaponSwapTime[id] < 4.0)
        return HAM_IGNORED;

    new iGroundWpn = GetGroundWeaponType(ent);
    if (!iGroundWpn || !IsPrimaryWeapon(iGroundWpn))
        return HAM_IGNORED;

    new iGroundTier = GetRoleWeaponTier(id, iGroundWpn, g_bRoundEnded);
    new iCurPrim = GetUserPrimaryWeapon(id);
    new iCurTier = GetRoleWeaponTier(id, iCurPrim, g_bRoundEnded);

    // If ground weapon is strictly superior to currently held weapon according to role:
    if (iGroundTier > iCurTier)
    {
        if (iCurPrim > 0)
        {
            DropUserPrimary(id);
        }
        g_flLastWeaponSwapTime[id] = flGameTime;

        new szBotName[32], szGroundWpnName[32], szOldWpnName[32];
        get_user_name(id, szBotName, charsmax(szBotName));
        GetWeaponCleanName(iGroundWpn, szGroundWpnName, charsmax(szGroundWpnName));
        GetWeaponCleanName(iCurPrim, szOldWpnName, charsmax(szOldWpnName));

        new CsTeams:team = cs_get_user_team(id);
        if (iCurPrim > 0)
        {
            BroadcastTeamCall(team, "^4[Weapon Upgrade]^3 %s %s^1 upgraded: ^3%s^1 -> ^4%s^1!", 
                g_szRoleTags[g_iPlayerRole[id]], szBotName, szOldWpnName, szGroundWpnName);
        }
        else
        {
            BroadcastTeamCall(team, "^4[Weapon Pickup]^3 %s %s^1 picked up ^4%s^1!", 
                g_szRoleTags[g_iPlayerRole[id]], szBotName, szGroundWpnName);
        }

        if (g_bRoundEnded)
        {
            g_iSavedWeapon[id] = iGroundWpn;
            g_bSavedWeaponForRole[id] = true;
        }

        remove_task(TASK_SV_REFILL_AMMO + id);
        set_task(0.05, "Task_RefillUserAmmoDelayed", TASK_SV_REFILL_AMMO + id);
    }

    return HAM_IGNORED;
}

public Task_RefillUserAmmoDelayed(taskid)
{
    new id = taskid - TASK_SV_REFILL_AMMO;
    if (is_user_alive(id))
    {
        new iPrim = GetUserPrimaryWeapon(id);
        if (iPrim > 0)
        {
            if (iPrim == CSW_AWP)
                cs_set_user_bpammo(id, CSW_AWP, 30);
            else
                cs_set_user_bpammo(id, iPrim, 90);
        }
    }
}

public Task_DeliverSavedWeapons()
{
    if (IsKnifeRoundActive() || IsWarmupActive() || !get_pcvar_num(g_pCvarRoles) || !get_pcvar_num(g_pCvarRoleDeliver))
        return;

    DeliverTeamSavedWeapons(CS_TEAM_T);
    DeliverTeamSavedWeapons(CS_TEAM_CT);
}

DeliverTeamSavedWeapons(CsTeams:team)
{
    // -------------------------------------------------------------
    // 1. AWP DELIVERY: Deliver saved AWP to team AWPer if they need it!
    // -------------------------------------------------------------
    new iAwper = 0;
    for (new i = 1; i <= 32; i++)
    {
        if (is_user_alive(i) && cs_get_user_team(i) == team && g_iPlayerRole[i] == ROLE_AWPER)
        {
            iAwper = i;
            break;
        }
    }

    if (iAwper > 0 && GetUserPrimaryWeapon(iAwper) != CSW_AWP)
    {
        for (new donor = 1; donor <= 32; donor++)
        {
            if (donor != iAwper && is_user_alive(donor) && is_user_bot(donor) && cs_get_user_team(donor) == team)
            {
                if (GetUserPrimaryWeapon(donor) == CSW_AWP)
                {
                    new szDonorName[32], szAwperName[32];
                    get_user_name(donor, szDonorName, charsmax(szDonorName));
                    get_user_name(iAwper, szAwperName, charsmax(szAwperName));

                    if (!is_user_bot(iAwper))
                    {
                        SpawnDroppedWeaponToss(donor, iAwper, CSW_AWP);
                        StripUserPrimary(donor);
                        PlayTeamAudio(team, "spk radio/roger");
                        BroadcastTeamCall(team, "^4[Role Delivery]^3 %s %s^1: Saved an ^4AWP^1 last round! Dropped for Sniper ^4%s^1!", 
                            g_szRoleTags[g_iPlayerRole[donor]], szDonorName, szAwperName);
                    }
                    else
                    {
                        if (cs_get_user_hasprim(iAwper))
                        {
                            StripUserPrimary(iAwper);
                        }
                        StripUserPrimary(donor);
                        give_item(iAwper, "weapon_awp");
                        cs_set_user_bpammo(iAwper, CSW_AWP, 30);
                        give_item(iAwper, "weapon_deagle");
                        cs_set_user_bpammo(iAwper, CSW_DEAGLE, 35);
                        give_item(iAwper, "item_assaultsuit");

                        BroadcastTeamCall(team, "^4[Role Delivery]^3 %s %s^1 passed saved ^4AWP^1 to Sniper %s %s^1!", 
                            g_szRoleTags[g_iPlayerRole[donor]], szDonorName, g_szRoleTags[ROLE_AWPER], szAwperName);
                    }

                    g_iSavedWeapon[donor] = 0;
                    g_bSavedWeaponForRole[donor] = false;
                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // 2. RIFLE DELIVERY (AK-47 / M4A1): Drop saved rifle to broke or Entry
    // -------------------------------------------------------------
    for (new donor = 1; donor <= 32; donor++)
    {
        if (is_user_alive(donor) && is_user_bot(donor) && cs_get_user_team(donor) == team)
        {
            new iDonorPrim = GetUserPrimaryWeapon(donor);
            new bool:bHasRifle = (iDonorPrim == CSW_AK47 || iDonorPrim == CSW_M4A1);

            if ((g_bSavedWeaponForRole[donor] && bHasRifle) || (g_iPlayerRole[donor] == ROLE_SUPPORT && cs_get_user_money(donor) >= 5500 && bHasRifle))
            {
                new iRecipient = 0;

                // Priority A: Human player with no primary
                for (new p = 1; p <= 32; p++)
                {
                    if (is_user_alive(p) && !is_user_bot(p) && cs_get_user_team(p) == team)
                    {
                        if (!cs_get_user_hasprim(p))
                        {
                            iRecipient = p;
                            break;
                        }
                    }
                }

                // Priority B: Entry Fragger with no primary or broke
                if (!iRecipient)
                {
                    for (new p = 1; p <= 32; p++)
                    {
                        if (p != donor && is_user_alive(p) && cs_get_user_team(p) == team && g_iPlayerRole[p] == ROLE_ENTRY)
                        {
                            if (!cs_get_user_hasprim(p) && cs_get_user_money(p) < 2500)
                            {
                                iRecipient = p;
                                break;
                            }
                        }
                    }
                }

                // Priority C: Any broke teammate
                if (!iRecipient)
                {
                    for (new p = 1; p <= 32; p++)
                    {
                        if (p != donor && is_user_alive(p) && cs_get_user_team(p) == team)
                        {
                            if (!cs_get_user_hasprim(p) && cs_get_user_money(p) < 2500)
                            {
                                iRecipient = p;
                                break;
                            }
                        }
                    }
                }

                if (iRecipient > 0)
                {
                    new szDonorName[32], szRecipName[32];
                    get_user_name(donor, szDonorName, charsmax(szDonorName));
                    get_user_name(iRecipient, szRecipName, charsmax(szRecipName));

                    new wpnId = (team == CS_TEAM_T) ? CSW_AK47 : CSW_M4A1;
                    new bool:bDonorM4A4 = (wpnId == CSW_M4A1) ? IsPlayerM4A4(donor) : false;

                    if (!is_user_bot(iRecipient))
                    {
                        SpawnDroppedWeaponToss(donor, iRecipient, wpnId, bDonorM4A4 ? 1 : 2);
                        StripUserPrimary(donor);
                        PlayTeamAudio(team, "spk radio/roger");
                        BroadcastTeamCall(team, "^4[Role Delivery]^3 %s %s^1 dropped saved ^4%s^1 for ^4%s^1!", 
                            g_szRoleTags[g_iPlayerRole[donor]], szDonorName, (wpnId == CSW_AK47) ? "AK-47" : (bDonorM4A4 ? "M4A4" : "M4A1-S"), szRecipName);
                    }
                    else
                    {
                        if (cs_get_user_hasprim(iRecipient))
                        {
                            StripUserPrimary(iRecipient);
                        }
                        StripUserPrimary(donor);
                        if (team == CS_TEAM_T)
                        {
                            give_item(iRecipient, "weapon_ak47");
                            cs_set_user_bpammo(iRecipient, CSW_AK47, 90);
                        }
                        else
                        {
                            new iWpn = give_item(iRecipient, "weapon_m4a1");
                            cs_set_user_bpammo(iRecipient, CSW_M4A1, 90);
                            if (pev_valid(iWpn))
                            {
                                set_pev(iWpn, pev_iuser1, bDonorM4A4 ? 1 : 2);
                                if (bDonorM4A4 && cs_get_weapon_silen(iWpn))
                                    cs_set_weapon_silen(iWpn, 0, 0);
                                else if (!bDonorM4A4 && !cs_get_weapon_silen(iWpn))
                                    cs_set_weapon_silen(iWpn, 1, 0);

                                if (is_user_alive(iRecipient) && cs_get_user_weapon(iRecipient) == CSW_M4A1)
                                {
                                    ExecuteHamB(Ham_Item_Deploy, iWpn);
                                }
                            }
                        }
                        give_item(iRecipient, "item_assaultsuit");

                        BroadcastTeamCall(team, "^4[Role Delivery]^3 %s %s^1 passed saved ^4%s^1 to %s %s^1!", 
                            g_szRoleTags[g_iPlayerRole[donor]], szDonorName, (wpnId == CSW_AK47) ? "AK-47" : (bDonorM4A4 ? "M4A4" : "M4A1-S"), 
                            g_szRoleTags[g_iPlayerRole[iRecipient]], szRecipName);
                    }

                    g_iSavedWeapon[donor] = 0;
                    g_bSavedWeaponForRole[donor] = false;
                }
            }
        }
    }
}

public Event_RoundEnd()
{
    g_bRoundEnded = true;
    g_flRoundEndTime = get_gametime();
}

public Event_RoundEndAudio()
{
    g_bRoundEnded = true;
    g_flRoundEndTime = get_gametime();
}

public Task_ScavengeGroundWeapons()
{
    if (!get_pcvar_num(g_pCvarEnable) || !get_pcvar_num(g_pCvarMicroOverride) || !get_pcvar_num(g_pCvarWeaponScavenge) || g_bInFreezeTime || IsWarmupActive() || IsKnifeRoundActive())
        return;

    new Float:flGameTime = get_gametime();

    for (new id = 1; id <= 32; id++)
    {
        if (!is_user_alive(id) || !is_user_bot(id))
            continue;

        // Skip if currently engaged in close dueling (unless round ended)
        if (!g_bRoundEnded && (flGameTime - g_flLastCombatTime[id] < 2.0))
            continue;

        // Only scavenge for 7 seconds after round end
        if (g_bRoundEnded && (flGameTime - g_flRoundEndTime > 7.0))
            continue;

        // Skip scavenging if bot recently swapped weapons (< 4.0s)
        if (flGameTime - g_flLastWeaponSwapTime[id] < 4.0)
            continue;

        new iCurPrim = GetUserPrimaryWeapon(id);
        new iCurTier = GetRoleWeaponTier(id, iCurPrim, g_bRoundEnded);

        // If bot already has their role's top weapon (Tier 4), skip scavenging unless round ended (saving for teammate)
        if (iCurTier >= 4 && !g_bRoundEnded)
            continue;

        new iBestEnt = 0;
        new iBestTier = iCurTier;
        new Float:flBestDist = g_bRoundEnded ? 2500.0 : 650.0;
        new Float:vecBotOrigin[3];
        pev(id, pev_origin, vecBotOrigin);

        // 1. Scan dropped weaponbox entities
        new ent = -1;
        while ((ent = find_ent_by_class(ent, "weaponbox")))
        {
            if (!pev_valid(ent))
                continue;

            new iWpn = GetGroundWeaponType(ent);
            new iTier = GetRoleWeaponTier(id, iWpn, g_bRoundEnded);

            if (iTier > iBestTier)
            {
                new Float:vecWpnOrigin[3];
                pev(ent, pev_origin, vecWpnOrigin);
                new Float:flDist = get_distance_f(vecBotOrigin, vecWpnOrigin);

                if (flDist < flBestDist)
                {
                    flBestDist = flDist;
                    iBestEnt = ent;
                    iBestTier = iTier;
                }
            }
        }

        // 2. Scan custom cs2_dropped_rifle entities
        ent = -1;
        while ((ent = find_ent_by_class(ent, "cs2_dropped_rifle")))
        {
            if (!pev_valid(ent))
                continue;

            new iWpn = pev(ent, pev_iuser1);
            new iTier = GetRoleWeaponTier(id, iWpn, g_bRoundEnded);

            if (iTier > iBestTier)
            {
                new Float:vecWpnOrigin[3];
                pev(ent, pev_origin, vecWpnOrigin);
                new Float:flDist = get_distance_f(vecBotOrigin, vecWpnOrigin);

                if (flDist < flBestDist)
                {
                    flBestDist = flDist;
                    iBestEnt = ent;
                    iBestTier = iTier;
                }
            }
        }

        if (iBestEnt > 0)
        {
            g_iScavengeTarget[id] = iBestEnt;
            g_flScavengeExpire[id] = flGameTime + (g_bRoundEnded ? 6.0 : 3.5);
        }
    }
}

public Task_EconomyWeaponDrop()
{
    if (!get_pcvar_num(g_pCvarEnable) || !get_pcvar_num(g_pCvarDropGuns) || IsWarmupActive() || IsKnifeRoundActive())
        return;

    RunTeamEconomyDrop(CS_TEAM_T);
    RunTeamEconomyDrop(CS_TEAM_CT);
}

stock GetMatchRoundNumber()
{
    new pCvar = get_cvar_pointer("amx_csgo_current_round");
    if (pCvar)
    {
        new iCvarRound = get_pcvar_num(pCvar);
        if (iCvarRound > 0)
            return iCvarRound;
    }
    return g_iRoundCount;
}

stock RunTeamEconomyDrop(CsTeams:team)
{
    if (IsKnifeRoundActive())
        return;

    new iRoundNum = GetMatchRoundNumber();
    new iRoundInHalf = ((iRoundNum - 1) % 12) + 1; // Round within MR12 half
    // Pistol rounds: Round 1 of Half 1 or Round 1 of Half 2 (nobody drops rifles)
    if (iRoundNum <= 1 || iRoundInHalf <= 1)
        return;

    // Scan for broke bots on this team who need a weapon
    for (new poor = 1; poor <= 32; poor++)
    {
        if (is_user_alive(poor) && is_user_bot(poor) && cs_get_user_team(poor) == team)
        {
            if (cs_get_user_hasprim(poor) || GetUserPrimaryWeapon(poor) > 0)
                continue;

            new iPoorMoney = cs_get_user_money(poor);
            if (iPoorMoney >= 2200)
                continue; // Can afford budget rifle/SMG

            new wpnId = (team == CS_TEAM_T) ? CSW_AK47 : CSW_M4A1;
            new wpnCost = (team == CS_TEAM_T) ? 2700 : 3100;
            new szWpnTag[16];
            copy(szWpnTag, charsmax(szWpnTag), (team == CS_TEAM_T) ? "AK-47" : (IsPlayerM4A4(poor) ? "M4A4" : "M4A1-S"));

            // AWPer role asks for AWP ONLY if round in half >= 3 AND someone on team actually has AWP money ($5750+)
            if (g_iPlayerRole[poor] == ROLE_AWPER && iRoundInHalf >= 3)
            {
                new bool:bCanAffordAwp = false;
                for (new i = 1; i <= 32; i++)
                {
                    if (i != poor && is_user_alive(i) && cs_get_user_team(i) == team)
                    {
                        new iM = cs_get_user_money(i);
                        if (is_user_bot(i) && iM >= 5750)
                        {
                            bCanAffordAwp = true;
                            break;
                        }
                        else if (!is_user_bot(i) && (cs_get_user_hasprim(i) || GetUserPrimaryWeapon(i) > 0) && iM >= 6250)
                        {
                            bCanAffordAwp = true;
                            break;
                        }
                    }
                }

                if (bCanAffordAwp)
                {
                    wpnId = CSW_AWP;
                    wpnCost = 4750;
                    copy(szWpnTag, charsmax(szWpnTag), "AWP");
                }
            }

            // Step A: Search for wealthy bot teammate
            new iBestRichBot = 0;
            new iMaxMoney = wpnCost + 1000;

            for (new rich = 1; rich <= 32; rich++)
            {
                if (rich != poor && is_user_alive(rich) && is_user_bot(rich) && cs_get_user_team(rich) == team)
                {
                    new iMoney = cs_get_user_money(rich);
                    if (g_iPlayerRole[rich] == ROLE_SUPPORT)
                        iMoney += 800;

                    if (iMoney >= iMaxMoney)
                    {
                        iMaxMoney = iMoney;
                        iBestRichBot = rich;
                    }
                }
            }

            if (iBestRichBot > 0)
            {
                new szPoorName[32], szRichName[32];
                get_user_name(poor, szPoorName, charsmax(szPoorName));
                get_user_name(iBestRichBot, szRichName, charsmax(szRichName));

                // Broadcast drop ONLY to this specific team!
                BroadcastTeamCall(team, "^4[Weapon Drop]^3 %s %s^1: Got you, ^4%s^1! Dropping ^4%s^1! Catch!",
                    g_szRoleTags[g_iPlayerRole[iBestRichBot]], szRichName, szPoorName, szWpnTag);

                new iCurMoney = cs_get_user_money(iBestRichBot);
                cs_set_user_money(iBestRichBot, max(0, iCurMoney - wpnCost));

                SpawnDroppedWeaponToss(iBestRichBot, poor, wpnId, IsPlayerM4A4(poor) ? 1 : 2);
                return; // Limit to 1 drop per freeze time to preserve economy
            }
            else
            {
                // Step B: No bot can afford it. Check if HUMAN player is wealthy on the SAME team!
                new iRichHuman = 0;
                for (new h = 1; h <= 32; h++)
                {
                    if (is_user_alive(h) && !is_user_bot(h) && cs_get_user_team(h) == team)
                    {
                        if ((cs_get_user_hasprim(h) || GetUserPrimaryWeapon(h) > 0) && cs_get_user_money(h) >= (wpnCost + 1500))
                        {
                            iRichHuman = h;
                            break;
                        }
                    }
                }

                if (iRichHuman > 0)
                {
                    new szPoorName[32], szHumanName[32];
                    get_user_name(poor, szPoorName, charsmax(szPoorName));
                    get_user_name(iRichHuman, szHumanName, charsmax(szHumanName));

                    // Announce politely to the human player on the SAME team only
                    BroadcastTeamCall(team, "^4[Drop Request]^3 %s %s^1: Hey ^4%s^1, you're rich ($%d)! Can you drop me an ^4%s^1 please?",
                        g_szRoleTags[g_iPlayerRole[poor]], szPoorName, szHumanName, cs_get_user_money(iRichHuman), szWpnTag);
                    client_cmd(iRichHuman, "spk radio/needbackup");
                    return; // Limit to 1 request per freeze time
                }
            }
        }
    }
}
