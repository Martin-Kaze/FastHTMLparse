#include "Parser.hpp"

#include <iostream>
#include <fstream>
#include <chrono>






int main() {
    
  
    Parser parser;
    auto start = std::chrono::steady_clock::now();

    std::ifstream file(
    "C:\\Users\\kisie\\Desktop\\test2.html",
    std::ios::binary );

    
    char buf[64 * 1024];

    while (file.read(buf, sizeof(buf)) || file.gcount()) {
    size_t n = file.gcount();
    parser.parsring(buf, n);
    }

    std::string eilute;
 
    auto end = std::chrono::steady_clock::now();
    auto time = std::chrono::duration<double>(end - start);

    
    std::cout << time.count() << " seconds\n";


    return 0;
}