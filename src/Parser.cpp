#include "Parser.hpp"
#include <bit>
#include <cstring>
#include <string_view>

namespace
{
    constexpr std::string_view void_tags[] = {
        "area", "base", "br", "col", "embed",
        "hr", "img", "input", "link", "meta",
        "source", "track", "wbr"};

    constexpr std::string_view raw_tags[] = {"script", "style"};
    constexpr std::string_view raw_ends[] = {"</script", "</style"};

    inline bool is_space(char c)
    {
        return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f';
    }

    inline bool is_lower_letter(char c)
    {
        return static_cast<unsigned>(c - 'a') < 26u;
    }

    // Packs a lowercase tag name into one integer: first 7 bytes + length in the top byte.
    // Exact for names up to 7 chars; longer names only collide if they share 7 chars and length.
    constexpr uint64_t make_key(const char *name, size_t len)
    {
        uint64_t k = 0;
        const size_t n = len < 7 ? len : 7;
        for (size_t j = 0; j < n; ++j)
            k |= uint64_t(static_cast<unsigned char>(name[j])) << (8 * j);
        return k | (uint64_t(len < 255 ? len : 255) << 56);
    }

    constexpr uint64_t key_of(std::string_view s) { return make_key(s.data(), s.size()); }

    // <p><p>, <li><li>, ... : opening one of these closes an open one of the same name on top.
    inline bool closes_same_tag(uint64_t key)
    {
        constexpr uint64_t keys[] = {key_of("p"), key_of("li"), key_of("dt"), key_of("dd"),
                                     key_of("option"), key_of("tr"), key_of("td"), key_of("th")};
        for (uint64_t k : keys)
            if (k == key)
                return true;
        return false;
    }
}

void Parser::set_match_callback(MatchCallback callback, void *context)
{
    on_match = callback;
    callback_context = context;
}

size_t Parser::get_tag_match_count() const { return tag_match_count; }
size_t Parser::get_attr_val_match_count() const { return attr_value_match_count; }
size_t Parser::get_attr_match_count() const { return attribute_match_count; }
size_t Parser::get_selector_match_count() const { return selector_match_count; }

void Parser::close_to(size_t d)
{
    while (capture_count > 0 && captures[capture_count - 1].depth >= d)
    {
        const Capture &cap = captures[--capture_count];
        on_match(std::string_view(current_text).substr(cap.start), callback_context);
    }
    if (capture_count == 0)
        current_text.clear();
    depth = d - 1;
}

void Parser::finish()
{
    close_to(1); // emits every open capture, innermost first
    untracked = 0;
    state = State::DATA;
    raw_match = 0;
    closing = false;
}

void Parser::reset()
{
    const MatchCallback cb = on_match;
    void *const ctx = callback_context;
    *this = Parser();
    on_match = cb;
    callback_context = ctx;
}

template <bool HasRel>
void Parser::finish_tag_set_state(const uint32_t id_required_mask, const uint32_t class_required_mask, const size_t count)
{
    if (closing)
    {
        if (untracked > 0)
        {
            --untracked; // closes an element nested deeper than MAX_DEPTH
        }
        else
        {
            // close the nearest open element with this name; anything above it was left unclosed
            const size_t stop = depth > CLOSE_SEARCH_LIMIT ? depth - CLOSE_SEARCH_LIMIT : 0;
            for (size_t d = depth; d > stop; --d)
            {
                if (key_stack[d] == tag_key)
                {
                    close_to(d);
                    break;
                }
            }
            // not found: stray end tag, ignored
        }
        state = State::DATA;
        return;
    }

    const uint32_t matched = tag_match_mask & (~id_required_mask | id_mask) & (~class_required_mask | class_mask);

    bool tracked = false;
    if (!tag_is_void)
    {
        if (untracked == 0 && depth + 1 < MAX_DEPTH)
        {
            ++depth;
            key_stack[depth] = tag_key;
            if constexpr (HasRel)
            {
                match_stack[depth] = matched;
                any_stack[depth] = any_stack[depth - 1] | matched;
            }
            tracked = true;
        }
        else
        {
            ++untracked;
        }
    }

    if (count > 0 && (matched & (1u << (count - 1))))
    {
        selector_match_count++;

        if (on_match != nullptr)
        {
            if (tracked)
                captures[capture_count++] = {depth, current_text.size()};
            else
                on_match(std::string_view{}, callback_context); // void element (or nested too deep): no text
        }
    }

    if (tag_raw_kind >= 0)
    {
        raw_kind = tag_raw_kind;
        raw_match = 0;
        state = State::RAWTEXT;
    }
    else
    {
        state = State::DATA;
    }
}

void Parser::match_tag_name(const char *name, size_t len, const SelectorParser::SelectorPart *parts, uint32_t any_tag_mask)
{
    tag_is_void = false;
    tag_raw_kind = -1;

    // no real tag name is longer than 32
    if (len > 32)
    {
        tag_match_mask &= any_tag_mask;
        return;
    }

    const std::string_view n(name, len);

    for (uint32_t m = tag_match_mask & ~any_tag_mask; m; m &= m - 1)
    {
        const unsigned k = std::countr_zero(m);
        if (std::string_view(parts[k].tag.ptr, parts[k].tag.length) != n)
            tag_match_mask &= ~(1u << k);
    }

    // void tags are 2..6 chars, raw tags 5..6
    if (len >= 2 && len <= 6)
    {
        for (std::string_view v : void_tags)
        {
            if (v == n)
            {
                tag_is_void = true;
                return;
            }
        }
        for (int k = 0; k < 2; k++)
        {
            if (raw_tags[k] == n)
            {
                tag_raw_kind = k;
                return;
            }
        }
    }
}

template <bool I, bool C, bool R>
void Parser::run(const char *buf, size_t size, SelectorParser &selectorParser)
{
    const auto *parts = selectorParser.get_parts();
    const size_t count = selectorParser.get_count();

    const uint32_t id_required_mask = static_cast<uint32_t>(selectorParser.get_id_required_mask());
    const uint32_t class_required_mask = static_cast<uint32_t>(selectorParser.get_class_required_mask());
    const uint32_t child_mask = static_cast<uint32_t>(selectorParser.get_child_mask());
    const uint32_t desc_mask = static_cast<uint32_t>(selectorParser.get_desc_mask());
    const uint32_t any_tag_mask = static_cast<uint32_t>(selectorParser.get_any_tag_mask());

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
            const char *next_tag = static_cast<const char *>(std::memchr(text_start, '<', size - i));
            const size_t text_length = next_tag ? static_cast<size_t>(next_tag - text_start) : size - i;

            if (capture_count > 0 && text_length > 0)
                current_text.append(text_start, text_length);

            if (next_tag == nullptr)
                return;

            i = static_cast<size_t>(next_tag - buf) + 1; // past the '<'
            name_len = 0;
            closing = false;

            if (i == size)
            {
                state = State::TAG_OPEN;
                return;
            }
        }
            [[fallthrough]];
        case State::TAG_OPEN:
        {
            c = buf[i];
            if (c == '!')
            {
                state = State::AFTER_BANG;
                bang_dashes = 0;
                break;
            }
            if (c == '?')
            {
                state = State::MARKUP_DECL;
                break;
            }

            if (c == '/')
            {
                closing = true;
                tag_match_mask = 0;
                ++i; // name starts after '/'
            }
            else if ((unsigned)((c | 0x20) - 'a') > 25u) // not a letter: '<' was just text
            {
                if (capture_count > 0)
                    current_text.push_back('<');
                state = State::DATA;
                --i;
                break;
            }
            else
            {
                id_mask = 0;
                class_mask = 0;
                closing = false;
            }
        }
            [[fallthrough]];
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
                state = State::TAG_NAME;
                return;
            }

            c = buf[i];
            tag_key = make_key(name_buf, name_len);

            if (closing)
            {
                // "</>" is ignored, "</3>" or "</ x>" is a bogus comment
                if (name_len == 0 || !is_lower_letter(name_buf[0]))
                {
                    state = (c == '>') ? State::DATA : State::MARKUP_DECL;
                    break;
                }
            }
            else
            {
                // implied end tag: <li>a<li>b closes the first li before the second one opens
                if (untracked == 0 && depth > 0 && key_stack[depth] == tag_key && closes_same_tag(tag_key))
                    close_to(depth);

                if constexpr (R)
                    tag_match_mask = 1u | ((match_stack[depth] << 1) & child_mask) | ((any_stack[depth] << 1) & desc_mask);
                else
                    tag_match_mask = 1u;

                match_tag_name(name_buf, name_len, parts, any_tag_mask);
            }

            tag_match_count += std::popcount(tag_match_mask);

            if (c == '>')
            {
                finish_tag_set_state<R>(id_required_mask, class_required_mask, count);
                break;
            }
        }
            [[fallthrough]]; // space or '/': BEFORE_ATTRIBUTE_NAME skips it
        case State::BEFORE_ATTRIBUTE_NAME:
        before_attr:
        {
            while (i < size && (is_space(buf[i]) || buf[i] == '/'))
                i++;
            if (i == size)
            {
                state = State::BEFORE_ATTRIBUTE_NAME;
                return;
            }
            if (buf[i] == '>')
            {
                finish_tag_set_state<R>(id_required_mask, class_required_mask, count);
                break;
            }
            attr_len = 0;
        }
            [[fallthrough]];
        case State::ATTRIBUTE_NAME:
        {
            while (i < size && !(is_space(buf[i]) || buf[i] == '/' || buf[i] == '>' || buf[i] == '='))
            {
                if (attr_len < 5)
                    attr_buf[attr_len] = (buf[i] >= 'A' && buf[i] <= 'Z') ? buf[i] + 32 : buf[i];
                attr_len++;
                i++;
            }
            if (i == size)
            {
                state = State::ATTRIBUTE_NAME;
                return;
            }

            current_attribute = Attribute::UNKNOWN;
            if (attr_len == 2 && attr_buf[0] == 'i' && attr_buf[1] == 'd')
                current_attribute = Attribute::ID;
            else if (attr_len == 5 && std::memcmp(attr_buf, "class", 5) == 0)
                current_attribute = Attribute::CLASSNAME;
        }
            [[fallthrough]]; // i stays on the delimiter
        case State::AFTER_ATTRIBUTE_NAME:
        {
            while (i < size && is_space(buf[i]))
                i++;
            if (i == size)
            {
                state = State::AFTER_ATTRIBUTE_NAME;
                return;
            }
            c = buf[i];

            if (c == '>')
            {
                finish_tag_set_state<R>(id_required_mask, class_required_mask, count);
                break;
            }
            if (c != '=')
                goto before_attr; // boolean attribute, the next name starts at buf[i]

            candidate_attribute_mask = 0;
            if (current_attribute != Attribute::UNKNOWN)
            {
                attribute_match_count++;
                if constexpr (I)
                {
                    if (current_attribute == Attribute::ID)
                        candidate_attribute_mask = tag_match_mask & id_required_mask;
                }
                if constexpr (C)
                {
                    if (current_attribute == Attribute::CLASSNAME)
                        candidate_attribute_mask = tag_match_mask & class_required_mask;
                }
            }
            value_position = 0;
            ++i; // past '=': VALUE_START must not see it
        }
            [[fallthrough]];
        case State::ATTRIBUTE_VALUE_START:
        {
            while (i < size && is_space(buf[i]))
                i++;
            if (i == size)
            {
                state = State::ATTRIBUTE_VALUE_START;
                return;
            }
            c = buf[i];

            if (c == '>') // "id=>"
            {
                finish_tag_set_state<R>(id_required_mask, class_required_mask, count);
                break;
            }

            quote_char = (c == '"' || c == '\'') ? c : 0;
            attribute_value_mask = candidate_attribute_mask;
            value_position = 0;

            // the value may match the selector: compare it char by char
            if (attribute_value_mask != 0)
            {
                state = State::ATTRIBUTE_VALUE;
                if (quote_char == 0)
                    --i; // unquoted: VALUE must read this first char
                break;   // quoted: i is on the quote, VALUE starts after it
            }

            // quoted and irrelevant: jump past the closing quote
            if (quote_char != 0)
            {
                const char *q = static_cast<const char *>(std::memchr(buf + i + 1, quote_char, size - i - 1));
                if (!q)
                {
                    state = State::ATTRIBUTE_VALUE;
                    return;
                }
                i = static_cast<size_t>(q - buf) + 1;
                goto before_attr;
            }

            // unquoted and irrelevant: scan to space or '>'
            while (i < size && !is_space(buf[i]) && buf[i] != '>')
                i++;
            if (i == size)
            {
                state = State::ATTRIBUTE_VALUE;
                return;
            }
            if (buf[i] == '>')
            {
                finish_tag_set_state<R>(id_required_mask, class_required_mask, count);
                break;
            }
            goto before_attr;
        }
        case State::ATTRIBUTE_VALUE:
        {
            const bool unquoted = (quote_char == 0);
            const bool value_ended = unquoted ? (is_space(c) || c == '>') : (c == quote_char);

            if (value_ended)
            {
                for (uint32_t m = attribute_value_mask; m; m &= m - 1)
                {
                    const unsigned k = std::countr_zero(m);
                    const size_t want = (current_attribute == Attribute::ID) ? parts[k].id.length : parts[k].class_name.length;
                    if (value_position != want)
                        attribute_value_mask &= ~(1u << k);
                }

                const uint32_t already = (current_attribute == Attribute::ID) ? id_mask : class_mask;
                attr_value_match_count += std::popcount(attribute_value_mask & ~already);

                if constexpr (I)
                {
                    if (current_attribute == Attribute::ID)
                        id_mask |= attribute_value_mask;
                }
                if constexpr (C)
                {
                    if (current_attribute == Attribute::CLASSNAME)
                        class_mask |= attribute_value_mask;
                }

                value_position = 0;
                state = State::BEFORE_ATTRIBUTE_NAME;

                if (c == '>')
                    finish_tag_set_state<R>(id_required_mask, class_required_mask, count);
                break;
            }

            // a space inside a quoted class value ends one class name
            if (C && !unquoted && is_space(c) && current_attribute == Attribute::CLASSNAME)
            {
                for (uint32_t m = attribute_value_mask; m; m &= m - 1)
                {
                    const unsigned k = std::countr_zero(m);
                    if (value_position != parts[k].class_name.length)
                        attribute_value_mask &= ~(1u << k);
                }

                attr_value_match_count += std::popcount(attribute_value_mask & ~class_mask);
                class_mask |= attribute_value_mask;

                attribute_value_mask = candidate_attribute_mask;
                value_position = 0;
                break;
            }

            for (uint32_t m = attribute_value_mask; m; m &= m - 1)
            {
                const unsigned k = std::countr_zero(m);
                const SelectorParser::StringView &want = (current_attribute == Attribute::ID) ? parts[k].id : parts[k].class_name;
                if (value_position >= want.length || c != want.ptr[value_position])
                    attribute_value_mask &= ~(1u << k);
            }

            value_position++;
            break;
        }
        case State::AFTER_BANG:
        {
            if (c == '-')
            {
                if (++bang_dashes == 2)
                {
                    dashes = 2; // "<!-->" and "<!--->" already close the comment
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
        }
        case State::COMMENT:
        {
            if (dashes == 0)
            {
                // only a '-' can start the end "-->", everything else is skipped
                const char *d = static_cast<const char *>(std::memchr(buf + i, '-', size - i));
                if (!d)
                    return;
                i = static_cast<size_t>(d - buf);
                c = '-';
            }

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
            }
            break;
        }
        case State::MARKUP_DECL:
        {
            const char *gt = static_cast<const char *>(std::memchr(buf + i, '>', size - i));
            if (!gt)
                return;
            i = static_cast<size_t>(gt - buf);
            state = State::DATA;
            break;
        }
        case State::RAWTEXT:
        {
            const std::string_view end = raw_ends[raw_kind];

            if (raw_match == end.size())
            {
                // "</script" was seen: it only ends the element if a delimiter follows
                raw_match = 0;
                if (c == '>' || c == '/' || is_space(c))
                {
                    const std::string_view name = raw_tags[raw_kind];
                    std::memcpy(name_buf, name.data(), name.size());
                    name_len = name.size();
                    closing = true;
                    tag_match_mask = 0;
                    state = State::TAG_NAME;
                    --i; // TAG_NAME sees this delimiter
                    break;
                }
                // "</scriptx": still raw text, continue with this char
            }

            if (raw_match == 0)
            {
                const char *lt = static_cast<const char *>(std::memchr(buf + i, '<', size - i));
                if (!lt)
                    return; // rest of this buffer is script/style text
                i = static_cast<size_t>(lt - buf);
                c = '<';
            }

            const char lc = (c >= 'A' && c <= 'Z') ? c + 32 : c;
            if (lc == end[raw_match])
                ++raw_match; // reaching end.size() waits for the next char
            else
                raw_match = (lc == '<') ? 1 : 0;
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
    switch (sp.get_flags())
    {
    case 0: run<0, 0, 0>(b, n, sp); break;
    case 1: run<1, 0, 0>(b, n, sp); break;
    case 2: run<0, 1, 0>(b, n, sp); break;
    case 3: run<1, 1, 0>(b, n, sp); break;
    case 4: run<0, 0, 1>(b, n, sp); break;
    case 5: run<1, 0, 1>(b, n, sp); break;
    case 6: run<0, 1, 1>(b, n, sp); break;
    case 7: run<1, 1, 1>(b, n, sp); break;
    }
}
