// Where do the loop passes go? fasthtml_state_stats <file.html> <selector>
// Built with FASTHTML_STATS, which counts bytes and loop iterations per state (slower than the real build).
#include "engine.hpp"
#include "selector_compiler.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "usage: " << argv[0] << " <file.html> <selector>\n";
        return 2;
    }
    std::ifstream f(argv[1], std::ios::binary);
    if (!f)
    {
        std::cerr << "FILE NOT OPEN: " << argv[1] << "\n";
        return 1;
    }
    const std::vector<char> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    fasthtml::detail::SelectorCompiler selector;
    if (!selector.compile(argv[2]))
    {
        std::cerr << "BAD SELECTOR: " << selector.error() << "\n";
        return 1;
    }

    auto engine = std::make_unique<fasthtml::detail::Engine>();
    constexpr size_t CHUNK = 64 * 1024;
    for (size_t off = 0; off < data.size(); off += CHUNK)
        engine->feed(data.data() + off, std::min(CHUNK, data.size() - off), selector);
    engine->finish();

    // same order as Engine::State
    const char *names[] = {"data", "tag_open", "tag_name", "attribute_name", "attribute_value",
                           "attribute_value_start", "after_bang", "comment", "markup_declaration", "raw_text",
                           "before_attribute_name", "after_attribute_name"};
    size_t tb = 0, ti = 0;
    for (int s = 0; s < 16; s++)
    {
        tb += engine->stat_bytes[s];
        ti += engine->stat_iters[s];
    }
    for (int s = 0; s < (int)std::size(names); s++)
        std::printf("%-24s bytes %6.2f%%  iters %6.2f%%  (%zu iters)\n", names[s],
                    100.0 * engine->stat_bytes[s] / tb, 100.0 * engine->stat_iters[s] / ti, engine->stat_iters[s]);
    std::printf("iters per KB: %.1f\nmatches:      %zu\n", 1024.0 * ti / tb, engine->matches());
    return 0;
}
