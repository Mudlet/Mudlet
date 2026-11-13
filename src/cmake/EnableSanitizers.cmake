include(${CMAKE_SOURCE_DIR}/3rdparty/cmake-scripts/sanitizers.cmake)

set_sanitizer_options(address DEFAULT -fno-omit-frame-pointer)
set_sanitizer_options(thread DEFAULT -fno-omit-frame-pointer)

# An Android app can only run an AddressSanitizer build with its runtime and a wrap.sh packed into the APK
if(ANDROID)
  set(USE_SANITIZER_DEFAULT "")
else()
  set(USE_SANITIZER_DEFAULT "address")
endif()
set(USE_SANITIZER
    "${USE_SANITIZER_DEFAULT}"
    CACHE STRING
    "Compile with sanitizers. Available sanitizers are: \
    Address, Memory, MemoryWithOrigins, Undefined, Thread, or Leak"
)

add_sanitizer_support(${USE_SANITIZER})
message(STATUS "Compiling with the following sanitizer(s) enabled: ${USE_SANITIZER}")
