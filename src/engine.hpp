// Internal: the streaming HTML state machine that matches one compiled selector.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include "selector_compiler.hpp"

namespace fasthtml::detail
{
    class Engine
    {
    public:
        static constexpr std::size_t max_depth = 1024;        // nesting that is tracked exactly
        static constexpr std::size_t close_search_limit = 32; // how far an end tag looks down for its open tag

        using Callback = void (*)(std::string_view text, void *user);

        void set_callback(Callback callback, void *user) { on_match_ = callback, user_ = user; }

        // Parses the next piece of the document. Pieces may be split anywhere.
        void feed(const char *data, std::size_t size, const SelectorCompiler &selector);
        void finish(); // end of input: emits the text of matched elements that are still open
        void reset();  // back to the initial state, keeps the callback

        std::size_t matches() const { return selector_match_count_; }
        std::size_t tag_matches() const { return tag_match_count_; }
        std::size_t attribute_matches() const { return attribute_match_count_; }
        std::size_t attribute_value_matches() const { return attr_value_match_count_; }

#ifdef FASTHTML_STATS
        std::size_t stat_bytes[16]{};
        std::size_t stat_iters[16]{};
#endif

        // order matters only for the stats names in bench/state_stats.cpp
        enum class State
        {
            data,
            tag_open,
            tag_name,
            attribute_name,
            attribute_value,
            attribute_value_start,
            after_bang,
            comment,
            markup_declaration,
            raw_text,
            before_attribute_name,
            after_attribute_name,
        };

    private:
        enum class Attribute
        {
            unknown,
            id,
            class_name,
        };

        // an open matched element whose text is collected for the callback
        struct Capture
        {
            std::size_t depth; // depth of the matched element
            std::size_t start; // where its text starts inside text_
        };

        template <bool HasId, bool HasClass, bool HasRel>
        void run(const char *buf, std::size_t size, const SelectorCompiler &selector);

        template <bool HasRel>
        void finish_tag(std::uint32_t id_required_mask, std::uint32_t class_required_mask, std::size_t count);

        void match_tag_name(const SelectorCompiler::Part *parts, std::uint32_t any_tag_mask);
        void close_to(std::size_t d); // closes the elements at depth >= d, emitting their captures

        // ------------------------------------------------------------------ state machine
        State state_ = State::data;
        bool closing_ = false;

        char name_buf_[32]{}; // tag name, lowercase, first 32 chars
        std::size_t name_len_ = 0;
        std::uint64_t tag_key_ = 0; // packed tag name, identifies the element on the stack

        bool tag_is_void_ = false;
        int tag_raw_kind_ = -1; // 0 = script, 1 = style, -1 = normal tag

        char attr_buf_[5]{}; // first 5 chars are enough to tell "id" / "class"
        std::size_t attr_len_ = 0;
        Attribute attribute_ = Attribute::unknown;

        char quote_char_ = 0;
        std::size_t value_position_ = 0;

        std::uint8_t bang_dashes_ = 0;
        std::uint8_t dashes_ = 0;

        std::size_t raw_match_ = 0;
        int raw_kind_ = 0;

        // ------------------------------------------------------------------ matching
        std::uint32_t tag_match_mask_ = 0;
        std::uint32_t id_mask_ = 0;
        std::uint32_t class_mask_ = 0;
        std::uint32_t candidate_attribute_mask_ = 0;
        std::uint32_t attribute_value_mask_ = 0;

        // ------------------------------------------------------------------ open elements
        std::size_t depth_ = 0;     // open elements, slot 0 is the document root
        std::size_t untracked_ = 0; // open elements nested deeper than max_depth
        std::uint64_t key_stack_[max_depth]{};
        std::uint32_t match_stack_[max_depth]{};
        std::uint32_t any_stack_[max_depth]{};

        // ------------------------------------------------------------------ captured text
        std::string text_;
        Capture captures_[max_depth]{};
        std::size_t capture_count_ = 0;

        Callback on_match_ = nullptr;
        void *user_ = nullptr;

        // ------------------------------------------------------------------ counters
        std::size_t tag_match_count_ = 0;
        std::size_t attribute_match_count_ = 0;
        std::size_t attr_value_match_count_ = 0;
        std::size_t selector_match_count_ = 0;
    };
}
