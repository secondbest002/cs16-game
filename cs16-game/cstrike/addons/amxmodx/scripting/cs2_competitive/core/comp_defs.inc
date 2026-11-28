#if defined _comp_defs_included
    #endinput
#endif
#define _comp_defs_included

#define PLUGIN  "CS:GO Competitive Match Mod"
#define VERSION "1.1.0"
#define AUTHOR  "Antigravity"


enum {
    STATE_WARMUP = 0,
    STATE_COUNTDOWN,
    STATE_KNIFE_ROUND,
    STATE_SIDE_PICK,
    STATE_LIVE_HALF_1,
    STATE_HALFTIME,
    STATE_LIVE_HALF_2,
    STATE_OVERTIME_1,
    STATE_OVERTIME_HALFTIME,
    STATE_OVERTIME_2,
    STATE_FINISHED
};

new g_MatchState = STATE_WARMUP;

// Match Scores & Rounds
new g_ScoreCT = 0;
new g_ScoreT = 0;
new g_RoundNumber = 0;
new g_MaxRoundsHalf = 12; // MR12 default
new g_TargetWinScore = 13; // First to 13 wins

// Overtime
new g_OTRoundNumber = 0;
new g_OTMaxRoundsHalf = 3; // MR3 Overtime
new g_OTTargetWinScore = 16;

// Knife Round
new CsTeams:g_KnifeWinner = CS_TEAM_UNASSIGNED;
new g_KnifeChooserId = 0;
new g_SidePickTimer = 15;
new bool:g_bKnifeRoundLive = false;

// Player Ready State
new bool:g_PlayerReady[33];
new g_CountdownTimer = 5;

// Tactical Timeout
new g_TimeoutsCT = 0;
new g_TimeoutsT = 0;
new bool:g_TimeoutQueuedCT = false;
new bool:g_TimeoutQueuedT = false;
new bool:g_IsTimeoutActive = false;
new g_TimeoutRemaining = 0;
new CsTeams:g_TimeoutTeam = CS_TEAM_UNASSIGNED;
new bool:g_bInFreezePeriod = false;

// Damage Tracking (Current Round)
new g_DamageDealt[33][33];
new g_HitsDealt[33][33];

// MVP Tracking
new g_KillsInRound[33];
new g_BombPlanter = 0;
new g_BombDefuser = 0;

// Match-Wide Scoreboard Telemetry (BP-04)
enum {
    WIN_REASON_NONE = 0,
    WIN_REASON_ELIM,
    WIN_REASON_BOMB,
    WIN_REASON_DEFUSE,
    WIN_REASON_TIME
};

#define MAX_ROUND_HISTORY 36

new g_PlayerAssists[33];
new g_PlayerMVPs[33];
new g_TotalMatchDamage[33];
new g_PendingWinReason = WIN_REASON_NONE;
new Float:g_LastRoundWinTime = 0.0;
new g_RoundHistCount = 0;
new g_RoundHistWinner[MAX_ROUND_HISTORY];
new g_RoundHistReason[MAX_ROUND_HISTORY];
new g_MsgPlayerStats = 0;
new g_MsgRoundHist = 0;

stock bool:IsLiveMatchState()
{
    return (g_MatchState == STATE_LIVE_HALF_1 || g_MatchState == STATE_LIVE_HALF_2 ||
            g_MatchState == STATE_OVERTIME_1 || g_MatchState == STATE_OVERTIME_2);
}

// CS:GO Loss Bonus Ladder (Levels 0..4)
// Level 0 = $1400, Level 1 = $1900, Level 2 = $2400, Level 3 = $2900, Level 4 = $3400
new g_LossStreakCT = 0;
new g_LossStreakT = 0;

// Cvars & Lifecycle Control (AMXX 1.10)
new pCvar_Mode;
new g_iCsgoMode = 1;
new pCvar_RedmActive;
new HamHook:g_hHamSpawnPost;
new HamHook:g_hHamKilledPost;
new HamHook:g_hHamDeployGlock;
new HamHook:g_hHamDeployUsp;
new bool:g_bEquippingBot[33];
new g_iBotEquipRetries[33];

// Master Guard O(1) Macro
#define IS_COMPETITIVE_ENABLED() (g_iCsgoMode > 0)
#define CHECK_COMPETITIVE_ACTIVE(%1) if (!IS_COMPETITIVE_ENABLED()) return %1

new pCvar_MaxRounds;
new pCvar_KnifeRound;
new pCvar_Warmup;
new pCvar_Timeouts;
new pCvar_TimeoutDuration;
new pCvar_OTMoney;
new pCvar_BotAutoReady;
new pCvar_LossBonus;
new pCvar_HudStyle;
new pCvar_MvpCard;
new pCvar_C4Timer;
new pCvar_ScreenFade;
new pCvar_DmgCard;
new pCvar_FreezeTime;
new pCvar_CurrentRound;
new pCvar_WarmupTime;

// Warmup DM Duration Tracker (in seconds)
new g_WarmupTimeLeft = 3300;

// Multiple HUD Sync Channels
new g_HudSyncHeader;
new g_HudSyncSubHeader;
new g_HudSyncDamage;

// C4 Plant Tracker
new bool:g_BombPlanted = false;
new Float:g_BombPlantedTime = 0.0;
new g_C4TimerCvar = 40;

// ScreenFade Engine Constants
#define FFADE_IN         0x0000
#define FFADE_OUT        0x0001
#define FFADE_MODULATE   0x0002
#define FFADE_STAYOUT    0x0004

// Player Entity Offsets (Linux diff: +5 for player)
#define OFFSET_TEAM 114
#define EXTRAOFFSET_PLAYER 5

// All buy aliases and equipment commands in Counter-Strike 1.6
new const g_BuyCommands[][] = {
    "buy", "buyequip", "cl_autobuy", "cl_rebuy", "cl_setautobuy", "cl_setrebuy",
    "rebuy", "autobuy", "buyammo1", "buyammo2", "primammo", "secammo",
    // Pistols
    "glock", "usp", "p228", "deagle", "elites", "fn57",
    // Shotguns
    "m3", "xm1014",
    // SMGs
    "mac10", "tmp", "mp5", "ump45", "p90",
    // Rifles
    "galil", "famas", "ak47", "m4a1", "sg552", "aug", "scout", "awp", "g3sg1", "sg550",
    // Machine Gun
    "m249",
    // Equipment & Grenades
    "vest", "vesthelm", "flash", "hegren", "sgren", "defuser", "nvgs", "shield"
};

// Safe wrapper around team lookup that works on bots/players without relying on pvPrivateData
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

    // Fallback to cstrike native in case AMX internal table hasn't updated yet
    new CsTeams:csteam = cs_get_user_team(id);
    if (csteam == CS_TEAM_T || csteam == CS_TEAM_CT || csteam == CS_TEAM_SPECTATOR)
        return csteam;

    // Fallback to CBasePlayer m_iTeam offset via fakemeta
    new iTeam = get_pdata_int(id, OFFSET_TEAM, EXTRAOFFSET_PLAYER);
    if (iTeam == 1) return CS_TEAM_T;
    if (iTeam == 2) return CS_TEAM_CT;
    if (iTeam == 3) return CS_TEAM_SPECTATOR;

    return CS_TEAM_UNASSIGNED;
}

// Auto-ready YaPB bots connected and assigned to CT or T
stock AutoReadyBots()
{
    new count = 0;
    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_connected(i) && is_user_bot(i))
        {
            new CsTeams:bteam = safe_cs_get_user_team(i);
            if (bteam == CS_TEAM_CT || bteam == CS_TEAM_T)
            {
                if (!g_PlayerReady[i])
                {
                    g_PlayerReady[i] = true;
                    count++;
                }
            }
        }
    }
    return count;
}

// Check if any human player in CT or T is already marked ready
stock bool:IsAnyHumanReady()
{
    for (new i = 1; i <= MaxClients; i++)
    {
        if (is_user_connected(i) && !is_user_bot(i) && g_PlayerReady[i])
        {
            new CsTeams:team = safe_cs_get_user_team(i);
            if (team == CS_TEAM_CT || team == CS_TEAM_T)
            {
                return true;
            }
        }
    }
    return false;
}
