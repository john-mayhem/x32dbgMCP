# MCP Server Testing Guide

This guide helps you diagnose issues with the x32dbg MCP server.

## Quick Test (Windows)

### Option 1: Batch Script (Simple)
```cmd
test-mcp-server.bat C:\Tools\mcp_server.py
```

This performs basic checks:
- ✅ Verifies the file exists
- ✅ Checks if `fastmcp` is installed
- ✅ Validates the import statement is correct

### Option 2: PowerShell Script (Comprehensive)
```powershell
powershell -ExecutionPolicy Bypass -File test-mcp-server.ps1 -ServerPath C:\Tools\mcp_server.py
```

This performs complete MCP protocol tests:
- ✅ Checks Python dependencies
- ✅ Validates import statements
- ✅ Starts the MCP server
- ✅ Sends MCP `initialize` request
- ✅ Lists all available tools
- ✅ Lists all available resources

## Expected Results

### ✅ Success
```
✅ PASSED: fastmcp module is installed
✅ PASSED: Server has correct import
✅ PASSED: Server started successfully
✅ PASSED: Server responded to initialize
✅ PASSED: Server has 17 tools registered
```

If you see this, **the MCP server is working!** The issue is with Claude Code integration.

### ❌ Import Error
```
❌ FAILED: Server has OLD import (mcp.server.fastmcp)
   Should be: from fastmcp import FastMCP
```

**Fix**: Replace `C:\Tools\mcp_server.py` with the fixed version from this repository.

### ❌ Missing Module
```
❌ FAILED: fastmcp module not found
```

**Fix**: Install dependencies:
```cmd
pip install fastmcp requests
```

## Troubleshooting Claude Code Integration

If the test passes but Claude Code still doesn't see the server:

### 1. Check Configuration
```cmd
type %USERPROFILE%\.claude.json
```

Should contain:
```json
{
  "mcpServers": {
    "x32dbg": {
      "command": "python",
      "args": ["C:\\Tools\\mcp_server.py"],
      "env": {
        "X64DBG_URL": "http://127.0.0.1:8888"
      }
    }
  }
}
```

### 2. Verify MCP Registration
```cmd
claude mcp list
```

Should show:
```
x32dbg: python C:\Tools\mcp_server.py - ✓ Connected
```

### 3. Restart Claude Code

**Important**: You must restart Claude Code **completely** (not just close the window):
- Right-click system tray icon → Quit
- Or: Task Manager → End Claude Code process
- Then start Claude Code again

### 4. Check MCP Server Logs

In Claude Code, check if tools are available:
```
> List available MCP tools
```

Should show tools like:
- `mcp__x32dbg__get_status`
- `mcp__x32dbg__read_memory`
- `mcp__x32dbg__set_breakpoint`
- etc.

## Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| "Connected" but no tools | Wrong import statement | Update `mcp_server.py` |
| Import error on startup | Missing `fastmcp` package | Run `pip install fastmcp` |
| Server won't start | Python path issues | Use full path to `python.exe` |
| Config not loaded | Claude Code cache | Delete `%APPDATA%\.claude\cache` |

## Manual Test (Advanced)

You can also test the server manually with netcat or by piping JSON:

```powershell
# Start the server
$env:X64DBG_URL="http://127.0.0.1:8888"
python C:\Tools\mcp_server.py

# In another terminal, send MCP request
@"
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}
"@ | python C:\Tools\mcp_server.py
```

## Need Help?

1. Run the PowerShell test script and save the output
2. Check Claude Code logs (if available)
3. Report the issue with both outputs

## Files

- `test-mcp-server.bat` - Simple batch script for basic checks
- `test-mcp-server.ps1` - Comprehensive PowerShell test script
- `mcp_server.py` - The actual MCP server (fixed version)
- `requirements.txt` - Python dependencies
