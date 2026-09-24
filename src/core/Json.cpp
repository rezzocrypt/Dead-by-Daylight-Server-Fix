#include "Json.h"

#include <cctype>

namespace js {

bool Parser::skipWs()
{
    while (i_ < s_.size())
    {
        char c = s_[i_];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            i_++;
        else
            break;
    }
    return i_ < s_.size();
}

char Parser::peek() const
{
    return i_ < s_.size() ? s_[i_] : '\0';
}

bool Parser::parseString(std::string& out)
{
    if (i_ >= s_.size() || peek() != '"')
        return false;
    i_++;
    out.clear();
    while (i_ < s_.size())
    {
        char c = s_[i_++];
        if (c == '"')
            return true;
        if (c == '\\')
        {
            if (i_ >= s_.size())
                return false;
            char e = s_[i_++];
            switch (e)
            {
                case '"':  out.push_back('"');  break;
                case '\\': out.push_back('\\'); break;
                case '/':  out.push_back('/');  break;
                case 'b':  out.push_back('\b'); break;
                case 'f':  out.push_back('\f'); break;
                case 'n':  out.push_back('\n'); break;
                case 'r':  out.push_back('\r'); break;
                case 't':  out.push_back('\t'); break;
                case 'u':  if (i_ + 4 > s_.size()) return false; i_ += 4; out.push_back('?'); break;
                default:   return false;
            }
        }
        else
        {
            out.push_back(c);
        }
    }
    return false;
}

void Parser::skipValue()
{
    if (i_ >= s_.size())
        return;
    char c = peek();
    if (c == '"')
    {
        std::string dummy;
        parseString(dummy);
        return;
    }
    if (c == '{')
    {
        i_++;
        skipWs();
        if (peek() == '}')
        {
            i_++;
            return;
        }
        for (;;)
        {
            std::string key;
            if (!parseString(key))
                return;
            skipWs();
            if (peek() != ':')
                return;
            i_++;
            skipValue();
            skipWs();
            if (peek() == ',')
            {
                i_++;
                skipWs();
                continue;
            }
            if (peek() == '}')
            {
                i_++;
                return;
            }
            return;
        }
    }
    if (c == '[')
    {
        i_++;
        skipWs();
        if (peek() == ']')
        {
            i_++;
            return;
        }
        for (;;)
        {
            skipValue();
            skipWs();
            if (peek() == ',')
            {
                i_++;
                skipWs();
                continue;
            }
            if (peek() == ']')
            {
                i_++;
                return;
            }
            return;
        }
    }
    // Primitive (number/true/false/null) - consume until a value delimiter.
    while (i_ < s_.size())
    {
        char d = s_[i_];
        if (d == ',' || d == '}' || d == ']' || d == ' ' || d == '\t' || d == '\n' || d == '\r' || d == '"')
            break;
        i_++;
    }
}

bool Parser::findKey(const std::string& key, std::string::size_type& valueStart)
{
    if (!skipWs() || peek() != '{')
        return false;
    i_++;
    for (;;)
    {
        skipWs();
        if (peek() == '}')
        {
            i_++;
            return false;
        }
        std::string k;
        if (!parseString(k))
            return false;
        skipWs();
        if (peek() != ':')
            return false;
        i_++;
        valueStart = i_;
        if (k == key)
            return true;
        skipValue();
        skipWs();
        if (peek() == ',')
        {
            i_++;
            continue;
        }
        if (peek() == '}')
        {
            i_++;
            return false;
        }
        return false;
    }
}

bool Parser::readStringMember(const std::string& key, std::string& out)
{
    std::string::size_type start = 0;
    if (!findKey(key, start))
        return false;
    i_ = start;
    return parseString(out);
}

bool Parser::forEachObjectInArray(const std::function<bool()>& body)
{
    if (!skipWs() || peek() != '[')
        return false;
    i_++;
    for (;;)
    {
        skipWs();
        if (peek() == ']')
        {
            i_++;
            return true;
        }
        if (peek() != '{')
            return false;
        i_++; // consume '{'; the body sees the members of the object
        if (!body())
            return false;
        skipWs();
        char c = peek();
        if (c == ',')
        {
            i_++;
            continue;
        }
        if (c == ']')
        {
            i_++;
            return true;
        }
        return false;
    }
}

bool Parser::atObjectEnd() const
{
    std::string::size_type j = i_;
    while (j < s_.size())
    {
        char c = s_[j];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            j++;
            continue;
        }
        return c == '}';
    }
    return true;
}

void Parser::eatObjectEnd()
{
    skipWs();
    if (peek() == '}')
        i_++;
}

bool Parser::nextMemberKey(std::string& key)
{
    if (peek() == ',')
        i_++; // consume the comma after the previous member's value
    skipWs();
    if (peek() == '}')
        return false;
    if (!parseString(key))
        return false;
    skipWs();
    if (peek() != ':')
        return false;
    i_++;
    skipWs(); // position at the start of the value (may be after whitespace)
    return true;
}

} // namespace js