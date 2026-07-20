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
    TKeySequenceEditTest
    TEncodingHelperTest
    PasswordMigrationTest
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

# Build-file consistency check. Independent of the Mudlet library: it only reads
# src/CMakeLists.txt and the src/ directory, so it does not link LIB_MUDLET_TARGET.
add_executable(CMakeListsConsistencyTest CMakeListsConsistencyTest.cpp)
target_link_libraries(CMakeListsConsistencyTest PRIVATE Qt6::Test)
target_compile_definitions(CMakeListsConsistencyTest PRIVATE MUDLET_SRC_DIR="${CMAKE_SOURCE_DIR}/src")
add_test(NAME CMakeListsConsistencyTest COMMAND $<TARGET_FILE:CMakeListsConsistencyTest>)
set_tests_properties(CMakeListsConsistencyTest PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
)

# MCP Tests - conditionally compiled when HttpServer is available
find_package(Qt6 OPTIONAL_COMPONENTS HttpServer)

if(Qt6HttpServer_FOUND)
    # MCP tests need to link against mudlet_core since TMCPLuaBridge uses mudlet singleton
    # Inherit sanitizer flags from parent project if set
    if(DEFINED CACHE{SANITIZER_COMPILE_OPTIONS_GLOBAL_CACHE})
        set(MCP_TEST_COMPILE_OPTIONS ${SANITIZER_COMPILE_OPTIONS_GLOBAL_CACHE})
        set(MCP_TEST_LINK_OPTIONS ${SANITIZER_COMPILE_OPTIONS_GLOBAL_CACHE})
    endif()

    add_executable(TMCPLuaBridgeTest TMCPLuaBridgeTest.cpp)
    target_compile_definitions(TMCPLuaBridgeTest PRIVATE INCLUDE_MCPSERVER TMCPLuaBridge_Test)
    target_compile_options(TMCPLuaBridgeTest PRIVATE ${MCP_TEST_COMPILE_OPTIONS})
    target_link_options(TMCPLuaBridgeTest PRIVATE ${MCP_TEST_LINK_OPTIONS})
    target_link_libraries(TMCPLuaBridgeTest Qt6::Test mudlet_core)
    add_test(NAME TMCPLuaBridgeTest COMMAND TMCPLuaBridgeTest)

    add_executable(TMCPServerTest TMCPServerTest.cpp)
    target_compile_definitions(TMCPServerTest PRIVATE INCLUDE_MCPSERVER TMCPServer_Test TMCPLuaBridge_Test)
    target_compile_options(TMCPServerTest PRIVATE ${MCP_TEST_COMPILE_OPTIONS})
    target_link_options(TMCPServerTest PRIVATE ${MCP_TEST_LINK_OPTIONS})
    target_link_libraries(TMCPServerTest Qt6::Test Qt6::HttpServer mudlet_core)
    add_test(NAME TMCPServerTest COMMAND TMCPServerTest)
endif()

add_subdirectory(functional_tests)
