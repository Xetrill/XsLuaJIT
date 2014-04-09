#ifndef XSLUALIB_XS_H
#define XSLUALIB_XS_H

extern "C" {
#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
};

#include "lxsext.h"

#include "lua_compat.h"
#include "xs_cb.hpp"
#include "xs_log.h"


// ============================================================================

#define XS_LIBNAME_XS  "Xs"
#define XS_TYPENAME_CB "CircularBuffer"


// ============================================================================

inline static void cb_typeError(lua_State* L, int narg)
{
    luaL_typeerror(L, narg, "userdata(" XS_TYPENAME_CB ")");
}

inline static void cb_nargError(lua_State* L, int narg)
{
    luaL_error(L, "%d arguments expected, got %d", narg, lua_gettop(L));
}

inline static CircularBuffer* cb_check(lua_State *L, int narg)
{
    CircularBuffer* cb = static_cast<CircularBuffer*>(lua_touserdata(L, narg));
    if (cb == NULL)
        cb_typeError(L, narg);
    return cb;
}


// ============================================================================

extern "C"
{

__declspec(dllexport) int luaopen_Xs(lua_State*);

}; // extern "C"

#endif // XSLUALIB_XS_H