if(NOT WITH_SENTRY)
    return()
endif()

message(STATUS "Building with Sentry enabled")

set(SENTRY_PATH "${CMAKE_SOURCE_DIR}/3rdparty/sentry-native")

set(SENTRY_CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${SENTRY_PATH}/install/
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_C_COMPILER=clang
    -DCMAKE_CXX_COMPILER=clang++
    -DSENTRY_BACKEND=crashpad
    -DSENTRY_BUILD_SHARED_LIBS=OFF
    -DSENTRY_INTEGRATION_QT=OFF
    -G Ninja
)

if(UNIX AND NOT APPLE)
    list(APPEND SENTRY_CMAKE_ARGS -DSENTRY_TRANSPORT=curl)
endif()

if(APPLE)
    execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE MACOSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm64")
        set(ARCH_LIST "arm64")
    elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64")
        set(ARCH_LIST "x86_64")
    else()
        set(ARCH_LIST "arm64;x86_64")
    endif()
    
    list(APPEND SENTRY_CMAKE_ARGS
        "-DCMAKE_OSX_ARCHITECTURES=${ARCH_LIST}"
        "-DCMAKE_OSX_SYSROOT=${MACOSX_SYSROOT}"
    )
endif()

include(ExternalProject)

ExternalProject_Add(
    sentry_native
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/sentry-native
    CMAKE_ARGS ${SENTRY_CMAKE_ARGS}
)

add_dependencies(${LIB_MUDLET_TARGET} sentry_native)

file(READ "${CMAKE_SOURCE_DIR}/src/sentry_dsn.txt" SENTRY_DSN_RAW)
string(STRIP "${SENTRY_DSN_RAW}" SENTRY_DSN)

target_compile_options(${LIB_MUDLET_TARGET} PRIVATE -g)

if(WIN32)
    target_link_options(${EXE_MUDLET_TARGET} PRIVATE -g -gcodeview)
    string(REPLACE "-Wl,-s" "" CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    string(REPLACE "-Wl,-s" "" CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
endif()

target_compile_definitions(${LIB_MUDLET_TARGET} PUBLIC
    WITH_SENTRY
    SENTRY_DSN="${SENTRY_DSN}"
    APP_DIR_PATH="${CMAKE_BINARY_DIR}/src"
    SENTRY_BUILD_STATIC
)

target_compile_definitions(${EXE_MUDLET_TARGET} PUBLIC
    WITH_SENTRY
    SENTRY_DSN="${SENTRY_DSN}"
    APP_DIR_PATH="${CMAKE_BINARY_DIR}/src"
    SENTRY_BUILD_STATIC
)

target_include_directories(${LIB_MUDLET_TARGET} PRIVATE
   "${SENTRY_PATH}/install/include/"
)

target_link_directories(${LIB_MUDLET_TARGET} PUBLIC
    "${SENTRY_PATH}/install/lib/"
)

target_link_libraries(${LIB_MUDLET_TARGET}
    sentry
    crashpad_client
    crashpad_handler_lib
    crashpad_minidump
    crashpad_mpack
    crashpad_snapshot
    crashpad_tools
    crashpad_util
    mini_chromium
    curl
)

if(APPLE)
    target_link_libraries(${LIB_MUDLET_TARGET} bsm)
elseif(WIN32)
    target_link_libraries(${LIB_MUDLET_TARGET}
        winhttp
        dbghelp
        version
    )
else()
    target_link_libraries(${LIB_MUDLET_TARGET} crashpad_compat)
endif()

add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SENTRY_PATH}/install/bin"
        "$<TARGET_FILE_DIR:${EXE_MUDLET_TARGET}>"
    COMMENT "Copying Sentry binaries next to the mudlet executable"
)

# if(APPLE)
#     add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
#         COMMAND dsymutil $<TARGET_FILE:${EXE_MUDLET_TARGET}> -o $<TARGET_FILE:${EXE_MUDLET_TARGET}>.dSYM
#         COMMAND strip -x $<TARGET_FILE:${EXE_MUDLET_TARGET}>
#         COMMENT "Creating .dSYM bundle and stripping executable"
#     )
# elseif(UNIX)
#     add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
#         COMMAND objcopy --only-keep-debug $<TARGET_FILE:${EXE_MUDLET_TARGET}> $<TARGET_FILE:${EXE_MUDLET_TARGET}>.debug
#         COMMAND strip --strip-debug $<TARGET_FILE:${EXE_MUDLET_TARGET}>
#         COMMAND objcopy --add-gnu-debuglink=$<TARGET_FILE:${EXE_MUDLET_TARGET}>.debug $<TARGET_FILE:${EXE_MUDLET_TARGET}>
#         COMMENT "Creating separate debug file and stripping executable"
#     )
# endif()

if(SENTRY_SEND_DEBUG)
    if(NOT DEFINED ENV{SENTRY_AUTH_TOKEN} OR "$ENV{SENTRY_AUTH_TOKEN}" STREQUAL "")
        message(FATAL_ERROR
            "[Option SENTRY_SEND_DEBUG enabled] The environment variable SENTRY_AUTH_TOKEN is missing.\n"
            "SENTRY_AUTH_TOKEN is required to authenticate with Sentry before uploading debug files.\n"
            "Fix: try exporting SENTRY_AUTH_TOKEN=\"...\""
        )
    else()
        add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
        COMMAND bash "${CMAKE_SOURCE_DIR}/CI/send_debug_files_to_sentry.sh" "$<TARGET_FILE:${EXE_MUDLET_TARGET}>"
        VERBATIM
    )
    endif()
endif()

if(APPLE)
    add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
        COMMAND dsymutil $<TARGET_FILE:${EXE_MUDLET_TARGET}> -o $<TARGET_FILE:${EXE_MUDLET_TARGET}>.dSYM
        COMMAND strip -x $<TARGET_FILE:${EXE_MUDLET_TARGET}>
        COMMENT "Creating .dSYM bundle and stripping executable"
    )
elseif(UNIX)
    add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
        COMMAND objcopy --only-keep-debug $<TARGET_FILE:${EXE_MUDLET_TARGET}> $<TARGET_FILE:${EXE_MUDLET_TARGET}>.debug
        COMMAND strip --strip-debug $<TARGET_FILE:${EXE_MUDLET_TARGET}>
        COMMAND objcopy --add-gnu-debuglink=$<TARGET_FILE:${EXE_MUDLET_TARGET}>.debug $<TARGET_FILE:${EXE_MUDLET_TARGET}>
        COMMENT "Creating separate debug file and stripping executable"
    )
else()
    add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
        COMMAND strip --strip-debug $<TARGET_FILE:${EXE_MUDLET_TARGET}>
    )
endif()
