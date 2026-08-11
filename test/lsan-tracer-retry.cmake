# Runs a command and reruns it once when LeakSanitizer's stop-the-world tracer
# crashed on the way out - a sanitizer runtime bug that says nothing about the
# code under test. See asan-suppressions.txt for the analysis; the short version
# is that the tracer segfaults dereferencing a range it was told to scan, aborts
# the leak check, and exits non-zero without ever reporting a leak.
#
# Usage:
#   cmake -P lsan-tracer-retry.cmake -- [--failure-marker=<path>] <command> [args...]
#
# Retrying needs positive evidence that the run itself was clean, or a genuine
# failure that happened to be followed by a tracer crash would be retried too.
# By default that evidence is QTest's own summary line. Runners that print no
# such summary - the Lua suite, whose summary LeakSanitizer's Die() eats along
# with the rest of the unflushed stdio - pass --failure-marker=<path> instead,
# naming the file they write when their tests fail; the run counts as clean
# while that file is absent.
#
# A leak report, a failing test, a hang killed from outside and a second tracer
# crash all still fail. There is exactly one retry.
cmake_minimum_required(VERSION 3.25.1)

set(failureMarker "")
set(command "")
set(parsingOptions TRUE)

if(CMAKE_ARGC GREATER 3)
    math(EXPR lastArgumentIndex "${CMAKE_ARGC} - 1")
    foreach(argumentIndex RANGE 3 ${lastArgumentIndex})
        set(argument "${CMAKE_ARGV${argumentIndex}}")
        if(parsingOptions)
            if(argument STREQUAL "--")
                continue()
            elseif(argument MATCHES "^--failure-marker=(.*)$")
                set(failureMarker "${CMAKE_MATCH_1}")
                continue()
            endif()
            set(parsingOptions FALSE)
        endif()
        list(APPEND command "${argument}")
    endforeach()
endif()

if(NOT command)
    message(FATAL_ERROR "lsan-tracer-retry: no command given")
endif()

# The output is echoed as it arrives as well as captured, so wrapping a test
# does not change what ctest shows for it.
function(run_command resultVariable logVariable)
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

function(tracer_crashed_after_a_clean_run log outVariable)
    set(${outVariable} FALSE PARENT_SCOPE)

    # Both halves. LeakSanitizer announces its other fatal errors with the same
    # second line, and the tracer prints the first one for signals it recovers
    # from, so either alone is a different situation
    if(NOT "${log}" MATCHES "Tracer caught signal")
        return()
    endif()
    if(NOT "${log}" MATCHES "LeakSanitizer has encountered a fatal error")
        return()
    endif()

    # The crash aborts before reporting begins, so a report should be impossible
    # here - but rerunning a run that found something is the one mistake this
    # launcher must not make
    if("${log}" MATCHES "(ERROR|SUMMARY): [A-Za-z]*Sanitizer:")
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

run_command(result log)
if(NOT result STREQUAL "0")
    tracer_crashed_after_a_clean_run("${log}" retryable)
    if(retryable)
        set(notice
            "lsan-tracer-retry: rerunning ${commandName} - it passed, then LeakSanitizer's tracer crashed on the way out (asan-suppressions.txt)")
        message("${notice}")
        # ctest swallows the output of a test that ends up passing, so leave a
        # trace in the job summary too - otherwise there is no way to tell how
        # often this fires
        if(DEFINED ENV{GITHUB_STEP_SUMMARY})
            file(APPEND "$ENV{GITHUB_STEP_SUMMARY}" "${notice}\n")
        endif()
        run_command(result log)
    endif()
endif()

if(NOT result STREQUAL "0")
    message(FATAL_ERROR "${commandName} failed: ${result}")
endif()
