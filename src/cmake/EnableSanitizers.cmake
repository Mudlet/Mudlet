include(${CMAKE_SOURCE_DIR}/3rdparty/cmake-scripts/sanitizers.cmake)

if(DEFINED ENV{MUDLET_SANITIZERS} AND NOT "$ENV{MUDLET_SANITIZERS}" STREQUAL "")

    # The available sanitizers are OS dependent - we ought to account for that
    set(SANITIZERS_SELECTED
        "$ENV{MUDLET_SANITIZERS}"
        CACHE STRING
        "Compile with sanitizers. Available sanitizers are (case-sensitive): \
        Address, Memory, MemoryWithOrigins, Undefined, Thread, or Leak"
    )

    # Only set the options on sanitizers we are using - otherwise an unneeded
    # availability/compatibility check is done for each one:
    LIST(FIND SANITIZERS_SELECTED "Address" HAS_ADDRESS)
    LIST(FIND SANITIZERS_SELECTED "Thread" HAS_THREAD)
    LIST(FIND SANITIZERS_SELECTED "Memory" HAS_MEMORY)
    LIST(FIND SANITIZERS_SELECTED "MemoryWithOrigins" HAS_MEMORYWITHORIGINS)
    if ("${HAS_ADDRESS}" GREATER_EQUAL 0)
        set_sanitizer_options(address DEFAULT -fno-omit-frame-pointer)
    endif()
    if ("${HAS_THREAD}" GREATER_EQUAL 0)
        set_sanitizer_options(thread DEFAULT -fno-omit-frame-pointer)
    endif()
    if ("${HAS_MEMORY}" GREATER_EQUAL 0)
        set_sanitizer_options(memory DEFAULT -fno-omit-frame-pointer)
    endif()
    if ("${HAS_MEMORYWITHORIGINS}" GREATER_EQUAL 0)
        set_sanitizer_options(memorywithorigins DEFAULT SANITIZER memory
            -fno-omit-frame-pointer
            -fsanitize-memory-track-origins)
    endif()

    LIST(JOIN SANITIZERS_SELECTED " " SANITIZERS_AS_STRING)
    add_sanitizer_support("${SANITIZERS_SELECTED}")
    # Don't report which sanitizers are being used here as this is used more
    # than once in a full build and there is no need to repeat the message,
    # it is now done in the second-level src/CMakeLists.txt file.
endif()
