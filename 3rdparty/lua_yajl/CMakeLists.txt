project(lua_yajl LANGUAGES C)

find_package(Lua51 REQUIRED)
find_package(YAJL REQUIRED)

add_library(lua_yajl STATIC lua_yajl.c)

target_link_libraries(lua_yajl ${LUA_LIBRARIES} ${YAJL_LIBRARIES})

target_include_directories(lua_yajl PRIVATE
  ${LUA_INCLUDE_DIR}
  ${YAJL_INCLUDE_DIR}
)
