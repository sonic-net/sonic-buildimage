/*
 * Credo full-device reset.
 * Most of them are implemented exactly the same way.
 */
#include "project.h"

#include "common/common_firmware.h"
#include "common/common_reset.h"

#include "sdk.h"

CredoError_t common_soft_reset(CredoSlice_t* slice) {
    // NOTE: make sure bits 15, 0 if mdio mode is used
    ERR_PROPS(writeReg(slice, REG_CHIP_RST, CHIP_SOFT_RST_VAL));

#ifdef REG_MDIO_MODE
    ERR_PROPS(common_set_mdio_mode(slice));
#endif

#ifdef REG_MCU_DEBUG
    ERR_PROPS(writeReg(slice, REG_MCU_DEBUG, 0x0));
#endif
    return CR_OK;
}

CredoError_t common_logic_reset(CredoSlice_t* slice) {
    ERR_PROPS(writeReg(slice, REG_CHIP_RST, CHIP_LOGIC_RST_VAL));
    return writeReg(slice, REG_CHIP_RST, 0);
}

CredoError_t common_mcu_reset(CredoSlice_t* slice) {
#ifdef REG_MCU_CLK_SEL
    ERR_PROPS(writeReg(slice, REG_MCU_CLK_SEL, 0x0));
#endif
    ERR_PROPS(writeReg(slice, REG_CHIP_RST, CHIP_CPU_RST_VAL));

    ERR_PROPS(writeReg(slice, REG_CHIP_RST, 0));
    return CR_OK;
}

CredoError_t common_mcu_reset_hold(CredoSlice_t* slice) {
#ifdef REG_MCU_CLK_SEL
    ERR_PROPS(writeReg(slice, REG_MCU_CLK_SEL, 0x0));
#endif
    return writeReg(slice, REG_CHIP_RST, CHIP_CPU_RST_VAL);
}

CredoError_t common_reg_reset(CredoSlice_t* slice) {
    ERR_PROPS(writeReg(slice, REG_CHIP_RST, CHIP_REG_RST_VAL));

#ifdef REG_MDIO_MODE
    ERR_PROPS(common_set_mdio_mode(slice));
#endif

#ifdef REG_MCU_DEBUG
    ERR_PROPS(writeReg(slice, REG_MCU_DEBUG, 0x0));
#endif
    return CR_OK;
}

// if the chip supports push_pull / ensure it is correctly set
CredoError_t common_set_mdio_mode(CredoSlice_t* slice) {
#ifdef REG_MDIO_MODE
    if (slice->data->is_mdio_push_pull) {
        LOGS_INFO("Configuring mdio clock to push/pull mode");
        // CRITICAL: must do a full register write as reads may not occur until after it is set
        ERR_PROPS(writeTop(slice, REG_MDIO_MODE, REG_MDIO_PUSH_PULL_MODE));
    }
#endif
    return CR_OK;
}

CredoError_t common_detect_mdio_mode(CredoSlice_t* slice) {
#ifdef REG_MDIO_MODE
    unsigned mode;

    ERR_PROPS(readTop(slice, REG_MDIO_MODE, &mode));
    mode = (mode >> REG_MDIO_MODE_BIT) & 1;  // perform shift to read correct register
    slice->data->is_mdio_push_pull = (bool)(mode);
#endif
    return CR_OK;
}
