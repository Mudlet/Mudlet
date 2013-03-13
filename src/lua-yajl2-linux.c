#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <lua.h>
#include <lauxlib.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define js_check_generator(L, narg) \
    (yajl_gen*)luaL_checkudata((L), (narg), "yajl.generator.meta")

static void* js_null;

static int js_generator(lua_State *L);
static int js_generator_value(lua_State *L);
static void js_parser_assert(lua_State* L,
                             yajl_status status,
                             yajl_handle* handle,
                             const unsigned char* json_text,
                             size_t json_text_len,
                             const char* file,
                             int line);
static int got_map_key(lua_State* L);
static int got_map_value(lua_State* L);


static double todouble(lua_State* L, const char* val, size_t len) {
    /* Convert into number using a temporary */
    char* tmp = (char*)lua_newuserdata(L, len+1);
    double num;
    memcpy(tmp, val, len);
    tmp[len] = '\0';
    num = strtod(tmp, NULL);
/*
        if ((num == HUGE_VAL || num == -HUGE_VAL) &&
            errno == ERANGE)
        {
            TODO: Add appropriate handling of large numbers by delegating.
        }
        TODO: How can we tell if there was information loss?  aka the
            number of significant digits in the string exceeds the
            significant digits in the double.
*/
    lua_pop(L, 1);
    return num;
}


static int js_to_string(lua_State *L) {
    yajl_gen* gen;
    const unsigned char *buf;
    size_t len;

    lua_pushcfunction(L, js_generator);
    /* convert_me, {extra}, ?, js_gen */
    if ( lua_istable(L, 2) ) {
        /* Be sure printer is not defined: */
        lua_pushliteral(L, "printer");
        lua_pushnil(L);
        lua_rawset(L, 2);
        lua_pushvalue(L, 2);
        /* convert_me, {extra}, ?, js_gen, {extra} */
    } else {
        lua_newtable(L);
        /* convert_me, {extra}, ?, js_gen, {} */
    }
    lua_call(L, 1, 1);
    /* convert_me, {extra}, ?, gen_ud */
    lua_pushcfunction(L, js_generator_value);
    /* convert_me, {extra}, ?, gen_ud, js_gen_val */
    lua_pushvalue(L, -2);
    /* convert_me, {extra}, ?, gen_ud, js_gen_val, gen_ud */
    lua_pushvalue(L, 1);
    /* convert_me, {extra}, ?, gen_ud, js_gen_val, gen_ud, convert_me */
    lua_call(L, 2, 0);
    /* convert_me, {extra}, ?, gen_ud */
    gen = js_check_generator(L, -1);
    yajl_gen_get_buf(*gen, &buf, &len);
    /* Copy into results: */
    lua_pushlstring(L, (char*)buf, len);
    yajl_gen_clear(*gen);
    return 1;
}

/* See STRATEGY section below */
static int to_value_null(void* ctx) {
    lua_State* L = (lua_State*)ctx;

    lua_getfield(L, LUA_REGISTRYINDEX, "yajl.null");
    (lua_tocfunction(L, -2))(L);

    return 1;
}

/* See STRATEGY section below */
static int to_value_boolean(void* ctx, int val) {
    lua_State* L = (lua_State*)ctx;

    lua_pushboolean(L, val);
    (lua_tocfunction(L, -2))(L);

    return 1;
}

/* See STRATEGY section below */
static int to_value_number(void* ctx, const char* val, size_t len) {
    lua_State* L = (lua_State*)ctx;
    lua_pushnumber(L, todouble(L, val, len));
    (lua_tocfunction(L, -2))(L);

    return 1;
}

/* See STRATEGY section below */
static int to_value_string(void* ctx, const unsigned char *val, size_t len) {
    lua_State* L = (lua_State*)ctx;

    lua_pushlstring(L, (const char*)val, len);
    (lua_tocfunction(L, -2))(L);

    return 1;
}

/* See STRATEGY section below */
static int got_map_value(lua_State* L) {
    /* ..., Table, Key, Func, Value */
    lua_insert(L, -2);
    lua_pop(L, 1);
    lua_rawset(L, -3);
    lua_pushnil(L); /* Store future key here. */
    lua_pushcfunction(L, got_map_key);

    return 0; /* Ignored. */
}

/* See STRATEGY section below */
static int got_map_key(lua_State* L) {
    lua_replace(L, -3);
    lua_pop(L, 1);
    lua_pushcfunction(L, got_map_value);

    return 0; /* Ignored. */
}

/* See STRATEGY section below */
static int got_array_value(lua_State* L) {
    /* ..., Table, Integer, Func, Value */
    lua_rawseti(L, -4, lua_tointeger(L, -3));
    lua_pushinteger(L, lua_tointeger(L, -2)+1);
    lua_replace(L, -3);

    return 0; /* Ignored. */
}

/* See STRATEGY section below */
static int to_value_start_map(void* ctx) {
    lua_State* L = (lua_State*)ctx;

    /* The layout of the stack for "objects" is:
       - Table we are appending to.
       - Storage for the "last key found".
       - Function to call in to_value_* functions.
       - Value pushed on the stack by to_value_* functions.
    */
    if ( ! lua_checkstack(L, 4) ) {
        /* TODO: So far, YAJL seems to be fine with the longjmp, but
           perhaps we should do errors a different way? */
        return luaL_error(L, "lua stack overflow");
    }

    lua_newtable(L);
    lua_pushnil(L); /* Store future key here. */
    lua_pushcfunction(L, got_map_key);

    return 1;
}

/* See STRATEGY section below */
static int to_value_start_array(void* ctx) {
    lua_State* L = (lua_State*)ctx;

    /* The layout of the stack for "arrays" is:
       - Table we are appending to.
       - The index to use for the next insertion.
       - Function to call in to_value_* functions.
       - Value pushed on the stack by to_value_* functions.
    */
    if ( ! lua_checkstack(L, 4) ) {
        /* TODO: So far, YAJL seems to be fine with the longjmp, but
           perhaps we should do errors a different way? */
        return luaL_error(L, "lua stack overflow");
    }

    lua_newtable(L);
    lua_pushinteger(L, 1);
    lua_pushcfunction(L, got_array_value);

    return 1;
}

/* See STRATEGY section below */
static int to_value_end(void* ctx) {
    lua_State* L = (lua_State*)ctx;

    /* Simply pop the stack and call the cfunction: */
    lua_pop(L, 2);
    (lua_tocfunction(L, -2))(L);

    return 1;
}

/* See STRATEGY section below */
static int noop(lua_State* L) {
    return 0;
}

/* See STRATEGY section below */
static yajl_callbacks js_to_value_callbacks = {
    to_value_null,
    to_value_boolean,
    NULL,
    NULL,
    to_value_number,
    to_value_string,
    to_value_start_map,
    to_value_string,
    to_value_end,
    to_value_start_array,
    to_value_end,
};


/* STRATEGY:
 *
 * Each of the js_to_value_callbacks perform these actions:
 *
 * [1] Push a new value onto the top of the Lua stack.
 *
 * [2] Call the function that was at the top of the Lua stack before
 *     step [1] occurred.
 *
 * The purpose of the function call in [2] is to take the value at the
 * top of the stack and store it in the appropriate location.
 * Initially, the function is the noop (no operation) function which
 * does nothing.  Therefore we know that the final result is on the
 * top of the Lua stack.
 *
 * The to_value_start_map and to_value_start_array callbacks are
 * different since they need to use a bit of the Lua stack to store
 * some state information.  When these callbacks are ran, they perform
 * these actions:
 *
 * [a] Push a new table which will represent the final "array" or
 *     "object" onto the top of the Lua stack.
 *
 * [b] Allocate space for the "key" (in the case of arrays, this is
 *     the index into the array to use as part of the next insertion)
 *
 * [c] Push the got_array_value or got_map_key function.
 *
 * The got_array_value function will take the value at the top of the
 * stack and insert it into the table created in step [a].  It will
 * then increment the index created in step [b].  As a final step, it
 * removes the value at the top of the stack.
 *
 * The got_map_key function simply takes the value at the top of the
 * stack and stores it in the space allocated by step [b] above.  It
 * then replaces the function pushed onto the stack by step [c] with
 * the got_map_value function.  As a final step, it removes the value
 * at the top of the stack.
 *
 * The got_map_value function takes the value at the top of the stack
 * and inserts it into the table created in step [a] with the key
 * whose space was allocated in step [b].  The function pushed onto
 * the stack by step [c] is then restored back to the got_map_key
 * function.  As a final step, it removes the value at the top of the
 * stack.
 */
static int js_to_value(lua_State *L) {
    yajl_handle          handle;
    size_t               len;
    const unsigned char* buff = (const unsigned char*) luaL_checklstring(L, 1, &len);

    if ( NULL == buff ) return 0;

    handle = yajl_alloc(&js_to_value_callbacks, NULL, (void*)L);
    lua_pushcfunction(L, noop);

    if ( lua_istable(L, 2) ) {
        lua_getfield(L, 2, "allow_comments");
        if ( ! lua_isnil(L, -1) ) {
            yajl_config(handle, yajl_allow_comments, lua_toboolean(L, -1));
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "check_utf8");
        if ( ! lua_isnil(L, -1) ) {
            yajl_config(handle, yajl_dont_validate_strings, !lua_toboolean(L, -1));
        }
        lua_pop(L, 1);
    }

    js_parser_assert(L,
                     yajl_parse(handle, buff, len),
                     &handle,
                     buff,
                     len,
                     __FILE__,
                     __LINE__);

    js_parser_assert(L,
                     yajl_complete_parse(handle),
                     &handle,
                     buff,
                     len,
                     __FILE__,
                     __LINE__);

    yajl_free(handle);

    return 1;
}

static int js_parser_null(void *ctx) {
    lua_State *L=(lua_State*)ctx;
    lua_getfield(L, lua_upvalueindex(2), "value");
    if ( ! lua_isnil(L, -1) ) {
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_getfield(L, LUA_REGISTRYINDEX, "yajl.null");
        lua_pushliteral(L, "null");
        lua_call(L, 3, 0);
    } else {
        lua_pop(L, 1);
    }

    return 1;
}

static int js_parser_boolean(void *ctx, int val) {
    lua_State *L=(lua_State*)ctx;

    lua_getfield(L, lua_upvalueindex(2), "value");
    if ( ! lua_isnil(L, -1) ) {
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushboolean(L, val);
        lua_pushliteral(L, "boolean");
        lua_call(L, 3, 0);
    } else {
        lua_pop(L, 1);
    }

    return 1;
}

static int js_parser_number(void *ctx, const char* buffer, size_t buffer_len) {
    lua_State *L=(lua_State*)ctx;

    lua_getfield(L, lua_upvalueindex(2), "value");
    if ( ! lua_isnil(L, -1) ) {
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushnumber(L, todouble(L, buffer, buffer_len));
        lua_pushliteral(L, "number");
        lua_call(L, 3, 0);
    } else {
        lua_pop(L, 1);
    }

    return 1;
}

static int js_parser_string(void *ctx, const unsigned char *val, size_t len) {
    lua_State *L=(lua_State*)ctx;

    lua_getfield(L, lua_upvalueindex(2), "value");
    if ( ! lua_isnil(L, -1) ) {
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushlstring(L, (const char*)val, len);
        lua_pushliteral(L, "string");
        lua_call(L, 3, 0);
    } else {
        lua_pop(L, 1);
    }

    return 1;
}

static int js_parser_start_map(void *ctx) {
    lua_State *L=(lua_State*)ctx;

    lua_getfield(L, lua_upvalueindex(2), "open_object");
    if ( ! lua_isnil(L, -1) ) {
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_call(L, 1, 0);
    } else {
        lua_pop(L, 1);
    }

    return 1;
}

static int js_parser_map_key(void *ctx, const unsigned char *val, size_t len) {
    lua_State *L=(lua_State*)ctx;

    /* TODO: Do we want to fall-back to calling "value"? */
    lua_getfield(L, lua_upvalueindex(2), "object_key");
    if ( ! lua_isnil(L, -1) ) {
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushlstring(L, (const char*)val, len);
        lua_call(L, 2, 0);
    } else {
        lua_pop(L, 1);
    }

    return 1;
}

static int js_parser_end_map(void *ctx) {
    lua_State *L=(lua_State*)ctx;

    lua_getfield(L, lua_upvalueindex(2), "close");
    if ( ! lua_isnil(L, -1) ) {
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushliteral(L, "object");
        lua_call(L, 2, 0);
    } else {
        lua_pop(L, 1);
    }

    return 1;
}

static int js_parser_start_array(void *ctx) {
    lua_State *L=(lua_State*)ctx;

    lua_getfield(L, lua_upvalueindex(2), "open_array");
    if ( ! lua_isnil(L, -1) ) {
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_call(L, 1, 0);
    } else {
        lua_pop(L, 1);
    }

    return 1;
}

static int js_parser_end_array(void *ctx) {
    lua_State *L=(lua_State*)ctx;

    lua_getfield(L, lua_upvalueindex(2), "close");
    if ( ! lua_isnil(L, -1) ) {
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushliteral(L, "array");
        lua_call(L, 2, 0);
    } else {
        lua_pop(L, 1);
    }

    return 1;
}

static yajl_callbacks js_parser_callbacks = {
    js_parser_null,
    js_parser_boolean,
    NULL,
    NULL,
    js_parser_number,
    js_parser_string,
    js_parser_start_map,
    js_parser_map_key,
    js_parser_end_map,
    js_parser_start_array,
    js_parser_end_array
};

static void js_parser_assert(lua_State* L,
                             yajl_status status,
                             yajl_handle* handle,
                             const unsigned char* json_text,
                             size_t json_text_len,
                             const char* file,
                             int line)
{
    int verbose = 1;
    unsigned char* msg;

    switch ( status ) {
    case yajl_status_ok:
        return;
    case yajl_status_client_canceled:
        lua_pushfstring(L, "Unreachable: yajl_status_client_canceled should never be returned since all callbacks return true at %s line %d",
                        file, line);
        break;
    case yajl_status_error:
        msg = yajl_get_error(*handle, verbose, json_text, json_text_len);
        lua_pushfstring(L, "InvalidJSONInput: %s at %s line %d", msg, file, line);
        yajl_free_error(*handle, msg);
        break;
    }
    lua_error(L);
}

static int js_parser_parse(lua_State *L) {
    yajl_handle* handle = (yajl_handle*)
        lua_touserdata(L, lua_upvalueindex(1));
    if ( lua_isnil(L, 1) ) {
        js_parser_assert(L,
                         yajl_complete_parse(*handle),
                         handle,
                         NULL,
                         0,
                         __FILE__,
                         __LINE__);
    } else {
        size_t len;
        const unsigned char* buff = (const unsigned char*) luaL_checklstring(L, 1, &len);
        if ( NULL == buff ) return 0;
        js_parser_assert(L,
                         yajl_parse(*handle, buff, len),
                         handle,
                         buff,
                         len,
                         __FILE__,
                         __LINE__);
    }
    return 0;
}

static int js_parser_delete(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);
    yajl_free(*(yajl_handle*)lua_touserdata(L, 1));
    return 0;
}

static int js_parser(lua_State *L) {
    yajl_handle* handle;

    luaL_checktype(L, 1, LUA_TTABLE);

    handle = (yajl_handle*)lua_newuserdata(L, sizeof(yajl_handle));

    *handle = yajl_alloc(&js_parser_callbacks, NULL, (void*)L);
    luaL_getmetatable(L, "yajl.parser.meta");
    lua_setmetatable(L, -2);

    lua_getfield(L, 1, "allow_comments");
    if ( ! lua_isnil(L, -1) ) {
        yajl_config(*handle, yajl_allow_comments, lua_toboolean(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "check_utf8");
    if ( ! lua_isnil(L, -1) ) {
        yajl_config(*handle, yajl_dont_validate_strings, !lua_toboolean(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "events");

    /* Create callback function that calls yajl_parse[_complete]()
     
      upvalue(1) = yajl_handle*
      upvalue(2) = events table */
    lua_pushcclosure(L, &js_parser_parse, 2);

    return 1;
}

static int js_generator_delete(lua_State *L) {
    yajl_gen* handle = js_check_generator(L, 1);
    yajl_gen_free(*handle);
    return 0;
}

static void js_generator_assert(lua_State *L,
                                yajl_gen_status status,
                                const char* file,
                                int line)
{
    switch ( status ) {
    case yajl_gen_status_ok:
    case yajl_gen_generation_complete:
        return;
    case yajl_gen_keys_must_be_strings:
        lua_pushfstring(L, "InvalidState: expected either a call to close() or string() since we are in the middle of an object declaration at %s line %d", file, line);
        break;
    case yajl_max_depth_exceeded:
        lua_pushfstring(L, "StackOverflow: YAJL's max generation depth was exceeded at %s line %d", file, line);
        break;
    case yajl_gen_in_error_state:
        lua_pushfstring(L, "AlreadyInError: generator method was called when the generator is already in an error state at %s line %d", file, line);
        break;
    default:
        lua_pushfstring(L, "Unreachable: yajl_gen_status (%d) not recognized at %s line %d", status, file, line);
        break;
    }
    lua_error(L);
}

static int js_generator_integer(lua_State *L) {
    js_generator_assert(L,
                        yajl_gen_integer(*js_check_generator(L, 1),
                                         luaL_checkinteger(L, 2)),
                        __FILE__, __LINE__);
    return 0;
}

static int js_generator_double(lua_State *L) {
    js_generator_assert(L,
                        yajl_gen_double(*js_check_generator(L, 1),
                                        luaL_checknumber(L, 2)),
                        __FILE__, __LINE__);
    return 0;
}

static int js_generator_number(lua_State *L) {

    /* It would be better to make it so an arbitrary string can be
       used here, however we would then need to validate that the
       generated string is a JSON number which is a bit beyond scope
       at this point.  Perhaps in the future we will loosen this
       restriction, it is always easier to loosen restrictions than it
       is to make new restrictions that break other people's code. */
    double num = luaL_checknumber(L, 2);

    size_t len;
    const char* str;

    /* These are special cases, not sure how better to represent them
       :-(. */
    if ( num == HUGE_VAL ) {
        str = "1e+666";
        len = 6;
    } else if ( num == -HUGE_VAL ) {
        str = "-1e+666";
        len = 7;
    } else if ( isnan(num) ) {
        str = "-0"; 
        len = 2;
   } else {
        str = luaL_checklstring(L, 2, &len);
    }
    js_generator_assert(L,
                        yajl_gen_number(*js_check_generator(L, 1),
                                        str, len),
                        __FILE__, __LINE__);
    return 0;
}

static int js_generator_string(lua_State *L) {
    size_t len;
    const unsigned char* str = (const unsigned char*)luaL_checklstring(L, 2, &len);
    js_generator_assert(L,
                        yajl_gen_string(*js_check_generator(L, 1),
                                        str, len),
                        __FILE__, __LINE__);
    return 0;
}

static int js_generator_null(lua_State *L) {
    js_generator_assert(L,
                        yajl_gen_null(*js_check_generator(L, 1)),
                        __FILE__, __LINE__);
    return 0;
}

static int js_generator_boolean(lua_State *L) {
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    js_generator_assert(L,
                        yajl_gen_bool(*js_check_generator(L, 1),
                                      lua_toboolean(L, 2)),
                        __FILE__, __LINE__);
    return 0;
}

#define JS_OPEN_OBJECT 1
#define JS_OPEN_ARRAY  2

static int js_generator_open_object(lua_State *L) {
    js_generator_assert(L,
                        yajl_gen_map_open(*js_check_generator(L, 1)),
                        __FILE__, __LINE__);
    /* Why doesn't yajl_gen keep track of this!? */
    lua_getfenv(L, 1);
    lua_getfield(L, -1, "stack");
    lua_pushinteger(L, JS_OPEN_OBJECT);
    lua_rawseti(L, -2, lua_objlen(L, -2) + 1);
    return 0;
}

static int js_generator_open_array(lua_State *L) {
    js_generator_assert(L,
                        yajl_gen_array_open(*js_check_generator(L, 1)),
                        __FILE__, __LINE__);
    /* Why doesn't yajl_gen keep track of this!? */
    lua_getfenv(L, 1);
    lua_getfield(L, -1, "stack");
    lua_pushinteger(L, JS_OPEN_ARRAY);
    lua_rawseti(L, -2, lua_objlen(L, -2) + 1);
    return 0;
}

static int js_generator_close(lua_State *L) {
    lua_Integer type;

    /* Why doesn't yajl_gen keep track of this!? */
    lua_getfenv(L, 1);
    lua_getfield(L, -1, "stack");
    lua_rawgeti(L, -1, lua_objlen(L, -1));
    if ( lua_isnil(L, -1) ) {
        lua_pushfstring(L, "StackUnderflow: Attempt to call close() when no array or object has been opened at %s line %d", __FILE__, __LINE__);
        lua_error(L);
    }
    type = lua_tointeger(L, -1);
    switch ( type ) {
    case JS_OPEN_OBJECT:
        js_generator_assert(L,
                            yajl_gen_map_close(*js_check_generator(L, 1)),
                            __FILE__, __LINE__);
        break;
    case JS_OPEN_ARRAY:
        js_generator_assert(L,
                            yajl_gen_array_close(*js_check_generator(L, 1)),
                            __FILE__, __LINE__);
        break;
    default:
        lua_pushfstring(L, "Unreachable: internal 'stack' contained invalid integer (%d) at %s line %d", type, __FILE__, __LINE__);
        lua_error(L);
    }
    /* delete the top of the "stack": */
    lua_pop(L, 1);
    lua_pushnil(L);
    lua_rawseti(L, -2, lua_objlen(L, -2));

    return 0;
}

static int js_generator_value(lua_State *L) {
    int max;
    int is_array;
    int type = lua_type(L, 2);

    switch ( type ) {
    case LUA_TNIL:
        return js_generator_null(L);
    case LUA_TNUMBER:
        return js_generator_number(L);
    case LUA_TBOOLEAN:
        return js_generator_boolean(L);
    case LUA_TSTRING:
        return js_generator_string(L);
    case LUA_TUSERDATA:
        if ( lua_topointer(L, 2) == js_null ) { 
            return js_generator_null(L);
        }
    case LUA_TLIGHTUSERDATA:
    case LUA_TTABLE:
    case LUA_TFUNCTION:
    case LUA_TTHREAD:
        if ( luaL_getmetafield(L, 2, "__gen_json") ) {
            if  ( lua_isfunction(L, -1) ) {
                lua_settop(L, 3); /* gen, obj, func */
                lua_insert(L, 1); /* func, gen, obj */
                lua_insert(L, 2); /* func, obj, gen */
                lua_call(L, 2, 0);
                return 0;
            }
            lua_pop(L, 1);
        }

        /* Simply ignore it, perhaps we should warn? */
        if ( type != LUA_TTABLE ) return 0;

        max      = 0;
        is_array = 1;

        /* First iterate over the table to see if it is an array: */
        lua_pushnil(L);
        while ( lua_next(L, 2) != 0 ) {
            if ( lua_type(L, -2) == LUA_TNUMBER ) {
                double num = lua_tonumber(L, -2);
                if ( num == floor(num) ) {
                    if ( num > max ) max = num;
                } else {
                    lua_pop(L, 2);
                    is_array = 0;
                    break;
                }
            } else {
                lua_pop(L, 2);
                is_array = 0;
                break;
            }
            lua_pop(L, 1);
        }

        if ( is_array ) {
            int i;
            js_generator_open_array(L);
            for ( i=1; i <= max; i++ ) {
                lua_pushinteger(L, i);
                lua_gettable(L, 2);

                /* RECURSIVE CALL:
                   gen, obj, ?, val, func, gen, val */
                lua_pushcfunction(L, js_generator_value);
                lua_pushvalue(L, 1);
                lua_pushvalue(L, -3);
                lua_call(L, 2, 0);

                lua_pop(L, 1);
            }
        } else {
            js_generator_open_object(L);

            lua_pushnil(L);
            while ( lua_next(L, 2) != 0 ) {
                /* gen, obj, ?, key, val, func, gen, key */
                lua_pushcfunction(L, js_generator_string);
                lua_pushvalue(L, 1);
                if ( lua_isstring(L, -4) ) {
                    lua_pushvalue(L, -4);
                } else {
                    /* Must coerce into a string: */
                    lua_getglobal(L, "tostring");
                    lua_pushvalue(L, -5);
                    lua_call(L, 1, 1);
                }
                lua_call(L, 2, 0);


                /* RECURSIVE CALL:
                   gen, obj, ?, key, val, func, gen, val */
                lua_pushcfunction(L, js_generator_value);
                lua_pushvalue(L, 1);
                lua_pushvalue(L, -3);
                lua_call(L, 2, 0);

                lua_pop(L, 1);
            }
        }
        js_generator_close(L);
        return 0;
    case LUA_TNONE:
        lua_pushfstring(L, "MissingArgument: second parameter to js_generator_value() must be defined at %s line %d", type, __FILE__, __LINE__);
    default:
        lua_pushfstring(L, "Unreachable: js_generator_value passed lua type (%d) not recognized at %s line %d", type, __FILE__, __LINE__);
    }
    /* Shouldn't get here: */
    lua_error(L);
    return 0;
}

typedef struct {
    lua_State* L;
    int        printer_ref;
} js_printer_ctx;

static void js_printer(void* void_ctx, const char* str, size_t len) {
    js_printer_ctx* ctx = (js_printer_ctx*)void_ctx;
    lua_State* L = ctx->L;

    /* refs */
    lua_getfield(L, LUA_REGISTRYINDEX, "yajl.refs");
    /* refs, printer */
    lua_rawgeti(L, -1, ctx->printer_ref);
    if ( lua_isfunction(L, -1) ) {
        lua_pushlstring(L, str, len);
        /* Not sure if yajl can handle longjmp's if this errors... */
        lua_call(L, 1, 0);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

static int js_generator(lua_State *L) {
    yajl_print_t   print = NULL;
    void *         ctx   = NULL;
    yajl_gen*      handle;

    luaL_checktype(L, 1, LUA_TTABLE);

    /* {args}, ?, tbl */
    lua_newtable(L);

    /* Validate and save in fenv so it isn't gc'ed: */
    lua_getfield(L, 1, "printer");
    if ( ! lua_isnil(L, -1) ) {
        js_printer_ctx* print_ctx;

        luaL_checktype(L, -1, LUA_TFUNCTION);

        lua_pushvalue(L, -1);

        /* {args}, ?, tbl, printer, printer */
        lua_setfield(L, -3, "printer");

        print_ctx = (js_printer_ctx*)
            lua_newuserdata(L, sizeof(js_printer_ctx));
        /* {args}, ?, tbl, printer, printer_ctx */

        lua_setfield(L, -3, "printer_ctx");
        /* {args}, ?, tbl, printer */

        lua_getfield(L, LUA_REGISTRYINDEX, "yajl.refs");
        /* {args}, ?, tbl, printer, refs */
        lua_insert(L, -2);
        /* {args}, ?, tbl, refs, printer */
        print_ctx->printer_ref = luaL_ref(L, -2);
        print_ctx->L = L;
        print = &js_printer;
        ctx   = print_ctx;
    }
    lua_pop(L, 1);
    /* {args}, ?, tbl */

    /* Sucks that yajl's generator doesn't keep track of this for me
       (this is a stack of strings "array" and "object" so I can keep
       track of what to "close"): */
    lua_newtable(L);
    lua_setfield(L, -2, "stack");

    /* {args}, ?, tbl */
    handle = (yajl_gen*)lua_newuserdata(L, sizeof(yajl_gen));
    *handle = yajl_gen_alloc(NULL);

    if ( print ) {
        yajl_gen_config(*handle, yajl_gen_print_callback, print, ctx);
    }

    /* Get the indent and save so it isn't gc'ed: */
    lua_getfield(L, 1, "indent");
    if ( ! lua_isnil(L, -1) ) {
        yajl_gen_config(*handle, yajl_gen_beautify, 1);
        yajl_gen_config(*handle, yajl_gen_indent_string, lua_tostring(L, -1));
        lua_setfield(L, -2, "indent");
    } else {
        lua_pop(L, 1);
    }
    /* {args}, ?, tbl */

    /* {args}, ?, tbl, ud, meta */
    luaL_getmetatable(L, "yajl.generator.meta");
    lua_setmetatable(L, -2);
    /* {args}, ?, tbl, ud */

    lua_insert(L, -2);
    /* {args}, ?, ud, tbl */
    lua_setfenv(L, -2);

    return 1;
}

static void js_create_parser_mt(lua_State *L) {
    luaL_newmetatable(L, "yajl.parser.meta");

    lua_pushcfunction(L, js_parser_delete);
    lua_setfield(L, -2, "__gc");

    lua_pop(L, 1);
}

static void js_create_generator_mt(lua_State *L) {
    luaL_newmetatable(L, "yajl.generator.meta");

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, js_generator_delete);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, js_generator_value);
    lua_setfield(L, -2, "value");

    lua_pushcfunction(L, js_generator_integer);
    lua_setfield(L, -2, "integer");

    lua_pushcfunction(L, js_generator_double);
    lua_setfield(L, -2, "double");

    lua_pushcfunction(L, js_generator_number);
    lua_setfield(L, -2, "number");

    lua_pushcfunction(L, js_generator_string);
    lua_setfield(L, -2, "string");

    lua_pushcfunction(L, js_generator_null);
    lua_setfield(L, -2, "null");

    lua_pushcfunction(L, js_generator_boolean);
    lua_setfield(L, -2, "boolean");

    lua_pushcfunction(L, js_generator_open_object);
    lua_setfield(L, -2, "open_object");

    lua_pushcfunction(L, js_generator_open_array);
    lua_setfield(L, -2, "open_array");

    lua_pushcfunction(L, js_generator_close);
    lua_setfield(L, -2, "close");

    lua_pop(L, 1);
}

static int js_null_tostring(lua_State* L) {
    lua_pushstring(L, "null");
    return 1;
}

static void js_create_null_mt(lua_State *L) {
    luaL_newmetatable(L, "yajl.null.meta");

    lua_pushcfunction(L, js_null_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_pop(L, 1);
}

LUALIB_API int luaopen_yajl(lua_State *L) {
    js_create_parser_mt(L);
    js_create_generator_mt(L);
    js_create_null_mt(L);

    /* Create the yajl.refs weak table: */
    lua_createtable(L, 0, 2);
    lua_pushliteral(L, "v"); /* tbl, "v" */
    lua_setfield(L, -2, "__mode");
    lua_pushvalue(L, -1);    /* tbl, tbl */
    lua_setmetatable(L, -2); /* tbl */
    lua_setfield(L, LUA_REGISTRYINDEX, "yajl.refs");

    lua_createtable(L, 0, 4);

    lua_pushcfunction(L, js_to_string);
    lua_setfield(L, -2, "to_string");

    lua_pushcfunction(L, js_to_value);
    lua_setfield(L, -2, "to_value");

    lua_pushcfunction(L, js_parser);
    lua_setfield(L, -2, "parser");

    lua_pushcfunction(L, js_generator);
    lua_setfield(L, -2, "generator");

    js_null = lua_newuserdata(L, 0);
    luaL_getmetatable(L, "yajl.null.meta");
    lua_setmetatable(L, -2);

    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "yajl.null");

    lua_setfield(L, -2, "null");

    return 1;
}
