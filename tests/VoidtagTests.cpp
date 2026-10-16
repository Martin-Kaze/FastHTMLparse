#include <gtest/gtest.h>
#include "Parser.hpp"
#include "Selector/SelectorLexer.hpp"
#include "Selector/SelectorParser.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace
{
    struct Out
    {
        size_t tags = 0, attrs = 0, attr_vals = 0, selectors = 0;
        std::vector<std::string> texts;
    };

    void collect(std::string_view t, void *ctx)
    {
        static_cast<std::vector<std::string> *>(ctx)->emplace_back(t);
    }

    // chunk = 0 -> whole html in one call, otherwise fed in pieces of `chunk` bytes
    Out run(const char *selector_text, const char *html, size_t chunk = 0, bool callback = true)
    {
        SelectorLexer lexer;
        std::string input = selector_text;
        lexer.query(input);

        SelectorParser selector;
        selector.parsring(lexer);

        Parser parser;
        Out o;
        if (callback)
            parser.set_match_callback(collect, &o.texts);

        const size_t n = std::strlen(html);
        if (chunk == 0)
            chunk = n ? n : 1;
        for (size_t off = 0; off < n; off += chunk)
            parser.parsring(html + off, std::min(chunk, n - off), selector);

        o.tags = parser.get_tag_match_count();
        o.attrs = parser.get_attr_match_count();
        o.attr_vals = parser.get_attr_val_match_count();
        o.selectors = parser.get_selector_match_count();
        return o;
    }

    using V = std::vector<std::string>;
    V L(std::initializer_list<std::string> l) { return V(l); }

    void ChunkInvariant(const char *selector, const char *html)
    {
        const Out whole = run(selector, html);
        for (size_t c : {1, 2, 3, 5, 8, 13})
        {
            SCOPED_TRACE(std::string(selector) + " chunk=" + std::to_string(c));
            const Out p = run(selector, html, c);
            EXPECT_EQ(whole.tags, p.tags);
            EXPECT_EQ(whole.attrs, p.attrs);
            EXPECT_EQ(whole.attr_vals, p.attr_vals);
            EXPECT_EQ(whole.selectors, p.selectors);
            EXPECT_EQ(whole.texts, p.texts);
        }
    }

    const char *kFullDoc =
        "<!DOCTYPE html>\n<html>\n<head>\n  <meta charset=\"utf-8\">\n  <title>T</title>\n"
        "  <link rel=\"stylesheet\" href=\"a.css\">\n</head>\n<body>\n"
        "  <p class=\"note\">Hi</p>\n  <img src=\"x.png\">\n  <p>Other</p>\n</body>\n</html>";

    const char *kVoid[] = {"area", "base", "br", "col", "embed", "hr", "img",
                           "input", "link", "meta", "source", "track", "wbr"};
}

// ------------------------------------------------------------- basics

TEST(ParserExtra, EmptyInput)
{
    Out r = run("div", "");
    EXPECT_EQ(r.tags, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(ParserExtra, PlainTextOnly)
{
    Out r = run("div", "hello world");
    EXPECT_EQ(r.tags, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(ParserExtra, TwoSiblingDivs)
{
    Out r = run("div", "<div>a</div><div>b</div>");
    EXPECT_EQ(r.tags, 2u);
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"a", "b"}));
}

TEST(ParserExtra, NonMatchingTag)
{
    Out r = run("div", "<span>a</span><p>b</p>");
    EXPECT_EQ(r.tags, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(ParserExtra, TagNameMustMatchExactly)
{
    Out r = run("div", "<divx>1</divx><di>2</di><div>3</div>");
    EXPECT_EQ(r.tags, 1u);
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"3"}));
}

TEST(ParserExtra, SingleLetterTag)
{
    Out r = run("p", "<p>a</p><pre>b</pre><p>c</p>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"a", "c"}));
}

TEST(ParserExtra, NestedTextWithoutMarkup)
{
    Out r = run("div.a", "<div class=\"a\">Hello <b>big</b> world</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"Hello big world"}));
}

TEST(ParserExtra, TextOutsideMatchNotCollected)
{
    Out r = run("div", "before<div>in</div>after");
    EXPECT_EQ(r.texts, L({"in"}));
}

TEST(ParserExtra, EmptyElementEmptyText)
{
    Out r = run("div", "<div></div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({""}));
}

TEST(ParserExtra, WorksWithoutCallback)
{
    Out r = run("div", "<div>a</div><div>b</div>", 0, false);
    EXPECT_EQ(r.tags, 2u);
    EXPECT_EQ(r.selectors, 2u);
}

// --------------------------------------------------------- attributes

TEST(ParserExtra, ClassMatch)
{
    Out r = run("div.a", "<div class=\"a\">x</div>");
    EXPECT_EQ(r.tags, 1u);
    EXPECT_EQ(r.attrs, 1u);
    EXPECT_EQ(r.attr_vals, 1u);
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(ParserExtra, ClassMismatch)
{
    Out r = run("div.a", "<div class=\"b\">x</div>");
    EXPECT_EQ(r.tags, 1u);
    EXPECT_EQ(r.attr_vals, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(ParserExtra, ClassMustMatchWholeValue)
{
    Out r = run("div.a", "<div class=\"ab\">1</div><div class=\"a\">2</div><div class=\"\">3</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"2"}));
}

TEST(ParserExtra, IdMatch)
{
    Out r = run("div#main", "<div id=\"main\" class=\"card\">x</div>");
    EXPECT_EQ(r.tags, 1u);
    EXPECT_EQ(r.attrs, 2u);
    EXPECT_EQ(r.attr_vals, 1u);
    EXPECT_EQ(r.selectors, 1u);
}

TEST(ParserExtra, IdMustMatchWholeValue)
{
    Out r = run("div#main", "<div id=\"mainx\">1</div><div id=\"mai\">2</div><div id=\"main\">3</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"3"}));
}

TEST(ParserExtra, IdAndClassBothRequired)
{
    EXPECT_EQ(run("div#m.c", "<div id=\"m\" class=\"c\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div#m.c", "<div class=\"c\" id=\"m\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div#m.c", "<div id=\"m\" class=\"z\">x</div>").selectors, 0u);
    EXPECT_EQ(run("div#m.c", "<div id=\"z\" class=\"c\">x</div>").selectors, 0u);
    EXPECT_EQ(run("div#m.c", "<div id=\"m\">x</div>").selectors, 0u);
    EXPECT_EQ(run("div#m.c", "<div class=\"c\">x</div>").selectors, 0u);
}

TEST(ParserExtra, SingleQuotedValue)
{
    Out r = run("div.a", "<div class='a'>x</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(ParserExtra, OtherQuoteInsideValue)
{
    EXPECT_EQ(run("div.a", "<div title='say \"hi\"' class=\"a\">x</div>").selectors, 1u);
}

TEST(ParserExtra, GreaterThanInsideQuotedValue)
{
    Out r = run("div.a", "<div title=\"a>b\" class=\"a\">x</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(ParserExtra, OtherAttributesBeforeClass)
{
    EXPECT_EQ(run("div.a", "<div data-id=\"5\" title=\"t\" class=\"a\">x</div>").selectors, 1u);
}

TEST(ParserExtra, LookalikeAttributeNamesIgnored)
{
    Out r = run("div.a", "<div classy=\"a\" id2=\"a\" data-id=\"a\" charset=\"a\" i=\"a\" c=\"a\">x</div>");
    EXPECT_EQ(r.attrs, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(ParserExtra, AttributesOnNonSelectedTagStillCounted)
{
    Out r = run("div", "<span id=\"a\" class=\"b\">x</span>");
    EXPECT_EQ(r.tags, 0u);
    EXPECT_EQ(r.attrs, 2u);
    EXPECT_EQ(r.attr_vals, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(ParserExtra, ExtraSpacesBetweenAttributes)
{
    EXPECT_EQ(run("div.a", "<div   data-x=\"1\"    class=\"a\"  >x</div>").selectors, 1u);
}

TEST(ParserExtra, ClassOnWrongTag)
{
    EXPECT_EQ(run("div.a", "<span class=\"a\">x</span>").selectors, 0u);
}

// ---------------------------------------------------------- void tags

TEST(ParserExtra, VoidTagsSkippedInsideMatch)
{
    Out r = run("div.x",
                "<div class=\"x\"><br><img src=\"a.png\"><input type=\"text\"><hr><span>hi</span></div>"
                "<div class=\"x\">two</div>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"hi", "two"}));
}

TEST(ParserExtra, AllVoidTagsAllForms)
{
    for (const char *t : kVoid)
    {
        std::string lower = t, up = t;
        for (char &c : up)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        const std::string forms[] = {
            "<" + lower + ">",
            "<" + lower + "/>",
            "<" + lower + " />",
            "<" + lower + " data=\"1\">",
            "<" + up + ">",
            "<" + up + " data=\"1\"/>",
        };

        for (const std::string &f : forms)
        {
            SCOPED_TRACE(f);
            std::string html = "<div class=\"x\">" + f + "ok</div><div class=\"x\">next</div>";
            Out r = run("div.x", html.c_str());
            EXPECT_EQ(r.selectors, 2u);
            EXPECT_EQ(r.texts, L({"ok", "next"}));
        }
    }
}

TEST(ParserExtra, ManyVoidTagsInARow)
{
    std::string html = "<div class=\"x\">";
    for (int i = 0; i < 2000; ++i)
        html += "<br><img src=\"a\"><hr/>";
    html += "end</div>";

    Out r = run("div.x", html.c_str());
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"end"}));
}

TEST(ParserExtra, LookalikeNonVoidTagsAreNotSkipped)
{
    // colgroup, linkx, b, inputs, tracks must open/close like normal tags
    Out r = run("div.x",
                "<div class=\"x\"><colgroup><col></colgroup><linkx>t</linkx><b>B</b>"
                "<inputs>i</inputs><tracks>k</tracks></div><div class=\"x\">next</div>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"tBik", "next"}));
}

TEST(ParserExtra, VoidInsideListItems)
{
    Out r = run("ul > li", "<ul><li>a<br>b</li><li>c</li></ul>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"ab", "c"}));
}

TEST(ParserExtra, OnlyVoidInsideElement)
{
    Out r = run("p", "<p><br></p><p>x</p>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"", "x"}));
}

// EXPECTED TO FAIL for now: selecting a void tag starts "collecting" and the capture only
// ends at the parent's closing tag, so the callback gets "b". Fix: if current_tag_is_void()
// in finish_tag_set_state, call on_match with empty text immediately instead of collecting.
TEST(ParserExtra, VoidTagAsTargetDoesNotCaptureParentTail)
{
    Out r = run("img", "<div>a<img>b</div>");
    EXPECT_EQ(r.selectors, 1u);
    for (const std::string &t : r.texts)
        EXPECT_TRUE(t.empty()) << "callback got: '" << t << "'";
}

// ------------------------------------------------------------ nesting

TEST(ParserExtra, WithNotGoodChild)
{
    Out r = run("div > p", "<div><div><p></p></div></div>");
    EXPECT_EQ(r.selectors, 1u);
}

TEST(ParserExtra, ChildOnlyInsideParent)
{
    Out r = run("ul > li", "<ul><li>a</li><li>b</li></ul><li>c</li>");
    EXPECT_EQ(r.tags, 3u); // ul + 2 li, the outer li is not a candidate
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"a", "b"}));
}

TEST(ParserExtra, ChildNotMatchedThroughGrandparent)
{
    Out r = run("ul > li", "<ul><div><li>a</li></div></ul>");
    EXPECT_EQ(r.selectors, 0u);
}

TEST(ParserExtra, SiblingListsAfterEachOther)
{
    Out r = run("ul > li", "<ul><li>a</li></ul><ol><li>b</li></ol><ul><li>c</li></ul>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"a", "c"}));
}

TEST(ParserExtra, ThreeLevelSelector)
{
    Out r = run("div > ul > li", "<div><ul><li>a</li></ul></div><ul><li>b</li></ul>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"a"}));
}

TEST(ParserExtra, ParentWithClass)
{
    Out r = run("div.box > p", "<div class=\"box\"><p>in</p></div><div class=\"other\"><p>out</p></div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"in"}));
}

TEST(ParserExtra, DeepNestingStaysBalanced)
{
    std::string html;
    for (int i = 0; i < 20; ++i)
        html += "<section>";
    html += "<div class=\"x\">deep</div>";
    for (int i = 0; i < 20; ++i)
        html += "</section>";
    html += "<div class=\"x\">after</div>";

    Out r = run("div.x", html.c_str());
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"deep", "after"}));
}

TEST(ParserExtra, FormattedListWithWhitespaceAndVoid)
{
    Out r = run("ul > li", "<ul>\n  <li>a</li>\n  <li>b<br>c</li>\n</ul>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"a", "bc"}));
}

// ------------------------------------------------------- full document

TEST(ParserExtra, DocTitle)
{
    Out r = run("title", kFullDoc);
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"T"}));
}

TEST(ParserExtra, DocThreeLevelPath)
{
    Out r = run("html > head > title", kFullDoc);
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"T"}));
}

TEST(ParserExtra, DocClassedParagraph)
{
    Out r = run("p.note", kFullDoc);
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"Hi"}));
}

TEST(ParserExtra, DocAllParagraphsInBody)
{
    Out r = run("body > p", kFullDoc);
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"Hi", "Other"}));
}

TEST(ParserExtra, DocWrongPath)
{
    EXPECT_EQ(run("head > p", kFullDoc).selectors, 0u);
    EXPECT_EQ(run("body > title", kFullDoc).selectors, 0u);
}

// ------------------------------------------------------------ chunking

TEST(ParserExtra, SameResultForAnyChunkSize)
{
    ChunkInvariant("div", "<div>a</div><div>b</div>");
    ChunkInvariant("div.a", "<div class=\"a\">Hello <b>big</b> world</div>");
    ChunkInvariant("div#main", "<div id=\"main\" class=\"card\">x</div>");
    ChunkInvariant("div.a", "<div title=\"a>b\" class='a'>x</div>");
    ChunkInvariant("div.x", "<div class=\"x\"><br><img src=\"a.png\"><input type=\"text\"><span>hi</span></div>");
    ChunkInvariant("div.x", "<div class=\"x\"><colgroup><col></colgroup><b>B</b></div>");
    ChunkInvariant("ul > li", "<ul><li>a</li><li>b</li></ul><li>c</li>");
    ChunkInvariant("html > head > title", kFullDoc);
    ChunkInvariant("body > p", kFullDoc);
    ChunkInvariant("p.note", kFullDoc);
}

// ------------------------------------------------- known issues (disabled)
// Run with --gtest_also_run_disabled_tests. Each one currently fails.

// boolean attribute before class: attribute_position is not reset on ' ' in ATTRIBUTE_NAME
TEST(ParserExtra, DISABLED_BooleanAttributeBeforeClass)
{
    EXPECT_EQ(run("button.a", "<button disabled class=\"a\">x</button>").selectors, 1u);
}

// only ' ' counts as whitespace inside a tag
TEST(ParserExtra, DISABLED_NewlineAndTabInsideTag)
{
    EXPECT_EQ(run("div.a", "<div\nclass=\"a\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div.a", "<div\tclass=\"a\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div.a", "<div class=\"a\"\n>x</div>").selectors, 1u);
}

// class is compared against the whole attribute value
TEST(ParserExtra, DISABLED_MultipleClassesInOneAttribute)
{
    EXPECT_EQ(run("div.a", "<div class=\"a b\">x</div><div class=\"b a\">y</div>").selectors, 2u);
}

// void tags are lowercased for matching, selector tag names are not
TEST(ParserExtra, DISABLED_UppercaseTagNames)
{
    EXPECT_EQ(run("div", "<DIV>x</DIV>").selectors, 1u);
}

// ATTRIBUTE_VALUE_START waits for a quote and swallows '>'
TEST(ParserExtra, DISABLED_UnquotedAttributeValue)
{
    EXPECT_EQ(run("div.a", "<div class=a>x</div><div class=a>y</div>").selectors, 2u);
}

// comments are parsed like tags
TEST(ParserExtra, DISABLED_CommentsIgnored)
{
    Out r = run("div", "<!-- <div>fake</div> --><div>real</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"real"}));
}

// script content is parsed like markup
TEST(ParserExtra, DISABLED_ScriptContentIsRawText)
{
    Out r = run("div", "<script>if (a<b) { x = \"<div>\"; }</script><div>real</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"real"}));
}