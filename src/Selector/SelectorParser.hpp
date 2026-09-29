#pragma once
#include <cstddef>
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

    const SelectorPart *get_parts() const;
    size_t get_count() const;
    void parsring(SelectorLexer &selectorLexer);

private:
    SelectorPart parts[16];
    size_t count = 0;
};