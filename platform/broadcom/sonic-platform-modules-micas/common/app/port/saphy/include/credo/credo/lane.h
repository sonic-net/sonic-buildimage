#ifndef CREDO_LANE_H
#define CREDO_LANE_H

#include "credo/base.h"
#include "credo/params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lane TX
 */
typedef enum {
    CR_TX_UNKNOWN,
    CR_TX_LOWPOWER,
    CR_TX_TRAFFIC,
    CR_TX_SQUELCH,
    CR_TX_PRBS_PAM4,
    CR_TX_PRBS_NRZ,
    CR_TX_FORCE_DISABLE,
    CR_TX_FORCE_PRBSS_PAM4,
    CR_TX_FORCE_PRBS_NRZ,
    CR_TX_FORCE_TRAFFIC,
    CR_TX_FORCE_TEST_PATT
} CredoLaneTxState_t;

/**
 * @brief Indicate the loopback mode type of the lane
 */
typedef enum {
    CR_LB_DISABLED,  // No loopback enabled
    CR_LB_TX_TO_RX,
    CR_LB_RX_TO_TX
} CredoLaneLoopbackMode_t;

/**
 * @brief Configure a lane into a mode
 * @deprecated use cr_phy_configure() for better naming v2.8.0
 * @param[in] slice slice handle
 * @param[in] lane lane to configure
 * @param[in] lane_mode lane mode to set
 * @param[in] speed speed of lane in Mb/s
 *
 * Lets try out
 *
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_configure_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed);

/**
 * @brief Configure a lane into phy loopback mode
 * @deprecated use cr_phy_configure_shallow_retimer() for better naming v2.8.0
 * @param[in] slice slice handle
 * @param[in] lane lane to configure
 * @param[in] speed speed of lane in Mb/s
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_configure_mode_loopback(CredoSlice_t* slice, int lane, uint32_t speed);

/**
 * @brief Unconfigure a lane back to off
 * @deprecated use cr_phy_destroy() for better naming v2.8.0
 * @param[in] slice slice handle
 * @param[in] lane lane to configure
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_destroy_mode(CredoSlice_t* slice, int lane);

/**
 * @brief get the value of the given lane option name
 * @param[in] slice slice handle
 * @param[in] lane lane index
 * @param[in] option_name option name
 * @param[out] value pointer to the return value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_option(CredoSlice_t* slice, int lane, const char* option_name, int* value);

/**
 * @brief set the value of the given lane option name
 * @param[in] slice slice handle
 * @param[in] lane lane index
 * @param[in] option_name option name
 * @param[in] value the value to set
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_set_option(CredoSlice_t* slice, int lane, const char* option_name, int value);

// Loopback

/**
 * @brief Set target lane loopback mode
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_set_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t mode);

/**
 * @brief Get target lane loopback mode
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t* mode);

// Lane Mode

/**
 * @brief Get target lane RX mode
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode);

/**
 * @brief Get RX lane speed
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] speed_kbps speed_kbps value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);

/**
 * @brief Get TX lane speed
 *
 * Certain chips allow lane tx and rx to run at different speeds
 *
 * @param[in] slice
 * @param[in] lane
 * @param[out] speed_kbps
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_tx_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);

/**
 * @brief Get TX lane mode
 *
 * @param[in] slice
 * @param[in] lane
 * @param[out] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_tx_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode);

/**
 * @brief Enable/Disable forcing the TX into squelch
 *
 * When force squelch is removed, it will go into normal traffic mode.
 * @note When unsquelching it does not store the previous tx state so you may have to reconfigure
 *
 * @param[in] slice slice
 * @param[in] lane lane to
 * @param[in] enable whether to enable force squelch
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_tx_force_squelch(CredoSlice_t* slice, int lane, bool enable);

/**
 * @brief Get target lane tx state
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] status
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_tx_get_status(CredoSlice_t* slice, int lane, CredoLaneTxState_t* status);

// RX Control
//

/**
 * @brief Enable/Disable forcing the RX into squelch
 *
 * Squelch prevents it from adapting to any signal from the transmitter
 *
 * When force squelch is removed, it will go into normal traffic mode.
 * @note When unsquelching it does not store the previous tx state so you may have to reconfigure
 *
 * @param[in] slice slice
 * @param[in] lane lane to
 * @param[in] enable whether to enable force squelch
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_rx_force_squelch(CredoSlice_t* slice, int lane, bool enable);

// Resets

/**
 * @brief Reset target lane
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_rx_reset(CredoSlice_t* slice, int lane);

/** @brief Get serdes parameter
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[in,out] data You must provide the value storage, type, and count. param.value buffer will be updated
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_param(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data);

/**
 * @brief Get serdes param information and use heap
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[out] data parameter data, you must free after use
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_get_paramh(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data);

/**
 * @brief Set serdes parameter
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[out] data
 * @return Error Code
 */
CREDOAPI CredoError_t cr_lane_set_param(CredoSlice_t* slice, const char* name, int index, const CredoParamData_t* data);

// utility for c11 that enables easy overload access
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

#define cr_lane_get_paramv(slice, name, index, val, count) cr_lane_get_param(slice, name, index, CR_PDATA(val, count))

#define cr_lane_set_paramv(slice, name, index, val, count) cr_lane_set_param(slice, name, index, CR_PDATA(val, count))

#endif

#ifdef __cplusplus
}
#endif

#endif
