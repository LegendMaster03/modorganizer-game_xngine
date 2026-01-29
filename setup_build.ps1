# PowerShell script to set up build environment and compile

Write-Host "Looking for Visual Studio or Build Tools..." -ForegroundColor Cyan

$vsPath = $null
$vcVarsPath = $null

# Check Visual Studio 2022
$paths2022 = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)

# Check Visual Studio 2019
$paths2019 = @(
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)

$allPaths = $paths2022 + $paths2019

foreach ($path in $allPaths) {
    if (Test-Path $path) {
        $vcVarsPath = $path
        Write-Host "Found: $path" -ForegroundColor Green
        break
    }
}

if (-not $vcVarsPath) {
    Write-Host "ERROR: Could not find Visual Studio or Build Tools!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please download and install one of the following:" -ForegroundColor Yellow
    Write-Host "  - Visual Studio Community (free)"
    Write-Host "  - Visual Studio Build Tools (free)"
    Write-Host "  - Download from: https://visualstudio.microsoft.com/downloads/"
    Write-Host ""
    Write-Host "Make sure to select 'Desktop development with C++'" -ForegroundColor Yellow
    exit 1
}

Write-Host "Setting up build environment..." -ForegroundColor Cyan

# Import the Visual Studio environment
cmd /c """$vcVarsPath"" && powershell -NoExit -Command {
    Write-Host 'Visual Studio environment loaded!' -ForegroundColor Green
    cd 'F:\Modding\MO2\plugins\modorganizer-game_redguard'
    
    if (Test-Path 'build') {
        Remove-Item 'build' -Recurse -Force
    }
    
    New-Item 'build' -ItemType Directory -Force | Out-Null
    cd 'build'
    
    Write-Host 'Configuring CMake...' -ForegroundColor Cyan
    cmake .. -G 'Visual Studio 17 2022' -A x64
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host 'CMake configuration successful!' -ForegroundColor Green
        Write-Host 'Now building the project...' -ForegroundColor Cyan
        cmake --build . --config Release
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host 'Build successful!' -ForegroundColor Green
            Write-Host 'DLL location: F:\Modding\MO2\plugins\modorganizer-game_redguard\build\Release\' -ForegroundColor Green
        } else {
            Write-Host 'Build failed!' -ForegroundColor Red
        }
    } else {
        Write-Host 'CMake configuration failed!' -ForegroundColor Red
    }
}"

