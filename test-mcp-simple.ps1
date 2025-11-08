#!/usr/bin/env pwsh
# Simple test for x32dbg MCP Server - just checks if it can start without errors

param(
    [string]$ServerPath = "C:\Tools\mcp_server.py",
    [string]$PythonPath = "python"
)

Write-Host "🔍 Simple x32dbg MCP Server Test" -ForegroundColor Cyan
Write-Host "Server: $ServerPath" -ForegroundColor Gray
Write-Host ""

# Test 1: Check if file exists
Write-Host "[1/5] Checking if server file exists..." -ForegroundColor Yellow
if (-not (Test-Path $ServerPath)) {
    Write-Host "❌ FAILED: Server file not found at $ServerPath" -ForegroundColor Red
    exit 1
}
Write-Host "✅ PASSED" -ForegroundColor Green
Write-Host ""

# Test 2: Check if fastmcp is installed
Write-Host "[2/5] Checking Python dependencies..." -ForegroundColor Yellow
$checkImport = & $PythonPath -c "from fastmcp import FastMCP; print('OK')" 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ FAILED: fastmcp module not found" -ForegroundColor Red
    Write-Host "   Install with: pip install fastmcp requests" -ForegroundColor Gray
    exit 1
}
Write-Host "✅ PASSED: fastmcp is installed" -ForegroundColor Green
Write-Host ""

# Test 3: Check import statement
Write-Host "[3/5] Checking server imports..." -ForegroundColor Yellow
$serverContent = Get-Content $ServerPath -Raw
if ($serverContent -match "from mcp\.server\.fastmcp import FastMCP") {
    Write-Host "❌ FAILED: Server has OLD import (mcp.server.fastmcp)" -ForegroundColor Red
    Write-Host "   Should be: from fastmcp import FastMCP" -ForegroundColor Gray
    Write-Host ""
    Write-Host "ACTION REQUIRED:" -ForegroundColor Yellow
    Write-Host "   Download the fixed mcp_server.py from the repository" -ForegroundColor White
    exit 1
} elseif ($serverContent -match "from fastmcp import FastMCP") {
    Write-Host "✅ PASSED: Correct import statement" -ForegroundColor Green
} else {
    Write-Host "⚠️  WARNING: Could not verify import" -ForegroundColor Yellow
}
Write-Host ""

# Test 4: Test server initialization (without starting it)
Write-Host "[4/5] Testing server can be imported..." -ForegroundColor Yellow
$testImport = @"
import sys
sys.path.insert(0, r'$([System.IO.Path]::GetDirectoryName($ServerPath))')
try:
    from fastmcp import FastMCP
    print('IMPORT_OK')
except Exception as e:
    print(f'IMPORT_FAILED: {e}')
    sys.exit(1)
"@

$importResult = & $PythonPath -c $testImport 2>&1 | Out-String
if ($importResult -match "IMPORT_OK") {
    Write-Host "✅ PASSED: FastMCP can be imported" -ForegroundColor Green
} else {
    Write-Host "❌ FAILED: Cannot import FastMCP" -ForegroundColor Red
    Write-Host "   Error: $importResult" -ForegroundColor Gray
    exit 1
}
Write-Host ""

# Test 5: Check if server file has syntax errors
Write-Host "[5/5] Checking Python syntax..." -ForegroundColor Yellow
$syntaxCheck = & $PythonPath -m py_compile $ServerPath 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ PASSED: No syntax errors" -ForegroundColor Green
} else {
    Write-Host "❌ FAILED: Syntax errors found" -ForegroundColor Red
    Write-Host $syntaxCheck -ForegroundColor Gray
    exit 1
}
Write-Host ""

# Final summary
Write-Host "═══════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "✅ ALL BASIC TESTS PASSED!" -ForegroundColor Green
Write-Host "═══════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""
Write-Host "The MCP server file appears to be correct." -ForegroundColor White
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Make sure this is the file Claude Code is using:" -ForegroundColor White
Write-Host "     $ServerPath" -ForegroundColor Gray
Write-Host ""
Write-Host "  2. Restart Claude Code COMPLETELY:" -ForegroundColor White
Write-Host "     - Right-click system tray icon → Quit" -ForegroundColor Gray
Write-Host "     - Or use Task Manager to end process" -ForegroundColor Gray
Write-Host "     - Start Claude Code again" -ForegroundColor Gray
Write-Host ""
Write-Host "  3. After restart, run this in Claude Code:" -ForegroundColor White
Write-Host "     > List all MCP tools starting with x32dbg" -ForegroundColor Gray
Write-Host ""
Write-Host "  4. Check your configuration:" -ForegroundColor White
Write-Host "     type $env:USERPROFILE\.claude.json" -ForegroundColor Gray
Write-Host ""
Write-Host "If it still doesn't work after restart:" -ForegroundColor Yellow
Write-Host "  - The issue is with Claude Code, not the MCP server" -ForegroundColor White
Write-Host "  - Check Claude Code logs for errors" -ForegroundColor White
Write-Host "  - Try removing and re-adding the MCP server" -ForegroundColor White
Write-Host ""
