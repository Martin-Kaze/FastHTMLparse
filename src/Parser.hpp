#define MAX_DEPTH 1024
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "./Selector/SelectorParser.hpp"
#include <bit>


class Parser
{
    std::string current_text;
    size_t capture_depth = 0;
    bool collecting = false;

    uint32_t void_match_mask = 0;

    uint32_t raw_tag_mask = 0;
    size_t raw_match = 0;
    int raw_kind = 0;

    void match_void_tag_char(char c);
    bool current_tag_is_void() const;

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
        HREF,
        SRC
    };

    State state = State::DATA;

    bool closing = false;

    size_t tag_position = 0;

    bool tag_matches = true;

    Attribute currect_attribute = Attribute::UNKNOWN;

    size_t tag_match_count = 0;

    size_t attribute_match_count = 0;
    size_t attr_value_match_count = 0;

    size_t selector_match_count = 0;

    size_t value_position = 0;

    uint8_t bang_dashes = 0;
    uint8_t dashes = 0;

    char quote_char = ' ';

    uint32_t match_stack[MAX_DEPTH]{};
    uint32_t any_stack[MAX_DEPTH]{};

    size_t depth = 0;

    uint32_t class_mask = 0;
    uint32_t id_mask = 0;
    uint32_t tag_match_mask = 0;

    uint32_t attribute_value_mask = 0;

    uint32_t candidate_attribute_mask = 0;

    template <bool HasId, bool HasClass, bool HasRel>
    void run(const char *buf, size_t size, SelectorParser &selectorParser);

    template <bool HasRel>
    void finish_tag_set_state(const size_t id_required_mask, const size_t class_required_mask, const size_t count);
    int current_raw_tag() const;
    void match_tag_name(const char *name, size_t len, const SelectorParser::SelectorPart *parts, uint32_t any_tag_mask);

    using MatchCallback = void (*)(std::string_view, void *);

    MatchCallback on_match = nullptr;
    void *callback_context = nullptr;

    char name_buf[32];
    size_t name_len = 0;

    char attr_buf[5]; // first 5 chars is enough to tell if its id or class
    size_t attr_len = 0;

public:
    void parsring(const char *buf, size_t size, SelectorParser &selectorParser);
    void set_match_callback(MatchCallback callback, void *context = nullptr);

    size_t get_tag_match_count() const;
    size_t get_attr_match_count() const;
    size_t get_attr_val_match_count() const;
    size_t get_selector_match_count() const;

    #ifdef PARSER_STATS
    size_t stat_bytes[16]{};
    size_t stat_iters[16]{};
#endif

};