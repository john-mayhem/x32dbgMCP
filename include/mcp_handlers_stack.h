#ifndef MCP_HANDLERS_STACK_H
#define MCP_HANDLERS_STACK_H

#include "mcp_common.h"

namespace MCPHandlers {
namespace Stack {

//=============================================================================
// Stack Operation Endpoints
//=============================================================================

// /stack/push - Push value onto stack
inline void HandleStackPush(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint value;
    if (!GetParamAddr(params, "value", value)) {
        SendResponse(client, 400, "text/plain", "Missing 'value' parameter");
        return;
    }

    duint prevTop = Script::Stack::Push(value);

    std::ostringstream response;
    response << "{\"success\":true"
             << ",\"previous_top\":\"" << ToHex(prevTop) << "\"}";
    SendResponse(client, 200, "application/json", response.str());
}

// /stack/pop - Pop value from stack
inline void HandleStackPop(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint value = Script::Stack::Pop();

    std::ostringstream response;
    response << "{\"success\":true"
             << ",\"value\":\"" << ToHex(value) << "\"}";
    SendResponse(client, 200, "application/json", response.str());
}

// /stack/peek - Peek at stack value
inline void HandleStackPeek(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    int offset = 0;
    GetParamInt(params, "offset", offset);

    duint value = Script::Stack::Peek(offset);

    std::ostringstream response;
    response << "{\"success\":true"
             << ",\"offset\":" << offset
             << ",\"value\":\"" << ToHex(value) << "\"}";
    SendResponse(client, 200, "application/json", response.str());
}

} // namespace Stack
} // namespace MCPHandlers

#endif // MCP_HANDLERS_STACK_H
