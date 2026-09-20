include(${CMAKE_SOURCE_DIR}/3rdparty/cmake-scripts/sanitizers.cmake)

set_sanitizer_options(address DEFAULT -fno-omit-frame-pointer)
set_sanitizer_options(thread DEFAULT -fno-omit-frame-pointer)

set(USE_SANITIZER
    "address"
    CACHE STRING
    "Compile with sanitizers. Available sanitizers are: \
    Address, Memory, MemoryWithOrigins, Undefined, Thread, or Leak"
)

add_sanitizer_support(${USE_SANITIZER})
message(STATUS "Compiling with the following sanitizer(s) enabled: ${USE_SANITIZER}")
