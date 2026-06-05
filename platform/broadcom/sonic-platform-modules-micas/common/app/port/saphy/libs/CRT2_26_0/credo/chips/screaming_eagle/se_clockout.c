#include "screaming_eagle.h"
#include "se_device.h"
#include "se_functions.h"

#include "sdk.h"

#define DIVIDER_COUNT 8
static unsigned divider_index_map[DIVIDER_COUNT] = {32, 64, 128, 256, 512, 1024, 2048, 4096};
static unsigned compute_divider_index(CredoSlice_t* slice, unsigned divider) {
    for (unsigned div_index = 0; div_index < DIVIDER_COUNT; div_index++) {
        if (divider_index_map[div_index] == divider) return div_index;
    }
    LOGS_WARN("Unknown divider %d, using %d", divider, divider_index_map[0]);
    return 0;
}

CredoError_t se_slice_check_clock_output(CredoSlice_t* slice, unsigned clock_output, unsigned* state, unsigned* lane,
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
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX0, lane));
        ERR_PROPS(readReg(slice, REG_TOP_CDR_DIV_MUX0, &divider_index));
    } else if (clock_output == 1) {
        ERR_PROPS(readReg(slice, REG_TOP_CDR_CLK_EN1, &clk_en));
        ERR_PROPS(readReg(slice, REG_TOP_EN_CKO_SG1, &cko_en));
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX1, lane));
        ERR_PROPS(readReg(slice, REG_TOP_CDR_DIV_MUX1, &divider_index));
    } else if (clock_output == 2) {
        ERR_PROPS(readReg(slice, REG_TOP_CDR_CLK_EN2, &clk_en));
        ERR_PROPS(readReg(slice, REG_TOP_EN_CKO_SG2, &cko_en));
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX2, lane));
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

static CredoError_t se_slice_disable_only_clock_output(CredoSlice_t* slice, unsigned clock_output) {
    unsigned lane = 0;

    if (clock_output == 0) {
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX0, &lane));
        ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO_DIFF, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_EN0, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_VREF_CLK_DIFF, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_BYPASS_DIFFREG, 0));
    } else if (clock_output == 1) {
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX1, &lane));
        ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO_SG1, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_EN1, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_VREF_CLK_SG1, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_BYPASS_SG1REG, 0));
    } else if (clock_output == 2) {
        ERR_PROPS(readReg(slice, REG_TOP_CDR_MUX2, &lane));
        ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO_SG2, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_EN2, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_VREF_CLK_SG2, 0));
        ERR_PROPS(writeReg(slice, REG_TOP_BYPASS_SG2REG, 0));
    } else {
        LOGS_ERROR("Invalid clock output to disable");
        return CR_INVALID_ARGS;
    }

    return CR_OK;
}

CredoError_t se_slice_disable_clock_output(CredoSlice_t* slice, unsigned clock_output) {
    ERR_PROPS(se_slice_disable_only_clock_output(slice, clock_output));

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
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_GATE, 0));
    }

    return CR_OK;
}

CredoError_t se_slice_enable_clock_output(CredoSlice_t* slice, unsigned clock_output, unsigned lane, unsigned divider) {
    if (clock_output > 2) {
        LOGS_ERROR("invalid clock output, 0-2 supported");
        return CR_INVALID_ARGS;
    }

    CredoDevice_t* device = slice->device;
    int slice_count;
    cr_device_get_slice_count(device, &slice_count);

    // screaming eagle clock output is shared between the slices, must ensure no 2 slices are using the same
    // clock_output
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
                        (err = se_slice_check_clock_output(disable_slice, clock_output, &clk_state, &disable_lane,
                                                           &disable_divider)),
                        goto unlock);

        if (clk_state == CKO_STATE_UNCONFIG) {
            goto unlock;  // skip if the current slice is not enabled
        };
        ERR_CATCH_SLICE(disable_slice, err = se_slice_disable_clock_output(disable_slice, clock_output), goto unlock);
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
    ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_GATE, 0xFFFF));

    if (clock_output == 0) {
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_MUX0, lane));
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
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_MUX1, lane));
    } else if (clock_output == 2) {
        ERR_PROPS(writeReg(slice, REG_TOP_EN_CKO_SG2, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_CLK_EN2, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_VREF_CLK_SG2, 7));
        ERR_PROPS(writeReg(slice, REG_TOP_BYPASS_SG2REG, 1));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_DIV_MUX2, divider_index));
        ERR_PROPS(writeReg(slice, REG_TOP_CDR_MUX2, lane));
    }

    return CR_OK;
}

CredoError_t se_slice_disable_all_clock_output(CredoSlice_t* slice) {
    // disable each individual clock output
    ERR_PROPS(se_slice_disable_only_clock_output(slice, 0));
    ERR_PROPS(se_slice_disable_only_clock_output(slice, 1));
    ERR_PROPS(se_slice_disable_clock_output(slice, 2));

    return CR_OK;
}
