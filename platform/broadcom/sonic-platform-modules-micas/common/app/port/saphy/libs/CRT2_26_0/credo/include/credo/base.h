#ifndef CREDO_TYPES_H
#define CREDO_TYPES_H

#ifndef SWIG
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

#include "credo/renamed.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef BUILD_HAL
#define CREDOAPI __declspec(dllimport)
#else
#define CREDOAPI __declspec(dllexport)
#endif
#ifdef BUILD_CRLUA
#define CRLUAAPI __declspec(dllexport)
#else
#define CRLUAAPI __declspec(dllimport)
#endif
#else
#define CREDOAPI
#define CRLUAAPI
#endif

// format checker for safety
#if defined(__clang__) || defined(__GNUC__)
#define CR_PRINTF_ATTRIBUTE_FORMAT(string_index, first_to_check) \
    __attribute__((format(printf, string_index, first_to_check)))
#else
#define CR_PRINTF_ATTRIBUTE_FORMAT(string_index, first_to_check)
#endif /* defined(FT_CLANG_COMPILER) || defined(FT_GCC_COMPILER) */

/* SDK information output, debug, error */

/**
 * A list of corresponding error code types. The list is sparse as most error code information will be kept outputted in
 * the logger.
 */
typedef enum {
    CR_SUCCESS = 0,
    CR_OK = 0,  //!< shorhand alias for CR_SUCCESS
    CR_FAIL = -1,
    CR_PHY_FW_DOWNLOAD_FAIL = -3,
    CR_FW_TIMEOUT = -4,
    CR_NOT_READY = -5,
    CR_OUT_OF_MEMORY = -6,
    CR_HAL_LOAD_FAIL = -7,
    CR_HAL_NOT_FOUND = -8,

    CR_INVALID_ARGS = -9,
    CR_MUTEX_TIMEOUT = -10,
    CR_UNSUPPORTED = -99,
    CR_NOTIMPLEMENTED = -100,
    CR_NOTIMPLEMENTED_HAL = -101,
} CredoError_t;

/**************************************************************************/
/* Context types. These are all opaque. */

/* Declare the structure first. Some old compiler just doesn't support it. */
struct CredoSdk;
struct CredoDevice;
struct CredoSlice;
/**
 * @brief Opaque handle that store driver and logger functions.
 * The SDK handle is opaque to simplify the public API.
 *
 */
typedef struct CredoSdk CredoSdk_t;

/**
 * @brief Opaque handle that stores slices and device specific information
 * The Device handle is opaque to simplify the public API.
 */
typedef struct CredoDevice CredoDevice_t;

/**
 * @brief Opaque handle that stores slice information
 * The Slice handle is opaque to simplify the public API.
 */
typedef struct CredoSlice CredoSlice_t;

/**
 * @brief The operation mode of the lane
 */
typedef enum {
    CR_LMODE_OFF = 0,
    CR_LMODE_NRZ = 1,
    CR_LMODE_PAM4 = 2,
    CR_LMODE_AN = 3,
    CR_LMODE_PAM3 = 4,
    CR_LMODE_DISABLE = 0xff
} CredoLaneMode_t;

/**
 * @brief
 */
typedef enum {
    CR_SIDE_HOST = 0,
    CR_SIDE_SYSTEM = 0,  //!< Alias for Host Side
    CR_SIDE_LINE = 1
} CredoSide_t;

#ifdef __cplusplus
}
#endif

#endif  // CREDO_BASE_H
