#ifndef CREDO_SERDES_H
#define CREDO_SERDES_H

#include "credo/base.h"
#include "credo/params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CR_COUPLING_AC or CR_COUPLING_DC coupling of receiver
 */
typedef enum {
    CR_COUPLING_DC = 0,  //!< CR_COUPLING_DC Coupling
    CR_COUPLING_AC       //!< CR_COUPLING_AC Coupling
} CredoLaneCoupling_t;

/**
 * @brief SerDes Channel Description
 * Info used to preset tx taps
 *
 */
typedef struct {
    CredoLaneMode_t mode;   //!< Lane mode of the lane
    unsigned speed;         //!< lane speed
    unsigned channel_loss;  //!< dB of channel loss, used if optical is false
    bool optical;           //!< Indicate channel is to optical module
    unsigned opt_vswing;    //!< Indicate optical swing level, used only if optical is enabled
} CredoChannelDesc_t;

// Polarity

/**
 * @brief Set target lane rx polarity
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] rx_pol
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_rx_polarity(CredoSlice_t* slice, int lane, int rx_pol);

/**
 * @brief Set target lane tx polarity
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] tx_pol
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_tx_polarity(CredoSlice_t* slice, int lane, int tx_pol);

/**
 * @brief Get target lane rx polarity
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] rx_pol
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_polarity(CredoSlice_t* slice, int lane, int* rx_pol);

/**
 * @brief Get target lane tx polarity
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] tx_pol
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_tx_polarity(CredoSlice_t* slice, int lane, int* tx_pol);

// Input Mode

/**
 * @brief Set target lane input couple mode
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] input_mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_rx_coupling(CredoSlice_t* slice, int lane, CredoLaneCoupling_t input_mode);

/**
 * @brief Get target lane input couple mode
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] input_mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_coupling(CredoSlice_t* slice, int lane, CredoLaneCoupling_t* input_mode);

// Gray & Pre Coding

/**
 * @brief Set target lane tx gray code
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] tx_gc 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_tx_gray_code(CredoSlice_t* slice, int lane, int tx_gc);

/**
 * @brief Set target lane rx gray code
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] rx_gc 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_rx_gray_code(CredoSlice_t* slice, int lane, int rx_gc);

/**
 * @brief Get target lane tx gray code enable status
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] tx_gc 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_tx_gray_code(CredoSlice_t* slice, int lane, int* tx_gc);

/**
 * @brief Get target lane rx gray code enable status
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] rx_gc 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_gray_code(CredoSlice_t* slice, int lane, int* rx_gc);

/**
 * @brief Set target lane tx precoder
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] tx_pc 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_tx_precoder(CredoSlice_t* slice, int lane, int tx_pc);

/**
 * @brief Set target lane rx precoder
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] rx_pc 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_rx_precoder(CredoSlice_t* slice, int lane, int rx_pc);

/**
 * @brief Get target lane tx precoder, return programmed value instead of register value if firmware support tx precoder
 * setting
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] tx_pc 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_tx_precoder(CredoSlice_t* slice, int lane, int* tx_pc);

/**
 * @brief Get target lane rx precoder
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] rx_pc 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_precoder(CredoSlice_t* slice, int lane, int* rx_pc);

// Bit swapping

/**
 * @brief Set target lane tx msb and lsb swap
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] tx_msb 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_tx_msb(CredoSlice_t* slice, int lane, int tx_msb);

/**
 * @brief Set target lane rx msb and lsb swap
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] rx_msb 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_rx_msb(CredoSlice_t* slice, int lane, int rx_msb);

/**
 * @brief Get target lane tx msb and lsb swap status
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] tx_msb 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_tx_msb(CredoSlice_t* slice, int lane, int* tx_msb);

/**
 * @brief Get target lane rx msb and lsb swap status
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] rx_msb 1 = enable, 0 = disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_msb(CredoSlice_t* slice, int lane, int* rx_msb);

// TX Taps

/**
 * @brief Set target lane taps value
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] taps tx taps value (f-N,...,f-2,f-1,f0,f1,f2,fN)
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_tx_taps(CredoSlice_t* slice, int lane, const int taps[]);

/**
 * @brief Set tx taps for a lane based on channel description
 *
 * These are basic presets, and will not give optimal performance. They are useful starting points before tuning tx
 * taps. This is not needed if you are using LT. These tx taps are typically omptimized around a duplicate of the chip
 * on the rx side.
 *
 * This should **not** be used in production, instead either use ANLT or run experiments to determine optical tx taps
 * for your channels.
 *
 *
 * @param[in] slice
 * @param[in] lane
 * @param[in] description
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_preset_tx_taps(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* description);

/**
 * @brief Get target lane taps value
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] taps tx taps value (f-N,...,f-2,f-1,f0,f1,f2,fN)
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_tx_taps(CredoSlice_t* slice, int lane, int taps[]);

/**
 * @brief status of the phy (Lane) layer FW "AdpDone"
 * @param[in] slice slice handle
 * @param[out] rdy ready status of the slice
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_all_phy_ready(CredoSlice_t* slice, unsigned* rdy);

/**
 * @brief status of the phy (Lane) layer for a specific lane FW "AdpDone"
 * @param[in] slice slice handle
 * @param[in] lane lane to use
 * @param[out] rdy ready status of the slice lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_phy_ready(CredoSlice_t* slice, int lane, unsigned* rdy);

/**
 * @brief Get serdes parameter
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[in,out] data You must provide the value storage, type, and count. param.value buffer will be updated
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_param(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data);

/**
 * @brief Get serdes param information and use heap
 *
 * data
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[out] data parameter data, you must free after use
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_paramh(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data);

/**
 * @brief Set serdes parameter
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[in] data
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_param(CredoSlice_t* slice, const char* name, int index,
                                          const CredoParamData_t* data);

// utility for c11 that enables easy overload access
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

#define cr_serdes_get_paramv(slice, name, index, val, count) \
    cr_serdes_get_param(slice, name, index, CR_PDATA(val, count))

#define cr_serdes_set_paramv(slice, name, index, val, count) \
    cr_serdes_set_param(slice, name, index, CR_PDATA(val, count))

#endif

#ifdef __cplusplus
}
#endif

#endif
