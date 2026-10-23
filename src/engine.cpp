#include "engine.hpp"
#include <bit>
#include <cstring>
#include <string_view>

namespace fasthtml::detail
{
using std::size_t;
using std::uint32_t;
using std::uint64_t;

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

void Engine::close_to(size_t d)
{
    while (capture_count_ > 0 && captures_[capture_count_ - 1].depth >= d)
    {
        const Capture &cap = captures_[--capture_count_];
        on_match_(std::string_view(text_).substr(cap.start), user_);
    }
    if (capture_count_ == 0)
        text_.clear();
    depth_ = d - 1;
}

void Engine::finish()
{
    close_to(1); // emits every open capture, innermost first
    untracked_ = 0;
    state_ = State::data;
    raw_match_ = 0;
    closing_ = false;
}

void Engine::reset()
{
    const Callback cb = on_match_;
    void *const ctx = user_;
    *this = Engine();
    on_match_ = cb;
    user_ = ctx;
}

template <bool HasRel>
void Engine::finish_tag(const uint32_t id_required_mask, const uint32_t class_required_mask, const size_t count)
{
    if (closing_)
    {
        if (untracked_ > 0)
        {
            --untracked_; // closes an element nested deeper than max_depth
        }
        else
        {
            // close the nearest open element with this name; anything above it was left unclosed
            const size_t stop = depth_ > close_search_limit ? depth_ - close_search_limit : 0;
            for (size_t d = depth_; d > stop; --d)
            {
                if (key_stack_[d] == tag_key_)
                {
                    close_to(d);
                    break;
                }
            }
            // not found: stray end tag, ignored
        }
        state_ = State::data;
        return;
    }

    const uint32_t matched = tag_match_mask_ & (~id_required_mask | id_mask_) & (~class_required_mask | class_mask_);

    bool tracked = false;
    if (!tag_is_void_)
    {
        if (untracked_ == 0 && depth_ + 1 < max_depth)
        {
            ++depth_;
            key_stack_[depth_] = tag_key_;
            if constexpr (HasRel)
            {
                match_stack_[depth_] = matched;
                any_stack_[depth_] = any_stack_[depth_ - 1] | matched;
            }
            tracked = true;
        }
        else
        {
            ++untracked_;
        }
    }

    if (count > 0 && (matched & (1u << (count - 1))))
    {
        selector_match_count_++;

        if (on_match_ != nullptr)
        {
            if (tracked)
                captures_[capture_count_++] = {depth_, text_.size()};
            else
                on_match_(std::string_view{}, user_); // void element (or nested too deep): no text
        }
    }

    if (tag_raw_kind_ >= 0)
    {
        raw_kind_ = tag_raw_kind_;
        raw_match_ = 0;
        state_ = State::raw_text;
    }
    else
    {
        state_ = State::data;
    }
}

void Engine::match_tag_name(const SelectorCompiler::Part *parts, uint32_t any_tag_mask)
{
    const char *name = name_buf_;
    const size_t len = name_len_;

    tag_is_void_ = false;
    tag_raw_kind_ = -1;

    // no real tag name is longer than 32
    if (len > 32)
    {
        tag_match_mask_ &= any_tag_mask;
        return;
    }

    const std::string_view n(name, len);

    for (uint32_t m = tag_match_mask_ & ~any_tag_mask; m; m &= m - 1)
    {
        const unsigned k = std::countr_zero(m);
        if (parts[k].tag.view() != n)
            tag_match_mask_ &= ~(1u << k);
    }

    // void tags are 2..6 chars, raw tags 5..6
    if (len >= 2 && len <= 6)
    {
        for (std::string_view v : void_tags)
        {
            if (v == n)
            {
                tag_is_void_ = true;
                return;
            }
        }
        for (int k = 0; k < 2; k++)
        {
            if (raw_tags[k] == n)
            {
                tag_raw_kind_ = k;
                return;
            }
        }
    }
}

template <bool I, bool C, bool R>
void Engine::run(const char *buf, size_t size, const SelectorCompiler &selector)
{
    const auto *parts = selector.parts();
    const size_t count = selector.part_count();

    const uint32_t id_required_mask = selector.id_required_mask();
    const uint32_t class_required_mask = selector.class_required_mask();
    const uint32_t child_mask = selector.child_mask();
    const uint32_t desc_mask = selector.descendant_mask();
    const uint32_t any_tag_mask = selector.any_tag_mask();

    for (size_t i = 0; i < size; i++)
    {
#ifdef FASTHTML_STATS
        const int st = (int)state_;
        const size_t i0 = i;
#endif
        char c = buf[i];

        switch (state_)
        {
        case State::data:
        {
            const char *text_start = buf + i;
            const char *next_tag = static_cast<const char *>(std::memchr(text_start, '<', size - i));
            const size_t text_length = next_tag ? static_cast<size_t>(next_tag - text_start) : size - i;

            if (capture_count_ > 0 && text_length > 0)
                text_.append(text_start, text_length);

            if (next_tag == nullptr)
                return;

            i = static_cast<size_t>(next_tag - buf) + 1; // past the '<'
            name_len_ = 0;
            closing_ = false;

            if (i == size)
            {
                state_ = State::tag_open;
                return;
            }
        }
            [[fallthrough]];
        case State::tag_open:
        {
            c = buf[i];
            if (c == '!')
            {
                state_ = State::after_bang;
                bang_dashes_ = 0;
                break;
            }
            if (c == '?')
            {
                state_ = State::markup_declaration;
                break;
            }

            if (c == '/')
            {
                closing_ = true;
                tag_match_mask_ = 0;
                ++i; // name starts after '/'
            }
            else if ((unsigned)((c | 0x20) - 'a') > 25u) // not a letter: '<' was just text
            {
                if (capture_count_ > 0)
                    text_.push_back('<');
                state_ = State::data;
                --i;
                break;
            }
            else
            {
                id_mask_ = 0;
                class_mask_ = 0;
                closing_ = false;
            }
        }
            [[fallthrough]];
        case State::tag_name:
        {
            while (i < size && !(buf[i] == '>' || buf[i] == '/' || is_space(buf[i])))
            {
                if (name_len_ < 32)
                    name_buf_[name_len_] = (buf[i] >= 'A' && buf[i] <= 'Z') ? buf[i] + 32 : buf[i];
                name_len_++;
                i++;
            }

            if (i == size)
            {
                state_ = State::tag_name;
                return;
            }

            c = buf[i];
            tag_key_ = make_key(name_buf_, name_len_);

            if (closing_)
            {
                // "</>" is ignored, "</3>" or "</ x>" is a bogus comment
                if (name_len_ == 0 || !is_lower_letter(name_buf_[0]))
                {
                    state_ = (c == '>') ? State::data : State::markup_declaration;
                    break;
                }
            }
            else
            {
                // implied end tag: <li>a<li>b closes the first li before the second one opens
                if (untracked_ == 0 && depth_ > 0 && key_stack_[depth_] == tag_key_ && closes_same_tag(tag_key_))
                    close_to(depth_);

                if constexpr (R)
                    tag_match_mask_ = 1u | ((match_stack_[depth_] << 1) & child_mask) | ((any_stack_[depth_] << 1) & desc_mask);
                else
                    tag_match_mask_ = 1u;

                match_tag_name(parts, any_tag_mask);
            }

            tag_match_count_ += std::popcount(tag_match_mask_);

            if (c == '>')
            {
                finish_tag<R>(id_required_mask, class_required_mask, count);
                break;
            }
        }
            [[fallthrough]]; // space or '/': BEFORE_ATTRIBUTE_NAME skips it
        case State::before_attribute_name:
        before_attr:
        {
            while (i < size && (is_space(buf[i]) || buf[i] == '/'))
                i++;
            if (i == size)
            {
                state_ = State::before_attribute_name;
                return;
            }
            if (buf[i] == '>')
            {
                finish_tag<R>(id_required_mask, class_required_mask, count);
                break;
            }
            attr_len_ = 0;
        }
            [[fallthrough]];
        case State::attribute_name:
        {
            while (i < size && !(is_space(buf[i]) || buf[i] == '/' || buf[i] == '>' || buf[i] == '='))
            {
                if (attr_len_ < 5)
                    attr_buf_[attr_len_] = (buf[i] >= 'A' && buf[i] <= 'Z') ? buf[i] + 32 : buf[i];
                attr_len_++;
                i++;
            }
            if (i == size)
            {
                state_ = State::attribute_name;
                return;
            }

            attribute_ = Attribute::unknown;
            if (attr_len_ == 2 && attr_buf_[0] == 'i' && attr_buf_[1] == 'd')
                attribute_ = Attribute::id;
            else if (attr_len_ == 5 && std::memcmp(attr_buf_, "class", 5) == 0)
                attribute_ = Attribute::class_name;
        }
            [[fallthrough]]; // i stays on the delimiter
        case State::after_attribute_name:
        {
            while (i < size && is_space(buf[i]))
                i++;
            if (i == size)
            {
                state_ = State::after_attribute_name;
                return;
            }
            c = buf[i];

            if (c == '>')
            {
                finish_tag<R>(id_required_mask, class_required_mask, count);
                break;
            }
            if (c != '=')
                goto before_attr; // boolean attribute, the next name starts at buf[i]

            candidate_attribute_mask_ = 0;
            if (attribute_ != Attribute::unknown)
            {
                attribute_match_count_++;
                if constexpr (I)
                {
                    if (attribute_ == Attribute::id)
                        candidate_attribute_mask_ = tag_match_mask_ & id_required_mask;
                }
                if constexpr (C)
                {
                    if (attribute_ == Attribute::class_name)
                        candidate_attribute_mask_ = tag_match_mask_ & class_required_mask;
                }
            }
            value_position_ = 0;
            ++i; // past '=': VALUE_START must not see it
        }
            [[fallthrough]];
        case State::attribute_value_start:
        {
            while (i < size && is_space(buf[i]))
                i++;
            if (i == size)
            {
                state_ = State::attribute_value_start;
                return;
            }
            c = buf[i];

            if (c == '>') // "id=>"
            {
                finish_tag<R>(id_required_mask, class_required_mask, count);
                break;
            }

            quote_char_ = (c == '"' || c == '\'') ? c : 0;
            attribute_value_mask_ = candidate_attribute_mask_;
            value_position_ = 0;

            // the value may match the selector: compare it char by char
            if (attribute_value_mask_ != 0)
            {
                state_ = State::attribute_value;
                if (quote_char_ == 0)
                    --i; // unquoted: VALUE must read this first char
                break;   // quoted: i is on the quote, VALUE starts after it
            }

            // quoted and irrelevant: jump past the closing_ quote
            if (quote_char_ != 0)
            {
                const char *q = static_cast<const char *>(std::memchr(buf + i + 1, quote_char_, size - i - 1));
                if (!q)
                {
                    state_ = State::attribute_value;
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
                state_ = State::attribute_value;
                return;
            }
            if (buf[i] == '>')
            {
                finish_tag<R>(id_required_mask, class_required_mask, count);
                break;
            }
            goto before_attr;
        }
        case State::attribute_value:
        {
            const bool unquoted = (quote_char_ == 0);
            const bool value_ended = unquoted ? (is_space(c) || c == '>') : (c == quote_char_);

            if (value_ended)
            {
                for (uint32_t m = attribute_value_mask_; m; m &= m - 1)
                {
                    const unsigned k = std::countr_zero(m);
                    const size_t want = (attribute_ == Attribute::id) ? parts[k].id.length : parts[k].class_name.length;
                    if (value_position_ != want)
                        attribute_value_mask_ &= ~(1u << k);
                }

                const uint32_t already = (attribute_ == Attribute::id) ? id_mask_ : class_mask_;
                attr_value_match_count_ += std::popcount(attribute_value_mask_ & ~already);

                if constexpr (I)
                {
                    if (attribute_ == Attribute::id)
                        id_mask_ |= attribute_value_mask_;
                }
                if constexpr (C)
                {
                    if (attribute_ == Attribute::class_name)
                        class_mask_ |= attribute_value_mask_;
                }

                value_position_ = 0;
                state_ = State::before_attribute_name;

                if (c == '>')
                    finish_tag<R>(id_required_mask, class_required_mask, count);
                break;
            }

            // a space inside a quoted class value ends one class name
            if (C && !unquoted && is_space(c) && attribute_ == Attribute::class_name)
            {
                for (uint32_t m = attribute_value_mask_; m; m &= m - 1)
                {
                    const unsigned k = std::countr_zero(m);
                    if (value_position_ != parts[k].class_name.length)
                        attribute_value_mask_ &= ~(1u << k);
                }

                attr_value_match_count_ += std::popcount(attribute_value_mask_ & ~class_mask_);
                class_mask_ |= attribute_value_mask_;

                attribute_value_mask_ = candidate_attribute_mask_;
                value_position_ = 0;
                break;
            }

            for (uint32_t m = attribute_value_mask_; m; m &= m - 1)
            {
                const unsigned k = std::countr_zero(m);
                const SelectorCompiler::Slice &want = (attribute_ == Attribute::id) ? parts[k].id : parts[k].class_name;
                if (value_position_ >= want.length || c != want.ptr[value_position_])
                    attribute_value_mask_ &= ~(1u << k);
            }

            value_position_++;
            break;
        }
        case State::after_bang:
        {
            if (c == '-')
            {
                if (++bang_dashes_ == 2)
                {
                    dashes_ = 2; // "<!-->" and "<!--->" already close the comment
                    state_ = State::comment;
                }
            }
            else if (c == '>')
            {
                state_ = State::data;
            }
            else
            {
                state_ = State::markup_declaration;
            }
            break;
        }
        case State::comment:
        {
            if (dashes_ == 0)
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
                if (dashes_ < 2)
                    dashes_++;
            }
            else if (c == '>' && dashes_ >= 2)
            {
                state_ = State::data;
                dashes_ = 0;
            }
            else
            {
                dashes_ = 0;
            }
            break;
        }
        case State::markup_declaration:
        {
            const char *gt = static_cast<const char *>(std::memchr(buf + i, '>', size - i));
            if (!gt)
                return;
            i = static_cast<size_t>(gt - buf);
            state_ = State::data;
            break;
        }
        case State::raw_text:
        {
            const std::string_view end = raw_ends[raw_kind_];

            if (raw_match_ == end.size())
            {
                // "</script" was seen: it only ends the element if a delimiter follows
                raw_match_ = 0;
                if (c == '>' || c == '/' || is_space(c))
                {
                    const std::string_view name = raw_tags[raw_kind_];
                    std::memcpy(name_buf_, name.data(), name.size());
                    name_len_ = name.size();
                    closing_ = true;
                    tag_match_mask_ = 0;
                    state_ = State::tag_name;
                    --i; // TAG_NAME sees this delimiter
                    break;
                }
                // "</scriptx": still raw text, continue with this char
            }

            if (raw_match_ == 0)
            {
                const char *lt = static_cast<const char *>(std::memchr(buf + i, '<', size - i));
                if (!lt)
                    return; // rest of this buffer is script/style text
                i = static_cast<size_t>(lt - buf);
                c = '<';
            }

            const char lc = (c >= 'A' && c <= 'Z') ? c + 32 : c;
            if (lc == end[raw_match_])
                ++raw_match_; // reaching end.size() waits for the next char
            else
                raw_match_ = (lc == '<') ? 1 : 0;
            break;
        }
        }
#ifdef FASTHTML_STATS
        stat_iters[st]++;
        stat_bytes[st] += i - i0 + 1;
#endif
    }
}

void Engine::feed(const char *data, size_t size, const SelectorCompiler &selector)
{
    constexpr auto I = SelectorCompiler::has_id, C = SelectorCompiler::has_class, R = SelectorCompiler::has_relation;
    switch (selector.flags())
    {
    case 0: run<false, false, false>(data, size, selector); break;
    case I: run<true, false, false>(data, size, selector); break;
    case C: run<false, true, false>(data, size, selector); break;
    case I | C: run<true, true, false>(data, size, selector); break;
    case R: run<false, false, true>(data, size, selector); break;
    case I | R: run<true, false, true>(data, size, selector); break;
    case C | R: run<false, true, true>(data, size, selector); break;
    case I | C | R: run<true, true, true>(data, size, selector); break;
    }
}
}
