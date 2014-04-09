#define lib_container_cpp
#define LUA_LIB

#include "lib_container.hpp"

#if LUAXS_ADDLIB_CONTAINER

#include <eastl/functional.h>
#include <eastl/vector.h>


//==============================================================================

extern "C" {

#define match_type(s, slen, type)                                              \
    ((slen) == _countof("" type) - 1 &&                                        \
     memcmp((s), "" type, (slen)) == 0)

static int libL_new_set(lua_State* const L)
{
    size_t len;
    const char* type = luaL_checklstring(L, 1, &len);

    if (match_type(type, len, "number"))
        lxs_cmakel(L, set_number, TYPE_SET_NUMBER);
    else if (match_type(type, len, "string"))
        lxs_cmakel(L, set_string, TYPE_SET_STRING);
    else
        luaL_argerror(L, 1,
            "unknown or unsupported type; "
            "only 'number' and 'string' are supported"
        );

    return 1;
}

static int libL_is_container(lua_State* const L)
{
    luaL_checkany(L, 1);

    lua_settop(L, 1);

    if (!lua_isuserdata(L, 1))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    lua_getmetatable(L, 1);
    if (!lua_istable(L, -1))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    lua_pushnil(L);
    while (lua_next(L, LUA_REGISTRYINDEX))
    {
        lua_pushvalue(L, -2);

        if (lua_isstring(L, -1))
        {
            size_t      len;
            const char* str = lua_tolstring(L, -1, &len);

            if (len >= 11 &&
                memcmp(str, "container_", 10) == 0 &&
                lua_istable(L, -2) &&
                lua_rawequal(L, -4, -2))
            {
                lua_pushboolean(L, 1);
                return 1;
            }
        }

        lua_pop(L, 2);
    }

    lua_pushboolean(L, 0);
    return 1;
}




static int libE_set_number_size(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self = lxs_ccheck_set_number(L, 1);
    lua_pushinteger(L, static_cast<int>(self->size()));

    lxs_assert_stack_end(L, 1);
    return 1;
}

static int libE_set_number_insert(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    lua_settop(L, 1);
    set_number* self = lxs_ccheck_set_number(L, 1);

    set_number_result result = self->insert(luaL_checknumber(L, 2));
    lua_pushboolean(L, result.second);

    lxs_assert_stack_at(L, 2);
    return 2;
}

static int libE_set_number_contains(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self = lxs_ccheck_set_number(L, 1);

    lua_pushboolean(L, self->find(luaL_checknumber(L, 2)) != self->end());

    lxs_assert_stack_end(L, 1);
    return 1;
}

static int libE_set_number_remove(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self = lxs_ccheck_set_number(L, 1);
    lua_pushboolean(L, self->erase(luaL_checknumber(L, 2) != 0));

    lua_settop(L, 2);
    lxs_assert_stack_at(L, 2);
    return 2;
}

static int libE_set_number_clear(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self = lxs_ccheck_set_number(L, 1);
    self->clear();

    lua_settop(L, 1);
    lxs_assert_stack_at(L, 1);
    return 1;
}

static int libE_set_number_equals(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    luaL_checktype(L, 1, LUA_TUSERDATA);
    if (lua_type(L, 2) != LUA_TUSERDATA)
    {
        lua_pushboolean(L, 0);
        lxs_assert_stack_end(L, 1);
        return 1;
    }

    set_number* self  = lxs_ccheck_set_number(L, 1);
    set_number* other = lxs_ccheck_set_number(L, 2);

    //if (self == other)
    //{
    //    lua_pushboolean(L, 1);
    //    lxs_assert_stack_end(L, 1);
    //    return 1;
    //}

    if (self->size() != other->size())
    {
        lua_pushboolean(L, 0);
        lxs_assert_stack_end(L, 1);
        return 1;
    }

    lua_pushboolean(L, eastl::equal(
        self->begin(),
        self->end(),
        other->begin(),
        eastl::equal_to<double>()
    ));
    lxs_assert_stack_end(L, 1);
    return 1;
}

static int libE_set_number_swap(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self = lxs_ccheck_set_number(L, 1);
    set_number* other = lxs_ccheck_set_number(L, 2);
    self->swap(*other);

    lua_settop(L, 1);
    lxs_assert_stack_at(L, 1);
    return 1;
}

/// for var {, var} in explist do block end
/// 
/// explist is evaluated once before the loop is entered.
/// Its results are an 'iterator function' (which sets the var values)
/// a 'state' (from which the values can be read), and
/// an 'initial value' (from which to iterate onwards). 
///
/// ipairs in Lua:
/// function ipairs(t)
///     local function ipairs_it(t, i)
///         i = i + 1
///         local v = t[i]
///         if v ~= nil then
///             return i, v
///         else
///             return nil
///         end
///     end
///     return ipairs_it, t, 0
/// end
static int libE_set_number_iter(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    range<set_number_iterator>::make_gen(L,
        *lxs_ccheck_set_number(L, 1),
        TYPE_SET_NUMBER_RANGE,
        _countof(TYPE_SET_NUMBER_RANGE) - 1
    );
    lua_insert(L, 1);
    lua_pushnil(L);

    lxs_assert_stack_end(L, 2);
    return 3;
}



static int libE_set_number_diff(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self  = lxs_ccheck_set_number(L, 1);
    set_number* other = lxs_ccheck_set_number(L, 2);

    if (self->size() > 0 && other->size() > 0)
    {
        //for (set_number_iterator it = other->begin(); it != other->end(); ++it)
        //{
        //    self->erase(*it);
        //}
        self->erase(other->begin(), other->end());
    }

    lua_settop(L, 1);
    lxs_assert_stack_at(L, 1);
    return 1;
}

static int libE_set_number_intersect(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self  = lxs_ccheck_set_number(L, 1);
    set_number* other = lxs_ccheck_set_number(L, 2);

    if (self->size() > 0 && other->size() > 0)
    {
        self->erase(
            eastl::find_first_not_of(
                self->begin(), self->end(),
                other->begin(), other->end()
            ),
            self->end()
        );
        //for (set_number_iterator it = other->begin(); it != other->end(); ++it)
        //{
        //    if (self->find(*it) == self->end())
        //    {
        //        self->erase(it);
        //    }
        //}
    }

    lua_settop(L, 1);
    lxs_assert_stack_at(L, 1);
    return 1;
}

static int libE_set_number_is_subset_of(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self  = lxs_ccheck_set_number(L, 1);
    set_number* other = lxs_ccheck_set_number(L, 2);

    if (self->size() == 0 || other->size() == 0)
    {
        lua_pushboolean(L, 0);
    }
    else
    {
        for (set_number_iterator it = self->begin(); it != self->end(); ++it)
        {
            if (other->find(*it) == other->end())
            {
                lua_pushboolean(L, 0);
                lxs_assert_stack_end(L, 1);
                return 1;
            }
        }
    }

    lua_pushboolean(L, 1);
    lxs_assert_stack_end(L, 1);
    return 1;
}

static int libE_set_number_is_superset_of(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self  = lxs_ccheck_set_number(L, 1);
    set_number* other = lxs_ccheck_set_number(L, 2);

    if (self->size() == 0 || other->size() == 0)
    {
        lua_pushboolean(L, 0);
    }
    else
    {
        for (set_number_iterator it = other->begin(); it != other->end(); ++it)
        {
            if (self->find(*it) == self->end())
            {
                lua_pushboolean(L, 0);
                lxs_assert_stack_end(L, 1);
                return 1;
            }
        }
    }

    lua_pushboolean(L, 1);
    lxs_assert_stack_end(L, 1);
    return 1;
}

static int libE_set_number_overlaps(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self  = lxs_ccheck_set_number(L, 1);
    set_number* other = lxs_ccheck_set_number(L, 2);

    if (self->size() == 0 || other->size() == 0)
    {
        lua_pushboolean(L, 0);
    }
    else
    {
        for (set_number_iterator it = other->begin(); it != other->end(); ++it)
        {
            if (self->find(*it) != self->end())
            {
                lua_pushboolean(L, 1);
                lxs_assert_stack_end(L, 1);
                return 1;
            }
        }

        lua_pushboolean(L, 0);
        lxs_assert_stack_end(L, 1);
        return 1;

        //lua_pushboolean(L,
        //    eastl::find_first_of(
        //        self->begin(), self->end(),
        //        other->begin(), other->end()
        //    ) != self->end()
        //);
    }

    lxs_assert_stack_end(L, 1);
    return 1;
}

/// The symmetric difference of two sets is formed by the elements that are
/// present in one of the sets, but not in the other.
static int libE_set_number_sym_diff(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self  = lxs_ccheck_set_number(L, 1);
    set_number* other = lxs_ccheck_set_number(L, 2);

    eastl::vector<double> out;
    eastl::set_symmetric_difference(
        self->begin(), self->end(),
        other->begin(), other->end(),
        eastl::back_inserter(out)
    );
    self->clear();
    self->insert(out.begin(), out.end());

    lua_settop(L, 1);
    lxs_assert_stack_at(L, 1);
    return 1;
}

static int libE_set_number_union(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    set_number* self  = lxs_ccheck_set_number(L, 1);
    set_number* other = lxs_ccheck_set_number(L, 2);

    if (other->size () > 0)
        self->insert(other->begin(), other->end());

    lua_settop(L, 1);
    lxs_assert_stack_at(L, 1);
    return 1;
}





static int libM_set_number_gc(lua_State* const L)
{
    lxs_assert_stack_begin(L);
    lxs_cfree(L, lxs_cget_set_number(L, 1));
    lxs_assert_stack_end(L, 0);
    return 0;
}

static int libM_set_number_len(lua_State* const L)
{
    return libE_set_number_size(L);
}

static int libM_set_number_eq(lua_State* const L)
{
    return libE_set_number_equals(L);
}

static int libM_set_number_call(lua_State* const L)
{
    return libL_new_set(L);
}

#undef match_type


//==============================================================================

static const luaL_Reg libL_funcs[] = {
    { "new_set",      libL_new_set      },
    { "is_container", libL_is_container },
    { NULL, NULL }
};


//------------------------------------------------------------------------------

static const luaL_Reg libE_set_number[] = {
    { "size",     libE_set_number_size     },
    { "insert",   libE_set_number_insert   },
    { "contains", libE_set_number_contains },
    { "remove",   libE_set_number_remove   },
    { "clear",    libE_set_number_clear    },
    { "equals",   libE_set_number_equals   },
    { "swap",     libE_set_number_swap     },
    { "iter",     libE_set_number_iter     },
    { "diff",           libE_set_number_diff           },
    { "intersect",      libE_set_number_intersect      },
    { "is_subset_of",   libE_set_number_is_subset_of   },
    { "is_superset_of", libE_set_number_is_superset_of },
    { "overlaps",       libE_set_number_overlaps       },
    { "sym_diff",       libE_set_number_sym_diff       },
    { "union",          libE_set_number_union          },
    { NULL, NULL }
};

static const luaL_Reg libM_set_number[] = {
    { "__gc",       libM_set_number_gc     },
    { "__len",      libM_set_number_len    },
    { "__eq",       libM_set_number_eq     },
    { NULL, NULL }
};

void register_container_set_number(lua_State* const L) 
{
    lua_createtable(L, 0, _countof(libE_set_number) - 1);
    luaI_openlib(L, NULL, libE_set_number, 0);
    ///-----------------------------------------------------

    /// create and store userdata instance metatable -------
    lua_createtable(L, 0, _countof(libM_set_number) - 1);
    luaI_openlib(L, NULL, libM_set_number, 0);

    lua_pushvalue(L, -2);
    lxs_rawsetl(L, -2, "__index");

    lxs_rawsetl(L, LUA_REGISTRYINDEX, TYPE_SET_NUMBER);
    lua_pop(L, 1);
    ///-----------------------------------------------------

    /// create and store range metatable -------------------
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, range<set_number_iterator>::destroy);
    lxs_rawsetl(L, -2, "__gc");
    lxs_rawsetl(L, LUA_REGISTRYINDEX, TYPE_SET_NUMBER_RANGE);
}



//------------------------------------------------------------------------------

LUA_API int luaopen_container(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    /// register container library (_G.container) --------------
    luaI_openlib(L, LUA_CONTAINERLIBNAME, libL_funcs, 0);
    ///---------------------------------------------------------

    /// register all available containers ----------------------
    register_container_set_number(L);
    ///---------------------------------------------------------

    lxs_assert_stack_end(L, 1);
    return 1;
}



//==============================================================================

#endif // LUAXS_ADDLIB_CONTAINER

}; // extern "C"
