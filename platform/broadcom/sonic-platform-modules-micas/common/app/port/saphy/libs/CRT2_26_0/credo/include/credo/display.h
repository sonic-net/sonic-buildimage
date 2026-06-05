#ifndef CREDO_DISPLAY_H
#define CREDO_DISPLAY_H

#include "credo.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display writer callback function
 * @param[in] ud custom userdata passed in
 * @param[in] format printf formatting string
 */
typedef void (*CredoDisplayWriter_t)(void* ud, const char* format, ...) CR_PRINTF_ATTRIBUTE_FORMAT(2, 3);

/**
 * @brief Display information to the logger
 *
 * Will log everything (even if an error occurs)
 * @param[in] slice slice to display information
 * @param[in] argv argument list
 * @param[in] argc argument count
 * @return Error Code
 */
CREDOAPI CredoError_t cr_display_info_log(CredoSlice_t* slice, const char* argv[], size_t argc);

/**
 * @brief low level operation to display information with callback
 *
 * @param[in] slice slice to display information
 * @param[in] argv argument list
 * @param[in] argc argument count
 * @param[in] writer writer callback
 * @param[in] ud userdata provided to writer callback
 * @return Error Code
 */
CREDOAPI CredoError_t cr_display_info(CredoSlice_t* slice, const char* argv[], size_t argc, CredoDisplayWriter_t writer,
                                      void* ud);

/**
 * @brief Capture display information into a string
 *
 * Even if there is an error code, there could be valid display information in `output`.
 * Check if `(*output) == NULL` or `(*output)[0] == '\0'`
 *
 * @note you must free the `output` once finished
 * @param[in] slice slice to display information
 * @param[in] argv argument list
 * @param[in] argc argument count
 * @param[out] output capture display information
 * @return Error Code
 */
CREDOAPI CredoError_t cr_display_info_buffer(CredoSlice_t* slice, const char* argv[], size_t argc, char** output);
/**
 * @brief Capture display information into a file
 * @param[in] slice slice to display information
 * @param[in] argv argument list
 * @param[in] argc argument count
 * @param[in] file path to file to append data
 * @return Error Code
 */
CREDOAPI CredoError_t cr_display_info_file(CredoSlice_t* slice, const char* argv[], size_t argc, const char* file);

#ifdef __cplusplus
}
#endif

#endif
