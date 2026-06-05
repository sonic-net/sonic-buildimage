#ifndef CR_DBG_FIRMWARE_H
#define CR_DBG_FIRMWARE_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display infromation from firmware
 * @deprecated Alias of display_slice_info
 * @param[in] slice slice handle
 * @param[in] command commamnd string to use
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_display_info(CredoSlice_t* slice, const char* command);

// Firmware Management
/**
 * @brief Unload firmware on the slice
 * @param[in] slice slice handle
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_unload(CredoSlice_t* slice);

/**
 * @brief Wait for firmware to provide magic word to indicat it is ready
 * @param[in] slice slice handle
 * @param[in] timeout how long to wait usec before exiting
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_wait_magic_word(CredoSlice_t* slice, unsigned timeout);

/**
 * @brief Wait for firmware to complete top pll calibration
 * @param[in] slice slice handle
 * @param[in] timeout how long to wait usec before exiting
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_wait_top_pll_cal(CredoSlice_t* slice, unsigned timeout);

/**
 * @brief Get loaded firmware magic word
 * @param[in] slice slice handle
 * @param[out] magic magic word value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_magic(CredoSlice_t* slice, unsigned* magic);

/**
 * @brief Get the firmware hash value
 * @param[in] slice slice handle
 * @param[out] hash firmware hash
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_hash(CredoSlice_t* slice, unsigned* hash);

/**
 * @brief Get the firmware crc code
 * @param[in] slice slice handle
 * @param[out] crc crc value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_crc(CredoSlice_t* slice, unsigned* crc);

/**
 * @brief Get the firmware release date
 * @param[in] slice slice handle
 * @param[out] date firmware date
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_date(CredoSlice_t* slice, unsigned* date);

// Firmware Commands

/**
 * @brief Run a firmware command
 * @param[in] slice slice handle
 * @param[in] cmd command to run
 * @param[in] param parameter for command
 * @param[out] response response status
 * @param[out] response_param response value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_cmd(CredoSlice_t* slice, unsigned cmd, unsigned param, unsigned* response,
                                      unsigned* response_param);

/**
 * @brief Run a firmware command w/ extra parameter
 * @param[in] slice slice handle
 * @param[in] cmd command to run
 * @param[in] param1 parameter 1 for command
 * @param[in] param2 parameter 2 for command
 * @param[out] response response status
 * @param[out] response_param1 response value 1
 * @param[out] response_param2 response value 2
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_cmd_ex(CredoSlice_t* slice, unsigned cmd, unsigned param1, unsigned param2,
                                         unsigned* response, unsigned* response_param1, unsigned* response_param2);

/**
 * @brief Subset of firmware commands for debug
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[in] section section to select
 * @param[in] index index to select
 * @param[out] response_params response value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_debug_cmd(CredoSlice_t* slice, int lane, unsigned section, unsigned index,
                                            unsigned* response_params);

/**
 * @brief Subset of firmware commands for debug
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[in] section section to select
 * @param[in] index index to select
 * @param[out] response1 for response value
 * @param[out] response2 for response value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_debug_cmd_ex(CredoSlice_t* slice, int lane, unsigned section, unsigned index,
                                               unsigned* response1, unsigned* response2);
// Firmware Registers

/**
 * @brief Read a firmware register
 * @param[in] slice slice handle
 * @param[in] addr register address
 * @param[out] value register value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_reg_rd(CredoSlice_t* slice, unsigned addr, unsigned* value);

/**
 * @brief Write a firmware register
 * @param[in] slice slice handle
 * @param[in] addr register addess
 * @param[in] value register value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_reg_wr(CredoSlice_t* slice, unsigned addr, unsigned value);

/**
 * @brief Read a firmware register extended
 * @param[in] slice slice handle
 * @param[in] addr register address
 * @param[in] section register section
 * @param[out] value register value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_reg_rd_ex(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned* value);

/**
 * @brief Write a firmware register extended
 * @param[in] slice slice handle
 * @param[in] addr register address
 * @param[in] section register section
 * @param[in] value register value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_reg_wr_ex(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned value);

#ifdef __cplusplus
}
#endif

#endif
