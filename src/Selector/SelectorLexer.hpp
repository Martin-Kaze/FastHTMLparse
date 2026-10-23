#pragma once
#include <cstddef>
#include <string>
#include <string_view>

class SelectorLexer
{
public:
    static constexpr size_t MAX_TOKENS = 32;

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
        const char *ptr;   // points into this lexer's own copy of the selector
        size_t len;
        bool space_before; // whitespace right before this token (descendant combinator)
    };

    SelectorLexer() = default;
    SelectorLexer(const SelectorLexer &) = delete; // tokens point into text_
    SelectorLexer &operator=(const SelectorLexer &) = delete;

    // Tokenizes `input` (copied, so it may change or die afterwards). Returns false on error.
    bool query(std::string_view input);

    bool ok() const;
    const std::string &error() const;
    std::string_view source() const; // the copied selector the tokens point into

    const Token *get_tokens() const;
    size_t get_count() const;

private:
    bool fail(std::string message);

    std::string text_;
    std::string error_;
    Token tokens[MAX_TOKENS];
    size_t count = 0;
};
