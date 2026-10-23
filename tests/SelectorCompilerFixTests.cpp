#include <gtest/gtest.h>

#include "engine.hpp"
#include "selector_compiler.hpp"
#include "selector_lexer.hpp"

#include <memory>
#include <string>
#include <vector>

using fasthtml::detail::Engine;
using fasthtml::detail::SelectorCompiler;
using fasthtml::detail::SelectorLexer;
using Relation = SelectorCompiler::Relation;

namespace
{
    std::string str(const SelectorCompiler::Slice &v) { return std::string(v.view()); }

    size_t matches(const SelectorCompiler &sel, const std::string &html)
    {
        auto engine = std::make_unique<Engine>(); // ~40 KB, keep it off the stack
        engine->feed(html.data(), html.size(), sel);
        return engine->matches();
    }

    SelectorCompiler compile(const std::string &text)
    {
        SelectorCompiler c;
        c.compile(text);
        return c; // the copy re-points the parts into its own storage
    }
}

// ------------------------------------------------- whitespace = descendant

TEST(SelectorCompilerFix, SpaceBeforeClassStartsNewPart)
{
    SelectorCompiler p = compile("div .x");
    ASSERT_EQ(p.part_count(), 2u);
    EXPECT_EQ(str(p.parts()[0].tag), "div");
    EXPECT_EQ(p.parts()[1].tag.length, 0u);
    EXPECT_EQ(str(p.parts()[1].class_name), "x");
    EXPECT_EQ(p.parts()[1].relation, Relation::descendant);
}

TEST(SelectorCompilerFix, SpaceBeforeIdAndBetweenClasses)
{
    SelectorCompiler a = compile("#a .b");
    ASSERT_EQ(a.part_count(), 2u);
    EXPECT_EQ(str(a.parts()[0].id), "a");
    EXPECT_EQ(str(a.parts()[1].class_name), "b");

    SelectorCompiler b = compile(".a .b");
    ASSERT_EQ(b.part_count(), 2u);
    EXPECT_EQ(b.parts()[1].relation, Relation::descendant);
}

TEST(SelectorCompilerFix, ChildWithSpacesAroundAndTabs)
{
    SelectorCompiler a = compile("div  >  .x");
    ASSERT_EQ(a.part_count(), 2u);
    EXPECT_EQ(a.parts()[1].relation, Relation::child);

    SelectorCompiler b = compile("div\t>\np");
    ASSERT_EQ(b.part_count(), 2u);
    EXPECT_EQ(str(b.parts()[1].tag), "p");
    EXPECT_EQ(b.parts()[1].relation, Relation::child);
}

TEST(SelectorCompilerFix, DescendantClassMatching)
{
    SelectorCompiler p = compile("div .x");
    EXPECT_EQ(matches(p, "<div class=\"x\">t</div>"), 0u);
    EXPECT_EQ(matches(p, "<div><b class=\"x\">t</b></div>"), 1u);
    EXPECT_EQ(matches(compile(".a .b"), "<i class=\"a\"><i class=\"b\"></i></i>"), 1u);
}

// --------------------------------------------------------------- case

TEST(SelectorCompilerFix, TagNamesLowercasedValuesKept)
{
    SelectorCompiler p = compile("DIV#Main.Box");
    EXPECT_EQ(str(p.parts()[0].tag), "div");
    EXPECT_EQ(str(p.parts()[0].id), "Main");
    EXPECT_EQ(str(p.parts()[0].class_name), "Box");
    EXPECT_EQ(matches(compile("DIV"), "<div>x</div><DIV>y</DIV>"), 2u);
}

// ---------------------------------------------------------- reuse / reset

TEST(SelectorCompilerFix, ReusedLexerAndParserStartClean)
{
    SelectorLexer lexer;
    SelectorCompiler parser;

    lexer.tokenize("div a");
    parser.compile(lexer);
    EXPECT_EQ(parser.descendant_mask(), 2u);

    lexer.tokenize("div > a");
    EXPECT_EQ(lexer.token_count(), 3u);
    parser.compile(lexer);
    EXPECT_EQ(parser.part_count(), 2u);
    EXPECT_EQ(parser.child_mask(), 2u);
    EXPECT_EQ(parser.descendant_mask(), 0u);
    EXPECT_EQ(matches(parser, "<div><p><a>x</a></p></div>"), 0u);
}

TEST(SelectorCompilerFix, ManyQueriesOnOneLexer)
{
    SelectorLexer lexer;
    for (int i = 0; i < 100; ++i)
        lexer.tokenize("div.a > p#b");
    EXPECT_TRUE(lexer.ok());
    EXPECT_EQ(lexer.token_count(), 7u);
}

// ------------------------------------------------------------- ownership

TEST(SelectorCompilerFix, SelectorTextMayChangeAfterParsing)
{
    std::string text = "div.x";
    SelectorLexer lexer;
    lexer.tokenize(text);
    text = "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz";
    SelectorCompiler parser;
    parser.compile(lexer);
    EXPECT_EQ(str(parser.parts()[0].tag), "div");
    EXPECT_EQ(str(parser.parts()[0].class_name), "x");
}

TEST(SelectorCompilerFix, ParserOutlivesLexer)
{
    SelectorCompiler parser;
    {
        SelectorLexer lexer;
        lexer.tokenize(std::string("p.note"));
        parser.compile(lexer);
    }
    EXPECT_EQ(matches(parser, "<p class=\"note\">x</p>"), 1u);
}

TEST(SelectorCompilerFix, CopiesAreIndependent)
{
    SelectorCompiler a = compile("ul > li.x");
    SelectorCompiler b = a;
    a = compile("span");
    EXPECT_EQ(str(b.parts()[1].tag), "li");
    EXPECT_EQ(str(b.parts()[1].class_name), "x");
    EXPECT_EQ(matches(b, "<ul><li class=\"x\">1</li></ul>"), 1u);
    EXPECT_EQ(matches(a, "<span>1</span>"), 1u);
}

// ------------------------------------------------------------ validation

TEST(SelectorCompilerFix, InvalidSelectorsAreRejected)
{
    const char *bad[] = {"", "   ", "div#", "div.", "#", ".", "div >", "> div", "div > > a",
                         ".a.b", "#a#b", "*", "a:hover", "a,b", "[x]", "a+b", "a~b", "div# x", "div. x"};
    for (const char *s : bad)
    {
        SCOPED_TRACE(std::string("selector: '") + s + "'");
        SelectorLexer lexer;
        lexer.tokenize(s);
        SelectorCompiler parser;
        EXPECT_FALSE(parser.compile(lexer));
        EXPECT_FALSE(parser.ok());
        EXPECT_FALSE(parser.error().empty());
        EXPECT_EQ(parser.part_count(), 0u);
        EXPECT_EQ(matches(parser, "<div class=\"a b\" id=\"a\"><a>x</a></div>"), 0u);
    }
}

TEST(SelectorCompilerFix, ValidSelectorsAreAccepted)
{
    const char *good[] = {"div", "h1", "my-el", "my_el", "#a", ".b", "div#a.b", "div.b#a", "a > b", "a b",
                          "a>b>c", "  div  ", "div\t.x"};
    for (const char *s : good)
    {
        SCOPED_TRACE(std::string("selector: '") + s + "'");
        SelectorLexer lexer;
        lexer.tokenize(s);
        SelectorCompiler parser;
        EXPECT_TRUE(parser.compile(lexer));
        EXPECT_TRUE(parser.ok());
    }
}

TEST(SelectorCompilerFix, PartAndTokenLimits)
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
    EXPECT_FALSE(lexer.tokenize(long_sel));
    EXPECT_FALSE(compile(long_sel).ok());
}
