# x32dbg MCP Server

**Full-Featured Model Context Protocol (MCP) server for x32dbg debugger**

Give Claude direct access to **43+ comprehensive x32dbg debugging capabilities** through natural language!

![Architecture](https://img.shields.io/badge/x32dbg-MCP%20Server-blue)
![Language](https://img.shields.io/badge/C++-Python-green)
![Tools](https://img.shields.io/badge/tools-43+-brightgreen)
![Status](https://img.shields.io/badge/status-stable-success)

---

## 🎯 What is This?

This project connects x32dbg (Windows debugger) to Claude AI through the Model Context Protocol (MCP). You can:

- Debug programs using natural language
- Ask Claude to analyze functions, find crypto, set breakpoints
- Get real-time debugging context and assistance
- Automate reverse engineering workflows

---

## 🚀 Quick Start

### Prerequisites

- **x32dbg** installed ([download here](https://x64dbg.com/))
- **Python 3.8+** ([download here](https://www.python.org/downloads/))
- **Visual Studio 2019+** with C++ support (for compiling plugin)
- **Claude Desktop** or **Claude Code** (VSCode extension)

### Installation

#### 1. Build the Plugin

Just double-click `build.bat` - it handles everything!

```batch
# OR manually:
cmake -S . -B build32 -A Win32 -DBUILD_BOTH_ARCHES=OFF
cmake --build build32 --config Release
```

The compiled plugin will be at: `build/MCPx64dbg.dp32`

#### 2. Install the Plugin

Copy the plugin to your x32dbg plugins folder:

```
C:\Program Files\x64dbg\release\x32\plugins\MCPx64dbg.dp32
```

#### 3. Setup Python MCP Server

```bash
# Install dependencies
pip install mcp requests

# Test the server
python mcp_server.py
```

#### 4. Configure Your Claude App

⚠️ **IMPORTANT:** Claude Desktop and Claude Code (VSCode) use **different configuration files**!

##### Option A: Claude Desktop (Standalone App)

Edit: `%APPDATA%\Claude\claude_desktop_config.json` (Windows) or `~/Library/Application Support/Claude/claude_desktop_config.json` (Mac/Linux)

```json
{
  "mcpServers": {
    "x32dbg": {
      "command": "python",
      "args": ["C:\\path\\to\\x64dbgMCP\\mcp_server.py"],
      "env": {
        "X64DBG_URL": "http://127.0.0.1:8888"
      }
    }
  }
}
```

##### Option B: Claude Code (VSCode Extension) ⭐ You're probably using this!

Create `.vscode/settings.json` in your workspace:

```json
{
  "claude.mcpServers": {
    "x32dbg": {
      "command": "python",
      "args": ["C:\\path\\to\\x64dbgMCP\\mcp_server.py"],
      "env": {
        "X64DBG_URL": "http://127.0.0.1:8888"
      }
    }
  }
}
```

**Then restart VSCode completely!**

---

## ✅ Verify Installation

1. **Start x32dbg**
2. **Check plugin loaded:** Press `ALT+L` to open logs, you should see:
   ```
   [MCP] Plugin loading...
   [MCP] HTTP server started on port 8888
   ```
3. **Test HTTP API:** Open browser to `http://127.0.0.1:8888/status`
4. **Start Claude Desktop** and verify x32dbg server appears in MCP settings

---

## 📖 Usage Examples

### Basic Debugging

```
You: "Set a breakpoint at 0x401000 and show me the disassembly"

Claude: [Uses set_breakpoint and disassemble_at tools]
```

### Function Analysis

```
You: "Analyze the current function"

Claude: [Uses analyze_function prompt, gets registers, disassembles, analyzes flow]
```

### Memory Operations

```
You: "Read 32 bytes from ESP and show me the stack contents"

Claude: [Uses get_register("esp") and read_memory tools]
```

### Automation

```
You: "Find all calls to GetProcAddress in the current module"

Claude: [Uses get_modules, searches memory, sets breakpoints]
```

---

## 🔧 Available Tools (43+)

> **📚 See [API-REFERENCE.md](API-REFERENCE.md) for complete API documentation**

### Core Operations (8)
- Status & control, command execution, debugging control
- Register read/write, memory read/write
- Disassembly, module listing

### Pattern & Memory Search (3)
- `find_pattern_in_memory` - Search for byte patterns
- `memory_search` - Find all occurrences
- `search_and_replace_pattern` - Pattern replacement

### Symbol & Analysis (15)
- `get_symbols` - List functions, imports, exports
- `set_label` / `get_label` / `delete_label` / `get_all_labels` - Label management
- `set_comment` / `get_comment` / `delete_comment` / `get_all_comments` - Comment management
- `resolve_label` - Resolve label names to addresses

### Stack Operations (3)
- `stack_push` / `stack_pop` / `stack_peek` - Stack manipulation

### Function Operations (4)
- `add_function` / `get_function_info` / `delete_function` / `get_all_functions`

### Bookmark Operations (4)
- `set_bookmark` / `check_bookmark` / `delete_bookmark` / `get_all_bookmarks`

### Miscellaneous Utilities (3)
- `parse_expression` - Evaluate complex expressions
- `resolve_api_address` - Find API addresses in debuggee
- `resolve_label_address` - Label resolution

---

## 🎨 MCP Resources

Resources provide Claude with contextual information:

- `debugger://status` - Current debugging state
- `debugger://modules` - Loaded modules list

---

## 🎭 MCP Prompts

Prompts guide Claude for common tasks:

- `analyze_function` - Start function analysis workflow
- `find_crypto` - Search for crypto patterns
- `trace_execution` - Setup execution tracing

---

## 🏗️ Architecture

```
Claude Desktop/VSCode
        ↕ (MCP Protocol)
  mcp_server.py (Python)
        ↕ (HTTP REST API)
  MCPx64dbg.dp32 (C++ Plugin)
        ↕ (x64dbg SDK)
      x32dbg.exe
```

---

## 🐛 Troubleshooting

### Plugin Not Loading

1. Check logs (ALT+L in x32dbg)
2. Ensure plugin is in correct folder
3. Try restarting x32dbg

### HTTP Server Not Responding

1. Check port 8888 is not in use: `netstat -ano | findstr 8888`
2. Check firewall settings
3. Try changing port in plugin (recompile needed)

### MCP Server Can't Connect

1. Ensure x32dbg is running with plugin loaded
2. Test manually: `curl http://127.0.0.1:8888/status`
3. Check X64DBG_URL environment variable

### Build Errors

1. Ensure Visual Studio is installed with C++ support
2. Ensure CMake is in PATH
3. Check `deps/pluginsdk` folder exists

---

## 📦 Repository Structure

```
x64dbgMCP/
├── build.bat              # One-click build script
├── mcp_server.py          # Python MCP server (860+ lines, 43+ tools)
├── src/
│   ├── MCPx64dbg.cpp      # C++ plugin main file (modular, 600+ lines)
│   └── MCPx64dbg_old.cpp  # Original version (backup)
├── include/               # Modular handler headers
│   ├── mcp_common.h       # Common utilities and helpers
│   ├── mcp_handlers_pattern.h      # Pattern/memory search
│   ├── mcp_handlers_annotation.h   # Symbols/labels/comments
│   ├── mcp_handlers_stack.h        # Stack operations
│   ├── mcp_handlers_function.h     # Functions/bookmarks
│   └── mcp_handlers_misc.h         # Misc utilities
├── deps/
│   └── pluginsdk/         # x64dbg SDK headers
├── build/
│   └── MCPx64dbg.dp32     # Compiled plugin
├── CMakeLists.txt         # Build configuration
├── API-REFERENCE.md       # Complete API documentation
└── README.md              # This file
```

---

## 🎯 Features

✅ **Full x64dbg SDK Integration** - 43+ tools covering all major x64dbg APIs
✅ **Modular Architecture** - Clean, maintainable C++ with organized header files
✅ **Complete Debugging Toolkit** - Memory, registers, breakpoints, symbols, labels, comments
✅ **Advanced Pattern Search** - Find byte patterns, search and replace
✅ **Stack Operations** - Direct stack manipulation
✅ **Function Analysis** - Define, analyze, and manage functions
✅ **JSON Responses** - Structured data for Claude
✅ **MCP Resources & Prompts** - Contextual information and guided workflows
✅ **Cross-Process** - No DLL injection needed
✅ **Safe** - Comprehensive error handling
✅ **Fast** - Direct x64dbg SDK access

---

## 🤝 Contributing

This is a comprehensive full-featured implementation of x64dbgMCP with major improvements:

**Version 3.0 (Current)**
- ✨ **43+ MCP tools** (up from 16)
- 🏗️ **Modular C++ architecture** with organized header files
- 🔍 **Complete x64dbg SDK integration** - pattern search, symbols, labels, comments, stack, functions, bookmarks
- 📖 **Comprehensive API documentation**
- 🎯 **Production-ready** with robust error handling

**Version 2.0 (Previous)**
- 63% code reduction in C++ plugin
- Complete Python rewrite with proper MCP support
- Added resources and prompts

**Version 1.0 (Original)**
- Basic x64dbg MCP integration

---

## 📝 License

MIT License - Do whatever you want with this!

---

## 🙏 Credits

- Original idea from [Wasdubya/x64dbgMCP](https://github.com/Wasdubya/x64dbgMCP)
- Built for the x64dbg debugger
- Uses Anthropic's Model Context Protocol (MCP)

---

## 🔗 Links

- [x64dbg](https://x64dbg.com/) - The debugger
- [MCP Documentation](https://modelcontextprotocol.io/) - Protocol docs
- [Claude Desktop](https://claude.ai/download) - Get Claude

---

**Happy Reversing! 🔍**
