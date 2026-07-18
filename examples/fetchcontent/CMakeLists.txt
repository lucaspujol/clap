# Standalone example: how a real consumer pulls clap in via FetchContent.
#
# This is NOT built by the top-level clap CMake — that build skips any example
# dir carrying its own CMakeLists.txt. Configure it on its own:
#
#   cmake -S . -B build
#   cmake --build build
#   ./build/fetchcontent --name Lucas --count 2
#
# To test against a local clap checkout instead of cloning from GitHub, point
# FetchContent at it (skips the clone, picks up uncommitted changes):
#
#   cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_CLAP=/path/to/clap

cmake_minimum_required(VERSION 3.16)
project(fetchcontent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)          # clap needs C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(clap
  GIT_REPOSITORY https://github.com/lucaspujol/clap.git
  GIT_TAG main)
FetchContent_MakeAvailable(clap)

add_executable(fetchcontent main.cpp)
target_link_libraries(fetchcontent PRIVATE clap)   # puts clap.hpp on the include path
