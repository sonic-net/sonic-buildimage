#include "dii.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Port Operations

CredoError_t cr_port_configure(CredoSlice_t* slice, CredoPortConfig_t* port_config, int force) {
    LOGS_API();
    CALL_HAL(slice, hal_fw_config_port(slice, port_config, force));
}

CredoError_t cr_port_query(CredoSlice_t* slice, uint32_t port_id, CredoPortConfig_t* port_config) {
    CHECK_PORT_VALID(slice, port_id);
    LOGS_API("port_id %u", port_id);
    CALL_HAL(slice, hal_fw_query_port(slice, port_id, port_config));
}

CredoError_t cr_port_is_link_up(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool* up) {
    CHECK_PORT_VALID(slice, port_id);
    LOGS_API("port_id %u", port_id);
    CALL_HAL(slice, hal_port_is_link_up(slice, port_id, side, up));
}

CredoError_t cr_port_destroy(CredoSlice_t* slice, uint32_t port_id) {
    CHECK_PORT_VALID(slice, port_id);
    LOGS_API("port_id %u", port_id);
    CALL_HAL(slice, hal_fw_teardown_port(slice, port_id));
}

CredoError_t cr_port_destroy_all(CredoSlice_t* slice) {
    LOGS_API();
    CALL_HAL(slice, hal_fw_clear_all_port(slice));
}

CredoError_t cr_port_get_option(CredoSlice_t* slice, int port_id, const char* name, int* value) {
    LOGS_API("port_id %d, name %s", port_id, name);
    CALL_HAL(slice, hal_get_option(slice, OPTION_DOMAIN_PORT, port_id, name, value));
}
CredoError_t cr_port_set_option(CredoSlice_t* slice, int port_id, const char* name, int value) {
    LOGS_API("port_id %d, name %s, value %d", port_id, name, value);
    CALL_HAL(slice, hal_set_option(slice, OPTION_DOMAIN_PORT, port_id, name, value));
}
CredoError_t cr_port_index_option_def(CredoSlice_t* slice, int index, CredoOption_t* option) {
    LOGS_API("index %d", index);
    CALL_HAL(slice, hal_get_option_definition(slice, OPTION_DOMAIN_PORT, index, option));
}
CredoError_t cr_port_get_option_count(CredoSlice_t* slice, int* count) {
    LOGS_API();
    CALL_HAL(slice, hal_get_option_count(slice, OPTION_DOMAIN_PORT, count));
}

CredoError_t cr_port_build(CredoSlice_t* slice, uint32_t port_id, const CredoPortSetup_t* setup) {
    LOGS_API("port_id %d", port_id);
    CALL_HAL(slice, hal_port_build(slice, port_id, setup));
}

CredoError_t cr_port_start(CredoSlice_t* slice, uint32_t port_id, bool force) {
    LOGS_API("port_id %d", port_id);
    CALL_HAL(slice, hal_port_start(slice, port_id, force));
}

CredoError_t cr_port_get_setup(CredoSlice_t* slice, uint32_t port_id, bool* started, CredoPortSetup_t* setup) {
    LOGS_API("port_id %d", port_id);
    CALL_HAL(slice, hal_port_get_setup(slice, port_id, started, setup));
}

CredoError_t cr_port_assign_id(CredoSlice_t* slice, uint32_t* port_id) {
    LOGS_API();
    CALL_HAL(slice, hal_port_assign_id(slice, port_id));
}

CredoError_t cr_port_get_paramh(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data) {
    return cr_param_get_paramh(slice, PARAM_DOMAIN_PORT, name, index, data);
}

CredoError_t cr_port_get_param(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data) {
    return cr_param_get_param(slice, PARAM_DOMAIN_PORT, name, index, data);
}

CredoError_t cr_port_set_param(CredoSlice_t* slice, const char* name, int index, const CredoParamData_t* data) {
    return cr_param_set_param(slice, PARAM_DOMAIN_PORT, name, index, data);
}
