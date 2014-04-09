#include "xs.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// http://lua.2524044.n2.nabble.com/Extending-Lua-for-multiple-light-userdata-types-td7643931.html
// http://lua-users.org/lists/lua-l/2009-10/msg00470.html
// ============================================================================

static int xscbL_make(lua_State* L)
{
    int size = luaL_checkinteger(L, 1);
#if defined(XSCB_MIN_SIZE) && XSCB_MIN_SIZE > 0
    if (size < XSCB_MIN_SIZE)
        luaL_error(L,
                   XS_TYPENAME_CB " to small (%d), expecting at least size 2",
                   XSCB_MIN_SIZE);
#endif

    CircularBuffer* cb = cbMake(size);
    lua_pushlightuserdata(L, static_cast<void*>(cb));
    return 1;
}

static int xscbL_free(lua_State* L)
{
    CircularBuffer* cb = cb_check(L, 1);
    if (!cbEmpty(cb))
    {
        for (int i = 0; i < cb->size; ++i)
        {
            int refNum;
            cbGet(cb, &refNum);
            if (refNum >= 0)
                luaL_unref(L, LUA_REGISTRYINDEX, refNum);
        }
    }
    cbFree(cb);
    return 0;
}

static int xscbL_put(lua_State* L)
{
    if (lua_gettop(L) != 2)
        cb_nargError(L, 2);

    CircularBuffer* cb = cb_check(L, 1);

    int oldRef;
    cbPut(cb, luaL_ref(L, LUA_REGISTRYINDEX), &oldRef);
    if (oldRef >= 0)
        luaL_unref(L, LUA_REGISTRYINDEX, oldRef);

    return 0;
}

static int xscbL_get(lua_State* L)
{
    CircularBuffer* cb = cb_check(L, 1);
    if (!cbEmpty(cb))
    {
        int refNum;
        cbGet(cb, &refNum);
        lua_rawgeti(L, LUA_REGISTRYINDEX, refNum);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

static int xscbL_empty(lua_State* L)
{
    const CircularBuffer* cb = cb_check(L, 1);
    lua_pushboolean(L, cbEmpty(cb) ? 1 : 0);
    return 1;
}

static int xscbL_full(lua_State* L)
{
    const CircularBuffer* cb = cb_check(L, 1);
    lua_pushboolean(L, cbFull(cb) ? 1 : 0);
    return 1;
}

static int xscbL_size(lua_State* L)
{
    const CircularBuffer* cb = cb_check(L, 1);
    lua_pushinteger(L, (LUA_INTEGER)cb->size);
    return 1;
}

static int xscbL_equals(lua_State* L)
{
    const CircularBuffer* lhs = cb_check(L, 1);
    const CircularBuffer* rhs = cb_check(L, 2);
    lua_pushboolean(L, cbEquals(lhs, rhs));
    return 1;
}

// ============================================================================

static const luaL_Reg xsLibFunctions[] =
{
    { "cbMake",   xscbL_make   },
    { "cbFree",   xscbL_free   },
    { "cbPut",    xscbL_put    },
    { "cbGet",    xscbL_get    },
    { "cbSize",   xscbL_size   },
    { "cbEquals", xscbL_equals },
    //{ "cbEmpty",  xscbL_empty  },
    //{ "cbFull",   xscbL_full   },
    { NULL, NULL }
};

int luaopen_Xs(lua_State* L)
{
    luaL_newlib(L, xsLibFunctions);

    lua_pushvalue(L, -1);
    lxs_rawset(L, LUA_GLOBALSINDEX, XS_LIBNAME_XS);

    return 1;
}

#ifdef __cplusplus
};
#endif
