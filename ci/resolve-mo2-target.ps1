[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("retail", "dev")]
    [string]$Target
)

$ErrorActionPreference = "Stop"

$headers = @{
    Accept = "application/vnd.github+json"
    "User-Agent" = "XnGine-CI"
}
if ($env:GITHUB_TOKEN) {
    $headers.Authorization = "Bearer $env:GITHUB_TOKEN"
}

function Get-GitHubJson {
    param([Parameter(Mandatory = $true)][string]$Uri)
    Invoke-RestMethod -Uri $Uri -Headers $headers
}

function Get-GitHubCommitSha {
    param(
        [Parameter(Mandatory = $true)][string]$Repository,
        [Parameter(Mandatory = $true)][string]$Ref
    )

    $encodedRef = [Uri]::EscapeDataString($Ref)
    (Get-GitHubJson "https://api.github.com/repos/$Repository/commits/$encodedRef").sha
}

function Get-MobIni {
    param([Parameter(Mandatory = $true)][string]$Ref)

    $uri = "https://raw.githubusercontent.com/ModOrganizer2/mob/$Ref/mob.ini"
    try {
        (Invoke-WebRequest -Uri $uri -Headers @{ "User-Agent" = "XnGine-CI" }).Content
    }
    catch {
        throw "MO2 target '$Ref' does not have matching mob metadata at $uri. Refusing to guess the toolchain."
    }
}

function Get-MobVersionValue {
    param(
        [Parameter(Mandatory = $true)][string]$Content,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $match = [regex]::Match($Content, "(?m)^\s*" + [regex]::Escape($Name) + "\s*=\s*([^\s#]+)")
    if (-not $match.Success) {
        throw "mob.ini does not define '$Name'."
    }
    $match.Groups[1].Value.Trim()
}

if ($Target -eq "retail") {
    $release = Get-GitHubJson "https://api.github.com/repos/ModOrganizer2/modorganizer/releases/latest"
    $mo2Ref = [string]$release.tag_name
    if (-not $mo2Ref) {
        throw "The latest stable MO2 release did not expose a tag."
    }

    $uibaseAsset = $release.assets | Where-Object { $_.name -match "-uibase\.7z$" } | Select-Object -First 1
    if (-not $uibaseAsset) {
        throw "MO2 $mo2Ref does not publish the expected uibase SDK asset."
    }

    $uibaseRef = $mo2Ref
    $mobRef = $mo2Ref
    $artifactSuffix = "mo2-$($mo2Ref.TrimStart('v'))-retail"
    $uibaseAssetUrl = [string]$uibaseAsset.browser_download_url
}
else {
    $mo2Ref = "master"
    $uibaseRef = "master"
    $mobRef = "master"
    $artifactSuffix = "mo2-dev"
    $uibaseAssetUrl = ""
}

$mobIni = Get-MobIni $mobRef
$qtVersion = Get-MobVersionValue -Content $mobIni -Name "qt"
$qtVs = Get-MobVersionValue -Content $mobIni -Name "qt_vs"

switch ($qtVs) {
    "2019" {
        $qtArch = "win64_msvc2019_64"
        $qtFolder = "msvc2019_64"
    }
    "2022" {
        $qtArch = "win64_msvc2022_64"
        $qtFolder = "msvc2022_64"
    }
    default {
        throw "Unsupported Qt MSVC toolchain '$qtVs' in mob.ini."
    }
}

$mo2Sha = Get-GitHubCommitSha -Repository "ModOrganizer2/modorganizer" -Ref $mo2Ref
$uibaseSha = Get-GitHubCommitSha -Repository "ModOrganizer2/modorganizer-uibase" -Ref $uibaseRef
$mobSha = Get-GitHubCommitSha -Repository "ModOrganizer2/mob" -Ref $mobRef
$cmakeCommonSha = Get-GitHubCommitSha -Repository "ModOrganizer2/cmake_common" -Ref "master"

$outputs = [ordered]@{
    target = $Target
    mo2_ref = $mo2Ref
    mo2_sha = $mo2Sha
    uibase_ref = $uibaseRef
    uibase_sha = $uibaseSha
    uibase_asset_url = $uibaseAssetUrl
    mob_ref = $mobRef
    mob_sha = $mobSha
    cmake_common_sha = $cmakeCommonSha
    qt_version = $qtVersion
    qt_arch = $qtArch
    qt_folder = $qtFolder
    artifact_suffix = $artifactSuffix
}

Write-Host "Resolved $Target target:"
$outputs.GetEnumerator() | ForEach-Object { Write-Host "  $($_.Key)=$($_.Value)" }

if ($env:GITHUB_OUTPUT) {
    $outputs.GetEnumerator() | ForEach-Object {
        "$($_.Key)=$($_.Value)" | Out-File -FilePath $env:GITHUB_OUTPUT -Encoding utf8 -Append
    }
}
