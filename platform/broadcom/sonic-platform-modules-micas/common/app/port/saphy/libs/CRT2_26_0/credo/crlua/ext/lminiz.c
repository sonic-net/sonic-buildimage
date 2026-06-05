#include "lauxlib.h"
#include "lua.h"
#include "miniz.h"

static int l_adler32(lua_State *L) {
    size_t len;
    const char *s = luaL_optlstring(L, 1, NULL, &len);
    mz_ulong init;
    if (!lua_isnoneornil(L, 2))
        init = (mz_ulong)luaL_checkinteger(L, 2);
    else
        init = mz_adler32(0, NULL, 0);
    if (s == NULL) {
        lua_pushinteger(L, init);
        return 1;
    }
    lua_pushinteger(L, (lua_Integer)mz_adler32(init, (const unsigned char *)s, len));
    return 1;
}

static int l_crc32(lua_State *L) {
    size_t len;
    const char *s = luaL_optlstring(L, 1, NULL, &len);
    mz_ulong init;
    if (!lua_isnoneornil(L, 2))
        init = (mz_ulong)luaL_checkinteger(L, 2);
    else
        init = mz_crc32(0, NULL, 0);
    if (s == NULL) {
        lua_pushinteger(L, init);
        return 1;
    }
    lua_pushinteger(L, (lua_Integer)mz_crc32(init, (const unsigned char *)s, len));
    return 1;
}

typedef tdefl_compressor lmz_Comp;

typedef struct lmz_Decomp {
    tinfl_decompressor decomp;
    mz_uint flags;
    mz_uint8 *curr;
    mz_uint8 dict[TINFL_LZ_DICT_SIZE];
} lmz_Decomp;

static void lmz_initcomp(lua_State *L, int start, lmz_Comp *c) {
    static const mz_uint probes[11] = {0, 1, 6, 32, 16, 32, 128, 256, 512, 768, 1500};
    int level = (int)luaL_optinteger(L, start, MZ_DEFAULT_LEVEL);
    mz_uint flags = probes[(level >= 0) ? MZ_MIN(10, level) : MZ_DEFAULT_LEVEL];
    tdefl_status status;
    if (lua_tointeger(L, start + 1) >= 0) flags |= TDEFL_WRITE_ZLIB_HEADER;
    if (level <= 3) flags |= TDEFL_GREEDY_PARSING_FLAG;
    if ((status = tdefl_init(c, NULL, NULL, flags)) != TDEFL_STATUS_OKAY)
        luaL_error(L, "compress failure (%d)", status);
}

static void lmz_initdecomp(lua_State *L, int start, lmz_Decomp *d) {
    int window_bits = (int)luaL_optinteger(L, start, 0);
    d->flags = window_bits >= 0 ? TINFL_FLAG_PARSE_ZLIB_HEADER : 0;
    d->flags |= TINFL_FLAG_HAS_MORE_INPUT;
    d->curr = d->dict;
    tinfl_init(&d->decomp);
}

static int lmz_compress(lua_State *L, int start, lmz_Comp *c, int flush) {
    size_t len, offset = 0, output = 0;
    const char *s = luaL_checklstring(L, start, &len);
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    for (;;) {
        size_t in_size = len - offset;
        size_t out_size = LUAL_BUFFERSIZE;
        tdefl_status status =
            tdefl_compress(c, s + offset, &in_size, (mz_uint8 *)luaL_prepbuffer(&b), &out_size, flush);
        offset += in_size;
        output += out_size;
        luaL_addsize(&b, out_size);
        if (status == TDEFL_STATUS_DONE) {
            luaL_pushresult(&b);
            lua_pushboolean(L, status == TDEFL_STATUS_DONE);
            lua_pushinteger(L, len);
            lua_pushinteger(L, output);
            return 4;
        } else if (status != TDEFL_STATUS_OKAY)
            luaL_error(L, "compress failure (%d)", status);
    }
}

static int lmz_decompress(lua_State *L, int start, lmz_Decomp *d) {
    size_t len, offset = 0, output = 0;
    const char *s = luaL_checklstring(L, start, &len);
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    for (;;) {
        size_t in_size = len - offset;
        size_t out_size = TINFL_LZ_DICT_SIZE - (d->curr - d->dict);
        tinfl_status status = tinfl_decompress(&d->decomp, (void *)(s + offset), &in_size, d->dict, d->curr, &out_size,
                                               d->flags & ~TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
        offset += in_size;
        output += out_size;
        if (out_size != 0) luaL_addlstring(&b, (char *)d->curr, out_size);
        if (status == TINFL_STATUS_DONE) {
            luaL_pushresult(&b);
            lua_pushboolean(L, status == TINFL_STATUS_DONE);
            lua_pushinteger(L, len);
            lua_pushinteger(L, output);
            return 4;
        } else if (status < 0)
            luaL_error(L, "decompress failure (%d)", status);
        d->curr = &d->dict[(d->curr + out_size - d->dict) & (TINFL_LZ_DICT_SIZE - 1)];
    }
}

static int l_compress(lua_State *L) {
    lua_settop(L, 3);
    lmz_Comp c;
    luaL_checkstring(L, 1);
    lmz_initcomp(L, 2, &c);
    return lmz_compress(L, 1, &c, TDEFL_FINISH);
}

static int l_decompress(lua_State *L) {
    lmz_Decomp d;
    luaL_checkstring(L, 1);
    lmz_initdecomp(L, 2, &d);
    return lmz_decompress(L, 1, &d);
}

LUAMOD_API int luaopen_miniz(lua_State *L) {
    luaL_Reg libs[] = {
        {"adler32", l_adler32},       {"crc32", l_crc32}, {"compress", l_compress},
        {"decompress", l_decompress}, {NULL, NULL},
    };
    luaL_newlib(L, libs);
    return 1;
}
