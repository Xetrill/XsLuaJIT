#define lib_game_cpp 1
#define LUA_LIB

#include "lib_game.hpp"

#if LUAXS_ADDLIB_GAME

#include <string.h>
#include <stdio.h>
#include <direct.h>



static path_map paths;



static bool need_init()
{
    return paths.empty();
}

static bool parse_line(lxs_string& line)
{
    if (lxs_sstarts_withc(&line, ';'))
        return true;
    return true;
}

static void init_game(lua_State* const L)
{
    lxs_string* root = lxs_screate(L, FILENAME_MAX + 1, true);

    if (!_getdcwd(_getdrive(), &root->data[0], FILENAME_MAX))
        lxs_error(L, "failed to retrieve current working directory");

    root->len = strlen(root->data);
    lxs_sappendc(L, root, '\\');

    paths.insert(eastl::make_pair("$fs_root$", root));

    lxs_sflines(L, ".\\fsgame.ltx", parse_line, 4096);
}


extern "C" {

static int libL_combine_path(lua_State* const L)
{
    lxs_assert_stack_begin(L);

    if (need_init())
        init_game(L);

    lxs_assert_stack_end(L, 1);
    return 1;
}

static const luaL_Reg libL_funcs[] = {
    { "combine_path", libL_combine_path },
    { NULL, NULL }
};

int luaopen_game(lua_State* L)
{
    lxs_assert_stack_begin(L);
    luaI_openlib(L, LUA_GAMELIBNAME, libL_funcs, 0);
    lxs_assert_stack_end(L, 1);
    return 1;
}

}; // extern "C"

#endif // LUAXS_ADDLIB_GAME
