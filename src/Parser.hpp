#define MAX_DEPTH 1024
#pragma once
#include <cstddef>
#include <cstdint>
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
    bool attr_value_matches = true;

    size_t selector_match_count = 0;

    size_t value_position = 0;
    size_t attribute_position = 0;

    bool attribute_matches = false;

    
    char quote_char = ' ';


    uint32_t match_stack[MAX_DEPTH]{};

    size_t depth =0;

    uint32_t class_mask = 0;
    uint32_t id_mask = 0;
    uint32_t tag_match_mask =0;
    uint32_t candidate_mask =0;
    uint32_t parent_mask = 0;

    uint32_t attribute_value_mask = 0;

    uint32_t candidate_attribute_mask = 0;
    
    void finish_tag_set_state();
    
    void match_tag_char(const char c, const SelectorParser::SelectorPart *parts, size_t count);

public:
   
    void parsring(const char *buf, size_t size, SelectorParser &selectorParser);

    size_t get_tag_match_count() const;
    size_t get_attr_match_count() const;
    size_t get_attr_val_match_count() const;
    size_t get_selector_match_count() const;
};