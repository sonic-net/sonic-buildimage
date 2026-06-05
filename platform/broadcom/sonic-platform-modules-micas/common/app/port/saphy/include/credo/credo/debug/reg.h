#ifndef CR_DBG_REG_H
#define CR_DBG_REG_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CR_REG_VERIFY_BURST = 0x1,            //!< Use burst register access
    CR_REG_VERIFY_USE_DURATION = 0x2,     //!< Verify for duration instead of total tests
    CR_REG_VERIFY_OVERRIDE_ADDRESS = 0x4  //!< Override specific register to use in testing

} CredoRegVerifyFlags_t;

typedef struct {
    uint32_t flags;
    uint32_t test_count;   //!< total test count
    double duration_sec;   //!< how long to run test (with flag set)
    unsigned address;      //!< specific address to test
    unsigned burst_width;  //!< how wide burst test should
} CredoRegVerifyConfig_t;

typedef struct {
    uint32_t reg_count;
    uint32_t fail_count;
    double duration_sec;
} CredoRegVerifyStats_t;

CREDOAPI CredoError_t cr_reg_verify(CredoSlice_t* slice, const CredoRegVerifyConfig_t* config,
                                    CredoRegVerifyStats_t* stats);

#ifdef __cplusplus
}
#endif

#endif
