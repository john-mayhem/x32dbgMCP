#ifndef MCP_HANDLERS_ANNOTATION_H
#define MCP_HANDLERS_ANNOTATION_H

#include "mcp_common.h"

namespace MCPHandlers {
namespace Annotation {

//=============================================================================
// Symbol Endpoints
//=============================================================================

// /symbols/list - Get all symbols (functions, imports, exports)
inline void HandleSymbolsList(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    ListInfo symbolList;
    if (!Script::Symbol::GetList(&symbolList)) {
        SendResponse(client, 500, "text/plain", "Failed to get symbol list");
        return;
    }

    Script::Symbol::SymbolInfo* symbols = (Script::Symbol::SymbolInfo*)symbolList.data;
    std::ostringstream response;
    response << "[";

    for (size_t i = 0; i < symbolList.count; i++) {
        if (i > 0) response << ",";
        response << "{\"module\":\"" << JsonEscape(symbols[i].mod) << "\""
                 << ",\"rva\":\"" << ToHex(symbols[i].rva) << "\""
                 << ",\"name\":\"" << JsonEscape(symbols[i].name) << "\""
                 << ",\"manual\":" << BoolToJson(symbols[i].manual)
                 << ",\"type\":\"";

        switch (symbols[i].type) {
            case Script::Symbol::Function: response << "function"; break;
            case Script::Symbol::Import: response << "import"; break;
            case Script::Symbol::Export: response << "export"; break;
            default: response << "unknown";
        }
        response << "\"}";
    }
    response << "]";

    BridgeFree(symbolList.data);
    SendResponse(client, 200, "application/json", response.str());
}

//=============================================================================
// Label Endpoints
//=============================================================================

// /label/set - Set a label at address
inline void HandleLabelSet(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    std::string text;
    bool manual = false;

    if (!GetParamAddr(params, "addr", addr) || !GetParam(params, "text", text)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' or 'text' parameter");
        return;
    }

    GetParamBool(params, "manual", manual);
    bool success = Script::Label::Set(addr, text.c_str(), manual, false); // false = not temporary

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /label/get - Get label at address
inline void HandleLabelGet(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    if (!GetParamAddr(params, "addr", addr)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
        return;
    }

    char text[MAX_LABEL_SIZE] = {0};
    bool success = Script::Label::Get(addr, text);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success)
             << ",\"text\":\"" << JsonEscape(text) << "\"}";
    SendResponse(client, 200, "application/json", response.str());
}

// /label/delete - Delete label at address
inline void HandleLabelDelete(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    if (!GetParamAddr(params, "addr", addr)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
        return;
    }

    bool success = Script::Label::Delete(addr);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /label/from_string - Resolve label name to address
inline void HandleLabelFromString(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    std::string label;
    if (!GetParam(params, "label", label)) {
        SendResponse(client, 400, "text/plain", "Missing 'label' parameter");
        return;
    }

    duint addr = 0;
    bool success = Script::Label::FromString(label.c_str(), &addr);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success)
             << ",\"address\":\"" << ToHex(addr) << "\"}";
    SendResponse(client, 200, "application/json", response.str());
}

// /label/list - Get all labels
inline void HandleLabelList(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    ListInfo labelList;
    if (!Script::Label::GetList(&labelList)) {
        SendResponse(client, 500, "text/plain", "Failed to get label list");
        return;
    }

    Script::Label::LabelInfo* labels = (Script::Label::LabelInfo*)labelList.data;
    std::ostringstream response;
    response << "[";

    for (size_t i = 0; i < labelList.count; i++) {
        if (i > 0) response << ",";
        response << "{\"module\":\"" << JsonEscape(labels[i].mod) << "\""
                 << ",\"rva\":\"" << ToHex(labels[i].rva) << "\""
                 << ",\"text\":\"" << JsonEscape(labels[i].text) << "\""
                 << ",\"manual\":" << BoolToJson(labels[i].manual) << "}";
    }
    response << "]";

    BridgeFree(labelList.data);
    SendResponse(client, 200, "application/json", response.str());
}

//=============================================================================
// Comment Endpoints
//=============================================================================

// /comment/set - Set comment at address
inline void HandleCommentSet(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    std::string text;
    bool manual = false;

    if (!GetParamAddr(params, "addr", addr) || !GetParam(params, "text", text)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' or 'text' parameter");
        return;
    }

    GetParamBool(params, "manual", manual);
    bool success = Script::Comment::Set(addr, text.c_str(), manual);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /comment/get - Get comment at address
inline void HandleCommentGet(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    if (!GetParamAddr(params, "addr", addr)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
        return;
    }

    char text[MAX_COMMENT_SIZE] = {0};
    bool success = Script::Comment::Get(addr, text);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success)
             << ",\"text\":\"" << JsonEscape(text) << "\"}";
    SendResponse(client, 200, "application/json", response.str());
}

// /comment/delete - Delete comment at address
inline void HandleCommentDelete(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    duint addr;
    if (!GetParamAddr(params, "addr", addr)) {
        SendResponse(client, 400, "text/plain", "Missing 'addr' parameter");
        return;
    }

    bool success = Script::Comment::Delete(addr);

    std::ostringstream response;
    response << "{\"success\":" << BoolToJson(success) << "}";
    SendResponse(client, 200, "application/json", response.str());
}

// /comment/list - Get all comments
inline void HandleCommentList(SOCKET client, const std::unordered_map<std::string, std::string>& params) {
    ListInfo commentList;
    if (!Script::Comment::GetList(&commentList)) {
        SendResponse(client, 500, "text/plain", "Failed to get comment list");
        return;
    }

    Script::Comment::CommentInfo* comments = (Script::Comment::CommentInfo*)commentList.data;
    std::ostringstream response;
    response << "[";

    for (size_t i = 0; i < commentList.count; i++) {
        if (i > 0) response << ",";
        response << "{\"module\":\"" << JsonEscape(comments[i].mod) << "\""
                 << ",\"rva\":\"" << ToHex(comments[i].rva) << "\""
                 << ",\"text\":\"" << JsonEscape(comments[i].text) << "\""
                 << ",\"manual\":" << BoolToJson(comments[i].manual) << "}";
    }
    response << "]";

    BridgeFree(commentList.data);
    SendResponse(client, 200, "application/json", response.str());
}

} // namespace Annotation
} // namespace MCPHandlers

#endif // MCP_HANDLERS_ANNOTATION_H
