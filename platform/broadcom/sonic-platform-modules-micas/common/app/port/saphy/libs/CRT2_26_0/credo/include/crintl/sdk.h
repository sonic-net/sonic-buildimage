#ifndef CRI_SDK_H
#define CRI_SDK_H

#include "sdk/base.h"

CREDOAPI void cri_sdk_set_post_read(CredoSdk_t* sdk, CredoPostReadRegister_t post_read_func);
CREDOAPI void cri_sdk_set_post_write(CredoSdk_t* sdk, CredoPostWriteRegister_t post_write_func);

#endif
