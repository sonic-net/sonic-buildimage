#include "project.h"

#include "common/common_firmware.h"

#include "sbs.h"
#include "sdk.h"

#include <stdlib.h>

#define BSIZE 1024

#if HAL_SUPPORT_EYE_MONITOR
static CredoError_t common_fw_em_print_axis(CredoSlice_t* slice, int vstep_side, int extent_mv, sbs* graph) {
    int vstep_full = vstep_side * 2 + 1;
    char buffer[BSIZE];

    /* line */
    for (int i = 0; i < vstep_full; i++) {
        buffer[i] = '-';
    }
    buffer[vstep_full] = '\0';
    buffer[vstep_side] = '+';
    sbscatprintf(graph, "%6s  +%s\n", "", buffer);

    /* ticks */
    sprintf(buffer, "%*c", vstep_full + 10, ' ');
    for (int i = -5; i <= -1; i++) {
        int mv = extent_mv * i / 5;
        int pos = vstep_side * i / 5 + vstep_side;
        int eos = snprintf(buffer + pos, BSIZE - pos, "%d", mv);
        buffer[pos + eos] = ' ';
        mv = extent_mv * -i / 5;
        pos = vstep_side * -i / 5 + vstep_side + 1;
        eos = snprintf(buffer + pos, BSIZE - pos, "%d", mv);
        buffer[pos + eos] = ' ';
    }
    buffer[vstep_side + 2] = '0';
    sbscatprintf(graph, "%7s%s\n", "", buffer);
    return CR_OK;
}

CredoError_t common_fw_em_start(CredoSlice_t* slice, int lane, int ber_exp, int flag) {
    FirmwareEMState_t em_state;
    unsigned cmd, detail2, response;

    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (!IS_LANE_MODE_PAM4_OR_NRZ(mode)) {
        LOGS_ERROR("Lane %d is not in pam4 or nrz mode", lane);
        return CR_FAIL;
    }

    cmd = FW_CMD_EYE_MON_START + lane;
    detail2 = ((ber_exp & 0xf) << 4) + (flag & 0x3);
    ERR_PROPS(hal_fw_cmd_ex(slice, cmd, 0, detail2, &response, NULL, NULL));

    em_state = (response >> 8) & 0xf;
    if (em_state != FW_EM_SUCCESS) {
        LOGS_WARN("[FW EM START][%d] EM start error, state %d", lane, em_state);
    }

    return CR_OK;
}

CredoError_t common_fw_em_stop(CredoSlice_t* slice, int lane) {
    FirmwareEMState_t em_state;
    unsigned cmd, response = 0;

    // firmware return error currently, skip error check
    cmd = FW_CMD_EYE_MON_STOP + lane;
    hal_fw_cmd(slice, cmd, 0, &response, NULL);
    em_state = response & 0xf;
    if (em_state != FW_EM_ERROR_CANCELL && em_state != FW_EM_ERROR_NOT_START) {
        LOGS_WARN("[FW EM STOP][%d] EM stop error, state %d", lane, em_state);
    }
    return CR_OK;
}

CredoError_t common_fw_em_progress(CredoSlice_t* slice, int lane, int* percent) {
    FirmwareEMState_t em_state;
    unsigned cmd, response;

    cmd = FW_CMD_EYE_MON_PROG + lane;
    ERR_PROPS(hal_fw_cmd(slice, cmd, 0, &response, NULL));

    em_state = (response >> 8) & 0xf;
    if (em_state != FW_EM_PROG_REPORT) {
        LOGS_WARN("[FW EM PROG][%d] EM seems not start yet, state %d", lane, em_state);
    }

    *percent = response & 0xff;

    return CR_OK;
}

CredoError_t common_fw_em_data(CredoSlice_t* slice, int lane, int** data, int* extent_mv) {
    CredoError_t ret;
    unsigned cmd, response, response2;
    int info[16];
    int vstep_side, hstep_side, vstep_full;

    ERR_PROPS(hal_fw_eye_monitor_range(slice, lane, &vstep_side, &hstep_side));
    vstep_full = vstep_side * 2 + 1;

    for (int phase = -hstep_side, pidx = 0; phase <= hstep_side; phase++, pidx++) {
        for (int margin = -vstep_side; margin <= vstep_side; margin += 16) {
            cmd = FW_CMD_EYE_MON_READ | (phase & 0xFF);
            ret = hal_fw_cmd_ex(slice, cmd, margin & 0xffff, lane, &response, &response2, NULL);
            if (ret != CR_OK) {
                switch (response & 0xFF) {
                    case FW_EM_ERROR_GOING_ON:
                        LOGS_ERROR("[FW EM READ][%d] EM is still running", lane);
                        return CR_FAIL;
                    case FW_EM_ERROR_NOT_START:
                        LOGS_ERROR("[FW EM READ][%d] EM did not start", lane);
                        return CR_FAIL;
                }
                return ret;
            }

            ERR_PROPS(common_fw_info_data(slice, 16, info));
            for (int i = 0; i < 16; i++) {
                int m = margin + vstep_side + i;
                if (m < vstep_full) {
                    data[m][pidx] = info[i];
                }
            }
        }
    }

    *extent_mv = response2;

    return CR_OK;
}

#ifdef FW_EM_VSTEP_SEPARATOR
CredoError_t common_fw_em_separator(CredoSlice_t* slice, int separator[5]) {
    ERR_PROPS(hal_fw_cmd(slice, FW_CMD_EYE_MON_READ, FW_EM_VSTEP_SEPARATOR, NULL, NULL));
    ERR_PROPS(common_fw_info_data(slice, 5, separator));
    return CR_OK;
}
#endif

// TODO, should use 1-D array
CredoError_t common_fw_em_print_bathtub_ascii(CredoSlice_t* slice, int** data, int vstep_side, int hstep_side,
                                              int extent_mv, const DisplayState_t* D) {
    int vstep_full = vstep_side * 2 + 1;
    sbs* graph = SBSNEW("", 16384);
    for (int i = 1; i < 19; i++) {
        if ((i & 1) == 0) {
            sbscatprintf(graph, "  1e-%d  |", i / 2);
        } else {
            sbscat(graph, "        |");
        }

        /* Calculate the lower and upper bound, all in -log(./16) scale */
        int lower = i * 8 + 4;
        int upper = i * 8 - 4;
        if (i == 1) upper = 0;
        for (int m = 0; m < vstep_full; m++) {
            sbscat(graph, (data[m][0] < lower && data[m][0] >= upper) ? "." : " ");
        }
        sbscat(graph, "\n");
    }

    common_fw_em_print_axis(slice, vstep_side, extent_mv, graph);
    DISPF(D, "%s", sbsstr(graph));
    return CR_OK;
}

CredoError_t common_fw_em_print_eye_monitor_ascii(CredoSlice_t* slice, int** data, int vstep_side, int hstep_side,
                                                  int extent_mv, const DisplayState_t* D) {
    static const char* legend[10] = {"@", "%", "#", "*", "+", "=", "-", ":", ".", " "};
    int vstep_full = vstep_side * 2 + 1;
    int hstep_full = hstep_side * 2 + 1;
    sbs* graph = SBSNEW("", 16384);

    for (int i = 0; i < hstep_full; i++) {
        int phase = i - hstep_side;
        if (abs(phase) == hstep_side) {
            sbscatprintf(graph, "%5.2f   |", phase * 0.0232);
        } else if (phase == 0) {
            sbscat(graph, "    0   |");
        } else {
            sbscat(graph, "        |");
        }

        for (int margin = 0; margin < vstep_full; margin++) {
            int ber_exp = data[margin][i];
            /* mapping: [i*16-8, i*16+8) => 10^{-i} */
            if (ber_exp == 0) ber_exp = 255;
            ber_exp = (ber_exp + 8) / 16;
            if (ber_exp < 1) {
                ber_exp = 1;
            } else if (ber_exp > 10) {
                ber_exp = 10;
            }
            sbscat(graph, legend[ber_exp - 1]);
        }
        sbscat(graph, "\n");
        // LOGS_INFO( "%s", buffer);
    }
    common_fw_em_print_axis(slice, vstep_side, extent_mv, graph);
    sbscat(graph, "\n     Legend: \"@%#*+=-:. \"\n");
    sbscat(graph, "     First character '@' means 1e-1, last character ' ' means 1e-10\n");
    DISPF(D, "%s", sbsstr(graph));
    return CR_OK;
}
#endif
