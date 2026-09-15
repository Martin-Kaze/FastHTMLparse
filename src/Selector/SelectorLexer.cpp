#include "SelectorLexer.hpp"

void SelectorLexer::query(std::string &input)
{

    size_t i = 0;

    while (i < input.size())
    {

        char c = input[i];

        switch (c)
        {
        case '#':
            tokens[count++] = {TokenType::HASH, &input[i], 1};
            i++;
            break;

        case '>':
            tokens[count++] = {TokenType::GREATER_THAN, &input[i], 1};
            i++;
            break;

        case '.':
            tokens[count++] = {TokenType::DOT, &input[i], 1};
            i++;
            break;

        case ' ':
            i++;
            break;

        default:

            size_t start = i;

            while (i < input.size() &&
                   input[i] != ' ' &&
                   input[i] != '#' &&
                   input[i] != '.' &&
                   input[i] != '>')
            {
                i++;
            }

            tokens[count++] = {
                TokenType::IDENT,
                &input[start],
                i - start};

            break;
        }
    }
}

const SelectorLexer::Token *SelectorLexer::get_tokens() const
{
    return tokens;
}

size_t SelectorLexer::get_count() const
{
    return count;
}