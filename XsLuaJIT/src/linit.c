/*
** $Id: linit.c,v 1.14.1.1 2007/12/27 13:02:25 roberto Exp $
** Initialization of libraries for lua.c
** See Copyright Notice in lua.h
*/


#define linit_c
#define LUA_LIB

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"


static const luaL_Reg lualibs[] = {
  { "",                 luaopen_base    },
  { LUA_LOADLIBNAME,    luaopen_package },
  { LUA_TABLIBNAME,     luaopen_table   },
  { LUA_IOLIBNAME,      luaopen_io      },
  { LUA_OSLIBNAME,      luaopen_os      },
  { LUA_STRLIBNAME,     luaopen_string  },
  { LUA_MATHLIBNAME,    luaopen_math    },
  { LUA_DBLIBNAME,      luaopen_debug   },
  { LUA_JITLIBNAME,     luaopen_jit     },
#if LUAXS_ADDLIB_MARSHAL
  { LUA_MARSHALLIBNAME, luaopen_marshal },
#endif
#if LUAXS_ADDLIB_BUFFER
  { LUA_BUFFERLIBNAME,  luaopen_buffer  },
#endif
#if LUAXS_ADDLIB_GAME
  { LUA_GAMELIBNAME,    luaopen_game    },
#endif
#if LUAXS_ADDLIB_CONTAINER    
  { LUA_CONTAINERLIBNAME, luaopen_container },
#endif
  { NULL, NULL }
};


LUALIB_API void luaL_openlibs(lua_State *L)
{
    const luaL_Reg *lib = lualibs;

    for (; lib->func; lib++)
    {
        lua_pushcfunction(L, lib->func);
        lua_pushstring(L, lib->name);
        lua_call(L, 1, 0);
    }
}
