#include "Parser.hpp"
#include "Selector/SelectorLexer.hpp"
#include "Selector/SelectorParser.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
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

    // selector parsed once; `query` must outlive the lexer
    
    SelectorLexer lexer;
    lexer.query(query);
    SelectorParser selectorParser;
    selectorParser.parsring(lexer);

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
        auto t1 = std::chrono::steady_clock::now();

        best = std::min(best, std::chrono::duration<double>(t1 - t0).count());

        // use the results so the work can't be optimized away
        tags = parser.get_tag_match_count();
        attrs = parser.get_attr_match_count();
        vals = parser.get_attr_val_match_count();
        sels = parser.get_selector_match_count();
        callbacks = cb_count;
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