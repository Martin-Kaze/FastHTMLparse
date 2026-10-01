#pragma once
#include <cstddef>
#include "./Selector/SelectorParser.hpp"
class Parser
{

    enum class State
    {
        DATA,
        TAG_OPEN,
        TAG_NAME,
        ATTRIBUTE_NAME,
        ATTRIBUTE_VALUE,
        ATTRIBUTE_VALUE_START,
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

    size_t attribute_match_count = 0;
    size_t attr_value_match_count = 0;

    size_t selector_match_count = 0;

    size_t value_position = 0;
    size_t attribute_position = 0;

    bool attribute_matches = false;
    bool attr_value_matches = true;
    bool id_matched = false;
    bool class_matched = false;

    char quote_char = ' ';

public:
    void parsring(const char *buf, size_t size, SelectorParser &selectorParser);

    size_t get_tag_match_count() const;
    size_t get_attr_match_count() const;
    size_t get_attr_val_match_count() const;
    size_t get_selector_match_count() const;
};