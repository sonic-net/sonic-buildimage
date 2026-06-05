#ifndef CREDO_SHELL_H
#define CREDO_SHELL_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CR_CMDLINE_SIZE 1024

/**
 * readline function signature that is custom readline function
 * @param[in] prompt prompt string
 * @param[out] input input string
 * @return Error Code
 */
typedef CredoError_t (*CredoReadLine_t)(const char* prompt, char input[CR_CMDLINE_SIZE]);

/**
 * @brief Set custom shell readline function
 * @param[in] func readline function pointer
 */
CRLUAAPI void cr_shell_set_readline(CredoReadLine_t func);

/**
 * @brief Shell
 *
 * @note Prefer using cr_shell_spawn_enhanced() unless using custom shell readline / logger hooks
 *
 * @param[in] slices slices to spawn shell. Use NULL to use current slices
 * @param[in] slice_count how many slices to spawn shell. Pass 0 for current slices
 * @return Error Code
 */
CRLUAAPI CredoError_t cr_shell_spawn(CredoSlice_t* slices[], int slice_count);

/**
 * @brief Spawn shell with battery included tools
 *
 * - Tab completion
 * - Line editing
 * - Command History
 *
 * @note Uses the currently selected slices for spawning the shell.

 * @return Error Code
 */
CRLUAAPI CredoError_t cr_shell_spawn_enhanced(void);

/**
 * @brief Run a shell socket server
 *
 *
 * Socket created at /tmp/credo-server.sock. Use cr_shell_spawn_server2 if you would like to specify a different
 * location
 * @return Error Code
 */
CRLUAAPI CredoError_t cr_shell_spawn_server(void);

/**
 * @brief Run a shell socket server
 *
 * Using Unix domain sockets, this will spawn a server that can be accessed via the crcli executable.
 * This is mainly for debugging a daemon process version of the sdk-- especially before the shell is integrated into
 * your own shell tool.
 *
 * Access the shell server using the `crcli` binary. Run `crcli --help` for more information on the client.
 * @param[in] socket_path path to set socket. Pass NULL to use the default path.
 * @return CRLUAAPI
 */
CRLUAAPI CredoError_t cr_shell_spawn_server2(const char* socket_path);

/**
 * @brief set shell slices
 * @param[in] slices slices to set
 * @param[in] slice_count how many slices
 * @return Error Code
 */
CRLUAAPI CredoError_t cr_shell_set_slices(CredoSlice_t* slices[], int slice_count);

/**
 * @brief Set the shell logger callback
 *
 * Separate optional logger for when running a shell command or a slice operation is being performed from a shell.
 * Otherwise it will default to normal sdk logger.
 * @param shell_log_cb shell log call back function
 * @return Error Code
 */
CRLUAAPI CredoError_t cr_shell_set_logger(CredoLog_t shell_log_cb);

/**
 * @brief shell writing streams
 */
typedef enum { CR_SHELL_STDOUT = 0, CR_SHELL_STDERR = 1 } CredoShellStream_t;

/**
 * @brief Lua writer of printed data
 * @param[in] ud user defined data passed in to callback
 * @param[in] stream indicates if stdout or stderr
 * @param[in] message message to write
 */
typedef void (*CredoShellWriter_t)(void* ud, CredoShellStream_t stream, const char* message);

/**
 * @brief Set the shell writer callback
 *
 * Custom writer for anything printed in the shell from lua. This is separate from the logger which handles
 * The writer is meant to essentially be a printf tool.
 * @param[in] writer writer callback
 * @param[in] ud userdata to pass to the callback (store state)
 * @return Error Code
 */
CRLUAAPI CredoError_t cr_shell_set_writer(CredoShellWriter_t writer, void* ud);

#ifdef __cplusplus
}
#endif

#endif  // CREDO_SHELL_H
