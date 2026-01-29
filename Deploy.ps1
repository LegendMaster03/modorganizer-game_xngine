# MO2 Plugin Deployment Script
# Automatically finds MO2 and deploys game_redguard.dll

param(
    [string]$MO2Path = $null,
    [switch]$Verbose = $false,
    [switch]$Test = $false
)

function Write-Status {
    param([string]$Message, [string]$Type = "Info")
    $colors = @{
        "Success" = "Green"
        "Error"   = "Red"
        "Warning" = "Yellow"
        "Info"    = "Cyan"
    }
    Write-Host $Message -ForegroundColor $colors[$Type]
}

Write-Host "`n╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║     MO2 Redguard Plugin Deployment Script                      ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════════════════════╝`n" -ForegroundColor Green

# Step 1: Find the DLL
Write-Status "[1] Locating game_redguard.dll..." "Info"
$dllPath = "bin\game_redguard.dll"

if (-not (Test-Path $dllPath)) {
    Write-Status "✗ ERROR: game_redguard.dll not found in bin\ directory" "Error"
    exit 1
}

$dllSize = (Get-Item $dllPath).Length
Write-Status "✓ Found: $dllPath ($dllSize bytes)" "Success"

# Step 2: Find MO2 Installation
Write-Status "`n[2] Finding MO2 installation..." "Info"

if ([string]::IsNullOrEmpty($MO2Path)) {
    $possiblePaths = @(
        "C:\Program Files\Mod Organizer 2",
        "C:\Program Files (x86)\Mod Organizer 2",
        "$env:LOCALAPPDATA\Mod Organizer 2"
    )
    
    foreach ($path in $possiblePaths) {
        if (Test-Path "$path\ModOrganizer.exe") {
            $MO2Path = $path
            Write-Status "✓ Found MO2 at: $path" "Success"
            break
        }
    }
}

if ([string]::IsNullOrEmpty($MO2Path)) {
    Write-Status "✗ ERROR: MO2 not found in standard locations" "Error"
    Write-Host "`nPlease provide MO2 path manually:"
    Write-Host "  .\Deploy.ps1 -MO2Path 'C:\path\to\MO2'"
    exit 1
}

# Step 3: Verify plugins directory
Write-Status "`n[3] Checking plugins directory..." "Info"
$pluginsDir = "$MO2Path\plugins"

if (-not (Test-Path $pluginsDir)) {
    Write-Status "Creating plugins directory..." "Warning"
    New-Item -ItemType Directory -Path $pluginsDir | Out-Null
}

Write-Status "✓ Plugins directory: $pluginsDir" "Success"

# Step 4: Deploy DLL
Write-Status "`n[4] Deploying plugin..." "Info"

try {
    $targetPath = "$pluginsDir\game_redguard.dll"
    
    if (Test-Path $targetPath) {
        $backupPath = "$targetPath.backup"
        Write-Host "  Backing up old DLL..."
        Copy-Item $targetPath $backupPath -Force | Out-Null
        Write-Host "  Backup saved to: $backupPath"
    }
    
    Copy-Item $dllPath $targetPath -Force | Out-Null
    Write-Status "✓ Deployed to: $targetPath" "Success"
    
} catch {
    Write-Status "✗ ERROR: Failed to deploy DLL" "Error"
    Write-Status $_.Exception.Message "Error"
    exit 1
}

# Step 5: Verify deployment
Write-Status "`n[5] Verifying deployment..." "Info"

if (Test-Path $targetPath) {
    $targetSize = (Get-Item $targetPath).Length
    Write-Status "✓ DLL verified at target location" "Success"
    Write-Status "  Size: $targetSize bytes" "Info"
} else {
    Write-Status "✗ ERROR: DLL not found after deployment" "Error"
    exit 1
}

# Step 6: Check for Qt6 dependencies
Write-Status "`n[6] Checking Qt6 dependencies..." "Info"

$qt6Dlls = @("Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll")
$missingDlls = @()

foreach ($dll in $qt6Dlls) {
    $dllPaths = @(
        "$MO2Path\bin\$dll",
        "$pluginsDir\$dll"
    )
    
    $found = $false
    foreach ($path in $dllPaths) {
        if (Test-Path $path) {
            Write-Status "  ✓ $dll found" "Success"
            $found = $true
            break
        }
    }
    
    if (-not $found) {
        $missingDlls += $dll
    }
}

if ($missingDlls.Count -gt 0) {
    Write-Status "  ⚠ Missing Qt6 DLLs: $($missingDlls -join ', ')" "Warning"
    Write-Host "  These might be needed if MO2 doesn't load the plugin"
}

# Step 7: Final instructions
Write-Status "`n[7] Deployment Complete!" "Success"

Write-Host @"

╔════════════════════════════════════════════════════════════════╗
║  NEXT STEPS                                                    ║
╚════════════════════════════════════════════════════════════════╝

1. RESTART MO2 (Important!)
   - Close MO2 completely
   - Wait 2-3 seconds
   - Reopen MO2

2. CHECK GAME DROPDOWN
   - Look for "The Elder Scrolls Adventures: Redguard"
   - Should appear in game selection list

3. CREATE INSTANCE
   - Click Redguard in dropdown
   - Set up game and data paths
   - Create new instance

4. IF NOT WORKING
   - Check: PLUGIN_DEBUGGING.md
   - Check MO2 logs: $MO2Path\profiles\[profile]\logs\
   - Search for "game_redguard" in logs

╔════════════════════════════════════════════════════════════════╗
║  DEPLOYMENT SUMMARY                                            ║
╚════════════════════════════════════════════════════════════════╝

Plugin File:     game_redguard.dll
Source:          $dllPath
Destination:     $targetPath
Size:            $dllSize bytes
MO2 Path:        $MO2Path
Deployment:      ✓ SUCCESS

"@

if ($Verbose) {
    Write-Host "`n[Verbose] File details:"
    Get-Item $targetPath | Format-Table Name, Length, LastWriteTime
}

exit 0
