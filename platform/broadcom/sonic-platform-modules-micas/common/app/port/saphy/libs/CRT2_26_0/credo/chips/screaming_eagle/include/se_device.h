#ifndef SE_SLICE_H
#define SE_SLICE_H

#include "stdbool.h"

#include "sdk.h"
#define SE_MAX_LANES   16
#define SE_MAX_PORT    16
#define SE_MAX_VSENSOR 1

typedef struct {
    bool enable;
    bool an_agent;
    uint32_t lane_map_a;
    uint32_t lane_map_b;
    int host_main_lane;
    int line_main_lane;
} SeISCInfo_t;

typedef struct SePortInfo {
    CredoPortSetup_t setup;
    // port options
    uint32_t direction;
    int host_main_lane;
    int line_main_lane;
    bool line_an;
    bool an_override;
    bool line_lt;
    bool host_lt;
    bool is_50g_nrz_mode;
    unsigned active_lane_map;
    unsigned switchover_select;
    SeISCInfo_t isc;
    int flexspeed_kbps;
    // status
    bool built;
    bool started;
} SePortInfo_t;

typedef struct {
    int flags;
    int phase_base;
} SeEyeMonitorInfo_t;

typedef enum {
    UNI_PORT_NONE = 0,
    UNI_PORT_REIMER = 1,
    UNI_PORT_BITMUX_MERGE = 2,
    UNI_PORT_BITMUX_SPLIT = 3
} SeUniPortType_t;

typedef struct SeSlice {
    CredoSlice_t base;
    CredoLaneMode_t lane_mode[SE_MAX_LANES];
    CredoLaneMode_t tx_lane_mode[SE_MAX_LANES];
    int uni_tx_lane_map[SE_MAX_LANES];  // uni_tx_lane_map[tx_lane] = rx_source_lane
    int uni_rx_lane_map[SE_MAX_LANES];  // uni_rx_lane_map[rx_lane] = tx_dest_lane
    SeUniPortType_t uni_tx_port_map[SE_MAX_LANES];
    SeUniPortType_t uni_rx_port_map[SE_MAX_LANES];
    SePortInfo_t port_info[SE_MAX_PORT];  // started port info
    SeEyeMonitorInfo_t em_info[SE_MAX_LANES];
    int tx_taps[SE_MAX_LANES][7];
    int em_vstep_side;
    unsigned fw_isi_timeout;
    uint32_t isc_slice_id;
    CredoTestPoint_t testpoint;
    bool testpoint_cleared;
    bool testpoint_configured;
    uint8_t vsensor_resolution;
    double start_time;
} SeSlice_t;

typedef struct {
    double baudrate;
    double vcorate;
    unsigned pll_n;
    unsigned pll_n_frac;
    unsigned tx_target_count;
    unsigned rx_target_count;
    unsigned fw_base_speed;  // we will give it the nearest speed
} flexspeed_config_t;

CredoError_t se_slice_init(CredoSlice_t* slice, const CredoSliceConfig_t* config);
CredoError_t se_slice_data_init(CredoSlice_t* slice);

CredoError_t se_allocate_slice(CredoSdk_t* sdk, CredoSlice_t** handle);

void se_free_slice(CredoSlice_t* handle);

CredoError_t se_slice_get_vsensor(CredoSlice_t* slice, double* vsensor);

CredoError_t se_slice_save_setup(CredoSlice_t* slice, const char* file_path);

#endif
