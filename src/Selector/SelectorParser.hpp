#pragma once
#include <cstddef>
#include <string>
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

    struct SelectorPart
    {
        std::string tag;
        std::string id;
        std::string class_name;
        Relation relation;
    };

   const SelectorPart* get_parts() const;
size_t get_count() const;

    void parsring(SelectorLexer &selectorLexer);

private:

    SelectorPart parts[16];
    size_t count = 0;
};