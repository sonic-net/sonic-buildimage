#ifndef BE_SLICE_H
#define BE_SLICE_H

#include "stdbool.h"

#include "sdk.h"
#define LANES          16
#define BE_MAX_PORT    8
#define BE_MAX_VSENSOR 1
#define FEC_MAX_NUM    8

typedef struct BePortInfo {
    CredoPortConfig_t port_config;

    bool configured;
} BePortInfo_t;

typedef struct BeSlice {
    CredoSlice_t base;
    CredoLaneMode_t lane_mode[LANES];
    BePortInfo_t port_info[BE_MAX_PORT];
    bool in_loopback[LANES];
} BeSlice_t;

CredoError_t be_slice_init(CredoSlice_t* slice, const CredoSliceConfig_t* config);
CredoError_t be_slice_data_init(CredoSlice_t* slice);

CredoError_t be_allocate_slice(CredoSdk_t* sdk, CredoSlice_t** handle);

void be_free_slice(CredoSlice_t* handle);

CredoError_t be_slice_get_vsensor(CredoSlice_t* slice, double* vsensor);
CredoError_t be_slice_save_setup(CredoSlice_t* slice, const char* file_path);

#endif
