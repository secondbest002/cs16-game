#if defined _arsenal_vault_included
    #endinput
#endif
#define _arsenal_vault_included

// ==============================================================================
// CLIENT CONNECTION & PERSISTENT NVAULT STORAGE
// ==============================================================================

public client_putinserver(id)
{
    g_iBotM4Override[id] = -1;
    #if defined ENABLE_ARSENAL_VARIANTS
    g_iPendingBuyWeapon[id] = 0;
    g_flPendingBuyTime[id]  = 0.0;
    #endif

    for (new slot = 0; slot < TOTAL_SLOTS; slot++)
    {
        g_iPlayerSkin[id][slot] = g_ActiveLoadoutSkin[slot];
    }

    if (!is_user_bot(id))
    {
        LoadPlayerLoadout(id);
    }
    else
    {
        AssignBotModernLoadouts(id);
    }
}

public client_authorized(id)
{
    if (id >= 1 && id <= MaxClients && !is_user_bot(id) && is_user_connected(id))
    {
        LoadPlayerLoadout(id);
    }
}

public client_disconnected(id)
{
    g_iBotM4Override[id] = -1;
    #if defined ENABLE_ARSENAL_VARIANTS
    g_iPendingBuyWeapon[id] = 0;
    g_flPendingBuyTime[id]  = 0.0;
    #endif
    remove_task(TASK_RESET_HUD + id);
    remove_task(TASK_TEAM_CHANGE + id);
}

stock GetPlayerKey(id, output[], maxlen)
{
    if (is_user_bot(id))
    {
        formatex(output, maxlen, "BOT_%d", id);
        return;
    }

    new auth[36];
    get_user_authid(id, auth, charsmax(auth));

    if (!auth[0] || equal(auth, "STEAM_ID_LAN") || equal(auth, "VALVE_ID_LAN") || equal(auth, "STEAM_ID_PENDING"))
    {
        get_user_name(id, output, maxlen);
    }
    else
    {
        copy(output, maxlen, auth);
    }

    // Sanitize key prefix for nvault (strip spaces, quotes, delimiters)
    for (new i = 0; output[i]; i++)
    {
        if (output[i] <= 32 || output[i] == 34 || output[i] == 39 || output[i] == ';' || output[i] == '#' || output[i] == ':')
            output[i] = '_';
    }
}

stock LoadPlayerLoadout(id)
{
    // 1. Initialize player skins from pre-game active loadout
    for (new slot = 0; slot < TOTAL_SLOTS; slot++)
    {
        if (g_ActiveLoadoutSkin[slot] >= 0 && g_ActiveLoadoutSkin[slot] < g_SkinCount[slot])
        {
            g_iPlayerSkin[id][slot] = g_ActiveLoadoutSkin[slot];
        }
        else
        {
            g_iPlayerSkin[id][slot] = 0;
        }
    }

    // 2. Load persistent choices from nvault if player is not a bot
    if (g_hVault != INVALID_HANDLE && !is_user_bot(id))
    {
        new key_prefix[64];
        GetPlayerKey(id, key_prefix, charsmax(key_prefix));

        for (new t = 0; t < 2; t++)
        {
            new team_str[4];
            copy(team_str, charsmax(team_str), (t == TEAM_CT_INDEX) ? "ct" : "t");

            for (new w = 1; w <= CSW_LAST_WEAPON; w++)
            {
                new slot = GET_SLOT(t, w);
                if (g_SkinCount[slot] <= 0)
                    continue;

                new vkey[96], saved_val[64];
                formatex(vkey, charsmax(vkey), "%s_%s_%d", key_prefix, team_str, w);
                if (nvault_get(g_hVault, vkey, saved_val, charsmax(saved_val)))
                {
                    for (new s = 0; s < g_SkinCount[slot]; s++)
                    {
                        if (equali(saved_val, g_SkinName[slot][s]))
                        {
                            g_iPlayerSkin[id][slot] = s;
                            break;
                        }
                    }
                }
            }
        }
    }
}

stock SavePlayerWeaponSkin(id, team, weapon_id)
{
    if (g_hVault == INVALID_HANDLE || is_user_bot(id))
        return;

    new key_prefix[64];
    GetPlayerKey(id, key_prefix, charsmax(key_prefix));

    new team_str[4];
    copy(team_str, charsmax(team_str), (team == TEAM_CT_INDEX) ? "ct" : "t");

    new slot = GET_SLOT(team, weapon_id);
    new s_idx = g_iPlayerSkin[id][slot];
    new vkey[96];
    formatex(vkey, charsmax(vkey), "%s_%s_%d", key_prefix, team_str, weapon_id);

    if (s_idx >= 0 && s_idx < g_SkinCount[slot])
    {
        nvault_set(g_hVault, vkey, g_SkinName[slot][s_idx]);
    }
}
