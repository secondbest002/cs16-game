#if defined _mod_gunshot_fx_included
    #endinput
#endif
#define _mod_gunshot_fx_included

public OnUpdateClientData_Post(id, sendweapons, cd_handle)
{
    if (!is_user_alive(id) || is_user_bot(id) || !get_pcvar_num(pCvar_Enable))
        return FMRES_IGNORED;

    new weapon_id = cs_get_user_weapon(id);
    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return FMRES_IGNORED;

    new CsTeams:team = safe_cs_get_user_team(id);
    if (team != CS_TEAM_CT && team != CS_TEAM_T)
        return FMRES_IGNORED;

    new team_index = (team == CS_TEAM_CT) ? TEAM_CT_INDEX : TEAM_T_INDEX;
    new slot = GET_SLOT(team_index, weapon_id);
    new s_idx = g_iPlayerSkin[id][slot];

    new iWpn = get_pdata_cbase(id, 373, 5);
    if (!pev_valid(iWpn))
        iWpn = fm_find_weapon_ent_by_owner(id, g_WeaponEntityNames[weapon_id]);

    if (weapon_id == CSW_M4A1)
    {
        if (pev_valid(iWpn))
        {
            new iVar = pev(iWpn, pev_iuser1);
            if (iVar == 1)
                s_idx = GetCatalogM4Index(true);
            else if (iVar == 2)
                s_idx = GetCatalogM4Index(false);
        }
    }
    else
    {
        if (pev_valid(iWpn))
        {
            new iSaved = pev(iWpn, pev_iuser2);
            if (iSaved > 0 && (iSaved - 1) < g_SkinCount[slot])
            {
                s_idx = iSaved - 1;
            }
        }
    }

    if (s_idx >= 0 && s_idx < g_SkinCount[slot] && g_SkinFireSoundCount[slot][s_idx] > 0)
    {
        set_cd(cd_handle, CD_flNextAttack, get_gametime() + 0.001);
        return FMRES_HANDLED;
    }

    return FMRES_IGNORED;
}

// ==============================================================================
// GUNSHOT BULLET DECALS & IMPACT EFFECTS (For Superceded Custom Gunfire Sounds)
// ==============================================================================
new const g_GunshotDecals[][] = {
    "{shot1",
    "{shot2",
    "{shot3",
    "{shot4",
    "{shot5"
};
new g_iGunshotDecalIndices[5] = { -1, -1, -1, -1, -1 };

stock CacheGunshotDecals()
{
    for (new i = 0; i < sizeof(g_GunshotDecals); i++)
    {
        g_iGunshotDecalIndices[i] = engfunc(EngFunc_DecalIndex, g_GunshotDecals[i]);
    }
}

stock GetRandomGunshotDecal()
{
    new idx = random_num(0, sizeof(g_GunshotDecals) - 1);
    if (g_iGunshotDecalIndices[idx] >= 0)
    {
        return g_iGunshotDecalIndices[idx];
    }

    new decal = engfunc(EngFunc_DecalIndex, g_GunshotDecals[idx]);
    if (decal >= 0)
    {
        g_iGunshotDecalIndices[idx] = decal;
        return decal;
    }

    return 41 + idx;
}

stock bool:IsWarmupOrDMActive()
{
    static pCvarWarmup = 0, pCvarRedm = 0, bool:bLookedUp = false;
    if (!bLookedUp)
    {
        pCvarWarmup = get_cvar_pointer("amx_csgo_is_warmup");
        pCvarRedm = get_cvar_pointer("redm_active");
        bLookedUp = true;
    }
    if (!pCvarWarmup)
        pCvarWarmup = get_cvar_pointer("amx_csgo_is_warmup");
    if (!pCvarRedm)
        pCvarRedm = get_cvar_pointer("redm_active");

    if (pCvarWarmup && get_pcvar_num(pCvarWarmup) > 0)
        return true;
    if (pCvarRedm && get_pcvar_num(pCvarRedm) > 0)
        return true;

    return false;
}

stock DrawGunshotDecalsAndEffects(invoker, weapon_id, const Float:angles[3])
{
    if (invoker < 1 || invoker > MaxClients || !is_user_alive(invoker))
        return;

    new bool:bSkipBotSparks = (bool:is_user_bot(invoker) && IsWarmupOrDMActive());

    new Float:vStart[3], Float:vViewOfs[3], Float:vOrigin[3];
    pev(invoker, pev_origin, vOrigin);
    pev(invoker, pev_view_ofs, vViewOfs);
    xs_vec_add(vOrigin, vViewOfs, vStart);

    new Float:vAimAngles[3];
    if (angles[0] != 0.0 || angles[1] != 0.0 || angles[2] != 0.0)
    {
        xs_vec_copy(angles, vAimAngles);
        if (vAimAngles[0] == 0.0)
        {
            // When torso angles are passed, pitch is 0.0; pull authoritative camera pitch
            new Float:vEyeAngles[3];
            pev(invoker, pev_v_angle, vEyeAngles);
            vAimAngles[0] = vEyeAngles[0];
        }
        new Float:vPunch[3];
        pev(invoker, pev_punchangle, vPunch);
        xs_vec_add(vAimAngles, vPunch, vAimAngles);
    }
    else
    {
        pev(invoker, pev_v_angle, vAimAngles);
        new Float:vPunch[3];
        pev(invoker, pev_punchangle, vPunch);
        xs_vec_add(vAimAngles, vPunch, vAimAngles);
    }

    new Float:vForward[3], Float:vRight[3], Float:vUp[3];
    angle_vector(vAimAngles, ANGLEVECTOR_FORWARD, vForward);
    angle_vector(vAimAngles, ANGLEVECTOR_RIGHT, vRight);
    angle_vector(vAimAngles, ANGLEVECTOR_UP, vUp);

    new bool:isShotgun = (weapon_id == CSW_M3 || weapon_id == CSW_XM1014);
    new numShots = isShotgun ? ((weapon_id == CSW_M3) ? 8 : 6) : 1;

    for (new s = 0; s < numShots; s++)
    {
        new Float:vDir[3], Float:vEnd[3];
        if (isShotgun)
        {
            new Float:flSpreadX = random_float(-0.045, 0.045);
            new Float:flSpreadY = random_float(-0.045, 0.045);
            vDir[0] = vForward[0] + (vRight[0] * flSpreadX) + (vUp[0] * flSpreadY);
            vDir[1] = vForward[1] + (vRight[1] * flSpreadX) + (vUp[1] * flSpreadY);
            vDir[2] = vForward[2] + (vRight[2] * flSpreadX) + (vUp[2] * flSpreadY);
            xs_vec_normalize(vDir, vDir);
        }
        else
        {
            xs_vec_copy(vForward, vDir);
        }

        xs_vec_mul_scalar(vDir, 8192.0, vEnd);
        xs_vec_add(vStart, vEnd, vEnd);

        engfunc(EngFunc_TraceLine, vStart, vEnd, DONT_IGNORE_MONSTERS, invoker, 0);

        new Float:flFraction;
        get_tr2(0, TR_flFraction, flFraction);
        if (flFraction >= 1.0)
            continue;

        new pHit = get_tr2(0, TR_pHit);
        if (pHit > 0 && pHit <= MaxClients)
            continue; // Player flesh impact, no bullet decal on players

        if (pev_valid(pHit))
        {
            new classname[32];
            pev(pHit, pev_classname, classname, charsmax(classname));
            if (equal(classname, "player") || equal(classname, "bot"))
                continue;
        }

        new Float:vecEndPos[3];
        get_tr2(0, TR_vecEndPos, vecEndPos);

        // 1. Particle effect (sparks and dust puff) - skipped for bots in Warmup/DM to prevent Xash3D ricochet audio & particle overload
        if (!bSkipBotSparks)
        {
            message_begin(MSG_BROADCAST, SVC_TEMPENTITY);
            write_byte(TE_GUNSHOT);
            write_coord_f(vecEndPos[0]);
            write_coord_f(vecEndPos[1]);
            write_coord_f(vecEndPos[2]);
            message_end();
        }

        // 2. Gunshot bullet hole decal
        new decalIndex = GetRandomGunshotDecal();
        message_begin(MSG_BROADCAST, SVC_TEMPENTITY);
        write_byte(TE_GUNSHOTDECAL);
        write_coord_f(vecEndPos[0]);
        write_coord_f(vecEndPos[1]);
        write_coord_f(vecEndPos[2]);
        write_short(pHit > 0 ? pHit : 0);
        write_byte(decalIndex);
        message_end();
    }
}

public OnPlaybackEvent_Pre(flags, invoker, eventid, Float:delay, Float:origin[3], Float:angles[3], Float:fparam1, Float:fparam2, iParam1, iParam2, bParam1, bParam2)
{
    if (!get_pcvar_num(pCvar_Enable) || invoker < 1 || invoker > MaxClients || !is_user_alive(invoker))
        return FMRES_IGNORED;

    new weapon_id = cs_get_user_weapon(invoker);
    if (weapon_id <= CSW_NONE || weapon_id > CSW_LAST_WEAPON)
        return FMRES_IGNORED;

    if (eventid != g_iWeaponEventId[weapon_id] || eventid == 0)
        return FMRES_IGNORED;

    new CsTeams:team = safe_cs_get_user_team(invoker);
    if (team != CS_TEAM_CT && team != CS_TEAM_T)
        return FMRES_IGNORED;

    new team_index = (team == CS_TEAM_CT) ? TEAM_CT_INDEX : TEAM_T_INDEX;
    new slot = GET_SLOT(team_index, weapon_id);
    new s_idx = g_iPlayerSkin[invoker][slot];

    new iWpn = get_pdata_cbase(invoker, 373, 5);
    if (!pev_valid(iWpn))
        iWpn = fm_find_weapon_ent_by_owner(invoker, g_WeaponEntityNames[weapon_id]);

    if (weapon_id == CSW_M4A1)
    {
        if (pev_valid(iWpn))
        {
            new iVar = pev(iWpn, pev_iuser1);
            if (iVar == 1)
                s_idx = GetCatalogM4Index(true);
            else if (iVar == 2)
                s_idx = GetCatalogM4Index(false);
        }
    }
    else
    {
        if (pev_valid(iWpn))
        {
            new iSaved = pev(iWpn, pev_iuser2);
            if (iSaved > 0 && (iSaved - 1) < g_SkinCount[slot])
            {
                s_idx = iSaved - 1;
            }
        }
    }

    if (s_idx < 0 || s_idx >= g_SkinCount[slot])
        return FMRES_IGNORED;

    new snd_count = g_SkinFireSoundCount[slot][s_idx];
    if (snd_count <= 0)
        return FMRES_IGNORED;

    new sound_file[128];
    if (snd_count == 1 || (snd_count > 1 && random_num(0, 1) == 0))
    {
        copy(sound_file, charsmax(sound_file), g_SkinFireSound1[slot][s_idx]);
    }
    else
    {
        copy(sound_file, charsmax(sound_file), g_SkinFireSound2[slot][s_idx]);
    }

    if (!sound_file[0])
        return FMRES_IGNORED;

    // Play custom gunfire sound cleanly without playing default 1.6 event sound!
    emit_sound(invoker, CHAN_WEAPON, sound_file, VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

    // Provide visual recoil and muzzleflash to invoker and observers
    set_pev(invoker, pev_effects, pev(invoker, pev_effects) | EF_MUZZLEFLASH);

    if (!is_user_bot(invoker))
    {
        new shoot_anim = GetWeaponShootAnim(weapon_id);
        if (shoot_anim >= 0)
        {
            SendWeaponAnim(invoker, shoot_anim);
        }
    }

    // Render bullet holes, wall impact sparks & dust particles for weapons with custom gunfire audio
    DrawGunshotDecalsAndEffects(invoker, weapon_id, angles);

    return FMRES_SUPERCEDE;
}

