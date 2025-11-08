#ifndef MCP_HANDLERS_FLAGS_H
#define MCP_HANDLERS_FLAGS_H

#include "mcp_common.h"

namespace MCPHandlers {
namespace Flags {

//=============================================================================
// CPU Flag Operations
//=============================================================================

// Helper to parse flag name to enum
inline bool ParseFlag(const std::string& name, Script::Flag::FlagEnum& flag) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "zf") flag = Script::Flag::ZF;
    else if (lower == "of") flag = Script::Flag::OF;
    else if (lower == "cf") flag = Script::Flag::CF;
    else if (lower == "pf") flag = Script::Flag::PF;
    else if (lower == "sf") flag = Script::Flag::SF;
    else if (lower == "tf") flag = Script::Flag::TF;
    else if (lower == "af") flag = Script::Flag::AF;
    else if (lower == "df") flag = Script::Flag::DF;
    else if (lower == "if") flag = Script::Flag::IF;
    else return false;

    return true;
}

// /flag/get - Get CPU flag value
inline void HandleFlagGet(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    std::string flagName;
    if (!GetParam(params, "flag", flagName)) {
        SendResponse(client, 400, "text/plain", "Missing 'flag' parameter");
        return;
    }

    Script::Flag::FlagEnum flag;
    if (!ParseFlag(flagName, flag)) {
        SendResponse(client, 400, "text/plain", "Invalid flag name (use: ZF, OF, CF, PF, SF, TF, AF, DF, IF)");
        return;
    }

    bool value = Script::Flag::Get(flag);

    std::ostringstream response;
    response << "{\"flag\":\"" << flagName << "\""
             << ",\"value\":" << BoolToJson(value) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /flag/set - Set CPU flag value
inline void HandleFlagSet(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    std::string flagName;
    bool value = false;

    if (!GetParam(params, "flag", flagName) || !GetParamBool(params, "value", value)) {
        SendResponse(client, 400, "text/plain", "Missing 'flag' or 'value' parameter");
        return;
    }

    Script::Flag::FlagEnum flag;
    if (!ParseFlag(flagName, flag)) {
        SendResponse(client, 400, "text/plain", "Invalid flag name (use: ZF, OF, CF, PF, SF, TF, AF, DF, IF)");
        return;
    }

    bool success = Script::Flag::Set(flag, value);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /flags/get_all - Get all CPU flags at once
inline void HandleFlagsGetAll(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    std::ostringstream response;
    response << "{"
             << "\"ZF\":" << BoolToJson(Script::Flag::GetZF()) << ","
             << "\"OF\":" << BoolToJson(Script::Flag::GetOF()) << ","
             << "\"CF\":" << BoolToJson(Script::Flag::GetCF()) << ","
             << "\"PF\":" << BoolToJson(Script::Flag::GetPF()) << ","
             << "\"SF\":" << BoolToJson(Script::Flag::GetSF()) << ","
             << "\"TF\":" << BoolToJson(Script::Flag::GetTF()) << ","
             << "\"AF\":" << BoolToJson(Script::Flag::GetAF()) << ","
             << "\"DF\":" << BoolToJson(Script::Flag::GetDF()) << ","
             << "\"IF\":" << BoolToJson(Script::Flag::GetIF())
             << "}";
    SendResponse(client, 200, "application/json", response.str());
}

} // namespace Flags
} // namespace MCPHandlers

#endif // MCP_HANDLERS_FLAGS_H
