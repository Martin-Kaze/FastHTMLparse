#include <gtest/gtest.h>

#include "selector_compiler.hpp"
#include "selector_lexer.hpp"

using fasthtml::detail::SelectorCompiler;
using fasthtml::detail::SelectorLexer;
using Relation = SelectorCompiler::Relation;


TEST(SelectorCompiler, TagWithClass)
{
    SelectorLexer lexer;

    std::string input = "div.product";
    lexer.tokenize(input);

    SelectorCompiler parser;
    parser.compile(lexer);

    const auto *parts = parser.parts();

    EXPECT_EQ(parser.part_count(), 1);
    EXPECT_EQ( std::string(parts[0].tag.ptr, parts[0].tag.length), "div");

    EXPECT_EQ(
        std::string(parts[0].class_name.ptr, parts[0].class_name.length),
        "product");

    EXPECT_EQ(parts[0].relation,
              Relation::root);
}

TEST(SelectorCompiler, TagWithId)
{
    SelectorLexer lexer;

    std::string input = "div#main";
    lexer.tokenize(input);

    SelectorCompiler parser;
    parser.compile(lexer);

    const auto *parts = parser.parts();

    EXPECT_EQ(parser.part_count(), 1);
    EXPECT_EQ( std::string(parts[0].tag.ptr, parts[0].tag.length), "div");

    EXPECT_EQ(
        std::string(parts[0].id.ptr, parts[0].id.length),
        "main");

    EXPECT_EQ(parts[0].relation,
              Relation::root);
}

TEST(SelectorCompiler, TagWithIdAndClass)
{
    SelectorLexer lexer;

    std::string input = "div#main.product";
    lexer.tokenize(input);

    SelectorCompiler parser;
    parser.compile(lexer);

    const auto *parts = parser.parts();

    EXPECT_EQ(parser.part_count(), 1);

    EXPECT_EQ( std::string(parts[0].tag.ptr, parts[0].tag.length), "div");

    EXPECT_EQ(
        std::string(parts[0].id.ptr, parts[0].id.length),
        "main");

    EXPECT_EQ(
        std::string(parts[0].class_name.ptr,
                    parts[0].class_name.length),
        "product");

    EXPECT_EQ(parts[0].relation,
              Relation::root);
}

TEST(SelectorCompiler, ClassThenId)
{
    SelectorLexer lexer;

    std::string input = "div.product#main";
    lexer.tokenize(input);

    SelectorCompiler parser;
    parser.compile(lexer);

    const auto *parts = parser.parts();

    EXPECT_EQ(parser.part_count(), 1);

    EXPECT_EQ( std::string(parts[0].tag.ptr, parts[0].tag.length), "div");

    EXPECT_EQ(
        std::string(parts[0].class_name.ptr,
                    parts[0].class_name.length),
        "product");

    EXPECT_EQ(
        std::string(parts[0].id.ptr,
                    parts[0].id.length),
        "main");
}

TEST(SelectorCompiler, MultipleModifiers)
{
    SelectorLexer lexer;

    std::string input = "div#main.product";
    lexer.tokenize(input);

    SelectorCompiler parser;
    parser.compile(lexer);

    const auto *parts = parser.parts();

    EXPECT_EQ(parser.part_count(), 1);

    EXPECT_EQ( std::string(parts[0].tag.ptr, parts[0].tag.length), "div");

    EXPECT_EQ(
        std::string(parts[0].id.ptr,
                    parts[0].id.length),
        "main");

    EXPECT_EQ(
        std::string(parts[0].class_name.ptr,
                    parts[0].class_name.length),
        "product");
}

TEST(SelectorCompiler, ModifiersWithChild)
{
    SelectorLexer lexer;

    std::string input = "div#main.product>p.title";
    lexer.tokenize(input);

    SelectorCompiler parser;
    parser.compile(lexer);

    const auto *parts = parser.parts();

    EXPECT_EQ(parser.part_count(), 2);

    // Part 0
    EXPECT_EQ( std::string(parts[0].tag.ptr, parts[0].tag.length), "div");

    EXPECT_EQ(
        std::string(parts[0].id.ptr,
                    parts[0].id.length),
        "main");

    EXPECT_EQ(
        std::string(parts[0].class_name.ptr,
                    parts[0].class_name.length),
        "product");

    EXPECT_EQ(parts[0].relation,
              Relation::root);

    // Part 1
    EXPECT_EQ( std::string(parts[1].tag.ptr, parts[1].tag.length), "p");

    EXPECT_EQ(
        std::string(parts[1].class_name.ptr,
                    parts[1].class_name.length),
        "title");

    EXPECT_EQ(parts[1].relation,
              Relation::child);
}

TEST(SelectorCompiler, ModifiersWithDescendant)
{
    SelectorLexer lexer;

    std::string input = "div#main.product p.title";
    lexer.tokenize(input);

    SelectorCompiler parser;
    parser.compile(lexer);

    const auto *parts = parser.parts();

    EXPECT_EQ(parser.part_count(), 2);

    EXPECT_EQ( std::string(parts[0].tag.ptr, parts[0].tag.length), "div");

    EXPECT_EQ(
        std::string(parts[0].id.ptr,
                    parts[0].id.length),
        "main");

    EXPECT_EQ(
        std::string(parts[0].class_name.ptr,
                    parts[0].class_name.length),
        "product");

    EXPECT_EQ(parts[0].relation,
              Relation::root);

    EXPECT_EQ( std::string(parts[1].tag.ptr, parts[1].tag.length), "p");

    EXPECT_EQ(
        std::string(parts[1].class_name.ptr,
                    parts[1].class_name.length),
        "title");

    EXPECT_EQ(parts[1].relation,
              Relation::descendant);
}