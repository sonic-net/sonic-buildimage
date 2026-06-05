#include "screaming_eagle.h"
#include "se_functions.h"

#include "utility.h"

CredoError_t se_time_start(CredoSlice_t* slice, double* unix_time) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (seslice->start_time != 0) {
        *unix_time = seslice->start_time;
        return CR_OK;
    }
    double current_time = get_walltime();
    double current_sys_timedelta;
    ERR_PROPS(se_time_system(slice, &current_sys_timedelta));
    seslice->start_time = current_time - current_sys_timedelta;
    *unix_time = seslice->start_time;
    return CR_OK;
}

CredoError_t se_time_system(CredoSlice_t* slice, double* timedelta) {
    unsigned msb = 0, lsb = 0;
    ERR_PROPS(hal_fw_debug_cmd_ex(slice, 0, SE_INFO, SE_INFO_SYS_TIME, &lsb, &msb));
    *timedelta = FW_STATE_TIME_UNIT * ((msb << 16) | lsb) / 1000;
    return CR_OK;
}
