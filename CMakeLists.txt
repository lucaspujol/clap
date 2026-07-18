cmake_minimum_required(VERSION 3.16)
project(clap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Library: header-only, INTERFACE target just carries the include dir ---
add_library(clap INTERFACE)
target_include_directories(clap INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Examples and tests are only for developing clap itself.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)

# --- Examples: one binary per examples/<name>/ subdir, built from all its .cpp ---
file(GLOB EXAMPLE_DIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/examples/*)
foreach(example_dir ${EXAMPLE_DIRS})
    # An example carrying its own CMakeLists.txt is a standalone project
    # (e.g. fetchcontent/) — it configures itself, skip it here.
    if(IS_DIRECTORY ${example_dir} AND NOT EXISTS ${example_dir}/CMakeLists.txt)
        file(GLOB example_srcs CONFIGURE_DEPENDS ${example_dir}/*.cpp)
        if(example_srcs)
            get_filename_component(example_name ${example_dir} NAME)
            add_executable(${example_name} ${example_srcs})
            target_link_libraries(${example_name} PRIVATE clap)
            target_compile_options(${example_name} PRIVATE -Wall -Wextra)
            set_target_properties(${example_name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${example_dir})
        endif()
    endif()
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

endif()
