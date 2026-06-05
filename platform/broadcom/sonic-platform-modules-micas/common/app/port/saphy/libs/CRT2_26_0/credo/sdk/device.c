#include "dii.h"

#include "sdk.h"

#include <stdio.h>
#include <stdlib.h>

CredoError_t cr_device_create(CredoSdk_t* sdk, CredoDeviceType_t device_type, CredoDevice_t** device) {
    LOGSDK_API("device type 0x%08X", device_type);
    const DeviceCapability_t* device_desc = cr_device_search(device_type);
    if (device == NULL) {
        return CR_INVALID_ARGS;
    }
    if (device_desc == NULL) {
        cr_log("sdk", CR_LOG_ERROR, "Unknown device ID 0x%06x", device_type);
        return CR_HAL_NOT_FOUND;
    }
    int i;
    const SliceCapability_t* slice_desc = device_desc->slice_capability;
    const HalFuncTable_t* funcs = slice_desc->hal_func_table;
    CredoDevice_t* d;
    CredoSlice_t* s;
    d = calloc(1, sizeof(CredoDevice_t));
    if (d == NULL) {
        return CR_OUT_OF_MEMORY;
    }
    d->sdk = sdk;
    d->desc = device_desc;
    for (i = 0; i < device_desc->slice_count; i++) {
        ERR_PROP((funcs->hal_allocate_slice)(sdk, &s));
        d->slices[i] = s;
        s->sdk = sdk;
        s->device = d;
        s->desc = slice_desc;
        s->hal = s->desc->hal_func_table;
        s->in_shell = false;
        s->slice_context = NULL;
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&s->lock, &attr);
    }
    *device = d;
    return CR_OK;
}

CredoError_t cr_device_destroy(CredoDevice_t* device) {
    LOGSDK_API();
    int i;
    const DeviceCapability_t* device_desc = device->desc;
    const SliceCapability_t* slice_desc = device_desc->slice_capability;
    for (i = 0; i < device->desc->slice_count; i++) {
        CredoSlice_t* slice = device->slices[i];
        pthread_mutex_destroy(&slice->lock);
        (slice_desc->hal_func_table->hal_free_slice)(device->slices[i]);
    }
    free(device);
    return CR_OK;
}

CredoError_t cr_device_get_type(CredoDevice_t* device, CredoDeviceType_t* device_type) {
    LOGSDK_API();
    if (device == NULL || device_type == NULL) {
        return CR_INVALID_ARGS;
    }
    *device_type = (CredoDeviceType_t)device->desc->device_type;
    return CR_OK;
}

CredoError_t cr_device_detect_type_push_pull(CredoSdk_t* sdk, void* slice_context, CredoDeviceType_t* device_type) {
    LOGSDK_API();
#define SDK_READ(addr, val)                               \
    ret = (sdk->read_register)(slice_context, addr, val); \
    if (ret != CR_OK) goto reg_fail;

#define SDK_WRITE(addr, val)                               \
    ret = (sdk->write_register)(slice_context, addr, val); \
    if (ret != CR_OK) goto reg_fail;

    CredoError_t ret = CR_OK;
    unsigned id;
    if (sdk == NULL || slice_context == NULL || device_type == NULL) return CR_INVALID_ARGS;

    *device_type = CREDO_FAKE_400;
    // set push pull mode
    SDK_WRITE(0xB73A, 0x40)

    // OWL
    SDK_READ(0x00CC, &id)
    if (id == 0x2161) {
        *device_type = CREDO_OWL_400;
    }

    // Osprey
    SDK_READ(0x00C2, &id)
    if (id == 0x2161) {
        *device_type = CREDO_OSPREY_400;
    }

    SDK_WRITE(0xB73A, 0x0)
    return (*device_type != CREDO_FAKE_400) ? CR_OK : CR_UNSUPPORTED;

reg_fail:
    SDK_WRITE(0xB73A, 0x0)
    cr_log("sdk", CR_LOG_ERROR, "register R/W fail");
    return ret;
}

CredoError_t cr_device_detect_type(CredoSdk_t* sdk, void* slice_context, CredoDeviceType_t* device_type) {
    LOGSDK_API();
#define SDK_READ(addr, val)                               \
    ret = (sdk->read_register)(slice_context, addr, val); \
    if (ret != CR_OK) goto reg_fail;

    CredoError_t ret = CR_OK;
    unsigned id;
    if (sdk == NULL || slice_context == NULL || device_type == NULL) return CR_INVALID_ARGS;

    // OWL
    SDK_READ(0x00CC, &id)
    if (id == 0x2161) {
        *device_type = CREDO_OWL_400;
        return CR_OK;
    }

    // Blackhawk
    SDK_READ(0x70C2, &id)
    if (id == 0x2161) {
        *device_type = CREDO_BLACKHAWK_400;
        return CR_OK;
    }

    // Heron
    SDK_READ(0x61C6, &id)
    if (id == 0x2161) {
        *device_type = CREDO_HERON_P3;
        return CR_OK;
    }

    // Osprey
    SDK_READ(0x00C2, &id)
    if (id == 0x2161) {
        *device_type = CREDO_OSPREY_400;
        return CR_OK;
    }

    // Bald Eagle
    SDK_READ(0x70CC, &id)
    if (id == 0x2161) {
        *device_type = CREDO_BALDEAGLE_400;
        return CR_OK;
    }

    // Nighthawk
    SDK_READ(0x01C6, &id)
    if (id == 0xA2A1) {
        *device_type = CREDO_NIGHTHAWK;
        return CR_OK;
    }

    return CR_UNSUPPORTED;

reg_fail:
    cr_log("sdk", CR_LOG_ERROR, "register read fail");
    return ret;
}

const char* cr_device_get_type_name(CredoDevice_t* device) {
    LOGSDK_API();
    if (device == NULL) {
        return "no device";
    }
    switch (device->desc->device_type) {
        case CREDO_BALDEAGLE_400:
            return "baldeagle_400";
        case CREDO_BALDEAGLE_800:
            return "baldeagle_800";

        case CREDO_OWL_400:
            return "owl_400";
        case CREDO_OWL_800:
            return "owl_800";

        case CREDO_BLACKHAWK_400:
            return "blackhawk_400";
        case CREDO_BLACKHAWK_800:
            return "blackhawk_800";

        case CREDO_HERON_P1:
            return "heron_p1";
        case CREDO_HERON_P3:
            return "heron_p3";
        case CREDO_HERON_1P0:
            return "heron_1p0";
        case CREDO_HERON_MR:
            return "heron_mr";

        case CREDO_OSPREY_400:
            return "osprey_400";
        case CREDO_OSPREY_800:
            return "osprey_800";
        case CREDO_OSPREY_AEC:
            return "osprey_aec";

        case CREDO_NUTCRACKER_32:
            return "nutcracker_32";

        case CREDO_KITE_TEST:
            return "kite_test";

        case CREDO_ADMIRAL_TEST:
            return "admiral_test";

        case CREDO_SEAHAWK:
            return "seahawk";

        case CREDO_OSTRICH_1P1:
            return "ostrich_1p1";

        case CREDO_SCREAMING_EAGLE_AEC:
            return "screaming_eagle_aec";
        case CREDO_SCREAMING_EAGLE:
            return "screaming_eagle";

        case CR_DEV_SPARROW_800:
            return "sparrow_800";

        case CR_DEV_VICEROY_TEST:
            return "viceroy_test";

        case CR_DEV_RAVEN_TEST:
            return "raven_test";

        case CR_DEV_NIGHTHAWK_1P0:
            return "nighthawk_1p0";
        case CR_DEV_NIGHTHAWK:
            return "nighthawk";

        case CR_DEV_BLUEJAY:
            return "bluejay";

        case CR_DEV_MONARCH2P1_TEST:
        case CR_DEV_MONARCH2P1:
            return "monarch2p1";

        case CR_DEV_SKIPPER_TEST:
        case CR_DEV_SKIPPER:
            return "skipper";

        case CREDO_FAKE_400:
            return "fake_400";
        case CREDO_FAKE_800:
            return "fake_800";
        case CR_DEV_FAKE_32:
            return "fake_32";
        default:
            return "unknown";
    }
}

CredoError_t cr_device_get_slice_count(CredoDevice_t* device, int* slice_count) {
    LOGSDK_API();
    *slice_count = device->desc->slice_count;
    return CR_OK;
}

CredoError_t cr_device_get_slice(CredoDevice_t* device, int slice_index, CredoSlice_t** slice) {
    LOGSDK_API();
    if (slice_index < 0 || slice_index >= device->desc->slice_count) return CR_FAIL;
    *slice = device->slices[slice_index];
    return CR_OK;
}
