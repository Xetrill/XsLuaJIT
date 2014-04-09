#ifndef LUA_BUFFLIB_H
#define LUA_BUFFLIB_H
#ifndef XS_BUFFLIB_DISABLE

#include "lua.h"
#include "lauxlib.h"

// #define BUFFLIB_API LUALIB_API
#define BUFFLIB_API

/* The registry key used to store the Buffer metatable */
#define BUFFERTYPE "bufflib_buffer"

/* The prefix used to access string library methods on Buffers */
#define STRINGPREFIX "s_"
#define STRINGPREFIXLEN 2

// 2013-08-21 12:23: ~Xetrill: I like my name better. 'mstr[ing]' would be okay too (mutable string).
// #define LIBNAME "bufflib"
#define LIBNAME "buffer"

/**
A class representing a string buffer.
@type Buffer
*/
typedef struct BL_Buffer {
    int ref; /* position of the buffer's userdata in the registry */
    char *b;  /* buffer address */
    size_t size;  /* buffer size */
    size_t length;  /* number of characters in buffer */
    lua_State *L;
    char initb[LUAL_BUFFERSIZE];  /* initial buffer */
} BL_Buffer;

BUFFLIB_API int bufflib_add(lua_State *L);
BUFFLIB_API int bufflib_addsep(lua_State *L);
BUFFLIB_API int bufflib_reset(lua_State *L);
BUFFLIB_API int bufflib_tostring(lua_State *L);
BUFFLIB_API int bufflib_len(lua_State *L);
BUFFLIB_API int bufflib_concat(lua_State *L);
BUFFLIB_API int bufflib_equal(lua_State *L);
BUFFLIB_API int bufflib_gc(lua_State *L);
BUFFLIB_API int bufflib_newbuffer(lua_State *L);
BUFFLIB_API int bufflib_isbuffer(lua_State *L);
BUFFLIB_API int bufflib_stringop(lua_State *L);
BUFFLIB_API int bufflib_index(lua_State *L);

LUA_API int luaopen_bufflib(lua_State *L);

#endif
#endif
