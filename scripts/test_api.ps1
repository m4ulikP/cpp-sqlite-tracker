# Automated Live HTTP API Verification Script for Phase 3
$ErrorActionPreference = "Continue"

$exePath = "build/bin/tracker_server.exe"
if (!(Test-Path $exePath)) {
    Write-Error "Server executable not found at $exePath. Run scripts/build.ps1 first."
    exit 1
}

Write-Host "===================================================================" -ForegroundColor Cyan
Write-Host "  Starting Student Tracker Server for Live API Verification..." -ForegroundColor Cyan
Write-Host "===================================================================" -ForegroundColor Cyan

$process = Start-Process -FilePath $exePath -PassThru -NoNewWindow
Start-Sleep -Seconds 2

$baseUrl = "http://localhost:8080"
$totalTests = 0
$passedTests = 0

function Assert-Test {
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
    # 1. Health Check
    $health = Invoke-RestMethod -Uri "$baseUrl/api/health" -Method Get
    Assert-Test -Name "GET /api/health" `
                -Condition ($health.success -eq $true -and $health.data.status -eq "healthy" -and $health.data.database -eq "connected") `
                -Details "Database status: $($health.data.database), Version: $($health.data.version)"

    # 2. CORS Header Verification
    $rawHealth = Invoke-WebRequest -Uri "$baseUrl/api/health" -Method Get -UseBasicParsing
    $corsHeader = $rawHealth.Headers["Access-Control-Allow-Origin"]
    Assert-Test -Name "CORS Header Check" `
                -Condition ($corsHeader -eq "*") `
                -Details "Access-Control-Allow-Origin: $corsHeader"

    # 3. List Items
    $itemsResp = Invoke-RestMethod -Uri "$baseUrl/api/items" -Method Get
    Assert-Test -Name "GET /api/items" `
                -Condition ($itemsResp.success -eq $true -and $itemsResp.data.Count -ge 6) `
                -Details "Retrieved $($itemsResp.data.Count) items"

    # 4. Filter Items by Type
    $expenseResp = Invoke-RestMethod -Uri "$baseUrl/api/items?type=expense" -Method Get
    $allExpense = ($expenseResp.data | Where-Object { $_.type -ne "expense" }).Count -eq 0
    Assert-Test -Name "GET /api/items?type=expense" `
                -Condition ($expenseResp.success -eq $true -and $allExpense -and $expenseResp.data.Count -gt 0) `
                -Details "Retrieved $($expenseResp.data.Count) expenses"

    # 5. Filter Items by Search
    $searchResp = Invoke-RestMethod -Uri "$baseUrl/api/items?search=Textbook" -Method Get
    Assert-Test -Name "GET /api/items?search=Textbook" `
                -Condition ($searchResp.success -eq $true -and $searchResp.data.Count -ge 1) `
                -Details "Matched: $($searchResp.data[0].title)"

    # 6. Create Item (POST)
    $createPayload = @{
        title = "Live API Integration Test Task"
        type = "task"
        category = "Project"
        amount = 0.0
        status = "pending"
        priority = "high"
        due_date = "2026-11-30"
        description = "Automated test verification payload"
    } | ConvertTo-Json

    $createResp = Invoke-RestMethod -Uri "$baseUrl/api/items" -Method Post -Body $createPayload -ContentType "application/json"
    $newId = $createResp.data.id
    Assert-Test -Name "POST /api/items (Create)" `
                -Condition ($createResp.success -eq $true -and $newId -gt 0) `
                -Details "Created item ID: $newId"

    # 7. Get Item by ID
    $getByIdResp = Invoke-RestMethod -Uri "$baseUrl/api/items/$newId" -Method Get
    Assert-Test -Name "GET /api/items/{id}" `
                -Condition ($getByIdResp.success -eq $true -and $getByIdResp.data.title -eq "Live API Integration Test Task") `
                -Details "Fetched item title: $($getByIdResp.data.title)"

    # 8. Update Item (PUT)
    $updatePayload = @{
        title = "Live API Integration Test Task - Completed"
        type = "task"
        category = "Project"
        amount = 0.0
        status = "completed"
        priority = "medium"
        due_date = "2026-11-30"
        description = "Updated during test run"
    } | ConvertTo-Json

    $updateResp = Invoke-RestMethod -Uri "$baseUrl/api/items/$newId" -Method Put -Body $updatePayload -ContentType "application/json"
    Assert-Test -Name "PUT /api/items/{id} (Update)" `
                -Condition ($updateResp.success -eq $true -and $updateResp.data.status -eq "completed") `
                -Details "Status updated to: $($updateResp.data.status)"

    # 9. Get Summary Metrics
    $summaryResp = Invoke-RestMethod -Uri "$baseUrl/api/items/summary" -Method Get
    $statsValid = ($summaryResp.success -eq $true -and $summaryResp.data.expenses.total_count -gt 0 -and $summaryResp.data.tasks.total_count -gt 0)
    Assert-Test -Name "GET /api/items/summary" `
                -Condition $statsValid `
                -Details "Expenses Total: $($summaryResp.data.expenses.total_amount), Task Completion: $($summaryResp.data.tasks.completion_percentage)%"

    # 10. Delete Item (DELETE)
    $delResp = Invoke-RestMethod -Uri "$baseUrl/api/items/$newId" -Method Delete
    Assert-Test -Name "DELETE /api/items/{id}" `
                -Condition ($delResp.success -eq $true -and $delResp.data.deleted -eq $true) `
                -Details "Item ID $newId deleted"

    # 11. Verify 404 on deleted item
    $notFoundCode = 0
    try {
        Invoke-RestMethod -Uri "$baseUrl/api/items/$newId" -Method Get
    } catch {
        $notFoundCode = $_.Exception.Response.StatusCode.value__
    }
    Assert-Test -Name "GET /api/items/{id} (Deleted -> 404)" `
                -Condition ($notFoundCode -eq 404) `
                -Details "HTTP Status: $notFoundCode"

    # 12. Validation Error Check: Invalid field (POST 400)
    $badPayload = @{
        title = ""
        type = "invalid_type"
    } | ConvertTo-Json

    $badCode = 0
    try {
        Invoke-RestMethod -Uri "$baseUrl/api/items" -Method Post -Body $badPayload -ContentType "application/json"
    } catch {
        $badCode = $_.Exception.Response.StatusCode.value__
    }
    Assert-Test -Name "POST /api/items with empty title -> 400" `
                -Condition ($badCode -eq 400) `
                -Details "HTTP Status: $badCode"

    # 13. Invalid JSON Body Check: Malformed JSON -> 400
    $malformedCode = 0
    try {
        Invoke-WebRequest -Uri "$baseUrl/api/items" -Method Post -Body "{ not valid json }" -ContentType "application/json" -UseBasicParsing
    } catch {
        $malformedCode = $_.Exception.Response.StatusCode.value__
    }
    Assert-Test -Name "POST /api/items with malformed JSON -> 400" `
                -Condition ($malformedCode -eq 400) `
                -Details "HTTP Status: $malformedCode"

} finally {
    Write-Host "===================================================================" -ForegroundColor Cyan
    Write-Host "  Stopping Test Server..." -ForegroundColor Cyan
    try {
        Invoke-RestMethod -Uri "$baseUrl/api/admin/shutdown" -Method Post -TimeoutSec 2 | Out-Null
    } catch {
        # Process might already be stopping
    }
    Start-Sleep -Seconds 1
    if (!$process.HasExited) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    }
    Write-Host "  Server stopped cleanly." -ForegroundColor Green
}

Write-Host "===================================================================" -ForegroundColor Cyan
Write-Host "  API Verification Report: $passedTests / $totalTests Passed" -ForegroundColor Cyan
Write-Host "===================================================================" -ForegroundColor Cyan

if ($passedTests -eq $totalTests) {
    exit 0
} else {
    exit 1
}
