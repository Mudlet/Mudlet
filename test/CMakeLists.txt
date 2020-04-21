find_package(Qt5Test REQUIRED)
find_package(Qt5 REQUIRED
        COMPONENTS Core
        Multimedia
        Network
        OpenGL
        UiTools
        Widgets
        Concurrent)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


enable_testing(true)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../src")
link_libraries(Qt5::Test
        Qt5::Concurrent
        Qt5::Core
        Qt5::Network
        Qt5::Multimedia
        Qt5::UiTools
        Qt5::Widgets)

add_executable(TEntityResolverTest TEntityResolverTest.cpp ../src/TEntityResolver.cpp)
add_test(NAME TEntityResolverTest COMMAND TEntityResolverTest)

add_executable(TEntityHandlerTest TEntityHandlerTest.cpp ../src/TEntityHandler.cpp ../src/TEntityResolver.cpp)
add_test(NAME TEntityHandlerTest COMMAND TEntityHandlerTest)

add_executable(TLinkStoreTest TLinkStoreTest.cpp ../src/TLinkStore.cpp ../src/TEntityResolver.cpp)
add_test(NAME TLinkStoreTest COMMAND TLinkStoreTest)


file(GLOB MXP_SOURCE ../src/TMxp*.cpp ../src/MxpTag.cpp ../src/TEntityHandler.cpp ../src/TEntityResolver.cpp ../src/TStrUtils.cpp)

add_executable(TMxpTagParserTest TMxpTagParserTest.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp ../src/MxpTag.cpp ../src/TStrUtils.cpp)
add_test(NAME TMxpTagParserTest COMMAND TMxpTagParserTest)

add_executable(TMxpSendTagHandlerTest TMxpSendTagHandlerTest.cpp ${MXP_SOURCE} ../src/TMxpSendTagHandler.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp)
add_test(NAME TMxpSendTagHandlerTest COMMAND TMxpSendTagHandlerTest)