# Builds Lua through 3rdparty/lua51's own CMakeLists, so it gets the flags that file gives lua51 on Android.
# Linked statically against bionic, the program runs on a Linux x86_64 host without an emulator.
cmake_minimum_required(VERSION 3.25)
project(android-lua51 C)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/lua51 lua51)
add_executable(lua51-openlibs openlibs.c)
target_link_libraries(lua51-openlibs PRIVATE LUA51::LUA51)
target_link_options(lua51-openlibs PRIVATE -static)
