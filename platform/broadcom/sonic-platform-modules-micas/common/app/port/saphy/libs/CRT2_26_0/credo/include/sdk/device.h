#ifndef DEVICE_H
#define DEVICE_H

#include "sdk/hal.h"
#include "sdk/types.h"

#include <pthread.h>
#include <stdbool.h>
/*
 * This data structure is used almost everywhere and is the core of the whole SDK.
 * The structure defined here is actually the base structure; it only contains the
 * core information that is useful to the DII layer. The HAL layer is supposed to
 * allocate a real structure and send it back. DII will use it as a base structure.
 * It is very similar to C++ base class and derived class. But we wanted to avoid
 * C++ interface for now. Maybe it will change in the future. For now, we will call
 * CredoSlice_t the "base class" and anything derived (e.g. OwlSlice_t) as "derived class".
 *
 * To make sure the pointer of derived class can be properly cast back/forth to base
 * class, the derived class it suggested to use the following declaration:
 *
 * typedef struct OwlSlice {
 *    CredoSlice_t base;            // if we use -fms-extension we will be able to omit "base"
 *    int owl_specific_field;
 * } OwlSlice_t;
 */
#define MAX_CREDO_SLICES 32
#define MAX_CREDO_LANES  32
// DO NOT MODIFY THIS STRUCT
struct CredoDevice {
    CredoSdk_t* sdk;
    const DeviceCapability_t* desc;
    uint32_t device_id;
    uint32_t flags;
    CredoSlice_t* slices[MAX_CREDO_SLICES];
    void* data;  // future-proof with the same proccess that data has for a slice. Only available in the HAL layer.
};

// enforce that Slice Data is only accessible in the hal layer
// this enables us to modify it at anytime without worrying about backwards compatibility new releases as the hal layer
// generates the struct
#ifdef BUILD_HAL
// a slice might not use everything in here, but it makes a lot of stuff easier if the slice is garunteed to have it
typedef struct CredoSliceData {
    unsigned base_offset;  // most likely used in IP product
    bool is_mdio_push_pull;
    CredoTime_t prbs_fec_timer[MAX_CREDO_LANES];  // prbs fec timer per lane
    CredoTime_t prbs_timer[MAX_CREDO_LANES];
    CredoTime_t tx_prbs_timer[MAX_CREDO_LANES];
    unsigned fw_cmd_timeout;
    unsigned fw_unload_timeout;
    unsigned top_cal_timeout;
    unsigned fw_spiflash_load_timeout;
    uint32_t ecid_raw[2];
    bool ecid_captured;
    int refclk_hz;
} CredoSliceData_t;

#else
typedef struct CredoSliceData CredoSliceData_t;
#endif

// DO NOT CHANGE struct
struct CredoSlice {
    CredoSdk_t* sdk;
    CredoDevice_t* device;
    bool in_shell;
    uint16_t revision;
    uint16_t sub_revision;
    const SliceCapability_t* desc;
    void* slice_context;
    uint32_t slice_id;
    uint32_t flags;
    pthread_mutex_t lock;
    CredoSliceData_t* data;
    const HalFuncTable_t* hal;  // short hand for hal haccess
};

#define IS_LANE_MODE_PAM4_OR_NRZ(lane_mode) (lane_mode == CR_LMODE_NRZ || lane_mode == CR_LMODE_PAM4)
#define IS_LANE_MODE_DISABLE(lane_mode)     (lane_mode == CR_LMODE_DISABLE)

#ifdef __cplusplus
extern "C" {
#endif

// lock and unlock functionality
CREDOAPI CredoError_t cr_slice_lock(CredoSlice_t* slice);
CREDOAPI void cr_slice_unlock(CredoSlice_t* slice);
#ifdef __cplusplus
}
#endif

#endif
