#if defined _cs2_sdk_weapon_registry_included
    #endinput
#endif
#define _cs2_sdk_weapon_registry_included

// ==============================================================================
// CS2 SDK - Canonical Weapon & Custom Knife Registry (Single Source of Truth)
// Consolidates 18 custom knife types across knife sounds, killfeed, & viewmodel.
// Safe with #pragma semicolon 1.
// ==============================================================================

enum _:CS2KnifeType
{
    CS2_KNIFE_DEFAULT = 0,
    CS2_KNIFE_KARAMBIT,
    CS2_KNIFE_BUTTERFLY,
    CS2_KNIFE_M9,
    CS2_KNIFE_BAYONET,
    CS2_KNIFE_BOWIE,
    CS2_KNIFE_FALCHION,
    CS2_KNIFE_DAGGERS,
    CS2_KNIFE_STILETTO,
    CS2_KNIFE_TALON,
    CS2_KNIFE_URSUS,
    CS2_KNIFE_SKELETON,
    CS2_KNIFE_FLIP,
    CS2_KNIFE_GUT,
    CS2_KNIFE_HUNTSMAN,
    CS2_KNIFE_NAVAJA,
    CS2_KNIFE_NOMAD,
    CS2_KNIFE_PARACORD,
    CS2_KNIFE_SURVIVAL,
    CS2_KNIFE_MAX
};

// NOTE: "m9" MUST be checked BEFORE "bayonet" because M9 Bayonet paths contain both!
stock cs2_detect_knife_type(const szModelPath[])
{
    if (!szModelPath[0])
        return CS2_KNIFE_DEFAULT;

    if (containi(szModelPath, "m9") != -1)
        return CS2_KNIFE_M9;
    if (containi(szModelPath, "butterfly") != -1 || containi(szModelPath, "bknife") != -1)
        return CS2_KNIFE_BUTTERFLY;
    if (containi(szModelPath, "karambit") != -1)
        return CS2_KNIFE_KARAMBIT;
    if (containi(szModelPath, "bayonet") != -1)
        return CS2_KNIFE_BAYONET;
    if (containi(szModelPath, "bowie") != -1)
        return CS2_KNIFE_BOWIE;
    if (containi(szModelPath, "falchion") != -1)
        return CS2_KNIFE_FALCHION;
    if (containi(szModelPath, "daggers") != -1 || containi(szModelPath, "push") != -1)
        return CS2_KNIFE_DAGGERS;
    if (containi(szModelPath, "stiletto") != -1 || containi(szModelPath, "stilletto") != -1)
        return CS2_KNIFE_STILETTO;
    if (containi(szModelPath, "talon") != -1 || containi(szModelPath, "widow") != -1)
        return CS2_KNIFE_TALON;
    if (containi(szModelPath, "ursus") != -1)
        return CS2_KNIFE_URSUS;
    if (containi(szModelPath, "skeleton") != -1)
        return CS2_KNIFE_SKELETON;
    if (containi(szModelPath, "flip") != -1)
        return CS2_KNIFE_FLIP;
    if (containi(szModelPath, "gut") != -1)
        return CS2_KNIFE_GUT;
    if (containi(szModelPath, "huntsman") != -1)
        return CS2_KNIFE_HUNTSMAN;
    if (containi(szModelPath, "navaja") != -1 || containi(szModelPath, "gypsy") != -1)
        return CS2_KNIFE_NAVAJA;
    if (containi(szModelPath, "nomad") != -1)
        return CS2_KNIFE_NOMAD;
    if (containi(szModelPath, "paracord") != -1)
        return CS2_KNIFE_PARACORD;
    if (containi(szModelPath, "survival") != -1)
        return CS2_KNIFE_SURVIVAL;

    return CS2_KNIFE_DEFAULT;
}

stock cs2_get_knife_token(kType, szToken[], maxlen)
{
    static const szTokens[CS2_KNIFE_MAX][] = {
        "knife",
        "knife_karambit",
        "knife_butterfly",
        "knife_m9",
        "knife_bayonet",
        "knife_bowie",
        "knife_falchion",
        "knife_daggers",
        "knife_stiletto",
        "knife_talon",
        "knife_ursus",
        "knife_skeleton",
        "knife_flip",
        "knife_gut",
        "knife_huntsman",
        "knife_navaja",
        "knife_nomad",
        "knife_paracord",
        "knife_survival"
    };

    if (kType < 0 || kType >= CS2_KNIFE_MAX)
        kType = CS2_KNIFE_DEFAULT;

    copy(szToken, maxlen, szTokens[kType]);
}

stock cs2_get_knife_display_name(kType, szName[], maxlen)
{
    static const szNames[CS2_KNIFE_MAX][] = {
        "KNIFE",
        "KARAMBIT",
        "BUTTERFLY KNIFE",
        "M9 BAYONET",
        "BAYONET",
        "BOWIE KNIFE",
        "FALCHION KNIFE",
        "SHADOW DAGGERS",
        "STILETTO",
        "TALON KNIFE",
        "URSUS KNIFE",
        "SKELETON KNIFE",
        "FLIP KNIFE",
        "GUT KNIFE",
        "HUNTSMAN",
        "NAVAJA KNIFE",
        "NOMAD KNIFE",
        "PARACORD KNIFE",
        "SURVIVAL KNIFE"
    };

    if (kType < 0 || kType >= CS2_KNIFE_MAX)
        kType = CS2_KNIFE_DEFAULT;

    copy(szName, maxlen, szNames[kType]);
}

// Verified against cstrike/sprites/hud.txt (640hud_cs2): only these 7 custom knives have d_knife_* sprites
stock bool:cs2_has_knife_killfeed_sprite(kType)
{
    switch (kType)
    {
        case CS2_KNIFE_KARAMBIT,
             CS2_KNIFE_BUTTERFLY,
             CS2_KNIFE_M9,
             CS2_KNIFE_BAYONET,
             CS2_KNIFE_STILETTO,
             CS2_KNIFE_TALON,
             CS2_KNIFE_HUNTSMAN:
        {
            return true;
        }
    }
    return false;
}
