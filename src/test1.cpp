#include <iostream>
#include <fstream>
#include <chrono>

void parser( const char* buf, size_t size ){

    for( size_t i =0; i< size; i++){
        char c = *(buf + i);
        std::cout<<c;
    }
}

int main() {
    
  

    auto start = std::chrono::steady_clock::now();

    std::ifstream file("C:\\Users\\kisie\\Desktop\\test2.html");

    
    char buf[64 * 1024];

    while (file.read(buf, sizeof(buf)) || file.gcount()) {
    size_t n = file.gcount();
    parser(buf, n);
    }

    std::string eilute;
 
    auto end = std::chrono::steady_clock::now();
    auto time = std::chrono::duration<double>(end - start);

    
    std::cout << time.count() << " seconds\n";


    return 0;
}