#ifndef CREDO_SLICE_H
#define CREDO_SLICE_H

#include "credo/base.h"
#include "credo/device.h"
#include "credo/params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Credo slice types
 *
 * All the possible credo slice types that are supported by the sdk.
 */
typedef enum {

    CR_SLC_BALDEAGLE = 0x000100,

    CR_SLC_OWL = 0x000200,
    CR_SLC_OWL_A0 = 0x000201,
    CR_SLC_OWL_B0 = 0x000202,
    CR_SLC_OWL_B0B = 0x000203,
    CR_SLC_OWL_B1 = 0x000204,

    CR_SLC_BLACKHAWK = 0x000300,
    CR_SLC_BLACKHAWK_DC = 0x000301,
    CR_SLC_BLACKHAWK_AC = 0x000302,

    CR_SLC_HERON = 0x000400,

    CR_SLC_OSPREY = 0x000500,

    CR_SLC_NUTCRACKER = 0x000600,

    CR_SLC_KITE = 0x000700,

    CR_SLC_ADMIRAL = 0x000800,

    CR_SLC_SEAHAWK = 0x000A00,

    CR_SLC_OSTRICH = 0x000B00,

    CR_SLC_SCREAMING_EAGLE = 0x000C00,

    CR_SLC_VICEROY_TEST = 0x000D00,
    CR_SLC_VICEROY_IN_NC = 0x000D01,

    CR_SLC_RAVEN = 0x000E00,

    CR_SLC_NIGHTHAWK = 0x000F00,
    CR_SLC_PEREGRINE_EVB = 0x000F01,
    CR_SLC_PEREGRINE = 0x000F02,
    CR_SLC_CROW = 0x000F03,

    CR_SLC_SKIPPER_TEST = 0x001000,

    CR_SLC_SPARROW = 0x001100,

    CR_SLC_BLUEJAY = 0x001200,

    CR_SLC_MONARCH2P1_TEST = 0x001300,

    CR_SLC_FAKE = 0x100000
} CredoSliceType_t;

/**
 * @brief Credo Family
 *
 * Credo Chip Families, used in things such as slash commands
 */
typedef enum {
    CR_FAMILY_BALDEAGLE = 0x1,
    CR_FAMILY_OWL = 0x2,
    CR_FAMILY_BLACKHAWK = 0x3,
    CR_FAMILY_HERON = 0x4,
    CR_FAMILY_OSPREY = 0x5,
    CR_FAMILY_NUTCRACKER = 0x6,
    CR_FAMILY_KITE = 0x7,
    CR_FAMILY_ADMIRAL = 0x8,
    CR_FAMILY_SEAHAWK = 0xA,
    CR_FAMILY_OSTRICH = 0xB,
    CR_FAMILY_SCREAMING_EAGLE = 0xC,
    CR_FAMILY_VICEROY = 0xD,
    CR_FAMILY_RAVEN = 0xE,
    CR_FAMILY_NIGHTHAWK = 0xF,
    CR_FAMILY_SKIPPER_TEST = 0x10,
    CR_FAMILY_SPARROW = 0x11,
    CR_FAMILY_BLUEJAY = 0x12,
    CR_FAMILY_MONARCH2P1_TEST = 0x13,
    CR_FAMILY_FAKE = 0x1000
} CredoFamily_t;

/**
 * @brief Slice init type enum
 * Used to configure the lanes for properties that the firmware cannot handle.
 *
 */
typedef enum {
    CR_INIT_FULL = 0,         //!< full reset, load firmware, initialize any sensible default settings
    CR_INIT_NO_FIRMWARE = 1,  //!< fully reset, do not load firmware, initialize any sensible default settings
    CR_INIT_WARM = 2,         //!< do not change any setting, fetch any settings from firmware (“warm boot”)
    CR_INIT_NONE = 3,         //!< do nothing, except setup the data structure (reg read/write setup)
    CR_INIT_SPIFLASH = 4      //!< full reset, load firmware from SPI flash, initialize any sensible default settings
} CredoSliceInitType_t;

/**
 * @brief Slice Configuration Structure
 */
typedef struct {
    CredoSliceInitType_t init_type;  //!< init type
    const char* firmware_filename;   //!< path to firmware file to load
    uint32_t slice_id;               //!< identifier of the slice for logging purposes and crsh slice selection
    void* slice_context;  //!< provide register access information. NOTE: this value must be in heap or globally as it
                          //!< is stored as a reference in the slice handle
} CredoSliceConfig_t;

/**
 * Initialize a slice with a configuration. Provide the context to communicate with the slice and other information such
 * as firmware to load.
 * @param[in] slice slice handle
 * @param[in] config configuration for the slice
 * @return Error Code
 */

CREDOAPI CredoError_t cr_slice_init(CredoSlice_t* slice, const CredoSliceConfig_t* config);

/**
 * @brief Convenience functio to re-initialize a slice
 *
 * Underneath it simply calls slice_init again but keeps track of the information you provided.
 *
 * @note only do this if the slice has already been initialized
 * @param slice slice handle
 * @param init initialization type
 * @param firmware firmware to use if the init_type requires it.
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_reinit(CredoSlice_t* slice, CredoSliceInitType_t init, const char* firmware);

// Slice Information
//

/**
 * @brief Get slice company OUI
 * @param[in] slice slice handle
 * @param[out] oui company OUI
 * @return Error code
 */
CREDOAPI CredoError_t cr_slice_get_oui(CredoSlice_t* slice, unsigned* oui);

/**
 * @brief Get slice model number
 * @param[in] slice slice handle
 * @param[out] model_number model number
 * @return Error code
 */
CREDOAPI CredoError_t cr_slice_get_model_number(CredoSlice_t* slice, unsigned* model_number);

/**
 * @brief Get slice revision number
 * @param[in] slice slice handle
 * @param[out] revision_number revision number
 * @return Error code
 */
CREDOAPI CredoError_t cr_slice_get_revision_number(CredoSlice_t* slice, unsigned* revision_number);

/**
 * @brief Get slice type of the slice.
 * @param[in] slice slice handle
 * @param[out] slice_type slice type of the slice
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_get_type(CredoSlice_t* slice, CredoSliceType_t* slice_type);

/**
 * @brief Get slice's chip family
 * @param[in] slice
 * @param[out] family
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_get_family(CredoSlice_t* slice, CredoFamily_t* family);

/**
 * @brief Get device type of the slice.
 * @param[in] slice slice handle
 * @param[out] device_type device type of the slice
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_get_device_type(const CredoSlice_t* slice, CredoDeviceType_t* device_type);

/**
 * @brief Get slice vsensor.
 * @param[in] slice slice handle
 * @param[out] vsensor slice voltage value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_get_vsensor(CredoSlice_t* slice, double* vsensor);

/**
 * @brief Get slice vsensor with input mux.
 * @param[in] slice slice handle
 * @param[in] sel_vin vsensor mux select
 * @param[out] vsensor slice voltage value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_get_vsensor_ex(CredoSlice_t* slice, unsigned sel_vin, double* vsensor);

/**
 * @brief Set the slice mdio mode between push/pull and pull-up.
 *
 * This should be set before cr_slice_init() is called.
 *
 * NOTE: Please confirm with Credo hardware team before using push/pull to ensure hardware configuration is correct.
 * Otherwise damage may occur to the board.
 *
 * @param slice slice to set mdio mode
 * @param is_push_pull set into push_pull mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_set_mdio_mode(CredoSlice_t* slice, bool is_push_pull);

/**
 * @brief get the value of the given option name
 * @param[in] slice slice handle
 * @param[in] option_name option name
 * @param[out] value pointer to the return value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_get_option(CredoSlice_t* slice, const char* option_name, int* value);

/**
 * @brief set the value of the given option name
 * @param[in] slice slice handle
 * @param[in] option_name option name
 * @param[in] value the value to set
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_set_option(CredoSlice_t* slice, const char* option_name, int value);

/**
 * @brief Get slice temperature
 * @param[in] slice slice handle
 * @param[out] temp temperature in Celsius
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_get_temperature(CredoSlice_t* slice, double* temp);

/** @brief Get serdes parameter
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[in,out] data You must provide the value storage, type, and count. param.value buffer will be updated
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_get_param(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data);

/**
 * @brief Get serdes param information and use heap
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[out] data parameter data, you must free after use
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_get_paramh(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data);

/**
 * @brief Set serdes parameter
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[out] data
 * @return Error Code
 */
CREDOAPI CredoError_t cr_slice_set_param(CredoSlice_t* slice, const char* name, int index,
                                         const CredoParamData_t* data);

/**
 * @brief Get slice id
 *
 * @param[in] slice
 * @return slice id
 */
CREDOAPI uint32_t cr_slice_id(CredoSlice_t* slice);

// utility for c11 that enables easy overload access
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

#define cr_slice_get_paramv(slice, name, index, val, count) cr_slice_get_param(slice, name, index, CR_PDATA(val, count))

#define cr_slice_set_paramv(slice, name, index, val, count) cr_slice_set_param(slice, name, index, CR_PDATA(val, count))

#endif

#ifdef __cplusplus
}
#endif

#endif
