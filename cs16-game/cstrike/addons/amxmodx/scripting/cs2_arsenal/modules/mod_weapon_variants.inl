#if defined _mod_weapon_variants_included
    #endinput
#endif
#define _mod_weapon_variants_included

// ==============================================================================
// WEAPON VARIANTS MODULE (Pricing, Authentic Clips, Meta Economics & Interop)
// Merged cleanly from cs2_weapon_variants.sma (shares active_loadout.ini pass)
// ==============================================================================

public Arsenal_Variants_Init()
{
    g_pCvarEnable       = register_cvar("amx_cs2_variants", "1");
    g_pCvarPricing      = register_cvar("amx_cs2_variant_pricing", "1");
    g_pCvarClipOverride = register_cvar("amx_cs2_variant_clips", "1");
    g_pCvarNotify       = register_cvar("amx_cs2_variant_notify", "1");

    g_HudSyncEco = CreateHudSyncObj();

    // Variant-specific PostFrame (reload clip override); Deploy/AddToPlayer/CurWeapon dispatched by cs2_arsenal.sma
    for (new i = 0; i < sizeof(g_szVariantWeapons); i++)
    {
        RegisterHam(Ham_Item_PostFrame, g_szVariantWeapons[i], "OnVariantPostFrame_Post", 1);
    }

    HookBuyCommands();
}

stock bool:Arsenal_IsVariantWeaponId(weapon_id)
{
    switch (weapon_id)
    {
        case CSW_P228, CSW_XM1014, CSW_MAC10, CSW_TMP, CSW_MP5NAVY, CSW_M249, CSW_M3, CSW_P90:
            return true;
    }
    return false;
}
stock bool:MatchesVariantToken(const text[], const variant_name[])
{
    if (!text[0])
        return false;

    if (variant_name[0])
    {
        if (equali(variant_name, "mag") || equali(variant_name, "mag7") || equali(variant_name, "mag-7"))
            return (containi(text, "mag7") != -1 || containi(text, "mag-7") != -1);

        if (equali(variant_name, "cz") || equali(variant_name, "cz75") || equali(variant_name, "cz-75"))
            return (containi(text, "cz75") != -1 || containi(text, "cz-75") != -1);

        if (equali(variant_name, "tec") || equali(variant_name, "tec9") || equali(variant_name, "tec-9"))
            return (containi(text, "tec9") != -1 || containi(text, "tec-9") != -1);

        if (equali(variant_name, "sawed") || equali(variant_name, "sawedoff") || equali(variant_name, "sawed-off"))
            return (containi(text, "sawed") != -1);

        return (containi(text, variant_name) != -1);
    }

    return (containi(text, "mag7") != -1 ||
            containi(text, "mag-7") != -1 ||
            containi(text, "negev") != -1 ||
            containi(text, "sawed") != -1 ||
            containi(text, "bizon") != -1 ||
            containi(text, "cz75") != -1 ||
            containi(text, "cz-75") != -1 ||
            containi(text, "tec9") != -1 ||
            containi(text, "tec-9") != -1 ||
            containi(text, "mp7") != -1);
}

stock bool:IsPlayerUsingVariant(id, weapon_id, const variant_name[] = "")
{
    if (!is_user_connected(id))
        return false;

    new held_weapon = is_user_alive(id) ? cs_get_user_weapon(id) : CSW_NONE;
    if (weapon_id <= CSW_NONE)
    {
        weapon_id = held_weapon;
    }

    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return false;

    // 1. Inspect player's currently active viewmodel only when holding this weapon_id
    if (is_user_alive(id) && held_weapon == weapon_id)
    {
        new szVm[128];
        pev(id, pev_viewmodel2, szVm, charsmax(szVm));
        if (MatchesVariantToken(szVm, variant_name))
            return true;
    }

    new team = (id >= 1 && id <= MaxClients) ? get_user_team(id) : 0;
    new team_index = (team == 2) ? TEAM_CT_INDEX : ((team == 1) ? TEAM_T_INDEX : -1);

    // 2. Check player's currently equipped catalog skin for this slot
    if (team_index != -1)
    {
        new slot = GET_SLOT(team_index, weapon_id);
        new s_idx = g_iPlayerSkin[id][slot];
        if (g_SkinCount[slot] > 0 && s_idx >= 0 && s_idx < g_SkinCount[slot])
        {
            if (MatchesVariantToken(g_SkinVModel[slot][s_idx], variant_name) ||
                MatchesVariantToken(g_SkinName[slot][s_idx], variant_name))
            {
                return true;
            }
        }
    }

    // 3. Fallback to active loadout cache by player's team
    new szLoadoutModel[64];
    if (team == 2)
        copy(szLoadoutModel, charsmax(szLoadoutModel), g_szActiveLoadoutCT[weapon_id]);
    else if (team == 1)
        copy(szLoadoutModel, charsmax(szLoadoutModel), g_szActiveLoadoutT[weapon_id]);

    return MatchesVariantToken(szLoadoutModel, variant_name);
}

stock LookupVariantPrice(weapon_id, const szVariant[] = "")
{
    if (szVariant[0])
    {
        if (containi(szVariant, "mag7") != -1 || containi(szVariant, "mag-7") != -1 || equali(szVariant, "mag"))
            return 1300;
        if (containi(szVariant, "negev") != -1)
            return 1700;
        if (containi(szVariant, "sawed") != -1)
            return 1100;
        if (containi(szVariant, "bizon") != -1)
            return 1400;
        if (containi(szVariant, "cz75") != -1 || containi(szVariant, "cz-75") != -1 || equali(szVariant, "cz"))
            return 500;
        if (containi(szVariant, "tec9") != -1 || containi(szVariant, "tec-9") != -1 || equali(szVariant, "tec"))
            return 500;
        if (containi(szVariant, "mp7") != -1)
            return 1500;
    }

    switch (weapon_id)
    {
        case CSW_XM1014:         return 1300; // MAG-7
        case CSW_M249:           return 1700; // Negev
        case CSW_M3:             return 1100; // Sawed-Off
        case CSW_P90:            return 1400; // PP-Bizon
        case CSW_TMP, CSW_MAC10: return 500;  // CZ75-Auto
        case CSW_P228:           return 500;  // CZ75-Auto / Tec-9
        case CSW_MP5NAVY:        return 1500; // MP7
    }
    return 0;
}

stock GetVariantInfo(id, weapon_id, szVariantName[], nameLen, &targetPrice, &refundAmount, &targetClip)
{
    targetPrice = 0;
    refundAmount = 0;
    targetClip = 0;
    szVariantName[0] = '^0';

    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return 0;

    switch (weapon_id)
    {
        case CSW_XM1014: // MAG-7
        {
            if (IsPlayerUsingVariant(id, weapon_id, "mag7"))
            {
                copy(szVariantName, nameLen, "MAG-7");
                targetPrice = LookupVariantPrice(weapon_id, szVariantName);
                refundAmount = 3000 - targetPrice; // XM1014 $3,000 - $1,300
                targetClip = 5;
                return 1;
            }
        }
        case CSW_M249: // Negev
        {
            if (IsPlayerUsingVariant(id, weapon_id, "negev"))
            {
                copy(szVariantName, nameLen, "Negev");
                targetPrice = LookupVariantPrice(weapon_id, szVariantName);
                refundAmount = 5750 - targetPrice; // M249 $5,750 - $1,700
                targetClip = 150;
                return 1;
            }
        }
        case CSW_M3: // Sawed-Off
        {
            if (IsPlayerUsingVariant(id, weapon_id, "sawed"))
            {
                copy(szVariantName, nameLen, "Sawed-Off");
                targetPrice = LookupVariantPrice(weapon_id, szVariantName);
                refundAmount = 1700 - targetPrice; // M3 $1,700 - $1,100
                targetClip = 8;
                return 1;
            }
        }
        case CSW_P90: // PP-Bizon
        {
            if (IsPlayerUsingVariant(id, weapon_id, "bizon"))
            {
                copy(szVariantName, nameLen, "PP-Bizon");
                targetPrice = LookupVariantPrice(weapon_id, szVariantName);
                refundAmount = 2350 - targetPrice; // P90 $2,350 - $1,400
                targetClip = 64;
                return 1;
            }
        }
        case CSW_TMP: // CZ75-Auto (CT legacy)
        {
            if (IsPlayerUsingVariant(id, weapon_id, "cz75"))
            {
                copy(szVariantName, nameLen, "CZ75-Auto");
                targetPrice = LookupVariantPrice(weapon_id, szVariantName);
                refundAmount = 1250 - targetPrice; // TMP $1,250 - $500
                targetClip = 12;
                return 1;
            }
        }
        case CSW_MAC10: // CZ75-Auto (T legacy)
        {
            if (IsPlayerUsingVariant(id, weapon_id, "cz75"))
            {
                copy(szVariantName, nameLen, "CZ75-Auto");
                targetPrice = LookupVariantPrice(weapon_id, szVariantName);
                refundAmount = 1400 - targetPrice; // MAC-10 $1,400 - $500
                targetClip = 12;
                return 1;
            }
        }
        case CSW_P228: // CZ75-Auto / Tec-9
        {
            if (IsPlayerUsingVariant(id, weapon_id, "cz75"))
            {
                copy(szVariantName, nameLen, "CZ75-Auto");
                targetPrice = LookupVariantPrice(weapon_id, szVariantName);
                refundAmount = 600 - targetPrice; // P228 $600 - $500
                targetClip = 12;
                return 1;
            }
            if (IsPlayerUsingVariant(id, weapon_id, "tec9"))
            {
                copy(szVariantName, nameLen, "Tec-9");
                targetPrice = LookupVariantPrice(weapon_id, szVariantName);
                refundAmount = 600 - targetPrice; // P228 $600 - $500
                targetClip = 18;
                return 1;
            }
        }
        case CSW_MP5NAVY: // MP7
        {
            if (IsPlayerUsingVariant(id, weapon_id, "mp7"))
            {
                copy(szVariantName, nameLen, "MP7");
                targetPrice = LookupVariantPrice(weapon_id, szVariantName);
                refundAmount = 1500 - targetPrice; // MP5NAVY $1,500 - $1,500
                targetClip = 30;
                return 1;
            }
        }
    }
    return 0;
}

// ==============================================================================
// 1. DYNAMIC WEAPON PRICING & ECONOMY REFUND
// ==============================================================================

stock HookBuyCommands()
{
    // Shotguns
    register_clcmd("xm1014",      "Cmd_BuyXM1014");
    register_clcmd("autoshotgun", "Cmd_BuyXM1014");
    register_clcmd("m3",          "Cmd_BuyM3");
    register_clcmd("12gauge",     "Cmd_BuyM3");

    // Heavy
    register_clcmd("m249",        "Cmd_BuyM249");

    // SMGs
    register_clcmd("p90",         "Cmd_BuyP90");
    register_clcmd("c90",         "Cmd_BuyP90");
    register_clcmd("tmp",         "Cmd_BuyTMP");
    register_clcmd("mp",          "Cmd_BuyTMP");
    register_clcmd("mac10",       "Cmd_BuyMAC10");
    register_clcmd("mp5",         "Cmd_BuyMP5");
    register_clcmd("mp5navy",     "Cmd_BuyMP5");
    register_clcmd("smg",         "Cmd_BuyMP5");

    // Pistols
    register_clcmd("p228",        "Cmd_BuyP228");
    register_clcmd("228compact",  "Cmd_BuyP228");

    // General buy / autobuy
    register_clcmd("cl_autobuy",  "Cmd_AutoBuy");
    register_clcmd("cl_rebuy",    "Cmd_ReBuy");
    register_clcmd("autobuy",     "Cmd_AutoBuy");
    register_clcmd("rebuy",       "Cmd_ReBuy");
    register_clcmd("menuselect",  "Cmd_MenuSelect");
    register_clcmd("buy",         "Cmd_Buy");
    register_clcmd("buyequip",    "Cmd_Buy");

    // Loadout reload command
}

public Cmd_BuyXM1014(id) { SetPendingBuy(id, CSW_XM1014); return PLUGIN_CONTINUE; }
public Cmd_BuyM249(id)   { SetPendingBuy(id, CSW_M249);   return PLUGIN_CONTINUE; }
public Cmd_BuyM3(id)     { SetPendingBuy(id, CSW_M3);     return PLUGIN_CONTINUE; }
public Cmd_BuyP90(id)    { SetPendingBuy(id, CSW_P90);    return PLUGIN_CONTINUE; }
public Cmd_BuyTMP(id)    { SetPendingBuy(id, CSW_TMP);    return PLUGIN_CONTINUE; }
public Cmd_BuyMAC10(id)  { SetPendingBuy(id, CSW_MAC10);  return PLUGIN_CONTINUE; }
public Cmd_BuyP228(id)   { SetPendingBuy(id, CSW_P228);   return PLUGIN_CONTINUE; }
public Cmd_BuyMP5(id)    { SetPendingBuy(id, CSW_MP5NAVY); return PLUGIN_CONTINUE; }

public Cmd_AutoBuy(id)   { g_flPendingBuyTime[id] = get_gametime(); return PLUGIN_CONTINUE; }
public Cmd_ReBuy(id)     { g_flPendingBuyTime[id] = get_gametime(); return PLUGIN_CONTINUE; }
public Cmd_MenuSelect(id){ g_flPendingBuyTime[id] = get_gametime(); return PLUGIN_CONTINUE; }
public Cmd_Buy(id)       { g_flPendingBuyTime[id] = get_gametime(); return PLUGIN_CONTINUE; }

stock SetPendingBuy(id, weaponId)
{
    g_iPendingBuyWeapon[id] = weaponId;
    g_flPendingBuyTime[id]  = get_gametime();
}

public Arsenal_Variants_AddToPlayerPost(iWpn, id)
{
    if (!get_pcvar_num(g_pCvarEnable) || !pev_valid(iWpn) || !is_user_alive(id))
        return HAM_IGNORED;

    new weapon_id = cs_get_weapon_id(iWpn);
    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON || !Arsenal_IsVariantWeaponId(weapon_id))
        return HAM_IGNORED;

    // Check if weapon is freshly spawned/purchased (pev_iuser3 == 0)
    new iHandled = pev(iWpn, pev_iuser3);
    if (iHandled != 0)
        return HAM_IGNORED;

    set_pev(iWpn, pev_iuser3, 1);

    // Apply CS2 Meta Pricing & Economy refund if genuine purchase occurred in buyzone
    if (get_pcvar_num(g_pCvarPricing))
    {
        new bool:bInBuyzone = bool:cs_get_user_buyzone(id);
        new Float:flTimeDiff = get_gametime() - g_flPendingBuyTime[id];
        new bool:bRecentlyBought = (flTimeDiff < 2.0 && g_flPendingBuyTime[id] > 0.0);

        if (bInBuyzone && bRecentlyBought)
        {
            new szName[32], targetPrice, refundAmount, targetClip;
            if (GetVariantInfo(id, weapon_id, szName, charsmax(szName), targetPrice, refundAmount, targetClip))
            {
                if (refundAmount > 0)
                {
                    new curMoney = cs_get_user_money(id);
                    new newMoney = min(16000, curMoney + refundAmount);
                    cs_set_user_money(id, newMoney, 1);

                    // User notification
                    if (get_pcvar_num(g_pCvarNotify) && !is_user_bot(id))
                    {
                        // Chat message
                        client_print_color(id, print_team_default, "^4[CS2 Economy]^1 %s purchased for ^4$%d^1 (^3$%d^1 refunded)",
                            szName, targetPrice, refundAmount);

                        // HUD display
                        set_hudmessage(50, 220, 50, -1.0, 0.72, 0, 0.05, 3.0, 0.1, 0.2);
                        ShowSyncHudMsg(id, g_HudSyncEco, "[CS2 Economy] %s: $%d ($%d Refunded)",
                            szName, targetPrice, refundAmount);
                    }
                }
            }
        }
    }

    g_iPendingBuyWeapon[id] = 0;
    g_flPendingBuyTime[id]  = 0.0;
    return HAM_IGNORED;
}

// ==============================================================================
// 2. AUTHENTIC CLIP SIZE OVERRIDES & MAGAZINE BEHAVIOR
// ==============================================================================

public Arsenal_Variants_DeployPost(iWpn)
{
    if (!get_pcvar_num(g_pCvarEnable) || !get_pcvar_num(g_pCvarClipOverride) || !pev_valid(iWpn))
        return HAM_IGNORED;

    new id = get_pdata_cbase(iWpn, m_pPlayer, extra_offset_weapon);
    if (!is_user_alive(id))
        return HAM_IGNORED;

    new weapon_id = cs_get_weapon_id(iWpn);
    if (!Arsenal_IsVariantWeaponId(weapon_id))
        return HAM_IGNORED;

    new szName[32], targetPrice, refundAmount, targetClip;

    if (!GetVariantInfo(id, weapon_id, szName, charsmax(szName), targetPrice, refundAmount, targetClip))
        return HAM_IGNORED;

    new curClip = cs_get_weapon_ammo(iWpn);
    new iClipInited = pev(iWpn, pev_iuser4);

    if (!iClipInited)
    {
        set_pev(iWpn, pev_iuser4, 1);

        switch (weapon_id)
        {
            case CSW_XM1014: // MAG-7: 5 clip
            {
                if (curClip > 5)
                {
                    new excess = curClip - 5;
                    cs_set_weapon_ammo(iWpn, 5);
                    cs_set_user_bpammo(id, CSW_XM1014, cs_get_user_bpammo(id, CSW_XM1014) + excess);
                }
            }
            case CSW_P90: // PP-Bizon: 64 clip
            {
                if (curClip < 64)
                {
                    new bp = cs_get_user_bpammo(id, CSW_P90);
                    new need = 64 - curClip;
                    new add = min(need, bp);
                    if (add > 0)
                    {
                        cs_set_weapon_ammo(iWpn, curClip + add);
                        cs_set_user_bpammo(id, CSW_P90, bp - add);
                    }
                    else if (curClip == 50)
                    {
                        cs_set_weapon_ammo(iWpn, 64);
                    }
                }
            }
            case CSW_M249: // Negev: 150 clip
            {
                if (curClip < 150)
                {
                    new bp = cs_get_user_bpammo(id, CSW_M249);
                    new need = 150 - curClip;
                    new add = min(need, bp);
                    if (add > 0)
                    {
                        cs_set_weapon_ammo(iWpn, curClip + add);
                        cs_set_user_bpammo(id, CSW_M249, bp - add);
                    }
                    else if (curClip == 100)
                    {
                        cs_set_weapon_ammo(iWpn, 150);
                    }
                }
            }
            case CSW_TMP, CSW_MAC10: // CZ75-Auto: 12 clip, 12 reserve
            {
                if (curClip > 12)
                {
                    cs_set_weapon_ammo(iWpn, 12);
                }
                if (cs_get_user_bpammo(id, weapon_id) > 12)
                {
                    cs_set_user_bpammo(id, weapon_id, 12);
                }
            }
            case CSW_P228: // CZ75-Auto (12 clip, 12 reserve) or Tec-9 (18 clip)
            {
                if (targetClip == 12)
                {
                    if (curClip > 12)
                    {
                        cs_set_weapon_ammo(iWpn, 12);
                    }
                    if (cs_get_user_bpammo(id, CSW_P228) > 12)
                    {
                        cs_set_user_bpammo(id, CSW_P228, 12);
                    }
                }
                else if (curClip < 18)
                {
                    new bp = cs_get_user_bpammo(id, CSW_P228);
                    new need = 18 - curClip;
                    new add = min(need, bp);
                    if (add > 0)
                    {
                        cs_set_weapon_ammo(iWpn, curClip + add);
                        cs_set_user_bpammo(id, CSW_P228, bp - add);
                    }
                    else if (curClip == 13)
                    {
                        cs_set_weapon_ammo(iWpn, 18);
                    }
                }
            }
        }
    }

    return HAM_IGNORED;
}

public OnVariantPostFrame_Post(iWpn)
{
    if (!get_pcvar_num(g_pCvarEnable) || !get_pcvar_num(g_pCvarClipOverride) || !pev_valid(iWpn))
        return HAM_IGNORED;

    new id = get_pdata_cbase(iWpn, m_pPlayer, extra_offset_weapon);
    if (!is_user_alive(id))
        return HAM_IGNORED;

    new weapon_id = cs_get_weapon_id(iWpn);
    if (!Arsenal_IsVariantWeaponId(weapon_id))
        return HAM_IGNORED;

    new szName[32], targetPrice, refundAmount, targetClip;

    if (!GetVariantInfo(id, weapon_id, szName, charsmax(szName), targetPrice, refundAmount, targetClip))
        return HAM_IGNORED;

    new iEntIdx = iWpn;
    if (iEntIdx >= sizeof(g_bWeaponInReload))
        iEntIdx = 0;

    // Shotgun MAG-7 Special Reload Loop
    if (weapon_id == CSW_XM1014)
    {
        new iSpecialReload = get_pdata_int(iWpn, m_iSpecialReload, extra_offset_weapon);
        if (iSpecialReload == 1)
        {
            // Box magazine fast load: fill up to 5 immediately and cock
            new cur = cs_get_weapon_ammo(iWpn);
            new bp = cs_get_user_bpammo(id, CSW_XM1014);
            new need = 5 - cur;
            if (need > 0 && bp > 0)
            {
                new add = min(need, bp);
                cs_set_weapon_ammo(iWpn, cur + add);
                cs_set_user_bpammo(id, CSW_XM1014, bp - add);
            }
            // Transition directly to pump/cock finish
            set_pdata_int(iWpn, m_iSpecialReload, 3, extra_offset_weapon);
            set_pdata_float(iWpn, m_flTimeWeaponIdle, 0.75, extra_offset_weapon);
            set_pdata_float(id, m_flNextAttack, 0.75, extra_offset_player);
        }
        else if (iSpecialReload == 2)
        {
            if (cs_get_weapon_ammo(iWpn) >= 5)
            {
                set_pdata_int(iWpn, m_iSpecialReload, 3, extra_offset_weapon);
            }
        }
        return HAM_IGNORED;
    }

    // Magazine Weapons Reload Finish Detection
    new inReload = get_pdata_int(iWpn, m_fInReload, extra_offset_weapon);
    if (inReload == 1)
    {
        if (iEntIdx > 0)
            g_bWeaponInReload[iEntIdx] = true;
    }
    else
    {
        if (iEntIdx > 0 && g_bWeaponInReload[iEntIdx])
        {
            g_bWeaponInReload[iEntIdx] = false;

            // Reload just completed! Adjust clip sizes
            switch (weapon_id)
            {
                case CSW_P90: // PP-Bizon: 64
                {
                    new cur = cs_get_weapon_ammo(iWpn);
                    if (cur < 64)
                    {
                        new bp = cs_get_user_bpammo(id, CSW_P90);
                        new need = 64 - cur;
                        new add = min(need, bp);
                        if (add > 0)
                        {
                            cs_set_weapon_ammo(iWpn, cur + add);
                            cs_set_user_bpammo(id, CSW_P90, bp - add);
                        }
                    }
                }
                case CSW_M249: // Negev: 150
                {
                    new cur = cs_get_weapon_ammo(iWpn);
                    if (cur < 150)
                    {
                        new bp = cs_get_user_bpammo(id, CSW_M249);
                        new need = 150 - cur;
                        new add = min(need, bp);
                        if (add > 0)
                        {
                            cs_set_weapon_ammo(iWpn, cur + add);
                            cs_set_user_bpammo(id, CSW_M249, bp - add);
                        }
                    }
                }
                case CSW_TMP, CSW_MAC10: // CZ75-Auto: 12
                {
                    new cur = cs_get_weapon_ammo(iWpn);
                    if (cur > 12)
                    {
                        new excess = cur - 12;
                        cs_set_weapon_ammo(iWpn, 12);
                        new bp = cs_get_user_bpammo(id, weapon_id);
                        cs_set_user_bpammo(id, weapon_id, min(12, bp + excess));
                    }
                }
                case CSW_P228: // CZ75-Auto (12) or Tec-9 (18)
                {
                    if (targetClip == 12)
                    {
                        new cur = cs_get_weapon_ammo(iWpn);
                        if (cur > 12)
                        {
                            new excess = cur - 12;
                            cs_set_weapon_ammo(iWpn, 12);
                            new bp = cs_get_user_bpammo(id, CSW_P228);
                            cs_set_user_bpammo(id, CSW_P228, min(12, bp + excess));
                        }
                    }
                    else
                    {
                        new cur = cs_get_weapon_ammo(iWpn);
                        if (cur < 18)
                        {
                            new bp = cs_get_user_bpammo(id, CSW_P228);
                            new need = 18 - cur;
                            new add = min(need, bp);
                            if (add > 0)
                            {
                                cs_set_weapon_ammo(iWpn, cur + add);
                                cs_set_user_bpammo(id, CSW_P228, bp - add);
                            }
                        }
                    }
                }
            }
        }
    }

    return HAM_IGNORED;
}

public Arsenal_Variants_CurWeapon(id)
{
    if (!get_pcvar_num(g_pCvarEnable) || !get_pcvar_num(g_pCvarClipOverride) || !is_user_alive(id))
        return;

    new weapon_id = read_data(2);
    new clip = read_data(3);

    // Enforce limits on active HUD update
    if (weapon_id == CSW_XM1014 && clip > 5)
    {
        if (IsPlayerUsingVariant(id, weapon_id, "mag7") || IsPlayerUsingVariant(id, weapon_id, "mag"))
        {
            new iWpn = get_pdata_cbase(id, 373, extra_offset_player);
            if (pev_valid(iWpn))
            {
                new excess = clip - 5;
                cs_set_weapon_ammo(iWpn, 5);
                cs_set_user_bpammo(id, CSW_XM1014, cs_get_user_bpammo(id, CSW_XM1014) + excess);
            }
        }
    }
    else if ((weapon_id == CSW_TMP || weapon_id == CSW_MAC10) && clip > 12)
    {
        if (IsPlayerUsingVariant(id, weapon_id, "cz") || IsPlayerUsingVariant(id, weapon_id, "cz75"))
        {
            new iWpn = get_pdata_cbase(id, 373, extra_offset_player);
            if (pev_valid(iWpn))
            {
                cs_set_weapon_ammo(iWpn, 12);
                if (cs_get_user_bpammo(id, weapon_id) > 12)
                    cs_set_user_bpammo(id, weapon_id, 12);
            }
        }
    }
}

// ==============================================================================
// 3. SHARED NATIVE & PUBLIC INTEROP API
// ==============================================================================

public bool:native_is_user_holding_variant(numParams)
{
    new id = get_param(1);
    new weapon_id = get_param(2);
    new szVariant[32];
    get_string(3, szVariant, charsmax(szVariant));

    return IsPlayerUsingVariant(id, weapon_id, szVariant);
}

public bool:native_get_user_weapon_variant(numParams)
{
    new id = get_param(1);
    new weapon_id = get_param(2);
    new maxlen = get_param(4);

    new szName[32], targetPrice, refundAmount, targetClip;
    if (GetVariantInfo(id, weapon_id, szName, charsmax(szName), targetPrice, refundAmount, targetClip))
    {
        set_string(3, szName, maxlen);
        return true;
    }
    return false;
}

public native_get_variant_price(numParams)
{
    new weapon_id = get_param(1);
    new szVariant[32];
    get_string(2, szVariant, charsmax(szVariant));

    return LookupVariantPrice(weapon_id, szVariant);
}

// Public functions for dynamic callfunc interoperability across AMXX modules
public bool:cs2_is_user_holding_variant(id, weapon_id, const szVariant[])
{
    return IsPlayerUsingVariant(id, weapon_id, szVariant);
}

public bool:cs2_get_user_weapon_variant(id, weapon_id, szBuffer[], maxlen)
{
    new szName[32], targetPrice, refundAmount, targetClip;
    if (GetVariantInfo(id, weapon_id, szName, charsmax(szName), targetPrice, refundAmount, targetClip))
    {
        copy(szBuffer, maxlen, szName);
        return true;
    }
    return false;
}

public cs2_get_variant_price(weapon_id, const szVariant[])
{
    return LookupVariantPrice(weapon_id, szVariant);
}
