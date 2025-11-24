if(NOT WITH_SENTRY)
    return()
endif()

message(STATUS "Building with Sentry enabled")

set(SENTRY_PATH "${CMAKE_SOURCE_DIR}/3rdparty/sentry-native")
set(SENTRY_COMMON_ARGS
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
    -DCMAKE_C_COMPILER=clang
    -DCMAKE_CXX_COMPILER=clang++
    -DSENTRY_BACKEND=crashpad
    -DSENTRY_BUILD_SHARED_LIBS=OFF
    -DSENTRY_INTEGRATION_QT=OFF
    -G Ninja
)

if(UNIX AND NOT APPLE)
    list(APPEND SENTRY_CMAKE_ARGS -DSENTRY_TRANSPORT=none)
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

    list(APPEND SENTRY_COMMON_ARGS
        "-DCMAKE_OSX_ARCHITECTURES=${ARCH_LIST}"
        "-DCMAKE_OSX_SYSROOT=${MACOSX_SYSROOT}"
    )
endif()

include(ExternalProject)

# 1) SENTRY WITHOUT TRANSPORT  → used by MUDLET
ExternalProject_Add(
    sentry_without_transport
    SOURCE_DIR ${SENTRY_PATH}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${SENTRY_PATH}/install_without_transport
        ${SENTRY_COMMON_ARGS}
        -DSENTRY_TRANSPORT=none
)


# 2) SENTRY WITH TRANSPORT  → used by CrashReporter
ExternalProject_Add(
    sentry_with_transport
    SOURCE_DIR ${SENTRY_PATH}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${SENTRY_PATH}/install_with_transport
        ${SENTRY_COMMON_ARGS}
)


add_dependencies(${LIB_MUDLET_TARGET} sentry_without_transport)

target_compile_options(${LIB_MUDLET_TARGET} PRIVATE -g)

if(WIN32)
    target_link_options(${EXE_MUDLET_TARGET} PRIVATE -g -gcodeview)
    string(REPLACE "-Wl,-s" "" CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    string(REPLACE "-Wl,-s" "" CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
endif()

target_compile_definitions(${LIB_MUDLET_TARGET} PUBLIC
    WITH_SENTRY
    SENTRY_BUILD_STATIC
)

target_include_directories(${LIB_MUDLET_TARGET} PRIVATE
   "${SENTRY_PATH}/install_without_transport/include/"
)
target_link_directories(${LIB_MUDLET_TARGET} PUBLIC
    "${SENTRY_PATH}/install_without_transport/lib/"
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
)

if(APPLE)
    target_link_libraries(${LIB_MUDLET_TARGET} bsm)
elseif(WIN32)
    target_link_libraries(${LIB_MUDLET_TARGET}
        dbghelp
        version
    )
else()
    target_link_libraries(${LIB_MUDLET_TARGET} crashpad_compat)
endif()

set(SENTRY_BINARIES "${SENTRY_PATH}/install_without_transport/bin")
set(STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/sentry_binaries.stamp")

add_custom_command(OUTPUT ${STAMP_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SENTRY_BINARIES} "$<TARGET_FILE_DIR:${EXE_MUDLET_TARGET}>"
    COMMAND ${CMAKE_COMMAND} -E touch ${STAMP_FILE}
    COMMENT "Copying Sentry binaries next to the mudlet executable"
)

add_custom_target(copy_sentry ALL DEPENDS ${STAMP_FILE})
add_dependencies(copy_sentry sentry_without_transport)
add_dependencies(${EXE_MUDLET_TARGET} copy_sentry)

if(APPLE)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
            COMMAND dsymutil $<TARGET_FILE:${EXE_MUDLET_TARGET}> -o $<TARGET_FILE:${EXE_MUDLET_TARGET}>.dSYM
            COMMAND strip -x $<TARGET_FILE:${EXE_MUDLET_TARGET}>
            COMMENT "Creating .dSYM bundle and stripping executable"
        )
    else()
        add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
            COMMAND dsymutil $<TARGET_FILE:${EXE_MUDLET_TARGET}> -o $<TARGET_FILE:${EXE_MUDLET_TARGET}>.dSYM
            COMMENT "Creating .dSYM bundle without stripping"
        )
    endif()
elseif(UNIX)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
            COMMAND objcopy --only-keep-debug $<TARGET_FILE:${EXE_MUDLET_TARGET}> $<TARGET_FILE:${EXE_MUDLET_TARGET}>.debug
            COMMAND strip --strip-debug $<TARGET_FILE:${EXE_MUDLET_TARGET}>
            COMMAND objcopy --add-gnu-debuglink=$<TARGET_FILE:${EXE_MUDLET_TARGET}>.debug $<TARGET_FILE:${EXE_MUDLET_TARGET}>
            COMMENT "Creating separate debug file and stripping executable"
        )
    else()
        add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
            COMMAND objcopy --only-keep-debug $<TARGET_FILE:${EXE_MUDLET_TARGET}> $<TARGET_FILE:${EXE_MUDLET_TARGET}>.debug
            COMMAND objcopy --add-gnu-debuglink=$<TARGET_FILE:${EXE_MUDLET_TARGET}>.debug $<TARGET_FILE:${EXE_MUDLET_TARGET}>
            COMMENT "Creating separate debug file without stripping"
        )
    endif()
else()
    add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_BINARY_DIR}/mudlet.pdb"
    )
endif()

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
else()
    if(WIN32)
        add_custom_command(TARGET ${EXE_MUDLET_TARGET} POST_BUILD
            COMMAND strip --strip-debug $<TARGET_FILE:${EXE_MUDLET_TARGET}>
        )
    endif()
endif()
