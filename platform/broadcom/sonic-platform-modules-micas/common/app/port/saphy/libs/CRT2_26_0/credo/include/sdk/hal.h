#ifndef HAL_H
#define HAL_H

/*
 * Common function configuration:
 * HAL_SUPPORT_TOP_PLL_CAL           Firmware support top pll calibration
 * HAL_SUPPORT_FW_ADAPT_COUNT        Firmware support adapt count
 * HAL_SUPPORT_FW_EYE                Firmware support eye reading
 * HAL_SUPPORT_FW_ISI                Firmware support isi reading
 * HAL_SUPPORT_EYE_MONITOR           Firmware support eye monitor
 * HAL_SUPPORT_FW_RATIO              Firmware support rx ratio
 * HAL_SUPPORT_FW_OF                 Firmware support rx of
 * HAL_SUPPORT_FW_HF                 Firmware support rx hf
 */

#include "sdk/hal_func_table.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

CREDOAPI void cr_register_device(const DeviceCapability_t*);
const DeviceCapability_t* cr_device_search(uint32_t device_type);

#define CHECK_HAL(slice, hal_func) ((slice->hal->hal_func == NULL) ? 0 : 1)

#define hal_func(slice, ...) \
    ((slice->hal->hal_func == NULL) ? CR_NOTIMPLEMENTED : slice->hal->hal_func(slice, __VA_ARGS__))

#ifdef __cplusplus
}
#endif

#endif
