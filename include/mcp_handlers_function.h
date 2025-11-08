#ifndef MCP_HANDLERS_FUNCTION_H
#define MCP_HANDLERS_FUNCTION_H

#include "mcp_common.h"

namespace MCPHandlers {
namespace Function {

//=============================================================================
// Function Analysis Endpoints
//=============================================================================

// /function/add - Add/define a function
inline void HandleFunctionAdd(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint start, end;
    bool manual = false;
    int instructionCount = 0;

    if (!GetParamAddr(params, "start", start) || !GetParamAddr(params, "end", end)) {
        SendResponse(client, 400, "text/plain", "Missing 'start' or 'end' parameter");
        return;
    }

    GetParamBool(params, "manual", manual);
    GetParamInt(params, "instruction_count", instructionCount);

    bool success = Script::Function::Add(start, end, manual, (duint)instructionCount);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /function/get - Get function info at address
inline void HandleFunctionGet(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    if (!GetParamAddr(params, "addr", addr)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
        return;
    }

    duint start = 0, end = 0, instructionCount = 0;
    bool success = Script::Function::Get(addr, &start, &end, &instructionCount);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success)
             << ",\"start\":\"" << ToHex(start) << "\""
             << ",\"end\":\"" << ToHex(end) << "\""
             << ",\"instruction_count\":" << instructionCount << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /function/delete - Delete function at address
inline void HandleFunctionDelete(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    if (!GetParamAddr(params, "addr", addr)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
        return;
    }

    bool success = Script::Function::Delete(addr);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /function/list - Get all functions
inline void HandleFunctionList(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    ListInfo functionList;
    if (!Script::Function::GetList(&functionList)) {
        SendResponse(client, 500, "text/plain", "Failed to get function list");
        return;
    }

    Script::Function::FunctionInfo* functions = (Script::Function::FunctionInfo*)functionList.data;
    std::ostringstream response;
    response << "[";

    for (size_t i = 0; i < functionList.count; i++) {
        if (i > 0) response << ",";
        response << "{\"module\":\"" << JsonEscape(functions[i].mod) << "\""
                 << ",\"rva_start\":\"" << ToHex(functions[i].rvaStart) << "\""
                 << ",\"rva_end\":\"" << ToHex(functions[i].rvaEnd) << "\""
                 << ",\"manual\":" << BoolToJson(functions[i].manual)
                 << ",\"instruction_count\":" << functions[i].instructioncount << "}";
    }
    response << "]";

    BridgeFree(functionList.data);
    SendResponse(client, 200, "application/json", response.str());
}

//=============================================================================
// Bookmark Endpoints
//=============================================================================

// /bookmark/set - Set bookmark at address
inline void HandleBookmarkSet(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    bool manual = false;

    if (!GetParamAddr(params, "addr", addr)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
        return;
    }

    GetParamBool(params, "manual", manual);
    bool success = Script::Bookmark::Set(addr, manual);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /bookmark/get - Check if bookmark exists at address
inline void HandleBookmarkGet(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    if (!GetParamAddr(params, "addr", addr)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
        return;
    }

    bool exists = Script::Bookmark::Get(addr);

    std::ostringstream response;
    response << "{\"exists\":" << BoolToJson(exists) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /bookmark/delete - Delete bookmark at address
inline void HandleBookmarkDelete(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    if (!GetParamAddr(params, "addr", addr)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
        return;
    }

    bool success = Script::Bookmark::Delete(addr);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /bookmark/list - Get all bookmarks
inline void HandleBookmarkList(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    ListInfo bookmarkList;
    if (!Script::Bookmark::GetList(&bookmarkList)) {
        SendResponse(client, 500, "text/plain", "Failed to get bookmark list");
        return;
    }

    Script::Bookmark::BookmarkInfo* bookmarks = (Script::Bookmark::BookmarkInfo*)bookmarkList.data;
    std::ostringstream response;
    response << "[";

    for (size_t i = 0; i < bookmarkList.count; i++) {
        if (i > 0) response << ",";
        response << "{\"module\":\"" << JsonEscape(bookmarks[i].mod) << "\""
                 << ",\"rva\":\"" << ToHex(bookmarks[i].rva) << "\""
                 << ",\"manual\":" << BoolToJson(bookmarks[i].manual) << "}";
    }
    response << "]";

    BridgeFree(bookmarkList.data);
    SendResponse(client, 200, "application/json", response.str());
}

} // namespace Function
} // namespace MCPHandlers

#endif // MCP_HANDLERS_FUNCTION_H
