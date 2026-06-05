#ifndef CR_DBG_OPTION_H
#define CR_DBG_OPTION_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Slice Option
 */
typedef struct {
#ifdef SWIG
%immutable;
#endif
    const char* name;         //!< option name
    const char* description;  //!< option description
#ifdef SWIG
%mutable;
#endif
} CredoSliceOption_t;

/**
 * @brief Option
 */
typedef struct {
#ifdef SWIG
%immutable;
#endif
    const char* name;         //!< option name
    const char* description;  //!< option description
#ifdef SWIG
%mutable;
#endif
} CredoOption_t;

/**
 * @brief Lane Option
 */
typedef struct {
#ifdef SWIG
%immutable;
#endif
    const char* name;         //!< option name
    const char* description;  //!< option description
#ifdef SWIG
%mutable;
#endif
} CredoLaneOption_t;

#ifdef __cplusplus
}
#endif

#endif
