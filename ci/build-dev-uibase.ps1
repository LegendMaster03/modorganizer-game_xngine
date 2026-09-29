[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SourcePath,
    [Parameter(Mandatory = $true)][string]$InstallPath,
    [Parameter(Mandatory = $true)][string]$QtRoot,
    [Parameter(Mandatory = $true)][string]$CmakeCommonPath
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

foreach ($requiredPath in @($SourcePath, $QtRoot, $CmakeCommonPath)) {
    if (-not (Test-Path $requiredPath)) {
        throw "Required uibase build path does not exist: $requiredPath"
    }
}

if (-not $env:VCPKG_ROOT) {
    throw "VCPKG_ROOT is not set."
}

$SourcePath = (Resolve-Path $SourcePath).Path
$QtRoot = (Resolve-Path $QtRoot).Path
$CmakeCommonPath = (Resolve-Path $CmakeCommonPath).Path
$InstallPath = [System.IO.Path]::GetFullPath($InstallPath)

# install-qt-action has used both the architecture directory itself and its
# parent as QT_ROOT_DIR over time. Normalize either layout before configuring.
$qtPrefix = $QtRoot
if (-not (Test-Path (Join-Path $qtPrefix "lib/cmake/Qt6"))) {
    foreach ($architecture in @("msvc2022_64", "msvc2019_64")) {
        $candidate = Join-Path $QtRoot $architecture
        if (Test-Path (Join-Path $candidate "lib/cmake/Qt6")) {
            $qtPrefix = $candidate
            break
        }
    }
}

if (-not (Test-Path (Join-Path $qtPrefix "lib/cmake/Qt6"))) {
    throw "Could not locate the Qt CMake package beneath '$QtRoot'."
}

if (-not (Test-Path (Join-Path $CmakeCommonPath "mo2-cmake-config.cmake"))) {
    throw "cmake_common does not expose mo2-cmake-config.cmake at '$CmakeCommonPath'."
}

if (Test-Path $InstallPath) {
    Remove-Item -Recurse -Force $InstallPath
}
New-Item -ItemType Directory -Force -Path $InstallPath | Out-Null

$prefixPath = @(
    $qtPrefix,
    $CmakeCommonPath,
    (Join-Path $InstallPath "lib/cmake")
) -join ";"

Write-Host "Qt CMake prefix: $qtPrefix"
Write-Host "MO2 CMake common prefix: $CmakeCommonPath"
Write-Host "uibase install prefix: $InstallPath"

Push-Location $SourcePath
try {
    Invoke-Native -Command "cmake" -Arguments @(
        "--preset", "vs2022-windows",
        "-DCMAKE_PREFIX_PATH=$prefixPath",
        "-DCMAKE_INSTALL_PREFIX=$InstallPath",
        "-DBUILD_TESTING=OFF"
    )

    Invoke-Native -Command "cmake" -Arguments @(
        "--build", "--preset", "vs2022-windows",
        "--config", "RelWithDebInfo",
        "--target", "INSTALL",
        "--parallel", "16"
    )
}
finally {
    Pop-Location
}

$lib = Get-ChildItem $InstallPath -Recurse -Filter uibase.lib | Select-Object -First 1
if (-not $lib) {
    throw "uibase install completed without producing uibase.lib."
}
if (-not (Test-Path (Join-Path $InstallPath "include/uibase/iplugingame.h"))) {
    throw "uibase install completed without public headers."
}

Write-Host "Installed current development uibase to $InstallPath"
