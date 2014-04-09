#ifndef lxs_def_h
#define lxs_def_h 1

#include <stdio.h>
#include <string.h>

#include "lxs_conf.h"
#include "lxsext_clog.h"


#ifdef __cplusplus
#  define XS_BEGIN_EXTERN_C   extern "C" {
#  define XS_END_EXTERN_C     };
#  define XS_BEGIN_EXTERN_CPP extern "C++" {
#  define XS_END_EXTERN_CPP   };
#else
#  define XS_BEGIN_EXTERN_C
#  define XS_END_EXTERN_C
#  define XS_BEGIN_EXTCPP
#  define XS_END_EXTCPP
#endif



#define XS_INLINE  __inline
#define XS_AINLINE __forceinline


#ifndef SIZE_T_MAX
#  define SIZE_T_MAX ((size_t)-1)
#endif

#ifndef MAX_PATH
#  define MAX_PATH 260
#endif

#ifdef __cplusplus
#  define STATIC_CAST(type,expr)      (static_cast<type>(expr))
#  define REINTERPRET_CAST(type,expr) (reinterpret_cast<type>(expr))
#  define CONST_CAST(type,expr)       (const_cast<type>(expr))
#  define DEFVAL(name, constant) name = constant
#else
#  define STATIC_CAST(type,expr)      ((type)(expr))
#  define REINTERPRET_CAST(type,expr) ((type)(expr))
#  define CONST_CAST(type,expr)       ((type)(expr))
#  define DEFVAL(name, constant) name
#endif


#ifndef __cplusplus
typedef enum { false, true } bool;
#endif





//------------------------------------------------------------------------------


#ifndef CLOG_STREAM
#  define CLOG_STREAM LUAXS_CLOG_STREAM
#endif

#define _XS_CLOG_PRINTFLN(L, type, msg, ...)                                   \
    (L, fprintf(CLOG_STREAM,                                                   \
        "[" type " (%p, `%s' (%s:%d)]:  " msg "\n",                            \
        L, lxs_tersesource(__FILE__), __FUNCTION__, __LINE__,                  \
        __VA_ARGS__)                                                           \
    )

#define _CLOG_STACK(L, ORDER_TOP2BOTTOM)                                       \
    (lxs_dumpstack(L,                                                          \
                   lxs_tersesource(__FILE__),                                  \
                   __FUNCTION__,                                               \
                   __LINE__,                                                   \
                   ORDER_TOP2BOTTOM)                                           \
    )

#if LUAXS_DEBUG
#  define CLOG_FLUSH(L)     (fflush(CLOG_STREAM))
#  define CLOG_STACK(L)     _CLOG_STACK(L, true)
#  define CLOG_STACK_REV(L) _CLOG_STACK(L, false)
#  define CERR(L, msg, ...) _XS_CLOG_PRINTFLN(L, "E", "" msg, __VA_ARGS__)
#  define CWRN(L, msg, ...) _XS_CLOG_PRINTFLN(L, "W", "" msg, __VA_ARGS__)
#  define CIFO(L, msg, ...) _XS_CLOG_PRINTFLN(L, "I", "" msg, __VA_ARGS__)
#  define CLOG(L, msg, ...) _XS_CLOG_PRINTFLN(L, "M", "" msg, __VA_ARGS__)
#  define CDBG(L, msg, ...) {                                                  \
        _XS_CLOG_PRINTFLN(L, "D", "" msg, __VA_ARGS__);                        \
        CLOG_FLUSH(L);                                                         \
    }
#  define CLOG_MARK(L) CDBG(L, "MARK #%d", __COUNTER__)
#else
#  define CERR(L, msg, ...)  ((void)0)
#  define CWRN(L, msg, ...)  ((void)0)
#  define CIFO(L, msg, ...)  ((void)0)
#  define CDBG(L, msg, ...)  ((void)0)
#  define CLOG(L, msg, ...)  ((void)0)
#  define CLOG_STACK(L)      ((void)0)
#  define CLOG_STACK_REV(L)  ((void)0)
#  define CLOG_FLUSH(L)      ((void)0)
#  define CDBG(L, msg, ...)  ((void)0)
#  define CLOG_MARK(L)       ((void)0)
#endif // LUAXS_CLOG

#if LUAXS_DEBUG
#  define lxs_assert_stack_begin(L)                                            \
    int __top__ = lua_gettop((L))
#  define lxs_assert_stack_end(L, n)                                           \
    lxs_assert((L), lua_gettop((L)) == __top__ + (n))
#  define lxs_assert_stack_at(L, n)                                            \
    lxs_assert((L), lua_gettop((L)) == (n))

#  if LUAXS_CLOG
#    define lxs_assert(L, expr)                                                \
    do                                                                         \
{                                                                              \
    if (!(expr))                                                               \
{                                                                              \
    CERR((L), "Assertion Failed: `" #expr "'");                                \
    CLOG_STACK(L);                                                             \
    assert(0 && #expr);                                                        \
}                                                                              \
} while (0)
#  else // LUAXS_CLOG
#    define lxs_assert(L, expr) { (void)L; assert(expr); }
#  endif // LUAXS_CLOG
#else
#  define lxs_assert(L, expr)        ((void)0)
#  define lxs_assert_stack_begin(L)  ((void)0)
#  define lxs_assert_stack_end(L, n) ((void)0)
#  define lxs_assert_stack_at(L, n)  ((void)0)
#endif // LUAXS_DEBUG












#endif // lxs_def_h
