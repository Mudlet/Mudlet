include(${CMAKE_SOURCE_DIR}/3rdparty/cmake-scripts/sanitizers.cmake)

if(DEFINED ENV{MUDLET_SANITIZERS} AND NOT "$ENV{MUDLET_SANITIZERS}" STREQUAL "")

    # The available sanitizers are OS dependent - we ought to account for that:
    set(SANITIZERS_REQUESTED "$ENV{MUDLET_SANITIZERS}")
    list(REMOVE_DUPLICATES SANITIZERS_REQUESTED)
    set(SANITIZERS_SELECTED "")

    # Only set the options on sanitizers we are using - otherwise an unneeded
    # availability/compatibility check is done for each one:
    list(FIND SANITIZERS_REQUESTED "address" HAS_ADDRESS)
    list(FIND SANITIZERS_REQUESTED "leak" HAS_LEAK)
    list(FIND SANITIZERS_REQUESTED "memory" HAS_MEMORY)
    list(FIND SANITIZERS_REQUESTED "memoryWithOrigins" HAS_MEMORYWITHORIGINS)
    list(FIND SANITIZERS_REQUESTED "thread" HAS_THREAD)
    list(FIND SANITIZERS_REQUESTED "type" HAS_TYPE)
    list(FIND SANITIZERS_REQUESTED "undefined" HAS_UNDEFINED)
    if ("${HAS_ADDRESS}" GREATER_EQUAL 0)
        list(APPEND SANITIZERS_SELECTED "address")
        list(REMOVE_ITEM SANITIZERS_REQUESTED "address")
        set_sanitizer_options(address DEFAULT -fno-omit-frame-pointer)
    endif()
    if ("${HAS_LEAK}" GREATER_EQUAL 0)
        if ("${HAS_ADDRESS}" GREATER_EQUAL 0)
            message(WARNING "The Leak sanitizer is automatically incorporated into the Address sanitizer and only needs to be specified if it is to be used stand-alone WITHOUT the latter.")
        else()
            list(APPEND SANITIZERS_SELECTED "leak")
            list(REMOVE_ITEM SANITIZERS_REQUESTED "leak")
            # Although this doesn't set any options it does check that the sanitizer is available
            set_sanitizer_options(leak DEFAULT "")
        endif()
    endif()
    if ("${HAS_THREAD}" GREATER_EQUAL 0)
        list(APPEND SANITIZERS_SELECTED "thread")
        list(REMOVE_ITEM SANITIZERS_REQUESTED "thread")
        set_sanitizer_options(thread DEFAULT -fno-omit-frame-pointer)
    endif()
    if ("${HAS_MEMORY}" GREATER_EQUAL 0)
        list(APPEND SANITIZERS_SELECTED "memory")
        list(REMOVE_ITEM SANITIZERS_REQUESTED "memory")
        set_sanitizer_options(memory DEFAULT -fno-omit-frame-pointer)
    endif()
    if ("${HAS_MEMORYWITHORIGINS}" GREATER_EQUAL 0)
        list(APPEND SANITIZERS_SELECTED "memoryWithOrigins")
        list(REMOVE_ITEM SANITIZERS_REQUESTED "memoryWithOrigins")
        set_sanitizer_options(memoryWithOrigins DEFAULT SANITIZER memory
            -fno-omit-frame-pointer
            -fsanitize-memory-track-origins)
    endif()
    # Also if you want UBSan to print symbolized stack trace for each error report
    # you need to run with UBSAN_OPTIONS=print_stacktrace=1 set in the environment
    # and use UBSAN_OPTIONS=log_path=.... to send the output to somewhere other
    # than stderr. llvm-symbolizer also needs to be a location in PATH.
    if ("${HAS_UNDEFINED}" GREATER_EQUAL 0)
        list(APPEND SANITIZERS_SELECTED "undefined")
        list(REMOVE_ITEM SANITIZERS_REQUESTED "undefined")
        set_sanitizer_options(undefined DEFAULT -g -fno-sanitize-merge -fno-omit-frame-pointer)
    endif()
    if ("${HAS_TYPE}" GREATER_EQUAL 0)
        list(APPEND SANITIZERS_SELECTED "type")
        list(REMOVE_ITEM SANITIZERS_REQUESTED "type")
        set_sanitizer_options(type DEFAULT "")
    endif()

    # add_sanitizer_support(... ) seems to need a space separated string of
    # sanitizers wanted
    list(JOIN SANITIZERS_SELECTED " " SANITIZERS_AS_STRING)
    list(JOIN SANITIZERS_REQUESTED "', '" UNRECOGNISED_AS_STRING)
else()
    unset(SANITIZERS_SELECTED)
    unset(SANITIZERS_AS_STRING)
    unset(UNRECOGNISED_AS_STRING)
endif()

# In the event that MUDLET_SANITIZERS is not defined or is empty this should
# clear any prior configuration. Don't report which sanitizers are being used
# here as this is used more than once in a full build and there is no need to
# repeat the message, it is now done in the second-level src/CMakeLists.txt
# file.
add_sanitizer_support("${SANITIZERS_SELECTED}")
