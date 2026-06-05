#include "dii.h"
#include "lock.h"

#include "sdk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned lock_timeout_usec = 0;

void cr_lock_set_timeout(unsigned timeout_usec) {
    lock_timeout_usec = timeout_usec;
}

unsigned cr_lock_get_timeout(void) {
    return lock_timeout_usec;
}

CredoError_t cr_slice_init(CredoSlice_t* slice, const CredoSliceConfig_t* config) {
    if (slice == NULL) return CR_FAIL;
    LOGS_API();
    SLICE_LOCK_GUARD(slice);
    slice->slice_context = config->slice_context;
    slice->slice_id = config->slice_id;
    slice->flags = 0;
    CredoError_t err = hal_slice_init(slice, config);
    SLICE_UNLOCK(slice);
    return err;
}

CredoError_t cr_slice_reinit(CredoSlice_t* slice, CredoSliceInitType_t init, const char* firmware) {
    LOGS_API("init %d, firmware %s", init, firmware);
    SLICE_LOCK_GUARD(slice);
    int tx_polarity_map[MAX_LANE_PER_SLICE] = {0};
    int rx_polarity_map[MAX_LANE_PER_SLICE] = {0};

    // Later could add a hal_slice_reinit API if needed

    // capture polarities to restore after init
    if (init == CR_INIT_NO_FIRMWARE || init == CR_INIT_FULL) {
        for (int lane = 0; lane < slice->desc->lane_count; lane++) {
            hal_get_tx_polarity(slice, lane, &tx_polarity_map[lane]);
            hal_get_rx_polarity(slice, lane, &rx_polarity_map[lane]);
        }
    }

    CredoSliceConfig_t config;
    config.init_type = init;
    config.firmware_filename = firmware;
    config.slice_id = slice->slice_id;
    config.slice_context = slice->slice_context;
    CredoError_t err = cr_slice_init(slice, &config);

    // reconfigure captured polarites
    if (init == CR_INIT_NO_FIRMWARE || init == CR_INIT_FULL) {
        for (int lane = 0; lane < slice->desc->lane_count; lane++) {
            hal_set_tx_polarity(slice, lane, tx_polarity_map[lane]);
            hal_set_rx_polarity(slice, lane, rx_polarity_map[lane]);
        }
    }

    SLICE_UNLOCK(slice);
    return err;
}

CredoError_t cr_slice_get_oui(CredoSlice_t* slice, unsigned* oui) {
    LOGS_API();
    CALL_HAL(slice, hal_slice_get_oui(slice, oui));
}

CredoError_t cr_slice_get_model_number(CredoSlice_t* slice, unsigned* model_number) {
    LOGS_API();
    CALL_HAL(slice, hal_slice_get_model_number(slice, model_number));
}

CredoError_t cr_slice_get_revision_number(CredoSlice_t* slice, unsigned* revision_number) {
    LOGS_API();
    CALL_HAL(slice, hal_slice_get_revision_number(slice, revision_number));
}

void* cr_slice_get_userdata(const CredoSlice_t* slice) {
    return NULL;
}

CredoError_t cr_slice_get_type(CredoSlice_t* slice, CredoSliceType_t* slice_type) {
    LOGS_API();
    CredoError_t err = hal_slice_get_type(slice, slice_type);
    if (err == CR_NOTIMPLEMENTED_HAL) {
        *slice_type = (CredoSliceType_t)slice->desc->slice_type;
        return CR_OK;
    }

    return err;
}

// used for selecting slices
CredoError_t cr_slice_get_family(CredoSlice_t* slice, CredoFamily_t* family) {
    *family = (CredoFamily_t)slice->desc->slice_type >> 8;
    return CR_OK;
}

CredoError_t cr_slice_get_device_type(const CredoSlice_t* _slice, CredoDeviceType_t* device_type) {
    CredoSlice_t* slice = (CredoSlice_t*)_slice;
    if (slice == NULL || device_type == NULL) return CR_INVALID_ARGS;
    LOGS_API();
    *device_type = slice->device->desc->device_type;
    return CR_OK;
}

CredoError_t cr_slice_set_userdata(CredoSlice_t* slice, void* userdata) {
    LOGS_API();
    return CR_NOTIMPLEMENTED;
}

CredoError_t cr_slice_get_limits(const CredoSlice_t* _slice, CredoSliceLimit_t* limits) {
    CredoSlice_t* slice = (CredoSlice_t*)_slice;
    if (slice == NULL || limits == NULL) return CR_FAIL;

    LOGS_API();
    limits->max_ports = slice->desc->port_count;
    limits->max_lanes = slice->desc->lane_count;
    limits->max_vsensors = slice->desc->vsensor_count;
    return CR_OK;
}

CredoError_t cr_slice_get_vsensor(CredoSlice_t* slice, double* vsensor) {
    LOGS_API();
    CALL_HAL(slice, hal_slice_get_vsensor(slice, vsensor));
}

CredoError_t cr_slice_get_vsensor_ex(CredoSlice_t* slice, unsigned sel_vin, double* vsensor) {
    LOGS_API();
    CALL_HAL(slice, hal_slice_get_vsensor_ex(slice, sel_vin, vsensor));
}

CredoError_t cr_sram_get_status(CredoSlice_t* slice, CredoSramStatus_t* sram_status) {
    LOGS_API();
    CALL_HAL(slice, hal_slice_get_sram_status(slice, sram_status));
}

CredoError_t cr_sram_generate_error(CredoSlice_t* slice, CredoSramStatus_t sram_status) {
    LOGS_API();
    CALL_HAL(slice, hal_slice_generate_sram_error(slice, sram_status));
}

CredoError_t cr_slice_load_setup(CredoSlice_t* slice, const char* file_path) {
    LOGS_API("file_path %s", file_path);
    CALL_HAL(slice, hal_slice_load_setup(slice, file_path));
}

CredoError_t cr_slice_save_setup(CredoSlice_t* slice, const char* file_path) {
    LOGS_API("file_path %s", file_path);
    CALL_HAL(slice, hal_slice_save_setup(slice, file_path));
}
CredoError_t cr_slice_set_mdio_mode(CredoSlice_t* slice, bool is_push_pull) {
    LOGS_API("is_push_pull %d", is_push_pull);
    CALL_HAL(slice, hal_slice_set_mdio_mode(slice, is_push_pull));
}

// Clock Output
CredoError_t cr_clockout_enable(CredoSlice_t* slice, unsigned clock_output, unsigned lane, unsigned divider) {
    LOGS_API("clock_output %u, lane %d, divider %u", clock_output, lane, divider);
    CALL_HAL(slice, hal_slice_enable_clock_output(slice, clock_output, lane, divider));
}
CredoError_t cr_clockout_disable_all(CredoSlice_t* slice) {
    LOGS_API();
    CALL_HAL(slice, hal_slice_disable_all_clock_output(slice));
}
CredoError_t cr_clockout_disable(CredoSlice_t* slice, unsigned clock_output) {
    LOGS_API("clock_output %u", clock_output);
    CALL_HAL(slice, hal_slice_disable_clock_output(slice, clock_output));
}

CredoError_t cr_slice_get_option_count(CredoSlice_t* slice, int* count) {
    if (slice == NULL) return CR_INVALID_ARGS;
    LOGS_API();
    *count = slice->device->desc->slice_capability->option_count;
    return CR_OK;
}

CredoError_t cr_slice_index_option_list(CredoSlice_t* slice, int index, CredoSliceOption_t* option) {
    if (slice == NULL) return CR_INVALID_ARGS;
    LOGS_API("index %d", index);
    int count = slice->device->desc->slice_capability->option_count;
    if (index < 0 || index >= count) return CR_INVALID_ARGS;
    const SliceOption_t* option_list = slice->device->desc->slice_capability->option_list;
    option->name = option_list[index].name;
    option->description = option_list[index].description;
    return CR_OK;
}

static const SliceOption_t* find_option(CredoSlice_t* slice, const char* name) {
    LOGS_API("name: %s", name);
    int option_count = slice->device->desc->slice_capability->option_count;
    const SliceOption_t* option_list = slice->device->desc->slice_capability->option_list;
    if (option_list == NULL || option_count == 0) return NULL;
    for (int i = 0; i < option_count; i++) {
        if (strcmp(name, option_list[i].name) == 0) {
            return &option_list[i];
        }
    }
    return NULL;
}

CredoError_t cr_slice_is_option_supported(CredoSlice_t* slice, const char* option_name) {
    if (slice == NULL || option_name == NULL) {
        return CR_INVALID_ARGS;
    }
    LOGS_API("option_name: %s", option_name);

    return find_option(slice, option_name) != NULL ? CR_OK : CR_NOTIMPLEMENTED;
}

CredoError_t cr_slice_get_option(CredoSlice_t* slice, const char* option_name, int* value) {
    if (slice == NULL || option_name == NULL || value == NULL) {
        return CR_INVALID_ARGS;
    }

    LOGS_API("option_name: %s", option_name);
    const SliceOption_t* option = find_option(slice, option_name);
    if (option == NULL) {
        return CR_NOTIMPLEMENTED;
    } else {
        if (option->get_func_ptr != NULL) {
            return option->get_func_ptr(slice, option_name, value);
        } else {
            return CR_NOTIMPLEMENTED;
        }
    }
}

CredoError_t cr_slice_set_option(CredoSlice_t* slice, const char* option_name, int value) {
    if (slice == NULL || option_name == NULL) {
        return CR_INVALID_ARGS;
    }

    LOGS_API("option_name: %s, value %d", option_name, value);
    const SliceOption_t* option = find_option(slice, option_name);

    if (option == NULL) {
        return CR_NOTIMPLEMENTED;
    } else {
        if (option->set_func_ptr != NULL) {
            return option->set_func_ptr(slice, option_name, value);
        } else {
            return CR_NOTIMPLEMENTED;
        }
    }
}

// Resets

CredoError_t cr_slice_soft_reset(CredoSlice_t* slice) {
    LOGS_API();
    CALL_HAL(slice, hal_soft_reset(slice));
}

CredoError_t cr_slice_logic_reset(CredoSlice_t* slice) {
    LOGS_API();
    CALL_HAL(slice, hal_logic_reset(slice));
}

CredoError_t cr_slice_mcu_reset(CredoSlice_t* slice) {
    LOGS_API();
    CALL_HAL(slice, hal_mcu_reset(slice));
}

CredoError_t cr_slice_mcu_reset_hold(CredoSlice_t* slice) {
    LOGS_API();
    CALL_HAL(slice, hal_mcu_reset_hold(slice));
}

CredoError_t cr_slice_reg_reset(CredoSlice_t* slice) {
    LOGS_API();
    CALL_HAL(slice, hal_reg_reset(slice));
}

// lock and unlock functionality
CredoError_t cr_slice_lock(CredoSlice_t* slice) {
    LOGS_API();
    if (cr_likely(lock_timeout_usec == 0)) {
        int lock_status = pthread_mutex_lock(&slice->lock);
        if (lock_status != 0) {
            LOGS_ERROR("unexpected mutex lock error code: %d", lock_status);
            return CR_FAIL;
        }
        return CR_OK;
    }
    CredoTime_t start_time;
    get_time(&start_time);
    do {
        int lock_status = pthread_mutex_trylock(&(slice)->lock);
        if (lock_status == 0) {
            return CR_OK;
        } else if (lock_status != EBUSY) {
            LOGS_ERROR("unexpected mutex lock error code: %d", lock_status);
            return CR_FAIL;
        }
        sleep_ms(5);
    } while (!is_timeout(&start_time, lock_timeout_usec));
    return CR_MUTEX_TIMEOUT;
}

void cr_slice_unlock(CredoSlice_t* slice) {
    LOGS_API();
    int lock_status = pthread_mutex_unlock(&(slice)->lock);
    if (lock_status != 0) {
        LOGS_ERROR("unexpected mutex unlock error code: %d", lock_status);
    }
}

CredoError_t cr_slice_get_temperature(CredoSlice_t* slice, double* temp) {
    LOGS_API();
    CALL_HAL(slice, hal_fw_get_slice_temp(slice, temp));
}

CredoError_t cr_slice_get_paramh(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data) {
    return cr_param_get_paramh(slice, PARAM_DOMAIN_SLICE, name, index, data);
}

CredoError_t cr_slice_get_param(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data) {
    return cr_param_get_param(slice, PARAM_DOMAIN_SLICE, name, index, data);
}

CredoError_t cr_slice_set_param(CredoSlice_t* slice, const char* name, int index, const CredoParamData_t* data) {
    return cr_param_set_param(slice, PARAM_DOMAIN_SLICE, name, index, data);
}

uint32_t cr_slice_id(CredoSlice_t* slice) {
    return slice->slice_id;
}

CredoError_t cr_efuse_read(CredoSlice_t* slice, int bank, uint32_t* val) {
    CALL_HAL(slice, hal_efuse_read(slice, bank, val));
}

CredoError_t cr_efuse_read_ecid(CredoSlice_t* slice, uint32_t ecid[2]) {
    CALL_HAL(slice, hal_efuse_read_ecid(slice, ecid));
}
