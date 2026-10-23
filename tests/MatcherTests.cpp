#include <gtest/gtest.h>
#include <fasthtml/fasthtml.hpp>

#include <cstring>
#include <string>

TEST(MatcherTag, FindsDiv)
{
    const auto sel = fasthtml::Selector::compile("div").value();

    fasthtml::Matcher matcher(sel);

    const char* html =
        "<div>"
        "<p>"
        "<div>"
        "<section>";

    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 2);
}

TEST(MatcherTag, DoesNotMatchSimilarTags)
{
    const auto sel = fasthtml::Selector::compile("div").value();

    fasthtml::Matcher matcher(sel);

    const char* html =
        "<div>"
        "<divx>"
        "<dive>"
        "<di>";

    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
}

TEST(MatcherTag, DoesNotMatchClosingTag)
{
    const auto sel = fasthtml::Selector::compile("div").value();

    fasthtml::Matcher matcher(sel);

    const char* html = "<div></div>";

    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
}

TEST(MatcherTag, MatchesAcrossChunks)
{
    const auto sel = fasthtml::Selector::compile("div").value();

    fasthtml::Matcher matcher(sel);

    const char* chunk1 = "<di";
    const char* chunk2 = "v>";

    matcher.feed(chunk1, 3);
    matcher.feed(chunk2, 2);

    EXPECT_EQ(matcher.stats().tag_matches, 1);
}

TEST(MatcherTag, MatchesAtDifferentChunkBoundaries)
{
    const auto sel = fasthtml::Selector::compile("div").value();

    {
        fasthtml::Matcher matcher(sel);

        matcher.feed("<", 1);
        matcher.feed("div>", 4);

        EXPECT_EQ(matcher.stats().tag_matches, 1);
    }

    {
        fasthtml::Matcher matcher(sel);

        matcher.feed("<d", 2);
        matcher.feed("iv>", 3);

        EXPECT_EQ(matcher.stats().tag_matches, 1);
    }

    {
        fasthtml::Matcher matcher(sel);

        matcher.feed("<di", 3);
        matcher.feed("v>", 2);

        EXPECT_EQ(matcher.stats().tag_matches, 1);
    }

    {
        fasthtml::Matcher matcher(sel);

        matcher.feed("<div", 4);
        matcher.feed(">", 1);

        EXPECT_EQ(matcher.stats().tag_matches, 1);
    }
}

TEST(MatcherTag, MismatchAcrossChunks)
{
    const auto sel = fasthtml::Selector::compile("div").value();

    fasthtml::Matcher matcher(sel);

    matcher.feed("<s", 2);
    matcher.feed("ection>", 7);

    EXPECT_EQ(matcher.stats().tag_matches, 0);
}

TEST(MatcherTest, MatchesId)
{

    const auto sel = fasthtml::Selector::compile("div#good").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div id="good">)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 1);
}

TEST(MatcherTest, DoesNotMatchWrongId)
{

    const auto sel = fasthtml::Selector::compile("div#good").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div id="bad">)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 0);
}

TEST(MatcherTest, DoesNotMatchIdPrefix)
{

    const auto sel = fasthtml::Selector::compile("div#good").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div id="goodxxx">)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 0);
}

TEST(MatcherTest, MatchesClass)
{

    const auto sel = fasthtml::Selector::compile("div.product").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div class="product">)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 1);
}

TEST(MatcherTest, DoesNotMatchWrongClass)
{

    const auto sel = fasthtml::Selector::compile("div.product").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div class="other">)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 0);
}

TEST(MatcherTest, MatchesIdAndClass)
{

    const auto sel = fasthtml::Selector::compile("div#good.product").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div id="good" class="product">)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 2);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 2);
}

TEST(MatcherTest, MatchesClassAndIdReversed)
{

    const auto sel = fasthtml::Selector::compile("div#good.product").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div class="product" id="good">)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 2);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 2);
}

TEST(MatcherTest, DoesNotMatchWrongIdWithCorrectClass)
{

    const auto sel = fasthtml::Selector::compile("div#good.product").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div class="product" id="bad">)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 2);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 1);
}

TEST(MatcherTest, DoesNotMatchWrongClassWithCorrectId)
{

    const auto sel = fasthtml::Selector::compile("div#good.product").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div id="good" class="other">)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 2);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 1);
}

TEST(MatcherTest, IgnoresUnknownAttributes)
{

    const auto sel = fasthtml::Selector::compile("div#good").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div title="hello" id="good">)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 1);
}

TEST(MatcherTest, DoesNotConfuseIdWithSimilarAttribute)
{

    const auto sel = fasthtml::Selector::compile("div#good").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div idk="good" id="good">)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 1);
}

TEST(MatcherTest, SupportsSingleQuotes)
{

    const auto sel = fasthtml::Selector::compile("div#good").value();

    fasthtml::Matcher matcher(sel);

    const char *html = R"(<div id='good'>)";
    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 1);
}

TEST(MatcherTest, MatchesAcrossChunks)
{

    const auto sel = fasthtml::Selector::compile("div#good").value();

    fasthtml::Matcher matcher(sel);

    const char *chunk1 = R"(<div id="go)";
    const char *chunk2 = R"(od">)";

    matcher.feed(chunk1, strlen(chunk1));
    matcher.feed(chunk2, strlen(chunk2));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 1);
}

TEST(MatcherTest, WrongValueAcrossChunks)
{

    const auto sel = fasthtml::Selector::compile("div#good").value();

    fasthtml::Matcher matcher(sel);

    const char *chunk1 = R"(<div id="ba)";
    const char *chunk2 = R"(d">)";

    matcher.feed(chunk1, strlen(chunk1));
    matcher.feed(chunk2, strlen(chunk2));

    EXPECT_EQ(matcher.stats().tag_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_matches, 1);
    EXPECT_EQ(matcher.stats().attribute_value_matches, 0);
}

TEST(MatcherTest, FullSelectorMatches)
{

    const auto sel = fasthtml::Selector::compile("div#good.product").value();

    fasthtml::Matcher matcher(sel);

    const char *html =
        R"(<div id="good" class="product">)";

    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.match_count(), 1);
}

TEST(MatcherTest, FullSelectorDoesNotMatchWrongId)
{

    const auto sel = fasthtml::Selector::compile("div#good.product").value();

    fasthtml::Matcher matcher(sel);

    const char *html =
        R"(<div id="bad" class="product">)";

    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.match_count(), 0);
}

TEST(MatcherTest, WithBadChild)
{

    const auto sel = fasthtml::Selector::compile("div > p").value();

    fasthtml::Matcher matcher(sel);

    const char *html =
        R"(<div id="bad" class="product"> <a> sup baby </a> <div/>)";

    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.match_count(), 0);
}

TEST(MatcherTest, WithGoodChild)
{

    const auto sel = fasthtml::Selector::compile("div > p").value();

    fasthtml::Matcher matcher(sel);

    const char *html =
        R"(<div id="bad" class="product"> <p> sup baby </p> <div/>)";

    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.match_count(), 1);
}

TEST(MatcherTest, WithNotGoodChild)
{

    const auto sel = fasthtml::Selector::compile("div > p").value();

    fasthtml::Matcher matcher(sel);

    const char *html =
        R"(<div><div><p></p></div></div>)";

    matcher.feed(html, strlen(html));

    EXPECT_EQ(matcher.match_count(), 1);
}