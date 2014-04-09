#ifndef XSLUALOB_XS_LOG_H
#define XSLUALOB_XS_LOG_H

#include "lua.h"
#include "lauxlib.h"

#include <stdio.h>

typedef struct
{
    FILE *h;
    int indentDepth;
} Logger;

static int logOpen(lua_State *const L);
static int logClose(lua_State *const L);
static int logIncreaseIndent(lua_State *const L);
static int logDecraseIndent(lua_State *const L);
static int logAdjustIndent(lua_State *const L);
static int logBeginBlock(lua_State *const L);
static int logEndBlock(lua_State *const L);
static int logWriteLine(lua_State *const L);

#endif // XSLUALOB_XS_LOG_H
