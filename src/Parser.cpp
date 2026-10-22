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

    constexpr std::string_view raw_tags[] = {"script", "style"};
    constexpr std::string_view raw_ends[] = {"</script", "</style"};

}

int Parser::current_raw_tag() const
{
    for (size_t k = 0; k < 2; ++k)
        if ((raw_tag_mask & (1u << k)) && tag_position == raw_tags[k].size())
            return (int)k;
    return -1; // -1 , means not raw
}

void Parser::set_match_callback(MatchCallback callback, void *context)
{
    on_match = callback;
    callback_context = context;
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

template <bool HasRel>
void Parser::finish_tag_set_state(const size_t id_required_mask, const size_t class_required_mask, const size_t count)
{
    int raw = closing ? -1 : current_raw_tag();
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
            if constexpr (HasRel)
            {
                match_stack[depth] = matched;
                any_stack[depth] = any_stack[depth - 1] | matched;
            }
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
    if (raw >= 0)
    {
        raw_kind = raw;
        raw_match = 0;
        state = State::RAWTEXT;
    }
    else
        state = State::DATA;
}

static inline bool is_space(char c)
{
    return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f';
}

void Parser::match_tag_name(const char *name, size_t len, const SelectorParser::SelectorPart *parts, uint32_t any_tag_mask)
{
    //  no real tag name is > 32,
    if (len > 32)
    {
        tag_match_mask &= any_tag_mask;
        void_match_mask = 0;
        raw_tag_mask = 0;
        return;
    }

    const std::string_view n(name, len);

    for (uint32_t m = tag_match_mask & ~any_tag_mask; m; m &= m - 1)
    {
        const unsigned k = std::countr_zero(m);
        const std::string_view want(parts[k].tag.ptr, parts[k].tag.length);
        if (want != n)
            tag_match_mask &= ~(1u << k);
    }

    void_match_mask = 0;
    for (size_t k = 0; k < void_tag_count; k++)
        if (void_tags[k] == n)
        {
            void_match_mask = 1u << k;
            break;
        }

    raw_tag_mask = 0;
    for (size_t k = 0; k < 2; k++)
        if (raw_tags[k] == n)
        {
            raw_tag_mask = 1u << k;
            break;
        }
}

template <bool I, bool C, bool R>
void Parser::run(const char *buf, size_t size, SelectorParser &selectorParser)
{
    const auto *parts = selectorParser.get_parts();
    size_t count = selectorParser.get_count();

    size_t id_required_mask = selectorParser.get_id_required_mask();
    size_t class_required_mask = selectorParser.get_class_required_mask();
    uint32_t child_mask = selectorParser.get_child_mask();
    uint32_t desc_mask = selectorParser.get_desc_mask();
    uint32_t any_tag_mask = selectorParser.get_any_tag_mask();

    for (size_t i = 0; i < size; i++)
    {

        #ifdef PARSER_STATS
    const int st = (int)state;
    const size_t i0 = i;
#endif
    char c = buf[i];

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
                name_len = 0;
                closing = false;
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
                tag_match_mask = 0;
                break;
            }
            if (c == '?')
            {
                state = State::MARKUP_DECL;
                break;
            }
            if ((unsigned)((c | 0x20) - 'a') > 25u) // not a letter
            {
                if (collecting)
                    current_text.push_back('<');
                state = State::DATA;
                --i;
                break;
            }
            id_mask = 0;
            class_mask = 0;

            closing = false;

            if constexpr (R)
            {
                tag_match_mask = 1u | ((match_stack[depth] << 1) & child_mask) | ((any_stack[depth] << 1) & desc_mask);
            }
            else
            {
                tag_match_mask = 1u;
            }

            state = State::TAG_NAME;
            --i;
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

            if (c == '-')
            {
                if (dashes < 2)
                    dashes++;
            }
            else if (c == '>' && dashes >= 2)
            {
                state = State::DATA;
                dashes = 0;
            }
            else
            {
                dashes = 0;
                break;
            }
            break;

        case State::MARKUP_DECL:
            if (c == '>')
                state = State::DATA;
            break;

        case State::RAWTEXT:
        {
            const char lc = (c >= 'A' && c <= 'Z') ? c + 32 : c;

            if (lc == raw_ends[raw_kind][raw_match])
            {
                if (++raw_match == raw_ends[raw_kind].size())
                {
                    raw_match = 0;
                    closing = true;
                    tag_position = 0;
                    name_len = 0;
                    tag_match_mask = 0;
                    state = State::TAG_NAME;
                }
            }
            else
                raw_match = (lc == '<') ? 1 : 0;
            break;
        }

        case State::TAG_NAME:
        {

            while (i < size && !(buf[i] == '>' || buf[i] == '/' || is_space(buf[i])))
            {
                if (name_len < 32)
                    name_buf[name_len] = (buf[i] >= 'A' && buf[i] <= 'Z') ? buf[i] + 32 : buf[i];
                name_len++;
                i++;
            }

            if (i == size)
            {
                return; // chunk ended inside the name, state stays TAG_NAME
            }

            c = buf[i];
            tag_position = name_len;

            if (!closing)
            {
                match_tag_name(name_buf, name_len, parts, any_tag_mask);
            }

            if (c == '>')
            {
                finish_tag_set_state<R>(id_required_mask, class_required_mask, count);
            }
            else
            {
                state = State::BEFORE_ATTRIBUTE_NAME;
            }

            tag_match_count += std::popcount(tag_match_mask);

            break;
        }

        case State::BEFORE_ATTRIBUTE_NAME:
        {
            while (i < size && (is_space(buf[i]) || buf[i] == '/'))
            {
                i++;
            }
            if (i == size)
            {
                return;
            }
            c = buf[i];

            if (c == '>')
            {
                finish_tag_set_state<R>(id_required_mask, class_required_mask, count);
                break;
            }

            attr_len = 0;
            state = State::ATTRIBUTE_NAME;
            --i;
            break;
        }

        case State::ATTRIBUTE_NAME:
        {
            while (i < size && !(is_space(buf[i]) || buf[i] == '/' || buf[i] == '>' || buf[i] == '='))
            {
                if (attr_len < 5)
                {
                    attr_buf[attr_len] = (buf[i] >= 'A' && buf[i] <= 'Z') ? buf[i] + 32 : buf[i];
                }
                attr_len++;
                i++;
            }
            if (i == size)
            {
                return;
            }

            currect_attribute = Attribute::UNKNOWN;

            if (attr_len == 2 && attr_buf[0] == 'i' && attr_buf[1] == 'd')
            {
                currect_attribute = Attribute::ID;
            }
            else if (attr_len == 5 && std::memcmp(attr_buf, "class", 5) == 0)
            {
                currect_attribute = Attribute::CLASSNAME;
            }

            state = State::AFTER_ATTRIBUTE_NAME;
            --i;

            break;
        }

        case State::AFTER_ATTRIBUTE_NAME:
        {
            while (i < size && is_space(buf[i]))
            {
                i++;
            }
            if (i == size)
            {
                return;
            }
            c = buf[i];

            if (c == '=')
            {
                candidate_attribute_mask = 0;
                if (currect_attribute != Attribute::UNKNOWN)
                {
                    attribute_match_count++;
                    if constexpr (I)
                    {
                        if (currect_attribute == Attribute::ID)
                            candidate_attribute_mask = tag_match_mask & id_required_mask;
                    }
                    if constexpr (C)
                    {
                        if (currect_attribute == Attribute::CLASSNAME)
                            candidate_attribute_mask = tag_match_mask & class_required_mask;
                    }
                }
                value_position = 0;
                state = State::ATTRIBUTE_VALUE_START;
                break;
            }

            if (c == '>')
            {
                finish_tag_set_state<R>(id_required_mask, class_required_mask, count);
                break;
            }

            // e.g. "disabled id=..." : previous attribute had no value, next name starts
            state = State::BEFORE_ATTRIBUTE_NAME;
            --i;
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

                if (attribute_value_mask == 0)
                {
                    const char *q = static_cast<const char *>(
                        std::memchr(buf + i + 1, c, size - i - 1));
                    if (!q)
                        return;
                    i = static_cast<size_t>(q - buf) - 1;
                }
            }
            else if (!is_space(c) && c != '>')
            {
                quote_char = 0; // 0 = unquoted
                value_position = 0;
                state = State::ATTRIBUTE_VALUE;
                attribute_value_mask = candidate_attribute_mask;
                --i;
            }
            else if (c == '>')
            {
                finish_tag_set_state<R>(id_required_mask, class_required_mask, count);
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
                for (uint32_t m = attribute_value_mask; m; m &= m - 1)
                {
                    unsigned k = std::countr_zero(m);

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
                    if constexpr (I)
                    {
                        id_mask |= attribute_value_mask;
                    }
                    break;

                case Attribute::CLASSNAME:
                    if constexpr (C)
                    {
                        class_mask |= attribute_value_mask;
                    }
                    break;

                default:
                    break;
                }

                value_position = 0;
                state = State::BEFORE_ATTRIBUTE_NAME;

                if (c == '>')
                {
                    finish_tag_set_state<R>(id_required_mask, class_required_mask, count);
                }

                break;
            }

            if (C && !unquoted && is_space(c) && currect_attribute == Attribute::CLASSNAME)
            {

                for (uint32_t m = attribute_value_mask; m; m &= m - 1)
                {
                    unsigned k = std::countr_zero(m);
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

            for (uint32_t m = attribute_value_mask; m; m &= m - 1)
            {
                unsigned k = std::countr_zero(m);

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
        #ifdef PARSER_STATS
    stat_iters[st]++;
    stat_bytes[st] += i - i0 + 1;
#endif
    }
}

void Parser::parsring(const char *b, size_t n, SelectorParser &sp)
{
    const uint8_t f = sp.get_flags();
    switch (f)
    {
    case 0:
        run<0, 0, 0>(b, n, sp);
        break;
    case 1:
        run<1, 0, 0>(b, n, sp);
        break;
    case 2:
        run<0, 1, 0>(b, n, sp);
        break;
    case 3:
        run<1, 1, 0>(b, n, sp);
        break;
    case 4:
        run<0, 0, 1>(b, n, sp);
        break;
    case 5:
        run<1, 0, 1>(b, n, sp);
        break;
    case 6:
        run<0, 1, 1>(b, n, sp);
        break;
    case 7:
        run<1, 1, 1>(b, n, sp);
        break;
    }
}