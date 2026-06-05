#include "screaming_eagle.h"
#include "se_device.h"
#include "se_functions.h"

#include <math.h>

static CredoError_t se_testpoint_clear_lane(CredoSlice_t* slice, int lane, bool top) {
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_PLL, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_VTSTGROUP_PLL, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TESTMODE_PLL, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TESTMODE_RX, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_VTSTGROUP_RX, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_RX, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_ADC, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_VTSTGROUP_ADC, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_AFE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_VTSTGROUP_AFE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_VTSTGROUP_CLKPHASE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_CLKPHASE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TEST_EN_BG, 0));

    ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_TESTMODE_PLL_TX, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_VTSTGROUP_PLL_TX, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_ENTSTPGROUP_PLL_TX, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_TESTMODE_TX, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_VTSTGROUP_TX, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_ENTSTPGROUP_TX, 0));

    if (top) {
        ERR_PROPS(writeReg(slice, REG_TOP_PLL_ENTSTPGROUP, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_PLL_VTSTGROUP, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_PLL_TESTMODE, 0));

        ERR_PROPS(writeReg(slice, REG_TOP_ENTSTPGROUP_REG18, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_VTSTGROUP_REG18, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_RESERVE_REG18, 0));
    }
    return CR_OK;
}

CredoError_t se_testpoint_clear(CredoSlice_t* slice) {
    unsigned fw_status = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_status));

    bool is_fw_support_read_vsensor = false;
    if (fw_status == 1) {
        unsigned fw_feature = 0;
        ERR_PROPS(se_fw_get_feature(slice, &fw_feature));
        is_fw_support_read_vsensor = (fw_feature & FW_FEATURE_SUPPORT_READ_VSENSOR) ? true : false;
    }

    // firmware take care testpoint clear
    if (fw_status == 1 && is_fw_support_read_vsensor == true) {
        SeSlice_t* seslice = (SeSlice_t*)slice;
        seslice->testpoint_configured = false;
        return CR_OK;
    }

    for (int lane = 0; lane < SE_MAX_LANES; lane++) {
        ERR_PROPS(se_testpoint_clear_lane(slice, lane, false));
    }

    ERR_PROPS(writeReg(slice, REG_TOP_PLL_ENTSTPGROUP, 0));
    ERR_PROPS(writeReg(slice, REG_TOP_PLL_VTSTGROUP, 0));
    ERR_PROPS(writeReg(slice, REG_TOP_PLL_TESTMODE, 0));

    ERR_PROPS(writeReg(slice, REG_TOP_ENTSTPGROUP_REG18, 0));
    ERR_PROPS(writeReg(slice, REG_TOP_VTSTGROUP_REG18, 0));
    ERR_PROPS(writeReg(slice, REG_TOP_RESERVE_REG18, 0));
    double ignore;
    ERR_PROPS(se_testpoint_read(slice, &ignore));  // init testpoint vsensor on clear for simplicity
    SeSlice_t* seslice = (SeSlice_t*)slice;
    seslice->testpoint_configured = false;
    return CR_OK;
}

static const int group_max_index[] = {21, 6, 32, 16, 5, 21, 13, 24, 7};

CredoError_t se_testpoint_select(CredoSlice_t* slice, const CredoTestPoint_t* testpoint) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    int lane = testpoint->lane;
    int index = testpoint->index;
    int mode = testpoint->mode;
    int group = testpoint->group;
    bool div2 = testpoint->div2;

    if (mode != 1 && mode != 2 && mode != 4) {
        LOGS_ERROR("Testpoint invalid mode: must be 1,2,4");
        return CR_INVALID_ARGS;
    }

    if (group < 0 || group >= 9) {
        LOGS_ERROR("Testpoint invalid group: must be 0-8");
        return CR_INVALID_ARGS;
    }
    if (index < 0 || index >= group_max_index[group]) {
        LOGS_ERROR("Testpoint invalid index for group: must be 0-%d", group_max_index[group] - 1);
        return CR_INVALID_ARGS;
    }
    if (lane < 0 || lane >= SE_MAX_LANES) {
        LOGS_ERROR("Testpoint invalid lane");
        return CR_INVALID_ARGS;
    }

    unsigned fw_status = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_status));

    bool is_fw_support_read_vsensor = false;
    if (fw_status == 1) {
        unsigned fw_feature = 0;
        ERR_PROPS(se_fw_get_feature(slice, &fw_feature));
        is_fw_support_read_vsensor = (fw_feature & FW_FEATURE_SUPPORT_READ_VSENSOR) ? true : false;
    }

    if (fw_status == 1 && is_fw_support_read_vsensor == true) {
        seslice->testpoint = *testpoint;
        seslice->testpoint_configured = true;
        return CR_OK;
    }

    // on first select assume testpoint could be in weird state, clear testpoint incase it was configured
    if (!seslice->testpoint_cleared) {
        ERR_PROPS(se_testpoint_clear(slice));
        seslice->testpoint_cleared = true;
        seslice->testpoint_configured = false;
    }
    // need to clear test points if different lane or group
    if (seslice->testpoint_configured && (seslice->testpoint.lane != lane || seslice->testpoint.group != group)) {
        ERR_PROPS(se_testpoint_clear_lane(slice, seslice->testpoint.lane, true));  // clear the previous configuration
        seslice->testpoint_configured = false;
    }

    switch (group) {
        case 0:  // rx_pll
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_PLL, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_VTSTGROUP_PLL, index));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TESTMODE_PLL, mode));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TSTBUF_DIV_EN, (div2) ? 1 : 0));
            break;
        case 1:  // rx (intp)
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_RX, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_CLKPHASE, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_VTSTGROUP_CLKPHASE, index));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TESTMODE_RX, mode));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TSTBUF_DIV_EN, (div2) ? 1 : 0));
            break;
        case 2:  // adc
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_ADC, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_RX, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_VTSTGROUP_ADC, index));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TESTMODE_RX, mode));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TSTBUF_DIV_EN, (div2) ? 1 : 0));
            break;
        case 3:  // afe
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_AFE, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_RX, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_VTSTGROUP_AFE, index));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TESTMODE_RX, mode));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TSTBUF_DIV_EN, (div2) ? 1 : 0));
            break;
        case 4:  // bg
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TEST_EN_BG, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_ENTSTPGROUP_PLL, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_VTSTGROUP_PLL, 21 + index));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TESTMODE_PLL, mode));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOPANA_TSTBUF_DIV_EN, (div2) ? 1 : 0));
            break;
        case 5:  // tx pll
            ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_ENTSTPGROUP_PLL_TX, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_VTSTGROUP_PLL_TX, index));
            ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_TESTMODE_PLL_TX, mode));
            ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_TSTBUF_DIV_EN_TX, (div2) ? 1 : 0));
            break;
        case 6:  // tx
            ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_ENTSTPGROUP_TX, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_VTSTGROUP_TX, index));
            ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_TESTMODE_TX, mode));
            ERR_PROPS(writeRegLane(slice, lane, REG_PLL_TX_TSTBUF_DIV_EN_TX, (div2) ? 1 : 0));
            break;
        case 7:  // top pll
            ERR_PROPS(writeReg(slice, REG_TOP_PLL_ENTSTPGROUP, 1));
            ERR_PROPS(writeReg(slice, REG_TOP_PLL_VTSTGROUP, index));
            ERR_PROPS(writeReg(slice, REG_TOP_PLL_TESTMODE, mode));
            break;
        case 8:  // top 1p8
            ERR_PROPS(writeReg(slice, REG_TOP_ENTSTPGROUP_REG18, 1));
            ERR_PROPS(writeReg(slice, REG_TOP_VTSTGROUP_REG18, index));
            ERR_PROPS(writeReg(slice, REG_TOP_RESERVE_REG18, mode));
            break;
        default:
            return CR_INVALID_ARGS;
    }
    // cache testpoint to help speed up
    seslice->testpoint = *testpoint;
    seslice->testpoint_configured = true;
    return CR_OK;
}

CredoError_t se_testpoint_read(CredoSlice_t* slice, double* vsensor) {
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
    } else {
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_AUTO_VS, 0x0));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_RSTB_VS, 0x3));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_CLK_CNT_VS, 0xf));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_CLK_SEL_VS, 0x1));

        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_CFG, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_SDE, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_RSTN, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_RUN, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_SENSOR_VS_RUN, 1));
        if (readReg(slice, REG_TOP_SENSOR_VS_DOUT, &data) != CR_OK) return CR_FAIL;
    }

    *vsensor = 1.2077 / 5 * (6.0f * (double)data / 16384 - 3.0f / pow(2, res[res_sel]) - 1.0f);
    return CR_OK;
}
