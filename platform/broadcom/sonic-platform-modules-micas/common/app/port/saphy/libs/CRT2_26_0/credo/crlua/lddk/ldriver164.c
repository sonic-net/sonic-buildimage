#include "api_driver164.h"

#include "credo.h"
#include "sdk.h"
#include "crintl/shell.h"

#include "lauxlib.h"
#include "lua.h"

#include <string.h>
#include <ctype.h>

extern void cr_log(const char* scope, CredoLogLevel_t level, const char* format, ...);


static char* trimwhitespace( const char *str)
{
    size_t len = strlen(str);
    if(len == 0)
        return 0;

    const char *end;
    size_t out_size;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
    {
        return NULL;
    }

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end++;

    // Set output size to minimum of trimmed string length and buffer size minus 1
    out_size = (end - str) < len-1 ? (end - str) : len-1;
    char* out = malloc(sizeof(char) * (out_size+ 1));
    if (out == NULL) {
        return NULL;
    }
    // Copy trimmed string and add null terminator
    memcpy(out, str, out_size);
    out[out_size] = 0;

    return out;
}

static int driver164_logger(int loglevel, const char* message) {
    CredoLogLevel_t credo_loglevel = CR_LOG_INFO;
    switch(loglevel) {
        case 0:
            credo_loglevel = CR_LOG_ERROR;
            break;
        case 1:
            credo_loglevel = CR_LOG_INFO;
            break;
        case 2:
            credo_loglevel = CR_LOG_WARN;
            break;
        case 3:
            credo_loglevel = CR_LOG_DEBUG;
            break;
        default:
            credo_loglevel = CR_LOG_INFO;
            break;
    }
    char* new_message = trimwhitespace(message);

    if (new_message != NULL) {
        cr_log("DDK", credo_loglevel, "%s", new_message);
    }
    return 0;
}

static const Driver164_Init_t g_credo_driver164 = {
    .Device_Read = (Driver164_Device_Read)cr_tcm_read,
    .Device_Write = (Driver164_Device_Write)cr_tcm_write,
    .Device_Logger =  (Driver164_Device_Logger)driver164_logger
};

static int lDriver164_Init(lua_State *L) {
    int err = Driver164_Init(&g_credo_driver164);
    if (err != 0) {
        luaL_error(L, "driver164: failed Driver164_Init");
    }
    return 0;
}

static int lDriver164_Exit(lua_State *L) {
    Driver164_Exit();
    return 0;
}

static int lDriver164_DataPath_Add(lua_State *L) {
    unsigned index = luaL_checkinteger(L, 1);
    unsigned slice_id = luaL_checkinteger(L,2);
    CredoSlice_t *slice = cr_shell_get_slice(slice_id);
    if (slice == NULL) {
        luaL_error(L, "driver164: invalid slice id %d", slice_id);
    }
    CredoMACsecDirection_t direction = luaL_checkinteger(L, 3);

    CredoMACsecDataPath_t credo_path = {0};
    CredoError_t err = cr_eip_get_macsec_datapath(slice, direction, &credo_path);
    if (err != CR_OK) {
        luaL_error(L, "driver164: cr_eip_get_macsec_datapath failed: %d", err);
    }
    char flags1 = (char)luaL_optinteger(L, 4, 0);
    char flags2 = (char)luaL_optinteger(L, 5, 0);

    Driver164_DataPath_t path = {0};
    path.PlatformContext_p = (void*)slice;
    path.StartByteOffset1 = credo_path.start_offset1;
    path.LastByteOffset1 = credo_path.end_offset1;
    path.StartByteOffset2 = credo_path.start_offset2;
    path.LastByteOffset2 = credo_path.end_offset2;
    path.Flags1 = flags1;
    path.Flags2 = flags2;

    int ddk_err = Driver164_DataPath_Add(index, &path);
    if (ddk_err != 0) {
        luaL_error(L, "driver164: Driver164_DataPath_Add failed: %d", ddk_err);
    }
    return 0;
}

static int lDriver164_DataPath_Remove(lua_State *L) {
    unsigned index = luaL_checkinteger(L, 1);
    int err = Driver164_DataPath_Remove(index);
    if (err != 0) {
        luaL_error(L, "driver164: failed Driver164_DataPath_Remove %d", err);
    }
    return 0;
}

static int lDriver164_DataPath_GetCount(lua_State *L) {
    unsigned count = Driver164_DataPath_GetCount();
    lua_pushinteger(L, count);
    return 1;
}

static int lDriver164_DataPath_GetProperties(lua_State*L) {
    unsigned index = luaL_checkinteger(L, 1);

    Driver164_DataPath_t path = {0};

    int err = Driver164_DataPath_GetProperties(index, &path);
    if (err != 0) {
        luaL_error(L, "driver164: failed Driver164_DataPath_GetProperties %d", err);
    }

    lua_newtable(L);
    lua_pushinteger(L, ((CredoSlice_t*)path.PlatformContext_p)->slice_id);
    lua_setfield(L, -2, "slice_id");
    lua_pushinteger(L, path.StartByteOffset1);
    lua_setfield(L, -2, "StartByteOffset1");
    lua_pushinteger(L, path.LastByteOffset1);
    lua_setfield(L, -2, "LastByteOffset1");
    lua_pushinteger(L, path.StartByteOffset2);
    lua_setfield(L, -2, "StartByteOffset2");
    lua_pushinteger(L, path.LastByteOffset1);
    lua_setfield(L, -2, "LastByteOffset2");
    lua_pushinteger(L, path.Flags1);
    lua_setfield(L, -2, "Flags1");
    lua_pushinteger(L, path.Flags2);
    lua_setfield(L, -2, "Flags2");

    return 1;
}

static const struct luaL_Reg driver164_functions[] = {
    {"Init", lDriver164_Init},
    {"Exit", lDriver164_Exit},
    {"DataPath_Add", lDriver164_DataPath_Add},
    {"DataPath_Remove", lDriver164_DataPath_Remove},
    {"DataPath_GetCount", lDriver164_DataPath_GetCount},
    {"DataPath_GetProperties", lDriver164_DataPath_GetProperties},
    {NULL, NULL},
};

LUALIB_API int luaopen_ddk__driver164(lua_State *L) {
    luaL_newlib(L, driver164_functions);
    return 1;
}
