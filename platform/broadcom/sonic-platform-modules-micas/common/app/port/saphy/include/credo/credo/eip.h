#ifndef CREDO_EIP_H
#define CREDO_EIP_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief
 */
typedef enum {
    CR_MACSEC_EGRESS,  //!< MACsec Egress
    CR_MACSEC_INGRESS
} CredoMACsecDirection_t;

/**
 * @brief MACsec data path for Egress and Ingress
 */
typedef struct {
    uint32_t start_offset1;
    uint32_t end_offset1;
    uint32_t start_offset2;
    uint32_t end_offset2;
} CredoMACsecDataPath_t;

/**
 * @brief Enable/Disable low latency bypass with respect to MACsec flow
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] direction MACsec direction
 * @param[in] enable
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_eip_set_low_latency_bypass(CredoSlice_t* slice, uint32_t portId,
                                                    CredoMACsecDirection_t direction, uint8_t enable);

/**
 * @brief Retrieve low latency bypass status with respect to MACsec flow
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] direction MACsec direction
 * @param[out] enable Bypass status
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_eip_get_low_latency_bypass(CredoSlice_t* slice, uint32_t portId,
                                                    CredoMACsecDirection_t direction, uint8_t* enable);

/**
 * @brief Get the client ID of specified port/side.
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Side selection
 * @param[out] client Holds value to be returned
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_eip_get_client_id(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, uint32_t* client);

/**
 * @brief Get the channel id of specified port.
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[out] channel_id Holds value to be returned
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_eip_get_channel_id(CredoSlice_t* slice, uint32_t portId, uint32_t* channel_id);

/**
 * @brief Return the data path of the given MACsec flow.
 * @param[in] slice Slice handle
 * @param[in] direction MACsec direction
 * @param[out] datapath Returned data path structure
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_eip_get_macsec_datapath(CredoSlice_t* slice, CredoMACsecDirection_t direction,
                                                 CredoMACsecDataPath_t* datapath);

/**
 * @brief Stops EIP before port is destroyed.
 * @param[in] slice
 * @param[in] portId
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_eip_stop(CredoSlice_t* slice, uint32_t portId);

/* MAC and EIP configuration function. Should be replaceable */

/**
 * @brief Configures EIP when port is created.
 * @param[in] slice
 * @param[in] portId
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_eip_configure(CredoSlice_t* slice, uint32_t portId);

/**
 * @brief Set the topology of the slice.
 *
 * The topology shall only be changed when no port is configured.
 *
 * @param[in] slice
 * @param[in] topology
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_eip_set_topology(CredoSlice_t* slice, const char* topology);

/**
 * @brief Get the current running topology of the slice.
 * @param[in] slice
 * @param[out] topology
 * @param[in] len
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_eip_get_topology(CredoSlice_t* slice, char* topology, unsigned len);

#ifdef __cplusplus
}
#endif

#endif
