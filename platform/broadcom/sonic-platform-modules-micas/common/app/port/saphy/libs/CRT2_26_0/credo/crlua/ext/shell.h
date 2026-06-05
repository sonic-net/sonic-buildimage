#ifndef SHELL_H
#define SHELL_H

#include "crintl/shell.h"
#include "credo.h"

#include <pthread.h>

#define CR_REGISTRY_SHELL_PROMPT "credo_shell_prompt"
#define CR_REGISTRY_COMPLETER    "credo_completer"
#define CR_REGISTRY_MUI          "credo_mui"
#define CR_REGISTRY_SLASH_MODE   "credo_slash_mode"
#define CR_REGISTRY_SHELL_OUTPUT "credo_shell_output"

typedef struct {
    CredoSlice_t **slices;
    size_t slice_count;
    cr_lua_completer_cb_t lua_add_completion;
    CredoLog_t logger;
    CredoShellWriter_t writer;
    void *writer_ud;
    pthread_mutex_t lock;
    CredoReadLine_t readline;
    bool restart;
    bool *slices_selected;
    bool in_server;
} crsh_t;

// global shell state
extern crsh_t crsh;

// utility to perform slice lock
#define SHELL_LOCK() (pthread_mutex_lock(&crsh.lock))

#define SHELL_UNLOCK() (pthread_mutex_unlock(&crsh.lock))

#endif
