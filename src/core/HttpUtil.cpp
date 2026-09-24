#include "HttpUtil.h"

#include <windows.h>
#include <wininet.h>

namespace HttpUtil {

bool GetText(const std::string& url, std::string& out, std::string& error, int timeoutMs)
{
    out.clear();
    error.clear();

    HINTERNET hInternet = InternetOpenA("DeadByDaylightRegionFix", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet)
    {
        error = "Failed to initialize the network layer.";
        return false;
    }

    DWORD timeout = static_cast<DWORD>(timeoutMs > 0 ? timeoutMs : 30000);
    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));

    HINTERNET hRequest = InternetOpenUrlA(
        hInternet,
        url.c_str(),
        nullptr,
        0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE |
            INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_PRAGMA_NOCACHE,
        0);

    bool result = false;
    if (!hRequest)
    {
        error = "Failed to open the URL (" + std::to_string(GetLastError()) + ").";
        goto cleanup;
    }

    {
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        if (!HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                            &statusCode, &statusSize, nullptr))
        {
            error = "Could not determine the HTTP status code.";
            goto cleanup;
        }
        if (statusCode != 200)
        {
            error = "HTTP request failed with status " + std::to_string(statusCode) + ".";
            goto cleanup;
        }

        char buf[65536];
        DWORD read = 0;
        while (InternetReadFile(hRequest, buf, sizeof(buf), &read) && read > 0)
        {
            out.append(buf, read);
            read = 0;
        }
        result = !out.empty();
        if (!result)
            error = "The response body was empty.";
    }

cleanup:
    if (hRequest)
        InternetCloseHandle(hRequest);
    InternetCloseHandle(hInternet);
    return result;
}

} // namespace HttpUtil