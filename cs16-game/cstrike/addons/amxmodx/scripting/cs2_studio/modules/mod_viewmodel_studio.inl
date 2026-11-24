#if defined _mod_viewmodel_studio_included
    #endinput
#endif
#define _mod_viewmodel_studio_included

// =============================================================================
// SUBSYSTEM B: PER-WEAPON VIEWMODEL STUDIO (Live Inspector + Dual Menu)
// Migrated verbatim from cs2_viewmodel_studio.sma (nVault: cs2_viewmodel)
// =============================================================================

public Vm_Module_Init()
{
    // Chat & Console Commands
    register_clcmd("say /vm", "Vm_Cmd_OpenStudioMenu");
    register_clcmd("say_team /vm", "Vm_Cmd_OpenStudioMenu");
    register_clcmd("say .vm", "Vm_Cmd_OpenStudioMenu");
    register_clcmd("say /viewmodel", "Vm_Cmd_OpenStudioMenu");
    register_clcmd("say_team /viewmodel", "Vm_Cmd_OpenStudioMenu");
    register_clcmd("say /fov", "Vm_Cmd_OpenStudioMenu");
    register_clcmd("say_team /fov", "Vm_Cmd_OpenStudioMenu");
    register_clcmd("say .fov", "Vm_Cmd_OpenStudioMenu");
    register_clcmd("vm_menu", "Vm_Cmd_OpenStudioMenu");

    // Event Hooks (Zero PreThink Overhead)
    register_event("CurWeapon", "Event_CurWeapon", "be", "1=1");
    RegisterHam(Ham_Spawn, "player", "OnPlayerSpawn_Post", 1);

    g_hVmVault = nvault_open(STUDIO_VM_VAULT_NAME);
    if (g_hVmVault == INVALID_HANDLE)
    {
        log_amx("[CS2-VM] Warning: Failed to open %s.nvault!", STUDIO_VM_VAULT_NAME);
    }
}

public Vm_Module_End()
{    if (g_hVmVault != INVALID_HANDLE)
    {
        for (new i = 1; i <= MaxClients; i++)
        {
            if (is_user_connected(i) && !is_user_bot(i) && !is_user_hltv(i))
            {
                SaveAllDirtyProfiles(i);
            }
        }
        nvault_close(g_hVmVault);
        g_hVmVault = INVALID_HANDLE;
    }
}

public Vm_Module_PutInServer(id)
{
    Studio_BuildPlayerKey(id, g_szVmVaultPrefix[id], charsmax(g_szVmVaultPrefix[]));
    ResetAllProfilesToDefault(id, false);

    g_iVmLastProfile[id]     = 0;
    g_bClientHasNativeVM[id] = false;
    g_bVmAdjustNegative[id]  = false;
    g_bVmFineStep[id]        = false;
    g_iVmMenuState[id]       = 0;

    LoadAllProfilesFromVault(id);
}

public Vm_Module_Disconnect(id)
{
    SaveAllDirtyProfiles(id);
}
public Vm_CvarQuery_NativeVMCallback(id, const szCvar[], const szValue[], const szParam[])
{
    if (!is_user_connected(id))
        return;

    if (!equal(szValue, "Bad CVAR request"))
    {
        g_bClientHasNativeVM[id] = true;
        if (is_user_alive(id))
        {
            g_iVmLastProfile[id] = 0;
            SyncActiveWeaponNow(id);
        }
    }
}

// =============================================================================
// WEAPON SWITCH & SPAWN SYNCHRONIZATION
// =============================================================================
public OnPlayerSpawn_Post(id)
{
    if (!is_user_alive(id) || is_user_bot(id) || is_user_hltv(id))
        return HAM_IGNORED;

    g_iVmLastProfile[id] = 0;
    remove_task(id + TASK_VM_SPAWN);
    set_task(0.15, "Task_SpawnSync", id + TASK_VM_SPAWN);
    return HAM_IGNORED;
}

public Task_SpawnSync(taskid)
{
    new id = taskid - TASK_VM_SPAWN;
    if (!is_user_alive(id) || is_user_bot(id) || is_user_hltv(id))
        return;

    SyncActiveWeaponNow(id);
}

public Event_CurWeapon(id)
{
    if (!is_user_alive(id) || is_user_bot(id) || is_user_hltv(id))
        return;

    new iWeaponId = read_data(2);
    if (iWeaponId <= 0 || iWeaponId > CSW_LAST_WEAPON)
        return;

    // Defer slightly for Knife & M4A1 so cs2_skin_inventory finishes applying v_model & pev_iuser1
    if (iWeaponId == CSW_KNIFE || iWeaponId == CSW_M4A1)
    {
        remove_task(id + TASK_VM_DEFER);
        set_task(0.04, "Task_DeferredWeaponSync", id + TASK_VM_DEFER);
        return;
    }

    remove_task(id + TASK_VM_DEFER);
    SyncProfileIfChanged(id, iWeaponId);
}

public Task_DeferredWeaponSync(taskid)
{
    new id = taskid - TASK_VM_DEFER;
    if (!is_user_alive(id) || is_user_bot(id) || is_user_hltv(id))
        return;

    SyncActiveWeaponNow(id);
}

stock SyncActiveWeaponNow(id)
{
    new iProfile = ResolveActiveProfile(id);
    if (IsValidProfile(iProfile))
    {
        SyncProfileIfChanged(id, iProfile);
    }
}

stock SyncProfileIfChanged(id, iProfile)
{
    if (!IsValidProfile(iProfile))
        return;

    if (g_iVmLastProfile[id] != iProfile)
    {
        g_iVmLastProfile[id] = iProfile;
        ApplyProfileToClient(id, iProfile);

        // If user switched weapons (e.g. via Q) while Studio menu is open, live-refresh menu to new weapon
        if (g_iVmMenuState[id] == 1)
        {
            ShowMainStudioMenu(id);
        }
        else if (g_iVmMenuState[id] == 2)
        {
            ShowToolsSubmenu(id);
        }
    }
}

stock ApplyProfileToClient(id, iProfile)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id) || !IsValidProfile(iProfile))
        return;

    client_cmd(id, "viewmodel_fov %.1f;viewmodel_offset_x %.2f;viewmodel_offset_y %.2f;viewmodel_offset_z %.2f;cl_bob %.3f;cl_weaponlag %.2f",
        g_flVmProfile[id][iProfile][VM_FOV],
        g_flVmProfile[id][iProfile][VM_X],
        g_flVmProfile[id][iProfile][VM_Y],
        g_flVmProfile[id][iProfile][VM_Z],
        g_flVmProfile[id][iProfile][VM_BOB],
        g_flVmProfile[id][iProfile][VM_LAG]);
}

// =============================================================================
// WEAPON / M4A4 / KNIFE TYPE RESOLUTION
// =============================================================================
stock ResolveActiveProfile(id)
{
    if (!is_user_alive(id))
        return 0;

    new iWeaponId = get_user_weapon(id);
    if (iWeaponId <= 0 || iWeaponId > CSW_LAST_WEAPON)
        return 0;

    if (iWeaponId == CSW_M4A1)
    {
        new iEnt = get_pdata_cbase(id, m_pActiveItem, XO_PLAYER);
        if (pev_valid(iEnt))
        {
            new iVariant = pev(iEnt, pev_iuser1);
            if (iVariant == 1)
                return PROFILE_M4A4;
            if (iVariant == 2)
                return CSW_M4A1;
        }

        // Fallback viewmodel path inspection
        new szVm[128];
        pev(id, pev_viewmodel2, szVm, charsmax(szVm));
        if (szVm[0] && containi(szVm, "m4a4") != -1)
            return PROFILE_M4A4;

        return CSW_M4A1;
    }

    if (iWeaponId == CSW_KNIFE)
    {
        new szVm[128];
        pev(id, pev_viewmodel2, szVm, charsmax(szVm));
        return DetectKnifeProfileFromModel(szVm);
    }

    return iWeaponId;
}

stock DetectKnifeProfileFromModel(const szModel[])
{
    new kType = cs2_detect_knife_type(szModel);
    if (kType <= CS2_KNIFE_DEFAULT || kType >= CS2_KNIFE_MAX)
        return CSW_KNIFE;

    return PROFILE_M4A4 + kType;
}

stock bool:IsValidProfile(iProfile)
{
    return (iProfile > 0 && iProfile < MAX_VM_PROFILES && g_szVmProfileKeys[iProfile][0] != 0);
}

// =============================================================================
// LIVE IN-GAME STUDIO MENUS
// =============================================================================
public Vm_Cmd_OpenStudioMenu(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return PLUGIN_HANDLED;

    if (!is_user_alive(id))
    {
        client_print_color(id, print_team_default, "^4[Viewmodel Studio]^1 Kamu harus hidup untuk live inspect viewmodel senjata.");
        return PLUGIN_HANDLED;
    }

    if (!g_bClientHasNativeVM[id])
    {
        // Re-probe once in case client just loaded
        query_client_cvar(id, "viewmodel_fov", "Vm_CvarQuery_NativeVMCallback");
    }

    new iProfile = ResolveActiveProfile(id);
    if (!IsValidProfile(iProfile))
        return PLUGIN_HANDLED;

    g_iVmLastProfile[id] = iProfile;
    ApplyProfileToClient(id, iProfile);
    ShowMainStudioMenu(id);
    return PLUGIN_HANDLED;
}

stock ShowMainStudioMenu(id)
{
    if (!is_user_alive(id))
    {
        g_iVmMenuState[id] = 0;
        return;
    }

    new iProfile = ResolveActiveProfile(id);
    if (!IsValidProfile(iProfile))
    {
        g_iVmMenuState[id] = 0;
        return;
    }

    g_iVmMenuState[id] = 1;

    new szTitle[192], szItem[96];
    formatex(szTitle, charsmax(szTitle), "\y[Viewmodel Studio]\w %s%s^n\dFOV: %.1f | X: %.2f | Y: %.2f | Z: %.2f",
        g_szVmProfileNames[iProfile],
        g_bVmProfileCustom[id][iProfile] ? " \y[Custom]" : " \d[Default]",
        g_flVmProfile[id][iProfile][VM_FOV],
        g_flVmProfile[id][iProfile][VM_X],
        g_flVmProfile[id][iProfile][VM_Y],
        g_flVmProfile[id][iProfile][VM_Z]);

    new hMenu = menu_create(szTitle, "Handler_MainStudioMenu");

    formatex(szItem, charsmax(szItem), "Direction: %s",
        g_bVmAdjustNegative[id] ? "\r[ - Decrease ]" : "\y[ + Increase ]");
    menu_additem(hMenu, szItem, "1");

    formatex(szItem, charsmax(szItem), "FOV (Reach): \y%.1f \d(50.0 - 75.0)",
        g_flVmProfile[id][iProfile][VM_FOV]);
    menu_additem(hMenu, szItem, "2");

    formatex(szItem, charsmax(szItem), "Offset X (Left/Right): \y%.2f",
        g_flVmProfile[id][iProfile][VM_X]);
    menu_additem(hMenu, szItem, "3");

    formatex(szItem, charsmax(szItem), "Offset Y (Fwd/Back): \y%.2f",
        g_flVmProfile[id][iProfile][VM_Y]);
    menu_additem(hMenu, szItem, "4");

    formatex(szItem, charsmax(szItem), "Offset Z (Up/Down): \y%.2f",
        g_flVmProfile[id][iProfile][VM_Z]);
    menu_additem(hMenu, szItem, "5");

    formatex(szItem, charsmax(szItem), "Step Precision: \y%s",
        g_bVmFineStep[id] ? "[Fine: 0.10 / FOV 0.5]" : "[Normal: 0.50 / FOV 2.0]");
    menu_additem(hMenu, szItem, "6");

    menu_additem(hMenu, "\yMore -> \wBob / Lag, Presets & Tools", "7");

    menu_setprop(hMenu, MPROP_EXIT, MEXIT_ALL);
    menu_setprop(hMenu, MPROP_EXITNAME, "Save & Exit");
    menu_display(id, hMenu, 0);
}

public Handler_MainStudioMenu(id, hMenu, item)
{
    if (item == MENU_EXIT || !is_user_alive(id))
    {
        menu_destroy(hMenu);
        g_iVmMenuState[id] = 0;
        SaveAllDirtyProfiles(id);
        return PLUGIN_HANDLED;
    }

    new szInfo[8], szDummy[2], iAccess, iCallback;
    menu_item_getinfo(hMenu, item, iAccess, szInfo, charsmax(szInfo), szDummy, charsmax(szDummy), iCallback);
    menu_destroy(hMenu);

    new iChoice = str_to_num(szInfo);
    new iProfile = ResolveActiveProfile(id);
    if (!IsValidProfile(iProfile))
    {
        g_iVmMenuState[id] = 0;
        return PLUGIN_HANDLED;
    }

    new Float:flDir     = g_bVmAdjustNegative[id] ? -1.0 : 1.0;
    new Float:flFovStep = g_bVmFineStep[id] ? 0.5 : 2.0;
    new Float:flOfsStep = g_bVmFineStep[id] ? 0.10 : 0.50;

    switch (iChoice)
    {
        case 1:
        {
            g_bVmAdjustNegative[id] = !g_bVmAdjustNegative[id];
        }
        case 2:
        {
            g_flVmProfile[id][iProfile][VM_FOV] = ClampFloat(
                g_flVmProfile[id][iProfile][VM_FOV] + (flDir * flFovStep), VM_FOV_MIN, VM_FOV_MAX);
            MarkProfileModified(id, iProfile);
        }
        case 3:
        {
            g_flVmProfile[id][iProfile][VM_X] = ClampFloat(
                g_flVmProfile[id][iProfile][VM_X] + (flDir * flOfsStep), VM_OFS_MIN, VM_OFS_MAX);
            MarkProfileModified(id, iProfile);
        }
        case 4:
        {
            g_flVmProfile[id][iProfile][VM_Y] = ClampFloat(
                g_flVmProfile[id][iProfile][VM_Y] + (flDir * flOfsStep), VM_OFS_MIN, VM_OFS_MAX);
            MarkProfileModified(id, iProfile);
        }
        case 5:
        {
            g_flVmProfile[id][iProfile][VM_Z] = ClampFloat(
                g_flVmProfile[id][iProfile][VM_Z] + (flDir * flOfsStep), VM_OFS_MIN, VM_OFS_MAX);
            MarkProfileModified(id, iProfile);
        }
        case 6:
        {
            g_bVmFineStep[id] = !g_bVmFineStep[id];
        }
        case 7:
        {
            ShowToolsSubmenu(id);
            return PLUGIN_HANDLED;
        }
    }

    ShowMainStudioMenu(id);
    return PLUGIN_HANDLED;
}

stock ShowToolsSubmenu(id)
{
    if (!is_user_alive(id))
    {
        g_iVmMenuState[id] = 0;
        return;
    }

    new iProfile = ResolveActiveProfile(id);
    if (!IsValidProfile(iProfile))
    {
        g_iVmMenuState[id] = 0;
        return;
    }

    g_iVmMenuState[id] = 2;

    new szTitle[128], szItem[96];
    formatex(szTitle, charsmax(szTitle), "\y[Viewmodel Studio -> Tools]\w %s", g_szVmProfileNames[iProfile]);

    new hMenu = menu_create(szTitle, "Handler_ToolsSubmenu");

    formatex(szItem, charsmax(szItem), "Weapon Bob (cl_bob): \y%.3f", g_flVmProfile[id][iProfile][VM_BOB]);
    menu_additem(hMenu, szItem, "1");

    formatex(szItem, charsmax(szItem), "Weapon Lag (cl_weaponlag): \y%.2f", g_flVmProfile[id][iProfile][VM_LAG]);
    menu_additem(hMenu, szItem, "2");

    menu_additem(hMenu, "Preset: \yCS2 Modern \d(FOV 68, X 1.5, Y 1.0, Z -1.0)", "3");
    menu_additem(hMenu, "Preset: \yDesktop Classic \d(FOV 60, X 0.0, Y 0.0, Z 0.0)", "4");
    menu_additem(hMenu, "Preset: \yTight Competitive \d(FOV 65, X -1.0, Y 1.5, Z -1.2)", "5");
    menu_additem(hMenu, "\rReset This Weapon to Default", "6");
    menu_additem(hMenu, "\yCopy This Weapon Settings to ALL Weapons", "7");

    menu_setprop(hMenu, MPROP_EXIT, MEXIT_ALL);
    menu_setprop(hMenu, MPROP_EXITNAME, "Back to Main Studio");
    menu_display(id, hMenu, 0);
}

public Handler_ToolsSubmenu(id, hMenu, item)
{
    if (item == MENU_EXIT)
    {
        menu_destroy(hMenu);
        if (is_user_alive(id))
        {
            ShowMainStudioMenu(id);
        }
        else
        {
            g_iVmMenuState[id] = 0;
            SaveAllDirtyProfiles(id);
        }
        return PLUGIN_HANDLED;
    }

    if (!is_user_alive(id))
    {
        menu_destroy(hMenu);
        g_iVmMenuState[id] = 0;
        SaveAllDirtyProfiles(id);
        return PLUGIN_HANDLED;
    }

    new szInfo[8], szDummy[2], iAccess, iCallback;
    menu_item_getinfo(hMenu, item, iAccess, szInfo, charsmax(szInfo), szDummy, charsmax(szDummy), iCallback);
    menu_destroy(hMenu);

    new iChoice = str_to_num(szInfo);
    new iProfile = ResolveActiveProfile(id);
    if (!IsValidProfile(iProfile))
    {
        g_iVmMenuState[id] = 0;
        return PLUGIN_HANDLED;
    }

    switch (iChoice)
    {
        case 1: // Cycle cl_bob
        {
            g_flVmProfile[id][iProfile][VM_BOB] = NextCycleValue(
                g_flVmProfile[id][iProfile][VM_BOB], g_flBobSteps, sizeof(g_flBobSteps));
            MarkProfileModified(id, iProfile);
        }
        case 2: // Cycle cl_weaponlag
        {
            g_flVmProfile[id][iProfile][VM_LAG] = NextCycleValue(
                g_flVmProfile[id][iProfile][VM_LAG], g_flLagSteps, sizeof(g_flLagSteps));
            MarkProfileModified(id, iProfile);
        }
        case 3: // Preset: CS2 Modern
        {
            ApplyPresetToProfile(id, iProfile, 68.0, 1.50, 1.00, -1.00, 0.015, 0.50);
            client_print_color(id, print_team_default, "^4[Viewmodel Studio]^1 Preset ^3CS2 Modern^1 diterapkan ke ^4%s^1.", g_szVmProfileNames[iProfile]);
        }
        case 4: // Preset: Desktop Classic
        {
            ApplyPresetToProfile(id, iProfile, VM_FOV_DEFAULT, VM_OFS_DEFAULT, VM_OFS_DEFAULT, VM_OFS_DEFAULT, VM_BOB_DEFAULT, VM_LAG_DEFAULT);
            client_print_color(id, print_team_default, "^4[Viewmodel Studio]^1 Preset ^3Desktop Classic^1 diterapkan ke ^4%s^1.", g_szVmProfileNames[iProfile]);
        }
        case 5: // Preset: Tight Competitive
        {
            ApplyPresetToProfile(id, iProfile, 65.0, -1.00, 1.50, -1.20, 0.005, 0.00);
            client_print_color(id, print_team_default, "^4[Viewmodel Studio]^1 Preset ^3Tight Competitive^1 diterapkan ke ^4%s^1.", g_szVmProfileNames[iProfile]);
        }
        case 6: // Reset Active Weapon
        {
            SetSingleProfileDefault(id, iProfile);
            DeleteProfileFromVault(id, iProfile);
            ApplyProfileToClient(id, iProfile);
            client_print_color(id, print_team_default, "^4[Viewmodel Studio]^1 Setting ^3%s^1 di-reset ke default.", g_szVmProfileNames[iProfile]);
        }
        case 7: // Copy Active to All Weapons
        {
            CopyProfileToAll(id, iProfile);
            client_print_color(id, print_team_default, "^4[Viewmodel Studio]^1 Setting ^3%s^1 berhasil disalin ke ^4semua senjata^1!", g_szVmProfileNames[iProfile]);
        }
    }

    ShowToolsSubmenu(id);
    return PLUGIN_HANDLED;
}

// =============================================================================
// PROFILE MUTATION & MATH HELPERS
// =============================================================================
stock MarkProfileModified(id, iProfile)
{
    g_bVmProfileCustom[id][iProfile] = true;
    g_bVmProfileDirty[id][iProfile]  = true;
    g_iVmLastProfile[id]             = iProfile;
    ApplyProfileToClient(id, iProfile);
    SaveSingleProfileToVault(id, iProfile);
}

stock ApplyPresetToProfile(id, iProfile, Float:fov, Float:x, Float:y, Float:z, Float:bob, Float:lag)
{
    g_flVmProfile[id][iProfile][VM_FOV] = fov;
    g_flVmProfile[id][iProfile][VM_X]   = x;
    g_flVmProfile[id][iProfile][VM_Y]   = y;
    g_flVmProfile[id][iProfile][VM_Z]   = z;
    g_flVmProfile[id][iProfile][VM_BOB] = bob;
    g_flVmProfile[id][iProfile][VM_LAG] = lag;
    MarkProfileModified(id, iProfile);
}

stock CopyProfileToAll(id, iSrcProfile)
{
    for (new p = 1; p < MAX_VM_PROFILES; p++)
    {
        if (!IsValidProfile(p))
            continue;

        g_flVmProfile[id][p][VM_FOV] = g_flVmProfile[id][iSrcProfile][VM_FOV];
        g_flVmProfile[id][p][VM_X]   = g_flVmProfile[id][iSrcProfile][VM_X];
        g_flVmProfile[id][p][VM_Y]   = g_flVmProfile[id][iSrcProfile][VM_Y];
        g_flVmProfile[id][p][VM_Z]   = g_flVmProfile[id][iSrcProfile][VM_Z];
        g_flVmProfile[id][p][VM_BOB] = g_flVmProfile[id][iSrcProfile][VM_BOB];
        g_flVmProfile[id][p][VM_LAG] = g_flVmProfile[id][iSrcProfile][VM_LAG];
        g_bVmProfileCustom[id][p] = true;
        g_bVmProfileDirty[id][p]  = true;
    }

    SaveAllDirtyProfiles(id);
    ApplyProfileToClient(id, iSrcProfile);
}

stock ResetAllProfilesToDefault(id, bool:bClearVault)
{
    for (new p = 1; p < MAX_VM_PROFILES; p++)
    {
        if (!IsValidProfile(p))
            continue;

        SetSingleProfileDefault(id, p);
        if (bClearVault)
        {
            DeleteProfileFromVault(id, p);
        }
    }
}

stock SetSingleProfileDefault(id, iProfile)
{
    g_flVmProfile[id][iProfile][VM_FOV] = VM_FOV_DEFAULT;
    g_flVmProfile[id][iProfile][VM_X]   = VM_OFS_DEFAULT;
    g_flVmProfile[id][iProfile][VM_Y]   = VM_OFS_DEFAULT;
    g_flVmProfile[id][iProfile][VM_Z]   = VM_OFS_DEFAULT;
    g_flVmProfile[id][iProfile][VM_BOB] = VM_BOB_DEFAULT;
    g_flVmProfile[id][iProfile][VM_LAG] = VM_LAG_DEFAULT;
    g_bVmProfileCustom[id][iProfile]    = false;
    g_bVmProfileDirty[id][iProfile]     = false;
}

stock Float:ClampFloat(Float:flVal, Float:flMin, Float:flMax)
{
    if (flVal < flMin) return flMin;
    if (flVal > flMax) return flMax;

    // Eliminate -0.00 floating point artifact
    if (flVal > -0.0005 && flVal < 0.0005)
        return 0.0;

    return flVal;
}

stock Float:NextCycleValue(Float:flCurrent, const Float:flSteps[], iSize)
{
    for (new i = 0; i < iSize; i++)
    {
        if (flCurrent < flSteps[i] - 0.0005)
            return flSteps[i];
    }
    return flSteps[0];
}

// =============================================================================
// NVAULT SERIALIZATION & DESERIALIZATION
// =============================================================================

stock SaveSingleProfileToVault(id, iProfile)
{
    if (g_hVmVault == INVALID_HANDLE || !IsValidProfile(iProfile) || !g_szVmVaultPrefix[id][0])
        return;

    new szKey[64], szVal[96];
    formatex(szKey, charsmax(szKey), "%s#%s", g_szVmVaultPrefix[id], g_szVmProfileKeys[iProfile]);
    formatex(szVal, charsmax(szVal), "%.1f %.2f %.2f %.2f %.3f %.2f",
        g_flVmProfile[id][iProfile][VM_FOV],
        g_flVmProfile[id][iProfile][VM_X],
        g_flVmProfile[id][iProfile][VM_Y],
        g_flVmProfile[id][iProfile][VM_Z],
        g_flVmProfile[id][iProfile][VM_BOB],
        g_flVmProfile[id][iProfile][VM_LAG]);

    nvault_set(g_hVmVault, szKey, szVal);
    g_bVmProfileDirty[id][iProfile] = false;
}

stock DeleteProfileFromVault(id, iProfile)
{
    if (g_hVmVault == INVALID_HANDLE || !IsValidProfile(iProfile) || !g_szVmVaultPrefix[id][0])
        return;

    new szKey[64];
    formatex(szKey, charsmax(szKey), "%s#%s", g_szVmVaultPrefix[id], g_szVmProfileKeys[iProfile]);
    nvault_remove(g_hVmVault, szKey);
}

stock SaveAllDirtyProfiles(id)
{
    if (g_hVmVault == INVALID_HANDLE || !g_szVmVaultPrefix[id][0])
        return;

    for (new p = 1; p < MAX_VM_PROFILES; p++)
    {
        if (g_bVmProfileDirty[id][p] && IsValidProfile(p))
        {
            SaveSingleProfileToVault(id, p);
        }
    }
}

stock LoadAllProfilesFromVault(id)
{
    if (g_hVmVault == INVALID_HANDLE || !g_szVmVaultPrefix[id][0])
        return;

    new szKey[64], szVal[96], iTs;
    new szTok[VM_DATA][16];

    for (new p = 1; p < MAX_VM_PROFILES; p++)
    {
        if (!IsValidProfile(p))
            continue;

        formatex(szKey, charsmax(szKey), "%s#%s", g_szVmVaultPrefix[id], g_szVmProfileKeys[p]);
        if (!nvault_lookup(g_hVmVault, szKey, szVal, charsmax(szVal), iTs))
            continue;

        if (explode_string(szVal, " ", szTok, sizeof(szTok), charsmax(szTok[])) >= VM_DATA)
        {
            g_flVmProfile[id][p][VM_FOV] = ClampFloat(str_to_float(szTok[_:VM_FOV]), VM_FOV_MIN, VM_FOV_MAX);
            g_flVmProfile[id][p][VM_X]   = ClampFloat(str_to_float(szTok[_:VM_X]),   VM_OFS_MIN, VM_OFS_MAX);
            g_flVmProfile[id][p][VM_Y]   = ClampFloat(str_to_float(szTok[_:VM_Y]),   VM_OFS_MIN, VM_OFS_MAX);
            g_flVmProfile[id][p][VM_Z]   = ClampFloat(str_to_float(szTok[_:VM_Z]),   VM_OFS_MIN, VM_OFS_MAX);
            g_flVmProfile[id][p][VM_BOB] = ClampFloat(str_to_float(szTok[_:VM_BOB]), VM_BOB_MIN, VM_BOB_MAX);
            g_flVmProfile[id][p][VM_LAG] = ClampFloat(str_to_float(szTok[_:VM_LAG]), VM_LAG_MIN, VM_LAG_MAX);
            g_bVmProfileCustom[id][p]    = true;
            g_bVmProfileDirty[id][p]     = false;
        }
    }
}
