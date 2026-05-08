cmake_minimum_required(VERSION 3.16)

# ─────────────────────────────────────────────────────────────────────────────
# Project declaration
# ─────────────────────────────────────────────────────────────────────────────
project(
    CVM
    VERSION     1.0.0
    DESCRIPTION "CVM++ — Stack-Based VM & Custom Scripting Language"
    LANGUAGES   CXX
)

# ─────────────────────────────────────────────────────────────────────────────
# C++ Standard
# Require C++17 for std::variant and structured bindings.
# ─────────────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)   # No GNU extensions — keeps it portable

# ─────────────────────────────────────────────────────────────────────────────
# Compiler flags
# Enable optimisations in Release; maximum warnings in Debug.
# Apple Silicon (arm64) and x86-64 are both handled by the same flags.
# ─────────────────────────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Common warnings
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
    -Wno-unused-parameter
)

# Release: full optimisation + LTO hint (Clang/GCC)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3)
endif()

# Debug: no optimisation, full debug info, sanitizers
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-O0 -g)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Source files
# ─────────────────────────────────────────────────────────────────────────────
set(CVM_SOURCES
    src/main.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/bytecode.cpp
    src/compiler.cpp
    src/vm.cpp
)

# ─────────────────────────────────────────────────────────────────────────────
# Executable target
# ─────────────────────────────────────────────────────────────────────────────
add_executable(cvm ${CVM_SOURCES})

target_include_directories(cvm PRIVATE src)

# ─────────────────────────────────────────────────────────────────────────────
# CTest integration — run sample scripts as smoke tests
# ─────────────────────────────────────────────────────────────────────────────
enable_testing()

# Each test runs a .cvm script and checks the exit code.
# The scripts' stdout is not checked here; add expected output files and
# `--compare` flags with a custom test runner for CI if desired.
function(add_cvm_test name script)
    add_test(
        NAME  ${name}
        COMMAND cvm ${CMAKE_SOURCE_DIR}/tests/${script}
    )
endfunction()

add_cvm_test(test_hello       hello.cvm)
add_cvm_test(test_arithmetic  arithmetic.cvm)
add_cvm_test(test_factorial   factorial.cvm)
add_cvm_test(test_fibonacci   fibonacci.cvm)
add_cvm_test(test_fizzbuzz    fizzbuzz.cvm)
add_cvm_test(test_booleans    booleans.cvm)

# ─────────────────────────────────────────────────────────────────────────────
# Install (optional)
# ─────────────────────────────────────────────────────────────────────────────
install(TARGETS cvm RUNTIME DESTINATION bin)
