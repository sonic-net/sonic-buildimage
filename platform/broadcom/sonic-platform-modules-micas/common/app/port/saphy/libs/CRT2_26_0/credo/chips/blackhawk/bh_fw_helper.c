#include "bh_device.h"
#include "bh_functions.h"
#include "blackhawk.h"

#include "utility.h"
#include "sdk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* bh_fw_speed_string(FirmwareSpeed_t speed) {
    switch (speed & 0xf) {
        case SPEED_OFF:
            return "OFF";
        case SPEED_1G:
            return "1G";
        case SPEED_10G:
            return "10G";
        case SPEED_20G:
            return "20G";
        case SPEED_25G:
            return "25G";
        case SPEED_26G:
            return "26G";
        case SPEED_27_9G:
            return "27.95G";
        case SPEED_28_125G:
            return "28.125G";
        case SPEED_28G:
            return "28G";
        case SPEED_53G:
            return "53G";
        case SPEED_55G:
            return "55G";
        case SPEED_55_9G:
            return "55.9G";
        case SPEED_56G:
            return "56.25G";
        case SPEED_56_15G:
            return "56.15G";
        default:
            return "???";
    }
}

unsigned bh_fw_speed_kbps(FirmwareSpeed_t speed_index) {
    switch (speed_index) {
        case SPEED_OFF:
            return 0;
        case SPEED_1G:
            return 1250000;
        case SPEED_10G:
            return 10312500;  // 10G * (33/32)
        case SPEED_20G:
            return 20625000;  // 20G * (33/32)
        case SPEED_25G:
            return 25781250;  // 25G * (33/32)
        case SPEED_26G:
            return 26562500;  // 25G * (34/32)
        case SPEED_27_9G:
            return 27952370;
        case SPEED_28G:
            return 28076100;
        case SPEED_28_125G:
            return 28125000;
        case SPEED_53G:
            return 53125000;  // 50G * (34/32)
        case SPEED_55G:
            return 55000000;
        case SPEED_55_9G:
            return 55904740;
        case SPEED_56G:
            return 56250000;
        case SPEED_56_15G:
            return 56152300;
        default:
            return 0;
    }
}

const char* bh_fw_config_mode_string(unsigned config_sel) {
    switch (config_sel & 0xf) {
        case MODE_RETIMER:
            return "RETIMER";
        case MODE_BITMUX_A1B2:
        case MODE_BITMUX_A2B1:
            return "BITMUX";
        case MODE_GEARBOX_50G:
        case MODE_GEARBOX_100G:
            return "GEARBOX";
        case MODE_PHY:
            return "PHY";
        case MODE_LOOPBACK:
            return "LOOPBACK";
        default:
            return "???";
    }
}

const char* bh_fw_gearbox_error_string(unsigned err_code) {
    switch (err_code & FW_GB_FEC_ERROR_CODE_MASK) {
        case 0:
            return "0";
        case FW_GB_FEC_ERROR_CODE_OK:
            return "OK";
        case FW_GB_RX_FEC_ERROR_CODE_MISSING_ALIGNMENT:
            return "MISSING_ALIGNMENT";
        case FW_GB_RX_FEC_ERROR_CODE_BURST_ERROR:
            return "BURST_ERROR";
        case FW_GB_RX_FEC_ERROR_CODE_UNCORRECTABLE_CFG:
            return "UNCORRECTABLE_CFG";
        case FW_GB_RX_FEC_ERROR_CODE_FIFO_MESSED_UP_HW_PDIFF:
            return "RX FIFO_MESSED_UP_HW_PDIFF";
        case FW_GB_RX_FEC_ERROR_CODE_FIFO_MESSED_UP_AVG:
            return "RX FIFO_MESSED_UP_AVG";
        case FW_GB_RX_FEC_ERROR_CODE_FIFO_MESSED_UP_DEV:
            return "RX FIFO_MESSED_UP_DEV";
        case FW_GB_RX_FEC_ERROR_CODE_FIFO_MESSED_UP_PRE:
            return "RX FIFO_MESSED_UP_PRE";
        case FW_GB_RX_FEC_ERROR_CODE_FIFO_MESSED_UP_NCW:
            return "RX FIFO_MESSED_UP_NCW";
        case FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_HW_PDIFF:
            return "TX FIFO_MESSED_UP_HW_PDIFF";
        case FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_AVG:
            return "TX FIFO_MESSED_UP_AVG";
        case FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_DEV:
            return "TX FIFO_MESSED_UP_DEV";
        case FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_PRE:
            return "TX FIFO_MESSED_UP_PRE";
        case FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_NCW:
            return "TX FIFO_MESSED_UP_NCW";
        case FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_BIP:
            return "TX FIFO_MESSED_UP_BIP";
        case FW_GB_FEC_ERROR_CODE_PHY_LOSS:
            return "PHY_LOSS";
        case FW_GB_CFG_TIMEOUT:
            return "CFG_TIMEOUT";
        case FW_GB_FEC_ERROR_CODE_NCW_TIMEOUT:
            return "NCW_TIMEOUT";
        default:
            return "Unknown error code";
    }
}

const char* bh_fw_gearbox_state_string(unsigned state) {
    switch (state) {
        case FW_GB_STATE_START:
            return "START";
        case FW_GB_STATE_WAIT_PHY:
            return "WAIT_PHY";
        case FW_GB_STATE_WAIT_LT:
            return "WAIT_LT";
        case FW_GB_STATE_CHECK_RX_FIFO:
            return "CHECK_RX_FIFO";
        case FW_GB_STATE_RX2TX_WAIT:
            return "RX2TX_WAIT";
        case FW_GB_STATE_CHECK_FEC_A:
            return "CHECK_FEC_A";
        case FW_GB_STATE_CHECK_TX_FIFO:
            return "CHECK_TX_FIFO";
        case FW_GB_STATE_DONE:
            return "DONE";
        case FW_GB_STATE_RELEASE_FIFO:
            return "RELEASE_FIFO";
        case FW_GB_STATE_PATCH_FIFO:
            return "PATCH_FIFO";
        default:
            return "Unknow state code";
    }
}

CredoError_t bh_fw_translate_multilane(unsigned multilane, unsigned* multilane_code) {
    switch (multilane) {
        case 1:
            *multilane_code = 0;
            break;
        case 2:
            *multilane_code = 1;
            break;
        case 4:
            *multilane_code = 2;
            break;
        case 8:
            *multilane_code = 3;
            break;
        default:
            return CR_NOTIMPLEMENTED;
    }
    return CR_OK;
}

CredoError_t bh_fw_translate_multilane_reverse(unsigned multilane_code, unsigned* multilane) {
    switch (multilane_code) {
        case 0:
            *multilane = 1;
            break;
        case 1:
        case 4:
            *multilane = 2;
            break;
        case 2:
        case 5:
            *multilane = 4;
            break;
        case 3:
            *multilane = 8;
            break;
        default:
            return CR_NOTIMPLEMENTED;
    }
    return CR_OK;
}

CredoError_t bh_fw_translate_lane_speed(unsigned speed, FirmwareSpeed_t* fw_speed) {
    switch (speed) {
        case CONFIG_50G:
            *fw_speed = SPEED_53G;
            break;
        case CONFIG_25G:
            *fw_speed = SPEED_25G;
            break;
        case CONFIG_20G:
            *fw_speed = SPEED_20G;
            break;
        case CONFIG_10G:
            *fw_speed = SPEED_10G;
            break;
        default:
            return CR_NOTIMPLEMENTED;
    }
    return CR_OK;
}

CredoError_t bh_fw_translate_lane_speed_reverse(FirmwareSpeed_t fw_speed, unsigned* speed) {
    switch (fw_speed) {
        case SPEED_56G:
            *speed = CONFIG_56_25G;
            break;
        case SPEED_56_15G:
            *speed = CONFIG_56_15G;
            break;
        case SPEED_55_9G:
            *speed = CONFIG_55_9G;
            break;
        case SPEED_55G:
            *speed = CONFIG_55G;
            break;
        case SPEED_53G:
            *speed = CONFIG_50G;
            break;
        case SPEED_28G:
            *speed = CONFIG_28G;
            break;
        case SPEED_27_9G:
            *speed = CONFIG_27_95G;
            break;
        case SPEED_25G:
        case SPEED_26G:
            *speed = CONFIG_25G;
            break;
        case SPEED_20G:
            *speed = CONFIG_20G;
            break;
        case SPEED_10G:
            *speed = CONFIG_10G;
            break;
        case SPEED_1G:
            *speed = CONFIG_1G;
            break;
        default:
            return CR_NOTIMPLEMENTED;
    }
    return CR_OK;
}

CredoError_t bh_fw_translate_config_mode_reverse(unsigned firmware_mode, CredoPortConnectionMode_t* mode) {
    switch (firmware_mode) {
        case MODE_RETIMER:
            *mode = CR_PORT_RETIMER;
            break;
        case MODE_BITMUX_A1B2:
        case MODE_BITMUX_A2B1:
            *mode = CR_PORT_BITMUX;
            break;
        case MODE_GEARBOX_100G:
        case MODE_GEARBOX_50G:
            *mode = CR_PORT_GEARBOX;
            break;
        default:
            return CR_UNSUPPORTED;
    }
    return CR_OK;
}

CredoError_t bh_fw_translate_FEC(CredoFecType_t fec, FirmwareFecType_t* fw_fec) {
    switch (fec) {
        case CR_FEC_NONE:
            *fw_fec = FW_FEC_OFF;
            break;
        case CR_FEC_FIRE_CODE:
            *fw_fec = FW_FEC_FIRE;
            break;
        case CR_FEC_RS_528:
            *fw_fec = FW_FEC_528;
            break;
        case CR_FEC_RS_544:
            *fw_fec = FW_FEC_544;
            break;
        default:
            return CR_NOTIMPLEMENTED;
    }
    return CR_OK;
}

CredoError_t bh_fw_translate_FEC_reverse(FirmwareFecType_t fw_fec, CredoFecType_t* fec) {
    switch (fw_fec) {
        case FW_FEC_OFF:
            *fec = CR_FEC_NONE;
            break;
        case FW_FEC_FIRE:
            *fec = CR_FEC_FIRE_CODE;
            break;
        case FW_FEC_528:
            *fec = CR_FEC_RS_528;
            break;
        case FW_FEC_544:
            *fec = CR_FEC_RS_544;
            break;
        default:
            return CR_NOTIMPLEMENTED;
    }
    return CR_OK;
}
