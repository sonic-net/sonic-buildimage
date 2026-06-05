#ifndef CRI_SERDES_H
#define CRI_SERDES_H

#ifdef __cplusplus
extern "C" {
#endif

CREDOAPI CredoError_t cri_serdes_reset_tx_taps(CredoSlice_t* slice, int lane);

#ifdef __cplusplus
}
#endif

#endif  // CRI_SERDES_H
