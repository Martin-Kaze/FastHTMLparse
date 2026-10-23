// Internal: splits a selector string into tokens.
#pragma once
#include <cstddef>
#include <string>
#include <string_view>

namespace fasthtml::detail
{
    class SelectorLexer
    {
    public:
        static constexpr std::size_t max_tokens = 32;

        enum class TokenType
        {
            ident,
            hash,
            greater_than,
            dot,
        };

        struct Token
        {
            TokenType type;
            const char *ptr;   // points into this lexer's own copy of the selector
            std::size_t len;
            bool space_before; // whitespace right before this token (descendant combinator)
        };

        SelectorLexer() = default;
        SelectorLexer(const SelectorLexer &) = delete; // tokens point into text_
        SelectorLexer &operator=(const SelectorLexer &) = delete;

        // Tokenizes `input` (copied, so it may change or go away afterwards). Returns false on error.
        bool tokenize(std::string_view input);

        bool ok() const { return error_.empty(); }
        const std::string &error() const { return error_; }
        std::string_view source() const { return text_; }

        const Token *tokens() const { return tokens_; }
        std::size_t token_count() const { return count_; }

    private:
        bool fail(std::string message);

        std::string text_;
        std::string error_;
        Token tokens_[max_tokens]{};
        std::size_t count_ = 0;
    };
}
