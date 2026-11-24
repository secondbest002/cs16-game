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

    // Pada Live Match (bFreeLoadout == false), jika uang < $1500 (Full Eco), hanya beri utility/pistol
    new bool:bFullBuy  = (bFreeLoadout || iMoney >= 4200);
    new bool:bForceBuy = (!bFullBuy && iMoney >= 2200);

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
        cs_set_user_bpammo(id, CSW_HEGRENADE, 0);
        cs_set_user_bpammo(id, CSW_FLASHBANG, 0);
        cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 0);
        cs_set_user_plant(id, 0, 0);
    }

    // 3. Berikan Kevlar + Helmet
    if (bFullBuy || bForceBuy)
    {
        cs_set_user_armor(id, 100, CS_ARMOR_VESTHELM);
    }
    else
    {
        cs_set_user_armor(id, 100, CS_ARMOR_KEVLAR);
    }

    // 4. Defuse Kit Policy:
    // Pastikan cs_set_user_defuse TIDAK PERNAH dipanggil saat Deathmatch, Warmup, Knife Round, atau Free For All.
    // Defuse kit hanya boleh ada saat Live Match MR12!
    new bool:bAllowDefuse = (bAllowUtility && !IsDeathmatchActive() && !IsWarmupActive() && !IsKnifeRoundActive() && !IsFreeForAllActive());
    if (iTeam == CS_TEAM_CT)
    {
        if (bAllowDefuse && (bFreeLoadout || iRole == ROLE_SUPPORT || iRole == ROLE_IGL || iMoney >= 200))
        {
            cs_set_user_defuse(id, 1, 0, 160, 0, "defuser", 0);
        }
        else
        {
            cs_set_user_defuse(id, 0);
        }
    }

    // 5. Tentukan Senjata Utama, Pistol, dan Utility Berdasarkan Role
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
                formatex(szPrimary, charsmax(szPrimary), (iTeam == CS_TEAM_T) ? "weapon_galil" : "weapon_famas");
                iPrimaryCSW  = (iTeam == CS_TEAM_T) ? CSW_GALIL : CSW_FAMAS;
                iPrimaryAmmo = 90;
            }

            formatex(szSecondary, charsmax(szSecondary), "weapon_deagle");
            iSecondaryCSW  = CSW_DEAGLE;
            iSecondaryAmmo = 35;

            if (bAllowUtility)
            {
                // Utility Entry: 1x Flashbang + 1x HE Grenade
                give_item(id, "weapon_flashbang");
                cs_set_user_bpammo(id, CSW_FLASHBANG, 1);
                give_item(id, "weapon_hegrenade");
                cs_set_user_bpammo(id, CSW_HEGRENADE, 1);
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
                formatex(szPrimary, charsmax(szPrimary), "weapon_scout");
                iPrimaryCSW  = CSW_SCOUT;
                iPrimaryAmmo = 90;
            }

            formatex(szSecondary, charsmax(szSecondary), "weapon_deagle");
            iSecondaryCSW  = CSW_DEAGLE;
            iSecondaryAmmo = 35;

            if (bAllowUtility)
            {
                // Utility AWPer: 1x Flashbang + 1x Smoke
                give_item(id, "weapon_flashbang");
                cs_set_user_bpammo(id, CSW_FLASHBANG, 1);
                give_item(id, "weapon_smokegrenade");
                cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 1);
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
                formatex(szPrimary, charsmax(szPrimary), (iTeam == CS_TEAM_T) ? "weapon_galil" : "weapon_famas");
                iPrimaryCSW  = (iTeam == CS_TEAM_T) ? CSW_GALIL : CSW_FAMAS;
                iPrimaryAmmo = 90;
            }

            formatex(szSecondary, charsmax(szSecondary), (iTeam == CS_TEAM_T) ? "weapon_p228" : "weapon_fiveseven");
            iSecondaryCSW  = (iTeam == CS_TEAM_T) ? CSW_P228 : CSW_FIVESEVEN;
            iSecondaryAmmo = (iTeam == CS_TEAM_T) ? 52 : 100;

            if (bAllowUtility)
            {
                // Utility Support: Full Utility (Smoke + 2x Flashbang + HE Grenade)
                give_item(id, "weapon_smokegrenade");
                cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 1);
                give_item(id, "weapon_flashbang");
                cs_set_user_bpammo(id, CSW_FLASHBANG, 2);
                give_item(id, "weapon_hegrenade");
                cs_set_user_bpammo(id, CSW_HEGRENADE, 1);
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
                formatex(szPrimary, charsmax(szPrimary), "weapon_mp5navy");
                iPrimaryCSW  = CSW_MP5NAVY;
                iPrimaryAmmo = 120;
            }

            formatex(szSecondary, charsmax(szSecondary), "weapon_usp");
            iSecondaryCSW  = CSW_USP;
            iSecondaryAmmo = 100;

            if (bAllowUtility)
            {
                // Utility Lurker: 1x Smoke + 1x Flashbang
                give_item(id, "weapon_smokegrenade");
                cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 1);
                give_item(id, "weapon_flashbang");
                cs_set_user_bpammo(id, CSW_FLASHBANG, 1);
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

            formatex(szSecondary, charsmax(szSecondary), "weapon_deagle");
            iSecondaryCSW  = CSW_DEAGLE;
            iSecondaryAmmo = 35;

            if (bAllowUtility)
            {
                // Utility IGL: Full Tactical Utility (Smoke + 2x Flashbang + HE Grenade)
                give_item(id, "weapon_smokegrenade");
                cs_set_user_bpammo(id, CSW_SMOKEGRENADE, 1);
                give_item(id, "weapon_flashbang");
                cs_set_user_bpammo(id, CSW_FLASHBANG, 2);
                give_item(id, "weapon_hegrenade");
                cs_set_user_bpammo(id, CSW_HEGRENADE, 1);
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
            new iCost = bFullBuy ? 4200 : (bForceBuy ? 2200 : 800);
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

        // Freeze bots during freeze time in live match & knife round
        if (g_bInFreezeTime && !IsWarmupActive() && !IsDeathmatchActive())
        {
            new flags = pev(id, pev_flags);
            set_pev(id, pev_flags, flags | FL_FROZEN);
            set_pev(id, pev_velocity, Float:{0.0, 0.0, 0.0});
        }

        // Loadout Warmup ditangani langsung oleh csgo_competitive.sma, dan Deathmatch oleh redm_listen_engine.sma
        if (!IsKnifeRoundActive() && !IsWarmupActive() && !IsDeathmatchActive())
        {
            remove_task(TASK_SV_SPAWN_EQUIP + id);
            set_task(0.10, "Task_SpawnRoleEquip", TASK_SV_SPAWN_EQUIP + id);
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

public Task_SpawnRoleEquip(taskid)
{
    new id = taskid - TASK_SV_SPAWN_EQUIP;
    if (is_user_alive(id) && is_user_bot(id) && !IsKnifeRoundActive())
    {
        EquipBotByRole(id, true, true);
    }
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
