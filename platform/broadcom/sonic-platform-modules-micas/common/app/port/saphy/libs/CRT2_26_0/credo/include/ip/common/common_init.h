#ifndef COMMON_INIT_H
#define COMMON_INIT_H

#include "sdk.h"

#ifndef TOP_CAL_TIMEOUT
#define TOP_CAL_TIMEOUT (150 * 1000)  // us
#endif

#ifndef FW_UNLOAD_TIMEOUT
#define FW_UNLOAD_TIMEOUT (100 * 1000)
#endif

#ifndef FW_LOAD_SPI_TIMEOUT
#define FW_LOAD_SPI_TIMEOUT ((500 * 1000) * 16)  // 500 ms * 16 MCU boot chain
#endif

void common_init_slice_data(CredoSlice_t* slice);
CredoError_t common_slice_set_mdio_mode(CredoSlice_t* slice, bool is_push_pull);

#endif  // COMMON_INIT_H
