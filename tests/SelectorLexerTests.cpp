#include <gtest/gtest.h>

#include "selector_lexer.hpp"

using fasthtml::detail::SelectorLexer;
using TokenType = SelectorLexer::TokenType;


TEST(SelectorLexer, SimpleTag)
{
    SelectorLexer lexer;

    std::string input = "div";
    lexer.tokenize(input);

    const auto* tokens = lexer.tokens();

    ASSERT_EQ(lexer.token_count(), 1);

    EXPECT_EQ(tokens[0].type,
              TokenType::ident);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");
}


TEST(SelectorLexer, Id)
{
    SelectorLexer lexer;

    std::string input = "div#main";
    lexer.tokenize(input);

    const auto* tokens = lexer.tokens();

    ASSERT_EQ(lexer.token_count(), 3);

    EXPECT_EQ(tokens[0].type,
              TokenType::ident);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(tokens[1].type,
              TokenType::hash);

    EXPECT_EQ(tokens[2].type,
              TokenType::ident);

    EXPECT_EQ(std::string(tokens[2].ptr, tokens[2].len),
              "main");
}


TEST(SelectorLexer, Class)
{
    SelectorLexer lexer;

    std::string input = "div.product";
    lexer.tokenize(input);

    const auto* tokens = lexer.tokens();

    ASSERT_EQ(lexer.token_count(), 3);

    EXPECT_EQ(tokens[0].type,
              TokenType::ident);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(tokens[1].type,
              TokenType::dot);

    EXPECT_EQ(tokens[2].type,
              TokenType::ident);

    EXPECT_EQ(std::string(tokens[2].ptr, tokens[2].len),
              "product");
}


TEST(SelectorLexer, IdAndClass)
{
    SelectorLexer lexer;

    std::string input = "div#main.product";
    lexer.tokenize(input);

    const auto* tokens = lexer.tokens();

    ASSERT_EQ(lexer.token_count(), 5);

    EXPECT_EQ(tokens[0].type,
              TokenType::ident);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(tokens[1].type,
              TokenType::hash);

    EXPECT_EQ(tokens[2].type,
              TokenType::ident);

    EXPECT_EQ(std::string(tokens[2].ptr, tokens[2].len),
              "main");

    EXPECT_EQ(tokens[3].type,
              TokenType::dot);

    EXPECT_EQ(tokens[4].type,
              TokenType::ident);

    EXPECT_EQ(std::string(tokens[4].ptr, tokens[4].len),
              "product");
}


TEST(SelectorLexer, Child)
{
    SelectorLexer lexer;

    std::string input = "div>p";
    lexer.tokenize(input);

    const auto* tokens = lexer.tokens();

    ASSERT_EQ(lexer.token_count(), 3);

    EXPECT_EQ(tokens[0].type,
              TokenType::ident);

    EXPECT_EQ(tokens[1].type,
              TokenType::greater_than);

    EXPECT_EQ(tokens[2].type,
              TokenType::ident);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(std::string(tokens[2].ptr, tokens[2].len),
              "p");
}


TEST(SelectorLexer, ChildWithSpaces)
{
    SelectorLexer lexer;

    std::string input = "div > p";
    lexer.tokenize(input);

    const auto* tokens = lexer.tokens();

    ASSERT_EQ(lexer.token_count(), 3);

    EXPECT_EQ(tokens[0].type,
              TokenType::ident);

    EXPECT_EQ(tokens[1].type,
              TokenType::greater_than);

    EXPECT_EQ(tokens[2].type,
              TokenType::ident);
}


TEST(SelectorLexer, Descendant)
{
    SelectorLexer lexer;

    std::string input = "div p";
    lexer.tokenize(input);

    const auto* tokens = lexer.tokens();

    ASSERT_EQ(lexer.token_count(), 2);

    EXPECT_EQ(tokens[0].type,
              TokenType::ident);

    EXPECT_EQ(tokens[1].type,
              TokenType::ident);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(std::string(tokens[1].ptr, tokens[1].len),
              "p");
}


TEST(SelectorLexer, ComplexSelector)
{
    SelectorLexer lexer;

    std::string input = "div.product>p.title";
    lexer.tokenize(input);

    const auto* tokens = lexer.tokens();

    ASSERT_EQ(lexer.token_count(), 7);

    EXPECT_EQ(tokens[0].type,
              TokenType::ident);
    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(tokens[1].type,
              TokenType::dot);

    EXPECT_EQ(tokens[2].type,
              TokenType::ident);
    EXPECT_EQ(std::string(tokens[2].ptr, tokens[2].len),
              "product");

    EXPECT_EQ(tokens[3].type,
              TokenType::greater_than);

    EXPECT_EQ(tokens[4].type,
              TokenType::ident);
    EXPECT_EQ(std::string(tokens[4].ptr, tokens[4].len),
              "p");

    EXPECT_EQ(tokens[5].type,
              TokenType::dot);

    EXPECT_EQ(tokens[6].type,
              TokenType::ident);
    EXPECT_EQ(std::string(tokens[6].ptr, tokens[6].len),
              "title");
}