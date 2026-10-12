#include "SelectorParser.hpp"

void SelectorParser::parsring(SelectorLexer &selectorLexer)
{

    count = 0;
id_required_mask = 0;
class_required_mask = 0;

    const auto *tokens = selectorLexer.get_tokens();
    const size_t token_count = selectorLexer.get_count();

    auto current_relation = SelectorParser::Relation::ROOT;

    size_t i = 0;

    while (i < token_count)
    {

        SelectorPart &part = parts[count++];

        part = {};

        part.relation = current_relation;

        if (tokens[i].type == SelectorLexer::TokenType::IDENT)
        {
            part.tag = {tokens[i].ptr, tokens[i].len};
            i++;
        }

        while (i < token_count)
        {

            if (tokens[i].type == SelectorLexer::TokenType::HASH)
            {
                i++;
                part.id = {tokens[i].ptr, tokens[i].len};
                i++;
            }

            else if (tokens[i].type == SelectorLexer::TokenType::DOT)
            {
                i++;
                part.class_name = {tokens[i].ptr, tokens[i].len};
                i++;
            }
            else
            {
                break;
            }
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

    for (size_t k = 0; k < count; k++)
    {
        if (parts[k].id.length > 0)
        {
            id_required_mask |= (1u << k);
        }
        if (parts[k].class_name.length > 0)
        {
            class_required_mask |= (1u << k);
        }
    }
}

const SelectorParser::SelectorPart *SelectorParser::get_parts() const
{
    return parts;
}

size_t SelectorParser::get_count() const
{
    return count;
}

size_t SelectorParser::get_id_required_mask() const
{

    return id_required_mask;
}

size_t SelectorParser::get_class_required_mask() const
{

    return class_required_mask;
}