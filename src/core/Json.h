#pragma once
#include <functional>
#include <string>

namespace js {

// Lightweight JSON reader for the two documents the app needs:
//  - regions.json (array of objects with string members)
//  - AWS ip-ranges.json (large object containing a "prefixes" array)
// It is intentionally minimal: no DOM, no numbers, no nested arrays of strings.
class Parser
{
public:
    explicit Parser(const std::string& text) : s_(text) {}

    // Current position must be at the start of an object ('{').
    // Returns the string value of the given member if present.
    bool readStringMember(const std::string& key, std::string& out);

    // Current position must be at the start of an object ('{').
    // If the object contains `key`, positions at the start of its value.
    bool findKey(const std::string& key, std::string::size_type& valueStart);

    // Current position must be at the start of an array ('[').
    // For each element object { ... } invokes body() while positioned at its '{'.
    // body() is responsible for consuming members up to and including the closing '}'.
    bool forEachObjectInArray(const std::function<bool()>& body);

    // Helpers used when iterating members of an object.
    bool atObjectEnd() const;
    void eatObjectEnd();
    // Expects the next token to be a member key followed by ':'. On success the
    // position is left right after the ':'. Returns false if the object ended.
    bool nextMemberKey(std::string& key);
    // Parses a JSON string (handles \" \\ \/ \b \f \n \r \t and skips \uXXXX).
    bool parseString(std::string& out);
    // Skips over a whole value (object/array/string/primitive) at the current position.
    void skipValue();

private:
    bool skipWs();
    char peek() const;

    const std::string& s_;
    std::string::size_type i_ = 0;
};

} // namespace js