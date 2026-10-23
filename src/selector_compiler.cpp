#include "selector_compiler.hpp"

namespace fasthtml::detail
{
    using TokenType = SelectorLexer::TokenType;

    SelectorCompiler::SelectorCompiler(const SelectorCompiler &other)
    {
        *this = other;
    }

    SelectorCompiler &SelectorCompiler::operator=(const SelectorCompiler &other)
    {
        if (this == &other)
            return *this;
        storage_ = other.storage_;
        error_ = other.error_;
        id_required_mask_ = other.id_required_mask_;
        class_required_mask_ = other.class_required_mask_;
        child_mask_ = other.child_mask_;
        descendant_mask_ = other.descendant_mask_;
        any_tag_mask_ = other.any_tag_mask_;
        flags_ = other.flags_;
        count_ = other.count_;
        for (std::size_t k = 0; k < max_parts; k++)
            parts_[k] = other.parts_[k];
        rebase(other);
        return *this;
    }

    void SelectorCompiler::rebase(const SelectorCompiler &from)
    {
        auto fix = [&](Slice &s)
        {
            if (s.ptr != nullptr)
                s.ptr = storage_.data() + (s.ptr - from.storage_.data());
        };
        for (std::size_t k = 0; k < count_; k++)
        {
            fix(parts_[k].tag);
            fix(parts_[k].id);
            fix(parts_[k].class_name);
        }
    }

    void SelectorCompiler::clear()
    {
        count_ = 0;
        id_required_mask_ = class_required_mask_ = child_mask_ = descendant_mask_ = any_tag_mask_ = 0;
        flags_ = 0;
        for (Part &p : parts_)
            p = {};
    }

    bool SelectorCompiler::fail(std::string message)
    {
        error_ = std::move(message);
        clear();
        return false;
    }

    bool SelectorCompiler::compile(std::string_view text)
    {
        SelectorLexer lexer;
        lexer.tokenize(text);
        return compile(lexer);
    }

    bool SelectorCompiler::compile(const SelectorLexer &lexer)
    {
        storage_.assign(lexer.source().data(), lexer.source().size());
        error_.clear();
        clear();

        if (!lexer.ok())
            return fail(lexer.error());

        const auto *tokens = lexer.tokens();
        const std::size_t n = lexer.token_count();
        if (n == 0)
            return fail("empty selector");

        // token pointers point into the lexer's text; storage_ is an identical copy
        const char *base = lexer.source().data();
        auto slice = [&](const SelectorLexer::Token &t) { return Slice{storage_.data() + (t.ptr - base), t.len}; };

        Relation relation = Relation::root;
        std::size_t i = 0;

        while (i < n)
        {
            if (count_ == max_parts)
                return fail("too many parts (more than 16)");
            if (tokens[i].type == TokenType::greater_than)
                return fail("'>' needs an element on both sides");

            Part &part = parts_[count_++];
            part = {};
            part.relation = relation;

            bool fresh = true; // nothing consumed for this part yet

            if (tokens[i].type == TokenType::ident)
            {
                part.tag = slice(tokens[i]);
                const std::size_t at = static_cast<std::size_t>(part.tag.ptr - storage_.data());
                for (std::size_t j = 0; j < part.tag.length; j++)
                {
                    char &ch = storage_[at + j];
                    if (ch >= 'A' && ch <= 'Z')
                        ch = static_cast<char>(ch + 32); // HTML tag names are case-insensitive
                }
                i++;
                fresh = false;
            }

            while (i < n && (tokens[i].type == TokenType::hash || tokens[i].type == TokenType::dot))
            {
                if (!fresh && tokens[i].space_before)
                    break; // "div .x": the space starts a new part

                const bool is_id = tokens[i].type == TokenType::hash;
                i++;
                if (i >= n || tokens[i].type != TokenType::ident || tokens[i].space_before)
                    return fail(is_id ? "'#' must be followed by a name" : "'.' must be followed by a name");

                Slice &slot = is_id ? part.id : part.class_name;
                if (slot.length != 0)
                    return fail(is_id ? "only one #id per part is supported" : "only one .class per part is supported");
                slot = slice(tokens[i]);
                i++;
                fresh = false;
            }

            if (i < n)
            {
                if (tokens[i].type == TokenType::greater_than)
                {
                    relation = Relation::child;
                    i++;
                    if (i >= n)
                        return fail("'>' needs an element on both sides");
                }
                else
                {
                    relation = Relation::descendant; // separated by whitespace
                }
            }
        }

        for (std::size_t k = 0; k < count_; k++)
        {
            const std::uint32_t bit = 1u << k;
            if (parts_[k].id.length > 0)
                id_required_mask_ |= bit;
            if (parts_[k].class_name.length > 0)
                class_required_mask_ |= bit;
            if (parts_[k].tag.length == 0)
                any_tag_mask_ |= bit;
            if (parts_[k].relation == Relation::child)
                child_mask_ |= bit;
            else if (parts_[k].relation == Relation::descendant)
                descendant_mask_ |= bit;
        }

        flags_ = static_cast<std::uint8_t>((id_required_mask_ ? has_id : 0) | (class_required_mask_ ? has_class : 0) |
                                           ((child_mask_ | descendant_mask_) ? has_relation : 0));
        return true;
    }
}
