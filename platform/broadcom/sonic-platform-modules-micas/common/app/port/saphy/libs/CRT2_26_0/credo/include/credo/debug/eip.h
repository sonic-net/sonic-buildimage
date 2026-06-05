#ifndef CR_DBG_EIP_H
#define CR_DBG_EIP_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Indicate the packet inject mode
 */
typedef enum { CR_PKTINJ_FUNC, CR_PKTINJ_DEBUG } CredoPacketInjectMode_t;

/**
 * @brief Packte inject configuration
 */
typedef struct {
    CredoPacketInjectMode_t* mode;
    bool* infinite;
    uint32_t* packet_number;
    uint16_t* packet_len;
    uint8_t* packet_data;
#ifdef SWIG
%immutable;
#endif
    uint16_t packet_data_len;
#ifdef SWIG
%mutable;
#endif
} CredoPacketInjectConfig_t;

/**
 * @brief Indicate the packet capture mode
 */
typedef enum {
    CR_PKTCAPT_UNCOND = 0,
    CR_PKTCAPT_DST_MACADDR = 1,
    CR_PKTCAPT_NONERASE = 1 << 16,
    CR_PKTCAPT_VPORT_EN = 1 << 17,
} CredoPacketCaptureMode_t;

/**
 * @brief Packte capture tcam configuration
 */
typedef struct {
    uint8_t dst_macaddr[6];
    uint8_t ethertype[2];
    uint8_t extension[6];
} CredoTcamPacketData_t;

/**
 * @brief Packte capture configuration
 */
typedef struct {
    uint32_t mode;
    uint32_t vport_index;
    uint32_t rule_index;
    CredoTcamPacketData_t key;
    CredoTcamPacketData_t mask;
} CredoPacketCaptureConfig_t;

/**
 * @brief Packet capture buffer status
 */
typedef struct {
    uint32_t captured_packets;
    uint32_t filled_segments;
    uint32_t dropped_packets;
} CredoPacketCaptureBufferStatus_t;

/**
 * @brief Packet capture buffer segment status
 */
typedef struct {
    bool truncated;
    uint32_t port_id;
    uint32_t vport_id;
} CredoPacketCaptureSegmentStatus_t;

// enlarge 14 to align 128
#define CR_PACKET_MAX_SIZE (1522 + 14)
// +-----+-----+------+-----------+---------+-----+
// | DST | SRC | VLAN | Type/Size | Payload | CRC |  Total size
// |  6  |  6  |  4   |    2      | 42-1500 |  4  |   64-1522
// +-----+-----+------+-----------+---------+-----+

/**
 * @brief Packet capture information
 */
typedef struct {
    uint8_t packet_data[CR_PACKET_MAX_SIZE];
#ifdef SWIG
%immutable;
#endif
    uint32_t size;
#ifdef SWIG
%mutable;
#endif
    CredoPacketCaptureSegmentStatus_t seg_status;
} CredoPacketCaptureInfo_t;

/**
 * @brief Packet inject in the given port.
 * Note that this function is still experimental
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Side selection
 * @param[in] pkt_config Inject Packet configuration
 * @return NS CredoError_t
 */
CREDOAPI CredoError_t cr_eip_inject_packet(CredoSlice_t* slice, uint32_t portId, CredoSide_t side,
                                           CredoPacketInjectConfig_t* pkt_config);

/**
 * @brief Packet capture setup in the given port.
 * Note that this function is still experimental
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Side selection
 * @param[in] pkt_config Capture Packet configuration
 * @return NS CredoError_t
 */
CREDOAPI CredoError_t cr_eip_setup_capture_packet(CredoSlice_t* slice, uint32_t portId, CredoSide_t side,
                                                  CredoPacketCaptureConfig_t* pkt_config);

/**
 * @brief Packet capture stop in the given port.
 * Note that this function is still experimental
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Side selection
 * @return NS CredoError_t
 */
CREDOAPI CredoError_t cr_eip_stop_capture_packet(CredoSlice_t* slice, uint32_t portId, CredoSide_t side);

/**
 * @brief Packet capture buffer status in the given port.
 * Note that this function is still experimental
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Side selection
 * @param[out] status Capture packet buffer status
 * @return NS CredoError_t
 */
CREDOAPI CredoError_t cr_eip_get_capture_packet_buffer_status(CredoSlice_t* slice, uint32_t portId, CredoSide_t side,
                                                              CredoPacketCaptureBufferStatus_t* status);

/**
 * @brief Clear packet capture buffer in the given port.
 * Note that this function is still experimental
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Side selection
 * @return NS CredoError_t
 */
CREDOAPI CredoError_t cr_eip_clear_capture_packet_buffer(CredoSlice_t* slice, uint32_t portId, CredoSide_t side);

/**
 * @brief Get packet capture info in the given port.
 * Note that this function is still experimental
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Side selection
 * @param[out] pkt_info Capture packet information
 * @return NS CredoError_t
 */
CREDOAPI CredoError_t cr_eip_get_capture_packet_info(CredoSlice_t* slice, uint32_t portId, CredoSide_t side,
                                                     CredoPacketCaptureInfo_t* pkt_info);

/**
 * @brief Print to logs the supported topology types
 * @param[in] slice
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_eip_display_topologies(CredoSlice_t* slice);

#ifdef __cplusplus
}
#endif

#endif
