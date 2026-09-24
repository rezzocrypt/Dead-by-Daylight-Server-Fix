#pragma once
#include <string>
#include <vector>

namespace util {

std::string      trim(const std::string& s);
std::string      toLower(const std::string& s);
bool             containsIgnoreCase(const std::string& haystack, const std::string& needle);
bool             endsWithIgnoreCase(const std::string& s, const std::string& suffix);
bool             fileExists(const std::string& path);

// Splits a file into lines, handling both \r\n and \n, stripping the EOL characters.
std::vector<std::string> readAllLines(const std::string& path, bool& ok);
// Writes lines terminated by \r\n to the given path (UTF-8/ASCII).
bool             writeAllLines(const std::string& path, const std::vector<std::string>& lines);

// Runs a process hidden and waits for it to finish. Returns its exit code (-1 on failure).
int              runHidden(const std::string& commandLine);

} // namespace util