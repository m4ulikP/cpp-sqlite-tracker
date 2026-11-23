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
    "backend/src/repository/ItemRepository.cpp"
)

if (Test-Path "backend/src/services/ItemService.cpp") {
    $sources += "backend/src/services/ItemService.cpp"
}
if (Test-Path "backend/src/controllers/ItemController.cpp") {
    $sources += "backend/src/controllers/ItemController.cpp"
}

$sqliteObj = "build/sqlite3.o"
if (Test-Path "backend/third_party/sqlite3.c") {
    $sqliteSrc = "backend/third_party/sqlite3.c"
    if (!(Test-Path $sqliteObj) -or ((Get-Item $sqliteSrc).LastWriteTime -gt (Get-Item $sqliteObj).LastWriteTime)) {
        Write-Host "==> Compiling SQLite3 C amalgamation to $sqliteObj..." -ForegroundColor Cyan
        $cCompiler = if (Test-Path "C:\msys64\ucrt64\bin\gcc.exe") { "C:\msys64\ucrt64\bin\gcc.exe" } else { $CompilerPath }
        & $cCompiler -O2 -c $sqliteSrc -o $sqliteObj
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Failed to compile sqlite3.c"
        }
    }
    $sources += $sqliteObj
} else {
    $linkFlags += "-lsqlite3"
}

$outputExe = "$buildBinDir/tracker_server.exe"

Write-Host "==> Compiling tracker_server.exe with C++20 (Warnings enabled)..." -ForegroundColor Cyan
& $CompilerPath -std=c++20 -O2 -Wall -Wextra -Wno-unused-parameter $includeFlags $sources $linkFlags -o $outputExe

if ($LASTEXITCODE -eq 0) {
    Write-Host "==> Build successful: $outputExe" -ForegroundColor Green
} else {
    Write-Error "Build failed with exit code $LASTEXITCODE"
}
