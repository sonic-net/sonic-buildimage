#ifndef RSFEC_H
#define RSFEC_H

#include "project.h"

/* RS-FEC requires hive "RSFec" defined. */
extern const RegHive_t RSFec[], FecA[];

#define RSFEC HIVE(RSFec)
#define FECA  HIVE(FecA)

/* RS-FEC */
#define REG_RSFEC_PCS_ALIGN     REGBITR(RSFEC, 0x0C9, 15)
#define REG_RSFEC_FEC_ALIGN     REGBITR(RSFEC, 0x0C9, 14)
#define REG_RSFEC_AM_LOCK       REGBITR(RSFEC, 0x0C9, 11, 8)
#define REG_RSFEC_CORR_CW_LSB   REGBITR(RSFEC, 0x0CA, 15, 0)
#define REG_RSFEC_CORR_CW_MSB   REGBITR(RSFEC, 0x0CB, 15, 0)
#define REG_RSFEC_UNCORR_CW_LSB REGBITR(RSFEC, 0x0CC, 15, 0)
#define REG_RSFEC_UNCORR_CW_MSB REGBITR(RSFEC, 0x0CD, 15, 0)
#define REG_RSFEC_LANE_MAPPING  REGBITR(RSFEC, 0x0CE, 7, 0)
#define REG_RSFEC_SYMBOL_LSB(N) REGBITR(RSFEC, 0x0D2 + (N * 2), 15, 0)
#define REG_RSFEC_SYMBOL_MSB(N) REGBITR(RSFEC, 0x0D3 + (N * 2), 15, 0)

// We should have used different hives (RSFEC + FECA) for this, but doing the simple workaround that doesnt require
// changing a bunch of other chips
#ifndef RSFEC_USE_FECA

#define RSFEC_FECA_HIVE RSFec

// read regiseters low->mid->high
/**
Low counter should be read first, and then followed by the read of mid/high counter immediately.

Let’s take the total codeword counter to elaboration. In the design it has a 48-bit counter and three related registers
(0x485d~0x485f) Normally, the counter value will be loaded into the three registers whenever it is incremented. Once you
read the low reg 0x485d, these three registers will be locked. They will not be updated with the new value of the
counter until another two reads happen. This leaves you chance to read out the high counter from register 0x485d and
0x485f. So please make sure reading these three registers is an atomic operation and read from low to high.

Reading 0x485d also clears the counter. But since the three registers are locked, you can still read out the previous
value instead of a bunch zero.
 */
#define REG_RSFEC_TOTAL_CW_CNTR_HIGH  REGBITR(RSFEC, 0x84D, 15, 0)  // tx cw counter
#define REG_RSFEC_TOTAL_CW_CNTR_MID   REGBITR(RSFEC, 0x84E, 15, 0)
#define REG_RSFEC_TOTAL_CW_CNTR_LOW   REGBITR(RSFEC, 0x84F, 15, 0)
#define REG_RSFEC_DIS_CW_BAD_S        REGBITR(RSFEC, 0x850, 12)
#define REG_RSFEC_TOTAL_BLK_CNTR_LOW  REGBITR(RSFEC, 0x85D, 15, 0)  // rx cw counter
#define REG_RSFEC_TOTAL_BLK_CNTR_MID  REGBITR(RSFEC, 0x85E, 15, 0)
#define REG_RSFEC_TOTAL_BLK_CNTR_HIGH REGBITR(RSFEC, 0x85F, 15, 0)
#define REG_RSFEC_ERR_BIT_CNTR_LOW    REGBITR(RSFEC, 0x87D, 15, 0)
#define REG_RSFEC_ERR_BIT_CNTR_MID    REGBITR(RSFEC, 0x87E, 15, 0)
#define REG_RSFEC_ERR_BIT_CNTR_HIGH   REGBITR(RSFEC, 0x87F, 15, 0)

#define REG_RSFEC_INBUF_CTRL  REGBITR(RSFEC, 0x840, 12, 11)
#define REG_RSFEC_OUTBUF_CTRL REGBITR(RSFEC, 0x850, 11, 10)
#define REG_RSFEC_HIST_BIN    REGBITR(RSFEC, 0x860, 4, 0)
#define REG_RSFEC_HIST_LSB    REGBITR(RSFEC, 0x861, 15, 0)
#define REG_RSFEC_HIST_MSB    REGBITR(RSFEC, 0x862, 15, 0)
#define REG_RSFEC_CNT_FREEZE  REGBITR(RSFEC, 0x863, 1)
#define REG_RSFEC_CNT_CLEAR   REGBITR(RSFEC, 0x863, 0)
#define REG_RSFEC_TX_FIFO_CUR REGBITR(RSFEC, 0x89C, 10, 0)
#define REG_RSFEC_TX_FIFO_MIN REGBITR(RSFEC, 0x89D, 10, 0)
#define REG_RSFEC_TX_FIFO_MAX REGBITR(RSFEC, 0x89E, 10, 0)
#define REG_RSFEC_RX_FIFO_CUR REGBITR(RSFEC, 0x89F, 10, 0)
#define REG_RSFEC_RX_FIFO_MIN REGBITR(RSFEC, 0x8A0, 10, 0)
#define REG_RSFEC_RX_FIFO_MAX REGBITR(RSFEC, 0x8A1, 10, 0)

#define RSFEC_CORR_CW_OFFSET        0x0CA
#define RSFEC_UNCORR_CW_OFFSET      0x0CC
#define RSFEC_SYMBOL_OFFSET(N)      (0x0D2 + (N * 2))
#define RSFEC_TOTAL_CW_CNTR_OFFSET  0x84D
#define RSFEC_TOTAL_BLK_CNTR_OFFSET 0x85D
#define RSFEC_ERR_BIT_CNTR_OFFSET   0x87D
#define RSFEC_HIST_OFFSET           0x861
#else

#define RSFEC_FECA_HIVE FecA

#define REG_RSFEC_DIS_CW_BAD_S        REGBITR(FECA, 0x50, 12)
#define REG_RSFEC_TOTAL_BLK_CNTR_LOW  REGBITR(FECA, 0x5D, 15, 0)
#define REG_RSFEC_TOTAL_BLK_CNTR_MID  REGBITR(FECA, 0x5E, 15, 0)
#define REG_RSFEC_TOTAL_BLK_CNTR_HIGH REGBITR(FECA, 0x5F, 15, 0)
#define REG_RSFEC_ERR_BIT_CNTR_LOW    REGBITR(FECA, 0x7D, 15, 0)
#define REG_RSFEC_ERR_BIT_CNTR_MID    REGBITR(FECA, 0x7E, 15, 0)
#define REG_RSFEC_ERR_BIT_CNTR_HIGH   REGBITR(FECA, 0x7F, 15, 0)

#define REG_RSFEC_INBUF_CTRL  REGBITR(FECA, 0x40, 12, 11)
#define REG_RSFEC_OUTBUF_CTRL REGBITR(FECA, 0x50, 11, 10)
#define REG_RSFEC_HIST_BIN    REGBITR(FECA, 0x60, 4, 0)
#define REG_RSFEC_HIST_LSB    REGBITR(FECA, 0x61, 15, 0)
#define REG_RSFEC_HIST_MSB    REGBITR(FECA, 0x62, 15, 0)
#define REG_RSFEC_CNT_FREEZE  REGBITR(FECA, 0x63, 1)
#define REG_RSFEC_CNT_CLEAR   REGBITR(FECA, 0x63, 0)
#define REG_RSFEC_TX_FIFO_CUR REGBITR(FECA, 0x9C, 10, 0)
#define REG_RSFEC_TX_FIFO_MIN REGBITR(FECA, 0x9D, 10, 0)
#define REG_RSFEC_TX_FIFO_MAX REGBITR(FECA, 0x9E, 10, 0)
#define REG_RSFEC_RX_FIFO_CUR REGBITR(FECA, 0x9F, 10, 0)
#define REG_RSFEC_RX_FIFO_MIN REGBITR(FECA, 0xA0, 10, 0)
#define REG_RSFEC_RX_FIFO_MAX REGBITR(FECA, 0xA1, 10, 0)

#define RSFEC_CORR_CW_OFFSET        0x0CA
#define RSFEC_UNCORR_CW_OFFSET      0x0CC
#define RSFEC_SYMBOL_OFFSET(N)      (0x0D2 + (N * 2))
#define RSFEC_TOTAL_BLK_CNTR_OFFSET 0x5D
#define RSFEC_ERR_BIT_CNTR_OFFSET   0x7D
#define RSFEC_HIST_OFFSET           0x61
#endif

CredoError_t rsfec_get_fec_align_status(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint32_t* fec_align);
CredoError_t rsfec_get_pcs_align_status(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint32_t* pcs_align);
CredoError_t rsfec_get_align_status(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                    CredoRSFECStatus_t* rsfec_status);
CredoError_t rsfec_get_fifo(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, CredoRSFECFifo_t* rsfec_fifo);
CredoError_t rsfec_get_lane_mapping(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, unsigned* lane_mapping);
CredoError_t rsfec_get_histogram(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int hist_bin, uint64_t* hist);
CredoError_t rsfec_get_corrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint64_t* corr_cw);
CredoError_t rsfec_get_uncorrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                            unsigned* uncorr_cw);
CredoError_t rsfec_get_symbol_error(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int fec_lane,
                                    unsigned* symbol_error);
CredoError_t rsfec_get_total_codewords(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint64_t* total_bits);
CredoError_t rsfec_get_corrected_bits(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                      uint64_t* corrected_bits);
CredoError_t rsfec_set_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool enable);
CredoError_t rsfec_get_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool* enable);
CredoError_t rsfec_reset_count(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side);
CredoError_t rsfec_get_buf_pointer(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, unsigned* in_buf,
                                   unsigned* out_buf);

#endif
