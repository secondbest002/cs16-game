#if defined _studio_client_probe_included
    #endinput
#endif
#define _studio_client_probe_included

// =============================================================================
// UNIFIED NON-BLOCKING CLIENT CAPABILITY PROBE
// Fires xhair_enable, cl_crosshairthickness, and viewmodel_fov queries once.
// =============================================================================

public Studio_Task_StartClientProbe(taskid)
{
    new id = taskid - TASK_STU_PROBE;
    if (!is_user_connected(id) || is_user_bot(id) || is_user_hltv(id))
        return;

    #if defined ENABLE_STUDIO_CROSSHAIR_HUD
    query_client_cvar(id, "xhair_enable", "Hud_CvarQuery_XhairCallback");
    query_client_cvar(id, "cl_crosshairthickness", "Hud_CvarQuery_GSModernCallback");

    if (g_bHudHasProfile[id])
    {
        Hud_Sync_AllSettings(id);
    }
    else
    {
        Hud_Sync_SniperSettings(id);
        Hud_Sync_MiscHudSettings(id);
    }
    #endif

    #if defined ENABLE_STUDIO_VIEWMODEL
    query_client_cvar(id, "viewmodel_fov", "Vm_CvarQuery_NativeVMCallback");
    #endif
}
