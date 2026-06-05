#include "lauxlib.h"
#include "lua.h"

extern int luaopen__credo(lua_State *L);
extern int luaopen__shell(lua_State *L);
extern int luaopen_cfort(lua_State *L);
extern int luaopen__libs(lua_State *L);
extern int luaopen__loader(lua_State *L);
extern int luaopen_lfs(lua_State *L);
extern int luaopen__crs(lua_State *L);
extern int luaopen__time(lua_State *L);
extern int luaopen_lpeg(lua_State *L);
extern int luaopen_miniz(lua_State *L);
extern int luaopen_b64(lua_State *L);
extern int luaopen_term_core(lua_State *L);
extern int luaopen_cjson(lua_State *L);
extern int luaopen_cjson_safe(lua_State *L);
extern int luaopen_environ_core(lua_State *L);

static const luaL_Reg custom_libs[] = {
    {"cjson", luaopen_cjson},
    {"cjson.safe", luaopen_cjson_safe},
    {"environ.core", luaopen_environ_core},
    {"_time", luaopen__time},
    {"lpeg", luaopen_lpeg},
    {"miniz", luaopen_miniz},
    {"_b64", luaopen_b64},
    {"term.core", luaopen_term_core},
    {"_credo", luaopen__credo},
    {"_crs", luaopen__crs},
    {"_shell", luaopen__shell},
    {"cfort", luaopen_cfort},
    {"lfs", luaopen_lfs},
    {"_libs", luaopen__libs},
    {"_loader", luaopen__loader},  // due to startup.lua this should be at the end
    {NULL, NULL},
};

// preload libraries but do not add to global
void crluaL_openlibs(lua_State *L) {
    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = custom_libs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 0);
        lua_pop(L, 1); /* remove lib */
    }
}
