@echo off
REM ============================================================================
REM x32dbg MCP Plugin - Build Script for Both x32 and x64
REM ============================================================================
REM This script automatically builds both .dp32 and .dp64 plugins
REM Just double-click this file to compile!
REM ============================================================================

echo.
echo ========================================
echo   x32dbg MCP Plugin Builder
echo   Building BOTH x32 and x64 versions
echo ========================================
echo.

REM Check if cmake is available
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake not found in PATH!
    echo.
    echo Please install CMake from: https://cmake.org/download/
    echo Or install it via Visual Studio Installer
    echo.
    pause
    exit /b 1
)

echo [*] CMake found: OK
echo.

REM Try to find Visual Studio automatically
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] Visual Studio not found!
    echo.
    echo Please install Visual Studio 2019 or later with C++ support
    echo Download from: https://visualstudio.microsoft.com/downloads/
    echo.
    pause
    exit /b 1
)

echo [*] Visual Studio found: OK
echo.

REM Clean old builds
if exist "build32" (
    echo [*] Cleaning old x32 build directory...
    rmdir /s /q build32
)
if exist "build64" (
    echo [*] Cleaning old x64 build directory...
    rmdir /s /q build64
)

REM ============================================================================
REM Build x32 (32-bit) Plugin
REM ============================================================================

echo.
echo ========================================
echo   Building x32 (32-bit) Plugin
echo ========================================
echo.

REM Create build directory
echo [*] Creating build32 directory...
mkdir build32

REM Configure CMake for 32-bit build
echo [*] Configuring CMake for x32 (Win32) build...
echo.
cmake -S . -B build32 -A Win32 -DBUILD_BOTH_ARCHES=OFF
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] x32 CMake configuration failed!
    pause
    exit /b 1
)

REM Build the plugin
echo.
echo [*] Building x32 plugin (Release mode)...
echo.
cmake --build build32 --config Release
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] x32 build failed!
    pause
    exit /b 1
)

echo.
echo [SUCCESS] x32 plugin built successfully!
echo.

REM ============================================================================
REM Build x64 (64-bit) Plugin
REM ============================================================================

echo.
echo ========================================
echo   Building x64 (64-bit) Plugin
echo ========================================
echo.

REM Create build directory
echo [*] Creating build64 directory...
mkdir build64

REM Configure CMake for 64-bit build
echo [*] Configuring CMake for x64 build...
echo.
cmake -S . -B build64 -A x64 -DBUILD_BOTH_ARCHES=OFF
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] x64 CMake configuration failed!
    pause
    exit /b 1
)

REM Build the plugin
echo.
echo [*] Building x64 plugin (Release mode)...
echo.
cmake --build build64 --config Release
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] x64 build failed!
    pause
    exit /b 1
)

echo.
echo [SUCCESS] x64 plugin built successfully!
echo.

REM ============================================================================
REM Copy All Files to Build Directory
REM ============================================================================

echo.
echo ========================================
echo   Copying Files to Build Directory
echo ========================================
echo.

REM Create output directory
if not exist "build" mkdir build

REM Copy x32 plugin
echo [*] Copying x32 plugin...
copy /Y "build32\Release\MCPx64dbg.dp32" "build\MCPx64dbg.dp32" >nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to copy x32 plugin!
    pause
    exit /b 1
)
echo     MCPx64dbg.dp32 - OK

REM Copy x64 plugin
echo [*] Copying x64 plugin...
copy /Y "build64\Release\MCPx64dbg.dp64" "build\MCPx64dbg.dp64" >nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to copy x64 plugin!
    pause
    exit /b 1
)
echo     MCPx64dbg.dp64 - OK

REM Copy Python MCP server
echo [*] Copying Python MCP server...
copy /Y "mcp_server.py" "build\mcp_server.py" >nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to copy Python MCP server!
    pause
    exit /b 1
)
echo     mcp_server.py - OK

REM Copy requirements.txt
echo [*] Copying requirements.txt...
copy /Y "requirements.txt" "build\requirements.txt" >nul
if %ERRORLEVEL% NEQ 0 (
    echo [WARNING] Failed to copy requirements.txt
) else (
    echo     requirements.txt - OK
)

REM Copy documentation
echo [*] Copying documentation...
copy /Y "README.md" "build\README.md" >nul 2>nul
copy /Y "API-REFERENCE.md" "build\API-REFERENCE.md" >nul 2>nul
echo     Documentation - OK

echo.
echo ========================================
echo   BUILD SUCCESSFUL!
echo ========================================
echo.
echo All files are in: %CD%\build\
echo.
echo Contents:
echo   - MCPx64dbg.dp32       (x32dbg 32-bit plugin)
echo   - MCPx64dbg.dp64       (x64dbg 64-bit plugin)
echo   - mcp_server.py        (Python MCP server - 48 tools)
echo   - requirements.txt     (Python dependencies)
echo   - README.md            (Documentation)
echo   - API-REFERENCE.md     (Complete API docs)
echo.
echo ========================================
echo   Installation Instructions
echo ========================================
echo.
echo 1. Install x32dbg plugins:
echo    Copy MCPx64dbg.dp32 to: x32dbg\plugins\
echo    Copy MCPx64dbg.dp64 to: x64dbg\plugins\
echo.
echo 2. Setup Python MCP Server:
echo    cd build
echo    pip install -r requirements.txt
echo.
echo 3. Configure Claude Code:
echo    Add to your .claude.json or VSCode settings:
echo    {
echo      "mcpServers": {
echo        "x32dbg": {
echo          "command": "python",
echo          "args": ["path\\to\\build\\mcp_server.py"],
echo          "env": {"X64DBG_URL": "http://127.0.0.1:8888"}
echo        }
echo      }
echo    }
echo.
echo 4. Restart x32dbg/x64dbg and check logs (ALT+L)
echo    You should see: [MCP] HTTP server started on port 8888
echo.
echo 5. Restart Claude Code to load the MCP server
echo.
echo ========================================
echo.
pause
