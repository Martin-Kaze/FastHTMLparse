#include "Parser.hpp"
#include <iostream>
#include <bit>
#include <string_view>
#include <cstring>

namespace
{
    constexpr std::string_view void_tags[] = {
        "area", "base", "br", "col", "embed",
        "hr", "img", "input", "link", "meta",
        "source", "track", "wbr"};

    constexpr size_t void_tag_count = 13;

    constexpr uint32_t all_void_candidates =
        (uint32_t{1} << void_tag_count) - 1u;
}

void Parser::set_match_callback(MatchCallback callback, void *context)
{
    on_match = callback;
    callback_context = context;
}

void Parser::match_void_tag_char(char c)
{

    for (size_t k = 0; k < void_tag_count; ++k)
    {
        const uint32_t bit = uint32_t{1} << k;

        if ((void_match_mask & bit) == 0)
        {
            continue;
        }

        if (tag_position >= void_tags[k].size() ||
            c != void_tags[k][tag_position])
        {
            void_match_mask &= ~bit;
        }
    }
}

bool Parser::current_tag_is_void() const
{
    for (size_t k = 0; k < void_tag_count; ++k)
    {
        const uint32_t bit = uint32_t{1} << k;

        if ((void_match_mask & bit) != 0 &&
            tag_position == void_tags[k].size())
        {
            return true;
        }
    }

    return false;
}

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

void Parser::finish_tag_set_state(const size_t id_required_mask, const size_t class_required_mask, const size_t count)
{
    if (closing)
    {
        if (collecting && capture_depth == depth)
        {
            collecting = false;

            on_match(std::string_view(current_text), callback_context);

            current_text.clear();
        }

        if (depth > 0)
        {
            depth--;
        }
    }
    else
    {
        uint32_t matched = tag_match_mask & (~id_required_mask | id_mask) & (~class_required_mask | class_mask);
        const bool is_void_tag = current_tag_is_void();
        if (!is_void_tag)
        {
            if (depth + 1 < MAX_DEPTH)
            {
                depth++;
            }
            match_stack[depth] = matched;
            any_stack[depth] = any_stack[depth - 1] | matched;
        }

        if (count > 0 && (matched & (1u << (count - 1))))
        {
            selector_match_count++;

            if (on_match != nullptr)
            {
                if (is_void_tag)
                {
                    on_match(std::string_view{}, callback_context);
                }
                else
                {
                    current_text.clear();
                    capture_depth = depth;
                    collecting = true;
                }
            }
        }
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

static inline bool is_space(char c)
{
    return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f';
}

void Parser::parsring(const char *buf, size_t size, SelectorParser &selectorParser)
{
    const auto *parts = selectorParser.get_parts();
    size_t count = selectorParser.get_count();

    size_t id_required_mask = selectorParser.get_id_required_mask();
    size_t class_required_mask = selectorParser.get_class_required_mask();

    for (size_t i = 0; i < size; i++)
    {

        char c = *(buf + i);

        switch (state)
        {

        case State::DATA:
        {
            const char *text_start = buf + i;

            const char *next_tag = static_cast<const char *>(
                std::memchr(text_start, '<', size - i));

            size_t text_length = next_tag
                                     ? static_cast<size_t>(next_tag - text_start)
                                     : size - i;

            if (collecting && text_length > 0)
            {
                current_text.append(text_start, text_length);
            }

            if (next_tag == nullptr)
            {
                return;
            }

            i = static_cast<size_t>(next_tag - buf);
            c = '<';

            if (c == '<')
            {
                state = State::TAG_OPEN;
                tag_position = 0;
                void_match_mask = all_void_candidates;
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
                        const uint32_t src = (parts[k].relation == SelectorParser::Relation::CHILD)
                                                 ? match_stack[depth]
                                                 : any_stack[depth];
                        if (src & (1u << (k - 1)))
                            candidate_mask |= 1u << k;
                    }
                }
            }
            break;
        }
        case State::TAG_OPEN:
        {
            if (c == '!')
            {
                state = State::AFTER_BANG;
                bang_dashes = 0;
                break;
            }

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
                if (c >= 'A' && c <= 'Z')
                {
                    c += 'a' - 'A';
                }
                match_void_tag_char(c);
                match_tag_char(c, parts, count);

                tag_position++;
            }

            state = State::TAG_NAME;

            break;
        }

        case State::AFTER_BANG:

            if (c == '-')
            {
                if (++bang_dashes == 2)
                {
                    dashes = 0;
                    state = State::COMMENT;
                }
            }
            else if (c == '>')
            {
                state = State::DATA;
            }
            else
            {
                state = State::MARKUP_DECL;
            }

            break;

        case State::COMMENT:

            if (c == '-') {
                if (dashes < 2)
                    dashes++;
            } else if (c == '>' && dashes >= 2) {
                state = State::DATA;
                dashes = 0;
            } else {
                break;
            }

        case State::MARKUP_DECL:
            if (c == '>')
                state = State::DATA;
            break;

        case State::TAG_NAME:
        {
            if (c == '>' || c == '/' || is_space(c))
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
                    finish_tag_set_state(id_required_mask, class_required_mask, count);
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
                    if (c >= 'A' && c <= 'Z')
                    {
                        c += 'a' - 'A';
                    }
                    match_void_tag_char(c);
                    match_tag_char(c, parts, count);
                }

                tag_position++;
            }

            break;
        }
        case State::ATTRIBUTE_NAME:
        {
            if (is_space(c))
            {
                if (attribute_position > 0)
                {
                    name_ended = true;
                }

                break;
            }

            if (name_ended)
            {
                attribute_position = 0;
                name_ended = false;
            }

            if (c == '>')
            {
                finish_tag_set_state(id_required_mask, class_required_mask, count);

                break;
            }

            if (c == '=')
            {
                name_ended = false;
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

                    default:
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
        }
        case State::ATTRIBUTE_VALUE_START:
        {
            if (c == '"' || c == '\'')
            {
                quote_char = c;
                value_position = 0;
                state = State::ATTRIBUTE_VALUE;
                attribute_value_mask = candidate_attribute_mask;
            }
            else if (!is_space(c) && c != '>')
            {
                quote_char = 0; // 0 = unquoted
                value_position = 0;
                state = State::ATTRIBUTE_VALUE;
                attribute_value_mask = candidate_attribute_mask;
                --i;
            }
            break;
        }
        case State::ATTRIBUTE_VALUE:
        {

            const bool unquoted = (quote_char == 0);
            const bool value_ended = unquoted ? (is_space(c) || c == '>')
                                              : (c == quote_char);

            if (value_ended)
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

                    default:
                        break;
                    }
                }

                const uint32_t already = (currect_attribute == Attribute::ID) ? id_mask : class_mask;
                attr_value_match_count += std::popcount(attribute_value_mask & ~already);

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
                name_ended = false;

                value_position = 0;

                attribute_matches = false;
                attr_value_matches = true;

                candidate_attribute = Attribute::UNKNOWN;
                currect_attribute = Attribute::UNKNOWN;

                state = State::ATTRIBUTE_NAME;

                if (c == '>')
                {
                    finish_tag_set_state(id_required_mask, class_required_mask, count);
                }

                break;
            }

            if (!unquoted && is_space(c) && currect_attribute == Attribute::CLASSNAME)
            {

                for (size_t k = 0; k < count; k++)
                {
                    if (!(attribute_value_mask & (1u << k)))
                        continue;
                    if (value_position != parts[k].class_name.length)
                        attribute_value_mask &= ~(1u << k);
                }

                const uint32_t already = (currect_attribute == Attribute::ID) ? id_mask : class_mask;
                attr_value_match_count += std::popcount(attribute_value_mask & ~already);
                class_mask |= attribute_value_mask;

                attribute_value_mask = candidate_attribute_mask;
                value_position = 0;
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
}
