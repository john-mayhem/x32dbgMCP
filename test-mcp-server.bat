@echo off
REM Simple batch script to test x32dbg MCP Server
REM Usage: test-mcp-server.bat [path-to-mcp_server.py]

setlocal enabledelayedexpansion

set "SERVER_PATH=%~1"
if "%SERVER_PATH%"=="" set "SERVER_PATH=C:\Tools\mcp_server.py"

echo ========================================
echo Testing x32dbg MCP Server
echo ========================================
echo Server: %SERVER_PATH%
echo.

REM Test 1: Check if file exists
echo [1/3] Checking if server file exists...
if not exist "%SERVER_PATH%" (
    echo [ERROR] Server file not found: %SERVER_PATH%
    exit /b 1
)
echo [OK] File exists
echo.

REM Test 2: Check if fastmcp is installed
echo [2/3] Checking if fastmcp is installed...
python -c "from fastmcp import FastMCP; print('OK')" 2>nul
if errorlevel 1 (
    echo [ERROR] fastmcp module not found
    echo.
    echo Please install dependencies:
    echo   pip install fastmcp requests
    echo.
    exit /b 1
)
echo [OK] fastmcp is installed
echo.

REM Test 3: Check import statement
echo [3/3] Checking server import statement...
findstr /C:"from mcp.server.fastmcp import FastMCP" "%SERVER_PATH%" >nul 2>&1
if not errorlevel 1 (
    echo [ERROR] Server has OLD import statement!
    echo.
    echo Found: from mcp.server.fastmcp import FastMCP
    echo Should be: from fastmcp import FastMCP
    echo.
    echo Please update your server file with the fixed version.
    exit /b 1
)

findstr /C:"from fastmcp import FastMCP" "%SERVER_PATH%" >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Could not verify FastMCP import
) else (
    echo [OK] Import statement is correct
)
echo.

REM Test 4: Try to start server (quick test)
echo ========================================
echo Starting server test...
echo ========================================
echo.
echo Press Ctrl+C if it hangs for more than 5 seconds
echo.

set X64DBG_URL=http://127.0.0.1:8888
timeout /t 2 /nobreak >nul

REM Try to import and check for errors
python -c "import sys; sys.path.insert(0, r'%~dp1'); exec(open(r'%SERVER_PATH%').read())" 2>&1 | findstr /C:"Traceback" /C:"Error" /C:"Exception"
if not errorlevel 1 (
    echo.
    echo [ERROR] Server has Python errors!
    echo Run the command below to see details:
    echo   python "%SERVER_PATH%"
    echo.
    exit /b 1
)

echo ========================================
echo Basic tests PASSED!
echo ========================================
echo.
echo To do a full MCP protocol test, run:
echo   powershell -ExecutionPolicy Bypass -File test-mcp-server.ps1
echo.
echo If Claude Code still doesn't see the server:
echo 1. Make sure you restarted Claude Code completely
echo 2. Check C:\Users\%USERNAME%\.claude.json for correct path
echo 3. Run: claude mcp list
echo.

endlocal
