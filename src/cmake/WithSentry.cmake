if(NOT WITH_SENTRY)
    return()
endif()

set(SENTRY_PATH "${CMAKE_SOURCE_DIR}/3rdparty/sentry-native")

# Check if sentry-native submodule is initialized
if(NOT EXISTS "${SENTRY_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "Sentry is enabled (WITH_SENTRY=ON) but the sentry-native submodule is not initialized.\n"
        "Either:\n"
        "  1. Initialize it: git submodule update --init 3rdparty/sentry-native\n"
        "  2. Disable Sentry: cmake -DWITH_SENTRY=OFF .."
    )
endif()

message(STATUS "Building with Sentry enabled")
set(SENTRY_COMMON_ARGS
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
    -DCMAKE_C_COMPILER=clang
    -DCMAKE_CXX_COMPILER=clang++
    -DSENTRY_BACKEND=crashpad
    -DSENTRY_BUILD_SHARED_LIBS=OFF
    -DSENTRY_INTEGRATION_QT=ON
    -G Ninja
)

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
    # On Windows the debug information must be emitted as CodeView (not DWARF) so
    # that lld can write a *separate* PDB. The shipped mudlet.exe then keeps only
    # the small RSDS debug-directory entry (which carries the module's debug-id),
    # while the bulk of the symbols live in mudlet.pdb. This is essential for
    # Sentry: crash minidumps identify each module by the debug-id recorded in the
    # shipped binary, so that id must be present in the exe we ship AND must match
    # the debug companion (the PDB) we upload. Previously we embedded DWARF in the
    # exe, uploaded the unstripped exe, then stripped the shipped exe - which
    # removed the RSDS record (empty debug-id) and changed size_of_image (different
    # code_id), so nothing ever matched and Windows crashes stayed unsymbolicated.
    target_compile_options(${LIB_MUDLET_TARGET} PRIVATE -gcodeview)
    target_compile_options(${EXE_MUDLET_TARGET} PRIVATE -g -gcodeview)
    # Derive the PDB name from the executable's base name so it always matches the
    # shipped exe (and the script's "${MUDLET_EXEC%.exe}.pdb" lookup) even if the
    # target's output name ever changes.
    target_link_options(${EXE_MUDLET_TARGET} PRIVATE
        -g -gcodeview
        "-Wl,--pdb=$<TARGET_FILE_DIR:${EXE_MUDLET_TARGET}>/$<TARGET_FILE_BASE_NAME:${EXE_MUDLET_TARGET}>.pdb"
    )
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
# The sentry Qt integration needs qInstallMessageHandler from Qt6::Core.
# CMake de-duplicates Qt6::Core, placing it before sentry in the link order.
# LINK_GROUP:RESCAN makes the linker scan repeatedly until all references resolve.
if(NOT APPLE)
    target_link_libraries(${LIB_MUDLET_TARGET}
        "$<LINK_GROUP:RESCAN,sentry,crashpad_client,crashpad_handler_lib,crashpad_minidump,crashpad_mpack,crashpad_snapshot,crashpad_tools,crashpad_util,mini_chromium,Qt6::Core>"
    )
else()
    # macOS linker doesn't support RESCAN (--start-group/--end-group)
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
endif()

if(APPLE)
    target_link_libraries(${LIB_MUDLET_TARGET} bsm)
elseif(WIN32)
    target_link_libraries(${LIB_MUDLET_TARGET}
        dbghelp
        version
        synchronization
    )
else()
    target_link_libraries(${LIB_MUDLET_TARGET} crashpad_compat unwind)
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
    # On Windows, lld writes mudlet.pdb at link time (see -Wl,--pdb above), so the
    # separate debug companion already exists and no post-build step is required.
    # The shipped mudlet.exe must NOT be stripped of its RSDS debug-directory
    # entry, otherwise its debug-id is lost and crash minidumps can no longer be
    # matched to the uploaded PDB.
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
endif()
