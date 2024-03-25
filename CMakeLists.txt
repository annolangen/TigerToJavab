# Basic CMake file for Tiger compiler with flex and bison

cmake_minimum_required(VERSION 3.9)

project(TigerCompiler)

# Add the Bison and Flex targets
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
BISON_TARGET(MyParser src/parser.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.cc DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hh)
FLEX_TARGET(MyScanner src/scanner.ll  ${CMAKE_CURRENT_BINARY_DIR}/scanner.cc DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/scanner.hh)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)
set_source_files_properties(src/driver.cc PROPERTIES OBJECT_DEPENDS ${BISON_MyParser_OUTPUT_HEADER})

# Add the include directory to the include path
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Set compiler flags
set(CMAKE_CXX_FLAGS "-Werror -std=c++17 -g3")

set(TESTED_FILE_STEMS checker debug_string symbol_table type_finder java_source)
set(TESTED_SRC_FILES "")
set(TESTED_TEST_FILES "")
foreach(S ${TESTED_FILE_STEMS})
  list(APPEND TESTED_SRC_FILES "src/${S}.cc")
  list(APPEND TESTED_TEST_FILES "src/${S}_test.cc")
endforeach()

# Define library for executable and tests
add_library(tc_lib src/driver.cc src/emit.cc src/binary_op.cc ${TESTED_SRC_FILES})

# Add the executable
add_executable(tc ${BISON_MyParser_OUTPUTS} ${FLEX_MyScanner_OUTPUTS} src/tc.cc)

# Add the libraries
target_link_libraries(tc tc_lib)

# Add the tests
Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests ${TESTED_TEST_FILES} ${BISON_MyParser_OUTPUTS} ${FLEX_MyScanner_OUTPUTS} src/testing/testing.cc)
target_link_libraries(tests tc_lib)
target_link_libraries(tests Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(tests)
