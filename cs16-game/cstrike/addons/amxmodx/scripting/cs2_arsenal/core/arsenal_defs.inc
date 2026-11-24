#if defined _arsenal_defs_included
    #endinput
#endif
#define _arsenal_defs_included

#define PLUGIN  "CS2 Arsenal (Skins & Variants)"
#define VERSION "2.0.0"
#define AUTHOR  "Antigravity"

#define TEAM_CT_INDEX 0
#define TEAM_T_INDEX  1

#define MAX_SKINS_PER_WEAPON 16
#define TOTAL_SLOTS (2 * (CSW_LAST_WEAPON + 1))
#define GET_SLOT(%1,%2) (%1 * (CSW_LAST_WEAPON + 1) + %2)

// Weapon categories for CS2 Loadout tree
enum _:WeaponCategory {
    CAT_KNIFE = 0,
    CAT_PISTOLS,
    CAT_MIDTIER,
    CAT_RIFLES
};

// Skin struct data (Pawn max 3 dimensions)
new g_SkinCount[TOTAL_SLOTS];
new g_SkinName[TOTAL_SLOTS][MAX_SKINS_PER_WEAPON][32];
new g_SkinVModel[TOTAL_SLOTS][MAX_SKINS_PER_WEAPON][128];
new g_SkinPModel[TOTAL_SLOTS][MAX_SKINS_PER_WEAPON][128];
new g_SkinWModel[TOTAL_SLOTS][MAX_SKINS_PER_WEAPON][128];
new g_SkinSubmodel[TOTAL_SLOTS][MAX_SKINS_PER_WEAPON];
new bool:g_SkinIsM4A4[TOTAL_SLOTS][MAX_SKINS_PER_WEAPON];
new g_SkinFireSound1[TOTAL_SLOTS][MAX_SKINS_PER_WEAPON][128];
new g_SkinFireSound2[TOTAL_SLOTS][MAX_SKINS_PER_WEAPON][128];
new g_SkinFireSoundCount[TOTAL_SLOTS][MAX_SKINS_PER_WEAPON];
new g_iWeaponEventId[CSW_LAST_WEAPON + 1];

// Player selected skin index [id][slot]
new g_iPlayerSkin[33][TOTAL_SLOTS];

// Active loadout defaults from active_loadout.ini [slot]
new g_ActiveLoadoutSkin[TOTAL_SLOTS];

// CVars
new pCvar_Enable;
new pCvar_AutoSilenceM4A1;
new pCvar_BlockSilencerM4A4;
new pCvar_Notify;
new pCvar_BotM4Mode;

// Bot M4 Configuration & Persistence
new g_iBotM4Override[33];
new Trie:g_tBotM4Preference = Invalid_Trie;

// Vault handle
new g_hVault = INVALID_HANDLE;

// ------------------------------------------------------------------------------
// Modern Weapon Variants State & Economics (merged from cs2_weapon_variants)
// ------------------------------------------------------------------------------
new g_pCvarEnable;
new g_pCvarPricing;
new g_pCvarClipOverride;
new g_pCvarNotify;

new g_HudSyncEco;

new g_szActiveLoadoutCT[CSW_LAST_WEAPON + 1][64];
new g_szActiveLoadoutT[CSW_LAST_WEAPON + 1][64];

new bool:g_bWeaponInReload[1024];

new g_iPendingBuyWeapon[33];
new Float:g_flPendingBuyTime[33];

new const g_szVariantWeapons[][] = {
    "weapon_p228",      // Tec-9
    "weapon_xm1014",    // MAG-7
    "weapon_mac10",     // CZ75-Auto (T)
    "weapon_tmp",       // CZ75-Auto (CT)
    "weapon_mp5navy",   // MP7
    "weapon_m249",      // Negev
    "weapon_m3",        // Sawed-Off
    "weapon_p90"        // PP-Bizon
};

// Standard model names for fallback discovery
new const g_WeaponModelNames[CSW_LAST_WEAPON + 1][] = {
    "",                  // 0 CSW_NONE
    "v_p228.mdl",        // 1 CSW_P228
    "",                  // 2 CSW_GLOCK (unused)
    "v_scout.mdl",       // 3 CSW_SCOUT
    "v_hegrenade.mdl",   // 4 CSW_HEGRENADE
    "v_xm1014.mdl",      // 5 CSW_XM1014
    "v_c4.mdl",          // 6 CSW_C4
    "v_mac10.mdl",       // 7 CSW_MAC10
    "v_aug.mdl",         // 8 CSW_AUG
    "v_smokegrenade.mdl", // 9 CSW_SMOKEGRENADE
    "v_elite.mdl",       // 10 CSW_ELITE
    "v_fiveseven.mdl",   // 11 CSW_FIVESEVEN
    "v_ump45.mdl",       // 12 CSW_UMP45
    "v_sg550.mdl",       // 13 CSW_SG550
    "v_galil.mdl",       // 14 CSW_GALIL
    "v_famas.mdl",       // 15 CSW_FAMAS
    "v_usp.mdl",         // 16 CSW_USP
    "v_glock18.mdl",     // 17 CSW_GLOCK18
    "v_awp.mdl",         // 18 CSW_AWP
    "v_mp5.mdl",         // 19 CSW_MP5NAVY
    "v_m249.mdl",        // 20 CSW_M249
    "v_m3.mdl",          // 21 CSW_M3
    "v_m4a1.mdl",        // 22 CSW_M4A1
    "v_tmp.mdl",         // 23 CSW_TMP
    "v_g3sg1.mdl",       // 24 CSW_G3SG1
    "v_flashbang.mdl",   // 25 CSW_FLASHBANG
    "v_deagle.mdl",      // 26 CSW_DEAGLE
    "v_sg552.mdl",       // 27 CSW_SG552
    "v_ak47.mdl",        // 28 CSW_AK47
    "v_knife.mdl",       // 29 CSW_KNIFE
    "v_p90.mdl"          // 30 CSW_P90
};

new const g_WeaponEntityNames[CSW_LAST_WEAPON + 1][] = {
    "",
    "weapon_p228",
    "",
    "weapon_scout",
    "weapon_hegrenade",
    "weapon_xm1014",
    "weapon_c4",
    "weapon_mac10",
    "weapon_aug",
    "weapon_smokegrenade",
    "weapon_elite",
    "weapon_fiveseven",
    "weapon_ump45",
    "weapon_sg550",
    "weapon_galil",
    "weapon_famas",
    "weapon_usp",
    "weapon_glock18",
    "weapon_awp",
    "weapon_mp5navy",
    "weapon_m249",
    "weapon_m3",
    "weapon_m4a1",
    "weapon_tmp",
    "weapon_g3sg1",
    "weapon_flashbang",
    "weapon_deagle",
    "weapon_sg552",
    "weapon_ak47",
    "weapon_knife",
    "weapon_p90"
};

// Friendly weapon display names
new const g_WeaponDisplayNames[CSW_LAST_WEAPON + 1][] = {
    "",
    "P228 / Tec-9 Slot",
    "",
    "SSG 08 (Scout)",
    "HE Grenade",
    "XM1014 / MAG-7 Slot",
    "C4 Explosive",
    "MAC-10 / CZ75-Auto Slot",
    "AUG",
    "Smoke Grenade",
    "Dual Berettas",
    "Five-SeveN",
    "UMP-45",
    "SG 550 Sniper",
    "Galil AR",
    "FAMAS",
    "USP-S",
    "Glock-18",
    "AWP",
    "MP5-SD / MP7 Slot",
    "M249 / Negev Slot",
    "M3 / Sawed-Off Slot",
    "M4 Slot (M4A4 / M4A1-S)",
    "TMP / CZ75-Auto Slot",
    "G3SG1 Auto-Sniper",
    "Flashbang",
    "Desert Eagle",
    "SG 552",
    "AK-47",
    "Knife Slot",
    "P90 / PP-Bizon Slot"
};

new const g_WeaponEventNames[CSW_LAST_WEAPON + 1][] = {
    "",
    "events/p228.sc",          // 1 CSW_P228
    "",                        // 2 CSW_GLOCK (unused)
    "events/scout.sc",         // 3 CSW_SCOUT
    "",                        // 4 CSW_HEGRENADE
    "events/xm1014.sc",        // 5 CSW_XM1014
    "",                        // 6 CSW_C4
    "events/mac10.sc",         // 7 CSW_MAC10
    "events/aug.sc",           // 8 CSW_AUG
    "",                        // 9 CSW_SMOKEGRENADE
    "events/elite_left.sc",    // 10 CSW_ELITE
    "events/fiveseven.sc",     // 11 CSW_FIVESEVEN
    "events/ump45.sc",         // 12 CSW_UMP45
    "events/sg550.sc",         // 13 CSW_SG550
    "events/galil.sc",         // 14 CSW_GALIL
    "events/famas.sc",         // 15 CSW_FAMAS
    "events/usp.sc",           // 16 CSW_USP
    "events/glock18.sc",       // 17 CSW_GLOCK18
    "events/awp.sc",           // 18 CSW_AWP
    "events/mp5n.sc",          // 19 CSW_MP5NAVY
    "events/m249.sc",          // 20 CSW_M249
    "events/m3.sc",            // 21 CSW_M3
    "events/m4a1.sc",          // 22 CSW_M4A1
    "events/tmp.sc",           // 23 CSW_TMP
    "events/g3sg1.sc",         // 24 CSW_G3SG1
    "",                        // 25 CSW_FLASHBANG
    "events/deagle.sc",        // 26 CSW_DEAGLE
    "events/sg552.sc",         // 27 CSW_SG552
    "events/ak47.sc",          // 28 CSW_AK47
    "",                        // 29 CSW_KNIFE
    "events/p90.sc"            // 30 CSW_P90
};


// Safe wrapper around team lookup that works on bots/players without relying on pvPrivateData
stock CsTeams:safe_cs_get_user_team(id)
{
    if (id < 1 || id > MaxClients || !is_user_connected(id))
        return CS_TEAM_UNASSIGNED;

    new team = get_user_team(id);
    switch (team)
    {
        case 1: return CS_TEAM_T;
        case 2: return CS_TEAM_CT;
        case 3: return CS_TEAM_SPECTATOR;
        default: return CS_TEAM_UNASSIGNED;
    }
    return CS_TEAM_UNASSIGNED;
}
