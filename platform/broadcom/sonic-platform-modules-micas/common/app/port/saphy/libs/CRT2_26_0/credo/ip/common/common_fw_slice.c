#include "project.h"

#include "common/common_firmware.h"

#include "sdk.h"

CredoError_t common_fw_get_slice_temp(CredoSlice_t *slice, double *temp) {
#ifdef FWREG_TOP_TEMPERATURE
    if (temp == NULL) return CR_INVALID_ARGS;

    int temp_raw;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_TEMPERATURE, (unsigned *)&temp_raw));

    if (temp_raw == 0x7FFF) {
        LOGS_ERROR("Temperature sensor not ready");
        return CR_FAIL;
    }

#ifdef HAL_CHIP_12NM
    *temp = (temp_raw / 4094.0 - 0.5) * 202.8 + 59.1 - 0.16 * ((double)T_CLK / T_DIV_RATIO);
#else
    double yds = 237.7 / 4096;
    double kds = 79.925;

    if ((temp_raw & (1 << 15)) != 0) {
        temp_raw -= 1 << 16;
    }
    *temp = (temp_raw * yds) - kds;
#endif

    return CR_OK;
#else
    return CR_NOTIMPLEMENTED;
#endif
}
