#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include "./Selector/SelectorParser.hpp"

class Parser
{
public:
    static constexpr size_t MAX_DEPTH = 1024;        // nesting that is tracked exactly
    static constexpr size_t CLOSE_SEARCH_LIMIT = 32; // how far an end tag looks down for its open tag

    using MatchCallback = void (*)(std::string_view, void *);

    void parsring(const char *buf, size_t size, SelectorParser &selectorParser);
    void finish(); // end of input: emits the text of matched elements that are still open
    void reset();  // back to the initial state, keeps the callback
    void set_match_callback(MatchCallback callback, void *context = nullptr);

    size_t get_tag_match_count() const;
    size_t get_attr_match_count() const;
    size_t get_attr_val_match_count() const;
    size_t get_selector_match_count() const;

#ifdef PARSER_STATS
    size_t stat_bytes[16]{};
    size_t stat_iters[16]{};
#endif

private:
    enum class State
    {
        DATA,
        TAG_OPEN,
        TAG_NAME,
        ATTRIBUTE_NAME,
        ATTRIBUTE_VALUE,
        ATTRIBUTE_VALUE_START,
        AFTER_BANG,
        COMMENT,
        MARKUP_DECL,
        RAWTEXT,
        BEFORE_ATTRIBUTE_NAME,
        AFTER_ATTRIBUTE_NAME,
    };

    enum class Attribute
    {
        UNKNOWN,
        ID,
        CLASSNAME,
    };

    // an open matched element whose text is being collected for the callback
    struct Capture
    {
        size_t depth; // depth of the matched element
        size_t start; // where its text starts inside current_text
    };

    template <bool HasId, bool HasClass, bool HasRel>
    void run(const char *buf, size_t size, SelectorParser &selectorParser);

    template <bool HasRel>
    void finish_tag_set_state(uint32_t id_required_mask, uint32_t class_required_mask, size_t count);

    void match_tag_name(const char *name, size_t len, const SelectorParser::SelectorPart *parts, uint32_t any_tag_mask);
    void close_to(size_t d); // closes the elements at depth >= d, emitting their captures

    // ------------------------------------------------------------------ state machine
    State state = State::DATA;
    bool closing = false;

    char name_buf[32]{}; // tag name, lowercase, first 32 chars
    size_t name_len = 0;
    uint64_t tag_key = 0; // packed tag name, identifies the element on the stack

    bool tag_is_void = false;
    int tag_raw_kind = -1; // 0 = script, 1 = style, -1 = normal tag

    char attr_buf[5]{}; // first 5 chars are enough to tell "id" / "class"
    size_t attr_len = 0;
    Attribute current_attribute = Attribute::UNKNOWN;

    char quote_char = 0;
    size_t value_position = 0;

    uint8_t bang_dashes = 0;
    uint8_t dashes = 0;

    size_t raw_match = 0;
    int raw_kind = 0;

    // ------------------------------------------------------------------ matching
    uint32_t tag_match_mask = 0;
    uint32_t id_mask = 0;
    uint32_t class_mask = 0;
    uint32_t candidate_attribute_mask = 0;
    uint32_t attribute_value_mask = 0;

    // ------------------------------------------------------------------ open elements
    size_t depth = 0;     // open elements, slot 0 is the document root
    size_t untracked = 0; // open elements nested deeper than MAX_DEPTH
    uint64_t key_stack[MAX_DEPTH]{};
    uint32_t match_stack[MAX_DEPTH]{};
    uint32_t any_stack[MAX_DEPTH]{};

    // ------------------------------------------------------------------ captured text
    std::string current_text;
    Capture captures[MAX_DEPTH]{};
    size_t capture_count = 0;

    MatchCallback on_match = nullptr;
    void *callback_context = nullptr;

    // ------------------------------------------------------------------ counters
    size_t tag_match_count = 0;
    size_t attribute_match_count = 0;
    size_t attr_value_match_count = 0;
    size_t selector_match_count = 0;
};
