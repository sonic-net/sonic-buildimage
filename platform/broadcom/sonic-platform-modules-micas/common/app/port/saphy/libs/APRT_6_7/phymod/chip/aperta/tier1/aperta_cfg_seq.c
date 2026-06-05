/*
*
* $Id: aperta_cfg_seq.c,  $
*
*  *
*  *
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
*  *
*  *
*
*/

#include "aperta_cfg_seq.h"
#include "bcmi_aperta_d_defs.h"
#include "bcmi_aperta_id_defs.h"
#include <phymod/phymod_acc.h>
#include <phymod/chip/aperta.h>

#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_diagnostics_dispatch.h>
#include <tier1/aperta_reg_access.h>
#include <tier1/aperta_pm_seq.h>
#include <include/aperta_tscbh_diagnostics.h>
#include <include/aperta_tscbh.h>
#include "blackhawk/tier1/blackhawk_tsc_functions.h"
#include <phymod/phymod_util.h>

extern unsigned char plp_aperta_ucode[];
extern uint32_t plp_aperta_ucode_len;

extern aperta_pm_info_t _plp_aperta_pm_info[APERTA_MAX_PM_INFO];
int plp_aperta_get_chip_id (const plp_aperta_phymod_access_t *pa, int *chip_id)
{
    BCMI_APERTA_D_CTRL_CHIP_IDr_t  lsb;
    BCMI_APERTA_D_CTRL_CHIP_REVISIONr_t msb;

    PHYMOD_MEMSET(&lsb, 0, sizeof(BCMI_APERTA_D_CTRL_CHIP_IDr_t));
    PHYMOD_MEMSET(&msb, 0, sizeof(BCMI_APERTA_D_CTRL_CHIP_REVISIONr_t));
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_CTRL_CHIP_REVISIONr(pa, &msb));
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_CTRL_CHIP_IDr(pa, &lsb));

    *chip_id = BCMI_APERTA_D_CTRL_CHIP_IDr_CHIP_ID_15_0f_GET(lsb) |
         (BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CHIP_ID_19_16f_GET(msb) << 16);

    return PHYMOD_E_NONE;
}

int plp_aperta_get_chip_rev (const plp_aperta_phymod_access_t *pa, uint32_t *chip_rev)
{
    BCMI_APERTA_D_CTRL_CHIP_REVISIONr_t msb;

    PHYMOD_MEMSET(&msb, 0, sizeof(BCMI_APERTA_D_CTRL_CHIP_REVISIONr_t));
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_CTRL_CHIP_REVISIONr(pa, &msb));

    *chip_rev = BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(msb);

    return PHYMOD_E_NONE;
}

/** Wait Msg Out from Master Micro
 * @param Pointer to Chip Cfg descriptor
 * @param Die Number
 * @param Expected Message
 * @param If 1, flag error if received Msg is different than the expected.
 * @param Time between register reads in miliseconds
 */
static int plp_aperta_wait_mst_msgout(const plp_aperta_phymod_access_t *acc, uint16_t exp_message, int flag_error, int poll_time)
{
    APERTA_MSGOUT_T msgout;
    int retry_count = APERTA_MICRO_RETRY_COUNT * 10;
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_t msg_out;

    do {
        /* read register*/
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr(acc, &msg_out));
        msgout = BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_GET(msg_out);
        if (msgout != 0) {
            if (flag_error && (msgout != exp_message)) {
                PHYMOD_DEBUG_ERROR(
                        ("ERR Recived msgout = (0x%x), exp_message = 0x%x addr:%d)\n", msgout, exp_message, acc->addr));
                return PHYMOD_E_INTERNAL;
            } else {
                /*PHYMOD_DIAG_OUT(("Recived msgout = (0x%x), exp_message = 0x%x)\n", msgout,  exp_message));*/
            }
        }
        /* wait before reading again */
        if (poll_time != 0) {
#ifdef SIM_VAL
            PHYMOD_USLEEP(5000);
            PHYMOD_DIAG_OUT(("Retry:%d\n", retry_count));
#else
            PHYMOD_USLEEP(poll_time * 1000);
#endif
        }
    } while ((--retry_count) && (msgout != exp_message));

    if ((!retry_count) && (msgout != exp_message)) {
        PHYMOD_DEBUG_ERROR(
                ("ERROR: Recived msgout = (0x%x), exp_message = 0x%x) -- retry:%d\n", msgout, exp_message, retry_count));
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL,
                (_PHYMOD_MSG("Firmware download failed")));
    }
    return PHYMOD_E_NONE;

}
static int plp_aperta_get_word_from_buffer(const uint8_t *buf, int index)
{
     uint16_t word = buf[(index*2)+1];
     word <<= 8;
     return (word | buf[index*2]);

}

STATIC int plp_aperta_bhawk_micro_reset(const plp_aperta_phymod_core_access_t *core)
{
    plp_aperta_phymod_phy_access_t phy;
    plp_aperta_phymod_phy_access_t *sa__ = &phy;
    int8_t micro_idx = 0;
    int side = 0;
    PHYMOD_MEMCPY(&phy, core, sizeof(plp_aperta_phymod_phy_access_t));
    for(side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
        sa__->port_loc = side;
        for (micro_idx = 3; micro_idx >= 0; micro_idx--) {
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_blackhawk_tsc_set_micro_idx(sa__, micro_idx));
            EFUN(wrc_micro_core_clk_en(0x0)); /* [0xd240].BIT[0] : MICRO_E_micro_core_clock_control0.micro_core_clk_en = 0            */
            EFUN(wrc_micro_core_rstb(0x0));   /* [0xd241].BIT[0] : MICRO_E_micro_core_reset_control0.micro_core_rstb = 0              */
        }
        EFUN(wrc_core_s_rstb(0x0));           /* [0xd101].BIT[0] : DIG_COM_RESET_CONTROL_PMD.core_s_rstb = 0 (Assert core reset)      */
        EFUN(wrc_core_s_rstb(0x1));           /* [0xd101].BIT[0] : DIG_COM_RESET_CONTROL_PMD.core_s_rstb = 1 (De-assert core reset)   */
    }
    return PHYMOD_E_NONE;
}
/**   Download and Fuse firmware
 *    This function is used to download the firmware through I2C/MDIO
 *    and fuse it to SPI EEPROM if prg_eeprom flag is set
 *
 *    @param pa                 Pointer to phymod access structure
 *    @param new_fw             Pointer to firmware array
 *    @param fw_length          Length of the firmware array
 *    @param prg_eeprom         Flag used to program EEPROM
 *
 *    @return num_bytes         number of bytes successfully downloaded
 */
int
_plp_aperta_download_prog_eeprom(const plp_aperta_phymod_core_access_t *core_access,
        uint8_t *plp_aperta_ucode, uint32_t fw_length, uint16_t master_en,
        uint16_t mst_boot_addr, uint8_t prg_eeprom, const plp_aperta_phymod_core_init_config_t* init_config)

{
#ifndef VIRTUAL_SIM_VAL
    int i, size0, size1 = 0, size2 = 0;
    uint16_t next_param = 0x1000;
    int retry_cnt = APERTA_MICRO_RETRY_COUNT, data1 = 0;
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_t msg_in;
    BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_t spi_code_load_en;
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL3r_t gen_ctrl3;
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_t boot_por;
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_t gen_ctrl2;
    BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_t start_ptr;
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_t msg_out;
    const plp_aperta_phymod_access_t *pa = &core_access->access;
    BCMI_APERTA_D_GEN_CNTRLS_GPREG_02r_t gpreg2;
    BCMI_APERTA_D_GEN_CNTRLS_BOOTr_t boot;
    BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_t misc_ctrl;
    bcm_plp_ext_fw_params_t ext_fw_params;
    unsigned int trans_size = 0, dload_idx = 0;
    uint32_t chip_rev = 0;
    plp_aperta_phymod_phy_access_t temp_access;
    BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t  bcast_enable;
    BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_t ctrl_1;
    
    PHYMOD_MEMSET(&bcast_enable, 0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t));
    PHYMOD_MEMCPY(&temp_access, core_access, sizeof(plp_aperta_phymod_phy_access_t));
    PHYMOD_MEMSET(&ext_fw_params,     0, sizeof(bcm_plp_ext_fw_params_t));
    PHYMOD_MEMSET(&boot_por,         0, sizeof(BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_t));
    PHYMOD_MEMSET(&msg_in,           0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_t));
    PHYMOD_MEMSET(&msg_out,          0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_t));
    PHYMOD_MEMSET(&gen_ctrl2,        0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_t));
    PHYMOD_MEMSET(&spi_code_load_en, 0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_t));
    PHYMOD_MEMSET(&gen_ctrl3,        0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL3r_t));
    PHYMOD_MEMSET(&start_ptr,        0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_t));
    PHYMOD_MEMSET(&gpreg2,           0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_GPREG_02r_t));
    PHYMOD_MEMSET(&boot,             0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_BOOTr_t));
    
    if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
        PHYMOD_IF_ERR_RETURN(
            init_config->firmware_loader(core_access->access.user_acc, &ext_fw_params));
        if (ext_fw_params.transfer_size < 64) {
            PHYMOD_DEBUG_ERROR(("ERROR: Transfer size cannot be less than 64 bytes"));
            return PHYMOD_E_PARAM;
        }
        if ((ext_fw_params.transfer_size % 2) != 0) {
            PHYMOD_DEBUG_ERROR(("ERROR: Transfer size cannot be Odd number\n"));
            return PHYMOD_E_PARAM;
        }

        if (ext_fw_params.firmware_address == NULL) {
            PHYMOD_DEBUG_ERROR(("ERROR: Invalid FW array\n"));
            return PHYMOD_E_PARAM;
        }
        if (ext_fw_params.fw_length == 0) {
            PHYMOD_DEBUG_ERROR(("ERROR: Invalid FW length\n"));
            return PHYMOD_E_PARAM;
        }
        if (ext_fw_params.transfer_size > ext_fw_params.fw_length) {
            PHYMOD_DEBUG_ERROR(("ERROR: Transfer size is greater than FW length\n"));
            return PHYMOD_E_PARAM;
        }
        fw_length = ext_fw_params.fw_length;
        trans_size = ext_fw_params.transfer_size;
    }

    /* With Broadcast enabled on both the DIE EEPROM Download will 
     * not work, So When bcast enabled on DIE B with EEPROM as dload method
     * DieB will not be taken out of reset*/
    if (prg_eeprom) {
        temp_access.access.addr &= ~1;
        temp_access.access.addr |= 1;
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&temp_access.access, &bcast_enable));
    }

    /* If do Die B first then take it out of reset*/
    if ((init_config->firmware_load_method != phymodFirmwareLoadMethodProgEEPROM) ||
            (prg_eeprom == 1 && ((bcast_enable.v[0] & 1) != 1))) {
        if (temp_access.access.addr & 1) {
            temp_access.access.addr &= ~1;
        }
        PHYMOD_IF_ERR_RETURN(
                READ_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r(&temp_access.access, &ctrl_1));
        BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCf_SET(ctrl_1, 1);
        BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCVALf_SET(ctrl_1, 1);
        PHYMOD_IF_ERR_RETURN(
               WRITE_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r(&temp_access.access, ctrl_1));
    }
    
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr(pa, &boot));
    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_READ_MICRO_BOOT_BOOT_PORr(pa, &boot_por));
    if (BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_DONE_ONCEf_GET(boot) &&
        (BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_GET(boot_por) == 3)) {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_bhawk_micro_reset(core_access));
    }
    /*Put master in to reset*/
    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 0);
    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));
    
    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 0);
    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_CTRL_MISC_CONTROL_TYPEr(pa, &misc_ctrl));
    BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_SET(misc_ctrl, 1);
    BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_SET(misc_ctrl, 0);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_CTRL_MISC_CONTROL_TYPEr(pa, misc_ctrl));

    /* STEP 1: Program master enable, slave enable, broadcast enable bits */
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, &spi_code_load_en));
    BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_SET(spi_code_load_en, 1);
    BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_SET(spi_code_load_en, 0x3);
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, spi_code_load_en));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, &spi_code_load_en));
    if ((BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_GET(spi_code_load_en)!= 0x3)||
        !(BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_GET(spi_code_load_en))) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_FAIL,
                (_PHYMOD_MSG("ERR: Download ENABLE IS NOT SET")));
    }

    if (prg_eeprom) {
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, &spi_code_load_en));
        BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_MST_OEBf_SET(spi_code_load_en, 1);
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, spi_code_load_en));

        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL3r(pa, &gen_ctrl3));
        BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL3r_UCSPI_SLOWf_SET(gen_ctrl3, 1);
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL3r(pa, gen_ctrl3));
    }
    /* Set Download done as '0'*/
    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_READ_MICRO_BOOT_BOOT_PORr(pa, &boot_por));
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_SET(boot_por, 0);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_SET(boot_por, 0);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_RST_ENf_SET(boot_por, 0x3);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(boot_por, 1);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_LARGE_MEMf_SET(boot_por, 1);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_SET(boot_por, 0);
    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_WRITE_MICRO_BOOT_BOOT_PORr(pa, boot_por));
    
    /* Dummy message out read for PHY-3767*/
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr(pa, &msg_out));

    /*Take master out of reset, updated for PHY-3767*/
    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 1);
    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 1);
    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_CTRL_MISC_CONTROL_TYPEr(pa, &misc_ctrl));
    BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_SET(misc_ctrl, 0);
    BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_SET(misc_ctrl, 0);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_CTRL_MISC_CONTROL_TYPEr(pa, misc_ctrl));
    do {
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr(pa, &boot));
        data1 = BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        PHYMOD_USLEEP(3000);
    } while((data1 != 1) && (--retry_cnt));
    if (retry_cnt == 0) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_FAIL,
                               (_PHYMOD_MSG("ERR:SERBOOT BUSY BIT NOT SET")));
    }
    retry_cnt = APERTA_MICRO_RETRY_COUNT;
#ifdef ATE_PRINT_ENABLED
	PHYMOD_CRIT_INFO(("ATE_GUIDELINES : FW Download preparation Completed...\n"));
#else
    PHYMOD_CRIT_INFO(("FW Download preparation Completed...\n"));
#endif
    BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_SET(start_ptr, mst_boot_addr);
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_GEN_CNTRLS_SPI_MST_CODE_START_PTRr(pa, start_ptr));

#ifdef SIM_VAL
    PHYMOD_USLEEP(300);
#else
    PHYMOD_USLEEP(10000);
#endif

    /*EEPROM*/
    if (prg_eeprom) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(pa, &chip_rev));
        PHYMOD_CRIT_INFO(("Enabling EEPROM\n"));

#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_FLASH, 1, 2000));
#else
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_FLASH, 1, 0));
#endif
        if (chip_rev == APERTA_REV_B0) {
            /* Send Erase EEPROM and Flash enabled*/
            BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 3);

        } else {
            /* Send Flash enabled*/
            BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 1);
        }
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_FLASH, 1, 2000));
#else
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_FLASH, 1, 0));
#endif
        /* Send write delay*/
        BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_FLASH, 1, 2000));
#else
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_FLASH, 1, 0));
#endif
        /* Send debug mode*/
        BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
    } else {
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_PLL_LOCK, 1, 2000));

        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_FLASH, 1, 2000));
#else
        /* Wait for Register Status, instead of message out*/
        /* Waiting for PLL lock
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_PLL_LOCK, 1, 0));*/
        /* Checking for flash enable*/
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_FLASH, 1, 0));
#endif
        BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in)); /* Flashing Disable*/
    }
#ifdef SIM_VAL
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_HEADER, 1, 2000));
#else

    if (prg_eeprom && chip_rev == APERTA_REV_B0) {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_HEADER, 1, 2));
    } else {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_HEADER, 1, 0));
    }
#endif
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(msg_in, mst_boot_addr);
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));

    if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
        size0 = plp_aperta_get_word_from_buffer(ext_fw_params.firmware_address, 13);
        size0 |= (plp_aperta_get_word_from_buffer(ext_fw_params.firmware_address, 14) & 0xF) << 16;
        size1 = plp_aperta_get_word_from_buffer(ext_fw_params.firmware_address, 19);
        size1 |= (plp_aperta_get_word_from_buffer(ext_fw_params.firmware_address, 20) & 0xF) << 16;
        size2 = plp_aperta_get_word_from_buffer(ext_fw_params.firmware_address, 25);
        size2 |= (plp_aperta_get_word_from_buffer(ext_fw_params.firmware_address, 26) & 0xF) << 16;
    } else {
        size0 = plp_aperta_get_word_from_buffer(plp_aperta_ucode, 13);
        size0 |= (plp_aperta_get_word_from_buffer(plp_aperta_ucode, 14) & 0xF) << 16;
        size1 = plp_aperta_get_word_from_buffer(plp_aperta_ucode, 19);
        size1 |= (plp_aperta_get_word_from_buffer(plp_aperta_ucode, 20) & 0xF) << 16;
        size2 = plp_aperta_get_word_from_buffer(plp_aperta_ucode, 25);
        size2 |= (plp_aperta_get_word_from_buffer(plp_aperta_ucode, 26) & 0xF) << 16;
    }

    /*adjusted to be multiple of 64B*/
    size0 = (size0 % APERTA_HEADER_SIZE) ?
            ((size0 / APERTA_HEADER_SIZE) + 1) * APERTA_HEADER_SIZE : size0;
    size1 = (size1 % APERTA_HEADER_SIZE) ?
            ((size1 / APERTA_HEADER_SIZE) + 1) * APERTA_HEADER_SIZE : size1;
    size2 = (size2 % APERTA_HEADER_SIZE) ?
            ((size2 / APERTA_HEADER_SIZE) + 1) * APERTA_HEADER_SIZE : size2;

    PHYMOD_CRIT_INFO(("FW Download Started...\n"));
    for (i = 0; i < (fw_length / 2); i++) {
        if ((i%32) == 0) {
            if ((i == 0) || (i == 32)) {
                next_param = APERTA_MSGOUT_HEADER;
            } else if (i == 64) {
                PHYMOD_CRIT_INFO(("FW Header Download Completed...\n"));
                next_param = 0x1000;
            } else if (i*2 == (128 + size0)) {
                next_param = 0x2000;
            } else if (i*2 == (128 + size0 + size1)) {
                next_param = 0x3000;
            } else if (i*2 == (128 + size0 + size1 + size2)) {
                next_param = 0x4000;
            } else {
                next_param++;
            }
        }
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, next_param, 1, 2000));
#else
    if (prg_eeprom) {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, next_param, 1, ((i%32) == 0)?2:0 ));
    } else {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_wait_mst_msgout(pa, next_param, 1, 0));
    }
#endif
        if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
            if (trans_size == 0) {
                PHYMOD_IF_ERR_RETURN(
                    init_config->firmware_loader(core_access->access.user_acc, &ext_fw_params));
                trans_size = ext_fw_params.transfer_size;
                dload_idx = 0;
            }
            BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(msg_in,
                    plp_aperta_get_word_from_buffer(ext_fw_params.firmware_address, dload_idx));
            dload_idx ++;
        } else {
            BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(msg_in,
                    plp_aperta_get_word_from_buffer(plp_aperta_ucode, i));
        }
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
        if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
            trans_size -= 2;
        }
    }
#ifdef SIM_VAL
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_DWNLD_DONE, 1, 2000));
#else
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_wait_mst_msgout(pa, APERTA_MSGOUT_DWNLD_DONE, 1, 1));
#endif

    PHYMOD_CRIT_INFO(("FW MST/SLV Download Completed...\n"));

    if (prg_eeprom == 1 && ((bcast_enable.v[0] & 1) == 1)) {
        PHYMOD_IF_ERR_RETURN( _plp_aperta_core_reset_set(core_access, phymodResetModeHard, 0));
        /* Giving 100milli sec time for FW dload*/
        PHYMOD_USLEEP(100000);
    }
#endif
    return PHYMOD_E_NONE;
}

int _plp_aperta_check_fw_download_status(const plp_aperta_phymod_core_access_t *core_access,
        plp_aperta_phymod_firmware_load_method_t load_method) {
#ifndef VIRTUAL_SIM_VAL
    uint16_t n_img = 0, no_of_img = 0, retry_cnt = 100;
    BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_t fw_ver;
    BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_t gpreg_1;
    BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_t dwnld_01;
    BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_t dwnld_03;
    uint32_t data = 0;
    uint32_t crc = 0;
    uint32_t aperta_fw_crc[2] = {0,0};

    PHYMOD_MEMSET(&gpreg_1, 0,  sizeof(BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_t));
    PHYMOD_MEMSET(&dwnld_01, 0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_t));
    PHYMOD_MEMSET(&dwnld_03, 0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_t));

    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&core_access->access, BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r,
                &aperta_fw_crc[0]));
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&core_access->access, BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r,
                &aperta_fw_crc[1]));

    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_GEN_CNTRLS_FIRMWARE_VERSIONr(&core_access->access, &fw_ver));

    n_img = 2; /* Get Number of image*/
    for (no_of_img = 0; no_of_img < n_img; ++no_of_img) {
        PHYMOD_IF_ERR_RETURN(
                PHYMOD_BUS_READ(&core_access->access, (BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r + (no_of_img*2)), &data));
        if (data != 0x600D) {
            PHYMOD_IF_ERR_RETURN(
                    PHYMOD_BUS_READ(&core_access->access, (BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r + (no_of_img*2) + 1), &data));
            crc = aperta_fw_crc[no_of_img]; /*Get CRC*/
            if (crc != data) {
                PHYMOD_DEBUG_ERROR(("ERROR: Image Dload status not correct for image:%d Got:0x%x expected:0x%x\n", 
                            no_of_img, data, aperta_fw_crc[no_of_img] ));
                return PHYMOD_E_INTERNAL;
            }
        }
    }
    do {
        PHYMOD_USLEEP(5000);
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_GPREG_01r(&core_access->access, &gpreg_1));
        if (BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1)& 1) {
            /* Check for uc active*/
            if ((BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1) & 0xFF00) != 0xFF00) {
                PHYMOD_DEBUG_ERROR(("FW init seq failed with :%x for Phy:%x\n",
                            BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1),
                            core_access->access.addr));
                return PHYMOD_E_FAIL;
            }
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_03r(&core_access->access, &dwnld_03));
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_01r(&core_access->access, &dwnld_01));
            PHYMOD_CRIT_INFO(
                    ("FW download success. FW ver:0x%x. CRC0:%x CRC1:%x\n",
                      BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_FIRMWARE_VERSION_VALf_GET(fw_ver),
                      BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_DOWNLOAD_01f_GET(dwnld_01),
                      BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_DOWNLOAD_03f_GET(dwnld_03)));
            return PHYMOD_E_NONE;
        }
    } while (--retry_cnt);
    PHYMOD_DEBUG_ERROR(
            ("ERROR: FW download Failure. Gpreg1:%x\n",
              BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1)));

    return PHYMOD_E_INTERNAL;
#else
    return PHYMOD_E_NONE;
#endif
}

extern int _plp_aperta_tscbh_core_init_pass2(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config, const plp_aperta_phymod_core_status_t* core_status);
extern int _plp_aperta_tscbh_core_init_pass1(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config, const plp_aperta_phymod_core_status_t* core_status);

int _plp_aperta_core_reset_set(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_reset_mode_t reset_mode, plp_aperta_phymod_reset_direction_t direction)
{
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_t core_reset;
    BCMI_APERTA_D_CTRL_SWGPREG0Er_t swgpreg_e;
    unsigned int pll = 0, lane = 0, speed = 0, index = 0;
    plp_aperta_phymod_phy_access_t phy_access;
    plp_aperta_phymod_phy_init_config_t init_config;
    plp_aperta_phymod_core_init_config_t core_init_config;
    aperta_fw_init_t fw_init_param;
    BCMI_APERTA_D_SWS_SWREG_002r_t  sw_reg02;
    plp_aperta_phymod_core_status_t core_status;

    PHYMOD_MEMSET(&sw_reg02, 0, sizeof(sw_reg02));
    PHYMOD_MEMSET(&core_init_config, 0, sizeof(plp_aperta_phymod_core_init_config_t));
    PHYMOD_MEMSET(&init_config, 0, sizeof(plp_aperta_phymod_phy_init_config_t));
    /*Using top hard reset*/
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL1r(&core->access, &core_reset));
    if (reset_mode == phymodResetModeHard) {
        BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_RESETBf_SET(core_reset, 0);
    } else {
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_SWS_SWREG_002r(&core->access, &sw_reg02)); 
       init_config.interface.device_aux_modes = &fw_init_param;
       core_init_config.pll0_div_init_value = phymod_APERTA_TSCBH_PLL_DIVNONE;
       core_init_config.interface.ref_clock = phymodRefClk156Mhz;
 
        /* Retreive macsec static bypass from swgpreg2*/
        fw_init_param.macsec_static_bypass= sw_reg02.v[0] ? 1 : 0;
        /*Soft reset*/
        PHYMOD_MEMCPY(&phy_access, core, sizeof(phy_access));
        BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET(core_reset, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_CTRL_SWGPREG0Er(&core->access, &swgpreg_e));
        pll = swgpreg_e.v[0] >> 8;
        if (pll == bcmplpapertaVco20p625G) {
            speed = 40000;
            if (core_init_config.interface.ref_clock == phymodRefClk156Mhz) {
                core_init_config.pll1_div_init_value = phymod_APERTA_TSCBH_PLL_DIV132;
            }
        } else if (pll == bcmplpapertaVco25p781G) {
            speed = 100000;
            if (core_init_config.interface.ref_clock == phymodRefClk156Mhz) {
                core_init_config.pll1_div_init_value = phymod_APERTA_TSCBH_PLL_DIV165;
            }
        } else if (pll == bcmplpapertaVco26p562G) {
            speed = 400000;
            if (core_init_config.interface.ref_clock == phymodRefClk156Mhz) {
                core_init_config.pll1_div_init_value = phymod_APERTA_TSCBH_PLL_DIV170;
            }
        }
        fw_init_param.pll1_vco_rate = pll;
    }
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&core->access, core_reset));
    
    if (reset_mode == phymodResetModeSoft) {
        BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_t gpreg_1;
        int retry_cnt = 100;
         for (index = 0; index < APERTA_MAX_PM_INFO; index++) {
            if (core->access.addr == _plp_aperta_pm_info[index].phy_id) {
                for (lane=0 ; lane < APERTA_PM_NUM_LANES; lane++) {
                    _plp_aperta_pm_info[index].speed[lane] = speed;
                    _plp_aperta_pm_info[index].sys_speed[lane] = speed;
                }
                break;
            }
        }

        do {
            PHYMOD_USLEEP(5000);
            PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_GEN_CNTRLS_GPREG_01r(&core->access, &gpreg_1));
            if (BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1)& 1) {
                /* Check for uc active*/
                if ((BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1) & 0xFF00) != 0xFF00) {
                    PHYMOD_DEBUG_ERROR(("FW init seq failed with :%x for Phy:%x\n",
                                BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1),
                                core->access.addr));
                    return PHYMOD_E_FAIL;
                }
                break;
            }
        } while (--retry_cnt);
        if (retry_cnt <= 0) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_FAIL,
                               (_PHYMOD_MSG("ERR:SOFT RESET FAILS")));
        }
        phy_access.access.lane_mask=0xFF;
        phy_access.port_loc=phymodPortLocSys;
        phy_access.access.tvco_pll_index=APERTA_TVCO_PLL_INDEX;
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta_tscbh_core_init_pass1((const plp_aperta_phymod_core_access_t*)&phy_access, &core_init_config, &core_status));
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta_tscbh_core_init_pass2((const plp_aperta_phymod_core_access_t*)&phy_access, &core_init_config, &core_status));
        phy_access.port_loc=phymodPortLocLine;
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta_tscbh_core_init_pass1((const plp_aperta_phymod_core_access_t*)&phy_access, &core_init_config, &core_status));
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta_tscbh_core_init_pass2((const plp_aperta_phymod_core_access_t*)&phy_access, &core_init_config, &core_status));
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_phy_init(&phy_access, &init_config));
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_CTRL_SWGPREG0Er(&core->access, swgpreg_e));

        (void) core_status;
        for (lane=0 ; lane < APERTA_PM_NUM_LANES; lane++) {
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_port_active_reset(&phy_access, lane));
        }
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta_core_firmware_info_get(const plp_aperta_phymod_access_t *acc, plp_aperta_phymod_core_firmware_info_t* fw_info)
{
    PHYMOD_IF_ERR_RETURN(PHYMOD_BUS_READ(
                       acc, BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr, &fw_info->fw_version));
    PHYMOD_IF_ERR_RETURN(PHYMOD_BUS_READ(
                       acc, BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r, &fw_info->fw_crc));

    return PHYMOD_E_NONE;
}

int plp_aperta_dload_fw(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config)
{
    int rv = 0;
    int mst_boot_addr = 0, master_en=1;

    if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
        rv = _plp_aperta_download_prog_eeprom(core, NULL, 0, master_en, mst_boot_addr,
            0,
            init_config);
    } else {
        rv = _plp_aperta_download_prog_eeprom(core, plp_aperta_ucode, plp_aperta_ucode_len, master_en, mst_boot_addr,
            (init_config->firmware_load_method == phymodFirmwareLoadMethodProgEEPROM) ? 1 : 0, init_config);
    }
    if ((rv != PHYMOD_E_NONE) && (rv != APERTA_FW_ALREADY_DOWNLOADED)) {
        PHYMOD_DEBUG_ERROR(("FW download failed : %d\n", rv));
        return PHYMOD_E_FAIL;
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta_phy_status_dump(const plp_aperta_phymod_phy_access_t *phy)
{
    plp_aperta_phymod_core_access_t core;
    plp_aperta_phymod_phy_access_t phy_copy, pm_phy_copy;
    int lane_index = 0, start_lane= 0, num_lane=0;
    phymod_firmware_lane_config_t firmware_lane_config;
    plp_aperta_phymod_phy_access_t *sa__ = &phy_copy;

    PHYMOD_MEMCPY(&core, phy, sizeof(plp_aperta_phymod_core_access_t));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(plp_aperta_phymod_phy_access_t));
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(plp_aperta_phymod_phy_access_t));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_DIAG_OUT((" ***************************************\n"));
    PHYMOD_DIAG_OUT((" ******* PHY status dump for Aperta PHY ID:0x%x ********\n", phy->access.addr));
    PHYMOD_DIAG_OUT((" ***************************************\n"));
    PHYMOD_DIAG_OUT((" ***************************************\n"));
    PHYMOD_DIAG_OUT((" ******* PHY status dump for side:%x ********\n", phy->port_loc));
    PHYMOD_DIAG_OUT((" ***************************************\n"));
    
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tscbh_phy_firmware_lane_config_get(phy, &firmware_lane_config));


    /* Enable eye margin control*/
    if (firmware_lane_config.ForcePAM4Mode) {
        for (lane_index = 0;lane_index < num_lane; lane_index++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + lane_index)) {
                continue;
            }
            sa__->access.lane_mask =  0x1 << (start_lane + lane_index);
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__,1));
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_blackhawk_tsc_set_pam_eye_margin_usr_ctrl(sa__, 1));
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__, 0));
        }
    }
    PHYMOD_SLEEP(5);
    if (phy->access.flags & BCM_PLP_INTERNAL_DUMP_L1) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_phy_pmd_info_dump(phy, "STD"));
    } else if (phy->access.flags & BCM_PLP_INTERNAL_DUMP_L2) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_phy_pmd_info_dump(phy, "STD"));
        PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_phy_pmd_info_dump(phy, "verbose"));
    } else if (phy->access.flags & BCM_PLP_INTERNAL_DUMP_L3) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_phy_pmd_info_dump(phy, "STD"));
        PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_phy_pmd_info_dump(phy, "verbose"));
        for (lane_index = 0; lane_index < APERTA_MAX_LANES; lane_index ++) {
            if (phy->access.lane_mask & (1 << lane_index)) {
                phy_copy.access.lane_mask = (1 << lane_index);
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_display_diag_data(&phy_copy, SRDS_DIAG_EYE));
            }
        }
    } else {
        PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_phy_pmd_info_dump(phy, "STD"));
    }
    if (firmware_lane_config.ForcePAM4Mode) {
        for (lane_index = 0;lane_index < num_lane; lane_index++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + lane_index)) {
                continue;
            }
	        sa__->access.lane_mask =  0x1 << (start_lane + lane_index);
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__,1));
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_blackhawk_tsc_set_pam_eye_margin_usr_ctrl(sa__, 0));
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__, 0));
        }
    }
    return PHYMOD_E_NONE;
}
/* I2C helper to read or write module
 * xfter_addr: To set transfer address for the module
 * slv_add:    Slave address to be programmed
 * xfer_cnt:   To set the transfer count
 * cmd:        I2C command type
 */
int _plp_aperta_set_module_command(const plp_aperta_phymod_access_t *pa, uint16_t xfer_addr,
                               uint32_t slv_addr, unsigned char xfer_cnt, APERTA_I2CM_CMD_E cmd)
{
    BCMI_APERTA_D_MODULE_CNTRL_CONTROLr_t module_ctrl;
    BCMI_APERTA_D_MODULE_CNTRL_STATUSr_t mctrl_status;
    BCMI_APERTA_D_MODULE_CNTRL_XFER_COUNTr_t mod_xfer_cnt;
    BCMI_APERTA_D_MODULE_CNTRL_ADDRESSr_t mod_add;
    BCMI_APERTA_D_MODULE_CNTRL_XFER_ADDRESSr_t mod_xfer_add;
    uint16_t retry_count = 5000, data = 0;
    uint32_t wait_timeout_us = 0;

    wait_timeout_us = ((2*(xfer_cnt+1))*100)/5;
    PHYMOD_MEMSET(&module_ctrl, 0, sizeof(BCMI_APERTA_D_MODULE_CNTRL_CONTROLr_t));
    PHYMOD_MEMSET(&mod_xfer_add, 0, sizeof(BCMI_APERTA_D_MODULE_CNTRL_XFER_ADDRESSr_t));
    PHYMOD_MEMSET(&mod_xfer_cnt, 0, sizeof(BCMI_APERTA_D_MODULE_CNTRL_XFER_COUNTr_t));
    PHYMOD_MEMSET(&mctrl_status, 0, sizeof(BCMI_APERTA_D_MODULE_CNTRL_STATUSr_t));
    PHYMOD_MEMSET(&mod_add, 0, sizeof(BCMI_APERTA_D_MODULE_CNTRL_ADDRESSr_t));

    if (cmd == APERTA_FLUSH) {
        /* Flush and reset the module controller */
        BCMI_APERTA_D_MODULE_CNTRL_CONTROLr_SET(module_ctrl, 0xC000);
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_MODULE_CNTRL_CONTROLr(pa, module_ctrl));
    } else {
        /* Program transfer address and transfer count */
        BCMI_APERTA_D_MODULE_CNTRL_XFER_ADDRESSr_SET(mod_xfer_add, xfer_addr);
        BCMI_APERTA_D_MODULE_CNTRL_XFER_COUNTr_SET(mod_xfer_cnt, xfer_cnt);
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_MODULE_CNTRL_XFER_ADDRESSr(pa, mod_xfer_add));
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_MODULE_CNTRL_XFER_COUNTr(pa, mod_xfer_cnt));
        if (cmd == APERTA_CURRENT_ADDRESS_READ) {
            /* Set current/sequential read operation */
            BCMI_APERTA_D_MODULE_CNTRL_CONTROLr_SET(module_ctrl, 0x8001);
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_MODULE_CNTRL_CONTROLr(pa, module_ctrl));
        } else if (cmd == APERTA_RANDOM_ADDRESS_READ ) {
            BCMI_APERTA_D_MODULE_CNTRL_ADDRESSr_SET(mod_add, slv_addr);
            /* Set random read operation */
            BCMI_APERTA_D_MODULE_CNTRL_CONTROLr_SET(module_ctrl, 0x8003);
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_MODULE_CNTRL_ADDRESSr(pa,mod_add));
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_MODULE_CNTRL_CONTROLr(pa, module_ctrl));
        } else { /* APERTA_I2C_WRITE */
            BCMI_APERTA_D_MODULE_CNTRL_ADDRESSr_SET(mod_add, slv_addr);
            BCMI_APERTA_D_MODULE_CNTRL_CONTROLr_SET(module_ctrl, 0x8022);
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_MODULE_CNTRL_ADDRESSr(pa,mod_add));
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_MODULE_CNTRL_CONTROLr(pa, module_ctrl));
        }
    }

    if ((cmd == APERTA_CURRENT_ADDRESS_READ) ||
        (cmd == APERTA_RANDOM_ADDRESS_READ) ||
        (cmd == APERTA_I2C_WRITE)) {
        /* Wait for I2C master transaction done, data is set to 1 when transaction is done */
        do {
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_MODULE_CNTRL_STATUSr(pa,&mctrl_status));
            data = BCMI_APERTA_D_MODULE_CNTRL_STATUSr_XACTION_DONEf_GET(mctrl_status);
            PHYMOD_USLEEP(wait_timeout_us);
        } while((data == 0) && --retry_count);
        if(!retry_count) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_TIMEOUT, (_PHYMOD_MSG("Module controller: I2C transaction failed..")));
        }
    }
    BCMI_APERTA_D_MODULE_CNTRL_CONTROLr_SET(module_ctrl, 0x3);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_MODULE_CNTRL_CONTROLr(pa, module_ctrl));

    return PHYMOD_E_NONE;
}

/* Helper function for module read or write operation */
int _plp_aperta_module_read_write_helper(const plp_aperta_phymod_phy_access_t *phy, uint32_t slv_dev_addr, uint32_t start_addr,
    uint32_t no_of_bytes, uint8_t operation_type, uint8_t *read_data, const uint8_t *write_data)
{
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL3r_t   gen_ctrl3;
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_t   gen_ctrl1;
    BCMI_APERTA_D_MODULE_CNTRL_DEV_IDr_t         dev_id;
    BCMI_APERTA_D_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_t mod_ram_mdio_ctrl;
    int module_id = 0;
    uint32_t rd_data = 0;
    uint16_t START_OF_NVRAM = 0x0;
    uint32_t index = 0;
    uint32_t lower_page_bytes = 0;
    uint32_t upper_page_bytes = 0;
    uint32_t upper_page_flag = 0;
    uint32_t lower_page_start_addr = 0;
    uint32_t upper_page_start_addr = 0;
    uint32_t lower_page_flag = 0;

    if (APERTA_IS_LINE_SIDE(phy)) {
        module_id = 1;
    } else {
        module_id = 0;
    }
    /* Set the QSFP mode */
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL3r(&phy->access, &gen_ctrl3));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL3r_QSFP_MODEf_SET(gen_ctrl3, 1);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL3r(&phy->access, gen_ctrl3));

    /* Flush and reset at the beginning */
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, &gen_ctrl1));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_MODCTRL_RSTBf_SET(gen_ctrl1, 0);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, gen_ctrl1));

    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, &gen_ctrl1));

    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_MODCTRL_RSTBf_SET(gen_ctrl1, 1);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, gen_ctrl1));

    /* Perform module controller reset and FLUSH */
    PHYMOD_IF_ERR_RETURN(
        _plp_aperta_set_module_command(&phy->access, 0, 0, 0, APERTA_FLUSH));

    /* Configure the slave device ID default is 0x50 */
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_MODULE_CNTRL_DEV_IDr(&phy->access, &dev_id));
    BCMI_APERTA_D_MODULE_CNTRL_DEV_IDr_SL_DEV_ADDf_SET(dev_id, slv_dev_addr);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_MODULE_CNTRL_DEV_IDr(&phy->access, dev_id));

    /* select module */
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL3r(&phy->access, &gen_ctrl3));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL3r_EXTMOD_SELECTf_SET(gen_ctrl3, module_id);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL3r(&phy->access, gen_ctrl3));

    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr(&phy->access, &mod_ram_mdio_ctrl));
    BCMI_APERTA_D_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_MD_EXTMOD_SELECTf_SET(mod_ram_mdio_ctrl, module_id);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr(&phy->access, mod_ram_mdio_ctrl));

    if(no_of_bytes == 0) {
        /* Perform module controller reset and FLUSH */
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta_set_module_command(&phy->access, 0, 0, 0, APERTA_FLUSH));

        return PHYMOD_E_NONE;
    }
    /* if requested number of bytes are not within the boundary (0- 255)
     * need to calculate what maximum number of bytes can be taken into
     * account for reading or writing to module
     */
    if ((no_of_bytes + start_addr) >= 256) {
        no_of_bytes = 255 - start_addr + 1;
    }

    /* To determine page to be written is lower page or upper page or
     * both lower and upper page
     */
    if ((start_addr + no_of_bytes - 1) > 127) {
        /* lower page */
        if (start_addr <= 127) {
            lower_page_bytes = 127 - start_addr + 1;
            lower_page_flag = 1;
            lower_page_start_addr = start_addr;
        }
        /* upper page */
        if ((start_addr + no_of_bytes) > 127) {
            upper_page_flag = 1;
            upper_page_bytes = no_of_bytes - lower_page_bytes;
            if(start_addr > 128) {
                upper_page_start_addr = start_addr;
            } else {
                upper_page_start_addr = 128;
            }
        }
    } else { /* only lower page */
        lower_page_bytes = no_of_bytes;
        lower_page_flag = 1;
        lower_page_start_addr = start_addr;
    }

    /* Write Operation */
    if (operation_type == 1) {
        /* Write data to NVRAM */
        for (index = 0; index < no_of_bytes; index++) {
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_reg32_write(phy, (0x10000 + APERTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM + start_addr + index),  write_data[index]));
        }
        if(lower_page_flag) {
            for (index = 0; index < (lower_page_bytes / 4); index ++) {
                PHYMOD_IF_ERR_RETURN(
                    _plp_aperta_set_module_command(&phy->access, APERTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM +
                        lower_page_start_addr + (4 * index), lower_page_start_addr + (4 * index), 3, APERTA_I2C_WRITE));
            }
            if ((lower_page_bytes % 4) > 0) {
                PHYMOD_IF_ERR_RETURN(
                    _plp_aperta_set_module_command(&phy->access, APERTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM +
                        lower_page_start_addr + (4 * index), lower_page_start_addr + (4 * index),
                                                       ((lower_page_bytes % 4) - 1), APERTA_I2C_WRITE));
            }
            lower_page_flag = 0;
        }
        if(upper_page_flag) {
            for (index = 0; index < (upper_page_bytes / 4); index++) {
                PHYMOD_IF_ERR_RETURN(
                    _plp_aperta_set_module_command(&phy->access, (APERTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM +
                        upper_page_start_addr + (4 * index)), upper_page_start_addr + (4 * index), 3, APERTA_I2C_WRITE));
            }
            if ((upper_page_bytes%4) > 0) {
                PHYMOD_IF_ERR_RETURN(
                    _plp_aperta_set_module_command(&phy->access, (APERTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM +
                        upper_page_start_addr + (4 * index)), upper_page_start_addr + (4 * index),
                                                        ((upper_page_bytes % 4) - 1), APERTA_I2C_WRITE));
            }
            upper_page_flag = 0;
        }
    } else if (operation_type == 0){ /* Read operation */
        if (lower_page_flag) {
            PHYMOD_IF_ERR_RETURN(
                _plp_aperta_set_module_command(&phy->access, APERTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM +
                    lower_page_start_addr, lower_page_start_addr, lower_page_bytes - 1, APERTA_RANDOM_ADDRESS_READ));
            lower_page_flag = 0;
        }
        /* Need to check with chip team how we can read upper page */
        if (upper_page_flag) {
            PHYMOD_IF_ERR_RETURN(
                _plp_aperta_set_module_command(&phy->access, APERTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM +
                    upper_page_start_addr, upper_page_start_addr, upper_page_bytes - 1, APERTA_RANDOM_ADDRESS_READ));
            upper_page_flag = 0;
        }

        /* Read data from NVRAM using I2C*/
        for (index = 0; index < no_of_bytes; index++) {
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_reg32_read(phy, (0x10000 + APERTA_MODULE_CNTRL_RAM_NVR0_ADR + start_addr + index),  &rd_data));
            read_data[index] = (unsigned char) (rd_data & 0xff);
        }
    } else {
        return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;
}

/* Module read function based on slave address, starting address and number of bytes */
int plp_aperta_module_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t slv_dev_addr, uint32_t start_addr, uint32_t no_of_bytes, uint8_t *read_data)
{
    PHYMOD_IF_ERR_RETURN(_plp_aperta_module_read_write_helper(phy, slv_dev_addr, start_addr, no_of_bytes,
        0 /* read operation */, read_data, NULL /* write data */));
    return PHYMOD_E_NONE;
}

/* Module write function based on slave address, starting address and number of bytes */
int plp_aperta_module_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t slv_dev_addr, uint32_t start_addr, uint32_t no_of_bytes, const uint8_t *write_data)
{
    PHYMOD_IF_ERR_RETURN(_plp_aperta_module_read_write_helper(phy, slv_dev_addr, start_addr,
        no_of_bytes, 1 /* write operation */, NULL /* read data */, write_data));
    return PHYMOD_E_NONE;
}

int _plp_aperta_phy_gpio_config_set(const plp_aperta_phymod_phy_access_t* phy, int pin_no, plp_aperta_phymod_gpio_mode_t gpio_mode)
{
    uint32_t  pad_ctrl_gpio_x_ctrl_rd = 0;
    uint32_t  pad_ctrl_gpio_x_ctrl_wr = 0;
    uint16_t data = 0;

    switch (gpio_mode) {
        case phymodGpioModeDisabled:
            return PHYMOD_E_NONE;
        case phymodGpioModeOutput:
            data = 0;
        break;
        case phymodGpioModeInput:
            data = 1;
        break;
        default:
          PHYMOD_DEBUG_ERROR(("Invalid GPIO MODE\n"));
          return PHYMOD_E_PARAM;
    }

    if((pin_no >= APERTA_GPIO_MIN_PIN) && (pin_no <= APERTA_GPIO_MAX_PIN)) {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_direct_reg_read(phy, APERTA_GPIO_CTRL1_BASE_ADDR + (pin_no * 4), &pad_ctrl_gpio_x_ctrl_rd));
        pad_ctrl_gpio_x_ctrl_wr = ((pad_ctrl_gpio_x_ctrl_rd & ~APERTA_GPIO_CTRL_OEBF_MASK) | data );
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_direct_reg_write(phy, APERTA_GPIO_CTRL1_BASE_ADDR + (pin_no * 4), pad_ctrl_gpio_x_ctrl_wr));
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Invalid GPIO pin selected")));
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta_phy_gpio_config_get(const plp_aperta_phymod_phy_access_t* phy, int pin_no, plp_aperta_phymod_gpio_mode_t* gpio_mode)
{
    uint32_t  pad_ctrl_gpio_x_ctrl_rd = 0;
    uint16_t  data = 0;

    if((pin_no >= APERTA_GPIO_MIN_PIN) && (pin_no <= APERTA_GPIO_MAX_PIN)) {
        PHYMOD_IF_ERR_RETURN(
             plp_aperta_direct_reg_read(phy, APERTA_GPIO_CTRL1_BASE_ADDR + (pin_no * 4), &pad_ctrl_gpio_x_ctrl_rd));
        data = (pad_ctrl_gpio_x_ctrl_rd & APERTA_GPIO_CTRL_OEBF_MASK);
        if (data) {
            *gpio_mode = phymodGpioModeInput ;
        } else {
            *gpio_mode = phymodGpioModeOutput ;
        }
    } else {
        *gpio_mode = phymodGpioModeDisabled ;
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Invalid GPIO pin selected")));
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta_phy_gpio_pin_value_set(const plp_aperta_phymod_phy_access_t* phy, int pin_no, int value)
{
    uint32_t  pad_ctrl_gpio_ctrl = 0;
    uint16_t  pull_up_dwn_data = ((value >> 0x1)& 0x1) ? 0x1 /* PULL UP */ : 0x2 /* PULL DOWN */ ;

    if ((pin_no >= APERTA_GPIO_MIN_PIN) && (pin_no <= APERTA_GPIO_MAX_PIN)) {
        PHYMOD_IF_ERR_RETURN(
             plp_aperta_direct_reg_read(phy, APERTA_GPIO_CTRL1_BASE_ADDR + (pin_no * 4), &pad_ctrl_gpio_ctrl));
        pad_ctrl_gpio_ctrl &= ~(3 << 5);
        pad_ctrl_gpio_ctrl |= ((value &1) ? (3 << 5) : 0);
        PHYMOD_IF_ERR_RETURN(
             plp_aperta_direct_reg_write(phy, APERTA_GPIO_CTRL1_BASE_ADDR + (pin_no * 4), pad_ctrl_gpio_ctrl));

        PHYMOD_IF_ERR_RETURN(
             plp_aperta_direct_reg_read(phy, APERTA_GPIO_CTRL0_BASE_ADDR + (pin_no * 4), &pad_ctrl_gpio_ctrl));
        if (pull_up_dwn_data == 1) { /* PULL UP*/
            pad_ctrl_gpio_ctrl &= ~(3);
            pad_ctrl_gpio_ctrl |= 1;
        }
        if (pull_up_dwn_data == 2) { /* PULL Down*/
            pad_ctrl_gpio_ctrl &= ~(3);
            pad_ctrl_gpio_ctrl |= 2;
        }
        PHYMOD_IF_ERR_RETURN(
             plp_aperta_direct_reg_write(phy, APERTA_GPIO_CTRL0_BASE_ADDR + (pin_no * 4), pad_ctrl_gpio_ctrl));
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Invalid GPIO pin selected")));
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta_phy_gpio_pin_value_get(const plp_aperta_phymod_phy_access_t* phy, int pin_no, int* value)
{
    uint32_t pad_ctrl_gpio_sts = 0;
    uint32_t pull_up_dwn_data = 0 ;
    uint16_t cfg_value = 0;
    
    if((pin_no >= APERTA_GPIO_MIN_PIN) && (pin_no <= APERTA_GPIO_MAX_PIN)) {
        PHYMOD_IF_ERR_RETURN(
             plp_aperta_direct_reg_read(phy, APERTA_GPIO_CTRL0_BASE_ADDR + (pin_no * 4), &pull_up_dwn_data));
        PHYMOD_IF_ERR_RETURN(
             plp_aperta_direct_reg_read(phy, APERTA_GPIO_STS_BASE_ADDR + (pin_no * 4), &pad_ctrl_gpio_sts));

        pull_up_dwn_data &= 0x03;

        if (pull_up_dwn_data & 0x1) {
            cfg_value = 0x1 ;
        } else if (pull_up_dwn_data & 2) {
            cfg_value = 0 ;
        }
        *value = (pad_ctrl_gpio_sts & 0x20) >> 5;
        *value |= cfg_value << 1 ;
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Invalid GPIO pin selected")));
    }
    return PHYMOD_E_NONE;
}

uint32_t _plp_aperta_convert_interface_type_to_numeric_value(plp_aperta_phymod_interface_t if_type)
{
    const plp_aperta_phymod_interface_t aperta_if_type_list[] = APERTA_IF_TYPE_LIST_ELEMENTS ;
    uint8_t i = 0 ;
    for(i=0; i < (sizeof(aperta_if_type_list)/sizeof(aperta_if_type_list[0])); i++) {
       if(aperta_if_type_list[i] == if_type){
           return(i) ;
       }
    }
    return(0);
}

plp_aperta_phymod_interface_t
_plp_aperta_convert_numeric_value_to_interface_type(uint32_t numeric_val)
{
    const plp_aperta_phymod_interface_t aperta_if_type_list[] = APERTA_IF_TYPE_LIST_ELEMENTS ;
    if(numeric_val < (sizeof(aperta_if_type_list)/sizeof(aperta_if_type_list[0]))) {
        return (aperta_if_type_list[numeric_val]) ;
        } else {
        return phymodInterfaceBypass;
        }
}

int plp_aperta_sw_intf_set(const plp_aperta_phymod_phy_access_t *phy, plp_aperta_phymod_interface_t if_type) {
    uint32_t if_type_numeric_val = 0, if_type_numeric_val_bit_4 = 0 ;
    uint32_t if_type_rd = 0;
    uint32_t if_type_wr = 0;
    uint8_t lane_index  = 0;
    uint8_t sys_side    = 0;

    if_type_numeric_val = _plp_aperta_convert_interface_type_to_numeric_value(if_type);
    if_type_numeric_val_bit_4 = (if_type_numeric_val & 0x10) >> 4;
    if_type_numeric_val &= 0xF;

    if (APERTA_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }

    for (lane_index = 0; lane_index < APERTA_MAX_LANES; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                  plp_aperta_direct_reg_read(phy, (APERTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
                                          (sys_side * APERTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) +
                                          (lane_index/4)),
                                          &if_type_rd));
           if_type_wr  = (if_type_rd & ~(APERTA_IF_TYPE_PER_LANE_STORAGE_MASK << ((lane_index & 0x3) << 0x2)));
           if_type_wr |= ((if_type_numeric_val & APERTA_IF_TYPE_PER_LANE_STORAGE_MASK) << ((lane_index & 0x3) << 0x2)) ;
            PHYMOD_IF_ERR_RETURN(
                  plp_aperta_direct_reg_write(phy, (APERTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
                                           (sys_side * APERTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) +
                                           (lane_index/4)),
                                           if_type_wr));
            break ;
        }
    }
    /* Using one more bit to support more interface type*/ 
    for (lane_index = 0; lane_index < APERTA_MAX_LANES; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                  plp_aperta_direct_reg_read(phy, APERTA_IF_TYPE_SAVE_SWGPREG_BIT4, &if_type_rd));
           if_type_wr  = if_type_rd & ~(1 << (lane_index + (sys_side*8)));
           if_type_wr |= (if_type_numeric_val_bit_4 & 1) << (lane_index + (sys_side*8));
           PHYMOD_IF_ERR_RETURN(
              plp_aperta_direct_reg_write(phy, APERTA_IF_TYPE_SAVE_SWGPREG_BIT4, if_type_wr));
           break ;
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_sw_intf_get(const plp_aperta_phymod_phy_access_t *phy, uint8_t lane_index, plp_aperta_phymod_interface_t *if_type) {
    uint32_t if_type_rd = 0;
    uint8_t sys_side    = 0;
    uint32_t if_type_numeric_val_bit_4 = 0;
    if (APERTA_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }
    PHYMOD_IF_ERR_RETURN(
          plp_aperta_direct_reg_read(phy, (APERTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
                                  (sys_side * APERTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) +
                                  (lane_index/4)),
                                  &if_type_rd));
    
    /* Using one more bit to support more interface type*/ 
    PHYMOD_IF_ERR_RETURN(
         plp_aperta_direct_reg_read(phy, APERTA_IF_TYPE_SAVE_SWGPREG_BIT4, &if_type_numeric_val_bit_4));
    if_type_numeric_val_bit_4 = (if_type_numeric_val_bit_4 >> (lane_index + (sys_side*8))) & 1;
    if_type_rd = (if_type_rd >> ((lane_index & 0x3)<<2)) & APERTA_IF_TYPE_PER_LANE_STORAGE_MASK;
    if_type_rd |= (if_type_numeric_val_bit_4 << 4); 


   *if_type = _plp_aperta_convert_numeric_value_to_interface_type(if_type_rd) ;
    return PHYMOD_E_NONE;
}


int _plp_aperta_phy_intr_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t intr_type_enable)
{
    int port = 0, speed = 0, lane_sel = 0, enable = 0, lane_data_rate = 0;
    unsigned int port_int_reg[8] = {BCMI_APERTA_D_EXT_INTR_CTRL_P0_EIERr, BCMI_APERTA_D_EXT_INTR_CTRL_P1_EIERr,
        BCMI_APERTA_D_EXT_INTR_CTRL_P2_EIERr, BCMI_APERTA_D_EXT_INTR_CTRL_P3_EIERr, BCMI_APERTA_D_EXT_INTR_CTRL_P4_EIERr,
        BCMI_APERTA_D_EXT_INTR_CTRL_P5_EIERr, BCMI_APERTA_D_EXT_INTR_CTRL_P6_EIERr, BCMI_APERTA_D_EXT_INTR_CTRL_P7_EIERr};
    BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_t common_eier;
    BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIERr_t m0_eier;
    unsigned int data = 0;

    PHYMOD_MEMSET(&common_eier, 0, sizeof(BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_t));
    PHYMOD_MEMSET(&m0_eier, 0, sizeof(BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIERr_t));

    APERTA_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, port, lane_sel);
    (void) lane_sel;
    enable = (intr_type_enable >> 31) & 1;
    if (intr_type_enable & APERTA_INTR_PORT) {
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, port_int_reg[port], &data));
        if (intr_type_enable & APERTA_INTR_PORT_PTP) {
            data &= ~(1 << 12);
            data |= (enable << 12);
        }
        if (intr_type_enable & APERTA_INTR_PORT_LINK_DOWN) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                data &= ~(1 << 11);
                data |= (enable << 11);
            } else {
                data &= ~(1 << 10);
                data |= (enable << 10);
            }
        }
        if (intr_type_enable & APERTA_INTR_PORT_SF_ERR_INTR) {
           data &= ~(1 << 5);
           data |= (enable << 5);
        }
        if (intr_type_enable & APERTA_INTR_PORT_SF_DED_INTR) {
           data &= ~(1 << 4);
           data |= (enable << 4);
        }
        if (intr_type_enable & APERTA_INTR_PORT_FC_ERR_INTR) {
            /* Egress & ingress*/
           data &= ~(1 << 3);
           data |= (enable << 3);
           data &= ~(1 << 9);
           data |= (enable << 9);
        }
        if (intr_type_enable & APERTA_INTR_PORT_FC_DED_INTR) {
            /* Egress*/
           data &= ~(1 << 2);
           data |= (enable << 2);
            /* ingress*/
           data &= ~(1 << 8);
           data |= (enable << 8);
        }
        if (intr_type_enable & APERTA_INTR_PORT_INTF_ERR_INTR) {
            /* Egress*/
           data &= ~(1 << 1);
           data |= (enable << 1);
            /* ingress*/
           data &= ~(1 << 7);
           data |= (enable << 7);
        }
        if (intr_type_enable & APERTA_INTR_PORT_INTF_DED_INTR) {
            /* Egress*/
           data &= ~(1 << 0);
           data |= (enable << 0);
            /* ingress*/
           data &= ~(1 << 6);
           data |= (enable << 6);
        }
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, port_int_reg[port], data));
    } else if (intr_type_enable & APERTA_INTR_COMMON) {
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_EXT_INTR_CTRL_COMMON_CTRL_EIERr(&phy->access, &common_eier));
        if (intr_type_enable & APERTA_INTR_CMN_MOD_ABS_RISING) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_ABS_0_RISINGf_SET(common_eier, enable);
            } else {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_ABS_1_RISINGf_SET(common_eier, enable);
            }
        }
        if (intr_type_enable & APERTA_INTR_CMN_MOD_ABS_FALL) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_ABS_0_FALLINGf_SET(common_eier, enable);
            } else {                                                                                                  
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_ABS_1_FALLINGf_SET(common_eier, enable);
            }
        }
        if (intr_type_enable & APERTA_INTR_CMN_MOD_EXT_RAISING) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_EXT_INTR_0_RISINGf_SET(common_eier, enable);
            } else {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_EXT_INTR_1_RISINGf_SET(common_eier, enable);
            }
        }
        if (intr_type_enable & APERTA_INTR_CMN_MOD_EXT_FALL) {                                 
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
               BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_EXT_INTR_0_FALLINGf_SET(common_eier, enable);
            } else {
               BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_EXT_INTR_1_FALLINGf_SET(common_eier, enable);
            }
        }
        if (intr_type_enable & APERTA_INTR_CMN_LMI_ERR) {
            BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_LMI_ERR_INTRf_SET(common_eier, enable);
        }
        if (intr_type_enable & APERTA_INTR_CMN_LMI_DED) {
            BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_LMI_DED_INTRf_SET(common_eier, enable);
        }
        if (intr_type_enable & APERTA_INTR_CMN_PMIF_ERR) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_SPMIF_ERR_INTRf_SET(common_eier, enable);
            } else {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_LPMIF_ERR_INTRf_SET(common_eier, enable);
            }
        }
        if (intr_type_enable & APERTA_INTR_CMN_PMIF_DED) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_SPMIF_DED_INTRf_SET(common_eier, enable);
            } else {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_LPMIF_DED_INTRf_SET(common_eier, enable);
            }
        }
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_EXT_INTR_CTRL_COMMON_CTRL_EIERr(&phy->access, common_eier));
    } else if (intr_type_enable & APERTA_INTR_MO) {
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_EXT_INTR_CTRL_M0_EIERr(&phy->access, &m0_eier));
        if (intr_type_enable & APERTA_INTR_M0_MST_FW_WDOG_EXP) {
            BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_FW_WDOG_EXPf_SET(m0_eier, enable);
        }
        if (intr_type_enable & APERTA_INTR_M0_MST_DRAM_DED) {
            BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_DRAM_DEDf_SET(m0_eier, enable);
        }
        if (intr_type_enable & APERTA_INTR_M0_MST_DED) {
            BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_DEDf_SET(m0_eier, enable);
        }
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_EXT_INTR_CTRL_M0_EIERr(&phy->access, m0_eier));
    } else {
        PHYMOD_DEBUG_ERROR(("Invalid Interrupt Type\n"));
        return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta_phy_intr_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *intr_enable)
{
    int port = 0, speed = 0, lane_sel = 0, lane_data_rate = 0;
    unsigned int port_int_reg[8] = {BCMI_APERTA_D_EXT_INTR_CTRL_P0_EIERr, BCMI_APERTA_D_EXT_INTR_CTRL_P1_EIERr,
        BCMI_APERTA_D_EXT_INTR_CTRL_P2_EIERr, BCMI_APERTA_D_EXT_INTR_CTRL_P3_EIERr, BCMI_APERTA_D_EXT_INTR_CTRL_P4_EIERr,
        BCMI_APERTA_D_EXT_INTR_CTRL_P5_EIERr, BCMI_APERTA_D_EXT_INTR_CTRL_P6_EIERr, BCMI_APERTA_D_EXT_INTR_CTRL_P7_EIERr};
    BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_t common_eier;
    BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIERr_t m0_eier;
    unsigned int data = 0;

    PHYMOD_MEMSET(&common_eier, 0, sizeof(BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_t));
    PHYMOD_MEMSET(&m0_eier, 0, sizeof(BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIERr_t));
    *intr_enable = 0;
    APERTA_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, port, lane_sel);
    (void) lane_sel;
    PHYMOD_IF_ERR_RETURN(
        PHYMOD_BUS_READ(&phy->access, port_int_reg[port], &data));
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_EXT_INTR_CTRL_COMMON_CTRL_EIERr(&phy->access, &common_eier));
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_EXT_INTR_CTRL_M0_EIERr(&phy->access, &m0_eier));

    if (data & (1<<12)) {
        *intr_enable |= APERTA_INTR_PORT_PTP;
    }
    if ((data & (1<<11)) && (APERTA_IS_SYSTEM_SIDE(phy))) {
        *intr_enable |= APERTA_INTR_PORT_LINK_DOWN;
    }
    if ((data & (1<<10)) && (APERTA_IS_LINE_SIDE(phy))) {
        *intr_enable |= APERTA_INTR_PORT_LINK_DOWN;
    }
    if (data & (1<<5)) {
        *intr_enable |= APERTA_INTR_PORT_SF_ERR_INTR;
    }
    if (data & (1<<4)) {
        *intr_enable |= APERTA_INTR_PORT_SF_DED_INTR;
    }
    if (data & (1<<3)) {
        *intr_enable |= APERTA_INTR_PORT_FC_ERR_INTR;
    }
    if (data & (1<<2)) {
        *intr_enable |= APERTA_INTR_PORT_FC_DED_INTR;
    }
    if (data & (1<<1)) {
        *intr_enable |= APERTA_INTR_PORT_INTF_ERR_INTR;
    }
    if (data & 1) {
        *intr_enable |= APERTA_INTR_PORT_INTF_DED_INTR;
    }
    if (APERTA_IS_SYSTEM_SIDE(phy)) {
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_ABS_0_RISINGf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_MOD_ABS_RISING;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_ABS_0_FALLINGf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_MOD_ABS_FALL;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_EXT_INTR_0_RISINGf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_MOD_EXT_RAISING;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_EXT_INTR_0_FALLINGf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_MOD_EXT_FALL;
        }
    } else {
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_ABS_1_RISINGf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_MOD_ABS_RISING;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_ABS_1_FALLINGf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_MOD_ABS_FALL;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_EXT_INTR_1_RISINGf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_MOD_EXT_RAISING;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_MOD_EXT_INTR_1_FALLINGf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_MOD_EXT_FALL;
        }
    }
    if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_LMI_ERR_INTRf_GET(common_eier)) {
        *intr_enable |= APERTA_INTR_CMN_LMI_ERR;
    }
    if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_LMI_DED_INTRf_GET(common_eier)) {
        *intr_enable |= APERTA_INTR_CMN_LMI_DED;
    }
    if (APERTA_IS_SYSTEM_SIDE(phy)) {
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_SPMIF_ERR_INTRf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_PMIF_ERR;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_SPMIF_DED_INTRf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_PMIF_DED;
        }
    } else {
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_LPMIF_ERR_INTRf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_PMIF_ERR;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIERr_ENABLE_LPMIF_DED_INTRf_GET(common_eier)) {
            *intr_enable |= APERTA_INTR_CMN_PMIF_DED;
        }

    }
    if (BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_FW_WDOG_EXPf_GET(m0_eier)) {
        *intr_enable |= APERTA_INTR_M0_MST_FW_WDOG_EXP;
    }
    if (BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_DRAM_DEDf_GET(m0_eier)) {
        *intr_enable |= APERTA_INTR_M0_MST_DRAM_DED;
    }
    if (BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_DEDf_GET(m0_eier)) {
        *intr_enable |= APERTA_INTR_M0_MST_DED;
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta_phy_intr_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *intr_sts)
{
    int port = 0, speed = 0, lane_sel = 0, lane_data_rate = 0;
    BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_t common_eipr;
    BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_t common_eisr;
    BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIPRr_t m0_eipr;
    BCMI_APERTA_D_EXT_INTR_CTRL_M0_EISRr_t m0_eisr;
    unsigned int data = 0, intr_type = 0, event = 0;

    PHYMOD_MEMSET(&common_eipr, 0, sizeof(BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_t));
    PHYMOD_MEMSET(&common_eisr, 0, sizeof(BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_t));
    PHYMOD_MEMSET(&m0_eipr, 0, sizeof(BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIPRr_t));
    PHYMOD_MEMSET(&m0_eisr, 0, sizeof(BCMI_APERTA_D_EXT_INTR_CTRL_M0_EISRr_t));
    APERTA_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, port, lane_sel);
    (void) lane_sel;

    intr_type = (*intr_sts) ? (*intr_sts & 0x7FFFFFFF): 0xFFFF;      (void) intr_type;
    event = (*intr_sts & 0x80000000) ? 1: 0;
    *intr_sts = 0;

    if (event) {
        unsigned int port_int_reg[8] = {BCMI_APERTA_D_EXT_INTR_CTRL_P0_EISRr, BCMI_APERTA_D_EXT_INTR_CTRL_P1_EISRr,
        BCMI_APERTA_D_EXT_INTR_CTRL_P2_EISRr, BCMI_APERTA_D_EXT_INTR_CTRL_P3_EISRr, BCMI_APERTA_D_EXT_INTR_CTRL_P4_EISRr,
        BCMI_APERTA_D_EXT_INTR_CTRL_P5_EISRr, BCMI_APERTA_D_EXT_INTR_CTRL_P6_EISRr, BCMI_APERTA_D_EXT_INTR_CTRL_P7_EISRr};
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, port_int_reg[port], &data));
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_EXT_INTR_CTRL_COMMON_CTRL_EISRr(&phy->access, &common_eisr));
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_EXT_INTR_CTRL_M0_EISRr(&phy->access, &m0_eisr));

        if (APERTA_IS_SYSTEM_SIDE(phy)) {
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_ABS_0_RISINGf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_ABS_RISING;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_ABS_0_FALLINGf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_ABS_FALL;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_EXT_INTR_0_RISINGf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_EXT_RAISING;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_EXT_INTR_0_FALLINGf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_EXT_FALL;
            }
        } else {
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_ABS_1_RISINGf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_ABS_RISING;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_ABS_1_FALLINGf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_ABS_FALL;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_EXT_INTR_1_RISINGf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_EXT_RAISING;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_EXT_INTR_1_FALLINGf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_EXT_FALL;
            }
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_LMI_ERR_INTRf_GET(common_eisr)) {
            *intr_sts |= APERTA_INTR_CMN_LMI_ERR;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_LMI_DED_INTRf_GET(common_eisr)) {
            *intr_sts |= APERTA_INTR_CMN_LMI_DED;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_LMI_DED_INTRf_GET(common_eisr)) {
            *intr_sts |= APERTA_INTR_CMN_LMI_DED;
        }
        if (APERTA_IS_SYSTEM_SIDE(phy)) {
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_SPMIF_ERR_INTRf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_PMIF_ERR;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_SPMIF_DED_INTRf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_PMIF_DED;
            }
        } else {
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_LPMIF_ERR_INTRf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_PMIF_ERR;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_LPMIF_DED_INTRf_GET(common_eisr)) {
                *intr_sts |= APERTA_INTR_CMN_PMIF_DED;
            }
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_M0_EISRr_MST_FW_WDOG_EXPf_GET(m0_eisr)) {
            *intr_sts |= APERTA_INTR_M0_MST_FW_WDOG_EXP;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_M0_EISRr_MST_DRAM_DEDf_GET(m0_eisr)) {
            *intr_sts |= APERTA_INTR_M0_MST_DRAM_DED;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_M0_EISRr_MST_DEDf_GET(m0_eisr)) {
            *intr_sts |= APERTA_INTR_M0_MST_DED;
        }
    } else {
        unsigned int port_int_reg[8] = {BCMI_APERTA_D_EXT_INTR_CTRL_P0_EIPRr, BCMI_APERTA_D_EXT_INTR_CTRL_P1_EIPRr,
        BCMI_APERTA_D_EXT_INTR_CTRL_P2_EIPRr, BCMI_APERTA_D_EXT_INTR_CTRL_P3_EIPRr, BCMI_APERTA_D_EXT_INTR_CTRL_P4_EIPRr,
        BCMI_APERTA_D_EXT_INTR_CTRL_P5_EIPRr, BCMI_APERTA_D_EXT_INTR_CTRL_P6_EIPRr, BCMI_APERTA_D_EXT_INTR_CTRL_P7_EIPRr};

        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, port_int_reg[port], &data));
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_EXT_INTR_CTRL_COMMON_CTRL_EIPRr(&phy->access, &common_eipr));
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_EXT_INTR_CTRL_M0_EIPRr(&phy->access, &m0_eipr));
        if (APERTA_IS_SYSTEM_SIDE(phy)) {
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_MOD_ABS_0_RISINGf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_ABS_RISING;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_MOD_ABS_0_FALLINGf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_ABS_FALL;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_MOD_EXT_INTR_0_RISINGf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_EXT_RAISING;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_MOD_EXT_INTR_0_FALLINGf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_EXT_FALL;
            }
        } else {
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_MOD_ABS_1_RISINGf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_ABS_RISING;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_MOD_ABS_1_FALLINGf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_ABS_FALL;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_MOD_EXT_INTR_1_RISINGf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_EXT_RAISING;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_MOD_EXT_INTR_1_FALLINGf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_MOD_EXT_FALL;
            }
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_LMI_ERR_INTRf_GET(common_eipr)) {
            *intr_sts |= APERTA_INTR_CMN_LMI_ERR;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_LMI_DED_INTRf_GET(common_eipr)) {
            *intr_sts |= APERTA_INTR_CMN_LMI_DED;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_LMI_DED_INTRf_GET(common_eipr)) {
            *intr_sts |= APERTA_INTR_CMN_LMI_DED;
        }
        if (APERTA_IS_SYSTEM_SIDE(phy)) {
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_SPMIF_ERR_INTRf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_PMIF_ERR;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_SPMIF_DED_INTRf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_PMIF_DED;
            }
        } else {
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_LPMIF_ERR_INTRf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_PMIF_ERR;
            }
            if (BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EIPRr_P_LPMIF_DED_INTRf_GET(common_eipr)) {
                *intr_sts |= APERTA_INTR_CMN_PMIF_DED;
            }
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIPRr_P_MST_FW_WDOG_EXPf_GET(m0_eipr)) {
            *intr_sts |= APERTA_INTR_M0_MST_FW_WDOG_EXP;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIPRr_P_MST_DRAM_DEDf_GET(m0_eipr)) {
            *intr_sts |= APERTA_INTR_M0_MST_DRAM_DED;
        }
        if (BCMI_APERTA_D_EXT_INTR_CTRL_M0_EIPRr_P_MST_DEDf_GET(m0_eipr)) {
            *intr_sts |= APERTA_INTR_M0_MST_DED;
        }
    }
    if (data & (1<<12)) {
        *intr_sts |= APERTA_INTR_PORT_PTP;
    }
    if ((data & (1<<11)) && (APERTA_IS_SYSTEM_SIDE(phy))) {
        *intr_sts |= APERTA_INTR_PORT_LINK_DOWN;
    }
    if ((data & (1<<10)) && (APERTA_IS_LINE_SIDE(phy))) {
        *intr_sts |= APERTA_INTR_PORT_LINK_DOWN;
    }
    if (data & (1<<5)) {
        *intr_sts |= APERTA_INTR_PORT_SF_ERR_INTR;
    }
    if (data & (1<<4)) {
        *intr_sts |= APERTA_INTR_PORT_SF_DED_INTR;
    }
    if (data & (1<<3)) {
        *intr_sts |= APERTA_INTR_PORT_FC_ERR_INTR;
    }
    if (data & (1<<2)) {
        *intr_sts |= APERTA_INTR_PORT_FC_DED_INTR;
    }
    if (data & (1<<1)) {
        *intr_sts |= APERTA_INTR_PORT_INTF_ERR_INTR;
    }
    if (data & 1) {
        *intr_sts |= APERTA_INTR_PORT_INTF_DED_INTR;
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta_phy_intr_status_clear(const plp_aperta_phymod_phy_access_t* phy, uint32_t intr_type)
{
    int port = 0, speed = 0, lane_sel = 0, lane_data_rate = 0;
    BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_t common_eisr;
    BCMI_APERTA_D_EXT_INTR_CTRL_M0_EISRr_t m0_eisr;
    unsigned int data = 0;
    unsigned int port_int_reg[8] = {BCMI_APERTA_D_EXT_INTR_CTRL_P0_EISRr, BCMI_APERTA_D_EXT_INTR_CTRL_P1_EISRr,
    BCMI_APERTA_D_EXT_INTR_CTRL_P2_EISRr, BCMI_APERTA_D_EXT_INTR_CTRL_P3_EISRr, BCMI_APERTA_D_EXT_INTR_CTRL_P4_EISRr,
    BCMI_APERTA_D_EXT_INTR_CTRL_P5_EISRr, BCMI_APERTA_D_EXT_INTR_CTRL_P6_EISRr, BCMI_APERTA_D_EXT_INTR_CTRL_P7_EISRr};

    PHYMOD_MEMSET(&common_eisr, 0, sizeof(BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_t));
    PHYMOD_MEMSET(&m0_eisr, 0, sizeof(BCMI_APERTA_D_EXT_INTR_CTRL_M0_EISRr_t));
    APERTA_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, port, lane_sel);
    (void) lane_sel;

    if (intr_type & APERTA_INTR_PORT) {
        if (intr_type & APERTA_INTR_PORT_PTP) {
            data |= (1 << 12);
        }
        if (intr_type & APERTA_INTR_PORT_LINK_DOWN) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                data |= (1 << 11);
            } else {
                data |= (1 << 10);
            }
        }
        if (intr_type & APERTA_INTR_PORT_SF_ERR_INTR) {
           data |= (1 << 5);
        }
        if (intr_type & APERTA_INTR_PORT_SF_DED_INTR) {
           data |= (1 << 4);
        }
        if (intr_type & APERTA_INTR_PORT_FC_ERR_INTR) {
            /* Egress & ingress*/
           data |= (1 << 3);
           data |= (1 << 9);
        }
        if (intr_type & APERTA_INTR_PORT_FC_DED_INTR) {
            /* Egress*/
           data |= (1 << 2);
            /* ingress*/
           data |= (1 << 8);
        }
        if (intr_type & APERTA_INTR_PORT_INTF_ERR_INTR) {
            /* Egress*/
           data |= (1 << 1);
            /* ingress*/
           data |= (1<< 7);
        }
        if (intr_type & APERTA_INTR_PORT_INTF_DED_INTR) {
            /* Egress*/
           data |= (1 << 0);
            /* ingress*/
           data |= (1 << 6);
        }
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, port_int_reg[port], data));
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, port_int_reg[port], &data));

    } else if (intr_type & APERTA_INTR_COMMON) {
        if (intr_type & APERTA_INTR_CMN_MOD_ABS_RISING) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_ABS_0_RISINGf_SET(common_eisr, 1);
            } else {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_ABS_1_RISINGf_SET(common_eisr, 1);
            }
        }
        if (intr_type & APERTA_INTR_CMN_MOD_ABS_FALL) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_ABS_0_FALLINGf_SET(common_eisr, 1);
            } else {                                                                                    
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_ABS_1_FALLINGf_SET(common_eisr, 1);
            }
        }
        if (intr_type & APERTA_INTR_CMN_MOD_EXT_RAISING) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_EXT_INTR_0_RISINGf_SET(common_eisr, 1);
            } else {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_EXT_INTR_1_RISINGf_SET(common_eisr, 1);
            }
        }
        if (intr_type & APERTA_INTR_CMN_MOD_EXT_FALL) {                                 
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
               BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_EXT_INTR_0_FALLINGf_SET(common_eisr, 1);
            } else {
               BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_MOD_EXT_INTR_1_FALLINGf_SET(common_eisr, 1);
            }
        }
        if (intr_type & APERTA_INTR_CMN_LMI_ERR) {
            BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_LMI_ERR_INTRf_SET(common_eisr, 1);
        }
        if (intr_type & APERTA_INTR_CMN_LMI_DED) {
            BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_LMI_DED_INTRf_SET(common_eisr, 1);
        }
        if (intr_type & APERTA_INTR_CMN_PMIF_ERR) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_SPMIF_ERR_INTRf_SET(common_eisr, 1);
            } else {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_LPMIF_ERR_INTRf_SET(common_eisr, 1);
            }
        }
        if (intr_type & APERTA_INTR_CMN_PMIF_DED) {
            if (APERTA_IS_SYSTEM_SIDE(phy)) {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_SPMIF_DED_INTRf_SET(common_eisr, 1);
            } else {
                BCMI_APERTA_D_EXT_INTR_CTRL_COMMON_CTRL_EISRr_LPMIF_DED_INTRf_SET(common_eisr, 1);
            }
        }
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_EXT_INTR_CTRL_COMMON_CTRL_EISRr(&phy->access, common_eisr));
    } else if (intr_type & APERTA_INTR_MO) {
        if (intr_type & APERTA_INTR_M0_MST_FW_WDOG_EXP) {
            BCMI_APERTA_D_EXT_INTR_CTRL_M0_EISRr_MST_FW_WDOG_EXPf_SET(m0_eisr, 1);
        }
        if (intr_type & APERTA_INTR_M0_MST_DRAM_DED) {
            BCMI_APERTA_D_EXT_INTR_CTRL_M0_EISRr_MST_DRAM_DEDf_SET(m0_eisr, 1);
        }
        if (intr_type & APERTA_INTR_M0_MST_DED) {
            BCMI_APERTA_D_EXT_INTR_CTRL_M0_EISRr_MST_DEDf_SET(m0_eisr, 1);
        }
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_EXT_INTR_CTRL_M0_EISRr(&phy->access, m0_eisr));
    }

    return PHYMOD_E_NONE;
}

#ifdef PHYMOD_TIMESYNC_SUPPORT /*---------------------------------------------------------------*/
    /*
     *   use the TimeSync functions defined in  phymod/common/ieee1588/timesync.c
     */

#else /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int _aperta_timesync_config_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_timesync_config_t* config)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_config_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_timesync_config_t* config)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_nco_addend_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t freq_step)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_nco_addend_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* freq_step)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_framesync_mode_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_timesync_framesync_t* framesync)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_framesync_mode_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_timesync_framesync_t* framesync)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_local_time_set(const plp_aperta_phymod_phy_access_t* phy, uint64_t local_time)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_local_time_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* local_time)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_load_ctrl_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t load_once, uint32_t load_always)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_load_ctrl_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* load_once, uint32_t* load_always)
{
    return PHYMOD_E_NONE;
}

/* User should provide appropriate ts_offset for Negative value*/
int _aperta_timesync_tx_timestamp_offset_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t ts_offset)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_tx_timestamp_offset_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* ts_offset)
{

    return PHYMOD_E_NONE;
}

/* User should provide appropriate ts_offset for Negative value*/
int _aperta_timesync_rx_timestamp_offset_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t ts_offset)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_rx_timestamp_offset_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* ts_offset)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_capture_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* cap_ts)
{
    return PHYMOD_E_NONE;
}

int _aperta_timesync_heartbeat_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* hb_ts)
{
    return PHYMOD_E_NONE;
}

#endif /* PHYMOD_TIMESYNC_SUPPORT */ /*---------------------------------------------------------*/

