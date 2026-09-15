#pragma once
#include <cstddef>
#include <string>

class SelectorLexer
{
public:
 enum class TokenType
    {
        IDENT,
        HASH,
        GREATER_THAN,
        DOT,
    };

    struct Token
    {
        TokenType type;
        const char *ptr;
        size_t len;
    };

    const Token *get_tokens() const;
    size_t get_count() const;
    void query(std::string &input);

    private:
     Token tokens[32];
    size_t count = 0;
};