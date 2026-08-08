# Runs a command and retries it once when it died from the LeakSanitizer
# stop-the-world tracer crash - a sanitizer runtime bug that has nothing to do
# with the code under test. See asan-suppressions.txt for the analysis; the
# short version is that LeakSanitizer's tracer segfaults dereferencing a range
# it was told to scan, aborts the whole leak check, and exits non-zero without
# ever reporting a leak.
#
# Usage:
#   cmake -P lsan-tracer-retry.cmake -- [--failure-marker=<path>] <command> [args...]
#
# The retry needs positive evidence that the run itself was clean, otherwise a
# genuine failure that happens to be followed by a tracer crash would be
# retried too. By default that evidence is QTest's own summary line. Runners
# that print no such summary - the Lua suite, whose summary LeakSanitizer's
# Die() eats along with the rest of the unflushed stdio - pass
# --failure-marker=<path> instead, naming the file they write when their tests
# fail; the run counts as clean while that file is absent.
#
# A real leak, a failing test, a hang killed from outside, or a second tracer
# crash all still fail.
cmake_minimum_required(VERSION 3.25.1)

set(failureMarker "")
set(command "")

if(CMAKE_ARGC GREATER 3)
    math(EXPR lastArgumentIndex "${CMAKE_ARGC} - 1")
    foreach(argumentIndex RANGE 3 ${lastArgumentIndex})
        set(argument "${CMAKE_ARGV${argumentIndex}}")
        if(NOT command)
            if(argument STREQUAL "--")
                continue()
            elseif(argument MATCHES "^--failure-marker=(.*)$")
                set(failureMarker "${CMAKE_MATCH_1}")
                continue()
            endif()
        endif()
        list(APPEND command "${argument}")
    endforeach()
endif()

if(NOT command)
    message(FATAL_ERROR "lsan-tracer-retry: no command given")
endif()

# The output is echoed as it arrives as well as captured, so wrapping a test
# does not change what ctest shows for it.
function(runCommand resultVariable logVariable)
    execute_process(
        COMMAND ${command}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE standardOutput
        ERROR_VARIABLE standardError
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
    )
    set(${resultVariable} "${result}" PARENT_SCOPE)
    set(${logVariable} "${standardOutput}${standardError}" PARENT_SCOPE)
endfunction()

function(tracerCrashedWithoutFailures log outVariable)
    set(${outVariable} FALSE PARENT_SCOPE)
    if(NOT "${log}" MATCHES "Tracer caught signal")
        return()
    endif()
    if(NOT "${log}" MATCHES "LeakSanitizer has encountered a fatal error")
        return()
    endif()
    if(failureMarker)
        if(EXISTS "${failureMarker}")
            return()
        endif()
    elseif(NOT "${log}" MATCHES "Totals: [0-9]+ passed, 0 failed")
        return()
    endif()
    set(${outVariable} TRUE PARENT_SCOPE)
endfunction()

list(GET command 0 commandName)
get_filename_component(commandName "${commandName}" NAME)

runCommand(result log)
if(NOT result STREQUAL "0")
    tracerCrashedWithoutFailures("${log}" retryable)
    if(retryable)
        message("")
        message("=== ${commandName} passed but LeakSanitizer's tracer crashed on the way out ===")
        message("=== this is the known sanitizer bug documented in asan-suppressions.txt, ===")
        message("=== not a leak and not a test failure, so running it once more ===")
        message("")
        # ctest swallows the output of a test that ends up passing, so leave a
        # trace in the job summary - otherwise there is no way to tell how often
        # this fires
        if(DEFINED ENV{GITHUB_STEP_SUMMARY})
            file(APPEND "$ENV{GITHUB_STEP_SUMMARY}"
                "Retried `${commandName}`: LeakSanitizer tracer crash, not a leak\n")
        endif()
        runCommand(result log)
    endif()
endif()

if(NOT result STREQUAL "0")
    message(FATAL_ERROR "${commandName} failed: ${result}")
endif()
