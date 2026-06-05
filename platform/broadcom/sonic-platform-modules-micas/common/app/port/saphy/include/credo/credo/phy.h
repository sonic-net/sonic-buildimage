#ifndef CREDO_PHY_H
#define CREDO_PHY_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief lane flags
 *
 */
typedef enum {
    CR_LFLAG_TX = (1 << 0),
    CR_LFLAG_RX = (1 << 1),
    CR_LFLAG_AN = (1 << 8),
    CR_LFLAG_LT = (1 << 9),
    CR_LFLAG_FLEXSPEED = (1 << 12),
    CR_LFLAG_LOOPBACK = (1 << 16)
} CredoLaneFlags_t;

/**
 * @brief Configure lane into phy mode
 * @param[in] slice slice to configure
 * @param[in] lane lane to configure
 * @param[in] lane_mode mode to configure
 * @param[in] speed speed to configure (Mb/s)
 * @param[in] flags special configuration features (0 is default)
 * @return Error Code
 */
CREDOAPI CredoError_t cr_phy_configure(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed,
                                       uint32_t flags);

/**
 * @brief Configure lane into shallow retimer
 * @param[in] slice slice to configure
 * @param[in] lane lane to configure
 * @param[in] lane_mode mode to configure
 * @param[in] speed speed to configure (Mb/s)
 * @param[in] flags special configuration features (0 is default)
 * @return Error Code
 */
CREDOAPI CredoError_t cr_phy_configure_shallow_retimer(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode,
                                                       unsigned speed, uint32_t flags);

/**
 * @brief Destroy Phy mode configured for a lane
 * @param[in] slice slice to destroy
 * @param[in] lane lane to destroy
 * @return Error Code
 */
CREDOAPI CredoError_t cr_phy_destroy(CredoSlice_t* slice, int lane);

/**
 * @brief Indicate if the PHY is up
 *
 * It is an alias of cr_serdes_get_phy_ready()
 *
 * @param[in] slice
 * @param[in] lane
 * @param[out] up is lane rx up
 * @return Error Code
 */
CREDOAPI CredoError_t cr_phy_is_link_up(CredoSlice_t* slice, int lane, bool* up);

#ifdef __cplusplus
}
#endif

#endif
