#!/usr/bin/env pwsh
# ACTUAL MCP Server Test - Tests the protocol, not dependencies
# This tells you if the MCP server is working or broken

param(
    [string]$ServerPath = "C:\Tools\mcp_server.py",
    [string]$PythonPath = "python"
)

Write-Host "═══════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "MCP SERVER PROTOCOL TEST" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "Server: $ServerPath" -ForegroundColor Gray
Write-Host ""

# Start the MCP server
Write-Host "[1/3] Starting MCP server..." -ForegroundColor Yellow

$env:X64DBG_URL = "http://127.0.0.1:8888"

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $PythonPath
$psi.Arguments = "`"$ServerPath`""
$psi.UseShellExecute = $false
$psi.RedirectStandardInput = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.CreateNoWindow = $true
$psi.EnvironmentVariables["X64DBG_URL"] = "http://127.0.0.1:8888"

$process = New-Object System.Diagnostics.Process
$process.StartInfo = $psi

try {
    [void]$process.Start()
    Start-Sleep -Milliseconds 500

    if ($process.HasExited) {
        Write-Host "❌ FAILED: Server crashed on startup" -ForegroundColor Red
        $err = $process.StandardError.ReadToEnd()
        Write-Host $err -ForegroundColor Red
        exit 1
    }

    Write-Host "✅ Server is running (PID: $($process.Id))" -ForegroundColor Green
    Write-Host ""

    # Send initialize request
    Write-Host "[2/3] Testing MCP initialize..." -ForegroundColor Yellow

    $initRequest = @{
        jsonrpc = "2.0"
        id = 1
        method = "initialize"
        params = @{
            protocolVersion = "2024-11-05"
            capabilities = @{}
            clientInfo = @{
                name = "test"
                version = "1.0"
            }
        }
    } | ConvertTo-Json -Compress -Depth 10

    $process.StandardInput.WriteLine($initRequest)
    $process.StandardInput.Flush()

    # Wait for response with timeout
    $response = $null
    $timeout = [DateTime]::Now.AddSeconds(5)

    while ([DateTime]::Now -lt $timeout) {
        if ($process.StandardOutput.Peek() -ge 0) {
            $response = $process.StandardOutput.ReadLine()
            break
        }
        Start-Sleep -Milliseconds 50
    }

    if (-not $response) {
        Write-Host "❌ FAILED: No response from server (timeout)" -ForegroundColor Red
        Write-Host ""
        Write-Host "This means the MCP server is NOT working!" -ForegroundColor Red
        $process.Kill()
        exit 1
    }

    $initResult = $response | ConvertFrom-Json
    if ($initResult.result) {
        Write-Host "✅ Server responded to initialize" -ForegroundColor Green
        Write-Host "   Server: $($initResult.result.serverInfo.name)" -ForegroundColor Gray
    } else {
        Write-Host "❌ FAILED: Invalid initialize response" -ForegroundColor Red
        Write-Host $response -ForegroundColor Gray
        $process.Kill()
        exit 1
    }
    Write-Host ""

    # Send tools/list request
    Write-Host "[3/3] Testing tools/list (THE CRITICAL TEST)..." -ForegroundColor Yellow

    $toolsRequest = @{
        jsonrpc = "2.0"
        id = 2
        method = "tools/list"
        params = @{}
    } | ConvertTo-Json -Compress -Depth 10

    $process.StandardInput.WriteLine($toolsRequest)
    $process.StandardInput.Flush()

    # Wait for tools response
    $toolsResponse = $null
    $timeout = [DateTime]::Now.AddSeconds(5)

    while ([DateTime]::Now -lt $timeout) {
        if ($process.StandardOutput.Peek() -ge 0) {
            $toolsResponse = $process.StandardOutput.ReadLine()
            break
        }
        Start-Sleep -Milliseconds 50
    }

    if (-not $toolsResponse) {
        Write-Host "❌ FAILED: No tools response (timeout)" -ForegroundColor Red
        $process.Kill()
        exit 1
    }

    $toolsResult = $toolsResponse | ConvertFrom-Json
    if ($toolsResult.result -and $toolsResult.result.tools) {
        $toolCount = $toolsResult.result.tools.Count

        if ($toolCount -eq 0) {
            Write-Host "❌ FAILED: Server has ZERO tools!" -ForegroundColor Red
            Write-Host ""
            Write-Host "╔════════════════════════════════════════════════╗" -ForegroundColor Red
            Write-Host "║  THE MCP SERVER IS BROKEN!                     ║" -ForegroundColor Red
            Write-Host "║  It's not registering any tools.               ║" -ForegroundColor Red
            Write-Host "║  You still have the OLD buggy file!            ║" -ForegroundColor Red
            Write-Host "╚════════════════════════════════════════════════╝" -ForegroundColor Red
            Write-Host ""
            Write-Host "FIX: Replace $ServerPath" -ForegroundColor Yellow
            Write-Host "     with the fixed version from the repository" -ForegroundColor Yellow
            $process.Kill()
            exit 1
        }

        Write-Host "✅ Server has $toolCount tools registered!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Tools:" -ForegroundColor Cyan
        foreach ($tool in $toolsResult.result.tools | Select-Object -First 10) {
            Write-Host "  • $($tool.name)" -ForegroundColor White
        }
        if ($toolCount -gt 10) {
            Write-Host "  ... and $($toolCount - 10) more" -ForegroundColor Gray
        }

    } else {
        Write-Host "❌ FAILED: Invalid tools response" -ForegroundColor Red
        Write-Host $toolsResponse -ForegroundColor Gray
        $process.Kill()
        exit 1
    }

    Write-Host ""
    Write-Host "═══════════════════════════════════════════════" -ForegroundColor Green
    Write-Host "✅ MCP SERVER IS WORKING CORRECTLY!" -ForegroundColor Green
    Write-Host "═══════════════════════════════════════════════" -ForegroundColor Green
    Write-Host ""
    Write-Host "The problem is NOT the MCP server." -ForegroundColor White
    Write-Host "The problem IS Claude Code integration." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Cyan
    Write-Host "  1. Close Claude Code COMPLETELY (Task Manager)" -ForegroundColor White
    Write-Host "  2. Restart Claude Code" -ForegroundColor White
    Write-Host "  3. Run: claude mcp list" -ForegroundColor White
    Write-Host "  4. Check tools are visible in Claude Code" -ForegroundColor White
    Write-Host ""

} catch {
    Write-Host "❌ ERROR: $_" -ForegroundColor Red
    exit 1
} finally {
    if ($process -and -not $process.HasExited) {
        $process.Kill()
    }
}
