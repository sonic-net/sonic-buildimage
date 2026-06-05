#include "dii.h"

#include <stdlib.h>
#include <string.h>

CredoError_t cr_param_get_param_count(CredoSlice_t* slice, int domain, int* count) {
    CALL_HAL(slice, hal_get_param_count(slice, domain, count));
}

CredoError_t cr_param_get_param_val_count(CredoSlice_t* slice, int domain, const char* name, int index, int* count) {
    CALL_HAL(slice, hal_get_param_val_count(slice, domain, name, index, count));
}

CredoError_t cr_param_get_param_val_set_count(CredoSlice_t* slice, int domain, const char* name, int index,
                                              int* count) {
    CALL_HAL(slice, hal_get_param_val_set_count(slice, domain, name, index, count));
}

CredoError_t cr_param_get_paramh(CredoSlice_t* slice, int domain, const char* name, int index, CredoParamData_t* data) {
    SLICE_LOCK_GUARD(slice);

    CredoParam_t param;
    bool found;

    CredoError_t err = cr_param_find_param_def(slice, domain, name, &found, &param);
    if (err != CR_OK) {
        goto fail;
    }
    if (!found) {
        err = CR_INVALID_ARGS;
        goto fail;
    }
    int count = param.count;
    err = cr_param_get_param_val_count(slice, domain, name, index, &count);
    if (err != CR_OK) {
        goto fail;
    }
    data->value.d = calloc(count, sizeof(double));  // double is largest size, so we will over provision
    data->count = (size_t)count;
    data->type = param.val_type;
    if (data->value.i == NULL) {
        err = CR_OUT_OF_MEMORY;
        goto fail;
    }
    err = cr_param_get_param(slice, domain, name, index, data);
    if (err != CR_OK) {
        free(data->value.d);
        goto fail;
    }
    SLICE_UNLOCK(slice);
    return CR_OK;

fail:
    SLICE_UNLOCK(slice);
    data->value.d = NULL;
    return err;
}

CredoError_t cr_param_get_param(CredoSlice_t* slice, int domain, const char* name, int index, CredoParamData_t* data) {
    CALL_HAL(slice, hal_param_get_data(slice, domain, name, index, data));
}

CredoError_t cr_param_set_param(CredoSlice_t* slice, int domain, const char* name, int index,
                                const CredoParamData_t* data) {
    CALL_HAL(slice, hal_param_set_data(slice, domain, name, index, data));
}

CredoError_t cr_param_index_param_def(CredoSlice_t* slice, int domain, int param_index, CredoParam_t* param) {
    CALL_HAL(slice, hal_index_param_def(slice, domain, param_index, param));
}
// returns param if it exists otherwise NULL (user provides the struct)
CredoError_t cr_param_find_param_def(CredoSlice_t* slice, int domain, const char* name, bool* found,
                                     CredoParam_t* param) {
    CALL_HAL(slice, hal_find_param_def(slice, domain, name, found, param));
}
