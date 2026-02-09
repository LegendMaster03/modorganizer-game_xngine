# Deployment script for XnGine game plugins
# This script handles locked files better than batch copy commands

param(
    [string]$BuildDir = ".\build",
    [string]$MO2PluginsDir = ""
)

$localEnv = Join-Path $PSScriptRoot "config\local.env.ps1"
if (Test-Path $localEnv) {
    . $localEnv
}

if (-not $MO2PluginsDir) {
    $MO2PluginsDir = $env:MO2_PLUGINS_DIR
}

if (-not $MO2PluginsDir) {
    Write-Host "ERROR: MO2_PLUGINS_DIR is not set. Use config\local.env.ps1 or an environment variable." -ForegroundColor Red
    exit 1
}

Write-Host "=== Deploying XnGine Plugins ===" -ForegroundColor Cyan

# Define DLL files to deploy and their source locations
$DllsToDeployParam = @(
    @{ src = "$BuildDir\bin\Release\plugins\game_battlespire.dll"; dst = "$MO2PluginsDir\game_battlespire.dll" },
    @{ src = "$BuildDir\bin\Release\plugins\game_redguard.dll"; dst = "$MO2PluginsDir\game_redguard.dll" },
    @{ src = "$BuildDir\bin\Release\plugins\game_daggerfall.dll"; dst = "$MO2PluginsDir\game_daggerfall.dll" }
)

$JsonFilesToDeploy = @(
    @{ src = ".\src\games\battlespire\gamebattlespire.json"; dst = "$MO2PluginsDir\game_battlespire.json" },
    @{ src = ".\src\games\redguard\gameredguard.json"; dst = "$MO2PluginsDir\game_redguard.json" },
    @{ src = ".\src\games\daggerfall\gamedaggerfall.json"; dst = "$MO2PluginsDir\game_daggerfall.json" }
)

# Function to forcefully move a file (handles locked files)
function Move-FileIgnoreLocks {
    param([string]$Source, [string]$Destination)
    
    if (-not (Test-Path $Source)) {
        Write-Host "  ✗ Source not found: $Source" -ForegroundColor Yellow
        return $false
    }
    
    # Try direct copy first
    try {
        Copy-Item -Path $Source -Destination $Destination -Force -ErrorAction Stop
        Write-Host "  ✓ $(Split-Path $Destination -Leaf)" -ForegroundColor Green
        return $true
    } catch {
        # If file is locked, move the old one and try again
        if (Test-Path $Destination) {
            try {
                Move-Item -Path $Destination -Destination "$Destination.backup" -Force -ErrorAction Stop
                Write-Host "  → Moved old file to .backup"
            } catch {
                Write-Host "  ✗ Failed to backup old file: $_" -ForegroundColor Red
                return $false
            }
        }
        
        # Try copy again
        try {
            Copy-Item -Path $Source -Destination $Destination -Force -ErrorAction Stop
            Write-Host "  ✓ $(Split-Path $Destination -Leaf)" -ForegroundColor Green
            return $true
        } catch {
            Write-Host "  ✗ Failed to deploy $(Split-Path $Destination -Leaf): $_" -ForegroundColor Red
            return $false
        }
    }
}

# Deploy DLLs
Write-Host "`nDeploying DLLs..." -ForegroundColor Yellow
$dllSuccess = $true
foreach ($dll in $DllsToDeployParam) {
    if (-not (Move-FileIgnoreLocks $dll.src $dll.dst)) {
        $dllSuccess = $false
    }
}

# Deploy JSON files
Write-Host "`nDeploying JSON metadata..." -ForegroundColor Yellow
foreach ($json in $JsonFilesToDeploy) {
    Move-FileIgnoreLocks $json.src $json.dst | Out-Null
}

Write-Host "`n========================================" -ForegroundColor Cyan
if ($dllSuccess) {
    Write-Host "Deployment SUCCESSFUL" -ForegroundColor Green
} else {
    Write-Host "Deployment completed with errors" -ForegroundColor Yellow
}
Write-Host "========================================`n" -ForegroundColor Cyan
