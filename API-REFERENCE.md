# x32dbg MCP Server - Complete API Reference

This document lists all available MCP tools and HTTP endpoints provided by the x32dbg MCP Server.

## Table of Contents

- [Core Status & Control](#core-status--control)
- [Register Operations](#register-operations)
- [Memory Operations](#memory-operations)
- [Pattern/Search Operations](#patternsearch-operations)
- [Debug Control](#debug-control)
- [Breakpoint Operations](#breakpoint-operations)
- [Disassembly & Modules](#disassembly--modules)
- [Symbol Operations](#symbol-operations)
- [Label Operations](#label-operations)
- [Comment Operations](#comment-operations)
- [Stack Operations](#stack-operations)
- [Function Operations](#function-operations)
- [Bookmark Operations](#bookmark-operations)
- [Miscellaneous Utilities](#miscellaneous-utilities)

---

## Core Status & Control

### get_status
**HTTP Endpoint:** `GET /status`

Get current debugger status including architecture and process state.

**Returns:**
```json
{
  "version": 3,
  "arch": "x32" | "x64",
  "debugging": true | false,
  "running": true | false
}
```

### execute_command
**HTTP Endpoint:** `GET /cmd?cmd=<command>`

Execute a raw x32dbg command.

**Parameters:**
- `cmd` (string): Command to execute (e.g., "bp main", "disasm 0x401000")

**Returns:**
```json
{
  "success": true | false,
  "command": "command_string"
}
```

---

## Register Operations

### get_register
**HTTP Endpoint:** `GET /register/get?name=<register>`

Get value of a CPU register.

**Parameters:**
- `name` (string): Register name (e.g., "eax", "ebx", "eip", "rax", "r8")

**Returns:**
```json
{
  "register": "eax",
  "value": "0x401000"
}
```

### set_register
**HTTP Endpoint:** `GET /register/set?name=<register>&value=<value>`

Set value of a CPU register.

**Parameters:**
- `name` (string): Register name
- `value` (string): Value to set (hex format)

**Returns:**
```json
{
  "success": true | false
}
```

---

## Memory Operations

### read_memory
**HTTP Endpoint:** `GET /memory/read?addr=<address>&size=<bytes>`

Read memory from the debugged process.

**Parameters:**
- `addr` (string): Memory address in hex format (e.g., "0x401000")
- `size` (int): Number of bytes to read (max: 1MB)

**Returns:**
```json
{
  "address": "0x401000",
  "size": 16,
  "data": "4883EC28488D0D..."
}
```

### write_memory
**HTTP Endpoint:** `GET /memory/write?addr=<address>&data=<hex_data>`

Write memory to the debugged process.

**Parameters:**
- `addr` (string): Memory address in hex format
- `data` (string): Hex string to write (e.g., "90909090" for NOPs)

**Returns:**
```json
{
  "success": true | false,
  "bytes_written": 4
}
```

---

## Pattern/Search Operations

### find_pattern_in_memory
**HTTP Endpoint:** `GET /pattern/find_mem?start=<address>&size=<bytes>&pattern=<pattern>`

Search for a byte pattern in memory.

**Parameters:**
- `start` (string): Starting address in hex format
- `size` (int): Size of memory region to search
- `pattern` (string): Byte pattern to find (e.g., "48 8B 05 ?? ?? ?? ??")

**Returns:**
```json
{
  "found": true | false,
  "address": "0x401234"
}
```

### search_and_replace_pattern
**HTTP Endpoint:** `GET /pattern/search_replace_mem?start=<address>&size=<bytes>&search=<pattern>&replace=<pattern>`

Search for a pattern and replace it with another pattern.

**Parameters:**
- `start` (string): Starting address in hex format
- `size` (int): Size of memory region
- `search` (string): Pattern to search for
- `replace` (string): Pattern to replace with

**Returns:**
```json
{
  "success": true | false
}
```

### memory_search
**HTTP Endpoint:** `GET /memory/search?start=<address>&size=<bytes>&pattern=<pattern>&max=<count>`

Search for all occurrences of a pattern in memory.

**Parameters:**
- `start` (string): Starting address
- `size` (int): Size of region to search
- `pattern` (string): Byte pattern to find
- `max` (int, optional): Maximum results (default: 100)

**Returns:**
```json
{
  "count": 3,
  "results": ["0x401000", "0x401234", "0x401567"]
}
```

---

## Debug Control

### run_process
**HTTP Endpoint:** `GET /debug/run`

Resume execution of the debugged process.

### pause_process
**HTTP Endpoint:** `GET /debug/pause`

Pause execution of the debugged process.

### step_execution
**HTTP Endpoint:** `GET /debug/step`

Step into the next instruction (single step).

### step_over
**HTTP Endpoint:** `GET /debug/stepover`

Step over the next instruction (skip calls).

### step_out
**HTTP Endpoint:** `GET /debug/stepout`

Step out of current function (return to caller).

---

## Breakpoint Operations

### set_breakpoint
**HTTP Endpoint:** `GET /breakpoint/set?addr=<address>`

Set a breakpoint at specified address.

**Parameters:**
- `addr` (string): Memory address in hex format

**Returns:**
```json
{
  "success": true | false
}
```

### delete_breakpoint
**HTTP Endpoint:** `GET /breakpoint/delete?addr=<address>`

Delete a breakpoint at specified address.

**Parameters:**
- `addr` (string): Memory address in hex format

**Returns:**
```json
{
  "success": true | false
}
```

---

## Disassembly & Modules

### disassemble_at
**HTTP Endpoint:** `GET /disasm?addr=<address>`

Disassemble instruction at specified address.

**Parameters:**
- `addr` (string): Memory address in hex format

**Returns:**
```json
{
  "address": "0x401000",
  "instruction": "push ebp",
  "size": 1
}
```

### get_modules
**HTTP Endpoint:** `GET /modules`

Get list of all loaded modules in the process.

**Returns:**
```json
[
  {
    "name": "program.exe",
    "base": "0x400000",
    "size": "0x10000",
    "entry": "0x401000",
    "path": "C:\\path\\to\\program.exe"
  }
]
```

---

## Symbol Operations

### get_symbols
**HTTP Endpoint:** `GET /symbols/list`

Get all symbols (functions, imports, exports) from loaded modules.

**Returns:**
```json
[
  {
    "module": "kernel32.dll",
    "rva": "0x1234",
    "name": "GetProcAddress",
    "manual": false,
    "type": "export"
  }
]
```

---

## Label Operations

### set_label
**HTTP Endpoint:** `GET /label/set?addr=<address>&text=<label>&manual=true`

Set a label at specified address.

**Parameters:**
- `addr` (string): Memory address in hex format
- `text` (string): Label text
- `manual` (bool, optional): Whether this is a manual label (default: false)

**Returns:**
```json
{
  "success": true | false
}
```

### get_label
**HTTP Endpoint:** `GET /label/get?addr=<address>`

Get label text at specified address.

**Returns:**
```json
{
  "success": true | false,
  "text": "main_function"
}
```

### delete_label
**HTTP Endpoint:** `GET /label/delete?addr=<address>`

Delete label at specified address.

### resolve_label
**HTTP Endpoint:** `GET /label/from_string?label=<name>`

Resolve a label name to its memory address.

**Parameters:**
- `label` (string): Label name to resolve

**Returns:**
```json
{
  "success": true | false,
  "address": "0x401000"
}
```

### get_all_labels
**HTTP Endpoint:** `GET /label/list`

Get all labels in the debugged process.

**Returns:**
```json
[
  {
    "module": "program.exe",
    "rva": "0x1000",
    "text": "main_function",
    "manual": true
  }
]
```

---

## Comment Operations

### set_comment
**HTTP Endpoint:** `GET /comment/set?addr=<address>&text=<comment>&manual=true`

Set a comment at specified address.

**Parameters:**
- `addr` (string): Memory address in hex format
- `text` (string): Comment text
- `manual` (bool, optional): Whether this is a manual comment

### get_comment
**HTTP Endpoint:** `GET /comment/get?addr=<address>`

Get comment at specified address.

**Returns:**
```json
{
  "success": true | false,
  "text": "This function decrypts the payload"
}
```

### delete_comment
**HTTP Endpoint:** `GET /comment/delete?addr=<address>`

Delete comment at specified address.

### get_all_comments
**HTTP Endpoint:** `GET /comment/list`

Get all comments in the debugged process.

**Returns:**
```json
[
  {
    "module": "program.exe",
    "rva": "0x1000",
    "text": "Function comment",
    "manual": true
  }
]
```

---

## Stack Operations

### stack_push
**HTTP Endpoint:** `GET /stack/push?value=<value>`

Push a value onto the stack.

**Parameters:**
- `value` (string): Value to push in hex format

**Returns:**
```json
{
  "success": true,
  "previous_top": "0x12FF40"
}
```

### stack_pop
**HTTP Endpoint:** `GET /stack/pop`

Pop a value from the stack.

**Returns:**
```json
{
  "success": true,
  "value": "0x401000"
}
```

### stack_peek
**HTTP Endpoint:** `GET /stack/peek?offset=<offset>`

Peek at a value on the stack without removing it.

**Parameters:**
- `offset` (int, optional): Stack offset (0 = top, 1 = next, etc.)

**Returns:**
```json
{
  "success": true,
  "offset": 0,
  "value": "0x401000"
}
```

---

## Function Operations

### add_function
**HTTP Endpoint:** `GET /function/add?start=<address>&end=<address>&manual=true`

Define a function at specified address range.

**Parameters:**
- `start` (string): Function start address
- `end` (string): Function end address
- `manual` (bool, optional): Manual function definition

**Returns:**
```json
{
  "success": true | false
}
```

### get_function_info
**HTTP Endpoint:** `GET /function/get?addr=<address>`

Get function information at specified address.

**Returns:**
```json
{
  "success": true,
  "start": "0x401000",
  "end": "0x401234",
  "instruction_count": 45
}
```

### delete_function
**HTTP Endpoint:** `GET /function/delete?addr=<address>`

Delete function at specified address.

### get_all_functions
**HTTP Endpoint:** `GET /function/list`

Get all defined functions in the debugged process.

**Returns:**
```json
[
  {
    "module": "program.exe",
    "rva_start": "0x1000",
    "rva_end": "0x1234",
    "manual": true,
    "instruction_count": 45
  }
]
```

---

## Bookmark Operations

### set_bookmark
**HTTP Endpoint:** `GET /bookmark/set?addr=<address>&manual=true`

Set a bookmark at specified address.

### check_bookmark
**HTTP Endpoint:** `GET /bookmark/get?addr=<address>`

Check if a bookmark exists at specified address.

**Returns:**
```json
{
  "exists": true | false
}
```

### delete_bookmark
**HTTP Endpoint:** `GET /bookmark/delete?addr=<address>`

Delete bookmark at specified address.

### get_all_bookmarks
**HTTP Endpoint:** `GET /bookmark/list`

Get all bookmarks in the debugged process.

**Returns:**
```json
[
  {
    "module": "program.exe",
    "rva": "0x1000",
    "manual": true
  }
]
```

---

## Miscellaneous Utilities

### parse_expression
**HTTP Endpoint:** `GET /misc/parse_expression?expr=<expression>`

Parse and evaluate an expression (registers, memory, labels, etc.).

**Parameters:**
- `expr` (string): Expression to evaluate (e.g., "[esp+8]", "eax+10")

**Returns:**
```json
{
  "success": true,
  "expression": "[esp+8]",
  "value": "0x12FF40"
}
```

### resolve_api_address
**HTTP Endpoint:** `GET /misc/get_proc_address?module=<module>&api=<name>`

Get the address of an API function in the debuggee.

**Parameters:**
- `module` (string): Module name (e.g., "kernel32.dll")
- `api` (string): API function name (e.g., "GetProcAddress")

**Returns:**
```json
{
  "success": true,
  "module": "kernel32.dll",
  "api": "GetProcAddress",
  "address": "0x76C01234"
}
```

### resolve_label_address
**HTTP Endpoint:** `GET /misc/resolve_label?label=<name>`

Resolve a label name to its address.

**Parameters:**
- `label` (string): Label name to resolve

**Returns:**
```json
{
  "success": true,
  "label": "main_function",
  "address": "0x401000"
}
```

---

## Summary

**Total Tools: 43+**

- Core Status & Control: 2
- Register Operations: 2
- Memory Operations: 2
- Pattern/Search Operations: 3
- Debug Control: 5
- Breakpoint Operations: 2
- Disassembly & Modules: 2
- Symbol Operations: 1
- Label Operations: 5
- Comment Operations: 4
- Stack Operations: 3
- Function Operations: 4
- Bookmark Operations: 4
- Miscellaneous Utilities: 3

---

## Architecture

```
┌─────────────┐         MCP          ┌─────────────┐      HTTP :8888     ┌─────────────┐         x64dbg SDK       ┌─────────────┐
│   Claude    │ ◄─────────────────► │ Python MCP  │ ◄──────────────────► │  C++ Plugin │ ◄────────────────────► │   x32dbg    │
│    Code     │   stdio/TCP         │   Server    │      REST API        │  (MCPx64dbg) │   Native API Calls    │  Debugger   │
└─────────────┘                      └─────────────┘                       └─────────────┘                        └─────────────┘
```

## Building

See [BUILD.md](BUILD.md) for build instructions.

## Usage

See [README.md](README.md) for usage instructions.
