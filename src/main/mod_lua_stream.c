/**
 * Provides a lua interface to the aerospike struct and functions
 *
 *
 *      aerospike.get(namespace, set, key): result<record>
 *      aerospike.put(namespace, set, key, table)
 *      aerospike.remove(namespace, set, key): result<bool>
 *      aerospike.update(record): result<record>
 *
 *
 */

#include "mod_lua_stream.h"
#include "mod_lua_iterator.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define MOD_LUA_STREAM "Stream"

/**
 * Read the item at index and convert to a stream
 */
as_stream * mod_lua_tostream(lua_State * l, int index) {
    as_stream * s = (as_stream *) lua_touserdata(l, index);
    if (s == NULL) luaL_typerror(l, index, MOD_LUA_STREAM);
    return s;
}

/**
 * Push a stream on to the lua stack
 */
as_stream * mod_lua_pushstream(lua_State * l, as_stream * s) {
    as_stream * ls = (as_stream *) lua_newuserdata(l, sizeof(as_stream));
    *ls = *s;
    luaL_getmetatable(l, MOD_LUA_STREAM);
    lua_setmetatable(l, -2);
    return ls;
}

/**
 * Get the user iterator from the stack at index
 */
static as_stream * mod_lua_checkstream(lua_State * l, int index) {
    as_stream * s = NULL;
    luaL_checktype(l, index, LUA_TUSERDATA);
    s = (as_stream *) luaL_checkudata(l, index, MOD_LUA_STREAM);
    if (s == NULL) luaL_typerror(l, index, MOD_LUA_STREAM);
    return s;
}

/**
 * Gets an iterator over the stream
 */
static int mod_lua_stream_iterator(lua_State * l) {
    as_stream * s = mod_lua_checkstream(l, 1);
    as_iterator * i = as_stream_iterator(s);
    mod_lua_pushiterator(l, i);
    return 1;
}

/**
 * stream methods
 */
static const luaL_reg mod_lua_stream_methods[] = {
    {"iterator",        mod_lua_stream_iterator},
    {0, 0}
};

/**
 * stream metatable
 */
static const luaL_reg mod_lua_stream_metatable[] = {
    {0, 0}
};

/**
 * Registers the iterator library
 */
int mod_lua_stream_register(lua_State * l) {
    int methods, metatable;

    luaL_register(l, MOD_LUA_STREAM, mod_lua_stream_methods);
    methods = lua_gettop(l);

    luaL_newmetatable(l, MOD_LUA_STREAM);
    
    luaL_register(l, 0, mod_lua_stream_metatable);
    metatable = lua_gettop(l);

    lua_pushliteral(l, "__index");
    lua_pushvalue(l, methods);
    lua_rawset(l, metatable);

    lua_pushliteral(l, "__metatable");
    lua_pushvalue(l, methods);
    lua_rawset(l, metatable);
    
    lua_pop(l, 1);

    return 1;
}
