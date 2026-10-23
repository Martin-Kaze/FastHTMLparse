// Prints the text of every element matching a selector, one per line.
//   fasthtml_extract_text page.html "h2"
#include <fasthtml/fasthtml.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
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
    std::stringstream html;
    html << file.rdbuf();

    // A lambda with captures: it must outlive the matcher, so keep it in a variable.
    std::size_t n = 0;
    auto print = [&n](std::string_view text) { std::cout << ++n << ": " << text << "\n"; };

    fasthtml::Matcher matcher(*selector);
    matcher.on_match(print);
    matcher.feed(html.str());
    matcher.finish();
    return 0;
}
