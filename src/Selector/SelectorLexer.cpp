#include "SelectorLexer.hpp"

namespace
{
    bool is_space(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
    }

    // letters, digits, '-', '_' and any non-ASCII byte (UTF-8 names)
    bool is_name_char(char c)
    {
        const unsigned char u = static_cast<unsigned char>(c);
        return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') ||
               u == '-' || u == '_' || u >= 0x80;
    }
}

bool SelectorLexer::fail(std::string message)
{
    error_ = std::move(message);
    count = 0;
    return false;
}

bool SelectorLexer::query(std::string_view input)
{
    text_.assign(input.data(), input.size());
    error_.clear();
    count = 0;

    bool space = false;
    size_t i = 0;
    while (i < text_.size())
    {
        const char c = text_[i];

        if (is_space(c))
        {
            space = true;
            i++;
            continue;
        }

        if (count == MAX_TOKENS)
            return fail("selector is too long (more than 32 tokens)");

        TokenType type;
        const size_t start = i;

        if (c == '#')
            type = TokenType::HASH, i++;
        else if (c == '.')
            type = TokenType::DOT, i++;
        else if (c == '>')
            type = TokenType::GREATER_THAN, i++;
        else if (is_name_char(c))
        {
            type = TokenType::IDENT;
            while (i < text_.size() && is_name_char(text_[i]))
                i++;
        }
        else
        {
            return fail(std::string("unsupported character '") + c + "' at position " + std::to_string(i));
        }

        tokens[count++] = {type, text_.data() + start, i - start, space};
        space = false;
    }
    return true;
}

bool SelectorLexer::ok() const { return error_.empty(); }
const std::string &SelectorLexer::error() const { return error_; }
std::string_view SelectorLexer::source() const { return text_; }
const SelectorLexer::Token *SelectorLexer::get_tokens() const { return tokens; }
size_t SelectorLexer::get_count() const { return count; }
