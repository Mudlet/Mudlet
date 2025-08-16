CMAKE_MINIMUM_REQUIRED(VERSION 3.25.1)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)


PROJECT(mudlet-tests
        LANGUAGES CXX)

ENABLE_TESTING()

find_package(Qt6 6.4.0 REQUIRED
    COMPONENTS Core
    Multimedia
    MultimediaWidgets
    Network
    OpenGL
    UiTools
    Widgets
    Concurrent
    Test)

link_libraries(
    Qt6::Test
    Qt6::Concurrent
    Qt6::Core
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::UiTools
    Qt6::Widgets)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake ${CMAKE_MODULE_PATH})

# Include the same keychain configuration as main project
include(IncludeOptionalModule)
include_optional_module(ENVIRONMENT_VARIABLE WITH_OWN_QTKEYCHAIN
                        OPTION_VARIABLE USE_OWN_QTKEYCHAIN
                        READABLE_NAME "own QtKeychain library")

if(NOT DEFINED USE_OWN_QTKEYCHAIN)
    # Check the parent project's setting
    get_directory_property(parent_USE_OWN_QTKEYCHAIN PARENT_DIRECTORY USE_OWN_QTKEYCHAIN)
    if(DEFINED parent_USE_OWN_QTKEYCHAIN)
        set(USE_OWN_QTKEYCHAIN ${parent_USE_OWN_QTKEYCHAIN})
    endif()
endif()

# Configure QtKeychain linking
if(USE_OWN_QTKEYCHAIN)
    # Link to the already built qt6keychain target from parent
    set(KEYCHAIN_LIBRARY qt6keychain)
else()
    find_package(Qt6Keychain REQUIRED)
    set(KEYCHAIN_LIBRARY Qt6::Keychain)
endif()


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../src")

add_executable(TEntityResolverTest TEntityResolverTest.cpp ../src/TEntityResolver.cpp)
add_test(NAME TEntityResolverTest COMMAND TEntityResolverTest)

add_executable(TEntityHandlerTest TEntityHandlerTest.cpp ../src/TEntityHandler.cpp ../src/TEntityResolver.cpp)
add_test(NAME TEntityHandlerTest COMMAND TEntityHandlerTest)

add_executable(TLinkStoreTest TLinkStoreTest.cpp ../src/TLinkStore.cpp ../src/TEntityResolver.cpp)
add_test(NAME TLinkStoreTest COMMAND TLinkStoreTest)

target_compile_definitions(TLinkStoreTest PRIVATE LinkStore_Test)

file(GLOB MXP_SOURCE ../src/TMxp*.cpp ../src/MxpTag.cpp ../src/TEntityHandler.cpp ../src/TEntityResolver.cpp ../src/TStringUtils.cpp)
list(FILTER MXP_SOURCE EXCLUDE REGEX ".*/src/TMxpMudlet.cpp")

add_executable(TMxpTagParserTest TMxpTagParserTest.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp ../src/MxpTag.cpp ../src/TStringUtils.cpp)
add_test(NAME TMxpTagParserTest COMMAND TMxpTagParserTest)

add_executable(TMxpSendTagHandlerTest TMxpSendTagHandlerTest.cpp ${MXP_SOURCE} ../src/TMxpSendTagHandler.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp)
add_test(NAME TMxpSendTagHandlerTest COMMAND TMxpSendTagHandlerTest)

add_executable(TMxpEntityTagHandlerTest TMxpEntityTagHandlerTest.cpp ${MXP_SOURCE} ../src/TMxpEntityTagHandler.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp)
add_test(NAME TMxpEntityTagHandlerTest COMMAND TMxpEntityTagHandlerTest)

add_executable(TMxpVersionTagTest TMxpVersionTagTest.cpp ../src/TMxpVersionTagHandler.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp ../src/MxpTag.cpp ../src/TStringUtils.cpp ../src/TMxpTagHandler.cpp)
add_test(NAME TMxpVersionTagTest COMMAND TMxpVersionTagTest)

add_executable(TMxpFormattingTagsTest TMxpFormattingTagsTest.cpp ../src/TMxpFormattingTagsHandler.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp ../src/MxpTag.cpp ../src/TStringUtils.cpp ../src/TMxpTagHandler.cpp)
add_test(NAME TMxpFormattingTagsTest COMMAND TMxpFormattingTagsTest)

add_executable(TMxpCustomElementTagHandlerTest TMxpCustomElementTagHandlerTest.cpp ${MXP_SOURCE} ../src/TMxpSendTagHandler.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp )
add_test(NAME TMxpCustomElementTagHandlerTest COMMAND TMxpCustomElementTagHandlerTest)

add_executable(TLuaInterfaceTest TLuaInterfaceTest.cpp ../src/LuaInterface.cpp ../src/TVar.cpp ../src/VarUnit.cpp)
add_test(NAME TLuaInterfaceTest COMMAND TLuaInterfaceTest)

find_package(Lua51 REQUIRED)
target_link_libraries(
    TLuaInterfaceTest
    LUA51::LUA51)

add_executable(SecureStringUtilsTest SecureStringUtilsTest.cpp ../src/SecureStringUtils.cpp)
target_link_libraries(SecureStringUtilsTest ${KEYCHAIN_LIBRARY})
if(USE_OWN_QTKEYCHAIN)
    target_compile_definitions(SecureStringUtilsTest PRIVATE INCLUDE_OWN_QT6_KEYCHAIN QTKEYCHAIN_NO_EXPORT)
endif()
add_test(NAME SecureStringUtilsTest COMMAND SecureStringUtilsTest)

add_executable(CredentialManagerTest CredentialManagerTest.cpp ../src/CredentialManager.cpp ../src/SecureStringUtils.cpp)
target_link_libraries(CredentialManagerTest ${KEYCHAIN_LIBRARY})
if(USE_OWN_QTKEYCHAIN)
    target_compile_definitions(CredentialManagerTest PRIVATE INCLUDE_OWN_QT6_KEYCHAIN QTKEYCHAIN_NO_EXPORT)
endif()
add_test(NAME CredentialManagerTest COMMAND CredentialManagerTest)
