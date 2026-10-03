#include "Parser.hpp"
#include <iostream>

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

void Parser::set_part(size_t k, const SelectorParser::SelectorPart *parts)
{
    current_part = k;
    needs_id = parts[k].id.length > 0;
    needs_class = parts[k].class_name.length > 0;
}

void Parser::parsring(const char *buf, size_t size, SelectorParser &selectorParser)
{
    const auto* parts = selectorParser.get_parts();

    set_part(current_part,parts);

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

                id_matched = false;
                class_matched = false;

                if (current_part == 0)
                {
                    tag_matches = true;
                }
                else
                {
                    if (parent_matched && depth == parent_depth)
                    {
                        tag_matches = true;
                    }
                    else
                    {
                        tag_matches = false;
                    }
                }
            }
            break;

        case State::TAG_OPEN:

            if (c == '/')
            {
                closing = true;
                state = State::TAG_NAME;
            }
            else
            {
                closing = false;

                if (tag_position >= parts[current_part].tag.length ||
                    c != parts[current_part].tag.ptr[tag_position])
                {
                    tag_matches = false;
                }

                tag_position++;
                state = State::TAG_NAME;
            }

            break;

        case State::TAG_NAME:
            if (c == '>' || c == ' ')
            {
                if (!closing &&
                    tag_matches &&
                    tag_position == parts[current_part].tag.length)
                {
                    tag_match_count++;
                }

                if (c == '>')
                {
                    finish_tag_set_state();
                    if (tag_matches &&
                        (!needs_id || id_matched) &&
                        (!needs_class || class_matched))
                    {
                        selector_match_count++;
                    }
                }
                else
                {
                    attribute_position = 0;
                    attribute_matches = false;
                    candidate_attribute = Attribute::UNKNOWN;
                    currect_attribute = Attribute::UNKNOWN;
                    state = State::ATTRIBUTE_NAME;
                }
            }
            else
            {
                if (!closing && tag_matches)
                {
                    if (tag_position >= parts[current_part].tag.length ||
                        c != parts[current_part].tag.ptr[tag_position])
                    {
                        tag_matches = false;
                    }
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

                if (tag_matches &&
                    (!needs_id || id_matched) &&
                    (!needs_class || class_matched))
                {
                    selector_match_count++;
                }
                
                break;
            }

            if (c == '=')
            {
                if (attribute_matches)
                {
                    currect_attribute = candidate_attribute;
                    attribute_match_count++;
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

                // potential id attribute
                if (needs_id && c == 'i')
                {
                    candidate_attribute = Attribute::ID;
                    attribute_matches = true;
                }
                else if (needs_class && c == 'c')
                {
                    candidate_attribute = Attribute::CLASSNAME;
                    attribute_matches = true;
                }
                else
                {
                    candidate_attribute = Attribute::UNKNOWN;
                    attribute_matches = false;
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
            }

            break;

        case State::ATTRIBUTE_VALUE:

            if (c == quote_char)
            {
                if (attr_value_matches)
                {
                    if (currect_attribute == Attribute::ID &&
                        value_position == parts[0].id.length)
                    {
                        id_matched = true;
                        attr_value_match_count++;
                    }
                    else if (currect_attribute == Attribute::CLASSNAME &&
                             value_position == parts[0].class_name.length)
                    {
                        class_matched = true;
                        attr_value_match_count++;
                    }
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

            if (currect_attribute == Attribute::ID)
            {
                if (value_position >= parts[0].id.length ||
                    c != parts[0].id.ptr[value_position])
                {
                    attr_value_matches = false;
                }

                value_position++;
            }
            else if (currect_attribute == Attribute::CLASSNAME)
            {
                if (value_position >= parts[0].class_name.length ||
                    c != parts[0].class_name.ptr[value_position])
                {
                    attr_value_matches = false;
                }

                value_position++;
            }

            break;
        }
    }
}
