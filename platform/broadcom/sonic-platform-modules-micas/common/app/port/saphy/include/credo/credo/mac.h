#ifndef CREDO_MAC_H
#define CREDO_MAC_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MAC statistics counters
 */
typedef struct {
    // RX statistic counters
    uint64_t etherStatsRxOctets;
    uint64_t OctetsReceivedOK;
    uint64_t aAlignmentErrors;
    uint64_t aPAUSEMACCtrlFramesReceived;
    uint64_t aFrameTooLongErrors;
    uint64_t aInRangeLengthErrors;
    uint64_t aFramesReceivedOK;
    uint64_t aFrameCheckSequenceErrors;
    uint64_t VLANReceivedOK;
    uint64_t ifInErrors;
    uint64_t ifInUcastPkts;
    uint64_t ifInMulticastPkts;
    uint64_t ifInBroadcastPkts;
    uint64_t etherStatsDropEvents;
    uint64_t etherStatsRxPkts;
    uint64_t etherStatsUndersizePkts;
    uint64_t etherStatsRxPkts64Octets;
    uint64_t etherStatsRxPkts65to127Octets;
    uint64_t etherStatsRxPkts128to255Octets;
    uint64_t etherStatsRxPkts256to511Octets;
    uint64_t etherStatsRxPkts512to1023Octets;
    uint64_t etherStatsRxPkts1024to1518Octets;
    uint64_t etherStatsRxPkts1519toMaxOctets;
    uint64_t etherStatsOversizePkts;
    uint64_t etherStatsJabbers;
    uint64_t etherStatsFragments;
    uint64_t aCBFCPAUSEFramesReceived_0;
    uint64_t aCBFCPAUSEFramesReceived_1;
    uint64_t aCBFCPAUSEFramesReceived_2;
    uint64_t aCBFCPAUSEFramesReceived_3;
    uint64_t aCBFCPAUSEFramesReceived_4;
    uint64_t aCBFCPAUSEFramesReceived_5;
    uint64_t aCBFCPAUSEFramesReceived_6;
    uint64_t aCBFCPAUSEFramesReceived_7;
    uint64_t aMACControlFramesReceived;

    // TX statistic counters
    uint64_t etherStatsTxOctets;
    uint64_t OctetsTransmittedOK;
    uint64_t aPAUSEMACCtrlFramesTransmitted;
    uint64_t aFramesTransmittedOK;
    uint64_t VLANTransmittedOK;
    uint64_t ifOutErrors;
    uint64_t ifOutUcastPkts;
    uint64_t ifOutMulticastPkts;
    uint64_t ifOutBroadcastPkts;
    uint64_t etherStatsTxPkts64Octets;
    uint64_t etherStatsTxPkts65to127Octets;
    uint64_t etherStatsTxPkts128to255Octets;
    uint64_t etherStatsTxPkts256to511Octets;
    uint64_t etherStatsTxPkts512to1023Octets;
    uint64_t etherStatsTxPkts1024to1518Octets;
    uint64_t etherStatsTxPkts1519toMaxOctets;
    uint64_t aCBFCPAUSEFramesTransmitted_0;
    uint64_t aCBFCPAUSEFramesTransmitted_1;
    uint64_t aCBFCPAUSEFramesTransmitted_2;
    uint64_t aCBFCPAUSEFramesTransmitted_3;
    uint64_t aCBFCPAUSEFramesTransmitted_4;
    uint64_t aCBFCPAUSEFramesTransmitted_5;
    uint64_t aCBFCPAUSEFramesTransmitted_6;
    uint64_t aCBFCPAUSEFramesTransmitted_7;
    uint64_t aMACControlFramesTransmitted;
    uint64_t etherStatsTxPkts;
} CredoMACStatistics_t;

/* Fault propagation control */
/**
 * @brief
 */
typedef enum {
    CR_FAULTPROP_NONE = 0,
    CR_FAULTPROP_LINE_TO_SYS = 1,
    CR_FAULTPROP_SYS_TO_LINE = 2,
    CR_FAULTPROP_BOTH = 3
} CredoFaultPropagation_t;

/* MAC register (32-bit) read/write. Address=mac_base_addr(port, side)+offset*4.
 * *** For status register (reg 16), must use mac_status_read() */

/**
 * @brief Reads any MAC register of specified port/side.
 *
 * See MAC register map for more information. For MAC status register (register 16),
 * use function mac_status_read() instead.
 *
 * @param[in] slice
 * @param[in] portId
 * @param[in] side
 * @param[in] offset
 * @param[out] val
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_mac_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                  unsigned* val);

/**
 * @brief Writes any MAC register of specified port/side.
 *
 * See MAC register map for more information.
 *
 * @param[in] slice
 * @param[in] portId
 * @param[in] side
 * @param[in] offset
 * @param[in] val
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_mac_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                   unsigned val);
/* MAC status read. Equivalent to read MAC offset 16 (address 0x40), but goes through firmware */

/**
 * @brief Reads MAC status register (MAC register 16) of specified port/side.
 *
 * Note the fault status (bit 0 and bit 1) are latch low.
 *
 * @param[in] slice
 * @param[in] portId
 * @param[in] side
 * @param[out] mac_status
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_mac_status_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned* mac_status);
/* MAC statistics register (32/64-bit) read/write. Address=mac_stat_base_addr(port, side)+offset*4.
 * For 64-bit registers, MSB are in register DATA_HI. */

/**
 * @brief Reads MAC statistics register for specified port/side.
 *
 * For 64-bit statistics, this returns 32 LSB only. The 32 MSBs can be read from register DATA_HI (MAC register 7) after
 * LSB is read.
 *
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Port side selection
 * @param[in] offset MAC statistics register offset
 * @param[out] val Holds value to be returned
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_mac_stats_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                        unsigned* val);
/**
 * @brief Writes MAC statistics register for specified port/side.
 *
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Port side selection
 * @param[in] offset MAC statistics register offset
 * @param[in] val Value to be written
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_mac_stats_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                         unsigned val);

/**
 * @brief Reads MAC statistics counters
 *
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Port side selection
 * @param[out] stats MAC statistics counters
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_mac_stats_read_counters(CredoSlice_t* slice, uint32_t portId, CredoSide_t side,
                                                 CredoMACStatistics_t* stats);

/**
 * @brief Clears MAC statistics counters
 *
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Port side selection
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_mac_stats_clear_counters(CredoSlice_t* slice, uint32_t portId, CredoSide_t side);

/* MAC and EIP stop function before port is stopped. */

/**
 * @brief Stops MAC before port is destroyed.
 * @param[in] slice
 * @param[in] portId
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_mac_stop(CredoSlice_t* slice, uint32_t portId);

/**
 * @brief Configures MAC when port is created.
 * @param[in] slice
 * @param[in] portId
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_mac_configure(CredoSlice_t* slice, uint32_t portId);
/* Per-slice topology control. Must change without running ports. */

/**
 * @brief Change the fault propagation control of specified port.
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] enable The desired fault propagation mode for this port
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_mac_set_faultprop(CredoSlice_t* slice, uint32_t portId, CredoFaultPropagation_t enable);

/**
 * @brief Get the current status of fault propagation control.
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[out] enable The current fault propagation mode for this port
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_mac_get_faultprop(CredoSlice_t* slice, uint32_t portId, CredoFaultPropagation_t* enable);

#ifdef __cplusplus
}
#endif

#endif
