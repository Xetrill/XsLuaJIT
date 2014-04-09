#define lxs_buffer_cpp
#define LUA_LIB

extern "C"
{

#include "luajit.h"

#if LUAXS_ADDLIB_BUFFER

#include "lauxlib.h"
#include "lualib.h"

#include "lxs_string.hpp"


//==============================================================================

static __inline void check_offset(lua_State* const L,
                                  lxs_string* const s,
                                  size_t real_offset,
                                  int lua_offset)
{
    lxs_assert_stack_begin(L);

    if (real_offset > lxs_slen(s))
        lxs_error(L,
            "offset (%d) out of range (1-%u)",
            lua_offset,
            lxs_slen(s)
        );
    lxs_assert_stack_end(L, 0);
}

static __inline void check_length(lua_State* const L,
                                  lxs_string* const s,
                                  size_t real_offset,
                                  size_t real_length,
                                  int lua_length)
{
    lxs_assert_stack_begin(L);

    if (real_offset + real_length > lxs_slen(s))
        lxs_error(L,
            "length (%d) out of range (1-%u)",
            lua_length,
            lxs_slen(s)
        );
    lxs_assert_stack_end(L, 0);
}

static __inline size_t opt_offset(lua_State* const L,
                                  lxs_string* const s,
                                  int narg)
{
    lxs_assert_stack_begin(L);

    size_t real_offset = 0;

    int offset = luaL_optinteger(L, narg, 1);
    if (offset > 0)         // access from the left/beginning of the buffer
        real_offset = static_cast<size_t>(offset) - 1u;
    else if (offset < 0)    // access from the right/end of the buffer
        real_offset = lxs_slen(s) + offset;
    else
        luaL_argerror(L, narg, "cannot be zero");

    check_offset(L, s, real_offset, offset);

    lxs_assert_stack_end(L, 0);
    return real_offset;
}

static __inline size_t opt_length(lua_State* const L,
                                  lxs_string* const s,
                                  size_t real_offset,
                                  int narg)
{
    lxs_assert_stack_begin(L);

    size_t real_length = 0;

    int length = luaL_optinteger(L, narg, 0);
    if (length == 0)
        real_length = lxs_slen(s);
    else if (length > 0)
        real_length = static_cast<size_t>(length);
    else
        luaL_argerror(L, narg, "cannot be less than zero");

    check_length(L, s, real_offset, real_length, length);

    lxs_assert_stack_end(L, 0);
    return real_length;
}


//------------------------------------------------------------------------------

/// buffer.cap(buffer, [size, [force = true]])
/// _buffer_:cap([size, [force = true]])
///
/// Returns the specified *buffer*'s current memory capacity; that is how much
/// memory it has allocated.
///
/// If the optional argument *size* is specified, allows changing the *buffer*'s
/// capacity. The optional third argument *force* changes how that is done.
/// When *force* is true, *size* will be used as is, otherwise if *force* is
/// false *size* will be used as a baseline minimum request, and rounded up to
/// the nearest power of 2 if necessary.
///
/// Example Usage:
///     local b = buffer.new()
///     b:cap(100) --> will allocate 100 bytes of memory
///     b:cap(0) --> will deallocate all previously allocated memory
static int libE_cap(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_scheckbuffer(L, 1);
    if (lua_isnumber(L, 2))
    {
        int msize = lua_tointeger(L, 2);
        luaL_argcheck(L, msize >= 0, 2, "must be greater or equal to zero");

        bool force = luaL_optbool(L, 3, true);

        lxs_srealloc(L, s, static_cast<size_t>(msize), force);
    }
    lua_pushinteger(L, static_cast<lua_Integer>(lxs_scap(s)));

    lxs_assert_stack_end(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
/// TODO: this implementation is stupid..., change to a simple setter/getter
///
/// <code>
/// b = buffer.new()
/// b:len(100) -- will expand the buffer, filling the new space with spaces
/// b:len(-99) -- will shrink the buffer
/// b:len(0)   -- will truncate the buffer
/// </code>
static int libE_len(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_scheckbuffer(L, 1);
    if (lua_isnumber(L, 2))
    {
        int delta = luaL_checkinteger(L, 2);
        if (delta == 0)
            lxs_sclear(L, s);
        else if (delta < 0)
            lxs_sshrink(L, s, abs(delta));
        else
            lxs_sexpand(L, s, delta);
    }
    lua_pushinteger(L, static_cast<lua_Integer>(lxs_slen(s)));

    lxs_assert_stack_end(L, 1);
    return 1;
}

/// buffer.clear(b)
/// buffer:clear()
///
/// Returns the *buffer* it was called on.
/// Sets a *buffer*'s internal cursor back to zero, thus freeing up all its memory
/// to be used again.
/// Use this to reset a buffer back to its original state, except for its memory
/// capacity.
///
/// Example Usage:
static int libE_clear(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_sclear(L, lxs_scheckbuffer(L, 1));

    lxs_assert_stack_end(L, 0);
    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.new([capacity])
/// buffer:new([capacity])
///
/// Creates and returns a new buffer, with either the default capacity or
/// optionally the *capacity* supplient.
///
/// Example Usage:
///     local b = buffer.new() --> b is a new buffer with default capacity
///     local b = buffer.new(32) b is a new buffer with 32 bytes of capacity
static int libE_new(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_snewbuffer(L, static_cast<size_t>(luaL_optinteger(L, 1, 0)));

    lxs_assert_stack_end(L, 1);
    return 1;
}

/// buffer.isbuffer(object)
///
/// Tests whether the supplient *object* is a buffer or not.
///
/// Example Usage:
///     buffer.isbuffer("string") --> false
///     buffer.isbuffer(buffer.new()) --> true
static int libL_isbuffer(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lua_pushboolean(L, lxs_sisbuffer(L, 1) ? 1 : 0);

    lxs_assert_stack_end(L, 1);
    return 1;
}

/// buffer.equals(lhs, rhs)
/// _buffer_:equals(other)
///
/// Tests whether two buffers or strings are equal or not.
/// Equality is determined via content length and rawequal.
///
/// Example Usage:
///     buffer.equals(buffer.new(), buffer.new()) --> true
///     buffer.equals(buffer.new(), "") --> true
///     local s = "abc"
///     local b = buffer.new()
///     b:equals(s) --> false
///     b:append(s)
///     b:equals(s) --> true
static int libE_equals(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    size_t      lhs_len  = 0;
    const char* lhs      = lxs_schecklstring(L, 1, &lhs_len);
    size_t      rhs_len  = 0;
    const char* rhs      = lxs_schecklstring(L, 2, &rhs_len);

    if (lhs_len != rhs_len)
    {
        lua_pushboolean(L, 0);
        lxs_assert_stack_end(L, 1);
        return 1;
    }

    lua_pushlstring(L, lhs, lhs_len);
    lua_pushlstring(L, rhs, rhs_len);
    lua_pushboolean(L, lua_rawequal(L, -1, -2));

    lxs_assert_stack_end(L, 3);
    return 1;
}

/// buffer.(b[, offset, [length]])
/// buffer:([offset, [length]])
///
/// Returns the *buffer* it was called on.
/// Changes all uppercase letters of the selected range to lowercase. The
/// default range is the whole buffer content, you can specifiy a custom range
/// by using *offset* and *length*.
/// The definition of what an uppercase letter is depends on the current locale.
///
/// Example Usage:
///     local b = buffer.new()
///     b:append("ABC"):tolower():tostring() --> "abc"
///     local b = buffer.new()
///     b:append("ABC"):tolower(1, 1):tostring() --> "aBC"
static int libE_tolower(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_scheckbuffer(L, 1);

    size_t off = opt_offset(L, s, 2);
    size_t len = opt_length(L, s, off, 3);

    lxs_stolower(L, s, off, len);
    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.toupper(b[, offset, [length]])
/// buffer:toupper([offset, [length]])
///
/// Returns the *buffer* it was called on.
/// Changes all lowercase letters of the selected range to uppercase. The
/// default range is the whole buffer content, you can specifiy a custom range
/// by using *offset* and *length*.
/// The definition of what an uppercase letter is depends on the current locale.
///
/// Example Usage:
///     local b = buffer.new()
///     b:append("abc"):toupper():tostring() --> "ABC"
///     local b = buffer.new()
///     b:append("abc"):toupper(1, 1):tostring() --> "Abc"
static int libE_toupper(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_scheckbuffer(L, 1);

    size_t off = opt_offset(L, s, 2);
    size_t len = opt_length(L, s, off, 3);

    lxs_stoupper(L, s, off, len);
    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.rep(buffer, count, [, what])
/// buffer:rep(count [, what])
///
/// Returns the *buffer* it was called on.
/// Appends to a *buffer* a string repeated *count* times. The appended string
/// can be omitted in which case the buffer will repeat whatever it currently
/// contains. Or specified through the last argument *what*.
///
/// Example Usage:
///     local b = buffer.new()
///     b:append("abc "):rep(2):tostring() --> "abc abc abc"
///     local b = buffer.new()
///     b:rep(3, "abc "):tostring() --> "abc abc abc"
static int libE_rep(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_scheckbuffer(L, 1);

    int count = luaL_checkinteger(L, 2);
    luaL_argcheck(L, count > 0, 2, "must be greater than zero");

    size_t len;
    const char* str;

    if (lua_gettop(L) > 2)
        str = lxs_stolstring(L, 3, &len);
    else
    {
        str = lxs_scstr(s);
        len = lxs_slen(s);
    }
    if (len > 0)
        lxs_sappend_repeat(L, s, count, str, len);

    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.append(buffer, string [, ...])
/// buffer:(string [, ...])
///
/// Returns the *buffer* it was called on.
/// Appends to a buffer all specified arguments, in the specified order.
/// All arguments must be strings.
///
/// Example Usage:
///     local b = buffer.new()
///     b:append("a"):append("b"):append("c"):tostring() --> "abc"
///     b:clear()
///     b:append("a", "b", "c"):tostring() --> "abc"
///     b:append(tostring(123)):tostring() --> "abc123"
static int libE_append(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_scheckbuffer(L, 1);

    size_t      len;
    const char* str;

    int top = lua_gettop(L);
    if (top > 2)
    {
        ++top;
        for (int i = 2; i < top; ++i)
        {
            str = luaL_checklstring(L, i, &len);
            if (len > 0)
                lxs_sappend(L, s, str, len);
        }
    }
    else
    {
        str = luaL_checklstring(L, 2, &len);
        if (len > 0)
            lxs_sappend(L, s, str, len);
    }
    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.insert(b, [offset,] string)
/// buffer:insert([offset,] string)
///
/// Returns the *buffer* it was called on.
///
/// Example Usage:
static int libE_insert(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_scheckbuffer(L, 1);

    size_t off = opt_offset(L, s, 2);
    size_t len;
    const char* str = lxs_schecklstring(L, 3, &len);

    lxs_sinsert(L, s, off, str, len);
    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
///
///
/// Example Usage:
static int libE_tostring(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_spushresult(L, lxs_scheckbuffer(L, 1));

    lxs_assert_stack_end(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
/// Returns the *buffer* it was called on.
///
/// Example Usage:
static int libE_trim_excess(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_sshrink_to_fit(L, lxs_scheckbuffer(L, 1));

    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
/// Returns the *buffer* it was called on.
///
/// Example Usage:
static int libE_reverse(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_scheckbuffer(L, 1);

    size_t off = opt_offset(L, s, 2);
    size_t len = opt_length(L, s, off, 3);

    lxs_sreverse(L, s, off, len);
    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
/// Returns the *buffer* it was called on.
///
/// Example Usage:
static int libE_replace(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_scheckbuffer(L, 1);

    int narg = 2;

    size_t off = 0;
    if (lua_isnumber(L, 2))
    {
        off = opt_offset(L, s, 2);
        ++narg;
    }
    size_t len;
    const char* str = lxs_schecklstring(L, narg, &len);

    lxs_sreplace(L, s, off, str, len);
    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
/// Returns the *buffer* it was called on.
///
/// Example Usage:
static int libE_remove(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_scheckbuffer(L, 1);

    size_t off = opt_offset(L, s, 2);
    size_t len = opt_length(L, s, off, 3);

    lxs_sremove(L, s, off, len);
    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
/// Returns the *buffer* it was called on.
///
/// Example Usage:
static int libE_trim(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_strim(L, lxs_scheckbuffer(L, 1));

    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
/// Returns the *buffer* it was called on.
///
/// Example Usage:
static int libE_ltrim(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_sltrim(L, lxs_scheckbuffer(L, 1));

    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
/// Returns the *buffer* it was called on.
///
/// Example Usage:
static int libE_rtrim(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_srtrim(L, lxs_scheckbuffer(L, 1));

    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
///
///
/// Example Usage:
static int libE_split(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s      = lxs_scheckbuffer(L, 1);

    size_t      slen;
    const char* seps   = luaL_checklstring(L, 2, &slen);

    luaL_argcheck(L, slen > 0, 2, "separator cannot be nil nor empty");

    lxs_assert_stack_end(L, 0);
    return lxs_split(L, lxs_scstr(s), lxs_slen(s), seps);
}

/// buffer.(b)
/// buffer:()
///
/// _G.string.sub:
///     Returns the substring of s that starts at i and continues until j;
///     i and j can be negative.
///
///     If j is absent, then it is assumed to be equal to -1 (which is the same
///     as the string length).
///
///     In particular, the call
///         string.sub(s,1,j) returns a prefix of s with length j,
///     and
///         string.sub(s, -i) returns a suffix of s with length i.
///
///
///
/// Example Usage:
static int libE_substr(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_scheckbuffer(L, 1);

    int i = luaL_checkinteger(L, 1);
    int j = luaL_optinteger(L, 2, -1);

    size_t offset;
    if (i > 0)
        offset = i + 1u;
    else if (i < 0)
        offset = s->len + i;
    else
        luaL_argerror(L, 1, "cannot be zero");
    check_offset(L, s, offset, i);

    size_t length;
    if (j > 0)
        length = j;
    else if (j < 0)
        length = s->len - offset;
    else
        luaL_argerror(L, 2, "cannot be zero");
    check_length(L, s, offset, length, j);

    size_t      len = 0;
    const char* str = lxs_ssubstr_nocopy(s, offset, length, &len);

    if (len == 0)
        lua_pushliteral(L, "");
    else
        lua_pushlstring(L, str, len);

    lxs_assert_stack_end(L, 1);
    return 1;
}


//------------------------------------------------------------------------------

/// buffer.(b)
/// buffer:()
///
///
///
/// Example Usage:
static int libM_tostring(lua_State* const L)
{
    return libE_tostring(L);
}

/// buffer.(b)
/// buffer:()
///
///
///
/// Example Usage:
static int libM_len(lua_State* const L)
{
    return libE_len(L);
}

static int libM_index(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_stobuffer(L, 1);

    luaL_checktype(L, 2, LUA_TNUMBER);
    size_t off = opt_offset(L, s, 2);

    lua_pushlstring(L, lxs_scstr(s, off), 1);

    lxs_assert_stack_end(L, 1);
    return 1;
}

static int libM_newindex(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_stobuffer(L, 1);

    luaL_checktype(L, 2, LUA_TNUMBER);
    size_t off = opt_offset(L, s, 2);

    size_t      len = 0;
    const char* str = lxs_schecklstring(L, 3, &len);

    lxs_sreplace(L, s, off, str, len);
    lxs_assert_stack_end(L, 0);

    lua_settop(L, 1);

    lxs_assert_stack_at(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
///
///
/// Example Usage:
static int libM_concat(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_string* lhs = lxs_stobuffer(L, 1);
    lxs_string* rhs = lxs_stobuffer(L, 2);

    if (lhs && rhs)
    {
        lxs_string* res = lxs_snewbuffer(L, lxs_slen(lhs) + lxs_slen(rhs) + 1u);
        if (lxs_slen(lhs) > 0)
            lxs_sappend(L, res, lxs_scstr(lhs), lxs_slen(lhs));
        if (lxs_slen(rhs) > 0)
            lxs_sappend(L, res, lxs_scstr(rhs), lxs_slen(rhs));
    }
    else
    {
        size_t      len;
        const char* str;
        if (lhs)
        {
            str = lua_tolstring(L, 2, &len);
            if (len > 0)
                lxs_sappend(L, lhs, str, len);
            lua_pushvalue(L, 1);
        }
        else
        {
            str = lua_tolstring(L, 1, &len);
            if (len > 0)
                lxs_sappend(L, rhs, str, len);
            lua_pushvalue(L, 2);
        }
    }

    lxs_assert_stack_end(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
///
///
/// Example Usage:
static int libM_eq(lua_State* const L)
{
    return libE_equals(L);
}

static int libM_gc(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lxs_sfreebuffer(L, lxs_stobuffer(L, 1));

    lxs_assert_stack_end(L, 0);
    return 0;
}


//------------------------------------------------------------------------------

/// buffer.(b)
/// buffer:()
///
///
///
/// Example Usage:
static int libL_call(lua_State* const L)
{
    return libE_new(L);
}

#if !LUAXS_STR_READONLY_OPTIONS
/// buffer.(b)
/// buffer:()
///
static int libL_growth_factor(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    if (lua_isnumber(L, 1))
    {
        lxs_sgrowth_factor(
            clamp(lua_tonumber(L, 1), 1.5, 4.0)
        );
    }
    lua_pushnumber(L, lxs_sgrowth_factor());

    lxs_assert_stack_end(L, 1);
    return 1;
}

/// buffer.(b)
/// buffer:()
///
static int libL_default_capacity(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    if (lua_isnumber(L, 1))
    {
        int n = lua_tointeger(L, 1);
#if LUAXS_STR_MAX_SINGLE_EXPANSION > 0
        if (n >= LUAXS_STR_MAX_SINGLE_EXPANSION)
            luaL_argerror(L, 1, "must be smaller than " #LUAXS_STR_MAX_SINGLE_EXPANSION);
#endif
#if LUAXS_STR_MAX_TOTAL_EXPANSION > 0
        if (n >= LUAXS_STR_MAX_TOTAL_EXPANSION)
            luaL_argerror(L, 1, "must be smaller than " #LUAXS_STR_MAX_TOTAL_EXPANSION);
#endif
        lxs_sdefault_capacity(clamp(
            lxs_mnext_pow2(static_cast<size_t>(n)), 128u, 4096u
        ));
    }
    lua_pushinteger(L, static_cast<lua_Integer>(lxs_sdefault_capacity()));

    lxs_assert_stack_end(L, 1);
    return 1;
}
#endif // !LUAXS_STR_READONLY_OPTIONS

//------------------------------------------------------------------------------

static const luaL_Reg libE_funcs[] = {
    { "len",              libE_len              },
    { "cap",              libE_cap              },
    { "clear",            libE_clear            },
    { "new",              libE_new              },
    { "isbuffer",         libL_isbuffer         },
    { "equals",           libE_equals           },
    { "lower",            libE_tolower          },
    { "upper",            libE_toupper          },
    { "rep",              libE_rep              },
    { "append",           libE_append           },
    { "insert",           libE_insert           },
    { "tostring",         libE_tostring         },
    { "trim_excess",      libE_trim_excess      },
    { "reverse",          libE_reverse          },
    { "replace",          libE_replace          },
    { "remove",           libE_remove           },
    { "trim",             libE_trim             },
    { "ltrim",            libE_ltrim            },
    { "rtrim",            libE_rtrim            },
    { "split",            libE_split            },
    { "substr",           libE_substr           },
#if !LUAXS_STR_READONLY_OPTIONS
    { "growth_factor",    libL_growth_factor    },
    { "default_capacity", libL_default_capacity },
#endif
    { NULL, NULL }
};

static const luaL_Reg libM_funcs[] = {
    { "__tostring", libM_tostring },
    { "__len",      libM_len      },
    { "__concat",   libM_concat   },
    //{ "__index",    libM_index    },
    //{ "__newindex", libM_newindex },
    { "__eq",       libM_eq       },
    { "__gc",       libM_gc       },
    { NULL, NULL }
};


//------------------------------------------------------------------------------

LUA_API int luaopen_buffer(lua_State* const L)
{
    lxs_assert_stack_begin(L);
    lxs_assert(L, sizeof(lxs_string) == 12);

    /// register buffer library (_G.buffer) ----------------
    luaI_openlib(L, LUA_BUFFERLIBNAME, libE_funcs, 0);

#if LUAXS_STR_READONLY_OPTIONS
    lua_pushinteger(L, static_cast<int>(LUAXS_STR_INITIAL_CAPACITY));
    lxs_rawsetl(L, -2, "default_capacity");

    lua_pushnumber(L, LUAXS_STR_GROWTH_FACTOR);
    lxs_rawsetl(L, -2, "growth_factor");
#endif
    ///-----------------------------------------------------

    /// create and store userdata instance metatable -------
    lua_createtable(L, 0, _countof(libM_funcs) - 1);
    luaI_openlib(L, NULL, libM_funcs, 0);

    lua_pushvalue(L, -2);
    lxs_rawsetl(L, -2, "__index");

    lxs_rawsetl(L, LUA_REGISTRYINDEX, LUA_BUFFERLIBNAME);
    ///-----------------------------------------------------

    /// create and assign _G.buffer's metatable
    lua_createtable(L, 0, 2);

    lua_pushcclosure(L, libL_call, 0);
    lxs_rawsetl(L, -2, "__call");

    lxs_rawgetl(L, LUA_REGISTRYINDEX, LUA_BUFFERLIBNAME);
    lxs_rawsetl(L, -2, "__metatable");

    lua_setmetatable(L, -2);
    ///-----------------------------------------------------

    lxs_assert_stack_end(L, 1);
    return 1;
}


#endif // LUAXS_ADDLIB_BUFFER
//==============================================================================

}; // extern "C"
