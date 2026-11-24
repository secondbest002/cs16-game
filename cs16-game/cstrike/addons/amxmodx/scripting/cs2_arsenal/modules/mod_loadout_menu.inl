#if defined _mod_loadout_menu_included
    #endinput
#endif
#define _mod_loadout_menu_included

// ==============================================================================
// CS2 LOADOUT MENUS
// ==============================================================================

public Cmd_OpenLoadout(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    ShowMainMenu(id);
    return PLUGIN_HANDLED;
}

public Cmd_OpenKnifeMenu(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    ShowDirectKnifeMenu(id);
    return PLUGIN_HANDLED;
}

public Cmd_OpenM4Menu(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    new CsTeams:team = safe_cs_get_user_team(id);
    new team_index = (team == CS_TEAM_T) ? TEAM_T_INDEX : TEAM_CT_INDEX;
    ShowWeaponSkinMenu(id, team_index, CSW_M4A1);
    return PLUGIN_HANDLED;
}

public Cmd_OpenGloveInfo(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    ShowGloveInfoMenu(id);
    return PLUGIN_HANDLED;
}

public Cmd_SelectM4A4Direct(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    new CsTeams:team = safe_cs_get_user_team(id);
    new team_index = (team == CS_TEAM_T) ? TEAM_T_INDEX : TEAM_CT_INDEX;
    new slot = GET_SLOT(team_index, CSW_M4A1);
    for (new s = 0; s < g_SkinCount[slot]; s++)
    {
        if (g_SkinIsM4A4[slot][s])
        {
            SelectWeaponSkin(id, team_index, CSW_M4A1, s);
            return PLUGIN_HANDLED;
        }
    }

    client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 No M4A4 skin found in catalog.");
    return PLUGIN_HANDLED;
}

public Cmd_SelectM4A1Direct(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    new CsTeams:team = safe_cs_get_user_team(id);
    new team_index = (team == CS_TEAM_T) ? TEAM_T_INDEX : TEAM_CT_INDEX;
    new slot = GET_SLOT(team_index, CSW_M4A1);
    for (new s = 0; s < g_SkinCount[slot]; s++)
    {
        if (!g_SkinIsM4A4[slot][s])
        {
            SelectWeaponSkin(id, team_index, CSW_M4A1, s);
            return PLUGIN_HANDLED;
        }
    }

    client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 No M4A1-S skin found in catalog.");
    return PLUGIN_HANDLED;
}

public Cmd_SelectTec9Direct(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    new bool:found = false;
    for (new t = 0; t < 2; t++)
    {
        new slot = GET_SLOT(t, CSW_P228);
        for (new s = 0; s < g_SkinCount[slot]; s++)
        {
            if (containi(g_SkinName[slot][s], "tec") != -1 || containi(g_SkinVModel[slot][s], "tec9") != -1)
            {
                SelectWeaponSkin(id, t, CSW_P228, s);
                found = true;
                break;
            }
        }
    }

    if (!found)
        client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 No Tec-9 skin found in catalog.");

    return PLUGIN_HANDLED;
}

public Cmd_SelectCZ75Direct(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    new bool:found = false;
    // CT slot: TMP
    new ct_slot = GET_SLOT(TEAM_CT_INDEX, CSW_TMP);
    for (new s = 0; s < g_SkinCount[ct_slot]; s++)
    {
        if (containi(g_SkinName[ct_slot][s], "cz") != -1 || containi(g_SkinVModel[ct_slot][s], "cz75") != -1)
        {
            SelectWeaponSkin(id, TEAM_CT_INDEX, CSW_TMP, s);
            found = true;
            break;
        }
    }

    // T slot: MAC-10
    new t_slot = GET_SLOT(TEAM_T_INDEX, CSW_MAC10);
    for (new s = 0; s < g_SkinCount[t_slot]; s++)
    {
        if (containi(g_SkinName[t_slot][s], "cz") != -1 || containi(g_SkinVModel[t_slot][s], "cz75") != -1)
        {
            SelectWeaponSkin(id, TEAM_T_INDEX, CSW_MAC10, s);
            found = true;
            break;
        }
    }

    if (!found)
        client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 No CZ75-Auto skin found in catalog.");

    return PLUGIN_HANDLED;
}

public Cmd_SelectMP7Direct(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    new bool:found = false;
    for (new t = 0; t < 2; t++)
    {
        new slot = GET_SLOT(t, CSW_MP5NAVY);
        for (new s = 0; s < g_SkinCount[slot]; s++)
        {
            if (containi(g_SkinName[slot][s], "mp7") != -1 || containi(g_SkinVModel[slot][s], "mp7") != -1)
            {
                SelectWeaponSkin(id, t, CSW_MP5NAVY, s);
                found = true;
                break;
            }
        }
    }

    if (!found)
        client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 No MP7 skin found in catalog.");

    return PLUGIN_HANDLED;
}

public Cmd_SelectBizonDirect(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    new bool:found = false;
    for (new t = 0; t < 2; t++)
    {
        new slot = GET_SLOT(t, CSW_P90);
        for (new s = 0; s < g_SkinCount[slot]; s++)
        {
            if (containi(g_SkinName[slot][s], "bizon") != -1 || containi(g_SkinVModel[slot][s], "bizon") != -1)
            {
                SelectWeaponSkin(id, t, CSW_P90, s);
                found = true;
                break;
            }
        }
    }

    if (!found)
        client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 No PP-Bizon skin found in catalog.");

    return PLUGIN_HANDLED;
}

public Cmd_SelectMAG7Direct(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    new bool:found = false;
    for (new t = 0; t < 2; t++)
    {
        new slot = GET_SLOT(t, CSW_XM1014);
        for (new s = 0; s < g_SkinCount[slot]; s++)
        {
            if (containi(g_SkinName[slot][s], "mag") != -1 || containi(g_SkinVModel[slot][s], "mag7") != -1)
            {
                SelectWeaponSkin(id, t, CSW_XM1014, s);
                found = true;
                break;
            }
        }
    }

    if (!found)
        client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 No MAG-7 skin found in catalog.");

    return PLUGIN_HANDLED;
}

public Cmd_SelectSawedOffDirect(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    new bool:found = false;
    for (new t = 0; t < 2; t++)
    {
        new slot = GET_SLOT(t, CSW_M3);
        for (new s = 0; s < g_SkinCount[slot]; s++)
        {
            if (containi(g_SkinName[slot][s], "sawed") != -1 || containi(g_SkinVModel[slot][s], "sawedoff") != -1)
            {
                SelectWeaponSkin(id, t, CSW_M3, s);
                found = true;
                break;
            }
        }
    }

    if (!found)
        client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 No Sawed-Off skin found in catalog.");

    return PLUGIN_HANDLED;
}

public Cmd_SelectNegevDirect(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    new bool:found = false;
    for (new t = 0; t < 2; t++)
    {
        new slot = GET_SLOT(t, CSW_M249);
        for (new s = 0; s < g_SkinCount[slot]; s++)
        {
            if (containi(g_SkinName[slot][s], "negev") != -1 || containi(g_SkinVModel[slot][s], "negev") != -1)
            {
                SelectWeaponSkin(id, t, CSW_M249, s);
                found = true;
                break;
            }
        }
    }

    if (!found)
        client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 No Negev skin found in catalog.");

    return PLUGIN_HANDLED;
}

public Cmd_ResetLoadout(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    ResetPlayerLoadout(id);
    return PLUGIN_HANDLED;
}

public Cmd_ReloadActiveLoadout(id, level, cid)
{

    new active_config[128];
    get_configsdir(active_config, charsmax(active_config));
    format(active_config, charsmax(active_config), "%s/active_loadout.ini", active_config);

    if (file_exists(active_config))
    {
        LoadActiveLoadoutConfig(active_config);
        for (new i = 1; i <= MaxClients; i++)
        {
            if (is_user_connected(i))
            {
                for (new s = 0; s < TOTAL_SLOTS; s++)
                {
                    g_iPlayerSkin[i][s] = g_ActiveLoadoutSkin[s];
                }
                if (is_user_alive(i))
                {
                    new wid = cs_get_user_weapon(i);
                    if (wid > CSW_NONE && wid <= CSW_LAST_WEAPON)
                    {
                        ApplyViewModel(i, wid, 0, true);
                    }
                }
            }
        }
        if (id == 0)
            server_print("[CS2 Loadout] active_loadout.ini reloaded successfully.");
        else
            client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 active_loadout.ini reloaded successfully.");
    }
    else
    {
        if (id == 0)
            server_print("[CS2 Loadout] active_loadout.ini not found.");
        else
            client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 active_loadout.ini not found.");
    }

    return PLUGIN_HANDLED;
}

// ------------------------------------------------------------------------------
// Main Menu
// ------------------------------------------------------------------------------

public ShowMainMenu(id)
{
    new title[128];
    formatex(title, charsmax(title), "\y[CS2 LOADOUT & INVENTORY]\w^n\dSesuaikan Skin, Knife & Loadout Tim Anda:\w");
    new menu = menu_create(title, "Handler_MainMenu");

    menu_additem(menu, "\w[CT] Counter-Terrorists Loadout", "1");
    menu_additem(menu, "\w[T]  Terrorists Loadout", "2");
    menu_additem(menu, "\w[Knife] Knife Selection (CT & T)", "3");
    menu_additem(menu, "\w[Gloves] Glove Options & Guide", "4");
    menu_additem(menu, "\w[Bot M4] CT Bot M4 Loadout System", "5");
    menu_additem(menu, "\r[Reset] Reset All to Defaults", "6");

    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, menu, 0);
}

public Handler_MainMenu(id, menu, item)
{
    if (item == MENU_EXIT || !is_user_connected(id))
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new szData[6], szName[64], iAccess, iCallback;
    menu_item_getinfo(menu, item, iAccess, szData, charsmax(szData), szName, charsmax(szName), iCallback);
    menu_destroy(menu);

    switch (str_to_num(szData))
    {
        case 1: ShowTeamLoadoutMenu(id, TEAM_CT_INDEX);
        case 2: ShowTeamLoadoutMenu(id, TEAM_T_INDEX);
        case 3: ShowDirectKnifeMenu(id);
        case 4: ShowGloveInfoMenu(id);
        case 5: ShowBotM4Menu(id);
        case 6: ResetPlayerLoadout(id);
    }

    return PLUGIN_HANDLED;
}

// ------------------------------------------------------------------------------
// Team Loadout Menu (CT or T)
// ------------------------------------------------------------------------------

public ShowTeamLoadoutMenu(id, team_index)
{
    new title[128];
    formatex(title, charsmax(title), "\y[CS2 %s Loadout]\w^nSelect Weapon Category:",
        (team_index == TEAM_CT_INDEX) ? "Counter-Terrorist" : "Terrorist");

    new menu = menu_create(title, (team_index == TEAM_CT_INDEX) ? "Handler_CTLoadout" : "Handler_TLoadout");

    // Knife slot line with active knife name
    new knife_slot = GET_SLOT(team_index, CSW_KNIFE);
    new knife_s = g_iPlayerSkin[id][knife_slot];
    new knife_item[96], k_name[32];
    copy(k_name, charsmax(k_name), g_SkinName[knife_slot][knife_s]);
    if (!k_name[0]) copy(k_name, charsmax(k_name), "Default Knife");
    formatex(knife_item, charsmax(knife_item), "\w[Knife] Knife Slot \y[%s]\w", k_name);
    menu_additem(menu, knife_item, "0");

    menu_additem(menu, "\w[Pistols] Pistols (USP-S, Glock, Deagle, Berettas...)", "1");
    menu_additem(menu, "\w[Mid-Tier] Mid-Tier (MP5-SD, UMP-45, XM1014, Shotguns...)", "2");
    menu_additem(menu, "\w[Rifles] Rifles & Snipers (M4 Slot, AK-47, AWP, Scout...)", "3");

    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, menu, 0);
}

public Handler_CTLoadout(id, menu, item)
{
    if (item == MENU_EXIT || !is_user_connected(id))
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new szData[6], szName[64], iAccess, iCallback;
    menu_item_getinfo(menu, item, iAccess, szData, charsmax(szData), szName, charsmax(szName), iCallback);
    menu_destroy(menu);

    new choice = str_to_num(szData);
    if (choice == 0)
    {
        ShowWeaponSkinMenu(id, TEAM_CT_INDEX, CSW_KNIFE);
    }
    else
    {
        ShowCategoryMenu(id, TEAM_CT_INDEX, choice);
    }

    return PLUGIN_HANDLED;
}

public Handler_TLoadout(id, menu, item)
{
    if (item == MENU_EXIT || !is_user_connected(id))
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new szData[6], szName[64], iAccess, iCallback;
    menu_item_getinfo(menu, item, iAccess, szData, charsmax(szData), szName, charsmax(szName), iCallback);
    menu_destroy(menu);

    new choice = str_to_num(szData);
    if (choice == 0)
    {
        ShowWeaponSkinMenu(id, TEAM_T_INDEX, CSW_KNIFE);
    }
    else
    {
        ShowCategoryMenu(id, TEAM_T_INDEX, choice);
    }

    return PLUGIN_HANDLED;
}

// ------------------------------------------------------------------------------
// Category Menu
// ------------------------------------------------------------------------------

public ShowCategoryMenu(id, team_index, category_index)
{
    new title[128];
    new cat_name[32];
    switch (category_index)
    {
        case 1: copy(cat_name, charsmax(cat_name), "Pistols");
        case 2: copy(cat_name, charsmax(cat_name), "Mid-Tier");
        case 3: copy(cat_name, charsmax(cat_name), "Rifles & Snipers");
    }

    formatex(title, charsmax(title), "\y[CS2 %s] %s:\w",
        (team_index == TEAM_CT_INDEX) ? "CT" : "T", cat_name);

    new menu = menu_create(title, "Handler_CategoryMenu");

    // Populate category weapons
    new weapons_to_show[12], wep_count = 0;

    if (category_index == 1) // Pistols
    {
        if (team_index == TEAM_CT_INDEX)
        {
            weapons_to_show[wep_count++] = CSW_USP;
            weapons_to_show[wep_count++] = CSW_DEAGLE;
            weapons_to_show[wep_count++] = CSW_FIVESEVEN;
            weapons_to_show[wep_count++] = CSW_P228;
            weapons_to_show[wep_count++] = CSW_ELITE;
        }
        else
        {
            weapons_to_show[wep_count++] = CSW_GLOCK18;
            weapons_to_show[wep_count++] = CSW_DEAGLE;
            weapons_to_show[wep_count++] = CSW_ELITE;
            weapons_to_show[wep_count++] = CSW_P228;
        }
    }
    else if (category_index == 2) // Mid-Tier
    {
        weapons_to_show[wep_count++] = CSW_MP5NAVY;
        weapons_to_show[wep_count++] = CSW_UMP45;
        weapons_to_show[wep_count++] = CSW_XM1014;
        weapons_to_show[wep_count++] = CSW_M3;
        weapons_to_show[wep_count++] = CSW_P90;
        if (team_index == TEAM_CT_INDEX)
            weapons_to_show[wep_count++] = CSW_TMP;
        else
            weapons_to_show[wep_count++] = CSW_MAC10;
        weapons_to_show[wep_count++] = CSW_M249;
    }
    else if (category_index == 3) // Rifles
    {
        if (team_index == TEAM_CT_INDEX)
        {
            weapons_to_show[wep_count++] = CSW_M4A1; // M4 slot (M4A4 & M4A1-S)
            weapons_to_show[wep_count++] = CSW_AWP;
            weapons_to_show[wep_count++] = CSW_SCOUT;
            weapons_to_show[wep_count++] = CSW_FAMAS;
            weapons_to_show[wep_count++] = CSW_AUG;
            weapons_to_show[wep_count++] = CSW_SG550;
        }
        else
        {
            weapons_to_show[wep_count++] = CSW_AK47;
            weapons_to_show[wep_count++] = CSW_M4A1; // M4 slot (M4A4 & M4A1-S)
            weapons_to_show[wep_count++] = CSW_AWP;
            weapons_to_show[wep_count++] = CSW_SCOUT;
            weapons_to_show[wep_count++] = CSW_GALIL;
            weapons_to_show[wep_count++] = CSW_SG552;
            weapons_to_show[wep_count++] = CSW_G3SG1;
        }
    }

    for (new i = 0; i < wep_count; i++)
    {
        new wid = weapons_to_show[i];
        new slot = GET_SLOT(team_index, wid);
        new s_idx = g_iPlayerSkin[id][slot];
        new item_str[96], item_info[16];
        new skin_name[32];
        copy(skin_name, charsmax(skin_name), g_SkinName[slot][s_idx]);
        if (!skin_name[0]) copy(skin_name, charsmax(skin_name), "Default");

        formatex(item_str, charsmax(item_str), "%s \y[%s]\w",
            g_WeaponDisplayNames[wid], skin_name);
        formatex(item_info, charsmax(item_info), "%d_%d", team_index, wid);

        menu_additem(menu, item_str, item_info);
    }

    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, menu, 0);
}

public Handler_CategoryMenu(id, menu, item)
{
    if (item == MENU_EXIT || !is_user_connected(id))
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new szData[16], szName[64], iAccess, iCallback;
    menu_item_getinfo(menu, item, iAccess, szData, charsmax(szData), szName, charsmax(szName), iCallback);
    menu_destroy(menu);

    new team_str[6], wid_str[6];
    strtok(szData, team_str, charsmax(team_str), wid_str, charsmax(wid_str), '_');

    new team_idx = str_to_num(team_str);
    new wid = str_to_num(wid_str);

    ShowWeaponSkinMenu(id, team_idx, wid);
    return PLUGIN_HANDLED;
}

// ------------------------------------------------------------------------------
// Weapon Skin Selection Menu
// ------------------------------------------------------------------------------

public ShowWeaponSkinMenu(id, team_index, weapon_id)
{
    new title[128];
    formatex(title, charsmax(title), "\y[CS2 %s] Select Skin: %s\w",
        (team_index == TEAM_CT_INDEX) ? "CT" : "T", g_WeaponDisplayNames[weapon_id]);

    new menu = menu_create(title, "Handler_WeaponSkinMenu");

    new slot = GET_SLOT(team_index, weapon_id);
    new cur_skin = g_iPlayerSkin[id][slot];
    new total_skins = g_SkinCount[slot];

    if (total_skins == 0)
    {
        menu_additem(menu, "\d(No custom skins registered - using default model)\w", "none");
    }
    else
    {
        for (new s = 0; s < total_skins; s++)
        {
            new item_str[96], item_data[16];
            new bool:is_equipped = (s == cur_skin);

            formatex(item_str, charsmax(item_str), "%s%s",
                g_SkinName[slot][s], is_equipped ? " \r(Equipped)\w" : "");
            formatex(item_data, charsmax(item_data), "%d_%d_%d", team_index, weapon_id, s);

            menu_additem(menu, item_str, item_data);
        }
    }

    if (weapon_id == CSW_M4A1 && team_index == TEAM_CT_INDEX)
    {
        menu_additem(menu, "\y[Bot Loadout] CT Bot M4 Settings...\w", "bot_m4_manage");
    }

    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, menu, 0);
}

public Handler_WeaponSkinMenu(id, menu, item)
{
    if (item == MENU_EXIT || !is_user_connected(id))
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new szData[24], szName[64], iAccess, iCallback;
    menu_item_getinfo(menu, item, iAccess, szData, charsmax(szData), szName, charsmax(szName), iCallback);
    menu_destroy(menu);

    if (equal(szData, "bot_m4_manage"))
    {
        ShowBotM4Menu(id);
        return PLUGIN_HANDLED;
    }

    if (equal(szData, "none"))
        return PLUGIN_HANDLED;

    new team_str[6], rest[16], wid_str[6], skin_str[6];
    strtok(szData, team_str, charsmax(team_str), rest, charsmax(rest), '_');
    strtok(rest, wid_str, charsmax(wid_str), skin_str, charsmax(skin_str), '_');

    new team_idx = str_to_num(team_str);
    new wid = str_to_num(wid_str);
    new skin_idx = str_to_num(skin_str);

    SelectWeaponSkin(id, team_idx, wid, skin_idx);
    return PLUGIN_HANDLED;
}

// ------------------------------------------------------------------------------
// Direct Knife Menu (/knife)
// ------------------------------------------------------------------------------

public ShowDirectKnifeMenu(id)
{
    new title[128];
    formatex(title, charsmax(title), "\y[CS2 Knife Loadout] Select Team:\w");
    new menu = menu_create(title, "Handler_DirectKnifeMenu");

    new ct_slot = GET_SLOT(TEAM_CT_INDEX, CSW_KNIFE);
    new t_slot = GET_SLOT(TEAM_T_INDEX, CSW_KNIFE);

    new ct_k = g_iPlayerSkin[id][ct_slot];
    new t_k = g_iPlayerSkin[id][t_slot];

    new ct_line[96], t_line[96];
    new ct_name[32], t_name[32];
    copy(ct_name, charsmax(ct_name), g_SkinName[ct_slot][ct_k]);
    if (!ct_name[0]) copy(ct_name, charsmax(ct_name), "Default Knife");
    copy(t_name, charsmax(t_name), g_SkinName[t_slot][t_k]);
    if (!t_name[0]) copy(t_name, charsmax(t_name), "Default Knife");

    formatex(ct_line, charsmax(ct_line), "[CT] Knife Slot \y[%s]\w", ct_name);
    formatex(t_line, charsmax(t_line), "[T]  Knife Slot \y[%s]\w", t_name);

    menu_additem(menu, ct_line, "0");
    menu_additem(menu, t_line, "1");

    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, menu, 0);
}

public Handler_DirectKnifeMenu(id, menu, item)
{
    if (item == MENU_EXIT || !is_user_connected(id))
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new szData[6], szName[64], iAccess, iCallback;
    menu_item_getinfo(menu, item, iAccess, szData, charsmax(szData), szName, charsmax(szName), iCallback);
    menu_destroy(menu);

    new team_idx = str_to_num(szData);
    ShowWeaponSkinMenu(id, team_idx, CSW_KNIFE);
    return PLUGIN_HANDLED;
}

// ------------------------------------------------------------------------------
// Glove Guide Menu (/gloves)
// ------------------------------------------------------------------------------

public ShowGloveInfoMenu(id)
{
    new menu = menu_create("\y[CS2 Glove System & Guide]\w", "Handler_GloveInfoMenu");

    menu_additem(menu, "\w- Gloves are baked directly into each weapon model.", "1");
    menu_additem(menu, "\w- CT sleeves & T arms are preserved automatically.", "2");
    menu_additem(menu, "\w- Use: \ytools/cs2_model_baker.py\w to bake new custom gloves.", "3");
    menu_additem(menu, "\w- Return to Main Loadout", "4");

    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, menu, 0);
}

public Handler_GloveInfoMenu(id, menu, item)
{
    if (item == MENU_EXIT || !is_user_connected(id))
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    menu_destroy(menu);
    ShowMainMenu(id);
    return PLUGIN_HANDLED;
}

// ------------------------------------------------------------------------------
// Skin Selection Logic & Persistence
// ------------------------------------------------------------------------------

stock SelectWeaponSkin(id, team, weapon_id, skin_idx)
{
    new slot = GET_SLOT(team, weapon_id);
    if (skin_idx < 0 || skin_idx >= g_SkinCount[slot])
        return;

    g_iPlayerSkin[id][slot] = skin_idx;
    SavePlayerWeaponSkin(id, team, weapon_id);

    if (get_pcvar_num(pCvar_Notify))
    {
        client_print_color(id, print_team_default,
            "^4[CS2 Loadout]^1 Equiped ^3%s^1 untuk ^4%s^1 (%s)!",
            g_SkinName[slot][skin_idx],
            g_WeaponDisplayNames[weapon_id],
            (team == TEAM_CT_INDEX) ? "CT" : "T");
    }

    // Instantly update active weapon viewmodel if player is currently holding it
    if (is_user_alive(id))
    {
        new CsTeams:user_team = safe_cs_get_user_team(id);
        new user_team_idx = (user_team == CS_TEAM_CT) ? TEAM_CT_INDEX : TEAM_T_INDEX;

        if (user_team_idx == team && cs_get_user_weapon(id) == weapon_id)
        {
            if (weapon_id == CSW_M4A1)
            {
                new iWpn = fm_find_weapon_ent_by_owner(id, "weapon_m4a1");
                if (pev_valid(iWpn))
                {
                    new bool:is_a4 = g_SkinIsM4A4[slot][skin_idx];
                    set_pev(iWpn, pev_iuser1, is_a4 ? 1 : 2);
                    if (is_a4 && cs_get_weapon_silen(iWpn))
                    {
                        cs_set_weapon_silen(iWpn, 0, 0);
                    }
                    else if (!is_a4 && get_pcvar_num(pCvar_AutoSilenceM4A1) && !cs_get_weapon_silen(iWpn))
                    {
                        cs_set_weapon_silen(iWpn, 1, 0);
                    }
                }
            }
            else
            {
                new iWpn = fm_find_weapon_ent_by_owner(id, g_WeaponEntityNames[weapon_id]);
                if (pev_valid(iWpn))
                {
                    set_pev(iWpn, pev_iuser2, skin_idx + 1);
                }
            }
            ApplyViewModel(id, weapon_id, 0, true);
        }
    }
}

stock ResetPlayerLoadout(id)
{
    for (new t = 0; t < 2; t++)
    {
        for (new w = 0; w <= CSW_LAST_WEAPON; w++)
        {
            new slot = GET_SLOT(t, w);
            g_iPlayerSkin[id][slot] = g_ActiveLoadoutSkin[slot];
            if (g_SkinCount[slot] > 0)
            {
                SavePlayerWeaponSkin(id, t, w);
            }
        }
    }

    if (is_user_alive(id))
    {
        new wid = cs_get_user_weapon(id);
        if (wid > CSW_NONE && wid <= CSW_LAST_WEAPON)
        {
            ApplyViewModel(id, wid, 0, true);
        }
    }

    client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 Seluruh loadout CT & T telah direset ke default.");
}

// ==============================================================================
// CT BOT M4 LOADOUT MENUS & COMMANDS (/botm4)
// ==============================================================================

public Cmd_BotM4Command(id)
{
    if (!get_pcvar_num(pCvar_Enable) || !is_user_connected(id))
        return PLUGIN_CONTINUE;

    if (!CanAccessBotM4Menu(id))
    {
        client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 Akses ditolak. Hanya server host / admin yang dapat mengatur Bot M4.");
        return PLUGIN_HANDLED;
    }

    new args[32];
    read_args(args, charsmax(args));
    remove_quotes(args);
    trim(args);

    if (containi(args, "role") != -1 || containi(args, "smart") != -1)
    {
        SetBotM4Mode(1, id);
        return PLUGIN_HANDLED;
    }
    else if (containi(args, "mix") != -1 || containi(args, "50") != -1)
    {
        SetBotM4Mode(2, id);
        return PLUGIN_HANDLED;
    }
    else if (containi(args, "m4a4") != -1 || containi(args, "a4") != -1)
    {
        SetBotM4Mode(3, id);
        return PLUGIN_HANDLED;
    }
    else if (containi(args, "m4a1") != -1 || containi(args, "a1") != -1)
    {
        SetBotM4Mode(4, id);
        return PLUGIN_HANDLED;
    }
    else if (containi(args, "auto") != -1 || containi(args, "mirror") != -1 || containi(args, "default") != -1)
    {
        SetBotM4Mode(0, id);
        return PLUGIN_HANDLED;
    }

    ShowBotM4Menu(id);
    return PLUGIN_HANDLED;
}

public ShowBotM4Menu(id)
{
    if (!CanAccessBotM4Menu(id))
    {
        client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 Menu pengaturan Bot M4 hanya dapat diakses oleh Admin.");
        return;
    }

    new cur_mode = get_pcvar_num(pCvar_BotM4Mode);
    new title[192];
    formatex(title, charsmax(title), "\y[CS2 CT BOT M4 LOADOUT]\w^nActive Mode: \r%s\w^n^nSelect CT Bot M4 Distribution:",
        (cur_mode == 1) ? "Role-Based (Smart)" :
        (cur_mode == 2) ? "50/50 Split" :
        (cur_mode == 3) ? "All M4A4" :
        (cur_mode == 4) ? "All M4A1-S" : "Follow Active Loadout");

    new menu = menu_create(title, "Handler_BotM4Menu");

    new item[128];
    formatex(item, charsmax(item), "\wRole-Based (Smart) \d(Entry=M4A4, Supp=M4A1-S)\w %s", (cur_mode == 1) ? "\y[ACTIVE]\w" : "");
    menu_additem(menu, item, "1");

    formatex(item, charsmax(item), "\w50/50 Split \d(Realistic Roster Split)\w %s", (cur_mode == 2) ? "\y[ACTIVE]\w" : "");
    menu_additem(menu, item, "2");

    formatex(item, charsmax(item), "\wAll M4A4 \d(All CT Bots Equip M4A4)\w %s", (cur_mode == 3) ? "\y[ACTIVE]\w" : "");
    menu_additem(menu, item, "3");

    formatex(item, charsmax(item), "\wAll M4A1-S \d(All CT Bots Equip M4A1-S)\w %s", (cur_mode == 4) ? "\y[ACTIVE]\w" : "");
    menu_additem(menu, item, "4");

    formatex(item, charsmax(item), "\wFollow Active Loadout \d(Default Companion)\w %s", (cur_mode == 0) ? "\y[ACTIVE]\w" : "");
    menu_additem(menu, item, "0");

    menu_additem(menu, "\yIndividual Bot Customization...\w", "99");

    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, menu, 0);
}

public Handler_BotM4Menu(id, menu, item)
{
    if (item == MENU_EXIT || !is_user_connected(id))
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new szData[6], szName[64], iAccess, iCallback;
    menu_item_getinfo(menu, item, iAccess, szData, charsmax(szData), szName, charsmax(szName), iCallback);
    menu_destroy(menu);

    new choice = str_to_num(szData);
    if (choice == 99)
    {
        ShowIndividualBotM4Menu(id);
    }
    else
    {
        SetBotM4Mode(choice, id);
        ShowBotM4Menu(id);
    }

    return PLUGIN_HANDLED;
}

public ShowIndividualBotM4Menu(id)
{
    new menu = menu_create("\y[Individual Bot M4 Customization]\w^nSelect CT Bot to toggle (Auto -> M4A4 -> M4A1-S):", "Handler_IndividualBotM4Menu");

    new bot_count = 0;
    new slot = GET_SLOT(TEAM_CT_INDEX, CSW_M4A1);

    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_connected(i) && is_user_bot(i) && safe_cs_get_user_team(i) == CS_TEAM_CT)
        {
            bot_count++;
            new szBotName[32], item_str[128], item_data[8];
            get_user_name(i, szBotName, charsmax(szBotName));

            new s_idx = g_iPlayerSkin[i][slot];
            new bool:is_a4 = g_SkinIsM4A4[slot][s_idx];

            new status[24];
            if (g_iBotM4Override[i] == -1)
            {
                formatex(status, charsmax(status), "Auto (%s)", is_a4 ? "M4A4" : "M4A1-S");
            }
            else if (g_iBotM4Override[i] == 0)
            {
                copy(status, charsmax(status), "Force M4A4");
            }
            else
            {
                copy(status, charsmax(status), "Force M4A1-S");
            }

            formatex(item_str, charsmax(item_str), "%s \y[%s]\w", szBotName, status);
            num_to_str(i, item_data, charsmax(item_data));
            menu_additem(menu, item_str, item_data);
        }
    }

    if (bot_count == 0)
    {
        menu_additem(menu, "\d(No Bots currently in CT team)\w", "0");
    }

    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, menu, 0);
}

public Handler_IndividualBotM4Menu(id, menu, item)
{
    if (item == MENU_EXIT || !is_user_connected(id))
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new szData[8], szName[64], iAccess, iCallback;
    menu_item_getinfo(menu, item, iAccess, szData, charsmax(szData), szName, charsmax(szName), iCallback);
    menu_destroy(menu);

    new target_bot = str_to_num(szData);
    if (target_bot >= 1 && target_bot <= MaxClients && is_user_connected(target_bot))
    {
        if (g_iBotM4Override[target_bot] == -1)
            g_iBotM4Override[target_bot] = 0; // Force M4A4
        else if (g_iBotM4Override[target_bot] == 0)
            g_iBotM4Override[target_bot] = 1; // Force M4A1-S
        else
            g_iBotM4Override[target_bot] = -1; // Auto

        AssignBotM4Loadout(target_bot);
        if (is_user_alive(target_bot) && cs_get_user_weapon(target_bot) == CSW_M4A1)
        {
            ApplyViewModel(target_bot, CSW_M4A1, 0, true);
        }

        new szBotName[32];
        get_user_name(target_bot, szBotName, charsmax(szBotName));
        client_print_color(id, print_team_default, "^4[CS2 Loadout]^1 Bot ^3%s^1 set to: ^4%s^1.",
            szBotName,
            (g_iBotM4Override[target_bot] == 0) ? "Force M4A4" :
            (g_iBotM4Override[target_bot] == 1) ? "Force M4A1-S" : "Auto (Team Mode)");

        ShowIndividualBotM4Menu(id);
    }

    return PLUGIN_HANDLED;
}



public bool:_native_is_m4a4(plugin, params)
{
    return bool:cs2_is_m4a4(get_param(1));
}

public _native_get_active_skin_name(plugin, params)
{
    new id = get_param(1);
    new weapon_id = get_param(2);
    if (id < 1 || id > MaxClients || !is_user_connected(id) || weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return 0;

    new CsTeams:team = safe_cs_get_user_team(id);
    new team_index = (team == CS_TEAM_CT) ? TEAM_CT_INDEX : TEAM_T_INDEX;
    new slot = GET_SLOT(team_index, weapon_id);
    new s_idx = g_iPlayerSkin[id][slot];
    if (s_idx < 0 || s_idx >= g_SkinCount[slot])
        return 0;

    set_string(3, g_SkinName[slot][s_idx], get_param(4));
    return 1;
}

public _native_get_active_skin_vmodel(plugin, params)
{
    new id = get_param(1);
    new weapon_id = get_param(2);
    if (id < 1 || id > MaxClients || !is_user_connected(id) || weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return 0;

    new CsTeams:team = safe_cs_get_user_team(id);
    new team_index = (team == CS_TEAM_CT) ? TEAM_CT_INDEX : TEAM_T_INDEX;
    new slot = GET_SLOT(team_index, weapon_id);
    new s_idx = g_iPlayerSkin[id][slot];
    if (s_idx < 0 || s_idx >= g_SkinCount[slot])
        return 0;

    set_string(3, g_SkinVModel[slot][s_idx], get_param(4));
    return 1;
}

public _native_get_active_submodel(plugin, params)
{
    new id = get_param(1);
    new weapon_id = get_param(2);
    if (id < 1 || id > MaxClients || !is_user_connected(id) || weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return -1;

    new CsTeams:team = safe_cs_get_user_team(id);
    new team_index = (team == CS_TEAM_CT) ? TEAM_CT_INDEX : TEAM_T_INDEX;
    new slot = GET_SLOT(team_index, weapon_id);
    new s_idx = g_iPlayerSkin[id][slot];
    if (s_idx < 0 || s_idx >= g_SkinCount[slot])
        return -1;

    return g_SkinSubmodel[slot][s_idx];
}
