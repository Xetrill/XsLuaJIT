/*
** $Id: lualib.h,v 1.36 2005/12/27 17:12:00 roberto Exp $
** Lua standard libraries
** See Copyright Notice in lua.h
*/


#ifndef lualib_h
#define lualib_h

#include "lua.h"


/* Key to file-handle type */
#define LUA_FILEHANDLE		"FILE*"


#define LUA_COLIBNAME     	"coroutine"
#define LUA_TABLIBNAME	     "table"
#define LUA_IOLIBNAME	     "io"
#define LUA_OSLIBNAME	     "os"
#define LUA_STRLIBNAME	     "string"
#define LUA_MATHLIBNAME	     "math"
#define LUA_DBLIBNAME	     "debug"
#define LUA_LOADLIBNAME	     "package"
#define LUA_JITLIBNAME       "jit"
#define LUA_MARSHALLIBNAME   "marshal"
#define LUA_BUFFERLIBNAME    "buffer"
#define LUA_GAMELIBNAME      "game"
#define LUA_CONTAINERLIBNAME "container"
#define LUA_MEMORYLIBNAME    "memory"


LUALIB_API int (luaopen_base) (lua_State *L);
LUALIB_API int (luaopen_table) (lua_State *L);
LUALIB_API int (luaopen_io) (lua_State *L);
LUALIB_API int (luaopen_os) (lua_State *L);
LUALIB_API int (luaopen_string) (lua_State *L);
LUALIB_API int (luaopen_math) (lua_State *L);
LUALIB_API int (luaopen_debug) (lua_State *L);
LUALIB_API int (luaopen_package) (lua_State *L);
LUALIB_API int (luaopen_jit) (lua_State *L);

#if LUAXS_ADDLIB_MARSHAL
LUALIB_API int (luaopen_marshal) (lua_State *L);
#endif
#if LUAXS_ADDLIB_BUFFER
LUALIB_API int (luaopen_buffer) (lua_State *L);
#endif
#if LUAXS_ADDLIB_GAME
LUALIB_API int (luaopen_game) (lua_State* L);
#endif
#if LUAXS_ADDLIB_CONTAINER
LUALIB_API int (luaopen_container) (lua_State* L);
#endif
#if LUAXS_ADDLIB_MEMORY
LUALIB_API int (luaopen_memory) (lua_State* L);
#endif


/* open all previous libraries */
LUALIB_API void (luaL_openlibs) (lua_State *L);


#ifndef lua_assert
#define lua_assert(x)	((void)0)
#endif

#endif // lualib_h
