#if defined _savage_defs_included
    #endinput
#endif
#define _savage_defs_included

#define PLUGIN_VERSION "3.4.0"
#define PLUGIN_AUTHOR  "Antigravity"

#define TASK_GRENADE_CHECK    2048

// Competitive 5-Role Strings (Maps to SavageRole enum in bot_savage_ai.inc)

new const g_szRoleNames[][] = {
    "Unassigned",
    "Entry Fragger",
    "AWPer",
    "Support",
    "Lurker / Anchor",
    "IGL"
};

new const g_szRoleTags[][] = {
    "[NONE]",
    "[Entry]",
    "[AWPer]",
    "[Support]",
    "[Lurker]",
    "[IGL]"
};

// Strategic Calls Enum
enum {
    STRAT_NONE = 0,
    STRAT_RUSH_A,
    STRAT_RUSH_B,
    STRAT_DEFAULT,
    STRAT_SPLIT,
    STRAT_ECO
};

new const g_szStratNames[][] = {
    "None",
    "Fast Rush Bombsite A",
    "Execute Bombsite B",
    "Default 1-3-1 (Hold for Picks)",
    "Split Mid to Site",
    "Full Eco / Group Save"
};

// CVAR Pointers - Core
new g_pCvarEnable;
new g_pCvarMicroOverride;
new g_pCvarCounterStrafe;
new g_pCvarJiggle;
new g_pCvarTradeFrag;
new g_pCvarAntiFlash;
new g_pCvarTriggerbot;
new g_pCvarHiveMind;
new g_pCvarReloadCover;

// CVAR Pointers - Combat & Movement
new g_pCvarRecoil;
new g_pCvarBhop;
new g_pCvarWallbang;
new g_pCvarDoublePeek;
new g_pCvarFakeDefuse;
new g_pCvarJumpScout;
new g_pCvarDropGuns;

// CVAR Pointers - Competitive & Tactics
new g_pCvarRoles;
new g_pCvarHolding;
new g_pCvarCalls;
new g_pCvarKnifeMidBrawl;
new g_pCvarKnifeRoundActive;
new g_pCvarWeaponScavenge;
new g_pCvarRoleDeliver;
new g_pCvarChatCalls;
new g_pCvarChatCooldown;

// Danger & Care Zones Enum (v3.4)
enum {
    ZONE_NONE = 0,
    ZONE_SITE_A,
    ZONE_SITE_B,
    ZONE_MID,
    ZONE_SHORT,
    ZONE_LONG,
    ZONE_FLANK,
    ZONE_AWP,
    ZONE_PUSH
};
new g_iActiveDangerZone[CsTeams];
new Float:g_flDangerZoneExpire[CsTeams];
new Float:g_vecDangerPoint[CsTeams][3];

// Tactical Chat & Pacing State Tracking (v3.4)
new g_iTradeLeader[33];
new Float:g_flTradeFollowUntil[33];
new Float:g_flRearFlankUntil[33];
new Float:g_vecRearFlankAngle[33][3];
new Float:g_flSilentWalkUntil[CsTeams];
new Float:g_flHoldAngleUntil[CsTeams];
new Float:g_flLastChatCallTime[33];

// Damage Tracking for Tactical Intel
new g_iLastDamageTarget[33];
new g_iLastDamageDealt[33];
new Float:g_flLastDamageTime[33];

// State Tracking
new g_iPlayerRole[33];
new bool:g_bRoleSelected[33];
new g_iTeamStrat[CsTeams];
new bool:g_bInFreezeTime = false;
new g_iRoundCount = 0;
new Float:g_flRoundStartTime = 0.0;

// Team Economy Mode Enum & Interactive Calls
enum {
    ECON_NORMAL = 0,
    ECON_ECO,
    ECON_FORCE,
    ECON_HALF,
    ECON_FULL
};
new g_iTeamEconomyCall[CsTeams] = { ECON_NORMAL, ECON_NORMAL, ECON_NORMAL };

// Tactical Mid Lineup & Bombsite Tracking
new bool:g_bMidSmokedThisRound[CsTeams] = { false, false, false };
new bool:g_bMidFlashedThisRound[CsTeams] = { false, false, false };
new Float:g_vecBombsiteA[3];
new Float:g_vecBombsiteB[3];
new bool:g_bBombsitesFound = false;
new Float:g_flLineupThrowUntil[33];
new bool:g_bLineupActive[33];

// Saved Weapon & Role Delivery State Tracking
new g_iSavedWeapon[33];
new bool:g_bSavedWeaponForRole[33];
new bool:g_bRoundEnded = false;
new Float:g_flRoundEndTime;
new g_iScavengeTarget[33];
new Float:g_flScavengeExpire[33];
new Float:g_flLastWeaponSwapTime[33];

// Holding Angle State Tracking (Engine-Safe Maxspeed & 1Hz Tick-Gate)
new bool:g_bHoldingAngle[33];
new Float:g_flHoldingUntil[33];
new Float:g_flNextHoldCheck[33];
new Float:g_flOriginalMaxSpeed[33];

// Mid Point & Knife Round Tracking
new Float:g_vecMapMidPoint[3];
new bool:g_bMidPointCalculated;
new Float:g_flKnifeRoundStartTime;
new bool:g_bAntiStallTriggered;

// Combat State Tracking
new Float:g_flLastCombatTime[33];
new Float:g_flNextStrafeSwitch[33];
new Float:g_flNextTriggerCheck[33];
new Float:g_flTriggerBurstUntil[33];
new bool:g_bStrafeDirection[33];
new bool:g_bDuckTapActive[33];
new Float:g_flFlashDodgeUntil[33];

// Trade-Frag State Tracking
new g_iTradeTarget[33];
new Float:g_flTradeExpire[33];

// Hive-Mind Team Intel
new Float:g_vecTeamThreatOrigin[CsTeams][3];
new Float:g_flTeamThreatTime[CsTeams];
new g_iTeamThreatTarget[CsTeams];

// Sound ESP & Wallbang Tracking
new g_iSoundTarget[33];
new Float:g_flSoundTargetExpire[33];
new Float:g_vecSoundTargetOrigin[33][3];
new Float:g_flNextWallbangBurst[33];

// Fake Defuse State Tracking
new bool:g_bDidFakeDefuse[33];
new Float:g_flFakeDefuseAmbushUntil[33];
new Float:g_vecFakeDefuseAmbushAngle[33][3];

// Double-Peek Pair Assignment
new g_iDoublePeekPartner[33];
new bool:g_bIsCrouchPeeker[33];
new Float:g_flDoublePeekExpire[33];

// Tactical Callout Cooldowns
new Float:g_flNextContactCall[CsTeams];
new Float:g_flNextBombCall[CsTeams];
new Float:g_flNextTeamMolotovTime[CsTeams];

// YaPB <-> AMXX Action Lease Arbitration Bridge (BP-01)
enum SavageLeaseType
{
    LEASE_NONE = 0,
    LEASE_TACTICAL_HOLD = 1,
    LEASE_UTILITY_THROW = 2,
    LEASE_HAZARD_EVADE = 3
};

new bool:g_bYapbAvailable = false;
new SavageLeaseType:g_iBotLeaseType[33];
new Float:g_flBotLeaseUntil[33];
new Float:g_flBotLeaseStart[33];
new Float:g_vecBotLeaseTarget[33][3];
new g_iBotLeaseWeaponCSW[33];
new bool:g_bBotLeaseAttackSent[33];
new Float:g_flNextKnifeGoalPush[33];
new Float:g_flNextMacroGoalPush[33];
new bool:g_bBombPlantedActive = false;
new bool:g_bRetakeUtilThrown = false;
new Float:g_vecPlantedC4Origin[3];
new g_iRetakeSyncTicks = 0;
