#define lxsext_clog_c
#define LUA_CORE

#include "lua.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>


//==============================================================================

#define SRC_DIR (LUA_DIRSEP "src" LUA_DIRSEP)

const char* lxs_tersesource(const char* path)
{
    char* src = strstr(path, SRC_DIR);
    return (src)
        ? CONST_CAST(const char*, src + sizeof(SRC_DIR) - 1)
        : path;
}

#undef SRC_DIR

//------------------------------------------------------------------------------

static void _lxs_dumpstack_writeln(lua_State* const L, int absIdx, int relIdx)
{
    int type = lua_type(L, absIdx);

    fprintf(CLOG_STREAM, "  [%2d|%2d] (%s)  ",
        absIdx, relIdx, lua_typename(L, type));

    switch (type)
    {
    case LUA_TNONE:
        fputs("none", CLOG_STREAM);
        break;
    case LUA_TNIL:
        fputs("nil", CLOG_STREAM);
        break;
    case LUA_TBOOLEAN:
        fputs(lua_toboolean(L, absIdx) ? "true" : "false", CLOG_STREAM);
        break;
    case LUA_TNUMBER:
        fprintf(CLOG_STREAM, LUA_NUMBER_FMT, lua_tonumber(L, absIdx));
        break;
    case LUA_TSTRING:
        fputc('`', CLOG_STREAM);
        fputs(lua_tostring(L, absIdx), CLOG_STREAM);
        fputc('\'', CLOG_STREAM);
        break;
    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
        //fprintf(CLOG_STREAM, "userdata (%p)", lua_topointer(L, absIdx));
        //break;
    case LUA_TTABLE:
    case LUA_TFUNCTION:
    case LUA_TTHREAD:
    default:
        fprintf(CLOG_STREAM, "%s (%p)", lua_typename(L, type),
                lua_topointer(L, absIdx));
        break;
    }

    fputc('\n', CLOG_STREAM);
}

void lxs_dumpstack(lua_State* L,
                   const char* file,
                   const char* func,
                   const uint32_t line,
                   const bool fromTop2Bottom)
{
    int i, top = lua_gettop(L);

    fprintf(CLOG_STREAM,
            "[S (%p, `%s' (%s:%u)]: stack size %d\n",
            L, file, func, line, top
    );

    if (fromTop2Bottom)
    {
        for (i = top; i > 0; --i)
            _lxs_dumpstack_writeln(L, i, -top - 1 + i);
    }
    else
    {
        for (i = 1; i <= top; ++i)
            _lxs_dumpstack_writeln(L, i, -top - 1 + i);
    }

#if LUAXS_DEBUG
    fflush(CLOG_STREAM);
#endif
}



//------------------------------------------------------------------------------
#if 0
void lxs_dumpups(lua_State* const L, int funcidx)
{
    fputs("upvalues:", CLOG_STREAM);

    int i = 0;
    while ((name = lua_getupvalue(L, funcidx, i)))
    {
        if (strlen(name) == 0)
            fprintf(CLOG_STREAM, "  <C:%d>: (%s) ", i, lua_typename(L, -1));
        else
            fprintf(CLOG_STREAM, "  `%s': (%s) ", name, , lua_typename(L, -1));

        switch (lua_type(L, -1))
        {
        case LUA_TNONE:
            fputs("none", CLOG_STREAM);
            break;
        case LUA_TNIL:
            fputs("nil", CLOG_STREAM);
            break;
        case LUA_TBOOLEAN:
            fputs(lua_toboolean(L, -1) ? "true" : "false", CLOG_STREAM);
            break;
        case LUA_TNUMBER:
            fprintf(CLOG_STREAM, LUA_NUMBER_FMT, lua_tonumber(L, -1));
            break;
        case LUA_TSTRING:
            fputc('`', CLOG_STREAM);
            fputs(lua_tostring(L, -1), CLOG_STREAM);
            fputc('\'', CLOG_STREAM);
            break;
        case LUA_TLIGHTUSERDATA:
        case LUA_TUSERDATA:
        case LUA_TTABLE:
        case LUA_TFUNCTION:
        case LUA_TTHREAD:
        default:
            fprintf(CLOG_STREAM, "@ %p", lua_topointer(L, -1));
            break;
        }
        fputc('\n', CLOG_STREAM);

        ++i;
    }
}
#endif

//------------------------------------------------------------------------------

/// @see http://stackoverflow.com/a/7776146/105211
void lxs_hexdump(FILE* file, const char* desc, const void* addr, uint32_t len)
{
    uint_fast32_t i, r;
    uint_fast8_t  buff[17];
    uint_fast8_t* pc = (void*)addr;

    if (desc != NULL)
    {
        fputs(desc, file);
        fputs(":\n", file);
    }

    for (i = 0; i < len; ++i)
    {
        //r = (i % 16)
        r = i & 15;

        if (r == 0)
        {
            if (i != 0)
            {
                fputs("  ", file);
                fputs(buff, file);
                fputc('\n', file);
            }
            fprintf(file, "  %04x ", i);
        }

        fprintf(file, " %02x", pc[i]);

        buff[r] = (iscntrl(pc[i]))
            ? '.'
            : pc[i];

        buff[r + 1] = '\0';
    }

    while ((r = i & 15) != 0)
    {
        fputs("   ", file);
        ++i;
    };

    fputs("  ", file);
    fputs(buff, file);
    fputc('\n', file);
}

