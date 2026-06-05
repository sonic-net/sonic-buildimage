#ifndef CREDO_DATACAP_H
#define CREDO_DATACAP_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback to write data
 *
 * @param[in] ud custom userdata passed in to the start
 * @param[in] format printf formatting string
 */
typedef void (*CredoDataWriter_t)(void* ud, const char* format, ...) CR_PRINTF_ATTRIBUTE_FORMAT(2, 3);

/**
 * @brief Data catpure argument structure
 * Simple key, value
 */
typedef struct {
    const char* name;
    int value;
} CredoDataCaptureArg_t;

/**
 * @brief convenience macro to make creating the data args array easier
 */
#define CREDO_DATA_ARGS(...) ((CredoDataCaptureArg_t[]){__VA_ARGS__})

/**
 * @brief Low level data capture API
 *
 * @note use this only if you need something other than cr_data_capture_file() or cr_data_capture_buffer()
 * @param[in] slice slice to perform command
 * @param[in] command command to run
 * @param[in] argv argument list
 * @param[in] argc argument count
 * @param[in] writer call back function that writes the data
 * @param[in] ud data passed into callback writer
 * @return Error Code
 */
CREDOAPI CredoError_t cr_data_capture(CredoSlice_t* slice, const char* command, const CredoDataCaptureArg_t argv[],
                                      size_t argc, CredoDataWriter_t writer, void* ud);

/**
 * @brief Capture data to a file
 * @param[in] slice slice to perform command
 * @param[in] command command to run
 * @param[in] argv argument list
 * @param[in] argc argument count
 * @param[in] file file to write data to
 * @return Error Code
 */
CREDOAPI CredoError_t cr_data_capture_file(CredoSlice_t* slice, const char* command, const CredoDataCaptureArg_t argv[],
                                           size_t argc, const char* file);

/**
 * @brief Capture data to a buffer
 *
 * @note you must free `output` once done with the data
 * @param[in] slice slice to perform command
 * @param[in] command command to perform
 * @param[in] argv argument list
 * @param[in] argc number of arguments
 * @param[out] output buffer created that stores data.
 * @return Error Code
 */
CREDOAPI CredoError_t cr_data_capture_buffer(CredoSlice_t* slice, const char* command,
                                             const CredoDataCaptureArg_t argv[], size_t argc, char** output);

#ifdef __cplusplus
}
#endif

#endif
