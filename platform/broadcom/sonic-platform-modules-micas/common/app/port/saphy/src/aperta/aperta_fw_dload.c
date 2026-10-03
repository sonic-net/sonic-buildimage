/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * aperta_fw_dload.c - APERTA (81394) main firmware download over MDIO
 *
 * Method 1 (Internal / unicast): the host pushes the APERTA main ucode
 * (0xD019, plp_aperta_ucode[]) word-by-word into the GEN_CNTRLS_MST_MSGINr
 * mailbox, and the on-die boot loader consumes it 64 bytes at a time while
 * reporting progress through GEN_CNTRLS_MST_MSGOUTr handshake tokens.
 *
 * All register access is routed through _phy_sdk_bus_read/write,
 ********************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "aperta_phy_sdk.h"

/* TSC (Blackhawk) register base - used only by _bhawk_micro_reset below */
#define APERTA_FW_TSCBH_BASE        0x18000000u

/* Firmware blob (provided by aperta_ucode.c) */
extern uint8_t  plp_aperta_ucode[];
extern uint32_t plp_aperta_ucode_len;

/* ==========================================================================
 * plp_aperta_get_word_from_buffer
 * Byte stream -> 16-bit word (big-endian, matches FW header layout).
 * ========================================================================== */
static int plp_aperta_get_word_from_buffer(const uint8_t *buf, int index)
{
    uint16_t word = buf[(index * 2) + 1];
    word <<= 8;
    return (word | buf[index * 2]);
}

/* ==========================================================================
 * plp_aperta_wait_mst_msgout
 * Poll MST_MSGOUTr until it equals exp_message.
 *   flag_error : if 1, return error when a different nonzero value appears
 *   poll_time  : inter-poll delay in ms (0 = tight poll)
 * ========================================================================== */
static int plp_aperta_wait_mst_msgout(const plp_aperta_phymod_access_t *acc, uint16_t exp_message, int flag_error, int poll_time)
{
    int retry_count = APERTA_MICRO_RETRY_COUNT * 10;
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_t msg_out;
    uint16_t msgout;

    do {
        PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr(acc, msg_out));
        msgout = (uint16_t)BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_GET(msg_out);
        if (msgout != 0) {
            if (flag_error && (msgout != exp_message)) {
                LOG_INFO("ERR recvd msgout=0x%x exp=0x%x addr:%d\n", msgout, exp_message, acc->addr);
                return PHYMOD_E_INTERNAL;
            }
        }
        if (poll_time != 0) {
            PHYMOD_USLEEP(poll_time * 1000);
        }
    } while ((--retry_count) && (msgout != exp_message));

    if ((!retry_count) && (msgout != exp_message)) {
        LOG_INFO("ERR wait msgout=0x%x exp=0x%x retry:%d\n", msgout, exp_message, retry_count);
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _bhawk_micro_reset
 * Put all TSC (Blackhawk) micros into reset before a re-download.
* Only exercised on re-download (SERBOOT_DONE_ONCE && SLV_DWLD_DONE==3).
 * ========================================================================== */
static int _bhawk_micro_reset(const plp_aperta_phymod_phy_access_t *core)
{
    plp_aperta_phymod_phy_access_t phy;
    int micro_idx;
    int side;
    uint32_t rd;

    PHYMOD_MEMCPY(&phy, core, sizeof(phy));
    for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
        phy.port_loc = (plp_aperta_phymod_port_loc_t)side;
        for (micro_idx = 3; micro_idx >= 0; micro_idx--) {
            /* wrc_micro_core_clk_en(0) : 0xd240 bit0 = 0 */
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy, APERTA_FW_TSCBH_BASE | 0xd240, &rd));
            rd &= ~0x1u;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&phy, APERTA_FW_TSCBH_BASE | 0xd240, rd));
            /* wrc_micro_core_rstb(0) : 0xd241 bit0 = 0 */
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy, APERTA_FW_TSCBH_BASE | 0xd241, &rd));
            rd &= ~0x1u;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&phy, APERTA_FW_TSCBH_BASE | 0xd241, rd));
        }
        /* wrc_core_s_rstb toggle : 0xd101 bit0 */
        PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy, APERTA_FW_TSCBH_BASE | 0xd101, &rd));
        rd &= ~0x1u;
        PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&phy, APERTA_FW_TSCBH_BASE | 0xd101, rd));
        PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy, APERTA_FW_TSCBH_BASE | 0xd101, &rd));
        rd = (rd & ~0x1u) | 0x1u;
        PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&phy, APERTA_FW_TSCBH_BASE | 0xd101, rd));
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _plp_aperta_download_prog_eeprom
 * Internal unicast download of the APERTA main ucode via the MST_MSGIN
 * mailbox (prg_eeprom = 0). EEPROM programming is out of scope for this version.
 * ========================================================================== */
static int _plp_aperta_download_prog_eeprom(const plp_aperta_phymod_phy_access_t *core_access, uint8_t *ucode, uint32_t fw_length, uint16_t mst_boot_addr)
{
    int i, size0 = 0, size1 = 0, size2 = 0;
    uint16_t next_param = 0x1000;
    int retry_cnt = APERTA_MICRO_RETRY_COUNT, data1 = 0;
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_t msg_in;
    BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_t spi_code_load_en;
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_t boot_por;
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_t gen_ctrl2;
    BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_t start_ptr;
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_t msg_out;
    BCMI_APERTA_D_GEN_CNTRLS_BOOTr_t boot;
    BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_t misc_ctrl;
    BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_t ctrl_1;
    plp_aperta_phymod_phy_access_t temp_access;
    const plp_aperta_phymod_access_t *pa = &core_access->access;

    PHYMOD_MEMSET(&msg_in,           0, sizeof(msg_in));
    PHYMOD_MEMSET(&spi_code_load_en, 0, sizeof(spi_code_load_en));
    PHYMOD_MEMSET(&boot_por,         0, sizeof(boot_por));
    PHYMOD_MEMSET(&gen_ctrl2,        0, sizeof(gen_ctrl2));
    PHYMOD_MEMSET(&start_ptr,        0, sizeof(start_ptr));
    PHYMOD_MEMSET(&msg_out,          0, sizeof(msg_out));
    PHYMOD_MEMSET(&boot,             0, sizeof(boot));
    PHYMOD_MEMSET(&misc_ctrl,        0, sizeof(misc_ctrl));
    PHYMOD_MEMSET(&ctrl_1,           0, sizeof(ctrl_1));

    /* De-assert EXT_UC_RSTB_OUT (use even die address as the SDK does) */
    PHYMOD_MEMCPY(&temp_access, core_access, sizeof(temp_access));
    if (temp_access.access.addr & 1) {
        temp_access.access.addr &= ~1;
    }
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r(&temp_access.access, ctrl_1));
    BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCf_SET(ctrl_1, 1);
    BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCVALf_SET(ctrl_1, 1);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r(&temp_access.access, ctrl_1));

    /* On re-download, hold the TSC micros in reset first */
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr(pa, boot));
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_MICRO_BOOT_BOOT_PORr(pa, boot_por));
    if (BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_DONE_ONCEf_GET(boot) &&
        (BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_GET(boot_por) == 3)) {
        PHYMOD_IF_ERR_RETURN(_bhawk_micro_reset(core_access));
    }

    /* Put master in reset */
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 0);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 0);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_CTRL_MISC_CONTROL_TYPEr(pa, misc_ctrl));
    BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_SET(misc_ctrl, 1);
    BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_SET(misc_ctrl, 0);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_CTRL_MISC_CONTROL_TYPEr(pa, misc_ctrl));

    /* STEP 1: enable master + slave code download, verify read-back */
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, spi_code_load_en));
    LOG_INFO("SPI_CODE_LOAD_EN read(before)=0x%x phy=0x%x\n", spi_code_load_en.v[0], pa->addr);
    BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_SET(spi_code_load_en, 1);
    BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_SET(spi_code_load_en, 0x3);
    LOG_INFO("SPI_CODE_LOAD_EN write=0x%x\n", spi_code_load_en.v[0]);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, spi_code_load_en));
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, spi_code_load_en));
    LOG_INFO("SPI_CODE_LOAD_EN read(after)=0x%x\n", spi_code_load_en.v[0]);
    if ((BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_GET(spi_code_load_en) != 0x3) ||
        !(BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_GET(spi_code_load_en))) {
        LOG_INFO("ERR: download enable not set\n");
        /* DIAG: verify base-free address decode after PHYMOD_APERTA_DIRECT_BASE_ADR=0.
         * chip id (0x18b00) must read 0x1394. If RESET stage wrote GEN_CONTROL1
         * correctly the write below should stick (read back 0x310031). */
        {
            uint32_t d_val = 0;
            int d_rv;
            /* 1) chip id: proves base-free address decoding works */
            d_rv = _phy_sdk_bus_read(pa, 0x00018b00u, &d_val);
            LOG_INFO("DIAG chip_id  0x%08x rv=%d val=0x%x\n", 0x00018b00u, d_rv, d_val);
            /* 2) raw write + read-back of SPI_CODE_LOAD_EN (base-free) */
            d_rv = _phy_sdk_bus_write(pa, 0x00018210u, 0x310031u);
            LOG_INFO("DIAG write    0x%08x rv=%d\n", 0x00018210u, d_rv);
            d_rv = _phy_sdk_bus_read(pa, 0x00018210u, &d_val);
            LOG_INFO("DIAG read     0x%08x rv=%d val=0x%x\n", 0x00018210u, d_rv, d_val);
            /* 3) GEN_CONTROL1 / GEN_CONTROL2 base-free reads */
            d_rv = _phy_sdk_bus_read(pa, 0x00018201u, &d_val);
            LOG_INFO("DIAG GEN_CTL1 0x%08x rv=%d val=0x%x\n", 0x00018201u, d_rv, d_val);
            d_rv = _phy_sdk_bus_read(pa, 0x00018202u, &d_val);
            LOG_INFO("DIAG GEN_CTL2 0x%08x rv=%d val=0x%x\n", 0x00018202u, d_rv, d_val);
        }
        return PHYMOD_E_FAIL;
    }

    /* Set download-done = 0, serboot = 1, large mem = 1 */
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_MICRO_BOOT_BOOT_PORr(pa, boot_por));
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_SET(boot_por, 0);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_SET(boot_por, 0);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_RST_ENf_SET(boot_por, 0x3);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(boot_por, 1);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_LARGE_MEMf_SET(boot_por, 1);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_SET(boot_por, 0);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_MICRO_BOOT_BOOT_PORr(pa, boot_por));

    /* Dummy message-out read (PHY-3767) */
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr(pa, msg_out));

    /* Take master out of reset */
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 1);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 1);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_CTRL_MISC_CONTROL_TYPEr(pa, misc_ctrl));
    BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_SET(misc_ctrl, 0);
    BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_SET(misc_ctrl, 0);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_CTRL_MISC_CONTROL_TYPEr(pa, misc_ctrl));

    /* Wait for serboot busy to be set */
    do {
        PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr(pa, boot));
        data1 = BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        PHYMOD_USLEEP(3000);
    } while ((data1 != 1) && (--retry_cnt));
    if (retry_cnt == 0) {
        LOG_INFO("ERR: SERBOOT BUSY BIT NOT SET\n");
        return PHYMOD_E_FAIL;
    }
    retry_cnt = APERTA_MICRO_RETRY_COUNT;
    LOG_INFO("FW Download preparation Completed and start to Download...\n");

    BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_SET(start_ptr, mst_boot_addr);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_SPI_MST_CODE_START_PTRr(pa, start_ptr));
    PHYMOD_USLEEP(10000);

    /* Internal path (no EEPROM): wait for flash enable, then disable flash */
    PHYMOD_IF_ERR_RETURN(plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_FLASH, 1, 0));
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 0);   /* flash disable */
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));

    /* Wait for header request, then send the boot start address */
    PHYMOD_IF_ERR_RETURN(plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_HEADER, 1, 0));
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(msg_in, mst_boot_addr);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));

    /* Parse image sizes from the FW header */
    size0 = plp_aperta_get_word_from_buffer(ucode, 13);
    size0 |= (plp_aperta_get_word_from_buffer(ucode, 14) & 0xF) << 16;
    size1 = plp_aperta_get_word_from_buffer(ucode, 19);
    size1 |= (plp_aperta_get_word_from_buffer(ucode, 20) & 0xF) << 16;
    size2 = plp_aperta_get_word_from_buffer(ucode, 25);
    size2 |= (plp_aperta_get_word_from_buffer(ucode, 26) & 0xF) << 16;

    /* Round up to 64-byte multiples */
    size0 = (size0 % APERTA_HEADER_SIZE) ?
            ((size0 / APERTA_HEADER_SIZE) + 1) * APERTA_HEADER_SIZE : size0;
    size1 = (size1 % APERTA_HEADER_SIZE) ?
            ((size1 / APERTA_HEADER_SIZE) + 1) * APERTA_HEADER_SIZE : size1;
    size2 = (size2 % APERTA_HEADER_SIZE) ?
            ((size2 / APERTA_HEADER_SIZE) + 1) * APERTA_HEADER_SIZE : size2;

    LOG_INFO("FW Download Started... (0x%x bytes)\n", fw_length);
    for (i = 0; i < (int)(fw_length / 2); i++) {
        if ((i % 32) == 0) {
            if ((i == 0) || (i == 32)) {
                next_param = APERTA_MSGOUT_HEADER;
            } else if (i == 64) {
                LOG_INFO("FW Header Download Completed...\n");
                next_param = 0x1000;
            } else if (i * 2 == (128 + size0)) {
                next_param = 0x2000;
            } else if (i * 2 == (128 + size0 + size1)) {
                next_param = 0x3000;
            } else if (i * 2 == (128 + size0 + size1 + size2)) {
                next_param = 0x4000;
            } else {
                next_param++;
            }
        }
        PHYMOD_IF_ERR_RETURN(plp_aperta_wait_mst_msgout(pa, next_param, 1, 0));
        BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(msg_in, plp_aperta_get_word_from_buffer(ucode, i));
        PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_DWNLD_DONE, 1, 1));
    LOG_INFO("FW MST/SLV Download Completed...\n");
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * plp_aperta_dload_fw - top-level firmware download entry
 * fw_method: 1 = Internal (unicast), 2 = ProgEEPROM, 3 = External (unsupported here)       
 * ========================================================================== */
int plp_aperta_dload_fw(const plp_aperta_phymod_phy_access_t *core, int fw_method)
{
    int rv;
    int mst_boot_addr = 0;

    if (core == NULL) {
        return PHYMOD_E_PARAM;
    }
    if (fw_method == 3) {   /* phymodFirmwareLoadMethodExternal */
        LOG_INFO("External FW loader is not supported\n");
        return PHYMOD_E_UNAVAIL;
    }

    rv = _plp_aperta_download_prog_eeprom(core, plp_aperta_ucode, plp_aperta_ucode_len, mst_boot_addr);
    if ((rv != PHYMOD_E_NONE) && (rv != APERTA_FW_ALREADY_DOWNLOADED)) {
        LOG_INFO("FW download failed : %d\n", rv);
        return PHYMOD_E_FAIL;
    }
    return PHYMOD_E_NONE;
}
