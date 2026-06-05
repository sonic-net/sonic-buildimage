
#include "lauxlib.h"
#include "lua.h"

#include <time.h>

static int ltime_sleep(lua_State *L) {
    double sleep_time = luaL_checknumber(L, 1);
    luaL_argcheck(L, sleep_time >= 0, 1, "sleep length must be non-negative");
    struct timespec delay;
    time_t sleep_time_sec = (time_t)(sleep_time);
    delay.tv_sec = sleep_time_sec;
    delay.tv_nsec = (long)((sleep_time - (double)sleep_time_sec) * 1000000000);
    nanosleep(&delay, NULL);
    return 0;
}

static int ltime_time(lua_State *L) {
    struct timespec time;
    int err = clock_gettime(CLOCK_REALTIME, &time);
    if (err != 0) {
        luaL_error(L, "unable to get real time");
    }
    double out_time = (double)time.tv_sec + (double)time.tv_nsec / 1000000000;
    lua_pushnumber(L, out_time);
    return 1;
}

static int ltime_thread_time(lua_State *L) {
    struct timespec time;
    int err = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time);
    if (err != 0) {
        luaL_error(L, "unable to get thread time");
    }
    double out_time = (double)time.tv_sec + (double)time.tv_nsec / 1000000000;
    lua_pushnumber(L, out_time);
    return 1;
}

static int ltime_process_time(lua_State *L) {
    struct timespec time;
    int err = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
    if (err != 0) {
        luaL_error(L, "unable to get process time");
    }
    double out_time = (double)time.tv_sec + (double)time.tv_nsec / 1000000000;
    lua_pushnumber(L, out_time);
    return 1;
}

static int ltime_monotonic(lua_State *L) {
    struct timespec time;
    int err = clock_gettime(CLOCK_MONOTONIC, &time);
    if (err != 0) {
        luaL_error(L, "unable to get monotonic time");
    }
    double out_time = (double)time.tv_sec + (double)time.tv_nsec / 1000000000;
    lua_pushnumber(L, out_time);
    return 1;
}

static const struct luaL_Reg time_functions[] = {
    {"sleep", ltime_sleep},
    {"time", ltime_time},
    {"monotonic", ltime_monotonic},
    {"thread_time", ltime_thread_time},
    {"process_time", ltime_process_time},
    {NULL, NULL},
};

int luaopen__time(lua_State *L) {
    luaL_newlib(L, time_functions);
    return 1;
}
