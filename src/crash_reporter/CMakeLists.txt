cmake_minimum_required(VERSION 3.25.1)
project(MudletCrashReporter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "=== Sentry Configuration ===")
message(STATUS "WITH_SENTRY: ${WITH_SENTRY}")
if(NOT SENTRY_DSN OR SENTRY_DSN STREQUAL "")
    message(STATUS "SENTRY_DSN: NOT SET")
else()
    message(STATUS "SENTRY_DSN: SET")
endif()

if(NOT DEFINED ENV{SENTRY_AUTH_TOKEN} OR "$ENV{SENTRY_AUTH_TOKEN}" STREQUAL "")
    message(STATUS "SENTRY_AUTH_TOKEN: NOT SET")
else()
    message(STATUS "SENTRY_AUTH_TOKEN: SET")
endif()
message(STATUS "============================")

find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

add_executable(MudletCrashReporter main.cpp)


if(TARGET ${EXE_MUDLET_TARGET})
    set_target_properties(MudletCrashReporter PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${EXE_MUDLET_TARGET}>
    )
endif()

add_dependencies(MudletCrashReporter sentry_with_transport)

target_include_directories(MudletCrashReporter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/sentry-native/install_with_transport/include
     ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/sentry-native/src
)

target_compile_definitions(MudletCrashReporter PRIVATE
    SENTRY_BUILD_STATIC
    SENTRY_DSN="${SENTRY_DSN}"
)

target_link_directories(MudletCrashReporter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/sentry-native/install_with_transport/lib
)

set(MUDLET_CRASH_REPORTER_LIBS
    Qt6::Core Qt6::Widgets
    sentry
    crashpad_client crashpad_handler_lib crashpad_minidump
    crashpad_mpack crashpad_snapshot crashpad_tools crashpad_util
    mini_chromium
)

if(WIN32)
    list(APPEND MUDLET_CRASH_REPORTER_LIBS
        crashpad_compat
        ws2_32
        shlwapi
        winhttp
        version
        dbghelp
    )
elseif(APPLE)
    list(APPEND MUDLET_CRASH_REPORTER_LIBS
        bsm
        pthread
        dl
        z
        curl
    )
elseif(UNIX)
    list(APPEND MUDLET_CRASH_REPORTER_LIBS
        crashpad_compat
        pthread
        dl
        z
        curl
    )
endif()

target_link_libraries(MudletCrashReporter PRIVATE ${MUDLET_CRASH_REPORTER_LIBS})
