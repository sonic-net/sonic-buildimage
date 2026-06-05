#ifndef CR_DBG_PORT_H
#define CR_DBG_PORT_H

#include "credo/base.h"
#include "credo/debug/option.h"
#include "credo/debug/params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display information about a port to logger.
 * @deprecated Alias of display_slice_info where port_id is parsed to a string
 * @param[in] slice slice handle
 * @param[in] command command string to use
 * @param[in] port_id which port to use
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_display_info(CredoSlice_t* slice, const char* command, uint32_t port_id);

/**
 * @brief Get port option defintion at given index
 * @param slice
 * @param[in] index
 * @param[out] option
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_index_option_def(CredoSlice_t* slice, int index, CredoOption_t* option);

/**
 * @brief Get the number of port options availble
 * @param[in] slice
 * @param[out] count
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_get_option_count(CredoSlice_t* slice, int* count);

#ifdef __cplusplus
}
#endif

#endif
