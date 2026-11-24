#if defined _mod_studio_hub_included
    #endinput
#endif
#define _mod_studio_hub_included

// =============================================================================
// UNIFIED CLIENT STUDIO HUB (/studio)
// =============================================================================

public Studio_Hub_Init()
{
    register_clcmd("say /studio", "Studio_Cmd_OpenHub");
    register_clcmd("say_team /studio", "Studio_Cmd_OpenHub");
    register_clcmd("say .studio", "Studio_Cmd_OpenHub");
    register_clcmd("cs2_studio", "Studio_Cmd_OpenHub");
}

public Studio_Cmd_OpenHub(id)
{
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return PLUGIN_HANDLED;

    new hMenu = menu_create("\y[CS2 Client UX Studio]\w Select Customization Tool:", "Studio_Handler_Hub");

    #if defined ENABLE_STUDIO_CROSSHAIR_HUD
    menu_additem(hMenu, "Crosshair - Basic \d(/crosshair)", "1");
    menu_additem(hMenu, "Crosshair - Sniper Rifles \d(/scope)", "2");
    menu_additem(hMenu, "Misc HUD & Radar \d(/hud)", "3");
    #endif

    #if defined ENABLE_STUDIO_VIEWMODEL
    menu_additem(hMenu, "Per-Weapon Viewmodel Studio \d(/vm)", "4");
    #endif

    #if defined ENABLE_STUDIO_CROSSHAIR_HUD
    menu_additem(hMenu, "\ySave Console Crosshair to Vault \d(/savecrosshair)", "5");
    #endif

    menu_setprop(hMenu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, hMenu, 0);
    return PLUGIN_HANDLED;
}

public Studio_Handler_Hub(id, hMenu, item)
{
    if (item == MENU_EXIT || !is_user_connected(id))
    {
        menu_destroy(hMenu);
        return PLUGIN_HANDLED;
    }

    new szInfo[8], szDummy[2], iAccess, iCallback;
    menu_item_getinfo(hMenu, item, iAccess, szInfo, charsmax(szInfo), szDummy, charsmax(szDummy), iCallback);
    menu_destroy(hMenu);

    new iChoice = str_to_num(szInfo);
    switch (iChoice)
    {
        #if defined ENABLE_STUDIO_CROSSHAIR_HUD
        case 1: return Hud_Cmd_OpenCrosshairMenu(id);
        case 2: return Hud_Cmd_OpenSniperMenu(id);
        case 3: return Hud_Cmd_OpenMiscHudMenu(id);
        case 5: return Hud_Cmd_SaveCrosshair(id);
        #endif

        #if defined ENABLE_STUDIO_VIEWMODEL
        case 4: return Vm_Cmd_OpenStudioMenu(id);
        #endif
    }

    return PLUGIN_HANDLED;
}
