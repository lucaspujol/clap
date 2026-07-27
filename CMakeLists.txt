cmake_minimum_required(VERSION 3.16)

file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/include/clap.hpp CLAP_VERSION_LINE
     REGEX "^#define CLAP_VERSION ")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" CLAP_VERSION "${CLAP_VERSION_LINE}")
if(NOT CLAP_VERSION)
    set(CLAP_VERSION 0.0.0)
endif()

project(clap VERSION ${CLAP_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

add_library(clap INTERFACE)

target_include_directories(clap INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

add_library(clap::clap ALIAS clap)

include(CMakePackageConfigHelpers)
set(CLAP_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/clap)

install(TARGETS clap EXPORT clapTargets)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/clap.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT clapTargets
        FILE clapTargets.cmake
        NAMESPACE clap::
        DESTINATION ${CLAP_CMAKE_DIR})

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/clapConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/clapConfig.cmake
    INSTALL_DESTINATION ${CLAP_CMAKE_DIR})

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/clapConfigVersion.cmake
    COMPATIBILITY SameMinorVersion
    ARCH_INDEPENDENT)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/clapConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/clapConfigVersion.cmake
    DESTINATION ${CLAP_CMAKE_DIR})

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)

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
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz)
        # MSVC: link the same CRT as everything else here.
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
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
