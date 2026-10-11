#include <gtest/gtest.h>
#include "Parser.hpp"
#include "Selector/SelectorLexer.hpp"
#include "Selector/SelectorParser.hpp"

TEST(ParserTag, FindsDiv)
{
    SelectorLexer lexer;
    std::string selector = "div";

    lexer.query(selector);

    SelectorParser selectorParser;
    selectorParser.parsring(lexer);

    Parser parser;

    const char* html =
        "<div>"
        "<p>"
        "<div>"
        "<section>";

    parser.parsring(html, strlen(html), selectorParser);

    EXPECT_EQ(parser.get_tag_match_count(), 2);
}

TEST(ParserTag, DoesNotMatchSimilarTags)
{
    SelectorLexer lexer;
    std::string selector = "div";

    lexer.query(selector);

    SelectorParser selectorParser;
    selectorParser.parsring(lexer);

    Parser parser;

    const char* html =
        "<div>"
        "<divx>"
        "<dive>"
        "<di>";

    parser.parsring(html, strlen(html), selectorParser);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
}

TEST(ParserTag, DoesNotMatchClosingTag)
{
    SelectorLexer lexer;
    std::string selector = "div";

    lexer.query(selector);

    SelectorParser selectorParser;
    selectorParser.parsring(lexer);

    Parser parser;

    const char* html = "<div></div>";

    parser.parsring(html, strlen(html), selectorParser);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
}

TEST(ParserTag, MatchesAcrossChunks)
{
    SelectorLexer lexer;
    std::string selector = "div";

    lexer.query(selector);

    SelectorParser selectorParser;
    selectorParser.parsring(lexer);

    Parser parser;

    const char* chunk1 = "<di";
    const char* chunk2 = "v>";

    parser.parsring(chunk1, 3, selectorParser);
    parser.parsring(chunk2, 2, selectorParser);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
}

TEST(ParserTag, MatchesAtDifferentChunkBoundaries)
{
    SelectorLexer lexer;
    std::string selector = "div";

    lexer.query(selector);

    SelectorParser selectorParser;
    selectorParser.parsring(lexer);

    {
        Parser parser;

        parser.parsring("<", 1, selectorParser);
        parser.parsring("div>", 4, selectorParser);

        EXPECT_EQ(parser.get_tag_match_count(), 1);
    }

    {
        Parser parser;

        parser.parsring("<d", 2, selectorParser);
        parser.parsring("iv>", 3, selectorParser);

        EXPECT_EQ(parser.get_tag_match_count(), 1);
    }

    {
        Parser parser;

        parser.parsring("<di", 3, selectorParser);
        parser.parsring("v>", 2, selectorParser);

        EXPECT_EQ(parser.get_tag_match_count(), 1);
    }

    {
        Parser parser;

        parser.parsring("<div", 4, selectorParser);
        parser.parsring(">", 1, selectorParser);

        EXPECT_EQ(parser.get_tag_match_count(), 1);
    }
}

TEST(ParserTag, MismatchAcrossChunks)
{
    SelectorLexer lexer;
    std::string selector = "div";

    lexer.query(selector);

    SelectorParser selectorParser;
    selectorParser.parsring(lexer);

    Parser parser;

    parser.parsring("<s", 2, selectorParser);
    parser.parsring("ection>", 7, selectorParser);

    EXPECT_EQ(parser.get_tag_match_count(), 0);
}

TEST(ParserTest, MatchesId)
{
    SelectorLexer lexer;

    std::string input = "div#good";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div id="good">)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 1);
    EXPECT_EQ(parser.get_attr_val_match_count(), 1);
}


TEST(ParserTest, DoesNotMatchWrongId)
{
    SelectorLexer lexer;

    std::string input = "div#good";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div id="bad">)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 1);
    EXPECT_EQ(parser.get_attr_val_match_count(), 0);
}


TEST(ParserTest, DoesNotMatchIdPrefix)
{
    SelectorLexer lexer;

    std::string input = "div#good";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div id="goodxxx">)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 1);
    EXPECT_EQ(parser.get_attr_val_match_count(), 0);
}


TEST(ParserTest, MatchesClass)
{
    SelectorLexer lexer;

    std::string input = "div.product";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div class="product">)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 1);
    EXPECT_EQ(parser.get_attr_val_match_count(), 1);
}


TEST(ParserTest, DoesNotMatchWrongClass)
{
    SelectorLexer lexer;

    std::string input = "div.product";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div class="other">)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 1);
    EXPECT_EQ(parser.get_attr_val_match_count(), 0);
}


TEST(ParserTest, MatchesIdAndClass)
{
    SelectorLexer lexer;

    std::string input = "div#good.product";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div id="good" class="product">)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 2);
    EXPECT_EQ(parser.get_attr_val_match_count(), 2);
}


TEST(ParserTest, MatchesClassAndIdReversed)
{
    SelectorLexer lexer;

    std::string input = "div#good.product";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div class="product" id="good">)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 2);
    EXPECT_EQ(parser.get_attr_val_match_count(), 2);
}


TEST(ParserTest, DoesNotMatchWrongIdWithCorrectClass)
{
    SelectorLexer lexer;

    std::string input = "div#good.product";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div class="product" id="bad">)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 2);
    EXPECT_EQ(parser.get_attr_val_match_count(), 1);
}


TEST(ParserTest, DoesNotMatchWrongClassWithCorrectId)
{
    SelectorLexer lexer;

    std::string input = "div#good.product";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div id="good" class="other">)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 2);
    EXPECT_EQ(parser.get_attr_val_match_count(), 1);
}


TEST(ParserTest, IgnoresUnknownAttributes)
{
    SelectorLexer lexer;

    std::string input = "div#good";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div title="hello" id="good">)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 1);
    EXPECT_EQ(parser.get_attr_val_match_count(), 1);
}


TEST(ParserTest, DoesNotConfuseIdWithSimilarAttribute)
{
    SelectorLexer lexer;

    std::string input = "div#good";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div idk="good" id="good">)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 1);
    EXPECT_EQ(parser.get_attr_val_match_count(), 1);
}


TEST(ParserTest, SupportsSingleQuotes)
{
    SelectorLexer lexer;

    std::string input = "div#good";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html = R"(<div id='good'>)";
    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 1);
    EXPECT_EQ(parser.get_attr_val_match_count(), 1);
}


TEST(ParserTest, MatchesAcrossChunks)
{
    SelectorLexer lexer;

    std::string input = "div#good";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *chunk1 = R"(<div id="go)";
    const char *chunk2 = R"(od">)";

    parser.parsring(chunk1, strlen(chunk1), selector);
    parser.parsring(chunk2, strlen(chunk2), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 1);
    EXPECT_EQ(parser.get_attr_val_match_count(), 1);
}


TEST(ParserTest, WrongValueAcrossChunks)
{
    SelectorLexer lexer;

    std::string input = "div#good";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *chunk1 = R"(<div id="ba)";
    const char *chunk2 = R"(d">)";

    parser.parsring(chunk1, strlen(chunk1), selector);
    parser.parsring(chunk2, strlen(chunk2), selector);

    EXPECT_EQ(parser.get_tag_match_count(), 1);
    EXPECT_EQ(parser.get_attr_match_count(), 1);
    EXPECT_EQ(parser.get_attr_val_match_count(), 0);
}

TEST(ParserTest, FullSelectorMatches)
{
    SelectorLexer lexer;

    std::string input = "div#good.product";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html =
        R"(<div id="good" class="product">)";

    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_selector_match_count(), 1);
}

TEST(ParserTest, FullSelectorDoesNotMatchWrongId)
{
    SelectorLexer lexer;

    std::string input = "div#good.product";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html =
        R"(<div id="bad" class="product">)";

    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_selector_match_count(), 0);
}

TEST(ParserTest, WithBadChild)
{
    SelectorLexer lexer;

    std::string input = "div > p";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html =
        R"(<div id="bad" class="product"> <a> sup baby </a> <div/>)";

    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_selector_match_count(), 1);
}

TEST(ParserTest, WithGoodChild)
{
    SelectorLexer lexer;

    std::string input = "div > p";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html =
        R"(<div id="bad" class="product"> <p> sup baby </p> <div/>)";

    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_selector_match_count(), 2);
}

TEST(ParserTest, WithNotGoodChild)
{
    SelectorLexer lexer;

    std::string input = "div > p";
    lexer.query(input);

    SelectorParser selector;
    selector.parsring(lexer);

    Parser parser;

    const char *html =
        R"(<div><div><p></p></div></div>)";

    parser.parsring(html, strlen(html), selector);

    EXPECT_EQ(parser.get_selector_match_count(), 1);
}