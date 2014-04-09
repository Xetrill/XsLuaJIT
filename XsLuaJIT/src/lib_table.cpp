/*
** $Id: ltablib.c,v 1.38.1.3 2008/02/14 16:46:58 roberto Exp $
** Library for Table Manipulation
** See Copyright Notice in lua.h
*/

extern "C"
{

#include <stddef.h>

#define ltablib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#include "lobject.h"

#include "lxsext.h"
#include "lxs_def.h"
#include "lxs_string.hpp"

#define aux_getn(L,n)	(luaL_checktype(L, n, LUA_TTABLE), luaL_getn(L, n))


static int libE_foreachi(lua_State *L)
{
    int i;
    int n = aux_getn(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    for (i = 1; i <= n; ++i)
    {
        lua_pushvalue(L, 2);  /* function */
        lua_pushinteger(L, i);  /* 1st argument */
        lua_rawgeti(L, 1, i);  /* 2nd argument */
        lua_call(L, 2, 1);
        if (!lua_isnil(L, -1))
            return 1;

        lua_pop(L, 1);  /* remove nil result */
    }
    return 0;
}

static int libE_foreach(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_pushnil(L);  /* first key */
    while (lua_next(L, 1))
    {
        lua_pushvalue(L, 2);   /* function */
        lua_pushvalue(L, -3);  /* key */
        lua_pushvalue(L, -3);  /* value */
        lua_call(L, 2, 1);
        if (!lua_isnil(L, -1))
            return 1;

        lua_pop(L, 2);  /* remove value and result */
    }
    return 0;
}

static int libE_maxn (lua_State *L) {
  lua_Number max = 0;
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_pushnil(L);  /* first key */
  while (lua_next(L, 1)) {
    lua_pop(L, 1);  /* remove value */
    if (lua_type(L, -1) == LUA_TNUMBER) {
      lua_Number v = lua_tonumber(L, -1);
      if (v > max) max = v;
    }
  }
  lua_pushnumber(L, max);
  return 1;
}

static int libE_getn (lua_State *L) {
  lua_pushinteger(L, aux_getn(L, 1));
  return 1;
}

static int libE_setn (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
#ifndef luaL_setn
  luaL_setn(L, 1, luaL_checkint(L, 2));
#else
  lxs_error(L, LUA_QL("libE_setn") " is obsolete");
#endif
  lua_pushvalue(L, 1);
  return 1;
}

static int libE_insert (lua_State *L) {
  int e = aux_getn(L, 1) + 1;  /* first empty element */
  int pos;  /* where to insert new element */
  switch (lua_gettop(L)) {
    case 2: {  /* called with only 2 arguments */
      pos = e;  /* insert new element at the end */
      break;
    }
    case 3: {
      int i;
      pos = luaL_checkint(L, 2);  /* 2nd argument is the position */
      if (pos > e) e = pos;  /* `grow' array if necessary */
      for (i = e; i > pos; i--) {  /* move up elements */
        lua_rawgeti(L, 1, i-1);
        lua_rawseti(L, 1, i);  /* t[i] = t[i-1] */
      }
      break;
    }
    default: {
      return lxs_error(L, "wrong number of arguments to " LUA_QL("insert"));
    }
  }
  luaL_setn(L, 1, e);  /* new size */
  lua_rawseti(L, 1, pos);  /* t[pos] = v */
  return 0;
}

static int libE_remove (lua_State *L) {
  int e = aux_getn(L, 1);
  int pos = luaL_optint(L, 2, e);
  if (!(1 <= pos && pos <= e))  /* position is outside bounds? */
   return 0;  /* nothing to remove */
  luaL_setn(L, 1, e - 1);  /* t.n = n-1 */
  lua_rawgeti(L, 1, pos);  /* result = t[pos] */
  for ( ;pos<e; pos++) {
    lua_rawgeti(L, 1, pos+1);
    lua_rawseti(L, 1, pos);  /* t[pos] = t[pos+1] */
  }
  lua_pushnil(L);
  lua_rawseti(L, 1, e);  /* t[e] = nil */
  return 1;
}


static void addfield (lua_State *L, luaL_Buffer *b, int i) {
  lua_rawgeti(L, 1, i);
  if (!lua_isstring(L, -1))
    lxs_error(L, "invalid value (%s) at index %d in table for "
                  LUA_QL("concat"), luaL_typename(L, -1), i);
  luaL_addvalue(b);
}

static int libE_concat (lua_State *L)
{
    luaL_Buffer b;
    size_t lsep;
    int i, last;
    const char *sep = luaL_optlstring(L, 2, "", &lsep);
    luaL_checktype(L, 1, LUA_TTABLE);
    i = luaL_optint(L, 3, 1);
    last = luaL_opt(L, luaL_checkint, 4, luaL_getn(L, 1));
    luaL_buffinit(L, &b);
    for (; i < last; i++)
    {
        addfield(L, &b, i);
        luaL_addlstring(&b, sep, lsep);
    }
    if (i == last)  /* add last value (if interval was not empty) */
        addfield(L, &b, i);
    luaL_pushresult(&b);
    return 1;
}



/*
** {======================================================
** Quicksort
** (based on `Algorithms in MODULA-3', Robert Sedgewick;
**  Addison-Wesley, 1993.)
*/


static void set2 (lua_State *L, int i, int j) {
  lua_rawseti(L, 1, i);
  lua_rawseti(L, 1, j);
}

static int sort_comp (lua_State *L, int a, int b) {
  if (!lua_isnil(L, 2)) {  /* function? */
    int res;
    lua_pushvalue(L, 2);
    lua_pushvalue(L, a-1);  /* -1 to compensate function */
    lua_pushvalue(L, b-2);  /* -2 to compensate function and `a' */
    lua_call(L, 2, 1);
    res = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return res;
  }
  else  /* a < b? */
    return lua_lessthan(L, a, b);
}

static void auxsort (lua_State *L, int l, int u) {
  while (l < u) {  /* for tail recursion */
    int i, j;
    /* libE_sort elements a[l], a[(l+u)/2] and a[u] */
    lua_rawgeti(L, 1, l);
    lua_rawgeti(L, 1, u);
    if (sort_comp(L, -1, -2))  /* a[u] < a[l]? */
      set2(L, l, u);  /* swap a[l] - a[u] */
    else
      lua_pop(L, 2);
    if (u-l == 1) break;  /* only 2 elements */
    i = (l+u)/2;
    lua_rawgeti(L, 1, i);
    lua_rawgeti(L, 1, l);
    if (sort_comp(L, -2, -1))  /* a[i]<a[l]? */
      set2(L, i, l);
    else {
      lua_pop(L, 1);  /* remove a[l] */
      lua_rawgeti(L, 1, u);
      if (sort_comp(L, -1, -2))  /* a[u]<a[i]? */
        set2(L, i, u);
      else
        lua_pop(L, 2);
    }
    if (u-l == 2) break;  /* only 3 elements */
    lua_rawgeti(L, 1, i);  /* Pivot */
    lua_pushvalue(L, -1);
    lua_rawgeti(L, 1, u-1);
    set2(L, i, u-1);
    /* a[l] <= P == a[u-1] <= a[u], only need to libE_sort from l+1 to u-2 */
    i = l; j = u-1;
    for (;;) {  /* invariant: a[l..i] <= P <= a[j..u] */
      /* repeat ++i until a[i] >= P */
      while (lua_rawgeti(L, 1, ++i), sort_comp(L, -1, -2)) {
        if (i>u) lxs_error(L, "invalid order function for sorting");
        lua_pop(L, 1);  /* remove a[i] */
      }
      /* repeat --j until a[j] <= P */
      while (lua_rawgeti(L, 1, --j), sort_comp(L, -3, -1)) {
        if (j<l) lxs_error(L, "invalid order function for sorting");
        lua_pop(L, 1);  /* remove a[j] */
      }
      if (j<i) {
        lua_pop(L, 3);  /* pop pivot, a[i], a[j] */
        break;
      }
      set2(L, i, j);
    }
    lua_rawgeti(L, 1, u-1);
    lua_rawgeti(L, 1, i);
    set2(L, u-1, i);  /* swap pivot (a[u-1]) with a[i] */
    /* a[l..i-1] <= a[i] == P <= a[i+1..u] */
    /* adjust so that smaller half is in [j..i] and larger one in [l..u] */
    if (i-l < u-i) {
      j=l; i=i-1; l=i+2;
    }
    else {
      j=i+1; i=u; u=j-2;
    }
    auxsort(L, j, i);  /* call recursively the smaller one */
  }  /* repeat the routine for the larger one */
}

static int libE_sort (lua_State *L) {
  int n = aux_getn(L, 1);
  luaL_checkstack(L, 40, "");  /* assume array is smaller than 2^40 */
  if (!lua_isnoneornil(L, 2))  /* is there a 2nd argument? */
    luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_settop(L, 2);  /* make sure there is two arguments */
  auxsort(L, 1, n);
  return 0;
}


/******************************************************************************
 * ltablib.c extensions
 *****************************************************************************/

#if LUAXS_EXTEND_TABLIB

#define DEFINE_MINMAX_VEC_FUNC(NAME, GETTER, ZV)\
    static int NAME(lua_State* const L)         \
    {                                           \
        lxs_assert_stack_begin(L);              \
                                                \
        double num = ZV;                        \
                                                \
        luaL_checktype(L, 1, LUA_TTABLE);       \
        luaL_checktype(L, 2, LUA_TFUNCTION);    \
                                                \
        int len = luaL_getn(L, 1);              \
        for (int i = 1; i <= len; ++i)          \
        {                                       \
            lua_pushvalue(L, 2);                \
            lua_pushinteger(L, i);              \
            lua_rawgeti(L, 1, i);               \
            lua_call(L, 2, 1);                  \
                                                \
            num = GETTER(L, -1, num);           \
                                                \
            lua_pop(L, 1);                      \
        }                                       \
                                                \
        if (len == 0)                           \
            lua_pushnil(L);                     \
        else                                    \
            lua_pushnumber(L, num);             \
                                                \
        lxs_assert_stack_end(L, 1);             \
        return 1;                               \
    }

#define DEFINE_MINMAX_MAP_FUNC(NAME, GETTER, ZV)\
    static int NAME(lua_State* const L)         \
    {                                           \
        bool empty = true;                      \
        double num = ZV;                        \
                                                \
        luaL_checktype(L, 1, LUA_TTABLE);       \
        luaL_checktype(L, 2, LUA_TFUNCTION);    \
                                                \
        lua_settop(L, 2);                       \
                                                \
        lua_pushnil(L);                         \
        while (lua_next(L, 1))                  \
        {                                       \
            empty = false;                      \
                                                \
            lua_pushvalue(L, -2);               \
            lua_insert(L, -2);                  \
            lua_pushvalue(L, 2);                \
            lua_insert(L, -3);                  \
            lua_call(L, 2, 1);                  \
                                                \
            num = GETTER(L, -1, num);           \
                                                \
            lua_pop(L, 1);                      \
        }                                       \
                                                \
        if (empty)                              \
            lua_pushnil(L);                     \
        else                                    \
            lua_pushnumber(L, num);             \
                                                \
        lxs_assert_stack_at(L, 3);              \
        return 1;                               \
    }

DEFINE_MINMAX_VEC_FUNC(libE_mini, lxs_mind, HUGE_VAL);
DEFINE_MINMAX_VEC_FUNC(libE_maxi, lxs_maxd, 0);
#undef DEFINE_MINMAX_VEC_FUNC
DEFINE_MINMAX_MAP_FUNC(libE_min, lxs_mind, HUGE_VAL);
DEFINE_MINMAX_MAP_FUNC(libE_max, lxs_maxd, 0);
#undef DEFINE_MINMAX_MAP_FUNC

/// table.create([vector_size[, hashmap_size]])
///
/// Creates a new table and prepares it by allocating memory ahead of
/// time/its usage.
///
/// The first argument *vector_size* specifies how many vector-like entries will
/// be pre-allocated. Lua understands tables who's keys are numbers from 1-n
/// without any interruptions in between as vectors.
/// The second argument *hashmap_size* specifies how many hashed entries are
/// pre-allocated, that is any non-vector entry.
/// 
/// This is useful to better performance by reducing memory (re)allocations and
/// can be utilized for any table created at runtime who's entry count can be
/// predetermined.
static int libE_create(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    int listSize = luaL_optinteger(L, 1, 0);
    if (listSize < 0)
        luaL_argerror(L, 1, "Cannot be less than zero.");

    int hashSize = luaL_optinteger(L, 2, 0);
    if (hashSize < 0)
        luaL_argerror(L, 2, "Cannot be less than zero.");

    lua_createtable(L, listSize, hashSize);

    lxs_assert_stack_end(L, 1);
    return 1;
}

/// table.countall(table)
///
/// Returns the total number of entries in the *table*, this includes the vector
/// part of the table as well as the hashmap part.
/// If you have an vector-like table use table.getn or the #-operator instead,
/// as those have better performance.
/// But, if you have a hashmap-like table, you can use this function instead
/// of iterating over it.
///
/// Usage example:
///     table.countall(
///         {
///             ['a'] = 1,
///             ['b'] = 2,
///             ['c'] = 3,
///             [1] = 4,
///             [2] = 5,
///             [3] = 6,
///         }
///    )
///    --> 6
static int libE_countall(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    luaL_checktype(L, 1, LUA_TTABLE);

    lua_settop(L, 1);

    int count = 0;
    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        ++count;

        lua_pop(L, 1);
    }

    lxs_assert(L, count >= static_cast<int>(lua_objlen(L, 1)));

    lua_pushinteger(L, count);

    lxs_assert_stack_end(L, 1);
    return 1;
}

static int lxs_tablib_joinall(lua_State* const L)
{
    lxs_assert_stack_begin(L);
    size_t      dlen;
    const char* delim = luaL_checklstring(L, 1, &dlen);

    int top = lua_gettop(L);
    if (top <= 2)
        luaL_argerror(L, 2, "need at least 2 arguments");
    else if (lua_isnoneornil(L, 2))
        luaL_argerror(L, 2, "cannot be nil");

    xbuf_decl(b);
    xbuf_init(L, b);
    for (int i = 2; i <= top; ++i)
    {
        size_t      elen;
        const char* estr = luaL_checklstring(L, i, &elen);

        if (elen > 0)
            xbuf_addlstring(L, b, estr, elen);

        if (dlen > 0 && i + 1 <= top)
            xbuf_addlstring(L, b, delim, dlen);
    }
    xbuf_pushresult(L, b);

#if LUAXS_STR_PERSISTENT_BUFFER
    lxs_assert_stack_end(L, 1);
#else
    lxs_assert_stack_end(L, 1 + (b.lvl - 1));
#endif
    return 1;
}

/// table.joini(table[, delimiter])
///
/// Concatenables all entries of a *table*, optionally separating them using a
/// *delimiter* string.
/// Only the vector part of a table is traversed.
///
/// Usage example:
///     table.joini({ ''            }, ',') --> ''
///     table.joini({ '', ''        }, ',') --> ','
///     table.joini({ 'a', 'b'      }, ',') --> 'a,b'
static int libE_joini(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    luaL_checktype(L, 1, LUA_TTABLE);

    size_t      dlen;
    const char* delim = luaL_optlstring(L, 2, NULL, &dlen);

    xbuf_decl(b);
    xbuf_init(L, b);

    int vlen = lua_objlen(L, 1);
    for (int i = 1; i <= vlen; ++i)
    {
        lua_rawgeti(L, 1, i);
    
        size_t      elen;
        const char* estr = luaL_checklstring(L, -1, &elen);

        if (elen > 0)
            xbuf_addlstring(L, b, estr, elen);

        if (dlen > 0 && i + 1 <= vlen)
            xbuf_addlstring(L, b, delim, dlen);

        lua_pop(L, 1);
    }
    xbuf_pushresult(L, b);

#if LUAXS_STR_PERSISTENT_BUFFER
    lxs_assert_stack_end(L, 1);
#else
    lxs_assert_stack_end(L, 1 + (b.lvl - 1));
#endif
    return 1;
}

/// table.join(table[, delimiter])
///
/// Concatenables all entries of a *table*, optionally separating them using a
/// *delimiter* string.
/// The table is traversed using *next* and which means traversal order is
/// undefined or the non-vector part of the table.
///
/// Usage example:
///     table.join({ ''            }, ',') --> ''
///     table.join({ '', ''        }, ',') --> ','
///     table.join({ 'a', 'b'      }, ',') --> 'a,b'
static int libE_join(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    luaL_checktype(L, 1, LUA_TTABLE);

    size_t      dlen;
    const char* delim = luaL_optlstring(L, 2, NULL, &dlen);

    xbuf_decl(b);
    xbuf_init(L, b);

    bool add_delim = false;

    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        if (add_delim)
            xbuf_addlstring(L, b, delim, dlen);
        else if (dlen > 0)
            add_delim = true;

        size_t      elen;
        const char* estr = luaL_checklstring(L, -1, &elen);

        if (elen > 0)
            xbuf_addlstring(L, b, estr, elen);

        lua_pop(L, 1);
    }
    xbuf_pushresult(L, b);

#if LUAXS_STR_PERSISTENT_BUFFER
    lxs_assert_stack_end(L, 1);
#else
    lxs_assert_stack_end(L, 1 + (b.lvl - 1));
#endif
    return 1;
}

/// table.counti(table, predicate)
///
/// Iterates over the vector part of a *table* invoking a *predicate* function
/// for each index-value pair to determine how many pairs match the predicate.
///
/// The predicate has the following signature: *predicate(index, value)* and
/// must return a boolean.
///
/// Usage example:
///     table.counti(
///         { 1, 2, 3 },
///         function (i, v)
///             return v % 2 ~= 0
///         end
///     )
///     --> 2
static int libE_counti(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    int count = 0;

    int len = lua_objlen(L, 1);
    for (int i = 1; i <= len; ++i)
    {
        lua_pushvalue(L, 2);    // predicate func
        lua_pushinteger(L, i);  // arg-1: index
        lua_rawgeti(L, 1, i);   // arg-2: value
        lua_call(L, 2, 1);

        if (lua_toboolean(L, -1) != 0)
            ++count;

        lua_pop(L, 1);
    }

    lua_pushinteger(L, count);

    lxs_assert_stack_end(L, 1);
    return 1;
}

/// table.count(table, predicate)
///
/// Iterates over a *table* invoking a *predicate* function for each key-value
/// pair to determine how many pairs match the predicate.
///
/// The predicate has the following signature: *predicate(key, value)* and must
/// return a boolean.
///
/// Usage example:
///     table.count(
///         {
///             ['a'] = 1,
///             ['b'] = 2,
///             ['c'] = 3
///         },
///         function (k, v)
///             return v % 2 ~= 0
///         end
///     )
///     --> 2
static int libE_count(lua_State* const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_settop(L, 2);

    int count = 0;

    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_pushvalue(L, 2);
        lua_insert(L, -3);
        lua_call(L, 2, 1);

        if (lua_toboolean(L, -1) != 0)
            ++count;

        lua_pop(L, 1);
    }

    lua_pushinteger(L, count);

    lxs_assert_stack_at(L, 3);
    return 1;
}

/// table.sum(table, selector)
///
/// Iterates over the vector part of a *table* summing up values using a
/// *selector* function.
/// For empty tables nil is returned.
/// 
/// The selector has the following signature: *selector(index, value)* and must
/// return the number to sum up.
///
/// Usage example:
///     table.sumi(
///         { 1, 2, 3 },
///         function (k, v)
///             return v % 2 ~= 0 and v or 0
///         end
///     )
///     --> 4
static int libE_sumi(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    double sum = 0;

    int len = luaL_getn(L, 1);
    for (int i = 1; i <= len; ++i)
    {
        lua_pushvalue(L, 2);
        lua_pushinteger(L, i);
        lua_rawgeti(L, 1, i);
        lua_call(L, 2, 1);

        sum += (luaL_checktype(L, -1, LUA_TNUMBER), lua_tonumber(L, -1));

        lua_pop(L, 1);
    }

    if (len == 0)
        lua_pushnil(L);
    else
        lua_pushnumber(L, sum);

    lxs_assert_stack_end(L, 1);
    return 1;
}

/// table.sum(table, selector)
///
/// Iterates over a *table* summing up values using a *selector* function.
/// For empty tables nil is returned.
/// 
/// The selector has the following signature: *selector(key, value)* and must
/// return the number to sum up.
///
/// Usage example:
///     table.sum(
///         {
///             ['a'] = 1,
///             ['b'] = 2,
///             ['c'] = 3
///         },
///         function (k, v)
///             return v % 2 ~= 0 and v or 0
///         end
///     )
///     --> 4
static int libE_sum(lua_State* const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_settop(L, 2);

    bool empty = true;
    double sum = 0;

    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        empty = false;

        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_pushvalue(L, 2);
        lua_insert(L, -3);
        lua_call(L, 2, 1);

        sum += (luaL_checktype(L, -1, LUA_TNUMBER), lua_tonumber(L, -1));

        lua_pop(L, 1);
    }

    if (empty)
        lua_pushnil(L);
    else
        lua_pushnumber(L, sum);
    lxs_assert_stack_at(L, 3);
    return 1;
}

/// table.findi(table, selector)
///
/// Iterators over the vector part of a *table* invokding a *selector* function
/// to match a key-value pair to some custom criteria and returning its key.
/// If no match is found 0 is returned.
/// 
/// The selector has the following signature: *selector(key, value)* and must
/// return a boolean.
///
/// Example usage:
///     table.findi({ 1, 2, 3 }, function (k, v) return v % 2 == 0 end) --> 2
///     table.findi({ 1, 2, 3 }, function (k, v) return v > 3 end) --> 0
static int libE_findi(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    int len = lua_objlen(L, 1);
    for (int i = 1; i <= len; ++i)
    {
        lua_pushvalue(L, 2);    // func
        lua_pushinteger(L, i);  // index
        lua_rawgeti(L, 1, i);   // value
        lua_call(L, 2, 1);

        if (lua_toboolean(L, -1) != 0)
        {
            lua_pushinteger(L, i);
            lxs_assert_stack_end(L, 2);
            return 1;
        }

        lua_pop(L, 1);
    }

    lua_pushinteger(L, 0);
    lxs_assert_stack_end(L, 1);
    return 1;
}

/// table.anyi(table, predicate)
///
/// Iterates over the vector part of a *table* invoking a *predicate* function
/// to check whether a index-value pair matches some specific criteria.
/// Returns *true* if any match was found and *false* otherwise.
/// 
/// The predicate has the following signature: *predicate(key, value)* and must
/// return a boolean.
///
/// Example usage:
///     table.anyi(
///         {
///             ['a'] = 1,
///             ['b'] = 2,
///             ['c'] = 3
///         },
///         function (k, v)
///             return v % 2 == 0
///         end
///     )
///     --> true
///
/// TODO: return the matched index as well OR remove in favor of findi()
static int libE_anyi(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    int len = luaL_getn(L, 1);
    for (int i = 1; i <= len; ++i)
    {
        lua_pushvalue(L, 2);
        lua_pushinteger(L, i);
        lua_rawgeti(L, 1, i);
        lua_call(L, 2, 1);

        if (lua_toboolean(L, -1) != 0)
        {
            lua_pushboolean(L, 1);
            lxs_assert_stack_end(L, 2);
            return 1;
        }

        lua_pop(L, 1);
    }

    lua_pushboolean(L, 0);
    lxs_assert_stack_end(L, 1);
    return 1;
}

/// table.find(table, selector)
///
/// Iterators over a *table* invokding a *selector* function to match a
/// key-value pair to some custom criteria and returning its key.
/// If no match is found nil is returned.
/// 
/// The selector has the following signature: *selector(key, value)* and must
/// return a boolean.
///
/// Example usage:
///     table.find(
///         {
///             ['a'] = 1,
///             ['b'] = 2,
///             ['c'] = 3
///         },
///         function (k, v)
///             return v % 2 == 0
///         end
///     )
///     --> 'b'
static int libE_find(lua_State* const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_settop(L, 2);

    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_pushvalue(L, 2);
        lua_insert(L, -3);
        lua_call(L, 2, 1);

        if (lua_toboolean(L, -1) != 0)
        {
            lua_pop(L, 1);
            lxs_assert_stack_at(L, 3);
            return 1;
        }

        lua_pop(L, 1);
    }

    lua_pushnil(L);
    lxs_assert_stack_at(L, 3);
    return 1;
}

/// table.any(table, predicate)
///
/// Iterates over a *table* invoking a *predicate* function to check whether
/// a key-value pair matches some specific criteria.
/// Returns *true* if any match was found and *false* otherwise.
/// 
/// The predicate has the following signature: *predicate(key, value)* and must
/// return a boolean.
///
/// Example usage:
///     table.any(
///         {
///             ['a'] = 1,
///             ['b'] = 2,
///             ['c'] = 3
///         },
///         function (k, v)
///             return v % 2 == 0
///         end
///     )
///     --> true
///
/// TODO: return the matched key as well OR remove in favor of find()
static int libE_any(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_settop(L, 2);

    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_pushvalue(L, 2);
        lua_insert(L, -3);
        lua_call(L, 2, 1);
        if (lua_toboolean(L, -1) != 0)
        {
            lua_pushboolean(L, 1);
            lxs_assert_stack_at(L, 5);
            return 1;
        }

        lua_pop(L, 1);
    }

    lua_pushboolean(L, 0);
    lxs_assert_stack_at(L, 3);
    return 1;
}

/// table.mapi(table, modifier)
///
/// Iterates over the vector part of a *table* invoking a *modifier* function
/// for each index. The return value of the modifier will be the new entry
/// value.
/// The modifier has the following signature: *modifier(index, value)*.
///
/// Example usage:
///     table.mapi(
///         { 1, 2, 3 },
///         function (i, v)
///             return v % 2 == 0 and v * v or v
///         end
///     )
///     --> {
///         [1] = 1,
///         [2] = 4,
///         [3] = 3
///     }
static int libE_mapi(lua_State* const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_settop(L, 2);

    int len = luaL_getn(L, 1);
    lua_createtable(L, max(0, len - 1), 0);

    for (int i = 1; i <= len; ++i)
    {
        lua_pushvalue(L, 2);
        lua_pushinteger(L, i);
        lua_rawgeti(L, 1, i);
        lua_call(L, 2, 1);

        lua_rawseti(L, 3, i);
    }

    lxs_assert_stack_at(L, 3);
    return 1;
}

/// table.map(table, modifier[, raw = false])
///
/// Iterates over a *table* invoking a *modifier* function for each key. The
/// return value of the modifier will be the new key value.
/// The modifier has the following signature: *modifier(key, value)*.
/// Keys are assigned *raw* if the third parameter is *true*, bypassing any
/// eventual metatable.
///
/// Example usage:
///     table.map(
///         {
///             ['a'] = 1,
///             ['b'] = 2,
///             ['c'] = 3
///         },
///         function (k, v)
///             return v % 2 == 0 and v * v or v
///         end
///     )
///     --> {
///         ['a'] = 1,
///         ['b'] = 4,
///         ['c'] = 3
///     }
static int libE_map(lua_State* const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    bool raw = luaL_optbool(L, 3, false);
    
    lua_settop(L, 2);
    lua_createtable(L, 0, 0);

    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_pushvalue(L, 2);
        lua_insert(L, -3);
        lua_call(L, 2, 1);

        if (raw)
            lua_rawset(L, 3);
        else
            lua_settable(L, 3);
    }

    lxs_assert_stack_at(L, 3);
    return 1;
}

/// table.retaini(table, selector)
///
/// Iterates over the vector part of a *table* invoking a *selector* function to
/// decide whether each index will be retained or be removed. Leaving only
/// indices that passed the selector check remaining.
///
/// The selector has the following signature: *selector(index, value)* and must
/// return a boolean.
/// Returning *true* will cause the index to remain, returning *false* and the
/// index will be removed.
///
/// Example usage:
///     table.retaini(
///         { 1, 2, 3 },
///         function (i, v)
///             return v % 2 == 0
///         end
///     )
///     --> { [1] = 2 }
static int libE_retaini(lua_State* const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_settop(L, 2);

    int len = luaL_getn(L, 1);
    int j   = 0;
    lua_createtable(L, max(0, len - 1), 0);

    for (int i = 1; i <= len; ++i)
    {
        lua_pushvalue(L, 2);
        lua_pushinteger(L, i);
        lua_rawgeti(L, 1, i);
        lua_call(L, 2, 1);

        if (lua_toboolean(L, -1) != 0)
        {
            lua_rawgeti(L, 1, i);
            lua_rawseti(L, 3, ++j);
        }

        lua_pop(L, 1);
    }

    lxs_assert_stack_at(L, 3);
    return 1;
}

/// table.retain(table, selector[, raw = false])
///
/// Iterates over a *table* invoking a *selector* function to decide whether
/// each key-value pair will be retained or be removed. Leaving only key-value
/// pairs that passed the selector check remaining.
///
/// The selector has the following signature: *selector(key, value)* and must
/// return a boolean.
/// Returning *true* will cause the key to remain, returning
/// *false* and the key will be removed.
///
/// Keys are removed *raw* if the third parameter is *true*, bypassing any
/// eventual metatable.
///
/// Example usage:
///     table.retain(
///         {
///             ['a'] = 1,
///             ['b'] = 2,
///             ['c'] = 3
///         },
///         function (k, v)
///             return v % 2 == 0
///         end
///     )
///     --> { ['b'] = 2 }
static int libE_retain(lua_State* const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    bool raw = luaL_optbool(L, 3, false);

    lua_settop(L, 2);

    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_pushvalue(L, 2);
        lua_insert(L, -3);
        lua_call(L, 2, 1);

        if (lua_toboolean(L, -1) == 0)
        {
            lua_pushnil(L);
            if (raw)
                lua_rawset(L, 1);
            else
                lua_settable(L, 1);
        }

        lua_pop(L, 1);
    }

    lua_settop(L, 1);
    lxs_assert_stack_at(L, 1);
    return 1;
}

/// table.alli(table, selector)
///
/// Iterates over the vector part of a *table* invoking a *selector* function
/// for each index-value pair, copying every pair that matches the selector
/// into a new table and returning it.
/// 
/// The selector has the following signature: *selector(key, value)* and must
/// return a boolean.
///
/// Usage example:
///     table.alli(
///         { 1, 2, 3 },
///         function (k, v)
///             return v % 2 ~= 0
///         end
///     )
///     --> {
///         [1] = 1,
///         [2] = 3
///     }
static int libE_alli(lua_State* const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_settop(L, 2);

    int len = luaL_getn(L, 1);
    int j   = 0;
    lua_createtable(L, max(0, len - 1), 0);

    for (int i = 1; i <= len; ++i)
    {
        lua_pushvalue(L, 2);
        lua_pushinteger(L, i);
        lua_rawgeti(L, 1, i);
        lua_call(L, 2, 1);

        if (lua_toboolean(L, -1) != 0)
        {
            lua_rawgeti(L, 1, i);
            lua_rawseti(L, 3, ++j);
        }

        lua_pop(L, 1);
    }

    lxs_assert_stack_at(L, 3);
    return 1;
}

/// table.all(table, selector)
///
/// Iterates over a *table* invoking a *selector* function for each key-value
/// pair, copying every pair that matches the selector into a new table and
/// returning it.
/// 
/// The selector has the following signature: *selector(key, value)* and must
/// return a boolean.
///
/// Usage example:
///     table.all(
///         {
///             ['a'] = 1,
///             ['b'] = 2,
///             ['c'] = 3
///         },
///         function (k, v)
///             return v % 2 ~= 0
///         end
///     )
///     --> {
///         ['a'] = 1,
///         ['c'] = 3
///     }
static int libE_all(lua_State* const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_settop(L, 2);
    lua_newtable(L);

    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        lua_pushvalue(L, -2);
        lua_pushvalue(L, -2);
        lua_pushvalue(L, 2);
        lua_insert(L, -3);
        lua_call(L, 2, 1);

        if (lua_toboolean(L, -1) != 0)
        {
            lua_pop(L, 1);
            lua_pushvalue(L, -2);
            lua_rawsetr(L, 3);
        }
        else
            lua_pop(L, 2);
    }

    lxs_assert_stack_at(L, 3);
    return 1;
}

#endif // LUAXS_EXTEND_TABLIB

//------------------------------------------------------------------------------


static const luaL_Reg libE_funcs[] = {
    { "concat"   , libE_concat   },
    { "foreach"  , libE_foreach  },
    { "foreachi" , libE_foreachi },
    { "getn"     , libE_getn     },
    { "maxn"     , libE_maxn     },
    { "insert"   , libE_insert   },
    { "remove"   , libE_remove   },
    { "setn"     , libE_setn     },
    { "sort"     , libE_sort     },
#if LUAXS_EXTEND_TABLIB
    { "create",   libE_create   },
    { "countall", libE_countall }, // TODO: think of a better name
    //{ "joinall",  lxs_tablib_joinall   },
// vectors
    { "joini",   libE_joini   },
    { "counti",  libE_counti  },
    { "mini",    libE_mini    },
    { "maxi",    libE_maxi    },
    { "sumi",    libE_sumi    },
    { "findi",   libE_findi   },
    { "anyi",    libE_anyi    },
    { "alli",    libE_alli    },
    { "mapi",    libE_mapi    },
    { "retaini", libE_retaini },
// hashtables -- work on vectors as well, but are slower due to potential cache misses
    { "join",    libE_join   },
    { "count",   libE_count  },
    { "min",     libE_min    },
    { "max",     libE_max    },
    { "sum",     libE_sum    },
    { "find",    libE_find   },
    { "any",     libE_any    },
    { "all",     libE_all    },
    { "map",     libE_map    },
    { "retain",  libE_retain },
#endif // LUAXS_EXTEND_TABLIB
    { NULL, NULL }
};

LUALIB_API int luaopen_table (lua_State *L)
{
    lxs_assert_stack_begin(L);
    
    luaL_register(L, LUA_TABLIBNAME, libE_funcs);

    lxs_assert_stack_end(L, 1);
    return 1;
}

}; // extern "C"