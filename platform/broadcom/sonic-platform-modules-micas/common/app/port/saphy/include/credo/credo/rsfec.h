#ifndef CREDO_RSFEC_H
#define CREDO_RSFEC_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fec type of the port
 */
typedef enum {
    CR_FEC_NONE,  //!< NO FEC
    CR_FEC_FIRE_CODE,
    CR_FEC_RS_528,
    CR_FEC_RS_544
} CredoFecType_t;

/**
 * @brief PortRsfec fifo status
 */
typedef struct {
    uint32_t tx_min;
    uint32_t tx_cur;
    uint32_t tx_max;
    uint32_t rx_min;
    uint32_t rx_cur;
    uint32_t rx_max;
} CredoRSFECFifo_t;

/**
 * @brief PortRsfec status
 */
typedef struct {
    bool pcs_aligned;   //!< FEC encoder lock and align status
    bool fec_aligned;   //!< RS-FEC receive lanes lock and align status
    bool AM_locked[4];  //!< RS-FEC receivce lane[0-4] lock and align status
} CredoRSFECStatus_t;

/**
 * @brief Get RS-FEC align status
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @param[out] rsfec_status
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_get_align_status(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                CredoRSFECStatus_t* rsfec_status);

/**
 * @brief Get RS-FEC fifo status
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @param[out] rsfec_fifo
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_get_fifo(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                        CredoRSFECFifo_t* rsfec_fifo);

/**
 * @brief Get RS-FEC lane mapping
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @param[out] lane_mapping [7:6] mapped to PMA lane 3
 *                     [5:4] mapped to PMA lane 2
 *                     [3:2] mapped to PMA lane 1
 *                     [1:0] mapped to PMA lane 0
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_get_lane_mapping(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                unsigned* lane_mapping);

/**
 * @brief Get RS-FEC histogram for given hist bin
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @param[in] hist_bin
 * @param[out] hist
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_get_histogram(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int hist_bin,
                                             uint64_t* hist);

/**
 * @brief Get RS-FEC corrected codewords count
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @param[out] corr_cw
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_get_corrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                      uint64_t* corr_cw);

/**
 * @brief Get RS-FEC uncorrected codewords count
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @param[out] uncorr_cw
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_get_uncorrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                        unsigned* uncorr_cw);

// NOTE: keep for backwards compatibility since the typo func may be in use
CREDOAPI CredoError_t cr_rsfec_get_symobl_error(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int fec_lane,
                                                unsigned* symbol_error);

/**
 * @brief Get RS-FEC symbol error for each lane
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @param[in] fec_lane per-lane index
 * @param[out] symbol_error
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_get_symbol_error(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int fec_lane,
                                                unsigned* symbol_error);

/**
 * @brief Get RS-FEC total bits
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @param[out] total_bits
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_get_total_bits(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                              uint64_t* total_bits);

/**
 * @brief Get RS-FEC total codewords received.
 * @param slice
 * @param port_id
 * @param side
 * @param total_cw
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_get_total_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                  uint64_t* total_cw);

/**
 * @brief Get RS-FEC corrected bits
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @param[out] corrected_bits
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_get_corrected_bits(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                  uint64_t* corrected_bits);

/**
 * @brief Set RS-FEC count freeze state
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @param[in] enable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_set_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool enable);

/**
 * @brief Get RS-FEC count freeze state
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @param[out] enable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_get_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool* enable);

/**
 * @brief Reset RS-FEC count
 * @param[in] slice slice handle
 * @param[in] port_id
 * @param[in] side
 * @return Error Code
 */
CREDOAPI CredoError_t cr_rsfec_reset_count(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side);

#ifdef __cplusplus
}
#endif

#endif
