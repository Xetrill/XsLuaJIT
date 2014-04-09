/*
** $Id: lmathlib.c,v 1.67.1.1 2007/12/27 13:02:25 roberto Exp $
** Standard mathematical library
** See Copyright Notice in lua.h
*/

#include <stdlib.h>
#include <math.h>

#define lmathlib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#include "lxsext.h"

#if LUAXS_MATHLIB_SIMD
#  include "xscpu.h"
#  include "xssemath.h"
//#  define USE_SSE2
//#  include "sse_mathfun.h"
#endif // LUAXS_MATHLIB_SIMD

#undef PI
#define PI (3.14159265358979323846)
#define RAD_PER_DEG (PI/180.0)



static int math_crt_abs (lua_State *L) {
  lua_pushnumber(L, fabs(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_sin (lua_State *L) {
  lua_pushnumber(L, sin(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_sinh (lua_State *L) {
  lua_pushnumber(L, sinh(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_cos (lua_State *L) {
  lua_pushnumber(L, cos(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_cosh (lua_State *L) {
  lua_pushnumber(L, cosh(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_tan (lua_State *L) {
  lua_pushnumber(L, tan(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_tanh (lua_State *L) {
  lua_pushnumber(L, tanh(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_asin (lua_State *L) {
  lua_pushnumber(L, asin(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_acos (lua_State *L) {
  lua_pushnumber(L, acos(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_atan (lua_State *L) {
  lua_pushnumber(L, atan(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_atan2 (lua_State *L) {
  lua_pushnumber(L, atan2(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
  return 1;
}

static int math_crt_ceil(lua_State *L)
{
    lua_pushnumber(L, ceil(luaL_checknumber(L, 1)));
    return 1;
}

static int math_crt_floor(lua_State *L)
{
    lua_pushnumber(L, floor(luaL_checknumber(L, 1)));
    return 1;
}

static int math_crt_fmod (lua_State *L) {
  lua_pushnumber(L, fmod(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
  return 1;
}

static int math_crt_modf (lua_State *L) {
  double ip;
  double fp = modf(luaL_checknumber(L, 1), &ip);
  lua_pushnumber(L, ip);
  lua_pushnumber(L, fp);
  return 2;
}

static int math_crt_sqrt (lua_State *L)
{
    lua_pushnumber(L, sqrt(luaL_checknumber(L, 1)));
    return 1;
}

static int math_crt_pow (lua_State *L) {
  lua_pushnumber(L, pow(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
  return 1;
}

static int math_crt_log (lua_State *L) {
  lua_pushnumber(L, log(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_log10(lua_State *L)
{
    lua_pushnumber(L, log10(luaL_checknumber(L, 1)));
    return 1;
}

static int math_crt_exp (lua_State *L) {
  lua_pushnumber(L, exp(luaL_checknumber(L, 1)));
  return 1;
}

static int math_crt_deg (lua_State *L)
{
    lua_pushnumber(L, luaL_checknumber(L, 1) / RAD_PER_DEG);
    return 1;
}

static int math_crt_rad (lua_State *L)
{
    lua_pushnumber(L, luaL_checknumber(L, 1) * RAD_PER_DEG);
    return 1;
}

static int math_crt_frexp (lua_State *L) {
  int e;
  lua_pushnumber(L, frexp(luaL_checknumber(L, 1), &e));
  lua_pushinteger(L, e);
  return 2;
}

static int math_crt_ldexp (lua_State *L) {
  lua_pushnumber(L, ldexp(luaL_checknumber(L, 1), luaL_checkint(L, 2)));
  return 1;
}

static int math_crt_min (lua_State *L)
{
    int n = lua_gettop(L);  /* number of arguments */
    lua_Number dmin = luaL_checknumber(L, 1);
    int i;
    for (i=2; i<=n; i++)
        dmin = lxs_mind(L, i, dmin);
    lua_pushnumber(L, dmin);
    return 1;
}


static int math_crt_max (lua_State *L)
{
    int n = lua_gettop(L);  /* number of arguments */
    lua_Number dmax = luaL_checknumber(L, 1);
    int i;
    for (i=2; i<=n; i++)
        dmax = lxs_maxd(L, i, dmax);
    lua_pushnumber(L, dmax);
    return 1;
}


static int math_crt_random (lua_State *L) {
  /* the `%' avoids the (rare) case of r==1, and is needed also because on
     some systems (SunOS!) `rand()' may return a value larger than RAND_MAX */
  lua_Number r = (lua_Number)(rand()%RAND_MAX) / (lua_Number)RAND_MAX;
  switch (lua_gettop(L)) {  /* check number of arguments */
    case 0: {  /* no arguments */
      lua_pushnumber(L, r);  /* Number between 0 and 1 */
      break;
    }
    case 1: {  /* only upper limit */
      int u = luaL_checkint(L, 1);
      luaL_argcheck(L, 1<=u, 1, "interval is empty");
      lua_pushnumber(L, floor(r*u)+1);  /* int between 1 and `u' */
      break;
    }
    case 2: {  /* lower and upper limits */
      int l = luaL_checkint(L, 1);
      int u = luaL_checkint(L, 2);
      luaL_argcheck(L, l<=u, 2, "interval is empty");
      lua_pushnumber(L, floor(r*(u-l+1))+l);  /* int between `l' and `u' */
      break;
    }
    default: return lxs_error(L, "wrong number of arguments");
  }
  return 1;
}


static int math_crt_randomseed (lua_State *L) {
  srand(luaL_checkint(L, 1));
  return 0;
}


/*******************************************************************************
 * lmathlib.c extensions
 ******************************************************************************/

//=== CRT ======================================================================

static int math_crt_clamp(lua_State* const L)
{
    double num = luaL_checknumber(L, 1);
    double min = luaL_optnumber(L, 2, 0.0);
    double max = luaL_optnumber(L, 3, 1.0);
    num = (min < num) ? min : num;
    num = (max > num) ? max : num;
    lua_pushnumber(L, num);
    return 1;
}

static int math_crt_log2(lua_State* const L)
{
    const static double LOG2 = 0.69314718055994530942; // rounded, ...94217...
    lua_pushnumber(L, log(luaL_checknumber(L, 1)) / LOG2);
	return 1;
}

static int math_crt_round(lua_State* const L)
{
    double value = luaL_checknumber(L, 1);
    if (lua_isnumber(L, 2))
    {
        const int adjustment = (int)pow(10, lua_tonumber(L, 2));
        lua_pushnumber(L, floor(value * adjustment + 0.5) / adjustment);
    }
    else
        lua_pushnumber(L, floor(value * 0.5));
    return 1;
}

#if LUAXS_DEBUG
static int math_crt_setSSE2(lua_State* const L)
{
    lua_pushboolean(L, _set_SSE2_enable(luaL_checkint(L, 1)));
    return 1;
}
#endif


//=== SSE2 =====================================================================

#if LUAXS_MATHLIB_SIMD

static int math_sse2_clamp(lua_State* const L)
{
    lua_pushnumber(L,
        _xs_sse2_clamp(luaL_checknumber(L, 1),
                        luaL_optnumber(L, 2, 0.0),
                        luaL_optnumber(L, 3, 1.0)));
    return 1;
}

static int math_sse2_round(lua_State* const L)
{
    lua_pushnumber(L, floor(_xs_sse2_add(luaL_checknumber(L, 1), 0.5)));
    return 1;
}

static int math_sse2_log2(lua_State* const L)
{
    const static double LOG2 = 0.69314718055994530942;
    // TODO SSE version of log()
    lua_pushnumber(L, _xs_sse2_div(log(luaL_checknumber(L, 1)), LOG2));
    return 1;
}

static int math_sse2_log10(lua_State* const L)
{
    // TODO SSE version of log()
    const static double LOG10 = 2.30258509299404568402;
    lua_pushnumber(L, _xs_sse2_div(log(luaL_checknumber(L, 1)), LOG10));
    return 1;
}

static int math_sse2_min(lua_State* const L)
{
    int i, n = lua_gettop(L);
    lua_Number dmin = luaL_checknumber(L, 1);

    for (i = 2; i <= n; ++i)
        dmin = _xs_sse2_min(dmin, luaL_checknumber(L, i));

    lua_pushnumber(L, dmin);
    return 1;
}

static int math_sse2_max(lua_State* const L)
{
    int i, n = lua_gettop(L);
    lua_Number dmax = luaL_checknumber(L, 1);

    for (i = 2; i <= n; ++i)
        dmax = _xs_sse2_max(dmax, luaL_checknumber(L, i));

    lua_pushnumber(L, dmax);
    return 1;
}

static int math_sse2_rad(lua_State* const L)
{
    lua_pushnumber(L, _xs_sse2_mul(luaL_checknumber(L, 1), RAD_PER_DEG));
    return 1;
}

static int math_sse2_deg(lua_State* const L)
{
    lua_pushnumber(L, _xs_sse2_div(luaL_checknumber(L, 1), RAD_PER_DEG));
    return 1;
}

static int math_sse2_sqrt(lua_State* const L)
{
    lua_pushnumber(L, _xs_sse2_sqrt(luaL_checknumber(L, 1)));
    return 1;
}


//=== SSE4 =====================================================================

static int math_sse41_round(lua_State* const L)
{
    lua_pushnumber(L, _xs_sse41_round(luaL_checknumber(L, 1)));
    return 1;
}

static int math_sse41_ceil(lua_State* const L) 
{
    lua_pushnumber(L, _xs_sse41_ceil(luaL_checknumber(L, 1)));
    return 1;
}

static int math_sse41_floor(lua_State* const L)
{
    lua_pushnumber(L, _xs_sse41_floor(luaL_checknumber(L, 1)));
    return 1;
}

#endif // LUAXS_MATHLIB_SIMD


//==============================================================================

#if LUAXS_EXTEND_MATHLIB
static int math_crt_overwrite(lua_State* const L);
static int math_sse2_overwrite(lua_State* const L);
static int math_sse41_overwrite(lua_State* const L);
#endif // LUAXS_EXTEND_MATHLIB

static const luaL_Reg mathlib[] = {
    { "abs",         math_crt_abs         },
    { "acos",        math_crt_acos        },
    { "asin",        math_crt_asin        },
    { "atan2",       math_crt_atan2       },
    { "atan",        math_crt_atan        },
    { "ceil",        math_crt_ceil        },
    { "cosh",        math_crt_cosh        },
    { "cos",         math_crt_cos         },
    { "deg",         math_crt_deg         },
    { "exp",         math_crt_exp         },
    { "floor",       math_crt_floor       },
    { "fmod",        math_crt_fmod        },
    { "frexp",       math_crt_frexp       },
    { "ldexp",       math_crt_ldexp       },
    { "log10",       math_crt_log10       },
    { "log",         math_crt_log         },
    { "max",         math_crt_max         },
    { "min",         math_crt_min         },
    { "modf",        math_crt_modf        },
    { "pow",         math_crt_pow         },
    { "rad",         math_crt_rad         },
    { "random",      math_crt_random      },
    { "randomseed",  math_crt_randomseed  },
    { "sinh",        math_crt_sinh        },
    { "sin",         math_crt_sin         },
    { "sqrt",        math_crt_sqrt        },
    { "tanh",        math_crt_tanh        },
    { "tan",         math_crt_tan         },
#if LUAXS_EXTEND_MATHLIB
    { "clamp",       math_crt_clamp       },
    { "round",       math_crt_round       },
    { "log2",        math_crt_log2        },
# if LUAXS_MATHLIB_SIMD
    { "useCRT",      math_crt_overwrite   },
    { "useSSE2",     math_sse2_overwrite  },
    { "useSSE41",    math_sse41_overwrite },
# endif // !LUAXS_MATHLIB_SIMD
# if LUAXS_DEBUG
    { "useCRTSSE2",  math_crt_setSSE2     },
# endif // LUAXS_DEBUG
#endif // LUAXS_EXTEND_MATHLIB
    { NULL, NULL }
};

#if LUAXS_EXTEND_MATHLIB
# if LUAXS_MATHLIB_SIMD
static int math_crt_overwrite(lua_State* const L)
{
    lxs_rawget(L, LUA_GLOBALSINDEX, "math");
    lxs_pushfuncs(L, mathlib);
    CIFO(L, "CRT: enabled");
    return 0;
}

static int math_sse2_overwrite(lua_State* const L)
{
    bool result;
    static const luaL_Reg funcs[] =
    {
        { "clamp", math_sse2_clamp },
        { "round", math_sse2_round },
        { "log2",  math_sse2_log2  },
        { "log10", math_sse2_log10 },
        { "min",   math_sse2_min   },
        { "max",   math_sse2_max   },
        { "rad",   math_sse2_rad   },
        { "deg",   math_sse2_deg   },
        { "sqrt",  math_sse2_sqrt  },
        { NULL, NULL }
    };
    lxs_rawget(L, LUA_GLOBALSINDEX, "math");
    lxs_pushfuncs(L, funcs);
    result = _set_SSE2_enable(true);
    CIFO(L, "SSE2: enabled");
    CIFO(L, "SSE2 (CRT): %s", result ? "enabled" : "disabled");
    return 0;
}

static int math_sse41_overwrite(lua_State* const L)
{
    static const luaL_Reg funcs[] =
    {
        { "round", math_sse41_round },
        { "floor", math_sse41_floor },
        { "ceil",  math_sse41_ceil  },
        { NULL, NULL }
    };
    lxs_rawget(L, LUA_GLOBALSINDEX, "math");
    lxs_pushfuncs(L, funcs);
    CIFO(L, "SSE41: enabled");
    return 0;
}
# endif // LUAXS_MATHLIB_SIMD
#endif // LUAXS_EXTEND_MATHLIB

//==============================================================================

LUALIB_API int luaopen_math(lua_State *L)
{
    lxs_assert_stack_begin(L);

    luaL_register(L, LUA_MATHLIBNAME, mathlib);

    lua_pushnumber(L, PI);
    lua_setfield(L, -2, "pi");

    lua_pushnumber(L, HUGE_VAL);
    lua_setfield(L, -2, "huge");

#if defined(LUA_COMPAT_MOD)
    lua_getfield(L, -1, "fmod");
    lua_setfield(L, -2, "mod");
#endif

#if LUAXS_MATHLIB_SIMD
    xs_cpu_getinfo();

# if LUAXS_DEBUG
#  define CCPU(L, name, flag) \
    CDBG(L, "CPU " name ": %s", xs_cpu_supports(flag) ? "Supported" : "N/A");
    CCPU(L, "SSE3",           XS_CPU_SSE3I);
    CCPU(L, "SSSE3",          XS_CPU_SSE3S);
    CCPU(L, "SSE4",           XS_CPU_SSE4);
    CCPU(L, "SSE4.1",         XS_CPU_SSE41);
    CCPU(L, "SSE4.2",         XS_CPU_SSE42);
    CCPU(L, "SSE4a",          XS_CPU_SSE4A);
    CCPU(L, "Misaligned SSE", XS_CPU_MISALIGNED_SSE);
    CCPU(L, "MMX",            XS_CPU_MMX);
    CCPU(L, "3DNow! Ext",     XS_CPU_3DNOW_EXT);
    CCPU(L, "3DNow!",         XS_CPU_3DNOW);
#  undef CCPU
# endif // LUAXS_DEBUG

    if (xs_cpu_isintel())
    {
        lua_pushcclosure(L, math_sse2_overwrite, 0);
        lua_call(L, 0, 0);
    }
    else
    {
        bool result = _set_SSE2_enable(false);
        CDBG(L, "SSE2 (CRT): %s", result ? "enabled" : "disabled");
    }

    if (xs_cpu_supports(XS_CPU_SSE41))
    {
        lua_pushcclosure(L, math_sse41_overwrite, 0);
        lua_call(L, 0, 0);
    }
#endif // LUAXS_MATHLIB_SIMD

    lxs_assert_stack_end(L, 1);
    return 1;
}
