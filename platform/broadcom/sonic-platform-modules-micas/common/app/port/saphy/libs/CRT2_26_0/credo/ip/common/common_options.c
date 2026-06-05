#include "project.h"

#include "common/options.h"

#include "utility.h"

#include <string.h>

static bool is_valid_index(CredoSlice_t* slice, OptionDomain_t domain, int index) {
    switch (domain) {
        case OPTION_DOMAIN_SLICE:
            return index == 0;
        case OPTION_DOMAIN_LANE:
            return index >= 0 && index < slice->desc->lane_count;
        case OPTION_DOMAIN_PORT:
            return index >= 0 && index < slice->desc->port_count;
        default:
            LOGS_ERROR("Invalid domain %d", domain);
            // unknown type that needs a validator
            return false;
    }
}

static char* option_domain_to_string(OptionDomain_t domain) {
    switch (domain) {
        case OPTION_DOMAIN_SLICE:
            return "slice";
        case OPTION_DOMAIN_PORT:
            return "port";
        case OPTION_DOMAIN_LANE:
            return "lane";
        default:
            return "?";
    }
}

static const OptionHandler_t* find_option_domain_list(CredoSlice_t* slice, OptionDomain_t domain, int* count) {
    switch (domain) {
#if HAL_SUPPORT_OPTIONS_PORT
        case OPTION_DOMAIN_PORT:
            *count = option_port_count;
            return option_port_list;
#endif
        default:
            *count = 0;
            return NULL;
    }
}

static CredoError_t find_option_handler(CredoSlice_t* slice, OptionDomain_t domain, const char* name,
                                        const OptionHandler_t** option) {
    int option_domain_count = 0;
    const OptionHandler_t* option_domain_list = find_option_domain_list(slice, domain, &option_domain_count);
    if (option_domain_list == NULL || option_domain_count <= 0) return CR_NOTIMPLEMENTED;

    for (int i = 0; i < option_domain_count; i++) {
        if (strcmp(name, option_domain_list[i].name) == 0) {
            *option = &option_domain_list[i];
            return CR_OK;
        }
    }
    return CR_INVALID_ARGS;
}

CredoError_t option_set_option(CredoSlice_t* slice, OptionDomain_t domain, int index, const char* name, int value) {
    const OptionHandler_t* option = NULL;
    ERR_PROPS(find_option_handler(slice, domain, name, &option));
    if (option->setter == NULL) {
        LOGS_ERROR("%s option \"%s\" has no setter", option_domain_to_string(domain), name);
        return CR_NOTIMPLEMENTED;
    }
    if (!is_valid_index(slice, domain, index)) {
        LOGS_ERROR("%s option \"%s\" invalid index %d", option_domain_to_string(domain), name, index);
    }
    return option->setter(slice, name, index, value);
}
CredoError_t option_get_option(CredoSlice_t* slice, OptionDomain_t domain, int index, const char* name, int* value) {
    const OptionHandler_t* option = NULL;
    ERR_PROPS(find_option_handler(slice, domain, name, &option));
    if (option->getter == NULL) {
        LOGS_ERROR("%s option \"%s\" has no getter", option_domain_to_string(domain), name);
        return CR_NOTIMPLEMENTED;
    }
    if (!is_valid_index(slice, domain, index)) {
        LOGS_ERROR("%s option \"%s\" invalid index %d", option_domain_to_string(domain), name, index);
    }
    return option->getter(slice, name, index, value);
}
CredoError_t option_get_count(CredoSlice_t* slice, OptionDomain_t domain, int* count) {
    int option_count = 0;
    const OptionHandler_t* option_domain_list = find_option_domain_list(slice, domain, &option_count);
    if (option_domain_list == NULL || option_count < 0) return CR_NOTIMPLEMENTED;
    *count = option_count;
    return CR_OK;
}
CredoError_t option_get_def(CredoSlice_t* slice, OptionDomain_t domain, int index, CredoOption_t* option) {
    int option_count = 0;
    const OptionHandler_t* option_domain_list = find_option_domain_list(slice, domain, &option_count);
    if (option_domain_list == NULL || option_count < 0) return CR_NOTIMPLEMENTED;

    if (index >= option_count || index < 0) {
        return CR_INVALID_ARGS;
    }
    const OptionHandler_t* option_handler = &option_domain_list[index];
    option->name = option_handler->name;
    option->description = option_handler->description;
    return CR_OK;
}
