cmake_minimum_required(VERSION 3.21)

project(fasthtml
    VERSION 1.0.0
    DESCRIPTION "Streaming CSS-selector matcher for HTML"
    LANGUAGES CXX)

include(GNUInstallDirs)

# ---------------------------------------------------------------- options
# Tests, bench and examples are on by default only when this is the main project,
# so adding fasthtml to another project builds just the library.
option(FASTHTML_BUILD_TESTS    "Build the unit tests (downloads googletest)" ${PROJECT_IS_TOP_LEVEL})
option(FASTHTML_BUILD_BENCH    "Build the benchmark tools"                   ${PROJECT_IS_TOP_LEVEL})
option(FASTHTML_BUILD_EXAMPLES "Build the examples"                          ${PROJECT_IS_TOP_LEVEL})
option(FASTHTML_NATIVE         "Optimize for this machine's CPU (-march=native); binaries may not run elsewhere" OFF)
option(FASTHTML_LTO            "Link-time optimization for this project's own targets" ${PROJECT_IS_TOP_LEVEL})
option(FASTHTML_INSTALL        "Generate install rules"                       ${PROJECT_IS_TOP_LEVEL})

if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# LTO only when we are the top-level project: a static library made of LTO objects
# would force every user to link with LTO too.
if(FASTHTML_LTO AND PROJECT_IS_TOP_LEVEL)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT fasthtml_ipo_ok OUTPUT fasthtml_ipo_msg LANGUAGES CXX)
    if(fasthtml_ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(STATUS "fasthtml: LTO not supported here: ${fasthtml_ipo_msg}")
    endif()
endif()

# Warnings and CPU tuning for our own targets only (PRIVATE, never forced on users).
# Optimization comes from the build type (Release = -O3 / /O2).
function(fasthtml_target_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(FASTHTML_NATIVE)
            target_compile_options(${target} PRIVATE /arch:AVX2)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if(FASTHTML_NATIVE)
            target_compile_options(${target} PRIVATE -march=native)
        endif()
    endif()
endfunction()

# ---------------------------------------------------------------- library
set(FASTHTML_CORE_SOURCES
    src/engine.cpp
    src/selector_compiler.cpp
    src/selector_lexer.cpp
)

add_library(fasthtml
    src/fasthtml.cpp
    ${FASTHTML_CORE_SOURCES}
)
add_library(fasthtml::fasthtml ALIAS fasthtml)

target_compile_features(fasthtml PUBLIC cxx_std_20)
target_include_directories(fasthtml
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
fasthtml_target_options(fasthtml)

# ---------------------------------------------------------------- tests
if(FASTHTML_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # MSVC: same runtime as the tests
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(fasthtml_tests
        tests/ApiTests.cpp
        tests/MatcherTests.cpp
        tests/ExtraMatcherTests.cpp
        tests/SelectorLexerTests.cpp
        tests/SelectorCompilerTests.cpp
        tests/SelectorCompilerFixTests.cpp
    )
    # the selector and engine tests also test the internal classes
    target_include_directories(fasthtml_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(fasthtml_tests PRIVATE fasthtml GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(fasthtml_tests)
endif()

# ---------------------------------------------------------------- bench
if(FASTHTML_BUILD_BENCH)
    add_executable(fasthtml_bench bench/bench.cpp)
    target_link_libraries(fasthtml_bench PRIVATE fasthtml)
    fasthtml_target_options(fasthtml_bench)

    # per-state byte / iteration counters: compiles its own copy of the engine with FASTHTML_STATS
    add_executable(fasthtml_state_stats bench/state_stats.cpp ${FASTHTML_CORE_SOURCES})
    target_compile_features(fasthtml_state_stats PRIVATE cxx_std_20)
    target_compile_definitions(fasthtml_state_stats PRIVATE FASTHTML_STATS)
    target_include_directories(fasthtml_state_stats PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    fasthtml_target_options(fasthtml_state_stats)
endif()

# ---------------------------------------------------------------- examples
if(FASTHTML_BUILD_EXAMPLES)
    foreach(example count extract_text)
        add_executable(fasthtml_${example} examples/${example}.cpp)
        target_link_libraries(fasthtml_${example} PRIVATE fasthtml)
    endforeach()
endif()

# ---------------------------------------------------------------- install
# Lets other CMake projects use:  find_package(fasthtml CONFIG REQUIRED)
#                                 target_link_libraries(app PRIVATE fasthtml::fasthtml)
if(FASTHTML_INSTALL)
    include(CMakePackageConfigHelpers)

    install(TARGETS fasthtml
        EXPORT fasthtmlTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(EXPORT fasthtmlTargets
        NAMESPACE fasthtml::
        FILE fasthtmlConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fasthtml
    )
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/fasthtmlConfigVersion.cmake
        COMPATIBILITY SameMajorVersion
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fasthtmlConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fasthtml
    )
endif()
