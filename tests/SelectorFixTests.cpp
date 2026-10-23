#include <gtest/gtest.h>

#include "Parser.hpp"
#include "Selector/SelectorLexer.hpp"
#include "Selector/SelectorParser.hpp"

#include <string>
#include <vector>

namespace
{
    std::string str(const SelectorParser::StringView &v) { return std::string(v.ptr ? v.ptr : "", v.length); }

    size_t matches(const SelectorParser &sel, const std::string &html)
    {
        SelectorParser copy = sel;
        Parser parser;
        parser.parsring(html.data(), html.size(), copy);
        return parser.get_selector_match_count();
    }

    SelectorParser compile(const std::string &text)
    {
        SelectorLexer lexer;
        lexer.query(text);
        SelectorParser parser;
        parser.parsring(lexer);
        return parser; // copy re-points the parts into the copy's own storage
    }
}

// ------------------------------------------------- whitespace = descendant

TEST(SelectorFix, SpaceBeforeClassStartsNewPart)
{
    SelectorParser p = compile("div .x");
    ASSERT_EQ(p.get_count(), 2u);
    EXPECT_EQ(str(p.get_parts()[0].tag), "div");
    EXPECT_EQ(p.get_parts()[1].tag.length, 0u);
    EXPECT_EQ(str(p.get_parts()[1].class_name), "x");
    EXPECT_EQ(p.get_parts()[1].relation, SelectorParser::Relation::DESCENDANT);
}

TEST(SelectorFix, SpaceBeforeIdAndBetweenClasses)
{
    SelectorParser a = compile("#a .b");
    ASSERT_EQ(a.get_count(), 2u);
    EXPECT_EQ(str(a.get_parts()[0].id), "a");
    EXPECT_EQ(str(a.get_parts()[1].class_name), "b");

    SelectorParser b = compile(".a .b");
    ASSERT_EQ(b.get_count(), 2u);
    EXPECT_EQ(b.get_parts()[1].relation, SelectorParser::Relation::DESCENDANT);
}

TEST(SelectorFix, ChildWithSpacesAroundAndTabs)
{
    SelectorParser a = compile("div  >  .x");
    ASSERT_EQ(a.get_count(), 2u);
    EXPECT_EQ(a.get_parts()[1].relation, SelectorParser::Relation::CHILD);

    SelectorParser b = compile("div\t>\np");
    ASSERT_EQ(b.get_count(), 2u);
    EXPECT_EQ(str(b.get_parts()[1].tag), "p");
    EXPECT_EQ(b.get_parts()[1].relation, SelectorParser::Relation::CHILD);
}

TEST(SelectorFix, DescendantClassMatching)
{
    SelectorParser p = compile("div .x");
    EXPECT_EQ(matches(p, "<div class=\"x\">t</div>"), 0u);
    EXPECT_EQ(matches(p, "<div><b class=\"x\">t</b></div>"), 1u);
    EXPECT_EQ(matches(compile(".a .b"), "<i class=\"a\"><i class=\"b\"></i></i>"), 1u);
}

// --------------------------------------------------------------- case

TEST(SelectorFix, TagNamesLowercasedValuesKept)
{
    SelectorParser p = compile("DIV#Main.Box");
    EXPECT_EQ(str(p.get_parts()[0].tag), "div");
    EXPECT_EQ(str(p.get_parts()[0].id), "Main");
    EXPECT_EQ(str(p.get_parts()[0].class_name), "Box");
    EXPECT_EQ(matches(compile("DIV"), "<div>x</div><DIV>y</DIV>"), 2u);
}

// ---------------------------------------------------------- reuse / reset

TEST(SelectorFix, ReusedLexerAndParserStartClean)
{
    SelectorLexer lexer;
    SelectorParser parser;

    lexer.query("div a");
    parser.parsring(lexer);
    EXPECT_EQ(parser.get_desc_mask(), 2u);

    lexer.query("div > a");
    EXPECT_EQ(lexer.get_count(), 3u);
    parser.parsring(lexer);
    EXPECT_EQ(parser.get_count(), 2u);
    EXPECT_EQ(parser.get_child_mask(), 2u);
    EXPECT_EQ(parser.get_desc_mask(), 0u);
    EXPECT_EQ(matches(parser, "<div><p><a>x</a></p></div>"), 0u);
}

TEST(SelectorFix, ManyQueriesOnOneLexer)
{
    SelectorLexer lexer;
    for (int i = 0; i < 100; ++i)
        lexer.query("div.a > p#b");
    EXPECT_TRUE(lexer.ok());
    EXPECT_EQ(lexer.get_count(), 7u);
}

// ------------------------------------------------------------- ownership

TEST(SelectorFix, SelectorTextMayChangeAfterParsing)
{
    std::string text = "div.x";
    SelectorLexer lexer;
    lexer.query(text);
    text = "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz";
    SelectorParser parser;
    parser.parsring(lexer);
    EXPECT_EQ(str(parser.get_parts()[0].tag), "div");
    EXPECT_EQ(str(parser.get_parts()[0].class_name), "x");
}

TEST(SelectorFix, ParserOutlivesLexer)
{
    SelectorParser parser;
    {
        SelectorLexer lexer;
        lexer.query(std::string("p.note"));
        parser.parsring(lexer);
    }
    EXPECT_EQ(matches(parser, "<p class=\"note\">x</p>"), 1u);
}

TEST(SelectorFix, CopiesAreIndependent)
{
    SelectorParser a = compile("ul > li.x");
    SelectorParser b = a;
    a = compile("span");
    EXPECT_EQ(str(b.get_parts()[1].tag), "li");
    EXPECT_EQ(str(b.get_parts()[1].class_name), "x");
    EXPECT_EQ(matches(b, "<ul><li class=\"x\">1</li></ul>"), 1u);
    EXPECT_EQ(matches(a, "<span>1</span>"), 1u);
}

// ------------------------------------------------------------ validation

TEST(SelectorFix, InvalidSelectorsAreRejected)
{
    const char *bad[] = {"", "   ", "div#", "div.", "#", ".", "div >", "> div", "div > > a",
                         ".a.b", "#a#b", "*", "a:hover", "a,b", "[x]", "a+b", "a~b", "div# x", "div. x"};
    for (const char *s : bad)
    {
        SCOPED_TRACE(std::string("selector: '") + s + "'");
        SelectorLexer lexer;
        lexer.query(s);
        SelectorParser parser;
        EXPECT_FALSE(parser.parsring(lexer));
        EXPECT_FALSE(parser.ok());
        EXPECT_FALSE(parser.error().empty());
        EXPECT_EQ(parser.get_count(), 0u);
        EXPECT_EQ(matches(parser, "<div class=\"a b\" id=\"a\"><a>x</a></div>"), 0u);
    }
}

TEST(SelectorFix, ValidSelectorsAreAccepted)
{
    const char *good[] = {"div", "h1", "my-el", "my_el", "#a", ".b", "div#a.b", "div.b#a", "a > b", "a b",
                          "a>b>c", "  div  ", "div\t.x"};
    for (const char *s : good)
    {
        SCOPED_TRACE(std::string("selector: '") + s + "'");
        SelectorLexer lexer;
        lexer.query(s);
        SelectorParser parser;
        EXPECT_TRUE(parser.parsring(lexer));
        EXPECT_TRUE(parser.ok());
    }
}

TEST(SelectorFix, PartAndTokenLimits)
{
    std::string sixteen, seventeen;
    for (int i = 0; i < 16; ++i)
        sixteen += "a ";
    seventeen = sixteen + "a";
    EXPECT_TRUE(compile(sixteen).ok());
    EXPECT_FALSE(compile(seventeen).ok());

    std::string long_sel; // 9 parts of "a.b" joined by '>' = 35 tokens
    for (int i = 0; i < 9; ++i)
        long_sel += (i ? ">" : "") + std::string("a.b");
    SelectorLexer lexer;
    EXPECT_FALSE(lexer.query(long_sel));
    EXPECT_FALSE(compile(long_sel).ok());
}
