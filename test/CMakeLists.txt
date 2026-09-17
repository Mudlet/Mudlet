cmake_minimum_required(VERSION 3.25.1)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT WIN32)
    include(${CMAKE_SOURCE_DIR}/src/cmake/EnableSanitizers.cmake)
endif()

find_package(Qt6 6.8.2 REQUIRED COMPONENTS Test Network Gui Widgets)

# On Windows Qt diverts QTest's stdout to OutputDebugString unless it believes
# stderr has a console attached, which an MSYS2 shell or a CI runner does not give
# it, so a failing test reports an exit code and nothing else (#9747). Deferred so
# that tests registered further down the file are covered too; TESTS does not
# descend into subdirectories, hence the second call in functional_tests.
function(restore_windows_test_output)
    get_property(registeredTests DIRECTORY PROPERTY TESTS)
    foreach(testName ${registeredTests})
        set_property(TEST ${testName} APPEND PROPERTY ENVIRONMENT "QT_ASSUME_STDERR_HAS_CONSOLE=1")
    endforeach()
endfunction()
cmake_language(DEFER CALL restore_windows_test_output)

# Where a test's config root goes. CredentialManager and SecureStringUtils write
# per-profile encryption keys and passwords under QStandardPaths::AppConfigLocation
# without asking Mudlet where its config root is, so for a QTEST_MAIN program that
# is a directory named after the test class in the home directory of whoever runs
# it - a path no source scan can recognise and nothing in the test tree names
# (#10777). Pointing it at a directory of the build tree instead costs the tests
# that already redirect nothing, since qputenv() in initTestCase() overrides it.
#
# On Linux the lever is XDG_CONFIG_HOME. macOS never reads it: AppConfigLocation there is
# NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask) plus
# "/Preferences" and the application name, so the lever is CFFIXED_USER_HOME, the
# home directory override CoreFoundation resolves that call against. It has to
# arrive from here rather than from the test, because a test process cannot move it
# once Foundation has answered - which is why the per-test qputenv() in
# initTestCase() covers Linux only, and why HomeUntouchedTest checks this redirect
# rather than trusting it (test/ci/home-untouched-test.sh). Windows has no such
# override - SHGetKnownFolderPath reads no environment - so it stays uncovered.
#
# For XDG this is a backstop and not the mechanism: a build-tree
# directory outlives the run, where a test's own QTemporaryDir does not - so a test
# still has to redirect for itself to get a config root that is empty at the start
# of every run. setupConfig() is unaffected either way: it takes $XDG_CONFIG_HOME/mudlet
# only when that directory holds profiles/ (see MudletApp::xdgConfigDir()), which this does
# not create - deliberately, since seeding the opt-in here would give a test that
# forgets a config root that accumulates profiles between runs, which is the
# collision #9999 was about.
#
# Deferred, and per directory, for the same reason as restore_windows_test_output().
function(sandbox_test_config_dirs)
    get_property(registeredTests DIRECTORY PROPERTY TESTS)
    foreach(testName ${registeredTests})
        set(testConfigDir "${CMAKE_CURRENT_BINARY_DIR}/xdg-config/${testName}")
        set_property(TEST ${testName} APPEND PROPERTY ENVIRONMENT "XDG_CONFIG_HOME=${testConfigDir}")
        if(APPLE)
            # CoreFoundation is handed a home directory rather than a config root,
            # and one that has to exist - Qt creates a config root on the way to
            # writing into it, and no one creates a home
            file(MAKE_DIRECTORY "${testConfigDir}")
            set_property(TEST ${testName} APPEND PROPERTY ENVIRONMENT "CFFIXED_USER_HOME=${testConfigDir}")
        endif()
    endforeach()
endfunction()
cmake_language(DEFER CALL sandbox_test_config_dirs)

set(UNIT_TESTS
    TEntityResolverTest
    TEntityHandlerTest
    LuaLiteralTest
    UntrustedTextTest
    TLinkStoreTest
    TMxpTagParserTest
    TMxpSendTagHandlerTest
    TMxpEntityTagHandlerTest
    TMxpFrameDestTagHandlerTest
    TMxpVersionTagTest
    TMxpFormattingTagsTest
    TMxpCustomElementTagHandlerTest
    TMxpModeSecurityTest
    TMxpEdgeCasesTest
    TMxpElementDefinitionHandlerTest
    TLuaInterfaceTest
    EventLoopPumpTest
    TVariableEditorTest
    SecureStringUtilsTest
    CredentialManagerTest
    CredentialManagerKeychainTest
    OAuthClientFlowTest
    DiscordTest
    TTextEditBlinkTest
    TAreaSpanIndexTest
    TAreaZLevelIndexTest
    TAreaGridIndexTest
    TKeySequenceEditTest
    TEncodingHelperTest
    PasswordMigrationTest
    TMediaPathTraversalTest
    ProfileNameValidationTest
)

foreach(test_name ${UNIT_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    add_dependencies(${test_name} ${LIB_MUDLET_TARGET})
    # mudlet_lsan_hooks has to be linked explicitly, see src/CMakeLists.txt
    target_link_libraries(${test_name} PRIVATE Qt6::Test ${LIB_MUDLET_TARGET} mudlet_lsan_hooks)
    add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
    )
endforeach()

# A regression in what EventLoopPumpTest covers hangs rather than fails, so cap
# it well under ctest's default 25 minutes. The same goes for the coordinate
# limit cases in TAreaGridIndexTest, which never return once a viewport scan
# stops terminating.
set_tests_properties(EventLoopPumpTest TAreaGridIndexTest PROPERTIES TIMEOUT 60)

# TKeySequenceEditTest's focus traversal cases need an active window, which an X
# server with no window manager never gives them, so a plain `xvfb-run ctest`
# reported two failures that meant nothing and cost the activation timeout twice
# (#9575). The offscreen platform synthesises activation, and is already how the
# Linux CI job runs the whole suite. Only on X11: macOS and Windows have a real
# window manager, and running there natively is the only coverage of platform
# focus traversal there is. APPEND so the sanitizer setting above survives.
if(UNIX AND NOT APPLE)
    set_property(TEST TKeySequenceEditTest APPEND PROPERTY ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

# Every ctest run has a display that can activate a window, one way or the other,
# so a skipped traversal case there is a regression and not an environment. Only
# somebody running the binary by hand on a bare X server is allowed the skip.
set_property(TEST TKeySequenceEditTest APPEND PROPERTY ENVIRONMENT "MUDLET_REQUIRE_WINDOW_ACTIVATION=1")

# Which releases' notes the update offer shows. Built from the updater sources
# rather than linked against the Mudlet library, because the library only
# contains them when configured with USE_UPDATER.
add_executable(ReleaseChangelogSpanTest
    ReleaseChangelogSpanTest.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/Feed.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/Release.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/SemVer.cpp
)
target_link_libraries(ReleaseChangelogSpanTest PRIVATE Qt6::Test Qt6::Network Qt6::Gui)
add_test(NAME ReleaseChangelogSpanTest COMMAND $<TARGET_FILE:ReleaseChangelogSpanTest>)
set_tests_properties(ReleaseChangelogSpanTest PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
)

# The IRC client's message formatting. Built from the one source it covers rather
# than linked against the Mudlet library: the formatter is static and reaches for
# nothing but communi, so this needs neither the library nor the ~250MB a link
# against it costs every build tree.
add_executable(IrcMessageFormatterTest
    IrcMessageFormatterTest.cpp
    ${CMAKE_SOURCE_DIR}/src/ircmessageformatter.cpp
)
target_include_directories(IrcMessageFormatterTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(IrcMessageFormatterTest PRIVATE Qt6::Test communi)
add_test(NAME IrcMessageFormatterTest COMMAND $<TARGET_FILE:IrcMessageFormatterTest>)
set_tests_properties(IrcMessageFormatterTest PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
)

# What communi makes of the multi-line numerics an IRC server sends, including
# the ones it sends out of order. Needs nothing but communi for the same reason
# the formatter test above does not.
add_executable(IrcMessageComposerTest IrcMessageComposerTest.cpp)
target_link_libraries(IrcMessageComposerTest PRIVATE Qt6::Test Qt6::Network communi)
add_test(NAME IrcMessageComposerTest COMMAND $<TARGET_FILE:IrcMessageComposerTest>)
set_tests_properties(IrcMessageComposerTest PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
    TIMEOUT 60
)

# DiscordTest checks the Lua API permission gating contract by scanning the source
target_compile_definitions(DiscordTest PRIVATE MUDLET_SRC_DIR="${CMAKE_SOURCE_DIR}/src")

# Build-file consistency check. Independent of the Mudlet library: it only reads
# src/CMakeLists.txt and the src/ directory, so it does not link LIB_MUDLET_TARGET.
add_executable(CMakeListsConsistencyTest CMakeListsConsistencyTest.cpp)
target_link_libraries(CMakeListsConsistencyTest PRIVATE Qt6::Test)
target_compile_definitions(CMakeListsConsistencyTest PRIVATE MUDLET_SRC_DIR="${CMAKE_SOURCE_DIR}/src")
add_test(NAME CMakeListsConsistencyTest COMMAND $<TARGET_FILE:CMakeListsConsistencyTest>)
set_tests_properties(CMakeListsConsistencyTest PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
)

# The version comparison every update offer is decided by. Built from the one
# updater source it covers rather than linked against the Mudlet library, because
# the library only contains it when configured with USE_UPDATER - which CI's
# -testing builds are not. utils.h reaches QEnterEvent, hence Qt6::Gui.
add_executable(SemVerTest
    SemVerTest.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/SemVer.cpp
)
target_link_libraries(SemVerTest PRIVATE Qt6::Test Qt6::Gui)
add_test(NAME SemVerTest COMMAND $<TARGET_FILE:SemVerTest>)
set_tests_properties(SemVerTest PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
)

# Pairing of a release's assets with its SHA256SUMS.txt. Built from the updater
# sources rather than linked against the Mudlet library, because the library only
# contains them when configured with USE_UPDATER.
add_executable(ReleaseChecksumPairingTest
    ReleaseChecksumPairingTest.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/Feed.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/Release.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/SemVer.cpp
)
target_link_libraries(ReleaseChecksumPairingTest PRIVATE Qt6::Test Qt6::Network Qt6::Gui)
add_test(NAME ReleaseChecksumPairingTest COMMAND $<TARGET_FILE:ReleaseChecksumPairingTest>)
set_tests_properties(ReleaseChecksumPairingTest PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
)

# Which releases the updater offers as an update. Built from the updater sources
# rather than linked against the Mudlet library, because the library only
# contains them when configured with USE_UPDATER.
add_executable(ReleasePlatformAssetTest
    ReleasePlatformAssetTest.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/Feed.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/Release.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/SemVer.cpp
)
target_link_libraries(ReleasePlatformAssetTest PRIVATE Qt6::Test Qt6::Network Qt6::Gui)
add_test(NAME ReleasePlatformAssetTest COMMAND $<TARGET_FILE:ReleasePlatformAssetTest>)
set_tests_properties(ReleasePlatformAssetTest PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
)

# The $XDG_CONFIG_HOME opt-in recipe the tests isolate themselves with. Reads the
# test sources at runtime, so like CMakeListsConsistencyTest it links nothing.
add_executable(XdgRecipeConsistencyTest XdgRecipeConsistencyTest.cpp)
target_link_libraries(XdgRecipeConsistencyTest PRIVATE Qt6::Test)
target_compile_definitions(XdgRecipeConsistencyTest PRIVATE MUDLET_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME XdgRecipeConsistencyTest COMMAND $<TARGET_FILE:XdgRecipeConsistencyTest>)
set_tests_properties(XdgRecipeConsistencyTest PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
)

# What XdgRecipeConsistencyTest's source scan cannot see: whether the bytes a test
# writes actually stay out of the home directory. Sweeps the real one for a config
# root named after any credential-store test - which covers the sources compiled
# into the grouped functional binaries too, since that needs no binary of its own -
# and runs the standalone ones under a home directory of their own. Not on Windows,
# where there is no shell for it and nothing redirects AppConfigLocation anyway.
if(NOT WIN32)
    add_test(NAME HomeUntouchedTest
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/ci/home-untouched-test.sh ${CMAKE_CURRENT_BINARY_DIR}
    )
    # The home sweep reports on what has already run, so it has to run last of the
    # tests that could leave something there - ctest -j otherwise starts it among
    # the first few and it sweeps a home directory nothing has written to yet.
    # Derived from the sources, and by the same rule as the script's own candidates,
    # so a test renamed or added needs no edit here either. A name that is not a
    # registered test is ignored by ctest, which is what lets this cover the sources
    # compiled into the grouped functional binaries without knowing which those are.
    file(GLOB credentialStoreSources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/functional_tests/*.cpp)
    set(credentialStoreTests)
    foreach(candidateSource ${credentialStoreSources})
        file(STRINGS ${candidateSource} credentialStoreUse REGEX "CredentialManager|SecureStringUtils" LIMIT_COUNT 1)
        if(credentialStoreUse)
            get_filename_component(candidateName ${candidateSource} NAME_WE)
            list(APPEND credentialStoreTests ${candidateName})
        endif()
    endforeach()
    set_tests_properties(HomeUntouchedTest PROPERTIES DEPENDS "${credentialStoreTests}")
endif()

# Checks the release-publishing scripts that keep SHA256SUMS.txt covering every
# release binary - a binary without an entry is one the updater refuses to install
if(NOT WIN32)
    add_test(NAME ReleaseChecksumsTest
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/ci/release-checksums-test.sh
    )
endif()

# The updater reads its version from the release tag, so a tag like "Mudlet-5.0"
# stops every existing user being offered the release, silently
if(NOT WIN32)
    add_test(NAME ReleaseTagVersionTest
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/ci/release-tag-version-test.sh
    )
endif()

# Checks the milestone lookup that add-milestone runs - the one that matched a
# title that no longer existed and assigned nothing for months, without ever
# failing. gh is stubbed, so there is no network and no token
if(NOT WIN32)
    add_test(NAME MilestoneResolutionTest
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/ci/milestone-resolution-test.sh
    )
endif()

add_subdirectory(functional_tests)

# CI runners are elevated, so nothing there catches a test executable Windows
# refuses to launch for looking like an installer (#9748) - hence this gate,
# whose message spells the heuristic out.
function(mudlet_reject_uac_installer_name name)
    string(TOLOWER "${name}" lowercaseName)
    if(lowercaseName MATCHES "install|setup|update|patch")
        message(FATAL_ERROR
            "'${name}' cannot be used as a test name: Windows takes an unsigned executable whose name "
            "contains '${CMAKE_MATCH_0}' for an installer and refuses to launch it without elevation "
            "(#9748), so ctest reports BAD_COMMAND for it in an ordinary developer shell. Rename the "
            "source file and its target to describe what the test asserts, watching for install, setup, "
            "update and patch as substrings - Dispatch carries one.")
    endif()
endfunction()

function(mudlet_reject_uac_installer_target_names directory)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target ${targets})
        get_target_property(targetType ${target} TYPE)
        if(NOT targetType STREQUAL "EXECUTABLE")
            continue()
        endif()
        set_property(GLOBAL APPEND PROPERTY mudletUacCheckedExecutables "${target}")
        get_target_property(executableName ${target} OUTPUT_NAME)
        if(NOT executableName)
            set(executableName ${target})
        endif()
        mudlet_reject_uac_installer_name("${executableName}")
    endforeach()

    get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(subdirectory ${subdirectories})
        mudlet_reject_uac_installer_target_names("${subdirectory}")
    endforeach()
endfunction()

# Both passes are needed. Targets carry the name that actually reaches disk,
# OUTPUT_NAME included, but only for the tests this configuration builds:
# NewReleaseDialogTeardownTest, for one, is registered only under USE_UPDATER.
# Source names are what the registration loops derive executable names from,
# and are present whatever the configuration.
function(mudlet_check_test_executable_names directory)
    mudlet_reject_uac_installer_target_names("${directory}")

    get_property(checkedExecutables GLOBAL PROPERTY mudletUacCheckedExecutables)
    if(NOT checkedExecutables)
        message(FATAL_ERROR "The test executable name check found no executables under ${directory}, so it is checking nothing.")
    endif()

    file(GLOB_RECURSE testSources "${directory}/*.cpp")
    if(NOT testSources)
        message(FATAL_ERROR "The test executable name check found no sources under ${directory}, so it is checking nothing.")
    endif()
    foreach(testSource ${testSources})
        get_filename_component(sourceName "${testSource}" NAME_WE)
        mudlet_reject_uac_installer_name("${sourceName}")
    endforeach()
endfunction()

# Deferred rather than called outright, so that targets registered below this
# line, and in subdirectories added below it, are checked as well
cmake_language(DEFER CALL mudlet_check_test_executable_names "${CMAKE_CURRENT_SOURCE_DIR}")
