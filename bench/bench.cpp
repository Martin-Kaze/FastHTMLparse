// Throughput benchmark: fasthtml_bench <file.html> <selector> [cb]
// Prints the counters and the best of 7 runs. With "cb" a counting callback is installed,
// which also measures the cost of collecting the matched text.
#include <fasthtml/fasthtml.hpp>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static void count_cb(std::string_view, void *ctx)
{
    ++*static_cast<size_t *>(ctx);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "usage: " << argv[0] << " <file.html> <selector> [cb]\n";
        return 2;
    }
    const char *path = argv[1];
    const std::string query = argv[2];
    const bool use_callback = argc > 3 && std::string(argv[3]) == "cb";

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

    std::string error;
    const auto selector = fasthtml::Selector::compile(query, &error);
    if (!selector)
    {
        std::cerr << "BAD SELECTOR: " << error << "\n";
        return 1;
    }

    constexpr size_t CHUNK = 64 * 1024; // like reading from a socket or a file stream
    constexpr int RUNS = 7;

    double best = 1e9;
    fasthtml::Stats stats;
    size_t callbacks = 0;

    for (int r = 0; r < RUNS; ++r)
    {
        fasthtml::Matcher matcher(*selector);
        size_t cb_count = 0;
        if (use_callback)
            matcher.on_match(count_cb, &cb_count);

        const auto t0 = std::chrono::steady_clock::now();
        for (size_t off = 0; off < size; off += CHUNK)
            matcher.feed(data.data() + off, std::min(CHUNK, size - off));
        matcher.finish();
        const auto t1 = std::chrono::steady_clock::now();

        best = std::min(best, std::chrono::duration<double>(t1 - t0).count());
        stats = matcher.stats(); // use the results so the work can't be optimized away
        callbacks = cb_count;
    }

    std::cout << "selector:  " << query << "\n"
              << "file:      " << size / 1e6 << " MB\n"
              << "tags:      " << stats.tag_matches << "\n"
              << "attrs:     " << stats.attribute_matches << "\n"
              << "attr vals: " << stats.attribute_value_matches << "\n"
              << "selectors: " << stats.matches << "\n";
    if (use_callback)
        std::cout << "callbacks: " << callbacks << "\n";
    std::cout << "best time: " << best << " s\n"
              << "speed:     " << (size / 1e6) / best << " MB/s\n";
    return 0;
}
