# Redguard Mod Validation Diagnostic Script (PowerShell)
# Run from your MO2 profile directory to validate all mods

param(
    [string]$ModsPath = ".\Mods"
)

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Redguard Mod Validation Diagnostic" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Check if Mods directory exists
if (-not (Test-Path $ModsPath -PathType Container)) {
    Write-Host "ERROR: Mods directory not found at $(Resolve-Path $ModsPath)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please run this script from your MO2 profile directory" -ForegroundColor Yellow
    Write-Host "Example: F:\Modding\MO2\profiles\Redguard" -ForegroundColor Yellow
    exit 1
}

Write-Host "Found Mods directory at: $(Resolve-Path $ModsPath)" -ForegroundColor Green
Write-Host ""

$modCount = 0
$validCount = 0
$invalidMods = @()

# Get all mod folders
$modFolders = Get-ChildItem -Path $ModsPath -Directory -ErrorAction SilentlyContinue

if ($null -eq $modFolders) {
    Write-Host "No mod folders found in $ModsPath" -ForegroundColor Yellow
    exit 0
}

# Iterate through each mod
foreach ($modFolder in $modFolders) {
    $modName = $modFolder.Name
    $aboutPath = Join-Path $modFolder.FullName "About.txt"
    $modCount++
    
    Write-Host "[MOD #$modCount] $modName" -ForegroundColor Cyan
    
    if (-not (Test-Path $aboutPath)) {
        Write-Host "  Status: " -NoNewline
        Write-Host "INVALID" -ForegroundColor Red
        Write-Host "  Reason: Missing About.txt"
        $invalidMods += @{name = $modName; reason = "Missing About.txt"}
        Write-Host ""
        continue
    }
    
    # Read About.txt
    try {
        $content = Get-Content -Path $aboutPath -Encoding UTF8 -ErrorAction Stop
        
        if ($null -eq $content -or $content.Count -eq 0) {
            Write-Host "  Status: " -NoNewline
            Write-Host "INVALID" -ForegroundColor Red
            Write-Host "  Reason: About.txt is empty"
            $invalidMods += @{name = $modName; reason = "Empty About.txt"}
            Write-Host ""
            continue
        }
        
        # Convert to array if single line
        if ($content -is [string]) {
            $lines = @($content)
        } else {
            $lines = @($content)
        }
        
        Write-Host "  About.txt found: $($lines.Count) lines"
        
        # Check required lines
        $version = ""
        $author = ""
        $description = ""
        
        if ($lines.Count -gt 0) {
            $version = $lines[0].Trim()
            if ($lines.Count -gt 0) { Write-Host "  Line 1 (Version): $version" }
        }
        
        if ($lines.Count -gt 1) {
            $author = $lines[1].Trim()
            if ($lines.Count -gt 1) { Write-Host "  Line 2 (Author): $author" }
        }
        
        if ($lines.Count -gt 2) {
            $description = $lines[2..($lines.Count-1)] -join "`n"
            Write-Host "  Line 3 (Desc start): $($description.Substring(0, [Math]::Min(40, $description.Length)))..."
        }
        
        # Validation
        if ([string]::IsNullOrWhiteSpace($version)) {
            Write-Host "  Status: " -NoNewline
            Write-Host "INVALID" -ForegroundColor Red
            Write-Host "  Reason: Empty version line"
            $invalidMods += @{name = $modName; reason = "Empty version"}
        } elseif ([string]::IsNullOrWhiteSpace($author)) {
            Write-Host "  Status: " -NoNewline
            Write-Host "INVALID" -ForegroundColor Red
            Write-Host "  Reason: Empty author line"
            $invalidMods += @{name = $modName; reason = "Empty author"}
        } elseif ($lines.Count -lt 3) {
            Write-Host "  Status: " -NoNewline
            Write-Host "INVALID" -ForegroundColor Red
            Write-Host "  Reason: Less than 3 lines (need version, author, description)"
            $invalidMods += @{name = $modName; reason = "Less than 3 lines"}
        } else {
            Write-Host "  Status: " -NoNewline
            Write-Host "VALID" -ForegroundColor Green
            $validCount++
        }
        
    } catch {
        Write-Host "  Status: " -NoNewline
        Write-Host "INVALID" -ForegroundColor Red
        Write-Host "  Reason: Could not read About.txt - $($_.Exception.Message)"
        $invalidMods += @{name = $modName; reason = "Read error: $($_.Exception.Message)"}
    }
    
    Write-Host ""
}

# Summary
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Summary" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Total mods found: $modCount"
Write-Host "Valid mods: " -NoNewline
if ($validCount -gt 0) {
    Write-Host "$validCount" -ForegroundColor Green
} else {
    Write-Host "$validCount" -ForegroundColor Red
}
Write-Host "Invalid mods: " -NoNewline
if ($invalidMods.Count -gt 0) {
    Write-Host "$($invalidMods.Count)" -ForegroundColor Red
} else {
    Write-Host "$($invalidMods.Count)" -ForegroundColor Green
}
Write-Host ""

# Details about invalid mods
if ($invalidMods.Count -gt 0) {
    Write-Host "Invalid Mods:" -ForegroundColor Red
    foreach ($mod in $invalidMods) {
        Write-Host "  - $($mod.name)" -ForegroundColor Red
        Write-Host "    Reason: $($mod.reason)" -ForegroundColor Yellow
    }
    Write-Host ""
    Write-Host "For solutions, see MOD_VALIDATION_TROUBLESHOOTING.md" -ForegroundColor Yellow
}

if ($validCount -eq 0 -and $modCount -gt 0) {
    Write-Host "WARNING: No valid mods found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please check:" -ForegroundColor Yellow
    Write-Host "1. Each mod has an About.txt file" -ForegroundColor Yellow
    Write-Host "2. About.txt has at least 3 lines" -ForegroundColor Yellow
    Write-Host "3. Line 1 is not empty (version)" -ForegroundColor Yellow
    Write-Host "4. Line 2 is not empty (author)" -ForegroundColor Yellow
    Write-Host ""
}

Write-Host "Done!" -ForegroundColor Green
