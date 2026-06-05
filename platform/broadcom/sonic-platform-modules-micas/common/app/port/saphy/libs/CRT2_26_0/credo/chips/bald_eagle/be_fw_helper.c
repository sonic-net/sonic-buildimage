#include "bald_eagle.h"
#include "be_device.h"
#include "be_functions.h"

#include "utility.h"
#include "sdk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* be_fw_speed_string(FirmwareSpeed_t speed) {
    switch (speed) {
        case SPEED_OFF:
            return "OFF";
        case SPEED_10G:
            return "10G";
        case SPEED_20G:
            return "20G";
        case SPEED_25G:
            return "25G";
        case SPEED_26G:
            return "26G";
        case SPEED_28G:
            return "28G";
        case SPEED_51G:
            return "51G";
        case SPEED_53G:
            return "53G";
        case SPEED_56G:
            return "56G";
        default:
            return "???";
    }
}

const char* be_fw_config_mode_string(unsigned firmware_mode) {
    switch (firmware_mode) {
        case MODE_RETIMER_NRZ:
        case MODE_RETIMER_PAM4:
        case MODE_RETIMER_CROSS_NRZ:
        case MODE_RETIMER_CROSS_PAM4:
            return "RETIMER";
        case MODE_BITMUX_A1B2_NRZ:
        case MODE_BITMUX_A1B2_PAM4:
        case MODE_BITMUX_A2B1_NRZ:
        case MODE_BITMUX_A2B1_PAM4:
            return "BITMUX";
        case MODE_GEARBOX_100G_NRZ:
        case MODE_GEARBOX_100G_PAM4:
        case MODE_GEARBOX_50G_NRZ:
        case MODE_GEARBOX_50G_PAM4:
            return "GEARBOX";
        case MODE_PHY_NRZ:
        case MODE_PHY_PAM4:
            return "PHY";
        case MODE_LOOPBACK_NRZ:
        case MODE_LOOPBACK_PAM4:
            return "LOOPBACK";
            break;
        default:
            return "????????";
    }
}

CredoError_t be_fw_translate_config_mode_reverse(unsigned firmware_mode, CredoPortConnectionMode_t* mode) {
    switch (firmware_mode) {
        case MODE_RETIMER_NRZ:
        case MODE_RETIMER_PAM4:
        case MODE_RETIMER_CROSS_NRZ:
        case MODE_RETIMER_CROSS_PAM4:
            *mode = CR_PORT_RETIMER;
            break;
        case MODE_BITMUX_A1B2_NRZ:
        case MODE_BITMUX_A1B2_PAM4:
        case MODE_BITMUX_A2B1_NRZ:
        case MODE_BITMUX_A2B1_PAM4:
            *mode = CR_PORT_BITMUX;
            break;
        case MODE_GEARBOX_100G_NRZ:
        case MODE_GEARBOX_100G_PAM4:
        case MODE_GEARBOX_50G_NRZ:
        case MODE_GEARBOX_50G_PAM4:
            *mode = CR_PORT_GEARBOX;
            break;
        default:
            return CR_UNSUPPORTED;
    }
    return CR_OK;
}

unsigned be_fw_speed_kbps(FirmwareSpeed_t speed_index) {
    switch (speed_index) {
        case SPEED_OFF:
            return 0;
        case SPEED_10G:
            return 10312500;  // 10G * (33/32)
        case SPEED_20G:
            return 20625000;  // 20G * (33/32)
        case SPEED_25G:
            return 25781250;  // 25G * (33/32)
        case SPEED_26G:
            return 26562500;  // 25G * (34/32)
        case SPEED_28G:
            return 28125000;
        case SPEED_51G:
            return 51562500;  // 50G * (33/32)
        case SPEED_53G:
            return 53125000;  // 50G * (34/32)
        case SPEED_56G:
            return 56250000;
        default:
            return 0;
    }
}

CredoError_t be_fw_translate_lane_speed(unsigned speed, FirmwareSpeed_t* fw_speed) {
    switch (speed) {
        case CONFIG_10G:
            *fw_speed = SPEED_10G;
            break;
        case CONFIG_20G:
            *fw_speed = SPEED_20G;
            break;
        case CONFIG_25G:
            *fw_speed = SPEED_25G;
            break;
        case CONFIG_26G:
            *fw_speed = SPEED_26G;
            break;
        case CONFIG_51G:
            *fw_speed = SPEED_51G;
            break;
        case CONFIG_50G:
        case CONFIG_53G:
            *fw_speed = SPEED_53G;
            break;
        case CONFIG_56G:
            *fw_speed = SPEED_56G;
            break;
        default:
            return CR_NOTIMPLEMENTED;
    }
    return CR_OK;
}

CredoError_t be_fw_translate_lane_speed_reverse(FirmwareSpeed_t fw_speed, unsigned* speed) {
    switch (fw_speed) {
        case SPEED_51G:
        case SPEED_53G:
        case SPEED_56G:
            *speed = CONFIG_50G;
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
        default:
            return CR_UNSUPPORTED;
    }
    return CR_OK;
}
