# Automated dependency setup for Student Expense & Task Tracker
$ErrorActionPreference = "Stop"

$thirdPartyDir = Join-Path $PSScriptRoot "..\backend\third_party"
if (!(Test-Path $thirdPartyDir)) {
    New-Item -ItemType Directory -Path $thirdPartyDir -Force | Out-Null
}

$nlohmannDir = Join-Path $thirdPartyDir "nlohmann"
if (!(Test-Path $nlohmannDir)) {
    New-Item -ItemType Directory -Path $nlohmannDir -Force | Out-Null
}

Write-Host "==> Downloading cpp-httplib (httplib.h)..."
$httplibUrl = "https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.18.7/httplib.h"
$httplibDest = Join-Path $thirdPartyDir "httplib.h"
Invoke-WebRequest -Uri $httplibUrl -OutFile $httplibDest
Write-Host "    Saved to: $httplibDest"

Write-Host "==> Downloading nlohmann/json (json.hpp)..."
$jsonUrl = "https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp"
$jsonDest = Join-Path $nlohmannDir "json.hpp"
Invoke-WebRequest -Uri $jsonUrl -OutFile $jsonDest
Write-Host "    Saved to: $jsonDest"

Write-Host "==> Downloading SQLite3 amalgamation..."
$sqliteZipUrl = "https://www.sqlite.org/2024/sqlite-amalgamation-3450300.zip"
$tempZip = Join-Path $env:TEMP "sqlite.zip"
$tempExtract = Join-Path $env:TEMP "sqlite_extract"

Invoke-WebRequest -Uri $sqliteZipUrl -OutFile $tempZip
Expand-Archive -Path $tempZip -DestinationPath $tempExtract -Force

$extractedSqlite = Get-ChildItem -Path $tempExtract -Filter "sqlite3.c" -Recurse | Select-Object -First 1
if ($extractedSqlite) {
    Copy-Item -Path (Join-Path $extractedSqlite.DirectoryName "sqlite3.c") -Destination $thirdPartyDir -Force
    Copy-Item -Path (Join-Path $extractedSqlite.DirectoryName "sqlite3.h") -Destination $thirdPartyDir -Force
    Write-Host "    SQLite3 amalgamation extracted to: $thirdPartyDir"
}

Remove-Item -Path $tempZip -Force -ErrorAction SilentlyContinue
Remove-Item -Path $tempExtract -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "==> All third-party dependencies are successfully configured!" -ForegroundColor Green
