# ==============================================================================
# check_logs.ps1 — Automated Runtime Log Digest
# Blueprint 01: Agent DevEx & Automated Guardrails Suite
# ==============================================================================
[CmdletBinding()]
param(
    [int]$Sessions = 1,
    [switch]$RawErrors
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$GameDir = (Resolve-Path "$ScriptDir\..").Path
$CstrikeDir = Join-Path $GameDir "cstrike"
$LogsDir = Join-Path $CstrikeDir "addons\amxmodx\logs"

Write-Host "===============================================================================" -ForegroundColor Cyan
Write-Host " [RUNTIME LOG DIGEST] CS 1.6 / Xash3D AMXX & Console Log Parser" -ForegroundColor Cyan
Write-Host " Scanning last $Sessions session(s) | Target: $GameDir" -ForegroundColor Gray
Write-Host "===============================================================================" -ForegroundColor Cyan

$FatalBucket = @{}
$PrecacheBucket = @{}
$InitBucket = @{}
$EngineBucket = @{}

function Add-ToBucket($dict, $line) {
    # Strip timestamp e.g. "L 11/23/2026 - 10:04:05: "
    $clean = $line -replace '^L \d\d/\d\d/\d{4} - \d\d:\d\d:\d\d:\s*', ''
    if ($dict.ContainsKey($clean)) {
        $dict[$clean]++
    } else {
        $dict[$clean] = 1
    }
}

# 1. SCAN AMXX ERROR LOGS
if (Test-Path $LogsDir) {
    $latestErrorLog = Get-ChildItem -Path $LogsDir -Filter "error_*.log" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($latestErrorLog) {
        Write-Host "Latest Error Log: $($latestErrorLog.Name)" -ForegroundColor DarkGray
        $content = Get-Content $latestErrorLog.FullName -Encoding UTF8
        # Slice session
        $markerIndices = @()
        for ($i = 0; $i -lt $content.Length; $i++) {
            if ($content[$i] -match "Start of error session\.") {
                $markerIndices += $i
            }
        }
        $startIndex = 0
        if ($markerIndices.Count -gt 0) {
            $sliceCount = [Math]::Min($Sessions, $markerIndices.Count)
            $startIndex = $markerIndices[$markerIndices.Count - $sliceCount]
        }

        for ($i = $startIndex; $i -lt $content.Length; $i++) {
            $line = $content[$i].Trim()
            if (-not $line -or $line -match "Start of error session\." -or $line -match "Info \(map") { continue }
            
            if ($line -match "Run time error|bad load|Invalid plugin|set_fail_state|Native error|Stack error|forced exit") {
                Add-ToBucket $FatalBucket $line
            } elseif ($line -match "not exist|not precached|can't load|missing") {
                Add-ToBucket $PrecacheBucket $line
            } else {
                Add-ToBucket $FatalBucket $line
            }
        }
    }
}

# 2. SCAN AMXX STANDARD L LOGS
if (Test-Path $LogsDir) {
    $latestLLog = Get-ChildItem -Path $LogsDir -Filter "L*.log" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($latestLLog) {
        Write-Host "Latest AMXX Log:  $($latestLLog.Name)" -ForegroundColor DarkGray
        $content = Get-Content $latestLLog.FullName -Encoding UTF8
        # Find mapchange or log started markers
        $markerIndices = @()
        for ($i = 0; $i -lt $content.Length; $i++) {
            if ($content[$i] -match "-------- Mapchange to|Log file started|Server logging data") {
                $markerIndices += $i
            }
        }
        $startIndex = 0
        if ($markerIndices.Count -gt 0) {
            $sliceCount = [Math]::Min($Sessions, $markerIndices.Count)
            $startIndex = $markerIndices[$markerIndices.Count - $sliceCount]
        }

        for ($i = $startIndex; $i -lt $content.Length; $i++) {
            $line = $content[$i].Trim()
            if (-not $line) { continue }

            if ($line -match "\[CS2|\[Bot Savage|\[CS:GO|\[CSGO|\[ReDM|Mode Competitive") {
                Add-ToBucket $InitBucket $line
            } elseif ($line -match "Run time error|failed|error") {
                Add-ToBucket $FatalBucket $line
            } elseif ($line -match "disabled - check your gamedata|sv global variable is not available") {
                Add-ToBucket $EngineBucket $line
            }
        }
    }
}

# 3. SCAN QCONSOLE LOG
$qconsoleCandidates = @(
    (Join-Path $CstrikeDir "qconsole.log"),
    (Join-Path $GameDir "qconsole.log")
)
$qconsoleFound = $false
foreach ($qc in $qconsoleCandidates) {
    if (Test-Path $qc) {
        $qconsoleFound = $true
        Write-Host "Latest Console:   $qc" -ForegroundColor DarkGray
        $content = Get-Content $qc -Encoding UTF8
        $markerIndices = @()
        for ($i = 0; $i -lt $content.Length; $i++) {
            if ($content[$i] -match "Server logging data|Spawning local server") {
                $markerIndices += $i
            }
        }
        $startIndex = 0
        if ($markerIndices.Count -gt 0) {
            $sliceCount = [Math]::Min($Sessions, $markerIndices.Count)
            $startIndex = $markerIndices[$markerIndices.Count - $sliceCount]
        }
        for ($i = $startIndex; $i -lt $content.Length; $i++) {
            $line = $content[$i].Trim()
            if (-not $line) { continue }
            if ($line -match "SV_StartSound|SZ_GetSpace|Host_Error|Cbuf_AddText|Mod_LoadModel") {
                Add-ToBucket $EngineBucket $line
            } elseif ($line -match "couldn't open|not found|missing") {
                Add-ToBucket $PrecacheBucket $line
            }
        }
        break
    }
}
if (-not $qconsoleFound) {
    Write-Host "Console Log:      (No active qconsole.log found)" -ForegroundColor DarkGray
}

Write-Host "-------------------------------------------------------------------------------" -ForegroundColor Gray

# PRINT BUCKETS
$totalEntries = $FatalBucket.Count + $PrecacheBucket.Count + $InitBucket.Count + $EngineBucket.Count

if ($totalEntries -eq 0) {
    Write-Host "No anomalies or events recorded in scanned session(s). Logs are clean!" -ForegroundColor Green
    exit 0
}

if ($FatalBucket.Count -gt 0) {
    Write-Host "`n[1] FATAL / RUNTIME ERRORS:" -ForegroundColor Red
    foreach ($kv in $FatalBucket.GetEnumerator()) {
        $countStr = if ($kv.Value -gt 1) { " (x$($kv.Value))" } else { "" }
        Write-Host "  * $($kv.Key)$countStr" -ForegroundColor Red
    }
}

if ($PrecacheBucket.Count -gt 0) {
    Write-Host "`n[2] PRECACHE / ASSET WARNINGS:" -ForegroundColor Yellow
    foreach ($kv in $PrecacheBucket.GetEnumerator()) {
        $countStr = if ($kv.Value -gt 1) { " (x$($kv.Value))" } else { "" }
        Write-Host "  * $($kv.Key)$countStr" -ForegroundColor Yellow
    }
}

if ($EngineBucket.Count -gt 0) {
    Write-Host "`n[3] CLIENT / ENGINE WARNINGS:" -ForegroundColor DarkYellow
    foreach ($kv in $EngineBucket.GetEnumerator()) {
        $countStr = if ($kv.Value -gt 1) { " (x$($kv.Value))" } else { "" }
        Write-Host "  * $($kv.Key)$countStr" -ForegroundColor DarkYellow
    }
}

if ($InitBucket.Count -gt 0) {
    Write-Host "`n[4] PLUGIN INIT STATUS:" -ForegroundColor Cyan
    foreach ($kv in $InitBucket.GetEnumerator()) {
        $countStr = if ($kv.Value -gt 1) { " (x$($kv.Value))" } else { "" }
        Write-Host "  * $($kv.Key)$countStr" -ForegroundColor Gray
    }
}

Write-Host "===============================================================================" -ForegroundColor Cyan
