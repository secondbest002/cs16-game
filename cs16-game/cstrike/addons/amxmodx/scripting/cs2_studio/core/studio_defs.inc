#if defined _studio_defs_included
    #endinput
#endif
#define _studio_defs_included

#define STUDIO_PLUGIN_NAME    "CS2 Client UX Studio"
#define STUDIO_PLUGIN_VERSION "2.3.0"
#define STUDIO_PLUGIN_AUTHOR  "Principal Engine Architect"

#define STUDIO_HUD_VAULT_NAME "cs2_crosshair"
#define STUDIO_VM_VAULT_NAME  "cs2_viewmodel"
#define HUD_BATCH_INTERVAL    0.15

// Dual independent nVault handles (Zero Player Data Loss)
new g_hHudVault = INVALID_HANDLE;
new g_hVmVault  = INVALID_HANDLE;

// =============================================================================
// SUBSYSTEM A: CROSSHAIR, SNIPER SCOPE, MISC HUD & RADAR DEFINITIONS
// =============================================================================
enum _:PlayerHudProfile
{
    HUD_CH_STYLE,               // cl_crosshairstyle (0 - 5) / xhair_dynamic_move
    Float:HUD_CH_SIZE,          // cl_crosshairsize / xhair_size (0.0 - 20.0)
    Float:HUD_CH_THICKNESS,     // cl_crosshairthickness / xhair_thick (0.0 - 20.0)
    Float:HUD_CH_GAP,           // cl_crosshairgap / xhair_gap (-30.0 - 30.0)
    HUD_CH_USEALPHA,            // cl_crosshairusealpha (0 / 1)
    HUD_CH_ALPHA,               // cl_crosshairalpha / xhair_color alpha (0 - 255)
    HUD_CH_OUTLINE,             // cl_crosshair_drawoutline / xhair_pad (0 / 1)
    Float:HUD_CH_OUTLINE_THICK, // cl_crosshair_outlinethickness / xhair_pad (0.0 - 3.0)
    HUD_CH_DOT,                 // cl_crosshairdot / xhair_dot (0 / 1)
    HUD_CH_TSHAPE,              // cl_crosshair_t / xhair_t (0 / 1)
    HUD_CH_WPNGAP,              // cl_crosshairgap_useweaponvalue / xhair_gap_useweaponvalue (0 / 1)
    HUD_CH_FPSREC,              // cl_crosshair_recoverytime_on_fps (0 / 1)
    HUD_CH_COLOR_IDX,           // cl_crosshaircolor (0..5)
    HUD_CH_COLOR_R,             // cl_crosshaircolor_r / xhair_color R (0 - 255)
    HUD_CH_COLOR_G,             // cl_crosshaircolor_g / xhair_color G (0 - 255)
    HUD_CH_COLOR_B,             // cl_crosshaircolor_b / xhair_color B (0 - 255)

    HUD_SNIPER_WIDTH,           // cl_crosshair_sniper_width (1 - 10)
    HUD_SNIPER_STYLE,           // cl_crosshair_sniper_style (0=Classic, 1=CS2 Vignette, 2=Dot, 3=CS2 Frameless)
    HUD_SNIPER_POSTS,           // cl_crosshair_sniper_outer_posts (0=Uniform Thin, 1=Thick Outer Posts)
    HUD_SNIPER_BLUR,            // cl_crosshair_sniper_show_normal_inaccuracy (0 / 1)
    HUD_SNIPER_BOB,             // cl_bob_sniper (0 / 1)

    HUD_RADAR_SQUARE,           // hud_radar_square (0 / 1)
    HUD_RADAR_LOCKED,           // hud_radar_locked (0 / 1)
    Float:HUD_RADAR_SCALE,      // hud_radar_scale (0.5 - 2.0)
    HUD_COLOR_PRESET            // hud_color preset index (0 - 5)
};

new any:g_ePlayerHud[MAX_PLAYERS + 1][PlayerHudProfile];
new bool:g_bHudProfileDirty[MAX_PLAYERS + 1];
new bool:g_bHudHasProfile[MAX_PLAYERS + 1];
new bool:g_bIsXashCS16[MAX_PLAYERS + 1];
new bool:g_bIsGSModern[MAX_PLAYERS + 1];
new g_iHudBatchStep[MAX_PLAYERS + 1];

new const g_szHudStyleNames[][] = {
    "Default Dynamic",
    "Default Static",
    "Classic Split",
    "Classic Dynamic",
    "Classic Static",
    "Classic Legacy (Firing Only)"
};

new const g_szHudColorNames[][] = {
    "Green (#vgui_select)",
    "Red",
    "Blue",
    "Yellow",
    "Cyan",
    "Custom RGB (White)"
};

new const g_szHudSniperStyles[][] = {
    "Classic CS 1.6 Scope",
    "CS2 Modern Scope (Vignette)",
    "Center Dot Only",
    "CS2 Frameless (No Vignette)"
};

new const g_szHudColorRGB[][][] = {
    { "Amber Classic", "255 180 30" },
    { "CS2 Cyan",      "50 230 250" },
    { "Emerald Green", "50 250 80"  },
    { "Crimson Red",   "250 60 60"  },
    { "Pure White",    "245 245 245"},
    { "Violet Neon",   "190 90 255" }
};

// =============================================================================
// SUBSYSTEM B: PER-WEAPON VIEWMODEL STUDIO DEFINITIONS
// =============================================================================
#define VM_FOV_MIN          50.0
#define VM_FOV_MAX          75.0
#define VM_FOV_DEFAULT      60.0

#define VM_OFS_MIN          -3.5
#define VM_OFS_MAX          3.5
#define VM_OFS_DEFAULT      0.0

#define VM_BOB_MIN          0.0
#define VM_BOB_MAX          0.030
#define VM_BOB_DEFAULT      0.010

#define VM_LAG_MIN          0.0
#define VM_LAG_MAX          1.50
#define VM_LAG_DEFAULT      0.0

enum
{
    PROFILE_M4A4 = 31,
    PROFILE_KNIFE_KARAMBIT,
    PROFILE_KNIFE_BUTTERFLY,
    PROFILE_KNIFE_M9,
    PROFILE_KNIFE_BAYONET,
    PROFILE_KNIFE_BOWIE,
    PROFILE_KNIFE_FALCHION,
    PROFILE_KNIFE_DAGGERS,
    PROFILE_KNIFE_STILETTO,
    PROFILE_KNIFE_TALON,
    PROFILE_KNIFE_URSUS,
    PROFILE_KNIFE_SKELETON,
    PROFILE_KNIFE_FLIP,
    PROFILE_KNIFE_GUT,
    PROFILE_KNIFE_HUNTSMAN,
    PROFILE_KNIFE_NAVAJA,
    PROFILE_KNIFE_NOMAD,
    PROFILE_KNIFE_PARACORD,
    PROFILE_KNIFE_SURVIVAL,
    MAX_VM_PROFILES
};

enum _:VM_DATA
{
    Float:VM_FOV = 0,
    Float:VM_X,
    Float:VM_Y,
    Float:VM_Z,
    Float:VM_BOB,
    Float:VM_LAG
};

new const g_szVmProfileKeys[MAX_VM_PROFILES][] = {
    "",                 // 0  Unused
    "p228",             // 1  CSW_P228
    "",                 // 2  Unused
    "scout",            // 3  CSW_SCOUT
    "hegrenade",        // 4  CSW_HEGRENADE
    "xm1014",           // 5  CSW_XM1014
    "c4",               // 6  CSW_C4
    "mac10",            // 7  CSW_MAC10
    "aug",              // 8  CSW_AUG
    "smokegrenade",     // 9  CSW_SMOKEGRENADE
    "elite",            // 10 CSW_ELITE
    "fiveseven",        // 11 CSW_FIVESEVEN
    "ump45",            // 12 CSW_UMP45
    "sg550",            // 13 CSW_SG550
    "galil",            // 14 CSW_GALIL
    "famas",            // 15 CSW_FAMAS
    "usp",              // 16 CSW_USP
    "glock18",          // 17 CSW_GLOCK18
    "awp",              // 18 CSW_AWP
    "mp5navy",          // 19 CSW_MP5NAVY
    "m249",             // 20 CSW_M249
    "m3",               // 21 CSW_M3
    "m4a1s",            // 22 CSW_M4A1 (M4A1-S)
    "tmp",              // 23 CSW_TMP
    "g3sg1",            // 24 CSW_G3SG1
    "flashbang",        // 25 CSW_FLASHBANG
    "deagle",           // 26 CSW_DEAGLE
    "sg552",            // 27 CSW_SG552
    "ak47",             // 28 CSW_AK47
    "knife_default",    // 29 CSW_KNIFE (Default)
    "p90",              // 30 CSW_P90
    "m4a4",             // 31 M4A4 Variant
    "knife_karambit",   // 32
    "knife_butterfly",  // 33
    "knife_m9",         // 34
    "knife_bayonet",    // 35
    "knife_bowie",      // 36
    "knife_falchion",   // 37
    "knife_daggers",    // 38
    "knife_stiletto",   // 39
    "knife_talon",      // 40
    "knife_ursus",      // 41
    "knife_skeleton",   // 42
    "knife_flip",       // 43
    "knife_gut",        // 44
    "knife_huntsman",   // 45
    "knife_navaja",     // 46
    "knife_nomad",      // 47
    "knife_paracord",   // 48
    "knife_survival"    // 49
};

new const g_szVmProfileNames[MAX_VM_PROFILES][] = {
    "None",
    "P228 / Tec-9",
    "None",
    "Scout (SSG 08)",
    "HE Grenade",
    "XM1014 / MAG-7",
    "C4 Explosive",
    "MAC-10 / CZ75 (T)",
    "AUG",
    "Smoke Grenade",
    "Dual Berettas",
    "Five-SeveN",
    "UMP-45",
    "SG 550",
    "Galil AR",
    "FAMAS",
    "USP-S",
    "Glock-18",
    "AWP",
    "MP5 / MP7",
    "M249 / Negev",
    "M3 / Sawed-Off",
    "M4A1-S",
    "TMP / CZ75 (CT)",
    "G3SG1",
    "Flashbang",
    "Desert Eagle",
    "SG 552",
    "AK-47",
    "Knife (Default)",
    "P90 / PP-Bizon",
    "M4A4",
    "Knife: Karambit",
    "Knife: Butterfly",
    "Knife: M9 Bayonet",
    "Knife: Bayonet",
    "Knife: Bowie",
    "Knife: Falchion",
    "Knife: Shadow Daggers",
    "Knife: Stiletto",
    "Knife: Talon",
    "Knife: Ursus",
    "Knife: Skeleton",
    "Knife: Flip",
    "Knife: Gut",
    "Knife: Huntsman",
    "Knife: Navaja",
    "Knife: Nomad",
    "Knife: Paracord",
    "Knife: Survival"
};

new g_flVmProfile[MAX_PLAYERS + 1][MAX_VM_PROFILES][VM_DATA];
new bool:g_bVmProfileCustom[MAX_PLAYERS + 1][MAX_VM_PROFILES];
new bool:g_bVmProfileDirty[MAX_PLAYERS + 1][MAX_VM_PROFILES];

new g_iVmLastProfile[MAX_PLAYERS + 1];
new bool:g_bClientHasNativeVM[MAX_PLAYERS + 1];
new bool:g_bVmAdjustNegative[MAX_PLAYERS + 1];
new bool:g_bVmFineStep[MAX_PLAYERS + 1];
new g_iVmMenuState[MAX_PLAYERS + 1];
new g_szVmVaultPrefix[MAX_PLAYERS + 1][40];

new const Float:g_flBobSteps[] = { 0.000, 0.005, 0.010, 0.015, 0.020, 0.025, 0.030 };
new const Float:g_flLagSteps[] = { 0.00, 0.25, 0.50, 0.80, 1.00, 1.25, 1.50 };

// Shared Player Vault Key Sanitizer (bit-identical for cs2_crosshair & cs2_viewmodel)
stock Studio_BuildPlayerKey(id, szKey[], iLen)
{
    new szAuth[36];
    get_user_authid(id, szAuth, charsmax(szAuth));

    if (!szAuth[0] || equal(szAuth, "STEAM_ID_LAN") || equal(szAuth, "VALVE_ID_LAN") || equal(szAuth, "STEAM_ID_PENDING"))
    {
        get_user_name(id, szKey, iLen);
    }
    else
    {
        copy(szKey, iLen, szAuth);
    }

    if (!szKey[0])
    {
        formatex(szKey, iLen, "PLAYER_%d", id);
    }

    for (new i = 0; szKey[i]; i++)
    {
        if (szKey[i] <= 32 || szKey[i] == 34 || szKey[i] == 39 || szKey[i] == ';' || szKey[i] == '#' || szKey[i] == ':')
            szKey[i] = '_';
    }
}

stock Studio_ClearPlayerTasks(id)
{
    remove_task(id + TASK_STU_PROBE);
    remove_task(id + TASK_HUD_SYNC);
    remove_task(id + TASK_HUD_SAVE);
    remove_task(id + TASK_VM_DEFER);
    remove_task(id + TASK_VM_SPAWN);
}
