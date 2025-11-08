#!/usr/bin/env python3
"""
Test x32dbg MCP Server using actual MCP protocol
This sends proper JSON-RPC requests and validates responses
"""
import sys
import json
import subprocess
import time
import os
from pathlib import Path

def send_request(process, method, params=None, request_id=1):
    """Send a JSON-RPC request to the MCP server"""
    request = {
        "jsonrpc": "2.0",
        "id": request_id,
        "method": method,
    }
    if params is not None:
        request["params"] = params

    request_json = json.dumps(request) + "\n"
    process.stdin.write(request_json)
    process.stdin.flush()

def read_response(process, timeout=3):
    """Read a JSON-RPC response from the MCP server"""
    process.stdout.flush()
    start = time.time()

    while time.time() - start < timeout:
        line = process.stdout.readline()
        if line:
            try:
                return json.loads(line)
            except json.JSONDecodeError:
                continue

    return None

def test_mcp_server(server_path, python_cmd="python"):
    """Test the MCP server with actual protocol messages"""
    print("🔍 Testing x32dbg MCP Server (Protocol Test)")
    print(f"Server: {server_path}")
    print(f"Python: {python_cmd}")
    print()

    # Check file exists
    if not Path(server_path).exists():
        print(f"❌ Server file not found: {server_path}")
        return False

    # Start the server
    print("[1/4] Starting MCP server...")
    env = os.environ.copy()
    env["X64DBG_URL"] = "http://127.0.0.1:8888"

    try:
        process = subprocess.Popen(
            [python_cmd, server_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
            env=env
        )
    except Exception as e:
        print(f"❌ Failed to start server: {e}")
        return False

    # Give it a moment to start
    time.sleep(0.5)

    if process.poll() is not None:
        stderr = process.stderr.read()
        print(f"❌ Server exited immediately")
        print(f"Error: {stderr}")
        return False

    print("✅ Server started")
    print()

    try:
        # Test 1: Initialize
        print("[2/4] Sending initialize request...")
        send_request(process, "initialize", {
            "protocolVersion": "2024-11-05",
            "capabilities": {},
            "clientInfo": {
                "name": "test-client",
                "version": "1.0.0"
            }
        })

        response = read_response(process, timeout=5)
        if not response:
            print("❌ No response to initialize")
            return False

        if "result" not in response:
            print(f"❌ Invalid response: {response}")
            return False

        server_info = response["result"].get("serverInfo", {})
        print(f"✅ Server responded: {server_info.get('name', 'unknown')}")
        print()

        # Test 2: Send initialized notification
        print("[3/4] Sending initialized notification...")
        initialized = {
            "jsonrpc": "2.0",
            "method": "notifications/initialized",
            "params": {}
        }
        process.stdin.write(json.dumps(initialized) + "\n")
        process.stdin.flush()
        time.sleep(0.2)
        print("✅ Notification sent")
        print()

        # Test 3: List tools
        print("[4/4] Requesting tools list...")
        send_request(process, "tools/list", {}, request_id=2)

        response = read_response(process, timeout=5)
        if not response:
            print("❌ No response to tools/list")
            return False

        if "result" not in response or "tools" not in response["result"]:
            print(f"❌ Invalid tools response: {response}")
            return False

        tools = response["result"]["tools"]
        print(f"✅ Server has {len(tools)} tools registered")
        print()

        if len(tools) == 0:
            print("❌ NO TOOLS FOUND!")
            print("   This means the @mcp.tool() decorators are not working!")
            print("   The server has the OLD import bug!")
            return False

        print("Tools found:")
        for tool in tools[:5]:
            print(f"  • {tool['name']}")
        if len(tools) > 5:
            print(f"  ... and {len(tools) - 5} more")
        print()

        # Success!
        print("═" * 50)
        print("✅ ALL PROTOCOL TESTS PASSED!")
        print("═" * 50)
        print()
        print("The MCP server is working correctly!")
        print()
        print("If Claude Code still doesn't see it:")
        print("  1. Restart Claude Code COMPLETELY")
        print("  2. Check ~/.claude.json configuration")
        print("  3. Run: claude mcp list")
        print()

        return True

    finally:
        # Clean up
        process.terminate()
        try:
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Test x32dbg MCP Server")
    parser.add_argument("--server", default="C:\\Tools\\mcp_server.py",
                       help="Path to mcp_server.py")
    parser.add_argument("--python", default="python",
                       help="Python command")

    args = parser.parse_args()

    success = test_mcp_server(args.server, args.python)
    sys.exit(0 if success else 1)
