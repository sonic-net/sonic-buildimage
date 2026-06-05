/*
 * SDK context
 */
#include "sdk_rev.h"

#include "crintl/sdk.h"
#include "sdk.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

CredoError_t cr_sdk_create(const CredoSdkConfig_t* config, CredoSdk_t** sdk_return) {
    CredoSdk_t* sdk = calloc(1, sizeof(CredoSdk_t));
    if (sdk == NULL) return CR_OUT_OF_MEMORY;
    sdk->read_register = config->read_register;
    sdk->write_register = config->write_register;
    sdk->burst_read_register = NULL;
    sdk->burst_write_register = NULL;
    *sdk_return = sdk;
    return CR_OK;
}

void cr_sdk_set_burst_read(CredoSdk_t* sdk, CredoReadRegisterBurst_t burst_read_func) {
    sdk->burst_read_register = burst_read_func;
}

void cr_sdk_set_burst_write(CredoSdk_t* sdk, CredoWriteRegisterBurst_t burst_write_func) {
    sdk->burst_write_register = burst_write_func;
}

void cr_sdk_set_broadcast_write(CredoSdk_t* sdk, CredoWriteRegisterBroadcast_t func) {
    sdk->broadcast_write_register = func;
}

void cr_sdk_set_broadcast_burst_write(CredoSdk_t* sdk, CredoWriteRegisterBroadcastBurst_t func) {
    sdk->burst_broadcast_write_register = func;
}

void cr_sdk_set_loglevel(CredoSdk_t* sdk, CredoLogLevel_t max_level) {
    sdk->max_log_level = max_level;
}

CredoLogLevel_t cr_sdk_get_loglevel(CredoSdk_t* sdk) {
    return sdk->max_log_level;
}

CredoError_t cr_sdk_destroy(CredoSdk_t* sdk) {
    free(sdk);
    return CR_OK;
}

uint32_t cr_sdk_version(void) {
    uint32_t rev =
        ((CREDO_SDK_REV_MAJOR & 0xFF) << 16) | ((CREDO_SDK_REV_MINOR & 0xFF) << 8) | ((CREDO_SDK_REV_PATCH & 0xFF));
    return rev;
}

char const* cr_sdk_version_str(void) {
    return CREDO_SDK_REV;
}

void cri_sdk_set_post_read(CredoSdk_t* sdk, CredoPostReadRegister_t post_read_func) {
    sdk->post_read = post_read_func;
}

void cri_sdk_set_post_write(CredoSdk_t* sdk, CredoPostWriteRegister_t post_write_func) {
    sdk->post_write = post_write_func;
}
