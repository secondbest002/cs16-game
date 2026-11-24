@echo off
setlocal enabledelayedexpansion
title Counter-Strike 1.6 (Pure Xash3D FWGS) - ReDeathmatch
echo ===============================================================================
echo Starting Counter-Strike 1.6 on Xash3D Engine with ReDeathmatch...
echo Mode: Instant Respawn, JSON Random Spawns, Weapon Menus, YaPB DM Bots
echo ===============================================================================

cd /d "%~dp0"

:: 1. Apply Deathmatch Profile (Zero-delay atomic switch)
copy /y "cstrike\addons\amxmodx\configs\plugins-deathmatch.ini" "cstrike\addons\amxmodx\configs\plugins.ini" >nul 2>&1
(echo exec deathmatch.cfg)> "cstrike\mode.cfg"

:: 2. Launch Pure Xash3D Engine Standalone
start "" "xash3d.exe" -game cstrike -dll addons/metamod/dlls/metamod.dll -fullscreen -high -condebug +log on +mp_logfile 1 +exec deathmatch.cfg
exit
