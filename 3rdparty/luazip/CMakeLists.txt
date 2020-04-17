project(luazip LANGUAGES C)

find_package(Lua51 REQUIRED)
find_package(ZZip REQUIRED)

add_library(luazip INTERFACE)

target_link_libraries(luazip INTERFACE LUA51::LUA51 ZZip::ZZip)
target_include_directories(luazip INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
