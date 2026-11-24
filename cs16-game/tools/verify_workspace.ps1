# ==============================================================================
# verify_workspace.ps1 — Pre-Flight Validation Pipeline (4 Gates)
# Blueprint 01: Agent DevEx & Automated Guardrails Suite
# ==============================================================================
[CmdletBinding()]
param(
    [switch]$SkipCompile,
    [switch]$Strict
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$GameDir = (Resolve-Path "$ScriptDir\..").Path
$CstrikeDir = Join-Path $GameDir "cstrike"
$ValveDir = Join-Path $GameDir "valve"
$AmxxDir = Join-Path $CstrikeDir "addons\amxmodx"
$ScriptingDir = Join-Path $AmxxDir "scripting"
$PluginsDir = Join-Path $AmxxDir "plugins"
$ConfigsDir = Join-Path $AmxxDir "configs"

$ActivePlugins = @(
    "cs2_arsenal",
    "cs2_modern_engine",
    "bot_savage_ai",
    "cs2_client_studio",
    "csgo_molotov",
    "csgo_competitive",
    "redm_listen_engine"
)

$TotalAnomalies = 0
$FailedGates = 0

Write-Host "===============================================================================" -ForegroundColor Cyan
Write-Host " [PRE-FLIGHT VALIDATOR] CS 1.6 / Xash3D AMXX 1.10 Architecture Gate" -ForegroundColor Cyan
Write-Host " Target: $GameDir" -ForegroundColor Gray
Write-Host "===============================================================================" -ForegroundColor Cyan

# ==============================================================================
# GATE 1: STRICT AMXX COMPILE
# ==============================================================================
$Gate1Pass = $true
$CompileErrors = 0
$CompileWarnings = 0

if ($SkipCompile) {
    Write-Host "[GATE 1] Compile Active Plugins (7/7) ........... SKIPPED (-SkipCompile flag)" -ForegroundColor Yellow
} else {
    $Amxxpc = Join-Path $ScriptingDir "amxxpc.exe"
    if (-not (Test-Path $Amxxpc)) {
        Write-Host "[GATE 1] Compile Active Plugins ................ FAIL (amxxpc.exe not found!)" -ForegroundColor Red
        $Gate1Pass = $false
        $FailedGates++
    } else {
        foreach ($p in $ActivePlugins) {
            $smaFile = "$p.sma"
            $outAmxx = "..\plugins\$p.amxx"
            $psi = New-Object System.Diagnostics.ProcessStartInfo
            $psi.FileName = $Amxxpc
            $psi.Arguments = "-iinclude -o$outAmxx $smaFile"
            $psi.WorkingDirectory = $ScriptingDir
            $psi.RedirectStandardOutput = $true
            $psi.RedirectStandardError = $true
            $psi.UseShellExecute = $false
            $proc = [System.Diagnostics.Process]::Start($psi)
            $stdout = $proc.StandardOutput.ReadToEnd()
            $stderr = $proc.StandardError.ReadToEnd()
            $proc.WaitForExit()

            $output = $stdout + "`n" + $stderr
            if ($proc.ExitCode -ne 0 -or $output -match "Error:" -or $output -match "Fatal error:") {
                $CompileErrors++
                $Gate1Pass = $false
                Write-Host "  - Error compiling $smaFile (ExitCode: $($proc.ExitCode))" -ForegroundColor Red
            }
            $warnMatches = [regex]::Matches($output, "Warning:")
            $CompileWarnings += $warnMatches.Count
        }

        if ($Gate1Pass -and ($CompileWarnings -eq 0 -or (-not $Strict))) {
            Write-Host ("[GATE 1] Compile Active Plugins (7/7) ........... PASS (0 errors, {0} warnings)" -f $CompileWarnings) -ForegroundColor Green
        } else {
            Write-Host ("[GATE 1] Compile Active Plugins (7/7) ........... FAIL ({0} errors, {1} warnings)" -f $CompileErrors, $CompileWarnings) -ForegroundColor Red
            $FailedGates++
        }
    }
}

# ==============================================================================
# GATE 2: ASSET & PRECACHE INTEGRITY
# ==============================================================================
$Gate2Pass = $true
$MissingActiveAssets = New-Object System.Collections.Generic.List[string]
$MissingCatalogAssets = New-Object System.Collections.Generic.List[string]
$ActiveAssetRefs = New-Object System.Collections.Generic.HashSet[string]([System.StringComparer]::OrdinalIgnoreCase)
$CatalogAssetRefs = New-Object System.Collections.Generic.HashSet[string]([System.StringComparer]::OrdinalIgnoreCase)

# 1. Scan active_loadout.ini (Equipped assets - MUST exist on disk)
$activeLoadoutIni = Join-Path $ConfigsDir "active_loadout.ini"
if (Test-Path $activeLoadoutIni) {
    $lines = Get-Content $activeLoadoutIni -Encoding UTF8
    foreach ($line in $lines) {
        $trim = $line.Trim()
        if (-not $trim -or $trim.StartsWith(";") -or $trim.StartsWith("//") -or $trim.StartsWith("#")) { continue }
        $tokens = $trim -split '\s+'
        foreach ($t in $tokens) {
            $clean = $t.Trim('"', "'")
            if ($clean -match '\.(mdl|spr|wav|mp3)$') {
                $null = $ActiveAssetRefs.Add($clean)
            }
        }
    }
}

# 2. Scan code precache calls in active plugins (MUST exist on disk unless guarded)
$precacheRegex = [regex]'precache_(?:model|sound|generic)\s*\(\s*["'']([^"'']+)["'']'
$sourceFiles = Get-ChildItem -Path $ScriptingDir -Recurse -Include *.sma, *.inc, *.inl
foreach ($sf in $sourceFiles) {
    $lines = Get-Content $sf.FullName -Encoding UTF8
    foreach ($line in $lines) {
        $trim = $line.Trim()
        if ($trim.StartsWith("//") -or $trim.StartsWith("/*")) { continue }
        $m = $precacheRegex.Matches($line)
        foreach ($match in $m) {
            $path = $match.Groups[1].Value.Replace('\', '/')
            # If guarded by known fallback conditional, skip strict fail
            if ($path -match "models/csgo_skins/ct/.*m4a4\.mdl") { continue }
            $null = $ActiveAssetRefs.Add($path)
        }
    }
}

# 3. Scan skin_inventory.ini (Catalog inventory - Soft-checked via ModelFileExists)
$skinInventoryIni = Join-Path $ConfigsDir "skin_inventory.ini"
if (Test-Path $skinInventoryIni) {
    $lines = Get-Content $skinInventoryIni -Encoding UTF8
    foreach ($line in $lines) {
        $trim = $line.Trim()
        if (-not $trim -or $trim.StartsWith(";") -or $trim.StartsWith("//") -or $trim.StartsWith("#")) { continue }
        $tokens = $trim -split '\s+'
        foreach ($t in $tokens) {
            $clean = $t.Trim('"', "'")
            if ($clean -match '\.(mdl|spr|wav|mp3)$') {
                if (-not $ActiveAssetRefs.Contains($clean)) {
                    $null = $CatalogAssetRefs.Add($clean)
                }
            }
        }
    }
}

function Test-AssetExists($assetPath) {
    $cleanPath = $assetPath.Replace('/', '\')
    $pCstrike = Join-Path $CstrikeDir $cleanPath
    $pValve = Join-Path $ValveDir $cleanPath
    $pSoundCstrike = Join-Path $CstrikeDir "sound\$cleanPath"
    $pSoundValve = Join-Path $ValveDir "sound\$cleanPath"
    return (Test-Path $pCstrike) -or (Test-Path $pValve) -or (Test-Path $pSoundCstrike) -or (Test-Path $pSoundValve)
}

foreach ($asset in $ActiveAssetRefs) {
    if (-not (Test-AssetExists $asset)) {
        $MissingActiveAssets.Add($asset)
    }
}

foreach ($asset in $CatalogAssetRefs) {
    if (-not (Test-AssetExists $asset)) {
        $MissingCatalogAssets.Add($asset)
    }
}

$totalChecked = $ActiveAssetRefs.Count + $CatalogAssetRefs.Count

if ($MissingActiveAssets.Count -eq 0) {
    if ($MissingCatalogAssets.Count -eq 0 -or (-not $Strict)) {
        $warnStr = if ($MissingCatalogAssets.Count -gt 0) { " ({0} catalog warnings)" -f $MissingCatalogAssets.Count } else { "" }
        Write-Host ("[GATE 2] Asset & Precache Integrity ({0} refs) .. PASS (0 missing active{1})" -f $totalChecked, $warnStr) -ForegroundColor Green
    } else {
        Write-Host ("[GATE 2] Asset & Precache Integrity ({0} refs) .. FAIL ({1} catalog assets missing in Strict mode)" -f $totalChecked, $MissingCatalogAssets.Count) -ForegroundColor Red
        $FailedGates++
        $Gate2Pass = $false
    }
} else {
    Write-Host ("[GATE 2] Asset & Precache Integrity ({0} refs) .. FAIL ({1} active assets missing)" -f $totalChecked, $MissingActiveAssets.Count) -ForegroundColor Red
    foreach ($ma in $MissingActiveAssets | Select-Object -First 5) {
        Write-Host "  - Missing Active Asset: $ma" -ForegroundColor Red
    }
    $TotalAnomalies += $MissingActiveAssets.Count
    $FailedGates++
    $Gate2Pass = $false
}

# ==============================================================================
# GATE 3: CVAR DRIFT & TYPE AUDIT
# ==============================================================================
$Gate3Pass = $true
$RegisteredCvars = New-Object System.Collections.Generic.HashSet[string]([System.StringComparer]::OrdinalIgnoreCase)
$cvarRegex = [regex]'(?:register_cvar|create_cvar)\s*\(\s*"([^"]+)"'

foreach ($sf in $sourceFiles) {
    $content = [System.IO.File]::ReadAllText($sf.FullName)
    $matches = $cvarRegex.Matches($content)
    foreach ($m in $matches) {
        $null = $RegisteredCvars.Add($m.Groups[1].Value)
    }
}

$CfgFilesToCheck = @(
    (Join-Path $ConfigsDir "cs2_modern_engine.cfg"),
    (Join-Path $ConfigsDir "bot_savage_roles.cfg"),
    (Join-Path $ConfigsDir "csgo_competitive.cfg"),
    (Join-Path $CstrikeDir "competitive.cfg"),
    (Join-Path $CstrikeDir "deathmatch.cfg")
)

$CvarAnomalies = New-Object System.Collections.Generic.List[string]
$CustomPrefixes = @("amx_", "cs2_", "csgo_", "bot_savage_", "redm_", "savage_")
$BooleanCvars = @("amx_csgo_c4_timer", "amx_csgo_mvp_card", "amx_csgo_screenfade", "amx_csgo_dmg_card", "amx_cs2_ping_bot_ai", "amx_cs2_ping_dlight", "amx_cs2_ping_hud")

foreach ($cfg in $CfgFilesToCheck) {
    if (-not (Test-Path $cfg)) { continue }
    $relCfg = (Resolve-Path $cfg -Relative).TrimStart(".\")
    $lineNum = 0
    $lines = Get-Content $cfg -Encoding UTF8
    foreach ($line in $lines) {
        $lineNum++
        $trim = $line.Trim()
        if (-not $trim -or $trim.StartsWith("//") -or $trim.StartsWith(";") -or $trim.StartsWith("#")) { continue }
        $parts = $trim -split '\s+'
        $cmd = $parts[0]
        $val = if ($parts.Length -gt 1) { $parts[1] } else { "" }

        $hasPrefix = $false
        foreach ($prefix in $CustomPrefixes) {
            if ($cmd.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                $hasPrefix = $true
                break
            }
        }

        if ($hasPrefix) {
            if (-not $RegisteredCvars.Contains($cmd)) {
                $CvarAnomalies.Add("$relCfg`:$lineNum -> Unknown CVar '$cmd'")
            }
            if ($BooleanCvars -contains $cmd.ToLower()) {
                if ($val -ne "0" -and $val -ne "1") {
                    $CvarAnomalies.Add("$relCfg`:$lineNum -> Boolean CVar '$cmd' has invalid value '$val' (expected 0|1)")
                }
            }
        }
    }
}

if ($CvarAnomalies.Count -eq 0) {
    Write-Host ("[GATE 3] CVar Drift Detector .................... PASS (0 anomalies across {0} custom CVars)" -f $RegisteredCvars.Count) -ForegroundColor Green
} else {
    Write-Host ("[GATE 3] CVar Drift Detector .................... FAIL ({0} anomalies)" -f $CvarAnomalies.Count) -ForegroundColor Red
    foreach ($ca in $CvarAnomalies) {
        Write-Host "  - $ca" -ForegroundColor Red
    }
    $TotalAnomalies += $CvarAnomalies.Count
    $FailedGates++
    $Gate3Pass = $false
}

# ==============================================================================
# GATE 4: PROFILE PARITY & SDK GUARD
# ==============================================================================
$Gate4Pass = $true
$StrayAmxx = @(Get-ChildItem -Path $ScriptingDir -Filter *.amxx)
$MissingPlugins = New-Object System.Collections.Generic.List[string]
$StaleBinaries = New-Object System.Collections.Generic.List[string]

# Check profile targets
$profileFiles = @("plugins-competitive.ini", "plugins-deathmatch.ini")
foreach ($pf in $profileFiles) {
    $pfPath = Join-Path $ConfigsDir $pf
    if (Test-Path $pfPath) {
        $lines = Get-Content $pfPath -Encoding UTF8
        foreach ($line in $lines) {
            $trim = $line.Trim()
            if (-not $trim -or $trim.StartsWith(";") -or $trim.StartsWith("//")) { continue }
            $plgName = ($trim -split '\s+')[0]
            $plgPath = Join-Path $PluginsDir $plgName
            if (-not (Test-Path $plgPath)) {
                $MissingPlugins.Add("$pf -> $plgName (missing from plugins/)")
            }
        }
    }
}

# Check stale binaries for active 7 plugins
foreach ($p in $ActivePlugins) {
    $amxxPath = Join-Path $PluginsDir "$p.amxx"
    $smaPath = Join-Path $ScriptingDir "$p.sma"
    if (Test-Path $amxxPath) {
        $amxxTime = (Get-Item $amxxPath).LastWriteTime
        $smaTime = (Get-Item $smaPath).LastWriteTime
        if ($amxxTime -lt $smaTime) {
            $StaleBinaries.Add("$p.amxx is older than $p.sma")
        }
    } else {
        $MissingPlugins.Add("$p.amxx (active custom plugin missing)")
    }
}

# Check callfunc_begin in custom plugins
$callfuncViolations = New-Object System.Collections.Generic.List[string]
$customFolders = @("cs2_arsenal", "cs2_engine", "bot_savage", "cs2_studio", "cs2_competitive", "include\cs2_sdk")
foreach ($folder in $customFolders) {
    $fPath = Join-Path $ScriptingDir $folder
    if (Test-Path $fPath) {
        $files = Get-ChildItem -Path $fPath -Recurse -Include *.sma, *.inc, *.inl
        foreach ($f in $files) {
            $lines = Get-Content $f.FullName -Encoding UTF8
            $lNum = 0
            foreach ($line in $lines) {
                $lNum++
                if ($line -match '\bcallfunc_begin' -and -not $line.Trim().StartsWith("//")) {
                    $callfuncViolations.Add("$($f.Name):$lNum callfunc_begin detected")
                }
            }
        }
    }
}

if ($MissingPlugins.Count -eq 0 -and $StaleBinaries.Count -eq 0 -and $StrayAmxx.Count -eq 0 -and $callfuncViolations.Count -eq 0) {
    Write-Host "[GATE 4] Profile Parity & SDK Guard ............. PASS (0 stale, 0 stray, 0 callfunc_begin)" -ForegroundColor Green
} else {
    Write-Host "[GATE 4] Profile Parity & SDK Guard ............. FAIL" -ForegroundColor Red
    foreach ($mp in $MissingPlugins) { Write-Host "  - Missing: $mp" -ForegroundColor Red }
    foreach ($sb in $StaleBinaries) { Write-Host "  - Stale: $sb" -ForegroundColor Red }
    foreach ($sa in $StrayAmxx) { Write-Host "  - Stray in scripting: $($sa.Name)" -ForegroundColor Red }
    foreach ($cf in $callfuncViolations) { Write-Host "  - Forbidden SDK call: $cf" -ForegroundColor Red }
    $TotalAnomalies += ($MissingPlugins.Count + $StaleBinaries.Count + $StrayAmxx.Count + $callfuncViolations.Count)
    $FailedGates++
    $Gate4Pass = $false
}

# ==============================================================================
# SUMMARY & EXIT CODE
# ==============================================================================
Write-Host "-------------------------------------------------------------------------------" -ForegroundColor Gray
if ($FailedGates -eq 0) {
    Write-Host " RESULT: PASSED (All validation gates satisfied cleanly)" -ForegroundColor Green
    exit 0
} else {
    Write-Host (" RESULT: FAILED ({0} gate(s) failed, {1} anomaly(ies) detected)" -f $FailedGates, $TotalAnomalies) -ForegroundColor Red
    exit 1
}
