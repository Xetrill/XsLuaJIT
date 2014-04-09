/*
** A library for string buffers in Lua.
** Copyright (c) 2013 Choonster
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
**
** Major portions taken verbatim or adapted from Lua 5.2's auxilary library.
** Copyright (C) 1994-2013 Lua.org, PUC-Rio. See Copyright Notice at the page below:
**     http://www.lua.org/license.html
*/

/**
A library for string buffers in Lua.

The buffer code in this library is largely adapted from Lua 5.2's luaL\_Buffer code.
The main difference is that Buffers store their contents in the registry instead of the stack when their length exceeds LUAL\_BUFFERSIZE.
This prevents the C char array holding the current contents from being garbage collected until a larger array is required or the BL_Buffer is reset or garbage collected.
You don't need to know any of this to use the library, it's just extra information for people curious about the implementation.


Just like regular strings in Lua, string buffers can contain embedded nulls (\0).


Similar to Lua's string library, most BL_Buffer methods can be called as `buff:method(...)` or `bufflib.method(buff, ...)` (where `buff` is a BL_Buffer).
Note that not all methods use the same name in the BL_Buffer metatable and the `bufflib` table.
The primary examples of this are the metamethods, which use the required metamethod names in the metatable and more descriptive names in the `bufflib` table (e.g. the `__len` metamethod is the same as `bufflib.length`).


In addition to the functions shown here, you can call any method from the global `string` table (not just functions from the string library) on a BL_Buffer (either as a method or a function from the `bufflib` table) by prefixing the name with `s_`.
When you call a BL_Buffer method with the `s_` prefix, it calls the equivalent `string` function with the BL_Buffer's contents as the first argument followed by any other arguments supplied to the method. None of these methods modify the original BL_Buffer.

For example, `bufflib.s_gsub(buff, ...)` and `buff:s_gsub(...)` are both equivalent to `str:gsub(...)` (where `buff` is a BL_Buffer and `str` is the BL_Buffer's contents as a string).

Buffers define metamethods for equality (==), length (#), concatenation (..) and tostring(). See the documentation of each metamethod for details.

@module bufflib
*/

#ifndef XS_BUFFLIB_DISABLE
#include <string.h>

#include "lua_bufflib.h"

/* Add s to the BL_Buffer's character count */
#define addsize(B,s)       ((B)->length += (s))

/* If the BL_Buffer is storing its contents as a userdata in the registry, unreference it and allow it to be collected */
#define buff_unref(L, B) if ((B)->ref != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, (B)->ref)

/*
	Prepares the BL_Buffer for a string of length sz to be added and returns a pointer that the string can be copied into with memcpy().
	If there isn't enough space in the current char array, creates a new one as a userdata and stores it in the registry.
	Releases the previously stored userdata (if any), allowing it be garbage collected.
*/
static char *prepbuffsize (BL_Buffer *B, size_t sz) {
	lua_State *L = B->L;
	if (B->size - B->length < sz) {  /* not enough space? */
		char *newbuff;
		size_t newsize = B->size * 2;  /* double buffer size */
		if (newsize - B->length < sz)  /* not big enough? */
			newsize = B->length + sz;
		if (newsize < B->length || newsize - B->length < sz)
			luaL_error(L, "buffer too large");
		/* create larger buffer */
		newbuff = (char *)lua_newuserdata(L, newsize * sizeof(char));
		/* move content to new buffer */
		memcpy(newbuff, B->b, B->length * sizeof(char));
		buff_unref(L, B);  /* remove old buffer */
		B->ref = luaL_ref(L, LUA_REGISTRYINDEX);
		B->b = newbuff;
		B->size = newsize;
	}
	return &B->b[B->length];
}

/*
	Initialize a BL_Buffer for use with the given lua_State.
	The BL_Buffer must only be used with the lua_State it was initialised with.
*/
static void buffinit(lua_State *L, BL_Buffer *B) {
	B->L = L;
	B->b = B->initb;
	B->length = 0;
	B->size = LUAL_BUFFERSIZE;
	B->ref = LUA_NOREF;
}

/*
	Creates a new BL_Buffer as a userdata, sets its metatable and initialises it.
	The new BL_Buffer will be left on the top of the stack.
*/
static BL_Buffer *newbuffer(lua_State *L) {
	BL_Buffer *B = (BL_Buffer *)lua_newuserdata(L, sizeof(BL_Buffer));
	luaL_setmetatable(L, BUFFERTYPE);
	buffinit(L, B);
	return B;
}

/* Returns a pointer to the BL_Buffer at index i */
#define getbuffer(L, i) ((BL_Buffer *)luaL_checkudata(L, i, BUFFERTYPE))

/* Is the value at index i a BL_Buffer? */
#define isbuffer(L, i) (luaL_testudata(L, i, BUFFERTYPE) != NULL)

/* Call luaL_toslstring and pop the string from the stack */
#define tolstring(L, i, l) luaL_tolstring(L, i, l); lua_pop(L, 1)

/* Add string arguments to the buffer, starting from firstarg and ending at (numargs-offset). */
static void addstrings(BL_Buffer *B, int firstarg, int offset) {
	lua_State *L = B->L;
	int numargs = lua_gettop(L) - offset;

	size_t len;
	const char *str;
	char *b;

	int i;
	for (i = firstarg; i <= numargs; i++) {
		len = -1;
		str = tolstring(L, i, &len);
		b = prepbuffsize(B, len);
		memcpy(b, str, len * sizeof(char));
		addsize(B, len);
	}
}

/*
	Add strings to the buffer, with a separator string between each one.
	Assumes the separator is argument 2 and arguments 3 to numargs are the strings to be added.
*/
static void addsepstrings(BL_Buffer *B) {
	lua_State *L = B->L;
	int numargs = lua_gettop(L);

	size_t seplen = -1;
	const char *sep;

	size_t len = -1;
	const char *str;
	char *b;

	int i;

	sep = tolstring(L, 2, &seplen); /* Get the separator string */

	for (i = 3; i < numargs; i++) { /* For arguments 3 to (numargs-1), add the string argument followed by the separator */
		len = -1;
		str = tolstring(L, i, &len);

		b = prepbuffsize(B, len + seplen); /* Prepare the buffer for the length of the string plus the length of the separator */
		memcpy(b, str, len * sizeof(char));
		addsize(B, len);

		b += len; /* Get a pointer to the position after the string */
		memcpy(b, sep, seplen * sizeof(char));
		addsize(B, seplen);
	}

	len = -1;
	str = tolstring(L, numargs, &len); /* Add the final string */
	b = prepbuffsize(B, len);
	memcpy(b, str, len * sizeof(char));
	addsize(B, len);
}

/* Pushes the argument at index i (which should be a BL_Buffer userdata) onto the stack to return it. */
static int pushbuffer(lua_State *L, int i) {
	lua_pushvalue(L, i);
	return 1;
}

/**
Add some strings to the @{BL_Buffer}.
All non-string arguments are converted to strings following the same rules as the `tostring()` function.

@function add
@param ... Some values to add to the BL_Buffer.
@return BL_Buffer The BL_Buffer object.
*/
static int bufflib_add(lua_State *L) {
	BL_Buffer *B = getbuffer(L, 1);
	addstrings(B, 2, 0);
	return pushbuffer(L, 1);
}

/**
Add some strings to the @{BL_Buffer}, each separated by the specified separator string.
All non-string arguments are converted to strings following the same rules as the `tostring()` function.

@function addsep
@string sep The string to use as a separator.
@param ... Some values to add to the BL_Buffer.
@return BL_Buffer The BL_Buffer object.
*/
static int bufflib_addsep(lua_State *L) {
	BL_Buffer *B = getbuffer(L, 1);
	addsepstrings(B);
	return pushbuffer(L, 1);
}

/**
Reset the @{BL_Buffer} to its initial (empty) state.
If the BL_Buffer was storing its contents in the registry, this is removed so it can be garbage collected.

@function reset
@return BL_Buffer The BL_Buffer object.
*/
static int bufflib_reset(lua_State *L) {
	BL_Buffer *B = getbuffer(L, 1);
	buff_unref(L, B); /* If the buffer is storing its contents in the registry, remove it (and allow it to be garbage collected) before resetting */
	buffinit(L, B); /* Re-initialise the BL_Buffer */
	return pushbuffer(L, 1);
}

/**
Converts the @{BL_Buffer} to a string representing its current conents.
The BL_Buffer remains unchanged.

@function __tostring
@return string The contents of the BL_Buffer
*/
static int bufflib_tostring(lua_State *L) {
	BL_Buffer *B = getbuffer(L, 1);
	lua_pushlstring(L, B->b, B->length);
	return 1;
}

/**
Metamethod for the `#` (length) operation.
Returns the length of the @{BL_Buffer}'s current contents.

@function __len
@return int length
*/
static int bufflib_len(lua_State *L) {
	BL_Buffer *B = getbuffer(L, 1);
	lua_pushinteger(L, B->length);
	return 1;
}

/**
Metamethod for the `..` (concatenation) operation.
If both arguments are @{BL_Buffer|Buffers}, creates a new BL_Buffer from their joined contents.
Otherwise adds a value to the BL_Buffer, converting it to a string following the same rules as the `tostring()` function.

@function __concat
@param B A BL_Buffer to concatenate with this one or some value to to add to it.
@return BL_Buffer Either the new BL_Buffer created from the two BL_Buffer arguments or the BL_Buffer that the non-BL_Buffer argument was added to.
*/
static int bufflib_concat(lua_State *L) {
	int arg1IsBuffer = isbuffer(L, 1);
	int arg2IsBuffer = isbuffer(L, 2);

	if (arg1IsBuffer && arg1IsBuffer) { /* If both arguments are Buffers, combine them into a new BL_Buffer */
		BL_Buffer *destbuff = newbuffer(L);

		size_t len1 = -1, len2 = -1;
		const char *str1, *str2;
		char *b;

		str1 = tolstring(L, 1, &len1);
		str2 = tolstring(L, 2, &len2);

		b = prepbuffsize(destbuff, len1 + len2); /* Prepare the BL_Buffer for the combined length of the strings */
		memcpy(b, str1, len1 * sizeof(char)); /* Add the first string */
		addsize(destbuff, len1);

		b += len1; /* Get a pointer to the position after the first string */
		memcpy(b, str2, len2 * sizeof(char)); /* Add the second string */
		addsize(destbuff, len2);

		return 1; /* The new BL_Buffer is already on the stack */
	} else { /* If only one argument is a BL_Buffer, add the non-BL_Buffer argument to it */
		size_t len = -1;
		const char *str;
		char *b;
		int buffindex;
		BL_Buffer *destbuff;

		if (arg1IsBuffer) {
			destbuff = getbuffer(L, 1);
			str = tolstring(L, 2, &len);
			buffindex = 1;
		} else {
			destbuff = getbuffer(L, 2);
			str = tolstring(L, 1, &len);
			buffindex = 2;
		}

		b = prepbuffsize(destbuff, len);
		memcpy(b, str, len);
		addsize(destbuff, len);

		return pushbuffer(L, buffindex);
	}
}

/**
Metamethod for the `==` (equality) operation.
Returns true if the @{BL_Buffer|Buffers} hold the same contents, false if they don't.
The contents of each BL_Buffer are converted to Lua strings and then the two strings are compared to obtain the result.

@function __eq
@param BL_Buffer buff2 The BL_Buffer to compare with this one.
@return bool equal Do the Buffers hold the same contents?
*/
static int bufflib_equal(lua_State *L){
	BL_Buffer *buff1 = getbuffer(L, 1);
	BL_Buffer *buff2 = getbuffer(L, 2);

    if (buff1->length != buff2->length)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

	lua_pushlstring(L, buff1->b, buff1->length); /* Push the contents of each BL_Buffer onto the stack */
	lua_pushlstring(L, buff2->b, buff2->length);

	lua_pushboolean(L, lua_rawequal(L, -1, -2)); /* Return the result of a lua_rawequal comparison of the strings */
	return 1;
}

/*
Garbage collection metamethod. This should never be called by the user, so it's not included in the documentation.
If the BL_Buffer is storing its contents as a userdata in the registry, unreferences it; allowing it to be collected
*/
static int bufflib_gc(lua_State *L){
	BL_Buffer *B = getbuffer(L, 1);
	buff_unref(L, B);
	return 0;
}

/**
BL_Buffer Manipulation.

The functions in this section are available in the `bufflib` table, returned from `require"bufflib"`.

@section manipulation
*/

/**
Creates a new BL_Buffer object, optionally adding some strings to the new BL_Buffer.
All non-string arguments are converted to strings following the same rules as the `tostring()` function.

@function newbuffer
@param[opt] ... Some values to add to the new BL_Buffer.
@return BL_Buffer The new BL_Buffer object.
*/
static int bufflib_newbuffer(lua_State *L){
	BL_Buffer *B = newbuffer(L);
	addstrings(B, 1, 1); /* Pass addstrings an offset of 1 to account for the new userdata at the top of the stack. */
	return 1; /* The new BL_Buffer is already on the stack */
}

/**
Tests whether or not the argument is a @{BL_Buffer}.

@function isbuffer
@param arg The value to check.
@return bool Is this a BL_Buffer?
*/
static int bufflib_isbuffer(lua_State *L){
	lua_pushboolean(L, isbuffer(L, 1));
	return 1;
}

/*
	Calls a function stored at upvalue 1 with the BL_Buffer's string contents as the first argument and any other arguments passed to the function after that.
*/
static int bufflib_stringop(lua_State *L){
	BL_Buffer *B = getbuffer(L, 1);
	int numargs = lua_gettop(L);

	lua_remove(L, 1); /* Remove the BL_Buffer from the stack */

	lua_pushvalue(L, lua_upvalueindex(1)); /* String function to call */
	lua_insert(L, 1); /* Insert it before the arguments */

	lua_pushlstring(L, B->b, B->length); /* BL_Buffer contents as a string */
	lua_insert(L, 2); /* Insert it between the function and the arguments */

	lua_call(L, numargs, LUA_MULTRET); /* Call the function */
	return lua_gettop(L); /* The function's return values are the only values on the stack, we return them all from this function */
}

/*
	Function for the __index metamethod.
	First it looks up the key in the BL_Buffer metatable and returns its value if it has one.
	If the key doesn't exist in the metatable and it starts with "s_", looks up the rest of the key in the global table "string" (if it exists).
	If the key's value is a function, creates a closure of the stringop function with the string function as the first upvalue.
	This closure is stored with the original key in both the metatable and the library table and then returned.
	Returns nil if the above conditions aren't met.
*/
static int bufflib_index(lua_State *L){
	const char *key = luaL_checkstring(L, 2);
	const char *pos, *strkey;

	lua_getmetatable(L, 1); /* Get the BL_Buffer's metatable */
	lua_getfield(L, -1, key); /* mt[key] */

	if (!lua_isnil(L, -1)){ /* If the key exists in the metatable, return its value */
		return 1;
	} else {
		lua_pop(L, 1); /* Pop the nil from the stack */
	}

	pos = strstr(key, STRINGPREFIX);
	if (pos == NULL || (pos - key) != 0){ /* If the key doesn't start with "s_", return nil */
		lua_pushnil(L);
		return 1;
	}

	lua_getglobal(L, "string");
	if (!lua_istable(L, -1)){ /* If there's no string table, return nil */
		lua_pushnil(L);
		return 1;
	}

	strkey = key + STRINGPREFIXLEN; /* Move past the prefix */
	lua_getfield(L, -1, strkey); /* _G.string[strkey] */
	lua_remove(L, -2); /* Remove the string table */

	if (!lua_isfunction(L, -1)){ /* If there's no function at strkey, return nil */
		lua_pushnil(L);
		return 1;
	}

	lua_pushcclosure(L, bufflib_stringop, 1); /* Push the stringop function as a closure with the string function as the first upvalue */

	lua_pushvalue(L, -1); /* Push a copy of the closure */
	lua_setfield(L, 3, key); /* mt[key] = closure (Pops the first copied closure) */

	lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
	lua_getfield(L, -1, "bufflib");

	lua_pushvalue(L, -3); /* Push a second copy of the closure */
	lua_setfield(L, -2, key); /* bufflib[key] = closure (Pops the second copied closure) */
	lua_pop(L, 2); /* Pop the _LOADED and bufflib tables */

	return 1; /* Return the original closure */
}

/**
Equivalent to @{BL_Buffer:add|`buff:add(...)`}.
@function add
@param BL_Buffer buff The BL_Buffer to add the values to.
@param ... Some values to add to the BL_Buffer.
@return BL_Buffer The BL_Buffer object.
*/

/**
Equivalent to @{BL_Buffer:addsep|`buff:addsep(sep, ...)`}.
@function addsep
@param BL_Buffer buff The BL_Buffer to add the values to.
@string sep The string to use as a separator.
@param ... Some values to add to the BL_Buffer.
@return BL_Buffer The BL_Buffer object.
*/

/**
Equivalent to @{BL_Buffer:reset|`buff:reset()`}.
@function reset
@param BL_Buffer buff The BL_Buffer to reset.
@return BL_Buffer The emptied BL_Buffer object.
*/

/**
Equivalent to `tostring(buff)` or @{BL_Buffer:__tostring|`buff:__tostring()`}.
@function tostring
@param BL_Buffer buff The BL_Buffer to convert to a string.
@return string The BL_Buffer's contents as a string.
*/

/**
Equivalent to `#buff` or @{BL_Buffer:__len|`buff:__len()`}.
@function length
@param BL_Buffer buff The BL_Buffer to get the length of.
@return int length
*/

/**
Equivalent to `buff1 .. buff2` or @{BL_Buffer:__concat|`buff1:__concat(buff2)`}.
@function concat
@param buff1 A BL_Buffer or some other value.
@param buff2 A BL_Buffer or some other value.
@return BL_Buffer Either the new BL_Buffer created from the BL_Buffer arguments or the BL_Buffer that the non-BL_Buffer argument was added to.
*/

/**
Equivalent to `buff1 == buff2` or @{BL_Buffer:__eq|`buff1:__eq(buff2)`}.
@function equal
@param BL_Buffer buff1 A BL_Buffer
@param BL_Buffer buff2 Another BL_Buffer
@return bool Do the Buffers hold the same contents?
*/

/**
The initial length of the char array used by @{BL_Buffer|Buffers}.
This is set by the value of the LUAL_BUFFERSIZE macro defined in luaconf.h.
This field is not actually used by lua_bufflib, so changing it won't affect the length of new Buffers.
@int buffersize
*/

static struct luaL_Reg metareg[] = {
	{"__concat", bufflib_concat},
	{"__eq", bufflib_equal},
	{"__len", bufflib_len},
	{"__tostring", bufflib_tostring},
	{"__gc", bufflib_gc},
	{"add", bufflib_add},
	{"addsep", bufflib_addsep},
	{"reset", bufflib_reset},
	{NULL, NULL}
};

static struct luaL_Reg libreg[] = {
	{"add", bufflib_add},
	{"addsep", bufflib_addsep},
	{"equal", bufflib_equal},
	{"concat", bufflib_concat},
	{"length", bufflib_len},
	{"new", bufflib_newbuffer},
	{"reset", bufflib_reset},
	{"tostring", bufflib_tostring},
	{"isbuffer", bufflib_isbuffer},
	{NULL, NULL}
};

int luaopen_bufflib(lua_State *L) {
	int strIndex, metaIndex, libIndex;

	luaL_newmetatable(L, BUFFERTYPE); /* Create the metatable */
	luaL_setfuncs(L, metareg, 0);
	lua_pushcfunction(L, bufflib_index);
	lua_setfield(L, -2, "__index"); /* mt.__index = bufflib_index */

	luaL_newlib(L, libreg); /* Create the library table */
	lua_pushinteger(L, LUAL_BUFFERSIZE);
	lua_setfield(L, -2, "buffersize");

	lua_getglobal(L, "string");
	if (lua_isnil(L, -1)){ /* If there's no string table, return now */
		lua_pop(L, 1);
		return 1;
	}

	strIndex = lua_gettop(L); /* lua_next probably won't like relative (negative) indices, so record the absolute index of the string table */
	metaIndex = strIndex - 2; /* Record the absolute values of the metatable and library table so we don't have to deal with relative indices in the loop */
	libIndex = strIndex - 1;

	lua_pushnil(L);
	while (lua_next(L, strIndex) != 0){
		const char *newkey;

		if (lua_type(L, -2) != LUA_TSTRING || !lua_isfunction(L, -1)){ /* If the key isn't a string or the value isn't a function, pop the value and jump to the next pair */
			lua_pop(L, 1);
			continue;
		}

		lua_pushliteral(L, STRINGPREFIX); /* Push the prefix string */
		lua_pushvalue(L, -3); /* Push a copy of the key */
		lua_concat(L, 2); /* prefix .. key (pops the two strings)*/
		newkey = lua_tostring(L, -1);
		lua_pop(L, 1); /* Pop the new string */

		lua_pushcclosure(L, bufflib_stringop, 1); /* Push the stringop function as a closure with the string function as the first upvalue */
		lua_pushvalue(L, -1); /* Push a copy of the closure */

		lua_setfield(L, metaIndex, newkey); /* Pops the copied closure */
		lua_setfield(L, libIndex, newkey); /* Pops the original closure, the original key is left on the top of the stack */
	}

	lua_pop(L, 1); /* Pop the string table */

	return 1; /* Return the library table */
}

#endif
