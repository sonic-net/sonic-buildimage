#ifndef CR_DBG_SLICE_H
#define CR_DBG_SLICE_H

#include "credo/base.h"
#include "credo/debug/option.h"
#include "credo/debug/params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get custom data stored in the slice handle.
 * @deprecated since v1.2.0, store slice specific information in the slice context
 * @param[in] slice slice handle
 * @return user data
 */
CREDOAPI void* cr_slice_get_userdata(const CredoSlice_t* slice);

/**
 * @brief Store custom data in slice handle.
 * @deprecated since v1.2.0, store slice specific information in the slice context
 * @param[in] slice slice handle
 * @param[in] userdata data to store
 * @return Error code
 */
CREDOAPI CredoError_t cr_slice_set_userdata(CredoSlice_t* slice, void* userdata);

/**
 * @brief Slice Capability Structure
 */
typedef struct {
    int max_ports;     //!< max ports
    int max_lanes;     //!< max lanes
    int max_vsensors;  //!< max voltage sensors
} CredoSliceLimit_t;

/**
 * @brief Get slice limitation.
 * @param[in] slice slice handle
 * @param[out] limits slice limitation
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_get_limits(const CredoSlice_t* slice, CredoSliceLimit_t* limits);

/**
 * @brief Read a register from the slice
 * @param[in] slice slice handle
 * @param[in] address register address to read
 * @param[out] value register value read
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_read(CredoSlice_t* slice, unsigned address, unsigned* value);

/**
 * @brief Write a register in the slice
 * @param[in] slice slice handle
 * @param[in] address register address to write
 * @param[in] value register value to write
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_write(CredoSlice_t* slice, unsigned address, unsigned value);

/**
 * @brief Read contiguous registers in the slice
 * @param[in] slice slice handle
 * @param[in] first_address first register address to read
 * @param[out] value Array to store all values from burst read
 * @param[in] count how many registers to read
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_burst_read(CredoSlice_t* slice, unsigned first_address, unsigned value[],
                                          unsigned count);

/**
 * @brief Write contiguous registers in the slice
 * @param[in] slice slice handle
 * @param[in] first_address first register address to write
 * @param[in] value Array of all the register values to write
 * @param[in] count how many register to write
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_burst_write(CredoSlice_t* slice, unsigned first_address, const unsigned value[],
                                           unsigned count);

/**
 * @brief Load a register setup file to a slice
 * @param[in] slice slice handle
 * @param[in] file_path_or_data path to register setup file or data to use (must include '\n' if data)
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_load_setup(CredoSlice_t* slice, const char* file_path_or_data);

/**
 * @brief Save register setup of a slice to a file
 * @param[in] slice slice handle
 * @param[in] file_path path to save file
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_save_setup(CredoSlice_t* slice, const char* file_path);

/**
 * @brief Display information about a slice.
 *
 * Use command="help" or empty string to display documentation of commands, parameters, and descriptions available.
 * @note this is an alias of cr_display_info_log()
 * @param[in] slice slice handle
 * @param[in] command command string to use
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_display_info(CredoSlice_t* slice, const char* command);

/**
 * @brief Get the number of slice option
 * @param[in] slice
 * @param[out] count number of slice options
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_get_option_count(CredoSlice_t* slice, int* count);

/**
 * @brief Get lane option by index
 * @param[in] slice
 * @param[in] index lane option index to get
 * @param[out] option slice option information
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_index_option_list(CredoSlice_t* slice, int index, CredoSliceOption_t* option);

/**
 * @brief query if the given option name is supported
 * @param[in] slice slice handle
 * @param[in] option_name option name of the query
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_is_option_supported(CredoSlice_t* slice, const char* option_name);

/**
 * @brief Software reset, reset FIFO, PHY, register map, cpu
 * @param[in] slice slice handle
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_soft_reset(CredoSlice_t* slice);

/**
 * @brief Logic reset, reset FIFO, PHY
 * @param[in] slice slice handle
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_logic_reset(CredoSlice_t* slice);

/**
 * @brief CPU reset
 * @param[in] slice slice handle
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_mcu_reset(CredoSlice_t* slice);

/**
 * @brief CPU reset, keep in reset mode
 * @param[in] slice slice handle
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_mcu_reset_hold(CredoSlice_t* slice);

/**
 * @brief Register map reset
 * @param[in] slice slice handle
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_reg_reset(CredoSlice_t* slice);

/**
 * @brief Read efuse bank
 *
 * @param[in] slice
 * @param[in] bank bank to read
 * @param[out] val value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_efuse_read(CredoSlice_t* slice, int bank, uint32_t* val);
/**
 * @brief Read raw ECID value
 *
 * Must go through post-proccessing to capture information
 *
 * @param[in] slice
 * @param[out] ecid
 * @return Error Code
 */
CREDOAPI CredoError_t cr_efuse_read_ecid(CredoSlice_t* slice, uint32_t ecid[2]);

/**
 * @addtogroup SliceParam
 * @{
 */
#define CR_SLCPARAM_FW_CMD_TIMEOUT    "fw_cmd_timeout"
#define CR_SLCPARAM_FW_UNLOAD_TIMEOUT "fw_unload_timeout"
#define CR_SLCPARAM_FW_UP_TIME        "fw_up_time"
#define CR_SLCPARAM_TOP_CAL_TIMEOUT   "top_cal_timeout"
#define CR_SLCPARAM_REFCLK            "refclk"
#define CR_SLCPARAM_FFCLK             "ffclk"
#define CR_SLCPARAM_PAM4_CHNL         "pam4_chnl"
#define CR_SLCPARAM_RX_REFCLK_SEL     "rx_refclk_sel"
#define CR_SLCPARAM_TX_REFCLK_SEL     "tx_refclk_sel"
#define CR_SLCPARAM_REFCLK_B2I        "refclk_b2i"
#define CR_SLCPARAM_SLICE_ID          "slice_id"
/** @} */

#ifdef __cplusplus
}
#endif

#endif
