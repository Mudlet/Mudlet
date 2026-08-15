if(NOT WIN32)
    include(${CMAKE_SOURCE_DIR}/src/cmake/EnableSanitizers.cmake)
endif()

# Defined in the parent test/CMakeLists.txt; TESTS is per-directory, so call it here too.
cmake_language(DEFER CALL restore_windows_test_output)

set(FUNCTIONAL_TEST_SOURCES
    ConfigDirOverrideTest.cpp
    TelnetTextDisplayedTest.cpp
    TelnetSubnegotiationTest.cpp
    TelnetSgrDefaultColorTest.cpp
    TelnetTlsPromptTest.cpp
    TelnetBenchmark.cpp
    cTelnetBufferTest.cpp
    DefaultGameDeleteTest.cpp
    ResetProfileTest.cpp
    TOscTest.cpp
    TUserWindowTest.cpp
    SubCommandLineLifetimeTest.cpp
    TriggerEditorTest.cpp
    TFeedTriggersRecursionTest.cpp
    TriggerSameLineMatchTest.cpp
    dlgTriggerEditorUndoRedoTest.cpp
    EditorBannerViewSwitchTest.cpp
    TDiscordModeTest.cpp
    WindowStateGettersTest.cpp
    MainConsoleSelectionTest.cpp
    MapAreaReassignmentTest.cpp
    TelnetStringSequenceRecoveryTest.cpp
    EnableDisableByNameTest.cpp
    UnitDeferredDeleteTest.cpp
    ProfileLifecycleTest.cpp
    MxpFramePlacementTest.cpp
    TelnetReconnectStateTest.cpp
    SavedVariableFenceTest.cpp
    ColorTriggerFilterChildTest.cpp
    CsiCursorForwardTest.cpp
    CopyAsImageTest.cpp
    GlyphOverflowTest.cpp
    GMCPCharLoginTest.cpp
    VariableEditorWriteBackTest.cpp
    InsertTextCapTest.cpp
    ScriptEventHandlerLifetimeTest.cpp
    SetScriptCallbackTest.cpp
    ClearWindowLogTest.cpp
    ProfileSaveShutdownRaceTest.cpp
    XMLexportVariablesTest.cpp
    SgrUnderlineStyleTest.cpp
    LogRestartDuplicateLineTest.cpp
    ProfileRoundTripTest.cpp
    ProfileLoadTempFileTest.cpp
    PackageSelfRemovalTest.cpp
    UnitProcessingDepthTest.cpp
    PackageRemovalSaveTeardownTest.cpp
    ModuleSaveTeardownTest.cpp
    MapRoundTripTest.cpp
    MapProgressDialogSeamTest.cpp
    MapCloseDuringImportTest.cpp
    StarterUiSectionsTest.cpp
    TtsInterruptingSpeakTest.cpp
    UndoServerWrapTest.cpp
    TelnetConnectLookupRaceTest.cpp
    NarrowWindowWrapTest.cpp
    HostWidgetDecouplingTest.cpp
    ActionSelfRemovalTest.cpp
    ProfileFolderNameTest.cpp
    ProfileDeletionSafetyTest.cpp
    DialogTeardownTest.cpp
    HostChildTeardownTest.cpp
    ConnectionDialogCrashTest.cpp
    EdbeeReinitTest.cpp
    TMediaLoopTest.cpp
    DefaultPackagesTest.cpp
    StarterUiTriggerCostTest.cpp
    StarterUiGaugePanelTest.cpp
    ProfileSwitchShortcutTest.cpp
    ExperiencedPlayerGateTest.cpp
    WindowBackgroundTest.cpp
    EmbeddedMapperCreationTest.cpp
)

# The updater sources are only built with USE_UPDATER, and on macOS the
# Updater wraps Sparkle instead of creating an UpdateDialog
if(USE_UPDATER AND NOT APPLE)
    list(APPEND FUNCTIONAL_TEST_SOURCES NewReleaseDialogTeardownTest.cpp)
endif()

set(FUNCTIONAL_TEST_UTILS
    TelnetServerStub.cpp
    DiscordIpcServerStub.cpp
)

# Manual replay tool for verifying mUndoServerWrap against captured game
# output - built but deliberately not registered with ctest:
add_executable(UndoServerWrapReplay UndoServerWrapReplay.cpp ${FUNCTIONAL_TEST_UTILS})
add_dependencies(UndoServerWrapReplay ${LIB_MUDLET_TARGET})
target_link_libraries(UndoServerWrapReplay PRIVATE Qt6::Test ${LIB_MUDLET_TARGET} mudlet_lsan_hooks)
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
target_link_libraries(PipelineBenchmark PRIVATE Qt6::Test ${LIB_MUDLET_TARGET} mudlet_lsan_hooks)
set_target_properties(PipelineBenchmark PROPERTIES ENABLE_EXPORTS ON)
if(REGISTER_PERF_BENCHMARK)
    add_test(NAME PipelineBenchmark COMMAND $<TARGET_FILE:PipelineBenchmark>)
    # large corpus fed repeatedly, so allow a generous timeout
    set_tests_properties(PipelineBenchmark PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;ASAN_OPTIONS=detect_leaks=0"
        LABELS "functional"
        TIMEOUT 600)
endif()

# Tests that run with the leak check off:
# - dlgTriggerEditorUndoRedoTest still leaks, never destroying its
#   dlgTriggerEditor and leaving ~3.4MB of tree-item QIcon/QPixmap behind
# - NewReleaseDialogTeardownTest still leaks, on Qt's one-time system
#   CA-certificate store load on first TLS use, cached for the process lifetime
# - TriggerSameLineMatchTest replaces ASAN_OPTIONS further down to add an RSS
#   ceiling, which drops detect_leaks with it
set(leakCheckExcludedTests
    dlgTriggerEditorUndoRedoTest
    NewReleaseDialogTeardownTest
    TriggerSameLineMatchTest
)

# Apple's ASan runtime has no LeakSanitizer, so detect_leaks is only meaningful
# where the address or leak sanitizer is actually compiled in
if(NOT APPLE AND "${USE_SANITIZER}" MATCHES "[Aa]ddress|[Ll]eak")
    set(leakCheckAvailable TRUE)
else()
    set(leakCheckAvailable FALSE)
endif()

# mudlet_lsan_hooks has to be linked explicitly, see src/CMakeLists.txt
foreach(test_file ${FUNCTIONAL_TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file} ${FUNCTIONAL_TEST_UTILS})
    add_dependencies(${test_name} ${LIB_MUDLET_TARGET})
    target_link_libraries(${test_name} PRIVATE Qt6::Test ${LIB_MUDLET_TARGET} mudlet_lsan_hooks)
    set_target_properties(${test_name} PROPERTIES ENABLE_EXPORTS ON)
    if(leakCheckAvailable AND NOT test_name IN_LIST leakCheckExcludedTests)
        # intercept_tls_get_addr=0 is what stops the tracer crash of #9809, at
        # the cost of not scanning dynamic TLS blocks for roots. Up to GCC 13 and
        # LLVM 16, the sanitizer runtime's __tls_get_addr interceptor - not
        # LeakSanitizer itself, which is why this is an ASAN_OPTIONS flag -
        # decides such a block carries a pre-2.25 glibc header when
        # `tls_beg % 4096` equals sizeof(Glibc_2_19_tls_header), 16 on 64-bit,
        # and reads the two words before it as the block's address and length.
        # Since glibc 2.25 there is no header there and a malloc'd block lands on
        # that offset by coincidence - roughly 1 in 256, always for the
        # mmap-backed ones - so the range recorded is garbage and the tracer dies
        # scanning it. That coincidence is why #9809 reads as a flake.
        # google/sanitizers#1322; GCC 14 and LLVM 17 replaced the guess with an
        # allocator lookup, so all four copies of this flag - here, the two
        # build-mudlet workflows, and src/LsanHooks.cpp - can go once the legs
        # that need them build with one of those.
        set(leakCheck "detect_leaks=1:intercept_tls_get_addr=0")
        # Kept as insurance rather than for a crash we still expect: if the flag
        # above ever proves incomplete, the tracer prints the stack and TLS range
        # of each thread as it reaches one, so the last line before a crash names
        # the range it died scanning - the one thing the crash line alone does
        # not say (#9809). Seven lines a run, and only on a failing test, since
        # ctest hides the output of one that passes.
        set(leakLog "LSAN_OPTIONS=log_threads=1;")
    else()
        set(leakCheck "detect_leaks=0")
        set(leakLog "")
    endif()
    add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${leakLog}ASAN_OPTIONS=${leakCheck}"
        LABELS "functional"
        TIMEOUT 60  # seconds
    )
endforeach()

# ResetProfileTest creates a full profile per test method (30 tests),
# so it needs a longer timeout than the default 60s
set_tests_properties(ResetProfileTest PROPERTIES TIMEOUT 300)

# Undo/redo tests need a longer timeout due to large batch operations
set_tests_properties(dlgTriggerEditorUndoRedoTest PROPERTIES TIMEOUT 300)

# A regression in the self-re-creating-trigger cases does not fail, it hangs and
# grows the heap by ~110MB/s: the RSS ceiling and the timeout are what turn that
# into a reported failure. ENVIRONMENT replaces the loop's, hence QT_QPA_PLATFORM
# again, and the lost detect_leaks is why the test is in leakCheckExcludedTests.
set_tests_properties(TriggerSameLineMatchTest PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;ASAN_OPTIONS=detect_leaks=0:hard_rss_limit_mb=3000"
    TIMEOUT 300)

# GMCPCharLoginTest creates a fresh profile per test method, so it needs a longer timeout
set_tests_properties(GMCPCharLoginTest PROPERTIES TIMEOUT 600)

# TelnetReconnectStateTest creates a fresh profile per test method and reconnects it several
# times per method, so it needs a longer timeout
set_tests_properties(TelnetReconnectStateTest PROPERTIES TIMEOUT 300)

# TelnetConnectLookupRaceTest creates a fresh profile per test method and waits out a connect to
# an address nothing answers on, so it needs a longer timeout
set_tests_properties(TelnetConnectLookupRaceTest PROPERTIES TIMEOUT 300)

# GlyphOverflowTest creates a fresh profile per test method and sweeps two font
# families across 22 sizes, so it needs a longer timeout
set_tests_properties(GlyphOverflowTest PROPERTIES TIMEOUT 600)

# InsertTextCapTest creates a fresh profile per test method, so it needs a longer timeout
set_tests_properties(InsertTextCapTest PROPERTIES TIMEOUT 300)

# LogRestartDuplicateLineTest creates a fresh profile per test method, so it needs a longer timeout
set_tests_properties(LogRestartDuplicateLineTest PROPERTIES TIMEOUT 300)

# ProfileLifecycleTest opens and closes several profiles, and every open is a
# full profile load and every close a full save, so it needs a longer timeout
set_tests_properties(ProfileLifecycleTest PROPERTIES TIMEOUT 300)

# TMediaLoopTest probes the audio backend and creates a fresh profile per test method,
# and each clip has to be waited out in real time, so it needs a longer timeout.
# QTEST_MAIN does not run src/main.cpp, so pin QT_MEDIA_BACKEND to what main.cpp picks for
# the shipped application - otherwise the tests silently exercise Qt's default backend for
# the platform rather than the one users actually get.
if(APPLE)
    set(mediaLoopTestBackend "darwin")
elseif(WIN32)
    set(mediaLoopTestBackend "ffmpeg")
else()
    set(mediaLoopTestBackend "")
endif()
set_tests_properties(TMediaLoopTest PROPERTIES TIMEOUT 300)
if(mediaLoopTestBackend)
    # APPEND, so the environment set by the loop above stays in force rather than being replaced
    set_property(TEST TMediaLoopTest APPEND PROPERTY ENVIRONMENT "QT_MEDIA_BACKEND=${mediaLoopTestBackend}")
endif()

# StarterUiSectionsTest creates a fresh profile per test method and one of them
# waits out the delay before the interface comes back on duty, so it needs a
# longer timeout
set_tests_properties(StarterUiSectionsTest PROPERTIES TIMEOUT 300)

# The round-trip tests boot a full mudlet instance and save/reload profile and
# map data, so they need a longer timeout
set_tests_properties(ProfileRoundTripTest MapRoundTripTest MapProgressDialogSeamTest MapCloseDuringImportTest PROPERTIES TIMEOUT 300)

# MapAreaReassignmentTest reports a cost curve built from thousands of rooms,
# which takes minutes rather than milliseconds if the quadratic term returns
set_tests_properties(MapAreaReassignmentTest PROPERTIES TIMEOUT 300)

# HostWidgetDecouplingTest creates a fresh profile per test method, so it needs a longer timeout
set_tests_properties(HostWidgetDecouplingTest PROPERTIES TIMEOUT 300)

# ConnectionDialogCrashTest waits out two real profile copies (15s budget each)
# on top of a full mudlet start under ASan, so the 60s default is too tight for
# a loaded machine even though the file runs in ~2s
set_tests_properties(ConnectionDialogCrashTest PROPERTIES TIMEOUT 300)

# NarrowWindowWrapTest creates a fresh profile per test method, so it needs a longer timeout
set_tests_properties(NarrowWindowWrapTest PROPERTIES TIMEOUT 300)

# TelnetStringSequenceRecoveryTest runs its introducer matrix over a real
# socket, waiting out cTelnet's 300ms idle flush in several cases and
# reconnecting in others, so it needs a longer timeout
set_tests_properties(TelnetStringSequenceRecoveryTest PROPERTIES TIMEOUT 300)

# CopyAsImageTest creates a fresh profile per test method, so it needs a longer timeout
set_tests_properties(CopyAsImageTest PROPERTIES TIMEOUT 300)

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
