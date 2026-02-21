param(
  [Parameter(Mandatory = $true)]
  [string]$SaveFolder,

  [Parameter(Mandatory = $true)]
  [int]$LevelId,

  [switch]$NoBackup
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($LevelId -lt 0 -or $LevelId -gt 255) {
  throw "LevelId must be between 0 and 255."
}

$saveVarsPath = Join-Path $SaveFolder "SAVEVARS.DAT"
if (-not (Test-Path $saveVarsPath -PathType Leaf)) {
  throw "SAVEVARS.DAT not found: $saveVarsPath"
}

[byte[]]$bytes = [System.IO.File]::ReadAllBytes($saveVarsPath)
if ($bytes.Length -lt 1056) {
  throw "SAVEVARS.DAT too small ($($bytes.Length) bytes)."
}

if (-not $NoBackup) {
  $backupPath = "$saveVarsPath.bak"
  [System.IO.File]::WriteAllBytes($backupPath, $bytes)
  Write-Host "Backup written:" $backupPath
}

# CurrentLevel appears at start of the Misc block.
# Documentation offsets may be 1-based/0-based depending on source, so write both
# positions used in parser for robust testing.
foreach ($offset in @(1051, 1052)) {
  if ($offset + 3 -lt $bytes.Length) {
    $bytes[$offset + 0] = [byte]($LevelId -band 0xFF)
    $bytes[$offset + 1] = 0
    $bytes[$offset + 2] = 0
    $bytes[$offset + 3] = 0
  }
}

[System.IO.File]::WriteAllBytes($saveVarsPath, $bytes)
Write-Host "Patched CurrentLevel to" $LevelId "in" $saveVarsPath
