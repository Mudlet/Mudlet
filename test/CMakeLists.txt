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
    SecureStringUtilsTest
    CredentialManagerTest
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

add_executable(PasswordMigrationTest PasswordMigrationTest.cpp ../src/CredentialManager.cpp ../src/SecureStringUtils.cpp)
target_link_libraries(PasswordMigrationTest PRIVATE Qt6::Test ${KEYCHAIN_LIBRARY})
if(USE_OWN_QTKEYCHAIN)
    target_compile_definitions(PasswordMigrationTest PRIVATE INCLUDE_OWN_QT6_KEYCHAIN QTKEYCHAIN_NO_EXPORT)
endif()
add_test(NAME PasswordMigrationTest COMMAND $<TARGET_FILE:PasswordMigrationTest>)

add_subdirectory(functional_tests)
