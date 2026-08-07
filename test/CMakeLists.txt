cmake_minimum_required(VERSION 3.25.1)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT WIN32)
    include(${CMAKE_SOURCE_DIR}/src/cmake/EnableSanitizers.cmake)
endif()

find_package(Qt6 6.8.2 REQUIRED COMPONENTS Test Network Widgets)

set(UNIT_TESTS
    TEntityResolverTest
    TEntityHandlerTest
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
# it well under ctest's default 25 minutes.
set_tests_properties(EventLoopPumpTest PROPERTIES TIMEOUT 60)

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

# Pairing of a release's assets with its SHA256SUMS.txt. Built from the updater
# sources rather than linked against the Mudlet library, because the library only
# contains them when configured with USE_UPDATER.
add_executable(UpdaterChecksumTest
    UpdaterChecksumTest.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/Feed.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/Release.cpp
    ${CMAKE_SOURCE_DIR}/src/updater/SemVer.cpp
)
target_link_libraries(UpdaterChecksumTest PRIVATE Qt6::Test Qt6::Network Qt6::Widgets)
add_test(NAME UpdaterChecksumTest COMMAND $<TARGET_FILE:UpdaterChecksumTest>)
set_tests_properties(UpdaterChecksumTest PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
)

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
