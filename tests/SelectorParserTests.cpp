#include <gtest/gtest.h>

#include "Selector/SelectorLexer.hpp"
#include "Selector/SelectorParser.hpp"


TEST(SelectorParser, TagWithClass)
{
    SelectorLexer lexer;

    std::string input = "div.product";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto *parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 1);
    EXPECT_EQ( std::string(parts[0].tag.ptr, parts[0].tag.length), "div");

    EXPECT_EQ(
        std::string(parts[0].class_name.ptr, parts[0].class_name.length),
        "product");

    EXPECT_EQ(parts[0].relation,
              SelectorParser::Relation::ROOT);
}

TEST(SelectorParser, TagWithId)
{
    SelectorLexer lexer;

    std::string input = "div#main";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto *parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 1);
    EXPECT_EQ( std::string(parts[0].tag.ptr, parts[0].tag.length), "div");

    EXPECT_EQ(
        std::string(parts[0].id.ptr, parts[0].id.length),
        "main");

    EXPECT_EQ(parts[0].relation,
              SelectorParser::Relation::ROOT);
}

TEST(SelectorParser, TagWithIdAndClass)
{
    SelectorLexer lexer;

    std::string input = "div#main.product";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto *parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 1);

    EXPECT_EQ( std::string(parts[0].tag.ptr, parts[0].tag.length), "div");

    EXPECT_EQ(
        std::string(parts[0].id.ptr, parts[0].id.length),
        "main");

    EXPECT_EQ(
        std::string(parts[0].class_name.ptr,
                    parts[0].class_name.length),
        "product");

    EXPECT_EQ(parts[0].relation,
              SelectorParser::Relation::ROOT);
}

TEST(SelectorParser, ClassThenId)
{
    SelectorLexer lexer;

    std::string input = "div.product#main";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto *parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 1);

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

TEST(SelectorParser, MultipleModifiers)
{
    SelectorLexer lexer;

    std::string input = "div#main.product";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto *parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 1);

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

TEST(SelectorParser, ModifiersWithChild)
{
    SelectorLexer lexer;

    std::string input = "div#main.product>p.title";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto *parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 2);

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
              SelectorParser::Relation::ROOT);

    // Part 1
    EXPECT_EQ( std::string(parts[1].tag.ptr, parts[1].tag.length), "p");

    EXPECT_EQ(
        std::string(parts[1].class_name.ptr,
                    parts[1].class_name.length),
        "title");

    EXPECT_EQ(parts[1].relation,
              SelectorParser::Relation::CHILD);
}

TEST(SelectorParser, ModifiersWithDescendant)
{
    SelectorLexer lexer;

    std::string input = "div#main.product p.title";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto *parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 2);

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
              SelectorParser::Relation::ROOT);

    EXPECT_EQ( std::string(parts[1].tag.ptr, parts[1].tag.length), "p");

    EXPECT_EQ(
        std::string(parts[1].class_name.ptr,
                    parts[1].class_name.length),
        "title");

    EXPECT_EQ(parts[1].relation,
              SelectorParser::Relation::DESCENDANT);
}