#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <sstream>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <iomanip>

// Include all modular handlers
#include "mcp_common.h"
#include "mcp_handlers_pattern.h"
#include "mcp_handlers_annotation.h"
#include "mcp_handlers_stack.h"
#include "mcp_handlers_function.h"
#include "mcp_handlers_misc.h"
#include "mcp_handlers_assembler.h"
#include "mcp_handlers_flags.h"

#pragma comment(lib, "ws2_32.lib")

#ifdef _WIN64
    #pragma comment(lib, "x64dbg.lib")
    #define ARCH_NAME "x64"
#else
    #pragma comment(lib, "x32dbg.lib")
    #define ARCH_NAME "x32"
#endif

// Plugin info
#define PLUGIN_NAME "x32dbg MCP Server"
#define PLUGIN_VERSION 3
#define DEFAULT_PORT 8888
#define MAX_REQUEST_SIZE 16384

// Globals
int g_pluginHandle;
HANDLE g_serverThread = NULL;
bool g_running = false;
int g_port = DEFAULT_PORT;
SOCKET g_serverSocket = INVALID_SOCKET;
std::mutex g_mutex;

// Forward declarations
DWORD WINAPI ServerThread(LPVOID lpParam);
std::string ParseRegister(const std::string& name, Script::Register::RegisterEnum& reg);

//=============================================================================
// Plugin Initialization
//=============================================================================

bool pluginInit(PLUG_INITSTRUCT* initStruct) {
    initStruct->pluginVersion = PLUGIN_VERSION;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, PLUGIN_NAME, _TRUNCATE);
    g_pluginHandle = initStruct->pluginHandle;

    _plugin_logprintf("[MCP] Plugin loading...\n");

    g_serverThread = CreateThread(NULL, 0, ServerThread, NULL, 0, NULL);
    if (g_serverThread) {
        g_running = true;
        _plugin_logprintf("[MCP] HTTP server started on port %d\n", g_port);
    } else {
        _plugin_logprintf("[MCP] Failed to start server!\n");
    }

    return true;
}

void pluginStop() {
    _plugin_logprintf("[MCP] Stopping plugin...\n");
    g_running = false;

    if (g_serverSocket != INVALID_SOCKET) {
        closesocket(g_serverSocket);
        g_serverSocket = INVALID_SOCKET;
    }

    if (g_serverThread) {
        WaitForSingleObject(g_serverThread, 2000);
        CloseHandle(g_serverThread);
        g_serverThread = NULL;
    }
}

bool pluginSetup() {
    return true;
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct) {
    return pluginInit(initStruct);
}

extern "C" __declspec(dllexport) void plugstop() {
    pluginStop();
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT* setupStruct) {
    pluginSetup();
}

//=============================================================================
// HTTP Parsing Utilities
//=============================================================================

std::string UrlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            std::istringstream is(str.substr(i + 1, 2));
            if (is >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::unordered_map<std::string, std::string> ParseQuery(const std::string& query) {
    std::unordered_map<std::string, std::string> params;
    size_t pos = 0;

    while (pos < query.length()) {
        size_t nextPos = query.find('&', pos);
        if (nextPos == std::string::npos) nextPos = query.length();

        std::string pair = query.substr(pos, nextPos - pos);
        size_t eqPos = pair.find('=');

        if (eqPos != std::string::npos) {
            std::string key = UrlDecode(pair.substr(0, eqPos));
            std::string value = UrlDecode(pair.substr(eqPos + 1));
            params[key] = value;
        }

        pos = nextPos + 1;
    }

    return params;
}

//=============================================================================
// Register Parsing (kept from original)
//=============================================================================

std::string ParseRegister(const std::string& name, Script::Register::RegisterEnum& reg) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "eax") reg = Script::Register::EAX;
    else if (lower == "ebx") reg = Script::Register::EBX;
    else if (lower == "ecx") reg = Script::Register::ECX;
    else if (lower == "edx") reg = Script::Register::EDX;
    else if (lower == "esi") reg = Script::Register::ESI;
    else if (lower == "edi") reg = Script::Register::EDI;
    else if (lower == "ebp") reg = Script::Register::EBP;
    else if (lower == "esp") reg = Script::Register::ESP;
    else if (lower == "eip") reg = Script::Register::EIP;
#ifdef _WIN64
    else if (lower == "rax") reg = Script::Register::RAX;
    else if (lower == "rbx") reg = Script::Register::RBX;
    else if (lower == "rcx") reg = Script::Register::RCX;
    else if (lower == "rdx") reg = Script::Register::RDX;
    else if (lower == "rsi") reg = Script::Register::RSI;
    else if (lower == "rdi") reg = Script::Register::RDI;
    else if (lower == "rbp") reg = Script::Register::RBP;
    else if (lower == "rsp") reg = Script::Register::RSP;
    else if (lower == "rip") reg = Script::Register::RIP;
    else if (lower == "r8") reg = Script::Register::R8;
    else if (lower == "r9") reg = Script::Register::R9;
    else if (lower == "r10") reg = Script::Register::R10;
    else if (lower == "r11") reg = Script::Register::R11;
    else if (lower == "r12") reg = Script::Register::R12;
    else if (lower == "r13") reg = Script::Register::R13;
    else if (lower == "r14") reg = Script::Register::R14;
    else if (lower == "r15") reg = Script::Register::R15;
#endif
    else return "Unknown register: " + name;

    return "";
}

//=============================================================================
// API Request Router
//=============================================================================

void HandleRequest(SOCKET client, const std::string& method, const std::string& path,
                  const std::unordered_map<std::string, std::string>& params,
                  const std::string& body) {

    try {
        std::ostringstream response;

        // ===== Core Status & Control =====
        if (path == "/status") {
            response << "{\"version\":" << PLUGIN_VERSION
                    << ",\"arch\":\"" << ARCH_NAME << "\""
                    << ",\"debugging\":" << BoolToJson(DbgIsDebugging())
                    << ",\"running\":" << BoolToJson(DbgIsRunning())
                    << "}";
            SendResponse(client, 200, "application/json", response.str());
        }
        else if (path == "/cmd") {
            auto it = params.find("cmd");
            if (it == params.end()) {
                SendResponse(client, 400, "text/plain", "Missing 'cmd' parameter");
                return;
            }

            bool success = DbgCmdExecDirect(it->second.c_str());
            response << "{\"success\":" << BoolToJson(success)
                    << ",\"command\":\"" << JsonEscape(it->second) << "\"}";
            SendResponse(client, 200, "application/json", response.str());
        }

        // ===== Register Operations =====
        else if (path == "/register/get") {
            auto it = params.find("name");
            if (it == params.end()) {
                SendResponse(client, 400, "text/plain", "Missing 'name' parameter");
                return;
            }

            Script::Register::RegisterEnum reg;
            std::string err = ParseRegister(it->second, reg);
            if (!err.empty()) {
                SendResponse(client, 400, "text/plain", err);
                return;
            }

            duint value = Script::Register::Get(reg);
            response << "{\"register\":\"" << it->second << "\",\"value\":\"" << ToHex(value) << "\"}";
            SendResponse(client, 200, "application/json", response.str());
        }
        else if (path == "/register/set") {
            auto nameIt = params.find("name");
            auto valueIt = params.find("value");
            if (nameIt == params.end() || valueIt == params.end()) {
                SendResponse(client, 400, "text/plain", "Missing 'name' or 'value' parameter");
                return;
            }

            Script::Register::RegisterEnum reg;
            std::string err = ParseRegister(nameIt->second, reg);
            if (!err.empty()) {
                SendResponse(client, 400, "text/plain", err);
                return;
            }

            duint value = std::stoull(valueIt->second, nullptr, 0);
            bool success = Script::Register::Set(reg, value);

            response << "{\"success\":" << BoolToJson(success) << "}";
            SendResponse(client, 200, "application/json", response.str());
        }

        // ===== Memory Operations =====
        else if (path == "/memory/read") {
            auto addrIt = params.find("addr");
            auto sizeIt = params.find("size");
            if (addrIt == params.end() || sizeIt == params.end()) {
                SendResponse(client, 400, "text/plain", "Missing 'addr' or 'size' parameter");
                return;
            }

            duint addr = std::stoull(addrIt->second, nullptr, 0);
            duint size = std::stoull(sizeIt->second, nullptr, 0);

            if (size > 1024 * 1024) {
                SendResponse(client, 400, "text/plain", "Size too large (max 1MB)");
                return;
            }

            std::vector<unsigned char> buffer(size);
            duint bytesRead = 0;

            if (!Script::Memory::Read(addr, buffer.data(), size, &bytesRead)) {
                SendResponse(client, 500, "text/plain", "Failed to read memory");
                return;
            }

            response << "{\"address\":\"" << ToHex(addr) << "\",\"size\":" << bytesRead << ",\"data\":\"";
            for (duint i = 0; i < bytesRead; i++) {
                response << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i];
            }
            response << "\"}";
            SendResponse(client, 200, "application/json", response.str());
        }
        else if (path == "/memory/write") {
            auto addrIt = params.find("addr");
            auto dataIt = params.find("data");
            if (addrIt == params.end() || dataIt == params.end()) {
                SendResponse(client, 400, "text/plain", "Missing 'addr' or 'data' parameter");
                return;
            }

            duint addr = std::stoull(addrIt->second, nullptr, 0);
            std::string hexData = dataIt->second;

            std::vector<unsigned char> buffer;
            for (size_t i = 0; i < hexData.length(); i += 2) {
                if (i + 1 >= hexData.length()) break;
                unsigned char byte = (unsigned char)std::stoi(hexData.substr(i, 2), nullptr, 16);
                buffer.push_back(byte);
            }

            duint bytesWritten = 0;
            bool success = Script::Memory::Write(addr, buffer.data(), buffer.size(), &bytesWritten);

            response << "{\"success\":" << BoolToJson(success)
                    << ",\"bytes_written\":" << bytesWritten << "}";
            SendResponse(client, 200, "application/json", response.str());
        }

        // ===== Pattern/Search Operations =====
        else if (path == "/pattern/find_mem") {
            MCPHandlers::Pattern::HandleFindMem(client, params);
        }
        else if (path == "/pattern/search_replace_mem") {
            MCPHandlers::Pattern::HandleSearchReplaceMem(client, params);
        }
        else if (path == "/memory/search") {
            MCPHandlers::Pattern::HandleMemorySearch(client, params);
        }

        // ===== Debug Control =====
        else if (path == "/debug/run") {
            Script::Debug::Run();
            SendResponse(client, 200, "application/json", "{\"success\":true}");
        }
        else if (path == "/debug/pause") {
            Script::Debug::Pause();
            SendResponse(client, 200, "application/json", "{\"success\":true}");
        }
        else if (path == "/debug/step") {
            Script::Debug::StepIn();
            SendResponse(client, 200, "application/json", "{\"success\":true}");
        }
        else if (path == "/debug/stepover") {
            Script::Debug::StepOver();
            SendResponse(client, 200, "application/json", "{\"success\":true}");
        }
        else if (path == "/debug/stepout") {
            Script::Debug::StepOut();
            SendResponse(client, 200, "application/json", "{\"success\":true}");
        }

        // ===== Breakpoint Operations =====
        else if (path == "/breakpoint/set") {
            auto addrIt = params.find("addr");
            if (addrIt == params.end()) {
                SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
                return;
            }

            duint addr = std::stoull(addrIt->second, nullptr, 0);
            bool success = Script::Debug::SetBreakpoint(addr);

            response << "{\"success\":" << BoolToJson(success) << "}";
            SendResponse(client, 200, "application/json", response.str());
        }
        else if (path == "/breakpoint/delete") {
            auto addrIt = params.find("addr");
            if (addrIt == params.end()) {
                SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
                return;
            }

            duint addr = std::stoull(addrIt->second, nullptr, 0);
            bool success = Script::Debug::DeleteBreakpoint(addr);

            response << "{\"success\":" << BoolToJson(success) << "}";
            SendResponse(client, 200, "application/json", response.str());
        }

        // ===== Disassembly & Modules =====
        else if (path == "/disasm") {
            auto addrIt = params.find("addr");
            if (addrIt == params.end()) {
                SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
                return;
            }

            duint addr = std::stoull(addrIt->second, nullptr, 0);
            DISASM_INSTR instr;
            DbgDisasmAt(addr, &instr);

            response << "{\"address\":\"" << ToHex(addr) << "\""
                    << ",\"instruction\":\"" << JsonEscape(instr.instruction) << "\""
                    << ",\"size\":" << instr.instr_size << "}";
            SendResponse(client, 200, "application/json", response.str());
        }
        else if (path == "/modules") {
            ListInfo moduleList;
            if (!Script::Module::GetList(&moduleList)) {
                SendResponse(client, 500, "text/plain", "Failed to get module list");
                return;
            }

            Script::Module::ModuleInfo* modules = (Script::Module::ModuleInfo*)moduleList.data;
            response << "[";
            for (size_t i = 0; i < moduleList.count; i++) {
                if (i > 0) response << ",";
                response << "{\"name\":\"" << JsonEscape(modules[i].name) << "\""
                        << ",\"base\":\"" << ToHex(modules[i].base) << "\""
                        << ",\"size\":\"" << ToHex(modules[i].size) << "\""
                        << ",\"entry\":\"" << ToHex(modules[i].entry) << "\""
                        << ",\"path\":\"" << JsonEscape(modules[i].path) << "\"}";
            }
            response << "]";
            BridgeFree(moduleList.data);

            SendResponse(client, 200, "application/json", response.str());
        }

        // ===== Symbol/Label/Comment Operations =====
        else if (path == "/symbols/list") {
            MCPHandlers::Annotation::HandleSymbolsList(client, params);
        }
        else if (path == "/label/set") {
            MCPHandlers::Annotation::HandleLabelSet(client, params);
        }
        else if (path == "/label/get") {
            MCPHandlers::Annotation::HandleLabelGet(client, params);
        }
        else if (path == "/label/delete") {
            MCPHandlers::Annotation::HandleLabelDelete(client, params);
        }
        else if (path == "/label/from_string") {
            MCPHandlers::Annotation::HandleLabelFromString(client, params);
        }
        else if (path == "/label/list") {
            MCPHandlers::Annotation::HandleLabelList(client, params);
        }
        else if (path == "/comment/set") {
            MCPHandlers::Annotation::HandleCommentSet(client, params);
        }
        else if (path == "/comment/get") {
            MCPHandlers::Annotation::HandleCommentGet(client, params);
        }
        else if (path == "/comment/delete") {
            MCPHandlers::Annotation::HandleCommentDelete(client, params);
        }
        else if (path == "/comment/list") {
            MCPHandlers::Annotation::HandleCommentList(client, params);
        }

        // ===== Stack Operations =====
        else if (path == "/stack/push") {
            MCPHandlers::Stack::HandleStackPush(client, params);
        }
        else if (path == "/stack/pop") {
            MCPHandlers::Stack::HandleStackPop(client, params);
        }
        else if (path == "/stack/peek") {
            MCPHandlers::Stack::HandleStackPeek(client, params);
        }

        // ===== Function & Bookmark Operations =====
        else if (path == "/function/add") {
            MCPHandlers::Function::HandleFunctionAdd(client, params);
        }
        else if (path == "/function/get") {
            MCPHandlers::Function::HandleFunctionGet(client, params);
        }
        else if (path == "/function/delete") {
            MCPHandlers::Function::HandleFunctionDelete(client, params);
        }
        else if (path == "/function/list") {
            MCPHandlers::Function::HandleFunctionList(client, params);
        }
        else if (path == "/bookmark/set") {
            MCPHandlers::Function::HandleBookmarkSet(client, params);
        }
        else if (path == "/bookmark/get") {
            MCPHandlers::Function::HandleBookmarkGet(client, params);
        }
        else if (path == "/bookmark/delete") {
            MCPHandlers::Function::HandleBookmarkDelete(client, params);
        }
        else if (path == "/bookmark/list") {
            MCPHandlers::Function::HandleBookmarkList(client, params);
        }

        // ===== Misc Utility Operations =====
        else if (path == "/misc/parse_expression") {
            MCPHandlers::Misc::HandleParseExpression(client, params);
        }
        else if (path == "/misc/resolve_label") {
            MCPHandlers::Misc::HandleResolveLabel(client, params);
        }
        else if (path == "/misc/get_proc_address") {
            MCPHandlers::Misc::HandleGetProcAddress(client, params);
        }

        // ===== Assembler Operations =====
        else if (path == "/assembler/assemble") {
            MCPHandlers::Assembler::HandleAssemble(client, params);
        }
        else if (path == "/assembler/assemble_mem") {
            MCPHandlers::Assembler::HandleAssembleMem(client, params);
        }

        // ===== CPU Flag Operations =====
        else if (path == "/flag/get") {
            MCPHandlers::Flags::HandleFlagGet(client, params);
        }
        else if (path == "/flag/set") {
            MCPHandlers::Flags::HandleFlagSet(client, params);
        }
        else if (path == "/flags/get_all") {
            MCPHandlers::Flags::HandleFlagsGetAll(client, params);
        }

        else {
            SendResponse(client, 404, "text/plain", "Endpoint not found");
        }
    }
    catch (const std::exception& e) {
        std::string error = std::string("{\"error\":\"") + JsonEscape(e.what()) + "\"}";
        SendResponse(client, 500, "application/json", error);
    }
}

//=============================================================================
// Server Thread
//=============================================================================

DWORD WINAPI ServerThread(LPVOID lpParam) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        _plugin_logprintf("[MCP] WSAStartup failed\n");
        return 1;
    }

    g_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_serverSocket == INVALID_SOCKET) {
        _plugin_logprintf("[MCP] Failed to create socket\n");
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddr.sin_port = htons((u_short)g_port);

    if (bind(g_serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        _plugin_logprintf("[MCP] Bind failed (port %d already in use?)\n", g_port);
        closesocket(g_serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(g_serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        _plugin_logprintf("[MCP] Listen failed\n");
        closesocket(g_serverSocket);
        WSACleanup();
        return 1;
    }

    _plugin_logprintf("[MCP] Server listening on http://127.0.0.1:%d\n", g_port);

    u_long mode = 1;
    ioctlsocket(g_serverSocket, FIONBIO, &mode);

    while (g_running) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(g_serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) {
            if (!g_running) break;
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                _plugin_logprintf("[MCP] Accept error: %d\n", WSAGetLastError());
            }
            Sleep(10);
            continue;
        }

        // Read request
        char buffer[MAX_REQUEST_SIZE];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::string request(buffer);

            // Parse HTTP request line
            size_t firstLineEnd = request.find("\r\n");
            if (firstLineEnd != std::string::npos) {
                std::string requestLine = request.substr(0, firstLineEnd);

                size_t methodEnd = requestLine.find(' ');
                size_t urlEnd = requestLine.find(' ', methodEnd + 1);
                if (methodEnd != std::string::npos && urlEnd != std::string::npos) {
                    std::string method = requestLine.substr(0, methodEnd);
                    std::string url = requestLine.substr(methodEnd + 1, urlEnd - methodEnd - 1);

                    std::string path, query;
                    size_t queryStart = url.find('?');
                    if (queryStart != std::string::npos) {
                        path = url.substr(0, queryStart);
                        query = url.substr(queryStart + 1);
                    } else {
                        path = url;
                    }

                    auto params = ParseQuery(query);

                    // Extract body
                    size_t bodyStart = request.find("\r\n\r\n");
                    std::string body = (bodyStart != std::string::npos) ? request.substr(bodyStart + 4) : "";

                    HandleRequest(clientSocket, method, path, params, body);
                }
            }
        }

        closesocket(clientSocket);
    }

    closesocket(g_serverSocket);
    g_serverSocket = INVALID_SOCKET;
    WSACleanup();

    return 0;
}
