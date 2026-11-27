#if defined _mod_crosshair_hud_included
    #endinput
#endif
#define _mod_crosshair_hud_included

// =============================================================================
// SUBSYSTEM A: CROSSHAIR, SNIPER SCOPE, MISC HUD & RADAR MANAGER
// Migrated verbatim from cs2_custom_hud.sma (nVault: cs2_crosshair)
// =============================================================================

public Hud_Module_Init()
{
    // Chat & Console Triggers for Crosshair Menu
    register_clcmd("say /crosshair", "Hud_Cmd_OpenCrosshairMenu");
    register_clcmd("say_team /crosshair", "Hud_Cmd_OpenCrosshairMenu");
    register_clcmd("say .crosshair", "Hud_Cmd_OpenCrosshairMenu");
    register_clcmd("crosshair_menu", "Hud_Cmd_OpenCrosshairMenu");

    // Chat & Console Triggers for Crosshair Save
    register_clcmd("say /savecrosshair", "Hud_Cmd_SaveCrosshair");
    register_clcmd("say_team /savecrosshair", "Hud_Cmd_SaveCrosshair");
    register_clcmd("say /crosshair_save", "Hud_Cmd_SaveCrosshair");
    register_clcmd("say .savecrosshair", "Hud_Cmd_SaveCrosshair");
    register_clcmd("crosshair_save", "Hud_Cmd_SaveCrosshair");
    register_clcmd("save_crosshair", "Hud_Cmd_SaveCrosshair");

    // Chat & Console Triggers for Sniper Scope Menu
    register_clcmd("say /scope", "Hud_Cmd_OpenSniperMenu");
    register_clcmd("say_team /scope", "Hud_Cmd_OpenSniperMenu");
    register_clcmd("say .scope", "Hud_Cmd_OpenSniperMenu");

    // Chat & Console Triggers for Misc HUD & Radar Menu
    register_clcmd("say /hud", "Hud_Cmd_OpenMiscHudMenu");
    register_clcmd("say_team /hud", "Hud_Cmd_OpenMiscHudMenu");
    register_clcmd("say .hud", "Hud_Cmd_OpenMiscHudMenu");

    g_hHudVault = nvault_open(STUDIO_HUD_VAULT_NAME);
    if (g_hHudVault == INVALID_HANDLE)
    {
        log_amx("[CS2-HUD] Warning: Gagal membuka database %s.nvault!", STUDIO_HUD_VAULT_NAME);
    }
}

public Hud_Module_End()
{    if (g_hHudVault != INVALID_HANDLE)
    {
        for (new i = 1; i <= MaxClients; i++)
        {
            if (is_user_connected(i) && !is_user_bot(i) && !is_user_hltv(i))
            {
                if (g_bHudProfileDirty[i])
                {
                    SaveProfileToVault(i);
                }
            }
        }
        nvault_close(g_hHudVault);
        g_hHudVault = INVALID_HANDLE;
    }
}

public Hud_Module_PutInServer(id)
{
    ResetToDefaults(id);
    g_bHudProfileDirty[id] = false;
    g_bIsXashCS16[id]      = false;
    g_bIsGSModern[id]      = false;
    g_iHudBatchStep[id]    = 0;

    g_bHudHasProfile[id]   = LoadProfileFromVault(id);
}

public Hud_Module_Disconnect(id)
{
    if (g_bHudProfileDirty[id])
    {
        SaveProfileToVault(id);
        g_bHudProfileDirty[id] = false;
    }
}
stock ResetToDefaults(id)
{
    g_ePlayerHud[id][HUD_CH_STYLE]          = 4;    // Classic static
    g_ePlayerHud[id][HUD_CH_SIZE]           = 4.0;  // CS2 modern size
    g_ePlayerHud[id][HUD_CH_THICKNESS]      = 1.0;  // Thickness 1.0
    g_ePlayerHud[id][HUD_CH_GAP]            = -1.0; // Gap -1
    g_ePlayerHud[id][HUD_CH_USEALPHA]       = 1;
    g_ePlayerHud[id][HUD_CH_ALPHA]          = 255;  // Opacity 100%
    g_ePlayerHud[id][HUD_CH_OUTLINE]        = 1;
    g_ePlayerHud[id][HUD_CH_OUTLINE_THICK]  = 0.5;  // Outline 0.5
    g_ePlayerHud[id][HUD_CH_DOT]            = 0;
    g_ePlayerHud[id][HUD_CH_TSHAPE]         = 0;
    g_ePlayerHud[id][HUD_CH_WPNGAP]         = 0;
    g_ePlayerHud[id][HUD_CH_FPSREC]         = 0;
    g_ePlayerHud[id][HUD_CH_COLOR_IDX]      = 0;    // Green
    g_ePlayerHud[id][HUD_CH_COLOR_R]        = 50;
    g_ePlayerHud[id][HUD_CH_COLOR_G]        = 250;
    g_ePlayerHud[id][HUD_CH_COLOR_B]        = 50;

    g_ePlayerHud[id][HUD_SNIPER_WIDTH]      = 1;
    g_ePlayerHud[id][HUD_SNIPER_STYLE]      = 1;    // CS2 Modern Scope (Vignette) by default
    g_ePlayerHud[id][HUD_SNIPER_POSTS]      = 0;    // Uniform thin hairline by default
    g_ePlayerHud[id][HUD_SNIPER_BLUR]       = 1;    // Movement inaccuracy blur
    g_ePlayerHud[id][HUD_SNIPER_BOB]        = 0;

    g_ePlayerHud[id][HUD_RADAR_SQUARE]      = 1;
    g_ePlayerHud[id][HUD_RADAR_LOCKED]      = 0;
    g_ePlayerHud[id][HUD_RADAR_SCALE]       = 1.0;
    g_ePlayerHud[id][HUD_COLOR_PRESET]      = 0;
}


public Hud_CvarQuery_XhairCallback(id, const szCvar[], const szValue[], const szParam[])
{
    if (is_user_connected(id) && !equal(szValue, "Bad CVAR request"))
    {
        g_bIsXashCS16[id] = true;
    }
}

public Hud_CvarQuery_GSModernCallback(id, const szCvar[], const szValue[], const szParam[])
{
    if (is_user_connected(id) && !equal(szValue, "Bad CVAR request"))
    {
        g_bIsGSModern[id] = true;
    }
}

// =============================================================================
// UNIVERSAL DUAL-SYNC ENGINE (Xash3D CS16Client + GoldSrc Modern + Vanilla)
// =============================================================================

stock Sync_CrosshairGeometry(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return;

    new Float:flSize = Float:g_ePlayerHud[id][HUD_CH_SIZE];
    new Float:flThick = Float:g_ePlayerHud[id][HUD_CH_THICKNESS];
    new Float:flGap = Float:g_ePlayerHud[id][HUD_CH_GAP];
    new Float:flXhairThick = (flThick <= 0.0) ? 1.0 : flThick;

    // 1. Xash3D CS16Client Vector Crosshair
    client_cmd(id, "xhair_enable 1;xhair_size %.1f;xhair_thick %.1f;xhair_gap %.1f", flSize, flXhairThick, flGap);

    // 2. GoldSrc Modern Vector Crosshair
    client_cmd(id, "cl_crosshairsize %.1f;cl_crosshairthickness %.1f;cl_crosshairgap %.1f", flSize, flThick, flGap);

    // 3. Vanilla CS 1.6 Legacy Fallback
    new szSizeVanilla[12];
    if (flSize <= 4.0) copy(szSizeVanilla, charsmax(szSizeVanilla), "small");
    else if (flSize <= 9.0) copy(szSizeVanilla, charsmax(szSizeVanilla), "medium");
    else copy(szSizeVanilla, charsmax(szSizeVanilla), "large");

    client_cmd(id, "cl_crosshair_size ^"%s^"", szSizeVanilla);
}

stock Sync_CrosshairStyle(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return;

    new iStyle = g_ePlayerHud[id][HUD_CH_STYLE];
    new iDyn = (iStyle == 1 || iStyle == 4) ? 0 : 1;

    // Xash3D CS16Client
    client_cmd(id, "xhair_dynamic_move %d;xhair_dynamic_scale %d", iDyn, iDyn);

    // GoldSrc Modern & Vanilla
    client_cmd(id, "cl_crosshairstyle %d;cl_dynamiccrosshair %d", iStyle, iDyn);
}

stock Sync_CrosshairColor(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return;

    new iR = g_ePlayerHud[id][HUD_CH_COLOR_R];
    new iG = g_ePlayerHud[id][HUD_CH_COLOR_G];
    new iB = g_ePlayerHud[id][HUD_CH_COLOR_B];
    new iAlpha = g_ePlayerHud[id][HUD_CH_ALPHA];
    new iUseAlpha = g_ePlayerHud[id][HUD_CH_USEALPHA];
    new iTranslucent = (iAlpha < 255) ? 1 : 0;
    new iColorIdx = g_ePlayerHud[id][HUD_CH_COLOR_IDX];

    // 1. Xash3D CS16Client (RGBA string controls color and opacity simultaneously)
    client_cmd(id, "xhair_color ^"%d %d %d %d^"", iR, iG, iB, iAlpha);

    // 2. GoldSrc Modern
    client_cmd(id, "cl_crosshairusealpha %d;cl_crosshairalpha %d", iUseAlpha, iAlpha);
    client_cmd(id, "cl_crosshaircolor %d;cl_crosshaircolor_r %d;cl_crosshaircolor_g %d;cl_crosshaircolor_b %d", iColorIdx, iR, iG, iB);

    // 3. Vanilla CS 1.6 Legacy
    client_cmd(id, "cl_crosshair_color ^"%d %d %d^";cl_crosshair_translucent %d", iR, iG, iB, iTranslucent);
}

stock Sync_CrosshairToggles(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return;

    new iDot = g_ePlayerHud[id][HUD_CH_DOT];
    new iTShape = g_ePlayerHud[id][HUD_CH_TSHAPE];
    new iWpnGap = g_ePlayerHud[id][HUD_CH_WPNGAP];
    new iFpsRec = g_ePlayerHud[id][HUD_CH_FPSREC];
    new iOutline = g_ePlayerHud[id][HUD_CH_OUTLINE];
    new Float:flOutlineThick = Float:g_ePlayerHud[id][HUD_CH_OUTLINE_THICK];
    new Float:flPad = (iOutline && flOutlineThick > 0.0) ? flOutlineThick : (iOutline ? 0.5 : 0.0);

    // 1. Xash3D CS16Client
    client_cmd(id, "xhair_dot %d;xhair_t %d;xhair_pad %.1f;xhair_gap_useweaponvalue %d", iDot, iTShape, flPad, iWpnGap);

    // 2. GoldSrc Modern
    client_cmd(id, "cl_crosshairdot %d;cl_crosshair_t %d", iDot, iTShape);
    client_cmd(id, "cl_crosshair_drawoutline %d;cl_crosshair_outlinethickness %.1f", iOutline, flOutlineThick);
    client_cmd(id, "cl_crosshairgap_useweaponvalue %d;cl_crosshair_recoverytime_on_fps %d", iWpnGap, iFpsRec);
}

stock Hud_Sync_SniperSettings(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return;

    client_cmd(id, "cl_crosshair_sniper_width %d;cl_crosshair_sniper_style %d;cl_crosshair_sniper_outer_posts %d",
        g_ePlayerHud[id][HUD_SNIPER_WIDTH], g_ePlayerHud[id][HUD_SNIPER_STYLE], g_ePlayerHud[id][HUD_SNIPER_POSTS]);
    client_cmd(id, "cl_crosshair_sniper_show_normal_inaccuracy %d;cl_bob_sniper %d",
        g_ePlayerHud[id][HUD_SNIPER_BLUR], g_ePlayerHud[id][HUD_SNIPER_BOB]);
    client_cmd(id, "cl_bobcycle_sniper 0.98;cl_bobup_sniper 0.5");
}

stock Hud_Sync_MiscHudSettings(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return;

    new iPreset = g_ePlayerHud[id][HUD_COLOR_PRESET];
    client_cmd(id, "hud_color ^"%s^";con_color ^"%s^"", g_szHudColorRGB[iPreset][1], g_szHudColorRGB[iPreset][1]);
    client_cmd(id, "hud_radar_square %d;hud_radar_locked %d;hud_radar_scale %.2f;cl_radartype 1",
        g_ePlayerHud[id][HUD_RADAR_SQUARE],
        g_ePlayerHud[id][HUD_RADAR_LOCKED],
        Float:g_ePlayerHud[id][HUD_RADAR_SCALE]);
}

stock Hud_Sync_AllSettings(id)
{
    remove_task(id + TASK_HUD_SYNC);
    g_iHudBatchStep[id] = 1;
    set_task(HUD_BATCH_INTERVAL, "Task_InjectCvarBatch", id + TASK_HUD_SYNC, _, _, "a", 3);
}

public Task_InjectCvarBatch(taskid)
{
    new id = taskid - TASK_HUD_SYNC;
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
    {
        remove_task(taskid);
        return;
    }

    switch (g_iHudBatchStep[id])
    {
        case 1:
        {
            Sync_CrosshairGeometry(id);
            Sync_CrosshairStyle(id);
        }
        case 2:
        {
            Sync_CrosshairColor(id);
            Sync_CrosshairToggles(id);
        }
        case 3:
        {
            Hud_Sync_SniperSettings(id);
            Hud_Sync_MiscHudSettings(id);
        }
    }
    g_iHudBatchStep[id]++;
}

// =============================================================================
// SUB-TAB 1 MENU: CROSSHAIR - BASIC (say /crosshair)
// =============================================================================
public Hud_Cmd_OpenCrosshairMenu(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return PLUGIN_HANDLED;

    new szClientMode[48];
    if (g_bIsXashCS16[id])
        copy(szClientMode, charsmax(szClientMode), "\rNative Xash3D (xhair)");
    else if (g_bIsGSModern[id])
        copy(szClientMode, charsmax(szClientMode), "\rGoldSrc Modern (cl_crosshair)");
    else
        copy(szClientMode, charsmax(szClientMode), "\yUniversal Dual-Sync");

    new szTitle[128], szItem[96];
    formatex(szTitle, charsmax(szTitle), "\y[Options -> HUD]\w Crosshair - Basic^n\dClient Mode: %s", szClientMode);

    new hMenu = menu_create(szTitle, "Handler_CrosshairMenu");

    formatex(szItem, charsmax(szItem), "Style: \y%s", g_szHudStyleNames[g_ePlayerHud[id][HUD_CH_STYLE]]);
    menu_additem(hMenu, szItem, "1");

    formatex(szItem, charsmax(szItem), "Size (0-20): \y%.1f", Float:g_ePlayerHud[id][HUD_CH_SIZE]);
    menu_additem(hMenu, szItem, "2");

    formatex(szItem, charsmax(szItem), "Thickness (0-20): \y%.1f", Float:g_ePlayerHud[id][HUD_CH_THICKNESS]);
    menu_additem(hMenu, szItem, "3");

    formatex(szItem, charsmax(szItem), "Gap (-30 to 30): \y%.1f", Float:g_ePlayerHud[id][HUD_CH_GAP]);
    menu_additem(hMenu, szItem, "4");

    formatex(szItem, charsmax(szItem), "Opacity (Alpha): \y%d%% (%d)", (g_ePlayerHud[id][HUD_CH_ALPHA] * 100) / 255, g_ePlayerHud[id][HUD_CH_ALPHA]);
    menu_additem(hMenu, szItem, "5");

    formatex(szItem, charsmax(szItem), "Outline (0-3): \y%s (%.1f px)", g_ePlayerHud[id][HUD_CH_OUTLINE] ? "ON" : "OFF", Float:g_ePlayerHud[id][HUD_CH_OUTLINE_THICK]);
    menu_additem(hMenu, szItem, "6");

    formatex(szItem, charsmax(szItem), "Color: \y%s", g_szHudColorNames[g_ePlayerHud[id][HUD_CH_COLOR_IDX]]);
    menu_additem(hMenu, szItem, "7");

    formatex(szItem, charsmax(szItem), "Center Dot: %s", g_ePlayerHud[id][HUD_CH_DOT] ? "\y[ON]" : "\d[OFF]");
    menu_additem(hMenu, szItem, "8");

    formatex(szItem, charsmax(szItem), "T-Shape (No Top Bar): %s", g_ePlayerHud[id][HUD_CH_TSHAPE] ? "\y[ON]" : "\d[OFF]");
    menu_additem(hMenu, szItem, "9");

    formatex(szItem, charsmax(szItem), "Gap Based on Weapon: %s", g_ePlayerHud[id][HUD_CH_WPNGAP] ? "\y[ON]" : "\d[OFF]");
    menu_additem(hMenu, szItem, "10");

    formatex(szItem, charsmax(szItem), "Recovery Depends on FPS: %s", g_ePlayerHud[id][HUD_CH_FPSREC] ? "\y[ON]" : "\d[OFF]");
    menu_additem(hMenu, szItem, "11");

    menu_additem(hMenu, "\ySwitch to:\w Crosshair - Sniper Rifles (/scope)", "12");
    menu_additem(hMenu, "\ySwitch to:\w Misc HUD & Radar (/hud)", "13");
    menu_additem(hMenu, "\rUse Defaults (Reset)", "14");

    menu_setprop(hMenu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, hMenu, 0);
    return PLUGIN_HANDLED;
}

public Handler_CrosshairMenu(id, hMenu, item)
{
    if (item == MENU_EXIT)
    {
        menu_destroy(hMenu);
        return PLUGIN_HANDLED;
    }

    new szInfo[8], szDummy[2], iAccess, iCallback;
    menu_item_getinfo(hMenu, item, iAccess, szInfo, charsmax(szInfo), szDummy, charsmax(szDummy), iCallback);
    menu_destroy(hMenu);

    new iChoice = str_to_num(szInfo);
    g_bHudProfileDirty[id] = true;

    switch (iChoice)
    {
        case 1: // Style
        {
            g_ePlayerHud[id][HUD_CH_STYLE] = (g_ePlayerHud[id][HUD_CH_STYLE] + 1) % sizeof(g_szHudStyleNames);
            Sync_CrosshairStyle(id);
        }
        case 2: // Size
        {
            g_ePlayerHud[id][HUD_CH_SIZE] = Float:g_ePlayerHud[id][HUD_CH_SIZE] + 1.0;
            if (Float:g_ePlayerHud[id][HUD_CH_SIZE] > 20.0) g_ePlayerHud[id][HUD_CH_SIZE] = 0.0;
            Sync_CrosshairGeometry(id);
        }
        case 3: // Thickness
        {
            g_ePlayerHud[id][HUD_CH_THICKNESS] = Float:g_ePlayerHud[id][HUD_CH_THICKNESS] + 0.5;
            if (Float:g_ePlayerHud[id][HUD_CH_THICKNESS] > 10.0) g_ePlayerHud[id][HUD_CH_THICKNESS] = 0.0;
            Sync_CrosshairGeometry(id);
        }
        case 4: // Gap
        {
            g_ePlayerHud[id][HUD_CH_GAP] = Float:g_ePlayerHud[id][HUD_CH_GAP] + 1.0;
            if (Float:g_ePlayerHud[id][HUD_CH_GAP] > 15.0) g_ePlayerHud[id][HUD_CH_GAP] = -15.0;
            Sync_CrosshairGeometry(id);
        }
        case 5: // Opacity / Alpha
        {
            g_ePlayerHud[id][HUD_CH_ALPHA] -= 51;
            if (g_ePlayerHud[id][HUD_CH_ALPHA] < 51) g_ePlayerHud[id][HUD_CH_ALPHA] = 255;
            Sync_CrosshairColor(id);
        }
        case 6: // Outline
        {
            g_ePlayerHud[id][HUD_CH_OUTLINE_THICK] = Float:g_ePlayerHud[id][HUD_CH_OUTLINE_THICK] + 0.5;
            if (Float:g_ePlayerHud[id][HUD_CH_OUTLINE_THICK] > 3.0)
            {
                g_ePlayerHud[id][HUD_CH_OUTLINE_THICK] = 0.0;
                g_ePlayerHud[id][HUD_CH_OUTLINE] = 0;
            }
            else
            {
                g_ePlayerHud[id][HUD_CH_OUTLINE] = 1;
            }
            Sync_CrosshairToggles(id);
        }
        case 7: // Color
        {
            g_ePlayerHud[id][HUD_CH_COLOR_IDX] = (g_ePlayerHud[id][HUD_CH_COLOR_IDX] + 1) % sizeof(g_szHudColorNames);

            switch (g_ePlayerHud[id][HUD_CH_COLOR_IDX])
            {
                case 0: { g_ePlayerHud[id][HUD_CH_COLOR_R] = 50;  g_ePlayerHud[id][HUD_CH_COLOR_G] = 250; g_ePlayerHud[id][HUD_CH_COLOR_B] = 50; }  // Green
                case 1: { g_ePlayerHud[id][HUD_CH_COLOR_R] = 250; g_ePlayerHud[id][HUD_CH_COLOR_G] = 50;  g_ePlayerHud[id][HUD_CH_COLOR_B] = 50; }  // Red
                case 2: { g_ePlayerHud[id][HUD_CH_COLOR_R] = 50;  g_ePlayerHud[id][HUD_CH_COLOR_G] = 50;  g_ePlayerHud[id][HUD_CH_COLOR_B] = 250; } // Blue
                case 3: { g_ePlayerHud[id][HUD_CH_COLOR_R] = 250; g_ePlayerHud[id][HUD_CH_COLOR_G] = 250; g_ePlayerHud[id][HUD_CH_COLOR_B] = 50; }  // Yellow
                case 4: { g_ePlayerHud[id][HUD_CH_COLOR_R] = 50;  g_ePlayerHud[id][HUD_CH_COLOR_G] = 250; g_ePlayerHud[id][HUD_CH_COLOR_B] = 250; } // Cyan
                case 5: { g_ePlayerHud[id][HUD_CH_COLOR_R] = 255; g_ePlayerHud[id][HUD_CH_COLOR_G] = 255; g_ePlayerHud[id][HUD_CH_COLOR_B] = 255; } // Custom RGB White
            }
            Sync_CrosshairColor(id);
        }
        case 8: // Center Dot
        {
            g_ePlayerHud[id][HUD_CH_DOT] = !g_ePlayerHud[id][HUD_CH_DOT];
            Sync_CrosshairToggles(id);
        }
        case 9: // T-Shape
        {
            g_ePlayerHud[id][HUD_CH_TSHAPE] = !g_ePlayerHud[id][HUD_CH_TSHAPE];
            Sync_CrosshairToggles(id);
        }
        case 10: // Weapon Gap
        {
            g_ePlayerHud[id][HUD_CH_WPNGAP] = !g_ePlayerHud[id][HUD_CH_WPNGAP];
            Sync_CrosshairToggles(id);
        }
        case 11: // FPS Rec
        {
            g_ePlayerHud[id][HUD_CH_FPSREC] = !g_ePlayerHud[id][HUD_CH_FPSREC];
            Sync_CrosshairToggles(id);
        }
        case 12: return Hud_Cmd_OpenSniperMenu(id);
        case 13: return Hud_Cmd_OpenMiscHudMenu(id);
        case 14:
        {
            ResetToDefaults(id);
            Hud_Sync_AllSettings(id);
        }
    }

    g_bHudHasProfile[id] = true;
    SaveProfileToVault(id);

    Hud_Cmd_OpenCrosshairMenu(id);
    return PLUGIN_HANDLED;
}

// =============================================================================
// SUB-TAB 2 MENU: CROSSHAIR - SNIPER RIFLES (say /scope)
// =============================================================================
public Hud_Cmd_OpenSniperMenu(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return PLUGIN_HANDLED;

    new szItem[96];
    new hMenu = menu_create("\y[Options -> HUD]\w Crosshair - Sniper Rifles", "Handler_SniperMenu");

    formatex(szItem, charsmax(szItem), "Scope Line Thickness (1-10): \y%d px", g_ePlayerHud[id][HUD_SNIPER_WIDTH]);
    menu_additem(hMenu, szItem, "1");

    formatex(szItem, charsmax(szItem), "Scope Style: \y%s", g_szHudSniperStyles[g_ePlayerHud[id][HUD_SNIPER_STYLE]]);
    menu_additem(hMenu, szItem, "2");

    formatex(szItem, charsmax(szItem), "Scope Outer Edges: %s", g_ePlayerHud[id][HUD_SNIPER_POSTS] ? "\y[Thick Posts]" : "\y[Uniform Thin]");
    menu_additem(hMenu, szItem, "3");

    formatex(szItem, charsmax(szItem), "Dynamic Blur on Movement: %s", g_ePlayerHud[id][HUD_SNIPER_BLUR] ? "\y[ON]" : "\d[OFF]");
    menu_additem(hMenu, szItem, "4");

    formatex(szItem, charsmax(szItem), "Sniper Bobcycle (Scope Sway): %s", g_ePlayerHud[id][HUD_SNIPER_BOB] ? "\y[ON - 0.98]" : "\d[OFF - Locked]");
    menu_additem(hMenu, szItem, "5");

    menu_additem(hMenu, "\yBack to:\w Crosshair - Basic (/crosshair)", "6");
    menu_additem(hMenu, "\ySwitch to:\w Misc HUD & Radar (/hud)", "7");

    menu_setprop(hMenu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, hMenu, 0);
    return PLUGIN_HANDLED;
}

public Handler_SniperMenu(id, hMenu, item)
{
    if (item == MENU_EXIT)
    {
        menu_destroy(hMenu);
        return PLUGIN_HANDLED;
    }

    new szInfo[8], szDummy[2], iAccess, iCallback;
    menu_item_getinfo(hMenu, item, iAccess, szInfo, charsmax(szInfo), szDummy, charsmax(szDummy), iCallback);
    menu_destroy(hMenu);

    new iChoice = str_to_num(szInfo);
    g_bHudProfileDirty[id] = true;

    switch (iChoice)
    {
        case 1:
        {
            g_ePlayerHud[id][HUD_SNIPER_WIDTH] = (g_ePlayerHud[id][HUD_SNIPER_WIDTH] % 10) + 1;
            Hud_Sync_SniperSettings(id);
        }
        case 2:
        {
            g_ePlayerHud[id][HUD_SNIPER_STYLE] = (g_ePlayerHud[id][HUD_SNIPER_STYLE] + 1) % sizeof(g_szHudSniperStyles);
            Hud_Sync_SniperSettings(id);
        }
        case 3:
        {
            g_ePlayerHud[id][HUD_SNIPER_POSTS] = !g_ePlayerHud[id][HUD_SNIPER_POSTS];
            Hud_Sync_SniperSettings(id);
        }
        case 4:
        {
            g_ePlayerHud[id][HUD_SNIPER_BLUR] = !g_ePlayerHud[id][HUD_SNIPER_BLUR];
            Hud_Sync_SniperSettings(id);
        }
        case 5:
        {
            g_ePlayerHud[id][HUD_SNIPER_BOB] = !g_ePlayerHud[id][HUD_SNIPER_BOB];
            Hud_Sync_SniperSettings(id);
        }
        case 6: return Hud_Cmd_OpenCrosshairMenu(id);
        case 7: return Hud_Cmd_OpenMiscHudMenu(id);
    }

    g_bHudHasProfile[id] = true;
    SaveProfileToVault(id);

    Hud_Cmd_OpenSniperMenu(id);
    return PLUGIN_HANDLED;
}

// =============================================================================
// SUB-TAB 3 MENU: MISC HUD & RADAR (say /hud)
// =============================================================================
public Hud_Cmd_OpenMiscHudMenu(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return PLUGIN_HANDLED;

    new szItem[96];
    new hMenu = menu_create("\y[Options -> HUD]\w Misc (Global Color & Radar)", "Handler_MiscHudMenu");

    new iPreset = g_ePlayerHud[id][HUD_COLOR_PRESET];
    formatex(szItem, charsmax(szItem), "Global HUD Color: \y%s (%s)", g_szHudColorRGB[iPreset][0], g_szHudColorRGB[iPreset][1]);
    menu_additem(hMenu, szItem, "1");

    formatex(szItem, charsmax(szItem), "Radar Shape: \y%s", g_ePlayerHud[id][HUD_RADAR_SQUARE] ? "Modern Square (CS2)" : "Classic Circle");
    menu_additem(hMenu, szItem, "2");

    formatex(szItem, charsmax(szItem), "Radar Orientation: \y%s", g_ePlayerHud[id][HUD_RADAR_LOCKED] ? "Locked North (Static)" : "Rotating with Player");
    menu_additem(hMenu, szItem, "3");

    formatex(szItem, charsmax(szItem), "Radar Zoom Scale: \y%.2f", Float:g_ePlayerHud[id][HUD_RADAR_SCALE]);
    menu_additem(hMenu, szItem, "4");

    menu_additem(hMenu, "\yBack to:\w Crosshair - Basic (/crosshair)", "5");
    menu_additem(hMenu, "\ySwitch to:\w Crosshair - Sniper Rifles (/scope)", "6");

    menu_setprop(hMenu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, hMenu, 0);
    return PLUGIN_HANDLED;
}

public Handler_MiscHudMenu(id, hMenu, item)
{
    if (item == MENU_EXIT)
    {
        menu_destroy(hMenu);
        return PLUGIN_HANDLED;
    }

    new szInfo[8], szDummy[2], iAccess, iCallback;
    menu_item_getinfo(hMenu, item, iAccess, szInfo, charsmax(szInfo), szDummy, charsmax(szDummy), iCallback);
    menu_destroy(hMenu);

    new iChoice = str_to_num(szInfo);
    g_bHudProfileDirty[id] = true;

    switch (iChoice)
    {
        case 1:
        {
            g_ePlayerHud[id][HUD_COLOR_PRESET] = (g_ePlayerHud[id][HUD_COLOR_PRESET] + 1) % sizeof(g_szHudColorRGB);
            Hud_Sync_MiscHudSettings(id);
        }
        case 2:
        {
            g_ePlayerHud[id][HUD_RADAR_SQUARE] = !g_ePlayerHud[id][HUD_RADAR_SQUARE];
            Hud_Sync_MiscHudSettings(id);
        }
        case 3:
        {
            g_ePlayerHud[id][HUD_RADAR_LOCKED] = !g_ePlayerHud[id][HUD_RADAR_LOCKED];
            Hud_Sync_MiscHudSettings(id);
        }
        case 4:
        {
            g_ePlayerHud[id][HUD_RADAR_SCALE] = Float:g_ePlayerHud[id][HUD_RADAR_SCALE] + 0.25;
            if (Float:g_ePlayerHud[id][HUD_RADAR_SCALE] > 2.0) g_ePlayerHud[id][HUD_RADAR_SCALE] = 0.50;
            Hud_Sync_MiscHudSettings(id);
        }
        case 5: return Hud_Cmd_OpenCrosshairMenu(id);
        case 6: return Hud_Cmd_OpenSniperMenu(id);
    }

    g_bHudHasProfile[id] = true;
    SaveProfileToVault(id);

    Hud_Cmd_OpenMiscHudMenu(id);
    return PLUGIN_HANDLED;
}

// =============================================================================
// CONSOLE / CHAT CLIENT CROSSHAIR IMPORT & AUTO-SAVE
// =============================================================================
public Hud_Cmd_SaveCrosshair(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return PLUGIN_HANDLED;

    query_client_cvar(id, "xhair_size", "CvarQuery_ImportSize");
    query_client_cvar(id, "xhair_gap", "CvarQuery_ImportGap");
    query_client_cvar(id, "xhair_thick", "CvarQuery_ImportThick");
    query_client_cvar(id, "xhair_dot", "CvarQuery_ImportDot");
    query_client_cvar(id, "xhair_t", "CvarQuery_ImportT");
    query_client_cvar(id, "xhair_color", "CvarQuery_ImportColor");
    query_client_cvar(id, "cl_crosshair_sniper_style", "CvarQuery_ImportSniperStyle");
    query_client_cvar(id, "cl_crosshair_sniper_width", "CvarQuery_ImportSniperWidth");
    query_client_cvar(id, "cl_crosshair_sniper_outer_posts", "CvarQuery_ImportSniperPosts");

    remove_task(id + TASK_HUD_SAVE);
    set_task(0.3, "Task_DeferredSaveProfile", id + TASK_HUD_SAVE);
    client_print_color(id, print_team_default, "^4[CS2 HUD]^1 Membaca konfigurasi crosshair dari konsol...");
    return PLUGIN_HANDLED;
}

public CvarQuery_ImportSize(id, const szCvar[], const szValue[], const szParam[])
{
    if (is_user_connected(id) && !equal(szValue, "Bad CVAR request"))
    {
        g_ePlayerHud[id][HUD_CH_SIZE] = str_to_float(szValue);
    }
}

public CvarQuery_ImportGap(id, const szCvar[], const szValue[], const szParam[])
{
    if (is_user_connected(id) && !equal(szValue, "Bad CVAR request"))
    {
        g_ePlayerHud[id][HUD_CH_GAP] = str_to_float(szValue);
    }
}

public CvarQuery_ImportThick(id, const szCvar[], const szValue[], const szParam[])
{
    if (is_user_connected(id) && !equal(szValue, "Bad CVAR request"))
    {
        g_ePlayerHud[id][HUD_CH_THICKNESS] = str_to_float(szValue);
    }
}

public CvarQuery_ImportDot(id, const szCvar[], const szValue[], const szParam[])
{
    if (is_user_connected(id) && !equal(szValue, "Bad CVAR request"))
    {
        g_ePlayerHud[id][HUD_CH_DOT] = str_to_num(szValue);
    }
}

public CvarQuery_ImportT(id, const szCvar[], const szValue[], const szParam[])
{
    if (is_user_connected(id) && !equal(szValue, "Bad CVAR request"))
    {
        g_ePlayerHud[id][HUD_CH_TSHAPE] = str_to_num(szValue);
    }
}

public CvarQuery_ImportColor(id, const szCvar[], const szValue[], const szParam[])
{
    if (is_user_connected(id) && !equal(szValue, "Bad CVAR request"))
    {
        new szR[8], szG[8], szB[8], szA[8];
        parse(szValue, szR, charsmax(szR), szG, charsmax(szG), szB, charsmax(szB), szA, charsmax(szA));
        g_ePlayerHud[id][HUD_CH_COLOR_R] = str_to_num(szR);
        g_ePlayerHud[id][HUD_CH_COLOR_G] = str_to_num(szG);
        g_ePlayerHud[id][HUD_CH_COLOR_B] = str_to_num(szB);
        if (szA[0]) g_ePlayerHud[id][HUD_CH_ALPHA] = str_to_num(szA);
    }
}

public CvarQuery_ImportSniperStyle(id, const szCvar[], const szValue[], const szParam[])
{
    if (is_user_connected(id) && !equal(szValue, "Bad CVAR request"))
    {
        g_ePlayerHud[id][HUD_SNIPER_STYLE] = clamp(str_to_num(szValue), 0, sizeof(g_szHudSniperStyles) - 1);
    }
}

public CvarQuery_ImportSniperWidth(id, const szCvar[], const szValue[], const szParam[])
{
    if (is_user_connected(id) && !equal(szValue, "Bad CVAR request"))
    {
        g_ePlayerHud[id][HUD_SNIPER_WIDTH] = clamp(str_to_num(szValue), 1, 10);
    }
}

public CvarQuery_ImportSniperPosts(id, const szCvar[], const szValue[], const szParam[])
{
    if (is_user_connected(id) && !equal(szValue, "Bad CVAR request"))
    {
        g_ePlayerHud[id][HUD_SNIPER_POSTS] = clamp(str_to_num(szValue), 0, 1);
    }
}

public Task_DeferredSaveProfile(taskid)
{
    new id = taskid - TASK_HUD_SAVE;
    if (is_user_connected(id) && !is_user_bot(id) && !is_user_hltv(id))
    {
        g_bHudHasProfile[id] = true;
        SaveProfileToVault(id);
        client_print_color(id, print_team_default, "^4[CS2 HUD]^1 Konfigurasi crosshair ^3tersimpan permanen ke vault^1! (Size: %.1f, Gap: %.1f, Thick: %.1f)",
            Float:g_ePlayerHud[id][HUD_CH_SIZE], Float:g_ePlayerHud[id][HUD_CH_GAP], Float:g_ePlayerHud[id][HUD_CH_THICKNESS]);
    }
}

// =============================================================================
// NVAULT SERIALIZATION & DESERIALIZATION ENGINE
// =============================================================================

stock SaveProfileToVault(id)
{
    if (g_hHudVault == INVALID_HANDLE || !is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return;

    new szKey[64], szData[256];
    Studio_BuildPlayerKey(id, szKey, charsmax(szKey));

    formatex(szData, charsmax(szData), "%d %.1f %.1f %.1f %d %d %d %.1f %d %d %d %d %d %d %d %d %d %d %d %d %d %d %.2f %d %d",
        g_ePlayerHud[id][HUD_CH_STYLE],
        Float:g_ePlayerHud[id][HUD_CH_SIZE],
        Float:g_ePlayerHud[id][HUD_CH_THICKNESS],
        Float:g_ePlayerHud[id][HUD_CH_GAP],
        g_ePlayerHud[id][HUD_CH_USEALPHA],
        g_ePlayerHud[id][HUD_CH_ALPHA],
        g_ePlayerHud[id][HUD_CH_OUTLINE],
        Float:g_ePlayerHud[id][HUD_CH_OUTLINE_THICK],
        g_ePlayerHud[id][HUD_CH_DOT],
        g_ePlayerHud[id][HUD_CH_TSHAPE],
        g_ePlayerHud[id][HUD_CH_WPNGAP],
        g_ePlayerHud[id][HUD_CH_FPSREC],
        g_ePlayerHud[id][HUD_CH_COLOR_IDX],
        g_ePlayerHud[id][HUD_CH_COLOR_R],
        g_ePlayerHud[id][HUD_CH_COLOR_G],
        g_ePlayerHud[id][HUD_CH_COLOR_B],
        g_ePlayerHud[id][HUD_SNIPER_WIDTH],
        g_ePlayerHud[id][HUD_SNIPER_STYLE],
        g_ePlayerHud[id][HUD_SNIPER_BLUR],
        g_ePlayerHud[id][HUD_SNIPER_BOB],
        g_ePlayerHud[id][HUD_RADAR_SQUARE],
        g_ePlayerHud[id][HUD_RADAR_LOCKED],
        Float:g_ePlayerHud[id][HUD_RADAR_SCALE],
        g_ePlayerHud[id][HUD_COLOR_PRESET],
        g_ePlayerHud[id][HUD_SNIPER_POSTS]);

    nvault_set(g_hHudVault, szKey, szData);
    g_bHudProfileDirty[id] = false;
    g_bHudHasProfile[id]   = true;
}

stock bool:LoadProfileFromVault(id)
{
    if (g_hHudVault == INVALID_HANDLE || !is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return false;

    new szKey[64], szData[256], iTimestamp;
    Studio_BuildPlayerKey(id, szKey, charsmax(szKey));

    if (!nvault_lookup(g_hHudVault, szKey, szData, charsmax(szData), iTimestamp))
        return false;

    new szTokens[25][16];
    new iCount = explode_string(szData, " ", szTokens, sizeof(szTokens), charsmax(szTokens[]));
    if (iCount < 16)
        return false;

    g_ePlayerHud[id][HUD_CH_STYLE]          = str_to_num(szTokens[0]);
    g_ePlayerHud[id][HUD_CH_SIZE]           = str_to_float(szTokens[1]);
    g_ePlayerHud[id][HUD_CH_THICKNESS]      = str_to_float(szTokens[2]);
    g_ePlayerHud[id][HUD_CH_GAP]            = str_to_float(szTokens[3]);
    g_ePlayerHud[id][HUD_CH_USEALPHA]       = str_to_num(szTokens[4]);
    g_ePlayerHud[id][HUD_CH_ALPHA]          = str_to_num(szTokens[5]);
    g_ePlayerHud[id][HUD_CH_OUTLINE]        = str_to_num(szTokens[6]);
    g_ePlayerHud[id][HUD_CH_OUTLINE_THICK]  = str_to_float(szTokens[7]);
    g_ePlayerHud[id][HUD_CH_DOT]            = str_to_num(szTokens[8]);
    g_ePlayerHud[id][HUD_CH_TSHAPE]         = str_to_num(szTokens[9]);
    g_ePlayerHud[id][HUD_CH_WPNGAP]         = str_to_num(szTokens[10]);
    g_ePlayerHud[id][HUD_CH_FPSREC]         = str_to_num(szTokens[11]);
    g_ePlayerHud[id][HUD_CH_COLOR_IDX]      = str_to_num(szTokens[12]);
    g_ePlayerHud[id][HUD_CH_COLOR_R]        = str_to_num(szTokens[13]);
    g_ePlayerHud[id][HUD_CH_COLOR_G]        = str_to_num(szTokens[14]);
    g_ePlayerHud[id][HUD_CH_COLOR_B]        = str_to_num(szTokens[15]);

    if (iCount >= 20)
    {
        g_ePlayerHud[id][HUD_SNIPER_WIDTH]  = clamp(str_to_num(szTokens[16]), 1, 10);
        g_ePlayerHud[id][HUD_SNIPER_STYLE]  = clamp(str_to_num(szTokens[17]), 0, sizeof(g_szHudSniperStyles) - 1);
        g_ePlayerHud[id][HUD_SNIPER_BLUR]   = str_to_num(szTokens[18]);
        g_ePlayerHud[id][HUD_SNIPER_BOB]    = str_to_num(szTokens[19]);
    }
    if (iCount >= 24)
    {
        g_ePlayerHud[id][HUD_RADAR_SQUARE]  = str_to_num(szTokens[20]);
        g_ePlayerHud[id][HUD_RADAR_LOCKED]  = str_to_num(szTokens[21]);
        g_ePlayerHud[id][HUD_RADAR_SCALE]   = str_to_float(szTokens[22]);
        g_ePlayerHud[id][HUD_COLOR_PRESET]  = str_to_num(szTokens[23]);
    }
    if (iCount >= 25)
    {
        g_ePlayerHud[id][HUD_SNIPER_POSTS]  = clamp(str_to_num(szTokens[24]), 0, 1);
    }

    return true;
}
