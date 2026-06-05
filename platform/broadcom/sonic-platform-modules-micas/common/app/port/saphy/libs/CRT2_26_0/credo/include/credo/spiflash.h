#ifndef CREDO_SPIFLASH_H
#define CREDO_SPIFLASH_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Load firmware from SPI flash onto slice
 * @param[in] slice slice handle
 * @param[in] partition_num partition number in SPI flash
 * @return Error Code
 */
CREDOAPI CredoError_t cr_spiflash_load_firmware(CredoSlice_t* slice, int partition_num);

/**
 * @brief Display MBR information
 * @param[in] slice slice handle
 * @return Error Code
 */
CREDOAPI CredoError_t cr_spiflash_display_mbr(CredoSlice_t* slice);

/**
 * @brief Format MBR in SPI flash
 * @param[in] slice slice handle
 * @param[in] flash_kb_size kilobytes size for flash
 * @param[in] force force to format MBR
 * @return Error Code
 */
CREDOAPI CredoError_t cr_spiflash_format_mbr(CredoSlice_t* slice, unsigned flash_kb_size, int force);

/**
 * @brief Read firmware from SPI flash
 * @param[in] slice slice handle
 * @param[in] fwname firmware path
 * @param[in] partition_num partition number in SPI flash
 * @return Error Code
 */
CREDOAPI CredoError_t cr_spiflash_read_firmware(CredoSlice_t* slice, const char* fwname, int partition_num);

/**
 * @brief Write firmware into SPI flash
 * @param[in] slice slice handle
 * @param[in] fwname firmware path
 * @param[in] partition_num partition number in SPI flash
 * @param[in] force force to write firmware to flash
 * @return Error Code
 */
CREDOAPI CredoError_t cr_spiflash_write_firmware(CredoSlice_t* slice, const char* fwname, int partition_num, int force);

/**
 * @brief set partition number for SPI flash
 * @param[in] slice slice handle
 * @param[in] partition_num partition number in SPI flash
 * @return Error Code
 */
CREDOAPI CredoError_t cr_spiflash_set_partition(CredoSlice_t* slice, int partition_num);

#ifdef __cplusplus
}
#endif

#endif
