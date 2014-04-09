#define lxs_string_cpp
#define LUA_CORE

extern "C"
{
#include "lxs_string.hpp"

#include "lauxlib.h"
#include "lobject.h"
#if LUAXS_STR_PERSISTENT_BUFFER
#  include "lstate.h"
#endif

#include <ctype.h>

extern "C++"
{
#include "leastl.hpp"

#include <eastl/bitset.h>
};


//==============================================================================

#if LUAXS_STR_PERSISTENT_BUFFER

lxs_string* lxs_spb_get(lua_State* const L)
{
    lxs_assert(L, L);
    
    lxs_string* s = L->pb;
    if (s)
        lxs_sclear(L, s);
    else
        s = lxs_spb_create(G(L)->mainthread, L);

    return s;
}

lxs_string* lxs_spb_create(lua_State* const Lmain, lua_State* const Lcoro)
{
    lxs_assert(Lmain, Lmain);
    lxs_assert(Lmain, G(Lmain)->mainthread == Lmain);

    lxs_string* s = lxs_screate(
        Lmain,
        LUAXS_PB_INITIAL_CAPACITY,
        true
    );
    lxs_assert(Lmain, s);
    lxs_assert(Lmain, s->cap == LUAXS_PB_INITIAL_CAPACITY);
    lxs_assert(Lmain, s->len == 0);
    lxs_assert(Lmain, s->data);

    return (Lcoro ? Lcoro : Lmain)->pb = s;
}

void lxs_spb_destroy(lua_State* const Lmain, lua_State* const Lcoro)
{
    lxs_assert(Lmain, Lmain);
    lxs_assert(Lmain, G(Lmain)->mainthread == Lmain);

    lxs_sdestroy(Lmain, (Lcoro ? Lcoro : Lmain)->pb);
}

#endif // LUAXS_STR_PERSISTENT_BUFFER


//==============================================================================
// internal API

#if LUAXS_STR_READONLY_OPTIONS
#  define LXS_GROTH_FACTOR (LUAXS_STR_GROWTH_FACTOR)
#else
#  define LXS_GROTH_FACTOR (lxs_sgrowth_factor())
#endif // LUAXS_STR_READONLY_OPTIONS


#if !LUAXS_STR_READONLY_OPTIONS
static double s_growth_factor    = LUAXS_STR_GROWTH_FACTOR;
static size_t s_default_capacity = LUAXS_STR_INITIAL_CAPACITY;

size_t lxs_sdefault_capacity(size_t new_default_capacity /*= 0u*/)
{
    if (s_default_capacity != 0u)
        s_default_capacity = new_default_capacity;

    return s_default_capacity;
}

double lxs_sgrowth_factor(double new_growth_factor /*= 0d*/)
{
    assert(new_growth_factor == 0.0 || new_growth_factor > 1.0);

    if (new_growth_factor != 0.0)
        s_growth_factor = new_growth_factor;

    return s_growth_factor;
}
#endif // !LUAXS_STR_READONLY_OPTIONS

lxs_string* lxs_screate(lua_State* const L,
                        size_t cap /*= 0u*/,
                        bool force /*= true*/)
{
    lxs_assert(L, L);

    lxs_string* s = NULL;
    if (cap == 0u)
    {
        s = static_cast<lxs_string*>(luaM_malloc(L, sizeof(lxs_string)));
    }
    else
    {
        if (!force)
            cap = lxs_mgrow(L, 0u, cap, true, LXS_GROTH_FACTOR);

        lxs_assert(L, cap > 0u);
        lxs_scheck_mem_limits(L, 0u, cap, 0u);

        s = static_cast<lxs_string*>(
            luaM_malloc(L, sizeof(lxs_string) + sizeof(char) * cap)
        );

        s->data = reinterpret_cast<char*>(s + sizeof(lxs_string));
        lxs_assert(L, s->data);
    }
    lxs_assert(L, s);

    s->cap = cap;

    return s;
}

void lxs_sdestroy(lua_State* const L, lxs_string* s)
{
    lxs_assert(L, L);

    if (s)
    {
        if (s->cap != 0u && s->data)
            luaM_freemem(L, s->data, s->cap);
        luaM_freemem(L, s, sizeof(lxs_string));

        s = NULL;
    }
}

void lxs_sinit(lua_State* const L, lxs_string* const s, size_t capacity /*= 0*/)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    if (capacity == 0)
#if LUAXS_STR_READONLY_OPTIONS
        capacity = LUAXS_STR_INITIAL_CAPACITY;
#else
        capacity = lxs_sdefault_capacity();
#endif // LUAXS_STR_READONLY_OPTIONS

    lxs_scheck_mem_limits(L, 0u, capacity, 0u);

    char* buffer = static_cast<char*>(luaM_malloc(L, capacity));
    if (buffer == NULL)
        lxs_error(L, MEMERRMSG);

    s->cap  = capacity;
    s->len  = 0;
    s->data = buffer;
}

void lxs_srealloc(lua_State* const L,
                  lxs_string* s,
                  size_t new_capacity,
                  bool force /*= false*/)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    if (new_capacity == s->cap)
        return;

    new_capacity = lxs_mgrow(L,
        s->cap,
        new_capacity,
        force,
        LXS_GROTH_FACTOR
    );

    if (new_capacity == 0u)
    {
        if (s->data)
            luaM_freemem(L, s->data, s->cap);

        s->cap  = 0u;
        s->len  = 0u;
        s->data = NULL;
    }
    else
    {
        lxs_scheck_mem_limits(L, s->cap, new_capacity, 0u);

        char* buffer = static_cast<char*>(luaM_realloc(L,
            s->data,
            s->cap,
            new_capacity
        ));

        if (buffer == NULL)
            lxs_error(L, MEMERRMSG);

        if (new_capacity - 1u < s->len)
            s->len  = new_capacity - 1u;
        s->cap  = new_capacity;
        s->data = buffer;

        buffer[s->len]       = '\0';
        buffer[new_capacity] = '\0';
    }
}

void lxs_sshrink_to_fit(lua_State* const L, lxs_string* const s)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    if (s->cap == 0u)
        return;

    if (s->len == 0u)
    {
        lxs_srealloc(L, s, 0u, true);
        return;
    }

    size_t min = s->len + 1u;
    if (min < s->cap)
        lxs_srealloc(L, s, min, true);
}

void lxs_sensure_space(lua_State* const L,
                       lxs_string* const s,
                       size_t additional_length)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    size_t total = s->len + additional_length;
    if (s->cap == 0u || total > s->cap - 1u)
        lxs_srealloc(L, s, total + 1u, false);
}

void lxs_sappendc(lua_State* const L, lxs_string* const s, char c)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    lxs_sensure_space(L, s, 1);

    s->data[s->len] = c;
    s->len += 1;

    lxs_sterminate(s);
}

void lxs_sappend_top(lua_State* const L, lxs_string* s)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    size_t len;
    const char *str = lua_tolstring(L, -1, &len);

    if (len > 0)
        lxs_sappend(L, s, str, len);

    lxs_sterminate(s); 
}

void lxs_sappends(lua_State* const L,
                  lxs_string* s,
                  const char* const buf)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    lxs_assert(L, buf);

    lxs_sappend(L, s, buf, strlen(buf));
}

void lxs_sappend(lua_State* const L,
                 lxs_string* s,
                 const char* const buf,
                 size_t len)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    lxs_assert(L, buf);
    lxs_assert(L, len > 0);

    lxs_sensure_space(L, s, len);

    memcpy(&s->data[s->len], buf, len);
    s->len += len;

    lxs_sterminate(s);
}

void lxs_sappend_format(lua_State* const L,
                        lxs_string* const s,
                        const char* fmt,
                        ...)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    lxs_assert(L, fmt);

    va_list args;
    va_start(args, fmt);
    lxs_sappend_vformat(L, s, fmt, args);
    va_end(args);
}

void lxs_sappend_vformat(lua_State* const L,
                         lxs_string* const s,
                         const char* fmt,
                         va_list args)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    lxs_assert(L, fmt);
    lxs_assert(L, args);

    int len = _vscprintf(fmt, args);
    if (len < 0)
        lxs_error(L, "invalid format string and/or format arguments");
    else if (len == 0)
        return;

    lxs_sensure_space(L, s, static_cast<size_t>(len));

    vsprintf_s(&s->data[s->len], lxs_sfree(s) + 1, fmt, args);
    s->len += len;

    lxs_sterminate(s);
}

void lxs_sclear(lua_State* const L, lxs_string* const s)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    if (s->cap > 0)
        s->data[0] = '\0';
    s->len = 0;
}

bool lxs_sisbuffer(lua_State* const L, int narg)
{
    lxs_assert(L, L);

    void* ptr = lua_touserdata(L, narg);
    if (ptr)
    {
        if (lua_getmetatable(L, -1))
        {
            lxs_rawgetl(L, LUA_REGISTRYINDEX, LUA_BUFFERLIBNAME);
            if (lua_rawequal(L, -1, -2))
            {
                lua_pop(L, 2);
                return true;
            }
            lua_pop(L, 2);
        }
    }

    return false;
}

bool lxs_sisbufferorstring(lua_State* const L, int narg)
{
    lxs_assert(L, L);

    return lua_isstring(L, narg) || lxs_sisbuffer(L, narg);
}

int lxs_scompare(lxs_string* const s, const char* str, size_t len /*= 0u*/)
{
    assert(s);
    assert(s->data);
    assert(str);
    assert(len == 0 || len <= strlen(str));

    if (len == 0u)
        len = strlen(str);
    if (s->len != len)
        return (s->len > len) ? 1 : -1;
    return strncmp(const_cast<const char*>(s->data), str, len);
}

int lxs_sicompare(lxs_string* const s,
                  const char* str,
                  size_t len /*= 0u*/)
{
    assert(s);
    assert(s->data);
    assert(str);
    assert(len == 0 || len <= strlen(str));

    if (len == 0u)
        len = strlen(str);
    if (s->len != len)
        return (s->len > len) ? 1 : -1;
    return _strnicmp(const_cast<const char*>(s->data), str, len);
}

char* lxs_sfind(lxs_string* const s, const char* str, size_t offset /*= 0u*/)
{
    assert(s);
    assert(s->data);
    assert(str);

    return strstr(&s->data[offset], const_cast<char*>(str));
}

char* lxs_sfindc(lxs_string* const s, char c, size_t offset /*= 0u*/)
{
    assert(s);
    assert(s->data);

    return strchr(&s->data[offset], c);
}

char* lxs_srfindc(lxs_string* const s, char c, size_t offset /*= 0u*/)
{
    assert(s);
    assert(s->data);

    return strrchr(&s->data[offset], c);
}

bool lxs_sequal(lxs_string* const lhs, lxs_string* const rhs)
{
    assert(lhs);
    assert(rhs);
    assert(lhs->data);
    assert(rhs->data);

    if (lhs == rhs)
        return true;
    if (lhs->len != rhs->len)
        return false;
    return strncmp(lhs->data, rhs->data, lhs->len) == 0;
}

bool lxs_sequals(lxs_string* const s,
                 const char* str,
                 size_t len /*= 0u*/)
{
    assert(s);
    assert(s->data);
    assert(str);
    assert(len == 0 || len <= strlen(str));

    if (len == 0u)
        len = strlen(str);
    if (s->len != len)
        return false;
    return strncmp(const_cast<const char*>(s->data), str, len) == 0;
}

bool lxs_sstarts_with(lxs_string* const s,
                     const char* str,
                     size_t len /*= 0u*/)
{
    assert(s);
    assert(s->data);
    assert(str);
    assert(len == 0 || len <= strlen(str));

    if (len == 0u)
        len = strlen(str);
    if (s->len < len)
        return false;
    return strncmp(const_cast<const char*>(&s->data[0]), str, len) == 0;
}

bool lxs_sends_with(lxs_string* const s,
                   const char* str,
                   size_t len /*= 0u*/)
{
    assert(s);
    assert(s->data);
    assert(str);
    assert(len == 0 || len <= strlen(str));

    if (len == 0u)
        len = strlen(str);
    if (s->len < len)
        return false;
    return strncmp(
        const_cast<const char*>(&s->data[s->len - len - 1u]),
        str,
        len
    ) == 0;
}

bool lxs_sistarts_with(lxs_string* const s,
                      const char* str,
                      size_t len /*= 0u*/)
{
    assert(s);
    assert(s->data);
    assert(str);
    assert(len == 0 || len <= strlen(str));

    if (len == 0u)
        len = strlen(str);
    if (s->len < len)
        return false;
    return _strnicmp(const_cast<const char*>(&s->data[0]), str, len) == 0;
}

bool lxs_siends_with(lxs_string* const s,
                    const char* str,
                    size_t len /*= 0u*/)
{
    assert(s);
    assert(s->data);
    assert(str);
    assert(len == 0 || len <= strlen(str));

    if (len == 0u)
        len = strlen(str);
    if (s->len < len)
        return false;
    return _strnicmp(
        const_cast<const char*>(&s->data[s->len - len - 1u]),
        str,
        len
    ) == 0;
}

bool lxs_sstarts_withc(lxs_string* const s, char c)
{
    assert(s);
    assert(s->data);

    if (s->len == 0)
        return false;
    return s->data[0] == c;
}

bool lxs_sends_withc(lxs_string* const s, char c)
{
    assert(s);
    assert(s->data);

    if (s->len == 0)
        return false;
    return s->data[s->len - 1u] == c;
}

const char* lxs_ssubstr_nocopy(lxs_string* const s,
                               size_t offset,
                               size_t length,
                               size_t* len /*= NULL*/)
{
    assert(s);
    assert(s->data);
    assert(length > 0);

    if (s->len == 0u || offset >= s->len)
    {
        if (len)
            *len = 0;
        return NULL;
    }

    if (len)
        *len = min(s->len - offset + length, s->len - offset);
    return const_cast<const char*>(&s->data[offset]);
}

lxs_string* lxs_ssubstr(lua_State* const L,
                        lxs_string* const s,
                        size_t offset,
                        size_t length)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    lxs_assert(L, s->data);
    lxs_assert(L, length > 0);

    lxs_string* sub = lxs_screate(L, length + 1u);

    size_t      len;
    const char* str = lxs_ssubstr_nocopy(s, offset, length, &len);

    lxs_sappend(L, sub, str, len);
    return sub;
}

void lxs_spushresult(lua_State* const L, lxs_string* const s)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    if (s->len == 0)
        lua_pushliteral(L, "");
    else
        lua_pushlstring(L, s->data, s->len);
}

void lxs_stoupper(lua_State* const L,
                  lxs_string* const s,
                  size_t offset /*= 0*/,
                  size_t length /*= 0*/)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    lxs_sabort_if_empty(s);

    if (offset == 0 && length == 0)
        length = s->len;

    for (size_t i = offset; length > 0; ++i, --length)
    {
        s->data[i] = static_cast<char>(toupper(s->data[i]));
    }
}

void lxs_stolower(lua_State* const L,
                  lxs_string* const s,
                  size_t offset /*= 0*/,
                  size_t length /*= 0*/)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    lxs_sabort_if_empty(s);

    if (offset == 0 && length == 0)
        length = s->len;

    for (size_t i = offset; length > 0; ++i, --length)
    {
        s->data[i] = static_cast<char>(tolower(s->data[i]));
    }
}

void lxs_strim(lua_State* const L, lxs_string* const s)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    lxs_sabort_if_empty(s);

    const char* front = &s->data[0];
    const char* end   = &s->data[s->len - 1];
    size_t      len   = s->len;

    for (; len && isspace(*front); --len, ++front) {}
    for (; len && isspace(*end);   --len, --end)   {}

    memmove(&s->data[0], front, len);
    s->len = len;

    lxs_sterminate(s);
}

void lxs_sltrim(lua_State* const L, lxs_string* const s)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    
    lxs_sabort_if_empty(s);

    const char* front = &s->data[0];
    size_t      len   = s->len;

    while (len && isspace(*front))
    {
        --len;
        ++front;
    }

    memmove(&s->data[0], front, len);
    s->len = len;
}

void lxs_srtrim(lua_State* const L, lxs_string* const s)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    
    lxs_sabort_if_empty(s);

    const char* end = &s->data[s->len - 1];
    size_t      len = s->len;

    while (len && isspace(*end))
    {
        --len;
        --end;
    }

    s->len = len;

    lxs_sterminate(s);
}

/// Note: this implementation is differant from lstrlib.cpp, this implementation
/// drops empty tokens.
const char** lxs_ssplit(lua_State* const L,
                        lxs_string* const s,
                        const char* seps,
                        size_t seps_len,
                        int& count)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    lxs_assert(L, s->data);
    lxs_assert(L, seps);
    lxs_assert(L, strlen(seps) >= seps_len);

    if (seps_len == 0)
        seps_len = strlen(seps);

    if (s->len == 0 || seps_len == 0)
    {
        count = 1;
        return const_cast<const char**>(&s->data);
    }

    lxs_string result;
    lxs_sinit(L, &result, s->len + 32);

    eastl::bitset<255> seps_lookup;
    while (*seps)
    {
        seps_lookup[STATIC_CAST(unsigned char, *seps)] = true;
        seps++;
    }

    const char* front = s->data;
    const char* end   = s->data + s->len;
    bool in_token = false;

    for (const char* it = front; it != end; ++it)
    {
        if (seps_lookup[STATIC_CAST(unsigned char, *it)])
        {
            if (in_token)
            {
                lxs_sappend(L, &result, front, it - front);
                lxs_sappendc(L, &result, '\0');

                in_token = false;
            }
        }
        else if (!in_token)
        {
            front = it;
            in_token = true;
        }
    }
    if (in_token)
    {
        lxs_sappend(L, &result, front, end - front);
        lxs_sappendc(L, &result, '\0');
    }
    lxs_sappendc(L, &result, '\0');

    if (result.len + 2u >= result.cap)
        lxs_srealloc(L, &result, result.len + 2u, true);

    return const_cast<const char**>(&result.data);
}

void lxs_sreverse(lua_State* const L,
                  lxs_string* const s,
                  size_t offset /*= 0*/,
                  size_t length /*= 0*/)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    
    lxs_sabort_if_empty(s);

    if (offset == 0 && length == 0)
        length = s->len;

    char* front = &s->data[offset]; 
    char* end   = &s->data[offset + length - 1];
    char  temp;

    while (front < end)
    {
        temp   = *front;
        *front = *end;
        *end   = temp;

        ++front;
        --end;
    }
}

void lxs_sinsert(lua_State* const L,
                 lxs_string* const s,
                 size_t offset,
                 const char* str,
                 size_t len /*= 0*/)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    lxs_assert(L, str);
    lxs_assert(L, len == 0 || len <= strlen(str));

    if (len == 0)
        len = strlen(str);

    lxs_sensure_space(L, s, len);

    if (offset == s->len)
    {
        memcpy(&s->data[offset], str, len);
    }
    else
    {
        memcpy(&s->data[offset + len], &s->data[offset], s->len - offset + len);
        memcpy(&s->data[offset], str, len);
    }
    s->len += len;

    lxs_sterminate(s);
}

void lxs_sshrink(lua_State* const L, lxs_string* const s, size_t len)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    if (len == 0u)
        return;

    s->len = max(0u, s->len - len);
}

void lxs_sexpand(lua_State* const L, lxs_string* const s, size_t len)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    if (len == 0u)
        return;

    lxs_sensure_space(L, s, len);

    memset(&s->data[s->len], ' ', len);
    s->len += len;

    lxs_sterminate(s);
}

void lxs_sreplace(lua_State* const L,
                  lxs_string* const s,
                  size_t offset,
                  const char* str,
                  size_t len /*= 0*/)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    lxs_assert(L, str);
    lxs_assert(L, len == 0 || len <= strlen(str));

    if (len == 0)
        len = strlen(str);

    size_t total = max(s->len, offset + len);
    lxs_sensure_space(L, s, len);

    memcpy(&s->data[offset], str, len);
    s->len = total;

    lxs_sterminate(s);
}

void lxs_sremove(lua_State* const L,
                 lxs_string* const s,
                 size_t offset /*= 0*/,
                 size_t length /*= 0*/)
{
    lxs_assert(L, L);
    lxs_assert(L, s);

    if (offset == 0 && length == 0)
    {
        lxs_sclear(L, s);
        return;
    }

    if (offset + length != s->len)
        memcpy(&s->data[offset], &s->data[offset + length], length);
    s->len -= length;

    lxs_sterminate(s);
}

void lxs_sappend_repeat(lua_State* const L,
                        lxs_string* const s,
                        size_t count,
                        const char* str,
                        size_t len /*= 0*/)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    lxs_assert(L, str);
    lxs_assert(L, len == 0 || strlen(str) >= len);

    lxs_sensure_space(L, s, len * count);

    while (count > 0)
    {
        memcpy(&s->data[s->len], str, len);
        s->len += len;

        --count;
    }

    lxs_sterminate(s);
}

/// Uses MurmurHash3_x86_32
/// @see http://code.google.com/p/smhasher/
/// @see http://code.google.com/p/smhasher/wiki/MurmurHash3
uint32_t lxs_shash(lxs_string* const s)
{
    assert(s);
    assert(s->data);

    const uint8_t* data = (const uint8_t*)s->data; //-V2005
    const int nblocks = s->len / 4;

    uint32_t h1 = (5381 << 16) + 5381; // seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4); //-V2005

    for(int i = -nblocks; i; ++i)
    {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1  = _rotl(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1  = _rotl(h1, 13);
        h1  = h1 * 5 + 0xe6546b64;
    }

    const uint8_t* tail = (const uint8_t*)(data + nblocks * 4); //-V2005

    uint32_t k1 = 0;

    switch(s->len & 3)
    {
    case 3:
        k1 ^= tail[2] << 16;
    case 2:
        k1 ^= tail[1] << 8;
    case 1:
        k1 ^= tail[0];
        k1 *= c1;
        k1  = _rotl(k1,15);
        k1 *= c2;
        h1 ^= k1;
    }

    h1 ^= s->len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;
    return h1;
}


//------------------------------------------------------------------------------
// public API

void lxs_scheck_range(lua_State* const L,
                      lxs_string* const s,
                      size_t offset,
                      size_t length)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    lxs_assert_stack_begin(L);

    if (offset != 0 && offset > s->len)
        lxs_error(L, "offset (%u) out of range (0-%u)", offset, s->len);
    if (length != 0 && offset + length > s->len)
        lxs_error(L, "length (%u) out of range (0-%u)", length, s->len);

    lxs_assert_stack_end(L, 0);
}

lxs_string* lxs_snewbuffer(lua_State* const L, size_t capacity /*= 0*/)
{
    lxs_assert(L, L);
    lxs_assert_stack_begin(L);

    lxs_string* s = static_cast<lxs_string*>(
        lua_newuserdata(L, sizeof(lxs_string))
    );
    lxs_assert(L, s && lua_isuserdata(L, -1));

    lxs_rawgetl(L, LUA_REGISTRYINDEX, LUA_BUFFERLIBNAME);
    lxs_assert(L, lua_istable(L, -1));
    lua_setmetatable(L, -2);

    lxs_sinit(L, s, capacity);

    lxs_assert_stack_end(L, 1);
    return s;
}

void lxs_sfreebuffer(lua_State* const L, lxs_string* s)
{
    lxs_assert(L, L);
    lxs_assert_stack_begin(L);

    if (s && s->data)
        luaM_freemem(L, s->data, s->cap);

    lxs_assert_stack_end(L, 0);
}

lxs_string* lxs_stobuffer(lua_State* const L, int narg)
{
    lxs_assert(L, L);

    return static_cast<lxs_string*>(lua_touserdata(L, narg));
}

lxs_string* lxs_scheckbuffer(lua_State* const L, int narg)
{
    lxs_assert(L, L);

    return static_cast<lxs_string*>(
        luaL_checkudata(L, narg, LUA_BUFFERLIBNAME)
    );
}

const char* lxs_schecklstring(lua_State* const L,
                              int narg,
                              size_t* len /*= NULL*/)
{
    lxs_assert(L, L);
    lxs_assert_stack_begin(L);

    if (lua_isstring(L, narg))
    {
        lxs_assert_stack_end(L, 0);
        return lua_tolstring(L, narg, len);
    }

    lxs_string* s = lxs_scheckbuffer(L, narg);
    if (len)
        *len = s->len;

    lxs_assert_stack_end(L, 0);
    return const_cast<const char*>(s->data);
}

const char* lxs_soptlstring(lua_State* const L,
                            int narg,
                            const char* def,
                            size_t* len /*= NULL*/)
{
    lxs_assert(L, L);
    lxs_assert_stack_begin(L);

    if (lua_isnoneornil(L, narg))
    {
        if (len)
            *len = (def) ? strlen(def) : 0;
        lxs_assert_stack_end(L, 0);
        return def;
    }

    lxs_assert_stack_end(L, 0);
    return lxs_schecklstring(L, narg, len);
}

const char* lxs_stolstring(lua_State* const L, int narg, size_t* len /*= NULL*/)
{
    lxs_assert(L, L);
    lxs_assert_stack_begin(L);

    lxs_string* s = lxs_stobuffer(L, narg);
    if (s)
    {
        if (len)
            *len = s->len;
        lxs_assert_stack_end(L, 0);
        return const_cast<const char*>(s->data);
    }

    lxs_assert_stack_end(L, 0);
    return lua_tolstring(L, narg, len);
}


//------------------------------------------------------------------------------

FILE* lxs_sfload(lua_State* const L,
                 lxs_string* const s,
                 const char* path,
                 size_t max_len /*= 0*/)
{
    lxs_assert(L, L);
    lxs_assert(L, s);
    lxs_assert(L, path);
    lxs_assert(L, strlen(path) <= 260);
    lxs_assert(L, max_len == 0u || max_len >= 3u); // potential BOM

    if (max_len == 0u)
        max_len = SIZE_T_MAX - s->len - 1u;

    FILE* f = fopen(path, "s");
    if (f)
    {
        size_t size = min(max_len, static_cast<size_t>(fseek(f, 0, SEEK_END)));
        lxs_sensure_space(L, s, size);
        fseek(f, 0, SEEK_SET);

        size -= lxs_sfskipbom(L, f, s);

        if (size > 0u)
            lxs_sfread(L, f, s, size);
    }
    return f;
}

size_t lxs_sfskipbom(lua_State* const L, FILE* const f, lxs_string* const s)
{
    if (lxs_sfread(L, f, s, 3u) == 3u &&
        static_cast<int>(s->data[s->len - 3]) == 0xEF &&
        static_cast<int>(s->data[s->len - 2]) == 0xBB &&
        static_cast<int>(s->data[s->len - 1]) == 0xBF)
    {
        lxs_sremove(L, s, s->len - 3u, 3u);
        return 3u;
    }
    return 0u;
}

size_t lxs_sfread(lua_State* const L,
                  FILE* const f,
                  lxs_string* const s,
                  size_t len)
{
    lxs_assert(L, L);
    lxs_assert(L, f);
    lxs_assert(L, s);
    lxs_assert(L, len > 0u);

    lxs_sensure_space(L, s, len);

    return fread(&s->data[s->len], sizeof(char), len, f);
}


//==============================================================================

}; // extern "C"