#include "project.h"

#include "rsfec/rsfec.h"

#include <stdlib.h>

CredoError_t rsfec_get_fec_align_status(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint32_t* fec_align) {
    int index = 0;
    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_FEC_ALIGN, fec_align));
    return CR_OK;
}

CredoError_t rsfec_get_pcs_align_status(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint32_t* pcs_align) {
    int index = 0;
    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_PCS_ALIGN, pcs_align));
    return CR_OK;
}

CredoError_t rsfec_get_align_status(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                    CredoRSFECStatus_t* rsfec_status) {
    int index = 0;
    unsigned pcs_align, fec_align, am_lock;

    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_PCS_ALIGN, &pcs_align));
    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_FEC_ALIGN, &fec_align));
    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_AM_LOCK, &am_lock));

    rsfec_status->pcs_aligned = pcs_align;
    rsfec_status->fec_aligned = fec_align;
    for (int i = 0; i < 4; i++) {
        rsfec_status->AM_locked[i] = (am_lock >> i) & 0x1;
    }

    return CR_OK;
}

CredoError_t rsfec_get_fifo(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, CredoRSFECFifo_t* rsfec_fifo) {
    int index = 0;
    unsigned rx_min, rx_cur, rx_max, tx_min, tx_cur, tx_max;

    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_TX_FIFO_MIN, &tx_min));
    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_TX_FIFO_CUR, &tx_cur));
    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_TX_FIFO_MAX, &tx_max));
    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_RX_FIFO_MIN, &rx_min));
    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_RX_FIFO_CUR, &rx_cur));
    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_RX_FIFO_MAX, &rx_max));

    rsfec_fifo->tx_min = tx_min;
    rsfec_fifo->tx_cur = tx_cur;
    rsfec_fifo->tx_max = tx_max;
    rsfec_fifo->rx_min = rx_min;
    rsfec_fifo->rx_cur = rx_cur;
    rsfec_fifo->rx_max = rx_max;

    return CR_OK;
}

CredoError_t rsfec_get_lane_mapping(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, unsigned* lane_mapping) {
    int index = 0;
    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));
    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_LANE_MAPPING, lane_mapping));
    return CR_OK;
}

CredoError_t rsfec_get_histogram(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int hist_bin,
                                 uint64_t* hist) {
    int index = 0;
    unsigned data[2] = {0};

    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    ERR_PROPS(writeRegLane(slice, index, REG_RSFEC_HIST_BIN, hist_bin));

    unsigned addr = cr_addr_reg(slice, index, RSFEC_FECA_HIVE, RSFEC_HIST_OFFSET);
    CredoError_t ret = hal_fw_atomic_read(slice, addr, 2, data);
    if (ret != CR_OK) {
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_HIST_LSB, data + 0));
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_HIST_MSB, data + 1));
    }

    *hist = (data[1] << 16) | data[0];

    if (hist_bin == 0) {
        ERR_PROPS(writeRegLane(slice, index, REG_RSFEC_HIST_BIN, 17));

        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_HIST_LSB, data));

        *hist |= ((uint64_t)data[0] << 32);
    }
    return CR_OK;
}

CredoError_t rsfec_get_corrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint64_t* corr_cw) {
    int index = 0;
    unsigned data[2] = {0};

    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    unsigned addr = cr_addr_reg(slice, index, RSFec, RSFEC_CORR_CW_OFFSET);
    CredoError_t ret = hal_fw_atomic_read(slice, addr, 2, data);
    if (ret != CR_OK) {
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_CORR_CW_LSB, data + 0));
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_CORR_CW_MSB, data + 1));
    }

    *corr_cw = (data[1] << 16) | data[0];
    return CR_OK;
}

CredoError_t rsfec_get_uncorrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                            unsigned* uncorr_cw) {
    int index = 0;
    unsigned data[2] = {0};

    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    unsigned addr = cr_addr_reg(slice, index, RSFec, RSFEC_UNCORR_CW_OFFSET);
    CredoError_t ret = hal_fw_atomic_read(slice, addr, 2, data);
    if (ret != CR_OK) {
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_UNCORR_CW_LSB, data + 0));
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_UNCORR_CW_MSB, data + 1));
    }

    *uncorr_cw = (data[1] << 16) | data[0];
    return CR_OK;
}

CredoError_t rsfec_get_symbol_error(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int fec_lane,
                                    unsigned* symbol_error) {
    int index = 0;
    unsigned data[2] = {0};

    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    unsigned addr = cr_addr_reg(slice, index, RSFec, RSFEC_SYMBOL_OFFSET(fec_lane));
    CredoError_t ret = hal_fw_atomic_read(slice, addr, 2, data);
    if (ret != CR_OK) {
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_SYMBOL_LSB(fec_lane), data + 0));
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_SYMBOL_MSB(fec_lane), data + 1));
    }

    *symbol_error = (data[1] << 16) | data[0];
    return CR_OK;
}

CredoError_t rsfec_get_total_codewords(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint64_t* total_bits) {
    int index = 0;
    unsigned data[3] = {0};

    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    unsigned addr = cr_addr_reg(slice, index, RSFEC_FECA_HIVE, RSFEC_TOTAL_BLK_CNTR_OFFSET);
    CredoError_t ret = hal_fw_atomic_read(slice, addr, 3, data);
    if (ret != CR_OK) {
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_TOTAL_BLK_CNTR_LOW, data + 0));
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_TOTAL_BLK_CNTR_MID, data + 1));
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_TOTAL_BLK_CNTR_HIGH, data + 2));
    }

    *total_bits = ((uint64_t)data[2] << 32) | (data[1] << 16) | data[0];
    return CR_OK;
}

CredoError_t rsfec_get_corrected_bits(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                      uint64_t* corrected_bits) {
    int index = 0;
    unsigned data[3] = {0};

    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    unsigned addr = cr_addr_reg(slice, index, RSFEC_FECA_HIVE, RSFEC_ERR_BIT_CNTR_OFFSET);
    CredoError_t ret = hal_fw_atomic_read(slice, addr, 3, data);
    if (ret != CR_OK) {
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_ERR_BIT_CNTR_LOW, data + 0));
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_ERR_BIT_CNTR_MID, data + 1));
        ERR_PROPS(readRegLane(slice, index, REG_RSFEC_ERR_BIT_CNTR_HIGH, data + 2));
    }

    *corrected_bits = ((uint64_t)data[2] << 32) | (data[1] << 16) | data[0];
    return CR_OK;
}

CredoError_t rsfec_set_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool enable) {
    int index = 0;
    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));
    ERR_PROPS(writeRegLane(slice, index, REG_RSFEC_CNT_FREEZE, enable));
    return CR_OK;
}

CredoError_t rsfec_get_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool* enable) {
    int index = 0;
    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));
    unsigned freeze;
    ERR_PROPS(readRegLane(slice, index, REG_RSFEC_CNT_FREEZE, &freeze));
    *enable = freeze;
    return CR_OK;
}

CredoError_t rsfec_reset_count(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side) {
    int index = 0;
    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    ERR_PROPS(writeRegLane(slice, index, REG_RSFEC_CNT_CLEAR, 0x1));
    ERR_PROPS(writeRegLane(slice, index, REG_RSFEC_CNT_CLEAR, 0x0));
    return CR_OK;
}

CredoError_t rsfec_get_buf_pointer(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, unsigned* in_buf,
                                   unsigned* out_buf) {
    int index = 0;
    ERR_PROPS(hal_get_rsfec_index(slice, port_id, side, &index));

    if (in_buf) ERR_PROPS(readRegLane(slice, index, REG_RSFEC_INBUF_CTRL, in_buf));
    if (out_buf) ERR_PROPS(readRegLane(slice, index, REG_RSFEC_OUTBUF_CTRL, out_buf));
    return CR_OK;
}
