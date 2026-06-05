#ifndef COMMON_OPTIONS_H
#define COMMON_OPTIONS_H

#include "sdk.h"

typedef CredoError_t (*OptionGetter)(CredoSlice_t* slice, const char* name, int index, int* value);
typedef CredoError_t (*OptionSetter)(CredoSlice_t* slice, const char* name, int index, int value);

#define OPTION_DEF(name, description, getter, setter) \
    { name, description, getter, setter }

typedef struct {
    const char* name;
    const char* description;
    OptionGetter getter;
    OptionSetter setter;
} OptionHandler_t;

#ifdef HAL_SUPPORT_OPTIONS_PORT
extern const OptionHandler_t option_port_list[];
extern const int option_port_count;
#endif

CredoError_t option_set_option(CredoSlice_t* slice, OptionDomain_t domain, int index, const char* name, int value);
CredoError_t option_get_option(CredoSlice_t* slice, OptionDomain_t domain, int index, const char* name, int* value);
CredoError_t option_get_count(CredoSlice_t* slice, OptionDomain_t domain, int* count);
CredoError_t option_get_def(CredoSlice_t* slice, OptionDomain_t domain, int index, CredoOption_t* option);

#endif
