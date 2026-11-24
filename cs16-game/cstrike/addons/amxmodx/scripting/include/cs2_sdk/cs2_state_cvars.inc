#if defined _cs2_sdk_state_cvars_included
    #endinput
#endif
#define _cs2_sdk_state_cvars_included

// ==============================================================================
// CS2 SDK - Cached O(1) Cross-Plugin Gamemode State Queries
// Safe with #pragma semicolon 1.
// ==============================================================================

stock bool:cs2_state_is_warmup()
{
    static pCvar = 0;
    if (!pCvar)
        pCvar = get_cvar_pointer("amx_csgo_is_warmup");

    return (pCvar && get_pcvar_num(pCvar) == 1);
}

stock bool:cs2_state_is_timeout()
{
    static pCvar = 0;
    if (!pCvar)
        pCvar = get_cvar_pointer("amx_csgo_is_timeout");

    return (pCvar && get_pcvar_num(pCvar) == 1);
}

stock bool:cs2_state_is_deathmatch()
{
    static pCvar = 0;
    if (!pCvar)
        pCvar = get_cvar_pointer("redm_active");

    return (pCvar && get_pcvar_num(pCvar) == 1);
}

stock bool:cs2_state_is_ffa()
{
    static pCvar = 0;
    if (!pCvar)
        pCvar = get_cvar_pointer("redm_freeforall");

    return (pCvar && get_pcvar_num(pCvar) == 1);
}

stock bool:cs2_state_is_knife_round()
{
    static pCvar = 0;
    if (!pCvar)
        pCvar = get_cvar_pointer("bot_savage_kniferound");

    return (pCvar && get_pcvar_num(pCvar) == 1);
}

stock cs2_state_get_match_round()
{
    static pCvar = 0;
    if (!pCvar)
        pCvar = get_cvar_pointer("amx_csgo_current_round");

    return pCvar ? get_pcvar_num(pCvar) : 0;
}
