#ifndef lib_game_hpp
#define lib_game_hpp 1

extern "C" {
#include "lua.h"
};

#if LUAXS_ADDLIB_GAME

extern "C" {
#include "lualib.h"
#include "lauxlib.h"
};

#include "leastl.hpp"
#include "eastl/vector.h"
#include "eastl/hash_map.h"

#include "lxs_string.hpp"


typedef eastl::hash_map<const char*, lxs_string*> path_map;



#endif // LUAXS_ADDLIB_GAME
#endif // lib_game_hpp
