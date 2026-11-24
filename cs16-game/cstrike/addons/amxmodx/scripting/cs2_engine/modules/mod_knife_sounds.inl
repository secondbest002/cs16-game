// ==============================================================================
// CS2 MODERN ENGINE - SUB-MODULE: DYNAMIC KNIFE SOUND ENGINE
// Model-name driven CS:GO / CS2 knife sound manager (Deploy, Inspect, Stab)
// ==============================================================================

#if defined _mod_knife_sounds_included
    #endinput
#endif
#define _mod_knife_sounds_included

// CVars
new ks_pCvar_Enable;

// Knife Types (aligned 1:1 with cs2_sdk/cs2_weapon_registry.inc)
enum _:KsKnifeType {
    KS_KNIFE_UNKNOWN   = -1,
    KS_KNIFE_DEFAULT   = CS2_KNIFE_DEFAULT,
    KS_KNIFE_KARAMBIT  = CS2_KNIFE_KARAMBIT,
    KS_KNIFE_BUTTERFLY = CS2_KNIFE_BUTTERFLY,
    KS_KNIFE_M9        = CS2_KNIFE_M9,
    KS_KNIFE_BAYONET   = CS2_KNIFE_BAYONET,
    KS_KNIFE_BOWIE     = CS2_KNIFE_BOWIE,
    KS_KNIFE_FALCHION  = CS2_KNIFE_FALCHION,
    KS_KNIFE_DAGGERS   = CS2_KNIFE_DAGGERS,
    KS_KNIFE_STILETTO  = CS2_KNIFE_STILETTO,
    KS_KNIFE_TALON     = CS2_KNIFE_TALON,
    KS_KNIFE_URSUS     = CS2_KNIFE_URSUS,
    KS_KNIFE_SKELETON  = CS2_KNIFE_SKELETON,
    KS_KNIFE_FLIP      = CS2_KNIFE_FLIP,
    KS_KNIFE_GUT       = CS2_KNIFE_GUT,
    KS_KNIFE_HUNTSMAN  = CS2_KNIFE_HUNTSMAN,
    KS_KNIFE_NAVAJA    = CS2_KNIFE_NAVAJA,
    KS_KNIFE_NOMAD     = CS2_KNIFE_NOMAD,
    KS_KNIFE_PARACORD  = CS2_KNIFE_PARACORD,
    KS_KNIFE_SURVIVAL  = CS2_KNIFE_SURVIVAL
};

new ks_PlayerKnifeType[33] = { KS_KNIFE_UNKNOWN, ... };
new Float:ks_flLastInspectTime[33];

// Sound files definitions - Modern Unique Standardized Paths
#define KS_SOUND_DEPLOY_DEFAULT     "weapons/csgo/knife/knife_deploy1.wav"
#define KS_SOUND_DEPLOY_KARAMBIT    "weapons/csgo/knife_karambit/knife_karambit_draw.wav"
#define KS_SOUND_DEPLOY_M9          "weapons/csgo/knife_m9/knife_m9_draw.wav"
#define KS_SOUND_DEPLOY_BAYONET     "weapons/csgo/knife_bayonet/knife_bayonet_draw.wav"
#define KS_SOUND_DEPLOY_FLIP        "weapons/csgo/knife_flip/knife_flip_draw.wav"
#define KS_SOUND_DEPLOY_HUNTSMAN    "weapons/csgo/knife_huntsman/knife_huntsman_draw.wav"
#define KS_SOUND_DEPLOY_BOWIE       "weapons/csgo/knife_bowie/knife_bowie_draw.wav"
#define KS_SOUND_DEPLOY_FALCHION    "weapons/csgo/knife_falchion/knife_falchion_draw.wav"
#define KS_SOUND_DEPLOY_DAGGERS     "weapons/csgo/knife_daggers/knife_daggers_draw.wav"
#define KS_SOUND_DEPLOY_STILETTO    "weapons/csgo/knife_stiletto/knife_stiletto_draw.wav"
#define KS_SOUND_DEPLOY_TALON       "weapons/csgo/knife_talon/knife_talon_draw.wav"
#define KS_SOUND_DEPLOY_URSUS       "weapons/csgo/knife_ursus/knife_ursus_draw.wav"
#define KS_SOUND_DEPLOY_NAVAJA      "weapons/csgo/knife_navaja/knife_navaja_draw.wav"
#define KS_SOUND_DEPLOY_BUTTERFLY   "weapons/csgo/knife_butterfly/knife_butterfly_draw_01.wav"

// Inspect sounds - Unique per knife variant
#define KS_SOUND_SWOOSH_START       "weapons/csgo/movement1.wav"
#define KS_SOUND_SWOOSH_MID         "weapons/csgo/movement2.wav"
#define KS_SOUND_SWOOSH_END         "weapons/csgo/movement3.wav"

#define KS_SOUND_KARAMBIT_SPIN1     "weapons/csgo/knife_karambit/knife_karambit_spin_01.wav"
#define KS_SOUND_KARAMBIT_SPIN2     "weapons/csgo/knife_karambit/knife_karambit_spin_02.wav"
#define KS_SOUND_KARAMBIT_SPIN3     "weapons/csgo/knife_karambit/knife_karambit_spin_03.wav"

#define KS_SOUND_BF_LOOK01_A        "weapons/csgo/knife_butterfly/look01_a.wav"
#define KS_SOUND_BF_LOOK01_B        "weapons/csgo/knife_butterfly/look01_b.wav"
#define KS_SOUND_BF_LOOK02_A        "weapons/csgo/knife_butterfly/look02_a.wav"
#define KS_SOUND_BF_LOOK02_B        "weapons/csgo/knife_butterfly/look02_b.wav"
#define KS_SOUND_BF_LOOK03_A        "weapons/csgo/knife_butterfly/look03_a.wav"
#define KS_SOUND_BF_LOOK03_B        "weapons/csgo/knife_butterfly/look03_b.wav"

#define KS_SOUND_M9_INSPECT_START   "weapons/csgo/knife_m9/knife_m9_inspect_start.wav"
#define KS_SOUND_M9_INSPECT_END     "weapons/csgo/knife_m9/knife_m9_inspect_end.wav"

#define KS_SOUND_BAYONET_LOOK_START "weapons/csgo/knife_bayonet/bayonet_insp_start.wav"
#define KS_SOUND_BAYONET_LOOK_END   "weapons/csgo/knife_bayonet/bayonet_insp_end.wav"

#define KS_SOUND_FLIP_LOOK_START    "weapons/csgo/knife_flip/knife_flip_inspect_start.wav"
#define KS_SOUND_FLIP_LOOK_END      "weapons/csgo/knife_flip/knife_flip_inspect_end.wav"

#define KS_SOUND_HUNTSMAN_LOOK_START "weapons/csgo/knife_huntsman/huntsman_insp_start.wav"
#define KS_SOUND_HUNTSMAN_LOOK_END   "weapons/csgo/knife_huntsman/huntsman_insp_end.wav"

#define KS_SOUND_BOWIE_LOOK_START   "weapons/csgo/knife_bowie/knife_bowie_inspect_start.wav"
#define KS_SOUND_BOWIE_LOOK_END     "weapons/csgo/knife_bowie/knife_bowie_inspect_end.wav"
#define KS_SOUND_FALCHION_LOOK      "weapons/csgo/knife_falchion/knife_falchion_inspect.wav"
#define KS_SOUND_DAGGERS_LOOK       "weapons/csgo/knife_daggers/daggers_insp_start.wav"
#define KS_SOUND_URSUS_FLIP         "weapons/csgo/knife_ursus/knife_ursus_flip.wav"
#define KS_SOUND_URSUS_CATCH        "weapons/csgo/knife_ursus/knife_ursus_catch.wav"

// Attack / Stab sounds - Dedicated per knife
#define KS_SOUND_BF_STAB            "weapons/csgo/knife_butterfly/knife_butterfly_stab.wav"
#define KS_SOUND_KARAMBIT_STAB      "weapons/csgo/knife_karambit/knife_karambit_stab.wav"
#define KS_SOUND_DAGGERS_STAB       "weapons/csgo/knife_daggers/knife_daggers_stab.wav"

// List of all sounds to precache safely
new const ks_AllSoundsToPrecache[][] = {
    KS_SOUND_DEPLOY_DEFAULT,
    KS_SOUND_DEPLOY_KARAMBIT,
    KS_SOUND_DEPLOY_M9,
    KS_SOUND_DEPLOY_BAYONET,
    KS_SOUND_DEPLOY_FLIP,
    KS_SOUND_DEPLOY_HUNTSMAN,
    KS_SOUND_DEPLOY_BUTTERFLY,
    KS_SOUND_DEPLOY_BOWIE,
    KS_SOUND_DEPLOY_FALCHION,
    KS_SOUND_DEPLOY_DAGGERS,
    KS_SOUND_DEPLOY_STILETTO,
    KS_SOUND_DEPLOY_TALON,
    KS_SOUND_DEPLOY_URSUS,
    KS_SOUND_DEPLOY_NAVAJA,
    KS_SOUND_SWOOSH_START,
    KS_SOUND_SWOOSH_MID,
    KS_SOUND_SWOOSH_END,
    KS_SOUND_KARAMBIT_SPIN1,
    KS_SOUND_KARAMBIT_SPIN2,
    KS_SOUND_KARAMBIT_SPIN3,
    KS_SOUND_M9_INSPECT_START,
    KS_SOUND_M9_INSPECT_END,
    KS_SOUND_BAYONET_LOOK_START,
    KS_SOUND_BAYONET_LOOK_END,
    KS_SOUND_FLIP_LOOK_START,
    KS_SOUND_FLIP_LOOK_END,
    KS_SOUND_HUNTSMAN_LOOK_START,
    KS_SOUND_HUNTSMAN_LOOK_END,
    KS_SOUND_BOWIE_LOOK_START,
    KS_SOUND_BOWIE_LOOK_END,
    KS_SOUND_FALCHION_LOOK,
    KS_SOUND_DAGGERS_LOOK,
    KS_SOUND_URSUS_FLIP,
    KS_SOUND_URSUS_CATCH,
    KS_SOUND_BF_STAB,
    KS_SOUND_KARAMBIT_STAB,
    KS_SOUND_DAGGERS_STAB,
    // Butterfly Keyframe Events
    "weapons/csgo/knife_butterfly/draw1.wav",
    "weapons/csgo/knife_butterfly/draw2.wav",
    "weapons/csgo/knife_butterfly/draw_1.wav",
    "weapons/csgo/knife_butterfly/draw_2.wav",
    "weapons/csgo/knife_butterfly/insp1a.wav",
    "weapons/csgo/knife_butterfly/insp2a.wav",
    "weapons/csgo/knife_butterfly/insp1b.wav",
    "weapons/csgo/knife_butterfly/insp2b.wav",
    "weapons/csgo/knife_butterfly/insp1c.wav",
    "weapons/csgo/knife_butterfly/insp2c.wav",
    "weapons/csgo/knife_butterfly/backstab.wav",
    // Stiletto & Talon Keyframe Events
    "weapons/csgo/knife_stiletto/knife_stiletto_draw.wav",
    "weapons/csgo/knife_stiletto/knife_stiletto_flip_01.wav",
    "weapons/csgo/knife_talon/knife_talon_draw.wav",
    "weapons/csgo/knife_talon/knife_talon_inspect_start.wav"
};

// -------------------------------------------------------------
// Lifecycle Hooks
// -------------------------------------------------------------
public Mod_KnifeSounds_Init()
{
    ks_pCvar_Enable = register_cvar("amx_cs2_knife_sounds", "1");

    RegisterHam(Ham_Item_Deploy, "weapon_knife", "ks_OnKnifeDeployPost", 1);
    register_event("CurWeapon", "ks_Event_CurWeapon", "be", "1=1");
    register_event("ResetHUD", "ks_Event_ResetHUD", "b");
    register_event("TeamInfo", "ks_Event_TeamInfo", "a");

    register_forward(FM_EmitSound, "ks_fw_EmitSound");

    // Universal Weapon Inspect Command Registrations
    register_clcmd("lookatweapon", "ks_Cmd_LookAtWeapon");
    register_clcmd("weaponlookat", "ks_Cmd_LookAtWeapon");
    register_clcmd("+lookatweapon", "ks_Cmd_LookAtWeapon");
    register_clcmd("-lookatweapon", "ks_Cmd_LookAtWeapon_End");
    register_clcmd("+weaponlookat", "ks_Cmd_LookAtWeapon");
    register_clcmd("-weaponlookat", "ks_Cmd_LookAtWeapon_End");
    register_clcmd("inspect", "ks_Cmd_LookAtWeapon");
    register_clcmd("say /inspect", "ks_Cmd_LookAtWeapon");
}

public Mod_KnifeSounds_Precache()
{
    for (new i = 0; i < sizeof(ks_AllSoundsToPrecache); i++)
    {
        new fullpath[128];
        formatex(fullpath, charsmax(fullpath), "sound/%s", ks_AllSoundsToPrecache[i]);
        if (file_exists(fullpath) || file_exists(fullpath, true))
        {
            precache_sound(ks_AllSoundsToPrecache[i]);
        }
    }
    precache_sound("weapons/knife_slash1.wav");
}

public Mod_KnifeSounds_PutInServer(id)
{
    ks_PlayerKnifeType[id] = KS_KNIFE_UNKNOWN;
    ks_flLastInspectTime[id] = 0.0;
}

public Mod_KnifeSounds_Disconnect(id)
{
    ks_PlayerKnifeType[id] = KS_KNIFE_UNKNOWN;
    ks_flLastInspectTime[id] = 0.0;
    ks_ClearInspectTasks(id);
}

public Mod_KnifeSounds_NewRound()
{
    for (new i = 1; i <= 32; i++)
    {
        ks_flLastInspectTime[i] = 0.0;
        ks_ClearInspectTasks(i);
    }
}

public Mod_KnifeSounds_RoundEnd()
{
    for (new i = 1; i <= 32; i++)
    {
        ks_ClearInspectTasks(i);
    }
}

public ks_Event_ResetHUD(id)
{
    if (id >= 1 && id <= MaxClients)
    {
        ks_PlayerKnifeType[id] = KS_KNIFE_UNKNOWN;
        ks_ClearInspectTasks(id);
    }
}

public ks_Event_TeamInfo()
{
    new id = read_data(1);
    if (id >= 1 && id <= MaxClients)
    {
        ks_PlayerKnifeType[id] = KS_KNIFE_UNKNOWN;
        ks_ClearInspectTasks(id);
    }
}

// -------------------------------------------------------------
// Pure Dynamic Model Name Detection (delegated to cs2_weapon_registry)
// -------------------------------------------------------------
stock ks_DetectKnifeTypeFromModel(const model[])
{
    if (!model[0])
        return KS_KNIFE_UNKNOWN;

    return cs2_detect_knife_type(model);
}

stock ks_DetectKnifeType(id)
{
    if (!is_user_alive(id))
        return KS_KNIFE_UNKNOWN;

    new v_model[128];
    pev(id, pev_viewmodel2, v_model, charsmax(v_model));
    return ks_DetectKnifeTypeFromModel(v_model);
}

// -------------------------------------------------------------
// Deploy Events
// -------------------------------------------------------------
public ks_OnKnifeDeployPost(ent)
{
    if (!get_pcvar_num(pCvar_MasterEnable) || !get_pcvar_num(ks_pCvar_Enable))
        return HAM_IGNORED;

    if (!pev_valid(ent))
        return HAM_IGNORED;

    new id = get_pdata_cbase(ent, 41, 4);
    if (id < 1 || id > MaxClients || !is_user_alive(id))
        return HAM_IGNORED;

    ks_ClearInspectTasks(id);

    new v_model[128];
    pev(id, pev_viewmodel2, v_model, charsmax(v_model));

    new ktype = ks_DetectKnifeTypeFromModel(v_model);
    if (ktype != KS_KNIFE_UNKNOWN)
    {
        ks_PlayerKnifeType[id] = ktype;
        ks_PlayKnifeDeploySound(id);
    }
    else
    {
        remove_task(TASK_KS_DEPLOY_FALLBACK + id);
        set_task(0.02, "ks_Task_DeferredDeployCheck", TASK_KS_DEPLOY_FALLBACK + id);
    }

    return HAM_IGNORED;
}

public ks_Task_DeferredDeployCheck(taskid)
{
    new id = taskid - TASK_KS_DEPLOY_FALLBACK;
    if (id < 1 || id > MaxClients || !is_user_alive(id))
        return;

    if (get_user_weapon(id) != CSW_KNIFE)
        return;

    new v_model[128];
    pev(id, pev_viewmodel2, v_model, charsmax(v_model));
    new ktype = ks_DetectKnifeTypeFromModel(v_model);
    if (ktype != KS_KNIFE_UNKNOWN)
    {
        ks_PlayerKnifeType[id] = ktype;
        ks_PlayKnifeDeploySound(id);
    }
}

public ks_Event_CurWeapon(id)
{
    if (id < 1 || id > MaxClients || !is_user_alive(id))
        return;

    new weapon_id = read_data(2);
    if (weapon_id != CSW_KNIFE)
    {
        ks_ClearInspectTasks(id);
        return;
    }

    new v_model[128];
    pev(id, pev_viewmodel2, v_model, charsmax(v_model));
    new ktype = ks_DetectKnifeTypeFromModel(v_model);
    if (ktype != KS_KNIFE_UNKNOWN)
        ks_PlayerKnifeType[id] = ktype;
}

// -------------------------------------------------------------
// Sound Playback Helpers
// -------------------------------------------------------------
stock ks_PlayKnifeDeploySound(id)
{
    if (is_user_bot(id))
        return;

    new v_model[128];
    pev(id, pev_viewmodel2, v_model, charsmax(v_model));

    new ktype = ks_DetectKnifeTypeFromModel(v_model);
    if (ktype == KS_KNIFE_UNKNOWN)
    {
        ktype = ks_PlayerKnifeType[id];
    }
    else
    {
        ks_PlayerKnifeType[id] = ktype;
    }

    // Knives with native MDL animation event 5004 draw sounds play them automatically
    // in exact sync with their viewmodel flip frames. Do NOT play redundant or
    // mismatched draw sounds over client console spk:
    if (ktype == KS_KNIFE_M9 || ktype == KS_KNIFE_BUTTERFLY || ktype == KS_KNIFE_STILETTO || ktype == KS_KNIFE_TALON || ktype == KS_KNIFE_KARAMBIT)
    {
        return;
    }

    new sound[128];
    switch (ktype)
    {
        case KS_KNIFE_BAYONET:
            copy(sound, charsmax(sound), KS_SOUND_DEPLOY_BAYONET);
        case KS_KNIFE_FLIP:
            copy(sound, charsmax(sound), KS_SOUND_DEPLOY_FLIP);
        case KS_KNIFE_HUNTSMAN:
            copy(sound, charsmax(sound), KS_SOUND_DEPLOY_HUNTSMAN);
        case KS_KNIFE_BOWIE:
            copy(sound, charsmax(sound), KS_SOUND_DEPLOY_BOWIE);
        case KS_KNIFE_FALCHION:
            copy(sound, charsmax(sound), KS_SOUND_DEPLOY_FALCHION);
        case KS_KNIFE_DAGGERS:
            copy(sound, charsmax(sound), KS_SOUND_DEPLOY_DAGGERS);
        case KS_KNIFE_URSUS:
            copy(sound, charsmax(sound), KS_SOUND_DEPLOY_URSUS);
        case KS_KNIFE_NAVAJA:
            copy(sound, charsmax(sound), KS_SOUND_DEPLOY_NAVAJA);
        default:
            copy(sound, charsmax(sound), KS_SOUND_DEPLOY_DEFAULT);
    }

    if (sound[0])
    {
        client_cmd(id, "spk %s", sound);
    }
}

// -------------------------------------------------------------
// Sound Interception (FM_EmitSound)
// -------------------------------------------------------------
public ks_fw_EmitSound(id, channel, const sample[], Float:volume, Float:attn, flags, pitch)
{
    if (!get_pcvar_num(pCvar_MasterEnable) || !get_pcvar_num(ks_pCvar_Enable))
        return FMRES_IGNORED;

    if (id < 1 || id > MaxClients || !is_user_alive(id))
        return FMRES_IGNORED;

    if (get_user_weapon(id) != CSW_KNIFE)
        return FMRES_IGNORED;

    if (equal(sample, "weapons/knife_deploy1.wav") || equal(sample, "weapons/knife_deploy.wav"))
    {
        return FMRES_SUPERCEDE;
    }

    if (equal(sample, "weapons/knife_slash2.wav") || containi(sample, "knife_slash2.wav") != -1)
    {
        emit_sound(id, channel, "weapons/knife_slash1.wav", volume, attn, flags, pitch);
        return FMRES_SUPERCEDE;
    }

    if (equal(sample, "weapons/knife_stab.wav"))
    {
        new v_model[128];
        pev(id, pev_viewmodel2, v_model, charsmax(v_model));
        new ktype = ks_DetectKnifeTypeFromModel(v_model);

        if (ktype == KS_KNIFE_BUTTERFLY)
        {
            emit_sound(id, channel, KS_SOUND_BF_STAB, volume, attn, flags, pitch);
            return FMRES_SUPERCEDE;
        }
        else if (ktype == KS_KNIFE_KARAMBIT)
        {
            emit_sound(id, channel, KS_SOUND_KARAMBIT_STAB, volume, attn, flags, pitch);
            return FMRES_SUPERCEDE;
        }
        else if (ktype == KS_KNIFE_DAGGERS)
        {
            emit_sound(id, channel, KS_SOUND_DAGGERS_STAB, volume, attn, flags, pitch);
            return FMRES_SUPERCEDE;
        }
    }

    return FMRES_IGNORED;
}

// -------------------------------------------------------------
// Universal Inspect Command (lookatweapon / weaponlookat / 'F' key)
// -------------------------------------------------------------
public ks_Cmd_LookAtWeapon_End(id)
{
    return PLUGIN_HANDLED;
}

stock ks_GetWeaponInspectAnim(id, iWpn, weapon_ent)
{
    switch (iWpn)
    {
        case CSW_KNIFE:
        {
            new v_model[128];
            pev(id, pev_viewmodel2, v_model, charsmax(v_model));
            new ktype = ks_DetectKnifeTypeFromModel(v_model);
            if (ktype == KS_KNIFE_BUTTERFLY || ktype == KS_KNIFE_KARAMBIT || ktype == KS_KNIFE_BAYONET || ktype == KS_KNIFE_M9 || ktype == KS_KNIFE_STILETTO || ktype == KS_KNIFE_FLIP)
            {
                return random_num(8, 10);
            }
            return 8;
        }
        case CSW_AK47:      return 6;
        case CSW_M4A1:
        {
            if (pev_valid(weapon_ent) && cs_get_weapon_silen(weapon_ent))
                return 14;
            return 15;
        }
        case CSW_AWP:       return 6;
        case CSW_DEAGLE:    return 6;
        case CSW_GLOCK18:   return 13;
        case CSW_USP:
        {
            if (pev_valid(weapon_ent) && cs_get_weapon_silen(weapon_ent))
                return 16;
            return 17;
        }
        case CSW_ELITE:     return 16;
        case CSW_FAMAS:     return 6;
        case CSW_GALIL:     return 6;
        case CSW_AUG:       return 6;
        case CSW_SG552:     return 6;
        case CSW_SCOUT:     return 5;
        case CSW_G3SG1:     return 5;
        case CSW_SG550:     return 5;
        case CSW_MP5NAVY:   return 6;
        case CSW_UMP45:     return 6;
        case CSW_P90:       return 6;
        case CSW_TMP:       return 6;
        case CSW_MAC10:     return 6;
        case CSW_FIVESEVEN: return 6;
        case CSW_P228:      return 7;
        case CSW_XM1014:    return 7;
        case CSW_M3:        return 7;
        case CSW_M249:      return 5;
    }
    return -1;
}

stock Float:ks_GetKnifeInspectDuration(ktype, iAnim)
{
    switch (ktype)
    {
        case KS_KNIFE_BUTTERFLY: return (iAnim == 10) ? 4.92 : ((iAnim == 9) ? 4.58 : 4.21);
        case KS_KNIFE_KARAMBIT:  return 4.60;
        case KS_KNIFE_BAYONET:   return 4.80;
        case KS_KNIFE_M9:        return 4.83;
        case KS_KNIFE_STILETTO:  return (iAnim == 9) ? 5.03 : 4.41;
        case KS_KNIFE_FLIP:      return 4.80;
        case KS_KNIFE_HUNTSMAN:  return 6.23;
        case KS_KNIFE_TALON:     return 4.60;
        case KS_KNIFE_BOWIE:     return 4.80;
        case KS_KNIFE_FALCHION:  return 4.50;
        case KS_KNIFE_DAGGERS:   return 4.00;
        case KS_KNIFE_URSUS:     return 4.50;
        case KS_KNIFE_SKELETON:  return 4.50;
        case KS_KNIFE_GUT:       return 4.50;
        case KS_KNIFE_NAVAJA:    return 4.20;
        case KS_KNIFE_NOMAD:     return 4.50;
        case KS_KNIFE_PARACORD:  return 4.50;
        case KS_KNIFE_SURVIVAL:  return 4.50;
        default:                 return 4.00;
    }
    return 4.00;
}

stock Float:ks_GetWeaponInspectDuration(id, iWpn, weapon_ent, iAnim)
{
    #pragma unused weapon_ent
    if (iWpn == CSW_KNIFE)
    {
        new v_model[128];
        pev(id, pev_viewmodel2, v_model, charsmax(v_model));
        new ktype = ks_DetectKnifeTypeFromModel(v_model);
        return ks_GetKnifeInspectDuration(ktype, iAnim);
    }

    switch (iWpn)
    {
        case CSW_AK47:      return 4.40;
        case CSW_M4A1:      return 4.88;
        case CSW_AWP:       return 5.00;
        case CSW_DEAGLE:    return 4.60;
        case CSW_GLOCK18:   return 5.20;
        case CSW_USP:       return 6.12;
        case CSW_ELITE:     return 4.52;
        case CSW_FAMAS:     return 3.37;
        case CSW_GALIL:     return 3.67;
        case CSW_AUG:       return 3.47;
        case CSW_SG552:     return 3.71;
        case CSW_SCOUT:     return 5.08;
        case CSW_G3SG1:     return 3.57;
        case CSW_SG550:     return 4.20;
        case CSW_MP5NAVY:   return 5.37;
        case CSW_UMP45:     return 5.37;
        case CSW_P90:       return 4.23;
        case CSW_TMP:       return 5.83;
        case CSW_MAC10:     return 5.13;
        case CSW_FIVESEVEN: return 5.20;
        case CSW_P228:      return 5.20;
        case CSW_XM1014:    return 4.44;
        case CSW_M3:        return 4.50;
        case CSW_M249:      return 5.53;
    }
    return 4.00;
}

public ks_Cmd_LookAtWeapon(id)
{
    if (!get_pcvar_num(pCvar_MasterEnable) || !get_pcvar_num(ks_pCvar_Enable))
        return PLUGIN_CONTINUE;

    if (!is_user_alive(id))
        return PLUGIN_CONTINUE;

    new Float:flGameTime = get_gametime();
    if (flGameTime - ks_flLastInspectTime[id] < 1.2)
        return PLUGIN_HANDLED;

    ks_flLastInspectTime[id] = flGameTime;
    ks_ClearInspectTasks(id);

    new iWpn = get_user_weapon(id);
    new weapon_ent = cs_get_user_weapon_entity(id);

    // Trigger visual viewmodel inspect sequence
    new iAnim = ks_GetWeaponInspectAnim(id, iWpn, weapon_ent);
    if (iAnim >= 0)
    {
        set_pev(id, pev_weaponanim, iAnim);

        new iBody = (pev_valid(weapon_ent)) ? pev(weapon_ent, pev_body) : 0;
        message_begin(MSG_ONE, SVC_WEAPONANIM, _, id);
        write_byte(iAnim);
        write_byte(iBody);
        message_end();

        if (pev_valid(weapon_ent))
        {
            // Delay next weapon idle so default idle sequence doesn't cut inspect short.
            // Offset 48 (m_flTimeWeaponIdle) must be set in absolute gametime matching the exact sequence duration.
            new Float:flInspectDuration = ks_GetWeaponInspectDuration(id, iWpn, weapon_ent, iAnim);
            set_pdata_float(weapon_ent, 48, flGameTime + flInspectDuration, 4);
        }
    }

    if (iWpn != CSW_KNIFE)
        return PLUGIN_HANDLED;

    new v_model[128];
    pev(id, pev_viewmodel2, v_model, charsmax(v_model));
    new ktype = ks_DetectKnifeTypeFromModel(v_model);
    ks_PlayerKnifeType[id] = ktype;

    // Knives with native MDL animation event 5004 inspect animations (Butterfly, Stiletto, Talon)
    // have frame-perfect audio triggered directly by the client engine as the sequence plays.
    // We should NOT emit arbitrary out-of-sync server sounds or delayed tasks for them.
    if (ktype == KS_KNIFE_BUTTERFLY || ktype == KS_KNIFE_STILETTO || ktype == KS_KNIFE_TALON)
    {
        return PLUGIN_HANDLED;
    }

    switch (ktype)
    {
        case KS_KNIFE_KARAMBIT:
        {
            emit_sound(id, CHAN_STATIC, KS_SOUND_KARAMBIT_SPIN1, 1.0, ATTN_NORM, 0, PITCH_NORM);
            set_task(0.35, "ks_Task_KarambitSpin2", TASK_KS_INSPECT_BASE + id);
            set_task(1.80, "ks_Task_KarambitSpin3", TASK_KS_INSPECT_STEP2 + id);
        }
        case KS_KNIFE_M9:
        {
            emit_sound(id, CHAN_STATIC, KS_SOUND_M9_INSPECT_START, 0.8, ATTN_NORM, 0, PITCH_NORM);
            set_task(0.8, "ks_Task_M9InspectEnd", TASK_KS_INSPECT_BASE + id);
        }
        case KS_KNIFE_BAYONET:
        {
            emit_sound(id, CHAN_STATIC, KS_SOUND_BAYONET_LOOK_START, 0.8, ATTN_NORM, 0, PITCH_NORM);
            set_task(0.8, "ks_Task_BayonetInspectEnd", TASK_KS_INSPECT_BASE + id);
        }
        case KS_KNIFE_FLIP:
        {
            emit_sound(id, CHAN_STATIC, KS_SOUND_FLIP_LOOK_START, 0.8, ATTN_NORM, 0, PITCH_NORM);
            set_task(0.8, "ks_Task_FlipInspectEnd", TASK_KS_INSPECT_BASE + id);
        }
        case KS_KNIFE_HUNTSMAN:
        {
            emit_sound(id, CHAN_STATIC, KS_SOUND_HUNTSMAN_LOOK_START, 0.8, ATTN_NORM, 0, PITCH_NORM);
            set_task(0.8, "ks_Task_HuntsmanInspectEnd", TASK_KS_INSPECT_BASE + id);
        }
        case KS_KNIFE_BOWIE:
        {
            emit_sound(id, CHAN_STATIC, KS_SOUND_BOWIE_LOOK_START, 1.0, ATTN_NORM, 0, PITCH_NORM);
            set_task(2.0, "ks_Task_BowieLookEnd", TASK_KS_INSPECT_BASE + id);
        }
        case KS_KNIFE_FALCHION:
        {
            emit_sound(id, CHAN_STATIC, KS_SOUND_FALCHION_LOOK, 1.0, ATTN_NORM, 0, PITCH_NORM);
        }
        case KS_KNIFE_DAGGERS:
        {
            emit_sound(id, CHAN_STATIC, KS_SOUND_DAGGERS_LOOK, 1.0, ATTN_NORM, 0, PITCH_NORM);
        }
        case KS_KNIFE_URSUS, KS_KNIFE_SKELETON, KS_KNIFE_PARACORD:
        {
            emit_sound(id, CHAN_STATIC, KS_SOUND_URSUS_FLIP, 1.0, ATTN_NORM, 0, PITCH_NORM);
            set_task(1.2, "ks_Task_UrsusCatch", TASK_KS_INSPECT_BASE + id);
        }
        default:
        {
            emit_sound(id, CHAN_STATIC, KS_SOUND_SWOOSH_START, 0.8, ATTN_NORM, 0, PITCH_NORM);
            set_task(0.8, "ks_Task_BladeSwooshEnd", TASK_KS_INSPECT_BASE + id);
        }
    }

    return PLUGIN_CONTINUE;
}

// -------------------------------------------------------------
// Scheduled Inspect Stages
// -------------------------------------------------------------
public ks_Task_KarambitSpin2(taskid)
{
    new id = taskid - TASK_KS_INSPECT_BASE;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_KARAMBIT_SPIN2, 1.0, ATTN_NORM, 0, PITCH_NORM);
    }
}

public ks_Task_KarambitSpin3(taskid)
{
    new id = taskid - TASK_KS_INSPECT_STEP2;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_KARAMBIT_SPIN3, 1.0, ATTN_NORM, 0, PITCH_NORM);
    }
}

public ks_Task_ButterflyLook1b(taskid)
{
    new id = taskid - TASK_KS_INSPECT_BASE;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_BF_LOOK01_B, 1.0, ATTN_NORM, 0, PITCH_NORM);
    }
}

public ks_Task_ButterflyLook2b(taskid)
{
    new id = taskid - TASK_KS_INSPECT_BASE;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_BF_LOOK02_B, 1.0, ATTN_NORM, 0, PITCH_NORM);
    }
}

public ks_Task_ButterflyLook3b(taskid)
{
    new id = taskid - TASK_KS_INSPECT_BASE;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_BF_LOOK03_B, 1.0, ATTN_NORM, 0, PITCH_NORM);
    }
}

public ks_Task_BladeSwooshEnd(taskid)
{
    new id = taskid - TASK_KS_INSPECT_BASE;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_SWOOSH_END, 0.7, ATTN_NORM, 0, PITCH_NORM);
    }
}

public ks_Task_M9InspectEnd(taskid)
{
    new id = taskid - TASK_KS_INSPECT_BASE;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_M9_INSPECT_END, 0.7, ATTN_NORM, 0, PITCH_NORM);
    }
}

public ks_Task_BayonetInspectEnd(taskid)
{
    new id = taskid - TASK_KS_INSPECT_BASE;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_BAYONET_LOOK_END, 0.7, ATTN_NORM, 0, PITCH_NORM);
    }
}

public ks_Task_FlipInspectEnd(taskid)
{
    new id = taskid - TASK_KS_INSPECT_BASE;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_FLIP_LOOK_END, 0.7, ATTN_NORM, 0, PITCH_NORM);
    }
}

public ks_Task_HuntsmanInspectEnd(taskid)
{
    new id = taskid - TASK_KS_INSPECT_BASE;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_HUNTSMAN_LOOK_END, 0.7, ATTN_NORM, 0, PITCH_NORM);
    }
}

public ks_Task_BowieLookEnd(taskid)
{
    new id = taskid - TASK_KS_INSPECT_BASE;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_BOWIE_LOOK_END, 1.0, ATTN_NORM, 0, PITCH_NORM);
    }
}

public ks_Task_UrsusCatch(taskid)
{
    new id = taskid - TASK_KS_INSPECT_BASE;
    if (id >= 1 && id <= MaxClients && is_user_alive(id) && get_user_weapon(id) == CSW_KNIFE)
    {
        emit_sound(id, CHAN_STATIC, KS_SOUND_URSUS_CATCH, 1.0, ATTN_NORM, 0, PITCH_NORM);
    }
}

stock ks_ClearInspectTasks(id)
{
    remove_task(TASK_KS_INSPECT_BASE + id);
    remove_task(TASK_KS_INSPECT_STEP2 + id);
    remove_task(TASK_KS_INSPECT_STEP3 + id);
    remove_task(TASK_KS_DEPLOY_FALLBACK + id);
}
