#include "SelectorParser.hpp"

using TokenType = SelectorLexer::TokenType;

SelectorParser::SelectorParser(const SelectorParser &other)
{
    *this = other;
}

SelectorParser &SelectorParser::operator=(const SelectorParser &other)
{
    if (this == &other)
        return *this;
    storage_ = other.storage_;
    error_ = other.error_;
    id_required_mask = other.id_required_mask;
    class_required_mask = other.class_required_mask;
    child_mask = other.child_mask;
    desc_mask = other.desc_mask;
    any_tag_mask = other.any_tag_mask;
    flags = other.flags;
    count = other.count;
    for (size_t k = 0; k < MAX_PARTS; k++)
        parts[k] = other.parts[k];
    rebase(other);
    return *this;
}

void SelectorParser::rebase(const SelectorParser &from)
{
    auto fix = [&](StringView &v)
    {
        if (v.ptr != nullptr)
            v.ptr = storage_.data() + (v.ptr - from.storage_.data());
    };
    for (size_t k = 0; k < count; k++)
    {
        fix(parts[k].tag);
        fix(parts[k].id);
        fix(parts[k].class_name);
    }
}

bool SelectorParser::fail(std::string message)
{
    error_ = std::move(message);
    count = 0;
    id_required_mask = class_required_mask = child_mask = desc_mask = any_tag_mask = 0;
    flags = 0;
    return false;
}

bool SelectorParser::parsring(const SelectorLexer &selectorLexer)
{
    storage_.assign(selectorLexer.source().data(), selectorLexer.source().size());
    error_.clear();
    count = 0;
    id_required_mask = class_required_mask = child_mask = desc_mask = any_tag_mask = 0;
    flags = 0;

    if (!selectorLexer.ok())
        return fail(selectorLexer.error());

    const auto *tokens = selectorLexer.get_tokens();
    const size_t n = selectorLexer.get_count();
    if (n == 0)
        return fail("empty selector");

    // token pointers point into the lexer's text; storage_ is an identical copy
    const char *base = selectorLexer.source().data();
    auto view = [&](const SelectorLexer::Token &t) { return StringView{storage_.data() + (t.ptr - base), t.len}; };

    Relation relation = Relation::ROOT;
    size_t i = 0;

    while (i < n)
    {
        if (count == MAX_PARTS)
            return fail("too many parts (more than 16)");
        if (tokens[i].type == TokenType::GREATER_THAN)
            return fail("'>' needs an element on both sides");

        SelectorPart &part = parts[count++];
        part = {};
        part.relation = relation;

        bool fresh = true; // nothing consumed for this part yet

        if (tokens[i].type == TokenType::IDENT)
        {
            part.tag = view(tokens[i]);
            for (size_t j = 0; j < part.tag.length; j++)
            {
                char &ch = storage_[static_cast<size_t>(part.tag.ptr - storage_.data()) + j];
                if (ch >= 'A' && ch <= 'Z')
                    ch = static_cast<char>(ch + 32); // HTML tag names are case-insensitive
            }
            i++;
            fresh = false;
        }

        while (i < n && (tokens[i].type == TokenType::HASH || tokens[i].type == TokenType::DOT))
        {
            if (!fresh && tokens[i].space_before)
                break; // "div .x": the space starts a new part

            const bool is_id = tokens[i].type == TokenType::HASH;
            i++;
            if (i >= n || tokens[i].type != TokenType::IDENT || tokens[i].space_before)
                return fail(is_id ? "'#' must be followed by a name" : "'.' must be followed by a name");

            StringView &slot = is_id ? part.id : part.class_name;
            if (slot.length != 0)
                return fail(is_id ? "only one #id per part is supported" : "only one .class per part is supported");
            slot = view(tokens[i]);
            i++;
            fresh = false;
        }

        if (i < n)
        {
            if (tokens[i].type == TokenType::GREATER_THAN)
            {
                relation = Relation::CHILD;
                i++;
                if (i >= n)
                    return fail("'>' needs an element on both sides");
            }
            else
            {
                relation = Relation::DESCENDANT; // separated by whitespace
            }
        }
    }

    for (size_t k = 0; k < count; k++)
    {
        const uint32_t bit = 1u << k;
        if (parts[k].id.length > 0)
            id_required_mask |= bit;
        if (parts[k].class_name.length > 0)
            class_required_mask |= bit;
        if (parts[k].tag.length == 0)
            any_tag_mask |= bit;
        if (parts[k].relation == Relation::CHILD)
            child_mask |= bit;
        else if (parts[k].relation == Relation::DESCENDANT)
            desc_mask |= bit;
    }

    flags = static_cast<uint8_t>((id_required_mask != 0) << 0 | (class_required_mask != 0) << 1 |
                                 ((child_mask | desc_mask) != 0) << 2);
    return true;
}

bool SelectorParser::ok() const { return error_.empty(); }
const std::string &SelectorParser::error() const { return error_; }
const SelectorParser::SelectorPart *SelectorParser::get_parts() const { return parts; }
size_t SelectorParser::get_count() const { return count; }
size_t SelectorParser::get_id_required_mask() const { return id_required_mask; }
size_t SelectorParser::get_class_required_mask() const { return class_required_mask; }
size_t SelectorParser::get_child_mask() const { return child_mask; }
size_t SelectorParser::get_desc_mask() const { return desc_mask; }
size_t SelectorParser::get_any_tag_mask() const { return any_tag_mask; }
uint8_t SelectorParser::get_flags() const { return flags; }
