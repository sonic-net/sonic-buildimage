#ifndef SHELL_BINDINGS_H
#define SHELL_BINDINGS_H
#include "credo.h"

#include <stdlib.h>

// Functions that need to be available for our libraries to interact with the shell but not external

#ifdef __cplusplus
extern "C" {
#endif

#define CR_REGISTRY_CMD_SLICES     "credo_cmd_slices"
#define CR_REGISTRY_CMD_SLICES_MAP "credo_cmd_slices_map"

CRLUAAPI CredoSlice_t* cr_shell_get_slice(unsigned slice_id);

typedef void (*cr_lua_completer_cb_t)(const char* buf, void* lc, size_t pos);

CRLUAAPI CredoError_t cr_lua_exec(int argc, char* argv[]);
CRLUAAPI CredoError_t cr_lua_exec_full(int argc, char* argv[], CredoSlice_t* slices[], size_t slice_count);

CRLUAAPI void cr_lua_set_completion_adder(cr_lua_completer_cb_t cb);

CRLUAAPI void cr_lua_get_completions(const char* buf, void* lc, size_t pos);

CRLUAAPI void cr_lua_strfree(char** retstr);

#ifdef __cplusplus
}
#endif

#endif
