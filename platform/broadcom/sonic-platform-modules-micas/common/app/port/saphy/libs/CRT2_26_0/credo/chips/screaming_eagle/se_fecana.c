#include "screaming_eagle.h"
#include "se_device.h"
#include "se_functions.h"

#include "fec_analyzer/fec_analyzer.h"

#include "utility.h"
// need to modify the FEC analyxer code so that clock and ana are never disabled, but we can still detect if fecana was
// configured

#define SE_FECANA_CONTROL_OFF 0x0003  // need to keep these on for SRAM to work properly
#define SE_FECANA_CONTROL_RUN 0x8003  // set the top bit on as a new way to detect fecana is configured

static int convert_prbs_pattern_mode_to_fec_index(CredoSlice_t* slice, CredoLanePrbsPattern_t pattern) {
    switch (pattern) {
        case CR_PRBS31:
            return 3;
        case CR_PRBS15:
            return 2;
        case CR_PRBS13:
            return 1;
        case CR_PRBS9:
            return 0;
        default:
            LOGS_INFO("[fec analyzer] prbs type unknown, assuming prbs31");
            return 3;
    }
}

CredoError_t se_fec_analyzer_set_config(CredoSlice_t* slice, int lane, int enable, CredoFecAnalyzerConfig_t* config) {
    ERR_PROPS(se_fw_set_fec_clk(slice, lane, enable));

    if (!enable) {
        return writeRegLane(slice, lane, REG_FECANA_CONTROL, SE_FECANA_CONTROL_OFF);
    }

    // check what the prbs pattern is for the lane
    int prbs_enable;
    CredoLanePrbsPattern_t pattern;
    ERR_PROPS(hal_get_rx_prbs(slice, lane, &prbs_enable, &pattern));

    if (!prbs_enable) {
        pattern = CR_PRBS_UNKNOWN;
    }

    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_SETUP, FECANA_SETUP_START1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_SETUP, FECANA_SETUP_START2));

    ERR_PROP_SLICE(
        slice, writeRegLane(slice, lane, REG_FECANA_PRBS_MODE, convert_prbs_pattern_mode_to_fec_index(slice, pattern)));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_SYMBOL_SIZE, config->symbol_size));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_CODEWORD_SIZE, config->codeword_size));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_THRES1, config->threshold));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_THRES2, config->threshold));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_TEI_TYPE, config->error_type));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_TEO_TYPE, config->error_type));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_CONTROL, FECANA_CONTROL_CLEAR));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_CONTROL, SE_FECANA_CONTROL_RUN));

    get_time(&slice->data->prbs_fec_timer[lane]);
    return CR_OK;
}

CredoError_t se_fec_analyzer_get_config(CredoSlice_t* slice, int lane, int* enable, CredoFecAnalyzerConfig_t* config) {
    unsigned reg;
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_CONTROL, &reg));

    *enable = ((reg & SE_FECANA_CONTROL_RUN) != SE_FECANA_CONTROL_RUN) ? 0 : 1;

    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_SYMBOL_SIZE, &reg));
    config->symbol_size = reg;
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_CODEWORD_SIZE, &reg));
    config->codeword_size = reg;
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_THRES1, &reg));
    config->threshold = reg;
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_TEI_TYPE, &reg));
    config->error_type = reg;
    return CR_OK;
}

CredoError_t se_fec_analyzer_reset(CredoSlice_t* slice, int lane) {
    int enable;
    CredoFecAnalyzerConfig_t config;
    ERR_PROPS(hal_get_fec_analyzer(slice, lane, &enable, &config));
    if (!enable) {
        return CR_OK;
    }
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_AUTO_SYNC_EN, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_CONTROL, FECANA_CONTROL_CLEAR));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_CONTROL, SE_FECANA_CONTROL_RUN));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_AUTO_SYNC_EN, 0));

    get_time(&slice->data->prbs_fec_timer[lane]);
    return CR_OK;
}
