#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include "SelectorLexer.hpp"

class SelectorParser
{
public:
    static constexpr size_t MAX_PARTS = 16;

    enum class Relation
    {
        ROOT,
        CHILD,
        DESCENDANT
    };

    struct StringView
    {
        const char *ptr = nullptr; // points into this parser's own copy of the selector
        size_t length = 0;
    };

    struct SelectorPart
    {
        StringView tag{}; // lowercase
        StringView id{};
        StringView class_name{};
        Relation relation = Relation::ROOT;
    };

    SelectorParser() = default;
    SelectorParser(const SelectorParser &other);
    SelectorParser &operator=(const SelectorParser &other);

    // Builds the parts from the lexer's tokens. Returns false (and matches nothing) on error.
    bool parsring(const SelectorLexer &selectorLexer);

    bool ok() const;
    const std::string &error() const;

    const SelectorPart *get_parts() const;
    size_t get_count() const;
    size_t get_id_required_mask() const;
    size_t get_class_required_mask() const;
    size_t get_desc_mask() const;
    size_t get_child_mask() const;
    size_t get_any_tag_mask() const;

    uint8_t get_flags() const;

private:
    bool fail(std::string message);
    void rebase(const SelectorParser &from); // re-point parts into this->storage_ after a copy

    std::string storage_; // copy of the selector text, tag names lowercased
    std::string error_;

    uint32_t id_required_mask = 0;
    uint32_t class_required_mask = 0;
    uint32_t child_mask = 0;
    uint32_t desc_mask = 0;
    uint32_t any_tag_mask = 0;

    uint8_t flags = 0;

    SelectorPart parts[MAX_PARTS];
    size_t count = 0;
};
