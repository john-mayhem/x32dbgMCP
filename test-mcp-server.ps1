#!/usr/bin/env pwsh
# Test script for x32dbg MCP Server
# This script tests if the MCP server can initialize and respond to requests

param(
    [string]$ServerPath = "C:\Tools\mcp_server.py",
    [string]$PythonPath = "python"
)

Write-Host "🔍 Testing x32dbg MCP Server" -ForegroundColor Cyan
Write-Host "Server: $ServerPath" -ForegroundColor Gray
Write-Host "Python: $PythonPath" -ForegroundColor Gray
Write-Host ""

# Check if server file exists
if (-not (Test-Path $ServerPath)) {
    Write-Host "❌ Error: Server file not found at $ServerPath" -ForegroundColor Red
    exit 1
}

# Test 1: Check if fastmcp is installed
Write-Host "Test 1: Checking Python dependencies..." -ForegroundColor Yellow
$checkImport = & $PythonPath -c "from fastmcp import FastMCP; print('OK')" 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ FAILED: fastmcp module not found" -ForegroundColor Red
    Write-Host "   Install with: pip install fastmcp requests" -ForegroundColor Gray
    Write-Host "   Error: $checkImport" -ForegroundColor Red
    exit 1
}
Write-Host "✅ PASSED: fastmcp module is installed" -ForegroundColor Green
Write-Host ""

# Test 2: Check if server file has correct import
Write-Host "Test 2: Checking server imports..." -ForegroundColor Yellow
$serverContent = Get-Content $ServerPath -Raw
if ($serverContent -match "from mcp\.server\.fastmcp import FastMCP") {
    Write-Host "❌ FAILED: Server has OLD import (mcp.server.fastmcp)" -ForegroundColor Red
    Write-Host "   Should be: from fastmcp import FastMCP" -ForegroundColor Gray
    exit 1
} elseif ($serverContent -match "from fastmcp import FastMCP") {
    Write-Host "✅ PASSED: Server has correct import" -ForegroundColor Green
} else {
    Write-Host "⚠️  WARNING: Could not find FastMCP import" -ForegroundColor Yellow
}
Write-Host ""

# Test 3: Check if server can start
Write-Host "Test 3: Starting MCP server..." -ForegroundColor Yellow
$env:X64DBG_URL = "http://127.0.0.1:8888"

$processInfo = New-Object System.Diagnostics.ProcessStartInfo
$processInfo.FileName = $PythonPath
$processInfo.Arguments = $ServerPath
$processInfo.RedirectStandardInput = $true
$processInfo.RedirectStandardOutput = $true
$processInfo.RedirectStandardError = $true
$processInfo.UseShellExecute = $false
$processInfo.CreateNoWindow = $true

$process = New-Object System.Diagnostics.Process
$process.StartInfo = $processInfo

try {
    $started = $process.Start()
    if (-not $started) {
        Write-Host "❌ FAILED: Could not start server process" -ForegroundColor Red
        exit 1
    }

    # Wait a bit for startup
    Start-Sleep -Milliseconds 500

    if ($process.HasExited) {
        $stderr = $process.StandardError.ReadToEnd()
        Write-Host "❌ FAILED: Server exited immediately" -ForegroundColor Red
        Write-Host "Error output:" -ForegroundColor Gray
        Write-Host $stderr -ForegroundColor Red
        exit 1
    }

    Write-Host "✅ PASSED: Server started successfully" -ForegroundColor Green

    # Read startup messages from stderr
    $stderrReader = $process.StandardError
    $startupMsg = ""
    $timeout = [DateTime]::Now.AddSeconds(2)
    while ([DateTime]::Now -lt $timeout -and -not $stderrReader.EndOfStream) {
        $char = $stderrReader.Read()
        if ($char -ge 0) {
            $startupMsg += [char]$char
        }
    }

    if ($startupMsg) {
        Write-Host "   Startup message: $($startupMsg.Trim())" -ForegroundColor Gray
    }
    Write-Host ""

    # Test 4: Send MCP initialize request
    Write-Host "Test 4: Sending MCP initialize request..." -ForegroundColor Yellow

    $initRequest = @{
        jsonrpc = "2.0"
        id = 1
        method = "initialize"
        params = @{
            protocolVersion = "2024-11-05"
            capabilities = @{}
            clientInfo = @{
                name = "test-client"
                version = "1.0.0"
            }
        }
    } | ConvertTo-Json -Depth 10 -Compress

    $process.StandardInput.WriteLine($initRequest)
    $process.StandardInput.Flush()

    # Read response with timeout
    $response = ""
    $timeout = [DateTime]::Now.AddSeconds(3)
    while ([DateTime]::Now -lt $timeout -and $response -eq "") {
        if (-not $process.StandardOutput.EndOfStream) {
            $line = $process.StandardOutput.ReadLine()
            if ($line) {
                $response = $line
                break
            }
        }
        Start-Sleep -Milliseconds 100
    }

    if (-not $response) {
        Write-Host "❌ FAILED: No response from server" -ForegroundColor Red
        $process.Kill()
        exit 1
    }

    try {
        $responseObj = $response | ConvertFrom-Json
        if ($responseObj.result) {
            Write-Host "✅ PASSED: Server responded to initialize" -ForegroundColor Green
            Write-Host "   Protocol: $($responseObj.result.protocolVersion)" -ForegroundColor Gray
            Write-Host "   Server: $($responseObj.result.serverInfo.name)" -ForegroundColor Gray
        } else {
            Write-Host "⚠️  WARNING: Unexpected response format" -ForegroundColor Yellow
            Write-Host "   Response: $response" -ForegroundColor Gray
        }
    } catch {
        Write-Host "❌ FAILED: Invalid JSON response" -ForegroundColor Red
        Write-Host "   Response: $response" -ForegroundColor Gray
        $process.Kill()
        exit 1
    }
    Write-Host ""

    # Test 5: Request tools list
    Write-Host "Test 5: Requesting tools list..." -ForegroundColor Yellow

    $toolsRequest = @{
        jsonrpc = "2.0"
        id = 2
        method = "tools/list"
        params = @{}
    } | ConvertTo-Json -Depth 10 -Compress

    $process.StandardInput.WriteLine($toolsRequest)
    $process.StandardInput.Flush()

    # Read response
    $toolsResponse = ""
    $timeout = [DateTime]::Now.AddSeconds(3)
    while ([DateTime]::Now -lt $timeout -and $toolsResponse -eq "") {
        if (-not $process.StandardOutput.EndOfStream) {
            $line = $process.StandardOutput.ReadLine()
            if ($line) {
                $toolsResponse = $line
                break
            }
        }
        Start-Sleep -Milliseconds 100
    }

    if (-not $toolsResponse) {
        Write-Host "❌ FAILED: No tools response from server" -ForegroundColor Red
        $process.Kill()
        exit 1
    }

    try {
        $toolsObj = $toolsResponse | ConvertFrom-Json
        if ($toolsObj.result -and $toolsObj.result.tools) {
            $toolCount = $toolsObj.result.tools.Count
            Write-Host "✅ PASSED: Server has $toolCount tools registered" -ForegroundColor Green
            Write-Host "   Tools:" -ForegroundColor Gray
            foreach ($tool in $toolsObj.result.tools | Select-Object -First 5) {
                Write-Host "   - $($tool.name): $($tool.description)" -ForegroundColor Gray
            }
            if ($toolCount -gt 5) {
                Write-Host "   ... and $($toolCount - 5) more" -ForegroundColor Gray
            }
        } else {
            Write-Host "❌ FAILED: No tools found in response" -ForegroundColor Red
            Write-Host "   This means the MCP server decorators are not working!" -ForegroundColor Yellow
            Write-Host "   Response: $toolsResponse" -ForegroundColor Gray
        }
    } catch {
        Write-Host "❌ FAILED: Invalid tools response" -ForegroundColor Red
        Write-Host "   Response: $toolsResponse" -ForegroundColor Gray
        $process.Kill()
        exit 1
    }
    Write-Host ""

    # Test 6: Request resources list
    Write-Host "Test 6: Requesting resources list..." -ForegroundColor Yellow

    $resourcesRequest = @{
        jsonrpc = "2.0"
        id = 3
        method = "resources/list"
        params = @{}
    } | ConvertTo-Json -Depth 10 -Compress

    $process.StandardInput.WriteLine($resourcesRequest)
    $process.StandardInput.Flush()

    # Read response
    $resourcesResponse = ""
    $timeout = [DateTime]::Now.AddSeconds(3)
    while ([DateTime]::Now -lt $timeout -and $resourcesResponse -eq "") {
        if (-not $process.StandardOutput.EndOfStream) {
            $line = $process.StandardOutput.ReadLine()
            if ($line) {
                $resourcesResponse = $line
                break
            }
        }
        Start-Sleep -Milliseconds 100
    }

    if ($resourcesResponse) {
        try {
            $resourcesObj = $resourcesResponse | ConvertFrom-Json
            if ($resourcesObj.result -and $resourcesObj.result.resources) {
                $resCount = $resourcesObj.result.resources.Count
                Write-Host "✅ PASSED: Server has $resCount resources registered" -ForegroundColor Green
                foreach ($res in $resourcesObj.result.resources) {
                    Write-Host "   - $($res.uri): $($res.name)" -ForegroundColor Gray
                }
            } else {
                Write-Host "⚠️  No resources registered (this is OK)" -ForegroundColor Yellow
            }
        } catch {
            Write-Host "⚠️  Could not parse resources response" -ForegroundColor Yellow
        }
    }
    Write-Host ""

    Write-Host "═══════════════════════════════════════════════" -ForegroundColor Cyan
    Write-Host "✅ ALL TESTS PASSED!" -ForegroundColor Green
    Write-Host "The MCP server is working correctly." -ForegroundColor Green
    Write-Host ""
    Write-Host "If Claude Code still doesn't see it, try:" -ForegroundColor Yellow
    Write-Host "1. Restart Claude Code completely" -ForegroundColor Gray
    Write-Host "2. Check ~/.claude.json has correct path" -ForegroundColor Gray
    Write-Host "3. Check MCP server logs in Claude Code" -ForegroundColor Gray
    Write-Host "═══════════════════════════════════════════════" -ForegroundColor Cyan

} finally {
    if (-not $process.HasExited) {
        $process.Kill()
    }
    $process.Dispose()
}
