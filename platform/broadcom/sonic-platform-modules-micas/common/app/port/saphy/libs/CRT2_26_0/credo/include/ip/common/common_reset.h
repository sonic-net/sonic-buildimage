#ifndef COMMON_RESET_H
#define COMMON_RESET_H

#include "sdk.h"

CredoError_t common_soft_reset(CredoSlice_t* slice);
CredoError_t common_logic_reset(CredoSlice_t* slice);
CredoError_t common_mcu_reset(CredoSlice_t* slice);
CredoError_t common_mcu_reset_hold(CredoSlice_t* slice);
CredoError_t common_reg_reset(CredoSlice_t* slice);
CredoError_t common_set_mdio_mode(CredoSlice_t* slice);
CredoError_t common_detect_mdio_mode(CredoSlice_t* slice);

#endif
