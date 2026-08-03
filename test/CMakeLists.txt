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
    target_link_libraries(${test_name} PRIVATE Qt6::Test ${LIB_MUDLET_TARGET})
    add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
    )
endforeach()

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

add_subdirectory(functional_tests)
