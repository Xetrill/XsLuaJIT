#ifndef lxs_api_h
#define lxs_api_h 1
#define LUA_CORE

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#include "luajit.h"
#include "lauxlib.h"

#include "lxs_def.h"

#if LUAXS_DEBUG && defined(NDEBUG)
#  define _RESTORE_NDEBUG
#  undef  NDEBUG
#endif
#include <assert.h>
#ifdef _RESTORE_NDEBUG
#  define NDEBUG
#  undef  _RESTORE_NDEBUG
#endif


//==============================================================================
// Public C APIs
//==============================================================================

LUA_API FILE* lxs_checkfilep(lua_State* const L, int idx);


#if LUAXS_CLOG

LUA_API void  lxs_dumpstack(lua_State* L,
                            const char* file,
                            const char* func,
                            const uint32_t line,
                            const bool fromTop2Bottom);

LUA_API void  lxs_dumpups(lua_State* L, int funcidx);

LUA_API void  lxs_hexdump(FILE* file,
                          const char* desc,
                          const void* addr,
                          uint32_t len);

LUA_API const char* lxs_tersesource(const char* path);

#endif // LUAXS_CLOG



//==============================================================================
// Internal C APIs
//==============================================================================

int lxs_split(lua_State* const L, const char* src, size_t len, const char* seps);

size_t lxs_countfuncs(const luaL_Reg* funcs);
size_t lxs_counttable(lua_State* const L, int narg);
void   lxs_pushfuncs(lua_State* const L, const luaL_Reg* funcs);


XS_AINLINE static int abs_index_m1(lua_State* const L, int idx);
XS_AINLINE static void lxs_rawget(lua_State* const L, int idx, const char* key, size_t len);
XS_AINLINE static void lxs_rawset(lua_State* const L, int idx, const char* key, size_t len);

#ifdef __cplusplus
}; // extern "C"
#endif




//==============================================================================
// Internal C/C++ APIs
//============================================================================== 

XS_INLINE size_t lxs_mnext_pow2(size_t v);

XS_INLINE size_t lxs_mgrow(lua_State* const L,
                           size_t current_cap,
                           size_t new_cap,
                           bool DEFVAL(force, false),
                           double DEFVAL(growth_factor, LUAXS_STR_GROWTH_FACTOR));


XS_AINLINE static lua_Number lxs_mind(lua_State* const L, int idx, lua_Number rhs);
XS_AINLINE static lua_Number lxs_maxd(lua_State* const L, int idx, lua_Number rhs);

XS_AINLINE static lua_Integer lxs_min(lua_State* const L, int idx, lua_Integer rhs);
XS_AINLINE static lua_Integer lxs_max(lua_State* const L, int idx, lua_Integer rhs);


XS_AINLINE static void lxs_set_mt(lua_State* const L);
XS_AINLINE static lua_State* lxs_mt();



enum XS_CPU {
    XS_CPU_SSE3I,
    XS_CPU_SSE3S,
    XS_CPU_SSE4,
    XS_CPU_SSE41,
    XS_CPU_SSE42,
    XS_CPU_SSE4A,
    XS_CPU_MISALIGNED_SSE,
    XS_CPU_MMX,
    XS_CPU_3DNOW_EXT,
    XS_CPU_3DNOW,
    XS_CPU__MAX
};

static bool xs_cpu_isintel(void);
static bool xs_cpu_isamd(void);
static void xs_cpu_getinfo(void);
static bool xs_cpu_supports(enum XS_CPU flag);





#if LUAXS_EXTEND_CORE

void lxs_core_runtime(lua_State* const L);

#if LUAXS_CORE_TRACEBACK
void lxs_core_traceback(lua_State* const L);
#endif

#if LUAXS_CORE_ATEXIT
void lxs_core_atexit(lua_State* const L, bool from_error, bool from_pcall);
#endif

#if LUAXS_CORE_LUASTACK
void lxs_core_luastack(lua_State* const L);
#endif

#endif // LUAXS_EXTEND_CORE


#if LUAXS_EXTENT_SCRIPTS_ONLY

XS_AINLINE static void lxs_incr_newstate_cnt(lua_State* const L);
XS_AINLINE static bool lxs_canextend(lua_State* const L);

#else

XS_AINLINE bool lxs_canextend(lua_State* const L);

#endif // LUAXS_EXTENT_SCRIPTS_ONLY




//==============================================================================
// C++ APIs
//==============================================================================

#ifdef __cplusplus
extern "C++" {

#include "leastl.hpp"
#include <eastl/hash_map.h>



namespace lxs
{
#if LUAXS_CORE_TRACEBACK_EX
    typedef eastl::hash_map<lua_CFunction, const char*> stdfun_map;
    typedef eastl::hash_map<const char*, stdfun_map>    stdlib_map; 
    typedef eastl::pair<const char*, const char*>       stdfun_name;

    stdfun_map& reg_lib(lua_State* const L, const char* name, int size);
    void        reg_fun(lua_State* const L, stdfun_map& lib, const char* name, lua_CFunction func);

    //void reg_ud(lua_State* const L, const void* ud);
    //void reg_ud(lua_State* const L, const void* ud, const char* meta, size_t len);
    //bool reg_find_ud(lua_State* const L, const void* ud);
    //bool reg_find_ud(lua_State* const L, const char* meta, size_t len);

    bool reg_find_name(lua_State* const L, lua_CFunction fun, stdfun_name& out);
    bool reg_find_name(lua_State* const L, const char* lib, lua_CFunction fun, stdfun_name& out);
#endif // LUAXS_CORE_TRACEBACK_EX
} // ns xs

};
#endif // __cplusplus











//==============================================================================
// API implementations
//==============================================================================

static lua_State* s_mt = NULL;

void lxs_set_mt(lua_State* const L)
{
    lxs_assert(L, L);

    /// The game creates 2 Lua states; the last created state is the one
    /// used to execute scripts.
    /// Could be that the other state is LuaJIT's compiler state.
    //assert(s_mt == NULL);

    s_mt = L;
}

lua_State* lxs_mt()
{
    lxs_assert(s_mt, s_mt);

    return s_mt;
}




int abs_index_m1(lua_State* const L, int idx)
{
    return (idx > 0 || idx <= LUA_REGISTRYINDEX
        ? idx
        : lua_gettop(L) + idx);
}

void lxs_rawget(lua_State* const L, int idx, const char* key, size_t len)
{
    lxs_assert(L, L);
    lxs_assert(L, key);
    lxs_assert(L, len > 0 && len <= strlen(key));

    lua_pushlstring(L, key, len);
    lua_rawget(L, abs_index_m1(L, idx));
};

void lxs_rawset(lua_State* const L, int idx, const char* key, size_t len)
{
    lxs_assert(L, L);
    lxs_assert(L, key);
    lxs_assert(L, len > 0 && len <= strlen(key));

    lua_pushlstring(L, key, len);
    lua_rawsetr(L, abs_index_m1(L, idx));
};







XS_AINLINE static double _lxs_minf(double lhs, double rhs)
{
    return (lhs < rhs) ? lhs : rhs;
}
XS_AINLINE static double _lxs_maxf(double lhs, double rhs)
{
    return (lhs > rhs) ? lhs : rhs;
}

static lua_Number lxs_mind(lua_State* const L, int idx, lua_Number rhs)
{
    return _lxs_minf(luaL_checknumber(L, idx), rhs);
}
static lua_Number lxs_maxd(lua_State* const L, int idx, lua_Number rhs)
{
    return _lxs_maxf(luaL_checknumber(L, idx), rhs);
}

static lua_Integer lxs_min(lua_State* const L, int idx, lua_Integer rhs)
{
    lua_Integer i;
    lua_Number  n = _lxs_minf(luaL_checkinteger(L, idx), rhs);
    lua_number2int(i, n);
    return i;
}

static lua_Integer lxs_max(lua_State* const L, int idx, lua_Integer rhs)
{
    lua_Integer i;
    lua_Number  n = _lxs_maxf(luaL_checkinteger(L, idx), rhs);
    lua_number2int(i, n);
    return i;
}




#if LUAXS_EXTENT_SCRIPTS_ONLY

static uint8_t s_newstate_count = 0;

void lxs_incr_newstate_cnt(lua_State* const L)
{
    ++s_newstate_count;
}

bool lxs_canextend(lua_State* const L)
{
    //CDBG(L, "counter: %d, extendable: %s",
    //    s_newstate_count,
    //    (s_newstate_count > 1 ? "yes" : "no")
    //);
    return s_newstate_count > 1;
}

#else // !LUAXS_EXTENT_SCRIPTS_ONLY

bool lxs_canextend(lua_State* const L)
{
    UNUSED(L);
    return true;
}

#endif // LUAXS_EXTENT_SCRIPTS_ONLY





//==============================================================================
// Macros
//==============================================================================


#define lxs_rawgetl(L, idx, key)                                               \
    lxs_rawget(L, idx, "" key, sizeof(key) / sizeof(key[0]) - 1)
#define lxs_rawsetl(L, idx, key)                                               \
    lxs_rawset(L, idx, "" key, sizeof(key) / sizeof(key[0]) - 1)

#define lxs_error(L, fmt, ...)                                                 \
    (luaL_error(L,                                                             \
                "[ERROR (%p, `%s' (%s:%d)]:  " fmt,                            \
                L, lxs_tersesource(__FILE__), __FUNCTION__, __LINE__,          \
                __VA_ARGS__)                                                   \
    )




#endif // lxs_api_h