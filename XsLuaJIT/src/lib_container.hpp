#ifndef lxs_container_hpp
#define lxs_container_hpp 1

extern "C" {

#include "luajit.h"

#if LUAXS_ADDLIB_CONTAINER

#include "lauxlib.h"
#include "lualib.h"
#include "lmem.h"

#include <string.h>

}; // extern "C"

#include "leastl.hpp"
#include <eastl/algorithm.h>
#include <eastl/set.h>







//template<class T>
//struct ref_handle
//{
//    int id;
//
//    ref_handle()
//        : id(LUA_NOREF)
//    {
//    }
//
//    void pin(lua_State* const L, int narg)
//    {
//        id = luaL_ref(L, LUA_REGISTRYINDEX);
//    }
//
//    void unpin(lua_State* const L)
//    {
//        luaL_unref(L, LUA_REGISTRYINDEX, id);
//        id = LUA_NOREF;
//    }
//
//    void get(lua_State* const L) const
//    {
//        lua_rawgeti(L, LUA_REGISTRYINDEX, id);
//    }
//};





typedef struct lstring
{
    size_t      len;
    const char *data;

    lstring()
        : len(0)
        , data(NULL)
    {
    }
} lstring;

/// That said, to define a strict weak order on a set of objects O, you need to
/// a binary relation r(x, y) such that the following conditions hold for
/// elements x, y, and z from O:
///
/// irreflexive:
///     r(x, x)
///     == false
///
/// asymmetric:
///     r(x, y)
///     == true implies
///     r(y, x)
///     == false
///
/// transitive:
///     r(x, y)
///     == true and
///     r(y, z)
///     == true implies
///     r(x, z)
///     == true
///
/// incomparability:
///     r(x, y)
///     == false and
///     r(y, x)
///     == false and
///     r(y, z)
///     == false and
///     r(z, y)
///     == false implies
///     r(x, z)
///     == false and
///     r(z, x)
///     == false
///
//template<>
//struct eastl::less<double>
//    : public eastl::binary_function<lstring, lstring, bool>
//{
//    bool operator()(const double& lhs, const double& rhs) const
//    {
//        return lhs < rhs;
//    }
//};
//
//
//template<>
//struct eastl::less<lstring>
//    : public eastl::binary_function<lstring, lstring, bool>
//{
//    bool operator()(const lstring& lhs, const lstring& rhs) const
//    {
//        if (memcmp(lhs.data, rhs.data, eastl::min(lhs.len, rhs.len)) < 0)
//            return true;
//        return false;
//    }
//};




typedef eastl::set<double>                    set_number;
typedef set_number::insert_return_type set_number_result;
typedef set_number::const_iterator     set_number_iterator;

typedef eastl::set<lstring>            set_string;
typedef set_string::insert_return_type set_string_result;
typedef set_string::const_iterator     set_string_iterator;


#define TYPE_SET_NUMBER       "container_set_number"
#define TYPE_SET_NUMBER_RANGE "container_set_number_range"

#define TYPE_SET_STRING       "container_set_string"
#define TYPE_SET_STRING_RANGE "container_set_string_range"



template<class Iterator>
inline static int push_from_iterator(lua_State *const L, Iterator &it)
{
    lxs_error(L, "Not implemented!");
    return 0;
}

template<>
inline static int push_from_iterator<set_number_iterator>(
    lua_State *const L,
    set_number_iterator &it
)
{
    lua_pushnumber(L, *it);
    return 1;
}

template<>
inline static int push_from_iterator<set_string_iterator>(
    lua_State *const L,
    set_string_iterator &it
)
{
    lstring s = *it;
    lua_pushlstring(L, s.data, s.len);
    return 1;
}

template<class Iterator>
struct range
{
    Iterator front;
    Iterator end;

    range(Iterator front, Iterator end)
        : front(front)
        , end(end)
    {
    }

    static int destroy(lua_State *const L)
    {
        range *self = static_cast<range *>(lua_touserdata(L, 1));

        self->~range();

        return 0;
    }

    static int next(lua_State *const L)
    {
        range *self = static_cast<range *>(
                          lua_touserdata(L, lua_upvalueindex(1))
                      );

        if (self->front != self->end)
        {
            int pushes = push_from_iterator<Iterator>(L, self->front);
            ++self->front;
            return pushes;
        }
        else
        {
            return 0;
        }
    }

    inline static int make_gen(lua_State *const L,
                               Iterator &front,
                               Iterator &end,
                               const char *type,
                               size_t len)
    {
        void *self = lua_newuserdata(L, sizeof(range<Iterator>));
        lxs_assert(L, self);

        lxs_rawget(L, LUA_REGISTRYINDEX, type, len);
        lxs_assert(L, lua_istable(L, -1));
        lua_setmetatable(L, -2);

        new (self) range<Iterator>(front, end);

        lua_pushcclosure(L, range<Iterator>::next, 1);
        return 1;
    };

    template<class Container>
    inline static int make_gen(lua_State *const L,
                               Container &cont,
                               const char *type,
                               size_t len)
    {
        return range<Container::const_iterator>::make_gen(
                   L,
                   cont.begin(),
                   cont.end(),
                   type,
                   len
               );
    }
}; // cs range






#define lxs_cmakel(L, type, tname)                                             \
    lxs_cmake<type>(L, "" tname, _countof(tname) - 1)

template<typename Container>
inline static Container *lxs_cmake(lua_State *const L,
                                   const char *type,
                                   size_t len)
{
    lxs_assert(L, L);
    lxs_assert_stack_begin(L);

    Container *cont = static_cast<Container *>(
                          lua_newuserdata(L, sizeof(Container))
                      );
    lxs_assert(L, cont && lua_isuserdata(L, -1));

    lxs_rawget(L, LUA_REGISTRYINDEX, type, len);
    lxs_assert(L, lua_istable(L, -1));
    lua_setmetatable(L, -2);

    new (cont) Container();

    lxs_assert_stack_end(L, 1);
    return cont;
}


template<typename Container>
inline static void lxs_cfree(lua_State *const L, Container *cont)
{
    lxs_assert(L, L);
    lxs_assert_stack_begin(L);

    cont->~Container();

    lxs_assert_stack_end(L, 0);
}


#define lxs_cget_set_number(L, narg)                                    \
    lxs_cget<set_number>(L, (narg))

template<typename Container>
inline static Container *lxs_cget(lua_State *const L, int narg)
{
    lxs_assert(L, L);

    return static_cast<Container *>(lua_touserdata(L, narg));
}


#define lxs_ccheck_set_number(L, narg)                                  \
    lxs_ccheck<set_number>(L, (narg), TYPE_SET_NUMBER)

template<typename Container>
inline static Container *lxs_ccheck(lua_State *const L,
                                    int narg,
                                    const char *type)
{
    lxs_assert(L, L);
    lxs_assert(L, type);

    return static_cast<Container *>(luaL_checkudata(L, narg, type));
}



#endif // LUAXS_ADDLIB_CONTAINER
#endif // lxs_container_hpp
