#include "bald_eagle.h"
#include "be_device.h"
#include "be_functions.h"

#include "canary/canary_serdes.h"
#include "common/common_firmware.h"
#include "common/common_init.h"
#include "common/common_reset.h"
#include "fec_analyzer/fec_analyzer.h"

#include "utility.h"
#include "sdk.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CredoError_t be_slice_data_init(CredoSlice_t* slice) {
    BeSlice_t* be_slice = (BeSlice_t*)slice;

    memset(&(be_slice->lane_mode), 0, sizeof(BeSlice_t) - sizeof(CredoSlice_t));
    common_init_slice_data(slice);

    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        be_slice->port_info[port_id].configured = false;
        be_slice->port_info[port_id].port_config.port_id = CR_PORT_UNCONFIGURED;
    }
    return CR_OK;
}

CredoError_t be_allocate_slice(CredoSdk_t* sdk, CredoSlice_t** handle) {
    BeSlice_t* slice = calloc(1, sizeof(BeSlice_t));
    CredoSliceData_t* data = malloc(sizeof(CredoSliceData_t));
    if (slice == NULL || data == NULL) return CR_OUT_OF_MEMORY;
    CredoSlice_t* crslice = (CredoSlice_t*)slice;
    crslice->data = data;
    *handle = crslice;
    common_init_slice_data(crslice);
    return CR_OK;
}

void be_free_slice(CredoSlice_t* handle) {
    free(handle->data);
    free(handle);
}

static CredoError_t be_slice_warm_init(CredoSlice_t* slice) {
    BeSlice_t* be_slice = (BeSlice_t*)slice;

    /* Lane mode */
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        ERR_PROPS(be_update_lane_mode(slice, lane));
    }

    unsigned optical_mask = 0;
    ERR_PROPS(be_fw_get_nrz_optical_mode(slice, &optical_mask));

    unsigned fec_en = 0;
    ERR_PROPS(readReg(slice, REG_FEC_EN, &fec_en));

    unsigned port_id = 0;
    unsigned short lane_taken = 0;
    for (int i = 0; i < HOST_LANES; i++) {
        BePortInfo_t* port_info = &be_slice->port_info[port_id];
        CredoPortConfig_t* port_config = &port_info->port_config;
        if ((1 << i) & lane_taken) continue;  // already counted

        unsigned lane_link_host, lane_link_line;
        unsigned speed_code, lane_speed;
        unsigned mode = 0;
        CredoPortConnectionMode_t conn_mode;
        ERR_PROPS(hal_fw_debug_cmd(slice, i, TOP_DEBUG, TOP_DEBUG_LANE_LINK, &lane_link_host));
        ERR_PROPS(hal_fw_debug_cmd(slice, i, TOP_DEBUG, TOP_DEBUG_LANE_LINK_ALL, &lane_link_line));
        ERR_PROPS(hal_fw_debug_cmd(slice, i, TOP_DEBUG, TOP_DEBUG_CONFIG_SEL, &mode));
        ERR_PROPS(common_fw_get_speed_index(slice, i, &speed_code));
        lane_link_line ^= lane_link_host;
        if (lane_link_host == 0 || lane_link_line == 0) {
            /* All "port" need both side */
            continue;
        }
        lane_taken |= lane_link_host | lane_link_line;
        port_info->configured = true;
        port_config->line_no_of_lanes = 0;
        port_config->host_no_of_lanes = 0;
        port_config->flags = 0;
        for (int j = 0; j < slice->desc->lane_count; j++) {
            if ((1 << j) & lane_link_host) {
                if (port_config->host_no_of_lanes++ == 0) port_config->host_start_lane = j;
            }
            if ((1 << j) & lane_link_line) {
                if (port_config->line_no_of_lanes++ == 0) port_config->line_start_lane = j;
            }
        }
        ERR_PROPS(be_fw_translate_lane_speed_reverse(speed_code, &lane_speed));
        ERR_PROPS(be_fw_translate_config_mode_reverse((mode >> 4) & 0xf, &conn_mode));

        unsigned lt_on = 0;
        ERR_PROPS(be_fw_get_lane_link_training(slice, port_config->line_start_lane, &lt_on));
        if (lt_on != 0) {
            port_config->flags |= CR_PFLAG_LINE_SIDE_ANLT;
        }

        if (optical_mask & (1 << port_config->line_start_lane)) {
            port_config->flags |= CR_PFLAG_LINE_SIDE_OPTICAL;
        }

        port_config->line_fec_type = CR_FEC_NONE;
        port_config->host_fec_type = CR_FEC_NONE;
        port_config->connection_mode = conn_mode;
        port_config->speed = lane_speed * port_config->host_no_of_lanes;
        port_config->port_id = port_id;

        switch (conn_mode) {
            case CR_PORT_RETIMER:
                if (lane_speed == SPEED_26G) {
                    port_config->line_fec_type = CR_FEC_RS_544;
                } else if (lane_speed == SPEED_51G) {
                    port_config->line_fec_type = CR_FEC_RS_528;
                }
                break;
            case CR_PORT_GEARBOX:
                port_config->host_fec_type = CR_FEC_RS_544;
                if (port_config->speed == CONFIG_50G) {
                    port_config->line_fec_type = CR_FEC_RS_528;
                } else {
                    if (fec_en & (1 << (port_config->line_start_lane / 2))) {
                        port_config->line_fec_type = CR_FEC_RS_528;
                    } else {
                        port_config->line_fec_type = CR_FEC_NONE;
                    }
                }
                break;
            case CR_PORT_BITMUX:
                if (port_config->speed == CONFIG_50G || port_config->speed == CONFIG_100G) {
                    port_config->host_fec_type = CR_FEC_RS_544;
                    port_config->line_fec_type = CR_FEC_RS_544;
                }
                break;
            default:
                return CR_UNSUPPORTED;
        }
        port_id++;
    }

    return CR_OK;
}

static const unsigned default_tx_taps_scale[5] = {0, 0, 1, 0, 0};

CredoError_t be_slice_init(CredoSlice_t* slice, const CredoSliceConfig_t* config) {
    ERR_PROPS(hal_slice_data_init(slice));

    switch (config->init_type) {
        case CR_INIT_NONE:
            return CR_OK;
        case CR_INIT_WARM:
            return be_slice_warm_init(slice);
        case CR_INIT_NO_FIRMWARE:
            /* Do a soft reset */
            ERR_PROP(common_soft_reset(slice));
            /* Initialize and calibrate top PLL */
            ERR_PROP(be_top_pll_init(slice));
            break;
        case CR_INIT_FULL:
            /* Do a soft reset */
            ERR_PROP(common_soft_reset(slice));
            /* Initialize and calibrate top PLL */
            ERR_PROP(be_top_pll_init(slice));
            /* Load firmware */
            ERR_PROPS(be_fw_download_from_file(slice, config->firmware_filename));
            break;
        default:
            return CR_FAIL;
    }
    /* Initialize BaldEagle slice */
    for (int i = 0; i < slice->desc->lane_count; i++) {
        ERR_PROPS(canary_set_tx_taps_scale(slice, i, default_tx_taps_scale));
        ERR_PROPS(canary_set_tx_msb(slice, i, 0));
        ERR_PROPS(canary_set_rx_msb(slice, i, 0));
        ERR_PROPS(canary_set_rx_prbs_nrz(slice, i, 0, CR_PRBS31));
        ERR_PROPS(fec_analyzer_set_config(slice, i, 0, NULL));
    }

    return CR_OK;
}

CredoError_t be_slice_get_vsensor(CredoSlice_t* slice, double* vsensor) {
    uint32_t data;

    ERR_PROP(writeReg(slice, REG_TOP_SENSOR_CLK_CNT_VS, 0x10d));
    ERR_PROP(writeReg(slice, REG_TOP_SENSOR_CLK_SEL_VS, 0));
    ERR_PROP(writeReg(slice, REG_TOP_SENSOR_VS_SEL_VIN, 8));
    ERR_PROP(writeReg(slice, REG_TOP_SENSOR_VS_PD, 1));
    sleep_us(100 * 1000);
    ERR_PROP(writeReg(slice, REG_TOP_SENSOR_VS_PD, 0));
    sleep_us(100 * 1000);
    ERR_PROP(writeReg(slice, REG_TOP_SENSOR_VS_PD, 0));
    sleep_us(100 * 1000);
    ERR_PROP(writeReg(slice, REG_TOP_SENSOR_VS_RSTN, 1));
    ERR_PROP(writeReg(slice, REG_TOP_SENSOR_VS_RUN, 0));
    sleep_us(100 * 1000);
    ERR_PROP(writeReg(slice, REG_TOP_SENSOR_VS_RUN, 1));
    sleep_us(100 * 1000);

    for (int i = 0; i < BE_MAX_VSENSOR; i++) {
        ERR_PROP(writeReg(slice, REG_TOP_SENSOR_CLK_SEL_VS, (i << 3)));
        sleep_us(300 * 1000);

        ERR_PROP(readReg(slice, REG_TOP_SENSOR_VS_DOUT, &data));
        vsensor[i] = (((double)data + 1.0f) / 256.0) * 1.224;
    }

    return CR_OK;
}

CredoError_t be_slice_save_setup(CredoSlice_t* slice, const char* file_path) {
    FILE* fp = fopen(file_path, "w");
    const RegHive_t* hive = NULL;
    CredoError_t ret = CR_OK;
    unsigned count = 0;
#define NUM_TOP_PLL (2)

    if (fp == NULL) {
        LOGS_ERROR("file %s open fail, %s\n", file_path, strerror(errno));
        return CR_FAIL;
    }

    ERR_CATCH((ret = hal_slice_get_reghive(slice, HIVENAME_TOPPLL, &hive)), goto exit);
    fprintf(fp, "##---------------------------------------##\n");
    fprintf(fp, "##                TOP PLL                ##\n");
    fprintf(fp, "##---------------------------------------##\n");
    for (int top_pll = 0; top_pll < NUM_TOP_PLL; top_pll++) {
        for (unsigned offset = 0; offset < 0x14; offset++) {
            unsigned value;
            unsigned address = cr_addr_reg(slice, top_pll, hive, offset);

            ERR_CATCH((ret = readTop(slice, address, &value)), goto exit);
            count++;
            fprintf(fp, "%04X %04X\n", address, value);
        }
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

    ERR_CATCH((ret = hal_slice_get_reghive(slice, HIVENAME_SERDES, &hive)), goto exit);
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
