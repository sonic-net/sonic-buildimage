#include "lauxlib.h"
#include "lua.h"
#include "shell.h"

#include "sdk.h"

static inline bool check_in_shell(lua_State *L) {
    lua_pushliteral(L, "credo_in_shell");
    lua_gettable(L, LUA_REGISTRYINDEX);
    bool in_shell = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return in_shell;
}

static int lshell_in_shell(lua_State *L) {
    lua_pushliteral(L, "credo_in_shell");
    lua_gettable(L, LUA_REGISTRYINDEX);
    return 1;
}

static int lshell_in_server(lua_State *L) {
    bool in_shell = check_in_shell(L);
    lua_pushboolean(L, in_shell && crsh.in_server);
    return 1;
}

static int lshell_get_slice_list(lua_State *L) {
    // shallow copy the slice list
    lua_pushliteral(L, CR_REGISTRY_CMD_SLICES);
    lua_gettable(L, LUA_REGISTRYINDEX);
    lua_newtable(L);
    size_t count = lua_rawlen(L, -2);
    for (size_t i = 0; i < count; i++) {
        lua_pushinteger(L, (lua_Integer)i + 1);
        lua_gettable(L, -3);
        lua_pushinteger(L, (lua_Integer)i + 1);
        lua_pushvalue(L, -2);
        lua_settable(L, -4);
        lua_pop(L, 1);
    }
    return 1;
}

// register the special interactive functions
static int lshell_register_interactive(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    lua_pushstring(L, CR_REGISTRY_COMPLETER);
    lua_pushvalue(L, 1);
    lua_settable(L, LUA_REGISTRYINDEX);

    lua_pushstring(L, CR_REGISTRY_MUI);
    lua_pushvalue(L, 2);
    lua_settable(L, LUA_REGISTRYINDEX);

    lua_pushliteral(L, CR_REGISTRY_SHELL_PROMPT);
    lua_pushvalue(L, 3);
    lua_settable(L, LUA_REGISTRYINDEX);

    return 0;
}

static int lshell_set_slash_mode(lua_State *L) {
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    lua_pushstring(L, CR_REGISTRY_SLASH_MODE);
    lua_pushvalue(L, 1);
    lua_settable(L, LUA_REGISTRYINDEX);
    return 0;
}

static int lshell_get_slash_mode(lua_State *L) {
    lua_pushstring(L, CR_REGISTRY_SLASH_MODE);
    lua_gettable(L, LUA_REGISTRYINDEX);
    bool is_slash_mode = lua_toboolean(L, -1);
    lua_pop(L, 1);
    lua_pushboolean(L, is_slash_mode);
    return 1;
}

// user passes in a True/False list by index for the slices that are selected
static int lshell_set_selected_slices(lua_State *L) {
    if (!check_in_shell(L)) {
        luaL_error(L, "Cannot set selected slices when in command mode");
        return 0;
    }
    luaL_checktype(L, 1, LUA_TTABLE);
    for (size_t i = 0; i < crsh.slice_count; i++) {
        lua_pushinteger(L, (lua_Integer)i + 1);
        lua_gettable(L, -2);
        crsh.slices_selected[i] = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    return 0;
}

static int lshell_get_selected_slices(lua_State *L) {
    if (!check_in_shell(L)) {
        lua_pushcfunction(L, lshell_get_slice_list);
        lua_call(L, 0, 1);
        return 1;
    }
    lua_newtable(L);
    size_t pos = 0;
    for (size_t i = 0; i < crsh.slice_count; i++) {
        if (!crsh.slices_selected[i]) {
            continue;
        }
        lua_pushinteger(L, (lua_Integer)pos + 1);
        lua_pushinteger(L, crsh.slices[i]->slice_id);
        lua_settable(L, -3);
        pos += 1;
    }
    return 1;
}

static int lshell_write_error(lua_State *L) {
    const char *error = luaL_optstring(L, 1, "");
    lua_writestringerror(L, "%s", error);
    return 0;
}

static int lshell_write(lua_State *L) {
    size_t slen = 0;
    const char *s = luaL_optlstring(L, 1, "", &slen);
    lua_writestring(L, s, (int)slen);
    return 0;
}

static int lshell_add_completion(lua_State *L) {
    luaL_argcheck(L, lua_islightuserdata(L, 1), 1, "Invalid line completion state");
    void *lc = lua_touserdata(L, 1);
    const char *str = luaL_checkstring(L, 2);
    size_t pos = luaL_checkinteger(L, 3);
    luaL_argcheck(L, lua_tointeger(L, 3), 3, "Invalid line completion position");

    cr_lua_completer_cb_t completion = crsh.lua_add_completion;
    if (completion != NULL) {
        completion(str, lc, pos);
    }
    return 0;
}

#define LUA_MAXINPUT 1024
static int lshell_input(lua_State *L) {
    bool in_shell = check_in_shell(L);
    if (!in_shell) {
        luaL_error(L, "cannot get input in command mode");
    }
    const char *prompt = luaL_optstring(L, 1, "");
    char buffer[1024];
    CredoReadLine_t func = crsh.readline;
    if (func != NULL) {
        if (func(prompt, buffer) != CR_OK) {
            luaL_error(L, "unable to read input");
        }
    } else {
        fputs(prompt, stdout);
        fflush(stdout);
        if (fgets(buffer, LUA_MAXINPUT, stdin) == NULL) {
            luaL_error(L, "unable to read input");
        }
    }
    lua_pushstring(L, buffer);
    return 1;
}

static const struct luaL_Reg shell_functions[] = {
    {"get_slice_list", lshell_get_slice_list},
    {"register_interactive", lshell_register_interactive},
    {"set_slash_mode", lshell_set_slash_mode},
    {"get_slash_mode", lshell_get_slash_mode},
    {"set_selected_slices", lshell_set_selected_slices},
    {"get_selected_slices", lshell_get_selected_slices},
    {"add_completion", lshell_add_completion},
    {"input", lshell_input},
    {"in_shell", lshell_in_shell},
    {"in_server", lshell_in_server},
    {"write", lshell_write},
    {"write_error", lshell_write_error},
    {NULL, NULL},
};

int luaopen__shell(lua_State *L) {
    luaL_newlib(L, shell_functions);
    return 1;
}
