#ifndef COMMON_PARAMS_H
#define COMMON_PARAMS_H

#include "sdk.h"

#define PARAM_FLAG_NONE 0

#define SP_FW_CMD_TIMEOUT CR_SLCPARAM_FW_CMD_TIMEOUT, "default is 10000 us, set 0 to default.", CR_PARAM_TYPE_OPTION
#define SP_FW_UNLOAD_TIMEOUT \
    CR_SLCPARAM_FW_UNLOAD_TIMEOUT, "default is 100000 us, set 0 to default.", CR_PARAM_TYPE_OPTION
#define SP_FW_UP_TIME      CR_SLCPARAM_FW_UP_TIME, "firmware up time, seconds", CR_PARAM_TYPE_PARAM
#define SP_TOP_CAL_TIMEOUT CR_SLCPARAM_TOP_CAL_TIMEOUT, "default is 150000 us, set 0 to default.", CR_PARAM_TYPE_OPTION
#define SP_SLICE_ID        CR_SLCPARAM_SLICE_ID, "slice_id", CR_PARAM_TYPE_OPTION
#define SP_REFCLK          CR_SLCPARAM_REFCLK, "set 0 to default.", CR_PARAM_TYPE_OPTION
#define SP_FFCLK           CR_SLCPARAM_FFCLK, "feed forward clock enable.", CR_PARAM_TYPE_OPTION
#define SP_PAM4_CHNL       CR_SLCPARAM_PAM4_CHNL, "channel length.", CR_PARAM_TYPE_OPTION
#define SP_TOP_PLL_CAP     CR_SPARAM_TOP_PLL_CAP, "Top level pll cap", CR_PARAM_TYPE_PARAM
#define SP_RX_REFCLK_SEL   CR_SLCPARAM_RX_REFCLK_SEL, "rx_refclk_sel.", CR_PARAM_TYPE_OPTION
#define SP_TX_REFCLK_SEL   CR_SLCPARAM_TX_REFCLK_SEL, "tx_refclk_sel.", CR_PARAM_TYPE_OPTION
#define SP_REFCLK_B2I      CR_SLCPARAM_REFCLK_B2I, "refclk bottom to internal.", CR_PARAM_TYPE_OPTION

#define SP_PORT_LINK           CR_PPARAM_LINK, "Port link status, [host link, line link]", CR_PARAM_TYPE_PARAM
#define SP_PORT_LINK_HOST      CR_PPARAM_LINK_HOST, "Port host link status", CR_PARAM_TYPE_PARAM
#define SP_PORT_LINK_LINE      CR_PPARAM_LINK_LINE, "Port line link status", CR_PARAM_TYPE_PARAM
#define SP_PORT_HOST_MAC       CR_PPARAM_HOST_MAC, "Refer to {c:enum}`CredoTCMDevice_t` for values.", CR_PARAM_TYPE_STATUS
#define SP_PORT_LINE_MAC       CR_PPARAM_LINE_MAC, "Refer to {c:enum}`CredoTCMDevice_t` for values.", CR_PARAM_TYPE_STATUS
#define SP_PORT_HOST_PCS       CR_PPARAM_HOST_PCS, "Refer to {c:enum}`CredoTCMDevice_t` for values.", CR_PARAM_TYPE_STATUS
#define SP_PORT_LINE_PCS       CR_PPARAM_LINE_PCS, "Refer to {c:enum}`CredoTCMDevice_t` for values.", CR_PARAM_TYPE_STATUS
#define SP_PORT_HOST_REG_SPACE CR_PPARAM_HOST_REG_SPACE, "Refer to {c:enum}`CredoTCMRegSpace_t`", CR_PARAM_TYPE_STATUS
#define SP_PORT_LINE_REG_SPACE CR_PPARAM_LINE_REG_SPACE, "Refer to {c:enum}`CredoTCMRegSpace_t`", CR_PARAM_TYPE_STATUS
#define SP_PORT_BITMUX_FIFO    CR_PPARAM_BITMUX_FIFO, "Individual lane bitmux fifo", CR_PARAM_TYPE_PARAM
#define SP_PORT_RETIMER_FIFO   CR_PPARAM_RETIMER_FIFO, "Individual lane retimer fifo", CR_PARAM_TYPE_PARAM

#define SP_UP_TIME           CR_SPARAM_UP_TIME, "lane up time from last link up event", CR_PARAM_TYPE_STATUS
#define SP_DOWN_TIME         CR_SPARAM_DOWN_TIME, "lane down time from last link down event", CR_PARAM_TYPE_STATUS
#define SP_LMS_CLOCK         CR_SPARAM_LMS_CLOCK, "lms clock for DSP module, clock for dfe_f1 and ffe", CR_PARAM_TYPE_OPTION
#define SP_RX_ADAPT          CR_SPARAM_RX_ADAPT, "RX firmware Adapt Count.", CR_PARAM_TYPE_STATUS
#define SP_RX_AGCGAIN        CR_SPARAM_RX_AGCGAIN, "RX AGC gain", CR_PARAM_TYPE_PARAM
#define SP_RX_AGCATTEN       CR_SPARAM_RX_AGCATTEN, "RX AGC atten", CR_PARAM_TYPE_PARAM
#define SP_RX_ATTEN_GAIN     CR_SPARAM_RX_ATTEN_GAIN, "RX Attenuator Gain", CR_PARAM_TYPE_PARAM
#define SP_RX_ATTEN_PASSIVE  CR_SPARAM_RX_ATTEN_PASSIVE, "RX Attenuator Passive", CR_PARAM_TYPE_PARAM
#define SP_RX_ATTEN_TERMTUNE CR_SPARAM_RX_ATTEN_TERMTUNE, "RX Attenuator Termtune", CR_PARAM_TYPE_PARAM
#define SP_RX_READY          CR_SPARAM_RX_READY, "RX CDR lock also known as lane ready", CR_PARAM_TYPE_PARAM
#define SP_RX_CHANNEL_EST                                                                                \
    CR_SPARAM_RX_CHANNEL_EST, "RX firmware Channel Estimate. Provides an estimate of the Channel Loss.", \
        CR_PARAM_TYPE_STATUS
#define SP_RX_CHANNEL_HF       CR_SPARAM_RX_CHANNEL_HF, "RX firmware Channel High Frequency Counter", CR_PARAM_TYPE_STATUS
#define SP_RX_CHANNEL_OF       CR_SPARAM_RX_CHANNEL_OF, "RX firmware Channel Overflow Counter", CR_PARAM_TYPE_STATUS
#define SP_RX_KF               CR_SPARAM_RX_KF, "RX K factor for generating freq_accu", CR_PARAM_TYPE_PARAM
#define SP_RX_KP               CR_SPARAM_RX_KP, "RX K factor for generating phase0", CR_PARAM_TYPE_PARAM
#define SP_RX_ENVELOPE         CR_SPARAM_RX_ENVELOPE, "RX Envelope", CR_PARAM_TYPE_PARAM
#define SP_RX_CTLE             CR_SPARAM_RX_CTLE, "RX CTLE", CR_PARAM_TYPE_PARAM
#define SP_RX_CTLE_INDEX       CR_SPARAM_RX_CTLE_INDEX, "RX CTLE table index", CR_PARAM_TYPE_PARAM
#define SP_RX_TIA1_BIAS        CR_SPARAM_RX_TIA1_BIAS, "RX CTLE1 TIA stage bias current", CR_PARAM_TYPE_PARAM
#define SP_RX_DAC              CR_SPARAM_RX_DAC, "RX DAC", CR_PARAM_TYPE_PARAM
#define SP_RX_DELTA            CR_SPARAM_RX_DELTA, "RX delta", CR_PARAM_TYPE_PARAM
#define SP_RX_THS              CR_SPARAM_RX_THS, "RX threshold", CR_PARAM_TYPE_PARAM
#define SP_RX_DC_SAR           CR_SPARAM_RX_DC_SAR, "RX dc sar", CR_PARAM_TYPE_PARAM
#define SP_RX_GAIN_SAR         CR_SPARAM_RX_GAIN_SAR, "RX gain sar", CR_PARAM_TYPE_PARAM
#define SP_RX_DFE_NL_MODE      CR_SPARAM_RX_DFE_NL_MODE, "RX dfe nonlinear mode", CR_PARAM_TYPE_PARAM
#define SP_RX_DFE              CR_SPARAM_RX_DFE, "RX dfe", CR_PARAM_TYPE_PARAM
#define SP_RX_DFE_F0           CR_SPARAM_RX_DFE_F0, "RX dfe f0", CR_PARAM_TYPE_PARAM
#define SP_RX_DFE_F1           CR_SPARAM_RX_DFE_F1, "RX dfe f1", CR_PARAM_TYPE_PARAM
#define SP_RX_DFE_ALL          CR_SPARAM_RX_DFE_ALL, "RX dfe for all phase", CR_PARAM_TYPE_PARAM
#define SP_RX_VGA              CR_SPARAM_RX_VGA, "RX vga coe", CR_PARAM_TYPE_PARAM
#define SP_RX_EDGE             CR_SPARAM_RX_EDGE, "RX edge", CR_PARAM_TYPE_PARAM
#define SP_RX_EYE_HEIGHT       CR_SPARAM_RX_EYE_HEIGHT, "RX averaged eye", CR_PARAM_TYPE_PARAM
#define SP_RX_EYE_ALL          CR_SPARAM_RX_EYE_ALL, "RX all eye for all phase", CR_PARAM_TYPE_PARAM
#define SP_RX_EYE              CR_SPARAM_RX_EYE, "RX eye, if mutiple phases case, it means phase 0", CR_PARAM_TYPE_PARAM
#define SP_RX_F1OVER3          CR_SPARAM_RX_F1OVER3, "RX f1/3", CR_PARAM_TYPE_PARAM
#define SP_RX_FFE_FLIP_COUNTER CR_SPARAM_RX_FFE_FLIP_COUNTER, "RX FFE flip counter", CR_PARAM_TYPE_PARAM
#define SP_RX_FFE_KACCU        CR_SPARAM_RX_FFE_KACCU, "RX FFE KACCU", CR_PARAM_TYPE_PARAM
#define SP_RX_FFE_NBIAS        CR_SPARAM_RX_FFE_NBIAS, "RX FFE NBIAS", CR_PARAM_TYPE_PARAM
#define SP_RX_FFE_TAPS_ALL     CR_SPARAM_RX_FFE_TAPS_ALL, "RX all ffe taps for all phase", CR_PARAM_TYPE_PARAM
#define SP_RX_FFE_TAPS         CR_SPARAM_RX_FFE_TAPS, "RX FFE taps", CR_PARAM_TYPE_PARAM
#define SP_RX_FFE_TAPS_FINE    CR_SPARAM_RX_FFE_TAPS_FINE, "RX FFE taps fine", CR_PARAM_TYPE_PARAM
#define SP_RX_FFE_DEGENDL_TIA  CR_SPARAM_RX_FFE_DEGENDL_TIA, "RX FFE degendl tia", CR_PARAM_TYPE_PARAM
#define SP_RX_FFE_WEIGHTING_TABLE \
    CR_SPARAM_RX_FFE_WEIGHTING_TABLE, "RX FFE weighting table: 5x9 Matrix", CR_PARAM_TYPE_PARAM
#define SP_RX_GRAYCODE       CR_SPARAM_RX_GRAYCODE, "RX Gray code", CR_PARAM_TYPE_CONTROL_DEBUG
#define SP_RX_INPUT_COUPLING CR_SPARAM_RX_INPUT_COUPLING, "0 for DC, 1 for AC", CR_PARAM_TYPE_CONTROL
#define SP_RX_LINKLOST       CR_SPARAM_RX_LINKLOST, "RX firmware Link Lost Count. Clear on read.", CR_PARAM_TYPE_STATUS
#define SP_RX_MSBLSB         CR_SPARAM_RX_MSBLSB, "RX data endianess", CR_PARAM_TYPE_CONTROL_DEBUG
#define SP_RX_PLL_CAP        CR_SPARAM_RX_PLL_CAP, "RX pll cap", CR_PARAM_TYPE_PARAM
#define SP_RX_POL            CR_SPARAM_RX_POL, "RX Polarity", CR_PARAM_TYPE_CONTROL
#define SP_RX_PPM            CR_SPARAM_RX_PPM, "RX PPM", CR_PARAM_TYPE_PARAM
#define SP_RX_PRECODER       CR_SPARAM_RX_PRECODER, "RX pre coder", CR_PARAM_TYPE_CONTROL_DEBUG
#define SP_RX_READAPT        CR_SPARAM_RX_READAPT, "RX firmware Re-adapt Count. Clear on read.", CR_PARAM_TYPE_STATUS
#define SP_RX_SIGNAL_DETECT  CR_SPARAM_RX_SIGNAL_DETECT, "RX Signal Detect", CR_PARAM_TYPE_PARAM
#define SP_RX_SKEF           CR_SPARAM_RX_SKEF, "RX Skin effect", CR_PARAM_TYPE_PARAM
#define SP_RX_SKEF_ADDCAP    CR_SPARAM_RX_SKEF_ADDCAP, "RX Skin Effect addcap", CR_PARAM_TYPE_PARAM
#define SP_RX_SKEF_DEGEN     CR_SPARAM_RX_SKEF_DEGEN, "RX Skin Effect degen", CR_PARAM_TYPE_PARAM
#define SP_RX_SKEF_EN        CR_SPARAM_RX_SKEF_EN, "RX Skin Effect enable", CR_PARAM_TYPE_PARAM
#define SP_RX_SKEF_GAIN      CR_SPARAM_RX_SKEF_GAIN, "RX Skin Effect gain", CR_PARAM_TYPE_PARAM
#define SP_TX_GRAYCODE       CR_SPARAM_TX_GRAYCODE, "TX Gray code", CR_PARAM_TYPE_CONTROL_DEBUG
#define SP_TX_MSBLSB         CR_SPARAM_TX_MSBLSB, "TX data endianess", CR_PARAM_TYPE_CONTROL_DEBUG
#define SP_TX_PLL_CAP        CR_SPARAM_TX_PLL_CAP, "TX pll cap", CR_PARAM_TYPE_PARAM
#define SP_TX_POL            CR_SPARAM_TX_POL, "TX Polarity", CR_PARAM_TYPE_CONTROL
#define SP_TX_PRECODER       CR_SPARAM_TX_PRECODER, "TX pre coder", CR_PARAM_TYPE_CONTROL_DEBUG
#define SP_TX_TAPS           CR_SPARAM_TX_TAPS, "TX FFE Taps", CR_PARAM_TYPE_CONTROL
#define SP_TX_TAPS_RAW       CR_SPARAM_TX_TAPS_RAW, "TX FFE Taps Raw value without padding", CR_PARAM_TYPE_CONTROL
#define SP_TX_TAPS_SCALE     CR_SPARAM_TX_TAPS_SCALE, "TX FFE Taps scale", CR_PARAM_TYPE_PARAM
#define SP_RX_ISI            CR_SPARAM_RX_ISI, "RX ISI", CR_PARAM_TYPE_PARAM
#define SP_RX_ISI_ALL        CR_SPARAM_RX_ISI_ALL, "RX ISI for all phase", CR_PARAM_TYPE_PARAM
#define SP_TX_TAPS_NL        CR_SPARAM_TX_TAPS_NONLINEAR, "TX taps nonlinear", CR_PARAM_TYPE_PARAM
#define SP_TX_INTP           CR_SPARAM_TX_INTERPLOTATOR, "TX interplotator", CR_PARAM_TYPE_PARAM
#define SP_PLL_LOCK          CR_SPARAM_PLL_LOCK, "PLL lock", CR_PARAM_TYPE_PARAM
#define SP_TOP_PLL_LOCK      CR_SPARAM_TOP_PLL_LOCK, "Top PLL lock", CR_PARAM_TYPE_PARAM
#define SP_TX_PHASE          CR_SPARAM_TX_PHASE, "TX phase delay", CR_PARAM_TYPE_PARAM
#define SP_RX_PHASE          CR_SPARAM_RX_PHASE, "RX phase delay", CR_PARAM_TYPE_PARAM

#define SP_LANE_STATE CR_LPARAM_STATE, "The firmware state", CR_PARAM_TYPE_STATUS

// too many to deal with using a union
typedef void (*ParamSetter_t)(void);
typedef void (*ParamGetter_t)(void);

typedef CredoError_t (*ParamCount_t)(CredoSlice_t* slice, int index, int* count);

typedef struct {
    const char* name;         // name of the serdes_control
    const char* description;  // desription of serdes param
    const char* type;
    CredoParamIndex_t index_type;
    CredoParamValue_t val_type;  // indicate value type
    int count;                   // number of values
    ParamGetter_t getter;        // provide the getter (has_getter checks if == NULL if val_type)
    ParamSetter_t setter;        // provide the getter (has_setter checks if == NULL if val_type)
    ParamCount_t counter;
    ParamCount_t set_counter;
    uint64_t flags;
} ParamHandler_t;

#ifdef HAL_SUPPORT_PARAMS_SLICE
extern const ParamHandler_t param_slice_list[];
extern const int param_slice_count;
#endif

#ifdef HAL_SUPPORT_PARAMS_PORT
extern const ParamHandler_t param_port_list[];
extern const int param_port_count;
#endif

#ifdef HAL_SUPPORT_PARAMS_LANE
extern const ParamHandler_t param_lane_list[];
extern const int param_lane_count;
#endif

#ifdef HAL_SUPPORT_PARAMS_SERDES
extern const ParamHandler_t param_serdes_list[];
extern const int param_serdes_count;
#endif

CredoError_t param_set_param(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index, int value);
CredoError_t param_get_param(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index, int* value);
CredoError_t param_set_param_dbl(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index, double value);
CredoError_t param_get_param_dbl(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index, double* value);

CredoError_t param_get_data(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                            CredoParamData_t* data);

CredoError_t param_set_data(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                            const CredoParamData_t* data);

CredoError_t param_get_param_count(CredoSlice_t* slice, ParamDomain_t domain, int* count);
CredoError_t param_get_param_val_set_count(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                                           int* count);
CredoError_t param_get_param_val_count(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                                       int* count);

CredoError_t param_index_param_def(CredoSlice_t* slice, ParamDomain_t domain, int param_index, CredoParam_t* param);
// returns param if it exists otherwise NULL (user provides the struct)
CredoError_t param_find_param_def(CredoSlice_t* slice, ParamDomain_t domain, const char* name, bool* found,
                                  CredoParam_t* param);

#define PARAM_DEF(name, description, type, idxtype, valtype, count, getter, setter, counter, set_counter, flags) \
    {                                                                                                            \
        name, description, type, idxtype, valtype, count, (ParamGetter_t)(getter), (ParamSetter_t)(setter),      \
            (ParamCount_t)(counter), (ParamCount_t)(set_counter), flags                                          \
    }

// param_info composed of name, description, type
// top, int
#define PARAM_TOP_INT_FLG(param_info, getter, setter, flags) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_TOP, CR_PARAM_VAL_INT, 1, getter, setter, NULL, NULL, flags)
#define PARAM_TOP_INTLIST_FLG(param_info, count, getter, setter, flags) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_TOP, CR_PARAM_VAL_INT, count, getter, setter, NULL, NULL, flags)

#define PARAM_TOP_INT(param_info, getter, setter) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_TOP, CR_PARAM_VAL_INT, 1, getter, setter, NULL, NULL, PARAM_FLAG_NONE)
#define PARAM_TOP_INTLIST(param_info, count, getter, setter) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_TOP, CR_PARAM_VAL_INT, count, getter, setter, NULL, NULL, PARAM_FLAG_NONE)
#define PARAM_TOP_VAR_INTLIST_FLG(param_info, max_count, getter, setter, counter, set_counter, flags)            \
    PARAM_DEF(param_info, CR_PARAM_INDEX_TOP, CR_PARAM_VAL_INT, max_count, getter, setter, counter, set_counter, \
              (flags) | CR_PARAM_FLAG_VAR_COUNT)

#define PARAM_TOP_DBL(param_info, getter, setter) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_TOP, CR_PARAM_VAL_FLOAT, 1, getter, setter, NULL, NULL, PARAM_FLAG_NONE)

// port, int
#define PARAM_PORT_INT_FLG(param_info, getter, setter, flags) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_PORT, CR_PARAM_VAL_INT, 1, getter, setter, NULL, NULL, flags)
#define PARAM_PORT_INTLIST_FLG(param_info, count, getter, setter, flags) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_PORT, CR_PARAM_VAL_INT, count, getter, setter, NULL, NULL, flags)

#define PARAM_PORT_INT(param_info, getter, setter) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_PORT, CR_PARAM_VAL_INT, 1, getter, setter, NULL, NULL, PARAM_FLAG_NONE)
#define PARAM_PORT_INTLIST(param_info, count, getter, setter) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_PORT, CR_PARAM_VAL_INT, count, getter, setter, NULL, NULL, PARAM_FLAG_NONE)

// lane, int
#define PARAM_LANE_INT_FLG(param_info, getter, setter, flags) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_LANE, CR_PARAM_VAL_INT, 1, getter, setter, NULL, NULL, flags)
#define PARAM_LANE_INTLIST_FLG(param_info, count, getter, setter, flags) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_LANE, CR_PARAM_VAL_INT, count, getter, setter, NULL, NULL, flags)
#define PARAM_LANE_VAR_INTLIST_FLG(param_info, max_count, getter, setter, counter, set_counter, flags)            \
    PARAM_DEF(param_info, CR_PARAM_INDEX_LANE, CR_PARAM_VAL_INT, max_count, getter, setter, counter, set_counter, \
              (flags) | CR_PARAM_FLAG_VAR_COUNT)

#define PARAM_LANE_INT(param_info, getter, setter) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_LANE, CR_PARAM_VAL_INT, 1, getter, setter, NULL, NULL, PARAM_FLAG_NONE)
#define PARAM_LANE_INTLIST(param_info, count, getter, setter) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_LANE, CR_PARAM_VAL_INT, count, getter, setter, NULL, NULL, PARAM_FLAG_NONE)

// lane, float
#define PARAM_LANE_DBL_FLG(param_info, getter, setter, flags) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_LANE, CR_PARAM_VAL_FLOAT, 1, getter, setter, NULL, NULL, flags)
#define PARAM_LANE_DBLLIST_FLG(param_info, count, getter, setter, flags) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_LANE, CR_PARAM_VAL_FLOAT, count, getter, setter, NULL, NULL, flags)

#define PARAM_LANE_DBL(param_info, getter, setter) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_LANE, CR_PARAM_VAL_FLOAT, 1, getter, setter, NULL, NULL, PARAM_FLAG_NONE)
#define PARAM_LANE_DBLLIST(param_info, count, getter, setter) \
    PARAM_DEF(param_info, CR_PARAM_INDEX_LANE, CR_PARAM_VAL_FLOAT, count, getter, setter, NULL, NULL, PARAM_FLAG_NONE)

#endif
