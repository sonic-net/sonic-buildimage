#ifndef COMMON_PRBS_H
#define COMMON_PRBS_H

#include "common/common_display.h"

#include "sdk.h"

CredoError_t common_get_rx_prbs_ber(CredoSlice_t* slice, int lane, int time_ms, double* ber);
CredoError_t common_get_tx_prbs_ber(CredoSlice_t* slice, int lane, unsigned time_ms, double ber[2]);
CredoError_t common_get_rx_prbs_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms);
CredoError_t common_get_tx_prbs_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms);
// use internally only
CredoError_t common_get_rx_prbs_ber_all(CredoSlice_t* slice, const int lanes[], unsigned time_ms, double ber[],
                                        unsigned count);
CredoError_t common_get_tx_prbs_ber_all(CredoSlice_t* slice, const int lanes[], unsigned time_ms, double ber[][2],
                                        unsigned count);
CredoError_t common_prbs_phase_error(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D);
#endif
