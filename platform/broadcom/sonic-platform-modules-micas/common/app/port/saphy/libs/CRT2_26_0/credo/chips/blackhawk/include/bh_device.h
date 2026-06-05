#ifndef BH_SLICE_H
#define BH_SLICE_H

#include "stdbool.h"

#include "sdk.h"
#define LANES          16
#define BH_MAX_PORT    8
#define BH_MAX_VSENSOR 1
#define FEC_MAX_NUM    8

typedef struct {
    CredoPortConfig_t port_config;

    bool configured;
} BhPortInfo_t;

typedef struct BhSlice {
    CredoSlice_t base;
    CredoLaneMode_t lane_mode[LANES];
    BhPortInfo_t port_info[BH_MAX_PORT];
    int em_flags[LANES];
    int tx_taps[LANES][7];
    unsigned fw_feature;
    bool fw_feature_captured;
} BhSlice_t;

CredoError_t bh_slice_init(CredoSlice_t* slice, const CredoSliceConfig_t* config);
CredoError_t bh_slice_data_init(CredoSlice_t* slice);

CredoError_t bh_allocate_slice(CredoSdk_t* sdk, CredoSlice_t** handle);

void bh_free_slice(CredoSlice_t* handle);

CredoError_t bh_slice_get_type(CredoSlice_t* slice, CredoSliceType_t* slice_type);
CredoError_t bh_slice_get_vsensor(CredoSlice_t* slice, double* vsensor);

CredoError_t bh_slice_save_setup(CredoSlice_t* slice, const char* file_path);
#endif
