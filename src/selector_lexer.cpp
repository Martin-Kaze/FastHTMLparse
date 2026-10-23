#include "selector_lexer.hpp"

namespace fasthtml::detail
{
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
        count_ = 0;
        return false;
    }

    bool SelectorLexer::tokenize(std::string_view input)
    {
        text_.assign(input.data(), input.size());
        error_.clear();
        count_ = 0;

        bool space = false;
        std::size_t i = 0;
        while (i < text_.size())
        {
            const char c = text_[i];

            if (is_space(c))
            {
                space = true;
                i++;
                continue;
            }

            if (count_ == max_tokens)
                return fail("selector is too long (more than 32 tokens)");

            TokenType type;
            const std::size_t start = i;

            if (c == '#')
            {
                type = TokenType::hash;
                i++;
            }
            else if (c == '.')
            {
                type = TokenType::dot;
                i++;
            }
            else if (c == '>')
            {
                type = TokenType::greater_than;
                i++;
            }
            else if (is_name_char(c))
            {
                type = TokenType::ident;
                while (i < text_.size() && is_name_char(text_[i]))
                    i++;
            }
            else
            {
                return fail(std::string("unsupported character '") + c + "' at position " + std::to_string(i));
            }

            tokens_[count_++] = {type, text_.data() + start, i - start, space};
            space = false;
        }
        return true;
    }
}
