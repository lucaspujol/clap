cmake_minimum_required(VERSION 3.16)
project(clap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Library --------------------------------------------------------------
# Header-only: INTERFACE target carries the include dir, nothing is compiled
# here. Consumers link `clap` and get include/clap.hpp on their include path.
add_library(clap INTERFACE)
target_include_directories(clap INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- Examples -------------------------------------------------------------
# One binary per examples/*.cpp, emitted back into examples/ for easy running.
# CONFIGURE_DEPENDS => cmake re-globs on build, so new files are picked up.
file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp)
foreach(example_src ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${example_src} NAME_WE)
    add_executable(${example_name} ${example_src})
    target_link_libraries(${example_name} PRIVATE clap)
    target_compile_options(${example_name} PRIVATE -Wall -Wextra)
    set_target_properties(${example_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples)
endforeach()

# --- Tests ----------------------------------------------------------------
option(CLAP_BUILD_TESTS "Build tests" ON)

if(CLAP_BUILD_TESTS)
    find_package(GTest REQUIRED)
    add_executable(clap_tests tests/clap_tests.cpp)
    target_link_libraries(clap_tests PRIVATE clap GTest::gtest GTest::gtest_main)
    target_compile_options(clap_tests PRIVATE -Wall -Wextra)

    enable_testing()
    add_test(NAME clap_tests COMMAND clap_tests)
endif()
