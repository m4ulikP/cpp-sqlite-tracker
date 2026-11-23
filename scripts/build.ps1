# Direct C++20 build script for Windows
param (
    [string]$CompilerPath = ""
)

$ErrorActionPreference = "Stop"

# Detect C++ compiler
if (-not $CompilerPath) {
    if (Test-Path "C:\msys64\ucrt64\bin\g++.exe") {
        $CompilerPath = "C:\msys64\ucrt64\bin\g++.exe"
    } elseif (Get-Command g++ -ErrorAction SilentlyContinue) {
        $CompilerPath = (Get-Command g++).Source
    } else {
        Write-Error "No suitable C++ compiler found. Please install MSYS2 UCRT64 or MinGW-w64."
    }
}

Write-Host "==> Using C++ Compiler: $CompilerPath" -ForegroundColor Cyan

$buildBinDir = "build/bin"
if (!(Test-Path $buildBinDir)) {
    New-Item -ItemType Directory -Path $buildBinDir -Force | Out-Null
}

$includeFlags = @(
    "-Ibackend/include",
    "-Ibackend/third_party"
)

$linkFlags = @(
    "-lws2_32",
    "-lcrypt32",
    "-pthread"
)

$sources = @(
    "backend/src/main.cpp",
    "backend/src/db/DatabaseManager.cpp",
    "backend/src/repository/ItemRepository.cpp",
    "backend/src/services/ItemService.cpp",
    "backend/src/controllers/ItemController.cpp"
)

if (Test-Path "backend/third_party/sqlite3.c") {
    $sources += "backend/third_party/sqlite3.c"
} else {
    $linkFlags += "-lsqlite3"
}

$outputExe = "$buildBinDir/tracker_server.exe"

Write-Host "==> Compiling tracker_server.exe with C++20..." -ForegroundColor Cyan
& $CompilerPath -std=c++20 -O2 $includeFlags $sources $linkFlags -o $outputExe

if ($LASTEXITCODE -eq 0) {
    Write-Host "==> Build successful: $outputExe" -ForegroundColor Green
} else {
    Write-Error "Build failed with exit code $LASTEXITCODE"
}
