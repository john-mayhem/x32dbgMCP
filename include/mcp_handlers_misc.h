#ifndef MCP_HANDLERS_MISC_H
#define MCP_HANDLERS_MISC_H

#include "mcp_common.h"

namespace MCPHandlers {
namespace Misc {

//=============================================================================
// Miscellaneous Utility Endpoints
//=============================================================================

// /misc/parse_expression - Parse and evaluate expression
inline void HandleParseExpression(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    std::string expression;
    if (!GetParam(params, "expr", expression)) {
        SendResponse(client, 400, "text/plain", "Missing 'expr' parameter");
        return;
    }

    duint value = 0;
    bool success = Script::Misc::ParseExpression(expression.c_str(), &value);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success)
             << ",\"expression\":\"" << JsonEscape(expression) << "\""
             << ",\"value\":\"" << ToHex(value) << "\"}";
    SendResponse(client, 200, "application/json", response.str());
}

// /misc/resolve_label - Resolve label to address
inline void HandleResolveLabel(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    std::string label;
    if (!GetParam(params, "label", label)) {
        SendResponse(client, 400, "text/plain", "Missing 'label' parameter");
        return;
    }

    duint addr = Script::Misc::ResolveLabel(label.c_str());

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(addr != 0)
             << ",\"label\":\"" << JsonEscape(label) << "\""
             << ",\"address\":\"" << ToHex(addr) << "\"}";
    SendResponse(client, 200, "application/json", response.str());
}

// /misc/get_proc_address - Get API address in debuggee
inline void HandleGetProcAddress(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    std::string module, api;
    if (!GetParam(params, "module", module) || !GetParam(params, "api", api)) {
        SendResponse(client, 400, "text/plain", "Missing 'module' or 'api' parameter");
        return;
    }

    duint addr = Script::Misc::RemoteGetProcAddress(module.c_str(), api.c_str());

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(addr != 0)
             << ",\"module\":\"" << JsonEscape(module) << "\""
             << ",\"api\":\"" << JsonEscape(api) << "\""
             << ",\"address\":\"" << ToHex(addr) << "\"}";
    SendResponse(client, 200, "application/json", response.str());
}

} // namespace Misc
} // namespace MCPHandlers

#endif // MCP_HANDLERS_MISC_H
