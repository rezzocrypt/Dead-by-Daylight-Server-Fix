#include "Util.h"

#include <cctype>
#include <fstream>
#include <windows.h>

namespace util {

std::string trim(const std::string& s)
{
    size_t b = 0;
    size_t e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) b++;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) e--;
    return s.substr(b, e - b);
}

std::string toLower(const std::string& s)
{
    std::string r = s;
    for (char& c : r)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

bool containsIgnoreCase(const std::string& haystack, const std::string& needle)
{
    std::string h = toLower(haystack);
    std::string n = toLower(needle);
    return h.find(n) != std::string::npos;
}

bool endsWithIgnoreCase(const std::string& s, const std::string& suffix)
{
    if (s.size() < suffix.size()) return false;
    return toLower(s.substr(s.size() - suffix.size())) == toLower(suffix);
}

bool fileExists(const std::string& path)
{
    DWORD attr = GetFileAttributesA(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

std::vector<std::string> readAllLines(const std::string& path, bool& ok)
{
    std::vector<std::string> lines;
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open())
    {
        ok = false;
        return lines;
    }
    std::string current;
    char c;
    while (f.get(c))
    {
        if (c == '\n')
        {
            if (!current.empty() && current.back() == '\r')
                current.pop_back();
            lines.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(c);
        }
    }
    if (!current.empty())
        lines.push_back(current);
    ok = true;
    return lines;
}

bool writeAllLines(const std::string& path, const std::vector<std::string>& lines)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f.is_open())
        return false;
    for (const auto& line : lines)
    {
        f << line << "\r\n";
    }
    f.flush();
    return f.good();
}

int runHidden(const std::string& commandLine)
{
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::string cmd = commandLine; // mutable copy for CreateProcessA
    if (!CreateProcessA(nullptr, &cmd[0], nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS,
                        nullptr, nullptr, &si, &pi))
    {
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int>(exitCode);
}

} // namespace util