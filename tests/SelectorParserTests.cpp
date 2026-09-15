#include <gtest/gtest.h>

#include "Selector/SelectorLexer.hpp"
#include "Selector/SelectorParser.hpp"


TEST(SelectorParser, SimpleTag)
{
    SelectorLexer lexer;

    std::string input = "div";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto* parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 1);

    EXPECT_EQ(parts[0].tag, "div");
    EXPECT_EQ(parts[0].id, "");
    EXPECT_EQ(parts[0].class_name, "");

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

    const auto* parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 1);

    EXPECT_EQ(parts[0].tag, "div");
    EXPECT_EQ(parts[0].id, "main");
    EXPECT_EQ(parts[0].class_name, "");

    EXPECT_EQ(parts[0].relation,
              SelectorParser::Relation::ROOT);
}


TEST(SelectorParser, TagWithClass)
{
    SelectorLexer lexer;

    std::string input = "div.product";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto* parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 1);

    EXPECT_EQ(parts[0].tag, "div");
    EXPECT_EQ(parts[0].id, "");
    EXPECT_EQ(parts[0].class_name, "product");

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

    const auto* parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 1);

    EXPECT_EQ(parts[0].tag, "div");
    EXPECT_EQ(parts[0].id, "main");
    EXPECT_EQ(parts[0].class_name, "product");

    EXPECT_EQ(parts[0].relation,
              SelectorParser::Relation::ROOT);
}


TEST(SelectorParser, Child)
{
    SelectorLexer lexer;

    std::string input = "div>p";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto* parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 2);

    EXPECT_EQ(parts[0].tag, "div");
    EXPECT_EQ(parts[0].relation,
              SelectorParser::Relation::ROOT);

    EXPECT_EQ(parts[1].tag, "p");
    EXPECT_EQ(parts[1].relation,
              SelectorParser::Relation::CHILD);
}


TEST(SelectorParser, Descendant)
{
    SelectorLexer lexer;

    std::string input = "div p";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto* parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 2);

    EXPECT_EQ(parts[0].tag, "div");
    EXPECT_EQ(parts[0].relation,
              SelectorParser::Relation::ROOT);

    EXPECT_EQ(parts[1].tag, "p");
    EXPECT_EQ(parts[1].relation,
              SelectorParser::Relation::DESCENDANT);
}


TEST(SelectorParser, ComplexChild)
{
    SelectorLexer lexer;

    std::string input = "div.product>p.title";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto* parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 2);

    EXPECT_EQ(parts[0].tag, "div");
    EXPECT_EQ(parts[0].class_name, "product");
    EXPECT_EQ(parts[0].relation,
              SelectorParser::Relation::ROOT);

    EXPECT_EQ(parts[1].tag, "p");
    EXPECT_EQ(parts[1].class_name, "title");
    EXPECT_EQ(parts[1].relation,
              SelectorParser::Relation::CHILD);
}


TEST(SelectorParser, ComplexDescendant)
{
    SelectorLexer lexer;

    std::string input = "div.product p.title";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto* parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 2);

    EXPECT_EQ(parts[0].tag, "div");
    EXPECT_EQ(parts[0].class_name, "product");
    EXPECT_EQ(parts[0].relation,
              SelectorParser::Relation::ROOT);

    EXPECT_EQ(parts[1].tag, "p");
    EXPECT_EQ(parts[1].class_name, "title");
    EXPECT_EQ(parts[1].relation,
              SelectorParser::Relation::DESCENDANT);
}


TEST(SelectorParser, MultipleChildren)
{
    SelectorLexer lexer;

    std::string input = "div>p>span";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto* parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 3);

    EXPECT_EQ(parts[0].tag, "div");
    EXPECT_EQ(parts[0].relation,
              SelectorParser::Relation::ROOT);

    EXPECT_EQ(parts[1].tag, "p");
    EXPECT_EQ(parts[1].relation,
              SelectorParser::Relation::CHILD);

    EXPECT_EQ(parts[2].tag, "span");
    EXPECT_EQ(parts[2].relation,
              SelectorParser::Relation::CHILD);
}


TEST(SelectorParser, MixedRelations)
{
    SelectorLexer lexer;

    std::string input = "div>p span";
    lexer.query(input);

    SelectorParser parser;
    parser.parsring(lexer);

    const auto* parts = parser.get_parts();

    EXPECT_EQ(parser.get_count(), 3);

    EXPECT_EQ(parts[0].tag, "div");
    EXPECT_EQ(parts[0].relation,
              SelectorParser::Relation::ROOT);

    EXPECT_EQ(parts[1].tag, "p");
    EXPECT_EQ(parts[1].relation,
              SelectorParser::Relation::CHILD);

    EXPECT_EQ(parts[2].tag, "span");
    EXPECT_EQ(parts[2].relation,
              SelectorParser::Relation::DESCENDANT);
}