#if defined _mod_m4_split_included
    #endinput
#endif
#define _mod_m4_split_included

// ==============================================================================
// CT BOT M4 LOADOUT MANAGEMENT & ROLE INTEGRATION
// ==============================================================================

stock bool:CanAccessBotM4Menu(id)
{
    if (!is_dedicated_server())
        return true;
    if (is_user_admin(id) || access(id, ADMIN_CFG) || access(id, ADMIN_BAN))
        return true;
    return false;
}

stock GetBotRole(id)
{
    return _:cs2_api_get_player_role(id);
}

stock GetCatalogM4Index(bool:bM4A4)
{
    new slot = GET_SLOT(TEAM_CT_INDEX, CSW_M4A1);
    for (new s = 0; s < g_SkinCount[slot]; s++)
    {
        if (bM4A4 && g_SkinIsM4A4[slot][s])
            return s;
        if (!bM4A4 && !g_SkinIsM4A4[slot][s])
            return s;
    }
    return 0;
}

stock AssignBotModernLoadouts(id)
{
    if (!is_user_connected(id) || !is_user_bot(id) || !is_user_alive(id))
        return;

    new CsTeams:team = safe_cs_get_user_team(id);
    if (team != CS_TEAM_CT && team != CS_TEAM_T)
        return;

    new team_index = (team == CS_TEAM_CT) ? TEAM_CT_INDEX : TEAM_T_INDEX;
    for (new w = 1; w <= CSW_LAST_WEAPON; w++)
    {
        if (w == CSW_M4A1)
            continue; // Handled by AssignBotM4Loadout

        new slot = GET_SLOT(team_index, w);
        if (g_SkinCount[slot] > 0)
        {
            new active_idx = g_ActiveLoadoutSkin[slot];
            if (active_idx >= 0 && active_idx < g_SkinCount[slot])
            {
                g_iPlayerSkin[id][slot] = active_idx;
                if (is_user_alive(id) && g_WeaponEntityNames[w][0])
                {
                    new iWpn = fm_find_weapon_ent_by_owner(id, g_WeaponEntityNames[w]);
                    if (pev_valid(iWpn))
                    {
                        set_pev(iWpn, pev_iuser2, active_idx + 1);
                    }
                }
            }
        }
    }
}

stock AssignBotM4Loadout(id)
{
    if (!is_user_connected(id) || !is_user_bot(id) || !is_user_alive(id))
        return;

    new CsTeams:team = safe_cs_get_user_team(id);
    if (team != CS_TEAM_CT)
        return;

    new slot = GET_SLOT(TEAM_CT_INDEX, CSW_M4A1);
    if (g_SkinCount[slot] <= 0)
        return;

    new szBotName[32];
    get_user_name(id, szBotName, charsmax(szBotName));

    new bool:chooseM4A4 = false;

    // 0. If bot already has an active weapon_m4a1 with an explicit variant (e.g. DM auto-equip), respect it
    new iCurWpn = fm_find_weapon_ent_by_owner(id, "weapon_m4a1");
    if (pev_valid(iCurWpn) && g_iBotM4Override[id] == -1)
    {
        new iCurVar = pev(iCurWpn, pev_iuser1);
        if (iCurVar == 1)
        {
            chooseM4A4 = true;
        }
        else if (iCurVar == 2)
        {
            chooseM4A4 = false;
        }
    }
    // 1. Check individual override first
    else if (g_iBotM4Override[id] == 0) // Force M4A4
    {
        chooseM4A4 = true;
    }
    else if (g_iBotM4Override[id] == 1) // Force M4A1-S
    {
        chooseM4A4 = false;
    }
    else
    {
        new mode = get_pcvar_num(pCvar_BotM4Mode);
        switch (mode)
        {
            case 0: // Mirror Active Loadout
            {
                new active_idx = g_ActiveLoadoutSkin[slot];
                if (active_idx >= 0 && active_idx < g_SkinCount[slot])
                    chooseM4A4 = g_SkinIsM4A4[slot][active_idx];
                else
                    chooseM4A4 = false;
            }
            case 1: // Smart Role-Based (Default)
            {
                new role = GetBotRole(id);
                // ROLE_ENTRY (1), ROLE_LURKER (4) -> M4A4
                // ROLE_SUPPORT (3), ROLE_IGL (5), ROLE_AWPER (2) -> M4A1-S
                if (role == 1 || role == 4)
                {
                    chooseM4A4 = true;
                }
                else if (role == 3 || role == 5 || role == 2)
                {
                    chooseM4A4 = false;
                }
                else
                {
                    // Fallback if role is not yet determined: check session preference or alternate by id
                    new savedPref = 0;
                    if (g_tBotM4Preference != Invalid_Trie && TrieGetCell(g_tBotM4Preference, szBotName, savedPref))
                    {
                        chooseM4A4 = (savedPref == 1);
                    }
                    else
                    {
                        chooseM4A4 = (id % 2 == 1);
                    }
                }
            }
            case 2: // Mixed 50/50
            {
                new ct_bot_index = 0;
                for (new i = 1; i <= MaxClients; i++)
                {
                    if (is_user_connected(i) && is_user_bot(i) && is_user_alive(i) && safe_cs_get_user_team(i) == CS_TEAM_CT)
                    {
                        ct_bot_index++;
                        if (i == id)
                            break;
                    }
                }
                chooseM4A4 = (ct_bot_index % 2 == 1);
            }
            case 3: // Force All M4A4
            {
                chooseM4A4 = true;
            }
            case 4: // Force All M4A1-S
            {
                chooseM4A4 = false;
            }
            default:
            {
                chooseM4A4 = true;
            }
        }
    }

    g_iPlayerSkin[id][slot] = GetCatalogM4Index(chooseM4A4);
    if (g_tBotM4Preference != Invalid_Trie)
    {
        TrieSetCell(g_tBotM4Preference, szBotName, chooseM4A4 ? 1 : 2);
    }

    new iWpn = fm_find_weapon_ent_by_owner(id, "weapon_m4a1");
    if (pev_valid(iWpn))
    {
        new target_variant = chooseM4A4 ? 1 : 2;
        set_pev(iWpn, pev_iuser1, target_variant);
        if (target_variant == 1 && cs_get_weapon_silen(iWpn))
        {
            cs_set_weapon_silen(iWpn, 0, 0);
        }
        else if (target_variant == 2 && get_pcvar_num(pCvar_AutoSilenceM4A1) && !cs_get_weapon_silen(iWpn))
        {
            cs_set_weapon_silen(iWpn, 1, 0);
        }
    }
}

stock SyncAllCTBots()
{
    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_connected(i) && is_user_bot(i) && is_user_alive(i))
        {
            AssignBotModernLoadouts(i);
            new CsTeams:team = safe_cs_get_user_team(i);
            if (team == CS_TEAM_CT)
            {
                AssignBotM4Loadout(i);
                if (cs_get_user_weapon(i) == CSW_M4A1)
                {
                    ApplyViewModel(i, CSW_M4A1, 0, true);
                }
            }
        }
    }
}

stock SetBotM4Mode(mode, issuer)
{
    set_pcvar_num(pCvar_BotM4Mode, mode);

    for (new i = 1; i <= MaxClients; i++)
    {
        g_iBotM4Override[i] = -1;
    }

    SyncAllCTBots();

    new const szModeNames[][] = {
        "Ikuti Active Loadout",
        "Role-Based (Smart)",
        "Campuran 50/50",
        "Semua M4A4",
        "Semua M4A1-S"
    };

    new issuer_name[32];
    if (issuer > 0 && is_user_connected(issuer))
        get_user_name(issuer, issuer_name, charsmax(issuer_name));
    else
        copy(issuer_name, charsmax(issuer_name), "Server Host");

    client_print_color(0, print_team_default,
        "^4[CS2 Loadout]^1 Mode M4 Bot CT disetel ke: ^3%s^1 oleh ^4%s^1.",
        szModeNames[mode], issuer_name);
}

public cs2_is_m4a4(id)
{
    if (id < 1 || id > MaxClients || !is_user_connected(id))
        return 0;

    new iWpn = fm_find_weapon_ent_by_owner(id, "weapon_m4a1");
    if (pev_valid(iWpn))
    {
        new iVar = pev(iWpn, pev_iuser1);
        if (iVar == 1) return 1;
        if (iVar == 2) return 0;
    }

    new slot = GET_SLOT(TEAM_CT_INDEX, CSW_M4A1);
    new s_idx = g_iPlayerSkin[id][slot];
    if (s_idx < 0 || s_idx >= g_SkinCount[slot])
        s_idx = 0;
    return g_SkinIsM4A4[slot][s_idx] ? 1 : 0;
}


// ==============================================================================
// M4A4 vs M4A1-S MECHANICS (Silencer Locking & World Drop)
// ==============================================================================

public OnM4Deploy_Pre(iWpn)
{
    if (!get_pcvar_num(pCvar_Enable) || !pev_valid(iWpn))
        return HAM_IGNORED;

    new id = get_pdata_cbase(iWpn, 41, 4);
    if (id < 1 || id > MaxClients || !is_user_alive(id))
        id = pev(iWpn, pev_owner);

    if (id < 1 || id > MaxClients || !is_user_alive(id))
        return HAM_IGNORED;

    new CsTeams:team = safe_cs_get_user_team(id);
    if (team != CS_TEAM_CT && team != CS_TEAM_T)
        return HAM_IGNORED;

    if (is_user_bot(id) && team == CS_TEAM_CT && pev(iWpn, pev_iuser1) == 0)
    {
        AssignBotM4Loadout(id);
    }

    new team_index = (team == CS_TEAM_CT) ? TEAM_CT_INDEX : TEAM_T_INDEX;
    new slot = GET_SLOT(team_index, CSW_M4A1);
    new s_idx = g_iPlayerSkin[id][slot];
    if (s_idx < 0 || s_idx >= g_SkinCount[slot])
        s_idx = 0;

    new iVariant = pev(iWpn, pev_iuser1);
    if (iVariant == 0)
    {
        iVariant = g_SkinIsM4A4[slot][s_idx] ? 1 : 2;
        set_pev(iWpn, pev_iuser1, iVariant);
    }

    if (iVariant == 1) // M4A4
    {
        if (cs_get_weapon_silen(iWpn))
            cs_set_weapon_silen(iWpn, 0, 0);
    }
    else // M4A1-S
    {
        if (get_pcvar_num(pCvar_AutoSilenceM4A1) && !cs_get_weapon_silen(iWpn))
            cs_set_weapon_silen(iWpn, 1, 0);
    }

    return HAM_IGNORED;
}

public OnM4AddToPlayer_Post(iWpn, id)
{
    if (!get_pcvar_num(pCvar_Enable) || !pev_valid(iWpn) || !is_user_alive(id))
        return HAM_IGNORED;

    new CsTeams:team = safe_cs_get_user_team(id);
    if (team != CS_TEAM_CT && team != CS_TEAM_T)
        return HAM_IGNORED;

    if (is_user_bot(id) && team == CS_TEAM_CT && pev(iWpn, pev_iuser1) == 0)
    {
        AssignBotM4Loadout(id);
    }

    new team_index = (team == CS_TEAM_CT) ? TEAM_CT_INDEX : TEAM_T_INDEX;
    new slot = GET_SLOT(team_index, CSW_M4A1);
    new s_idx = g_iPlayerSkin[id][slot];
    if (s_idx < 0 || s_idx >= g_SkinCount[slot])
        s_idx = 0;

    new iVariant = pev(iWpn, pev_iuser1);
    if (iVariant == 0)
    {
        iVariant = g_SkinIsM4A4[slot][s_idx] ? 1 : 2;
        set_pev(iWpn, pev_iuser1, iVariant);
    }

    if (iVariant == 1) // M4A4
    {
        if (cs_get_weapon_silen(iWpn))
            cs_set_weapon_silen(iWpn, 0, 0);
    }
    else // M4A1-S
    {
        if (get_pcvar_num(pCvar_AutoSilenceM4A1) && !cs_get_weapon_silen(iWpn))
            cs_set_weapon_silen(iWpn, 1, 0);
    }

    return HAM_IGNORED;
}

public OnM4SecondaryAttack_Pre(iWpn)
{
    if (!get_pcvar_num(pCvar_Enable) || !pev_valid(iWpn))
        return HAM_IGNORED;

    new id = get_pdata_cbase(iWpn, 41, 4);
    if (id < 1 || id > MaxClients || !is_user_alive(id))
        id = pev(iWpn, pev_owner);

    if (id < 1 || id > MaxClients || !is_user_alive(id))
        return HAM_IGNORED;

    new iVariant = pev(iWpn, pev_iuser1);
    if (iVariant == 1) // M4A4: block silencer completely
    {
        if (cs_get_weapon_silen(iWpn))
            cs_set_weapon_silen(iWpn, 0, 0);
        return HAM_SUPERCEDE;
    }

    // M4A1-S: CS2 permanent silencer lock (never allow detachment)
    if (get_pcvar_num(pCvar_BlockSilencerM4A4))
    {
        if (!cs_get_weapon_silen(iWpn))
        {
            cs_set_weapon_silen(iWpn, 1, 0);
        }
        return HAM_SUPERCEDE;
    }

    return HAM_IGNORED;
}

static bool:g_bSettingDropModel = false;

stock GetWeaponShootAnim(weapon_id)
{
    switch (weapon_id)
    {
        case CSW_P228:    return random_num(1, 3);
        case CSW_TMP:     return random_num(3, 5);
        case CSW_MAC10:   return random_num(3, 5);
        case CSW_MP5NAVY: return random_num(3, 5);
        case CSW_P90:     return random_num(3, 5);
        case CSW_XM1014:  return random_num(1, 2);
        case CSW_M3:      return random_num(1, 2);
        case CSW_M249:    return random_num(1, 2);
        case CSW_M4A1:    return random_num(8, 10);
    }
    return -1;
}

stock SendWeaponAnim(id, anim)
{
    set_pev(id, pev_weaponanim, anim);
    message_begin(MSG_ONE_UNRELIABLE, SVC_WEAPONANIM, _, id);
    write_byte(anim);
    write_byte(pev(id, pev_body));
    message_end();
}

public OnSetModel_Post(ent, const model[])
{
    if (!get_pcvar_num(pCvar_Enable) || !pev_valid(ent))
        return FMRES_IGNORED;

    if (g_bSettingDropModel)
        return FMRES_IGNORED;

    new classname[32];
    pev(ent, pev_classname, classname, charsmax(classname));

    // Only process dropped weaponbox entities
    if (!equal(classname, "weaponbox"))
        return FMRES_IGNORED;

    new id = pev(ent, pev_owner);
    new CsTeams:team = CS_TEAM_UNASSIGNED;
    if (id >= 1 && id <= MaxClients && is_user_connected(id))
    {
        team = safe_cs_get_user_team(id);
    }

    new team_index = (team == CS_TEAM_T) ? TEAM_T_INDEX : TEAM_CT_INDEX;

    // Check weapon entity inside weaponbox
    new iWpn = cs_get_weaponbox_item(ent);
    new weapon_id = CSW_NONE;
    if (pev_valid(iWpn))
    {
        weapon_id = cs_get_weapon_id(iWpn);
    }
    else
    {
        // Fallback identification from model name
        for (new w = 1; w <= CSW_LAST_WEAPON; w++)
        {
            if (g_WeaponModelNames[w][0] && containi(model, g_WeaponModelNames[w][2]) != -1)
            {
                weapon_id = w;
                break;
            }
        }
    }

    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return FMRES_IGNORED;

    // 1. M4A1 / M4A4 Handling
    if (weapon_id == CSW_M4A1)
    {
        new slot = GET_SLOT(TEAM_CT_INDEX, CSW_M4A1);
        new s_idx = (id >= 1 && id <= MaxClients) ? g_iPlayerSkin[id][slot] : 0;
        new bool:is_a4 = false;

        if (pev_valid(iWpn))
        {
            new iVar = pev(iWpn, pev_iuser1);
            if (iVar == 1) is_a4 = true;
            else if (iVar == 2) is_a4 = false;
            else
            {
                is_a4 = (team == CS_TEAM_CT) ? g_SkinIsM4A4[slot][s_idx] : false;
                set_pev(iWpn, pev_iuser1, is_a4 ? 1 : 2);
            }
        }
        else
        {
            is_a4 = (team == CS_TEAM_CT) ? g_SkinIsM4A4[slot][s_idx] : false;
        }

        new target_idx = GetCatalogM4Index(is_a4);
        if (target_idx >= 0 && target_idx < g_SkinCount[slot] && g_SkinWModel[slot][target_idx][0])
        {
            if (!equal(model, g_SkinWModel[slot][target_idx]))
            {
                g_bSettingDropModel = true;
                engfunc(EngFunc_SetModel, ent, g_SkinWModel[slot][target_idx]);
                g_bSettingDropModel = false;
            }
            set_pev(ent, pev_iuser1, is_a4 ? 1 : 2);
            return FMRES_SUPERCEDE;
        }
        return FMRES_IGNORED;
    }

    // 2. All other weapons (Tec-9, CZ75, MP7, PP-Bizon, MAG-7, Sawed-Off, Negev, etc.)
    new slot = GET_SLOT(team_index, weapon_id);
    new s_idx = (id >= 1 && id <= MaxClients) ? g_iPlayerSkin[id][slot] : 0;

    if (pev_valid(iWpn))
    {
        new iSaved = pev(iWpn, pev_iuser2);
        if (iSaved > 0 && (iSaved - 1) < g_SkinCount[slot])
        {
            s_idx = iSaved - 1;
        }
        else if (s_idx >= 0 && s_idx < g_SkinCount[slot])
        {
            set_pev(iWpn, pev_iuser2, s_idx + 1);
        }
    }

    if (s_idx >= 0 && s_idx < g_SkinCount[slot] && g_SkinWModel[slot][s_idx][0])
    {
        if (!equal(model, g_SkinWModel[slot][s_idx]))
        {
            g_bSettingDropModel = true;
            engfunc(EngFunc_SetModel, ent, g_SkinWModel[slot][s_idx]);
            g_bSettingDropModel = false;
            set_pev(ent, pev_iuser2, s_idx + 1);
            set_pev(ent, pev_iuser1, weapon_id);
            return FMRES_SUPERCEDE;
        }
    }

    return FMRES_IGNORED;
}

public Arsenal_SkinModels_AddToPlayerPost(iWpn, id)
{
    if (!get_pcvar_num(pCvar_Enable) || !pev_valid(iWpn) || !is_user_alive(id))
        return HAM_IGNORED;

    new weapon_id = cs_get_weapon_id(iWpn);
    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON || weapon_id == CSW_M4A1)
        return HAM_IGNORED;

    new CsTeams:team = safe_cs_get_user_team(id);
    if (team != CS_TEAM_CT && team != CS_TEAM_T)
        return HAM_IGNORED;

    new team_index = (team == CS_TEAM_CT) ? TEAM_CT_INDEX : TEAM_T_INDEX;
    new slot = GET_SLOT(team_index, weapon_id);

    // If weapon has no skin stamped yet (e.g. freshly purchased), stamp player's equipped skin
    new iSaved = pev(iWpn, pev_iuser2);
    if (iSaved == 0)
    {
        new s_idx = g_iPlayerSkin[id][slot];
        set_pev(iWpn, pev_iuser2, s_idx + 1);
    }

    return HAM_IGNORED;
}

