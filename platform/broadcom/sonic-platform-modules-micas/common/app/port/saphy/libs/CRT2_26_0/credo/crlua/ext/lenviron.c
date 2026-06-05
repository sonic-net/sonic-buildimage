#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <crt_externs.h>
#endif

/*
ENV_DECLARE_ENVIRON                          extern char **environ

ENV_PUTENV_HAS_UNDERLINE                     _putenv

ENV_SET_BY_PUTENV                            putenv("KEY=value")
ENV_SET_BY_SETENV                            setenv("KEY", "value", 1)

ENV_UNSET_BY_PUTENV                          putenv("KEY")
ENV_UNSET_BY_PUTENV_EQ                       putenv("KEY=")
ENV_UNSET_BY_UNSETENV                        unsetenv("KEY")
ENV_UNSETENV_RET_VOID
ENV_UNSET_BY_SETENV                          setenv("KEY", "", 1)
ENV_UNSET_BY_SETENV_NULL                     setenv("KEY", NULL, 1)
*/

/*export*/
#ifdef _WIN32
#define LENV_EXPORT_API __declspec(dllexport)
#else
#define LENV_EXPORT_API LUALIB_API
#endif

#ifdef _WIN32
#define ENV_EXPORT_WIN
#endif

#ifdef _MSC_VER
#define ENV_PUTENV_HAS_UNDERLINE
#define ENV_SET_BY_PUTENV
#define ENV_UNSET_BY_PUTENV_EQ
#define ENV_DECLARE_ENVIRON 0
#endif

#if !defined(ENV_SET_BY_PUTENV) && !defined(ENV_SET_BY_SETENV)
#if _BSD_SOURCE || _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
#define ENV_SET_BY_SETENV
#endif
#if defined(__APPLE__)
#define ENV_SET_BY_SETENV
#endif
#if defined(_WIN32)
#define ENV_SET_BY_PUTENV
#endif
#endif

#if !defined(ENV_UNSET_BY_PUTENV) && !defined(ENV_UNSET_BY_PUTENV_EQ) && !defined(ENV_UNSET_BY_UNSETENV) && \
    !defined(ENV_UNSET_BY_SETENV) && !defined(ENV_UNSET_BY_SETENV_NULL)
#if _BSD_SOURCE || _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
#define ENV_UNSET_BY_UNSETENV
#endif
#if defined(__APPLE__)
#define ENV_UNSET_BY_UNSETENV
#endif
#if defined(_WIN32)
#define ENV_UNSET_BY_PUTENV_EQ
#define ENV_DECLARE_ENVIRON 0
#endif
#endif

#if !defined(ENV_DECLARE_ENVIRON)
#if defined(__APPLE__)
#define environ             (*_NSGetEnviron())
#define ENV_DECLARE_ENVIRON 0
#endif
#endif

#if !defined(ENV_DECLARE_ENVIRON)
#define ENV_DECLARE_ENVIRON 1
#endif

static int str_upper(lua_State *L, int idx) {
    size_t l, i;
    luaL_Buffer b;
    const char *s = luaL_checklstring(L, idx, &l);
    for (i = 0; i < l; i++) {
        if (islower(s[i])) break;
    }
    if (i == l) return 1;

    luaL_buffinit(L, &b);
    for (i = 0; i < l; i++) luaL_addchar(&b, toupper((unsigned char)(s[i])));
    luaL_pushresult(&b);
    lua_remove(L, (idx > 0) ? idx : idx - 1);
    lua_insert(L, idx);
    return 1;
}

#ifdef ENV_EXPORT_WIN
#include <windows.h>

#ifdef _MSC_VER
#include <strsafe.h>
#define _USE_STR_SAFE_
#else
#include <stdio.h>
#endif

void push_lasterr(lua_State *L, const char *lpszFunction) {
    char *lpMsgBuf;
    char *lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

    lpDisplayBuf = LocalAlloc(LMEM_ZEROINIT, (lstrlen(lpMsgBuf) + lstrlen(lpszFunction) + 40));

#ifdef _USE_STR_SAFE_
    StringCchPrintfA(lpDisplayBuf, LocalSize((LPVOID)lpDisplayBuf), "%s failed with error %u: %s", lpszFunction, dw,
                     lpMsgBuf);
#else
    snprintf(lpDisplayBuf, LocalSize((LPVOID)lpDisplayBuf), "%s failed with error %u: %s", lpszFunction,
             (unsigned int)dw, lpMsgBuf);
#endif

    lua_pushstring(L, (LPTSTR)lpDisplayBuf);
    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

static int l_expenv_win(lua_State *L) {
#define INFO_BUFFER_SIZE 1024
    static char infoBuf[INFO_BUFFER_SIZE];
    const char *envVar = luaL_checkstring(L, 1);
    DWORD bufCharCount = INFO_BUFFER_SIZE;
    luaL_checktype(L, 2, LUA_TNONE);

    if (!*envVar) {
        lua_pushstring(L, "");
        return 1;
    }

    bufCharCount = ExpandEnvironmentStrings(envVar, infoBuf, INFO_BUFFER_SIZE);
    if (!bufCharCount) {
        lua_pushnil(L);
        push_lasterr(L, "\"expenv\"");
        return 2;
    }

    if (bufCharCount <= INFO_BUFFER_SIZE) {
        lua_pushstring(L, infoBuf);
        return 1;
    }

    {
        DWORD newBufSize = bufCharCount + 1;
        char *buf = malloc(newBufSize);

        if (!buf) {
            lua_pushnil(L);
            push_lasterr(L, "\"expenv\"(alloc memory)");
            return 2;
        }

        bufCharCount = ExpandEnvironmentStrings(envVar, buf, newBufSize);
        if (!bufCharCount) {
            free(buf);
            lua_pushnil(L);
            push_lasterr(L, "\"expenv\"");
            return 2;
        }

        if (bufCharCount > newBufSize) {
            free(buf);
            lua_pushnil(L);
            lua_pushstring(L, "\"expenv: can not alloc enough memoty\"");
            return 2;
        }

        lua_pushstring(L, buf);
        free(buf);
        return 1;
    }

#undef INFO_BUFFER_SIZE
}

static int l_getenviron_win(lua_State *L) {
    int as_map = lua_toboolean(L, 1);
    LPSTR lpszVariable;
    LPVOID lpvEnv;
    int i = 0;
    luaL_checktype(L, 2, LUA_TNONE);

    lpvEnv = GetEnvironmentStrings();
    if (lpvEnv == NULL) {
        lua_pushnil(L);
        push_lasterr(L, "getenviron failed:");
        return 2;
    }

    lua_newtable(L);
    if (as_map)
        for (lpszVariable = (LPTSTR)lpvEnv; *lpszVariable; lpszVariable++) {
            size_t len = lstrlen(lpszVariable);
            const char *eq = strchr(('=' == lpszVariable[0]) ? &lpszVariable[1] : &lpszVariable[0], '=');
            if (!eq) {
                lua_pushstring(L, lpszVariable);
                lua_rawseti(L, -2, ++i);
            } else {
                lua_pushlstring(L, lpszVariable, eq - lpszVariable);
                str_upper(L, -1);
                lua_pushstring(L, &eq[1]);
                lua_rawset(L, -3);
            }
            lpszVariable += len;
        }
    else
        for (lpszVariable = (LPTSTR)lpvEnv; *lpszVariable; lpszVariable++) {
            size_t len = lstrlen(lpszVariable);
            lua_pushstring(L, lpszVariable);
            lua_rawseti(L, -2, ++i);
            lpszVariable += len;
        }

    FreeEnvironmentStrings(lpvEnv);
    return 1;
}

static int l_getenv_win(lua_State *L) {
#define INFO_BUFFER_SIZE 1024

    static char infoBuf[INFO_BUFFER_SIZE];
    const char *envVar = luaL_checkstring(L, 1);
    DWORD bufCharCount = INFO_BUFFER_SIZE;
    luaL_checktype(L, 2, LUA_TNONE);

    SetLastError(ERROR_SUCCESS);
    bufCharCount = GetEnvironmentVariable(envVar, infoBuf, INFO_BUFFER_SIZE);

    if (!bufCharCount) {
        DWORD err = GetLastError();
        if (ERROR_SUCCESS == err) {  // empty string
            lua_pushliteral(L, "");
            return 1;
        }

        lua_pushnil(L);
        if (ERROR_ENVVAR_NOT_FOUND == err) return 1;

        push_lasterr(L, "\"getenv\"");
        return 2;
    }

    if (bufCharCount <= INFO_BUFFER_SIZE) {
        lua_pushstring(L, infoBuf);
        return 1;
    }

    {
        DWORD newBufSize = bufCharCount + 1;
        char *buf = malloc(newBufSize);

        if (!buf) {
            lua_pushnil(L);
            push_lasterr(L, "\"getenv\"(alloc memory)");
            return 2;
        }

        bufCharCount = GetEnvironmentVariable(envVar, buf, newBufSize);
        if (!bufCharCount) {
            free(buf);
            lua_pushnil(L);
            if (ERROR_ENVVAR_NOT_FOUND == GetLastError()) return 1;
            push_lasterr(L, "\"expenv\"");
            return 2;
        }

        if (bufCharCount > newBufSize) {
            free(buf);
            lua_pushnil(L);
            lua_pushstring(L, "\"expenv: can not alloc enough memoty\"");
            return 2;
        }

        lua_pushstring(L, buf);
        free(buf);
        return 1;
    }

#undef INFO_BUFFER_SIZE
}

static int call_win_setenv(lua_State *L, const char *envKey, const char *envVar) {
    BOOL ret = SetEnvironmentVariable(envKey, envVar);
    if (!ret) {
        lua_pushnil(L);
        push_lasterr(L, "\"setenv\"");
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int l_setenv_win(lua_State *L) {
    const char *envKey = luaL_checkstring(L, 1);
    luaL_checktype(L, 3, LUA_TNONE);

    if (lua_isnil(L, 2) || lua_isnone(L, 2))  // unset
        return call_win_setenv(L, envKey, NULL);
    return call_win_setenv(L, envKey, luaL_checkstring(L, 2));
}

static int l_update_win(lua_State *L) {
    DWORD dwReturnValue;
    LRESULT ret = SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM) "Environment", SMTO_ABORTIFHUNG,
                                     5000, &dwReturnValue);
    if (ret) {
        lua_pushnumber(L, dwReturnValue);
        return 1;
    }

    lua_pushnil(L);
    push_lasterr(L, "\"updateenv\"");
    return 2;
}

#endif  // ENV_EXPORT_WIN

//------------------------------------------
// implementation of setenv
//{-----------------------------------------

#if defined(ENV_UNSET_BY_PUTENV_EQ) && !defined(ENV_UNSET_BY_PUTENV)
#define ENV_UNSET_BY_PUTENV
#endif

#if defined(ENV_UNSET_BY_SETENV_NULL) && !defined(ENV_UNSET_BY_SETENV)
#define ENV_UNSET_BY_SETENV
#endif

#if defined(ENV_SET_BY_PUTENV) || defined(ENV_UNSET_BY_PUTENV)
#define ENV_USE_PUTENV
#endif

#ifdef ENV_USE_PUTENV
#ifdef ENV_PUTENV_HAS_UNDERLINE
#define ENV_PUTENV _putenv
#else
#define ENV_PUTENV putenv
#endif
#endif

#ifndef ENV_PUTENV_IS_ERROR
#define ENV_PUTENV_IS_ERROR(V) ((V) != 0)
#endif

#ifndef ENV_SETENV_IS_ERROR
#define ENV_SETENV_IS_ERROR(V) ((V) != 0)
#endif

#ifdef ENV_USE_PUTENV
static int env_call_putenv(lua_State *L) {
    int ret = ENV_PUTENV(lua_tostring(L, -1));
    if (ENV_PUTENV_IS_ERROR(ret)) {
        lua_pushnil(L);
        lua_pushnumber(L, ret);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}
#endif

static int env_setenv(lua_State *L) {
    const char *envKey = luaL_checkstring(L, 1);
    const char *envVar = (lua_isnil(L, 2) || lua_isnone(L, 2)) ? NULL : luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TNONE);

    /* set environment value */
    if (envVar) {  // set
#ifdef ENV_SET_BY_PUTENV
        lua_pushliteral(L, "=");
        lua_insert(L, -2);
        lua_concat(L, 3);
        return env_call_putenv(L);
#elif defined(ENV_SET_BY_SETENV)
        int ret = setenv(envKey, envVar, 1);
        if (ENV_SETENV_IS_ERROR(ret)) {
            lua_pushnil(L);
            lua_pushnumber(L, ret);
            return 2;
        }
        lua_pushboolean(L, 1);
        return 1;
#else
#error "Do not how set environment variable!"
#endif
    }

    /* unset environment value */
    if (lua_gettop(L) == 2) lua_pop(L, 1);  // remove nil

#ifdef ENV_UNSET_BY_PUTENV
#ifdef ENV_UNSET_BY_PUTENV_EQ
    lua_pushliteral(L, "=");
    lua_concat(L, 2);
#endif
    return env_call_putenv(L);
#elif defined(ENV_UNSET_BY_UNSETENV)
    int ret =
#ifdef ENV_UNSETENV_RET_VOID
        0;
#endif
    unsetenv(envKey);
    if (ENV_SETENV_IS_ERROR(ret)) {
        lua_pushnil(L);
        lua_pushnumber(L, ret);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
#elif defined(ENV_UNSET_BY_SETENV)
    int ret = setenv(envKey,
#ifdef ENV_UNSET_BY_SETENV_NULL
                     NULL
#else
                     ""
#endif
                     ,
                     1);
    if (ENV_SETENV_IS_ERROR(ret)) {
        lua_pushnil(L);
        lua_pushnumber(L, ret);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
#else
#error "Do not how unset environment variable!"
#endif
}

//}-----------------------------------------

#if ENV_DECLARE_ENVIRON
extern char **environ;
#endif

static int l_getenviron(lua_State *L) {
    int as_map = lua_toboolean(L, 1);
    char **e = environ;
    char *var = NULL;
    int i = 0;
    luaL_checktype(L, 2, LUA_TNONE);

    lua_newtable(L);
    if (e) {
        if (as_map)
            while ((var = *(e++))) {
                const char *eq = strchr(('=' == var[0]) ? &var[1] : &var[0], '=');
                if (!eq) {
                    lua_pushstring(L, var);
                    lua_rawseti(L, -2, ++i);
                    continue;
                }
                lua_pushlstring(L, var, eq - var);
                str_upper(L, -1);
                lua_pushstring(L, &eq[1]);
                lua_rawset(L, -3);
            }
        else
            while ((var = *(e++))) {
                lua_pushstring(L, var);
                lua_rawseti(L, -2, ++i);
            }
    }
    return 1;
}

static int l_getenv(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);
    const char *value = getenv(key);
    luaL_checktype(L, 2, LUA_TNONE);

    if (!value)
        lua_pushnil(L);
    else
        lua_pushstring(L, value);
    return 1;
}

static int l_setenv(lua_State *L) {
    return env_setenv(L);
}

static const struct luaL_Reg env_lib[] = {
    {"environ", l_getenviron},
    {"get", l_getenv},
    {"set", l_setenv},

#ifdef ENV_EXPORT_WIN
    {"environ_win", l_getenviron_win},
    {"expand_win", l_expenv_win},
    {"set_win", l_setenv_win},
    {"get_win", l_getenv_win},
    {"update_win", l_update_win},
#endif  // ENV_EXPORT_WIN

    {NULL, NULL} /* sentinel */
};

LENV_EXPORT_API int luaopen_environ(lua_State *L) {
    lua_newtable(L);

#if LUA_VERSION_NUM >= 502
    luaL_setfuncs(L, env_lib, 0);
#else
    luaL_openlib(L, NULL, env_lib, 0);
#endif

    return 1;
}

LENV_EXPORT_API int luaopen_environ_core(lua_State *L) {
    return luaopen_environ(L);
}
