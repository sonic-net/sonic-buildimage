#include "project.h"

#include "common/common_firmware.h"

#include "utility.h"
#include "sdk.h"

/*
 * Firmware debug ex command
 * Input
 *  Major command   = 0xB
 *  cmd.3:0         = section
 *  Detail1.15:0    = index
 *  Detail2.4:0     = lane number
 * Output
 *  Detail1         = result
 *  Detail2         = result1
 */
CredoError_t common_fw_debug_cmd_ex(CredoSlice_t* slice, unsigned lane, unsigned section, unsigned index,
                                    unsigned* value1, unsigned* value2) {
#if defined(HAL_FW_DEBUG_CMD_EX_OLD) && (HAL_FW_DEBUG_CMD_EX_OLD == 1)
    return hal_fw_cmd_ex(slice, FW_CMD_INFO + ((section & 0xf) << 4) + (lane & 0xf), index, 0, NULL, value1, value2);
#else
    return hal_fw_cmd_ex(slice, FW_CMD_INFO + (section & 0xf), index, lane, NULL, value1, value2);
#endif
}

CredoError_t common_fw_debug_cmd(CredoSlice_t* slice, unsigned lane, unsigned section, unsigned index,
                                 unsigned* value) {
#if defined(HAL_FW_DEBUG_CMD_OLD) && (HAL_FW_DEBUG_CMD_OLD == 1)
    return hal_fw_cmd(slice, FW_CMD_INFO + ((section & 0xf) << 4) + (lane & 0xf), index, NULL, value);
#else
    return hal_fw_debug_cmd_ex(slice, lane, section, index, value, NULL);
#endif
}
