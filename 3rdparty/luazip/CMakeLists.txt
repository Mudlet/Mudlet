project(luazip LANGUAGES C)

find_package(Lua51 REQUIRED)
find_package(ZZIPLIB REQUIRED)

add_library(luazip INTERFACE)

target_link_libraries(luazip INTERFACE LUA51::LUA51 ZZIPLIB::ZZIPLIB)
target_include_directories(luazip INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
