#if defined _mod_skin_models_included
    #endinput
#endif
#define _mod_skin_models_included

// ==============================================================================
// VIEWMODEL & WEAPON DEPLOYMENT LOGIC
// ==============================================================================

public Arsenal_SkinModels_DeployPost(iWpn)
{
    if (!get_pcvar_num(pCvar_Enable) || !pev_valid(iWpn))
        return HAM_IGNORED;

    new id = get_pdata_cbase(iWpn, 41, 4);
    if (id < 1 || id > MaxClients || !is_user_alive(id))
        return HAM_IGNORED;

    new weapon_id = cs_get_weapon_id(iWpn);
    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
    {
        weapon_id = cs_get_user_weapon(id);
    }

    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return HAM_IGNORED;

    ApplyViewModel(id, weapon_id, iWpn, true);
    return HAM_IGNORED;
}

public Arsenal_SkinModels_CurWeapon(id)
{
    if (!get_pcvar_num(pCvar_Enable) || id < 1 || id > MaxClients || !is_user_alive(id))
        return;

    if (read_data(1) != 1)
        return;

    new weapon_id = read_data(2);
    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return;

    ApplyViewModel(id, weapon_id, 0, false);
}

public Event_ResetHUD(id)
{
    if (id >= 1 && id <= MaxClients && is_user_connected(id))
    {
        remove_task(TASK_RESET_HUD + id);
        set_task(0.1, "Task_ResetHUD", TASK_RESET_HUD + id);
    }
}

public OnPlayerSpawn_Post(id)
{
    if (id >= 1 && id <= MaxClients && is_user_alive(id))
    {
        remove_task(TASK_RESET_HUD + id);
        set_task(0.1, "Task_ResetHUD", TASK_RESET_HUD + id);
    }
    return HAM_IGNORED;
}

public Task_ResetHUD(taskid)
{
    new id = taskid - TASK_RESET_HUD;
    if (id < 1 || id > MaxClients || !is_user_alive(id))
        return;

    if (is_user_bot(id))
    {
        AssignBotModernLoadouts(id);
        if (safe_cs_get_user_team(id) == CS_TEAM_CT)
        {
            AssignBotM4Loadout(id);
        }
    }

    new weapon_id = cs_get_user_weapon(id);
    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return;

    ApplyViewModel(id, weapon_id, 0, true);
}

public Event_RoundStart_Skin()
{
    remove_task(TASK_INV_SYNC_BOTS);
    set_task(0.2, "Task_RoundStartSyncBots", TASK_INV_SYNC_BOTS);
}

public Task_RoundStartSyncBots()
{
    SyncAllCTBots();
}

public Event_TeamInfo()
{
    new id = read_data(1);
    if (id >= 1 && id <= MaxClients && is_user_alive(id))
    {
        remove_task(TASK_TEAM_CHANGE + id);
        set_task(0.1, "Task_TeamChange", TASK_TEAM_CHANGE + id);
    }
}

public Task_TeamChange(taskid)
{
    new id = taskid - TASK_TEAM_CHANGE;
    if (id < 1 || id > MaxClients || !is_user_alive(id))
        return;

    if (is_user_bot(id))
    {
        AssignBotModernLoadouts(id);
        if (safe_cs_get_user_team(id) == CS_TEAM_CT)
        {
            AssignBotM4Loadout(id);
        }
    }

    new weapon_id = cs_get_user_weapon(id);
    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return;

    ApplyViewModel(id, weapon_id, 0, true);
}


stock ApplyViewModel(id, weapon_id, iWpn, bool:force)
{
    new CsTeams:team = safe_cs_get_user_team(id);
    if (team != CS_TEAM_T && team != CS_TEAM_CT)
        return;

    new team_index = (team == CS_TEAM_CT) ? TEAM_CT_INDEX : TEAM_T_INDEX;
    new slot = GET_SLOT(team_index, weapon_id);
    new s_idx = g_iPlayerSkin[id][slot];

    if (!pev_valid(iWpn) && weapon_id > CSW_NONE && weapon_id <= CSW_LAST_WEAPON && g_WeaponEntityNames[weapon_id][0])
    {
        iWpn = fm_find_weapon_ent_by_owner(id, g_WeaponEntityNames[weapon_id]);
    }

    if (weapon_id != CSW_M4A1 && pev_valid(iWpn))
    {
        new iSaved = pev(iWpn, pev_iuser2);
        if (iSaved > 0 && (iSaved - 1) < g_SkinCount[slot])
        {
            s_idx = iSaved - 1;
        }
        else
        {
            set_pev(iWpn, pev_iuser2, s_idx + 1);
        }
    }

    if (s_idx < 0 || s_idx >= g_SkinCount[slot])
    {
        s_idx = 0;
        g_iPlayerSkin[id][slot] = 0;
    }

    if (g_SkinCount[slot] == 0)
        return;

    if (!g_SkinVModel[slot][s_idx][0])
        return;

    // Viewmodel v_
    if (force)
    {
        set_pev(id, pev_viewmodel2, g_SkinVModel[slot][s_idx]);
    }
    else
    {
        new current_v[128];
        pev(id, pev_viewmodel2, current_v, charsmax(current_v));
        if (!equal(current_v, g_SkinVModel[slot][s_idx]))
        {
            set_pev(id, pev_viewmodel2, g_SkinVModel[slot][s_idx]);
        }
    }

    // Apply Submodel (Bodygroup) if configured
    if (g_SkinSubmodel[slot][s_idx] >= 0)
    {
        set_pev(id, pev_body, g_SkinSubmodel[slot][s_idx]);
        if (pev_valid(iWpn))
        {
            set_pdata_float(iWpn, 38, get_gametime() + 0.001, 4);
        }
    }

    // Playermodel p_ (if defined)
    if (g_SkinPModel[slot][s_idx][0])
    {
        set_pev(id, pev_weaponmodel2, g_SkinPModel[slot][s_idx]);
    }

    // Special M4 mechanics (Both CT & T)
    if (weapon_id == CSW_M4A1 && (team == CS_TEAM_CT || team == CS_TEAM_T))
    {
        if (!pev_valid(iWpn))
        {
            iWpn = fm_find_weapon_ent_by_owner(id, "weapon_m4a1");
        }

        if (pev_valid(iWpn))
        {
            new iVariant = pev(iWpn, pev_iuser1);
            if (iVariant == 0)
            {
                iVariant = g_SkinIsM4A4[slot][s_idx] ? 1 : 2;
                set_pev(iWpn, pev_iuser1, iVariant);
            }

            if (iVariant == 1) // M4A4
            {
                new ct_slot = GET_SLOT(TEAM_CT_INDEX, CSW_M4A1);
                new target_idx = (team == CS_TEAM_CT && g_SkinIsM4A4[ct_slot][s_idx]) ? s_idx : GetCatalogM4Index(true);
                if (target_idx < 0 || target_idx >= g_SkinCount[ct_slot])
                    target_idx = 0;

                if (g_SkinVModel[ct_slot][target_idx][0])
                    set_pev(id, pev_viewmodel2, g_SkinVModel[ct_slot][target_idx]);
                if (g_SkinPModel[ct_slot][target_idx][0])
                    set_pev(id, pev_weaponmodel2, g_SkinPModel[ct_slot][target_idx]);

                if (cs_get_weapon_silen(iWpn))
                {
                    cs_set_weapon_silen(iWpn, 0, 0);
                }
            }
            else // M4A1-S (iVariant == 2)
            {
                new target_idx = (!g_SkinIsM4A4[slot][s_idx]) ? s_idx : GetCatalogM4Index(false);
                if (target_idx < 0 || target_idx >= g_SkinCount[slot])
                    target_idx = 0;

                if (g_SkinVModel[slot][target_idx][0])
                    set_pev(id, pev_viewmodel2, g_SkinVModel[slot][target_idx]);
                if (g_SkinPModel[slot][target_idx][0])
                    set_pev(id, pev_weaponmodel2, g_SkinPModel[slot][target_idx]);

                if (get_pcvar_num(pCvar_AutoSilenceM4A1) && !cs_get_weapon_silen(iWpn))
                {
                    cs_set_weapon_silen(iWpn, 1, 0);
                }
            }
        }
    }
}

stock fm_find_weapon_ent_by_owner(id, const classname[])
{
    if (id >= 1 && id <= MaxClients && is_user_connected(id))
    {
        new iActive = get_pdata_cbase(id, 373, 5);
        if (pev_valid(iActive))
        {
            new szActiveClass[32];
            pev(iActive, pev_classname, szActiveClass, charsmax(szActiveClass));
            if (equal(szActiveClass, classname))
                return iActive;
        }
    }

    new ent = -1;
    while ((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", classname)) > 0)
    {
        if (pev(ent, pev_owner) == id || get_pdata_cbase(ent, 41, 4) == id)
            return ent;
    }
    return 0;
}

