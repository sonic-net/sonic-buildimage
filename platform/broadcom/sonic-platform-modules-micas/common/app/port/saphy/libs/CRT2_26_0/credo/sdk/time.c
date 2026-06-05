#include "dii.h"

CredoError_t cr_time_start(CredoSlice_t* slice, double* unix_time) {
    CALL_HAL(slice, hal_time_start(slice, unix_time));
}

CredoError_t cr_time_system(CredoSlice_t* slice, double* timedelta) {
    CALL_HAL(slice, hal_time_system(slice, timedelta));
}
