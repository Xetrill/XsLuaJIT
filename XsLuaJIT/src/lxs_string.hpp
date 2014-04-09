#ifndef lxs_string_hpp
#define lxs_string_hpp 1
#define LUA_CORE

#include "luajit.h"
#include "lualib.h"
#include "lmem.h"
#include "lxsmem.hpp"

#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>


//==============================================================================

typedef struct _lxs_string
{
    size_t cap;
    size_t len;
    char*  data;
} lxs_string;


//==============================================================================
// core/persistent buffer extensions

#if LUAXS_STR_PERSISTENT_BUFFER
lxs_string* lxs_spb_get(lua_State* const L);
lxs_string* lxs_spb_create(lua_State* const L, lua_State* const L1);
void        lxs_spb_destroy(lua_State* const L, lua_State* const L1);

#  define xbuf_decl(name)                lxs_string* name
#  define xbuf_init(L, name)             name = lxs_spb_get(L)
#  define xbuf_addlstring(L, name, s, l) lxs_sappend(L, name, s, l)
#  define xbuf_addstring(L, name, s)     lxs_sappends(L, name, s)
#  define xbuf_addliteral(L, name, lit)  lxs_sappendl(L, name, "" lit)
#  define xbuf_addchar(L, name, c)       lxs_sappendc(L, name, c)
#  define xbuf_addvalue(L, name)         lxs_sappend_top(L, name)
#  define xbuf_pushresult(L, name)       lxs_spushresult(L, name)

#  define lxs_spb_ptr(name)     name
#  define lxs_spb_decl(L, name) lxs_string* name = lxs_spb_get(L)
#else
#  define lxs_spb_ptr(name)     &name
#  define lxs_spb_decl(L, name) lxs_string name; lxs_sinit(L, &name)
#endif



//==============================================================================

__forceinline size_t lxs_slen(lxs_string* const s)
{
    assert(s);

    return s->len;
}

__forceinline size_t lxs_scap(lxs_string* const s)
{
    assert(s);

    return s->cap;
}

__forceinline char* lxs_sdata(lxs_string* const s, size_t DEFVAL(offset, 0u))
{
    assert(s);
    assert(s->data);
    assert(offset == 0u || offset < s->cap);

    return (offset != 0) ? &s->data[offset] : s->data;
}

__forceinline const char* lxs_scstr(lxs_string* const s, size_t DEFVAL(offset, 0u))
{
    return CONST_CAST(const char*, lxs_sdata(s, offset));
}

__forceinline char* lxs_sbegin(lxs_string* const s, size_t DEFVAL(offset, 0u))
{
    assert(s);
    assert(s->data);
    assert(offset == 0u || offset < s->cap);

    return &s->data[offset];
}

__forceinline char* lxs_send(lxs_string* const s)
{
    assert(s);
    assert(s->data);

    return &s->data[s->len];
}

__forceinline char* lxs_slast(lxs_string* const s)
{
    assert(s);
    assert(s->data);

    return (s->len > 0)
        ? &s->data[s->len - 1]
        : &s->data[s->len];
}

__forceinline char* lxs_sat(lxs_string* const s, size_t index)
{
    assert(s);
    assert(s->data);

    return &s->data[index];
}

__forceinline size_t lxs_sfree(lxs_string* const s)
{
    assert(s);

    return (s->cap - 1u) - s->len;
}

__forceinline void lxs_sterminate(lxs_string* const s)
{
    assert(s);
    assert(s->data);

    s->data[s->len] = '\0';
}

__forceinline void lxs_scheck_mem_limits(lua_State* const L,
                                         size_t cur_cap,
                                         size_t new_cap,
                                         size_t add_len)
{
#if defined(LUAXS_STR_MAX_SINGLE_EXPANSION) || \
    defined(LUAXS_STR_MAX_TOTAL_EXPANSION)

#  if LUAXS_STR_MAX_SINGLE_EXPANSION > 0
    if (new_cap > LUAXS_STR_MAX_SINGLE_EXPANSION ||
        add_len > LUAXS_STR_MAX_SINGLE_EXPANSION)
        lxs_error(L,
                  "requesting too much memory at once %u bytes (limit: %u)",
                  new_cap + add_len,
                  LUAXS_STR_MAX_SINGLE_EXPANSION
        );
#  endif // LUAXS_STR_MAX_SINGLE_EXPANSION

#  if LUAXS_STR_MAX_TOTAL_EXPANSION > 0
    if (new_cap + cur_cap > LUAXS_STR_MAX_TOTAL_EXPANSION)
        lxs_error(L,
                  "requesting too much total memory %u bytes (limit: %u)",
                  new_cap + cur_cap,
                  LUAXS_STR_MAX_TOTAL_EXPANSION
        );
    if (cur_cap + add_len > LUAXS_STR_MAX_TOTAL_EXPANSION)
        lxs_error(L,
                  "requesting too much total memory %u bytes (limit: %u)",
                  cur_cap + add_len,
                  LUAXS_STR_MAX_TOTAL_EXPANSION
        );
#  endif // LUAXS_STR_MAX_TOTAL_EXPANSION

#else   // LUAXS_STR_MAX_SINGLE_EXPANSION, LUAXS_STR_MAX_TOTAL_EXPANSION
    UNUSED(L);
    UNUSED(cur_cap);
    UNUSED(new_cap);
    UNUSED(add_len);
#endif  // LUAXS_STR_MAX_SINGLE_EXPANSION, LUAXS_STR_MAX_TOTAL_EXPANSION
}

#define lxs_sabort_if_empty(s) if (s->len == 0u) return


//------------------------------------------------------------------------------

#if !LUAXS_STR_READONLY_OPTIONS
__inline size_t lxs_sdefault_capacity(size_t new_default_capacity = 0u);
__inline double lxs_sgrowth_factor(double new_growth_factor = 0.0);
#endif // !LUAXS_STR_READONLY_OPTIONS


//------------------------------------------------------------------------------

lxs_string* lxs_screate(lua_State* const L,
                        size_t DEFVAL(cap, 0u),
                        bool DEFVAL(force, true));
void lxs_sdestroy(lua_State* const L, lxs_string* s);

void lxs_sinit(lua_State* const L,
               lxs_string* const s,
               size_t DEFVAL(capacity, 0u));

void lxs_srealloc(lua_State* const L,
                  lxs_string* const s,
                  size_t new_capacity,
                  bool DEFVAL(force, false));

void lxs_sshrink_to_fit(lua_State* const L, lxs_string* const s);

void lxs_sensure_space(lua_State* const L,
                       lxs_string* const s,
                       size_t additional_length);

void lxs_sappendc(lua_State* const L, lxs_string* s, char c);

void lxs_sappend_top(lua_State* const L, lxs_string* s);

void lxs_sappends(lua_State* const L,
                  lxs_string* const s,
                  const char* const buf);

void lxs_sappend(lua_State* const L,
                 lxs_string* const s,
                 const char* const buf,
                 size_t DEFVAL(len, 0u));

void lxs_sappend_format(lua_State* const L,
                        lxs_string* const s,
                        const char* const fmt,
                        ...);
void lxs_sappend_vformat(lua_State* const L,
                         lxs_string* const s,
                         const char* const fmt,
                         va_list args);

void lxs_sclear(lua_State* const L, lxs_string* const s);

void lxs_spushresult(lua_State* const L, lxs_string* const s);

bool lxs_sisbuffer(lua_State* const L, int narg);
bool lxs_sisbufferorstring(lua_State* const L, int narg);

bool lxs_sequal(lxs_string* const lhs, lxs_string* const rhs);
bool lxs_sequals(lxs_string* const lhs, const char* str, size_t DEFVAL(len, 0u));

int lxs_scompare(lxs_string* const s, const char* str, size_t DEFVAL(len, 0u));
int lxs_sicompare(lxs_string* const s, const char* str, size_t DEFVAL(len, 0u));

char* lxs_sfind(lxs_string* const s, const char* str, size_t DEFVAL(offset, 0u));
char* lxs_sfindc(lxs_string* const s, char c, size_t DEFVAL(offset, 0u));
char* lxs_srfindc(lxs_string* const s, char c, size_t DEFVAL(offset, 0u));

bool lxs_sstarts_with(lxs_string* const s, const char* str, size_t DEFVAL(len, 0u));
bool lxs_sends_with(lxs_string* const s, const char* str, size_t DEFVAL(len, 0u));

bool lxs_sistarts_with(lxs_string* const s, const char* str, size_t DEFVAL(len, 0u));
bool lxs_siends_with(lxs_string* const s, const char* str, size_t DEFVAL(len, 0u));

bool lxs_sstarts_withc(lxs_string* const s, char c);
bool lxs_sends_withc(lxs_string* const s, char c);

lxs_string* lxs_ssubstr(lua_State* const L,
                        lxs_string* s,
                        size_t offset,
                        size_t length);
const char* lxs_ssubstr_nocopy(lxs_string* const s,
                               size_t offset,
                               size_t length,
                               size_t* DEFVAL(len, NULL));

void lxs_stoupper(lua_State* const L,
                  lxs_string* const s,
                  size_t DEFVAL(offset, 0u),
                  size_t DEFVAL(length, 0u));
void lxs_stolower(lua_State* const L,
                  lxs_string* const s,
                  size_t DEFVAL(offset, 0u),
                  size_t DEFVAL(length, 0u));

void lxs_strim(lua_State* const L, lxs_string* const s);
void lxs_sltrim(lua_State* const L, lxs_string* const s);
void lxs_srtrim(lua_State* const L, lxs_string* const s);

#ifdef __cplusplus
const char** lxs_ssplit(lua_State* const L,
                        lxs_string* const s,
                        const char* sep,
                        size_t sep_len,
                        int& count);
#endif

void lxs_sreverse(lua_State* const L,
                  lxs_string* const s,
                  size_t DEFVAL(offset, 0u),
                  size_t DEFVAL(length, 0u));

void lxs_sinsert(lua_State* const L,
                 lxs_string* const s,
                 size_t offset,
                 const char* str,
                 size_t DEFVAL(len, 0u));

void lxs_sshrink(lua_State* const L, lxs_string* const s, size_t len);
void lxs_sexpand(lua_State* const L, lxs_string* const s, size_t len);

void lxs_sreplace(lua_State* const L,
                  lxs_string* const s,
                  size_t offset,
                  const char* str,
                  size_t DEFVAL(len, 0u));

void lxs_sremove(lua_State* const L,
                 lxs_string* const s,
                 size_t DEFVAL(offset, 0u),
                 size_t DEFVAL(length, 0u));

void lxs_sappend_repeat(lua_State* const L,
                        lxs_string* const s,
                        size_t count,
                        const char* str,
                        size_t DEFVAL(len, 0u));

uint32_t lxs_shash(lxs_string* const s);


//------------------------------------------------------------------------------

void lxs_scheck_range(lua_State* const L,
                      lxs_string* const s,
                      size_t offset,
                      size_t length);

void lxs_sfreebuffer(lua_State* const L, lxs_string* s);
lxs_string* lxs_snewbuffer(lua_State* const L, size_t DEFVAL(capacity, 0));

lxs_string* lxs_stobuffer(lua_State* const L, int narg);
lxs_string* lxs_scheckbuffer(lua_State* const L, int narg);

const char* lxs_schecklstring(lua_State* const L,
                              int narg,
                              size_t* DEFVAL(len, NULL));
const char* lxs_soptlstring(lua_State* const L,
                            int narg,
                            const char* def,
                            size_t* DEFVAL(len, NULL));

const char* lxs_stolstring(lua_State* const L,
                           int narg,
                           size_t* DEFVAL(len, NULL));


//------------------------------------------------------------------------------

FILE* lxs_sfload(lua_State* const L,
                    lxs_string* const s,
                    const char* path,
                    size_t DEFVAL(max_len, 0));

size_t lxs_sfskipbom(lua_State* const L, FILE* const f, lxs_string* const s);

size_t lxs_sfread(lua_State* const L,
                  FILE* const f,
                  lxs_string* const s,
                  size_t size);


//==============================================================================

#ifdef __cplusplus
extern "C++" {

template<typename T>
static inline T min(T lhs, T rhs)
{
    return (lhs < rhs) ? lhs : rhs;
}

template<typename T>
static inline T max(T lhs, T rhs)
{
    return (lhs > rhs) ? lhs : rhs;
}

template<typename T>
static inline T clamp(T value, T min, T max)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}

static inline void lxs_sflines(lua_State* const L,
                               const char* path,
                               bool (*handler)(lxs_string&),
                               size_t capacity = 1024u)
{
    lxs_assert(L, L);
    lxs_assert(L, path);
    lxs_assert(L, handler);

    FILE* f = fopen(path, "r");
    if (!f)
        return;

    lxs_string* line = lxs_screate(L, capacity, true);
    lxs_assert(L, line);
    while (!feof(f))
    {
        lxs_sclear(L, line);
        fgets(line->data, line->cap - 1u, f);

        if (lxs_sends_withc(line, '\n'))
            line->len -= 1u;
        if (lxs_sends_withc(line, '\r'))
            line->len -= 1u;

        if (!handler(*line))
            break;
    }
}

#if 0
class LXsString : public lxs_string
{
public:
    //-- Constructor & Destructor ----------------------------------------------

    LXsString()
    {
        cap  = 0u;
        len  = 0u;
        data = NULL;
    }

    LXsString(size_t capacity)
    {
        lxs_sinit(lxs_getstate(), this, capacity);
    }

    LXsString(const char* str)
    {
        assert(str);

        cap  = strlen(str) + 1u;
        len  = strlen(str);
        data = const_cast<char*>(str);
    }
    
    LXsString(const char* str, size_t len)
    {
        assert(str);
        assert(strlen(str) >= len);

        cap  = strlen(str) + 1u;
        len  = min(len, strlen(str));
        data = const_cast<char*>(str);
    }

    LXsString(lxs_string* const s)
    {
        assert(s);

        cap  = s->cap;
        len  = s->len;
        data = s->data;
    }
    
    LXsString(lxs_string* const s, size_t len)
    {
        assert(s);

        cap  = s->cap;
        len  = min(len, s->len);
        data = s->data;
    }
    
    LXsString(lxs_string* const s, size_t offset, size_t length)
    {
        assert(s);
        assert(&s->data[offset]);
        assert(&s->data[offset + length]);

        cap    = s->cap;
        length = length;
        data   = &s->data[offset];
    }

    LXsString(const LXsString& other)
    {
        cap  = other.cap;
        len  = other.len;
        if (other.cap > 0u)
        {
            lxs_sinit(lxs_getstate(), this, other.len + 1u);
            memcpy(data, other.data, other.len);
        }
        else
        {
            data = NULL;
        }
    }

    ~LXsString()
    {
        if (data)
        {
            luaM_freemem(lxs_getstate(), data, cap);
            data = NULL;
            cap  = 0u;
        }
    }

    //-- Operators -------------------------------------------------------------

    inline const LXsString& operator= (char c)
    {
        lxs_sclear(lxs_getstate(), this);
        lxs_sappendc(lxs_getstate(), this, c);
        return *this;
    }

    inline const LXsString& operator= (const char *s)
    {
        assert(s);

        lxs_sclear(lxs_getstate(), this);
        lxs_sappend(lxs_getstate(), this, s, strlen(s));
        return *this;
    }

    inline const LXsString& operator= (const LXsString& s)
    {
        lxs_sclear(lxs_getstate(), this);
        lxs_sappend(lxs_getstate(), this, s.data, s.len);
        return *this;
    }

    inline const LXsString& operator= (const lxs_string& s)
    {
        lxs_sclear(lxs_getstate(), this);
        lxs_sappend(lxs_getstate(), this, s.data, s.len);
        return *this;
    }

    inline const LXsString& operator+= (char c)
    {
        lxs_sappendc(lxs_getstate(), this, c);
        return *this;
    }

    inline const LXsString& operator+= (const char *s)
    {
        assert(s);
        lxs_sappend(lxs_getstate(), this, s, strlen(s));
        return *this;
    }

    inline const LXsString& operator+= (const LXsString& s)
    {
        lxs_sappend(lxs_getstate(), this, s.data, s.len);
        return *this;
    }

    inline const LXsString& operator+= (const lxs_string& s)
    {
        lxs_sappend(lxs_getstate(), this, s.data, s.len);
        return *this;
    }

    inline const LXsString& operator*= (int count)
    {
        lxs_sappend_repeat(lxs_getstate(), this, count, data, len);
        return *this;
    }

    inline bool operator== (const LXsString& s) const
    {
        return lxs_sequal((lxs_string*)this, (lxs_string*)&s);
    }

    inline bool operator== (const char* s) const
    {
        assert(s);

        return lxs_sequals((lxs_string*)this, s);
    }

    inline bool operator!= (const LXsString& s) const
    {
        return !lxs_sequal((lxs_string*)this, (lxs_string*)&s);
    }

    inline bool operator!= (const char* s) const
    {
        assert(s);

        return !lxs_sequals((lxs_string*)this, s);
    }

    inline bool operator< (const LXsString& s) const
    {
        return lxs_scompare((lxs_string*)this, s.data, s.len) < 0;
    }

    inline bool operator< (const char* s) const
    {
        assert(s);

        return lxs_scompare((lxs_string*)this, s) < 0;
    }

    inline bool operator> (const LXsString& s) const
    {
        return lxs_scompare((lxs_string*)this, s.data, s.len) > 0;
    }

    inline bool operator> (const char* s) const
    {
        assert(s);

        return lxs_scompare((lxs_string*)this, s) > 0;
    }

    inline bool operator<= (const LXsString& s) const
    {
        return lxs_scompare((lxs_string*)this, s) <= 0;
    }

    inline bool operator<= (const char* s) const
    {
        assert(s);

        return lxs_scompare((lxs_string*)this, s) <= 0;
    }

    inline bool operator>= (const LXsString& s) const
    {
        return lxs_scompare((lxs_string*)this, s) >= 0;
    }

    inline bool operator>= (const char* s) const
    {
        assert(s);

        return lxs_scompare((lxs_string*)this, s) >= 0;
    }

    inline operator const char* () const
    {
        return const_cast<const char*>(data);
    }

    inline char* operator[] (size_t offset) const
    {
        return &data[offset];
    }

    //-- Methods ---------------------------------------------------------------

    inline char* at(size_t offset) const
    {
        if (data == NULL || offset >= len)
            return NULL;
        return &data[offset];
    }

    inline const LXsString& shrink_to_fit()
    {
        lxs_sshrink_to_fit(lxs_getstate(), this);
        return *this;
    }

    inline const LXsString& append(char c)
    {
        lxs_sappendc(lxs_getstate(), this, c);
        return *this;
    }

    inline const LXsString& append(const char* s, size_t len = 0u)
    {
        assert(s);
        assert(len == 0u || strlen(s) >= len);

        lxs_sappend(lxs_getstate(), this, s, len);
        return *this;
    }

    inline const LXsString& append(lxs_string* const s)
    {
        assert(s);

        lxs_sappend(lxs_getstate(), this, s->data, s->len);
        return *this;
    }

    inline const LXsString& append(const LXsString& s)
    {
        lxs_sappend(lxs_getstate(), this, s.data, s.len);
        return *this;
    }

    inline const LXsString& append_repeat(size_t count,
                                          const char* s,
                                          size_t len = 0u)
    {
        assert(s);
        assert(len == 0u || strlen(s) >= len);

        lxs_sappend_repeat(lxs_getstate(), this, count, s, len);
        return *this;
    }

    inline const LXsString& append_repeat(size_t count, lxs_string* const s)
    {
        assert(s);

        lxs_sappend_repeat(lxs_getstate(), this, count, s->data, s->len);
        return *this;
    }

    inline const LXsString& append_repeat(size_t count, const LXsString& s)
    {
        lxs_sappend_repeat(lxs_getstate(), this, count, s.data, s.len);
        return *this;
    }

    inline const LXsString& append_format(const char* fmt, ...)
    {
        assert(fmt);

        va_list args;
        va_start(args, fmt);
        lxs_sappend_vformat(lxs_getstate(), this, fmt, args);
        va_end(args);
        return *this;
    }

    inline const LXsString& append_format(const char* fmt, va_list args)
    {
        assert(fmt);
        assert(args);

        lxs_sappend_vformat(lxs_getstate(), this, fmt, args);
        return *this;
    }

    inline const LXsString& clear()
    {
        lxs_sclear(lxs_getstate(), this);
        return *this;
    }

    inline int compare(const char* s, size_t len = 0u) const 
    {
        assert(s);
        assert(len == 0u || strlen(s) >= len);

        return lxs_scompare((lxs_string*)this, s, len);
    }

    inline int compare(lxs_string* const s) const 
    {
        assert(s);

        return lxs_scompare((lxs_string*)this, s->data, s->len);
    }

    inline int compare(const LXsString& s) const 
    {
        return lxs_scompare((lxs_string*)this, s.data, s.len);
    }

    inline int compare_nocase(const char* s, size_t len = 0u) const 
    {
        assert(s);
        assert(len == 0u || strlen(s) >= len);

        return lxs_sicompare((lxs_string*)this, s, len);
    }

    inline int compare_nocase(lxs_string* const s) const 
    {
        assert(s);

        return lxs_sicompare((lxs_string*)this, s->data, s->len);
    }

    inline int compare_nocase(const LXsString& s) const 
    {
        return lxs_sicompare((lxs_string*)this, s.data, s.len);
    }

    inline char* find_first_of(char c, size_t offset = 0u) const
    {
        return lxs_sfindc((lxs_string*)this, c, offset);
    }

    inline char* find_first_of(const char* s, size_t offset = 0u) const
    {
        return lxs_sfind((lxs_string*)this, s, offset);
    }

    inline char* find_first_of(lxs_string* const s, size_t offset = 0u) const
    {
        return lxs_sfind((lxs_string*)this, s->data, offset);
    }

    inline char* find_first_of(const LXsString& s, size_t offset = 0u) const
    {
        return lxs_sfind((lxs_string*)this, s.data, offset);
    }

    inline char* find_last_of(char c, size_t offset = 0u) const
    {
        return lxs_srfindc((lxs_string*)this, c, offset);
    }

    inline bool starts_with(char c) const
    {
        return lxs_sstarts_withc((lxs_string*)this, c);
    }

    inline bool starts_with(const char* s, size_t len = 0u) const
    {
        assert(s);
        assert(len == 0u || strlen(s) >= len);

        return lxs_sstarts_with((lxs_string*)this, s, len);
    }

    inline bool starts_with(lxs_string* const s) const
    {
        assert(s);

        return lxs_sstarts_with((lxs_string*)this, s->data, s->len);
    }

    inline bool starts_with(const LXsString& s) const
    {
        return lxs_sstarts_with((lxs_string*)this, s.data, s.len);
    }

    inline bool ends_with(char c) const
    {
        return lxs_sends_withc((lxs_string*)this, c);
    }

    inline bool ends_with(const char* s, size_t len = 0u) const
    {
        assert(s);
        assert(len == 0u || strlen(s) >= len);

        return lxs_sends_with((lxs_string*)this, s, len);
    }

    inline bool ends_with(lxs_string* const s) const
    {
        assert(s);

        return lxs_sends_with((lxs_string*)this, s->data, s->len);
    }

    inline bool ends_with(const LXsString& s) const
    {
        return lxs_sends_with((lxs_string*)this, s.data, s.len);
    }

    inline bool starts_with_nocase(const char* s, size_t len = 0u) const
    {
        assert(s);
        assert(len == 0u || strlen(s) >= len);

        return lxs_sistarts_with((lxs_string*)this, s, len);
    }

    inline bool starts_with_nocase(lxs_string* const s) const
    {
        assert(s);

        return lxs_sistarts_with((lxs_string*)this, s->data, s->len);
    }

    inline bool starts_with_nocase(const LXsString& s) const
    {
        return lxs_sistarts_with((lxs_string*)this, s.data, s.len);
    }

    inline bool ends_with_nocase(const char* s, size_t len = 0u) const
    {
        assert(s);
        assert(len == 0u || strlen(s) >= len);

        return lxs_siends_with((lxs_string*)this, s, len);
    }

    inline bool ends_with_nocase(lxs_string* const s) const
    {
        assert(s);

        return lxs_siends_with((lxs_string*)this, s->data, s->len);
    }

    inline bool ends_with_nocase(const LXsString& s) const
    {
        return lxs_siends_with((lxs_string*)this, s.data, s.len);
    }

    inline const LXsString& to_upper(size_t offset = 0u, size_t length = 0u)
    {
        lxs_stoupper(lxs_getstate(), this, offset, length);
        return *this;
    }

    inline const LXsString& to_lower(size_t offset = 0u, size_t length = 0u)
    {
        lxs_stolower(lxs_getstate(), this, offset, length);
        return *this;
    }

    inline const LXsString& trim()
    {
        lxs_strim(lxs_getstate(), this);
        return *this;
    }

    inline const LXsString& left_trim()
    {
        lxs_sltrim(lxs_getstate(), this);
        return *this;
    }

    inline const LXsString& right_trim()
    {
        lxs_srtrim(lxs_getstate(), this);
        return *this;
    }

    inline const LXsString& reverse(size_t offset = 0u, size_t length = 0u)
    {
        lxs_sreverse(lxs_getstate(), this, offset, length);
        return *this;
    }

    inline const LXsString& insert(size_t offset, const char* s, size_t len = 0u)
    {
        assert(s);
        assert(len == 0u || strlen(s) >= len);

        lxs_sinsert(lxs_getstate(), this, offset, s, len);
        return *this;
    }

    inline const LXsString& insert(size_t offset, lxs_string* const s)
    {
        assert(s);

        lxs_sinsert(lxs_getstate(), this, offset, s->data, s->len);
        return *this;
    }

    inline const LXsString& insert(size_t offset, const LXsString& s)
    {
        lxs_sinsert(lxs_getstate(), this, offset, s.data, s.len);
        return *this;
    }

    inline const LXsString& shrink_buffer(size_t amount)
    {
        lxs_sshrink(lxs_getstate(), this, amount);
        return *this;
    }

    inline const LXsString& expand_buffer(size_t amount)
    {
        lxs_sexpand(lxs_getstate(), this, amount);
        return *this;
    }

    inline const LXsString& replace(size_t offset, const char* s, size_t len = 0u)
    {
        assert(s);
        assert(len == 0u || strlen(s) >= len);

        lxs_sreplace(lxs_getstate(), this, offset, s, len);
        return *this;
    }

    inline const LXsString& replace(size_t offset, lxs_string* const s)
    {
        assert(s);

        lxs_sreplace(lxs_getstate(), this, offset, s->data, s->len);
        return *this;
    }

    inline const LXsString& replace(size_t offset, const LXsString& s)
    {
        lxs_sreplace(lxs_getstate(), this, offset, s.data, s.len);
        return *this;
    }

    inline const LXsString& remove(size_t offset = 0u, size_t length = 0u)
    {
        lxs_sremove(lxs_getstate(), this, offset, length);
        return *this;
    }

    inline uint32_t hash() const
    {
        return lxs_shash((lxs_string*)this);
    }
}; // cs LXsString
#endif
}; // extern "C++"
#endif

//==============================================================================

#define lxs_sappendl(L, s, literal) \
    lxs_sappend(L, s, "" literal, (sizeof(literal) / sizeof(char)) - 1)

#define lxs_sstarts_withl(s, literal) \
    lxs_sstarts_with(s, "" literal, (sizeof(literal) / sizeof(char)) - 1)
#define lxs_sends_withl(s, literal) \
    lxs_sends_with(s, "" literal, (sizeof(literal) / sizeof(char)) - 1)

#define lxs_ssplitl(L, s, literal, count) \
    lxs_ssplit(L, s, "" literal, (sizeof(literal) / sizeof(char)) - 1, count)


//==============================================================================

#endif // lxs_string_hpp