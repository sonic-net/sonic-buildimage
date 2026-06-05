#include "bh_device.h"
#include "bh_functions.h"
#include "blackhawk.h"

#include "common/common_firmware.h"
#include "common/common_init.h"
#include "common/common_reset.h"
#include "condor_lp/condor_lp_serdes.h"

#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CredoError_t bh_slice_data_init(CredoSlice_t* slice) {
    static int default_tx_taps[7] = {0, 0, 0, 112, 0, 0, 0};  // HW default value
    BhSlice_t* bh_slice = (BhSlice_t*)slice;
    // zero-set all of blackhawk
    memset(&(bh_slice->lane_mode), 0, sizeof(BhSlice_t) - sizeof(CredoSlice_t));
    common_init_slice_data(slice);

    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        bh_slice->em_flags[lane] = 0;
        memcpy(bh_slice->tx_taps[lane], default_tx_taps, sizeof(int) * 7);
    }

    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        bh_slice->port_info[port_id].configured = false;
        bh_slice->port_info[port_id].port_config.port_id = CR_PORT_UNCONFIGURED;
    }

    bh_slice->fw_feature = 0;
    bh_slice->fw_feature_captured = false;

    return CR_OK;
}

CredoError_t bh_allocate_slice(CredoSdk_t* sdk, CredoSlice_t** handle) {
    BhSlice_t* slice = calloc(1, sizeof(BhSlice_t));
    CredoSliceData_t* data = malloc(sizeof(CredoSliceData_t));
    if (slice == NULL || data == NULL) return CR_OUT_OF_MEMORY;

    CredoSlice_t* crslice = (CredoSlice_t*)slice;
    crslice->data = data;
    *handle = crslice;
    common_init_slice_data(crslice);
    return CR_OK;
}

void bh_free_slice(CredoSlice_t* handle) {
    free(handle->data);
    free(handle);
}

CredoError_t bh_slice_warm_init(CredoSlice_t* slice) {
    BhSlice_t* bh_slice = (BhSlice_t*)slice;
    LOGS_INFO("[Slice init] performing a warm initialization");

    LOGS_DEBUG("[Slice init] capturing lane mode information from firmware");
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        ERR_PROPS(condor_lp_get_tx_taps(slice, lane, bh_slice->tx_taps[lane]));
        ERR_PROPS(bh_update_lane_mode_ex(slice, lane, true));
    }
    LOGS_DEBUG("[Slice init] capturing port information from firmware");
    CredoPortConfig_t port_config = {0};  // value unused as we are just updating the store
    for (unsigned port = 0; port < BH_MAX_PORT; port++) {
        ERR_PROPS(bh_port_info(slice, port, &port_config, true));
    }
    return CR_OK;
}

CredoError_t bh_slice_init(CredoSlice_t* slice, const CredoSliceConfig_t* config) {
    unsigned fw_status = 0;

    ERR_PROPS(hal_slice_data_init(slice));
    // properly configure the state if user is re-initializing

    if (config->init_type == CR_INIT_NONE) {
        return CR_OK;
    }

    if (config->init_type == CR_INIT_WARM) {
        ERR_PROPS(bh_fw_get_status(slice, &fw_status));
        if (fw_status == 0) {
            LOGS_WARN("[Slice init] firmware is not running, skip warm init");
            return CR_OK;
        }

        ERR_PROPS(bh_slice_warm_init(slice));

        return CR_OK;
    } else if (config->init_type == CR_INIT_NO_FIRMWARE) {
        ERR_PROPS(bh_fw_get_status(slice, &fw_status));
        /* Do a soft reset */
        ERR_PROP(common_soft_reset(slice));

        /* if there have firmware running before, try to wait fw ready */
        if (fw_status == 1) {
            common_fw_wait_ready(slice);  // skip error
        }
    } else if (config->init_type == CR_INIT_FULL) {
        /* Load firmware */
        ERR_PROPS(bh_fw_download(slice, config->firmware_filename));
    } else if (config->init_type == CR_INIT_SPIFLASH) {
        CredoTime_t start_time;
        get_time(&start_time);
        LOGS_DEBUG("[SPI-Flash] checking for boot load timeout of %dms", FW_LOAD_SPI_TIMEOUT / 1000);
        ERR_PROPS(hal_fw_unload(slice));
        ERR_PROPS(hal_soft_reset(slice));
        ERR_PROPS(hal_fw_wait_magic_word(slice, FW_LOAD_SPI_TIMEOUT));
        ERR_PROPS(common_fw_wait_ready(slice));

        LOGS_INFO("[Firmware load] Succeed to load firmware from SPI flash using %lu usecs",
                  (unsigned long)us_passed(&start_time));
        unsigned fw_date = 0xFFFF, fw_hash = 0xFFFFFF, fw_crc = 0xFFFF;
        hal_fw_date(slice, &fw_date);
        hal_fw_hash(slice, &fw_hash);
        hal_fw_crc(slice, &fw_crc);
        unsigned fw_ver = 0;
        char fw_ver_str[32] = "Unknown";
        if (hal_fw_ver(slice, &fw_ver) == CR_OK) {
            stringify_firmware_version(fw_ver, fw_ver_str);
        }
        LOGS_INFO("[Firmware Info] Version: %s, Date Code: 0x%04x, Hash Code: 0x%06x, CRC Code: 0x%04x", fw_ver_str,
                  fw_date, fw_hash, fw_crc);
    }

    int acfg_state = 0;
    bh_fw_get_acfg_status(slice, &acfg_state, NULL);
    if (acfg_state == TOP_DEBUG_ACFG_STATE_OK) return CR_OK;

    /* Initialize Blackhawk slice */
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        ERR_PROPS(condor_lp_set_tx_msb(slice, lane, 0));
        ERR_PROPS(condor_lp_set_rx_msb(slice, lane, 0));
        ERR_PROPS(condor_lp_set_rx_prbs_nrz(slice, lane, 0, CR_PRBS31));
        ERR_PROPS(bh_fec_analyzer_set_config(slice, lane, 0, NULL));
    }

    return CR_OK;
}

CredoError_t bh_slice_get_type(CredoSlice_t* slice, CredoSliceType_t* slice_type) {
    uint64_t version = 0;
    ERR_PROP(condor_lp_get_version_id(slice, 0, &version));

    if ((version >> 20) != 0x216100A) return CR_FAIL;

    if ((version & 0xF) == 1) {
        *slice_type = CREDO_BLACKHAWK_DC;
    } else if ((version & 0xF) == 3) {
        *slice_type = CREDO_BLACKHAWK_AC;
    } else {
        *slice_type = CREDO_BLACKHAWK;
    }
    return CR_OK;
}

CredoError_t bh_slice_get_vsensor(CredoSlice_t* slice, double* vsensor) {
    uint32_t data;
    for (int i = 0; i < BH_MAX_VSENSOR; i++) {
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_AUTO_VS, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_RSTB_VS, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_CLK_CNT_VS, 0xa));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_CLK_SEL_VS, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_SDE, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_RSTN, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_RUN, 0));

        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_RUN, 1));
        sleep_us(1000);

        /* It seems that rdy bit is always zero, need check
        uint32_t rdy;
        int retry = 0;
        do {
            sleep_us(100);
            ERR_PROPS( readReg(slice, REG_TOP_SENSOR_VS_RDY, &rdy));
            if (rdy == 1) break;
            retry++;
        } while(retry < 10);
        if (retry == 10) {
            LOGS_WARN( "Get vsensor %d timeout", i);
        }
        */

        ERR_PROPS(readReg(slice, REG_TOP_SENSOR_VS_DOUT, &data));
        vsensor[i] = 1.2077 / 5 * (6.0f * data / 16384 - 3.0f / 16384 - 1.0f);
    }

    return CR_OK;
}

CredoError_t bh_slice_save_setup(CredoSlice_t* slice, const char* file_path) {
    FILE* fp = fopen(file_path, "w");
    const RegHive_t* hive = NULL;
    CredoError_t ret = CR_OK;
    unsigned count = 0;

    if (fp == NULL) {
        LOGS_ERROR("file %s open fail, %s\n", file_path, strerror(errno));
        return CR_FAIL;
    }

    ERR_CATCH((ret = hal_slice_get_reghive(slice, HIVENAME_TOPPLL, &hive)), goto exit);
    fprintf(fp, "##---------------------------------------##\n");
    fprintf(fp, "##                TOP PLL                ##\n");
    fprintf(fp, "##---------------------------------------##\n");
    for (unsigned offset = 0; offset < 0x16; offset++) {
        unsigned value;
        unsigned address = cr_addr_reg(slice, 0, hive, offset);

        ERR_CATCH((ret = readTop(slice, address, &value)), goto exit);
        count++;
        fprintf(fp, "%04X %04X\n", address, value);
    }
    fprintf(fp, "\n\n");

    ERR_CATCH((ret = hal_slice_get_reghive(slice, HIVENAME_TOP, &hive)), goto exit);
    fprintf(fp, "##---------------------------------------##\n");
    fprintf(fp, "##                  TOP                  ##\n");
    fprintf(fp, "##---------------------------------------##\n");
    for (unsigned offset = 0; offset < 0x100; offset++) {
        unsigned value;
        unsigned address = cr_addr_reg(slice, 0, hive, offset);

        ERR_CATCH((ret = readTop(slice, address, &value)), goto exit);
        count++;
        fprintf(fp, "%04X %04X\n", address, value);
    }
    fprintf(fp, "\n\n");

    ERR_CATCH((ret = hal_slice_get_reghive(slice, HIVENAME_SERDES_RX, &hive)), goto exit);
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        fprintf(fp, "##---------------------------------------##\n");
        fprintf(fp, "##                 LANE %3d              ##\n", lane);
        fprintf(fp, "##---------------------------------------##\n");
        for (unsigned offset = 0; offset < (1 << hive->shift); offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, lane, hive, offset);

            ERR_CATCH((ret = readTop(slice, address, &value)), goto exit);
            count++;
            fprintf(fp, "%04X %04X\n", address, value);
        }
        fprintf(fp, "\n\n");
    }

    ERR_CATCH((ret = hal_slice_get_reghive(slice, HIVENAME_RSFEC, &hive)), goto exit);
    for (int index = 0; index < 8; index++) {
        fprintf(fp, "##---------------------------------------##\n");
        fprintf(fp, "##    RSFEC IEEE index %d (0xC8-0xFF)     ##\n", index);
        fprintf(fp, "##---------------------------------------##\n");
        for (unsigned offset = 0xC8; offset < 0x100; offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, index, hive, offset);

            ERR_CATCH((ret = readTop(slice, address, &value)), goto exit);
            count++;
            fprintf(fp, "%04X %04X\n", address, value);
        }
        fprintf(fp, "\n\n");
    }

    for (int index = 0; index < 8; index++) {
        fprintf(fp, "##---------------------------------------##\n");
        fprintf(fp, "##   RSFEC IEEE 2 index %d (0x100-0x11B)  ##\n", index);
        fprintf(fp, "##---------------------------------------##\n");
        for (unsigned offset = 0x100; offset < 0x11C; offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, index, hive, offset + 0x300);

            ERR_CATCH((ret = readTop(slice, address, &value)), goto exit);
            count++;
            fprintf(fp, "%04X %04X\n", address, value);
        }
        fprintf(fp, "\n\n");
    }

    for (int index = 0; index < 8; index++) {
        fprintf(fp, "##---------------------------------------##\n");
        fprintf(fp, "##    RSFEC Custom index %d (0x00-0xD4)   ##\n", index);
        fprintf(fp, "##---------------------------------------##\n");
        for (unsigned offset = 0; offset < 0xD5; offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, index, hive, offset + 0x800);

            ERR_CATCH((ret = readTop(slice, address, &value)), goto exit);
            count++;
            fprintf(fp, "%04X %04X\n", address, value);
        }
        fprintf(fp, "\n\n");
    }

exit:
    LOGS_INFO("%d registers were read to file %s", count, file_path);
    if (fp) fclose(fp);
    return ret;
}

#define DIVIDER_COUNT 8
static unsigned divider_index_map[DIVIDER_COUNT] = {32, 64, 128, 256, 512, 1024, 2048, 4096};
static unsigned compute_divider_index(CredoSlice_t* slice, unsigned divider) {
    for (unsigned div_index = 0; div_index < DIVIDER_COUNT; div_index++) {
        if (divider_index_map[div_index] == divider) return div_index;
    }
    LOGS_WARN("Unknown divider %d, using %d", divider, divider_index_map[0]);
    return 0;
}

CredoError_t bh_slice_check_clock_output(CredoSlice_t* slice, unsigned clock_output, unsigned* state, unsigned* lane,
                                         unsigned* divider) {
    unsigned clk_en;  // fw may controlled
    unsigned cko_en;  // user controlled
    // check top clock is enabled
    ERR_PROPS(readReg(slice, REG_TOP_EN_CKO, &clk_en));

    if (!clk_en) {
        *state = CKO_STATE_UNCONFIG;
        return CR_OK;
    }
    unsigned divider_index;
    if (clock_output == 0) {
        ERR_PROPS(readReg(slice, REG_TOP_CDR_CLK_EN0, &clk_en));
        ERR_PROPS(readReg(slice, REG_TOP_EN_CKO_DIFF, &cko_en));
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX(0), lane));
        ERR_PROPS(readReg(slice, REG_TOP_CDR_DIV_MUX0, &divider_index));
    } else if (clock_output == 1) {
        ERR_PROPS(readReg(slice, REG_TOP_CDR_CLK_EN1, &clk_en));
        ERR_PROPS(readReg(slice, REG_TOP_EN_CKO_SG1, &cko_en));
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX(1), lane));
        ERR_PROPS(readReg(slice, REG_TOP_CDR_DIV_MUX1, &divider_index));
    } else if (clock_output == 2) {
        ERR_PROPS(readReg(slice, REG_TOP_CDR_CLK_EN2, &clk_en));
        ERR_PROPS(readReg(slice, REG_TOP_EN_CKO_SG2, &cko_en));
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX(2), lane));
        ERR_PROPS(readReg(slice, REG_TOP_CDR_DIV_MUX2, &divider_index));
    } else {
        LOGS_ERROR("Invalid clock output");
        return CR_INVALID_ARGS;
    }

    if (divider_index < DIVIDER_COUNT) {
        *divider = divider_index_map[divider_index];
    } else {
        LOGS_WARN("Unknown divider index %d", divider_index);
    }

    if (cko_en == 1 && clk_en == 1) {
        *state = CKO_STATE_ACTIVE;
    } else if (cko_en == 1 && clk_en == 0) {
        *state = CKO_STATE_SQUELCHED;
    } else if (cko_en == 0 && clk_en == 0) {
        *state = CKO_STATE_UNCONFIG;
    } else {
        *state = CKO_STATE_INVALID;
    }
    return CR_OK;
}

static CredoError_t bh_slice_disable_only_clock_output(CredoSlice_t* slice, unsigned clock_output) {
    unsigned lane = 0;

    if (clock_output == 0) {
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX(0), &lane));
        ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO_DIFF, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_EN0, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_VREF_CLK_DIFF, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_BYPASS_DIFFREG, 0));
    } else if (clock_output == 1) {
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX(1), &lane));
        ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO_SG1, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_EN1, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_VREF_CLK_SG1, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_BYPASS_SG1REG, 0));
    } else if (clock_output == 2) {
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX(2), &lane));
        ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO_SG2, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_EN2, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_VREF_CLK_SG2, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_BYPASS_SG2REG, 0));
    } else {
        LOGS_ERROR("Invalid clock output to disable");
        return CR_INVALID_ARGS;
    }

    ERR_PROPS(bh_fw_clock_outout_notify(slice, lane, clock_output, false));

    return CR_OK;
}

CredoError_t bh_slice_disable_clock_output(CredoSlice_t* slice, unsigned clock_output) {
    ERR_PROPS(bh_slice_disable_only_clock_output(slice, clock_output));

    unsigned clk0_en, clk1_en, clk2_en;

    ERR_PROPS(readReg(slice, REG_TOP_CDR_CLK_EN0, &clk0_en));
    ERR_PROPS(readReg(slice, REG_TOP_CDR_CLK_EN1, &clk1_en));
    ERR_PROPS(readReg(slice, REG_TOP_CDR_CLK_EN2, &clk2_en));

    // if all clocks are disabled shut off the top level clock output
    if (!clk0_en && !clk1_en && !clk2_en) {
        LOGS_INFO("All clock outputs are disabled, turning off shared slice clock");
        ERR_PROPS(writeReg(slice, REG_TOP_PU_RVDD, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_PU_BG, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_EN_RVDDVCO, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_VRVDD, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_VBG_C, 0));
    }

    return CR_OK;
}

CredoError_t bh_slice_enable_clock_output(CredoSlice_t* slice, unsigned clock_output, unsigned lane, unsigned divider) {
    // validate correct input
    if (!bh_is_valid_lane(slice, lane)) {
        LOGS_ERROR("invalid lane for clock output %d", lane);
        return CR_INVALID_ARGS;
    }
    if (clock_output > 2) {
        LOGS_ERROR("invalid clock output, 0-2 supported");
        return CR_INVALID_ARGS;
    }

    CredoDevice_t* device = slice->device;
    int slice_count;
    cr_device_get_slice_count(device, &slice_count);

    // blackhawk clock output is shared between the slices, must ensure no 2 slices are using the same clock_output
    for (int slice_idx = 0; slice_idx < slice_count; slice_idx++) {
        CredoSlice_t* disable_slice;
        CredoError_t err = CR_OK;
        ERR_PROPS(cr_device_get_slice(device, slice_idx, &disable_slice));
        if (slice == disable_slice) continue;  // skip current slice
        unsigned clk_state;
        unsigned disable_lane;
        unsigned disable_divider;

        // lock the slice
        ERR_PROP_SLICE(disable_slice, cr_slice_lock(disable_slice));

        ERR_CATCH_SLICE(disable_slice,
                        (err = bh_slice_check_clock_output(disable_slice, clock_output, &clk_state, &disable_lane,
                                                           &disable_divider)),
                        goto unlock);

        if (clk_state == CKO_STATE_UNCONFIG) {
            goto unlock;  // skip if the current slice is not enabled
        };
        ERR_CATCH_SLICE(disable_slice, err = bh_slice_disable_clock_output(disable_slice, clock_output), goto unlock);
    unlock:
        cr_slice_unlock(disable_slice);
        if (err != CR_OK) return err;
    }

    unsigned divider_index = compute_divider_index(slice, divider);

    ERR_PROPS(writeReg(slice, REG_TOP_PU_RVDD, 1));
    ERR_PROPS(writeReg(slice, REG_TOP_PU_BG, 1));
    ERR_PROPS(writeReg(slice, REG_TOP_EN_RVDDVCO, 1));
    ERR_PROPS(writeReg(slice, REG_TOP_BYPASS_RVDDREG, 0));
    ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO, 1));
    ERR_PROPS(writeReg(slice, REG_TOP_VRVDD, 3));
    ERR_PROPS(writeReg(slice, REG_TOP_VBG_C, 0xF));

    if (clock_output == 0) {
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_MUX(0), lane));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_EN0, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_DIV_MUX0, divider_index));
        ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO_DIFF, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_VREF_CLK_DIFF, 7));
        ERR_PROPS(writeReg(slice, REG_TOP_BYPASS_DIFFREG, 1));
    } else if (clock_output == 1) {
        ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO_SG1, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_EN1, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_VREF_CLK_SG1, 7));
        ERR_PROPS(writeReg(slice, REG_TOP_BYPASS_SG1REG, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_DIV_MUX1, divider_index));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_MUX(1), lane));
    } else if (clock_output == 2) {
        ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO_SG2, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_EN2, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_VREF_CLK_SG2, 7));
        ERR_PROPS(writeReg(slice, REG_TOP_BYPASS_SG2REG, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_DIV_MUX2, divider_index));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_MUX(2), lane));
    }

    ERR_PROPS(bh_fw_clock_outout_notify(slice, lane, clock_output, true));

    return CR_OK;
}

CredoError_t bh_slice_disable_all_clock_output(CredoSlice_t* slice) {
    // disable each individual clock output
    ERR_PROPS(bh_slice_disable_only_clock_output(slice, 0));
    ERR_PROPS(bh_slice_disable_only_clock_output(slice, 1));
    ERR_PROPS(bh_slice_disable_clock_output(slice, 2));

    return CR_OK;
}
