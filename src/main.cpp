#include "Parser.hpp"

#include <iostream>
#include <fstream>
#include <chrono>
#include "Selector/SelectorLexer.hpp"
#include "Selector/SelectorParser.hpp"
#include <string_view>

void process_value(std::string_view text, void*)
{
    std::cout << "DATA: [" << text << "]\n";

    if (text == std::string_view{"29.99"})
    {
        std::cout << "Expected price found\n";
    }

}
int main()
{

    auto start = std::chrono::steady_clock::now();

    std::ifstream file(
        "../src/test.html",
        std::ios::binary);

    if (!file)
    {
        std::cout << "FILE NOT OPEN\n";
        return 1;
    }

    std::string query = "div#main > p.text";

    SelectorLexer lexer;
    lexer.query(query);

    SelectorParser selectorParser;
    selectorParser.parsring(lexer);

    Parser parser;
    parser.set_match_callback(process_value);

    char buf[64 * 1024];

    while (file.read(buf, sizeof(buf)) || file.gcount())
    {
        size_t n = static_cast<size_t>(file.gcount());
        parser.parsring(buf, n, selectorParser);
    }

    std::string eilute;

    auto end = std::chrono::steady_clock::now();
    auto time = std::chrono::duration<double>(end - start);

    std::cout << std::endl;
    std::cout << time.count() << " seconds\n";

    return 0;
}