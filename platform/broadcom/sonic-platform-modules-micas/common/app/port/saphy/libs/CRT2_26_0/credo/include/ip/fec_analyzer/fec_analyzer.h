#ifndef FEC_ANALYZER_H
#define FEC_ANALYZER_H

#include "sdk.h"

/* FEC analyzer requires hive "FecAnalyzer" defined. */
extern const RegHive_t FecAnalyzer[];

#define FECANA HIVE(FecAnalyzer)

/* FEC analyzer */
#define REG_FECANA_SRC_SEL       REGBITR(FECANA, 0x00, 15, 6)
#define REG_FECANA_SYMBOL_MODE   REGBITR(FECANA, 0x00, 5, 4)
#define REG_FECANA_SYMBOL_SIZE   REGBITR(FECANA, 0x00, 3, 0)
#define REG_FECANA_CONTROL       REGBITR(FECANA, 0x01, 15, 0)
#define REG_FECANA_CNT_CLR       REGBITR(FECANA, 0x01, 3)
#define REG_FECANA_CLK_EN        REGBITR(FECANA, 0x01, 1)
#define REG_FECANA_EN            REGBITR(FECANA, 0x01, 0)
#define REG_FECANA_SETUP         REGBITR(FECANA, 0x09, 15, 0)
#define REG_FECANA_PRBS_MODE     REGBITR(FECANA, 0x08, 4, 3)
#define REG_FECANA_CODEWORD_SIZE REGBITR(FECANA, 0x04, 12, 0)
#define REG_FECANA_RESET_HIS     REGBITR(FECANA, 0x05, 12)
#define REG_FECANA_THRES1        REGBITR(FECANA, 0x05, 11, 7)
#define REG_FECANA_THRES2        REGBITR(FECANA, 0x05, 6, 3)
#define REG_FECANA_N_HIS         REGBITR(FECANA, 0x05, 2, 0)
#define REG_FECANA_TEI_TYPE      REGBITR(FECANA, 0x0B, 1, 0)
#define REG_FECANA_TEO_TYPE      REGBITR(FECANA, 0x0C, 1, 0)
#define REG_FECANA_READ_SEL      REGBITR(FECANA, 0x0D, 15, 0)
#define REG_FECANA_READ_DATA     REGBITR(FECANA, 0x07, 15, 0)
#define REG_FECANA_AUTO_SYNC_EN  REGBITR(FECANA, 0x09, 13)

#define FECANA_SETUP_START1  0x2202
#define FECANA_SETUP_START2  0x6202
#define FECANA_CONTROL_CLEAR 0x000B
#define FECANA_CONTROL_RUN   0x0003
#define FECANA_CONTROL_OFF   0x0000

// FEC analyzer counter. All counters are 32-bit and take two slots each.
#define FECANA_COUNTER_SEI     0
#define FECANA_COUNTER_BEI     2
#define FECANA_COUNTER_TEI     4
#define FECANA_COUNTER_TEO     6
#define FECANA_COUNTER_HIST(I) ((I)*2 + 12)

#if HAL_SUPPORT_FEC_ANA
CredoError_t fec_analyzer_set_mux(CredoSlice_t* slice, int lane, int input_src);
CredoError_t fec_analyzer_get_mux(CredoSlice_t* slice, int lane, int* input_src);
CredoError_t fec_analyzer_set_config(CredoSlice_t* slice, int lane, int enable, CredoFecAnalyzerConfig_t* config);
CredoError_t fec_analyzer_get_config(CredoSlice_t* slice, int lane, int* enable, CredoFecAnalyzerConfig_t* config);
CredoError_t fec_analyzer_reset(CredoSlice_t* slice, int lane);
CredoError_t fec_analyzer_get_read_counter(CredoSlice_t* slice, int lane, int counter_sel, unsigned* counter);
CredoError_t fec_analyzer_get_counter(CredoSlice_t* slice, int lane, unsigned* pre_fec, unsigned* post_fec);
CredoError_t fec_analyzer_set_hist_group(CredoSlice_t* slice, int lane, int group);
CredoError_t fec_analyzer_get_hist_group(CredoSlice_t* slice, int lane, int* group);
CredoError_t fec_analyzer_get_hist_counter(CredoSlice_t* slice, int lane, unsigned hist_data[4]);
CredoError_t fec_analyzer_get_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms);
CredoError_t fec_analyzer_get_error_rate(CredoSlice_t* slice, int lane, int counter_sel, int duration_ms,
                                         double* error_rate);
CredoError_t fec_analyzer_get_autosync(CredoSlice_t* slice, int lane, bool* enable);

// utility function
CredoError_t fec_analyzer_calculate_error_rate(CredoSlice_t* slice, int lane, int counter_sel,
                                               unsigned long duration_ms, double* error_rate, unsigned start_count,
                                               unsigned end_count);
#endif

#endif
