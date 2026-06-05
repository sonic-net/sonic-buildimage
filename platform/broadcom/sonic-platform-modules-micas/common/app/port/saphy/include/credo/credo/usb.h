#ifndef CREDO_USB_H
#define CREDO_USB_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CR_USB_PROTO_PCIE = 0,
    CR_USB_PROTO_USB = 1,
    CR_USB_PROTO_SATA = 2,
    CR_USB_PROTO_DISPLAY_PORT = 3,
    CR_USB_PROTO_USB4 = 7,
    CR_USB_PROTO_NONE = 0xFF
} CredoUsbProtocol_t;

/**
 * @brief Configure a lane into a usb mode
 * @param[in] slice slice handle
 * @param[in] lane lane to configure
 * @param[in] lane_mode lane mode to set
 * @param[in] speed speed of lane in Mb/s
 * @param[in] flags flags
 * @return Error Code
 */
CREDOAPI CredoError_t cr_usb_phy_configure(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed,
                                           uint32_t flags);

/**
 * @brief Unconfigure a lane back to off
 * @param[in] slice slice handle
 * @param[in] lane lane to configure
 * @param[in] flags flags
 * @return Error Code
 */
CREDOAPI CredoError_t cr_usb_phy_destroy(CredoSlice_t* slice, int lane, uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif
