#include "b64.h"
#include "lauxlib.h"
#include "lua.h"

#define B64_BUFF_SIZE 32768

static int l_b64_encode(lua_State* L) {
    size_t data_len;
    const char* data = luaL_checklstring(L, 1, &data_len);
    base64_encodestate state;
    base64_init_encodestate(&state);
    luaL_Buffer B;
    luaL_buffinit(L, &B);
    size_t offset = 0;

    char tmp_buff[2 * B64_BUFF_SIZE];  // 4/3 is an approximate upper bound on growth

    while (offset < data_len) {
        size_t in_size = data_len - offset;
        if (in_size > B64_BUFF_SIZE) {
            in_size = B64_BUFF_SIZE;
        }
        int count = base64_encode_block(data + offset, (int)in_size, tmp_buff, &state);
        luaL_addlstring(&B, tmp_buff, count);
        offset += in_size;
    }
    int count = base64_encode_blockend(tmp_buff, &state);
    tmp_buff[count] = 0;
    luaL_addlstring(&B, tmp_buff, count);
    luaL_pushresult(&B);
    return 1;
}

static int l_b64_decode(lua_State* L) {
    size_t data_len;
    const char* data = luaL_checklstring(L, 1, &data_len);
    base64_decodestate state;
    base64_init_decodestate(&state);
    luaL_Buffer B;
    luaL_buffinit(L, &B);
    char tmp_buff[B64_BUFF_SIZE];
    size_t offset = 0;
    while (offset < data_len) {
        size_t in_size = data_len - offset;
        if (in_size > B64_BUFF_SIZE) {
            in_size = B64_BUFF_SIZE;
        }
        int count = base64_decode_block(data + offset, (int)in_size, tmp_buff, &state);
        luaL_addlstring(&B, tmp_buff, count);
        offset += in_size;
    }
    luaL_pushresult(&B);
    return 1;
}

LUAMOD_API int luaopen_b64(lua_State* L) {
    luaL_Reg libs[] = {
        {"encode", l_b64_encode},
        {"decode", l_b64_decode},
        {NULL, NULL},
    };
    luaL_newlib(L, libs);
    return 1;
}
