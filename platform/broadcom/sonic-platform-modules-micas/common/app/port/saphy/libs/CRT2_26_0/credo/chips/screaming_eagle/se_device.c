#include "screaming_eagle.h"
#include "se_device.h"
#include "se_functions.h"

#include "common/common_firmware.h"
#include "common/common_init.h"
#include "common/common_reset.h"
#include "swift/swift_serdes.h"

#include "utility.h"
#include "sdk.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int default_tx_taps[7] = {0, 0, 0, 53, 0, 0, 0};  // HW default value

CredoError_t se_slice_data_init(CredoSlice_t* slice) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;

    unsigned spiflash_timeout = slice->data->fw_spiflash_load_timeout;

    memset(&(se_slice->lane_mode), 0, sizeof(SeSlice_t) - sizeof(CredoSlice_t));
    common_init_slice_data(slice);

    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        se_slice->em_info[lane] = (SeEyeMonitorInfo_t){0};
        memcpy(se_slice->tx_taps[lane], default_tx_taps, sizeof(int) * 7);
    }

    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        se_slice->port_info[port_id].started = false;
    }
    se_slice->fw_isi_timeout = FW_ISI_TIMEOUT_PER_PHASE;
    se_slice->isc_slice_id = 0xFFFF;
    se_slice->start_time = 0;
    se_slice->vsensor_resolution = FW_VSENSOR_DEFAULT_RESOLUTION;
    // restore spiflash timeout
    slice->data->fw_spiflash_load_timeout = spiflash_timeout;
    return CR_OK;
}

CredoError_t se_allocate_slice(CredoSdk_t* sdk, CredoSlice_t** handle) {
    SeSlice_t* slice = calloc(1, sizeof(SeSlice_t));
    if (slice == NULL) return CR_OUT_OF_MEMORY;
    CredoSliceData_t* data = malloc(sizeof(CredoSliceData_t));
    if (data == NULL) {
        free(slice);
        return CR_OUT_OF_MEMORY;
    }
    CredoSlice_t* crslice = (CredoSlice_t*)slice;
    crslice->data = data;
    *handle = crslice;
    common_init_slice_data(crslice);
    return CR_OK;
}

void se_free_slice(CredoSlice_t* handle) {
    free(handle->data);
    free(handle);
}

static CredoError_t se_slice_warm_boot(CredoSlice_t* slice) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    unsigned fw_status = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_status));
    if (fw_status == 0) {
        LOGS_WARN("[Slice init] firmware is not running, skip warm init");
        return CR_OK;
    }
    LOGS_INFO("[Slice init] performing a warm initialization");

    /* Lane mode */
    LOGS_DEBUG("[Slice init] capturing lane mode information from firmware");
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        ERR_PROPS(se_update_lane_mode(slice, lane));

        ERR_PROPS(swift_get_tx_taps(slice, lane, se_slice->tx_taps[lane]));
        unsigned fw_speed = SPEED_OFF;
        ERR_PROP(common_fw_get_speed_index(slice, lane, &fw_speed));

        CredoLaneMode_t lane_mode;
        ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));

        if (fw_speed == SPEED_25G || (fw_speed == SPEED_53G && lane_mode == CR_LMODE_PAM4)) {
            se_slice->tx_taps[lane][2] = se_slice->tx_taps[lane][1];
            se_slice->tx_taps[lane][4] = se_slice->tx_taps[lane][5];
            se_slice->tx_taps[lane][1] = se_slice->tx_taps[lane][5] = 0;
        }
    }
    LOGS_DEBUG("[Slice init] capturing port setup information from firmware");
    for (unsigned port = 0; port < slice->desc->port_count; port++) {
        CredoError_t ret = se_port_capture_info(slice, port, &se_slice->port_info[port]);
        if (ret != CR_OK) {
            LOGS_ERROR("[Slice init] restore port %d info fail.", port);
        }
    }

    return CR_OK;
}

CredoError_t se_soft_reset_without_fw_running(CredoSlice_t* slice) {
    /*
     * Use mcu_reset in fw_unload + reg_reset + logic_reset instead of soft_reset,
     * or fw_load will fail because of firmware already running after
     * soft_reset to trigger boot from SPI when valid firmware in SPI flash.
     */
    ERR_PROPS(hal_fw_unload(slice));
    ERR_PROPS(hal_reg_reset(slice));
    ERR_PROPS(hal_logic_reset(slice));
    return CR_OK;
}

CredoError_t se_slice_init(CredoSlice_t* slice, const CredoSliceConfig_t* config) {
    unsigned fw_status = 0;
    ERR_PROPS(hal_slice_data_init(slice));

    switch (config->init_type) {
        case CR_INIT_NONE:
            return CR_OK;
        case CR_INIT_WARM:
            return se_slice_warm_boot(slice);
        case CR_INIT_NO_FIRMWARE:
            ERR_PROPS(hal_fw_get_status(slice, &fw_status));

            /* Do a soft reset */
            ERR_PROP(common_soft_reset(slice));

            /* if there have firmware running before, try to wait fw ready */
            if (fw_status == 1) {
                common_fw_wait_ready(slice);  // skip error
            }
            break;
        case CR_INIT_FULL:
            /* Load firmware */
            ERR_PROPS(se_fw_download(slice, config->firmware_filename));
            break;
        case CR_INIT_SPIFLASH: {
            CredoTime_t start_time;
            get_time(&start_time);
            LOGS_DEBUG("[SPI-Flash] checking for bootchain load timeout of %dms",
                       slice->data->fw_spiflash_load_timeout / 1000);
            while (!is_timeout(&start_time, (long)slice->data->fw_spiflash_load_timeout)) {
                ERR_PROPS(hal_fw_get_status(slice, &fw_status));
                if (fw_status == 1) {
                    break;
                }
                sleep_ms(10);
            }
            if (fw_status == 0) {
                LOGS_ERROR("[SPI-Flash] firmware bootchain load timeout");
                return CR_PHY_FW_DOWNLOAD_FAIL;
            }
            break;
        }
        default:
            return CR_FAIL;
    }

    return CR_OK;
}

CredoError_t se_slice_get_vsensor(CredoSlice_t* slice, double* vsensor) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    int res[] = {14, 12, 10, 8};
    int res_sel = 0;

    unsigned fw_status = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_status));

    bool is_fw_support_read_vsensor = false;
    if (fw_status == 1) {
        unsigned fw_feature = 0;
        ERR_PROPS(se_fw_get_feature(slice, &fw_feature));
        is_fw_support_read_vsensor = (fw_feature & FW_FEATURE_SUPPORT_READ_VSENSOR) ? true : false;
    }

    uint32_t data = 0;
    if (fw_status == 1 && is_fw_support_read_vsensor == true) {
        res_sel = seslice->vsensor_resolution;
        ERR_PROPS(se_fw_testpoint_read(slice, &data));
        vsensor[0] = 1.2077 / 5 * (6.0f * data / 16384 - 3.0f / pow(2, res[res_sel]) - 1.0f);
        return CR_OK;
    }

    ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_AUTO_VS, 0x0));
    ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_RSTB_VS, 0x3));
    ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_CLK_CNT_VS, 0xf));
    ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_CLK_SEL_VS, 0x1));

    for (int i = 0; i < slice->desc->vsensor_count; i++) {
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_CFG, i));
        // sleep_ms(50);
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_SDE, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_RSTN, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_RUN, 0));
        // sleep_ms(50);
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_RUN, 1));

        // sleep_ms(20);
        if (readReg(slice, REG_TOP_SENSOR_VS_DOUT, &data) != CR_OK) return CR_FAIL;
        vsensor[i] = 1.2077 / 5 * (6.0f * data / 16384 - 3.0f / pow(2, res[res_sel]) - 1.0f);
    }

    return CR_OK;
}

CredoError_t se_slice_save_setup(CredoSlice_t* slice, const char* file_path) {
    FILE* fp = fopen(file_path, "w");
    const RegHive_t* hive = NULL;
    CredoError_t ret = CR_OK;
    unsigned count = 0;
    unsigned reg_range = 0;

    if (fp == NULL) {
        LOGS_ERROR("file %s open fail, %s\n", file_path, strerror(errno));
        return CR_FAIL;
    }

    hive = TopPLL;
    reg_range = 0x2B;
    fprintf(fp, "##---------------------------------------##\n");
    fprintf(fp, "##     pll_fsyn_reg (0x00-0x%02X)          ##\n", reg_range);
    fprintf(fp, "##---------------------------------------##\n");
    for (unsigned offset = 0x0; offset < reg_range + 1; offset++) {
        unsigned value;
        unsigned address = cr_addr_reg(slice, 0, hive, offset);

        if (cr_slice_read(slice, address, &value) != CR_OK) {
            ret = CR_FAIL;
            goto exit;
        } else {
            count++;
            fprintf(fp, "%04X %04X\n", address, value);
        }
    }
    fprintf(fp, "\n\n");

    hive = Top;
    reg_range = 0x7C;
    fprintf(fp, "##---------------------------------------##\n");
    fprintf(fp, "##     SE_LR_REG (0x01-0x%02X)          ##\n", reg_range);
    fprintf(fp, "##---------------------------------------##\n");
    for (unsigned offset = 0x1; offset < reg_range + 1; offset++) {
        unsigned value;
        unsigned address = cr_addr_reg(slice, 0, hive, offset);

        if (cr_slice_read(slice, address, &value) != CR_OK) {
            ret = CR_FAIL;
            goto exit;
        } else {
            count++;
            fprintf(fp, "%04X %04X\n", address, value);
        }
    }
    fprintf(fp, "\n\n");

    hive = PHY;
    reg_range = 0xE9;
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        fprintf(fp, "##---------------------------------------##\n");
        fprintf(fp, "##    RXDSP_REG lane%3d (0x00-0x%02X)      ##\n", lane, reg_range);
        fprintf(fp, "##---------------------------------------##\n");
        for (unsigned offset = 0x0; offset < reg_range + 1; offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, lane, hive, offset);

            if (cr_slice_read(slice, address, &value) != CR_OK) {
                ret = CR_FAIL;
                goto exit;
            } else {
                count++;
                fprintf(fp, "%04X %04X\n", address, value);
            }
        }
        fprintf(fp, "\n\n");
    }

    reg_range = 0x7F;
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        fprintf(fp, "##---------------------------------------##\n");
        fprintf(fp, "##    RX_DIG_REG lane%3d (0x01-0x%02X)     ##\n", lane, reg_range);
        fprintf(fp, "##---------------------------------------##\n");
        for (unsigned offset = 0x1; offset < reg_range + 1; offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, lane, hive, offset + 0x100);

            if (cr_slice_read(slice, address, &value) != CR_OK) {
                ret = CR_FAIL;
                goto exit;
            } else {
                count++;
                fprintf(fp, "%04X %04X\n", address, value);
            }
        }
        fprintf(fp, "\n\n");
    }

    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        fprintf(fp, "##---------------------------------------##\n");
        fprintf(fp, "##    ANA_TOP_REG lane%3d (0xA8-0xFF)    ##\n", lane);
        fprintf(fp, "##---------------------------------------##\n");
        for (unsigned offset = 0xA8; offset < 0x100; offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, lane, hive, offset + 0x100);

            if (cr_slice_read(slice, address, &value) != CR_OK) {
                ret = CR_FAIL;
                goto exit;
            } else {
                count++;
                fprintf(fp, "%04X %04X\n", address, value);
            }
        }
        fprintf(fp, "\n\n");
    }

    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        fprintf(fp, "##---------------------------------------##\n");
        fprintf(fp, "##   debug_sram_reg lane%3d (0x00-0x12)  ##\n", lane);
        fprintf(fp, "##---------------------------------------##\n");
        for (unsigned offset = 0x0; offset <= 0x12; offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, lane, hive, offset + 0x180);

            if (cr_slice_read(slice, address, &value) != CR_OK) {
                ret = CR_FAIL;
                goto exit;
            } else {
                count++;
                fprintf(fp, "%04X %04X\n", address, value);
            }
        }
        fprintf(fp, "\n\n");
    }

    hive = TX_TOP;
    reg_range = 0xBD;
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        fprintf(fp, "##------------------------------------------##\n");
        fprintf(fp, "##   tx_56g_digital_top lane%3d (0xA0-0x%02X) ##\n", lane, reg_range);
        fprintf(fp, "##------------------------------------------##\n");
        for (unsigned offset = 0xA0; offset < reg_range + 1; offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, lane, hive, offset);

            if (cr_slice_read(slice, address, &value) != CR_OK) {
                ret = CR_FAIL;
                goto exit;
            } else {
                count++;
                fprintf(fp, "%04X %04X\n", address, value);
            }
        }
        fprintf(fp, "\n\n");
    }

    reg_range = 0xA3;
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        fprintf(fp, "##------------------------------------------##\n");
        fprintf(fp, "##      pelican_ana_reg lane%3d (0x%02X-0xBF) ##\n", lane, reg_range);
        fprintf(fp, "##------------------------------------------##\n");
        for (unsigned offset = reg_range; offset < 0xC0; offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, lane, hive, offset + 0x100);

            if (cr_slice_read(slice, address, &value) != CR_OK) {
                ret = CR_FAIL;
                goto exit;
            } else {
                count++;
                fprintf(fp, "%04X %04X\n", address, value);
            }
        }
        fprintf(fp, "\n\n");
    }

    hive = TopOneLane;
    reg_range = 0x48;
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        fprintf(fp, "##---------------------------------------##\n");
        fprintf(fp, "##  reg_dsp_one_lane lane%3d (0x00-0x%02X) ##\n", lane, reg_range);
        fprintf(fp, "##---------------------------------------##\n");
        for (unsigned offset = 0x0; offset < reg_range + 1; offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, lane, hive, offset);

            if (cr_slice_read(slice, address, &value) != CR_OK) {
                ret = CR_FAIL;
                goto exit;
            } else {
                count++;
                fprintf(fp, "%04X %04X\n", address, value);
            }
        }
        fprintf(fp, "\n\n");
    }

    fprintf(fp, "##---------------------------------------##\n");
    fprintf(fp, "##           ecc_reg (0x00-0x01)         ##\n");
    fprintf(fp, "##---------------------------------------##\n");
    for (unsigned offset = 0x0; offset < 0x2; offset++) {
        unsigned value;
        unsigned address = 0xC000 + offset;

        if (cr_slice_read(slice, address, &value) != CR_OK) {
            ret = CR_FAIL;
            goto exit;
        } else {
            count++;
            fprintf(fp, "%04X %04X\n", address, value);
        }
    }
    fprintf(fp, "\n\n");

exit:
    LOGS_INFO("%d registers were read to file %s", count, file_path);
    if (fp) fclose(fp);
    return ret;
}

CredoError_t se_slice_get_revision(CredoSlice_t* slice, unsigned* revision_number) {
    ERR_PROPS(cr_slice_read(slice, 0xFFFE, revision_number));
    return CR_OK;
}
