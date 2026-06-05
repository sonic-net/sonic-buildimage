#ifndef CREDO_SLASH_H
#define CREDO_SLASH_H

#include "credo/base.h"
#include "credo/shell.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Low level command runner that allows callbacks
 * @param[in] slices slices to run command (of the same type)
 * @param[in] slice_count number of slices
 * @param[in] command slash command
 * @param[in] writer data writer callback
 * @param[in] ud userdata state to pass to writer callback
 * @return Error Code
 */
CRLUAAPI CredoError_t cr_slash_command(CredoSlice_t* slices[], size_t slice_count, const char* command,
                                       CredoShellWriter_t writer, void* ud);
/**
 * @brief Run command and write data to string buffer
 * @note must free `output` once finished
 * @param[in] slices slices to run command
 * @param[in] slice_count number fo slices
 * @param[in] command slash command
 * @param[out] output output of command
 * @return Error Code
 */
CRLUAAPI CredoError_t cr_slash_command_buffer(CredoSlice_t* slices[], size_t slice_count, const char* command,
                                              char** output);
/**
 * @brief Run command and append data to file
 * @param[in] slices slices to run command
 * @param[in] slice_count number fo slices
 * @param[in] command slash command
 * @param[in] file file to write data to
 * @return Error Code
 */
CRLUAAPI CredoError_t cr_slash_command_file(CredoSlice_t* slices[], size_t slice_count, const char* command,
                                            const char* file);

/**
 * @brief Run command and print with logger
 * @param[in] slices slices to run command
 * @param[in] slice_count number fo slices
 * @param[in] command slash command
 * @return Error Code
 */
CRLUAAPI CredoError_t cr_slash_command_log(CredoSlice_t* slices[], size_t slice_count, const char* command);

#ifdef __cplusplus
}
#endif

#endif  // CREDO_SLASH_H
