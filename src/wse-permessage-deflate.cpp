#include "wse-permessage-deflate.h"
#include <algorithm>
#include <map>
#include <string>
#include <vector>

struct ExtensionInfo {
  std::string extension;
  std::map<std::string, std::string> params;
};

ExtensionInfo parseExtensionInfo(const std::string str) {
  std::vector<std::string> parts = split(str, ";");

  std::transform(parts.cbegin(), parts.cend(), parts.begin(), trim);

  std::string extension = parts[0];
  std::map<std::string, std::string> params;
  for (size_t i = 1; i < parts.size(); i++) {
    std::vector<std::string> param = split(parts[i], "=");
    params[trim(param[0])] = param.size() > 1 ? trim(param[1]) : "";
  }
  ExtensionInfo result;
  result.extension = extension;
  result.params = params;
  return result;
}

bool parseWindowBits(const ExtensionInfo& extInfo, const std::string& param, bool* present, int* value) {
  *present = false;
  *value = 0;

  auto match = extInfo.params.find(param);
  if (match == extInfo.params.end()) {
    // Not present--this is valid, so return true
    return true;
  }
  
  *present = true;

  if (match->second.size() == 0) {
    // Param is present, but value is not
    return true;
  }

  if (match->second.size() > 2) {
    // Too many digits
    return false;
  }
  *value = atoi(match->second.c_str());
  if (*value < 8 || *value > 15) {
    // Out of range
    return false;
  }
  return true;
}

bool parsePermessageDeflate(const ExtensionInfo& extInfo, WebSocketConnectionContext* pContext) {
  if (extInfo.extension != "permessage-deflate") {
    return false;
  }

  pContext->permessageDeflate = true;
  if (extInfo.params.find("server_no_context_takeover") != extInfo.params.end()) {
    pContext->serverNoContextTakeover = true;
  }
  if (extInfo.params.find("client_no_context_takeover") != extInfo.params.end()) {
    pContext->clientNoContextTakeover = true;
  }

  bool hasServerMaxWindowBits;
  bool hasClientMaxWindowBits;

  if (!parseWindowBits(extInfo, "server_max_window_bits", &hasServerMaxWindowBits, &pContext->serverMaxWindowBits)) {
    return false;
  }
  if (hasServerMaxWindowBits && pContext->serverMaxWindowBits == 0) {
    // If server_max_window_bits is present, the value is required
    return false;
  }

  if (!parseWindowBits(extInfo, "client_max_window_bits", &hasClientMaxWindowBits, &pContext->clientMaxWindowBits)) {
    return false;
  }

    // Set defaults
  if (pContext->serverMaxWindowBits <= 0) {
    pContext->serverMaxWindowBits = 15;
  }
  if (pContext->clientMaxWindowBits <= 0) {
    pContext->clientMaxWindowBits = 15;
  }
  
  return true;
}

bool handle(const RequestHeaders& requestHeaders,
  ResponseHeaders* pResponseHeaders,
  WebSocketConnectionContext* pContext) {

  auto swe = requestHeaders.find("sec-websocket-extensions");
  if (swe != requestHeaders.end()) {
    auto extensions = split(swe->second, ",");
    std::vector<ExtensionInfo> extInfos;
    std::transform(extensions.begin(), extensions.end(),
      std::back_inserter(extInfos),
      parseExtensionInfo);

    for (auto &extInfo : extInfos) {
      if (trim(extInfo.extension) == "permessage-deflate") {
        if (!parsePermessageDeflate(extInfo, pContext)) {
          return false;
        }
      }
    }
  }

  if (pResponseHeaders && pContext->permessageDeflate) {
    std::string params;
    if (pContext->clientNoContextTakeover) {
      params.append("; client_no_context_takeover");
    }
    if (pContext->serverNoContextTakeover) {
      params.append("; server_no_context_takeover");
    }
    if (pContext->serverMaxWindowBits != 0) {
      params.append("; server_max_window_bits=");
      params.append(std::to_string(pContext->serverMaxWindowBits));
    }
    if (pContext->clientMaxWindowBits != 0) {
      params.append("; client_max_window_bits=");
      params.append(std::to_string(pContext->clientMaxWindowBits));
    }
    std::string exts = "permessage-deflate" + params;
    pResponseHeaders->push_back(std::make_pair("Sec-WebSocket-Extensions", exts));
  }

  return true;
}

namespace permessage_deflate {

bool isValid(const RequestHeaders& requestHeaders) {
  WebSocketConnectionContext context;
  return handle(requestHeaders, NULL, &context);
}

void handshake(const RequestHeaders& requestHeaders,
  ResponseHeaders* pResponseHeaders,
  WebSocketConnectionContext* pContext) {

  handle(requestHeaders, pResponseHeaders, pContext);
}

} // namespace permessage_deflate