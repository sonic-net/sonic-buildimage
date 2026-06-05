#include "project.h"

#include "common/params.h"

#include "utility.h"

#include <string.h>

// internally dispatch single param getter/setter as array with single value
typedef CredoError_t (*ParamSetterInt)(CredoSlice_t* slice, int lane, int value);
typedef CredoError_t (*ParamGetterInt)(CredoSlice_t* slice, int lane, int* value);
typedef CredoError_t (*ParamSetterDbl)(CredoSlice_t* slice, int lane, double value);
typedef CredoError_t (*ParamGetterDbl)(CredoSlice_t* slice, int lane, double* value);
typedef CredoError_t (*ParamSetterMultiInt)(CredoSlice_t* slice, int lane, const int value[]);
typedef CredoError_t (*ParamGetterMultiInt)(CredoSlice_t* slice, int lane, int value[]);
typedef CredoError_t (*ParamSetterMultiDbl)(CredoSlice_t* slice, int lane, const double value[]);
typedef CredoError_t (*ParamGetterMultiDbl)(CredoSlice_t* slice, int lane, double value[]);

static bool is_valid_index(CredoSlice_t* slice, const ParamHandler_t* handler, int index) {
    switch (handler->index_type) {
        case CR_PARAM_INDEX_TOP:
            return index == 0;
        case CR_PARAM_INDEX_SIDE:
            return index == CR_SIDE_LINE || index == CR_SIDE_HOST;
        case CR_PARAM_INDEX_LANE:
            return index >= 0 && index < slice->desc->lane_count;
        case CR_PARAM_INDEX_PORT:
            return index >= 0 && index < slice->desc->port_count;
        default:
            LOGS_ERROR("Invalid index_type %d", handler->index_type);
            // unknown type that needs a validator
            return false;
    }
}

static const ParamHandler_t* find_param_domain_list(CredoSlice_t* slice, ParamDomain_t domain, int* count) {
    switch (domain) {
#if HAL_SUPPORT_PARAMS_SLICE
        case PARAM_DOMAIN_SLICE:
            *count = param_slice_count;
            return param_slice_list;
#endif
#if HAL_SUPPORT_PARAMS_PORT
        case PARAM_DOMAIN_PORT:
            *count = param_port_count;
            return param_port_list;
#endif
#if HAL_SUPPORT_PARAMS_LANE
        case PARAM_DOMAIN_LANE:
            *count = param_lane_count;
            return param_lane_list;
#endif
#if HAL_SUPPORT_PARAMS_SERDES
        case PARAM_DOMAIN_SERDES:
            *count = param_serdes_count;
            return param_serdes_list;
#endif

        default:
            *count = 0;
            return NULL;
    }
}

static CredoError_t find_param_handler(CredoSlice_t* slice, ParamDomain_t domain, const char* name,
                                       const ParamHandler_t** param) {
    int param_domain_count = 0;
    const ParamHandler_t* param_domain_list = find_param_domain_list(slice, domain, &param_domain_count);
    if (param_domain_list == NULL || param_domain_count <= 0) return CR_NOTIMPLEMENTED;

    for (int i = 0; i < param_domain_count; i++) {
        if (strcmp(name, param_domain_list[i].name) == 0) {
            *param = &param_domain_list[i];
            return CR_OK;
        }
    }
    return CR_INVALID_ARGS;
}

CredoError_t param_get_data(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                            CredoParamData_t* data) {
    const ParamHandler_t* param = NULL;
    ERR_PROPS(find_param_handler(slice, domain, name, &param));
    if (!is_valid_index(slice, param, index)) return CR_INVALID_ARGS;
    if (param->getter == NULL) {
        LOGS_ERROR("Parameter %s has no getter", name);
        return CR_NOTIMPLEMENTED;
    }
    // relax restriction between int and unsigned
    if (data->type != param->val_type &&
        !((data->type == CR_PARAM_VAL_INT || data->type == CR_PARAM_VAL_UINT) &&
          (param->val_type == CR_PARAM_VAL_INT || param->val_type == CR_PARAM_VAL_UINT))) {
        LOGS_ERROR("Parameter %s provided incorrect val_type %d (expected %d)", param->name, data->type,
                   param->val_type);
        return CR_INVALID_ARGS;
    }

    int count = param->count;
    if (param->counter != NULL) {
        param->counter(slice, index, &count);
    }

    if (data->count < count) {
        LOGS_ERROR("Invalid get count for '%s' %d (expected >= %d)", param->name, (int)data->count, count);
        return CR_INVALID_ARGS;
    }
    data->type = param->val_type;
    data->count = count;

    return ((ParamGetterInt)(param->getter))(slice, index, data->value.i);
}

CredoError_t param_set_data(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                            const CredoParamData_t* data) {
    const ParamHandler_t* param = NULL;
    ERR_PROPS(find_param_handler(slice, domain, name, &param));
    if (!is_valid_index(slice, param, index)) return CR_INVALID_ARGS;
    if (param->setter == NULL) {
        LOGS_ERROR("Parameter %s has no setter", name);
        return CR_NOTIMPLEMENTED;
    }
    // relax restriction between int and unsigned
    if (data->type != param->val_type &&
        !((data->type == CR_PARAM_VAL_INT || data->type == CR_PARAM_VAL_UINT) &&
          (param->val_type == CR_PARAM_VAL_INT || param->val_type == CR_PARAM_VAL_UINT))) {
        LOGS_ERROR("Parameter %s provided incorrect val_type %d (expected %d)", param->name, data->type,
                   param->val_type);
        return CR_INVALID_ARGS;
    }
    int var_param_count = param->count;

    if (param->set_counter != NULL) {  // use set counter if available
        param->set_counter(slice, index, &var_param_count);
    } else if (param->counter != NULL) {
        param->counter(slice, index, &var_param_count);
    }

    if (data->count != var_param_count) {
        LOGS_ERROR("Invalid set count for '%s' %d (expected %d)", param->name, (int)data->count, var_param_count);
        return CR_INVALID_ARGS;
    }
    if (param->count > 1) {
        return ((ParamSetterMultiInt)(param->setter))(slice, index, data->value.i);
    } else {
        switch (param->val_type) {
            case CR_PARAM_VAL_INT:
            case CR_PARAM_VAL_UINT:
                return ((ParamSetterInt)(param->setter))(slice, index, *data->value.i);
            case CR_PARAM_VAL_FLOAT:
                return ((ParamSetterDbl)(param->setter))(slice, index, *data->value.d);
            default:
                return CR_NOTIMPLEMENTED;
        }
    }
}

CredoError_t param_get_param_count(CredoSlice_t* slice, ParamDomain_t domain, int* count) {
    int param_count = 0;
    const ParamHandler_t* param_domain_list = find_param_domain_list(slice, domain, &param_count);
    if (param_domain_list == NULL || param_count <= 0) return CR_NOTIMPLEMENTED;
    *count = param_count;
    return CR_OK;
}

// user passes an increasing parameter index until NULL
CredoError_t param_index_param_def(CredoSlice_t* slice, ParamDomain_t domain, int param_index, CredoParam_t* param) {
    int param_domain_count = 0;
    const ParamHandler_t* param_domain_list = find_param_domain_list(slice, domain, &param_domain_count);

    if (param_domain_count == 0 || param_domain_list == NULL) {
        return CR_NOTIMPLEMENTED;
    }

    if (param_index >= param_domain_count || param_index < 0) {
        return CR_INVALID_ARGS;
    }

    const ParamHandler_t* param_handler = &param_domain_list[param_index];
    param->name = param_handler->name;
    param->type = param_handler->type;
    param->description = param_handler->description;
    param->index_type = param_handler->index_type;
    param->val_type = param_handler->val_type;
    param->count = param_handler->count;
    param->has_getter = (param_handler->getter != NULL);
    param->has_setter = (param_handler->setter != NULL);
    param->flags = param_handler->flags;
    return CR_OK;
}
// returns param if it exists otherwise NULL (user provides the struct)
CredoError_t param_find_param_def(CredoSlice_t* slice, ParamDomain_t domain, const char* name, bool* found,
                                  CredoParam_t* param) {
    const ParamHandler_t* param_handler = NULL;
    CredoError_t err = find_param_handler(slice, domain, name, &param_handler);

    if (err != CR_OK) {
        *found = false;
        return CR_OK;
    }
    *found = true;
    param->name = param_handler->name;
    param->description = param_handler->description;
    param->type = param_handler->type;
    param->val_type = param_handler->val_type;
    param->index_type = param_handler->index_type;
    param->count = param_handler->count;
    param->has_getter = (param_handler->getter != NULL);
    param->has_setter = (param_handler->setter != NULL);
    return CR_OK;
}

CredoError_t param_get_param_val_count(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                                       int* count) {
    const ParamHandler_t* param_handler = NULL;
    ERR_PROPS(find_param_handler(slice, domain, name, &param_handler));

    if (param_handler->counter == NULL) {
        *count = param_handler->count;
        return CR_OK;
    }
    return param_handler->counter(slice, index, count);
}

CredoError_t param_get_param_val_set_count(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                                           int* count) {
    const ParamHandler_t* param_handler = NULL;
    ERR_PROPS(find_param_handler(slice, domain, name, &param_handler));

    if (param_handler->set_counter != NULL) {
        return param_handler->set_counter(slice, index, count);
    } else if (param_handler->counter != NULL) {
        return param_handler->counter(slice, index, count);
    } else {
        *count = param_handler->count;
        return CR_OK;
    }
}
