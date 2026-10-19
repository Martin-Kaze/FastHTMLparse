#pragma once
#include <cstddef>
#include <cstdint>
#include "SelectorLexer.hpp"

class SelectorParser
{

public:
    enum class Relation
    {
        ROOT,
        CHILD,
        DESCENDANT
    };

    struct StringView
    {
        const char *ptr;
        size_t length;
    };

    struct SelectorPart
    {
        StringView tag{};
        StringView id{};
        StringView class_name{};
        Relation relation = Relation::ROOT;
    };


    const SelectorPart *
    get_parts() const;
    size_t get_count() const;
    size_t get_id_required_mask() const;
    size_t get_class_required_mask() const;
    size_t get_desc_mask() const;
    size_t get_child_mask() const;

    uint8_t get_flags() const;

    void parsring(SelectorLexer &selectorLexer);

private:
    size_t id_required_mask = 0;
    size_t class_required_mask = 0;
    size_t child_mask = 0;
    size_t desc_mask = 0;

    uint8_t flags = 0;
    
    SelectorPart parts[16];
    size_t count = 0;
};