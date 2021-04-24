# CMakeList.txt : CMake project for regren.
#
cmake_minimum_required (VERSION 3.10)

# Require out-of-source builds
file (TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if (EXISTS "${LOC_PATH}")
    message (FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file).")
endif()

project ("regren")

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")

# Sources
add_executable (regren "main.cpp")

# External headers
target_include_directories (regren PRIVATE "extern/gsl/include")

# Tests
add_test (NAME MainTest COMMAND regren "--test")
