#include "Parser.hpp"
#include "Selector/SelectorLexer.hpp"
#include "Selector/SelectorParser.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

static void count_cb(std::string_view, void *ctx)
{
    ++*static_cast<size_t *>(ctx);
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "../src/test1.html";
    std::string query = argc > 2 ? argv[2] : "#mwBMw > a";
    const bool use_callback = argc > 3 && std::string(argv[3]) == "cb";

    // read the file once, outside the timing
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
    {
        std::cerr << "FILE NOT OPEN: " << path << "\n";
        return 1;
    }
    const size_t size = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<char> data(size);
    f.read(data.data(), static_cast<std::streamsize>(size));

    // selector parsed once (the lexer keeps its own copy of the text)
    SelectorLexer lexer;
    lexer.query(query);
    SelectorParser selectorParser;
    if (!selectorParser.parsring(lexer))
    {
        std::cerr << "BAD SELECTOR: " << selectorParser.error() << "\n";
        return 1;
    }

    constexpr size_t CHUNK = 64 * 1024;
    constexpr int RUNS = 7;

    double best = 1e9;
    size_t tags = 0, attrs = 0, vals = 0, sels = 0, callbacks = 0;

    for (int r = 0; r < RUNS; ++r)
    {
        Parser parser; // fresh state each run
        size_t cb_count = 0;
        if (use_callback)
            parser.set_match_callback(count_cb, &cb_count);

        auto t0 = std::chrono::steady_clock::now();
        for (size_t off = 0; off < size; off += CHUNK)
            parser.parsring(data.data() + off, std::min(CHUNK, size - off), selectorParser);
        parser.finish(); // emits elements still open at the end
        auto t1 = std::chrono::steady_clock::now();

        best = std::min(best, std::chrono::duration<double>(t1 - t0).count());

        // use the results so the work can't be optimized away
        tags = parser.get_tag_match_count();
        attrs = parser.get_attr_match_count();
        vals = parser.get_attr_val_match_count();
        sels = parser.get_selector_match_count();
        callbacks = cb_count;

        #ifdef PARSER_STATS
if (r == RUNS - 1) {
    const char *names[] = {
    "DATA", "TAG_OPEN", "TAG_NAME", "ATTRIBUTE_NAME", "ATTRIBUTE_VALUE",
    "ATTRIBUTE_VALUE_START", "AFTER_BANG", "COMMENT", "MARKUP_DECL", "RAWTEXT",
    "BEFORE_ATTRIBUTE_NAME", "AFTER_ATTRIBUTE_NAME" };
    size_t tb = 0, ti = 0;
    for (int s = 0; s < 16; s++) { tb += parser.stat_bytes[s]; ti += parser.stat_iters[s]; }
    for (int s = 0; s < (int)std::size(names); s++)
        printf("%-24s bytes %6.2f%%  iters %6.2f%%  (%zu iters)\n", names[s],
               100.0 * parser.stat_bytes[s] / tb, 100.0 * parser.stat_iters[s] / ti, parser.stat_iters[s]);
    printf("iters per KB: %.1f\n", 1024.0 * ti / tb);
}
#endif
    }

    std::cout << "selector:  " << query << "\n"
              << "file:      " << size / 1e6 << " MB\n"
              << "tags:      " << tags << "\n"
              << "attrs:     " << attrs << "\n"
              << "attr vals: " << vals << "\n"
              << "selectors: " << sels << "\n";
    if (use_callback)
        std::cout << "callbacks: " << callbacks << "\n";
    std::cout << "best time: " << best << " s\n"
              << "speed:     " << (size / 1e6) / best << " MB/s\n";
    return 0;
}