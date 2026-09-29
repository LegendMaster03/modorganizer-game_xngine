[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SourcePath,
    [Parameter(Mandatory = $true)][string]$InstallPath,
    [Parameter(Mandatory = $true)][string]$QtRoot
)

$ErrorActionPreference = "Stop"

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

foreach ($requiredPath in @($SourcePath, $QtRoot)) {
    if (-not (Test-Path $requiredPath)) {
        throw "Required uibase build path does not exist: $requiredPath"
    }
}

if (-not $env:VCPKG_ROOT) {
    throw "VCPKG_ROOT is not set."
}

$vcpkgToolchain = Join-Path $env:VCPKG_ROOT "scripts/buildsystems/vcpkg.cmake"
if (-not (Test-Path $vcpkgToolchain)) {
    throw "vcpkg CMake toolchain was not found at $vcpkgToolchain"
}

$buildPath = Join-Path (Split-Path $InstallPath -Parent) "build-uibase"
if (Test-Path $buildPath) {
    Remove-Item -Recurse -Force $buildPath
}
if (Test-Path $InstallPath) {
    Remove-Item -Recurse -Force $InstallPath
}

$configureArgs = @(
    "-S", $SourcePath,
    "-B", $buildPath,
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-T", "v143",
    "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain",
    "-DVCPKG_TARGET_TRIPLET=x64-windows-static-md",
    "-DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON",
    "-DVCPKG_MANIFEST_FEATURES=standalone",
    "-DBUILD_TESTING=OFF",
    "-DCMAKE_PREFIX_PATH=$QtRoot",
    "-DCMAKE_INSTALL_PREFIX=$InstallPath"
)

Invoke-Native -Command "cmake" -Arguments $configureArgs
Invoke-Native -Command "cmake" -Arguments @("--build", $buildPath, "--config", "RelWithDebInfo", "--parallel")
Invoke-Native -Command "cmake" -Arguments @("--install", $buildPath, "--config", "RelWithDebInfo")

$lib = Get-ChildItem $InstallPath -Recurse -Filter uibase.lib | Select-Object -First 1
if (-not $lib) {
    throw "uibase install completed without producing uibase.lib."
}
if (-not (Test-Path (Join-Path $InstallPath "include/uibase/iplugingame.h"))) {
    throw "uibase install completed without public headers."
}

Write-Host "Installed current development uibase to $InstallPath"
