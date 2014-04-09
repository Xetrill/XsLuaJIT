#ifndef lxsext_strlib_h
#define lxsext_strlib_h

#include <string>

int lxsext_strlib_joinvector(lua_State* const L)
{
    std::string buf;
    std::string sep = luaL_optlstring(L, 1, NULL, NULL);
    int veclen;

    if (lua_isnumber(L, 2)
    {
        size_t allocHint = luaL_optinteger(L, 2, 0);
        if (allocHint < 0)
            luaL_argerror(L, 2, "cannot be less than zero");
        if (allocHint > 0)
            buf.resize(allocHint);
        lua_remove(L, 2);
    }

    luaL_checktype(L, 2, LUA_TTABLE);
    veclen = luaL_getn(L, 2);

    for (i = 1; i <= veclen; ++i)
    {
        typename std::string::size_type len;

        lua_rawgeti(L, 2, i);

        buf.append(luaL_checklstring(L, -1, &itmlen), itmlen);
        if (!sep.empty())
            buf.append(sep);

        lua_pop(L, 1);
    }
    lua_pushlstring(L, buf.c_str(), buf.length());

    return 1;
}

#endif // lxsext_strlib_h