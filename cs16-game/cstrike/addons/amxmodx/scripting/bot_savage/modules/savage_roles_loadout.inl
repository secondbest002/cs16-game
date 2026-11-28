#if defined _savage_roles_loadout_included
    #endinput
#endif
#define _savage_roles_loadout_included

stock bool:IsDeathmatchActive()
{
    static pCvar_RedmActive;
    if (!pCvar_RedmActive)
        pCvar_RedmActive = get_cvar_pointer("redm_active");

    return (pCvar_RedmActive != 0 && get_pcvar_num(pCvar_RedmActive) == 1);
}

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
    }

    new CsTeams:csteam = cs_get_user_team(id);
    if (csteam == CS_TEAM_T || csteam == CS_TEAM_CT || csteam == CS_TEAM_SPECTATOR)
        return csteam;

    return CS_TEAM_UNASSIGNED;
}

stock bool:IsFreeForAllActive()
{
    static pCvar_Ffa;
    if (!pCvar_Ffa)
        pCvar_Ffa = get_cvar_pointer("redm_freeforall");

    if (pCvar_Ffa && get_pcvar_num(pCvar_Ffa) == 1)
        return true;

    return (get_cvar_num("mp_friendlyfire") == 1 && get_cvar_num("yb_csdm_mode") == 2);
}

stock GetMaxBpammo(weapon_id)
{
    switch (weapon_id)
    {
        case CSW_AWP: return 30;
        case CSW_AK47, CSW_M4A1, CSW_GALIL, CSW_FAMAS, CSW_AUG, CSW_SG552, CSW_SCOUT, CSW_G3SG1, CSW_SG550: return 90;
        case CSW_MP5NAVY, CSW_TMP, CSW_MAC10: return 120;
        case CSW_UMP45, CSW_P90: return 100;
        case CSW_M249: return 200;
        case CSW_XM1014, CSW_M3: return 32;
        case CSW_DEAGLE: return 35;
        case CSW_USP, CSW_GLOCK18, CSW_FIVESEVEN: return 100;
        case CSW_P228: return 52;
        case CSW_ELITE: return 120;
    }
    return 90;
}

stock AssignBotRoleDeterministic(id)
{
    new CsTeams:iTeam = safe_cs_get_user_team(id);
    if (iTeam != CS_TEAM_T && iTeam != CS_TEAM_CT)
    {
        g_iPlayerRole[id] = ROLE_ENTRY + (id % 5);
        return;
    }

    // Hitung role apa saja yang sudah terpakai di tim yang sama
    new iRoleCount[6];
    for (new i = 1; i <= MaxClients; i++)
    {
        if (i == id || !is_user_connected(i))
            continue;

        if (safe_cs_get_user_team(i) == iTeam && g_iPlayerRole[i] != ROLE_NONE)
        {
            iRoleCount[g_iPlayerRole[i]]++;
        }
    }

    // Cari role dengan jumlah pemegang paling sedikit di tim tersebut
    new iBestRole = ROLE_ENTRY;
    new iMinCount = 999;

    for (new r = ROLE_ENTRY; r <= ROLE_IGL; r++)
    {
        if (iRoleCount[r] < iMinCount)
        {
            iMinCount = iRoleCount[r];
            iBestRole = r;
        }
    }

    g_iPlayerRole[id] = iBestRole;
}

stock GetTeamAverageMoney(CsTeams:iTeam)
{
    new iTotal = 0;
    new iCount = 0;

    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_alive(i) && safe_cs_get_user_team(i) == iTeam)
        {
            iTotal += cs_get_user_money(i);
            iCount++;
        }
    }

    return (iCount > 0) ? (iTotal / iCount) : 0;
}

stock EquipBotByRole(id, bool:bFreeLoadout, bool:bAllowUtility = true)
{
    if (!xash_is_bot_combat_ready(id) || IsKnifeRoundActive())
        return 0;

    // Defense-in-depth: never grant grenades or defuse kits during Warmup DM or Pure DM
    if (IsDeathmatchActive() || IsWarmupActive())
    {
        bAllowUtility = false;
    }

    new CsTeams:iTeam = safe_cs_get_user_team(id);
    if (iTeam != CS_TEAM_T && iTeam != CS_TEAM_CT)
        return 0;

    if (g_iPlayerRole[id] == ROLE_NONE)
    {
        AssignBotRoleDeterministic(id);
    }

    new iRole = g_iPlayerRole[id];
    new iMoney = bFreeLoadout ? 16000 : cs_get_user_money(id);
    new iEconMode = bFreeLoadout ? ECON_FULL : g_iTeamEconomyCall[iTeam];

    new iRoundNum = cs2_state_get_match_round();
    if (iRoundNum <= 0)
        iRoundNum = g_iRoundCount;
    new iRoundInHalf = (iRoundNum > 0) ? (((iRoundNum - 1) % 12) + 1) : 1;
    new bool:bIsPistolRound = (!bFreeLoadout && !IsWarmupActive() && !IsDeathmatchActive() && iRoundInHalf == 1);

    // Pada Live Match (bFreeLoadout == false), patuhi perintah ekonomi IGL & Team Eco Guard
    new bool:bFullBuy = false;
    new bool:bForceBuy = false;
    if (bFreeLoadout)
    {
        bFullBuy = true;
    }
    else if (!bIsPistolRound)
    {
        if (iEconMode == ECON_ECO)
        {
            bFullBuy = false;
            bForceBuy = false;
        }
        else if (iEconMode == ECON_HALF)
        {
            bFullBuy = false;
            bForceBuy = (iMoney >= 2000);
        }
        else if (iEconMode == ECON_FORCE)
        {
            bFullBuy = (iMoney >= 4200);
            bForceBuy = (!bFullBuy && iMoney >= 1800);
        }
        else
        {
            bFullBuy = (iMoney >= 4200);
            if (!bFullBuy && iMoney >= 2000)
            {
                bForceBuy = (iEconMode == ECON_FULL || iRoundInHalf == 12 || GetTeamAverageMoney(iTeam) >= 3200);
            }
        }
    }

    // Cek apakah bot sudah membeli atau memiliki senjata slot1 (primary) pada spawn (misal via native YaPB buy)
    new iExistingPrimary = GetUserPrimaryWeapon(id);
    new bool:bHasPrimary = (!bFreeLoadout && iExistingPrimary > 0);

    // 1. Simpan status kepemilikan C4 sebelum strip jika tidak memiliki senjata utama
    new bool:bHadC4 = (bAllowUtility && iTeam == CS_TEAM_T && user_has_weapon(id, CSW_C4) != 0 && !IsDeathmatchActive() && !IsWarmupActive());

    // 2. Jika bot TIDAK memiliki primary weapon (atau mode Free Loadout Warmup/DM), bersihkan inventaris lama
    // Jika bot SUDAH membeli primary weapon pada Live Match spawn, JANGAN strip! Pertahankan senjatanya.
    if (!bHasPrimary)
    {
        strip_user_weapons(id);
        cs2_api_clear_user_molotov(id);
        give_item(id, "weapon_knife");

        // Kembalikan C4 jika sedang dalam Live Match MR12
        if (bHadC4)
        {
            give_item(id, "weapon_c4");
            cs_set_user_plant(id, 1, 1);
        }
    }

    if (!bAllowUtility)
    {
        cs2_api_clear_user_molotov(id);
        cs_set_user_bpammo(id, CSW_HEGRENADE, 0);
        cs_set_user_bpammo(id, CSW_FLASHBANG, 0);
        cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 0);
        cs_set_user_plant(id, 0, 0);
    }

    // Pistol Round ($800 budget realism: Round 1 & Round 13)
    if (bIsPistolRound && !bHasPrimary)
    {
        cs_set_user_defuse(id, 0);
        new szPistol[32], iPistolCSW, iPistolAmmo, iPistolCost;

        switch (iRole)
        {
            case ROLE_ENTRY, ROLE_AWPER:
            {
                copy(szPistol, charsmax(szPistol), "weapon_deagle");
                iPistolCSW = CSW_DEAGLE;
                iPistolAmmo = 35;
                iPistolCost = 650;
                cs_set_user_armor(id, 0, CS_ARMOR_NONE);
            }
            case ROLE_SUPPORT:
            {
                copy(szPistol, charsmax(szPistol), "weapon_p228");
                iPistolCSW = CSW_P228;
                iPistolAmmo = 52;
                iPistolCost = 600;
                cs_set_user_armor(id, 0, CS_ARMOR_NONE);
                if (bAllowUtility)
                {
                    give_item(id, "weapon_flashbang");
                    cs_set_user_bpammo(id, CSW_FLASHBANG, 1);
                    iPistolCost += 200;
                }
            }
            default:
            {
                copy(szPistol, charsmax(szPistol), (iTeam == CS_TEAM_T) ? "weapon_glock18" : "weapon_usp");
                iPistolCSW = (iTeam == CS_TEAM_T) ? CSW_GLOCK18 : CSW_USP;
                iPistolAmmo = (iTeam == CS_TEAM_T) ? 120 : 100;
                iPistolCost = 650;
                cs_set_user_armor(id, 100, CS_ARMOR_KEVLAR);
            }
        }

        new iPistolEnt = give_item(id, szPistol);
        cs_set_user_bpammo(id, iPistolCSW, iPistolAmmo);
        if (iPistolCSW == CSW_USP)
        {
            if (!pev_valid(iPistolEnt))
                iPistolEnt = xash_find_weapon_ent(id, "weapon_usp");
            if (pev_valid(iPistolEnt))
                cs_set_weapon_silen(iPistolEnt, 1, 0);
        }
        xash_force_deploy_weapon(id, iPistolEnt, szPistol, iPistolCSW);
        cs_set_user_money(id, max(0, iMoney - iPistolCost), 1);
        return 1;
    }

    // 3. Berikan Kevlar + Helmet (Eco penuh tidak mendapat armor gratis)
    if (bFullBuy || bForceBuy)
    {
        cs_set_user_armor(id, 100, CS_ARMOR_VESTHELM);
    }
    else if (!bHasPrimary)
    {
        cs_set_user_armor(id, 0, CS_ARMOR_NONE);
    }

    // 4. Defuse Kit Policy:
    // Pastikan cs_set_user_defuse TIDAK PERNAH dipanggil saat Deathmatch, Warmup, Knife Round, atau Free For All.
    // Defuse kit hanya boleh ada saat Live Match MR12 ketika tim sedang membeli!
    new bool:bAllowDefuse = (bAllowUtility && !IsDeathmatchActive() && !IsWarmupActive() && !IsKnifeRoundActive() && !IsFreeForAllActive());
    if (iTeam == CS_TEAM_CT)
    {
        if (bAllowDefuse && (bFreeLoadout || ((bFullBuy || bForceBuy) && (iRole == ROLE_SUPPORT || iRole == ROLE_IGL || iMoney >= 600))))
        {
            cs_set_user_defuse(id, 1, 0, 160, 0, "defuser", 0);
        }
        else
        {
            cs_set_user_defuse(id, 0);
        }
    }

    // 5. Tentukan Senjata Utama, Pistol, dan Utility Berdasarkan Role (Tier-1 + MAC-10/MP5/P250 Half-Buy)
    new szPrimary[32], iPrimaryCSW, iPrimaryAmmo;
    new szSecondary[32], iSecondaryCSW, iSecondaryAmmo;
    new bool:bAttachSilencer = false;

    switch (iRole)
    {
        case ROLE_ENTRY:
        {
            if (bFullBuy)
            {
                formatex(szPrimary, charsmax(szPrimary), (iTeam == CS_TEAM_T) ? "weapon_ak47" : "weapon_m4a1");
                iPrimaryCSW  = (iTeam == CS_TEAM_T) ? CSW_AK47 : CSW_M4A1;
                iPrimaryAmmo = 90;
            }
            else if (bForceBuy)
            {
                if (iTeam == CS_TEAM_T)
                {
                    new bool:bGalil = (iMoney >= 2700);
                    copy(szPrimary, charsmax(szPrimary), bGalil ? "weapon_galil" : "weapon_mac10");
                    iPrimaryCSW  = bGalil ? CSW_GALIL : CSW_MAC10;
                    iPrimaryAmmo = bGalil ? 90 : 100;
                }
                else
                {
                    new bool:bFamas = (iMoney >= 2950);
                    copy(szPrimary, charsmax(szPrimary), bFamas ? "weapon_famas" : "weapon_mp5navy");
                    iPrimaryCSW  = bFamas ? CSW_FAMAS : CSW_MP5NAVY;
                    iPrimaryAmmo = bFamas ? 90 : 120;
                }
            }

            if (bFullBuy || bForceBuy || iMoney >= 1800)
            {
                copy(szSecondary, charsmax(szSecondary), "weapon_deagle");
                iSecondaryCSW  = CSW_DEAGLE;
                iSecondaryAmmo = 35;
            }
            else
            {
                copy(szSecondary, charsmax(szSecondary), (iTeam == CS_TEAM_T) ? "weapon_glock18" : "weapon_usp");
                iSecondaryCSW  = (iTeam == CS_TEAM_T) ? CSW_GLOCK18 : CSW_USP;
                iSecondaryAmmo = (iTeam == CS_TEAM_T) ? 120 : 100;
            }

            if (bAllowUtility)
            {
                if (bFullBuy)
                {
                    // Utility Entry (Full Buy): Molotov + 1x Flashbang
                    give_item(id, "weapon_flashbang");
                    cs_set_user_bpammo(id, CSW_FLASHBANG, 1);
                    if (!cs2_api_give_user_molotov(id, bHasPrimary && !bFreeLoadout))
                    {
                        give_item(id, "weapon_hegrenade");
                        cs_set_user_bpammo(id, CSW_HEGRENADE, 1);
                    }
                }
                else if (bForceBuy)
                {
                    // Utility Entry (Force Buy): 1x Flashbang
                    give_item(id, "weapon_flashbang");
                    cs_set_user_bpammo(id, CSW_FLASHBANG, 1);
                }
            }
        }
        case ROLE_AWPER:
        {
            if (bFullBuy)
            {
                formatex(szPrimary, charsmax(szPrimary), "weapon_awp");
                iPrimaryCSW  = CSW_AWP;
                iPrimaryAmmo = 30;
            }
            else if (bForceBuy)
            {
                if (iEconMode == ECON_FORCE && iMoney >= 2750)
                {
                    copy(szPrimary, charsmax(szPrimary), "weapon_scout");
                    iPrimaryCSW  = CSW_SCOUT;
                    iPrimaryAmmo = 90;
                }
                else if (iTeam == CS_TEAM_T)
                {
                    new bool:bGalil = (iMoney >= 2700);
                    copy(szPrimary, charsmax(szPrimary), bGalil ? "weapon_galil" : "weapon_mac10");
                    iPrimaryCSW  = bGalil ? CSW_GALIL : CSW_MAC10;
                    iPrimaryAmmo = bGalil ? 90 : 100;
                }
                else
                {
                    new bool:bFamas = (iMoney >= 2950);
                    copy(szPrimary, charsmax(szPrimary), bFamas ? "weapon_famas" : "weapon_mp5navy");
                    iPrimaryCSW  = bFamas ? CSW_FAMAS : CSW_MP5NAVY;
                    iPrimaryAmmo = bFamas ? 90 : 120;
                }
            }

            if (bFullBuy || bForceBuy || iMoney >= 1800)
            {
                copy(szSecondary, charsmax(szSecondary), "weapon_deagle");
                iSecondaryCSW  = CSW_DEAGLE;
                iSecondaryAmmo = 35;
            }
            else
            {
                copy(szSecondary, charsmax(szSecondary), (iTeam == CS_TEAM_T) ? "weapon_glock18" : "weapon_usp");
                iSecondaryCSW  = (iTeam == CS_TEAM_T) ? CSW_GLOCK18 : CSW_USP;
                iSecondaryAmmo = (iTeam == CS_TEAM_T) ? 120 : 100;
            }

            if (bAllowUtility)
            {
                if (bFullBuy)
                {
                    // Utility AWPer (Full Buy): 1x Flashbang + 1x Smoke
                    give_item(id, "weapon_flashbang");
                    cs_set_user_bpammo(id, CSW_FLASHBANG, 1);
                    give_item(id, "weapon_smokegrenade");
                    cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 1);
                }
                else if (bForceBuy)
                {
                    // Utility AWPer (Force Buy): 1x Flashbang
                    give_item(id, "weapon_flashbang");
                    cs_set_user_bpammo(id, CSW_FLASHBANG, 1);
                }
            }
        }
        case ROLE_SUPPORT:
        {
            if (bFullBuy)
            {
                formatex(szPrimary, charsmax(szPrimary), (iTeam == CS_TEAM_T) ? "weapon_ak47" : "weapon_m4a1");
                iPrimaryCSW     = (iTeam == CS_TEAM_T) ? CSW_AK47 : CSW_M4A1;
                iPrimaryAmmo    = 90;
                bAttachSilencer = (iTeam == CS_TEAM_CT);
            }
            else if (bForceBuy)
            {
                copy(szPrimary, charsmax(szPrimary), (iTeam == CS_TEAM_T) ? "weapon_mac10" : "weapon_mp5navy");
                iPrimaryCSW  = (iTeam == CS_TEAM_T) ? CSW_MAC10 : CSW_MP5NAVY;
                iPrimaryAmmo = (iTeam == CS_TEAM_T) ? 100 : 120;
            }

            if (bFullBuy || bForceBuy || iMoney >= 1500)
            {
                copy(szSecondary, charsmax(szSecondary), "weapon_p228");
                iSecondaryCSW  = CSW_P228;
                iSecondaryAmmo = 52;
            }
            else
            {
                copy(szSecondary, charsmax(szSecondary), (iTeam == CS_TEAM_T) ? "weapon_glock18" : "weapon_usp");
                iSecondaryCSW  = (iTeam == CS_TEAM_T) ? CSW_GLOCK18 : CSW_USP;
                iSecondaryAmmo = (iTeam == CS_TEAM_T) ? 120 : 100;
            }

            if (bAllowUtility)
            {
                if (bFullBuy)
                {
                    // Utility Support (Full Buy): Molotov + 1x Smoke + 2x Flashbang
                    give_item(id, "weapon_smokegrenade");
                    cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 1);
                    give_item(id, "weapon_flashbang");
                    cs_set_user_bpammo(id, CSW_FLASHBANG, 2);
                    if (!cs2_api_give_user_molotov(id, bHasPrimary && !bFreeLoadout))
                    {
                        give_item(id, "weapon_hegrenade");
                        cs_set_user_bpammo(id, CSW_HEGRENADE, 1);
                    }
                }
                else if (bForceBuy)
                {
                    // Utility Support (Force Buy): 1x Smoke + 1x Flashbang
                    give_item(id, "weapon_smokegrenade");
                    cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 1);
                    give_item(id, "weapon_flashbang");
                    cs_set_user_bpammo(id, CSW_FLASHBANG, 1);
                }
            }
        }
        case ROLE_LURKER:
        {
            if (bFullBuy)
            {
                formatex(szPrimary, charsmax(szPrimary), (iTeam == CS_TEAM_T) ? "weapon_ak47" : "weapon_m4a1");
                iPrimaryCSW     = (iTeam == CS_TEAM_T) ? CSW_AK47 : CSW_M4A1;
                iPrimaryAmmo    = 90;
                bAttachSilencer = (iTeam == CS_TEAM_CT);
            }
            else if (bForceBuy)
            {
                copy(szPrimary, charsmax(szPrimary), (iTeam == CS_TEAM_T) ? "weapon_mac10" : "weapon_mp5navy");
                iPrimaryCSW  = (iTeam == CS_TEAM_T) ? CSW_MAC10 : CSW_MP5NAVY;
                iPrimaryAmmo = (iTeam == CS_TEAM_T) ? 100 : 120;
            }

            if (bForceBuy || iMoney >= 1500)
            {
                copy(szSecondary, charsmax(szSecondary), "weapon_p228");
                iSecondaryCSW  = CSW_P228;
                iSecondaryAmmo = 52;
            }
            else
            {
                copy(szSecondary, charsmax(szSecondary), (iTeam == CS_TEAM_T) ? "weapon_glock18" : "weapon_usp");
                iSecondaryCSW  = (iTeam == CS_TEAM_T) ? CSW_GLOCK18 : CSW_USP;
                iSecondaryAmmo = (iTeam == CS_TEAM_T) ? 120 : 100;
            }

            if (bAllowUtility)
            {
                if (bFullBuy)
                {
                    // Utility Lurker (Full Buy): 50% Molotov / 50% HE + 1x Smoke + 1x Flashbang
                    give_item(id, "weapon_smokegrenade");
                    cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 1);
                    give_item(id, "weapon_flashbang");
                    cs_set_user_bpammo(id, CSW_FLASHBANG, 1);

                    new bool:bGiveMolo = ((id + g_iRoundCount) % 2 == 0) || cs2_api_get_user_molotov(id);
                    if (bGiveMolo)
                    {
                        if (!cs2_api_give_user_molotov(id, bHasPrimary && !bFreeLoadout))
                        {
                            give_item(id, "weapon_hegrenade");
                            cs_set_user_bpammo(id, CSW_HEGRENADE, 1);
                        }
                    }
                    else if (!cs2_api_get_user_molotov(id))
                    {
                        give_item(id, "weapon_hegrenade");
                        cs_set_user_bpammo(id, CSW_HEGRENADE, 1);
                    }
                }
                else if (bForceBuy)
                {
                    // Utility Lurker (Force Buy): 1x Smoke
                    give_item(id, "weapon_smokegrenade");
                    cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 1);
                }
            }
        }
        case ROLE_IGL:
        {
            if (bFullBuy)
            {
                formatex(szPrimary, charsmax(szPrimary), (iTeam == CS_TEAM_T) ? "weapon_ak47" : "weapon_m4a1");
                iPrimaryCSW  = (iTeam == CS_TEAM_T) ? CSW_AK47 : CSW_M4A1;
                iPrimaryAmmo = 90;
            }
            else if (bForceBuy)
            {
                formatex(szPrimary, charsmax(szPrimary), "weapon_ump45");
                iPrimaryCSW  = CSW_UMP45;
                iPrimaryAmmo = 100;
            }

            if (bFullBuy)
            {
                copy(szSecondary, charsmax(szSecondary), "weapon_deagle");
                iSecondaryCSW  = CSW_DEAGLE;
                iSecondaryAmmo = 35;
            }
            else if (bForceBuy || iMoney >= 1500)
            {
                copy(szSecondary, charsmax(szSecondary), "weapon_p228");
                iSecondaryCSW  = CSW_P228;
                iSecondaryAmmo = 52;
            }
            else
            {
                copy(szSecondary, charsmax(szSecondary), (iTeam == CS_TEAM_T) ? "weapon_glock18" : "weapon_usp");
                iSecondaryCSW  = (iTeam == CS_TEAM_T) ? CSW_GLOCK18 : CSW_USP;
                iSecondaryAmmo = (iTeam == CS_TEAM_T) ? 120 : 100;
            }

            if (bAllowUtility)
            {
                if (bFullBuy)
                {
                    // Utility IGL (Full Buy): 1x HE Grenade + 1x Smoke + 2x Flashbang
                    give_item(id, "weapon_smokegrenade");
                    cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 1);
                    give_item(id, "weapon_flashbang");
                    cs_set_user_bpammo(id, CSW_FLASHBANG, 2);
                    if (!cs2_api_get_user_molotov(id))
                    {
                        give_item(id, "weapon_hegrenade");
                        cs_set_user_bpammo(id, CSW_HEGRENADE, 1);
                    }
                }
                else if (bForceBuy)
                {
                    // Utility IGL (Force Buy): 1x Flashbang
                    give_item(id, "weapon_flashbang");
                    cs_set_user_bpammo(id, CSW_FLASHBANG, 1);
                }
            }
        }
    }

    // 6. Handle Primary & Secondary Weapons
    if (bHasPrimary)
    {
        // Bot sudah memiliki primary: berikan full bpammo
        cs_set_user_bpammo(id, iExistingPrimary, GetMaxBpammo(iExistingPrimary));

        // Lengkapi secondary jika belum punya pistol yang memadai
        new iCurSecondary = GetUserSecondaryWeapon(id);
        if (iCurSecondary > 0)
        {
            cs_set_user_bpammo(id, iCurSecondary, GetMaxBpammo(iCurSecondary));
        }
        else
        {
            give_item(id, szSecondary);
            cs_set_user_bpammo(id, iSecondaryCSW, iSecondaryAmmo);
        }

        // Cari entity primary dan force deploy slot1
        new szExistingPrimary[32];
        get_weaponname(iExistingPrimary, szExistingPrimary, charsmax(szExistingPrimary));
        new iWeapEnt = xash_find_weapon_ent(id, szExistingPrimary);
        xash_force_deploy_weapon(id, iWeapEnt, szExistingPrimary, iExistingPrimary);
    }
    else
    {
        // Bot belum memiliki primary: berikan pistol role terlebih dahulu
        new iPistolEnt = give_item(id, szSecondary);
        cs_set_user_bpammo(id, iSecondaryCSW, iSecondaryAmmo);
        if (iSecondaryCSW == CSW_USP)
        {
            if (!pev_valid(iPistolEnt))
            {
                iPistolEnt = xash_find_weapon_ent(id, "weapon_usp");
            }
            if (pev_valid(iPistolEnt))
            {
                cs_set_weapon_silen(iPistolEnt, 1, 0);
            }
        }

        // Suntikkan senjata utama yang ditentukan role terakhir agar langsung jadi senjata aktif
        if (szPrimary[0] != EOS)
        {
            new iWeapEnt = give_item(id, szPrimary);
            cs_set_user_bpammo(id, iPrimaryCSW, iPrimaryAmmo);

            if (!pev_valid(iWeapEnt))
            {
                iWeapEnt = xash_find_weapon_ent(id, szPrimary);
            }

            if (iPrimaryCSW == CSW_M4A1 && pev_valid(iWeapEnt))
            {
                set_pev(iWeapEnt, pev_iuser1, bAttachSilencer ? 2 : 1); // 2 = M4A1-S, 1 = M4A4
                cs_set_weapon_silen(iWeapEnt, bAttachSilencer ? 1 : 0, 0);
            }

            xash_force_deploy_weapon(id, iWeapEnt, szPrimary, iPrimaryCSW);
        }
        else
        {
            xash_force_deploy_weapon(id, iPistolEnt, szSecondary, iSecondaryCSW);
        }

        // Kurangi uang hanya saat Live Match (bFreeLoadout == false) dan bot belum beli sendiri
        if (!bFreeLoadout)
        {
            new iCost = 0;
            if (bFullBuy)
                iCost = 4200;
            else if (bForceBuy)
                iCost = (iPrimaryCSW == CSW_MAC10 || iPrimaryCSW == CSW_MP5NAVY || iPrimaryCSW == CSW_UMP45) ? 2100 : 2800;
            else
                iCost = (iSecondaryCSW == CSW_DEAGLE) ? 650 : ((iSecondaryCSW == CSW_P228) ? 300 : 0);

            cs_set_user_money(id, max(0, iMoney - iCost), 1);
        }
    }

    return 1;
}


// ==============================================================================
// 3. ROLE LOADOUT ENFORCEMENT (Equips bots according to assigned role)
// ==============================================================================
public Task_EnforceRoleLoadouts()
{
    if (IsKnifeRoundActive() || !get_pcvar_num(g_pCvarRoles))
        return;

    // First ensure any dropped weapons in spawn are claimed before spending team cash!
    ScanAndClaimSpawnDroppedWeapons(CS_TEAM_T);
    ScanAndClaimSpawnDroppedWeapons(CS_TEAM_CT);

    for (new id = 1; id <= MaxClients; id++)
    {
        if (is_user_alive(id) && is_user_bot(id))
        {
            EquipBotByRole(id, false, true);
        }
    }
}


// ==============================================================================
// 5. ROLE SELECTION MENU
// ==============================================================================
public OnPlayerSpawnPost(id)
{
    if (!is_user_alive(id))
        return HAM_IGNORED;

    ResetBotState(id);

    if (is_user_bot(id))
    {
        // Pastikan bot selalu memiliki role begitu spawn
        if (g_iPlayerRole[id] == ROLE_NONE)
        {
            AssignBotRoleDeterministic(id);
        }
    }
    else
    {
        // Never show role menu during Warmup, Deathmatch, or Knife Round
        if (!IsWarmupActive() && !IsDeathmatchActive() && !IsKnifeRoundActive() && !g_bRoleSelected[id] && get_pcvar_num(g_pCvarRoles))
        {
            set_task(0.5, "Task_ShowRoleMenuDelayed", id + TASK_SV_ROLE_MENU);
        }
    }

    return HAM_IGNORED;
}

public Task_ShowRoleMenuDelayed(taskid)
{
    new id = taskid - TASK_SV_ROLE_MENU;
    if (is_user_connected(id) && !is_user_bot(id))
    {
        ShowRoleMenu(id);
    }
}

public Cmd_OpenRoleMenu(id)
{
    if (is_user_connected(id))
    {
        ShowRoleMenu(id);
    }
    return PLUGIN_HANDLED;
}

public ShowRoleMenu(id)
{
    new menu = menu_create("\y[Competitive CS] Choose Your Role:", "HandleRoleMenu");

    menu_additem(menu, "\w🎯 Entry Fragger \d(First Duel / Aggressive Peek)", "1");
    menu_additem(menu, "\w🔭 AWPer \d(Sniper / Long Angles / Area Denial)", "2");
    menu_additem(menu, "\w🛡️ Support \d(Utility / Drops / Trade Kills)", "3");
    menu_additem(menu, "\w🥷 Lurker / Anchor \d(Flanker / Stealth / Site Hold)", "4");
    menu_additem(menu, "\w👑 IGL \d(Tactician / Calls Strategies in Freeze)", "5");

    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, menu, 0);
}

public HandleRoleMenu(id, menu, item)
{
    if (item == MENU_EXIT)
    {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new szData[6], szName[64], iAccess, iCallback;
    menu_item_getinfo(menu, item, iAccess, szData, charsmax(szData), szName, charsmax(szName), iCallback);
    new iRole = str_to_num(szData);

    if (iRole >= ROLE_ENTRY && iRole <= ROLE_IGL)
    {
        g_iPlayerRole[id] = iRole;
        g_bRoleSelected[id] = true;

        new szPlayerName[32];
        get_user_name(id, szPlayerName, charsmax(szPlayerName));

        client_print_color(0, print_team_default, "^4[Role System]^3 %s^1 selected role: ^4%s^1!", szPlayerName, g_szRoleNames[iRole]);

        new CsTeams:team = cs_get_user_team(id);
        if (team == CS_TEAM_T || team == CS_TEAM_CT)
        {
            BalanceTeamRoles(team);
        }
    }

    menu_destroy(menu);
    return PLUGIN_HANDLED;
}

BalanceTeamRoles(CsTeams:team)
{
    if (!get_pcvar_num(g_pCvarRoles))
        return;

    new iHumanAwpCount = 0;
    new iHumanIglCount = 0;

    for (new i = 1; i <= 32; i++)
    {
        if (is_user_connected(i) && !is_user_bot(i) && cs_get_user_team(i) == team)
        {
            if (g_iPlayerRole[i] == ROLE_AWPER) iHumanAwpCount++;
            if (g_iPlayerRole[i] == ROLE_IGL)   iHumanIglCount++;
        }
    }

    new bool:bHasIgl = (iHumanIglCount > 0);
    new bool:bHasAwp = (iHumanAwpCount > 0);
    new bool:bHasSupport = false;
    new bool:bHasLurker = false;

    for (new i = 1; i <= 32; i++)
    {
        if (is_user_connected(i) && is_user_bot(i) && cs_get_user_team(i) == team)
        {
            if (!bHasIgl)
            {
                g_iPlayerRole[i] = ROLE_IGL;
                bHasIgl = true;
            }
            else if (!bHasAwp)
            {
                g_iPlayerRole[i] = ROLE_AWPER;
                bHasAwp = true;
            }
            else if (!bHasSupport)
            {
                g_iPlayerRole[i] = ROLE_SUPPORT;
                bHasSupport = true;
            }
            else if (!bHasLurker)
            {
                g_iPlayerRole[i] = ROLE_LURKER;
                bHasLurker = true;
            }
            else
            {
                g_iPlayerRole[i] = ROLE_ENTRY;
            }
            g_bRoleSelected[i] = true;
        }
    }
}

// ==============================================================================
// M4A4 vs M4A1-S DETECTION HELPERS
// ==============================================================================
stock fm_find_weapon_ent_by_owner(id, const classname[])
{
    new ent = -1;
    while ((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", classname)) > 0)
    {
        if (pev(ent, pev_owner) == id || get_pdata_cbase(ent, 41, 4) == id)
            return ent;
    }
    return 0;
}

stock bool:IsPlayerM4A4(id)
{
    if (id < 1 || id > 32 || !is_user_connected(id))
        return false;

    // 1. Check currently carried weapon entity pev_iuser1 first (1 = M4A4, 2 = M4A1-S)
    new iWpn = fm_find_weapon_ent_by_owner(id, "weapon_m4a1");
    if (pev_valid(iWpn))
    {
        new iVar = pev(iWpn, pev_iuser1);
        if (iVar == 1) return true;
        if (iVar == 2) return false;
    }

    // 2. Check cs2_loadout/cs2_arsenal native query for player's active loadout
    new bool:bAvailable = false;
    new bool:bIsM4A4 = cs2_api_is_m4a4(id, bAvailable);
    if (bAvailable)
        return bIsM4A4;

    // 3. Fallback: Entry Fraggers & Site Anchors equip M4A4
    if (g_iPlayerRole[id] == ROLE_ENTRY || g_iPlayerRole[id] == ROLE_LURKER)
        return true;

    return false;
}
