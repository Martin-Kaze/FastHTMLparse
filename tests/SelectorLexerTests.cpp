#include <gtest/gtest.h>

#include "Selector/SelectorLexer.hpp"


TEST(SelectorLexer, SimpleTag)
{
    SelectorLexer lexer;

    std::string input = "div";
    lexer.query(input);

    const auto* tokens = lexer.get_tokens();

    ASSERT_EQ(lexer.get_count(), 1);

    EXPECT_EQ(tokens[0].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");
}


TEST(SelectorLexer, Id)
{
    SelectorLexer lexer;

    std::string input = "div#main";
    lexer.query(input);

    const auto* tokens = lexer.get_tokens();

    ASSERT_EQ(lexer.get_count(), 3);

    EXPECT_EQ(tokens[0].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(tokens[1].type,
              SelectorLexer::TokenType::HASH);

    EXPECT_EQ(tokens[2].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(std::string(tokens[2].ptr, tokens[2].len),
              "main");
}


TEST(SelectorLexer, Class)
{
    SelectorLexer lexer;

    std::string input = "div.product";
    lexer.query(input);

    const auto* tokens = lexer.get_tokens();

    ASSERT_EQ(lexer.get_count(), 3);

    EXPECT_EQ(tokens[0].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(tokens[1].type,
              SelectorLexer::TokenType::DOT);

    EXPECT_EQ(tokens[2].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(std::string(tokens[2].ptr, tokens[2].len),
              "product");
}


TEST(SelectorLexer, IdAndClass)
{
    SelectorLexer lexer;

    std::string input = "div#main.product";
    lexer.query(input);

    const auto* tokens = lexer.get_tokens();

    ASSERT_EQ(lexer.get_count(), 5);

    EXPECT_EQ(tokens[0].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(tokens[1].type,
              SelectorLexer::TokenType::HASH);

    EXPECT_EQ(tokens[2].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(std::string(tokens[2].ptr, tokens[2].len),
              "main");

    EXPECT_EQ(tokens[3].type,
              SelectorLexer::TokenType::DOT);

    EXPECT_EQ(tokens[4].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(std::string(tokens[4].ptr, tokens[4].len),
              "product");
}


TEST(SelectorLexer, Child)
{
    SelectorLexer lexer;

    std::string input = "div>p";
    lexer.query(input);

    const auto* tokens = lexer.get_tokens();

    ASSERT_EQ(lexer.get_count(), 3);

    EXPECT_EQ(tokens[0].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(tokens[1].type,
              SelectorLexer::TokenType::GREATER_THAN);

    EXPECT_EQ(tokens[2].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(std::string(tokens[2].ptr, tokens[2].len),
              "p");
}


TEST(SelectorLexer, ChildWithSpaces)
{
    SelectorLexer lexer;

    std::string input = "div > p";
    lexer.query(input);

    const auto* tokens = lexer.get_tokens();

    ASSERT_EQ(lexer.get_count(), 3);

    EXPECT_EQ(tokens[0].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(tokens[1].type,
              SelectorLexer::TokenType::GREATER_THAN);

    EXPECT_EQ(tokens[2].type,
              SelectorLexer::TokenType::IDENT);
}


TEST(SelectorLexer, Descendant)
{
    SelectorLexer lexer;

    std::string input = "div p";
    lexer.query(input);

    const auto* tokens = lexer.get_tokens();

    ASSERT_EQ(lexer.get_count(), 2);

    EXPECT_EQ(tokens[0].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(tokens[1].type,
              SelectorLexer::TokenType::IDENT);

    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(std::string(tokens[1].ptr, tokens[1].len),
              "p");
}


TEST(SelectorLexer, ComplexSelector)
{
    SelectorLexer lexer;

    std::string input = "div.product>p.title";
    lexer.query(input);

    const auto* tokens = lexer.get_tokens();

    ASSERT_EQ(lexer.get_count(), 7);

    EXPECT_EQ(tokens[0].type,
              SelectorLexer::TokenType::IDENT);
    EXPECT_EQ(std::string(tokens[0].ptr, tokens[0].len),
              "div");

    EXPECT_EQ(tokens[1].type,
              SelectorLexer::TokenType::DOT);

    EXPECT_EQ(tokens[2].type,
              SelectorLexer::TokenType::IDENT);
    EXPECT_EQ(std::string(tokens[2].ptr, tokens[2].len),
              "product");

    EXPECT_EQ(tokens[3].type,
              SelectorLexer::TokenType::GREATER_THAN);

    EXPECT_EQ(tokens[4].type,
              SelectorLexer::TokenType::IDENT);
    EXPECT_EQ(std::string(tokens[4].ptr, tokens[4].len),
              "p");

    EXPECT_EQ(tokens[5].type,
              SelectorLexer::TokenType::DOT);

    EXPECT_EQ(tokens[6].type,
              SelectorLexer::TokenType::IDENT);
    EXPECT_EQ(std::string(tokens[6].ptr, tokens[6].len),
              "title");
}