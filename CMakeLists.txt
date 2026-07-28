cmake_minimum_required(VERSION 3.16)

file(STRINGS include/clap.hpp CLAP_VERSION_LINE REGEX "^#define CLAP_VERSION ")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" CLAP_VERSION "${CLAP_VERSION_LINE}")
if(NOT CLAP_VERSION)
    set(CLAP_VERSION 0.0.0)
endif()

project(clap VERSION ${CLAP_VERSION}
             DESCRIPTION "Command Line Argument Parser: simple light weight cli parser in C++20 and above."
             LANGUAGES CXX)

add_library(clap INTERFACE)
target_compile_features(clap INTERFACE cxx_std_20)
include(GNUInstallDirs)
target_include_directories(
    clap INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
add_library(clap::clap ALIAS clap)

set(CLAP_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/clap)

install(TARGETS clap EXPORT clapTargets)
# files for compiler
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/clap.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# files for cmake
install(
    EXPORT clapTargets
    FILE clapTargets.cmake
    NAMESPACE clap::
    DESTINATION ${CLAP_CMAKE_DIR})

include(CMakePackageConfigHelpers)

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

option(CLAP_BUILD_TESTS "Build tests" ON)
option(CLAP_COVERAGE "Build tests with coverage information" OFF)

# build examples
file(GLOB EXAMPLE_DIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/examples/*)
foreach(EXAMPLE_DIR ${EXAMPLE_DIRS})
  if(NOT IS_DIRECTORY ${EXAMPLE_DIR} OR EXISTS ${EXAMPLE_DIR}/CMakeLists.txt)
    continue()
  endif()
  file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS ${EXAMPLE_DIR}/*.cpp)
  get_filename_component(EXAMPLE_NAME ${EXAMPLE_DIR} NAME)
  add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCES})
  target_link_libraries(${EXAMPLE_NAME} PRIVATE clap)
  target_compile_options(${EXAMPLE_NAME} PRIVATE ${CLAP_WARNINGS})
  set_target_properties(${EXAMPLE_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${EXAMPLE_DIR})
endforeach()

# build tests
if(CLAP_BUILD_TESTS)
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz)
    FetchContent_MakeAvailable(googletest)
  endif()

  file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
  list(REMOVE_ITEM TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/coverage_probe.cpp) # only used for coverage tests

  add_executable(clap_tests ${TEST_SOURCES})
  target_include_directories(clap_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_link_libraries(clap_tests PRIVATE clap GTest::gtest GTest::gtest_main)
  target_compile_options(clap_tests PRIVATE ${CLAP_WARNINGS})

  if(CLAP_COVERAGE)
    target_compile_options(clap_tests PRIVATE --coverage -O0 -g)
    target_link_options(clap_tests PRIVATE --coverage)

    target_sources(clap_tests PRIVATE tests/coverage_probe.cpp)
    set_source_files_properties(tests/coverage_probe.cpp PROPERTIES COMPILE_OPTIONS -fkeep-inline-functions)
  endif()

  enable_testing()
  add_test(NAME clap_tests COMMAND clap_tests)
endif()

endif()
