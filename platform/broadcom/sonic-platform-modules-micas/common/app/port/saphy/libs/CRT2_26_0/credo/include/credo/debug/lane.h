#ifndef CR_DBG_LANE_H
#define CR_DBG_LANE_H

#include "credo/base.h"
#include "credo/debug/option.h"
#include "credo/debug/params.h"
#include "credo/params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display information about a lane.
 * @deprecated Alias of display_slice_info where lane is parsed to a string
 * @param[in] slice slice handle
 * @param[in] command command string to use
 * @param[in] lane which lane to use
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_display_info(CredoSlice_t* slice, const char* command, int lane);

/**
 * @brief Get the number of lane option
 * @param[in] slice
 * @param[out] count number of lane options
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_option_count(CredoSlice_t* slice, int* count);

/**
 * @brief Get lane option by index
 * @param[in] slice
 * @param[in] index index of lane option to get
 * @param[out] option lane option information
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_index_option_list(CredoSlice_t* slice, int index, CredoLaneOption_t* option);

/**
 * @brief query if the given lane option name is supported
 * @param[in] slice slice handle
 * @param[in] option_name option name of the query
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_is_option_supported(CredoSlice_t* slice, const char* option_name);

// Top Level

/**
 * @brief Lane Configuration Structure
 * Used to configure the lanes for properties that the firmware cannot handle.
 *
 */
typedef struct {
    uint8_t tx_polarity_swap;  //!< whether to swap tx polarity
    uint8_t rx_polarity_swap;  //!< whether to swap rx polarity
    // TODO: remove this isnt needed
    uint8_t facing_line_side;  //!< is the lane line side or host side
    uint8_t using_dc_input;    //!< is the lane ac or dc coupled
} CredoLaneConfig_t;

/**
 * @brief Set target lane config
 * @deprecated This implies these are the only parameters that need to be set when there are more. Just use the
 * individual functions instead
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] lane_config
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_set_config(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config);

/**
 * @brief Get target lane config
 * @deprecated This implies these are the only parameters that need to be set when there are more. Just use the
 * individual functions instead
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] lane_config
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_config(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config);

/**
 * @brief Get slice max host/line number
 * @param[in] slice slice handle
 * @param[out] host_lane
 * @param[out] line_lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_count(CredoSlice_t* slice, int* host_lane, int* line_lane);

/**
 * @brief Per lane Logic reset
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_logic_reset(CredoSlice_t* slice, int lane);

/**
 * @brief per lane register map reset
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_reg_reset(CredoSlice_t* slice, int lane);

/**
 * @brief Sync SDK lane mode with firmware
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_update_mode(CredoSlice_t* slice, int lane);

/**
 * @brief Enable target lane
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_enable(CredoSlice_t* slice, int lane);

/**
 * @brief Disable target lane, unsupported, this is an irreversible operation until chip reset
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_disable(CredoSlice_t* slice, int lane);

/**
 * @brief Set target lane tx state to quiet mode
 * @deprecated prefer using cr_lane_tx_force_squelch()
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_tx_disable(CredoSlice_t* slice, int lane);

/**
 * @brief Set target lane tx state to normal mode
 * @deprecated prefer using cr_lane_tx_force_squelch()
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_tx_no_disable(CredoSlice_t* slice, int lane);

/**
 * @brief Set target lane rx state to quiet mode
 * @deprecated prefer using cr_lane_rx_force_squelch()
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_rx_disable(CredoSlice_t* slice, int lane);

/**
 * @brief Set target lane rx state to normal mode
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_rx_no_disable(CredoSlice_t* slice, int lane);

#define CR_LPARAM_STATE "state"

/**
 * @brief Set SerDes Parameter with an integer value
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or lane
 * @param[in] name name of the parameter
 * @param[in] value value to set parameter
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_set_parami(CredoSlice_t* slice, const char* name, int index, int value);
/**
 * @brief Get SerDes Parameter with an integer value
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or lane
 * @param[in] name name of the parameter
 * @param[out] value value of the parameter
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_parami(CredoSlice_t* slice, const char* name, int index, int* value);

/**
 * @brief Set SerDes Parameter with an integer value
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or lane
 * @param[in] name name of the parameter
 * @param[in] value value to set parameter
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_set_paramu(CredoSlice_t* slice, const char* name, int index, unsigned value);
/**
 * @brief Get SerDes Parameter with an unsigned value
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or lane
 * @param[in] name name of the parameter
 * @param[out] value value of the parameter
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_paramu(CredoSlice_t* slice, const char* name, int index, unsigned* value);

/**
 * @brief Set SerDes Parameter with a double value
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or laner
 * @param[in] name name of the parameter
 * @param[in] value value to set parameter
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_set_paramf(CredoSlice_t* slice, const char* name, int index, double value);

/**
 * @brief Get SerDes Parameter with a double value
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or laner
 * @param[in] name name of the parameter
 * @param[out] value value of the parameter
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_paramf(CredoSlice_t* slice, const char* name, int index, double* value);

/**
 * @brief Set SerDes Parameter with multiple integer values
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or laner
 * @param[in] name name of the parameter
 * @param[in] values values to set
 * @param[in] count how many values the parameter contains -- must be the same as the definition
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_set_paramlisti(CredoSlice_t* slice, const char* name, int index, int values[], int count);
/**
 * @brief Get SerDes Parameter with multiple integer values
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or laner
 * @param[in] name name of the parameter
 * @param[out] values parameter values
 * @param[in] count how many values the parameter contains -- must be the same as the definition
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_paramlisti(CredoSlice_t* slice, const char* name, int index, int values[], int count);

/**
 * @brief Set SerDes Parameter with multiple unsigned values
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or laner
 * @param[in] name name of the parameter
 * @param[in] values values to set
 * @param[in] count how many values the parameter contains -- must be the same as the definition
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_set_paramlistu(CredoSlice_t* slice, const char* name, int index, unsigned values[],
                                             int count);
/**
 * @brief Get SerDes Parameter with multiple unsigned values
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or laner
 * @param[in] name name of the parameter
 * @param[out] values parameter values
 * @param[in] count how many values the parameter contains -- must be the same as the definition
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_paramlistu(CredoSlice_t* slice, const char* name, int index, unsigned values[],
                                             int count);

/**
 * @brief Set SerDes Parameter with multiple double values
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or laner
 * @param[in] name name of the parameter
 * @param[in] values values to set
 * @param[in] count how many values the parameter contains -- must be the same as the definition
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_set_paramlistf(CredoSlice_t* slice, const char* name, int index, double values[],
                                             int count);

/**
 * @brief Get parameter with multiple double values
 *
 *
 * @param[in] slice slice to use
 * @param[in] index index of parameter, can be top, side, or laner
 * @param[in] name name of the parameter
 * @param[out] values parameter values
 * @param[in] count how many values the parameter contains -- must be the same as the definition
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_paramlistf(CredoSlice_t* slice, const char* name, int index, double values[],
                                             int count);

/**
 * @brief Set target lane mode
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_set_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode);

#ifdef __cplusplus
}
#endif

#endif
