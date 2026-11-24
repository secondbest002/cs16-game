/**
 * =============================================================================
 * Plugin Name : CS2 Client UX Studio (Unified HUD, Crosshair, Scope & Viewmodel)
 * File Name   : cs2_client_studio.sma
 * Target      : AMX Mod X 1.10+ | ReGameDLL | Xash3D FWGS 0.21 (Patched client.dll)
 * Vault DBs   : data/vault/cs2_crosshair.vault + data/vault/cs2_viewmodel.vault
 * Architecture: Zero PreThink overhead, Dual-Vault Zero-Migration, Modular .inl
 * =============================================================================
 */

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>
#include <nvault>
#include <cs2_sdk/cs2_offsets>
#include <cs2_sdk/cs2_weapon_registry>

#pragma semicolon 1

#define ENABLE_STUDIO_CROSSHAIR_HUD
#define ENABLE_STUDIO_VIEWMODEL
#define ENABLE_STUDIO_UNIFIED_HUB

#include "cs2_studio/core/studio_taskids.inc"
#include "cs2_studio/core/studio_defs.inc"
#include "cs2_studio/core/studio_client_probe.inc"

#if defined ENABLE_STUDIO_CROSSHAIR_HUD
    #include "cs2_studio/modules/mod_crosshair_hud.inl"
#endif

#if defined ENABLE_STUDIO_VIEWMODEL
    #include "cs2_studio/modules/mod_viewmodel_studio.inl"
#endif

#if defined ENABLE_STUDIO_UNIFIED_HUB
    #include "cs2_studio/modules/mod_studio_hub.inl"
#endif

public plugin_init()
{
    register_plugin(STUDIO_PLUGIN_NAME, STUDIO_PLUGIN_VERSION, STUDIO_PLUGIN_AUTHOR);

    #if defined ENABLE_STUDIO_CROSSHAIR_HUD
    Hud_Module_Init();
    #endif

    #if defined ENABLE_STUDIO_VIEWMODEL
    Vm_Module_Init();
    #endif

    #if defined ENABLE_STUDIO_UNIFIED_HUB
    Studio_Hub_Init();
    #endif

    server_print("[CS2 Studio] Client UX Studio initialized (Dual-Vault: %s + %s).", STUDIO_HUD_VAULT_NAME, STUDIO_VM_VAULT_NAME);
}

public plugin_end()
{
    #if defined ENABLE_STUDIO_CROSSHAIR_HUD
    Hud_Module_End();
    #endif

    #if defined ENABLE_STUDIO_VIEWMODEL
    Vm_Module_End();
    #endif
}

public client_putinserver(id)
{
    Studio_ClearPlayerTasks(id);

    // Absolute Guardrail: Skip YaPB bots and HLTV completely
    if (is_user_bot(id) || is_user_hltv(id))
        return;

    #if defined ENABLE_STUDIO_CROSSHAIR_HUD
    Hud_Module_PutInServer(id);
    #endif

    #if defined ENABLE_STUDIO_VIEWMODEL
    Vm_Module_PutInServer(id);
    #endif

    // Single unified non-blocking capability probe + initial sync
    set_task(1.0, "Studio_Task_StartClientProbe", id + TASK_STU_PROBE);
}

public client_disconnected(id)
{
    Studio_ClearPlayerTasks(id);

    #if defined ENABLE_STUDIO_VIEWMODEL
    g_iVmMenuState[id] = 0;
    #endif

    if (is_user_bot(id) || is_user_hltv(id))
        return;

    #if defined ENABLE_STUDIO_CROSSHAIR_HUD
    Hud_Module_Disconnect(id);
    #endif

    #if defined ENABLE_STUDIO_VIEWMODEL
    Vm_Module_Disconnect(id);
    #endif
}
