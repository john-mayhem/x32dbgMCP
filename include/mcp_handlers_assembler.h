#ifndef MCP_HANDLERS_ASSEMBLER_H
#define MCP_HANDLERS_ASSEMBLER_H

#include "mcp_common.h"

namespace MCPHandlers {
namespace Assembler {

//=============================================================================
// Assembler Operations
//=============================================================================

// /assembler/assemble_mem - Assemble instruction directly into memory
inline void HandleAssembleMem(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    std::string instruction;

    if (!GetParamAddr(params, "addr", addr) || !GetParam(params, "instruction", instruction)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' or 'instruction' parameter");
        return;
    }

    bool success = Script::Assembler::AssembleMem(addr, instruction.c_str());

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /assembler/assemble - Assemble instruction to bytes (without writing to memory)
inline void HandleAssemble(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    std::string instruction;

    if (!GetParamAddr(params, "addr", addr) || !GetParam(params, "instruction", instruction)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' or 'instruction' parameter");
        return;
    }

    unsigned char dest[16] = {0};
    int size = 0;
    bool success = Script::Assembler::Assemble(addr, dest, &size, instruction.c_str());

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success)
             << ",\"size\":" << size
             << ",\"bytes\":\"";

    for (int i = 0; i < size; i++) {
        response << std::hex << std::setw(2) << std::setfill('0') << (int)dest[i];
    }
    response << "\"}";

    SendResponse(client, 200, "application/json", response.str());
}

} // namespace Assembler
} // namespace MCPHandlers

#endif // MCP_HANDLERS_ASSEMBLER_H
