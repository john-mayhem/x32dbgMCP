#!/usr/bin/env python3
"""
x32dbg MCP Server - Model Context Protocol server for x32dbg debugger
Provides Claude with direct access to x32dbg debugging capabilities
"""

import os
import sys
from typing import Optional, Dict, Any, List
import requests
from fastmcp import FastMCP

# Initialize MCP server
mcp = FastMCP("x32dbg")

# Configuration
X64DBG_URL = os.getenv("X64DBG_URL", "http://127.0.0.1:8888")
REQUEST_TIMEOUT = 5

#=============================================================================
# HTTP Communication Layer
#=============================================================================

class DebuggerError(Exception):
    """Raised when debugger operations fail"""
    pass

def api_request(endpoint: str, params: Optional[Dict[str, str]] = None) -> Any:
    """Make HTTP request to x32dbg plugin and return parsed response"""
    try:
        url = f"{X64DBG_URL}{endpoint}"
        response = requests.get(url, params=params or {}, timeout=REQUEST_TIMEOUT)
        response.raise_for_status()

        # Try to parse as JSON first
        try:
            return response.json()
        except ValueError:
            # Return text if not JSON
            return response.text.strip()

    except requests.exceptions.Timeout:
        raise DebuggerError("Request timed out - is x32dbg running?")
    except requests.exceptions.ConnectionError:
        raise DebuggerError("Cannot connect to x32dbg - is the plugin loaded?")
    except requests.exceptions.HTTPError as e:
        raise DebuggerError(f"HTTP error {e.response.status_code}: {e.response.text}")
    except Exception as e:
        raise DebuggerError(f"Unexpected error: {str(e)}")

#=============================================================================
# MCP Resources - Contextual Information
#=============================================================================

@mcp.resource("debugger://status")
def get_debugger_status() -> str:
    """Get current debugger status and basic information"""
    try:
        status = api_request("/status")
        return f"""Debugger Status:
- Architecture: {status.get('arch', 'unknown')}
- Debugging Active: {status.get('debugging', False)}
- Process Running: {status.get('running', False)}
- Plugin Version: {status.get('version', 'unknown')}
"""
    except DebuggerError as e:
        return f"Error: {str(e)}"

@mcp.resource("debugger://modules")
def get_loaded_modules() -> str:
    """Get list of all loaded modules in the debugged process"""
    try:
        modules = api_request("/modules")
        if not modules:
            return "No modules loaded (process not running?)"

        result = f"Loaded Modules ({len(modules)}):\n\n"
        for mod in modules:
            result += f"📦 {mod['name']}\n"
            result += f"   Base: {mod['base']}\n"
            result += f"   Size: {mod['size']}\n"
            result += f"   Entry: {mod['entry']}\n"
            result += f"   Path: {mod['path']}\n\n"

        return result
    except DebuggerError as e:
        return f"Error: {str(e)}"

#=============================================================================
# MCP Prompts - Common RE Tasks
#=============================================================================

@mcp.prompt()
def analyze_function() -> str:
    """Start analyzing a function in the debugged process"""
    return """I'll help you analyze this function. Let me:
1. Check the current debugging state
2. Get the current instruction pointer (EIP/RIP)
3. Disassemble the function
4. Examine registers and stack

First, let me check the debugger status..."""

@mcp.prompt()
def find_crypto() -> str:
    """Look for cryptographic operations in the current module"""
    return """I'll search for common crypto patterns. Let me:
1. Get the current module information
2. Search for crypto constants (magic numbers)
3. Look for suspicious loops and XOR operations
4. Check for common crypto function names

Starting analysis..."""

@mcp.prompt()
def trace_execution() -> str:
    """Set up execution tracing from current location"""
    return """I'll set up execution tracing. Let me:
1. Get current location
2. Set strategic breakpoints
3. Configure step-through analysis
4. Monitor register changes

Preparing trace..."""

#=============================================================================
# MCP Tools - Debugger Operations
#=============================================================================

@mcp.tool()
def get_status() -> Dict[str, Any]:
    """Get current debugger status including architecture and process state

    Returns:
        Dictionary with debugger status information
    """
    try:
        return api_request("/status")
    except DebuggerError as e:
        return {"error": str(e), "debugging": False, "running": False}

@mcp.tool()
def execute_command(cmd: str) -> Dict[str, Any]:
    """Execute a raw x32dbg command

    Args:
        cmd: Command to execute (e.g., "bp main", "disasm 0x401000")

    Returns:
        Dictionary with execution result
    """
    try:
        return api_request("/cmd", {"cmd": cmd})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def get_register(name: str) -> Dict[str, Any]:
    """Get value of a CPU register

    Args:
        name: Register name (e.g., "eax", "ebx", "eip", "esp")

    Returns:
        Dictionary with register name and value in hex format
    """
    try:
        return api_request("/register/get", {"name": name})
    except DebuggerError as e:
        return {"error": str(e), "register": name}

@mcp.tool()
def set_register(name: str, value: str) -> Dict[str, bool]:
    """Set value of a CPU register

    Args:
        name: Register name (e.g., "eax", "ebx")
        value: Value to set (hex format, e.g., "0x1000" or decimal)

    Returns:
        Dictionary indicating success or failure
    """
    try:
        return api_request("/register/set", {"name": name, "value": value})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def read_memory(addr: str, size: int = 16) -> Dict[str, Any]:
    """Read memory from the debugged process

    Args:
        addr: Memory address in hex format (e.g., "0x401000")
        size: Number of bytes to read (default: 16, max: 1024)

    Returns:
        Dictionary with address, size, and hex data
    """
    try:
        size = min(size, 1024)  # Safety limit
        result = api_request("/memory/read", {"addr": addr, "size": str(size)})

        # Add ASCII interpretation if data is present
        if "data" in result:
            hex_data = result["data"]
            ascii_str = ""
            for i in range(0, len(hex_data), 2):
                byte = int(hex_data[i:i+2], 16)
                ascii_str += chr(byte) if 32 <= byte < 127 else "."
            result["ascii"] = ascii_str

        return result
    except DebuggerError as e:
        return {"error": str(e), "address": addr}

@mcp.tool()
def write_memory(addr: str, data: str) -> Dict[str, Any]:
    """Write memory to the debugged process

    Args:
        addr: Memory address in hex format (e.g., "0x401000")
        data: Hex string to write (e.g., "90909090" for NOPs)

    Returns:
        Dictionary indicating success and bytes written
    """
    try:
        return api_request("/memory/write", {"addr": addr, "data": data})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def step_execution() -> Dict[str, bool]:
    """Step into the next instruction (single step)

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/debug/step")
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def step_over() -> Dict[str, bool]:
    """Step over the next instruction (skip calls)

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/debug/stepover")
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def step_out() -> Dict[str, bool]:
    """Step out of current function (return to caller)

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/debug/stepout")
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def run_process() -> Dict[str, bool]:
    """Resume execution of the debugged process

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/debug/run")
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def pause_process() -> Dict[str, bool]:
    """Pause execution of the debugged process

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/debug/pause")
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def set_breakpoint(addr: str) -> Dict[str, bool]:
    """Set a breakpoint at specified address

    Args:
        addr: Memory address in hex format (e.g., "0x401000")

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/breakpoint/set", {"addr": addr})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def delete_breakpoint(addr: str) -> Dict[str, bool]:
    """Delete a breakpoint at specified address

    Args:
        addr: Memory address in hex format (e.g., "0x401000")

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/breakpoint/delete", {"addr": addr})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def disassemble_at(addr: str) -> Dict[str, Any]:
    """Disassemble instruction at specified address

    Args:
        addr: Memory address in hex format (e.g., "0x401000")

    Returns:
        Dictionary with address, instruction text, and size
    """
    try:
        return api_request("/disasm", {"addr": addr})
    except DebuggerError as e:
        return {"error": str(e), "address": addr}

@mcp.tool()
def get_modules() -> List[Dict[str, str]]:
    """Get list of all loaded modules in the process

    Returns:
        List of dictionaries containing module information (name, base, size, entry, path)
    """
    try:
        modules = api_request("/modules")
        return modules if isinstance(modules, list) else []
    except DebuggerError as e:
        return [{"error": str(e)}]

@mcp.tool()
def analyze_current_location() -> Dict[str, Any]:
    """Get comprehensive information about current debugging location

    This is a convenience tool that fetches:
    - Current EIP/RIP register
    - Current instruction disassembly
    - Debugger status

    Returns:
        Dictionary with current location details
    """
    try:
        status = api_request("/status")
        eip_reg = "eip" if status.get("arch") == "x32" else "rip"
        eip_data = api_request("/register/get", {"name": eip_reg})

        if "value" in eip_data:
            instr = api_request("/disasm", {"addr": eip_data["value"]})
            return {
                "status": status,
                "location": eip_data["value"],
                "instruction": instr.get("instruction", "unknown"),
                "instruction_size": instr.get("size", 0)
            }

        return {"error": "Could not get current location", "status": status}
    except DebuggerError as e:
        return {"error": str(e)}

#=============================================================================
# MCP Tools - Pattern/Memory Search Operations
#=============================================================================

@mcp.tool()
def find_pattern_in_memory(start_addr: str, size: int, pattern: str) -> Dict[str, Any]:
    """Search for a byte pattern in memory

    Args:
        start_addr: Starting address in hex format (e.g., "0x401000")
        size: Size of memory region to search
        pattern: Byte pattern to find (e.g., "48 8B 05 ?? ?? ?? ??")

    Returns:
        Dictionary with found address or error
    """
    try:
        return api_request("/pattern/find_mem", {
            "start": start_addr,
            "size": str(size),
            "pattern": pattern
        })
    except DebuggerError as e:
        return {"found": False, "error": str(e)}

@mcp.tool()
def search_and_replace_pattern(start_addr: str, size: int, search_pattern: str, replace_pattern: str) -> Dict[str, bool]:
    """Search for a pattern and replace it with another pattern

    Args:
        start_addr: Starting address in hex format
        size: Size of memory region to search
        search_pattern: Pattern to search for
        replace_pattern: Pattern to replace with

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/pattern/search_replace_mem", {
            "start": start_addr,
            "size": str(size),
            "search": search_pattern,
            "replace": replace_pattern
        })
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def memory_search(start_addr: str, size: int, pattern: str, max_results: int = 100) -> Dict[str, Any]:
    """Search for all occurrences of a pattern in memory

    Args:
        start_addr: Starting address in hex format
        size: Size of memory region to search
        pattern: Byte pattern to find
        max_results: Maximum number of results to return (default: 100)

    Returns:
        Dictionary with count and list of addresses found
    """
    try:
        return api_request("/memory/search", {
            "start": start_addr,
            "size": str(size),
            "pattern": pattern,
            "max": str(max_results)
        })
    except DebuggerError as e:
        return {"count": 0, "results": [], "error": str(e)}

#=============================================================================
# MCP Tools - Symbol Operations
#=============================================================================

@mcp.tool()
def get_symbols() -> List[Dict[str, Any]]:
    """Get all symbols (functions, imports, exports) from loaded modules

    Returns:
        List of dictionaries containing symbol information
    """
    try:
        symbols = api_request("/symbols/list")
        return symbols if isinstance(symbols, list) else []
    except DebuggerError as e:
        return [{"error": str(e)}]

#=============================================================================
# MCP Tools - Label Operations
#=============================================================================

@mcp.tool()
def set_label(addr: str, text: str, manual: bool = True) -> Dict[str, bool]:
    """Set a label at specified address

    Args:
        addr: Memory address in hex format (e.g., "0x401000")
        text: Label text
        manual: Whether this is a manual label (default: True)

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/label/set", {
            "addr": addr,
            "text": text,
            "manual": "true" if manual else "false"
        })
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def get_label(addr: str) -> Dict[str, Any]:
    """Get label text at specified address

    Args:
        addr: Memory address in hex format

    Returns:
        Dictionary with label text
    """
    try:
        return api_request("/label/get", {"addr": addr})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def delete_label(addr: str) -> Dict[str, bool]:
    """Delete label at specified address

    Args:
        addr: Memory address in hex format

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/label/delete", {"addr": addr})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def resolve_label(label: str) -> Dict[str, Any]:
    """Resolve a label name to its memory address

    Args:
        label: Label name to resolve

    Returns:
        Dictionary with resolved address
    """
    try:
        return api_request("/label/from_string", {"label": label})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def get_all_labels() -> List[Dict[str, str]]:
    """Get all labels in the debugged process

    Returns:
        List of dictionaries containing label information
    """
    try:
        labels = api_request("/label/list")
        return labels if isinstance(labels, list) else []
    except DebuggerError as e:
        return [{"error": str(e)}]

#=============================================================================
# MCP Tools - Comment Operations
#=============================================================================

@mcp.tool()
def set_comment(addr: str, text: str, manual: bool = True) -> Dict[str, bool]:
    """Set a comment at specified address

    Args:
        addr: Memory address in hex format
        text: Comment text
        manual: Whether this is a manual comment (default: True)

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/comment/set", {
            "addr": addr,
            "text": text,
            "manual": "true" if manual else "false"
        })
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def get_comment(addr: str) -> Dict[str, Any]:
    """Get comment at specified address

    Args:
        addr: Memory address in hex format

    Returns:
        Dictionary with comment text
    """
    try:
        return api_request("/comment/get", {"addr": addr})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def delete_comment(addr: str) -> Dict[str, bool]:
    """Delete comment at specified address

    Args:
        addr: Memory address in hex format

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/comment/delete", {"addr": addr})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def get_all_comments() -> List[Dict[str, str]]:
    """Get all comments in the debugged process

    Returns:
        List of dictionaries containing comment information
    """
    try:
        comments = api_request("/comment/list")
        return comments if isinstance(comments, list) else []
    except DebuggerError as e:
        return [{"error": str(e)}]

#=============================================================================
# MCP Tools - Stack Operations
#=============================================================================

@mcp.tool()
def stack_push(value: str) -> Dict[str, Any]:
    """Push a value onto the stack

    Args:
        value: Value to push in hex format (e.g., "0x1000")

    Returns:
        Dictionary with previous stack top value
    """
    try:
        return api_request("/stack/push", {"value": value})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def stack_pop() -> Dict[str, Any]:
    """Pop a value from the stack

    Returns:
        Dictionary with popped value
    """
    try:
        return api_request("/stack/pop")
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def stack_peek(offset: int = 0) -> Dict[str, Any]:
    """Peek at a value on the stack without removing it

    Args:
        offset: Stack offset (0 = top, 1 = next, etc.)

    Returns:
        Dictionary with peeked value
    """
    try:
        return api_request("/stack/peek", {"offset": str(offset)})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

#=============================================================================
# MCP Tools - Function Operations
#=============================================================================

@mcp.tool()
def add_function(start_addr: str, end_addr: str, manual: bool = True) -> Dict[str, bool]:
    """Define a function at specified address range

    Args:
        start_addr: Function start address in hex format
        end_addr: Function end address in hex format
        manual: Whether this is a manual function definition (default: True)

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/function/add", {
            "start": start_addr,
            "end": end_addr,
            "manual": "true" if manual else "false"
        })
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def get_function_info(addr: str) -> Dict[str, Any]:
    """Get function information at specified address

    Args:
        addr: Memory address in hex format

    Returns:
        Dictionary with function start, end, and instruction count
    """
    try:
        return api_request("/function/get", {"addr": addr})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def delete_function(addr: str) -> Dict[str, bool]:
    """Delete function at specified address

    Args:
        addr: Memory address in hex format

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/function/delete", {"addr": addr})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def get_all_functions() -> List[Dict[str, Any]]:
    """Get all defined functions in the debugged process

    Returns:
        List of dictionaries containing function information
    """
    try:
        functions = api_request("/function/list")
        return functions if isinstance(functions, list) else []
    except DebuggerError as e:
        return [{"error": str(e)}]

#=============================================================================
# MCP Tools - Bookmark Operations
#=============================================================================

@mcp.tool()
def set_bookmark(addr: str, manual: bool = True) -> Dict[str, bool]:
    """Set a bookmark at specified address

    Args:
        addr: Memory address in hex format
        manual: Whether this is a manual bookmark (default: True)

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/bookmark/set", {
            "addr": addr,
            "manual": "true" if manual else "false"
        })
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def check_bookmark(addr: str) -> Dict[str, bool]:
    """Check if a bookmark exists at specified address

    Args:
        addr: Memory address in hex format

    Returns:
        Dictionary indicating if bookmark exists
    """
    try:
        return api_request("/bookmark/get", {"addr": addr})
    except DebuggerError as e:
        return {"exists": False, "error": str(e)}

@mcp.tool()
def delete_bookmark(addr: str) -> Dict[str, bool]:
    """Delete bookmark at specified address

    Args:
        addr: Memory address in hex format

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/bookmark/delete", {"addr": addr})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def get_all_bookmarks() -> List[Dict[str, str]]:
    """Get all bookmarks in the debugged process

    Returns:
        List of dictionaries containing bookmark information
    """
    try:
        bookmarks = api_request("/bookmark/list")
        return bookmarks if isinstance(bookmarks, list) else []
    except DebuggerError as e:
        return [{"error": str(e)}]

#=============================================================================
# MCP Tools - Miscellaneous Utilities
#=============================================================================

@mcp.tool()
def parse_expression(expression: str) -> Dict[str, Any]:
    """Parse and evaluate an expression (registers, memory, labels, etc.)

    Args:
        expression: Expression to evaluate (e.g., "[esp+8]", "eax+10")

    Returns:
        Dictionary with evaluated value
    """
    try:
        return api_request("/misc/parse_expression", {"expr": expression})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def resolve_api_address(module: str, api_name: str) -> Dict[str, Any]:
    """Get the address of an API function in the debuggee

    Args:
        module: Module name (e.g., "kernel32.dll")
        api_name: API function name (e.g., "GetProcAddress")

    Returns:
        Dictionary with API address in the debuggee
    """
    try:
        return api_request("/misc/get_proc_address", {
            "module": module,
            "api": api_name
        })
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def resolve_label_address(label: str) -> Dict[str, Any]:
    """Resolve a label name to its address

    Args:
        label: Label name to resolve

    Returns:
        Dictionary with resolved address
    """
    try:
        return api_request("/misc/resolve_label", {"label": label})
    except DebuggerError as e:
        return {"success": False, "error": str(e)}
#=============================================================================
# MCP Tools - Assembler Operations
#=============================================================================

@mcp.tool()
def assemble_instruction(addr: str, instruction: str) -> Dict[str, Any]:
    """Assemble an instruction to bytecode without writing to memory

    Args:
        addr: Address context for relative instructions (hex format)
        instruction: Assembly instruction to assemble (e.g., "mov eax, ebx")

    Returns:
        Dictionary with assembled bytes
    """
    try:
        return api_request("/assembler/assemble", {
            "addr": addr,
            "instruction": instruction
        })
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def assemble_and_patch(addr: str, instruction: str) -> Dict[str, bool]:
    """Assemble an instruction and write it directly to memory

    Args:
        addr: Memory address to write to (hex format)
        instruction: Assembly instruction to assemble and write

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/assembler/assemble_mem", {
            "addr": addr,
            "instruction": instruction
        })
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

#=============================================================================
# MCP Tools - CPU Flag Operations
#=============================================================================

@mcp.tool()
def get_cpu_flag(flag: str) -> Dict[str, Any]:
    """Get the value of a CPU flag

    Args:
        flag: Flag name (ZF, OF, CF, PF, SF, TF, AF, DF, IF)

    Returns:
        Dictionary with flag name and value
    """
    try:
        return api_request("/flag/get", {"flag": flag})
    except DebuggerError as e:
        return {"error": str(e)}

@mcp.tool()
def set_cpu_flag(flag: str, value: bool) -> Dict[str, bool]:
    """Set the value of a CPU flag

    Args:
        flag: Flag name (ZF, OF, CF, PF, SF, TF, AF, DF, IF)
        value: New flag value (True/False)

    Returns:
        Dictionary indicating success
    """
    try:
        return api_request("/flag/set", {
            "flag": flag,
            "value": "true" if value else "false"
        })
    except DebuggerError as e:
        return {"success": False, "error": str(e)}

@mcp.tool()
def get_all_cpu_flags() -> Dict[str, bool]:
    """Get all CPU flags at once

    Returns:
        Dictionary with all CPU flags (ZF, OF, CF, PF, SF, TF, AF, DF, IF)
    """
    try:
        return api_request("/flags/get_all")
    except DebuggerError as e:
        return {"error": str(e)}


#=============================================================================
# Main Entry Point
#=============================================================================

if __name__ == "__main__":
    # Check if x32dbg is reachable
    try:
        status = api_request("/status")
        print(f"✅ Connected to x32dbg (arch: {status.get('arch', 'unknown')})", file=sys.stderr)
    except Exception as e:
        print(f"⚠️  Warning: Cannot connect to x32dbg: {e}", file=sys.stderr)
        print(f"   Make sure x32dbg is running with the MCP plugin loaded!", file=sys.stderr)

    # Run the MCP server
    mcp.run()
