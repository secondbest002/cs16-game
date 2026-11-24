#if defined _cs2_sdk_api_included
    #endinput
#endif
#define _cs2_sdk_api_included

// ==============================================================================
// CS2 SDK - Unified Cross-Plugin Native Contracts & Optional Native Resilience
// Uses set_native_filter("cs2_sdk_native_filter") + LibraryExists() guards.
// Safe with #pragma semicolon 1.
// ==============================================================================

#if !defined _savage_role_defined
#define _savage_role_defined
enum _:SavageRole
{
    ROLE_NONE = 0,
    ROLE_ENTRY = 1,   // Entry Fragger: AK47/M4A1 (or Galil/Famas) + Deagle + 1x Flash + 1x HE
    ROLE_AWPER,       // Sniper: AWP (or Scout) + Deagle + 1x Flash + 1x Smoke
    ROLE_SUPPORT,     // Support Utility: AK47/M4A1 + P228/FiveSeven + Smoke + 2x Flash + HE + Defuse Kit
    ROLE_LURKER,      // Flanker/Anchor: AK47/M4A1-S (Silenced) + USP-S + Smoke + Flash
    ROLE_IGL,         // Tactical Leader: AK47/M4A1 + Deagle + Smoke + 2x Flash + HE + Defuse Kit
    MAX_SAVAGE_ROLES
};
#endif

// ------------------------------------------------------------------------------
// Provider: cs2_loadout (cs2_skin_inventory) / cs2_arsenal
// ------------------------------------------------------------------------------
native bool:cs2_is_m4a4_native(id);
native cs2_get_active_skin_name(id, weapon_id, out[], maxlen);
native cs2_get_active_skin_vmodel(id, weapon_id, out[], maxlen);
native cs2_get_active_submodel(id, weapon_id);

// ------------------------------------------------------------------------------
// Provider: cs2_variants (cs2_weapon_variants) / cs2_arsenal
// Guarded so providers exporting legacy public funcs of same name avoid collision
// ------------------------------------------------------------------------------
#if !defined CS2_API_PROVIDE_VARIANTS
native bool:cs2_is_user_holding_variant(id, weapon_id = 0, const variant_name[] = "");
native bool:cs2_get_user_weapon_variant(id, weapon_id, buffer[], maxlen);
native cs2_get_variant_price(weapon_id, const variant_name[] = "");
#endif

// ------------------------------------------------------------------------------
// Provider: cs2_engine (cs2_modern_engine)
// ------------------------------------------------------------------------------
native cs2_refund_player_primary(id, &refundedCost = 0, szItemName[] = "", maxlen = 0);
native cs2_refund_player_all(id, &refundedCost = 0);
native bool:cs2_can_player_refund(id);

// ------------------------------------------------------------------------------
// Provider: bot_savage_ai
// ------------------------------------------------------------------------------
native SavageRole:savage_get_player_role(id);
native savage_equip_bot_role(id, bool:bFreeLoadout = true, bool:bAllowUtility = false);

// ------------------------------------------------------------------------------
// Provider: csgo_molotov
// ------------------------------------------------------------------------------
native csgo_get_user_molotov(id);

// ------------------------------------------------------------------------------
// Optional Native Filter (Call set_native_filter("cs2_sdk_native_filter") inside plugin_natives())
// ------------------------------------------------------------------------------
public cs2_sdk_native_filter(const name[], index, trap)
{
    if (!trap)
    {
        if (equal(name, "cs2_", 4) || equal(name, "savage_", 7) || equal(name, "csgo_", 5))
        {
            return PLUGIN_HANDLED;
        }
    }
    return PLUGIN_CONTINUE;
}

// ------------------------------------------------------------------------------
// Library Availability Helpers
// ------------------------------------------------------------------------------
stock bool:cs2_api_has_loadout()
{
    return (LibraryExists("cs2_loadout", LibType_Library) || LibraryExists("cs2_arsenal", LibType_Library));
}

stock bool:cs2_api_has_variants()
{
    return (LibraryExists("cs2_variants", LibType_Library) || LibraryExists("cs2_arsenal", LibType_Library));
}

stock bool:cs2_api_has_engine()
{
    return bool:LibraryExists("cs2_engine", LibType_Library);
}

stock bool:cs2_api_has_savage_ai()
{
    return bool:LibraryExists("bot_savage_ai", LibType_Library);
}

stock bool:cs2_api_has_molotov()
{
    return bool:LibraryExists("csgo_molotov", LibType_Library);
}

// ------------------------------------------------------------------------------
// Guarded Safe Cross-Plugin Wrappers (Zero runtime errors when a plugin is off)
// ------------------------------------------------------------------------------
stock bool:cs2_api_is_m4a4(id, &bool:bAvailable = false)
{
    if (!cs2_api_has_loadout())
    {
        bAvailable = false;
        return false;
    }

    bAvailable = true;
    return cs2_is_m4a4_native(id);
}

#if !defined CS2_API_PROVIDE_VARIANTS
stock bool:cs2_api_is_holding_variant(id, weapon_id = 0, const variant_name[] = "")
{
    if (!cs2_api_has_variants())
        return false;

    return cs2_is_user_holding_variant(id, weapon_id, variant_name);
}
#endif

stock SavageRole:cs2_api_get_player_role(id)
{
    if (!cs2_api_has_savage_ai())
        return SavageRole:ROLE_NONE;

    return savage_get_player_role(id);
}

stock cs2_api_equip_bot_role(id, bool:bFreeLoadout = true, bool:bAllowUtility = false)
{
    if (!cs2_api_has_savage_ai())
        return 0;

    return savage_equip_bot_role(id, bFreeLoadout, bAllowUtility);
}

stock cs2_api_refund_player_primary(id, &refundedCost = 0, szItemName[] = "", maxlen = 0)
{
    if (!cs2_api_has_engine())
        return 0;

    return cs2_refund_player_primary(id, refundedCost, szItemName, maxlen);
}

stock cs2_api_refund_player_all(id, &refundedCost = 0)
{
    if (!cs2_api_has_engine())
        return 0;

    return cs2_refund_player_all(id, refundedCost);
}

stock bool:cs2_api_can_player_refund(id)
{
    if (!cs2_api_has_engine())
        return false;

    return cs2_can_player_refund(id);
}
