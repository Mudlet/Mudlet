# Static Analysis Configuration for Mudlet
# 
# This module integrates clang-tidy and cppcheck into the CMake build process.
# Static analysis runs during compilation without breaking the build.
#
# Usage:
#   1. Uncomment include(StaticAnalysis) in root CMakeLists.txt
#   2. Configure: cmake -DENABLE_STATIC_ANALYSIS=ON ..
#   3. Build normally:
#      - Linux: make -j $(nproc)
#      - macOS: make -j `sysctl -n hw.ncpu`
#
# Notes:
#   - clang-analyzer-* checks run the Clang Static Analyzer via clang-tidy
#   - Analysis warnings appear in build output but don't fail the build
#   - For scan-build wrapper usage, see: https://clang-analyzer.llvm.org/scan-build
#
# Clazy (Qt-specific static analysis):
#   Clazy requires using clang as the compiler (separate from clang-tidy integration).
#   Install: brew install clazy (macOS) or apt install clazy (Linux)
#   Usage (clean build directory required):
#     CLAZY_CHECKS="level0,level1" CXX=clazy cmake ..
#     make -j `sysctl -n hw.ncpu`
#   See: https://github.com/KDE/clazy

option(ENABLE_STATIC_ANALYSIS "Enable static analysis with clang-tidy and cppcheck" OFF)

if(ENABLE_STATIC_ANALYSIS)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    find_program(CPPCHECK_EXE NAMES "cppcheck")
    
    if(CLANG_TIDY_EXE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
        
        # Configure clang-tidy checks
        # Note: clang-analyzer-* enables the Clang Static Analyzer
        set(CLANG_TIDY_CHECKS
            "performance-*" 
            "bugprone-*"
            "clang-analyzer-*"
        )
        
        # Convert list to comma-separated string
        string(JOIN "," CLANG_TIDY_CHECKS_STR ${CLANG_TIDY_CHECKS})
        
        set(CMAKE_CXX_CLANG_TIDY 
            ${CLANG_TIDY_EXE};
            --checks=${CLANG_TIDY_CHECKS_STR};
            --header-filter=.*;
        )
        
        message(STATUS "clang-tidy integration enabled with checks: ${CLANG_TIDY_CHECKS_STR}")
        message(STATUS "  - performance-*: Performance optimizations")
        message(STATUS "  - bugprone-*: Bug detection")
        message(STATUS "  - clang-analyzer-*: Clang Static Analyzer (deep analysis)")
    else()
        message(WARNING "clang-tidy not found, static analysis disabled")
    endif()
    
    if(CPPCHECK_EXE)
        message(STATUS "Found cppcheck: ${CPPCHECK_EXE}")
        set(CMAKE_CXX_CPPCHECK 
            ${CPPCHECK_EXE};
            --enable=all;
            --inconclusive;
            --std=c++20;
            --suppress=missingInclude;
            --suppress=unusedFunction;
            --suppress=unmatchedSuppression;
        )
        message(STATUS "cppcheck integration enabled")
    else()
        message(STATUS "cppcheck not found, skipping cppcheck analysis")
    endif()
endif()