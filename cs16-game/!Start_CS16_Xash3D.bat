@echo off
setlocal enabledelayedexpansion
title Counter-Strike 1.6 (Pure Xash3D FWGS) - Competitive Match & Warmup
echo ===============================================================================
echo Starting Counter-Strike 1.6 on Xash3D Engine (Competitive Mode)...
echo Mode: Warmup Deathmatch -> .ready -> Knife Round -> MR12 Match LIVE
echo ===============================================================================

cd /d "%~dp0"

:: 1. Apply Competitive Match Profile (Zero-delay atomic switch)
copy /y "cstrike\addons\amxmodx\configs\plugins-competitive.ini" "cstrike\addons\amxmodx\configs\plugins.ini" >nul 2>&1
(echo exec competitive.cfg)> "cstrike\mode.cfg"

:: 2. Launch Pure Xash3D Engine Standalone
start "" "xash3d.exe" -game cstrike -dll addons/metamod/dlls/metamod.dll -fullscreen -high -condebug +log on +mp_logfile 1 +exec competitive.cfg
exit
