/*
 * ==============================================================================
 * CS2 ARSENAL (PILLAR 1: SKIN INVENTORY, LOADOUT, M4 SPLIT & WEAPON VARIANTS)
 * ==============================================================================
 * Unified Pillar 1 Orchestrator combining cs2_skin_inventory + cs2_weapon_variants.
 * Note: csgo_molotov.amxx intentionally remains a standalone plugin.
 * Vault DB: cstrike/addons/amxmodx/data/vault/cs2_loadout.nvault
 * ==============================================================================
 */

#include <amxmodx>
#include <amxmisc>
#include <cstrike>
#include <fakemeta>
#include <hamsandwich>
#include <nvault>
#include <xs>
#include <cs2_sdk/cs2_offsets>
#define CS2_API_PROVIDE_VARIANTS
#include <cs2_sdk/cs2_api>

#pragma dynamic 32768

// Feature Switchboard
#define ENABLE_ARSENAL_SKIN_MODELS
#define ENABLE_ARSENAL_M4_SPLIT
#define ENABLE_ARSENAL_GUNSHOT_FX
#define ENABLE_ARSENAL_VARIANTS

#include "cs2_arsenal/core/arsenal_taskids.inc"
#include "cs2_arsenal/core/arsenal_defs.inc"
#include "cs2_arsenal/core/arsenal_loadout.inc"
#include "cs2_arsenal/core/arsenal_vault.inc"

#if defined ENABLE_ARSENAL_SKIN_MODELS
    #include "cs2_arsenal/modules/mod_skin_models.inl"
#endif

#if defined ENABLE_ARSENAL_M4_SPLIT
    #include "cs2_arsenal/modules/mod_m4_split.inl"
#endif

#if defined ENABLE_ARSENAL_GUNSHOT_FX
    #include "cs2_arsenal/modules/mod_gunshot_fx.inl"
#endif

#if defined ENABLE_ARSENAL_VARIANTS
    #include "cs2_arsenal/modules/mod_weapon_variants.inl"
#endif

#include "cs2_arsenal/modules/mod_loadout_menu.inl"

public plugin_natives()
{
    register_library("cs2_arsenal");
    set_native_filter("cs2_sdk_native_filter");

    // Skin & Loadout Natives
    register_native("cs2_get_active_skin_name", "_native_get_active_skin_name");
    register_native("cs2_get_active_skin_vmodel", "_native_get_active_skin_vmodel");
    register_native("cs2_get_active_submodel", "_native_get_active_submodel");
    register_native("cs2_is_m4a4_native", "_native_is_m4a4");

    // Weapon Variants Natives
    #if defined ENABLE_ARSENAL_VARIANTS
    register_native("cs2_is_user_holding_variant", "native_is_user_holding_variant");
    register_native("cs2_get_user_weapon_variant", "native_get_user_weapon_variant");
    register_native("cs2_get_variant_price", "native_get_variant_price");
    #endif
}

public plugin_precache()
{
    Inv_Catalog_Precache();
}

public plugin_init()
{
    register_plugin(PLUGIN, VERSION, AUTHOR);
    // CVars
    pCvar_Enable             = register_cvar("cs2_loadout_enable", "1");
    pCvar_AutoSilenceM4A1    = register_cvar("cs2_m4_auto_silence", "1");
    pCvar_BlockSilencerM4A4  = register_cvar("cs2_m4_block_silencer", "1");
    pCvar_Notify             = register_cvar("cs2_loadout_notify", "1");
    pCvar_BotM4Mode          = register_cvar("cs2_bot_m4_mode", "1");

    // Chat / Client Commands
    register_clcmd("say /loadout", "Cmd_OpenLoadout");
    register_clcmd("say_team /loadout", "Cmd_OpenLoadout");
    register_clcmd("say /skin", "Cmd_OpenLoadout");
    register_clcmd("say_team /skin", "Cmd_OpenLoadout");
    register_clcmd("say /inv", "Cmd_OpenLoadout");
    register_clcmd("say_team /inv", "Cmd_OpenLoadout");
    register_clcmd("say /knife", "Cmd_OpenKnifeMenu");
    register_clcmd("say_team /knife", "Cmd_OpenKnifeMenu");
    register_clcmd("say /gloves", "Cmd_OpenGloveInfo");
    register_clcmd("say_team /gloves", "Cmd_OpenGloveInfo");
    register_clcmd("say /m4", "Cmd_OpenM4Menu");
    register_clcmd("say_team /m4", "Cmd_OpenM4Menu");
    register_clcmd("say /m4a4", "Cmd_SelectM4A4Direct");
    register_clcmd("say_team /m4a4", "Cmd_SelectM4A4Direct");
    register_clcmd("say /m4a1", "Cmd_SelectM4A1Direct");
    register_clcmd("say_team /m4a1", "Cmd_SelectM4A1Direct");
    register_clcmd("say /resetloadout", "Cmd_ResetLoadout");
    register_clcmd("say_team /resetloadout", "Cmd_ResetLoadout");

    // Modern Weapons Direct Commands
    register_clcmd("say /tec9", "Cmd_SelectTec9Direct");
    register_clcmd("say_team /tec9", "Cmd_SelectTec9Direct");
    register_clcmd("tec9", "Cmd_SelectTec9Direct");
    register_clcmd("say /cz75", "Cmd_SelectCZ75Direct");
    register_clcmd("say_team /cz75", "Cmd_SelectCZ75Direct");
    register_clcmd("say /cz", "Cmd_SelectCZ75Direct");
    register_clcmd("say_team /cz", "Cmd_SelectCZ75Direct");
    register_clcmd("cz75", "Cmd_SelectCZ75Direct");
    register_clcmd("cz", "Cmd_SelectCZ75Direct");
    register_clcmd("say /mp7", "Cmd_SelectMP7Direct");
    register_clcmd("say_team /mp7", "Cmd_SelectMP7Direct");
    register_clcmd("mp7", "Cmd_SelectMP7Direct");
    register_clcmd("say /bizon", "Cmd_SelectBizonDirect");
    register_clcmd("say_team /bizon", "Cmd_SelectBizonDirect");
    register_clcmd("bizon", "Cmd_SelectBizonDirect");
    register_clcmd("say /mag7", "Cmd_SelectMAG7Direct");
    register_clcmd("say_team /mag7", "Cmd_SelectMAG7Direct");
    register_clcmd("mag7", "Cmd_SelectMAG7Direct");
    register_clcmd("say /sawedoff", "Cmd_SelectSawedOffDirect");
    register_clcmd("say_team /sawedoff", "Cmd_SelectSawedOffDirect");
    register_clcmd("sawedoff", "Cmd_SelectSawedOffDirect");
    register_clcmd("say /negev", "Cmd_SelectNegevDirect");
    register_clcmd("say_team /negev", "Cmd_SelectNegevDirect");
    register_clcmd("negev", "Cmd_SelectNegevDirect");

    // CT Bot M4 Management Commands
    register_clcmd("say /botm4", "Cmd_BotM4Command");
    register_clcmd("say_team /botm4", "Cmd_BotM4Command");
    register_clcmd("say /m4bot", "Cmd_BotM4Command");
    register_clcmd("say_team /m4bot", "Cmd_BotM4Command");
    register_clcmd("botm4", "Cmd_BotM4Command");
    register_clcmd("m4bot", "Cmd_BotM4Command");

    // Console commands
    register_clcmd("loadout", "Cmd_OpenLoadout");
    register_clcmd("skin", "Cmd_OpenLoadout");
    register_clcmd("inv", "Cmd_OpenLoadout");
    register_clcmd("knife", "Cmd_OpenKnifeMenu");
    register_clcmd("gloves", "Cmd_OpenGloveInfo");
    register_clcmd("m4", "Cmd_OpenM4Menu");
    register_clcmd("m4a4", "Cmd_SelectM4A4Direct");
    // NOTE: Do NOT register raw "m4a1" as clcmd because it hijacks engine CS 1.6 weapon purchase (B-4-3 / m4a1)!
    // Use say /m4a1 or m4a1s instead.
    register_clcmd("m4a1s", "Cmd_SelectM4A1Direct");
    register_clcmd("say /reloadloadout", "Cmd_ReloadActiveLoadout");
    register_concmd("cs2_reload_loadout", "Cmd_ReloadActiveLoadout", ADMIN_CFG, "Reload active_loadout.ini");

    // Events
    register_event("CurWeapon", "Arsenal_Event_CurWeapon", "be", "1=1");
    register_event("ResetHUD", "Event_ResetHUD", "b");
    register_event("TeamInfo", "Event_TeamInfo", "a", "2=TERRORIST", "2=CT");
    register_logevent("Event_RoundStart_Skin", 2, "1=Round_Start");

    // Player Spawn Hook
    RegisterHam(Ham_Spawn, "player", "OnPlayerSpawn_Post", 1);

    // Unified Weapon Dispatch Hooks (deterministic ordering: skins stamp/apply before variant economy/clip adjustments)
    for (new i = 1; i <= CSW_LAST_WEAPON; i++)
    {
        if (g_WeaponEntityNames[i][0])
        {
            RegisterHam(Ham_Item_Deploy, g_WeaponEntityNames[i], "Arsenal_OnItemDeploy_Post", 1);
            if (i != CSW_M4A1)
            {
                RegisterHam(Ham_Item_AddToPlayer, g_WeaponEntityNames[i], "Arsenal_OnItemAddToPlayer_Post", 1);
            }
        }
    }

    // Specific M4 hooks for M4A4 vs M4A1-S mechanics
    #if defined ENABLE_ARSENAL_M4_SPLIT
    RegisterHam(Ham_Item_Deploy, "weapon_m4a1", "OnM4Deploy_Pre", 0);
    RegisterHam(Ham_Item_AddToPlayer, "weapon_m4a1", "OnM4AddToPlayer_Post", 1);
    RegisterHam(Ham_Weapon_SecondaryAttack, "weapon_m4a1", "OnM4SecondaryAttack_Pre", 0);
    register_forward(FM_SetModel, "OnSetModel_Post", 1);
    #endif

    // Gunfire sound suppression & custom playback
    #if defined ENABLE_ARSENAL_GUNSHOT_FX
    register_forward(FM_UpdateClientData, "OnUpdateClientData_Post", 1);
    register_forward(FM_PlaybackEvent, "OnPlaybackEvent_Pre", 0);
    CacheGunshotDecals();
    #endif

    // Weapon Variants Subsystem
    #if defined ENABLE_ARSENAL_VARIANTS
    Arsenal_Variants_Init();
    #endif

    // Initialize session-persistent Bot M4 preference Trie
    g_tBotM4Preference = TrieCreate();

    // Open persistent NVault storage (preserves legacy "cs2_loadout" vault DB)
    g_hVault = nvault_open("cs2_loadout");
    if (g_hVault == INVALID_HANDLE)
    {
        server_print("[CS2 Arsenal] Warning: Could not open cs2_loadout vault!");
    }

    server_print("[CS2 Arsenal] Pillar 1 initialized (Skins, M4 Split, Gunshot FX, Variants). Commands: /loadout, /knife, /m4, /gloves, /botm4");
}

public plugin_end()
{
    if (g_hVault != INVALID_HANDLE)
    {
        nvault_close(g_hVault);
        g_hVault = INVALID_HANDLE;
    }

    if (g_tBotM4Preference != Invalid_Trie)
    {
        TrieDestroy(g_tBotM4Preference);
        g_tBotM4Preference = Invalid_Trie;
    }
}

public Arsenal_OnItemDeploy_Post(iWpn)
{
    #if defined ENABLE_ARSENAL_SKIN_MODELS
    Arsenal_SkinModels_DeployPost(iWpn);
    #endif

    #if defined ENABLE_ARSENAL_VARIANTS
    Arsenal_Variants_DeployPost(iWpn);
    #endif

    return HAM_IGNORED;
}

public Arsenal_OnItemAddToPlayer_Post(iWpn, id)
{
    #if defined ENABLE_ARSENAL_M4_SPLIT
    Arsenal_SkinModels_AddToPlayerPost(iWpn, id);
    #endif

    #if defined ENABLE_ARSENAL_VARIANTS
    Arsenal_Variants_AddToPlayerPost(iWpn, id);
    #endif

    return HAM_IGNORED;
}

public Arsenal_Event_CurWeapon(id)
{
    #if defined ENABLE_ARSENAL_SKIN_MODELS
    Arsenal_SkinModels_CurWeapon(id);
    #endif

    #if defined ENABLE_ARSENAL_VARIANTS
    Arsenal_Variants_CurWeapon(id);
    #endif
}
