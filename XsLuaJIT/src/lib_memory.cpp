#define lib_memory_cpp
#define LUA_LIB

extern "C"
{
#include "lua.h"
};


#if LUAXS_ADDLIB_MEMORY

extern "C"
{
#include "lualib.h"
#include "lauxlib.h"
};


static bool is_ref_type(lua_State* const L, int narg)
{
    switch (lua_type(L, narg))
    {
    case LUA_TTABLE:
    case LUA_TUSERDATA
    case LUA_TLIGHTUSERDATA:
    case LUA_TFUNCTION:
    case LUA_TTHREAD:
        return true;
    case LUA_TNONE:
    case LUA_TNIL:
    case LUA_TBOOLEAN:
    case LUA_TSTRING:
    case LUA_TNUMBER:
    default:
        return false;
    }
}

extern "C"
{

static int libL_open_addr(lua_State* const L)
{
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);

    void* addr = lua_touserdata(L, 1);
    luaL_argcheck(L, addr, 1, "must be greater zero");



    return 0;
}

static int libL_open_obj(lua_State* const L)
{
    luaL_checkany(L, 1);
    luaL_argcheck(L, is_ref_type(L, 1), 1, "must be a reference type");
    return 0;
}



static const luaL_Reg libO_funcs[] = {
    // other
    { "seek",     libO_seek      },
    { "addr",     libO_addr      },
    { "len",      libO_len       },
    { "equals",   libO_equals    },
    { "tostring", libO_tostring  },
    // reading
    { "r_s8",     libO_r_sint8   },
    { "r_u8",     libO_r_uint8   },
    { "r_s16",    libO_r_sint16  },
    { "r_u16",    libO_r_uint16  },
    { "r_s32",    libO_r_sint32  },
    { "r_u32",    libO_r_uint32  },
    { "r_s64",    libO_r_sint64  },
    { "r_u64",    libO_r_uint64  },
    { "r_f32",    libO_r_float32 },
    { "r_f64",    libO_r_float64 },
    { "r_sz",     libO_r_stringz },
    { "r_sl",     libO_r_lstring },
    { "r_ptr",    libO_r_pointer },
    // writing
    { "w_s8",     libO_w_sint8   },
    { "w_u8",     libO_w_uint8   },
    { "w_s16",    libO_w_sint16  },
    { "w_u16",    libO_w_uint16  },
    { "w_s32",    libO_w_sint32  },
    { "w_u32",    libO_w_uint32  },
    { "w_s64",    libO_w_sint64  },
    { "w_u64",    libO_w_uint64  },
    { "w_f32",    libO_w_float32 },
    { "w_f64",    libO_w_float64 },
    { "w_sz",     libO_w_stringz },
    { "w_sl",     libO_w_lstring },
    { "w_ptr",    libO_w_pointer },
    { NULL, NULL }
};

static const luaL_Reg libM_funcs[] = {
    { "__gc",       libM_gc       },
    { "__eq",       libM_eq       },
    { "__len",      libM_len      },
    { "__tostring", libM_tostring },
    { NULL, NULL }
};

static const luaL_Reg libL_funcs[] = {
    { "open_addr", libL_open_addr },
    { "open_obj",  libL_open_obj  },
    { NULL, NULL }
};


int luaopen_memory(lua_State* L)
{
    lxs_assert_stack_begin(L);

    luaI_openlib(L, LUA_MEMORYLIBNAME, libL_funcs, 0);

    lxs_assert_stack_end(L, 1);
    return 1;
}

}; // extern "C"

#endif // LUAXS_ADDLIB_MEMORY
