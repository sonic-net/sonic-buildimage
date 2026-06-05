#ifndef TYPES_H
#define TYPES_H

#include "credo.h"

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LANE_PER_SLICE 128

/* driver types */

#define HIVENAME_SERDES          "serdes"
#define HIVENAME_SERDES_RX       "serdes_rx"
#define HIVENAME_SERDES_TX       "serdes_tx"
#define HIVENAME_SERDES_BSIDE    "serdes_bs"
#define HIVENAME_SERDES_RX_BSIDE "serdes_rx_bs"
#define HIVENAME_SERDES_TX_BSIDE "serdes_tx_bs"
#define HIVENAME_EFUSE           "efuse"
#define HIVENAME_TOPPLL          "toppll"
#define HIVENAME_TOP             "top"
#define HIVENAME_TOP_ANA         "top_ana"
#define HIVENAME_TOP_MACRO       "top_macro"
#define HIVENAME_PLL             "pll"
#define HIVENAME_PLL_TX          "pll_tx"
#define HIVENAME_FECANA          "fecana"
#define HIVENAME_TOPLANE         "toplane"
#define HIVENAME_TOPONELANE      "toponelane"
#define HIVENAME_TOPSENSOR       "topsensor"
#define HIVENAME_ECC             "ecc"
#define HIVENAME_FECA            "feca"
#define HIVENAME_FECB            "fecb"
#define HIVENAME_BITMUX          "bitmux"
#define HIVENAME_ANI             "ani"
#define HIVENAME_ANC             "anc"
#define HIVENAME_RSFEC           "rsfec"
#define HIVENAME_ENIGMA          "enigma"
#define HIVENAME_NOTEPAD         "notepad"
#define HIVENAME_EIP366EG        "EIP366Eg"
#define HIVENAME_EIP366IG        "EIP366Ig"
#define HIVENAME_EIP366SHELL     "EIP366Shell"
#define HIVENAME_EGRESS163       "Egress163"
#define HIVENAME_EGRESS164       "Egress164"
#define HIVENAME_INGRESS163      "Ingress163"
#define HIVENAME_INGRESS164      "Ingress164"

#define BASE_ADDRESS_EXTENDED 0xFF
#define MAX_BASE_EXTENDED     16
typedef struct RegHive {
    const char* hivename;
    unsigned base_addr[MAX_BASE_EXTENDED];
    int base_shift;
    int shift;
    int count;
} RegHive_t;

/* hal types */

typedef struct HalFuncTable HalFuncTable_t;

typedef CredoErrorCodes_t (*SliceSetOptionFunc_t)(CredoSlice_t* slice, const char* option_name, const int value);
typedef CredoErrorCodes_t (*SliceGetOptionFunc_t)(CredoSlice_t* slice, const char* option_name, int* value);
typedef struct SliceOption {
    const char* name;
    const char* description;
    const SliceSetOptionFunc_t set_func_ptr;
    const SliceGetOptionFunc_t get_func_ptr;
} SliceOption_t;

typedef CredoErrorCodes_t (*LaneSetOptionFunc_t)(CredoSlice_t* slice, int lane, const char* option_name,
                                                 const int value);
typedef CredoErrorCodes_t (*LaneGetOptionFunc_t)(CredoSlice_t* slice, int lane, const char* option_name, int* value);

// DO NOT MODIFY THESE structs

typedef struct LaneOption {
    const char* name;
    const char* description;
    const LaneSetOptionFunc_t set_func_ptr;
    const LaneGetOptionFunc_t get_func_ptr;
} LaneOption_t;

typedef struct SliceCapability {
    uint32_t slice_type;
    int port_count;
    int lane_count;
    int vsensor_count;
    const HalFuncTable_t* hal_func_table;
    int option_count;
    const SliceOption_t* option_list;
    int lane_option_count;
    const LaneOption_t* lane_option_list;
} SliceCapability_t;

typedef struct DeviceCapability {
    CredoDeviceType_t device_type;
    int slice_count;
    uint32_t lane_disable_mask[MAX_LANE_PER_SLICE / 32];
    const SliceCapability_t* slice_capability;
} DeviceCapability_t;

/* hal_func_table types */

// These must be kept static

typedef enum {
    TCMBurst16Bit = 0,
    TCMBurst32Bit = 1,
    TCMBurst48Bit = 2,
    TCMBurst64Bit = 3,
    TCMBurstUnknown = 0xFF
} TCMBurstWidth_t;

typedef enum { TCM_CORE_PCS, TCM_CORE_MAC } TCMCoreId_t;

typedef enum { PARAM_DOMAIN_SLICE, PARAM_DOMAIN_PORT, PARAM_DOMAIN_LANE, PARAM_DOMAIN_SERDES } ParamDomain_t;

typedef struct timespec CredoTime_t;

typedef enum { OPTION_DOMAIN_SLICE = 0, OPTION_DOMAIN_PORT = 1, OPTION_DOMAIN_LANE = 2 } OptionDomain_t;

#ifdef __cplusplus
}
#endif

#endif  // TYPES_H
