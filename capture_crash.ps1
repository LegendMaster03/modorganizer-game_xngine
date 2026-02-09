# Script to capture Redguard plugin crash with instructions
Write-Host "=== MO2 Redguard Plugin Crash Capture ===" -ForegroundColor Cyan
Write-Host ""

# Check if DebugView is available
$dbgviewPath = "C:\Sysinternals\Dbgview.exe"
if (-not (Test-Path $dbgviewPath)) {
    $dbgviewPath = "Dbgview.exe"
}

Write-Host "INSTRUCTIONS:" -ForegroundColor Yellow
Write-Host "1. Download DebugView from: https://learn.microsoft.com/en-us/sysinternals/downloads/debugview" -ForegroundColor White
Write-Host "2. Run DebugView.exe as Administrator" -ForegroundColor White
Write-Host "3. In DebugView: Capture > Capture Global Win32" -ForegroundColor White
Write-Host "4. In DebugView: Edit > Filter/Highlight > Include: *GameRedguard*" -ForegroundColor White
Write-Host "5. Launch MO2 using this script" -ForegroundColor White
Write-Host "6. Click 'Load this plugin' when the error dialog appears" -ForegroundColor White
Write-Host "7. Copy ALL the [GameRedguard] output from DebugView" -ForegroundColor White
Write-Host ""

$answer = Read-Host "Do you have DebugView running? (y/n)"

if ($answer -eq 'y') {
    Write-Host ""
    Write-Host "Launching MO2..." -ForegroundColor Green
    Start-Process "C:\Modding\MO2\ModOrganizer.exe"
    Write-Host ""
    Write-Host "When MO2 shows the plugin error dialog:" -ForegroundColor Yellow
    Write-Host "  1. Click 'Load this plugin'" -ForegroundColor White
    Write-Host "  2. Immediately check DebugView for the crash output" -ForegroundColor White
    Write-Host "  3. Copy all [GameRedguard] and [GameXngine] lines" -ForegroundColor White
    Write-Host ""
    Write-Host "The output will show which method was called last before the crash!" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "Please download and run DebugView first, then run this script again." -ForegroundColor Red
}
