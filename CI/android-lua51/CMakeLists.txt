# The bundled Lua 5.1 as an Android build gets it, and a program that opens its libraries. Linked
# statically against bionic, the program runs on a Linux x86_64 host without an emulator; see check.sh.
cmake_minimum_required(VERSION 3.25)
project(android-lua51 C)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/lua51 lua51)
add_executable(lua51-openlibs openlibs.c)
target_link_libraries(lua51-openlibs PRIVATE LUA51::LUA51)
target_link_options(lua51-openlibs PRIVATE -static)
