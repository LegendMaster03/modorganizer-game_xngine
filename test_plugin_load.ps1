Write-Host "=== MO2 Plugin Diagnostic ===" -ForegroundColor Cyan

# Check if DLLs exist
Write-Host "`n1. Checking DLL files..." -ForegroundColor Yellow
$dlls = @("game_redguard.dll", "game_arena.dll", "game_daggerfall.dll", "game_battlespire.dll")
foreach ($dll in $dlls) {
    $path = "C:\Modding\MO2\plugins\$dll"
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
    $path = "C:\Modding\MO2\plugins\$json"
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
    $path = "F:\Modding\MO2\dlls\$qtdll"
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
$env:QT_PLUGIN_PATH = "C:\Modding\MO2\plugins"

# Kill existing MO2
Stop-Process -Name "ModOrganizer" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2

# Clear old log
Remove-Item "$env:LOCALAPPDATA\ModOrganizer\Skyrim Special Edition\logs\mo_interface.log" -Force -ErrorAction SilentlyContinue

# Start MO2
cd "F:\Modding\MO2"
Start-Process "ModOrganizer.exe" -RedirectStandardError "D:\Projects\modorganizer-game_xngine\qt_debug.txt"

Write-Host "`n5. Waiting for MO2 to start..." -ForegroundColor Yellow
Start-Sleep -Seconds 10

# Check debug output
if (Test-Path "D:\Projects\modorganizer-game_xngine\qt_debug.txt") {
    Write-Host "`n6. Qt Debug Output:" -ForegroundColor Yellow
    Get-Content "D:\Projects\modorganizer-game_xngine\qt_debug.txt" | Select-Object -First 100
} else {
    Write-Host "  No debug output captured" -ForegroundColor Gray
}

Write-Host "`n=== Check MO2's instance selector for Redguard ===" -ForegroundColor Cyan
