#include "dii.h"

CredoError_t cr_testpoint_select(CredoSlice_t* slice, const CredoTestPoint_t* testpoint) {
    CALL_HAL(slice, hal_testpoint_select(slice, testpoint));
}

CredoError_t cr_testpoint_clear(CredoSlice_t* slice) {
    CALL_HAL(slice, hal_testpoint_clear(slice));
}

CredoError_t cr_testpoint_read(CredoSlice_t* slice, double* value) {
    CALL_HAL(slice, hal_testpoint_read(slice, value));
}
