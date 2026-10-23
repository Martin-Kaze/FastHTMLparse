#include <gtest/gtest.h>
#include <fasthtml/fasthtml.hpp>

#include <algorithm>
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
        const fasthtml::Selector selector = fasthtml::Selector::compile(selector_text).value();
        fasthtml::Matcher matcher(selector);
        Out o;
        if (callback)
            matcher.on_match(collect, &o.texts);

        const size_t n = std::strlen(html);
        if (chunk == 0)
            chunk = n ? n : 1;
        for (size_t off = 0; off < n; off += chunk)
            matcher.feed(html + off, std::min(chunk, n - off));

        const fasthtml::Stats s = matcher.stats();
        o.tags = s.tag_matches;
        o.attrs = s.attribute_matches;
        o.attr_vals = s.attribute_value_matches;
        o.selectors = s.matches;
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

TEST(MatcherExtra, EmptyInput)
{
    Out r = run("div", "");
    EXPECT_EQ(r.tags, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(MatcherExtra, PlainTextOnly)
{
    Out r = run("div", "hello world");
    EXPECT_EQ(r.tags, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(MatcherExtra, TwoSiblingDivs)
{
    Out r = run("div", "<div>a</div><div>b</div>");
    EXPECT_EQ(r.tags, 2u);
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"a", "b"}));
}

TEST(MatcherExtra, NonMatchingTag)
{
    Out r = run("div", "<span>a</span><p>b</p>");
    EXPECT_EQ(r.tags, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(MatcherExtra, TagNameMustMatchExactly)
{
    Out r = run("div", "<divx>1</divx><di>2</di><div>3</div>");
    EXPECT_EQ(r.tags, 1u);
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"3"}));
}

TEST(MatcherExtra, SingleLetterTag)
{
    Out r = run("p", "<p>a</p><pre>b</pre><p>c</p>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"a", "c"}));
}

TEST(MatcherExtra, NestedTextWithoutMarkup)
{
    Out r = run("div.a", "<div class=\"a\">Hello <b>big</b> world</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"Hello big world"}));
}

TEST(MatcherExtra, TextOutsideMatchNotCollected)
{
    Out r = run("div", "before<div>in</div>after");
    EXPECT_EQ(r.texts, L({"in"}));
}

TEST(MatcherExtra, EmptyElementEmptyText)
{
    Out r = run("div", "<div></div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({""}));
}

TEST(MatcherExtra, WorksWithoutCallback)
{
    Out r = run("div", "<div>a</div><div>b</div>", 0, false);
    EXPECT_EQ(r.tags, 2u);
    EXPECT_EQ(r.selectors, 2u);
}

// --------------------------------------------------------- attributes

TEST(MatcherExtra, ClassMatch)
{
    Out r = run("div.a", "<div class=\"a\">x</div>");
    EXPECT_EQ(r.tags, 1u);
    EXPECT_EQ(r.attrs, 1u);
    EXPECT_EQ(r.attr_vals, 1u);
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(MatcherExtra, ClassMismatch)
{
    Out r = run("div.a", "<div class=\"b\">x</div>");
    EXPECT_EQ(r.tags, 1u);
    EXPECT_EQ(r.attr_vals, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(MatcherExtra, ClassMustMatchWholeValue)
{
    Out r = run("div.a", "<div class=\"ab\">1</div><div class=\"a\">2</div><div class=\"\">3</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"2"}));
}

TEST(MatcherExtra, IdMatch)
{
    Out r = run("div#main", "<div id=\"main\" class=\"card\">x</div>");
    EXPECT_EQ(r.tags, 1u);
    EXPECT_EQ(r.attrs, 2u);
    EXPECT_EQ(r.attr_vals, 1u);
    EXPECT_EQ(r.selectors, 1u);
}

TEST(MatcherExtra, IdMustMatchWholeValue)
{
    Out r = run("div#main", "<div id=\"mainx\">1</div><div id=\"mai\">2</div><div id=\"main\">3</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"3"}));
}

TEST(MatcherExtra, IdAndClassBothRequired)
{
    EXPECT_EQ(run("div#m.c", "<div id=\"m\" class=\"c\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div#m.c", "<div class=\"c\" id=\"m\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div#m.c", "<div id=\"m\" class=\"z\">x</div>").selectors, 0u);
    EXPECT_EQ(run("div#m.c", "<div id=\"z\" class=\"c\">x</div>").selectors, 0u);
    EXPECT_EQ(run("div#m.c", "<div id=\"m\">x</div>").selectors, 0u);
    EXPECT_EQ(run("div#m.c", "<div class=\"c\">x</div>").selectors, 0u);
}

TEST(MatcherExtra, SingleQuotedValue)
{
    Out r = run("div.a", "<div class='a'>x</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(MatcherExtra, OtherQuoteInsideValue)
{
    EXPECT_EQ(run("div.a", "<div title='say \"hi\"' class=\"a\">x</div>").selectors, 1u);
}

TEST(MatcherExtra, GreaterThanInsideQuotedValue)
{
    Out r = run("div.a", "<div title=\"a>b\" class=\"a\">x</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(MatcherExtra, OtherAttributesBeforeClass)
{
    EXPECT_EQ(run("div.a", "<div data-id=\"5\" title=\"t\" class=\"a\">x</div>").selectors, 1u);
}

TEST(MatcherExtra, LookalikeAttributeNamesIgnored)
{
    Out r = run("div.a", "<div classy=\"a\" id2=\"a\" data-id=\"a\" charset=\"a\" i=\"a\" c=\"a\">x</div>");
    EXPECT_EQ(r.attrs, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(MatcherExtra, AttributesOnNonSelectedTagStillCounted)
{
    Out r = run("div", "<span id=\"a\" class=\"b\">x</span>");
    EXPECT_EQ(r.tags, 0u);
    EXPECT_EQ(r.attrs, 2u);
    EXPECT_EQ(r.attr_vals, 0u);
    EXPECT_EQ(r.selectors, 0u);
}

TEST(MatcherExtra, ExtraSpacesBetweenAttributes)
{
    EXPECT_EQ(run("div.a", "<div   data-x=\"1\"    class=\"a\"  >x</div>").selectors, 1u);
}

TEST(MatcherExtra, ClassOnWrongTag)
{
    EXPECT_EQ(run("div.a", "<span class=\"a\">x</span>").selectors, 0u);
}

// ---------------------------------------------------------- void tags

TEST(MatcherExtra, VoidTagsSkippedInsideMatch)
{
    Out r = run("div.x",
                "<div class=\"x\"><br><img src=\"a.png\"><input type=\"text\"><hr><span>hi</span></div>"
                "<div class=\"x\">two</div>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"hi", "two"}));
}

TEST(MatcherExtra, AllVoidTagsAllForms)
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

TEST(MatcherExtra, ManyVoidTagsInARow)
{
    std::string html = "<div class=\"x\">";
    for (int i = 0; i < 2000; ++i)
        html += "<br><img src=\"a\"><hr/>";
    html += "end</div>";

    Out r = run("div.x", html.c_str());
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"end"}));
}

TEST(MatcherExtra, LookalikeNonVoidTagsAreNotSkipped)
{
    // colgroup, linkx, b, inputs, tracks must open/close like normal tags
    Out r = run("div.x",
                "<div class=\"x\"><colgroup><col></colgroup><linkx>t</linkx><b>B</b>"
                "<inputs>i</inputs><tracks>k</tracks></div><div class=\"x\">next</div>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"tBik", "next"}));
}

TEST(MatcherExtra, VoidInsideListItems)
{
    Out r = run("ul > li", "<ul><li>a<br>b</li><li>c</li></ul>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"ab", "c"}));
}

TEST(MatcherExtra, OnlyVoidInsideElement)
{
    Out r = run("p", "<p><br></p><p>x</p>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"", "x"}));
}

// EXPECTED TO FAIL for now: selecting a void tag starts "collecting" and the capture only
// ends at the parent's closing tag, so the callback gets "b". Fix: if current_tag_is_void()
// in finish_tag_set_state, call on_match with empty text immediately instead of collecting.
TEST(MatcherExtra, VoidTagAsTargetDoesNotCaptureParentTail)
{
    Out r = run("img", "<div>a<img>b</div>");
    EXPECT_EQ(r.selectors, 1u);
    for (const std::string &t : r.texts)
        EXPECT_TRUE(t.empty()) << "callback got: '" << t << "'";
}

// ------------------------------------------------------------ nesting

TEST(MatcherExtra, WithNotGoodChild)
{
    Out r = run("div > p", "<div><div><p></p></div></div>");
    EXPECT_EQ(r.selectors, 1u);
}

TEST(MatcherExtra, ChildOnlyInsideParent)
{
    Out r = run("ul > li", "<ul><li>a</li><li>b</li></ul><li>c</li>");
    EXPECT_EQ(r.tags, 3u); // ul + 2 li, the outer li is not a candidate
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"a", "b"}));
}

TEST(MatcherExtra, ChildNotMatchedThroughGrandparent)
{
    Out r = run("ul > li", "<ul><div><li>a</li></div></ul>");
    EXPECT_EQ(r.selectors, 0u);
}

TEST(MatcherExtra, SiblingListsAfterEachOther)
{
    Out r = run("ul > li", "<ul><li>a</li></ul><ol><li>b</li></ol><ul><li>c</li></ul>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"a", "c"}));
}

TEST(MatcherExtra, ThreeLevelSelector)
{
    Out r = run("div > ul > li", "<div><ul><li>a</li></ul></div><ul><li>b</li></ul>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"a"}));
}

TEST(MatcherExtra, ParentWithClass)
{
    Out r = run("div.box > p", "<div class=\"box\"><p>in</p></div><div class=\"other\"><p>out</p></div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"in"}));
}

TEST(MatcherExtra, DeepNestingStaysBalanced)
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

TEST(MatcherExtra, FormattedListWithWhitespaceAndVoid)
{
    Out r = run("ul > li", "<ul>\n  <li>a</li>\n  <li>b<br>c</li>\n</ul>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"a", "bc"}));
}

// ------------------------------------------------------- full document

TEST(MatcherExtra, DocTitle)
{
    Out r = run("title", kFullDoc);
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"T"}));
}

TEST(MatcherExtra, DocThreeLevelPath)
{
    Out r = run("html > head > title", kFullDoc);
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"T"}));
}

TEST(MatcherExtra, DocClassedParagraph)
{
    Out r = run("p.note", kFullDoc);
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"Hi"}));
}

TEST(MatcherExtra, DocAllParagraphsInBody)
{
    Out r = run("body > p", kFullDoc);
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"Hi", "Other"}));
}

TEST(MatcherExtra, DocWrongPath)
{
    EXPECT_EQ(run("head > p", kFullDoc).selectors, 0u);
    EXPECT_EQ(run("body > title", kFullDoc).selectors, 0u);
}

// ------------------------------------------------------------ chunking

TEST(MatcherExtra, SameResultForAnyChunkSize)
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
TEST(MatcherExtra, BooleanAttributeBeforeClass)
{
    EXPECT_EQ(run("button.a", "<button disabled class=\"a\">x</button>").selectors, 1u);
}

// only ' ' counts as whitespace inside a tag
TEST(MatcherExtra, NewlineAndTabInsideTag)
{
    EXPECT_EQ(run("div.a", "<div\nclass=\"a\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div.a", "<div\tclass=\"a\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div.a", "<div class=\"a\"\n>x</div>").selectors, 1u);
}

// class is compared against the whole attribute value
TEST(MatcherExtra, MultipleClassesInOneAttribute)
{
    EXPECT_EQ(run("div.a", "<div class=\"a b\">x</div><div class=\"b a\">y</div>").selectors, 2u);
}

// void tags are lowercased for matching, selector tag names are not
TEST(MatcherExtra, UppercaseTagNames)
{
    EXPECT_EQ(run("div", "<DIV>x</DIV>").selectors, 1u);
}

// ATTRIBUTE_VALUE_START waits for a quote and swallows '>'
TEST(MatcherExtra, UnquotedAttributeValue)
{
    EXPECT_EQ(run("div.a", "<div class=a>x</div><div class=a>y</div>").selectors, 2u);
}

// comments are parsed like tags
TEST(MatcherExtra, CommentsIgnored)
{
    Out r = run("div", "<!-- <div>fake</div> --><div>real</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"real"}));
}

// script content is parsed like markup
TEST(MatcherExtra, ScriptContentIsRawText)
{
    Out r = run("div", "<script>if (a<b) { x = \"<div>\"; }</script><div>real</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"real"}));
}

TEST(MatcherExtra, DescendantSelector)
{
    EXPECT_EQ(run("ul li", "<ul><div><li>x</li></div></ul>").selectors, 1u);
}

// ul > li must NOT match when a div sits between
TEST(MatcherExtra, DescendantNotChild)
{
    EXPECT_EQ(run("ul > li", "<ul><div><li>x</li></div></ul>").selectors, 0u);
}

// no ul above, no match
TEST(MatcherExtra, DescendantNeedsAncestor)
{
    EXPECT_EQ(run("ul li", "<li>x</li>").selectors, 0u);
}

// ancestor closed before li, no match
TEST(MatcherExtra, DescendantAfterAncestorClosed)
{
    EXPECT_EQ(run("ul li", "<ul></ul><li>x</li>").selectors, 0u);
}

// deeper nesting
TEST(MatcherExtra, DescendantDeep)
{
    Out r = run("ul li", "<ul><div><p><li>x</li></p></div></ul>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

// '>' inside a quoted value must not end the tag.
TEST(QuoteSkip, GreaterThanInsideValue)
{
    EXPECT_EQ(run("a", "<a title=\"x><a>y\"></a>").tags, 1u);
}

// A '"' inside a single-quoted value: the search must use the opening quote.
TEST(QuoteSkip, SingleQuoteWithDoubleInside)
{
    EXPECT_EQ(run("a", "<a title='it\"s'><a>").tags, 2u);
}

// Wrong tag: id value is skipped, nothing counted.
TEST(QuoteSkip, WrongTagSkipsValue)
{
    Out o = run("div#main", "<span id=\"main\">");
    EXPECT_EQ(o.tags, 0u);
    EXPECT_EQ(o.attr_vals, 0u);
    EXPECT_EQ(o.selectors, 0u);
}

TEST(QuoteSkip, IdMatches)
{
    Out o = run("div#main", "<div id=\"main\">");
    EXPECT_EQ(o.tags, 1u);
    EXPECT_EQ(o.attr_vals, 1u);
    EXPECT_EQ(o.selectors, 1u);
}

TEST(QuoteSkip, IdDiffers)
{
    Out o = run("div#main", "<div id=\"mai\"><div id=\"mainx\">");
    EXPECT_EQ(o.attr_vals, 0u);
    EXPECT_EQ(o.selectors, 0u);
}

TEST(QuoteSkip, ClassList)
{
    Out o = run("div.b", "<div class=\"a b\">");
    EXPECT_EQ(o.attr_vals, 1u);
    EXPECT_EQ(o.selectors, 1u);
}

// Skipped href (contains '>') followed by a real id.
TEST(QuoteSkip, SkipThenMatch)
{
    Out o = run("div#main", "<div href=\"/x>y\" id=\"main\">");
    EXPECT_EQ(o.attr_vals, 1u);
    EXPECT_EQ(o.selectors, 1u);
}

// Every chunk size, including 1 (quote is the last byte of a chunk).
TEST(QuoteSkip, ChunkInvariant)
{
    ChunkInvariant("a", "<a title=\"x>y\"><a href='q\"r'>t</a></a>");
    ChunkInvariant("div#main", "<div href=\"/x>y\" id=\"main\">hi</div>");
    ChunkInvariant("div.b", "<div class=\"a b\" title=\"z\">x</div>");
    ChunkInvariant("div#main", "<span id=\"main\"><div id=\"main\">t</div></span>");
}

// Full document with a skipped-attribute selector.
TEST(QuoteSkip, FullDoc)
{
    ChunkInvariant("p", kFullDoc);
    ChunkInvariant("p.note", kFullDoc);
}
// id or class with no tags
TEST(Tagless, ClassOnly) { EXPECT_EQ(run(".b", "<div class=\"b\">").selectors, 1u); }
TEST(Tagless, IdOnly)    { EXPECT_EQ(run("#main", "<div id=\"main\">").selectors, 1u); }

// ===================================================== V3 regression tests

// ---------------------------------------------------------- tag names (v3a)

TEST(V3Tag, MixedCaseTagName)
{
    Out r = run("div", "<DiV>x</dIv>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(V3Tag, TagNameSplitAcrossCalls)
{
    Out r = run("div", "<DIV>x</DIV>", 3); // "<DI" | "V>x" | "</D" | "IV>"
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(V3Tag, WhitespaceEndsTagName)
{
    EXPECT_EQ(run("div", "<div\n>x</div>").selectors, 1u);
    EXPECT_EQ(run("div.a", "<div\tclass=\"a\">x</div>").selectors, 1u);
}

TEST(V3Tag, LongTagNameDoesNotBreakParsing)
{
    const std::string t(40, 'q');
    const std::string html = "<" + t + ">in</" + t + "><div>y</div>";
    Out r = run("div", html.c_str());
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"y"}));
}

TEST(V3Tag, LongTagNameKeepsTaglessParts)
{
    const std::string t(40, 'q');
    const std::string html = "<" + t + " class=\"x\">in</" + t + ">";
    EXPECT_EQ(run(".x", html.c_str()).selectors, 1u);
}

TEST(V3Tag, TagNameOf32CharsStillMatches)
{
    const std::string t(32, 'q');
    const std::string html = "<" + t + ">x</" + t + ">";
    Out r = run(t.c_str(), html.c_str());
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(V3Tag, SlashAfterNonVoidTagStillOpensIt)
{
    Out r = run("div", "<div/>x</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(V3Tag, SlashThenAttribute)
{
    EXPECT_EQ(run("div.a", "<div/class=\"a\">x</div>").selectors, 1u);
}

TEST(V3Tag, RawTagNeedsExactName)
{
    Out r = run("div", "<scripts><div>x</div></scripts><scrip><div>y</div></scrip>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"x", "y"}));
}

// ------------------------------------------------- '<' that is not a tag

TEST(V3Open, LessThanSpaceIsText)
{
    Out r = run("div", "<div>a < b</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"a < b"}));
}

TEST(V3Open, LessThanDigitIsText)
{
    EXPECT_EQ(run("div", "<div>x<3</div>").texts, L({"x<3"}));
}

TEST(V3Open, DoubleLessThan)
{
    Out r = run("div", "<div>a<<b>c</b></div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"a<c"}));
}

TEST(V3Open, XmlDeclarationSkipped)
{
    Out r = run("div", "<?xml version=\"1.0\"?><div>x</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(V3Open, LessThanAtEndOfInput)
{
    Out r = run("div", "<div>x</div><");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(V3Open, ChunkInvariant)
{
    ChunkInvariant("div", "<div>a < b<3 x<<i>y</i></div>");
    ChunkInvariant("div", "<?xml version=\"1.0\"?><div>x</div>");
}

// ------------------------------------------------------ attribute names (v3b)

TEST(V3Attr, SpacesAroundEquals)
{
    EXPECT_EQ(run("div#m", "<div id = \"m\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div.a", "<div class =\"a\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div.a", "<div class= \"a\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div.a", "<div class\t=\n\"a\">x</div>").selectors, 1u);
}

TEST(V3Attr, UppercaseAttributeNames)
{
    EXPECT_EQ(run("div#m.c", "<div ID=\"m\" CLASS=\"c\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div#m", "<div iD=\"m\">x</div>").selectors, 1u);
}

TEST(V3Attr, ValuesStayCaseSensitive)
{
    EXPECT_EQ(run("div.a", "<div class=\"A\">x</div>").selectors, 0u);
    EXPECT_EQ(run("div#m", "<div id=\"M\">x</div>").selectors, 0u);
}

TEST(V3Attr, BooleanAttributesAnywhere)
{
    EXPECT_EQ(run("div#m.c", "<div hidden id=\"m\" data-x class=\"c\" open>x</div>").selectors, 1u);
    EXPECT_EQ(run("div", "<div hidden>x</div>").selectors, 1u);
}

TEST(V3Attr, VoidTagWithBooleanAndClass)
{
    Out r = run("input.a", "<input disabled class=\"a\">");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({""}));
}

TEST(V3Attr, SlashAfterValue)
{
    Out r = run("div.a", "<div class=\"a\"/>x</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(V3Attr, EmptyValues)
{
    EXPECT_EQ(run("div.a", "<div id=\"\" class=\"a\">x</div>").selectors, 1u);
    EXPECT_EQ(run("div#m", "<div id=\"\">x</div>").selectors, 0u);
}

TEST(V3Attr, NamelessAttributeDoesNotHang)
{
    EXPECT_EQ(run("div.a", "<div =\"x\" class=\"a\">t</div>").selectors, 1u);
}

TEST(V3Attr, GreaterThanRightAfterEquals)
{
    Out r = run("div#m", "<div id=>x</div><div id=\"m\">y</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"y"}));
}

TEST(V3Attr, UnquotedValues)
{
    Out r = run("div#m.c", "<div id=m class=c>x</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"x"}));
}

TEST(V3Attr, ChunkInvariant)
{
    ChunkInvariant("div#m.c", "<div  DATA-x='1'  ID = \"m\"\n class\t=\"c\" hidden>x</div>");
    ChunkInvariant("div#m.c", "<div id=m class=c>x</div><div id=>y</div>");
    ChunkInvariant("input.a", "<div><input disabled class=\"a\"/><br/>t</div>");
}

// ------------------------------------------------- script / style (v3c)
// These pass now and must still pass after the memchr change in RAWTEXT.

TEST(V3Raw, LessThanInsideScript)
{
    Out r = run("div", "<script>if(a<b){x()}</script><div>t</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"t"}));
}

TEST(V3Raw, TagsInsideScriptIgnored)
{
    EXPECT_EQ(run("div", "<script>var s=\"<div>\";</script><div>t</div>").selectors, 1u);
}

TEST(V3Raw, PartialEndTagInsideScript)
{
    EXPECT_EQ(run("div", "<script>var s=\"</scr\"+\"ipt>\";</script><div>t</div>").selectors, 1u);
}

TEST(V3Raw, DoubleLessThanBeforeEnd)
{
    EXPECT_EQ(run("div", "<script>x<</script><div>t</div>").selectors, 1u);
}

TEST(V3Raw, UppercaseEndTags)
{
    EXPECT_EQ(run("div", "<SCRIPT>a<b</SCRIPT><style>p{}</STYLE><div>t</div>").selectors, 1u);
}

TEST(V3Raw, EmptyAndWithAttributes)
{
    Out r = run("div", "<script></script><script type=\"text/javascript\" src=\"a.js\"></script><div>t</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"t"}));
}

TEST(V3Raw, ManyLessThans)
{
    std::string html = "<script>";
    for (int i = 0; i < 1000; ++i)
        html += "a<b<<c</s ";
    html += "</script><div>t</div>";
    EXPECT_EQ(run("div", html.c_str()).selectors, 1u);
}

TEST(V3Raw, ScriptAsTarget)
{
    EXPECT_EQ(run("script", "<script>var a;</script>").selectors, 1u);
}

TEST(V3Raw, ChunkInvariant)
{
    ChunkInvariant("div", "<script>if(a<b){s=\"</scr\"}</script><style>p<q{}</STYLE><div>t</div>");
    ChunkInvariant("div", "<script type='x'>x<</script><div>t</div><script></script>");
}

// ------------------------------------------------------------ comments

TEST(V3Comment, EmptyComment)
{
    EXPECT_EQ(run("div", "<!----><div>t</div>").selectors, 1u);
}

TEST(V3Comment, DashesAndGreaterThanInside)
{
    Out r = run("div", "<!-- a -- b > c --><!-- x ---><div>t</div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"t"}));
}

TEST(V3Comment, ChunkInvariant)
{
    ChunkInvariant("div", "<!-- <div>x</div> --><!----><!-- a--b ---><div>t</div>");
}

// ------------------------------------------- realistic mix, every chunk size

TEST(V3Mix, WikiLikeSnippet)
{
    const char *html =
        "<?xml version=\"1.0\"?><!DOCTYPE html><HTML><head>"
        "<script>var c=a<b&&\"</s\";</script><style>a>b{}</STYLE></head>"
        "<body><!-- nav -- x --><div ID = \"m\" hidden CLASS=nav>"
        "<a href=\"/w?a=1&b=2\" class=\"x\">one</a> 1 < 2 "
        "<a class='x' title=\"a>b\">two</a><br/><img src=x.png>"
        "</div></body></HTML>";
    Out r = run("div > a.x", html);
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"one", "two"}));
    ChunkInvariant("div > a.x", html);
    ChunkInvariant("div#m.nav", html);
}

// ------------------------------------------- former known bugs, fixed after v3d

// bug 4: nested matches keep the outer element's text
TEST(V3Fixed, NestedMatchesKeepOuterText)
{
    EXPECT_EQ(run("div div", "<div><div><div>a</div>b</div></div>").texts, L({"a", "ab"}));
}

// bug 3: an unclosed <p> no longer makes depth drift
TEST(V3Fixed, UnclosedChildDoesNotLeak)
{
    EXPECT_EQ(run("div > a", "<div><p>x</div><a>q</a>").selectors, 0u);
}

// "</>" is ignored, it does not close the div
TEST(V3Fixed, EmptyEndTagIgnored)
{
    EXPECT_EQ(run("div", "<div>a</>b</div>").texts, L({"ab"}));
}

// "<!-->" closes the comment per spec
TEST(V3Fixed, AbruptCommentClose)
{
    EXPECT_EQ(run("div", "<!--><div>t</div>").selectors, 1u);
}

// "</scriptx" does not end the script
TEST(V3Fixed, ScriptEndNeedsDelimiter)
{
    EXPECT_EQ(run("div", "<script></scriptx><div>fake</div></script><div>t</div>").texts, L({"t"}));
}

// ---------------------------------------------------------------- comments

TEST(V3Fixed, CommentEdgeForms)
{
    Out r = run("div", "<!--><div>a</div><!---><div>b</div><!----><div>c</div><!-- x --><div>d</div>");
    EXPECT_EQ(r.selectors, 4u);
    EXPECT_EQ(r.texts, L({"a", "b", "c", "d"}));
}

TEST(V3Fixed, CommentChunkInvariant)
{
    ChunkInvariant("div", "<!-- a - b -- c ---><div>t</div><!--><div>u</div><!-- <div>x</div> -->");
}

// ------------------------------------------------------------- raw text end

TEST(V3Fixed, ScriptEndVariants)
{
    const char *html = "<script></scriptx>no</script ><div>a</div><style>x</style/><div>b</div>"
                       "<script>1</SCRIPT\n><div>c</div>";
    Out r = run("div", html);
    EXPECT_EQ(r.selectors, 3u);
    EXPECT_EQ(r.texts, L({"a", "b", "c"}));
    ChunkInvariant("div", html);
}

// --------------------------------------------------------- bogus end tags

TEST(V3Fixed, BogusEndTags)
{
    Out r = run("div", "<div>a</ x>b</3>c</>d</div><div>e</div>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"abcd", "e"}));
    ChunkInvariant("div", "<div>a</ x>b</3>c</>d</div><div>e</div>");
}

// ---------------------------------------------------------- nested captures

TEST(V3Fixed, NestedSameTagInnerFirst)
{
    Out r = run("div", "<div>a<div>b</div>c</div>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"b", "abc"}));
}

TEST(V3Fixed, NestedDifferentSelectorsParts)
{
    Out r = run("li", "<ul><li>a<ul><li>b</li></ul></li></ul>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"b", "ab"}));
}

TEST(V3Fixed, CallbacksEqualMatchesWhenAllClosed)
{
    Out r = run("div", "<div><div><div>a</div><p>b</p></div><div>c</div></div><div>d</div>");
    EXPECT_EQ(r.selectors, 5u);
    EXPECT_EQ(r.texts.size(), 5u);
}

TEST(V3Fixed, NestedChunkInvariant)
{
    ChunkInvariant("div", "<div>a<div>b<div>c</div>d</div>e</div>");
    ChunkInvariant("div div", "<div><div><div>a</div>b</div></div>");
}

// ------------------------------------------------------- close by tag name

TEST(V3Fixed, StrayEndTagIgnored)
{
    Out r = run("div", "<div>a</span>b</div><div>c</div>");
    EXPECT_EQ(r.texts, L({"ab", "c"}));
}

TEST(V3Fixed, VoidEndTagIgnored)
{
    EXPECT_EQ(run("div", "<div>a<br></br>b</div>").texts, L({"ab"}));
}

TEST(V3Fixed, AncestorCloseEndsUnclosedCapture)
{
    Out r = run(".x", "<div><p class=\"x\">a</div>b");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"a"}));
}

TEST(V3Fixed, DescendantNotLeakedAfterUnclosedChild)
{
    EXPECT_EQ(run("div a", "<div><p>x</div><a>q</a>").selectors, 0u);
}

TEST(V3Fixed, UppercaseEndTagClosesByName)
{
    Out r = run("div > a", "<DIV><P>x</Div><a>q</a><div><a>r</A></div>");
    EXPECT_EQ(r.selectors, 1u);
    EXPECT_EQ(r.texts, L({"r"}));
}

TEST(V3Fixed, ImpliedEndOfListItems)
{
    Out r = run("ul > li", "<ul><li>a<li>b<li>c</ul>");
    EXPECT_EQ(r.selectors, 3u);
    EXPECT_EQ(r.texts, L({"a", "b", "c"}));
}

TEST(V3Fixed, ImpliedEndOfParagraphs)
{
    Out r = run("body > p", "<body><p>a<p>b</body>");
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"a", "b"}));
}

TEST(V3Fixed, CloseByNameChunkInvariant)
{
    ChunkInvariant("div > a", "<div><p>x</div><a>q</a><div><a>r</a></span></div>");
    ChunkInvariant("ul > li", "<ul><li>a<li>b<li>c</ul><li>d</li>");
}

TEST(V3Fixed, DeeperThanMaxDepth)
{
    std::string html;
    for (int i = 0; i < 1100; ++i)
        html += "<div>";
    html += "<p class=\"x\">t</p>";
    for (int i = 0; i < 1100; ++i)
        html += "</div>";
    html += "<p class=\"x\">u</p>";

    Out r = run(".x", html.c_str());
    EXPECT_EQ(r.selectors, 2u);
    EXPECT_EQ(r.texts, L({"", "u"})); // too deep to track: matched, but no text
}

// ---------------------------------------------------------- finish / reset

TEST(V3Fixed, FinishEmitsOpenElements)
{
    fasthtml::Matcher matcher(fasthtml::Selector::compile("div").value());
    std::vector<std::string> texts;
    matcher.on_match(collect, &texts);
    matcher.feed("<div>abc<div>d");
    EXPECT_EQ(texts.size(), 0u);
    matcher.finish();
    EXPECT_EQ(texts, L({"d", "abcd"}));
}

TEST(V3Fixed, ResetStartsOverKeepsCallback)
{
    fasthtml::Matcher matcher(fasthtml::Selector::compile("p").value());
    std::vector<std::string> texts;
    matcher.on_match(collect, &texts);
    matcher.feed("<p>one</p><div><p>open");
    matcher.reset();
    matcher.feed("<p>two</p>");
    EXPECT_EQ(matcher.match_count(), 1u);
    EXPECT_EQ(texts, L({"one", "two"}));
}
