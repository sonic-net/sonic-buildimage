#ifndef CREDO_SDK_H
#define CREDO_SDK_H

#include "credo/base.h"
#include "credo/logger.h"

#ifdef __cplusplus
extern "C" {
#endif

// Driver API

/**
 * Read register function signature that a driver must provide.
 * @param[in] slice_context provides the slice context to read from the slice
 * @param[in] reg_addr register address to read
 * @param[out] val pass back the value
 * @return Error Code if driver fails to read value
 */
typedef CredoError_t (*CredoReadRegister_t)(void* slice_context, unsigned reg_addr, unsigned* val);

/**
 * Write register signature that a driver must provide.
 * @param[in] slice_context provides the slice context to write to the slice
 * @param[in] reg_addr register address to write
 * @param[in] val value to write
 * @return Error Code if driver fails to write value
 */
typedef CredoError_t (*CredoWriteRegister_t)(void* slice_context, unsigned reg_addr, unsigned val);

/* Optional device access */

/**
 * @brief Optional burst read register signature
 * @param[in] slice_context provides the slice context to burst read the slice
 * @param[in] first_addr the initial address to read from
 * @param[out] val where the read values should be stored
 * @param[in] count how many registers to read
 */
typedef CredoError_t (*CredoReadRegisterBurst_t)(void* slice_context, unsigned first_addr, unsigned val[],
                                                 unsigned count);

/**
 * @brief Optional burst write register signature
 * @param[in] slice_context provides the slice context to burst write the slice
 * @param[in] first_addr the initial address to write to
 * @param[in] val register values to write
 * @param[in] count how many registers to write
 */
typedef CredoError_t (*CredoWriteRegisterBurst_t)(void* slice_context, unsigned first_addr, const unsigned val[],
                                                  unsigned count);
/**
 * @brief Optional broadcast register signature
 * @param[in] slice_context provides the slice context to broadcast write to the slice
 * @param[in] reg_addr register address to write
 * @param[in] val value to write
 * @return Error Code if driver fails to write value
 */
typedef CredoError_t (*CredoWriteRegisterBroadcast_t)(void* slice_context, unsigned reg_addr, unsigned val);

/**
 * @brief Optional broadcast burst write register signature
 * @param[in] slice_context provides the slice context to broadcast burst write the slice
 * @param[in] first_addr the initial address to write to
 * @param[in] val register values to write
 * @param[in] count how many registers to write
 */
typedef CredoError_t (*CredoWriteRegisterBroadcastBurst_t)(void* slice_context, unsigned first_addr,
                                                           const unsigned val[], unsigned count);

/**
 * @brief Where to pass in driver and logger functions.
 */
typedef struct {
    CredoLog_t log;                       //!< Deprecated, use cr_logger_set
    CredoReadRegister_t read_register;    //!< Read register function pointer
    CredoWriteRegister_t write_register;  //!< Write register function pointer
    CredoLogLevel_t max_log_level;        //!< Deprecated, use cr_logger_set_level
} CredoSdkConfig_t;

// SDK Management

/**
 *
 * Providing a configuration, the sdk handle keeps the register access functions and logging functions. There is no
 * internal limit to the number of sdk handles you may create. This allows for flexibility in how external drivers are
 * implemented.
 *
 * @param[in] config reg access and logging information
 * @param[out] sdk handle to create
 * @return Error Code
 */
CREDOAPI CredoError_t cr_sdk_create(const CredoSdkConfig_t* config, CredoSdk_t** sdk);

/**
 * @brief Destroy sdk handle
 * @param[in] sdk handle to destroy
 * @return Error Code
 */
CREDOAPI CredoError_t cr_sdk_destroy(CredoSdk_t* sdk);

// SDK Information
/**
 * @brief Indicates what version of the top level sdk is run.
 * Encoded [8 bits major].[8 bits minor].[8 bits patch]
 * @return encoded sdk version
 */
CREDOAPI uint32_t cr_sdk_version(void);

/**
 * @brief Get the sdk revision string
 * @return stringified sdk revision
 */
CREDOAPI char const* cr_sdk_version_str(void);

// Chip Library

// Driver

/**
 *
 * Enable burst read functionality to speed up certain function operations (e.g., firmware commands). Burst reading is
 * for reading from contiguous register addresses.
 * @param[in] sdk sdk handle
 * @param[in] burst_read_func burst read function
 */
CREDOAPI void cr_sdk_set_burst_read(CredoSdk_t* sdk, CredoReadRegisterBurst_t burst_read_func);

/**
 * Enable burst read functionality to speed up certain function operations (e.g., firmware commands). Burst writing is
 * for writing to contiguous register addresses.
 * @param[in] sdk sdk handle
 * @param[in] burst_write_func burst write function
 */
CREDOAPI void cr_sdk_set_burst_write(CredoSdk_t* sdk, CredoWriteRegisterBurst_t burst_write_func);

/**
 * @brief Write to multiple slices at one time.
 *
 * NOTE: requires additional configuration from the driver to support feature
 * @param[in] sdk sdk handle
 * @param[in] func broadcast write function
 */
CREDOAPI void cr_sdk_set_broadcast_write(CredoSdk_t* sdk, CredoWriteRegisterBroadcast_t func);

/**
 * @brief Write to multiple slices at one time for contiguous registers.
 *
 * NOTE: requires additional configuration from the driver to support feature
 * @param[in] sdk sdk handle
 * @param[in] func broadcast burst write function
 */
CREDOAPI void cr_sdk_set_broadcast_burst_write(CredoSdk_t* sdk, CredoWriteRegisterBroadcastBurst_t func);

#ifdef __cplusplus
}
#endif

#endif
