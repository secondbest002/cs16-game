// ==============================================================================
// CS2 MODERN ENGINE - SUB-MODULE: KILL FEEDBACK & ACE SYSTEM
// Chimes 1K-5K ACE, Headshot Dink Sound, Skull Cards HUD, C4 10s Warning Music
// ==============================================================================

#if defined _mod_kill_feedback_included
    #endinput
#endif
#define _mod_kill_feedback_included

// Sound Paths (relative to sound/)
new const kf_SND_KILL1[]     = "cs2_kill/kill1.wav";
new const kf_SND_KILL2[]     = "cs2_kill/kill2.wav";
new const kf_SND_KILL3[]     = "cs2_kill/kill3.wav";
new const kf_SND_KILL4[]     = "cs2_kill/kill4.wav";
new const kf_SND_KILL5_ACE[] = "cs2_kill/kill5_ace.wav";
new const kf_SND_DINK[]      = "cs2_kill/headshot_dink.wav";
new const kf_SND_C4_WARN[]   = "cs2_kill/c4_10sec_warning.wav";

// CVars
new kf_pCvar_KillChimes;
new kf_pCvar_HeadshotDink;
new kf_pCvar_C4Warning;
new kf_pCvar_HudCards;

// Player Tracking (per round)
new kf_RoundKills[33];
new Float:kf_LastDinkTime[33];

// Per-Shot Instant Snapshot Tracking (keyed by victim)
new kf_LastShotFlags[33];
new kf_LastShotAttacker[33];
new Float:kf_LastShotTime[33];

// Pre-Shot Sniper Scope Snapshot (keyed by attacker, captured before bolt-action unscope)
new bool:kf_bShotWasScoped[33];
new Float:kf_flSniperShotTime[33];

// Bomb Tracking
new bool:kf_BombPlanted = false;

// HUD Sync Object
new kf_HudSyncCards;

// CS2 Kill Modifiers Tracking (Flashbang Alpha Curve)
new Float:kf_flFlashStart[33];
new Float:kf_flFlashHoldTime[33];
new Float:kf_flFlashFadeTime[33];
new kf_iFlashAlpha[33];
new kf_TempKillFlags[33];
new bool:kf_bSendingCustomDeathMsg = false;

#define KF_MAX_ACTIVE_SMOKES 16
new Float:kf_ActiveSmokes[KF_MAX_ACTIVE_SMOKES][3];
new Float:kf_ActiveSmokeExpire[KF_MAX_ACTIVE_SMOKES];
new kf_SmokeIndex = 0;

// -------------------------------------------------------------
// Lifecycle Hooks
// -------------------------------------------------------------
public Mod_KillFeedback_Init()
{
    // CVars
    kf_pCvar_KillChimes   = register_cvar("amx_cs2_kill_chimes", "1");
    kf_pCvar_HeadshotDink = register_cvar("amx_cs2_headshot_dink", "1");
    kf_pCvar_C4Warning    = register_cvar("amx_cs2_c4_warning", "1");
    kf_pCvar_HudCards     = register_cvar("amx_cs2_hud_cards", "1");

    // Events
    register_logevent("kf_Event_BombPlanted", 3, "2=Planted_The_Bomb");
    register_logevent("kf_Event_BombEnded", 3, "2=Defused_The_Bomb");
    register_logevent("kf_Event_BombEnded", 3, "2=Target_Bombed");
    register_logevent("kf_Event_BombEnded", 2, "1=Target_Bombed");

    // Ham Hooks
    RegisterHam(Ham_Killed, "player", "kf_OnPlayerKilledPost", 1);
    RegisterHam(Ham_TraceAttack, "player", "kf_OnPlayerTraceAttack");
    RegisterHam(Ham_TakeDamage, "player", "kf_OnPlayerTakeDamage");
    RegisterHam(Ham_Weapon_PrimaryAttack, "weapon_awp", "kf_OnSniperPrimaryAttackPre", 0);
    RegisterHam(Ham_Weapon_PrimaryAttack, "weapon_scout", "kf_OnSniperPrimaryAttackPre", 0);

    // DeathMsg Network Interception for Custom CS2 Weapon Killfeed
    register_message(get_user_msgid("DeathMsg"), "kf_Message_DeathMsg");
    register_message(get_user_msgid("ScreenFade"), "kf_Msg_ScreenFade");
    register_forward(FM_EmitSound, "kf_OnEmitSound");

    // HUD Sync
    kf_HudSyncCards = CreateHudSyncObj();
}

public Mod_KillFeedback_Precache()
{
    precache_sound(kf_SND_KILL1);
    precache_sound(kf_SND_KILL2);
    precache_sound(kf_SND_KILL3);
    precache_sound(kf_SND_KILL4);
    precache_sound(kf_SND_KILL5_ACE);
    precache_sound(kf_SND_DINK);
    precache_sound(kf_SND_C4_WARN);
    precache_model("sprites/640hud_cs2.spr");
}

stock kf_ResetPlayerData(id)
{
    kf_RoundKills[id] = 0;
    kf_LastDinkTime[id] = 0.0;
    kf_LastShotFlags[id] = 0;
    kf_LastShotAttacker[id] = 0;
    kf_LastShotTime[id] = 0.0;
    kf_flFlashStart[id] = 0.0;
    kf_flFlashHoldTime[id] = 0.0;
    kf_flFlashFadeTime[id] = 0.0;
    kf_iFlashAlpha[id] = 0;
    kf_TempKillFlags[id] = 0;
    kf_bShotWasScoped[id] = false;
    kf_flSniperShotTime[id] = 0.0;
}

public Mod_KillFeedback_PutInServer(id)
{
    kf_ResetPlayerData(id);
}

public Mod_KillFeedback_Disconnect(id)
{
    kf_ResetPlayerData(id);
}

public Mod_KillFeedback_NewRound()
{
    kf_BombPlanted = false;
    remove_task(TASK_KF_C4_10SEC);

    for (new s = 0; s < KF_MAX_ACTIVE_SMOKES; s++)
    {
        kf_ActiveSmokeExpire[s] = 0.0;
    }

    for (new i = 1; i <= 32; i++)
    {
        kf_ResetPlayerData(i);
    }
}

public Mod_KillFeedback_RoundEnd()
{
    kf_BombPlanted = false;
    remove_task(TASK_KF_C4_10SEC);
}

// -------------------------------------------------------------
// Helper: Check if active weapon fires penetrating bullets
// -------------------------------------------------------------
stock bool:kf_IsBulletWeapon(activeWpn, const szWpn[])
{
    if (szWpn[0] != '^0')
    {
        if (containi(szWpn, "knife") != -1 || equal(szWpn, "grenade") || equal(szWpn, "hegrenade") || equal(szWpn, "world"))
            return false;
    }

    if (activeWpn > 0 && activeWpn <= 30)
    {
        if ((CS2_PRIMARY_WEAPONS_BIT_SUM | CS2_SECONDARY_WEAPONS_BIT_SUM) & (1 << activeWpn))
            return true;
    }

    if (szWpn[0] != '^0')
    {
        return true;
    }

    return false;
}

// -------------------------------------------------------------
// Helper: Check if player is genuinely airborne (exclude ladders)
// -------------------------------------------------------------
stock bool:kf_IsPlayerAirborne(id)
{
    return (!(pev(id, pev_flags) & FL_ONGROUND) && pev(id, pev_movetype) != MOVETYPE_FLY);
}

// -------------------------------------------------------------
// Helper: Check if attacker is genuinely blinded (alpha >= 140)
// -------------------------------------------------------------
stock bool:kf_IsAttackerBlinded(id, Float:flGameTime)
{
    if (kf_iFlashAlpha[id] < 140)
        return false;

    new Float:elapsed = flGameTime - kf_flFlashStart[id];
    if (elapsed < 0.0)
        return false;

    if (elapsed < kf_flFlashHoldTime[id])
        return true;

    if (kf_flFlashFadeTime[id] <= 0.0)
        return false;

    new Float:fadeElapsed = elapsed - kf_flFlashHoldTime[id];
    if (fadeElapsed >= kf_flFlashFadeTime[id])
        return false;

    new Float:currentAlpha = float(kf_iFlashAlpha[id]) * (1.0 - (fadeElapsed / kf_flFlashFadeTime[id]));
    return (currentAlpha >= 140.0);
}

// -------------------------------------------------------------
// Pre-Shot Sniper Scope Capture (before CAWP/CScout resets FOV)
// -------------------------------------------------------------
public kf_OnSniperPrimaryAttackPre(wpnEnt)
{
    if (!get_pcvar_num(pCvar_MasterEnable))
        return HAM_IGNORED;

    if (pev_valid(wpnEnt) != VALID_PDATA)
        return HAM_IGNORED;

    new id = get_pdata_cbase(wpnEnt, m_pPlayer, XO_WEAPON);
    if (id >= 1 && id <= MaxClients && is_user_alive(id))
    {
        new Float:fov = 0.0;
        pev(id, pev_fov, fov);
        kf_bShotWasScoped[id] = (cs_get_user_zoom(id) > CS_SET_NO_ZOOM || (fov > 0.0 && fov < 90.0));
        kf_flSniperShotTime[id] = get_gametime();
    }

    return HAM_IGNORED;
}

// -------------------------------------------------------------
// Headshot Dink Sound & Per-Shot Instant Kill Flag Snapshot
// -------------------------------------------------------------
public kf_OnPlayerTraceAttack(victim, attacker, Float:damage, Float:direction[3], traceresult, damagebits)
{
    if (!get_pcvar_num(pCvar_MasterEnable))
        return HAM_IGNORED;

    if (!is_user_connected(attacker) || !is_user_alive(attacker))
        return HAM_IGNORED;

    if (attacker == victim)
        return HAM_IGNORED;

    if (cs_get_user_team(attacker) == cs_get_user_team(victim) && !get_cvar_num("mp_freeforall"))
        return HAM_IGNORED;

    new Float:flGameTime = get_gametime();

    if (victim >= 1 && victim <= MaxClients)
    {
        new shotFlags = CS2_KILL_NONE;

        // 1. Attacker Airborne (instant check at shot impact, exclude ladders)
        if (kf_IsPlayerAirborne(attacker))
        {
            shotFlags |= CS2_KILL_INAIR;
        }

        // 2. Attacker Blind (alpha >= 140)
        if (kf_IsAttackerBlinded(attacker, flGameTime))
        {
            shotFlags |= CS2_KILL_BLIND;
        }

        // 3. Noscope (bolt-action snipers unscope before FireBullets3, so read pre-shot snapshot)
        new activeWpn = get_user_weapon(attacker);
        if (activeWpn == CSW_AWP || activeWpn == CSW_SCOUT)
        {
            new bool:bWasScoped = (kf_bShotWasScoped[attacker] && (flGameTime - kf_flSniperShotTime[attacker] <= 0.05));
            if (!bWasScoped)
            {
                shotFlags |= CS2_KILL_NOSCOPE;
            }
        }
        else if (activeWpn == CSW_G3SG1 || activeWpn == CSW_SG550)
        {
            new Float:fov = 0.0;
            pev(attacker, pev_fov, fov);
            new zoom = cs_get_user_zoom(attacker);
            if ((zoom <= CS_SET_NO_ZOOM) && (fov == 0.0 || fov >= 90.0))
            {
                shotFlags |= CS2_KILL_NOSCOPE;
            }
        }

        // 4. Wallbang & Smoke (using exact bullet impact coordinates)
        new Float:vecImpact[3], Float:vecAttackerEyes[3];
        get_tr2(traceresult, TR_vecEndPos, vecImpact);
        ExecuteHam(Ham_EyePosition, attacker, vecAttackerEyes);

        if ((damagebits & (DMG_BULLET | DMG_NEVERGIB)) || kf_IsBulletWeapon(activeWpn, ""))
        {
            new tr = create_tr2();
            engfunc(EngFunc_TraceLine, vecAttackerEyes, vecImpact, IGNORE_MONSTERS, attacker, tr);
            new Float:flFraction;
            get_tr2(tr, TR_flFraction, flFraction);
            free_tr2(tr);

            if (flFraction < 0.99)
            {
                shotFlags |= CS2_KILL_WALLBANG;
            }
        }

        if (kf_HasActiveSmokes(flGameTime) && kf_IsLineThroughSmoke(vecAttackerEyes, vecImpact, flGameTime))
        {
            shotFlags |= CS2_KILL_SMOKE;
        }

        // Merge within same frame (handles multi-pellet shotguns); reset on new shot
        if (kf_LastShotTime[victim] == flGameTime && kf_LastShotAttacker[victim] == attacker)
        {
            kf_LastShotFlags[victim] |= shotFlags;
        }
        else
        {
            kf_LastShotFlags[victim] = shotFlags;
            kf_LastShotAttacker[victim] = attacker;
            kf_LastShotTime[victim] = flGameTime;
        }
    }

    // 5. Headshot dink audio
    if (get_pcvar_num(kf_pCvar_HeadshotDink))
    {
        new hitgroup = get_tr2(traceresult, TR_iHitgroup);
        if (hitgroup == HIT_HEAD)
        {
            if (flGameTime - kf_LastDinkTime[attacker] > 0.08)
            {
                kf_LastDinkTime[attacker] = flGameTime;
                client_cmd(attacker, "spk %s", kf_SND_DINK);
            }
        }
    }

    return HAM_IGNORED;
}

public kf_OnPlayerTakeDamage(victim, inflictor, attacker, Float:damage, damagebits)
{
    if (!get_pcvar_num(pCvar_MasterEnable))
        return HAM_IGNORED;

    if (victim < 1 || victim > MaxClients || attacker < 1 || attacker > MaxClients)
        return HAM_IGNORED;

    if (!is_user_connected(attacker) || attacker == victim)
        return HAM_IGNORED;

    if (cs_get_user_team(attacker) == cs_get_user_team(victim) && !get_cvar_num("mp_freeforall"))
        return HAM_IGNORED;

    new Float:flGameTime = get_gametime();

    // If TraceAttack already captured this exact hit in the same frame, preserve it
    if (kf_LastShotTime[victim] == flGameTime && kf_LastShotAttacker[victim] == attacker)
        return HAM_IGNORED;

    // Non-TraceAttack damage (HE Grenade, Molotov, etc.)
    new shotFlags = CS2_KILL_NONE;

    if (inflictor == attacker && is_user_alive(attacker))
    {
        if (kf_IsPlayerAirborne(attacker))
            shotFlags |= CS2_KILL_INAIR;
    }

    if (kf_IsAttackerBlinded(attacker, flGameTime))
        shotFlags |= CS2_KILL_BLIND;

    if (kf_HasActiveSmokes(flGameTime))
    {
        new Float:vecAttackerEyes[3], Float:vecVictimEyes[3];
        ExecuteHam(Ham_EyePosition, attacker, vecAttackerEyes);
        ExecuteHam(Ham_EyePosition, victim, vecVictimEyes);

        if (kf_IsLineThroughSmoke(vecAttackerEyes, vecVictimEyes, flGameTime))
            shotFlags |= CS2_KILL_SMOKE;
    }

    kf_LastShotFlags[victim] = shotFlags;
    kf_LastShotAttacker[victim] = attacker;
    kf_LastShotTime[victim] = flGameTime;

    return HAM_IGNORED;
}

// -------------------------------------------------------------
// CS2 Kill Modifiers Detection (Flash, Smoke)
// -------------------------------------------------------------
public kf_Msg_ScreenFade(msg_id, msg_dest, msg_entity)
{
    if (get_msg_args() >= 7)
    {
        new r = get_msg_arg_int(4);
        new g = get_msg_arg_int(5);
        new b = get_msg_arg_int(6);
        new a = get_msg_arg_int(7);

        // Flashbang fade is pure white with high alpha
        if (r == 255 && g == 255 && b == 255 && a >= 140)
        {
            new duration = get_msg_arg_int(1);
            new holdtime = get_msg_arg_int(2);
            if (msg_entity >= 1 && msg_entity <= MaxClients)
            {
                kf_flFlashStart[msg_entity] = get_gametime();
                kf_flFlashFadeTime[msg_entity] = float(duration) / 4096.0;
                kf_flFlashHoldTime[msg_entity] = float(holdtime) / 4096.0;
                kf_iFlashAlpha[msg_entity] = a;
            }
        }
    }
    return PLUGIN_CONTINUE;
}

public kf_OnEmitSound(entity, channel, const sample[], Float:volume, Float:attenuation, fFlags, pitch)
{
    if (equal(sample, "weapons/sg_explode.wav"))
    {
        if (pev_valid(entity))
        {
            new Float:origin[3];
            pev(entity, pev_origin, origin);
            kf_AddActiveSmoke(origin);
        }
    }
    return FMRES_IGNORED;
}

stock kf_AddActiveSmoke(const Float:origin[3])
{
    kf_ActiveSmokes[kf_SmokeIndex][0] = origin[0];
    kf_ActiveSmokes[kf_SmokeIndex][1] = origin[1];
    kf_ActiveSmokes[kf_SmokeIndex][2] = origin[2];
    kf_ActiveSmokeExpire[kf_SmokeIndex] = get_gametime() + 16.0;

    kf_SmokeIndex = (kf_SmokeIndex + 1) % KF_MAX_ACTIVE_SMOKES;
}

stock bool:kf_HasActiveSmokes(Float:flGameTime)
{
    for (new i = 0; i < KF_MAX_ACTIVE_SMOKES; i++)
    {
        if (flGameTime < kf_ActiveSmokeExpire[i])
            return true;
    }
    return false;
}

stock bool:kf_IsLineThroughSmoke(const Float:start[3], const Float:end[3], Float:now)
{
    new Float:vAB[3], Float:vAS[3];

    vAB[0] = end[0] - start[0];
    vAB[1] = end[1] - start[1];
    vAB[2] = end[2] - start[2];

    new Float:lenSq = vAB[0]*vAB[0] + vAB[1]*vAB[1] + vAB[2]*vAB[2];
    if (lenSq < 1.0)
        return false;

    for (new i = 0; i < KF_MAX_ACTIVE_SMOKES; i++)
    {
        if (now < kf_ActiveSmokeExpire[i])
        {
            vAS[0] = kf_ActiveSmokes[i][0] - start[0];
            vAS[1] = kf_ActiveSmokes[i][1] - start[1];
            vAS[2] = kf_ActiveSmokes[i][2] - start[2];

            new Float:dot = vAS[0]*vAB[0] + vAS[1]*vAB[1] + vAS[2]*vAB[2];
            new Float:t = dot / lenSq;
            if (t < 0.0) t = 0.0;
            else if (t > 1.0) t = 1.0;

            new Float:closest[3];
            closest[0] = start[0] + t * vAB[0];
            closest[1] = start[1] + t * vAB[1];
            closest[2] = start[2] + t * vAB[2];

            // Z-bounded cylinder: smoke cloud sits at origin[2] - 24 .. origin[2] + 85
            if (closest[2] < kf_ActiveSmokes[i][2] - 24.0 || closest[2] > kf_ActiveSmokes[i][2] + 85.0)
                continue;

            new Float:dx = kf_ActiveSmokes[i][0] - closest[0];
            new Float:dy = kf_ActiveSmokes[i][1] - closest[1];
            new Float:distXYSq = dx*dx + dy*dy;

            // Smoke horizontal radius ~135 (135^2 = 18225)
            if (distXYSq <= 18225.0)
            {
                return true;
            }
        }
    }
    return false;
}

// -------------------------------------------------------------
// Centralized CS2 Weapon & Knife Skin Resolvers
// -------------------------------------------------------------
stock bool:kf_ResolveCustomKnife(const szVm[], szToken[], maxTokenLen, szDisplayName[], maxNameLen)
{
    new kType = cs2_detect_knife_type(szVm);
    if (!cs2_has_knife_killfeed_sprite(kType))
        return false;

    cs2_get_knife_token(kType, szToken, maxTokenLen);
    cs2_get_knife_display_name(kType, szDisplayName, maxNameLen);
    return true;
}

stock bool:kf_ResolveCustomGunVariant(wid, const szVm[], szToken[], maxTokenLen, szDisplayName[], maxNameLen)
{
    switch (wid)
    {
        case CSW_TMP, CSW_MAC10:
        {
            if (containi(szVm, "cz75") != -1)
            {
                copy(szToken, maxTokenLen, "cz75");
                copy(szDisplayName, maxNameLen, "CZ75-AUTO");
                return true;
            }
        }
        case CSW_P228:
        {
            if (containi(szVm, "tec9") != -1)
            {
                copy(szToken, maxTokenLen, "tec9");
                copy(szDisplayName, maxNameLen, "TEC-9");
                return true;
            }
        }
        case CSW_XM1014:
        {
            if (containi(szVm, "mag7") != -1 || containi(szVm, "mag") != -1)
            {
                copy(szToken, maxTokenLen, "mag7");
                copy(szDisplayName, maxNameLen, "MAG-7");
                return true;
            }
        }
        case CSW_M3:
        {
            if (containi(szVm, "sawed") != -1)
            {
                copy(szToken, maxTokenLen, "sawedoff");
                copy(szDisplayName, maxNameLen, "SAWED-OFF");
                return true;
            }
        }
        case CSW_M249:
        {
            if (containi(szVm, "negev") != -1)
            {
                copy(szToken, maxTokenLen, "negev");
                copy(szDisplayName, maxNameLen, "NEGEV");
                return true;
            }
        }
        case CSW_MP5NAVY:
        {
            if (containi(szVm, "mp7") != -1)
            {
                copy(szToken, maxTokenLen, "mp7");
                copy(szDisplayName, maxNameLen, "MP7");
                return true;
            }
        }
        case CSW_P90:
        {
            if (containi(szVm, "bizon") != -1)
            {
                copy(szToken, maxTokenLen, "bizon");
                copy(szDisplayName, maxNameLen, "PP-BIZON");
                return true;
            }
        }
    }
    return false;
}

stock kf_EvaluateKillFlags(attacker, victim)
{
    if (victim < 1 || victim > MaxClients || !is_user_connected(attacker) || !is_user_connected(victim))
        return CS2_KILL_NONE;

    if (kf_LastShotAttacker[victim] == attacker && (get_gametime() - kf_LastShotTime[victim] <= 0.05))
        return kf_LastShotFlags[victim];

    return CS2_KILL_NONE;
}

// -------------------------------------------------------------
// Kill Streak & Progressive Chimes + HUD Cards
// -------------------------------------------------------------
public kf_OnPlayerKilledPost(victim, attacker, shouldgib)
{
    if (!get_pcvar_num(pCvar_MasterEnable))
        return HAM_IGNORED;

    if (!is_user_connected(attacker) || !is_user_alive(attacker))
        return HAM_IGNORED;

    if (attacker == victim)
        return HAM_IGNORED;

    if (cs_get_user_team(attacker) == cs_get_user_team(victim) && !get_cvar_num("mp_freeforall"))
        return HAM_IGNORED;

    // Ensure kf_TempKillFlags[victim] is populated if not already set in kf_Message_DeathMsg
    if (victim >= 1 && victim <= MaxClients)
    {
        if (!kf_TempKillFlags[victim])
        {
            kf_TempKillFlags[victim] = kf_EvaluateKillFlags(attacker, victim);
        }
    }

    kf_RoundKills[attacker]++;
    new kills = kf_RoundKills[attacker];

    // CS2 Kill Chimes Sound
    if (get_pcvar_num(kf_pCvar_KillChimes))
    {
        switch (kills)
        {
            case 1: client_cmd(attacker, "spk %s", kf_SND_KILL1);
            case 2: client_cmd(attacker, "spk %s", kf_SND_KILL2);
            case 3: client_cmd(attacker, "spk %s", kf_SND_KILL3);
            case 4: client_cmd(attacker, "spk %s", kf_SND_KILL4);
            default:
            {
                client_cmd(attacker, "spk %s", kf_SND_KILL5_ACE);
                new attacker_name[32];
                get_user_name(attacker, attacker_name, charsmax(attacker_name));
                client_print(0, print_chat, "[CS2] ★★★ ACE! %s eliminated the entire enemy squad! ★★★", attacker_name);
            }
        }
    }

    // CS2 HUD Skull Cards & Weapon Announcement
    if (get_pcvar_num(kf_pCvar_HudCards))
    {
        kf_ShowCs2KillCards(attacker, kills, victim);
    }

    if (victim >= 1 && victim <= MaxClients)
    {
        kf_TempKillFlags[victim] = 0;
        kf_LastShotFlags[victim] = 0;
        kf_LastShotAttacker[victim] = 0;
        kf_LastShotTime[victim] = 0.0;
        kf_iFlashAlpha[victim] = 0;
    }

    return HAM_IGNORED;
}

// -------------------------------------------------------------
// CS2 Custom Weapon Killfeed Network Interceptor
// -------------------------------------------------------------
stock kf_FindWeaponEntByOwner(id, const classname[])
{
    new ent = -1;
    while ((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", classname)) > 0)
    {
        if (pev(ent, pev_owner) == id || get_pdata_cbase(ent, 41, 4) == id)
            return ent;
    }
    return 0;
}

public kf_Message_DeathMsg(msg_id, msg_dest, msg_entity)
{
    if (!get_pcvar_num(pCvar_MasterEnable))
        return PLUGIN_CONTINUE;

    // Prevent recursive loop when re-broadcasting custom DeathMsg
    if (kf_bSendingCustomDeathMsg)
        return PLUGIN_CONTINUE;

    new killer = get_msg_arg_int(1);
    new victim = get_msg_arg_int(2);
    new is_hs  = get_msg_arg_int(3);
    new szWpn[32];
    get_msg_arg_string(4, szWpn, charsmax(szWpn));

    if (killer >= 1 && killer <= MaxClients && is_user_connected(killer))
    {
        new szVm[128];
        pev(killer, pev_viewmodel2, szVm, charsmax(szVm));

        // 1. M4A1 vs M4A4
        if (equal(szWpn, "m4a1"))
        {
            if (containi(szVm, "m4a4") != -1)
            {
                copy(szWpn, charsmax(szWpn), "m4a4");
            }
            else
            {
                new iWpn = kf_FindWeaponEntByOwner(killer, "weapon_m4a1");
                if (pev_valid(iWpn) && pev(iWpn, pev_iuser1) == 1)
                {
                    copy(szWpn, charsmax(szWpn), "m4a4");
                }
            }
        }
        // 2. Knives
        else if (equal(szWpn, "knife"))
        {
            new szCustomToken[32], szDummy[32];
            if (kf_ResolveCustomKnife(szVm, szCustomToken, charsmax(szCustomToken), szDummy, charsmax(szDummy)))
            {
                copy(szWpn, charsmax(szWpn), szCustomToken);
            }
        }
        // 3. SMGs / Pistols / Shotguns / Machineguns
        else
        {
            new activeWpn = get_user_weapon(killer);
            new szCustomToken[32], szDummy[32];
            if (kf_ResolveCustomGunVariant(activeWpn, szVm, szCustomToken, charsmax(szCustomToken), szDummy, charsmax(szDummy)))
            {
                copy(szWpn, charsmax(szWpn), szCustomToken);
            }
        }
    }

    // Determine CS2 modifier flags directly from per-shot snapshot
    new flags = CS2_KILL_NONE;
    if (killer >= 1 && killer <= MaxClients && is_user_connected(killer) && killer != victim)
    {
        flags = kf_EvaluateKillFlags(killer, victim);
    }

    if (is_hs)
    {
        flags |= CS2_KILL_HS;
    }

    // Keep kf_TempKillFlags[victim] updated so HUD cards in kf_OnPlayerKilledPost also receive the flags
    if (victim >= 1 && victim <= MaxClients)
    {
        kf_TempKillFlags[victim] = flags;
    }

    // Re-broadcast 5-byte DeathMsg containing flags
    kf_bSendingCustomDeathMsg = true;
    message_begin(msg_dest ? msg_dest : MSG_ALL, msg_id, _, msg_entity);
    write_byte(killer);
    write_byte(victim);
    write_byte(is_hs);
    write_string(szWpn);
    write_byte(flags);
    message_end();
    kf_bSendingCustomDeathMsg = false;

    return PLUGIN_HANDLED;
}

stock kf_GetActiveWeaponDisplayName(id, szOut[], maxLen)
{
    new szVm[128];
    pev(id, pev_viewmodel2, szVm, charsmax(szVm));

    new wid = get_user_weapon(id);

    // 1. Check knife variants
    if (wid == CSW_KNIFE)
    {
        new szDummy[32], szCustomName[32];
        if (kf_ResolveCustomKnife(szVm, szDummy, charsmax(szDummy), szCustomName, charsmax(szCustomName)))
        {
            copy(szOut, maxLen, szCustomName);
            return;
        }
        copy(szOut, maxLen, "KNIFE");
        return;
    }

    // 2. Check M4A1 vs M4A4
    if (wid == CSW_M4A1)
    {
        if (containi(szVm, "m4a4") != -1) copy(szOut, maxLen, "M4A4");
        else copy(szOut, maxLen, "M4A1-S");
        return;
    }

    // 3. Check custom gun variants (CZ75, Tec-9, MAG-7, Sawed-Off, Negev, MP7, Bizon)
    new szDummy[32], szCustomName[32];
    if (kf_ResolveCustomGunVariant(wid, szVm, szDummy, charsmax(szDummy), szCustomName, charsmax(szCustomName)))
    {
        copy(szOut, maxLen, szCustomName);
        return;
    }

    // 4. Standard CS weapons
    switch (wid)
    {
        case CSW_TMP: copy(szOut, maxLen, "TMP");
        case CSW_MAC10: copy(szOut, maxLen, "MAC-10");
        case CSW_P228: copy(szOut, maxLen, "P228");
        case CSW_XM1014: copy(szOut, maxLen, "XM1014");
        case CSW_M3: copy(szOut, maxLen, "M3 PUMP");
        case CSW_M249: copy(szOut, maxLen, "M249");
        case CSW_MP5NAVY: copy(szOut, maxLen, "MP5");
        case CSW_P90: copy(szOut, maxLen, "P90");
        case CSW_AK47: copy(szOut, maxLen, "AK-47");
        case CSW_AWP: copy(szOut, maxLen, "AWP");
        case CSW_DEAGLE: copy(szOut, maxLen, "DESERT EAGLE");
        case CSW_USP: copy(szOut, maxLen, "USP-S");
        case CSW_GLOCK18: copy(szOut, maxLen, "GLOCK-18");
        case CSW_FAMAS: copy(szOut, maxLen, "FAMAS");
        case CSW_GALIL: copy(szOut, maxLen, "GALIL AR");
        case CSW_SCOUT: copy(szOut, maxLen, "SSG 08");
        case CSW_AUG: copy(szOut, maxLen, "AUG");
        case CSW_SG552: copy(szOut, maxLen, "SG 553");
        case CSW_G3SG1: copy(szOut, maxLen, "G3SG1");
        case CSW_SG550: copy(szOut, maxLen, "SCAR-20");
        case CSW_FIVESEVEN: copy(szOut, maxLen, "FIVE-SEVEN");
        case CSW_ELITE: copy(szOut, maxLen, "DUAL BERETTAS");
        case CSW_HEGRENADE: copy(szOut, maxLen, "HE GRENADE");
        default: copy(szOut, maxLen, "WEAPON");
    }
}

stock kf_ShowCs2KillCards(attacker, kills, victim)
{
    if (!is_user_connected(attacker))
        return;

    new szWpnName[32];
    kf_GetActiveWeaponDisplayName(attacker, szWpnName, charsmax(szWpnName));

    new szVictimName[32];
    if (victim >= 1 && victim <= MaxClients && is_user_connected(victim))
        get_user_name(victim, szVictimName, charsmax(szVictimName));
    else
        copy(szVictimName, charsmax(szVictimName), "ENEMY");

    new flags = 0;
    if (victim >= 1 && victim <= MaxClients)
    {
        flags = kf_TempKillFlags[victim];
    }

    new bool:bIsHeadshot = false;
    if (flags & CS2_KILL_HS)
    {
        bIsHeadshot = true;
    }
    else if (attacker >= 1 && attacker <= MaxClients && (get_gametime() - kf_LastDinkTime[attacker] < 0.35))
    {
        bIsHeadshot = true;
    }

    new card_text[256];
    new hs_badge[96];
    hs_badge[0] = '^0';

    if (bIsHeadshot)
        add(hs_badge, charsmax(hs_badge), "  [🎯 HEADSHOT]");
    if (flags & CS2_KILL_NOSCOPE)
        add(hs_badge, charsmax(hs_badge), "  [NOSCOPE]");
    if (flags & CS2_KILL_WALLBANG)
        add(hs_badge, charsmax(hs_badge), "  [WALLBANG]");
    if (flags & CS2_KILL_SMOKE)
        add(hs_badge, charsmax(hs_badge), "  [SMOKE]");
    if (flags & CS2_KILL_BLIND)
        add(hs_badge, charsmax(hs_badge), "  [BLIND]");
    if (flags & CS2_KILL_INAIR)
        add(hs_badge, charsmax(hs_badge), "  [AIRBORNE]");

    switch (kills)
    {
        case 1:
        {
            set_hudmessage(255, 180, 40, -1.0, 0.80, 0, 0.05, 3.5, 0.1, 0.2);
            formatex(card_text, charsmax(card_text), "💀 ELIMINATED %s with [%s]%s^n[ 1K ]  [ -- ]  [ -- ]  [ -- ]  [ -- ]", szVictimName, szWpnName, hs_badge);
        }
        case 2:
        {
            set_hudmessage(255, 170, 30, -1.0, 0.80, 0, 0.05, 3.5, 0.1, 0.2);
            formatex(card_text, charsmax(card_text), "💀 ELIMINATED %s with [%s]%s^n[ 1K ]  [ 2K ]  [ -- ]  [ -- ]  [ -- ]", szVictimName, szWpnName, hs_badge);
        }
        case 3:
        {
            set_hudmessage(255, 140, 20, -1.0, 0.80, 0, 0.05, 3.5, 0.1, 0.2);
            formatex(card_text, charsmax(card_text), "💀 ELIMINATED %s with [%s]%s^n[ 1K ]  [ 2K ]  [ 3K ]  [ -- ]  [ -- ]", szVictimName, szWpnName, hs_badge);
        }
        case 4:
        {
            set_hudmessage(255, 90, 20, -1.0, 0.80, 0, 0.05, 4.0, 0.1, 0.2);
            formatex(card_text, charsmax(card_text), "💀 ELIMINATED %s with [%s]%s^n[ 1K ]  [ 2K ]  [ 3K ]  [ 4K ]  [ -- ]^n-- ONE ENEMY REMAINING --", szVictimName, szWpnName, hs_badge);
        }
        default:
        {
            set_hudmessage(255, 40, 40, -1.0, 0.78, 1, 0.05, 5.0, 0.1, 0.3);
            formatex(card_text, charsmax(card_text), "★ ★ ★  A C E  ★ ★ ★^n💀 ELIMINATED %s with [%s]%s^n[ 1K ]  [ 2K ]  [ 3K ]  [ 4K ]  [ 5K ]", szVictimName, szWpnName, hs_badge);
        }
    }

    ShowSyncHudMsg(attacker, kf_HudSyncCards, "%s", card_text);
}

// -------------------------------------------------------------
// CS:GO / CS2 10-Second Bomb Warning Music
// -------------------------------------------------------------
public kf_Event_BombPlanted()
{
    if (!get_pcvar_num(pCvar_MasterEnable))
        return;

    if (cs2_state_is_warmup())
        return;

    kf_BombPlanted = true;

    if (!get_pcvar_num(kf_pCvar_C4Warning))
        return;

    new c4_time = get_cvar_num("mp_c4timer");
    if (c4_time <= 0) c4_time = 40;

    new Float:delay_to_10s = float(c4_time) - 10.0;
    if (delay_to_10s > 0.0)
    {
        remove_task(TASK_KF_C4_10SEC);
        set_task(delay_to_10s, "kf_Task_Play10SecWarning", TASK_KF_C4_10SEC);
    }
}

public kf_Task_Play10SecWarning()
{
    if (!kf_BombPlanted || !get_pcvar_num(pCvar_MasterEnable))
        return;

    client_cmd(0, "spk %s", kf_SND_C4_WARN);

    for (new i = 1; i <= 32; i++)
    {
        if (is_user_connected(i) && is_user_alive(i))
        {
            new CsTeams:team = cs_get_user_team(i);
            if (team == CS_TEAM_CT)
            {
                new has_kit = cs_get_user_defuse(i);
                if (has_kit)
                {
                    client_print(i, print_center, "[!] 10 SECONDS LEFT - DEFUSE NOW (DEFUSE KIT: 5s) [!]");
                }
                else
                {
                    client_print(i, print_center, "[!] 10 SECONDS LEFT - NO KIT! MUST DEFUSE IMMEDIATELY [!]");
                }
            }
            else if (team == CS_TEAM_T)
            {
                client_print(i, print_center, "[!] 10 SECONDS TO DETONATION - GUARD THE C4! [!]");
            }
        }
    }
}

public kf_Event_BombEnded()
{
    kf_BombPlanted = false;
    remove_task(TASK_KF_C4_10SEC);
    client_cmd(0, "stopsound");
}
