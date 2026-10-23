// Counts the elements matching a selector, reading the file in pieces (streaming).
//   fasthtml_count page.html "div.content > p"
#include <fasthtml/fasthtml.hpp>

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "usage: " << argv[0] << " <file.html> <selector>\n";
        return 2;
    }

    std::string error;
    const auto selector = fasthtml::Selector::compile(argv[2], &error);
    if (!selector)
    {
        std::cerr << "invalid selector: " << error << "\n";
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file)
    {
        std::cerr << "cannot open " << argv[1] << "\n";
        return 1;
    }

    fasthtml::Matcher matcher(*selector);
    char buffer[64 * 1024];
    while (file.read(buffer, sizeof buffer) || file.gcount() > 0)
        matcher.feed(buffer, static_cast<std::size_t>(file.gcount()));
    matcher.finish();

    std::cout << matcher.match_count() << "\n";
    return 0;
}
