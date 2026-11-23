# End-to-End Verification Test Script for Phase 4
$ErrorActionPreference = "Continue"

Write-Host "===================================================================" -ForegroundColor Cyan
Write-Host "  Starting End-to-End Verification (Frontend + C++ REST Backend)  " -ForegroundColor Cyan
Write-Host "===================================================================" -ForegroundColor Cyan

$serverExe = "build/bin/tracker_server.exe"
if (!(Test-Path $serverExe)) {
    Write-Error "Server binary not found at $serverExe"
    exit 1
}

# 1. Start C++ Server
Write-Host "==> Starting tracker_server.exe on port 8080..." -ForegroundColor Cyan
$cppProc = Start-Process -FilePath $serverExe -PassThru -NoNewWindow
Start-Sleep -Seconds 2

# 2. Start Python Static Server on port 3000 for frontend CORS check
Write-Host "==> Starting Python http.server on port 3000 for frontend..." -ForegroundColor Cyan
$pythonProc = Start-Process -FilePath "C:\Python314\python.exe" -ArgumentList "-m http.server 3000 --directory frontend" -PassThru -NoNewWindow
Start-Sleep -Seconds 2

$totalTests = 0
$passedTests = 0

function Assert-E2E {
    param (
        [string]$Name,
        [bool]$Condition,
        [string]$Details = ""
    )
    $script:totalTests++
    if ($Condition) {
        $script:passedTests++
        Write-Host "  [PASS] $Name" -ForegroundColor Green -NoNewline
    } else {
        Write-Host "  [FAIL] $Name" -ForegroundColor Red -NoNewline
    }
    if ($Details) {
        Write-Host " - $Details" -ForegroundColor Gray
    } else {
        Write-Host ""
    }
}

try {
    # Test A: Direct Frontend Serving from C++ Server (Port 8080)
    $html8080 = Invoke-WebRequest -Uri "http://localhost:8080/" -Method Get -UseBasicParsing
    Assert-E2E -Name "C++ Server Serves index.html" `
               -Condition ($html8080.StatusCode -eq 200 -and $html8080.Content.Contains("Student Tracker")) `
               -Details "Status: $($html8080.StatusCode), Size: $($html8080.Content.Length) bytes"

    $css8080 = Invoke-WebRequest -Uri "http://localhost:8080/css/style.css" -Method Get -UseBasicParsing
    Assert-E2E -Name "C++ Server Serves style.css" `
               -Condition ($css8080.StatusCode -eq 200 -and $css8080.Content.Contains("--font-sans")) `
               -Details "CSS size: $($css8080.Content.Length) bytes"

    $js8080 = Invoke-WebRequest -Uri "http://localhost:8080/js/app.js" -Method Get -UseBasicParsing
    Assert-E2E -Name "C++ Server Serves app.js" `
               -Condition ($js8080.StatusCode -eq 200 -and $js8080.Content.Contains("apiClient")) `
               -Details "JS size: $($js8080.Content.Length) bytes"

    # Test B: Python Static Server Serving (Port 3000)
    $html3000 = Invoke-WebRequest -Uri "http://localhost:3000/" -Method Get -UseBasicParsing
    Assert-E2E -Name "Python Server Serves Frontend (Port 3000)" `
               -Condition ($html3000.StatusCode -eq 200) `
               -Details "Status: $($html3000.StatusCode)"

    # Test C: End-to-End Item Creation & SQLite Persistence
    $newItemPayload = @{
        title = "E2E Automated Verification Test Record"
        type = "task"
        category = "Project"
        amount = 0.0
        status = "pending"
        priority = "high"
        due_date = "2026-12-01"
        description = "Testing persistence across server restart"
    } | ConvertTo-Json

    $createResp = Invoke-RestMethod -Uri "http://localhost:8080/api/items" -Method Post -Body $newItemPayload -ContentType "application/json"
    $testItemId = $createResp.data.id
    Assert-E2E -Name "Create Item via REST API" `
               -Condition ($createResp.success -eq $true -and $testItemId -gt 0) `
               -Details "Created item ID: $testItemId"

    # Test D: Simulate Server Restart to verify SQLite database file persistence
    Write-Host "==> Simulating server restart to verify persistent SQLite storage..." -ForegroundColor Yellow
    Invoke-RestMethod -Uri "http://localhost:8080/api/admin/shutdown" -Method Post -TimeoutSec 2 | Out-Null
    Start-Sleep -Seconds 1
    if (!$cppProc.HasExited) {
        Stop-Process -Id $cppProc.Id -Force -ErrorAction SilentlyContinue
    }

    # Restart Server
    $cppProc = Start-Process -FilePath $serverExe -PassThru -NoNewWindow
    Start-Sleep -Seconds 2

    # Verify item persists across restart
    $persistedResp = Invoke-RestMethod -Uri "http://localhost:8080/api/items/$testItemId" -Method Get
    Assert-E2E -Name "Persistence Across Server Restart" `
               -Condition ($persistedResp.success -eq $true -and $persistedResp.data.title -eq "E2E Automated Verification Test Record") `
               -Details "Fetched item ID $testItemId after fresh server launch"

    # Test E: Update item
    $updatePayload = @{
        title = "E2E Automated Verification Test Record - Completed"
        type = "task"
        category = "Project"
        amount = 10.0
        status = "completed"
        priority = "medium"
        due_date = "2026-12-01"
        description = "Updated after restart"
    } | ConvertTo-Json

    $updateResp = Invoke-RestMethod -Uri "http://localhost:8080/api/items/$testItemId" -Method Put -Body $updatePayload -ContentType "application/json"
    Assert-E2E -Name "Update Item in SQLite" `
               -Condition ($updateResp.success -eq $true -and $updateResp.data.status -eq "completed") `
               -Details "Status is now: $($updateResp.data.status)"

    # Test F: Verify Summary Calculation Reflects Changes
    $summaryResp = Invoke-RestMethod -Uri "http://localhost:8080/api/items/summary" -Method Get
    Assert-E2E -Name "Summary Analytics Reflected" `
               -Condition ($summaryResp.success -eq $true -and $summaryResp.data.tasks.completed_count -gt 0) `
               -Details "Completed tasks count: $($summaryResp.data.tasks.completed_count)"

    # Test G: Delete Item to restore database state
    $deleteResp = Invoke-RestMethod -Uri "http://localhost:8080/api/items/$testItemId" -Method Delete
    Assert-E2E -Name "Delete Item from SQLite" `
               -Condition ($deleteResp.success -eq $true -and $deleteResp.data.deleted -eq $true) `
               -Details "Deleted ID $testItemId"

    # Test H: Confirm item is 404
    $verify404 = 0
    try {
        Invoke-RestMethod -Uri "http://localhost:8080/api/items/$testItemId" -Method Get
    } catch {
        $verify404 = $_.Exception.Response.StatusCode.value__
    }
    Assert-E2E -Name "Confirm Item Deleted in Database" `
               -Condition ($verify404 -eq 404) `
               -Details "HTTP Status: $verify404"

} finally {
    Write-Host "==> Cleaning up test server processes..." -ForegroundColor Cyan
    try {
        Invoke-RestMethod -Uri "http://localhost:8080/api/admin/shutdown" -Method Post -TimeoutSec 2 | Out-Null
    } catch {}
    Start-Sleep -Seconds 1

    if ($cppProc -and !$cppProc.HasExited) {
        Stop-Process -Id $cppProc.Id -Force -ErrorAction SilentlyContinue
    }
    if ($pythonProc -and !$pythonProc.HasExited) {
        Stop-Process -Id $pythonProc.Id -Force -ErrorAction SilentlyContinue
    }
    Write-Host "==> All test processes stopped cleanly." -ForegroundColor Green
}

Write-Host "===================================================================" -ForegroundColor Cyan
Write-Host "  End-to-End Verification Report: $passedTests / $totalTests Passed" -ForegroundColor Cyan
Write-Host "===================================================================" -ForegroundColor Cyan

if ($passedTests -eq $totalTests) {
    exit 0
} else {
    exit 1
}
