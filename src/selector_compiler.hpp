// Internal: turns selector tokens into parts plus the bitmasks the engine matches with.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include "selector_lexer.hpp"

namespace fasthtml::detail
{
    class SelectorCompiler
    {
    public:
        static constexpr std::size_t max_parts = 16;

        enum class Relation
        {
            root,       // first part
            child,      // "a > b"
            descendant, // "a b"
        };

        struct Slice
        {
            const char *ptr = nullptr; // points into this compiler's own copy of the selector
            std::size_t length = 0;

            std::string_view view() const { return {ptr ? ptr : "", length}; }
        };

        struct Part
        {
            Slice tag{}; // lowercase, empty = any tag
            Slice id{};
            Slice class_name{};
            Relation relation = Relation::root;
        };

        // Flags select the specialized engine loop.
        static constexpr std::uint8_t has_id = 1;
        static constexpr std::uint8_t has_class = 2;
        static constexpr std::uint8_t has_relation = 4;

        SelectorCompiler() = default;
        SelectorCompiler(const SelectorCompiler &other);
        SelectorCompiler &operator=(const SelectorCompiler &other);

        bool compile(std::string_view text);           // tokenize + compile
        bool compile(const SelectorLexer &lexer);      // returns false (and matches nothing) on error

        bool ok() const { return error_.empty(); }
        const std::string &error() const { return error_; }

        const Part *parts() const { return parts_; }
        std::size_t part_count() const { return count_; }

        // bit k = part k
        std::uint32_t id_required_mask() const { return id_required_mask_; }
        std::uint32_t class_required_mask() const { return class_required_mask_; }
        std::uint32_t child_mask() const { return child_mask_; }
        std::uint32_t descendant_mask() const { return descendant_mask_; }
        std::uint32_t any_tag_mask() const { return any_tag_mask_; }
        std::uint8_t flags() const { return flags_; }

    private:
        bool fail(std::string message);
        void clear();
        void rebase(const SelectorCompiler &from); // re-point parts into this->storage_ after a copy

        std::string storage_; // copy of the selector text, tag names lowercased
        std::string error_;

        std::uint32_t id_required_mask_ = 0;
        std::uint32_t class_required_mask_ = 0;
        std::uint32_t child_mask_ = 0;
        std::uint32_t descendant_mask_ = 0;
        std::uint32_t any_tag_mask_ = 0;
        std::uint8_t flags_ = 0;

        Part parts_[max_parts]{};
        std::size_t count_ = 0;
    };
}
