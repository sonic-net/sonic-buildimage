#ifndef CREDO_PARAMS_H
#define CREDO_PARAMS_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Indicates the return type of the serdes parameter
 *
 */
typedef enum {
    CR_PARAM_VAL_INT = 0,
    CR_PARAM_VAL_UINT = 1,
    CR_PARAM_VAL_FLOAT = 2,  //!< float means c double
} CredoParamValue_t;

typedef union {
    unsigned* u;
    int* i;
    double* d;
} CredoParamDataBuf_t;

typedef struct {
    CredoParamDataBuf_t value;
    CredoParamValue_t type;
    size_t count;
} CredoParamData_t;

#define CR_PDATA_I(data, cnt)     \
    (&(CredoParamData_t){         \
        .type = CR_PARAM_VAL_INT, \
        .count = (cnt),           \
        .value = {.i = (data)},   \
    })
#define CR_PDATA_U(data, cnt)      \
    (&(CredoParamData_t){          \
        .type = CR_PARAM_VAL_UINT, \
        .count = (cnt),            \
        .value = {.u = (data)},    \
    })
#define CR_PDATA_F(data, cnt)       \
    (&(CredoParamData_t){           \
        .type = CR_PARAM_VAL_FLOAT, \
        .count = (cnt),             \
        .value = {.d = (data)},     \
    })

// utility for c11 that enables simple
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

///@private
static inline CredoParamData_t* cr_pdata_i(CredoParamData_t* pdata, int* data, size_t count) {
    pdata->count = count;
    pdata->value.i = data;
    pdata->type = CR_PARAM_VAL_INT;
    return pdata;
}

///@private
static inline CredoParamData_t* cr_pdata_u(CredoParamData_t* pdata, unsigned* data, size_t count) {
    pdata->count = count;
    pdata->value.u = data;
    pdata->type = CR_PARAM_VAL_UINT;
    return pdata;
}

///@private
static inline CredoParamData_t* cr_pdata_f(CredoParamData_t* pdata, double* data, size_t count) {
    pdata->count = count;
    pdata->value.d = data;
    pdata->type = CR_PARAM_VAL_FLOAT;
    return pdata;
}

#define CR_PDATA(val, count)                                                                                      \
    (_Generic((val), int*: cr_pdata_i, double*: cr_pdata_f, unsigned*: cr_pdata_u)(&(CredoParamData_t){0}, (val), \
                                                                                   (count)))

#endif

#define CR_PDATA_FREE(d) free((d).value.i);

#ifdef __cplusplus
}
#endif

#endif
