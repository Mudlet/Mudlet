CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)


PROJECT(mudlet-tests
        LANGUAGES CXX)

ENABLE_TESTING()

find_package(Qt5Test REQUIRED)
find_package(Qt5 REQUIRED
        COMPONENTS Core
        Multimedia
        Network
        OpenGL
        UiTools
        Widgets
        Concurrent)

link_libraries(
		edbee-lib
		LUA51::LUA51
        Qt5::Test
        Qt5::Concurrent
        Qt5::Core
        Qt5::Network
        Qt5::Multimedia
        Qt5::UiTools
        Qt5::Widgets)

find_package(Lua51 REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake ${CMAKE_MODULE_PATH})


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../src")

add_executable(TEntityResolverTest TEntityResolverTest.cpp ../src/TEntityResolver.cpp)
add_test(NAME TEntityResolverTest COMMAND TEntityResolverTest)

add_executable(TEntityHandlerTest TEntityHandlerTest.cpp ../src/TEntityHandler.cpp ../src/TEntityResolver.cpp)
add_test(NAME TEntityHandlerTest COMMAND TEntityHandlerTest)

add_executable(TMxpEntityTagTest TMxpEntityTagTest.cpp TMxpStubClient.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp ../src/MxpTag.cpp ../src/TStringUtils.cpp ../src/TMxpEntityTagHandler.cpp ../src/TMxpTagHandler.cpp ../src/TEntityResolver.cpp)
add_test(NAME TMxpEntityTagTest COMMAND TMxpEntityTagTest)

add_executable(TMxpVersionTagTest TMxpVersionTagTest.cpp TMxpStubClient.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp ../src/MxpTag.cpp ../src/TStringUtils.cpp ../src/TMxpVersionTagHandler.cpp ../src/TMxpTagHandler.cpp)
add_test(NAME TMxpVersionTagTest COMMAND TMxpVersionTagTest)

file(GLOB MXP_SOURCE ../src/TMxp*.cpp ../src/MxpTag.cpp ../src/TEntityHandler.cpp ../src/TEntityResolver.cpp ../src/TStringUtils.cpp)
list(FILTER MXP_SOURCE EXCLUDE REGEX ".*/src/TMxpMudlet.cpp")

add_executable(TMxpTagParserTest TMxpTagParserTest.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp ../src/MxpTag.cpp ../src/TStringUtils.cpp)
add_test(NAME TMxpTagParserTest COMMAND TMxpTagParserTest)

add_executable(TMxpSendTagHandlerTest TMxpSendTagHandlerTest.cpp ${MXP_SOURCE} ../src/TMxpSendTagHandler.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp TMxpStubClient.cpp)
add_test(NAME TMxpSendTagHandlerTest COMMAND TMxpSendTagHandlerTest)
#
#add_executable(TMxpCustomElementTagHandlerTest TMxpCustomElementTagHandlerTest.cpp TMxpStubClient.cpp ${MXP_SOURCE} ../src/TMxpSendTagHandler.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp )
#add_test(NAME TMxpCustomElementTagHandlerTest COMMAND TMxpCustomElementTagHandlerTest)

add_executable(TLinkStoreTest TLinkStoreTest.cpp ../src/TLinkStore.cpp ../src/TEntityResolver.cpp)
add_test(NAME TLinkStoreTest COMMAND TLinkStoreTest)

target_compile_definitions(TLinkStoreTest PRIVATE LinkStore_Test)

add_executable(TLuaInterfaceTest TLuaInterfaceTest.cpp ../src/LuaInterface.cpp ../src/TVar.cpp ../src/VarUnit.cpp)
add_test(NAME TLuaInterfaceTest COMMAND TLuaInterfaceTest)

target_link_libraries(
    TLuaInterfaceTest
    LUA51::LUA51)
