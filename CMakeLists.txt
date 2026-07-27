cmake_minimum_required(VERSION 3.16)
project(clap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Library: header-only, INTERFACE target just carries the include dir ---
add_library(clap INTERFACE)
target_include_directories(clap INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Examples and tests are only for developing clap itself.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)

# Warning flags for everything built here. MSVC also needs /utf-8: sources
# carry UTF-8 text and cl assumes the system codepage without it.
if(MSVC)
    set(CLAP_WARNINGS /W4 /utf-8)
else()
    set(CLAP_WARNINGS -Wall -Wextra)
endif()

# --- Examples: one binary per examples/<name>/ subdir, built from all its .cpp ---
file(GLOB EXAMPLE_DIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/examples/*)
foreach(example_dir ${EXAMPLE_DIRS})
    # An example carrying its own CMakeLists.txt is a standalone project
    # (e.g. fetchcontent/): it configures itself, skip it here.
    if(IS_DIRECTORY ${example_dir} AND NOT EXISTS ${example_dir}/CMakeLists.txt)
        file(GLOB example_srcs CONFIGURE_DEPENDS ${example_dir}/*.cpp)
        if(example_srcs)
            get_filename_component(example_name ${example_dir} NAME)
            add_executable(${example_name} ${example_srcs})
            target_link_libraries(${example_name} PRIVATE clap)
            target_compile_options(${example_name} PRIVATE ${CLAP_WARNINGS})
            set_target_properties(${example_name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${example_dir})
        endif()
    endif()
endforeach()

# --- Tests ----------------------------------------------------------------
option(CLAP_BUILD_TESTS "Build tests" ON)
option(CLAP_COVERAGE "Build tests with coverage instrumentation" OFF)

if(CLAP_BUILD_TESTS)
    # Packaged GTest where the platform has one (apt); otherwise fetch it, so
    # macOS and Windows configure without a package manager step.
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz)
        # MSVC: link the same CRT as everything else here.
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    # One file per area (see tests/README.md); globbed so adding one needs no
    # build change. coverage_probe.cpp is added below, coverage builds only.
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    list(REMOVE_ITEM TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/coverage_probe.cpp)
    add_executable(clap_tests ${TEST_SOURCES})
    target_include_directories(clap_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_link_libraries(clap_tests PRIVATE clap GTest::gtest GTest::gtest_main)
    target_compile_options(clap_tests PRIVATE ${CLAP_WARNINGS})

    if(CLAP_COVERAGE)
        target_compile_options(clap_tests PRIVATE --coverage -O0 -g)
        target_link_options(clap_tests PRIVATE --coverage)

        target_sources(clap_tests PRIVATE tests/coverage_probe.cpp)
        set_source_files_properties(tests/coverage_probe.cpp
            PROPERTIES COMPILE_OPTIONS -fkeep-inline-functions)
    endif()

    enable_testing()
    add_test(NAME clap_tests COMMAND clap_tests)
endif()

endif()
