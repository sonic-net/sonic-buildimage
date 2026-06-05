#include "project.h"

#include "common/common_init.h"

#include "utility.h"

// perform basic initialization on slice data
void common_init_slice_data(CredoSlice_t* slice) {
    // configure prbs timer for each lane
    for (int lane = 0; lane < MAX_CREDO_LANES; lane++) {
        get_time(&slice->data->prbs_timer[lane]);
        get_time(&slice->data->prbs_fec_timer[lane]);
    }

    // slice->sdk == NULL means it is not re-init case
    if (slice->sdk == NULL) {
        slice->data->base_offset = 0;
        slice->data->fw_cmd_timeout = FW_CMD_TIMEOUT;
        slice->data->fw_unload_timeout = FW_UNLOAD_TIMEOUT;
        slice->data->is_mdio_push_pull = false;
        slice->data->top_cal_timeout = TOP_CAL_TIMEOUT;
        slice->data->fw_spiflash_load_timeout = FW_LOAD_SPI_TIMEOUT;
        slice->data->ecid_captured = false;
        slice->data->refclk_hz = 0;
    }
}

CredoError_t common_slice_set_mdio_mode(CredoSlice_t* slice, bool is_push_pull) {
    slice->data->is_mdio_push_pull = is_push_pull;
    return CR_OK;
}
