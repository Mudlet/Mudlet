if(NOT WIN32)
    include(${CMAKE_SOURCE_DIR}/src/cmake/EnableSanitizers.cmake)
endif()

set(FUNCTIONAL_TEST_SOURCES
    ConfigDirOverrideTest.cpp
    TelnetTextDisplayedTest.cpp
    TelnetSubnegotiationTest.cpp
    TelnetSgrDefaultColorTest.cpp
    TelnetTlsPromptTest.cpp
    TelnetBenchmark.cpp
    ResetProfileTest.cpp
    TOscTest.cpp
    TUserWindowTest.cpp
    TriggerEditorTest.cpp
    TFeedTriggersRecursionTest.cpp
    TriggerSameLineMatchTest.cpp
    dlgTriggerEditorUndoRedoTest.cpp
    TDiscordModeTest.cpp
    MainConsoleSelectionTest.cpp
    EnableDisableByNameTest.cpp
    ColorTriggerFilterChildTest.cpp
    GMCPCharLoginTest.cpp
    SetScriptCallbackTest.cpp
    ClearWindowLogTest.cpp
    XMLexportVariablesTest.cpp
    SgrUnderlineStyleTest.cpp
    LogRestartDuplicateLineTest.cpp
    ProfileRoundTripTest.cpp
    ProfileLoadTempFileTest.cpp
    PackageSelfUninstallTest.cpp
    MapRoundTripTest.cpp
    UndoServerWrapTest.cpp
    ActionSelfUninstallTest.cpp
)

set(FUNCTIONAL_TEST_UTILS
    TelnetServerStub.cpp
    DiscordIpcServerStub.cpp
)

# Manual replay tool for verifying mUndoServerWrap against captured game
# output - built but deliberately not registered with ctest:
add_executable(UndoServerWrapReplay UndoServerWrapReplay.cpp ${FUNCTIONAL_TEST_UTILS})
add_dependencies(UndoServerWrapReplay ${LIB_MUDLET_TARGET})
target_link_libraries(UndoServerWrapReplay PRIVATE Qt6::Test ${LIB_MUDLET_TARGET})
set_target_properties(UndoServerWrapReplay PROPERTIES ENABLE_EXPORTS ON)

# Report-only perf harness for manual before/after comparisons
# (test/compare-perf-baseline.py runs the built binary directly). Built with the
# functional tests so a binary always exists, but kept OUT of the default ctest
# suite: it feeds a huge fixed corpus many times and makes no timing assertions,
# so running it every CI pipeline only burns minutes. -DREGISTER_PERF_BENCHMARK=ON
# also registers it with ctest.
option(REGISTER_PERF_BENCHMARK "Register the report-only PipelineBenchmark with ctest (it is built either way)" OFF)
add_executable(PipelineBenchmark PipelineBenchmark.cpp ${FUNCTIONAL_TEST_UTILS})
add_dependencies(PipelineBenchmark ${LIB_MUDLET_TARGET})
target_link_libraries(PipelineBenchmark PRIVATE Qt6::Test ${LIB_MUDLET_TARGET})
set_target_properties(PipelineBenchmark PROPERTIES ENABLE_EXPORTS ON)
if(REGISTER_PERF_BENCHMARK)
    add_test(NAME PipelineBenchmark COMMAND $<TARGET_FILE:PipelineBenchmark>)
    # large corpus fed repeatedly, so allow a generous timeout
    set_tests_properties(PipelineBenchmark PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;ASAN_OPTIONS=detect_leaks=0"
        LABELS "functional"
        TIMEOUT 600)
endif()

foreach(test_file ${FUNCTIONAL_TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file} ${FUNCTIONAL_TEST_UTILS})
    add_dependencies(${test_name} ${LIB_MUDLET_TARGET})
    target_link_libraries(${test_name} PRIVATE Qt6::Test ${LIB_MUDLET_TARGET})
    set_target_properties(${test_name} PROPERTIES ENABLE_EXPORTS ON)
    add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;ASAN_OPTIONS=detect_leaks=0"
        LABELS "functional"
        TIMEOUT 60  # seconds
    )
endforeach()

# ResetProfileTest creates a full profile per test method (30 tests),
# so it needs a longer timeout than the default 60s
set_tests_properties(ResetProfileTest PROPERTIES TIMEOUT 300)

# Undo/redo tests need a longer timeout due to large batch operations
set_tests_properties(dlgTriggerEditorUndoRedoTest PROPERTIES TIMEOUT 300)

# GMCPCharLoginTest creates a fresh profile per test method, so it needs a longer timeout
set_tests_properties(GMCPCharLoginTest PROPERTIES TIMEOUT 300)

# LogRestartDuplicateLineTest creates a fresh profile per test method, so it needs a longer timeout
set_tests_properties(LogRestartDuplicateLineTest PROPERTIES TIMEOUT 300)

# The round-trip tests boot a full mudlet instance and save/reload profile and
# map data, so they need a longer timeout
set_tests_properties(ProfileRoundTripTest MapRoundTripTest PROPERTIES TIMEOUT 300)

# TDiscordModeTest drives the real discord-rpc library end-to-end. The library's
# reconnect backoff is a process-global (60s ceiling) that the suite's
# init/shutdown churn can inflate, so a fresh handshake can take a while (see
# establishDiscordLogin in TDiscordModeTest.cpp). 300s covers up to three such
# waits - the warm-up plus a per-test fallback in each end-to-end test, ~195s
# worst case - well beyond the default 60s.
set_tests_properties(TDiscordModeTest PROPERTIES TIMEOUT 300)

# Let TDiscordModeTest find the bundled discord-rpc library so the Lua API
# permission gating tests can run (they skip when it is unavailable)
if(WIN32)
    set(DISCORD_RPC_PATH_VARIABLE "PATH")
elseif(APPLE)
    set(DISCORD_RPC_PATH_VARIABLE "DYLD_LIBRARY_PATH")
else()
    set(DISCORD_RPC_PATH_VARIABLE "LD_LIBRARY_PATH")
endif()
set_property(TEST TDiscordModeTest APPEND PROPERTY
    ENVIRONMENT_MODIFICATION "${DISCORD_RPC_PATH_VARIABLE}=path_list_prepend:${CMAKE_SOURCE_DIR}/3rdparty/discord/rpc/lib")
