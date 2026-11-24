@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

set PLUGINS=cs2_arsenal cs2_modern_engine bot_savage_ai cs2_client_studio csgo_molotov csgo_competitive redm_listen_engine
set COUNT=0

echo ===============================================================================
echo AMX Mod X 1.10 Build Gate - Compiling Active Custom Plugins
echo ===============================================================================

for %%p in (%PLUGINS%) do (
    set /a COUNT+=1
    echo [!COUNT!] Compiling %%p.sma...
    amxxpc.exe -iinclude -o..\plugins\%%p.amxx %%p.sma
    if errorlevel 1 (
        echo.
        echo [BUILD FAILED] Error compiling %%p.sma! Aborting.
        exit /b 1
    )
    echo.
)

echo ===============================================================================
echo [BUILD SUCCESS] All !COUNT! plugins compiled cleanly into ..\plugins\
echo ===============================================================================
exit /b 0
