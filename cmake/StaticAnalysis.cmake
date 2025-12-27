# Static Analysis Configuration for Mudlet
# Add this to enable static analysis during builds

option(ENABLE_STATIC_ANALYSIS "Enable static analysis with clang-tidy and cppcheck" OFF)

if(ENABLE_STATIC_ANALYSIS)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    find_program(CPPCHECK_EXE NAMES "cppcheck")
    
    if(CLANG_TIDY_EXE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
        
        # Configure clang-tidy checks
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
            --header-filter=.*
        )
        
        message(STATUS "clang-tidy integration enabled")
    else()
        message(WARNING "clang-tidy not found, static analysis disabled")
    endif()
    
    if(CPPCHECK_EXE)
        message(STATUS "Found cppcheck: ${CPPCHECK_EXE}")
        set(CMAKE_CXX_CPPCHECK ${CPPCHECK_EXE};
            --enable=all;
            --inconclusive;
            --std=c++20;
            --suppress=missingInclude;
            --suppress=unusedFunction;
            --suppress=unmatchedSuppression;
        )
        message(STATUS "cppcheck integration enabled")
    endif()
endif()

# Compiler-specific static analysis flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    option(ENABLE_CLANG_ANALYZER "Enable Clang Static Analyzer" OFF)
    
    if(ENABLE_CLANG_ANALYZER)
        message(STATUS "Enabling Clang Static Analyzer")
        add_compile_options(--analyze)
        # Store analyzer results in build directory
        add_compile_options(-Xanalyzer -analyzer-output=html-single-file)
        add_compile_options(-Xanalyzer -analyzer-output-dir=${CMAKE_BINARY_DIR}/analyzer-reports)
    endif()
endif()