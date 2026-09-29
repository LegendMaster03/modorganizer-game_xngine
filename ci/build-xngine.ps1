[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][ValidateSet("retail", "dev")][string]$Target,
    [Parameter(Mandatory = $true)][string]$UibasePath,
    [Parameter(Mandatory = $true)][string]$UibaseLib,
    [Parameter(Mandatory = $true)][string]$Mo2SourcePath,
    [Parameter(Mandatory = $true)][string]$QtRoot,
    [string]$BuildDir = "build-ci",
    [string]$ArtifactDir = "artifacts/build"
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Invoke-Native {
    param(
        [Parameter(Mandatory = $true)][string]$Command,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    Write-Host "> $Command $($Arguments -join ' ')"
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Command exited with code $LASTEXITCODE."
    }
}

foreach ($requiredPath in @($UibasePath, $UibaseLib, $Mo2SourcePath, $QtRoot)) {
    if (-not (Test-Path $requiredPath)) {
        throw "Required build path does not exist: $requiredPath"
    }
}

if (-not $env:VCPKG_ROOT) {
    throw "VCPKG_ROOT is not set."
}
$vcpkgToolchain = Join-Path $env:VCPKG_ROOT "scripts/buildsystems/vcpkg.cmake"
if (-not (Test-Path $vcpkgToolchain)) {
    throw "vcpkg CMake toolchain was not found at $vcpkgToolchain"
}

$publicHeader = Get-ChildItem $UibasePath -Recurse -Filter iplugingame.h | Select-Object -First 1
if (-not $publicHeader) {
    throw "Could not locate iplugingame.h beneath uibase path '$UibasePath'."
}

$uibaseInclude = $publicHeader.Directory.FullName
if ((Split-Path $uibaseInclude -Leaf) -eq "uibase") {
    $uibaseRootInclude = Split-Path $uibaseInclude -Parent
}
else {
    $uibaseRootInclude = $uibaseInclude
}

$gameFeaturesInclude = Join-Path $uibaseInclude "game_features"
if (-not (Test-Path $gameFeaturesInclude)) {
    $gameFeatureHeader = Get-ChildItem $UibasePath -Recurse -Filter dataarchives.h | Where-Object {
        $_.Directory.Name -eq "game_features"
    } | Select-Object -First 1
    if (-not $gameFeatureHeader) {
        throw "Could not locate the uibase game_features headers beneath '$UibasePath'."
    }
    $gameFeaturesInclude = $gameFeatureHeader.Directory.FullName
}

Write-Host "uibase public include: $uibaseInclude"
Write-Host "uibase root include: $uibaseRootInclude"
Write-Host "uibase game_features include: $gameFeaturesInclude"

$buildPath = Join-Path $repoRoot $BuildDir
$artifactPath = Join-Path $repoRoot $ArtifactDir
$vcpkgInstalledPath = Join-Path $repoRoot "vcpkg_installed"

if (Test-Path $buildPath) {
    Remove-Item -Recurse -Force $buildPath
}
if (Test-Path $artifactPath) {
    Remove-Item -Recurse -Force $artifactPath
}
New-Item -ItemType Directory -Force -Path $artifactPath | Out-Null

$configureArgs = @(
    "-S", $repoRoot,
    "-B", $buildPath,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain",
    "-DVCPKG_INSTALLED_DIR=$vcpkgInstalledPath",
    "-DMO2_UIBASE_PATH=$UibasePath",
    "-DMO2_UIBASE_INCLUDE=$uibaseInclude",
    "-DMO2_UIBASE_ROOT_INCLUDE=$uibaseRootInclude",
    "-DMO2_UIBASE_GAME_FEATURES_INCLUDE=$gameFeaturesInclude",
    "-DMO2_UIBASE_LIB=$UibaseLib",
    "-DMO2_SRC_PATH=$Mo2SourcePath",
    "-DQT_ROOT=$QtRoot",
    "-DCMAKE_PREFIX_PATH=$QtRoot"
)

Invoke-Native -Command "cmake" -Arguments $configureArgs

$cachePath = Join-Path $buildPath "CMakeCache.txt"
$archiveHandlerEnabled = $false
if (Test-Path $cachePath) {
    $capabilityLine = Select-String -Path $cachePath -Pattern '^MO2_UIBASE_HAS_GAMEARCHIVEHANDLER:BOOL=(ON|OFF)$' | Select-Object -First 1
    if ($capabilityLine) {
        $archiveHandlerEnabled = $capabilityLine.Matches[0].Groups[1].Value -eq "ON"
    }
}

@(
    "MO2 GameArchiveHandler: $archiveHandlerEnabled",
    "Archive extractor integration: $archiveHandlerEnabled"
) | Set-Content -Path (Join-Path $artifactPath "capabilities.txt")

Write-Host "MO2 GameArchiveHandler capability detected: $archiveHandlerEnabled"
Invoke-Native -Command "cmake" -Arguments @("--build", $buildPath, "--config", "Release", "--parallel")

$pluginOutput = Join-Path $buildPath "bin/Release/plugins"
$expectedPlugins = @(
    "game_arena.dll",
    "game_battlespire.dll",
    "game_daggerfall.dll",
    "game_redguard.dll"
)

$artifactPlugins = Join-Path $artifactPath "plugins"
New-Item -ItemType Directory -Force -Path $artifactPlugins | Out-Null

foreach ($plugin in $expectedPlugins) {
    $source = Join-Path $pluginOutput $plugin
    if (-not (Test-Path $source)) {
        throw "Expected plugin was not produced for $Target: $source"
    }
    Copy-Item $source -Destination (Join-Path $artifactPlugins $plugin)
}

Write-Host "XnGine $Target build produced all four expected plugins."
