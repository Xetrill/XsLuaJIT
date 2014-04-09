/*
The MIT License

Copyright (c) 2007-2010 Aidin Abedi http://code.google.com/p/shinyprofiler/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "ShinyLua.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <string>

#include <direct.h>
#include <ctype.h>

// Note - not in the original STL, but exists in SGI STL and STLport
#if (SHINY_COMPILER == SHINY_COMPILER_GNUC) && !defined(STLPORT)
# include <tr1/unordered_map>
# include <tr1/unordered_set>
#else
# include <hash_set>
# include <hash_map>
#endif

#if SHINY_COMPILER == SHINY_COMPILER_GNUC && __GNUC__ >= 3 && __GNUC_MINOR__ >= 1 && !defined(STLPORT)
# define HASHMAP std::tr1::unordered_map
#else
# if SHINY_COMPILER == SHINY_COMPILER_MSVC
#  if _MSC_VER > 1300 && !defined(_STLP_MSVC)
#   define HASHMAP ::stdext::hash_map
#  else
#   define HASHMAP ::std::hash_map
#  endif
# else
#  define HASHMAP ::std::hash_map
# endif
#endif


/*---------------------------------------------------------------------------*/

struct Profile
{
	ShinyZone      zone;
	ShinyNodeCache cache;
	std::string    name;

	Profile(void)
    {
		ShinyZone_clear(&zone);
		cache = &_ShinyNode_dummy;
	}
};

typedef HASHMAP<const void*, Profile> ProfileMap;


/*---------------------------------------------------------------------------*/

bool g_running = false;

ProfileMap p_profiles;

std::string g_cwd;


/*---------------------------------------------------------------------------*/

std::string ShortenSource(const char* longName)
{
    static const std::string scripts = "\\gamedata\\scripts\\";

    std::string            name = longName;
    std::string::size_type mark = name.find(g_cwd);

    if (mark != std::string::npos)
        name.erase(mark, g_cwd.size());

    mark = name.find(scripts);
    if (mark != std::string::npos)
        name.erase(mark, scripts.size());

    return name;
}


/*---------------------------------------------------------------------------*/

std::string StringPrintV(const char* format, va_list args)
{
	int size = _vscprintf(format, args);
	std::string buffer;
	buffer.resize(size);
	vsnprintf(&buffer[0], size, format, args);
	return buffer;
}


/*---------------------------------------------------------------------------*/

std::string StringPrint(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	std::string str(StringPrintV(format, args));
	va_end(args);
	return str;
}

/*---------------------------------------------------------------------------*/

Profile* FindProfile(lua_State *L, lua_Debug *ar)
{
	const void *func = NULL;

	lua_getinfo(L, "f", ar);
	func = lua_topointer(L, -1);
	lua_pop(L, 1);

	Profile *prof = &p_profiles[func];

	if (!prof->zone.name)
    {
		lua_getinfo(L, "S", ar);

		switch (ar->what[0])
        {
		case 'L': // "Lua"
        {
			prof->name = StringPrint(
                "%s(%d):%s",
                ShortenSource(ar->source).c_str(),
                ar->linedefined,
                ar->name
            );
			break;
        }
		case 'C': // "C"
			prof->name  = "C:";
			prof->name += ar->name;
			break;
		default: // the impossible happened
			prof->name = "<unknown>";
		}

		prof->zone.name = prof->name.c_str();
	}

	return prof;
}


/*---------------------------------------------------------------------------*/

void callhook(lua_State *L, lua_Debug *ar)
{
	// ignore tail call
	if (ar->i_ci == 0 || ar->event == LUA_HOOKTAILRET)
        return;

	// ignore nameless function
	lua_getinfo(L, "n", ar);
	if (!ar->name)
        return;

	if (ar->event == LUA_HOOKCALL)
    {
		Profile *prof = FindProfile(L, ar);
		ShinyManager_lookupAndBeginNode(&Shiny_instance, &prof->cache, &prof->zone);
	}
    else // LUA_HOOKRET
    {
		ShinyManager_endCurNode(&Shiny_instance);
	}
}


/*---------------------------------------------------------------------------*/

int ShinyLua_update(lua_State *L)
{
	PROFILE_UPDATE();
	return 0;
}


/*---------------------------------------------------------------------------*/

int ShinyLua_clear(lua_State *L)
{
	PROFILE_CLEAR();
	return 0;
}


/*---------------------------------------------------------------------------*/

int ShinyLua_damping(lua_State *L)
{
	if (lua_gettop(L) == 1)
    {
        float num = STATIC_CAST(float, luaL_checknumber(L, 1));
        num = (num < 0.0f) ? 0.0f : num;
        num = (num > 1.0f) ? 1.0f : num;
		PROFILE_SET_DAMPING(num);
		return 0;
	}
    else
    {
		lua_pushnumber(L, PROFILE_GET_DAMPING());
		return 1;
	}
}


/*---------------------------------------------------------------------------*/

int ShinyLua_start(lua_State *L)
{
	if (!g_running)
    {
        if (!luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF))
            lxs_error(L, "Failed to disable jitting");

	    lua_sethook(L, callhook, LUA_MASKCALL | LUA_MASKRET, 0);
        g_running = true;
    }
    lua_pushboolean(L, 1);
    return 1;
}


/*---------------------------------------------------------------------------*/

int ShinyLua_stop(lua_State *L)
{
	if (g_running)
    {
	    lua_sethook(L, callhook, 0, 0);
        g_running = false;

        if (!luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON))
            lxs_error(L, "Failed to enable jitting.");
    }
    lua_pushboolean(L, 1);
    return 1;
}

/*---------------------------------------------------------------------------*/

int ShinyLua_is_running(lua_State *L)
{
	lua_pushboolean(L, g_running);
	return 1;
}


/*---------------------------------------------------------------------------*/

int ShinyLua_output(lua_State *L)
{
    switch (lua_type(L, 1))
    {
    case LUA_TNONE:
        PROFILE_OUTPUT(NULL);
        break;
    case LUA_TSTRING:
        PROFILE_OUTPUT(luaL_checkstring(L, 1));
        break;
    case LUA_TUSERDATA:
    {
        FILE* fs = lxs_checkfilep(L, 1);
        if (fs == NULL)
            luaL_argerror(L, 1, "attempt to use a closed file");
        PROFILE_OUTPUT_STREAM(fs);
        break;
    }
    default:
        luaL_argerror(L, 1, "string or userdata (" LUA_FILEHANDLE ")");
    }

	return 0;
}


/*---------------------------------------------------------------------------*/

int ShinyLua_tree_string(lua_State *L)
{
    std::string output = PROFILE_GET_TREE_STRING();
	lua_pushlstring(L, output.c_str(), output.length());
	return 1;
}


/*---------------------------------------------------------------------------*/

int ShinyLua_flat_string(lua_State *L)
{
    std::string output = PROFILE_GET_FLAT_STRING();
    lua_pushlstring(L, output.c_str(), output.length());
	return 1;
}


/*---------------------------------------------------------------------------*/

void init(void)
{
    char buffer[260];
    _getdcwd_nolock(_getdrive(), buffer, 260);

    char* c = buffer;
    for (; *c; c++)
        *c = tolower(*c);

    g_cwd = buffer;
}


/*---------------------------------------------------------------------------*/

#define SETFIELD(L, name, value)                                               \
    lua_pushliteral(L, "" value);                                              \
    lxs_rawset(L, 1, name)

int luaopen_shiny(lua_State *L)
{
    init();

	const luaL_Reg funcs[] = {
		{ "update",      ShinyLua_update      },
		{ "clear",       ShinyLua_clear       },
		{ "output",      ShinyLua_output      },
		{ "damping",     ShinyLua_damping     },
		{ "start",       ShinyLua_start       },
		{ "stop",        ShinyLua_stop        },
		{ "is_running",  ShinyLua_is_running  },
		{ "tree_string", ShinyLua_tree_string },
		{ "flat_string", ShinyLua_flat_string },
		{ NULL, NULL }
	};
    luaL_register(L, SHINY_LUA_LIBNAME, funcs);

    lua_remove(L, 1);
    SETFIELD(L, "_NAME",        SHINY_FULLNAME);
    SETFIELD(L, "_VERSION",     SHINY_VERSION);
    SETFIELD(L, "_DESCRIPTION", SHINY_DESCRIPTION);
    SETFIELD(L, "_COPYRIGHT",   SHINY_COPYRIGHT);
	return 1;
}

#undef SETFIELD
