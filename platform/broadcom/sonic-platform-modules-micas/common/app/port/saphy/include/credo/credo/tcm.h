#ifndef CREDO_TCM_H
#define CREDO_TCM_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TCM PCS/MAC device list
 */
typedef enum {
    CR_TCM_DEVICE_NA,
    CR_TCM_DEVICE_PHY,
    CR_TCM_DEVICE_SPCS0,
    CR_TCM_DEVICE_SPCS1,
    CR_TCM_DEVICE_SPCS2,
    CR_TCM_DEVICE_SPCS400G,
    CR_TCM_DEVICE_LPCS0,
    CR_TCM_DEVICE_LPCS1,
    CR_TCM_DEVICE_LPCS2,
    CR_TCM_DEVICE_LPCS400G,

    CR_TCM_DEVICE_SMAC0,
    CR_TCM_DEVICE_SMAC1,
    CR_TCM_DEVICE_SMAC2,
    CR_TCM_DEVICE_SMAC400G,
    CR_TCM_DEVICE_LMAC0,
    CR_TCM_DEVICE_LMAC1,
    CR_TCM_DEVICE_LMAC2,
    CR_TCM_DEVICE_LMAC400G,
    CR_TCM_DEVICE_MACSEC
} CredoTCMDevice_t;

/**
 * @brief TCM PCS/MAC register space list
 */
typedef enum {
    CR_TCM_REG_SPACE_NA = 0,
    CR_TCM_REG_SPACE_CH0 = 1,   //!< 10G,20G-2,25G,40G-4,50G-1,50G-2,100G-2,100G-4,200G
    CR_TCM_REG_SPACE_CH1 = 2,   //!< 10G,25G,50G-1
    CR_TCM_REG_SPACE_CH2 = 3,   //!< 10G,20G-2,25G,50G-1,50G-2,100G-2
    CR_TCM_REG_SPACE_CH3 = 4,   //!< 10G,25G,50G-1
    CR_TCM_REG_SPACE_CH00 = 5,  //!< 400G
    CR_TCM_REG_SPACE_RS_FEC = 6,
    CR_TCM_REG_SPACE_STATISTICS = 7,
    CR_TCM_REG_SPACE_RS_FEC_STATISTICS = 8
} CredoTCMRegSpace_t;

#define CR_TCM_MAX_REG_COUNT_BURST_IO 32

/**
 * @brief TCM burst IO data
 */
typedef struct {
    uint64_t value[CR_TCM_MAX_REG_COUNT_BURST_IO];
} CredoTCMBurstIOData_t;

/**
 * @brief TCM burst IO control struct
 */
typedef struct {
    uint32_t start0;
    uint32_t count0;
    uint32_t start1;
    uint32_t count1;
    CredoTCMDevice_t device_type;
    CredoTCMRegSpace_t reg_space;
} CredoTCMBurstIOCtrl_t;

#define CR_TCM_RANGE_64K_PAGE 0
#define CR_TCM_RANGE_1M_PAGE  1

/**
 * @brief TCM burst IO range program struct
 */
typedef struct {
    uint32_t base_addr;
    uint8_t range_exponent;
    uint8_t reg_count;
    uint8_t page_size;  //!< 0: 64K; 1: 1M
} CredoTCMBurstIORangeProgram_t;

/**
 * @brief TCM burst IO range param struct
 */
typedef struct {
    uint8_t ingress;
    uint8_t param_index;
    uint8_t page;
    uint8_t range;
} CredoTCMBurstIORangeParam_t;

/**
 * @brief TCM burst IO range struct
 */
typedef struct {
    CredoTCMBurstIORangeParam_t ranges[2];
    bool two_range;
    bool use_48bit;
} CredoTCMBurstIORange_t;

// TCM R/W
/**
 * @brief Read TCM register
 * @param[in] slice slice handle
 * @param[in] addr register address
 * @param[out] data register value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_tcm_read(CredoSlice_t* slice, unsigned addr, unsigned* data);

/**
 * @brief Write TCM regiser
 * @param[in] slice slice handle
 * @param[in] addr register address
 * @param[in] data register value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_tcm_write(CredoSlice_t* slice, unsigned addr, unsigned data);

/* TCM multiple raw address read/write. This go through firmware. 64-bit statistics read is automatic. */

/**
 * @brief Reads continuous TCM registers.
 *
 * This reads a continuous range of TCM registers with help of firmware.
 * 64-bit statistics read is automatically handled.
 *
 * @param[in] slice Slice handle
 * @param[in] first_address First TCM register address
 * @param[out] val Pointer to hold returned registers.
 * @param[in] count
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_tcm_burst_read(CredoSlice_t* slice, unsigned first_address, uint64_t val[], unsigned count);

/**
 * @brief Writes continuous TCM registers.
 *
 * This writes a continuous range of TCM registers with help of firmware.
 *
 * @param[in] slice
 * @param[in] first_address
 * @param[out] val
 * @param[in] count
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_tcm_burst_write(CredoSlice_t* slice, unsigned first_address, const uint64_t val[],
                                         unsigned count);

/**
 * @brief Writes TCM registers by device id and register space.
 *
 * This writes a TCM register by burst io control struct
 *
 * @param[in] slice
 * @param[in] io_ctrl
 * @param[in] value
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_tcm_memset(CredoSlice_t* slice, const CredoTCMBurstIOCtrl_t* io_ctrl, unsigned value);

/**
 * @brief Writes TCM registers by specific page program struct
 *
 * @param[in] slice
 * @param[in] index
 * @param[in] range_param
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_tcm_range_program(CredoSlice_t* slice, int index,
                                           const CredoTCMBurstIORangeProgram_t* range_param);

/**
 * @brief Read TCM registers parallel by specific range control sturct
 *
 * @param[in] slices
 * @param[in] slice_count
 * @param[in] io_range
 * @param[out] data
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_tcm_multi_slice_range_read(CredoSlice_t* slices[], int slice_count,
                                                    const CredoTCMBurstIORange_t* io_range[],
                                                    CredoTCMBurstIOData_t* data[]);

#ifdef __cplusplus
}
#endif

#endif
