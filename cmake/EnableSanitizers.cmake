############################################################################
#    Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          #
#                                                                          #
#    This program is free software; you can redistribute it and/or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

# Enable sanitizers for debugging and memory leak detection
# Usage: cmake -DENABLE_SANITIZERS=ON -DENABLE_LEAKSANITIZER=ON ..

option(ENABLE_SANITIZERS "Enable all sanitizers (AddressSanitizer + LeakSanitizer + UBSan)" OFF)
option(ENABLE_ADDRESSSANITIZER "Enable AddressSanitizer" OFF)  
option(ENABLE_LEAKSANITIZER "Enable LeakSanitizer (requires AddressSanitizer)" OFF)
option(ENABLE_UBSANITIZER "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_THREADSANITIZER "Enable ThreadSanitizer (mutually exclusive with ASan/LSan)" OFF)

# Check compiler support
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(SANITIZER_SUPPORTED TRUE)
else()
    set(SANITIZER_SUPPORTED FALSE)
    message(WARNING "Sanitizers require GCC or Clang compiler")
endif()

if(SANITIZER_SUPPORTED)
    set(SANITIZER_FLAGS "")
    set(SANITIZER_LIBS "")

    # Enable all sanitizers if requested
    if(ENABLE_SANITIZERS)
        set(ENABLE_ADDRESSSANITIZER ON)
        set(ENABLE_LEAKSANITIZER ON) 
        set(ENABLE_UBSANITIZER ON)
        message(STATUS "Enabling all sanitizers for memory leak investigation")
    endif()

    # ThreadSanitizer is mutually exclusive with AddressSanitizer
    if(ENABLE_THREADSANITIZER AND (ENABLE_ADDRESSSANITIZER OR ENABLE_LEAKSANITIZER))
        message(FATAL_ERROR "ThreadSanitizer cannot be used with AddressSanitizer or LeakSanitizer")
    endif()

    # AddressSanitizer
    if(ENABLE_ADDRESSSANITIZER)
        list(APPEND SANITIZER_FLAGS "-fsanitize=address")
        list(APPEND SANITIZER_FLAGS "-fno-omit-frame-pointer")
        message(STATUS "AddressSanitizer enabled")
    endif()

    # LeakSanitizer (requires AddressSanitizer)
    if(ENABLE_LEAKSANITIZER)
        if(NOT ENABLE_ADDRESSSANITIZER)
            message(STATUS "LeakSanitizer requires AddressSanitizer - enabling both")
            set(ENABLE_ADDRESSSANITIZER ON)
            list(APPEND SANITIZER_FLAGS "-fsanitize=address")
            list(APPEND SANITIZER_FLAGS "-fno-omit-frame-pointer")
        endif()
        list(APPEND SANITIZER_FLAGS "-fsanitize=leak")
        message(STATUS "LeakSanitizer enabled for memory leak detection")
    endif()

    # UndefinedBehaviorSanitizer
    if(ENABLE_UBSANITIZER)
        list(APPEND SANITIZER_FLAGS "-fsanitize=undefined")
        message(STATUS "UndefinedBehaviorSanitizer enabled")
    endif()

    # ThreadSanitizer
    if(ENABLE_THREADSANITIZER)
        list(APPEND SANITIZER_FLAGS "-fsanitize=thread")
        message(STATUS "ThreadSanitizer enabled")
    endif()

    # Apply sanitizer flags if any are enabled
    if(SANITIZER_FLAGS)
        string(JOIN " " SANITIZER_FLAGS_STR ${SANITIZER_FLAGS})
        
        # Add to compile and link flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS_STR}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS_STR}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS_STR}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_FLAGS_STR}")
        
        message(STATUS "Sanitizer flags applied: ${SANITIZER_FLAGS_STR}")
        
        # Environment variable hints for runtime
        message(STATUS "")
        message(STATUS "=== SANITIZER RUNTIME ENVIRONMENT ===")
        message(STATUS "For best results, set these environment variables:")
        if(ENABLE_LEAKSANITIZER OR ENABLE_ADDRESSSANITIZER)
            message(STATUS "  export ASAN_OPTIONS=\"detect_leaks=1:abort_on_error=1:check_initialization_order=1\"")
            message(STATUS "  export LSAN_OPTIONS=\"print_stats=1:report_objects=1\"")
        endif()
        if(ENABLE_UBSANITIZER)
            message(STATUS "  export UBSAN_OPTIONS=\"print_stacktrace=1:abort_on_error=1\"")
        endif()
        message(STATUS "=======================================")
        message(STATUS "")
    endif()
else()
    if(ENABLE_SANITIZERS OR ENABLE_ADDRESSSANITIZER OR ENABLE_LEAKSANITIZER OR ENABLE_UBSANITIZER OR ENABLE_THREADSANITIZER)
        message(WARNING "Sanitizers requested but not supported by current compiler")
    endif()
endif()