#ifndef CREDO_FW_H
#define CREDO_FW_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Load firmware onto slice
 * @param[in] slice slice handle
 * @param[in] image_file file path to firmware
 * @param[in] force force if firmware is already loaded
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_load(CredoSlice_t* slice, const char* image_file, int force);

/**
 * @brief Load firmware onto slice from buffer
 * @param[in] slice slice handle
 * @param[in] buf buffer to load from
 * @param[in] size size of the buffer
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_loadbuf(CredoSlice_t* slice, void* buf, size_t size);

/**
 * @brief Firmware load onto multiple slices simultaneously
 * @param[in] slices slices to load to simultaneously
 * @param[in] slice_count how many slices to load
 * @param[in] image_file file path to firmware binary
 * @param[in] delay_time_us delay between write frames before writing the next frame
 * @param[in] force force if firmware is already loaded
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_load_broadcast(CredoSlice_t* slices[], int slice_count, const char* image_file,
                                                 unsigned delay_time_us, int force);

/**
 * @brief Firmware load onto multiple slices simultaneously with a buffer
 * @param[in] slices slices to load simultaneously
 * @param[in] slice_count slice count
 * @param[in] buf buffer to load firmware
 * @param[in] size size of buffer
 * @param[in] delay_time_us delay between write frames before writing the next frame
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_loadbuf_broadcast(CredoSlice_t* slices[], int slice_count, void* buf, size_t size,
                                                    unsigned delay_time_us);

/**
 * @brief Get firmware running status
 * @param[in] slice slice handle
 * @param[out] status firmware status
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_firmware_get_status(CredoSlice_t* slice, unsigned* status);

// Firmware Information

/**
 * @brief Get the firmware version
 * @param[in] slice slice handle
 * @param[out] version firmware version encoded
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_version(CredoSlice_t* slice, unsigned* version);

/**
 * @brief Get the firmware version as a human readable string
 * @param[in] slice slice handle
 * @param[out] version firmware version string
 * @return Error Code
 */
CREDOAPI CredoError_t cr_firmware_version_str(CredoSlice_t* slice, char version[32]);

#ifdef __cplusplus
}
#endif

#endif
