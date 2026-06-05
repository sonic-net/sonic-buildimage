
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

static int lua_isatty(lua_State *L) {
    FILE **fp = (FILE **)luaL_checkudata(L, 1, LUA_FILEHANDLE);

    lua_pushboolean(L, isatty(fileno(*fp)));
    return 1;
}

int luaopen_term_core(lua_State *L) {
    lua_newtable(L);
    lua_pushcfunction(L, lua_isatty);
    lua_setfield(L, -2, "isatty");

    return 1;
}
