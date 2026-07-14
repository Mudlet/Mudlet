cmake_minimum_required(VERSION 3.25.1)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT WIN32)
    include(${CMAKE_SOURCE_DIR}/src/cmake/EnableSanitizers.cmake)
endif()

find_package(Qt6 6.8.2 REQUIRED COMPONENTS Test)

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
    TEncodingHelperTest
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

add_executable(PasswordMigrationTest PasswordMigrationTest.cpp)
add_dependencies(PasswordMigrationTest ${LIB_MUDLET_TARGET})
target_link_libraries(PasswordMigrationTest PRIVATE Qt6::Test ${LIB_MUDLET_TARGET})
add_test(NAME PasswordMigrationTest COMMAND $<TARGET_FILE:PasswordMigrationTest>)

# Build-file consistency check. Independent of the Mudlet library: it only reads
# src/CMakeLists.txt and the src/ directory, so it does not link LIB_MUDLET_TARGET.
add_executable(CMakeListsConsistencyTest CMakeListsConsistencyTest.cpp)
target_link_libraries(CMakeListsConsistencyTest PRIVATE Qt6::Test)
target_compile_definitions(CMakeListsConsistencyTest PRIVATE MUDLET_SRC_DIR="${CMAKE_SOURCE_DIR}/src")
add_test(NAME CMakeListsConsistencyTest COMMAND $<TARGET_FILE:CMakeListsConsistencyTest>)
set_tests_properties(CMakeListsConsistencyTest PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
)

add_subdirectory(functional_tests)
