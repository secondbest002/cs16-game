#if defined _cs2_sdk_offsets_included
    #endinput
#endif
#define _cs2_sdk_offsets_included

// ==============================================================================
// CS2 SDK - GoldSrc / Xash3D FWGS Offsets, Bitmasks & Bot Entity Helpers
// Safe to include in plugins with or without #pragma semicolon 1.
// ==============================================================================

#if !defined XO_WEAPON
    #define XO_WEAPON 4
#endif
#if !defined XO_PLAYER
    #define XO_PLAYER 5
#endif
#if !defined extra_offset_weapon
    #define extra_offset_weapon 4
#endif
#if !defined extra_offset_player
    #define extra_offset_player 5
#endif
#if !defined OFFSET_ITEM_LINUX
    #define OFFSET_ITEM_LINUX 4
#endif
#if !defined OFFSET_PLAYER_LINUX
    #define OFFSET_PLAYER_LINUX 5
#endif

#if !defined m_pPlayer
    #define m_pPlayer 41
#endif
#if !defined OFFSET_PLAYER
    #define OFFSET_PLAYER 41
#endif
#if !defined m_iId
    #define m_iId 43
#endif
#if !defined m_flNextPrimaryAttack
    #define m_flNextPrimaryAttack 46
#endif
#if !defined m_flNextSecondaryAttack
    #define m_flNextSecondaryAttack 47
#endif
#if !defined m_flTimeWeaponIdle
    #define m_flTimeWeaponIdle 48
#endif
#if !defined m_iClip
    #define m_iClip 51
#endif
#if !defined m_fInReload
    #define m_fInReload 54
#endif
#if !defined m_iSpecialReload
    #define m_iSpecialReload 55
#endif
#if !defined m_flNextAttack
    #define m_flNextAttack 83
#endif
#if !defined m_bIsDefusing
    #define m_bIsDefusing 99
#endif
#if !defined OFFSET_TEAM
    #define OFFSET_TEAM 114
#endif
#if !defined m_pActiveItem
    #define m_pActiveItem 373
#endif

#if !defined VALID_PDATA
    #define VALID_PDATA 2
#endif
#if !defined SVC_WEAPONANIM
    #define SVC_WEAPONANIM 35
#endif

// Legacy Xash3D offset aliases (backward compat with xash_bot_utils.inc)
#if !defined XASH_EXTRAOFFSET_PLAYER
    #define XASH_EXTRAOFFSET_PLAYER         5
#endif
#if !defined XASH_EXTRAOFFSET_WEAPONS
    #define XASH_EXTRAOFFSET_WEAPONS        4
#endif
#if !defined XASH_OFFSET_WEAPON_OWNER
    #define XASH_OFFSET_WEAPON_OWNER        41
#endif
#if !defined XASH_OFFSET_NEXT_PRIMARY_ATTACK
    #define XASH_OFFSET_NEXT_PRIMARY_ATTACK 46
#endif
#if !defined XASH_OFFSET_NEXT_SECONDARY_ATK
    #define XASH_OFFSET_NEXT_SECONDARY_ATK  47
#endif
#if !defined XASH_OFFSET_NEXT_ATTACK
    #define XASH_OFFSET_NEXT_ATTACK         83
#endif
#if !defined XASH_OFFSET_ACTIVE_ITEM
    #define XASH_OFFSET_ACTIVE_ITEM         373
#endif

// Bitfield Helpers (32-bit player bitmasks)
#if !defined cs2_get_bit
    #define cs2_get_bit(%1,%2) ((%2) & (1 << ((%1) & 31)))
#endif
#if !defined cs2_set_bit
    #define cs2_set_bit(%1,%2) ((%2) |= (1 << ((%1) & 31)))
#endif
#if !defined cs2_rem_bit
    #define cs2_rem_bit(%1,%2) ((%2) &= ~(1 << ((%1) & 31)))
#endif

// Weapon Bitmasks (requires amxconst.inc / CSW_* constants)
#if defined CSW_SCOUT
    #if !defined CS2_PRIMARY_WEAPONS_BIT_SUM
        const CS2_PRIMARY_WEAPONS_BIT_SUM = (1<<CSW_SCOUT)|(1<<CSW_XM1014)|(1<<CSW_MAC10)|(1<<CSW_AUG)|(1<<CSW_UMP45)|(1<<CSW_SG550)|(1<<CSW_GALIL)|(1<<CSW_FAMAS)|(1<<CSW_AWP)|(1<<CSW_MP5NAVY)|(1<<CSW_M249)|(1<<CSW_M3)|(1<<CSW_M4A1)|(1<<CSW_TMP)|(1<<CSW_G3SG1)|(1<<CSW_SG552)|(1<<CSW_AK47)|(1<<CSW_P90);
    #endif
    #if !defined CS2_SECONDARY_WEAPONS_BIT_SUM
        const CS2_SECONDARY_WEAPONS_BIT_SUM = (1<<CSW_P228)|(1<<CSW_ELITE)|(1<<CSW_FIVESEVEN)|(1<<CSW_USP)|(1<<CSW_GLOCK18)|(1<<CSW_DEAGLE);
    #endif
    #if !defined XASH_PRIMARY_WEAPONS_BIT_SUM
        const XASH_PRIMARY_WEAPONS_BIT_SUM = CS2_PRIMARY_WEAPONS_BIT_SUM;
    #endif
#endif

// CS2 Killfeed Modifier Bitmask Flags (Synced bit-for-bit with patched client.dll)
#if !defined CS2_KILL_NONE
    #define CS2_KILL_NONE       0
    #define CS2_KILL_HS         (1 << 0)
    #define CS2_KILL_WALLBANG   (1 << 1)
    #define CS2_KILL_NOSCOPE    (1 << 2)
    #define CS2_KILL_SMOKE      (1 << 3)
    #define CS2_KILL_BLIND      (1 << 4)
    #define CS2_KILL_INAIR      (1 << 5)
#endif

// Xash3D FWGS Bot & Weapon Entity Utilities
#if defined _cstrike_included && defined _fakemeta_included && defined _hamsandwich_included
stock xash_safe_respawn(id)
{
    ExecuteHamB(Ham_CS_RoundRespawn, id);
    set_pev(id, pev_deadflag, DEAD_NO);
}

stock bool:xash_is_bot_combat_ready(id)
{
    if (id < 1 || id > MaxClients || !is_user_connected(id))
        return false;

    if (get_user_health(id) <= 0)
        return false;

    set_pev(id, pev_deadflag, DEAD_NO);
    return true;
}

stock xash_find_weapon_ent(id, const szClass[])
{
    new iEnt = -1;
    while ((iEnt = engfunc(EngFunc_FindEntityByString, iEnt, "classname", szClass)) > 0)
    {
        if (pev(iEnt, pev_owner) == id || get_pdata_cbase(iEnt, XASH_OFFSET_WEAPON_OWNER, XASH_EXTRAOFFSET_WEAPONS) == id)
        {
            return iEnt;
        }
    }
    return 0;
}

stock xash_get_weapon_default_clip(iCSW)
{
    switch (iCSW)
    {
        case CSW_AWP, CSW_SCOUT:      return 10;
        case CSW_DEAGLE:              return 7;
        case CSW_USP:                 return 12;
        case CSW_GLOCK18, CSW_FIVESEVEN, CSW_SG550, CSW_G3SG1: return 20;
        case CSW_P228:                return 13;
        case CSW_FAMAS, CSW_UMP45:    return 25;
        case CSW_GALIL:               return 35;
        case CSW_P90:                 return 50;
        case CSW_M249:                return 100;
        case CSW_M3:                  return 8;
        case CSW_XM1014:              return 7;
        default:                      return 30;
    }
    return 30;
}

stock xash_destroy_weaponbox(iEnt)
{
    if (!pev_valid(iEnt))
        return;

    set_pev(iEnt, pev_solid, SOLID_NOT);
    set_pev(iEnt, pev_nextthink, get_gametime());
    dllfunc(DLLFunc_Think, iEnt);
}

stock xash_force_deploy_weapon(id, iWpnEnt, const szClass[], iCSW, iClip = 0)
{
    if (szClass[0] == EOS)
        return;

    if (iClip <= 0)
    {
        iClip = xash_get_weapon_default_clip(iCSW);
    }

    set_pdata_float(id, XASH_OFFSET_NEXT_ATTACK, 0.0, XASH_EXTRAOFFSET_PLAYER);
    engclient_cmd(id, szClass);

    if (!pev_valid(iWpnEnt))
    {
        iWpnEnt = xash_find_weapon_ent(id, szClass);
    }

    if (pev_valid(iWpnEnt))
    {
        if (cs_get_weapon_ammo(iWpnEnt) <= 0)
        {
            cs_set_weapon_ammo(iWpnEnt, iClip);
        }

        if (get_pdata_cbase(id, XASH_OFFSET_ACTIVE_ITEM, XASH_EXTRAOFFSET_PLAYER) != iWpnEnt)
        {
            set_pdata_cbase(id, XASH_OFFSET_ACTIVE_ITEM, iWpnEnt, XASH_EXTRAOFFSET_PLAYER);
            ExecuteHamB(Ham_Item_Deploy, iWpnEnt);
        }

        set_pdata_float(id, XASH_OFFSET_NEXT_ATTACK, 0.0, XASH_EXTRAOFFSET_PLAYER);
        set_pdata_float(iWpnEnt, XASH_OFFSET_NEXT_PRIMARY_ATTACK, 0.0, XASH_EXTRAOFFSET_WEAPONS);
        set_pdata_float(iWpnEnt, XASH_OFFSET_NEXT_SECONDARY_ATK, 0.0, XASH_EXTRAOFFSET_WEAPONS);
    }

    static msgCurWeapon = 0;
    if (!msgCurWeapon)
    {
        msgCurWeapon = get_user_msgid("CurWeapon");
    }

    message_begin(MSG_ONE, msgCurWeapon, _, id);
    write_byte(1);
    write_byte(iCSW);
    write_byte(iClip);
    message_end();
}
#endif
