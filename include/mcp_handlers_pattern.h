#ifndef MCP_HANDLERS_PATTERN_H
#define MCP_HANDLERS_PATTERN_H

#include "mcp_common.h"

namespace MCPHandlers {
namespace Pattern {

//=============================================================================
// Pattern/Memory Search Endpoints
//=============================================================================

// /pattern/find_mem - Find pattern in memory
inline void HandleFindMem(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint start, size;
    std::string pattern;

    if (!GetParamAddr(params, "start", start) || !GetParamAddr(params, "size", size) ||
        !GetParam(params, "pattern", pattern)) {
        SendResponse(client, 400, "text/plain", "Missing 'start', 'size', or 'pattern' parameter");
        return;
    }

    duint result = Script::Pattern::FindMem(start, size, pattern.c_str());

    std::ostringstream response;
    response << "{\"found\":" << BoolToJson(result != 0)
             << ",\"address\":\"" << ToHex(result) << "\"}";
    SendResponse(client, 200, "application/json", response.str());
}

// /pattern/search_replace_mem - Search and replace pattern in memory
inline void HandleSearchReplaceMem(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint start, size;
    std::string searchPattern, replacePattern;

    if (!GetParamAddr(params, "start", start) || !GetParamAddr(params, "size", size) ||
        !GetParam(params, "search", searchPattern) || !GetParam(params, "replace", replacePattern)) {
        SendResponse(client, 400, "text/plain",
            "Missing 'start', 'size', 'search', or 'replace' parameter");
        return;
    }

    bool success = Script::Pattern::SearchAndReplaceMem(start, size,
                                                        searchPattern.c_str(),
                                                        replacePattern.c_str());

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /memory/search - Search for bytes in memory
inline void HandleMemorySearch(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint start, size;
    std::string pattern;
    int maxResults = 100; // Default limit

    if (!GetParamAddr(params, "start", start) || !GetParamAddr(params, "size", size) ||
        !GetParam(params, "pattern", pattern)) {
        SendResponse(client, 400, "text/plain", "Missing 'start', 'size', or 'pattern' parameter");
        return;
    }

    GetParamInt(params, "max", maxResults);

    std::vector<duint> results;
    duint searchAddr = start;
    duint endAddr = start + size;

    while (searchAddr < endAddr && results.size() < (size_t)maxResults) {
        duint found = Script::Pattern::FindMem(searchAddr, endAddr - searchAddr, pattern.c_str());
        if (found == 0) break;

        results.push_back(found);
        searchAddr = found + 1; // Continue searching after this result
    }

    std::ostringstream response;
    response << "{\"count\":" << results.size() << ",\"results\":[";
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) response << ",";
        response << "\"" << ToHex(results[i]) << "\"";
    }
    response << "]}";

    SendResponse(client, 200, "application/json", response.str());
}

} // namespace Pattern
} // namespace MCPHandlers

#endif // MCP_HANDLERS_PATTERN_H
