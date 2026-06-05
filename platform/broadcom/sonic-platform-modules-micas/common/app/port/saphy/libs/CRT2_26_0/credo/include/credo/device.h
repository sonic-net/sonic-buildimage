#ifndef CREDO_DEVICE_H
#define CREDO_DEVICE_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Credo device types
 *
 * All the possible credo device types that are supported by the sdk.
 */
typedef enum {
    // IMPORTANT: Devices base signifier must be 4 bits larger than slices
    CR_DEV_BALDEAGLE_400 = 0x001001,
    CR_DEV_BALDEAGLE_800 = 0x001002,

    CR_DEV_OWL_400 = 0x002001,
    CR_DEV_OWL_800 = 0x002002,

    CR_DEV_BLACKHAWK_400 = 0x003001,
    CR_DEV_BLACKHAWK_800 = 0x003002,

    CR_DEV_HERON_P1 = 0x004001,
    CR_DEV_HERON_P3 = 0x004002,
    CR_DEV_HERON_1P0 = 0x004003,
    CR_DEV_HERON_MR = 0x004004,

    CR_DEV_OSPREY_400 = 0x005001,
    CR_DEV_OSPREY_800 = 0x005002,
    CR_DEV_OSPREY_AEC = 0x005003,

    CR_DEV_NUTCRACKER_32 = 0x006001,

    CR_DEV_KITE_TEST = 0x007001,

    CR_DEV_ADMIRAL_TEST = 0x008001,

    CR_DEV_SEAHAWK = 0x00A001,

    CR_DEV_OSTRICH_1P1 = 0x00B002,

    CR_DEV_SCREAMING_EAGLE_AEC = 0x00C001,
    CR_DEV_SCREAMING_EAGLE = 0x00C002,

    CR_DEV_VICEROY_TEST = 0x00D001,
    CR_DEV_VICEROY_IN_NC = 0x00D002,

    CR_DEV_RAVEN_TEST = 0x00E001,

    CR_DEV_NIGHTHAWK_1P0 = 0x00F001,
    CR_DEV_NIGHTHAWK = 0x00F002,

    CR_DEV_SKIPPER_TEST = 0x010001,
    CR_DEV_SKIPPER = 0x010002,

    CR_DEV_SPARROW_800 = 0x011001,

    CR_DEV_BLUEJAY = 0x012001,

    CR_DEV_MONARCH2P1_TEST = 0x013001,
    CR_DEV_MONARCH2P1 = 0x013002,

    CR_DEV_PEREGRINE_EVB = 0x014001,
    CR_DEV_PEREGRINE = 0x014002,

    CR_DEV_CROW = 0x015001,

    CR_DEV_FAKE_400 = 0x100001,
    CR_DEV_FAKE_800 = 0x100002,
    CR_DEV_FAKE_32 = 0x100003,

} CredoDeviceType_t;

/**
 * @brief Create a device
 *
 * Using a specified device_type create a device handle that can be used to access the slices for that device.
 *
 * @param[in] sdk sdk handle
 * @param[in] device_type what kind of Credo Device to create
 * @param[out] device device handle to create
 * @return Error Code
 */
CREDOAPI CredoError_t cr_device_create(CredoSdk_t* sdk, CredoDeviceType_t device_type, CredoDevice_t** device);

/**
 * @brief Destroy a device handle.
 * @param[in] device device handle to destroy
 * @return Error Code
 */
CREDOAPI CredoError_t cr_device_destroy(CredoDevice_t* device);

/**
 * @brief Detect device type of the slice.
 * @param[in] sdk sdk handle
 * @param[in] slice_context register access information
 * @param[out] device_type device type
 * @return Error Code
 */
CREDOAPI CredoError_t cr_device_detect_type(CredoSdk_t* sdk, void* slice_context, CredoDeviceType_t* device_type);

/**
 * @brief Detect device type of the slice with push pull mode. This function will set to push pull mode when reading
 * device info.
 * @param[in] sdk sdk handle
 * @param[in] slice_context register access information
 * @param[out] device_type device type
 * @return Error Code
 */
CREDOAPI CredoError_t cr_device_detect_type_push_pull(CredoSdk_t* sdk, void* slice_context,
                                                      CredoDeviceType_t* device_type);

/**
 * @brief Get device type of an allocated device.
 * @param[in] device device handle to destroy
 * @param[out] device_type what kind of Credo Device the device uses
 * @return Error Code
 */
CREDOAPI CredoError_t cr_device_get_type(CredoDevice_t* device, CredoDeviceType_t* device_type);

/**
 * @brief Get the device type string name
 * @param[in] device device handle
 * @return string name of device_type
 */
CREDOAPI const char* cr_device_get_type_name(CredoDevice_t* device);

// Slice Management
/**
 * @brief Get how many slices the device contains
 * @param[in] device device handle
 * @param[out] slice_count slice count for the device
 * @return Error Code
 */
CREDOAPI CredoError_t cr_device_get_slice_count(CredoDevice_t* device, int* slice_count);

/**
 * @brief Get a slice handle from the device.
 * @param[in] device device handle
 * @param[in] slice_index slice index to obtain
 * @param[out] slice slice handle
 * @return Error Code
 */
CREDOAPI CredoError_t cr_device_get_slice(CredoDevice_t* device, int slice_index, CredoSlice_t** slice);

#ifdef __cplusplus
}
#endif
#endif
