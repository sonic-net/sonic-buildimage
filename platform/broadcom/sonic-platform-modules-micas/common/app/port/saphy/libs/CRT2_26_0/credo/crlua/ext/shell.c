#include "crlua.h"
#include "shell.h"

#include "crintl/logger.h"
#include "credo.h"
#include "sdk.h"
#ifndef _WIN32  // no windows support for line noise
#include "linenoise.h"
#endif

#include <string.h>

crsh_t crsh = {.lock = PTHREAD_MUTEX_INITIALIZER, 0};

static void __attribute__((constructor)) shell_init(void) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&crsh.lock, &attr);
}

// utility to perform slice lock
#define SHELL_LOCK() (pthread_mutex_lock(&crsh.lock))

#define SHELL_UNLOCK() (pthread_mutex_unlock(&crsh.lock))

void cr_lua_set_completion_adder(cr_lua_completer_cb_t cb) {
    crsh.lua_add_completion = cb;
}

CredoError_t cr_shell_set_logger(CredoLog_t shell_log_cb) {
    SHELL_LOCK();
    crsh.logger = shell_log_cb;
    cri_logger_set_shell_logger(shell_log_cb);
    SHELL_UNLOCK();
    return CR_OK;
}

CredoError_t cr_shell_set_writer(CredoShellWriter_t writer, void *ud) {
    SHELL_LOCK();
    crsh.writer = writer;
    crsh.writer_ud = ud;
    SHELL_UNLOCK();
    return CR_OK;
}

CredoSlice_t *cr_shell_get_slice(unsigned slice_id) {
    for (size_t i = 0; i < crsh.slice_count; i++) {
        if (crsh.slices[i]->slice_id == slice_id) return crsh.slices[i];
    }
    return NULL;
}

static CredoError_t check_slice_valid(CredoSlice_t *slice) {
    return slice ? CR_OK : CR_FAIL;
}

static void free_crsh_slices(void) {
    if (crsh.slices) {
        free(crsh.slices);
        crsh.slices = NULL;
    }
    if (crsh.slices_selected) {
        free(crsh.slices_selected);
        crsh.slices_selected = NULL;
    }
}

static CredoError_t allocate_crsh_slices(size_t count) {
    free_crsh_slices();
    crsh.slices = (CredoSlice_t **)calloc(sizeof(CredoSlice_t *), count);
    crsh.slices_selected = (bool *)calloc(sizeof(bool), count);
    crsh.slice_count = count;
    return crsh.slices && crsh.slices_selected ? CR_OK : CR_OUT_OF_MEMORY;
}

static CredoError_t set_crsh_slices(CredoSlice_t **slices, size_t slice_count) {
    if (slice_count < 1) return CR_INVALID_ARGS;
    if (allocate_crsh_slices(slice_count) != CR_OK) {
        free_crsh_slices();
        return CR_OUT_OF_MEMORY;
    }

    memcpy(crsh.slices, slices, sizeof(CredoSlice_t *) * slice_count);
    memset(crsh.slices_selected, 0, sizeof(bool) * slice_count);

#define SWAP_SLICE(m, n)       \
    do {                       \
        CredoSlice_t *tmp = m; \
        m = n;                 \
        n = tmp;               \
    } while (0)

    // sort slice_id
    for (size_t i = 0; i < slice_count - 1; i++) {
        CredoSlice_t **slice = &crsh.slices[i];
        if (check_slice_valid(*slice) != CR_OK) continue;
        for (size_t j = i + 1; j < slice_count; j++) {
            if ((*slice)->slice_id > crsh.slices[j]->slice_id) {
                SWAP_SLICE(*slice, crsh.slices[j]);
            }
        }
    }

    // group sdk handle
    for (size_t i = 0; i < slice_count - 1; i++) {
        CredoSlice_t *slice = crsh.slices[i];
        if (check_slice_valid(slice) != CR_OK) continue;
        for (size_t j = i + 1; j < slice_count; j++) {
            if (slice->sdk == crsh.slices[j]->sdk) {
                if (i + 1 != j) SWAP_SLICE(crsh.slices[i + 1], crsh.slices[j]);
                break;
            }
        }
    }

    // group device handle
    for (size_t i = 0; i < slice_count - 1; i++) {
        CredoSlice_t *slice = crsh.slices[i];
        if (check_slice_valid(slice) != CR_OK) continue;
        for (size_t j = i + 1; j < slice_count; j++) {
            if (slice->sdk != crsh.slices[j]->sdk) break;
            if (slice->device == crsh.slices[j]->device) {
                if (i + 1 != j) SWAP_SLICE(crsh.slices[i + 1], crsh.slices[j]);
                break;
            }
        }
    }

    // sort slice_id in the same device
    for (size_t i = 0; i < slice_count - 1; i++) {
        CredoSlice_t **slice = &crsh.slices[i];
        if (check_slice_valid(*slice) != CR_OK) continue;
        for (size_t j = i + 1; j < slice_count; j++) {
            if ((*slice)->device != crsh.slices[j]->device) break;
            if ((*slice)->slice_id > crsh.slices[j]->slice_id) {
                SWAP_SLICE(*slice, crsh.slices[j]);
            }
        }
    }
    crsh.slices_selected[0] = true;  // always start with the first slice being selected
    return CR_OK;
}

// enhanced shell support only for non windows (posix) builds

#ifndef _WIN32  // no windows support for line noise

static CredoError_t credo_enhanced_readline(const char *prompt, char input[CR_CMDLINE_SIZE]) {
    char *line = NULL;
    while (line == NULL) {
        line = linenoise(prompt);
        if (line == NULL) {
            return CR_FAIL;
        }
        // make sure line is not too long
        if (strlen(line) >= CR_CMDLINE_SIZE) {
            linenoiseFree(line);
            line = NULL;
            continue;
        }
    }
    snprintf(input, CR_CMDLINE_SIZE, "%s", line);
    linenoiseHistoryAdd(line);
    linenoiseHistorySave("./.crsh_history");
    linenoiseFree(line);

    return CR_OK;
}

static const char *log_level_to_str(CredoLogLevel_t level) {
    switch (level) {
        case CR_LOG_TRACE:
            return "TRACE";
        case CR_LOG_DEBUG:
            return "DEBUG";
        case CR_LOG_INFO:
            return "INFO";
        case CR_LOG_WARN:
            return "WARNING";
        case CR_LOG_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}
static void credo_enhanced_shell_logger(void *slice_context, void *user_data, CredoLogLevel_t level, const char *scope,
                                        const char *message) {
    (void)(slice_context);
    (void)(user_data);
    // store log output
    const char separator = strchr(message, '\n') != NULL ? '\n' : ' ';
    const char *level_str = log_level_to_str(level);

    if (strcmp(scope, "lua") == 0) {
        printf("%s", message);
    } else if (level == CR_LOG_INFO) {
        printf("%s\n", message);
    } else {
        printf("[%-7s%8s]%c%s\n", level_str, scope, separator, message);
    }
}

static void cr_linenoise_add_completion(const char *buf, void *lc, size_t pos) {
    linenoiseAddCompletion((linenoiseCompletions *)lc, buf, pos);
}

CredoError_t cr_shell_spawn_enhanced() {
    SHELL_LOCK();
#ifndef _WIN32
    linenoiseHistoryLoad("./.crsh_history");
    linenoiseHistorySetMaxLen(100);
    linenoiseSetCompletionCallback((linenoiseCompletionCallback *)cr_lua_get_completions);
    cr_shell_set_readline(credo_enhanced_readline);
    cr_lua_set_completion_adder(cr_linenoise_add_completion);
#endif
    CredoLog_t old_shell_logger = crsh.logger;
    cr_shell_set_logger(credo_enhanced_shell_logger);

    CredoError_t err = cr_shell_spawn(NULL, 0);

    // clear shell configuration
#ifndef _WIN32
    cr_shell_set_readline(NULL);
#endif
    cr_shell_set_logger(old_shell_logger);
    SHELL_UNLOCK();
    return err;
}
#else
CredoError_t cr_shell_spawn_enhanced() {
    return cr_shell_spawn(NULL, 0);
}
#endif

CredoError_t cr_shell_spawn(CredoSlice_t *slices[], int slice_count) {
    SHELL_LOCK();
    if (slice_count == 0 && crsh.slice_count == 0) {
        SHELL_UNLOCK();
        return CR_INVALID_ARGS;
    }
    if (slice_count > 0) {
        set_crsh_slices(slices, slice_count);
    }
    CredoError_t err;
    // allow for the lua shell to be restarted
    do {
        crsh.restart = false;
        char lua[32] = "lua";
        char *argv[32] = {lua};
        err = cr_lua_exec_full(1, argv, crsh.slices, crsh.slice_count);
        if (err != CR_OK) {
            crsh.restart = false;
            break;
        }
    } while (crsh.restart);
    SHELL_UNLOCK();
    return err;
}

CredoError_t cr_shell_set_slices(CredoSlice_t *slices[], int slice_count) {
    if (slice_count <= 0) {
        return CR_INVALID_ARGS;
    }
    SHELL_LOCK();
    CredoError_t err = set_crsh_slices(slices, slice_count);
    SHELL_UNLOCK();
    return err;
}

void cr_shell_set_readline(CredoReadLine_t func) {
    crsh.readline = func;
}

#ifdef _WIN32
CredoError_t cr_shell_spawn_server2(const char *socket_path) {
    (void)socket_path;
    return CR_UNSUPPORTED;
}

CredoError_t cr_shell_spawn_server() {
    return CR_UNSUPPORTED;
}
#endif
