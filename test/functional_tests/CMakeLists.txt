if(NOT WIN32)
    include(${CMAKE_SOURCE_DIR}/src/cmake/EnableSanitizers.cmake)
endif()

# Defined in the parent test/CMakeLists.txt; TESTS is per-directory, so call it here too.
cmake_language(DEFER CALL restore_windows_test_output)

set(FUNCTIONAL_TEST_SOURCES
)

# The updater sources are only built with USE_UPDATER, and on macOS the
# Updater wraps Sparkle instead of creating an UpdateDialog
if(USE_UPDATER AND NOT APPLE)
    list(APPEND FUNCTIONAL_TEST_SOURCES NewReleaseDialogTeardownTest.cpp)
endif()

# dblsqd::Feed itself is built on every platform that has the updater
if(USE_UPDATER)
    list(APPEND FUNCTIONAL_TEST_SOURCES FeedChecksumRaceTest.cpp)
endif()

# Tests that share one executable rather than getting one each. A functional
# test binary statically links mudlet_core and costs a build tree ~250MB plus a
# link step, and none of that is per-test - so tests of one subsystem are built
# together and told on the command line which class to run. See GroupedTest.h.
#
# A new test belongs in its subsystem's list below if there is one, and then
# writes MUDLET_GROUPED_TEST_MAIN() rather than QTEST_MAIN(); a subsystem with
# no list yet still belongs in FUNCTIONAL_TEST_SOURCES. A grouped test's class
# name has to match its filename, since that is the name ctest passes on the
# command line and the group binary looks up.

# Windows, consoles and the text they draw
set(WINDOW_GROUP_TEST_SOURCES
    CopyAsImageTest.cpp
    GlyphOverflowTest.cpp
    LabelMovieRefusalTest.cpp
    MainConsoleSelectionTest.cpp
    MainWindowSizeReportTest.cpp
    MxpFramePlacementTest.cpp
    MxpWatchdogBufferLifetimeTest.cpp
    NarrowWindowWrapTest.cpp
    ProfileSwitchMiniconsoleTest.cpp
    ProfileSwitchShortcutTest.cpp
    SubCommandLineLifetimeTest.cpp
    WindowBackgroundTest.cpp
    WindowLayoutSaveTest.cpp
    WindowStateGettersTest.cpp
    WrapLineRewrapTest.cpp
)

# Profiles: their config dir, their folders, and loading, saving and deleting them
set(PROFILE_GROUP_TEST_SOURCES
    ConfigDirOverrideTest.cpp
    ConnectionDialogCrashTest.cpp
    DefaultGameDeleteTest.cpp
    ProfileDeletionSafetyTest.cpp
    ProfileFolderNameTest.cpp
    ProfileLifecycleTest.cpp
    ProfileLoadTempFileTest.cpp
    ProfileRoundTripTest.cpp
    ProfileSaveShutdownRaceTest.cpp
    ResetProfileTest.cpp
    SavedVariableFenceTest.cpp
    XMLexportVariablesTest.cpp
)

# Telnet, and the protocols and escape sequences carried over it
set(TELNET_GROUP_TEST_SOURCES
    CsiCursorForwardTest.cpp
    cTelnetBufferTest.cpp
    GMCPCharLoginTest.cpp
    TelnetBenchmark.cpp
    TelnetCharsetNegotiationTest.cpp
    TelnetConnectFailureTest.cpp
    TelnetConnectLookupRaceTest.cpp
    TelnetReconnectStateTest.cpp
    TelnetStringSequenceRecoveryTest.cpp
    TelnetTlsPromptTest.cpp
    TOscTest.cpp
)

# Triggers and the other scriptable units, and what a running script does to them
set(TRIGGER_GROUP_TEST_SOURCES
    CorruptTriggerPatternsTest.cpp
    MultilineTriggerReentrancyTest.cpp
    SetScriptCallbackTest.cpp
    TFeedTriggersRecursionTest.cpp
    UnitDeferredDeleteTest.cpp
    UnitProcessingDepthTest.cpp
)

# The script editor, and the trigger, variable and script forms it hosts
set(EDITOR_GROUP_TEST_SOURCES
    dlgTriggerEditorUndoRedoTest.cpp
    EdbeeReinitTest.cpp
    EditorBannerViewSwitchTest.cpp
    ScriptEventHandlerLifetimeTest.cpp
    TriggerEditorTest.cpp
    VariableEditorWriteBackTest.cpp
)

# The map: its rooms and areas, its file format, and the mapper widget
set(MAP_GROUP_TEST_SOURCES
    EmbeddedMapperCreationTest.cpp
    MapAreaReassignmentTest.cpp
    MapCloseDuringImportTest.cpp
    MapProgressDialogSeamTest.cpp
    MapRoundTripTest.cpp
)

# Packages and modules: installing them, saving them and removing them
set(PACKAGE_GROUP_TEST_SOURCES
    ActionSelfRemovalTest.cpp
    DefaultPackagesTest.cpp
    ModuleSaveTeardownTest.cpp
    PackageRemovalSaveTeardownTest.cpp
    PackageSelfRemovalTest.cpp
)

# Onboarding and in-app guidance: the starter UI, the tutorial invitation,
# the feature callouts and the experienced-player gate
set(STARTERUI_GROUP_TEST_SOURCES
    ExperiencedPlayerGateTest.cpp
    FeatureCalloutTest.cpp
    StarterUiGaugePanelTest.cpp
    StarterUiSectionsTest.cpp
    StarterUiTriggerCostTest.cpp
    TutorialInvitationGateTest.cpp
    TutorialInvitationLayoutTest.cpp
)

# Teardown: who owns a profile's dialogs and windows, and what becomes of them when it closes
set(TEARDOWN_GROUP_TEST_SOURCES
    DialogTeardownTest.cpp
    HostChildTeardownTest.cpp
    HostWidgetDecouplingTest.cpp
)

# Sound and music, text-to-speech, and the rich presence published to Discord
set(MEDIA_GROUP_TEST_SOURCES
    TDiscordModeTest.cpp
    TMediaLoopTest.cpp
    TtsInterruptingSpeakTest.cpp
)

set(FUNCTIONAL_TEST_UTILS
    TelnetServerStub.cpp
    DiscordIpcServerStub.cpp
)

# mudlet_lsan_hooks has to be linked explicitly, see src/CMakeLists.txt
macro(add_functional_test_binary target)
    add_executable(${target} ${ARGN} ${FUNCTIONAL_TEST_UTILS})
    add_dependencies(${target} ${LIB_MUDLET_TARGET})
    target_link_libraries(${target} PRIVATE Qt6::Test ${LIB_MUDLET_TARGET} mudlet_lsan_hooks)
    set_target_properties(${target} PROPERTIES ENABLE_EXPORTS ON)
endmacro()

# Manual replay tool for verifying mUndoServerWrap against captured game
# output - built but deliberately not registered with ctest:
add_functional_test_binary(UndoServerWrapReplay UndoServerWrapReplay.cpp)

# Report-only perf harness for manual before/after comparisons
# (test/compare-perf-baseline.py runs the built binary directly). Built with the
# functional tests so a binary always exists, but kept OUT of the default ctest
# suite: it feeds a huge fixed corpus many times and makes no timing assertions,
# so running it every CI pipeline only burns minutes. -DREGISTER_PERF_BENCHMARK=ON
# also registers it with ctest.
option(REGISTER_PERF_BENCHMARK "Register the report-only PipelineBenchmark with ctest (it is built either way)" OFF)
add_functional_test_binary(PipelineBenchmark PipelineBenchmark.cpp)
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
set(leakCheckExcludedTests
    dlgTriggerEditorUndoRedoTest
    NewReleaseDialogTeardownTest
)

# Apple's ASan runtime has no LeakSanitizer, so detect_leaks is only meaningful
# where the address or leak sanitizer is actually compiled in
if(NOT APPLE AND "${USE_SANITIZER}" MATCHES "[Aa]ddress|[Ll]eak")
    set(leakCheckAvailable TRUE)
else()
    set(leakCheckAvailable FALSE)
endif()

set(allFunctionalTestNames "")

foreach(test_file ${FUNCTIONAL_TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_functional_test_binary(${test_name} ${test_file})
    add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)
    list(APPEND allFunctionalTestNames ${test_name})
endforeach()

# One executable for the group, but a ctest case each: every test keeps its own
# add_test entry, which names the class for the group binary to run.
macro(add_grouped_test_binary target)
    add_functional_test_binary(${target} GroupedTestMain.cpp ${ARGN})
    foreach(grouped_test_file ${ARGN})
        get_filename_component(grouped_test_name ${grouped_test_file} NAME_WE)
        add_test(NAME ${grouped_test_name} COMMAND $<TARGET_FILE:${target}> ${grouped_test_name})
        list(APPEND allFunctionalTestNames ${grouped_test_name})
    endforeach()
endmacro()

add_grouped_test_binary(functional_window_tests ${WINDOW_GROUP_TEST_SOURCES})
add_grouped_test_binary(functional_profile_tests ${PROFILE_GROUP_TEST_SOURCES})
add_grouped_test_binary(functional_telnet_tests ${TELNET_GROUP_TEST_SOURCES})
add_grouped_test_binary(functional_trigger_tests ${TRIGGER_GROUP_TEST_SOURCES})
add_grouped_test_binary(functional_editor_tests ${EDITOR_GROUP_TEST_SOURCES})
add_grouped_test_binary(functional_map_tests ${MAP_GROUP_TEST_SOURCES})
add_grouped_test_binary(functional_package_tests ${PACKAGE_GROUP_TEST_SOURCES})
add_grouped_test_binary(functional_starterui_tests ${STARTERUI_GROUP_TEST_SOURCES})
add_grouped_test_binary(functional_teardown_tests ${TEARDOWN_GROUP_TEST_SOURCES})
add_grouped_test_binary(functional_media_tests ${MEDIA_GROUP_TEST_SOURCES})

# Every list's cases, so a grouped test's properties are the solo ones verbatim
foreach(test_name ${allFunctionalTestNames})
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
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${leakLog}ASAN_OPTIONS=${leakCheck}"
        LABELS "functional"
        TIMEOUT 60  # seconds
    )
endforeach()

# ResetProfileTest creates a full profile per test method, so it needs a longer
# timeout than the default 60s
set_tests_properties(ResetProfileTest PROPERTIES TIMEOUT 300)

# Undo/redo tests need a longer timeout due to large batch operations
set_tests_properties(dlgTriggerEditorUndoRedoTest PROPERTIES TIMEOUT 300)

# MultilineTriggerReentrancyTest creates a fresh profile per test method, so it needs a longer timeout
set_tests_properties(MultilineTriggerReentrancyTest PROPERTIES TIMEOUT 300)

# GMCPCharLoginTest creates a fresh profile per test method, so it needs a longer timeout
set_tests_properties(GMCPCharLoginTest PROPERTIES TIMEOUT 600)

# TelnetReconnectStateTest creates a fresh profile per test method and reconnects it several
# times per method, so it needs a longer timeout
set_tests_properties(TelnetReconnectStateTest PROPERTIES TIMEOUT 300)

# TelnetConnectFailureTest creates a fresh profile per test method and waits out a connection
# attempt that is refused and then retried, so it needs a longer timeout
set_tests_properties(TelnetConnectFailureTest PROPERTIES TIMEOUT 300)

# TelnetConnectLookupRaceTest creates a fresh profile per test method and waits out a connect to
# an address nothing answers on, so it needs a longer timeout
set_tests_properties(TelnetConnectLookupRaceTest PROPERTIES TIMEOUT 300)

# GlyphOverflowTest creates a fresh profile per test method and sweeps two font
# families across 22 sizes, so it needs a longer timeout
set_tests_properties(GlyphOverflowTest PROPERTIES TIMEOUT 600)

# TelnetCharsetNegotiationTest creates a fresh profile per test case, each of which
# also reconnects, so it needs a longer timeout
set_tests_properties(TelnetCharsetNegotiationTest PROPERTIES TIMEOUT 300)

# ProfileLifecycleTest opens and closes several profiles, and every open is a
# full profile load and every close a full save, so it needs a longer timeout
set_tests_properties(ProfileLifecycleTest PROPERTIES TIMEOUT 300)

# TMediaLoopTest probes the audio backend and creates a fresh profile per test method,
# and each clip has to be waited out in real time, so it needs a longer timeout.
# No test binary runs src/main.cpp, so pin QT_MEDIA_BACKEND to what main.cpp picks for
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

# WrapLineRewrapTest creates a fresh profile per test method, so it needs a longer timeout
set_tests_properties(WrapLineRewrapTest PROPERTIES TIMEOUT 300)

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
