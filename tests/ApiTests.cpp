#include <gtest/gtest.h>
#include <fasthtml/fasthtml.hpp>

#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using fasthtml::Matcher;
using fasthtml::Selector;

namespace
{
    void push(std::string_view t, void *ctx) { static_cast<std::vector<std::string> *>(ctx)->emplace_back(t); }
}

TEST(Api, CompileValidKeepsText)
{
    auto sel = Selector::compile("DIV.Note > p");
    ASSERT_TRUE(sel.has_value());
    EXPECT_EQ(sel->text(), "DIV.Note > p");
}

TEST(Api, CompileInvalidReportsError)
{
    std::string error = "old";
    auto sel = Selector::compile("div > ", &error);
    EXPECT_FALSE(sel.has_value());
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error, "old");

    EXPECT_FALSE(Selector::compile("a:hover").has_value());
    EXPECT_FALSE(Selector::compile("").has_value());
}

TEST(Api, CompileClearsErrorOnSuccess)
{
    std::string error = "old";
    EXPECT_TRUE(Selector::compile("p", &error).has_value());
    EXPECT_TRUE(error.empty());
}

TEST(Api, SelectorTextOutlivesInput)
{
    std::string text = "li.x";
    auto sel = Selector::compile(text).value();
    text = "garbage garbage garbage garbage garbage";
    EXPECT_EQ(fasthtml::count("<ul><li class=\"x\">a</li></ul>", sel), 1u);
}

TEST(Api, SelectorCopiesAndSharing)
{
    std::optional<Selector> original = Selector::compile("p");
    Selector copy = *original;
    original.reset();
    Matcher a(copy), b(copy);
    a.feed("<p>1</p>");
    b.feed("<p>1</p><p>2</p>");
    EXPECT_EQ(a.match_count(), 1u);
    EXPECT_EQ(b.match_count(), 2u);
}

TEST(Api, CallbackWithContext)
{
    Matcher m(Selector::compile("b").value());
    std::vector<std::string> texts;
    m.on_match(push, &texts);
    m.feed("<p>x<b>bold</b>y<b>again</b></p>");
    EXPECT_EQ(texts, (std::vector<std::string>{"bold", "again"}));
}

TEST(Api, CallbackLambdaWithCaptures)
{
    Matcher m(Selector::compile("b").value());
    std::vector<std::string> texts;
    auto collect = [&texts](std::string_view t) { texts.emplace_back(t); };
    m.on_match(collect);
    m.feed("<b>1</b><b>2</b>");
    EXPECT_EQ(texts, (std::vector<std::string>{"1", "2"}));
}

TEST(Api, CaptureLessLambdaAsCallback)
{
    Matcher m(Selector::compile("i").value());
    size_t n = 0;
    m.on_match([](std::string_view, void *user) { ++*static_cast<size_t *>(user); }, &n);
    m.feed("<i></i><i></i><i></i>");
    EXPECT_EQ(n, 3u);
}

TEST(Api, NoCallbackOnlyCounts)
{
    Matcher m(Selector::compile("div#a.b").value());
    m.feed("<div id=\"a\" class=\"b\"></div><div id=\"a\"></div>");
    const fasthtml::Stats s = m.stats();
    EXPECT_EQ(s.matches, 1u);
    EXPECT_EQ(s.tag_matches, 2u);
    EXPECT_EQ(s.attribute_matches, 3u);
    EXPECT_EQ(s.attribute_value_matches, 3u);
}

TEST(Api, FeedInAnyPieces)
{
    const std::string html = "<ul><li class=\"x\">a<b>b</b></li><li class=\"x\">c</li></ul>";
    const Selector sel = Selector::compile("ul > li.x").value();
    for (size_t piece = 1; piece <= html.size(); ++piece)
    {
        SCOPED_TRACE("piece=" + std::to_string(piece));
        Matcher m(sel);
        std::vector<std::string> texts;
        m.on_match(push, &texts);
        for (size_t off = 0; off < html.size(); off += piece)
            m.feed(std::string_view(html).substr(off, piece));
        m.finish();
        EXPECT_EQ(texts, (std::vector<std::string>{"ab", "c"}));
    }
}

TEST(Api, FinishEmitsUnclosed)
{
    Matcher m(Selector::compile("p").value());
    std::vector<std::string> texts;
    m.on_match(push, &texts);
    m.feed("<p>never closed");
    EXPECT_TRUE(texts.empty());
    m.finish();
    EXPECT_EQ(texts, (std::vector<std::string>{"never closed"}));
}

TEST(Api, ResetForNextDocument)
{
    Matcher m(Selector::compile("p").value());
    std::vector<std::string> texts;
    m.on_match(push, &texts);
    m.feed("<p>a</p><p>half");
    m.reset();
    EXPECT_EQ(m.match_count(), 0u);
    m.feed("<p>b</p>");
    m.finish();
    EXPECT_EQ(texts, (std::vector<std::string>{"a", "b"}));
    EXPECT_EQ(m.match_count(), 1u);
}

TEST(Api, MoveMatcher)
{
    Matcher a(Selector::compile("p").value());
    a.feed("<p>1</p>");
    Matcher b(std::move(a));
    b.feed("<p>2</p>");
    EXPECT_EQ(b.match_count(), 2u);

    Matcher c(Selector::compile("i").value());
    c = std::move(b);
    EXPECT_EQ(c.match_count(), 2u);
}

TEST(Api, OneCallHelpers)
{
    const char *html = "<div><p>one</p><p>two<br></p></div>";
    EXPECT_EQ(fasthtml::count(html, "p"), 2u);
    EXPECT_EQ(fasthtml::extract_text(html, "div > p"), (std::vector<std::string>{"one", "two"}));
    EXPECT_EQ(fasthtml::count(html, Selector::compile("br").value()), 1u);
}

TEST(Api, OneCallHelpersThrowOnBadSelector)
{
    bool thrown = false;
    try
    {
        (void)fasthtml::count("<p></p>", "p >");
    }
    catch (const std::invalid_argument &)
    {
        thrown = true;
    }
    EXPECT_TRUE(thrown);
}

TEST(Api, OneSelectorManyThreads)
{
    const Selector sel = Selector::compile("li").value();
    std::string html;
    for (int i = 0; i < 1000; ++i)
        html += "<ul><li>x</li><li>y</li></ul>";

    std::vector<size_t> results(4);
    std::vector<std::thread> threads;
    for (size_t t = 0; t < results.size(); ++t)
        threads.emplace_back([&, t] { results[t] = fasthtml::count(html, sel); });
    for (auto &th : threads)
        th.join();
    for (size_t r : results)
        EXPECT_EQ(r, 2000u);
}

TEST(Api, Version)
{
    EXPECT_EQ(fasthtml::version_major, 1);
}
