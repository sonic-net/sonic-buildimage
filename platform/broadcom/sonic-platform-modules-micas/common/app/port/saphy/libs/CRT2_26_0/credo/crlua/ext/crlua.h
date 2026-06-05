#ifndef CRLUA_H
#define CRLUA_H

#include "lua.h"

#include "credo.h"

void crluaL_openlibs(lua_State *L);
CredoError_t crlua_run(int argc, char *argv[]);
CredoError_t crlua_run_command(CredoSlice_t *slices[], size_t slice_count, const char *command,
                               CredoShellWriter_t writer, void *ud);
#endif
