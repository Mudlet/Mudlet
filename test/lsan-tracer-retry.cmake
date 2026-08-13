# Runs a command and reruns it once when LeakSanitizer's stop-the-world tracer
# died on the way out (#9809) - a sanitizer runtime bug that says nothing about
# the code under test, and that no entry in asan-suppressions.txt can reach,
# because it aborts the leak check before any leak is reported.
#
# Usage:
#   cmake -P lsan-tracer-retry.cmake -- [--failure-marker=<path>] <command> [args...]
#
# Rerunning takes evidence that the run itself was clean, or a genuine failure
# that happened to be followed by a tracer crash would be rerun too. By default
# that evidence is QTest's summary line. Busted prints no such line, so the Lua
# suite passes --failure-marker=<path> instead, naming the file it writes when
# its tests fail, and counts as clean while that file is absent. Absence is
# weaker evidence than a summary - a suite that never started leaves no marker
# either - so that mode leans on the caller checking the marker again afterwards
# the way both workflows do.
#
# A leak report, a failing test, a hang killed from outside and a second tracer
# crash all still fail. There is exactly one rerun, and it shares the caller's
# timeout rather than widening it. The command's own exit status is not
# preserved: every failure comes back from here as 1. Arguments reach the command
# as given except that a semicolon in one would split it, CMake lists being what
# they are; a mistyped option ahead of the command is run as the command, and
# fails saying so.
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

# run-tests.xml writes to MUDLET_TEST_FAILURE_MARKER when it is set, and only
# falls back to the path the workflow passes here if that write fails, so both
# have to be checked or a failed suite could look unmarked
set(failureMarkers "")
if(failureMarker)
    list(APPEND failureMarkers "${failureMarker}")
endif()
if(NOT "$ENV{MUDLET_TEST_FAILURE_MARKER}" STREQUAL "")
    list(APPEND failureMarkers "$ENV{MUDLET_TEST_FAILURE_MARKER}")
endif()

# Echoed as it arrives as well as captured, so ctest still shows everything the
# command printed
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

    # Both halves, and the signal number too. LeakSanitizer prints the second
    # line for its other fatal errors as well, and a tracer death on some other
    # signal is a failure nobody has looked at yet.
    if(NOT "${log}" MATCHES "Tracer caught signal 11")
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

    if(failureMarkers)
        foreach(marker ${failureMarkers})
            if(EXISTS "${marker}")
                return()
            endif()
        endforeach()
        # At least one test has to have passed: a run that skipped everything
        # reports "0 passed, 0 failed" and proves nothing
    elseif(NOT "${log}" MATCHES "Totals: [1-9][0-9]* passed, 0 failed")
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
            "lsan-tracer-retry: rerunning ${commandName} - it passed, then LeakSanitizer's tracer crashed on the way out (#9809)")
        message("${notice}")
        # The run that crashed is the only evidence of #9809 there is, and ctest
        # shows nothing for a test that goes on to pass, so carry the tracer's own
        # trace out with the notice: the thread ranges it walked, in order, and
        # the address it died on. The last range named is the one it faulted in.
        # LSAN_OPTIONS=log_threads=1 in the test environment is what fills it in;
        # without that only the crash line is here.
        string(REGEX MATCHALL "[^\n]*(Processing thread|Stack at|TLS at|DTLS |Tracer caught signal)[^\n]*"
            tracerTrace "${log}")
        list(JOIN tracerTrace "\n      " tracerTrace)
        # ctest swallows the output of a test that ends up passing, so leave a
        # trace in the job summary too - otherwise there is no way to tell how
        # often this fires. Skipped unless the path really is a file to append
        # to: file(APPEND) is fatal, and reporting the flake must never be what
        # fails a run.
        if(EXISTS "$ENV{GITHUB_STEP_SUMMARY}" AND NOT IS_DIRECTORY "$ENV{GITHUB_STEP_SUMMARY}")
            file(APPEND "$ENV{GITHUB_STEP_SUMMARY}"
                "- ${notice}\n\n      ${tracerTrace}\n\n")
        endif()
        run_command(result log)
    endif()
endif()

if(NOT result STREQUAL "0")
    string(JOIN " " commandLine ${command})
    message(FATAL_ERROR "${commandName} failed: ${result}\n  ${commandLine}")
endif()
