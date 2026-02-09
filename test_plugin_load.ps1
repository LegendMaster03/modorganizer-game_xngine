Write-Host "=== MO2 Plugin Diagnostic ===" -ForegroundColor Cyan

$localEnv = Join-Path $PSScriptRoot "config\local.env.ps1"
if (Test-Path $localEnv) {
    . $localEnv
}

$mo2PluginsDir = $env:MO2_PLUGINS_DIR
$mo2Root = $env:MO2_ROOT
if (-not $mo2Root -and $mo2PluginsDir) {
    $mo2Root = Split-Path $mo2PluginsDir -Parent
}

if (-not $mo2PluginsDir) {
    Write-Host "ERROR: MO2_PLUGINS_DIR is not set. Use config\local.env.ps1 or an environment variable." -ForegroundColor Red
    exit 1
}

if (-not $mo2Root) {
    Write-Host "ERROR: MO2_ROOT is not set. Use config\local.env.ps1 or an environment variable." -ForegroundColor Red
    exit 1
}

# Check if DLLs exist
Write-Host "`n1. Checking DLL files..." -ForegroundColor Yellow
$dlls = @("game_redguard.dll", "game_arena.dll", "game_daggerfall.dll", "game_battlespire.dll")
foreach ($dll in $dlls) {
    $path = Join-Path $mo2PluginsDir $dll
    if (Test-Path $path) {
        $size = (Get-Item $path).Length
        Write-Host "  ✓ $dll ($size bytes)" -ForegroundColor Green
    } else {
        Write-Host "  ✗ $dll MISSING" -ForegroundColor Red
    }
}

# Check JSON files
Write-Host "`n2. Checking JSON metadata..." -ForegroundColor Yellow
foreach ($dll in $dlls) {
    $json = $dll -replace ".dll", ".json"
    $path = Join-Path $mo2PluginsDir $json
    if (Test-Path $path) {
        $content = Get-Content $path | ConvertFrom-Json
        Write-Host "  ✓ $json - $($content.name)" -ForegroundColor Green
    } else {
        Write-Host "  ✗ $json MISSING" -ForegroundColor Red
    }
}

# Check Qt dependencies
Write-Host "`n3. Checking Qt dependencies..." -ForegroundColor Yellow
$qtDlls = @("Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "uibase.dll")
foreach ($qtdll in $qtDlls) {
    $path = Join-Path $mo2Root "dlls\$qtdll"
    if (Test-Path $path) {
        Write-Host "  ✓ $qtdll" -ForegroundColor Green
    } else {
        Write-Host "  ✗ $qtdll MISSING" -ForegroundColor Red
    }
}

# Start MO2 with Qt debug output
Write-Host "`n4. Starting MO2 with Qt debug output..." -ForegroundColor Yellow
Write-Host "   This will show why plugins fail to load..." -ForegroundColor Gray

# Set Qt debug environment variables
$env:QT_DEBUG_PLUGINS = "1"
$env:QT_PLUGIN_PATH = $mo2PluginsDir

# Kill existing MO2
Stop-Process -Name "ModOrganizer" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2

# Clear old log
Remove-Item "$env:LOCALAPPDATA\ModOrganizer\Skyrim Special Edition\logs\mo_interface.log" -Force -ErrorAction SilentlyContinue

# Start MO2
Set-Location $mo2Root
$qtDebugPath = Join-Path $PSScriptRoot "qt_debug.txt"
Start-Process "ModOrganizer.exe" -RedirectStandardError $qtDebugPath

Write-Host "`n5. Waiting for MO2 to start..." -ForegroundColor Yellow
Start-Sleep -Seconds 10

# Check debug output
if (Test-Path $qtDebugPath) {
    Write-Host "`n6. Qt Debug Output:" -ForegroundColor Yellow
    Get-Content $qtDebugPath | Select-Object -First 100
} else {
    Write-Host "  No debug output captured" -ForegroundColor Gray
}

Write-Host "`n=== Check MO2's instance selector for Redguard ===" -ForegroundColor Cyan
