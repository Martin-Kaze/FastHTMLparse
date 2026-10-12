#include "Parser.hpp"
#include <iostream>
#include <bit>

size_t Parser::get_tag_match_count() const
{
    return tag_match_count;
};

size_t Parser::get_attr_val_match_count() const
{
    return attr_value_match_count;
};

size_t Parser::get_attr_match_count() const
{
    return attribute_match_count;
};

size_t Parser::get_selector_match_count() const
{
    return selector_match_count;
}

void Parser::finish_tag_set_state()
{
    if (closing)
    {
        if (depth > 0)
            depth--;
    }
    else
    {
        depth++;
    }
    state = State::DATA;
}

void Parser::match_tag_char(const char c, const SelectorParser::SelectorPart *parts, size_t count)
{

    for (size_t k = 0; k < count; k++)
    {
        if (!(tag_match_mask & (1u << k)))
            continue;
        if (tag_position >= parts[k].tag.length ||
            c != parts[k].tag.ptr[tag_position])
        {
            tag_match_mask = tag_match_mask & ~(1u << k);
        }
    }
}



void Parser::parsring(const char *buf, size_t size, SelectorParser &selectorParser)
{
    const auto *parts = selectorParser.get_parts();
    size_t count = selectorParser.get_count();

    size_t id_required_mask  = selectorParser.get_id_required_mask();
    size_t class_required_mask = selectorParser.get_class_required_mask();

    for (size_t i = 0; i < size; i++)
    {

        char c = *(buf + i);

        switch (state)
        {

        case State::DATA:

            if (c == '<')
            {
                state = State::TAG_OPEN;
                tag_position = 0;
                closing = false;

                candidate_mask = 0;

                for (size_t k = 0; k < count; k++)
                {

                    if (k == 0)
                    {
                        candidate_mask = candidate_mask | (1 << 0);
                    }
                    else
                    {
                        parent_mask = match_stack[depth];
                        if (parent_mask & (1u << (k - 1)))
                        {
                            candidate_mask = candidate_mask | (1u << k);
                        }
                    }
                }
            }
            break;

        case State::TAG_OPEN:

            if (c == '/')
            {
                closing = true;
                state = State::TAG_NAME;

                candidate_mask = 0;
                tag_match_mask = 0;
            }
            else
            {

                id_mask = 0;
                class_mask = 0;

                closing = false;
                tag_match_mask = candidate_mask;

                match_tag_char(c, parts, count);
                tag_position++;
            }

            state = State::TAG_NAME;

            break;

        case State::TAG_NAME:

            if (c == '>' || c == ' ')
            {

                for (size_t k = 0; k < count; k++)
                {
                    if (!(tag_match_mask & (1u << k)))
                        continue;
                    if (tag_position != parts[k].tag.length)
                    {
                        tag_match_mask = tag_match_mask & ~(1u << k);
                    }
                }

                if (c == '>')
                {
                    finish_tag_set_state();
                }
                else
                {
                    attribute_position = 0;
                    attribute_matches = false;
                    candidate_attribute = Attribute::UNKNOWN;
                    currect_attribute = Attribute::UNKNOWN;
                    state = State::ATTRIBUTE_NAME;
                }

                for (size_t k = 0; k < count; k++)
                {
                    if (tag_match_mask & (1u << k))
                    {
                        tag_match_count++;
                    }
                }
            }
            else
            {
                if (!closing)
                {
                    match_tag_char(c, parts, count);
                }

                tag_position++;
            }

            break;

        case State::ATTRIBUTE_NAME:

            if (c == ' ')
            {
                break;
            }

            if (c == '>')
            {
                finish_tag_set_state();

                // used to return selector matches count

                break;
            }

            if (c == '=')
            {
                candidate_attribute_mask = 0;

                if (candidate_attribute == Attribute::ID &&
                    attribute_position != 2)
                {
                    attribute_matches = false;
                }
                else if (candidate_attribute == Attribute::CLASSNAME &&
                         attribute_position != 5)
                {
                    attribute_matches = false;
                }

                if (attribute_matches)
                {
                    currect_attribute = candidate_attribute;
                    attribute_match_count++;

                    switch (currect_attribute)
                    {
                    case Attribute::ID:

                        candidate_attribute_mask = tag_match_mask & id_required_mask;

                        break;

                    case Attribute::CLASSNAME:

                       candidate_attribute_mask = tag_match_mask & class_required_mask;

                        break;
                    }
                }
                else
                {
                    currect_attribute = Attribute::UNKNOWN;
                }

                value_position = 0;
                attr_value_matches = true;
                state = State::ATTRIBUTE_VALUE_START;

                break;
            }

            if (attribute_position == 0)
            {

                candidate_attribute = Attribute::UNKNOWN;
                attribute_matches = false;

                if (c == 'i')
                {
                    candidate_attribute = Attribute::ID;
                    attribute_matches = true;
                }
                else if (c == 'c')
                {
                    candidate_attribute = Attribute::CLASSNAME;
                    attribute_matches = true;
                }
                attribute_position++;
                break;
            }

            // verifying ID
            if (candidate_attribute == Attribute::ID)
            {

                if (attribute_position >= 2 || c != "id"[attribute_position])
                {
                    attribute_matches = false;
                }
            }

            // verifying CLASS
            else if (candidate_attribute == Attribute::CLASSNAME)
            {

                if (attribute_position >= 5 || c != "class"[attribute_position])
                {
                    attribute_matches = false;
                }
            }
            attribute_position++;

            break;

        case State::ATTRIBUTE_VALUE_START:

            if (c == '"' || c == '\'')
            {
                quote_char = c;
                value_position = 0;
                state = State::ATTRIBUTE_VALUE;
                attribute_value_mask = candidate_attribute_mask;
            }

            break;

        case State::ATTRIBUTE_VALUE:

            if (c == quote_char)
            {
                for (size_t k = 0; k < count; k++)
                {
                    if (!(attribute_value_mask & (1u << k)))
                        continue;

                    switch (currect_attribute)
                    {
                    case Attribute::ID:
                        if (value_position != parts[k].id.length)
                        {
                            attribute_value_mask &= ~(1u << k);
                        }
                        break;

                    case Attribute::CLASSNAME:
                        if (value_position != parts[k].class_name.length)
                        {
                            attribute_value_mask &= ~(1u << k);
                        }
                        break;
                    }
                }

                attr_value_match_count += std::popcount(attribute_value_mask);

                switch (currect_attribute)
                {
                case Attribute::ID:
                    id_mask |= attribute_value_mask;
                    break;

                case Attribute::CLASSNAME:
                    class_mask |= attribute_value_mask;
                    break;

                default:
                    break;
                }

                attribute_position = 0;
                value_position = 0;

                attribute_matches = false;
                attr_value_matches = true;

                candidate_attribute = Attribute::UNKNOWN;
                currect_attribute = Attribute::UNKNOWN;

                state = State::ATTRIBUTE_NAME;

                break;
            }

            for (size_t k = 0; k < count; k++)
        {
            if (!(attribute_value_mask & (1u << k)))
                continue;

            if (currect_attribute == Attribute::ID)
            {
                if (value_position >= parts[k].id.length ||
                    c != parts[k].id.ptr[value_position])
                {
                    attribute_value_mask &= ~(1u << k);
                }
            }
            else if (currect_attribute == Attribute::CLASSNAME)
            {
                if (value_position >= parts[k].class_name.length ||
                    c != parts[k].class_name.ptr[value_position])
                {
                    attribute_value_mask &= ~(1u << k);
                }
            }
        }

        value_position++;
        break;

        }

        
    }
}
