# Debugging Guide: x32dbg MCP Server Not Loading

## The Problem

You configured the MCP server in `claude_desktop_config.json`, but you're using **Claude Code (VSCode extension)**, not Claude Desktop. These are different applications!

## Solution: Configure for Claude Code (VSCode)

### Option 1: Workspace Settings (Recommended)

Create or edit `.vscode/settings.json` in your project:

```json
{
  "claude.mcpServers": {
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

### Option 2: User Settings

1. Open VSCode Settings (Ctrl+,)
2. Search for "claude mcp"
3. Click "Edit in settings.json"
4. Add the same configuration as above

### Option 3: Use Claude Desktop Instead

If you want to use Claude Desktop (the standalone app):

1. Your `claude_desktop_config.json` config is already correct:
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

2. **Close Claude Code**
3. **Open Claude Desktop** (the standalone app)
4. The x32dbg tools should appear there

---

## Quick Test

### 1. Verify Python & Dependencies Work

```bash
cd C:\Tools
pip install -r requirements.txt
python mcp_server.py
```

You should see: `✅ Connected to x32dbg (arch: x32)`

Press Ctrl+C to stop.

### 2. Test MCP Protocol Manually

```bash
echo {"method":"initialize","params":{"protocolVersion":"1.0","capabilities":{}}} | python mcp_server.py
```

Should return initialization response.

---

## Which App Are You Using?

- **Claude Desktop** = Standalone app (Electron), config in `%APPDATA%\Claude\claude_desktop_config.json`
- **Claude Code** = VSCode extension, config in VSCode settings

Check which one you have open! The VSCode extension won't read Claude Desktop's config file.

---

## Verify x32dbg Plugin is Running

1. Open x32dbg
2. Press ALT+L (logs)
3. Look for:
   ```
   [MCP] Plugin loading...
   [MCP] HTTP server started on port 8888
   ```

4. Test in browser: http://127.0.0.1:8888/status
   - Should return: `{"version":2,"arch":"x32","debugging":false,"running":false}`

---

## Still Not Working?

### Check VSCode Extension Logs

1. Open VSCode Output panel (Ctrl+Shift+U)
2. Select "Claude Code" from dropdown
3. Look for MCP server errors

### Enable Debug Logging

Add to your VSCode settings:
```json
{
  "claude.debug": true,
  "claude.mcpServers": { ... }
}
```

### Fallback: Direct Testing

Even without MCP integration, you can test the plugin directly:

```python
import requests

# Test status
r = requests.get("http://127.0.0.1:8888/status")
print(r.json())

# Get register
r = requests.get("http://127.0.0.1:8888/register/get?name=eax")
print(r.json())

# Get modules
r = requests.get("http://127.0.0.1:8888/modules")
print(r.json())
```

This proves the plugin works, even if MCP integration has issues.
