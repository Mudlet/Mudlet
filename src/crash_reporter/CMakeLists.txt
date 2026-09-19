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

find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)

add_executable(MudletCrashReporter main.cpp)


if(TARGET ${EXE_MUDLET_TARGET})
    set_target_properties(MudletCrashReporter PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${EXE_MUDLET_TARGET}>
    )
endif()

# Packaging copies this executable out of the build tree instead of installing
# it, so BUILD_RPATH rather than INSTALL_RPATH is what ships. $ORIGIN/lib is the
# AppImage layout and @executable_path/../Frameworks the .app bundle; with
# neither, the shipped binary knows only the build machine's Qt path and the
# loader kills it before it can report anything. Both are appended to the
# automatically determined entries, so build-tree runs are unaffected.
if(APPLE)
    set_target_properties(MudletCrashReporter PROPERTIES BUILD_RPATH "@executable_path/../Frameworks")
elseif(UNIX)
    set_target_properties(MudletCrashReporter PROPERTIES BUILD_RPATH "$ORIGIN/lib")
endif()

target_compile_definitions(MudletCrashReporter PRIVATE
    SENTRY_DSN="${SENTRY_DSN}"
)

target_link_libraries(MudletCrashReporter PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
