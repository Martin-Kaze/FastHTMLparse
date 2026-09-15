#include "SelectorParser.hpp"

void SelectorParser::parsring(SelectorLexer &selectorLexer)
{

    const auto *tokens = selectorLexer.get_tokens();
    const size_t token_count = selectorLexer.get_count();

    auto current_relation = SelectorParser::Relation::ROOT;

    size_t i = 0;

    while (i < token_count)
    {

        SelectorPart &part = parts[count++];

        part.relation = current_relation;

        if (tokens[i].type == SelectorLexer::TokenType::IDENT)
        {
            part.tag = std::string(tokens[i].ptr, tokens[i].len);
            i++;
        }

        if (i < token_count && tokens[i].type == SelectorLexer::TokenType::HASH)
        {
            i++;
            part.id = std::string(tokens[i].ptr, tokens[i].len);
            i++;
        }

        if (i < token_count && tokens[i].type == SelectorLexer::TokenType::DOT)
        {
            i++;
            part.class_name = std::string(tokens[i].ptr, tokens[i].len);
            i++;
        }

        if (i < token_count &&
            tokens[i].type == SelectorLexer::TokenType::GREATER_THAN)
        {
            current_relation = SelectorParser::Relation::CHILD;
            i++;
        }
        else if (i < token_count &&
                 tokens[i].type == SelectorLexer::TokenType::IDENT)
        {
            current_relation = SelectorParser::Relation::DESCENDANT;
        }
    }


}

const SelectorParser::SelectorPart* SelectorParser::get_parts() const
{
    return parts;
}

size_t SelectorParser::get_count() const
{
    return count;
}