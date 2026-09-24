#pragma once
#include <string>

// Simple HTTPS GET over WinINet.
namespace HttpUtil {

// Downloads the given URL into `out`. Returns false and fills `error` on failure.
bool GetText(const std::string& url, std::string& out, std::string& error, int timeoutMs = 30000);

} // namespace HttpUtil