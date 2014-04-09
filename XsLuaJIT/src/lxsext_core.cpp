#define lxsext_core_c
#define LUA_CORE

extern "C" {

#include "lxsext_core.h"

#include "luajit.h"
#include "lauxlib.h"
#include "lxs_def.h"
#include "lxs_string.hpp"

#include <signal.h>
#include <stdio.h>


//==============================================================================

static int _runtime_package_fix(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_rawgetl(L, LUA_GLOBALSINDEX, "package");
    if (lua_istable(L, -1))
    {
        lua_pushliteral(L,
                        "gamedata\\scripts\\?.script"
                        ";"
                        "gamedata\\scripts\\lib\\?.lua"
        );
        lxs_rawsetl(L, -2, "path");
    }
    lua_pop(L, 1);

    lxs_assert_stack_end(L, 0);
    return 0;
}

#if LUAXS_DEBUG
static void _runtime_dump_value(lua_State* const L, int idx)
{
    int type = lua_type(L, idx);
    switch (type)
    {
    case LUA_TNONE:
        fputs("none", CLOG_STREAM);
        break;
    case LUA_TNIL:
        fputs("nil", CLOG_STREAM);
        break;
    case LUA_TBOOLEAN:
        fputs(lua_toboolean(L, idx) ? "true" : "false", CLOG_STREAM);
        break;
    case LUA_TNUMBER:
        fprintf(CLOG_STREAM, LUA_NUMBER_FMT, lua_tonumber(L, idx));
        break;
    case LUA_TSTRING:
        fputc('`', CLOG_STREAM);
        fputs(lua_tostring(L, idx), CLOG_STREAM);
        fputc('\'', CLOG_STREAM);
        break;
    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
    case LUA_TTABLE:
    case LUA_TFUNCTION:
    case LUA_TTHREAD:
    default:
        fprintf(CLOG_STREAM, "%s (%p)", lua_typename(L, type), lua_topointer(L, idx));
        break;
    }
}

static int _runtime_dump_registry(lua_State* const L)
{
    lxs_assert_stack_begin(L);
    CIFO(L, "Lua registry:");

    lua_pushnil(L);
    while (lua_next(L, LUA_REGISTRYINDEX))
    {
        lua_pushvalue(L, -2);

        fputs("  ", CLOG_STREAM);
        _runtime_dump_value(L, -1);
        fputs(" = ", CLOG_STREAM);
        _runtime_dump_value(L, -2);
        fputc('\n', CLOG_STREAM);

        lua_pop(L, 2);
    }
    fflush(CLOG_STREAM);

    lxs_assert_stack_end(L, 0);
    return 0;
}


static int _runtime_jit_debugmode(lua_State* const L)
{
    lxs_assert_stack_begin(L);
    CIFO(L, "Enabling hooks (jit.debug)");

    lxs_rawgetl(L, LUA_GLOBALSINDEX, "jit");
    if (lua_istable(L, -1))
    {
        lxs_rawgetl(L, -1, "debug");
        if (lua_isfunction(L, -1))
            lua_call(L, 0, 0);
        else
            lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lxs_assert_stack_end(L, 0);
    return 0;
}

#endif // LUAXS_DEBUG


void lxs_core_runtime(lua_State* const L)
{
    static const lua_CFunction funcs[] =
    {
        _runtime_package_fix,
#if LUAXS_DEBUG
        _runtime_dump_registry,
        _runtime_jit_debugmode
#endif // LUAXS_DEBUG
    };

    lxs_assert_stack_begin(L);
    for (size_t i = 0; i < _countof(funcs); ++i)
    {
        funcs[i](L);
    }
    lxs_assert_stack_end(L, 0);
}


//==============================================================================

#if LUAXS_CORE_TRACEBACK

static int _traceback_invoke(lua_State* const L)
{
#if LUAXS_TRACEBACK_TOSTRING
    if (lua_type(L, 1) != LUA_TSTRING)
    {
        lxs_rawget(L, LUA_GLOBALSINDEX, "tostring");
        lua_insert(L, 1);
        lua_call(L, 1, 1);
    }
#else
    if (lua_type(L, 1) != LUA_TSTRING)
        return 0;
#endif

#if LUAXS_TRACEBACK_FROMREGISTRY
    lxs_rawgetl(L, LUA_REGISTRYINDEX, LUAXS_TRACEBACK_REGISTRYKEY);
    if (!lua_iscfunction(L, -1))
    {
        CERR(L, "Could not locate key 'traceback' in Lua registry.");

        lua_pop(L, 1);
        return 0;
    }
#else
    lxs_rawget(L, LUA_GLOBALSINDEX, "debug");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return 0;
    }

    lxs_rawget(L, -1, "traceback");
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);
        return 0;
    }
#endif

    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);

    return 1;
}

static void _traceback_signal_stop(lua_State* const L, lua_Debug* const D)
{
    CLOG_MARK(L);

    UNUSED(D);
    lua_sethook(L, NULL, 0, 0);
    lxs_error(L, "interrupted!");
}

static lua_State* _tracingL = NULL;

static void _traceback_signal_start(int i)
{
    CLOG_MARK(_tracingL);

    signal(i, SIG_DFL);
    lua_sethook(_tracingL,
                _traceback_signal_stop,
                LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT,
                1);
}


//------------------------------------------------------------------------------

void lxs_core_traceback(lua_State* const L)
{
    if (_tracingL)
        return;

    _tracingL = L;
    luaJIT_setmode(L, 2, LUAJIT_MODE_DEBUG);
    // TODO: check current debug mode and return to it when exiting; dont assume it's 0.

    int base = lua_gettop(L);

    lua_pushcfunction(L, _traceback_invoke);
    lua_insert(L, 1);

    signal(SIGINT, _traceback_signal_start);
    int status = lua_pcall(L, 1, 1, base);
    signal(SIGINT, SIG_DFL);
    
    lua_remove(L, base);

    if (status != 0)
        lua_gc(L, LUA_GCCOLLECT, 0);

    luaJIT_setmode(L, 0, LUAJIT_MODE_DEBUG);
    _tracingL = NULL;
}

#endif // LUAXS_CORE_TRACEBACK


//==============================================================================

#if LUAXS_CORE_ATEXIT

static bool exiting = false;

void lxs_core_atexit(lua_State* const L, bool from_error, bool from_pcall)
{
    if (exiting)
        return;

    lxs_assert_stack_begin(L);

    exiting = true;

    if (from_error)
    {
        CERR(L, "exiting from error, stack:");
        CLOG_STACK(L);
#if !LUAXS_DEBUG
        CLOG_FLUSH(L);
#endif
    }

    lxs_rawgetl(L, LUA_GLOBALSINDEX, "atexit");
    if (lua_isfunction(L, -1))
    {
        lua_pushvalue(L, lua_gettop(L) - 1);
        lua_pushboolean(L, from_error);
        lua_pushboolean(L, from_pcall);
        lua_call(L, 3, 0);

        //lua_pushnil(L);
        //lxs_rawset(L, LUA_GLOBALSINDEX, "atexit");
    }
    else
        lua_pop(L, 1);

    lxs_assert_stack_end(L, 0);
}

#endif // LUAXS_CORE_ATEXIT


//==============================================================================

#if LUAXS_CORE_LUASTACK

static __inline void _luastack_writeln(lua_State* const L,
                                       lxs_string* const s,
                                       int abs_idx,
                                       int rel_idx)
{
    lxs_assert_stack_begin(L);

    int type = lua_type(L, abs_idx);

    lxs_sappend_format(L, s, "  [%2d|%2d] (%s):  ",
                       abs_idx, rel_idx, lua_typename(L, type));

    switch (type)
    {
    case LUA_TNONE:
        //lxs_sappendl(L, s, "n/a");
        break;
    case LUA_TNIL:
        lxs_sappendl(L, s, "nil");
        break;
    case LUA_TBOOLEAN:
        if (lua_toboolean(L, abs_idx))
            lxs_sappendl(L, s, "true");
        else
            lxs_sappendl(L, s, "false");
	    break;
    case LUA_TNUMBER:
        lxs_sappend_format(L, s, LUA_NUMBER_FMT, lua_tonumber(L, abs_idx));
        break;
    case LUA_TSTRING:
    {
#if LUA_LUASTACK_TERSE > 0
        static const size_t MAXLEN = LUA_LUASTACK_TERSE;

        size_t      len = 0;
        const char* str = lua_tolstring(L, abs_idx, &len);

        lxs_sappendc(L, s, '`');
        if (len > MAXLEN)
        {
            lxs_sappend(L, s, str, MAXLEN - 3);
            lxs_sappendl(L, s, "...");
        }
        else if (len > 0)
            lxs_sappend(L, s, str, len);
        lxs_sappendc(L, s, '\'');
#else // LUA_LUASTACK_TERSE
        size_t      len = 0;
        const char* str = lua_tolstring(L, abs_idx, &len);

        lxs_sappendc(L, s, '`');
        if (len > 0)
            lxs_sappend(L, s, str, len);
        lxs_sappendc(L, s, '\'');
#endif // LUA_LUASTACK_TERSE
        break;
    }
    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
    case LUA_TTABLE:
    case LUA_TFUNCTION:
    case LUA_TTHREAD:
    default:
        lxs_sappend_format(L, s, "%s (%p)",
                           lua_typename(L, type), lua_topointer(L, abs_idx));
        break;
    }
    lxs_sappendc(L, s, '\n');

    lxs_assert_stack_end(L, 0);
}

#if LUAXS_STR_PERSISTENT_BUFFER
#   define sptr(x) x
#else
#   define sptr(x) &x
#endif

void lxs_core_luastack(lua_State* const L)
{
    int top = lua_gettop(L);
    if (lua_type(L, top) != LUA_TSTRING)
        return;

    lxs_assert_stack_begin(L);

#if LUAXS_STR_PERSISTENT_BUFFER
    lxs_string* s = lxs_spb_get(L);
#else
    lxs_string s;
    lxs_sinit(L, &s);
#endif

    size_t      len;
    const char* str = lua_tolstring(L, top, &len);
    if (len > 0)
    {
        lxs_sappend(L, sptr(s), str, len);
        lxs_sappendl(L, sptr(s), "\n\n");
    }

    lxs_sappendl(L, sptr(s), "lua stack:\n");
    for (int i = top; i > 0; --i)
    {
        _luastack_writeln(L, sptr(s), i, -top - 1 + i);
    }

    lxs_spushresult(L, sptr(s));
    lua_replace(L, top);

    lxs_assert_stack_end(L, 0);
}

#undef sptr

#endif // LUAXS_CORE_LUASTACK

}; // extern "C"
