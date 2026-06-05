#include "lauxlib.h"
#include "lua.h"

#include <stdbool.h>
#include <stdint.h>

#if !defined(_WIN32) && !defined(__APPLE__)
// we will use weak attribute to allow it to fail
#define HWL_API __attribute__((weak))

HWL_API bool HWL_RunCommand(const char *pszCommandLine);
// Get number of outputs of the run command
HWL_API uint16_t HWL_GetRunCommandOutputCount(void);
// Get number of items in each output of the run command
HWL_API uint16_t HWL_GetRunCommandOutputSubCount(uint16_t nOutputIndex);
// Get an output item, returns NULL if either index is invalid
HWL_API const char *HWL_GetRunCommandOutputItem(uint16_t nOutputIndex, uint16_t nItemIndex);

static int lcrs_run_command(lua_State *L) {
    const char *command = luaL_checkstring(L, 1);
    if (HWL_RunCommand == NULL) {
        luaL_error(L, "crs: library not found");
        return 0;
    }
    bool success = HWL_RunCommand(command);
    if (!success) {
        return luaL_error(L, "crs failed");
    }

    return 0;
}

static int lcrs_get_output_count(lua_State *L) {
    if (HWL_GetRunCommandOutputCount == NULL) {
        luaL_error(L, "crs: library not found");
        return 0;
    }
    lua_pushinteger(L, HWL_GetRunCommandOutputCount());
    return 1;
}

static int lcrs_get_output_subcount(lua_State *L) {
    if (HWL_GetRunCommandOutputSubCount == NULL) {
        luaL_error(L, "crs: library not found");
        return 0;
    }
    uint16_t index = luaL_checkinteger(L, 1);
    lua_pushinteger(L, HWL_GetRunCommandOutputSubCount(index));
    return 1;
}

static int lcrs_get_output_item(lua_State *L) {
    if (HWL_GetRunCommandOutputItem == NULL) {
        luaL_error(L, "crs: library not found");
        return 0;
    }
    uint16_t output_index = luaL_checkinteger(L, 1);
    uint16_t item_index = luaL_checkinteger(L, 1);
    lua_pushstring(L, HWL_GetRunCommandOutputItem(output_index, item_index));
    return 1;
}

static const struct luaL_Reg crs_functions[] = {
    {"run_command", lcrs_run_command},
    {"get_output_count", lcrs_get_output_count},
    {"get_output_subcount", lcrs_get_output_subcount},
    {"get_output_item", lcrs_get_output_item},
    {NULL, NULL},
};

int luaopen__crs(lua_State *L) {
    luaL_newlib(L, crs_functions);
    return 1;
}
#else
int luaopen__crs(lua_State *L) {
    return 0;
}
#endif
