#define lxs_api_cpp 1

#include "lxs_api.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lobject.h"

#include <stdio.h>
#include <string.h>
}; // extern "C"

#include "leastl.hpp"

#include <eastl/bitset.h>
#include <eastl/utility.h>
#include <eastl/hash_map.h>







extern "C" {


FILE* lxs_checkfilep(lua_State* const L, int idx)
{
    FILE** fp = (FILE**)luaL_checkudata(L, idx, LUA_FILEHANDLE);
    return (fp != NULL)
        ? *fp
        : NULL;
}






/// @see http://stackoverflow.com/a/5506223/105211
int lxs_split(lua_State* const L,
              const char* src,
              size_t len,
              const char* seps)
{
    lxs_assert(L, L);
    lxs_assert(L, src);
    lxs_assert(L, strlen(src) <= len);
    lxs_assert(L, seps);
    lxs_assert_stack_begin(L);

    lua_createtable(L, 1, 0);

    if (len == 0)
    {
        lua_pushliteral(L, "");
        lua_rawseti(L, -2, 1);
        lxs_assert_stack_end(L, 1);
        return 1;
    }

    eastl::bitset<255> seps_lookup;
    char sep; // to ensure only a single pointer-deref per separator
    while ((sep = *seps))
    {
        seps_lookup[static_cast<unsigned char>(sep)] = true;
        seps++;
    }

    const char* front = src;
    const char* end   = src + len;

    int i = 0;
    bool in_token = false;
    for (const char* it = front; it != end; ++it)
    {
        if (seps_lookup[static_cast<unsigned char>(*it)])
        {
            if (in_token)
            {
                lua_pushlstring(L, front, it - front);
                lua_rawseti(L, -2, ++i);

                in_token = false;
            }
            else
            {
                lua_pushliteral(L, "");
                lua_rawseti(L, -2, ++i);
            }
        }
        else if (!in_token)
        {
            front = it;
            in_token = true;
        }
    }
    if (in_token)
    {
        lua_pushlstring(L, front, end - front);
        lua_rawseti(L, -2, ++i);
    }
    else
    {
        lua_pushliteral(L, "");
        lua_rawseti(L, -2, ++i);
    }

    lxs_assert_stack_end(L, 1);
    return 1;
}

size_t lxs_countfuncs(const luaL_Reg* funcs)
{
    if (!funcs)
        return 0;

    size_t num = 0;
    for (; funcs->name; ++funcs)
    {
        ++num;
    }
    return num;
}

size_t lxs_counttable(lua_State* const L, int narg)
{
    lxs_assert_stack_begin(L);

    luaL_checktype(L, narg, LUA_TTABLE);

    size_t num = 0;

    lua_pushnil(L);
    while (lua_next(L, narg))
    {
        ++num;

        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lxs_assert_stack_end(L, 0);
    return num;
}

void lxs_pushfuncs(lua_State* const L, const luaL_Reg* funcs)
{
    lxs_assert_stack_begin(L);

    luaL_checktype(L, 1, LUA_TTABLE);

    for (; funcs->name; ++funcs)
    {
        lua_pushcclosure(L, funcs->func, 0);
        lua_pushstring(L, funcs->name);
        lua_rawsetr(L, 1);
    }

    lxs_assert_stack_end(L, 0);
}






}; // extern "C"






#if LUAXS_CORE_TRACEBACK_EX

static lxs::stdlib_map stdlib;

lxs::stdfun_map& lxs::reg_lib(lua_State* const L, const char* name, int size)
{
    lxs_assert(L, L);
    lxs_assert(L, name);
    lxs_assert(L, size > 0);

    lxs::stdlib_map::insert_return_type res = stdlib.insert(name);
    return (*res.first).second;
}

void lxs::reg_fun(lua_State* const L, stdfun_map& lib, const char* name, lua_CFunction func)
{
    lxs_assert(L, L);
    lxs_assert(L, name);
    lxs_assert(L, func);

    lib.insert(eastl::make_pair(func, name));
}

bool lxs::reg_find_name(lua_State* const L, lua_CFunction fun, stdfun_name& out)
{
    lxs_assert(L, L);
    lxs_assert(L, fun);

    for (stdlib_map::const_iterator it = stdlib.begin(); it != stdlib.end(); ++it)
    {
        stdfun_map::const_iterator fun_it = it->second.find(fun);
        if (fun_it != it->second.end())
        {
            out.first  = it->first;
            out.second = fun_it->second;
            return true;
        }
    }
    return false;
}

bool lxs::reg_find_name(lua_State* const L, const char* lib, lua_CFunction fun, stdfun_name& out)
{
    lxs_assert(L, L);
    lxs_assert(L, lib);
    lxs_assert(L, fun);

    stdlib_map::const_iterator lib_it = stdlib.find(lib);
    if (lib_it == stdlib.end())
        return false;

    stdfun_map funs = lib_it->second;
    stdfun_map::const_iterator fun_it = funs.find(fun);
    if (fun_it == funs.end())
        return false;

    out.first  = lib;
    out.second = fun_it->second;
    return true;
}

#endif // LUAXS_CORE_TRACEBACK_EX
