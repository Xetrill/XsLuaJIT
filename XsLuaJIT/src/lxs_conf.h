#ifndef lxs_conf_h
#define lxs_conf_h 1


////////////////////////////////////////////////////////////////////////////////
/// LUAXS_DEBUG
/// 
/// Defined as 0/1 or undefined.
/// Enables debug functions and macros, most notably this affects lxs_assert().
/// 
/// This setting is not related to NDEBUG or project configuration.
///
/// Macros:
///     lxs_assert(L, cond)
///
///     lxs_assert_stack_begin(L)
///         Used in conjunction with lxs_assert_stack_end() to verify how a
///         function increases or decreases stack size.
///         This macro captures the current stack size in a variable named
///         '__top__'.
///         
///     lxs_assert_stack_end(L, n)
///         Used in conjunction with lxs_assert_stack_begin() to verify how a
///         function increases or decreases stack size.
///         This macro verifies the earlier captured stack size with a specified
///         delta, if the delta should not much with the current stack size the
///         assertion triggers/fails.
///         
///         Usage example:
///         int get_pi(lua_State const *L)
///         {
///             lxs_assert_stack_begin(L);  // top = n
///             lua_pushinteger(L, M_PI);
///             lxs_assert_stack_end(L, 1); // top = n + 1
///             return 1;
///         }
///
///     lxs_assert_stack_at(L, n)
///         Verifies the current stack size matches a specified number.
///
#ifndef LUAXS_DEBUG
    #define LUAXS_DEBUG 1
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_CLOG
/// 
/// Defined as 0/1 or undefined.
/// Controls logging macros.
/// 
/// Macros:
///     CLOG(L, fmt, ...)
///     CERR(L, fmt, ...)
///     CWRN(L, fmt, ...)
///     CIFO(L, fmt, ...)
///     CDBG(L, fmt, ...)
///         Writes a single formatted message to LUAXS_CLOG_STREAM.
///         The message always contains the current Lua state (pointer address),
///         the current file and line number as well as the current function
///         name.
///         
///         CDBG messages will only be printed/compiled if LUAXS_DEBUG is 1.
///         
///     CLOG_STACK(L)
///         Dumps Lua's stack in a human readable form to CLOG_STREAM.
///         Stack numbers are provided absolute and relative.
///         The stack is iterated from its bottom up.
///     CLOG_STACK_REV(L)
///         Sames as CLOG_STACK(), except in reverse.
///
///     CLOG_FLUSH(L)
///         Flushes the LUAXS_CLOG_STREAM buffer.
///         When LUAXS_DEBUG is 1, the buffer gets  flushed as soon as new data
///         is available.
///         
///     lxs_assert(L, cond)
///         Modifies lxs_assert() to log failed assertions via CERR().
///
/// This option is expected to be enabled (or undefined).
/// 
#ifndef LUAXS_CLOG
    #define LUAXS_CLOG 1
#endif

////////////////////////////////////////////////////////////////////////////////
/// LUAXS_CLOG_STREAM (LUAXS_CLOG)
/// 
/// Defined as a global FILE* variable or function call or undefined.
/// Specifies the stream that is used for messaging and logging. This can be
/// a function call returning a FILE* structure and likewise can just be stdout.
///
#ifndef LUAXS_CLOG_STREAM
    #define LUAXS_CLOG_STREAM stdout
#endif


////////////////////////////////////////////////////////////////////////////////
/// LUAXS_EXTENT_SCRIPTS_ONLY
/// 
/// Defined as 0/1 or undefined.
/// LuaJIT creates two lua_State instances, and if this option is enabled only
/// the state which is used to process scripts will be extended.
/// 
/// The other state is used by the jitter to compile traces.
///
/// There shouldn't be any need to disable this.
///
#ifndef LUAXS_EXTENT_SCRIPTS_ONLY
    #define LUAXS_EXTENT_SCRIPTS_ONLY 1
#endif


////////////////////////////////////////////////////////////////////////////////
/// LUAXS_STR_LJ2_CTYPE
/// 
/// Defined to 0/1 or undefined.
/// Changes the implementation used for character type operations.
/// When set to 0, the CRT library 'ctype.h' is used.
/// When set to 1, the same implementation as in LuaJIT 2 is used.
/// 
/// Disabled by default, as my benchmarks for 'tolower' and 'isdigit' showed
/// that at best the LJ2 implementation gets even to the CRT.
///
#ifndef LUAXS_STR_LJ2_CTYPE
    #define LUAXS_STR_LJ2_CTYPE 0
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_STR_INITIAL_CAPACITY
///
/// Defined to a positive integer which is a power of 2 or undefined.
/// Specifies the amount of memory in bytes which are allocated if not
/// explicitly specified.
/// This value must be positive and should be a power of 2.
/// 
#ifndef LUAXS_STR_INITIAL_CAPACITY
    #define LUAXS_STR_INITIAL_CAPACITY 256
#endif


////////////////////////////////////////////////////////////////////////////////
/// LUAXS_STR_GROWTH_FACTOR
/// 
/// Defined to a positive single greater than 1.0 or undefined.
/// Buffer re-allocation growth factor. The buffers current size is multiplied
/// with this value and then the rounded to the nearest power of 2.
/// 
#ifndef LUAXS_STR_GROWTH_FACTOR
    #define LUAXS_STR_GROWTH_FACTOR 1.5f
#endif


////////////////////////////////////////////////////////////////////////////////
/// LUAXS_STR_READONLY_OPTIONS
///
/// Defined as 0/1 or undefined.
/// Handles if 'initial capacity' and 'growth factor' can be changed globally
/// for a lua_State from Lua.
/// If enabled 'default_capacity' and 'growth_factor' are pushes as constants
/// to the buffer library.
/// If disabled 'default_capacity' and 'growth_factor' are functions, getters
/// and setters.
/// The setters cannot specify arbitrary values; 'growth_factor' must be grater
/// 1.0 and 'default_capacity' is checked against LUAXS_STR_MAX_SINGLE_EXPANSION
/// and LUAXS_STR_MAX_TOTAL_EXPANSION unless they are set to 0.
/// 
#ifndef LUAXS_STR_READONLY_OPTIONS
    #define LUAXS_STR_READONLY_OPTIONS 1
#endif


////////////////////////////////////////////////////////////////////////////////
/// LUAXS_STR_MAX_SINGLE_EXPANSION
/// 
/// Defined to a positive integer including zero (to disable) or undefined.
/// Specifies the largest possible single reallocation that is permitted.
/// This affects every lxs_string function that can request additional memory.
/// 
/// Using a value of 0 disables this check.
/// 
#ifndef LUAXS_STR_MAX_SINGLE_EXPANSION
    #define LUAXS_STR_MAX_SINGLE_EXPANSION 10240
#endif


////////////////////////////////////////////////////////////////////////////////
/// LUAXS_STR_MAX_TOTAL_EXPANSION
/// 
/// Defined to a positive integer including zero (to disable) or undefined.
/// Specifies the overall largest possible single (re)allocation that is
/// permitted.
/// 
/// Using a value of 0 disables this check.
/// 
#ifndef LUAXS_STR_MAX_TOTAL_EXPANSION
    #define LUAXS_STR_MAX_TOTAL_EXPANSION 40960
#endif


////////////////////////////////////////////////////////////////////////////////
/// LUAXS_STR_PERSISTENT_BUFFER
/// 
/// EXPERIMENTAL: Currently enabling this results in a crash when quitting.
///
/// If set to 1, a persistent buffer is created; much like LuaJIT 2 does.
/// Which is then available for LuaJIT's internal operations and can act as a
/// viable replacement luaL_Buffer with dynamic memory management.
/// Currently a few functions in lstrlib and ltablib use this.
/// 
#ifndef LUAXS_STR_PERSISTENT_BUFFER
    #define LUAXS_STR_PERSISTENT_BUFFER 0
#endif

////////////////////////////////////////////////////////////////////////////////
/// LUAXS_PB_INITIAL_CAPACITY (LUAXS_STR_PERSISTENT_BUFFER)
///
/// Defined to a positive power of 2 integer or undefined.
/// Specifies the default allocation size of persistent buffers.
/// 
#ifndef LUAXS_PB_INITIAL_CAPACITY
    #define LUAXS_PB_INITIAL_CAPACITY 1024
#endif

////////////////////////////////////////////////////////////////////////////////
/// LUAXS_PB_ONDEMAND_FIBERS (LUAXS_STR_PERSISTENT_BUFFER)
/// 
/// Defined as 0/1 or undefined.
/// Changes how persistent buffers are allocated for Lua threads (coroutines).
/// If enabled persistent buffers are only allocated when needed.
/// If disabled persistent buffers are always allocated.
/// 
#ifndef LUAXS_PB_ONDEMAND_FIBERS
    #define LUAXS_PB_ONDEMAND_FIBERS 0
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_EASTL_LUAM_MALLOC
///
/// EXPERIMENTAL: In its current state luaM_* functions are not usable.
/// 
/// Defined as 0/1 or undefined.
/// Changes how the EASTL allocator works (leastl.hpp; LuaAllocator:allocator).
/// If enabled, it will use luaM_malloc/luaM_freemem -- this method does not
/// support aligned and/or offset allocations.
/// If disabled, it will use _aligned_offset_malloc/_aligned_free.
/// 
#ifndef LUAXS_EASTL_LUAM_MALLOC
    #define LUAXS_EASTL_LUAM_MALLOC 0
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_EXTEND_CORE
/// 
/// Defined as 0/1 or undefined.
/// Global switch to enable or disable all extensions.
/// See LUAXS_EXTEND_CORE_* for specific options/features.
/// 
#ifndef LUAXS_EXTEND_CORE
    #define LUAXS_EXTEND_CORE 1
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_CORE_TERSE
/// 
/// Defined as 0/1 or undefined.
/// Makes Lua more terse, by omitted useless information. Useless means full
/// file paths; they are useless because the root can only be where the game
/// is installed.
///
/// This changes luaO_chunkid(), specifically how it handles lua_Debug.short_src.
/// 
#ifndef LUAXS_CORE_TERSE
    #define LUAXS_CORE_TERSE 1
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_CORE_LUASTACK
///
/// Defined as 0/1 or undefined.
/// Changes how Lua outputs error information.
/// If enabled, an attempt is made to append a Lua stack representation to
/// to the error message/output.
/// 
/// To help with understanding the new information the whole Lua library is
/// printed to stdout, if LUAXS_CLOG is enabled as well. This allows for
/// otherwise meaningless function pointer addresses to be mapped to a function.
///
#ifndef LUAXS_CORE_LUASTACK
    #define LUAXS_CORE_LUASTACK 1
#endif

////////////////////////////////////////////////////////////////////////////////
/// LUA_LUASTACK_TERSE (LUAXS_CORE_LUASTACK)
/// 
/// Defined as a positive integer including zero or undefined.
/// If enabled (greater zero), truncates long strings down to the specified
/// value.
/// A value of 77 will result in a column break at column 80.
/// If disabled (zero), strings will be printed however long they are.
/// 
#ifndef LUA_LUASTACK_TERSE
    #define LUA_LUASTACK_TERSE 0
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_CORE_ATEXIT
///
/// Defined to 0/1 or undefined.
/// This allows for a loose global hook to be defined from Lua.
/// If enabled, whenever Lua exits (no matter how) it will try to locate a
/// function called 'atexit' in the global scope (_G/LUA_GLOBALSINDEX) and if so
/// it is called providing the best possible information on why or what failed.
/// If disabled, nothing happens.
/// 
/// The function signature is as follows:
///     atexit(errobj, from_error, from_pcall)
///         errobj: undefined; whatever is at the top of the stack.
///             In almost all cases this will a be error message.
///         from_error: boolean; whether an error occurred or not.
///             If Lua exits gracefully, this will be false otherwise true.
///         from_pcall: boolean; whether Lua is running in protected mode.
///             Protected/sandbox mode is used by Lua's pcall and xpcall
///             functions.
///
/// Usage example:
///     function atexit(errobj, from_error, from_pcall)
///         local f = io.stdout
///         f:write('\n=== atexit ===\n')
///         f:write('  errobj:     ', type(errobj), '\n')
///         f:write('  from_error: ', tostring(from_error), '\n')
///         f:write('  from_pcall: ', tostring(from_pcall), '\n')
///         f:write('  tostring(errobj):  \'', tostring(errobj) , '\'\n')
///         f:write('=== /atexit ===\n')
///         f:flush()
///     end
/// 
#ifndef LUAXS_CORE_ATEXIT
    #define LUAXS_CORE_ATEXIT 1
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_CORE_TRACEBACK_EX
///
/// Defined to 0/1 or undefined.
///
#ifndef LUAXS_CORE_TRACEBACK_EX
    #define LUAXS_CORE_TRACEBACK_EX 1
#endif

#ifndef LUAXS_CORE_TRACEBACK_EX_NAME
    #define LUAXS_CORE_TRACEBACK_EX_NAME "stdlib"
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_CORE_TRACEBACK
///
/// EXPERIMENTAL: This feature is not fully functional.
///
/// Defined to 0/1 or undefined.
/// Changes (or rather attempts to change) what output is provided when an error
/// occurs.
/// If enabled, an implicit call to debug.traceback() is made and it's output
/// appended to whatever error message exists.
/// If disabled, nothing happens; vanilla behavior.
/// 
/// While it works, most of the time, the times when it doesn't, it hides error
/// messages. LuaUnit might be the best example of this, as soon as this option
/// is enabled, LuaUnit will only spit out 'function: <pointer>' instead of what
/// assertion actually failed.
/// 
#ifndef LUAXS_CORE_TRACEBACK
    #define LUAXS_CORE_TRACEBACK 0
#endif

////////////////////////////////////////////////////////////////////////////////
/// LUAXS_TRACEBACK_TOSTRING (LUAXS_CORE_TRACEBACK)
///
/// Defined to 0/1 or undefined.
/// Only relevant if LUAXS_EXTEND_CORE and LUAXS_CORE_TRACEBACK are defined.
/// If set to 1, Lua's tostring() function is called if the error object isn't a
/// string.
/// 
#ifndef LUAXS_TRACEBACK_TOSTRING
    #define LUAXS_TRACEBACK_TOSTRING 0
#endif

////////////////////////////////////////////////////////////////////////////////
/// LUAXS_TRACEBACK_FROMREGISTRY (LUAXS_CORE_TRACEBACK)
/// 
/// Defined to 0/1 or undefined.
/// When enabled, the debug.traceback function is saved in the Lua registry
/// table. Which somewhat stabilizes the traceback feature. Because Lua allows
/// function to be overwritten.
/// It's still possible through the debug library to do just that, but not as
/// accessible.
/// 
#ifndef LUAXS_TRACEBACK_FROMREGISTRY
#define LUAXS_TRACEBACK_FROMREGISTRY 1
#endif

////////////////////////////////////////////////////////////////////////////////
/// LUAXS_TRACEBACK_REGISTRYKEY (LUAXS_TRACEBACK_FROMREGISTRY)
/// 
/// Defined to a unique string or undefined.
/// Specifies the string which is used to store the debug.traceback function
/// in the Lua registry table.
/// As this is a key string, it has to be unique.
/// 
#ifndef LUAXS_TRACEBACK_REGISTRYKEY
    #define LUAXS_TRACEBACK_REGISTRYKEY "xs_traceback"
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_CORE_SIMD
/// 
/// EXPERIMENTAL: This feature is not fully implemented nor tested. 
///
/// Defined to 0/1 or undefined.
/// If enabled, some Lua's internals use SIMD instructions instead of the CRT.
/// If disabled, C's math.h is used.
/// Note however that regardless of this option SSE3 is required (fisttp).
///
/// IMPORTANT: This define is controlled via project configuration.
/// 
//#define LUAXS_CORE_SIMD 0



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_CORE_FORMAT
///
/// Defined as 0/1/2 or undefined.
/// Changes how string formatting is implemented in Lua's core internals.
/// It does so by changing the implementation in luaO_pushvfstring.
/// 
/// Allowed values:
///   0: No changes are made; vanilla behavior
///   1: support for %x and %X is added
///   2: the whole implementation is swapped out and vsprintf is called directly
///      and only once, instead of for each and every single token
/// EXPERIMENTAL: Needs to be considered unstable, because I have to assume that
///               the original implementation was needed. At least at some point
///               during Lua's development.
///               But as it's now, I can't see any reason for re-implementing
///               printf; even less so by using printf.
///
#ifndef LUAXS_CORE_FORMAT
    #define LUAXS_CORE_FORMAT 2
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_EXTEND_*
///
/// Defined to 0/1 or undefined.
/// LUAXS_EXTEND_* extend a specific library. Adding additional functionality.
/// Mostly this just means new functions.
/// 
/// LUAXS_EXTEND_DBLIB:
/// LUAXS_EXTEND_IOLIB:
/// LUAXS_EXTEND_MATHLIB:
/// LUAXS_EXTEND_STRLIB:
/// LUAXS_EXTEND_TABLIB:
/// 
#ifndef LUAXS_EXTEND_DBLIB
    #define LUAXS_EXTEND_DBLIB 1
#endif
#ifndef LUAXS_EXTEND_IOLIB
    #define LUAXS_EXTEND_IOLIB 1
#endif
#ifndef LUAXS_EXTEND_MATHLIB
    #define LUAXS_EXTEND_MATHLIB 1
#endif
#ifndef LUAXS_EXTEND_STRLIB
    #define LUAXS_EXTEND_STRLIB 1
#endif
#ifndef LUAXS_EXTEND_TABLIB
    #define LUAXS_EXTEND_TABLIB 1
#endif
//#define LUAXS_EXTEND_BASELIB 1
//#define LUAXS_EXTEND_OSLIB 1



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_ADDLIB_*
/// 
/// Defined to 0/1 or undefined.
/// Each LUAXS_ADDLIB_* specifies if new non-default libraries shall be added
/// to Lua's library repository or not.
///
/// Unless otherwise specified, each new library luaopen_* function is called
/// from luaopen_jit, as it's last operation.
/// This is because, as it happens, STALKER loads the JITTER last.
/// 
/// LUAXS_ADDLIB_BUFFER:
///     Implements lxs_string as a Lua library thus providing a mutable string
///     buffer.
///     Which can outperform many vanilla string operations, especially those
///     which require many writes.
///
/// LUAXS_ADDLIB_MARSHAL:
///     Provides a serialization library, which is aware of cycles and 
///     functions as well as fast.
///     One caveat though is, it serializes to a string and doesn't provide
///     capabilities to instead use a file.
///
/// LUAXS_ADDLIB_GAME:
///
#ifndef LUAXS_ADDLIB_BUFFER
    #define LUAXS_ADDLIB_BUFFER 1
#endif
#ifndef LUAXS_ADDLIB_MARSHAL
    #define LUAXS_ADDLIB_MARSHAL 1
#endif
#ifndef LUAXS_ADDLIB_CONTAINER
    #define LUAXS_ADDLIB_CONTAINER 0
#endif
#ifndef LUAXS_ADDLIB_GAME
    #define LUAXS_ADDLIB_GAME 0
#endif
#ifndef LUAXS_ADDLIB_MEMORY
    #define LUAXS_ADDLIB_MEMORY 0
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_STREAMLINE_*
///
/// Defined to 0/1 or undefined.
/// Streamlines a specific library. Typically this means
/// code/functionality that would never be used in STALKER is removed.
///
/// LUAXS_STREAMLINE_LOADLIB:
///     loadlib.c: require() is only using 3 loader functions instead of 4.
/// 
#ifndef LUAXS_STREAMLINE_LOADLIB
    #define LUAXS_STREAMLINE_LOADLIB 1
#endif



////////////////////////////////////////////////////////////////////////////////
/// LUAXS_MATHLIB_SIMD
///
/// EXPERIMENTAL: This feature is not fully implemented nor tested.
/// 
/// Defined to 0/1 or undefined.
/// Changes some implementations for Lua's math library to use SIMD instructions.
/// Support is CPU dependent.
/// 
/// IMPORTANT: This define is controlled via project configuration.
/// 
//#define LUAXS_MATHLIB_SIMD 0



//------------------------------------------------------------------------------

#if defined(LUA_CORE) && LUAXS_CORE_SIMD
#  include "lxsext_simd.h"
#endif



//------------------------------------------------------------------------------

#if LUAXS_STR_PERSISTENT_BUFFER && !defined(LUAXS_PB_INITIAL_CAPACITY)
#  error LUAXS_PB_INITIAL_CAPACITY needs to be defined
#endif

#if defined(LUAXS_STR_MAX_SINGLE_EXPANSION) && \
    defined(LUAXS_STR_MAX_TOTAL_EXPANSION)
#  if LUAXS_STR_MAX_TOTAL_EXPANSION <= LUAXS_STR_MAX_SINGLE_EXPANSION
#    error LUAXS_STR_MAX_TOTAL_EXPANSION must be greater than\
    LUAXS_STR_MAX_SINGLE_EXPANSION
#  endif
#endif
#ifndef LUAXS_STR_INITIAL_CAPACITY
#  error LUAXS_STR_INITIAL_CAPACITY needs to be defined
#endif
#ifndef LUAXS_STR_GROWTH_FACTOR
#  error LUAXS_STR_GROWTH_FACTOR needs to be defined
#endif

#if !defined(LUAXS_CORE_FORMAT) || \
    LUAXS_CORE_FORMAT < 0       || \
    LUAXS_CORE_FORMAT > 2
#  error Invalid LUAXS_CORE_FORMAT value defined; use 0, 1 or 2
#endif

#endif // lxs_conf_h
