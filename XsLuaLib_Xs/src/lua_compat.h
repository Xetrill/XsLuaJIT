#pragma once
#ifndef XSLUALIB_XS_LUA_COMPAT_H
#define XSLUALIB_XS_LUA_COMPAT_H

#if LUA_VERSION_NUM < 502
#  define luaL_setfuncs(L,l,nups)      luaI_openlib((L),NULL,(l),(nups))
#  define luaL_newlibtable(L,l)	       lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)
#  define luaL_newlib(L,l)             luaL_newlibtable(L,l), luaL_setfuncs(L,l,0)
#  define luaL_typeerror(L,narg,tname) luaL_typerror(L,narg,tname)
#endif // LUA_VERSION_NUM < 502

#endif // XSLUALIB_XS_LUA_COMPAT_H