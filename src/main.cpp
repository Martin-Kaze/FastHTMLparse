#include "Parser.hpp"

#include <iostream>
#include <fstream>
#include <chrono>






int main() {
    
  
    Parser parser;
    auto start = std::chrono::steady_clock::now();



    std::ifstream file(
    "C:\\Users\\manvi\\Desktop\\test.html",
    std::ios::binary );

    if (!file) {
    std::cout << "FILE NOT OPEN\n";
    return 1;
}
    
    char buf[64 * 1024];

    while (file.read(buf, sizeof(buf)) || file.gcount()) {
    size_t n = file.gcount();
    parser.parsring(buf, n);
    }

    std::string eilute;
 
    auto end = std::chrono::steady_clock::now();
    auto time = std::chrono::duration<double>(end - start);

    std::cout << std::endl;
    std::cout << time.count() << " seconds\n";
    

    return 0;
}