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
        Qt5::Test
        Qt5::Concurrent
        Qt5::Core
        Qt5::Network
        Qt5::Multimedia
        Qt5::UiTools
        Qt5::Widgets)

if (CMAKE_COMPILER_IS_GNUCXX)
    SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
    SET(GCC_COVERAGE_LINK_FLAGS "-fprofile-arcs -fprofile-generate -lgcov --coverage")
    add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
    link_libraries(gcov)
endif ()


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../src")

add_executable(TEntityResolverTest TEntityResolverTest.cpp ../src/TEntityResolver.cpp)
add_test(NAME TEntityResolverTest COMMAND TEntityResolverTest)

add_executable(TEntityHandlerTest TEntityHandlerTest.cpp ../src/TEntityHandler.cpp ../src/TEntityResolver.cpp)
add_test(NAME TEntityHandlerTest COMMAND TEntityHandlerTest)

add_executable(TLinkStoreTest TLinkStoreTest.cpp ../src/TLinkStore.cpp ../src/TEntityResolver.cpp)
add_test(NAME TLinkStoreTest COMMAND TLinkStoreTest)


file(GLOB MXP_SOURCE ../src/TMxp*.cpp ../src/MxpTag.cpp ../src/TEntityHandler.cpp ../src/TEntityResolver.cpp ../src/TStringUtils.cpp)
list(FILTER MXP_SOURCE EXCLUDE REGEX ".*/src/TMxpMudlet.cpp")

add_executable(TMxpTagParserTest TMxpTagParserTest.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp ../src/MxpTag.cpp ../src/TStringUtils.cpp)
add_test(NAME TMxpTagParserTest COMMAND TMxpTagParserTest)

add_executable(TMxpSendTagHandlerTest TMxpSendTagHandlerTest.cpp ${MXP_SOURCE} ../src/TMxpSendTagHandler.cpp ../src/TMxpTagParser.cpp ../src/TMxpNodeBuilder.cpp)
add_test(NAME TMxpSendTagHandlerTest COMMAND TMxpSendTagHandlerTest)

add_executable(TBufferStyleTest TBufferStyleTest.cpp ../src/TBufferStyle.cpp ../src/TColorSettings.cpp ../src/TColorScheme.cpp ../src/TChar.cpp)
add_test(NAME TBufferStyleTest COMMAND TBufferStyleTest)
