cmake_minimum_required(VERSION 3.16)
project(clap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(clap STATIC
    src/App.cpp
    src/HelpFormatter.cpp
)

target_include_directories(clap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(clap PRIVATE -Wall -Wextra)

# --- example ---
add_executable(example main.cpp)
target_link_libraries(example PRIVATE clap)

# --- tests ---
option(CLAP_BUILD_TESTS "Build tests" ON)

if(CLAP_BUILD_TESTS)
    find_package(GTest REQUIRED)
    add_executable(clap_tests test/clap_tests.cpp)
    target_link_libraries(clap_tests PRIVATE clap GTest::gtest GTest::gtest_main)

    enable_testing()
    add_test(NAME clap_tests COMMAND clap_tests)
endif()
