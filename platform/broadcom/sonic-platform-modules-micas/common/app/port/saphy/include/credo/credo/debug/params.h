#ifndef CR_DBG_PARAMS_H
#define CR_DBG_PARAMS_H

#include "credo/base.h"
#include "credo/params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Indicates the index type of the parameter
 * Some examples are lane, top (only 1), or side index type.
 *
 */
typedef enum { CR_PARAM_INDEX_TOP, CR_PARAM_INDEX_SIDE, CR_PARAM_INDEX_LANE, CR_PARAM_INDEX_PORT } CredoParamIndex_t;

/**
 * @addtogroup SerdesParam
 * @{
 */
#define CR_PARAM_TYPE_CONTROL       "control"
#define CR_PARAM_TYPE_CONTROL_DEBUG "control-debug"
#define CR_PARAM_TYPE_OPTION        "option"
#define CR_PARAM_TYPE_PARAM         "param"
#define CR_PARAM_TYPE_STATUS        "status"

/** @} */

/**
 * @addtogroup SerdesParam
 * @{
 */
#define CR_PARAM_FLAG_VAR_COUNT (1 << 0)
/** @} */

/**
 * @brief SerdesParam Parameter definition
 */
typedef struct {
#ifdef SWIG
%immutable;
#endif
    const char* name;         //!< name of the serdes param
    const char* description;  //!< desription of serdes param
    const char* type;         //!< specific subdomain of parameters
#ifdef SWIG
%mutable;
#endif
    CredoParamIndex_t index_type;  //!< what kind of index does the parameter have
    CredoParamValue_t val_type;    //!< indicate value return type
    bool has_setter;               //!< user can set a value
    bool has_getter;               //!< user can get a value
    int count;                     //!< count 1= not multi, otherwise it is a multi parameter
    uint64_t flags;                //!< special flags about the lane param
} CredoParam_t;

CREDOAPI CredoError_t cr_param_get_param_count(CredoSlice_t* slice, int domain, int* count);

CREDOAPI CredoError_t cr_param_get_param_val_count(CredoSlice_t* slice, int domain, const char* name, int index,
                                                   int* count);

CREDOAPI CredoError_t cr_param_get_param_val_set_count(CredoSlice_t* slice, int domain, const char* name, int index,
                                                       int* count);

CREDOAPI CredoError_t cr_param_index_param_def(CredoSlice_t* slice, int domain, int param_index, CredoParam_t* param);

CREDOAPI CredoError_t cr_param_find_param_def(CredoSlice_t* slice, int domain, const char* name, bool* found,
                                              CredoParam_t* param);

CREDOAPI CredoError_t cr_param_get_param(CredoSlice_t* slice, int domain, const char* name, int index,
                                         CredoParamData_t* data);

CREDOAPI CredoError_t cr_param_get_paramh(CredoSlice_t* slice, int domain, const char* name, int index,
                                          CredoParamData_t* data);

CREDOAPI CredoError_t cr_param_set_param(CredoSlice_t* slice, int domain, const char* name, int index,
                                         const CredoParamData_t* data);

#ifdef __cplusplus
}
#endif

#endif
