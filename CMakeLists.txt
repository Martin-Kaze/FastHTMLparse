cmake_minimum_required(VERSION 3.20)

project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(main
    src/main.cpp
    src/Parser.cpp
    src/Selector/SelectorLexer.cpp
    src/Selector/SelectorParser.cpp
)


include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)


enable_testing()


add_executable(selector_tests
    tests/SelectorLexerTests.cpp
    tests/SelectorParserTests.cpp
    tests/ParserTests.cpp
    tests/VoidtagTests.cpp
    src/Selector/SelectorLexer.cpp
    src/Selector/SelectorParser.cpp
    src/Parser.cpp
)

target_include_directories(selector_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(selector_tests
    PRIVATE
    GTest::gtest_main
)


include(GoogleTest)

gtest_discover_tests(selector_tests)