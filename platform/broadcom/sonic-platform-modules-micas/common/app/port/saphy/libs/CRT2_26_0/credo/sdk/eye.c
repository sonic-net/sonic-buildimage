#include "dii.h"

#include <stdlib.h>

CredoError_t cr_eye_monitor_start(CredoSlice_t* slice, int lane, int ber_exp, int flag) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_eye_monitor_start(slice, lane, ber_exp, flag));
}

CredoError_t cr_eye_monitor_stop(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_eye_monitor_stop(slice, lane));
}

CredoError_t cr_eye_monitor_get_progress(CredoSlice_t* slice, int lane, int* percent) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_eye_monitor_progress(slice, lane, percent));
}

CredoError_t cr_eye_monitor_get_data(CredoSlice_t* slice, int lane, int** data, int* extent_mv) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_eye_monitor_data(slice, lane, data, extent_mv));
}

CredoError_t cr_eye_monitor_get_range(CredoSlice_t* slice, int lane, int* vstep_side, int* hstep_side) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_eye_monitor_range(slice, lane, vstep_side, hstep_side));
}

CredoError_t cr_eye_monitor_get_separator(CredoSlice_t* slice, int separator[5]) {
    LOGS_API();
    CALL_HAL(slice, hal_fw_eye_monitor_separator(slice, separator));
}

CredoError_t cr_eye_monitor_get_datah(CredoSlice_t* slice, int lane, CredoEyeData_t* data) {
    int vstep_side, hstep_side;
    CredoError_t err = cr_eye_monitor_get_range(slice, lane, &vstep_side, &hstep_side);
    if (err != CR_OK) {
        return err;
    }

    size_t rows = (vstep_side * 2) + 1;
    size_t cols = (hstep_side * 2) + 1;

    // single malloc 2d data
    int** eye_data = (int**)malloc(rows * sizeof(int*) + rows * cols * sizeof(int));
    if (eye_data == NULL) {
        return CR_OUT_OF_MEMORY;
    }
    for (size_t r = 0; r < rows; r++) {
        eye_data[r] = (int*)(eye_data + rows) + r * cols;
    }
    err = cr_eye_monitor_get_data(slice, lane, eye_data, &data->extent_mv);
    if (err != CR_OK) {
        free(eye_data);
    }
    data->data = eye_data;
    data->cols = cols;
    data->rows = rows;
    return CR_OK;
}
