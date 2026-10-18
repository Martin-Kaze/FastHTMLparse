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
    Attribute candidate_attribute = Attribute::UNKNOWN;

    Attribute currect_attribute = Attribute::UNKNOWN;

    size_t tag_match_count = 0;
    bool name_ended = false;
    size_t attribute_match_count = 0;
    size_t attr_value_match_count = 0;
    bool attr_value_matches = true;

    size_t selector_match_count = 0;

    size_t value_position = 0;
    size_t attribute_position = 0;

    bool attribute_matches = false;

    uint8_t bang_dashes = 0;
    uint8_t dashes = 0;

    char quote_char = ' ';

    uint32_t match_stack[MAX_DEPTH]{};
    uint32_t any_stack[MAX_DEPTH]{};

    size_t depth = 0;

    uint32_t class_mask = 0;
    uint32_t id_mask = 0;
    uint32_t tag_match_mask = 0;
    uint32_t candidate_mask = 0;
    uint32_t parent_mask = 0;

    uint32_t attribute_value_mask = 0;

    uint32_t candidate_attribute_mask = 0;

    void finish_tag_set_state(const size_t id_required_mask, const size_t class_required_mask, const size_t count);

    void match_tag_char(const char c, const SelectorParser::SelectorPart *parts, size_t count);

    using MatchCallback = void (*)(std::string_view, void *);

    MatchCallback on_match = nullptr;
    void *callback_context = nullptr;

public:
    void parsring(const char *buf, size_t size, SelectorParser &selectorParser);
    void set_match_callback(MatchCallback callback, void *context = nullptr);
    void match_raw_tag_char(char c);
    int current_raw_tag() const;

    size_t get_tag_match_count() const;
    size_t get_attr_match_count() const;
    size_t get_attr_val_match_count() const;
    size_t get_selector_match_count() const;
};