/*
** $Id: ldblib.c,v 1.104.1.4 2009/08/04 18:50:18 roberto Exp $
** Interface from Lua to its debug API
** See Copyright Notice in lua.h
*/

extern "C"
{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ldblib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#include "lxsext.h"
#include "lxs_string.hpp"

static int db_getregistry (lua_State *L) {
  lua_pushvalue(L, LUA_REGISTRYINDEX);
  return 1;
}


static int db_getmetatable (lua_State *L) {
  luaL_checkany(L, 1);
  if (!lua_getmetatable(L, 1)) {
    lua_pushnil(L);  /* no metatable */
  }
  return 1;
}


static int db_setmetatable (lua_State *L) {
  int t = lua_type(L, 2);
  luaL_argcheck(L, t == LUA_TNIL || t == LUA_TTABLE, 2,
                    "nil or table expected");
  lua_settop(L, 2);
  lua_pushboolean(L, lua_setmetatable(L, 1));
  return 1;
}


static int db_getfenv (lua_State *L) {
  luaL_checkany(L, 1);
  lua_getfenv(L, 1);
  return 1;
}


static int db_setfenv (lua_State *L) {
  luaL_checktype(L, 2, LUA_TTABLE);
  lua_settop(L, 2);
  if (lua_setfenv(L, 1) == 0)
    lxs_error(L, LUA_QL("setfenv")
                  " cannot change environment of given object");
  return 1;
}


static void settabss (lua_State *L, const char *i, const char *v) {
  lua_pushstring(L, v);
  lua_setfield(L, -2, i);
}


static void settabsi (lua_State *L, const char *i, int v) {
  lua_pushinteger(L, v);
  lua_setfield(L, -2, i);
}


static lua_State *getthread (lua_State *L, int *arg) {
  if (lua_isthread(L, 1)) {
    *arg = 1;
    return lua_tothread(L, 1);
  }
  else {
    *arg = 0;
    return L;
  }
}


static void treatstackoption (lua_State *L, lua_State *L1, const char *fname) {
  if (L == L1) {
    lua_pushvalue(L, -2);
    lua_remove(L, -3);
  }
  else
    lua_xmove(L1, L, 1);
  lua_setfield(L, -2, fname);
}


static int db_getinfo (lua_State *L) {
  lua_Debug ar;
  int arg;
  lua_State *L1 = getthread(L, &arg);
  const char *options = luaL_optstring(L, arg+2, "flnSu");
  if (lua_isnumber(L, arg+1)) {
    if (!lua_getstack(L1, (int)lua_tointeger(L, arg+1), &ar)) { //-V2005
      lua_pushnil(L);  /* level out of range */
      return 1;
    }
  }
  else if (lua_isfunction(L, arg+1)) {
    lua_pushfstring(L, ">%s", options);
    options = lua_tostring(L, -1);
    lua_pushvalue(L, arg+1);
    lua_xmove(L, L1, 1);
  }
  else
    return luaL_argerror(L, arg+1, "function or level expected");
  if (!lua_getinfo(L1, options, &ar))
    return luaL_argerror(L, arg+2, "invalid option");
  lua_createtable(L, 0, 2);
  if (strchr(options, 'S')) {
    settabss(L, "source", ar.source);
    settabss(L, "short_src", ar.short_src);
    settabsi(L, "linedefined", ar.linedefined);
    settabsi(L, "lastlinedefined", ar.lastlinedefined);
    settabss(L, "what", ar.what);
  }
  if (strchr(options, 'l'))
    settabsi(L, "currentline", ar.currentline);
  if (strchr(options, 'u'))
    settabsi(L, "nups", ar.nups);
  if (strchr(options, 'n')) {
    settabss(L, "name", ar.name);
    settabss(L, "namewhat", ar.namewhat);
  }
  if (strchr(options, 'L'))
    treatstackoption(L, L1, "activelines");
  if (strchr(options, 'f'))
    treatstackoption(L, L1, "func");
  return 1;  /* return table */
}


static int db_getlocal (lua_State *L) {
  int arg;
  lua_State *L1 = getthread(L, &arg);
  lua_Debug ar;
  const char *name;
  if (!lua_getstack(L1, luaL_checkint(L, arg+1), &ar))  /* out of range? */
    return luaL_argerror(L, arg+1, "level out of range");
  name = lua_getlocal(L1, &ar, luaL_checkint(L, arg+2));
  if (name) {
    lua_xmove(L1, L, 1);
    lua_pushstring(L, name);
    lua_pushvalue(L, -2);
    return 2;
  }
  else {
    lua_pushnil(L);
    return 1;
  }
}


static int db_setlocal (lua_State *L) {
  int arg;
  lua_State *L1 = getthread(L, &arg);
  lua_Debug ar;
  if (!lua_getstack(L1, luaL_checkint(L, arg+1), &ar))  /* out of range? */
    return luaL_argerror(L, arg+1, "level out of range");
  luaL_checkany(L, arg+3);
  lua_settop(L, arg+3);
  lua_xmove(L, L1, 1);
  lua_pushstring(L, lua_setlocal(L1, &ar, luaL_checkint(L, arg+2)));
  return 1;
}


static int auxupvalue (lua_State *L, int get) {
  const char *name;
  int n = luaL_checkint(L, 2);
  luaL_checktype(L, 1, LUA_TFUNCTION);
  if (lua_iscfunction(L, 1)) return 0;  /* cannot touch C upvalues from Lua */
  name = get ? lua_getupvalue(L, 1, n) : lua_setupvalue(L, 1, n);
  if (name == NULL) return 0;
  lua_pushstring(L, name);
  lua_insert(L, -(get+1));
  return get + 1;
}


static int db_getupvalue (lua_State *L) {
  return auxupvalue(L, 1);
}


static int db_setupvalue (lua_State *L) {
  luaL_checkany(L, 3);
  return auxupvalue(L, 0);
}



static const char KEY_HOOK = 'h';


static void hookf (lua_State *L, lua_Debug *ar) {
  static const char *const hooknames[] =
    {"call", "return", "line", "count", "tail return"};
  lua_pushlightuserdata(L, (void *)&KEY_HOOK); //-V2005
  lua_rawget(L, LUA_REGISTRYINDEX);
  lua_pushlightuserdata(L, L);
  lua_rawget(L, -2);
  if (lua_isfunction(L, -1)) {
    lua_pushstring(L, hooknames[(int)ar->event]); //-V2005
    if (ar->currentline >= 0)
      lua_pushinteger(L, ar->currentline);
    else lua_pushnil(L);
    lua_assert(lua_getinfo(L, "lS", ar));
    lua_call(L, 2, 0);
  }
}


static int makemask (const char *smask, int count) {
  int mask = 0;
  if (strchr(smask, 'c')) mask |= LUA_MASKCALL;
  if (strchr(smask, 'r')) mask |= LUA_MASKRET;
  if (strchr(smask, 'l')) mask |= LUA_MASKLINE;
  if (count > 0) mask |= LUA_MASKCOUNT;
  return mask;
}


static char *unmakemask (int mask, char *smask) {
  int i = 0;
  if (mask & LUA_MASKCALL) smask[i++] = 'c';
  if (mask & LUA_MASKRET) smask[i++] = 'r';
  if (mask & LUA_MASKLINE) smask[i++] = 'l';
  smask[i] = '\0';
  return smask;
}


static void gethooktable (lua_State *L) {
  lua_pushlightuserdata(L, (void *)&KEY_HOOK); //-V2005
  lua_rawget(L, LUA_REGISTRYINDEX);
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    lua_createtable(L, 0, 1);
    lua_pushlightuserdata(L, (void *)&KEY_HOOK); //-V2005
    lua_pushvalue(L, -2);
    lua_rawset(L, LUA_REGISTRYINDEX);
  }
}


static int db_sethook (lua_State *L) {
  int arg, mask, count;
  lua_Hook func;
  lua_State *L1 = getthread(L, &arg);
  if (lua_isnoneornil(L, arg+1)) {
    lua_settop(L, arg+1);
    func = NULL; mask = 0; count = 0;  /* turn off hooks */
  }
  else {
    const char *smask = luaL_checkstring(L, arg+2);
    luaL_checktype(L, arg+1, LUA_TFUNCTION);
    count = luaL_optint(L, arg+3, 0);
    func = hookf; mask = makemask(smask, count);
  }
  gethooktable(L);
  lua_pushlightuserdata(L, L1);
  lua_pushvalue(L, arg+1);
  lua_rawset(L, -3);  /* set new hook */
  lua_pop(L, 1);  /* remove hook table */
  lua_sethook(L1, func, mask, count);  /* set hooks */
  return 0;
}


static int db_gethook (lua_State *L) {
  int arg;
  lua_State *L1 = getthread(L, &arg);
  char buff[5];
  int mask = lua_gethookmask(L1);
  lua_Hook hook = lua_gethook(L1);
  if (hook != NULL && hook != hookf)  /* external hook? */
    lua_pushliteral(L, "external hook");
  else {
    gethooktable(L);
    lua_pushlightuserdata(L, L1);
    lua_rawget(L, -2);   /* get hook */
    lua_remove(L, -2);  /* remove hook table */
  }
  lua_pushstring(L, unmakemask(mask, buff));
  lua_pushinteger(L, lua_gethookcount(L1));
  return 3;
}


static int db_debug (lua_State *L) {
  for (;;) {
    char buffer[250];
    fputs("lua_debug> ", stderr);
    if (fgets(buffer, sizeof(buffer), stdin) == 0 ||
        strcmp(buffer, "cont\n") == 0)
      return 0;
    if (luaL_loadbuffer(L, buffer, strlen(buffer), "=(debug command)") ||
        lua_pcall(L, 0, 0, 0)) {
      fputs(lua_tostring(L, -1), stderr);
      fputs("\n", stderr);
    }
    lua_settop(L, 0);  /* remove eventual returns */
  }
}


#define LEVELS1	12	/* size of the first part of the stack */
#define LEVELS2	10	/* size of the second part of the stack */

static int db_traceback (lua_State *L)
{
    int level;
    int firstpart = 1;  /* still before eventual `...' */
    int arg;
    lua_State *L1 = getthread(L, &arg);
    lua_Debug ar;
    if (lua_isnumber(L, arg+2))
    {
        level = (int)lua_tointeger(L, arg+2); //-V2005
        lua_pop(L, 1);
    }
    else
        level = (L == L1) ? 1 : 0;  /* level 0 may be this own function */
    if (lua_gettop(L) == arg)
        lua_pushliteral(L, "");
    else if (!lua_isstring(L, arg+1))
        return 1;  /* message is not a string */
    else
        lua_pushliteral(L, "\n");

    lua_pushliteral(L, "stack traceback:");
    while (lua_getstack(L1, level++, &ar))
    {
        if (level > LEVELS1 && firstpart)
        {
            /* no more than `LEVELS2' more levels? */
            if (!lua_getstack(L1, level+LEVELS2, &ar))
                level--;  /* keep going */
            else
            {
                lua_pushliteral(L, "\n\t...");  /* too many levels */
                while (lua_getstack(L1, level+LEVELS2, &ar))  /* find last levels */
                    level++;
            }
            firstpart = 0;
            continue;
        }
        lua_pushliteral(L, "\n\t");
        lua_getinfo(L1, "Snl", &ar);
        lua_pushfstring(L, "%s:", ar.short_src);
        if (ar.currentline > 0)
            lua_pushfstring(L, "%d:", ar.currentline);
        if (*ar.namewhat != '\0')  /* is there a name? */
            lua_pushfstring(L, " in function " LUA_QS, ar.name);
        else
        {
            if (*ar.what == 'm')  /* main? */
                lua_pushfstring(L, " in main chunk");
            else if (*ar.what == 'C' || *ar.what == 't')
                lua_pushliteral(L, " ?");  /* C function or tail call */
            else
                lua_pushfstring(L, " in function <%s:%d>", ar.short_src, ar.linedefined);
        }
        lua_concat(L, lua_gettop(L) - arg);
    }
    lua_concat(L, lua_gettop(L) - arg);
    return 1;
}

#if LUAXS_EXTEND_DBLIB

/// Pushes a light-userdata object (a pointer) to the stack.
/// @param opt|mixed  Can be anything, including nil.
/// @return userdata  A pointer to whatever was at the top of the stack.
static int libL_getpointer(lua_State* const L)
{
    lua_pushlightuserdata(L, const_cast<void*>(lua_topointer(L, 1)));
    return 1;
}

/// Dumps a chunk of memory to a file stream.
/// 
/// Sample output:
/// 
/// optional_description_string:
///   0000  61 20 63 68 61 72 20 73 74 72 69 6e 67 20 67 72  a char string gr
///   0010  65 61 74 65 72 20 74 68 61 6e 20 31 36 20 63 68  eater than 16 ch
///   0020  61 72 73 00                                      ars.
///
/// @param mixed         Whatever shall be dumped, can be anything but nil.
/// @param integer       Length of the dump.
/// @param opt|string    An optional descriptional message.
/// @param opt|userdata  A file stream (io.open) where to print the output to.
/// @see http://stackoverflow.com/a/7776146/105211
static int libL_memdump(lua_State* const L)
{
    const void* addr = NULL;
    uint32_t    size = 0u;
    const char* desc = NULL;
    FILE*       file = NULL;
    int         top  = lua_gettop(L);

    luaL_argcheck(L, top >= 2, 1, "at least 2 arguments are expected");

    addr = lua_topointer(L, 1);
    size = (uint32_t)luaL_checkinteger(L, 2); //-V2005

    luaL_argcheck(L, addr != NULL, 1, "must be a valid and accessable memory address");
    luaL_argcheck(L, size > 0    , 2, "must be greater than zero");

    if (top >= 3)
    {
        int type = lua_type(L, 3);
        if (type == LUA_TSTRING)
            desc = luaL_checklstring(L, 3, NULL);
        else if (type == LUA_TUSERDATA)
            file = lxs_checkfilep(L, 3);

        if (top >= 4)
            file = lxs_checkfilep(L, 4);
    }

    lxs_hexdump(file ? file : stdout, desc, addr, size);
    return 0;
}


static void traceback_ex_append_desc(lua_State* const L, lxs_string* s, int type)
{
    lxs_assert_stack_begin(L);

    switch (type)
    {
    case LUA_TNONE:
        lxs_sappendl(L, s, "none");
        break;
    case LUA_TNIL:
        lxs_sappendl(L, s, "nil");
        break;
    case LUA_TBOOLEAN:
        if (lua_toboolean(L, -1))
            lxs_sappendl(L, s, "true");
        else
            lxs_sappendl(L, s, "false");
        break;
    case LUA_TNUMBER:
        lxs_sappend_format(L, s, LUA_NUMBER_FMT, lua_tonumber(L, -1));
        break;
    case LUA_TSTRING:
    {
        size_t len;
        const char* str = lua_tolstring(L, -1, &len);
        lxs_sappendc(L, s, '`');
        lxs_sappend(L, s, str, len);
        lxs_sappendc(L, s, '\'');
        break;
    }
    case LUA_TFUNCTION:
    {
        lua_CFunction func = lua_tocfunction(L, -1);
        if (func)
        {
            lxs::stdfun_name name;
            if (lxs::reg_find_name(L, func, name))
            {
                lxs_sappend_format(L, s,
                    "`%s.%s' @ %p",
                    name.first,
                    name.second,
                    func
                );
                break;
            }
        }
    }
    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA: // TODO: identify ud
    case LUA_TTHREAD:
    case LUA_TTABLE:    // TODO: identify or dump tables
    default:
        lxs_sappend_format(L, s, "@ %p", lua_topointer(L, -1));
        break;
    }
    lxs_sappendc(L, s, '\n');

    lxs_assert_stack_end(L, 0);
}

/// debug.traceback_ex([thread,] [message [, level]])
static int libL_traceback_ex(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    int level;

    int arg;
    lua_State* L1 = getthread(L, &arg);
    if (lua_isnumber(L, arg + 2))
    {
        level = lua_tointeger(L, arg + 2);
        lua_pop(L, 1);
    }
    else
        level = (L == L1) ? 1 : 0;

    lxs_spb_decl(L, s);
    if (lua_gettop(L) != arg && !lua_isstring(L, arg + 1))
        return 1;
    //else
    //    lxs_sappendc(L, lxs_spb_ptr(s), '\n');

    lua_Debug debug;
    while (lua_getstack(L1, level, &debug))
    {
        int top = lua_gettop(L);
        if (!lua_getinfo(L1, "nSluf", &debug) || !debug.what)
            break;

        // function
        lxs_sappend_format(L, lxs_spb_ptr(s), "[%2d]  ", level);
        if (debug.what[0] == 'm')       // main
        {
            if (debug.source[0] == '@')
            {
                lxs_sappend_format(L, lxs_spb_ptr(s),
                    "main chunk of file `%s' at line %d",
                    debug.source + 1,
                    debug.currentline
                );
            }
            else
            {
                lxs_sappend_format(L, lxs_spb_ptr(s),
                    "main chunk of `%s' at line %d",
                    debug.short_src,
                    debug.currentline
                );
            }
        }
        else if (debug.what[0] == 'C')  // C function
        {
            lxs::stdfun_name name;
            if (lxs::reg_find_name(L, lua_tocfunction(L, -1), name))
                lxs_sappend_format(L, lxs_spb_ptr(s),
                    "C function `%s' (%s) (%s.%s)",
                    debug.short_src,
                    debug.name,
                    name.first,
                    name.second
                );
            else
                lxs_sappend_format(L, lxs_spb_ptr(s),
                "C function `%s' (%s)",
                    debug.short_src,
                    debug.name
                );
        }
        else if (debug.what[0] == 't')  // tail call
        {
            lxs_sappend_format(L, lxs_spb_ptr(s),
                "tail call `%s'",
                debug.short_src
            );
        }
        else                            // lua function
        {
            lxs_sappend_format(L, lxs_spb_ptr(s),
                "Lua function: `%s' (%s:%d)",
                debug.name,
                debug.short_src,
                debug.linedefined
            );
        }
        lxs_sappendc(L, lxs_spb_ptr(s), '\n');

        // locals
        int         local_n = 1;
        const char* local = lua_getlocal(L, &debug, local_n);
        if (local)
        {
            lxs_sappendl(L, lxs_spb_ptr(s), "    local variables:\n");
            do
            {
                int type = lua_type(L, -1);
                lxs_sappend_format(
                    L,
                    lxs_spb_ptr(s),
                    "        %s = (%s) ",
                    local,
                    lua_typename(L, type)
                );
                traceback_ex_append_desc(L, lxs_spb_ptr(s), type);

                local = lua_getlocal(L1, &debug, ++local_n);
            } while (local);
            lua_pop(L, 1);
        }

        // upvalues
        if (lua_isfunction(L, -1) && debug.nups > 0)
        {
            lxs_sappendl(L, lxs_spb_ptr(s), "    upvalues:\n");
            for (int upval_n = 1; upval_n < debug.nups; ++upval_n)
            {
                const char* upval = lua_getupvalue(L1, -1, upval_n);
                if (upval)
                {
                    int type = lua_type(L, -1);
                    lxs_sappend_format(
                        L,
                        lxs_spb_ptr(s),
                        "        %s = (%s) ",
                        upval,
                        lua_typename(L, type)
                    );
                    traceback_ex_append_desc(L, lxs_spb_ptr(s), type);
                    lua_pop(L, 1);
                }
            }
        }

        lua_settop(L, top);
        memset(&debug, 0, sizeof(lua_Debug));
        ++level;
    }
    lxs_spushresult(L, lxs_spb_ptr(s));

    lxs_assert_stack_end(L, 1);
    return 1;
}

#endif // LUAXS_EXTEND_DBLIB


/******************************************************************************
 * ldblib.c extensions
 *****************************************************************************/

static const luaL_Reg dblib[] = {
    { "debug",         db_debug        },
    { "getfenv",       db_getfenv      },
    { "gethook",       db_gethook      },
    { "getinfo",       db_getinfo      },
    { "getlocal",      db_getlocal     },
    { "getregistry",   db_getregistry  },
    { "getmetatable",  db_getmetatable },
    { "getupvalue",    db_getupvalue   },
    { "setfenv",       db_setfenv      },
    { "sethook",       db_sethook      },
    { "setlocal",      db_setlocal     },
    { "setmetatable",  db_setmetatable },
    { "setupvalue",    db_setupvalue   },
    { "traceback",     db_traceback    },
#if LUAXS_EXTEND_DBLIB
    { "getpointer",    libL_getpointer   },
    { "memdump",       libL_memdump      },
    { "traceback_ex",  libL_traceback_ex },
#endif
    { NULL, NULL }
};


LUALIB_API int luaopen_debug(lua_State* L)
{
    lxs_assert_stack_begin(L);
    
    luaL_register(L, LUA_DBLIBNAME, dblib);

#if LUAXS_CORE_TRACEBACK && LUAXS_TRACEBACK_FROMREGISTRY
    lua_pushcfunction(L, db_traceback);
    lxs_rawsetl(L, LUA_REGISTRYINDEX, LUAXS_TRACEBACK_REGISTRYKEY);
#endif

    lxs_assert_stack_end(L, 1);
    return 1;
}

}; // extern "C"
