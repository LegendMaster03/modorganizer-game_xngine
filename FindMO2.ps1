# Quick MO2 Path Finder
# Run this to find where MO2 is installed

$mo2Paths = @()

Write-Host "`nSearching for MO2 installations...`n"

# Common installation paths
$searchPaths = @(
    "C:\Program Files\Mod Organizer 2",
    "C:\Program Files (x86)\Mod Organizer 2",
    "$env:LOCALAPPDATA\Mod Organizer 2",
    "C:\Games\Mod Organizer 2",
    "D:\Games\Mod Organizer 2",
    "E:\Games\Mod Organizer 2"
)

foreach ($path in $searchPaths) {
    if (Test-Path "$path\ModOrganizer.exe") {
        $mo2Paths += $path
        Write-Host "✓ Found: $path"
    }
}

if ($mo2Paths.Count -eq 0) {
    Write-Host "✗ No MO2 installations found in common locations"
    Write-Host "`nManual search:"
    Write-Host "1. Open File Explorer"
    Write-Host "2. Search for ModOrganizer.exe"
    Write-Host "3. Note the folder path"
    Write-Host "4. Run: .\Deploy.ps1 -MO2Path 'C:\Your\MO2\Path'"
} else {
    Write-Host "`n✓ Use with Deploy.ps1:"
    foreach ($path in $mo2Paths) {
        Write-Host "  .\Deploy.ps1 -MO2Path '$path'"
    }
}

Write-Host "`n"
