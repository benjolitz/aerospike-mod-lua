/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#include <arpa/inet.h> // byteswap
#include <endian.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <aerospike/as_val.h>

#include <aerospike/mod_lua_val.h>
#include <aerospike/mod_lua_bytes.h>
#include <aerospike/mod_lua_iterator.h>
#include <aerospike/mod_lua_reg.h>

#include "internal.h"



/*******************************************************************************
 * MACROS
 ******************************************************************************/

#define OBJECT_NAME "bytes"
#define CLASS_NAME  "Bytes"

/*******************************************************************************
 * BOX FUNCTIONS
 ******************************************************************************/

as_bytes * mod_lua_tobytes(lua_State * l, int index) {
	mod_lua_box * box = mod_lua_tobox(l, index, CLASS_NAME);
	return (as_bytes *) mod_lua_box_value(box);
}

as_bytes * mod_lua_pushbytes(lua_State * l, as_bytes * b) {
	mod_lua_box * box = mod_lua_pushbox(l, MOD_LUA_SCOPE_LUA, b, CLASS_NAME);
	return (as_bytes *) mod_lua_box_value(box);
}

static as_bytes * mod_lua_checkbytes(lua_State * l, int index) {
	mod_lua_box * box = mod_lua_checkbox(l, index, CLASS_NAME);
	return (as_bytes *) mod_lua_box_value(box);
}

static int mod_lua_bytes_gc(lua_State * l) {
	mod_lua_freebox(l, 1, CLASS_NAME);
	return 0;
}

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static int mod_lua_bytes_size(lua_State * l)
{
	// we expect 1 arg
	if ( lua_gettop(l) != 1 ) {
		lua_pushinteger(l, 0);
		return 1;
	}

	as_bytes * b = mod_lua_checkbytes(l, 1);
	
	// check preconditions:
	//	- b != NULL
	if ( !b ) {
		lua_pushinteger(l, 0);
		return 1;
	}

	lua_pushinteger(l, as_bytes_size(b));
	return 1;
}

static int mod_lua_bytes_capacity(lua_State * l)
{
	// we expect 1 arg
	if ( lua_gettop(l) != 1 ) {
		lua_pushinteger(l, 0);
		return 1;
	}

	as_bytes * b = mod_lua_checkbytes(l, 1);
	
	// check preconditions:
	//	- b != NULL
	if ( !b ) {
		lua_pushinteger(l, 0);
		return 1;
	}

	lua_pushinteger(l, as_bytes_capacity(b));
	return 1;
}

static int mod_lua_bytes_ensure(lua_State *l)
{
	// we expect 2 args
	if ( lua_gettop(l) != 2) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer c = luaL_optinteger(l, 2, 0);
	int 		r = luaL_optint(l, 3, 0);

	// check preconditions:
	//	- b != NULL
	//	- 0 <= c <= INT32_MAX
	if ( !b || 
		 c < 0 || c > UINT32_MAX ||
		 r < 0 || r > 1) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool res = as_bytes_ensure(b, (uint32_t) c, r==1);
	lua_pushboolean(l, res);
	return 1;
}

static int mod_lua_bytes_truncate(lua_State *l)
{
	// we expect 2 args
	if ( lua_gettop(l) != 2) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer n = luaL_optinteger(l, 2, 0);

	// check preconditions:
	//	- b != NULL
	//	- 0 <= v <= INT32_MAX
	if ( !b || 
		 n < 0 || n > UINT32_MAX ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool res = as_bytes_truncate(b, (uint32_t) n);
	lua_pushboolean(l, res);
	return 1;
}

static int mod_lua_bytes_new(lua_State * l)
{
	int argc = lua_gettop(l);

	as_bytes * bytes = NULL;

	if ( argc == 1 ) {
		// no arguments
		bytes = as_bytes_new(0);
	}
	else if ( argc == 2 ) {
		// single integer argument for capacity
		lua_Integer n = luaL_optinteger(l, 2, 0);
		bytes = as_bytes_new((uint32_t) n);
	}

	if ( !bytes ) {
		return 0;
	}

	mod_lua_pushbytes(l, bytes);
	return 1;
}

static int mod_lua_bytes_tostring(lua_State * l)
{
	// we expect 1 arg
	if ( lua_gettop(l) != 1 ) {
		lua_pushinteger(l, 0);
		return 1;
	}

	mod_lua_box *   box = mod_lua_checkbox(l, 1, CLASS_NAME);
	as_val *        val = mod_lua_box_value(box);
	char *          str = NULL;

	if ( val ) {
		str = as_val_tostring(val);
	}

	if ( str ) {
		lua_pushstring(l, str);
		free(str);
	}
	else {
		lua_pushstring(l, "Bytes()");
	}

	return 1;
}

/**
 *	Get the type of bytes:
 *
 *	----------{.c}
 *	uint bytes.type(bytes b)
 *	----------
 *
 */
static int mod_lua_bytes_get_type(lua_State * l)
{
	// we expect atleast 1 arg
	if ( lua_gettop(l) >= 1 ) {
		return 0;
	}

	as_bytes * b = mod_lua_checkbytes(l, 1);

	// check preconditions:
	//	- b != NULL
	if ( !b ) {
		return 0;
	}

	lua_pushinteger(l, as_bytes_get_type(b));
	return 1;
}

/**
 *	Set the type of bytes:
 *
 *	----------{.c}
 *	bool bytes.type(bytes b, uint 5)
 *	----------
 */
static int mod_lua_bytes_set_type(lua_State * l)
{
	// we expect 2 args
	if ( lua_gettop(l) != 2 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer t = luaL_optinteger(l, 2, 0);

	// check preconditions:
	//	- b != NULL
	if ( !b || !t ) {
		return 0;
	}

	as_bytes_set_type(b, t);
	return 0;
}


/******************************************************************************
 *	APPEND FUNCTIONS
 *****************************************************************************/

/**
 *	Append a byte value.
 *
 *	----------{.c}
 *	bool bytes.append_byte(bytes b, uint8 v)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param v	The uint8_t value append to b.
 *	
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_append_byte(lua_State * l)
{
	// we expect 2 args
	if ( lua_gettop(l) != 2 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer v = luaL_optinteger(l, 2, 0);

	// check preconditions:
	//	- b != NULL
	//	- UINT8_MIN <= v <= UINT8_MAX
	if ( !b || 
		 v < 0 || v > UINT8_MAX ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool 		res = false;
	uint32_t	pos = b->size;
	uint32_t 	size = 1;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		uint8_t	val	= htons((uint8_t) v);
		res	= as_bytes_append_byte(b, val);
	}

	lua_pushboolean(l, res);
	return 1;
}

/**
 *	Append an int16_t value.
 *
 *	----------{.c}
 *	bool bytes.append_int16(bytes b, int16 v)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param v	The int16_t value to append to b.
 *
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_append_int16(lua_State * l)
{
	// we expect 2 args
	if ( lua_gettop(l) != 2 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer v = luaL_optinteger(l, 2, 0);

	// check preconditions:
	//	- b != NULL
	//	- INT32_MIN <= v <= INT32_MAX
	if ( !b || 
		 v < INT16_MIN || v > INT16_MAX ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool 		res = false;
	uint32_t	pos = b->size;
	uint32_t 	size = 2;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		int16_t	val	= htons((int16_t) v);
		res	= as_bytes_append_int16(b, val);
	}

	lua_pushboolean(l, res);
	return 1;
}

/**
 *	Append an int32_t value.
 *
 *	----------{.c}
 *	bool bytes.append_int32(b, v)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param v	The int32_t value to append to b.
 *
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_append_int32(lua_State * l)
{
	// we expect 2 args
	if ( lua_gettop(l) != 2 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer	v = luaL_optinteger(l, 2, 0);

	// check preconditions:
	//	- b != NULL
	//	- INT32_MIN <= v <= INT32_MAX
	if ( !b || 
		 v < INT32_MIN || v > INT32_MAX ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool 		res = false;
	uint32_t	pos = b->size;
	uint32_t 	size = 4;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		int32_t	val	= htonl((int32_t) v);
		res	= as_bytes_append_int32(b, val);
	}

	lua_pushboolean(l, res);
	return 1;
}

/**
 *	Append an int64_t value.
 *
 *	----------{.c}
 *	bool bytes.append_int64(b, v)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param v	The int64_t value to append to b.
 *
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_append_int64(lua_State * l)
{
	// we expect 2 args
	if ( lua_gettop(l) != 2 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer v = luaL_optinteger(l, 2, 0); 

	// check preconditions:
	// 	- b != NULL 
	//	- INT64_MIN <= v <= INT64_MAX
	if ( !b || 
		 v < INT64_MIN || v > INT64_MAX ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool 		res = false;
	uint32_t	pos = b->size;
	uint32_t 	size = 8;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		int64_t	val	= be64toh((int64_t) v);
		res = as_bytes_append_int64(b, val);
	}

	lua_pushboolean(l, res);
	return 1;
}

/**
 *	Append a NULL-terminated string value.
 *
 *	----------{.c}
 *	bool bytes.append_string(bytes b, string v)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param v	The NULL-terminated string value to append to b.
 *
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_append_string(lua_State * l)
{
	// we expect 2 args
	if ( lua_gettop(l) != 2 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 		b = mod_lua_checkbytes(l, 1);
	size_t  		n = 0;
	const char *	v = luaL_optlstring(l, 2, NULL, &n);

	// check preconditions:
	// 	- b != NULL 
	//	- v != NULL
	//	- n != 0
	if ( !b || !v || !n ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool 		res = false;
	uint32_t	pos = b->size;
	uint32_t 	size = n;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		res = as_bytes_append(b, (uint8_t *) v, n);
	}

	lua_pushboolean(l, res);
	return 1;
}

/**
 *	Append a NULL-terminated string value.
 *
 *	----------{.c}
 *	bool bytes.append_string(bytes b, bytes v, uint32 n)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param v	The bytes value to append to b.
 *	@param n	The number of bytes to append to b.
 *
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_append_bytes(lua_State * l)
{
	// we expect 3 args
	if ( lua_gettop(l) != 3 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	as_bytes * 	v = mod_lua_checkbytes(l, 2);
	lua_Integer	n = luaL_optinteger(l, 3, 0); 

	// check preconditions:
	// 	- b != NULL 
	//	- v != NULL
	//	- UINT32_MIN <= n <= UINT32_MAX
	if ( !b || 
		 !v ||
		 n < 0 || n > UINT32_MAX ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool 		res = false;
	uint32_t	pos = b->size;
	uint32_t 	size = n > v->size ? v->size : n;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		res = as_bytes_append(b, (uint8_t *) v->value, size);
	}

	lua_pushboolean(l, res);
	return 1;
}

/******************************************************************************
 *	SET FUNCTIONS
 *****************************************************************************/

/**
 *	Set a byte value at specified index.
 *
 *	----------{.c}
 *	bool bytes.set_byte(bytes b, uint32 i, uint8 v)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param i	The index in b to set the value of.
 *	@param v	The uint8_t value to set at i.
 *
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_set_byte(lua_State * l)
{
	// we expect 3 args
	if ( lua_gettop(l) != 3 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer i = luaL_optinteger(l, 2, 0); 
	lua_Integer v = luaL_optinteger(l, 3, 0); 

	// check preconditions:
	// 	- b != NULL 
	//	- 1 <= i <= UINT32_MAX
	//	- 0 <= v <= UINT8_MAX
	if ( !b || 
		 i < 1 || i > UINT32_MAX || 
		 v < 0 || v > UINT8_MAX ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool 		res = false;
	uint32_t	pos = i - 1;
	uint32_t	size = 1;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		uint8_t	val	= htons((uint8_t) v);
		res	= as_bytes_set_byte(b, pos, val);
	}

	lua_pushboolean(l, res);
	return 1;
}

/**
 *	Set an int16_6 value at specified index.
 *
 *	----------{.c}
 *	bool bytes.set_int16(bytes b, uint32 i, int16 v)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param i	The index in b to set the value of.
 *	@param v	The int16_t value to set at i.
 *
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_set_int16(lua_State * l)
{
	// we expect 3 args
	if ( lua_gettop(l) != 3 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer	i = luaL_optinteger(l, 2, 0); 
	lua_Integer	v = luaL_optinteger(l, 3, 0); 

	// check preconditions:
	// 	- b != NULL 
	//	- 1 <= i <= UINT32_MAX
	//	- INT16_MIN <= v <= INT16_MAX
	if ( !b || 
		 i < 1 || i > UINT32_MAX || 
		 v < INT16_MIN || v > INT16_MAX ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool 		res = false;
	uint32_t	pos = i - 1;
	uint32_t	size = 2;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		int16_t	val	= htons((int16_t) v);
		res	= as_bytes_set_int16(b, pos, val);
	}

	lua_pushboolean(l, res);
	return 1;
}

/**
 *	Set an int32_t value at specified index.
 *
 *	----------{.c}
 *	bool bytes.set_int32(bytes b, uint32 i, int32 v)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param i	The index in b to set the value of.
 *	@param v	The int32_t value to set at i.
 *
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_set_int32(lua_State * l)
{
	// we expect 3 args
	if ( lua_gettop(l) != 3 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer	i = luaL_optinteger(l, 2, 0); 
	lua_Integer	v = luaL_optinteger(l, 3, 0);

	// check preconditions:
	// 	- b != NULL 
	//	- 1 <= i <= UINT32_MAX
	//	- INT32_MIN <= v <= INT32_MAX
	if ( !b || 
		 i < 1 || i > UINT32_MAX || 
		 v < INT32_MIN || v > INT32_MAX ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool 		res = false;
	uint32_t	pos = i - 1;
	uint32_t	size = 4;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		int32_t	val	= htonl((int32_t) v);
		res	= as_bytes_set_int32(b, pos, val);
	}

	lua_pushboolean(l, res);
	return 1;
}

/**
 *	Set an int64_t value at specified index.
 *
 *	----------{.c}
 *	bool bytes.set_int64(bytes b, uint32 i, int64 v)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param i	The index in b to set the value of.
 *	@param v	The int64_t value to set at i.
 *
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_set_int64(lua_State * l)
{
	// we expect 3 args
	if ( lua_gettop(l) != 3 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer	i = luaL_optinteger(l, 2, 0); 
	lua_Integer	v = luaL_optinteger(l, 3, 0); 

	// check preconditions:
	// 	- b != NULL 
	//	- 1 <= i <= UINT32_MAX
	//	- INT64_MIN <= v <= INT64_MAX
	if ( !b || 
		i < 1 || i > UINT32_MAX || 
		v < INT64_MIN || v > INT64_MAX ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool 		res = false;
	uint32_t	pos = i - 1;
	uint32_t	size = 8;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		int64_t	val	= be64toh((int64_t) v);
		res = as_bytes_set_int64(b, pos, val);
	}

	lua_pushboolean(l, res);
	return 1;
}

/**
 *	Set an NULL-terminated string value at specified index.
 *
 *	----------{.c}
 *	bool bytes.set_string(bytes b, uint32 i, string v)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param i	The index in b to set the value of.
 *	@param v	The NULL-terminated string value to set at i.
 *
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_set_string(lua_State * l)
{
	// we expect 3 args
	if ( lua_gettop(l) != 3 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 		b = mod_lua_checkbytes(l, 1);
	lua_Integer 	i = luaL_optinteger(l, 2, 0); 
	size_t  		n = 0;
	const char *	v = luaL_optlstring(l, 3, NULL, &n);

	// check preconditions:
	//	- b != NULL
	//	- 1 <= i <= UINT32_MAX 
	//	- v != NULL
	if ( !b || 
		 i < 1 || i > UINT32_MAX || 
		 !v ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool		res = false;
	uint32_t	pos = i - 1;
	uint32_t	size = n;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		res = as_bytes_set(b, pos, (uint8_t *) v, (uint32_t) n);
	}

	lua_pushboolean(l, res);
	return 1;
}

/**
 *	Copy `n` data from bytes `v` to bytes `b` value at specified index.
 *	
 *	----------{.c}
 *	bool bytes.set_bytes(bytes b, uint32 i, bytes v, uint32 n)
 *	----------
 *
 *	@param b 	The bytes to set a value in.
 *	@param i	The index in b to set the value of.
 *	@param v	The NULL-terminated string value to set at i.
 *	@param n	The number of bytes to copy from v in to b.
 *
 *	@return On success, true. Otherwise, false on error.
 */
static int mod_lua_bytes_set_bytes(lua_State * l)
{
	// we expect 3 args
	if ( lua_gettop(l) != 4 ) {
		lua_pushboolean(l, false);
		return 1;
	}

	as_bytes * 	b = mod_lua_checkbytes(l, 1);
	lua_Integer	i = luaL_optinteger(l, 2, 0); 
	as_bytes * 	v = mod_lua_checkbytes(l, 3);
	lua_Integer	n = luaL_optinteger(l, 4, 0); 

	// check preconditions:
	//	- b != NULL
	//	- 1 <= i <= UINT32_MAX 
	//	- v != NULL
	//	- 0 <= n <= UINT32_MAX
	if ( !b || 
		 i < 1 || i > UINT32_MAX || 
		 !v ||
		 n < 0 || n > UINT32_MAX ) {
		lua_pushboolean(l, false);
		return 1;
	}

	bool		res = false;
	uint32_t	pos = i - 1;
	uint32_t 	size = n > v->size ? v->size : n;

	// ensure we have capacity, if not, then resize
	if ( as_bytes_ensure(b, pos + size, true) == true ) {
		// write the bytes
		res = as_bytes_set(b, pos, (uint8_t *) v->value, (uint32_t) n);
	}

	lua_pushboolean(l, res);
	return 1;
}

/******************************************************************************
 *	GET FUNCTIONS
 *****************************************************************************/

/**
 *	Get an uint8_t value from the specified index.
 *	
 *	----------{.c}
 *	uint8 bytes.get_byte(bytes b, uint32 i)
 *	----------
 *	
 *	@param b 	The bytes to get a value from.
 *	@param i	The index in b to get the value of.
 *	
 *	@return On success, the value. Otherwise nil on failure.
 */
static int mod_lua_bytes_get_byte(lua_State * l)
{ 
	// we expect atleast 2 args
	if ( lua_gettop(l) >= 2) {
		return 0;
	}

	as_bytes *	b = mod_lua_checkbytes(l, 1);
	lua_Integer	i = luaL_optinteger(l, 2, 0);

	// check preconditions:
	//	- b != NULL
	//	- 1 <= i <= UINT32_MAX
	if ( !b || 
		 i < 1 || i > UINT32_MAX ) {
		return 0;
	}

	uint32_t pos = i - 1;
	uint8_t  val = 0;

	// get returns 0 on failure
	if ( as_bytes_get_byte(b, pos, &val) == 0 ) {
		return 0;
	}

	uint8_t res = ntohs(val);
	lua_pushinteger(l, res);
	return 1;
}

/**
 *	Get an int16 value from the specified index.
 *	
 *	----------{.c}
 *	int16 bytes.get_int16(bytes b, uint32 i)
 *	----------
 *	
 *	@param b 	The bytes to get a value from.
 *	@param i	The index in b to get the value of.
 *	
 *	@return On success, the value. Otherwise nil on failure.
 */
static int mod_lua_bytes_get_int16(lua_State * l)
{ 
	// we expect atleast 2 args
	if ( lua_gettop(l) >= 2) {
		return 0;
	}

	as_bytes *	b = mod_lua_checkbytes(l, 1);
	lua_Integer	i = luaL_optinteger(l, 2, 0); 

	// check preconditions:
	//	- b != NULL
	//	- 1 <= i <= UINT32_MAX
	if ( !b || 
		 i < 1 || i > UINT32_MAX ) {
		return 0;
	}

	uint32_t pos = i - 1;
	int16_t  val = 0;

	// get returns 0 on failure
	if ( as_bytes_get_int16(b, pos, &val) == 0 ) {
		return 0;
	}

	int16_t res = ntohs(val);
	lua_pushinteger(l, res);
	return 1;
}

/**
 *	Get an int32 value from the specified index.
 *	
 *	----------{.c}
 *	int32 bytes.get_int32(bytes b, uint32 i)
 *	----------
 *	
 *	@param b 	The bytes to get a value from.
 *	@param i	The index in b to get the value of.
 *	
 *	@return On success, the value. Otherwise nil on failure.
 */
static int mod_lua_bytes_get_int32(lua_State * l)
{ 
	// we expect atleast 2 args
	if ( lua_gettop(l) >= 2) {
		return 0;
	}

	as_bytes *	b = mod_lua_checkbytes(l, 1);
	lua_Integer	i = luaL_optinteger(l, 2, 0);

	// check preconditions:
	//	- b != NULL
	//	- 1 <= i <= UINT32_MAX
	if ( !b || 
		 i < 1 || i > UINT32_MAX ) {
		return 0;
	}

	uint32_t pos = i - 1;
	int32_t  val = 0;

	// get returns 0 on failure
	if ( as_bytes_get_int32(b, pos, &val) == 0 ) {
		return 0;
	}

	int32_t res = ntohl(val);
	lua_pushinteger(l, res);
	return 1;
}

/**
 *	Get an int64 value from the specified index.
 *	
 *	----------{.c}
 *	int64 bytes.get_int64(bytes b, uint32 i)
 *	----------
 *	
 *	@param b 	The bytes to get a value from.
 *	@param i	The index in b to get the value of.
 *	
 *	@return On success, the value. Otherwise nil on failure.
 */
static int mod_lua_bytes_get_int64(lua_State * l)
{ 
	// we expect atleast 2 args
	if ( lua_gettop(l) >= 2) {
		return 0;
	}

	as_bytes *	b = mod_lua_checkbytes(l, 1);
	lua_Integer	i = luaL_optinteger(l, 2, 0);

	// check preconditions:
	//	- b != NULL
	//	- 1 <= i <= UINT32_MAX
	if ( !b || 
		 i < 1 || i > UINT32_MAX ) {
		return 0;
	}

	uint32_t pos = i - 1;
	int64_t  val = 0;

	// get returns 0 on failure
	if ( as_bytes_get_int64(b, pos, &val) == 0 ) {
		return 0;
	}

	int64_t res = be64toh(val);
	lua_pushinteger(l, res);
	return 1;
}

/**
 *	Get an bytes value from the specified index.
 *	
 *	----------{.c}
 *	bytes bytes.get_bytes(bytes b, uint32 i, uint32 n)
 *	----------
 *	
 *	@param b 	The bytes to get a value from.
 *	@param i	The index in b to get the value from.
 *	@param n	The the length of the bytes to copy.
 *	
 *	@return On success, the value. Otherwise nil on failure.
 */
static int mod_lua_bytes_get_string(lua_State * l)
{
	// we expect atleast 3 args
	if ( lua_gettop(l) >= 3 ) {
		return 0;
	}

	as_bytes *	b = mod_lua_checkbytes(l, 1);
	lua_Integer	i = luaL_optinteger(l, 2, 0);
	lua_Integer	n = luaL_optinteger(l, 3, 0);

	// check preconditions:
	//	- b != NULL
	//	- 1 <= i <= UINT32_MAX
	//	- 0 <= n <= UINT32_MAX
	if ( !b || 
		 i < 1 || i > UINT32_MAX ||
		 n < 0 || n > UINT32_MAX ) {
		return 0;
	}

	uint32_t pos = i - 1;
	uint32_t len = (uint32_t) n;
	char *   val = (char *) calloc(len + 1, sizeof(char));

	if ( !val ) {
		return 0;
	}

	// copy into the the string
	memcpy(val, b->value + pos, len);
	val[len] = '\0';
	
	lua_pushlstring(l, val, len);
	return 1;
}

/**
 *	Get an bytes value from the specified index.
 *	
 *	----------{.c}
 *	bytes bytes.get_bytes(bytes b, uint32 i, uint32 n)
 *	----------
 *	
 *	@param b 	The bytes to get a value from.
 *	@param i	The index in b to get the value from.
 *	@param n	The the length of the bytes to copy.
 *	
 *	@return On success, the value. Otherwise nil on failure.
 */
static int mod_lua_bytes_get_bytes(lua_State * l)
{
	// we expect atleast 3 args
	if ( lua_gettop(l) >= 3 ) {
		return 0;
	}

	as_bytes *	b = mod_lua_checkbytes(l, 1);
	lua_Integer	i = luaL_optinteger(l, 2, 0);
	lua_Integer	n = luaL_optinteger(l, 3, 0);

	// check preconditions:
	//	- b != NULL
	//	- 1 <= i <= UINT32_MAX
	//	- 0 <= n <= UINT32_MAX
	if ( !b || 
		 i < 1 || i > UINT32_MAX ||
		 n < 0 || n > UINT32_MAX ) {
		return 0;
	}

	uint32_t pos = i - 1;
	uint32_t len = (uint32_t) n;
	uint8_t * raw = (uint8_t *) calloc(len, sizeof(uint8_t));

	if ( !raw ) {
		return 0;
	}

	// copy into the the buffer
	memcpy(raw, b->value + pos, len);
	
	// create a new bytes
	as_bytes * val = as_bytes_new_wrap(raw, len, true);

	if ( !val ) {
		return 0;
	}

	mod_lua_pushbytes(l, val);
	return 1;
}

/******************************************************************************
 * OBJECT TABLE
 *****************************************************************************/

static const luaL_reg bytes_object_table[] = {

	{"size",			mod_lua_bytes_size},
	{"capacity",		mod_lua_bytes_capacity},

	{"set_type",		mod_lua_bytes_set_type},
	{"get_type",		mod_lua_bytes_get_type},

	{"tostring",		mod_lua_bytes_tostring},
	
	{"append_string",	mod_lua_bytes_append_string},
	{"append_bytes",	mod_lua_bytes_append_bytes},
	{"append_byte",		mod_lua_bytes_append_byte},
	{"append_int16",	mod_lua_bytes_append_int16},
	{"append_int32",	mod_lua_bytes_append_int32},
	{"append_int64",	mod_lua_bytes_append_int64},
	
	{"set_string",		mod_lua_bytes_set_string},
	{"set_bytes",		mod_lua_bytes_set_bytes},
	{"set_byte",		mod_lua_bytes_set_byte},
	{"set_int16",		mod_lua_bytes_set_int16},
	{"set_int32",		mod_lua_bytes_set_int32},
	{"set_int64",		mod_lua_bytes_set_int64},
	
	{"put_string",		mod_lua_bytes_set_string},
	{"put_bytes",		mod_lua_bytes_set_bytes},
	{"put_byte",		mod_lua_bytes_set_byte},
	{"put_int16",		mod_lua_bytes_set_int16},
	{"put_int32",		mod_lua_bytes_set_int32},
	{"put_int64",		mod_lua_bytes_set_int64},
	
	{"get_bytes",		mod_lua_bytes_get_bytes},
	{"get_byte",		mod_lua_bytes_get_byte},
	{"get_int16",		mod_lua_bytes_get_int16},
	{"get_int32",		mod_lua_bytes_get_int32},
	{"get_int64",		mod_lua_bytes_get_int64},

	{"ensure",			mod_lua_bytes_ensure},
	{"truncate",		mod_lua_bytes_ensure},
	
	{0, 0}
};

static const luaL_reg bytes_object_metatable[] = {
	{"__call",          mod_lua_bytes_new},
	{0, 0}
};

/******************************************************************************
 * CLASS TABLE
 *****************************************************************************/

static const luaL_reg bytes_class_table[] = {
	{"putX",            mod_lua_bytes_tostring},
	{0, 0}
};

static const luaL_reg bytes_class_metatable[] = {
	{"__index",         mod_lua_bytes_get_byte},
	{"__newindex",      mod_lua_bytes_set_byte},
	{"__len",           mod_lua_bytes_size},
	{"__tostring",      mod_lua_bytes_tostring},
	{"__gc",            mod_lua_bytes_gc},
	{0, 0}
};

/******************************************************************************
 * REGISTER
 *****************************************************************************/

int mod_lua_bytes_register(lua_State * l) {
	mod_lua_reg_object(l, OBJECT_NAME, bytes_object_table, bytes_object_metatable);
	mod_lua_reg_class(l, CLASS_NAME, NULL, bytes_class_metatable);
	return 1;
}
