#include "dii.h"

#include "sdk.h"

CredoError_t cr_rsfec_get_align_status(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                       CredoRSFECStatus_t* rsfec_status) {
    LOGS_API("port_id %d, side %d", port_id, side);
    CALL_HAL(slice, hal_get_rsfec_align_status(slice, port_id, side, rsfec_status));
}

CredoError_t cr_rsfec_get_fifo(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, CredoRSFECFifo_t* rsfec_fifo) {
    LOGS_API("port_id %d, side %d", port_id, side);
    CALL_HAL(slice, hal_get_rsfec_fifo(slice, port_id, side, rsfec_fifo));
}

CredoError_t cr_rsfec_get_lane_mapping(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                       unsigned* lane_mapping) {
    LOGS_API("port_id %d, side %d", port_id, side);
    CALL_HAL(slice, hal_get_rsfec_lane_mapping(slice, port_id, side, lane_mapping));
}

CredoError_t cr_rsfec_get_histogram(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int hist_bin,
                                    uint64_t* hist) {
    LOGS_API("port_id %d, side %d, hist_bin %d", port_id, side, hist_bin);
    CALL_HAL(slice, hal_get_rsfec_histogram(slice, port_id, side, hist_bin, hist));
}

CredoError_t cr_rsfec_get_corrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                             uint64_t* corr_cw) {
    LOGS_API("port_id %d, side %d", port_id, side);
    CALL_HAL(slice, hal_get_rsfec_corrected_codeword(slice, port_id, side, corr_cw));
}

CredoError_t cr_rsfec_get_uncorrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                               unsigned* uncorr_cw) {
    LOGS_API("port_id %d, side %d", port_id, side);
    CALL_HAL(slice, hal_get_rsfec_uncorrected_codeword(slice, port_id, side, uncorr_cw));
}

CredoError_t cr_rsfec_get_symobl_error(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int fec_lane,
                                       unsigned* symbol_error) {
    LOGS_API("port_id %d, side %d", port_id, side);
    CALL_HAL(slice, hal_get_rsfec_symbol_error(slice, port_id, side, fec_lane, symbol_error));
}

CredoError_t cr_rsfec_get_symbol_error(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int fec_lane,
                                       unsigned* symbol_error) {
    LOGS_API("port_id %d, side %d", port_id, side);
    CALL_HAL(slice, hal_get_rsfec_symbol_error(slice, port_id, side, fec_lane, symbol_error));
}

CredoError_t cr_rsfec_get_total_bits(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint64_t* total_bits) {
    (void)(slice);
    (void)(port_id);
    (void)(side);
    (void)(total_bits);
    return CR_FAIL;
}

CredoError_t cr_rsfec_get_total_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint64_t* total_cw) {
    LOGS_API("port_id %d, side %d", port_id, side);
    CALL_HAL(slice, hal_get_rsfec_total_codeword(slice, port_id, side, total_cw));
}

CredoError_t cr_rsfec_get_corrected_bits(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                         uint64_t* corrected_bits) {
    LOGS_API("port_id %d, side %d", port_id, side);
    CALL_HAL(slice, hal_get_rsfec_corrected_bits(slice, port_id, side, corrected_bits));
}

CredoError_t cr_rsfec_set_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool enable) {
    LOGS_API("port_id %d, side %d, enable %d", port_id, side, enable);
    CALL_HAL(slice, hal_set_rsfec_count_freeze(slice, port_id, side, enable));
}

CredoError_t cr_rsfec_get_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool* enable) {
    LOGS_API("port_id %d, side %d", port_id, side);
    CALL_HAL(slice, hal_get_rsfec_count_freeze(slice, port_id, side, enable));
}

CredoError_t cr_rsfec_reset_count(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side) {
    LOGS_API("port_id %d, side %d", port_id, side);
    CALL_HAL(slice, hal_reset_rsfec_count(slice, port_id, side));
}
