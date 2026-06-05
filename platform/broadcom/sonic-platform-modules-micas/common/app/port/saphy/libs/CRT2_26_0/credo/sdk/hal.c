#include "sdk.h"

static const DeviceCapability_t* hal_registry[MAX_HAL_COUNT] = {0};
static size_t hal_registry_count = 0;

void cr_register_device(const DeviceCapability_t* device_capability) {
    hal_registry[hal_registry_count] = device_capability;
    hal_registry_count += 1;
}

const DeviceCapability_t* cr_device_search(uint32_t device_type) {
    for (size_t i = 0; i < hal_registry_count; i++) {
        if (hal_registry[i]->device_type == device_type) {
            return hal_registry[i];
        }
    }
    return NULL;
}
