#if defined _arsenal_loadout_included
    #endinput
#endif
#define _arsenal_loadout_included

// ==============================================================================
// PLUGIN PRECACHE & CONFIG INGESTION
// ==============================================================================

stock bool:ModelFileExists(const path[])
{
    if (!path[0])
        return false;

    if (file_exists(path, true) || file_exists(path, false))
        return true;

    new cstrike_path[160];
    formatex(cstrike_path, charsmax(cstrike_path), "cstrike/%s", path);
    if (file_exists(cstrike_path, false))
        return true;

    return false;
}

public Inv_Catalog_Precache()
{
    // 1. Initialize lists to 0
    for (new s = 0; s < TOTAL_SLOTS; s++)
    {
        g_SkinCount[s] = 0;
        for (new k = 0; k < MAX_SKINS_PER_WEAPON; k++)
        {
            g_SkinSubmodel[s][k] = -1;
        }
    }

    // 2. Parse configs/skin_inventory.ini
    new config_file[128];
    get_configsdir(config_file, charsmax(config_file));
    format(config_file, charsmax(config_file), "%s/skin_inventory.ini", config_file);

    if (file_exists(config_file))
    {
        LoadCatalogConfig(config_file);
    }
    else
    {
        server_print("[CS2 Loadout] Warning: %s not found. Using fallback discovery.", config_file);
    }

    // 3. Fallback discovery for any weapon without configured skins
    LoadFallbackSkins();

    // 3.5. Parse configs/active_loadout.ini for default equipped skins
    new active_config[128];
    get_configsdir(active_config, charsmax(active_config));
    format(active_config, charsmax(active_config), "%s/active_loadout.ini", active_config);

    if (file_exists(active_config))
    {
        LoadActiveLoadoutConfig(active_config);
    }
    else
    {
        for (new s = 0; s < TOTAL_SLOTS; s++)
        {
            g_ActiveLoadoutSkin[s] = 0;
        }
        for (new w = 0; w <= CSW_LAST_WEAPON; w++)
        {
            g_szActiveLoadoutCT[w][0] = '^0';
            g_szActiveLoadoutT[w][0] = '^0';
        }
    }

    // 4. Precache all registered models
    new precached_v = 0, precached_p = 0, precached_w = 0, precached_snd = 0;
    for (new slot = 0; slot < TOTAL_SLOTS; slot++)
    {
        for (new s = 0; s < g_SkinCount[slot]; s++)
        {
            if (g_SkinVModel[slot][s][0])
            {
                if (ModelFileExists(g_SkinVModel[slot][s]))
                {
                    new m_idx = precache_model(g_SkinVModel[slot][s]);
                    if (m_idx > 0)
                    {
                        precached_v++;
                    }
                    else
                    {
                        g_SkinVModel[slot][s][0] = '^0';
                    }
                }
                else
                {
                    g_SkinVModel[slot][s][0] = '^0';
                }
            }

            if (g_SkinPModel[slot][s][0])
            {
                if (ModelFileExists(g_SkinPModel[slot][s]))
                {
                    new m_idx = precache_model(g_SkinPModel[slot][s]);
                    if (m_idx > 0)
                    {
                        precached_p++;
                    }
                    else
                    {
                        g_SkinPModel[slot][s][0] = '^0';
                    }
                }
                else
                {
                    g_SkinPModel[slot][s][0] = '^0';
                }
            }

            if (g_SkinWModel[slot][s][0])
            {
                if (ModelFileExists(g_SkinWModel[slot][s]))
                {
                    new m_idx = precache_model(g_SkinWModel[slot][s]);
                    if (m_idx > 0)
                    {
                        precached_w++;
                    }
                    else
                    {
                        g_SkinWModel[slot][s][0] = '^0';
                    }
                }
                else
                {
                    g_SkinWModel[slot][s][0] = '^0';
                }
            }

            // Precache custom gunfire sounds
            if (g_SkinFireSoundCount[slot][s] > 0)
            {
                if (g_SkinFireSound1[slot][s][0])
                {
                    new snd_check[140];
                    formatex(snd_check, charsmax(snd_check), "sound/%s", g_SkinFireSound1[slot][s]);
                    if (file_exists(snd_check) || file_exists(snd_check, true) || file_exists(g_SkinFireSound1[slot][s]) || file_exists(g_SkinFireSound1[slot][s], true))
                    {
                        precache_sound(g_SkinFireSound1[slot][s]);
                        precached_snd++;
                    }
                }
                if (g_SkinFireSound2[slot][s][0])
                {
                    new snd_check[140];
                    formatex(snd_check, charsmax(snd_check), "sound/%s", g_SkinFireSound2[slot][s]);
                    if (file_exists(snd_check) || file_exists(snd_check, true) || file_exists(g_SkinFireSound2[slot][s]) || file_exists(g_SkinFireSound2[slot][s], true))
                    {
                        precache_sound(g_SkinFireSound2[slot][s]);
                        precached_snd++;
                    }
                }
            }
        }
    }

    // 5. Precache weapon fire events
    register_forward(FM_PrecacheEvent, "OnPrecacheEvent_Post", 1);
    for (new w = 1; w <= CSW_LAST_WEAPON; w++)
    {
        if (g_WeaponEventNames[w][0] && ModelFileExists(g_WeaponEventNames[w]))
        {
            g_iWeaponEventId[w] = precache_event(1, g_WeaponEventNames[w]);
        }
    }

    server_print("[CS2 Loadout] Catalog loaded: %d viewmodels, %d playermodels, %d worldmodels, %d gunfire sounds precached.",
        precached_v, precached_p, precached_w, precached_snd);
}

public OnPrecacheEvent_Post(type, const name[])
{
    for (new w = 1; w <= CSW_LAST_WEAPON; w++)
    {
        if (g_WeaponEventNames[w][0] && equal(g_WeaponEventNames[w], name))
        {
            g_iWeaponEventId[w] = get_orig_retval();
            break;
        }
    }
}

stock LoadCatalogConfig(const filepath[])
{
    new fp = fopen(filepath, "rt");
    if (!fp) return;

    new line[256];
    new current_team = -1; // -1 = both, 0 = CT, 1 = T
    new current_weapon_id = CSW_NONE;

    while (!feof(fp))
    {
        fgets(fp, line, charsmax(line));
        trim(line);

        if (!line[0] || line[0] == ';' || line[0] == '#' || (line[0] == '/' && line[1] == '/'))
            continue;

        // Section header e.g. [CT_KNIVES], [T_KNIVES], [CT_M4A1], [T_AK47]
        if (line[0] == '[' && line[strlen(line) - 1] == ']')
        {
            new section[64];
            copy(section, strlen(line) - 2, line[1]);
            trim(section);

            current_team = -1;
            if (containi(section, "CT_") == 0)
            {
                current_team = TEAM_CT_INDEX;
            }
            else if (containi(section, "T_") == 0)
            {
                current_team = TEAM_T_INDEX;
            }

            current_weapon_id = GetWeaponIdFromSection(section);
            continue;
        }

        if (current_weapon_id == CSW_NONE)
            continue;

        // Parse line: "Display Name" "v_model" ["p_model" "w_model" "submodel" "snd1" "snd2"]
        new name[32], v_mdl[128], p_mdl[128], w_mdl[128], p5[128], p6[128], p7[128];
        parse(line, name, charsmax(name), v_mdl, charsmax(v_mdl), p_mdl, charsmax(p_mdl), w_mdl, charsmax(w_mdl), p5, charsmax(p5), p6, charsmax(p6), p7, charsmax(p7));

        if (!name[0] || !v_mdl[0])
            continue;

        if (!ModelFileExists(v_mdl))
            continue;

        new submodel = -1;
        new snd1[128], snd2[128];
        if (p5[0] && (isdigit(p5[0]) || (p5[0] == '-' && isdigit(p5[1]))))
        {
            submodel = str_to_num(p5);
            copy(snd1, charsmax(snd1), p6);
            copy(snd2, charsmax(snd2), p7);
        }
        else
        {
            submodel = -1;
            copy(snd1, charsmax(snd1), p5);
            copy(snd2, charsmax(snd2), p6);
        }

        // Add to team(s)
        if (current_team == -1)
        {
            AddSkinToCatalog(TEAM_CT_INDEX, current_weapon_id, name, v_mdl, p_mdl, w_mdl, submodel, snd1, snd2);
            AddSkinToCatalog(TEAM_T_INDEX, current_weapon_id, name, v_mdl, p_mdl, w_mdl, submodel, snd1, snd2);
        }
        else
        {
            AddSkinToCatalog(current_team, current_weapon_id, name, v_mdl, p_mdl, w_mdl, submodel, snd1, snd2);
        }
    }

    fclose(fp);
}

stock bool:IsM4A4CatalogEntry(const name[], const v_mdl[], const p_mdl[], const w_mdl[], const snd1[], const snd2[])
{
    if (containi(v_mdl, "m4a4") != -1)
        return true;

    if (containi(v_mdl, "m4a1") != -1)
        return false;

    if (containi(name, "m4a4") != -1 ||
        containi(p_mdl, "m4a4") != -1 ||
        containi(w_mdl, "m4a4") != -1 ||
        containi(snd1, "m4a4") != -1 ||
        containi(snd2, "m4a4") != -1)
    {
        return true;
    }

    return false;
}

stock ExtractPathBasename(const path[], out[], maxlen)
{
    new last_slash = -1;
    for (new i = 0; path[i]; i++)
    {
        if (path[i] == '/' || path[i] == '\')
            last_slash = i;
    }
    copy(out, maxlen, path[last_slash + 1]);
}

stock AddSkinToCatalog(team, weapon_id, const name[], const v_mdl[], const p_mdl[], const w_mdl[], submodel = -1, const snd1[] = "", const snd2[] = "")
{
    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return;

    new slot = GET_SLOT(team, weapon_id);
    new idx = g_SkinCount[slot];
    if (idx >= MAX_SKINS_PER_WEAPON)
        return;

    // Check duplicate v_model + submodel
    for (new i = 0; i < idx; i++)
    {
        if (equal(g_SkinVModel[slot][i], v_mdl) && g_SkinSubmodel[slot][i] == submodel)
            return;
    }

    copy(g_SkinName[slot][idx], charsmax(g_SkinName[][]), name);
    copy(g_SkinVModel[slot][idx], charsmax(g_SkinVModel[][]), v_mdl);
    g_SkinSubmodel[slot][idx] = submodel;

    if (p_mdl[0] && ModelFileExists(p_mdl))
        copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), p_mdl);
    else
        g_SkinPModel[slot][idx][0] = '^0';

    if (w_mdl[0] && ModelFileExists(w_mdl))
        copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), w_mdl);
    else
        g_SkinWModel[slot][idx][0] = '^0';

    g_SkinFireSoundCount[slot][idx] = 0;
    g_SkinFireSound1[slot][idx][0] = '^0';
    g_SkinFireSound2[slot][idx][0] = '^0';

    new snd_check[140];
    if (snd1[0])
    {
        formatex(snd_check, charsmax(snd_check), "sound/%s", snd1);
        if (file_exists(snd1) || file_exists(snd1, true) || file_exists(snd_check) || file_exists(snd_check, true))
        {
            copy(g_SkinFireSound1[slot][idx], charsmax(g_SkinFireSound1[][]), snd1);
            g_SkinFireSoundCount[slot][idx]++;
        }
    }

    if (snd2[0])
    {
        formatex(snd_check, charsmax(snd_check), "sound/%s", snd2);
        if (file_exists(snd2) || file_exists(snd2, true) || file_exists(snd_check) || file_exists(snd_check, true))
        {
            copy(g_SkinFireSound2[slot][idx], charsmax(g_SkinFireSound2[][]), snd2);
            g_SkinFireSoundCount[slot][idx]++;
        }
    }

    // Modern Weapon Auto-Detection & Fallback Population
    // 1. M4A4
    if (weapon_id == CSW_M4A1)
    {
        if (IsM4A4CatalogEntry(name, v_mdl, p_mdl, w_mdl, snd1, snd2))
        {
            g_SkinIsM4A4[slot][idx] = true;

            // Auto-fill fallback p_ and w_ models for M4A4 if omitted
            if (!g_SkinPModel[slot][idx][0])
            {
                if (file_exists("models/pw_models/p_m4a4.mdl") || file_exists("models/pw_models/p_m4a4.mdl", true))
                    copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/pw_models/p_m4a4.mdl");
                else if (file_exists("models/csgo_skins/ct/p_m4a4.mdl") || file_exists("models/csgo_skins/ct/p_m4a4.mdl", true))
                    copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/csgo_skins/ct/p_m4a4.mdl");
            }
            if (!g_SkinWModel[slot][idx][0])
            {
                if (file_exists("models/pw_models/w_m4a4.mdl") || file_exists("models/pw_models/w_m4a4.mdl", true))
                    copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), "models/pw_models/w_m4a4.mdl");
                else if (file_exists("models/csgo_skins/ct/w_m4a4.mdl") || file_exists("models/csgo_skins/ct/w_m4a4.mdl", true))
                    copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), "models/csgo_skins/ct/w_m4a4.mdl");
            }
            if (g_SkinFireSoundCount[slot][idx] == 0)
            {
                if (file_exists("sound/weapons/csgo/m4a4/m4a4-1.wav") || file_exists("sound/weapons/csgo/m4a4/m4a4-1.wav", true))
                {
                    copy(g_SkinFireSound1[slot][idx], charsmax(g_SkinFireSound1[][]), "weapons/csgo/m4a4/m4a4-1.wav");
                    g_SkinFireSoundCount[slot][idx]++;
                }
                if (file_exists("sound/weapons/csgo/m4a4/m4a4-2.wav") || file_exists("sound/weapons/csgo/m4a4/m4a4-2.wav", true))
                {
                    copy(g_SkinFireSound2[slot][idx], charsmax(g_SkinFireSound2[][]), "weapons/csgo/m4a4/m4a4-2.wav");
                    g_SkinFireSoundCount[slot][idx]++;
                }
            }
        }
        else
        {
            g_SkinIsM4A4[slot][idx] = false;
            if (!g_SkinPModel[slot][idx][0])
            {
                if (file_exists("models/pw_models/p_m4a1.mdl") || file_exists("models/pw_models/p_m4a1.mdl", true))
                    copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/pw_models/p_m4a1.mdl");
                else if (file_exists("models/custom_skins/ct/p_m4a1.mdl") || file_exists("models/custom_skins/ct/p_m4a1.mdl", true))
                    copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/custom_skins/ct/p_m4a1.mdl");
                else if (file_exists("models/p_m4a1.mdl") || file_exists("models/p_m4a1.mdl", true))
                    copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/p_m4a1.mdl");
            }
            if (!g_SkinWModel[slot][idx][0])
            {
                if (file_exists("models/pw_models/w_m4a1.mdl") || file_exists("models/pw_models/w_m4a1.mdl", true))
                    copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), "models/pw_models/w_m4a1.mdl");
                else if (file_exists("models/w_m4a1.mdl") || file_exists("models/w_m4a1.mdl", true))
                    copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), "models/w_m4a1.mdl");
            }
        }
    }
    // 2. Tec-9 (P228 Slot)
    else if (weapon_id == CSW_P228 && (containi(name, "tec9") != -1 || containi(name, "tec-9") != -1 || containi(v_mdl, "tec9") != -1 || containi(v_mdl, "tec-9") != -1))
    {
        if (!g_SkinPModel[slot][idx][0] && (file_exists("models/pw_models/p_tec9.mdl") || file_exists("models/pw_models/p_tec9.mdl", true)))
            copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/pw_models/p_tec9.mdl");
        if (!g_SkinWModel[slot][idx][0] && (file_exists("models/pw_models/w_tec9.mdl") || file_exists("models/pw_models/w_tec9.mdl", true)))
            copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), "models/pw_models/w_tec9.mdl");
        if (g_SkinFireSoundCount[slot][idx] == 0)
        {
            copy(g_SkinFireSound1[slot][idx], charsmax(g_SkinFireSound1[][]), "weapons/csgo/tec9/tec9-1.wav");
            g_SkinFireSoundCount[slot][idx] = 1;
        }
    }
    // 3. CZ75-Auto (P228 Slot, with legacy TMP/MAC-10 compatibility)
    else if ((weapon_id == CSW_P228 || weapon_id == CSW_TMP || weapon_id == CSW_MAC10) && (containi(name, "cz75") != -1 || containi(name, "cz-75") != -1 || containi(v_mdl, "cz75") != -1 || containi(v_mdl, "cz-75") != -1))
    {
        if (!g_SkinPModel[slot][idx][0] && (file_exists("models/pw_models/p_cz75.mdl") || file_exists("models/pw_models/p_cz75.mdl", true)))
            copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/pw_models/p_cz75.mdl");
        if (!g_SkinWModel[slot][idx][0] && (file_exists("models/pw_models/w_cz75.mdl") || file_exists("models/pw_models/w_cz75.mdl", true)))
            copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), "models/pw_models/w_cz75.mdl");
        if (g_SkinFireSoundCount[slot][idx] == 0)
        {
            copy(g_SkinFireSound1[slot][idx], charsmax(g_SkinFireSound1[][]), "weapons/csgo/cz75a/cz75-1.wav");
            copy(g_SkinFireSound2[slot][idx], charsmax(g_SkinFireSound2[][]), "weapons/csgo/cz75a/cz75-2.wav");
            g_SkinFireSoundCount[slot][idx] = 2;
        }
    }
    // 4. MP7 (MP5 Slot)
    else if (weapon_id == CSW_MP5NAVY && (containi(name, "mp7") != -1 || containi(v_mdl, "mp7") != -1))
    {
        if (!g_SkinPModel[slot][idx][0] && (file_exists("models/pw_models/p_mp7.mdl") || file_exists("models/pw_models/p_mp7.mdl", true)))
            copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/pw_models/p_mp7.mdl");
        if (!g_SkinWModel[slot][idx][0] && (file_exists("models/pw_models/w_mp7.mdl") || file_exists("models/pw_models/w_mp7.mdl", true)))
            copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), "models/pw_models/w_mp7.mdl");
        if (g_SkinFireSoundCount[slot][idx] == 0)
        {
            copy(g_SkinFireSound1[slot][idx], charsmax(g_SkinFireSound1[][]), "weapons/csgo/mp7/mp7-1.wav");
            copy(g_SkinFireSound2[slot][idx], charsmax(g_SkinFireSound2[][]), "weapons/csgo/mp7/mp7-2.wav");
            g_SkinFireSoundCount[slot][idx] = 2;
        }
    }
    // 5. PP-Bizon (P90 Slot)
    else if (weapon_id == CSW_P90 && (containi(name, "bizon") != -1 || containi(v_mdl, "bizon") != -1))
    {
        if (!g_SkinPModel[slot][idx][0] && (file_exists("models/pw_models/p_bizon.mdl") || file_exists("models/pw_models/p_bizon.mdl", true)))
            copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/pw_models/p_bizon.mdl");
        if (!g_SkinWModel[slot][idx][0] && (file_exists("models/pw_models/w_bizon.mdl") || file_exists("models/pw_models/w_bizon.mdl", true)))
            copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), "models/pw_models/w_bizon.mdl");
        if (g_SkinFireSoundCount[slot][idx] == 0)
        {
            copy(g_SkinFireSound1[slot][idx], charsmax(g_SkinFireSound1[][]), "weapons/csgo/bizon/bizon-1.wav");
            g_SkinFireSoundCount[slot][idx] = 1;
        }
    }
    // 6. MAG-7 (XM1014 Slot)
    else if (weapon_id == CSW_XM1014 && (containi(name, "mag7") != -1 || containi(name, "mag-7") != -1 || containi(v_mdl, "mag7") != -1 || containi(v_mdl, "mag-7") != -1))
    {
        if (!g_SkinPModel[slot][idx][0] && (file_exists("models/pw_models/p_mag7.mdl") || file_exists("models/pw_models/p_mag7.mdl", true)))
            copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/pw_models/p_mag7.mdl");
        if (!g_SkinWModel[slot][idx][0] && (file_exists("models/pw_models/w_mag7.mdl") || file_exists("models/pw_models/w_mag7.mdl", true)))
            copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), "models/pw_models/w_mag7.mdl");
        if (g_SkinFireSoundCount[slot][idx] == 0)
        {
            copy(g_SkinFireSound1[slot][idx], charsmax(g_SkinFireSound1[][]), "weapons/csgo/mag7/mag7-1.wav");
            g_SkinFireSoundCount[slot][idx] = 1;
        }
    }
    // 7. Sawed-Off (M3 Slot)
    else if (weapon_id == CSW_M3 && (containi(name, "sawed") != -1 || containi(v_mdl, "sawedoff") != -1))
    {
        if (!g_SkinPModel[slot][idx][0] && (file_exists("models/pw_models/p_sawedoff.mdl") || file_exists("models/pw_models/p_sawedoff.mdl", true)))
            copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/pw_models/p_sawedoff.mdl");
        if (!g_SkinWModel[slot][idx][0] && (file_exists("models/pw_models/w_sawedoff.mdl") || file_exists("models/pw_models/w_sawedoff.mdl", true)))
            copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), "models/pw_models/w_sawedoff.mdl");
        if (g_SkinFireSoundCount[slot][idx] == 0)
        {
            copy(g_SkinFireSound1[slot][idx], charsmax(g_SkinFireSound1[][]), "weapons/csgo/sawedoff/sawedoff-1.wav");
            g_SkinFireSoundCount[slot][idx] = 1;
        }
    }
    // 8. Negev (M249 Slot)
    else if (weapon_id == CSW_M249 && (containi(name, "negev") != -1 || containi(v_mdl, "negev") != -1))
    {
        if (!g_SkinPModel[slot][idx][0] && (file_exists("models/pw_models/p_negev.mdl") || file_exists("models/pw_models/p_negev.mdl", true)))
            copy(g_SkinPModel[slot][idx], charsmax(g_SkinPModel[][]), "models/pw_models/p_negev.mdl");
        if (!g_SkinWModel[slot][idx][0] && (file_exists("models/pw_models/w_negev.mdl") || file_exists("models/pw_models/w_negev.mdl", true)))
            copy(g_SkinWModel[slot][idx], charsmax(g_SkinWModel[][]), "models/pw_models/w_negev.mdl");
        if (g_SkinFireSoundCount[slot][idx] == 0)
        {
            copy(g_SkinFireSound1[slot][idx], charsmax(g_SkinFireSound1[][]), "weapons/csgo/negev/negev-1.wav");
            copy(g_SkinFireSound2[slot][idx], charsmax(g_SkinFireSound2[][]), "weapons/csgo/negev/negev-2.wav");
            g_SkinFireSoundCount[slot][idx] = 2;
        }
    }

    g_SkinCount[slot]++;
}

stock GetWeaponIdFromSection(const section[])
{
    if (containi(section, "KNIFE") != -1 || containi(section, "KNIVES") != -1) return CSW_KNIFE;
    if (containi(section, "M4A1") != -1 || containi(section, "M4A4") != -1 || containi(section, "M4") != -1) return CSW_M4A1;
    if (containi(section, "AK47") != -1 || containi(section, "AK") != -1) return CSW_AK47;
    if (containi(section, "AWP") != -1) return CSW_AWP;
    if (containi(section, "SCOUT") != -1) return CSW_SCOUT;
    if (containi(section, "DEAGLE") != -1) return CSW_DEAGLE;
    if (containi(section, "USP") != -1) return CSW_USP;
    if (containi(section, "GLOCK18") != -1 || containi(section, "GLOCK") != -1) return CSW_GLOCK18;
    if (containi(section, "FAMAS") != -1) return CSW_FAMAS;
    if (containi(section, "GALIL") != -1) return CSW_GALIL;
    if (containi(section, "MP5NAVY") != -1 || containi(section, "MP5") != -1) return CSW_MP5NAVY;
    if (containi(section, "UMP45") != -1 || containi(section, "UMP") != -1) return CSW_UMP45;
    if (containi(section, "XM1014") != -1) return CSW_XM1014;
    if (containi(section, "M3") != -1) return CSW_M3;
    if (containi(section, "AUG") != -1) return CSW_AUG;
    if (containi(section, "SG552") != -1) return CSW_SG552;
    if (containi(section, "SG550") != -1) return CSW_SG550;
    if (containi(section, "G3SG1") != -1) return CSW_G3SG1;
    if (containi(section, "P90") != -1) return CSW_P90;
    if (containi(section, "P228") != -1) return CSW_P228;
    if (containi(section, "FIVESEVEN") != -1) return CSW_FIVESEVEN;
    if (containi(section, "ELITE") != -1) return CSW_ELITE;
    if (containi(section, "MAC10") != -1) return CSW_MAC10;
    if (containi(section, "TMP") != -1) return CSW_TMP;
    if (containi(section, "M249") != -1) return CSW_M249;

    // Direct Modern Section Names
    if (containi(section, "TEC9") != -1 || containi(section, "TEC") != -1) return CSW_P228;
    if (containi(section, "MP7") != -1) return CSW_MP5NAVY;
    if (containi(section, "BIZON") != -1) return CSW_P90;
    if (containi(section, "MAG7") != -1) return CSW_XM1014;
    if (containi(section, "SAWEDOFF") != -1 || containi(section, "SAWED") != -1) return CSW_M3;
    if (containi(section, "NEGEV") != -1) return CSW_M249;
    if (containi(section, "CZ75") != -1 || containi(section, "CZ") != -1)
    {
        if (containi(section, "CT_") == 0) return CSW_TMP;
        if (containi(section, "T_") == 0) return CSW_MAC10;
        return CSW_TMP;
    }

    return CSW_NONE;
}

stock LoadFallbackSkins()
{
    // Ensure every weapon has at least 1 default skin populated from models/custom_skins/ or models/csgo_skins/
    for (new w = 1; w <= CSW_LAST_WEAPON; w++)
    {
        if (!g_WeaponModelNames[w][0])
            continue;

        new ct_slot = GET_SLOT(TEAM_CT_INDEX, w);
        if (g_SkinCount[ct_slot] == 0)
        {
            new path_custom[128], path_csgo[128];
            formatex(path_custom, charsmax(path_custom), "models/custom_skins/ct/%s", g_WeaponModelNames[w]);
            formatex(path_csgo, charsmax(path_csgo), "models/csgo_skins/ct/%s", g_WeaponModelNames[w]);

            if (ModelFileExists(path_custom))
            {
                AddSkinToCatalog(TEAM_CT_INDEX, w, "Default Skin", path_custom, "", "");
            }
            else if (ModelFileExists(path_csgo))
            {
                AddSkinToCatalog(TEAM_CT_INDEX, w, "Default Skin", path_csgo, "", "");
            }
        }

        new t_slot = GET_SLOT(TEAM_T_INDEX, w);
        if (g_SkinCount[t_slot] == 0)
        {
            new path_custom[128], path_csgo[128];
            formatex(path_custom, charsmax(path_custom), "models/custom_skins/t/%s", g_WeaponModelNames[w]);
            formatex(path_csgo, charsmax(path_csgo), "models/csgo_skins/t/%s", g_WeaponModelNames[w]);

            if (ModelFileExists(path_custom))
            {
                AddSkinToCatalog(TEAM_T_INDEX, w, "Default Skin", path_custom, "", "");
            }
            else if (ModelFileExists(path_csgo))
            {
                AddSkinToCatalog(TEAM_T_INDEX, w, "Default Skin", path_csgo, "", "");
            }
        }
    }
}

// ==============================================================================
// ACTIVE LOADOUT INGESTION (DESKTOP COMPANION SYNC)
// ==============================================================================

stock LoadActiveLoadoutConfig(const filepath[])
{
    for (new s = 0; s < TOTAL_SLOTS; s++)
    {
        g_ActiveLoadoutSkin[s] = 0;
    }
    for (new w = 0; w <= CSW_LAST_WEAPON; w++)
    {
        g_szActiveLoadoutCT[w][0] = '^0';
        g_szActiveLoadoutT[w][0] = '^0';
    }

    new fp = fopen(filepath, "rt");
    if (!fp)
    {
        server_print("[CS2 Loadout] Error: Could not open %s", filepath);
        return;
    }

    new line[256];
    new current_team = -1;

    while (!feof(fp))
    {
        fgets(fp, line, charsmax(line));
        trim(line);

        if (!line[0] || line[0] == ';' || line[0] == '#' || (line[0] == '/' && line[1] == '/'))
            continue;

        // Section header [CT_ACTIVE], [T_ACTIVE], [CT], [T]
        if (line[0] == '[' && line[strlen(line) - 1] == ']')
        {
            new section[64];
            copy(section, strlen(line) - 2, line[1]);
            trim(section);

            current_team = -1;
            if (equali(section, "CT_ACTIVE") || equali(section, "CT") || containi(section, "CT_") == 0)
            {
                current_team = TEAM_CT_INDEX;
            }
            else if (equali(section, "T_ACTIVE") || equali(section, "T") || containi(section, "T_") == 0)
            {
                current_team = TEAM_T_INDEX;
            }
            continue;
        }

        if (current_team == -1)
            continue;

        // Parse KEY = "VALUE" or KEY "VALUE"
        new key[64], val[128];
        new eq_pos = contain(line, "=");
        if (eq_pos != -1)
        {
            copy(key, eq_pos, line);
            copy(val, charsmax(val), line[eq_pos + 1]);
        }
        else
        {
            parse(line, key, charsmax(key), val, charsmax(val));
        }

        trim(key);
        trim(val);

        // Strip enclosing quotes from val
        if (val[0] == '^"')
        {
            copy(val, charsmax(val), val[1]);
            new len = strlen(val);
            if (len > 0 && val[len - 1] == '^"')
            {
                val[len - 1] = '^0';
            }
            trim(val);
        }

        if (!key[0] || !val[0])
            continue;

        // Normalize slashes in val
        for (new i = 0; val[i]; i++)
        {
            if (val[i] == '\') val[i] = '/';
        }

        new weapon_id = GetWeaponIdFromSection(key);
        if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
            continue;

        // Single-pass sync for Weapon Variants lookup (populated even if slot has 0 catalog skins)
        if (current_team == TEAM_CT_INDEX)
        {
            copy(g_szActiveLoadoutCT[weapon_id], charsmax(g_szActiveLoadoutCT[]), val);
        }
        else if (current_team == TEAM_T_INDEX)
        {
            copy(g_szActiveLoadoutT[weapon_id], charsmax(g_szActiveLoadoutT[]), val);
        }

        new slot = GET_SLOT(current_team, weapon_id);
        if (g_SkinCount[slot] <= 0)
            continue;

        // Find skin matching by tiered priority: (1) exact path/name, (2) exact model filename, (3) controlled substring
        new matched_idx = -1;
        new val_base[64];
        ExtractPathBasename(val, val_base, charsmax(val_base));

        // Pass 1: Exact path or exact display name match
        for (new s = 0; s < g_SkinCount[slot]; s++)
        {
            new model_norm[128];
            copy(model_norm, charsmax(model_norm), g_SkinVModel[slot][s]);
            for (new i = 0; model_norm[i]; i++)
            {
                if (model_norm[i] == '\') model_norm[i] = '/';
            }

            if (equali(val, model_norm) || equali(val, g_SkinName[slot][s]))
            {
                matched_idx = s;
                break;
            }
        }

        // Pass 2: Exact model filename match (e.g. "v_m4a4.mdl")
        if (matched_idx == -1 && val_base[0])
        {
            for (new s = 0; s < g_SkinCount[slot]; s++)
            {
                new model_base[64];
                ExtractPathBasename(g_SkinVModel[slot][s], model_base, charsmax(model_base));
                if (model_base[0] && equali(val_base, model_base))
                {
                    matched_idx = s;
                    break;
                }
            }
        }

        // Pass 3: Controlled substring fallback (minimum 4 chars to avoid ambiguous short hits)
        if (matched_idx == -1 && strlen(val) >= 4)
        {
            for (new s = 0; s < g_SkinCount[slot]; s++)
            {
                new model_norm[128];
                copy(model_norm, charsmax(model_norm), g_SkinVModel[slot][s]);
                for (new i = 0; model_norm[i]; i++)
                {
                    if (model_norm[i] == '\') model_norm[i] = '/';
                }

                if (containi(model_norm, val) != -1 || containi(g_SkinName[slot][s], val) != -1)
                {
                    matched_idx = s;
                    break;
                }
            }
        }

        if (matched_idx != -1)
        {
            g_ActiveLoadoutSkin[slot] = matched_idx;
            server_print("[CS2 Loadout] Active loadout set: %s [%s] -> %s (index %d)",
                (current_team == TEAM_CT_INDEX) ? "CT" : "T", key, g_SkinName[slot][matched_idx], matched_idx);
        }
    }

    fclose(fp);
}

