#include <aperta2_cfg_seq.h>
#include <bcm_aperta2_id_defs.h>
#include <bcm_aperta2_direct_defs.h>
#include <phymod/phymod_acc.h>
#include <phymod/chip/aperta2.h>
#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_diagnostics_dispatch.h>
#include <tier1/aperta2_reg_access.h>
#include <tier1/aperta2_pm_seq.h>
#include <peregrine5_pc_diagnostics.h>
#include <peregrine5_pc_diag.h>

extern uint8_t plp_aperta2_ucode[];
extern uint32_t plp_aperta2_ucode_len;
extern aperta2_pm_info_t _plp_aperta2_pm_info[APERTA2_MAX_PM_INFO];

int plp_aperta2_get_chip_id (const plp_aperta2_phymod_access_t *pa, int *chip_id)
{
    BCM_APERTA2_DIRECT_CTRL_CHIP_IDr_t  lsb;
    BCM_APERTA2_DIRECT_CTRL_CHIP_REVISIONr_t msb;

    PHYMOD_MEMSET(&lsb, 0, sizeof(BCM_APERTA2_DIRECT_CTRL_CHIP_IDr_t));
    PHYMOD_MEMSET(&msb, 0, sizeof(BCM_APERTA2_DIRECT_CTRL_CHIP_REVISIONr_t));
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_CTRL_CHIP_REVISIONr(pa, &msb));
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_CTRL_CHIP_IDr(pa, &lsb));

    *chip_id = BCM_APERTA2_DIRECT_CTRL_CHIP_IDr_CHIP_ID_15_0f_GET(lsb) |
         (BCM_APERTA2_DIRECT_CTRL_CHIP_REVISIONr_CHIP_ID_19_16f_GET(msb) << 16);

    return PHYMOD_E_NONE;
}

int plp_aperta2_get_chip_rev (const plp_aperta2_phymod_access_t *pa, uint32_t *chip_rev)
{
    BCM_APERTA2_DIRECT_CTRL_CHIP_REVISIONr_t msb;

    PHYMOD_MEMSET(&msb, 0, sizeof(BCM_APERTA2_DIRECT_CTRL_CHIP_REVISIONr_t));
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_CTRL_CHIP_REVISIONr(pa, &msb));

    *chip_rev = BCM_APERTA2_DIRECT_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(msb) ;


    return PHYMOD_E_NONE;
}

int _plp_aperta2_core_reset_set(const plp_aperta2_phymod_core_access_t* core, plp_aperta2_phymod_reset_mode_t reset_mode, plp_aperta2_phymod_reset_direction_t direction)
{
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_t core_reset;
    
    /*Using top hard reset*/
    PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL1r(&core->access, &core_reset));
    if (reset_mode == phymodResetModeHard) {
        BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_RESETBf_SET(core_reset, 0);
    } else {
        BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET(core_reset,0);
    }
    PHYMOD_IF_ERR_RETURN(
          BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&core->access, core_reset));

    return PHYMOD_E_NONE;
}

int _plp_aperta2_core_firmware_info_get(const plp_aperta2_phymod_access_t *acc, plp_aperta2_phymod_core_firmware_info_t* fw_info)
{

    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(acc, BCM_APERTA2_DIRECT_GEN_CNTRLS_FW_INFO0r, &fw_info->fw_version));
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(acc, BCM_APERTA2_DIRECT_GEN_CNTRLS_DWNLD_01r, &fw_info->fw_crc));

    return PHYMOD_E_NONE;
}

/** Wait Msg Out from Master Micro
 * @param acc           Access structure for PHY 
 * @param exp_message   Expected Message
 * @param flag_error    If 1, flag error if received Msg is different than the expected.
 * @param poll_time     Time between register reads in miliseconds
 */
static int plp_aperta2_wait_mst_msgout(const plp_aperta2_phymod_access_t *acc, uint16_t exp_message, int flag_error, int poll_time)
{
    APERTA2_MSGOUT_T msgout;
    int retry_count = APERTA2_MICRO_RETRY_COUNT * 10;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGOUTr_t msg_out;

    do {
        /* read register*/
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_MST_MSGOUTr(acc, &msg_out));
        msgout = BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_GET(msg_out);
        if (msgout != 0) {
            if (flag_error && (msgout != exp_message)) {
                PHYMOD_DEBUG_ERROR(
                        ("ERR Recived msgout = (0x%x), exp_message = 0x%x addr:%d)\n", msgout, exp_message, acc->addr));
                return PHYMOD_E_INTERNAL;
            } else {
                /* Nothing to be Done*/
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

static int plp_aperta2_get_word_from_buffer(const uint8_t *buf, int index)
{
     uint16_t word = buf[(index*2)+1];
     word <<= 8;
     return (word | buf[index*2]);

}

/**   Download and Fuse firmware
 *    This function is used to download the firmware through I2C/MDIO
 *    and fuse it to SPI EEPROM if prg_eeprom flag is set
 *
 *    @param core_access        Pointer to core access structure
 *    @param plp_aperta2_ucode      Pointer to firmware array
 *    @param fw_length          Length of the firmware array
 *    @param master_en          reserved, used for future
 *    @param mst_boot_addr      master boot address
 *    @param prg_eeprom         enable/disable programming eeprom
 *    @param init_config        initialization configuration.
 *    @return PHYMOD_E_NONE on success downloaded
 */
int
_plp_aperta2_download_prog_eeprom(const plp_aperta2_phymod_core_access_t *core_access,
        uint8_t *plp_aperta2_ucode, uint32_t fw_length, uint16_t master_en,
        uint16_t mst_boot_addr, uint8_t prg_eeprom, const plp_aperta2_phymod_core_init_config_t* init_config)

{
#ifndef VIRTUAL_SIM_VAL
    int i, size0, size1 = 0, size2 = 0, size3 = 0;
    uint16_t next_param = 0x1000;
    int retry_cnt = APERTA2_MICRO_RETRY_COUNT, data1 = 0;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGINr_t msg_in;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_SPI_CODE_LOAD_ENr_t spi_code_load_en;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL3r_t gen_ctrl3;
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_t boot_por;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_t gen_ctrl2;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_t start_ptr;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGOUTr_t msg_out;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_02r_t gpreg2;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_t boot;
    BCM_APERTA2_DIRECT_CTRL_MISC_CONTROL0r_t misc_ctrl;
    bcm_plp_ext_fw_params_t ext_fw_params;
    unsigned int trans_size = 0, dload_idx = 0;
    plp_aperta2_phymod_phy_access_t temp_access;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t  bcast_enable;
    const plp_aperta2_phymod_access_t *pa = &core_access->access;
    
    PHYMOD_MEMCPY(&temp_access, core_access, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMSET(&ext_fw_params,    0, sizeof(bcm_plp_ext_fw_params_t));
    PHYMOD_MEMSET(&bcast_enable,     0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t));
    PHYMOD_MEMSET(&boot_por,         0, sizeof(BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_t));
    PHYMOD_MEMSET(&msg_in,           0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGINr_t));
    PHYMOD_MEMSET(&msg_out,          0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGOUTr_t));
    PHYMOD_MEMSET(&gen_ctrl2,        0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_t));
    PHYMOD_MEMSET(&spi_code_load_en, 0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_SPI_CODE_LOAD_ENr_t));
    PHYMOD_MEMSET(&gen_ctrl3,        0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL3r_t));
    PHYMOD_MEMSET(&start_ptr,        0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_t));
    PHYMOD_MEMSET(&gpreg2,           0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_02r_t));
    PHYMOD_MEMSET(&boot,             0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_t));
    
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

#if 0 
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_BOOTr(pa, &boot));
    PHYMOD_IF_ERR_RETURN(
         BCM_APERTA2_DIRECT_READ_MICRO_BOOT_BOOT_PORr(pa, &boot_por));
    if (BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_SERBOOT_DONE_ONCEf_GET(boot) &&
        (BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_GET(boot_por) == 3)) {
        PHYMOD_IF_ERR_RETURN(
                aperta2_bhawk_micro_reset(core_access));
    }
#endif
    /*Put master in to reset*/
    PHYMOD_IF_ERR_RETURN(
         BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_SPI2X_RSTBf_SET(gen_ctrl2, 1);
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_SPI_RSTBf_SET(gen_ctrl2, 1);
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 0);
    PHYMOD_IF_ERR_RETURN(
         BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));
    
    PHYMOD_IF_ERR_RETURN(
         BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 0);
    PHYMOD_IF_ERR_RETURN(
         BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));
    
    /* Wait for serboot busy to clear*/
    do {
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_BOOTr(pa, &boot));
        data1 = BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        PHYMOD_USLEEP(3000);
    } while((data1 != 0) && (--retry_cnt));
    retry_cnt = APERTA2_MICRO_RETRY_COUNT;

    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_CTRL_MISC_CONTROL0r(pa, &misc_ctrl));
    BCM_APERTA2_DIRECT_CTRL_MISC_CONTROL0r_EXT_UC_RSTB_IN_FRCf_SET(misc_ctrl, 1);
    BCM_APERTA2_DIRECT_CTRL_MISC_CONTROL0r_EXT_UC_RSTB_IN_FRCVALf_SET(misc_ctrl, 0);
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_WRITE_CTRL_MISC_CONTROL0r(pa, misc_ctrl));

    /* STEP 1: Program master enable, slave enable, broadcast enable bits */
    PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, &spi_code_load_en));
    BCM_APERTA2_DIRECT_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_SET(spi_code_load_en, 1);
    BCM_APERTA2_DIRECT_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_SET(spi_code_load_en, 0xF);
    PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, spi_code_load_en));
    PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, &spi_code_load_en));
    if ((BCM_APERTA2_DIRECT_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_GET(spi_code_load_en)!= 0xF)||
        !(BCM_APERTA2_DIRECT_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_GET(spi_code_load_en))) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_FAIL,
                (_PHYMOD_MSG("ERR: Download ENABLE IS NOT SET")));
    }

    if (prg_eeprom) {
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, &spi_code_load_en));
        BCM_APERTA2_DIRECT_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_MST_OEBf_SET(spi_code_load_en, 1);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, spi_code_load_en));

        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL3r(pa, &gen_ctrl3));
        BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL3r_UCSPI_SLOWf_SET(gen_ctrl3, 1);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL3r(pa, gen_ctrl3));
    }
    /* Set Download done as '0'*/
    PHYMOD_IF_ERR_RETURN(
         BCM_APERTA2_DIRECT_READ_MICRO_BOOT_BOOT_PORr(pa, &boot_por));
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_SET(boot_por, 0);
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_SET(boot_por, 0);
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_SLV_RST_ENf_SET(boot_por, 0xF);
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(boot_por, 1);
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_LARGE_MEMf_SET(boot_por, 1);
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_SET(boot_por, 0);
    PHYMOD_IF_ERR_RETURN(
         BCM_APERTA2_DIRECT_WRITE_MICRO_BOOT_BOOT_PORr(pa, boot_por));
    
    /* Dummy message out read for PHY-3767*/
    PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_MST_MSGOUTr(pa, &msg_out));

    /*Take master out of reset, updated for PHY-3767*/
    PHYMOD_IF_ERR_RETURN(
         BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 1);
    PHYMOD_IF_ERR_RETURN(
         BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    PHYMOD_IF_ERR_RETURN(
         BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 1);
    PHYMOD_IF_ERR_RETURN(
         BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_CTRL_MISC_CONTROL0r(pa, &misc_ctrl));
    BCM_APERTA2_DIRECT_CTRL_MISC_CONTROL0r_EXT_UC_RSTB_IN_FRCf_SET(misc_ctrl, 0);
    BCM_APERTA2_DIRECT_CTRL_MISC_CONTROL0r_EXT_UC_RSTB_IN_FRCVALf_SET(misc_ctrl, 0);
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_WRITE_CTRL_MISC_CONTROL0r(pa, misc_ctrl));
    do {
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_BOOTr(pa, &boot));
        data1 = BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        PHYMOD_USLEEP(3000);
    } while((data1 != 1) && (--retry_cnt));
    if (retry_cnt == 0) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_FAIL,
                               (_PHYMOD_MSG("ERR:SERBOOT BUSY BIT NOT SET")));
    }
    retry_cnt = APERTA2_MICRO_RETRY_COUNT;
#ifdef ATE_PRINT_ENABLED
	PHYMOD_CRIT_INFO(("ATE_GUIDELINES : FW Download preparation Completed...\n"));
#else
    PHYMOD_CRIT_INFO(("FW Download preparation Completed...\n"));
#endif
    BCM_APERTA2_DIRECT_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_SET(start_ptr, mst_boot_addr);
    PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_SPI_MST_CODE_START_PTRr(pa, start_ptr));

#ifdef SIM_VAL
    PHYMOD_USLEEP(300);
#else
    PHYMOD_USLEEP(10000);
#endif

    /*EEPROM*/
    if (prg_eeprom) {
        PHYMOD_CRIT_INFO(("Enabling EEPROM\n"));

#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_FLASH, 1, 2000));
#else

        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_FLASH, 1, 0));
#endif
        /* Send Erase and Flash enabled*/
        BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 3);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_FLASH, 1, 2000));
#else
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_FLASH, 1, 0));
#endif
        /* Send write delay*/
        BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 0);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_FLASH, 1, 2000));
#else
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_FLASH, 1, 0));
#endif
        /* Send debug mode*/
        BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 0);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
    } else {
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_PLL_LOCK, 1, 2000));

        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_FLASH, 1, 2000));
#else
        /* Wait for Register Status, instead of message out*/
        /* Waiting for PLL lock
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_PLL_LOCK, 1, 0));*/
        /* Checking for flash enable*/
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_FLASH, 1, 0));
#endif
        BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 0);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in)); /* Flashing Disable*/
    }
#ifdef SIM_VAL
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_HEADER, 1, 2000));
#else

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_HEADER, 1, 0));
#endif
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGINr_SET(msg_in, mst_boot_addr);
    PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));

    if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
        size0 = plp_aperta2_get_word_from_buffer(ext_fw_params.firmware_address, 13);
        size0 |= (plp_aperta2_get_word_from_buffer(ext_fw_params.firmware_address, 14) & 0xF) << 16;
        size1 = plp_aperta2_get_word_from_buffer(ext_fw_params.firmware_address, 19);
        size1 |= (plp_aperta2_get_word_from_buffer(ext_fw_params.firmware_address, 20) & 0xF) << 16;
        size2 = plp_aperta2_get_word_from_buffer(ext_fw_params.firmware_address, 25);
        size2 |= (plp_aperta2_get_word_from_buffer(ext_fw_params.firmware_address, 26) & 0xF) << 16;
        size3 = plp_aperta2_get_word_from_buffer(ext_fw_params.firmware_address, 31);
        size3 |= (plp_aperta2_get_word_from_buffer(ext_fw_params.firmware_address, 32) & 0xF) << 16;

    } else {
        size0 = plp_aperta2_get_word_from_buffer(plp_aperta2_ucode, 13);
        size0 |= (plp_aperta2_get_word_from_buffer(plp_aperta2_ucode, 14) & 0xF) << 16;
        size1 = plp_aperta2_get_word_from_buffer(plp_aperta2_ucode, 19);
        size1 |= (plp_aperta2_get_word_from_buffer(plp_aperta2_ucode, 20) & 0xF) << 16;
        size2 = plp_aperta2_get_word_from_buffer(plp_aperta2_ucode, 25);
        size2 |= (plp_aperta2_get_word_from_buffer(plp_aperta2_ucode, 26) & 0xF) << 16;
        size3 = plp_aperta2_get_word_from_buffer(plp_aperta2_ucode, 31);
        size3 |= (plp_aperta2_get_word_from_buffer(plp_aperta2_ucode, 32) & 0xF) << 16;

    }

    /*adjusted to be multiple of 64B*/
    size0 = (size0 % APERTA2_HEADER_SIZE) ?
            ((size0 / APERTA2_HEADER_SIZE) + 1) * APERTA2_HEADER_SIZE : size0;
    size1 = (size1 % APERTA2_HEADER_SIZE) ?
            ((size1 / APERTA2_HEADER_SIZE) + 1) * APERTA2_HEADER_SIZE : size1;
    size2 = (size2 % APERTA2_HEADER_SIZE) ?
            ((size2 / APERTA2_HEADER_SIZE) + 1) * APERTA2_HEADER_SIZE : size2;
    size3 = (size3 % APERTA2_HEADER_SIZE) ?
            ((size3 / APERTA2_HEADER_SIZE) + 1) * APERTA2_HEADER_SIZE : size3;

    PHYMOD_CRIT_INFO(("FW Download Started...\n"));
    for (i = 0; i < (fw_length / 2); i++) {
        if ((i%32) == 0) {
            if ((i == 0) || (i == 32)) {
                next_param = APERTA2_MSGOUT_HEADER;
            } else if (i == 64) {
                PHYMOD_CRIT_INFO(("FW Header Download Completed...\n"));
                next_param = 0x1000;
            } else if (i*2 == (128 + size0)) {
                next_param = 0x2000;
            } else if (i*2 == (128 + size0 + size1)) {
                next_param = 0x3000;
            } else if (i*2 == (128 + size0 + size1 + size2)) {
                next_param = 0x4000;
            } else if (i*2 == (128 + size0 + size1 + size2 + size3)) {
                next_param = 0x5000;
            }else {
                next_param++;
            }
        }
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, next_param, 1, 2000));
#else
    if (prg_eeprom) {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, next_param, 1, ((i%32) == 0)?2:0 ));
    } else {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_wait_mst_msgout(pa, next_param, 1, 0));
    }
#endif
        if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
            if (trans_size == 0) {
                PHYMOD_IF_ERR_RETURN(
                    init_config->firmware_loader(core_access->access.user_acc, &ext_fw_params));
                trans_size = ext_fw_params.transfer_size;
                dload_idx = 0;
            }
            BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGINr_SET(msg_in,
                    plp_aperta2_get_word_from_buffer(ext_fw_params.firmware_address, dload_idx));
            dload_idx ++;
        } else {
            BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGINr_SET(msg_in,
                    plp_aperta2_get_word_from_buffer(plp_aperta2_ucode, i));
        }
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
        if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
            trans_size -= 2;
        }
    }
#ifdef SIM_VAL
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_DWNLD_DONE, 1, 2000));
#else
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_wait_mst_msgout(pa, APERTA2_MSGOUT_DWNLD_DONE, 1, 0));
#endif

    PHYMOD_CRIT_INFO(("FW MST/SLV Download Completed...\n"));

    if (prg_eeprom == 1 && ((bcast_enable.v[0] & 1) == 1)) {
        PHYMOD_IF_ERR_RETURN( _plp_aperta2_core_reset_set(core_access, phymodResetModeHard, 0));
        /* Giving 100milli sec time for FW dload*/
        PHYMOD_USLEEP(100000);
    }
#endif
    return PHYMOD_E_NONE;
}
int _plp_aperta2_check_fw_download_status(const plp_aperta2_phymod_core_access_t *core_access,
        plp_aperta2_phymod_firmware_load_method_t load_method) {
#ifndef VIRTUAL_SIM_VAL
    uint16_t n_img = 0, no_of_img = 0, retry_cnt = 100;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_FW_INFO0r_t fw_ver;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_01r_t gpreg_1;
    uint32_t data = 0;
    uint32_t crc = 0;
    uint32_t aperta2_fw_crc[2] = {0,0};

    PHYMOD_MEMSET(&fw_ver, 0,  sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_FW_INFO0r_t));
    PHYMOD_MEMSET(&gpreg_1, 0,  sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_01r_t));

    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&core_access->access, BCM_APERTA2_DIRECT_GEN_CNTRLS_DWNLD_11r,
                &aperta2_fw_crc[0]));
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&core_access->access, BCM_APERTA2_DIRECT_GEN_CNTRLS_DWNLD_13r,
                &aperta2_fw_crc[1]));

    n_img = 2; /* Get Number of image*/
    for (no_of_img = 0; no_of_img < n_img; ++no_of_img) {
        PHYMOD_IF_ERR_RETURN(
                PHYMOD_BUS_READ(&core_access->access, (BCM_APERTA2_DIRECT_GEN_CNTRLS_DWNLD_00r + (no_of_img*4)), &data));
        if (data != 0x600D) {
            PHYMOD_IF_ERR_RETURN(
                    PHYMOD_BUS_READ(&core_access->access, (BCM_APERTA2_DIRECT_GEN_CNTRLS_DWNLD_00r + (no_of_img*4) + 1), &data));
            crc = aperta2_fw_crc[no_of_img]; /*Get CRC*/
            if (crc != data) {
                PHYMOD_DEBUG_ERROR(("ERROR: Image Dload status not correct for image:%d Got:0x%x expected:0x%x\n", 
                            no_of_img, data, aperta2_fw_crc[no_of_img] ));
                return PHYMOD_E_INTERNAL;
            }
        }
    }
    do {
        PHYMOD_USLEEP(5000);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GPREG_01r(&core_access->access, &gpreg_1));
        /* Check for uc active*/
        if (BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1)& 1) {
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_FW_INFO0r(&core_access->access, &fw_ver));
            PHYMOD_CRIT_INFO(
                    ("FW download success. FW ver:0x%x.\n",
                      BCM_APERTA2_DIRECT_GEN_CNTRLS_FW_INFO0r_CHIP_FW_VERSIONf_GET(fw_ver)));
            return PHYMOD_E_NONE;
        }
    } while (--retry_cnt);
    PHYMOD_DEBUG_ERROR(
            ("ERROR: FW download Failure. Gpreg1:%x\n",
              BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1)));

    return PHYMOD_E_INTERNAL;
#else
    return PHYMOD_E_NONE;
#endif
}

int plp_aperta2_dload_fw(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config)
{
    int rv = 0;
    int mst_boot_addr = 0, master_en=1;

    if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
        rv = _plp_aperta2_download_prog_eeprom(core, NULL, 0, master_en, mst_boot_addr,
            0,
            init_config);
    } else {
        rv = _plp_aperta2_download_prog_eeprom(core, plp_aperta2_ucode, plp_aperta2_ucode_len, master_en, mst_boot_addr,
            (init_config->firmware_load_method == phymodFirmwareLoadMethodProgEEPROM) ? 1 : 0, init_config);
    }
    if ((rv != PHYMOD_E_NONE)) {
        PHYMOD_DEBUG_ERROR(("FW download failed : %d\n", rv));
        return PHYMOD_E_FAIL;
    }

    return PHYMOD_E_NONE;
}


plp_aperta2_phymod_interface_t
_plp_aperta2_convert_numeric_value_to_interface_type(uint32_t numeric_val)
{
    const plp_aperta2_phymod_interface_t aperta2_if_type_list[] = APERTA2_IF_TYPE_LIST_ELEMENTS ;
    if (numeric_val < (sizeof(aperta2_if_type_list)/sizeof(aperta2_if_type_list[0]))) {
        return (aperta2_if_type_list[numeric_val]) ;
    } else {
        return phymodInterfaceBypass;
    }
}

uint32_t _plp_aperta2_convert_interface_type_to_numeric_value(plp_aperta2_phymod_interface_t if_type)
{
    const plp_aperta2_phymod_interface_t aperta2_if_type_list[] = APERTA2_IF_TYPE_LIST_ELEMENTS ;
    uint8_t i = 0 ;
    for(i=0; i < (sizeof(aperta2_if_type_list)/sizeof(aperta2_if_type_list[0])); i++) {
       if(aperta2_if_type_list[i] == if_type){
           return(i) ;
       }
    }
    return(0);
}


int plp_aperta2_sw_intf_set(const plp_aperta2_phymod_phy_access_t *phy, plp_aperta2_phymod_interface_t if_type) {
    uint32_t if_type_numeric_val = 0, if_type_numeric_val_bit_4 = 0 ;
    uint32_t if_type_rd = 0;
    uint32_t if_type_wr = 0;
    uint8_t lane_index  = 0;
    uint8_t sys_side    = 0;

    if_type_numeric_val = _plp_aperta2_convert_interface_type_to_numeric_value(if_type);
    if_type_numeric_val_bit_4 = (if_type_numeric_val & 0x10) >> 4;
    if_type_numeric_val &= 0xF;

    if (APERTA2_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_direct_reg_read(phy, (APERTA2_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
                            (sys_side * APERTA2_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) +
                            (lane_index/4)),
                        &if_type_rd));
            /* Clear the bits based on lane index*/
            if_type_wr  = (if_type_rd & ~(APERTA2_IF_TYPE_PER_LANE_STORAGE_MASK << ((lane_index & 0x3) << 0x2)));
            if_type_wr |= ((if_type_numeric_val & APERTA2_IF_TYPE_PER_LANE_STORAGE_MASK) << ((lane_index & 0x3) << 0x2)) ;
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_direct_reg_write(phy, (APERTA2_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
                            (sys_side * APERTA2_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) +
                            (lane_index/4)),
                        if_type_wr));
            break ;
        }
    }
    /* Using one more bit to support more interface type*/ 
    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                 plp_aperta2_direct_reg_read(phy, (APERTA2_IF_TYPE_SAVE_SWGPREG_LINE_BIT4+sys_side), &if_type_rd));
            /* Clearing corresonding bits */
            if_type_wr  = if_type_rd & ~(1 << (lane_index));
            if_type_wr |= (if_type_numeric_val_bit_4 & 1) << (lane_index);
            PHYMOD_IF_ERR_RETURN(
                 plp_aperta2_direct_reg_write(phy, (APERTA2_IF_TYPE_SAVE_SWGPREG_LINE_BIT4+sys_side), if_type_wr));
            break ;
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_sw_intf_get(const plp_aperta2_phymod_phy_access_t *phy, uint8_t lane_index, plp_aperta2_phymod_interface_t *if_type) {
    uint32_t if_type_rd = 0;
    uint8_t sys_side    = 0;
    uint32_t if_type_numeric_val_bit_4 = 0;
    if (APERTA2_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, (APERTA2_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
                    (sys_side * APERTA2_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) +
                    (lane_index/4)),
                &if_type_rd));

    /* Using one more bit to support more interface type*/ 
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, (APERTA2_IF_TYPE_SAVE_SWGPREG_LINE_BIT4 + sys_side), &if_type_numeric_val_bit_4));
    if_type_numeric_val_bit_4 = (if_type_numeric_val_bit_4 >> (lane_index)) & 1;
    if_type_rd = (if_type_rd >> ((lane_index & 0x3)<<2)) & APERTA2_IF_TYPE_PER_LANE_STORAGE_MASK;
    if_type_rd |= (if_type_numeric_val_bit_4 << 4); 


    *if_type = _plp_aperta2_convert_numeric_value_to_interface_type(if_type_rd) ;
    return PHYMOD_E_NONE;
}

/**   PHY Status dump
 *    This function is used to dump PMD status based on the flag.
 *
 *    @param phy                pointer to phy access structure
 *    @return PHYMOD_E_NONE on success downloaded
 */
int _plp_aperta2_phy_status_dump(const plp_aperta2_phymod_phy_access_t* phy)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    int lane_index = 0;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    PHYMOD_DIAG_OUT((" ***************************************\n"));
    PHYMOD_DIAG_OUT((" ******* PHY status dump for Aperta2 PHY ID:0x%x ********\n", phy->access.addr));
    PHYMOD_DIAG_OUT((" ***************************************\n"));
    PHYMOD_DIAG_OUT((" ***************************************\n"));
    PHYMOD_DIAG_OUT((" ******* PHY status dump for side:%x ********\n", phy->port_loc));
    PHYMOD_DIAG_OUT((" ***************************************\n"));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_display_core_state_legend(&phy_copy));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_display_lane_state_legend(&phy_copy));

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index ++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            phy_copy.access.lane_mask = (1 << lane_index);
            if (phy->access.flags & BCM_PLP_INTERNAL_DUMP_L1) {
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_display_diag_data(&phy_copy, APERTA2_SRDS_DUMP_L1));
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_display_lane_config(&phy_copy));
            } else if (phy->access.flags & BCM_PLP_INTERNAL_DUMP_L2) {
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_display_diag_data(&phy_copy, APERTA2_SRDS_DUMP_L2));
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_display_lane_config(&phy_copy));
            } else if (phy->access.flags & BCM_PLP_INTERNAL_DUMP_L3) {
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_display_diag_data(&phy_copy, APERTA2_SRDS_DUMP_L3));
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_display_lane_config(&phy_copy));
            } else {
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_display_diag_data(&phy_copy, APERTA2_SRDS_MIN_DUMP));
            }
        }
    }

    return PHYMOD_E_NONE;
}

/**   Initialize SW database
 *    This function is used to initialize SW DB with default speed.
 *
 *    @param core                pointer to core access structure
 *    init_config                pointer to core init configuration
 *    @return PHYMOD_E_NONE on success downloaded
 */
int plp_aperta2_init_db(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config) 
{
    int index = 0;
    int port_index = 0;

    if (!_plp_aperta2_pm_info[0].is_db_initialized) {
        /*Initialize SW DB*/
        for (index = 0; index < APERTA2_MAX_PM_INFO; index++) {
            _plp_aperta2_pm_info[index].pm_info = NULL ;
            _plp_aperta2_pm_info[index].phy_id = APERTA2_UNINIT_PHYS;
            _plp_aperta2_pm_info[index].is_fw_dloaded = 0;
            _plp_aperta2_pm_info[index].is_db_initialized = 1;
            _plp_aperta2_pm_info[index].init_state = 0;

            /* Default port Speed*/
            for (port_index = 0; port_index <16; port_index++) {
                _plp_aperta2_pm_info[index].port_info[port_index].line_speed = APERTA2_LINE_INIT_SPEED; 
                _plp_aperta2_pm_info[index].port_info[port_index].line_lane_map = 1 << port_index;
                _plp_aperta2_pm_info[index].port_info[port_index].sys_speed = APERTA2_LINE_INIT_SPEED; 
                _plp_aperta2_pm_info[index].port_info[port_index].sys_lane_map = 1 << port_index;
            }
        }
    }
    return PHYMOD_E_NONE;
}

/**   Workaround for M0 DRAM DED Error
 *    This function provides a work around for DED error that is seen 
 *    after power cycle. This has to be called after every soft reset.
 *
 *    @param core                pointer to core access structure
 *    @return PHYMOD_E_NONE
 */
int plp_aperta2_ded_wka(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config) 
{
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_t     micro_boot;
    BCM_APERTA2_DIRECT_PAD_CNTRL_SERBOOT_STATUS_0r_t  serboot_status;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_t         boot;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_t gen_ctrl1;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_t gen_ctrl2;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MSCRr_t         mscr;
    const plp_aperta2_phymod_access_t *pa = &core->access;

    int do_workaround = 1;
    int init_serboot = 0;

    PHYMOD_MEMSET(&micro_boot, 0,sizeof(BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_t)) ;
    PHYMOD_MEMSET(&serboot_status, 0,sizeof(BCM_APERTA2_DIRECT_PAD_CNTRL_SERBOOT_STATUS_0r_t)) ;
    PHYMOD_MEMSET(&boot,       0,sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_t))         ;
    PHYMOD_MEMSET(&gen_ctrl1,  0,sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_t)) ;
    PHYMOD_MEMSET(&gen_ctrl2,  0,sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_t)) ;
    PHYMOD_MEMSET(&mscr,       0,sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_MSCRr_t))         ;

    /*== Step 1. Check if serboot is set and DED is set*/
    PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_PAD_CNTRL_SERBOOT_STATUS_0r(pa, &serboot_status));
    init_serboot = BCM_APERTA2_DIRECT_PAD_CNTRL_SERBOOT_STATUS_0r_SERBOOT_DIN_RAW_PREf_GET(serboot_status);

    if (do_workaround == 1) {
        if (init_serboot == 1) {
            PHYMOD_IF_ERR_RETURN (
                    BCM_APERTA2_DIRECT_READ_MICRO_BOOT_BOOT_PORr(pa, &micro_boot));
            BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(micro_boot, 0);
            PHYMOD_IF_ERR_RETURN (
                    BCM_APERTA2_DIRECT_WRITE_MICRO_BOOT_BOOT_PORr(pa, micro_boot));

            /* Assert Soft reset to clear serboot busy*/
            PHYMOD_IF_ERR_RETURN(
                    BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL1r(pa, &gen_ctrl1));
            BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET(gen_ctrl1, 0);
            PHYMOD_IF_ERR_RETURN(
                    BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL1r(pa, gen_ctrl1));
            if (init_config->firmware_load_method != phymodFirmwareLoadMethodProgEEPROM) { 
                /* Set Serboot to 1*/
                BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(micro_boot, 1);
                PHYMOD_IF_ERR_RETURN (
                        BCM_APERTA2_DIRECT_WRITE_MICRO_BOOT_BOOT_PORr(pa, micro_boot));
            }
        }

        /* Set hresp_mask to 0*/
        PHYMOD_IF_ERR_RETURN (
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_MSCRr(pa, &mscr));
        BCM_APERTA2_DIRECT_GEN_CNTRLS_MSCRr_HRESP_MASKf_SET(mscr, 0);
        PHYMOD_IF_ERR_RETURN (
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_MSCRr(pa, mscr));

        /* Micro Resets Toggle*/
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
        BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 0);
        BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 0);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

        BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 1);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

        BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 1);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    }
    return PHYMOD_E_NONE;
}

/**   Interrupt Enable Set
 *    This function used to enable interrupt of user choice 
 *
 *    @param phy                Phy access structure
 *    @param port               port number
 *    @param intr_type_enable   interrupt type and enable value
 *
 *    @return PHYMOD_E_NONE
 */
int _plp_aperta2_intr_enable_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t port, uint32_t intr_type_enable)
{
    unsigned int port_int_reg = BCM_APERTA2_DIRECT_EXT_INTR_CTRL_OCT0_P0_EIERr;
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EIERr_t      com_dp;  /* 8b8f*/
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_t   com_ctrl0; /*8b8c*/
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_t   com_ctrl1;  /*8b89*/
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIERr_t             m0_en; /* 8b83*/
    unsigned int data = 0;
    int enable = 0;
    int octal = APERTA2_GET_OCTAL(phy->access.lane_mask);

    PHYMOD_MEMSET(&com_dp, 0, sizeof(com_dp));
    PHYMOD_MEMSET(&com_ctrl0, 0, sizeof(com_ctrl0)); 
    PHYMOD_MEMSET(&com_ctrl1, 0, sizeof(com_ctrl1));
    PHYMOD_MEMSET(&m0_en, 0, sizeof(m0_en));

    enable = (intr_type_enable >> 31) & 1;
    if (intr_type_enable & APERTA2_INTR_PORT) {
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, (port_int_reg + (port*3)), &data));
        if (intr_type_enable & APERTA2_INTR_PORT_PTP) {
            data &= ~(3 << 12);
            data |= ((enable ? 3 : 0) << 12);
        }
        if (intr_type_enable & APERTA2_INTR_PORT_LINK_DOWN) {
            if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                data &= ~(1 << 11);
                data |= (enable << 11);
            } else {
                data &= ~(1 << 10);
                data |= (enable << 10);
            }
        }
        if (intr_type_enable & APERTA2_INTR_PORT_SF_ERR_INTR) {
           data &= ~(1 << 5);
           data |= (enable << 5);
        }
        if (intr_type_enable & APERTA2_INTR_PORT_SF_DED_INTR) {
           data &= ~(1 << 4);
           data |= (enable << 4);
        }
        if (intr_type_enable & APERTA2_INTR_PORT_FC_ERR_INTR) {
            /* Egress & ingress*/
           data &= ~(1 << 3);
           data |= (enable << 3);
           data &= ~(1 << 9);
           data |= (enable << 9);
        }
        if (intr_type_enable & APERTA2_INTR_PORT_FC_DED_INTR) {
            /* Egress*/
           data &= ~(1 << 2);
           data |= (enable << 2);
            /* ingress*/
           data &= ~(1 << 8);
           data |= (enable << 8);
        }
        if (intr_type_enable & APERTA2_INTR_PORT_INTF_ERR_INTR) {
            /* Egress*/
           data &= ~(1 << 1);
           data |= (enable << 1);
            /* ingress*/
           data &= ~(1 << 7);
           data |= (enable << 7);
        }
        if (intr_type_enable & APERTA2_INTR_PORT_INTF_DED_INTR) {
            /* Egress*/
           data &= ~(1 << 0);
           data |= (enable << 0);
            /* ingress*/
           data &= ~(1 << 6);
           data |= (enable << 6);
        }
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, (port_int_reg + (port*3)), data));
    } 
    if (intr_type_enable & APERTA2_INTR_COMMON) {
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_DP_EIERr(&phy->access, &com_dp));
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_CTRL0_EIERr(&phy->access, &com_ctrl0));
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_CTRL1_EIERr(&phy->access, &com_ctrl1));

        if (intr_type_enable & APERTA2_INTR_CMN_MOD_ABS_RISING) {
            if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_0_OCT0_RISINGf_SET(com_ctrl1, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_0_OCT1_RISINGf_SET(com_ctrl1, enable);
                }
            } else {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_1_OCT0_RISINGf_SET(com_ctrl1, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_1_OCT1_RISINGf_SET(com_ctrl1, enable);
                }
            }
        }
        if (intr_type_enable & APERTA2_INTR_CMN_MOD_ABS_FALL) {
            if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_0_OCT0_FALLINGf_SET(com_ctrl1, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_0_OCT1_FALLINGf_SET(com_ctrl1, enable);
                }
            } else {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_1_OCT0_FALLINGf_SET(com_ctrl1, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_1_OCT1_FALLINGf_SET(com_ctrl1, enable);
                }
            }
        }
        if (intr_type_enable & APERTA2_INTR_CMN_MOD_EXT_FALL) {
            if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_0_OCT0_FALLINGf_SET(com_ctrl1, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_0_OCT1_FALLINGf_SET(com_ctrl1, enable);
                }
            } else {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_1_OCT0_FALLINGf_SET(com_ctrl1, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_1_OCT1_FALLINGf_SET(com_ctrl1, enable);
                }
            }
        }
        if (intr_type_enable & APERTA2_INTR_CMN_MOD_EXT_RAISING ) {                                 
            if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_0_OCT0_RISINGf_SET(com_ctrl1, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_0_OCT1_RISINGf_SET(com_ctrl1, enable);
                }
            } else {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_1_OCT0_RISINGf_SET(com_ctrl1, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_1_OCT1_RISINGf_SET(com_ctrl1, enable);
                }
            }

        }
        if (intr_type_enable & APERTA2_INTR_CMN_LMI_ERR) {
            BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LMI_ERR_INTRf_SET(com_ctrl0, enable);
        }
        if (intr_type_enable & APERTA2_INTR_CMN_LMI_DED) {
            BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LMI_DED_INTRf_SET(com_ctrl0, enable);
        }
        if (intr_type_enable & APERTA2_INTR_CMN_PMIF_ERR) {
            if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_SPMIF_OCT0_ERR_INTRf_SET(com_ctrl0, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_SPMIF_OCT1_ERR_INTRf_SET(com_ctrl0, enable);
                }
            } else {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LPMIF_OCT0_ERR_INTRf_SET(com_ctrl0, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LPMIF_OCT1_ERR_INTRf_SET(com_ctrl0, enable);
                }
            }
        }
        if (intr_type_enable & APERTA2_INTR_CMN_PMIF_DED) {
            if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_SPMIF_OCT0_DED_INTRf_SET(com_ctrl0, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_SPMIF_OCT1_DED_INTRf_SET(com_ctrl0, enable);
                }
            } else {
                if (octal == APERTA2_PM_OCTAL1) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LPMIF_OCT0_DED_INTRf_SET(com_ctrl0, enable);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LPMIF_OCT1_DED_INTRf_SET(com_ctrl0, enable);
                }
            }
        }
        if (intr_type_enable & APERTA2_INTR_CMN_DP_CDC_FIFO_DED_INTR) {
            if (octal == APERTA2_PM_OCTAL1) {
                BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EIERr_ENABLE_EGR_DP_CDC_FIFO_DED_INTR_OCT0f_SET(com_dp, enable);
                BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EIERr_ENABLE_ING_DP_CDC_FIFO_DED_INTR_OCT0f_SET(com_dp, enable);
            } else {
               BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EIERr_ENABLE_EGR_DP_CDC_FIFO_DED_INTR_OCT1f_SET(com_dp, enable);
               BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EIERr_ENABLE_ING_DP_CDC_FIFO_DED_INTR_OCT1f_SET(com_dp, enable);
            }
        }
        if (intr_type_enable & APERTA2_INTR_CMN_PTP_PPS_RAISING) {
            BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_PPS_INTR_RISINGf_SET(com_ctrl0, enable);
        }
        if (intr_type_enable & APERTA2_INTR_CMN_PTP_PPS_FALLING)  {
            BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_PPS_INTR_FALLINGf_SET(com_ctrl0, enable);
        }
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_WRITE_EXT_INTR_CTRL_COMMON_DP_EIERr(&phy->access, com_dp));
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_WRITE_EXT_INTR_CTRL_COMMON_CTRL0_EIERr(&phy->access, com_ctrl0));
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_WRITE_EXT_INTR_CTRL_COMMON_CTRL1_EIERr(&phy->access, com_ctrl1));

    } 
    if (intr_type_enable & APERTA2_INTR_MO) {
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_M0_EIERr(&phy->access, &m0_en));
        if (intr_type_enable & APERTA2_INTR_M0_MST_FW_WDOG_EXP) {
            BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_FW_WDOG_EXPf_SET(m0_en, enable);
        }
        if (intr_type_enable & APERTA2_INTR_M0_MST_DRAM_DED) {
            BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_DRAM_DEDf_SET(m0_en,enable);
        }
        if (intr_type_enable & APERTA2_INTR_M0_MST_DED) {
            BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_DEDf_SET(m0_en,enable);
        }
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_WRITE_EXT_INTR_CTRL_M0_EIERr(&phy->access, m0_en));
    }
    return PHYMOD_E_NONE;
}

/**   Interrupt Enable get
 *    This function used to get the enabled interrupt  
 *
 *    @param phy                Phy access structure
 *    @param port               port number
 *    @param intr_type_enable   interrupt type and enable value
 *
 *    @return PHYMOD_E_NONE
 */
int _plp_aperta2_intr_enable_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t port, uint32_t *intr_enable)
{
    unsigned int port_int_reg = BCM_APERTA2_DIRECT_EXT_INTR_CTRL_OCT0_P0_EIERr;
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EIERr_t      com_dp;  /* 8b8f*/
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_t   com_ctrl0; /*8b8c*/
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_t   com_ctrl1;  /*8b89*/
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIERr_t             m0_en; /* 8b83*/
    unsigned int data = 0;
    int octal = APERTA2_GET_OCTAL(phy->access.lane_mask);

    *intr_enable = 0;
    PHYMOD_IF_ERR_RETURN(
        PHYMOD_BUS_READ(&phy->access, (port_int_reg + (port*3)), &data));
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_DP_EIERr(&phy->access, &com_dp));
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_CTRL0_EIERr(&phy->access, &com_ctrl0));
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_CTRL1_EIERr(&phy->access, &com_ctrl1));
    PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_M0_EIERr(&phy->access, &m0_en));

    if (data & (1<<12)) {
        *intr_enable |= APERTA2_INTR_PORT_PTP;
    }
    if ((data & (1<<11)) && (APERTA2_IS_SYSTEM_SIDE(phy))) {
        *intr_enable |= APERTA2_INTR_PORT_LINK_DOWN;
    }
    if ((data & (1<<10)) && (APERTA2_IS_LINE_SIDE(phy))) {
        *intr_enable |= APERTA2_INTR_PORT_LINK_DOWN;
    }
    if (data & (1<<5)) {
        *intr_enable |= APERTA2_INTR_PORT_SF_ERR_INTR;
    }
    if (data & (1<<4)) {
        *intr_enable |= APERTA2_INTR_PORT_SF_DED_INTR;
    }
    if (data & (1<<3)) {
        *intr_enable |= APERTA2_INTR_PORT_FC_ERR_INTR;
    }
    if (data & (1<<2)) {
        *intr_enable |= APERTA2_INTR_PORT_FC_DED_INTR;
    }
    if (data & (1<<1)) {
        *intr_enable |= APERTA2_INTR_PORT_INTF_ERR_INTR;
    }
    if (data & 1) {
        *intr_enable |= APERTA2_INTR_PORT_INTF_DED_INTR;
    }
    if (APERTA2_IS_SYSTEM_SIDE(phy)) {
        if (octal == APERTA2_PM_OCTAL1) {
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_0_OCT0_RISINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_ABS_RISING;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_0_OCT0_FALLINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_ABS_FALL;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_0_OCT0_FALLINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_EXT_FALL;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_0_OCT0_RISINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_SPMIF_OCT0_ERR_INTRf_GET(com_ctrl0)) {
                *intr_enable |= APERTA2_INTR_CMN_PMIF_ERR;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_SPMIF_OCT0_DED_INTRf_GET(com_ctrl0)) {
                *intr_enable |= APERTA2_INTR_CMN_PMIF_DED;
            }
        } else {
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_0_OCT1_RISINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_ABS_RISING;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_0_OCT1_FALLINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_ABS_FALL;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_0_OCT1_FALLINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_EXT_FALL;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_0_OCT1_RISINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_SPMIF_OCT1_ERR_INTRf_GET(com_ctrl0)) {
                *intr_enable |= APERTA2_INTR_CMN_PMIF_ERR;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_SPMIF_OCT1_DED_INTRf_GET(com_ctrl0)) {
                *intr_enable |= APERTA2_INTR_CMN_PMIF_DED;
            }
        }
    } else {
        if (octal == APERTA2_PM_OCTAL1) {
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_1_OCT0_RISINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_ABS_RISING;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_1_OCT0_FALLINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_ABS_FALL;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_1_OCT0_FALLINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_EXT_FALL;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_1_OCT0_RISINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LPMIF_OCT0_ERR_INTRf_GET(com_ctrl0)) {
                *intr_enable |= APERTA2_INTR_CMN_PMIF_ERR;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LPMIF_OCT0_DED_INTRf_GET(com_ctrl0)) {
                *intr_enable |= APERTA2_INTR_CMN_PMIF_DED;
            }
        } else {
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_1_OCT1_RISINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_ABS_RISING;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_ABS_1_OCT1_FALLINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_ABS_FALL;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_1_OCT1_FALLINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_EXT_FALL;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIERr_ENABLE_MOD_EXT_INTR_1_OCT1_RISINGf_GET(com_ctrl1)) {
                *intr_enable |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LPMIF_OCT1_ERR_INTRf_GET(com_ctrl0)) {
                *intr_enable |= APERTA2_INTR_CMN_PMIF_ERR;
            }
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LPMIF_OCT1_DED_INTRf_GET(com_ctrl0)) {
                *intr_enable |= APERTA2_INTR_CMN_PMIF_DED;
            }
        }
    }
    if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LMI_ERR_INTRf_GET(com_ctrl0)) {
        *intr_enable |= APERTA2_INTR_CMN_LMI_ERR ;
    }
    if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_LMI_DED_INTRf_GET(com_ctrl0)) {
        *intr_enable |= APERTA2_INTR_CMN_LMI_DED;
    }
    if (octal == APERTA2_PM_OCTAL1) {
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EIERr_ENABLE_EGR_DP_CDC_FIFO_DED_INTR_OCT0f_GET(com_dp)) {
            *intr_enable |= APERTA2_INTR_CMN_DP_CDC_FIFO_DED_INTR;
        }
    } else {
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EIERr_ENABLE_EGR_DP_CDC_FIFO_DED_INTR_OCT1f_GET(com_dp)) {
            *intr_enable |= APERTA2_INTR_CMN_DP_CDC_FIFO_DED_INTR;
        }
    }
    if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_PPS_INTR_RISINGf_GET(com_ctrl0)) {
        *intr_enable |= APERTA2_INTR_CMN_PTP_PPS_RAISING;
    }
    if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIERr_ENABLE_PPS_INTR_FALLINGf_GET(com_ctrl0)) {
        *intr_enable |= APERTA2_INTR_CMN_PTP_PPS_FALLING;
    }
    if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_FW_WDOG_EXPf_GET(m0_en)) {
        *intr_enable |= APERTA2_INTR_M0_MST_FW_WDOG_EXP;
    }
    if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_DRAM_DEDf_GET(m0_en)) {
        *intr_enable |= APERTA2_INTR_M0_MST_DRAM_DED;
    }
    if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIERr_ENABLE_MST_DEDf_GET(m0_en)) {
        *intr_enable |= APERTA2_INTR_M0_MST_DED;
    }

    return PHYMOD_E_NONE;
}

/**   Interrupt Status get
 *    This function used to get the interrupt  status
 *
 *    @param phy                Phy access structure
 *    @param port               port number
 *    @param intr_sts   interrupt type and enable value
 *
 *    @return PHYMOD_E_NONE
 */

int _plp_aperta2_phy_intr_status_get(const plp_aperta2_phymod_phy_access_t* phy, unsigned int port, uint32_t *intr_sts)
{
    int event = 0;
    unsigned int port_int_reg = BCM_APERTA2_DIRECT_EXT_INTR_CTRL_OCT0_P0_EISRr, data = 0;
    int octal = APERTA2_GET_OCTAL(phy->access.lane_mask);
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EISRr_t      com_dp_s;  
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_t   com_ctrl0_s;
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_t   com_ctrl1_s;
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EISRr_t             m0_en_s;

    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EIPRr_t      com_dp_p;  
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_t   com_ctrl0_p;
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_t   com_ctrl1_p;
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIPRr_t             m0_en_p;

    event = (*intr_sts & 0x80000000) ? 1: 0;
    *intr_sts = 0;

    if (event) {
        port_int_reg = BCM_APERTA2_DIRECT_EXT_INTR_CTRL_OCT0_P0_EISRr;

        PHYMOD_IF_ERR_RETURN(
                PHYMOD_BUS_READ(&phy->access, (port_int_reg + (port*3)), &data));
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_DP_EISRr(&phy->access, &com_dp_s));
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_CTRL0_EISRr(&phy->access, &com_ctrl0_s));
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_CTRL1_EISRr(&phy->access, &com_ctrl1_s));
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_M0_EISRr(&phy->access, &m0_en_s));
        if (APERTA2_IS_SYSTEM_SIDE(phy)) {
            if (octal == APERTA2_PM_OCTAL1) {
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_0_OCT0_RISINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_RISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_0_OCT0_FALLINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_0_OCT0_FALLINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_0_OCT0_RISINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_SPMIF_OCT0_ERR_INTRf_GET(com_ctrl0_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_ERR;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_SPMIF_OCT0_DED_INTRf_GET(com_ctrl0_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_DED;
                }
            } else {
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_0_OCT1_RISINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_RISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_0_OCT1_FALLINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_0_OCT1_FALLINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_0_OCT1_RISINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_SPMIF_OCT1_ERR_INTRf_GET(com_ctrl0_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_ERR;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_SPMIF_OCT1_DED_INTRf_GET(com_ctrl0_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_DED;
                }
            }
        } else {
            if (octal == APERTA2_PM_OCTAL1) {
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_1_OCT0_RISINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_RISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_1_OCT0_FALLINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_1_OCT0_FALLINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_1_OCT0_RISINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LPMIF_OCT0_ERR_INTRf_GET(com_ctrl0_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_ERR;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LPMIF_OCT0_DED_INTRf_GET(com_ctrl0_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_DED;
                }
            } else {
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_1_OCT1_RISINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_RISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_1_OCT1_FALLINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_1_OCT1_FALLINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_1_OCT1_RISINGf_GET(com_ctrl1_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LPMIF_OCT1_ERR_INTRf_GET(com_ctrl0_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_ERR;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LPMIF_OCT1_DED_INTRf_GET(com_ctrl0_s)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_DED;
                }
            }
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LMI_ERR_INTRf_GET(com_ctrl0_s)) {
            *intr_sts |= APERTA2_INTR_CMN_LMI_ERR ;
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LMI_DED_INTRf_GET(com_ctrl0_s)) {
            *intr_sts |= APERTA2_INTR_CMN_LMI_DED;
        }
        if (octal == APERTA2_PM_OCTAL1) {
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EISRr_EGR_DP_CDC_FIFO_DED_INTR_OCT0f_GET(com_dp_s)) {
                *intr_sts |= APERTA2_INTR_CMN_DP_CDC_FIFO_DED_INTR;
            }
        } else {
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EISRr_EGR_DP_CDC_FIFO_DED_INTR_OCT1f_GET(com_dp_s)) {
                *intr_sts |= APERTA2_INTR_CMN_DP_CDC_FIFO_DED_INTR;
            }
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_PPS_INTR_RISINGf_GET(com_ctrl0_s)) {
            *intr_sts |= APERTA2_INTR_CMN_PTP_PPS_RAISING;
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_PPS_INTR_FALLINGf_GET(com_ctrl0_s)) {
            *intr_sts |= APERTA2_INTR_CMN_PTP_PPS_FALLING;
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EISRr_MST_FW_WDOG_EXPf_GET(m0_en_s)) {
            *intr_sts |= APERTA2_INTR_M0_MST_FW_WDOG_EXP;
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EISRr_MST_DRAM_DEDf_GET(m0_en_s)) {
            *intr_sts |= APERTA2_INTR_M0_MST_DRAM_DED;
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EISRr_MST_DEDf_GET(m0_en_s)) {
            *intr_sts |= APERTA2_INTR_M0_MST_DED;
        }

    } else {
        port_int_reg = BCM_APERTA2_DIRECT_EXT_INTR_CTRL_OCT0_P0_EIPRr;
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_DP_EIPRr(&phy->access, &com_dp_p));
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr(&phy->access, &com_ctrl0_p));
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr(&phy->access, &com_ctrl1_p));
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_M0_EIPRr(&phy->access, &m0_en_p));
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, (port_int_reg + (port*3)), &data));
        if (APERTA2_IS_SYSTEM_SIDE(phy)) {
            if (octal == APERTA2_PM_OCTAL1) {
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_ABS_0_OCT0_RISINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_RISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_ABS_0_OCT0_FALLINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_EXT_INTR_0_OCT0_FALLINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_EXT_INTR_0_OCT0_RISINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_SPMIF_OCT0_ERR_INTRf_GET(com_ctrl0_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_ERR;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_SPMIF_OCT0_DED_INTRf_GET(com_ctrl0_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_DED;
                }
            } else {
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_ABS_0_OCT1_RISINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_RISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_ABS_0_OCT1_FALLINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_EXT_INTR_0_OCT1_FALLINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_EXT_INTR_0_OCT1_RISINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_SPMIF_OCT1_ERR_INTRf_GET(com_ctrl0_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_ERR;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_SPMIF_OCT1_DED_INTRf_GET(com_ctrl0_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_DED;
                }
            }
        } else {
            if (octal == APERTA2_PM_OCTAL1) {
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_ABS_1_OCT0_RISINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_RISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_ABS_1_OCT0_FALLINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_EXT_INTR_1_OCT0_FALLINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_EXT_INTR_1_OCT0_RISINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_LPMIF_OCT0_ERR_INTRf_GET(com_ctrl0_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_ERR;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_LPMIF_OCT0_DED_INTRf_GET(com_ctrl0_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_DED;
                }
            } else {
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_ABS_1_OCT1_RISINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_RISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_ABS_1_OCT1_FALLINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_ABS_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_EXT_INTR_1_OCT1_FALLINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_FALL;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EIPRr_P_MOD_EXT_INTR_1_OCT1_RISINGf_GET(com_ctrl1_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_MOD_EXT_RAISING;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_LPMIF_OCT1_ERR_INTRf_GET(com_ctrl0_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_ERR;
                }
                if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_LPMIF_OCT1_DED_INTRf_GET(com_ctrl0_p)) {
                    *intr_sts |= APERTA2_INTR_CMN_PMIF_DED;
                }
            }
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_LMI_ERR_INTRf_GET(com_ctrl0_p)) {
            *intr_sts |= APERTA2_INTR_CMN_LMI_ERR ;
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_LMI_DED_INTRf_GET(com_ctrl0_p)) {
            *intr_sts |= APERTA2_INTR_CMN_LMI_DED;
        }
        if (octal == APERTA2_PM_OCTAL1) {
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EIPRr_P_EGR_DP_CDC_FIFO_DED_INTR_OCT0f_GET(com_dp_p)) {
                *intr_sts |= APERTA2_INTR_CMN_DP_CDC_FIFO_DED_INTR;
            }
        } else {
            if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EIPRr_P_EGR_DP_CDC_FIFO_DED_INTR_OCT1f_GET(com_dp_p)) {
                *intr_sts |= APERTA2_INTR_CMN_DP_CDC_FIFO_DED_INTR;
            }
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_PPS_INTR_RISINGf_GET(com_ctrl0_p)) {
            *intr_sts |= APERTA2_INTR_CMN_PTP_PPS_RAISING;
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EIPRr_P_PPS_INTR_FALLINGf_GET(com_ctrl0_p)) {
            *intr_sts |= APERTA2_INTR_CMN_PTP_PPS_FALLING;
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIPRr_P_MST_FW_WDOG_EXPf_GET(m0_en_p)) {
            *intr_sts |= APERTA2_INTR_M0_MST_FW_WDOG_EXP;
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIPRr_P_MST_DRAM_DEDf_GET(m0_en_p)) {
            *intr_sts |= APERTA2_INTR_M0_MST_DRAM_DED;
        }
        if (BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EIPRr_P_MST_DEDf_GET(m0_en_p)) {
            *intr_sts |= APERTA2_INTR_M0_MST_DED;
        }

    }
    if (data & (1<<12)) {
        *intr_sts |= APERTA2_INTR_PORT_PTP;
    }
    if ((data & (1<<11)) && (APERTA2_IS_SYSTEM_SIDE(phy))) {
        *intr_sts |= APERTA2_INTR_PORT_LINK_DOWN;
    }
    if ((data & (1<<10)) && (APERTA2_IS_LINE_SIDE(phy))) {
        *intr_sts |= APERTA2_INTR_PORT_LINK_DOWN;
    }
    if (data & (1<<5)) {
        *intr_sts |= APERTA2_INTR_PORT_SF_ERR_INTR;
    }
    if (data & (1<<4)) {
        *intr_sts |= APERTA2_INTR_PORT_SF_DED_INTR;
    }
    if (data & (1<<3)) {
        *intr_sts |= APERTA2_INTR_PORT_FC_ERR_INTR;
    }
    if (data & (1<<2)) {
        *intr_sts |= APERTA2_INTR_PORT_FC_DED_INTR;
    }
    if (data & (1<<1)) {
        *intr_sts |= APERTA2_INTR_PORT_INTF_ERR_INTR;
    }
    if (data & 1) {
        *intr_sts |= APERTA2_INTR_PORT_INTF_DED_INTR;
    }

    return PHYMOD_E_NONE;
}

/**   Interrupt Status Clear
 *    This function used to clear the interrupt
 *
 *    @param phy                Phy access structure
 *    @param port               port number
 *    @param intr_type          interrupt type to clear
 *
 *    @return PHYMOD_E_NONE
 */
int _plp_aperta2_phy_intr_status_clear(const plp_aperta2_phymod_phy_access_t* phy, unsigned int port, uint32_t intr_type)
{
    unsigned int port_int_reg = BCM_APERTA2_DIRECT_EXT_INTR_CTRL_OCT0_P0_EISRr;
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EISRr_t      com_dp_s;  
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_t   com_ctrl0_s;
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_t   com_ctrl1_s;
    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EISRr_t             m0_en_s;
    unsigned int data = 0;
    int octal = APERTA2_GET_OCTAL(phy->access.lane_mask);

    PHYMOD_MEMSET(&com_dp_s, 0, sizeof(com_dp_s));
    PHYMOD_MEMSET(&com_ctrl0_s, 0, sizeof(com_ctrl0_s));
    PHYMOD_MEMSET(&com_ctrl1_s, 0, sizeof(com_ctrl1_s));
    PHYMOD_MEMSET(&m0_en_s, 0, sizeof(m0_en_s));

    if (intr_type & APERTA2_INTR_PORT) {
        if (intr_type & APERTA2_INTR_PORT_PTP) {
            data |= (1 << 12);
            data |= (1 << 13);
        }
        if (intr_type & APERTA2_INTR_PORT_LINK_DOWN) {
            if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                data |= (1 << 11);
            } else {
                data |= (1 << 10);
            }
        }
        if (intr_type & APERTA2_INTR_PORT_SF_ERR_INTR) {
           data |= (1 << 5);
        }
        if (intr_type & APERTA2_INTR_PORT_SF_DED_INTR) {
           data |= (1 << 4);
        }
        if (intr_type & APERTA2_INTR_PORT_FC_ERR_INTR) {
            /* Egress & ingress*/
           data |= (1 << 3);
           data |= (1 << 9);
        }
        if (intr_type & APERTA2_INTR_PORT_FC_DED_INTR) {
            /* Egress*/
           data |= (1 << 2);
            /* ingress*/
           data |= (1 << 8);
        }
        if (intr_type & APERTA2_INTR_PORT_INTF_ERR_INTR) {
            /* Egress*/
           data |= (1 << 1);
            /* ingress*/
           data |= (1<< 7);
        }
        if (intr_type & APERTA2_INTR_PORT_INTF_DED_INTR) {
            /* Egress*/
           data |= (1 << 0);
            /* ingress*/
           data |= (1 << 6);
        }
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, (port_int_reg + (port*3)), data));
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, (port_int_reg + (port*3)), &data));

    } 
    if (intr_type & APERTA2_INTR_COMMON) {
        if (intr_type & APERTA2_INTR_CMN_MOD_ABS_RISING) {
            if (octal == APERTA2_PM_OCTAL1) {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_0_OCT0_RISINGf_SET(com_ctrl1_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_1_OCT0_RISINGf_SET(com_ctrl1_s, 1);
                }
            } else {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_0_OCT1_RISINGf_SET(com_ctrl1_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_1_OCT1_RISINGf_SET(com_ctrl1_s, 1);
                }
            }
        }
        if (intr_type & APERTA2_INTR_CMN_MOD_ABS_FALL) {
            if (octal == APERTA2_PM_OCTAL1) {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_0_OCT0_FALLINGf_SET(com_ctrl1_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_1_OCT0_FALLINGf_SET(com_ctrl1_s, 1);
                }
            } else {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_0_OCT1_FALLINGf_SET(com_ctrl1_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_ABS_1_OCT1_FALLINGf_SET(com_ctrl1_s, 1);
                }
            }

        }
        if (intr_type & APERTA2_INTR_CMN_MOD_EXT_RAISING) {
            if (octal == APERTA2_PM_OCTAL1) {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_0_OCT0_RISINGf_SET(com_ctrl1_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_1_OCT0_RISINGf_SET(com_ctrl1_s, 1);
                }
            } else {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_0_OCT1_RISINGf_SET(com_ctrl1_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_1_OCT1_RISINGf_SET(com_ctrl1_s, 1);
                }
            }
        }
        if (intr_type & APERTA2_INTR_CMN_MOD_EXT_FALL) {
            if (octal == APERTA2_PM_OCTAL1) {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_0_OCT0_FALLINGf_SET(com_ctrl1_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_1_OCT0_FALLINGf_SET(com_ctrl1_s, 1);
                }
            } else {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_0_OCT1_FALLINGf_SET(com_ctrl1_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL1_EISRr_MOD_EXT_INTR_1_OCT1_FALLINGf_SET(com_ctrl1_s, 1);
                }
            }
        }
        if (intr_type & APERTA2_INTR_CMN_LMI_ERR) {
            BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LMI_ERR_INTRf_SET(com_ctrl0_s, 1);
        }
        if (intr_type & APERTA2_INTR_CMN_LMI_DED) {
            BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LMI_DED_INTRf_SET(com_ctrl0_s, 1);
        }
        if (intr_type & APERTA2_INTR_CMN_PMIF_ERR) {
            if (octal == APERTA2_PM_OCTAL1) {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_SPMIF_OCT0_ERR_INTRf_SET(com_ctrl0_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LPMIF_OCT0_ERR_INTRf_SET(com_ctrl0_s, 1);
                }
            } else {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_SPMIF_OCT1_ERR_INTRf_SET(com_ctrl0_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LPMIF_OCT1_ERR_INTRf_SET(com_ctrl0_s, 1);
                }
            }
        }
        if (intr_type & APERTA2_INTR_CMN_PMIF_DED) {
            if (octal == APERTA2_PM_OCTAL1) {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_SPMIF_OCT0_DED_INTRf_SET(com_ctrl0_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LPMIF_OCT0_DED_INTRf_SET(com_ctrl0_s, 1);
                }
            } else {
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_SPMIF_OCT1_DED_INTRf_SET(com_ctrl0_s, 1);
                } else {
                    BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_LPMIF_OCT1_DED_INTRf_SET(com_ctrl0_s, 1);
                }
            }
        }
        if (intr_type & APERTA2_INTR_CMN_DP_CDC_FIFO_DED_INTR) {
            if (octal == APERTA2_PM_OCTAL1) {
                BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EISRr_EGR_DP_CDC_FIFO_DED_INTR_OCT0f_SET(com_dp_s, 1);
                BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EISRr_ING_DP_CDC_FIFO_DED_INTR_OCT0f_SET(com_dp_s, 1);
            } else {
                BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EISRr_EGR_DP_CDC_FIFO_DED_INTR_OCT1f_SET(com_dp_s, 1);
                BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_DP_EISRr_ING_DP_CDC_FIFO_DED_INTR_OCT1f_SET(com_dp_s, 1);
            }
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_DP_EISRr(&phy->access, &com_dp_s));

        }
        if (intr_type & APERTA2_INTR_CMN_PTP_PPS_RAISING) {
            BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_PPS_INTR_RISINGf_SET(com_ctrl0_s, 1);
        }
        if (intr_type & APERTA2_INTR_CMN_PTP_PPS_FALLING) {
            BCM_APERTA2_DIRECT_EXT_INTR_CTRL_COMMON_CTRL0_EISRr_PPS_INTR_FALLINGf_SET(com_ctrl0_s, 1);
        }

        if (intr_type & APERTA2_INTR_COMMON_CTRL0) {
            PHYMOD_IF_ERR_RETURN(
                  BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_CTRL0_EISRr(&phy->access, &com_ctrl0_s));
        }
        if (intr_type & APERTA2_INTR_COMMON_CTRL1) {
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_EXT_INTR_CTRL_COMMON_CTRL1_EISRr(&phy->access, &com_ctrl1_s));
        }
    }

    if (intr_type & APERTA2_INTR_M0_MST_FW_WDOG_EXP) {
        BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EISRr_MST_FW_WDOG_EXPf_SET(m0_en_s, 1);
    }
    if (intr_type & APERTA2_INTR_M0_MST_DRAM_DED) {
        BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EISRr_MST_DRAM_DEDf_SET(m0_en_s, 1);
    }
    if (intr_type & APERTA2_INTR_M0_MST_DED) {
        BCM_APERTA2_DIRECT_EXT_INTR_CTRL_M0_EISRr_MST_DEDf_SET(m0_en_s, 1); 
    }
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_WRITE_EXT_INTR_CTRL_M0_EISRr(&phy->access, m0_en_s));

    return PHYMOD_E_NONE;
}

/**   GPIO config 
 *    This function used to configure GPIO
 *
 *    @param phy                Phy access structure
 *    @param pin_no             GPIO Pin number
 *    @param gpio_mode          GPIO mode
 *
 *    @return PHYMOD_E_NONE
 */
int _plp_aperta2_phy_gpio_config_set(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, plp_aperta2_phymod_gpio_mode_t gpio_mode)
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

    if((pin_no >= APERTA2_GPIO_MIN_PIN) && (pin_no <= APERTA2_GPIO_MAX_PIN)) {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_read(phy, APERTA2_GPIO_CTRL1_BASE_ADDR + (pin_no * 4), &pad_ctrl_gpio_x_ctrl_rd));
        pad_ctrl_gpio_x_ctrl_wr = ((pad_ctrl_gpio_x_ctrl_rd & ~APERTA2_GPIO_CTRL_OEBF_MASK) | data );
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(phy, APERTA2_GPIO_CTRL1_BASE_ADDR + (pin_no * 4), pad_ctrl_gpio_x_ctrl_wr));
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Invalid GPIO pin selected")));
    }

    return PHYMOD_E_NONE;
}

/**   GPIO config 
 *    This function used to Get configured GPIO
 *
 *    @param phy                Phy access structure
 *    @param pin_no             GPIO Pin number
 *    @param gpio_mode          GPIO mode
 *
 *    @return PHYMOD_E_NONE
 */
int _plp_aperta2_phy_gpio_config_get(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, plp_aperta2_phymod_gpio_mode_t* gpio_mode)
{
    uint32_t  pad_ctrl_gpio_x_ctrl_rd = 0;
    uint16_t  data = 0;

    if((pin_no >= APERTA2_GPIO_MIN_PIN) && (pin_no <= APERTA2_GPIO_MAX_PIN)) {
        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_direct_reg_read(phy, APERTA2_GPIO_CTRL1_BASE_ADDR + (pin_no * 4), &pad_ctrl_gpio_x_ctrl_rd));
        data = (pad_ctrl_gpio_x_ctrl_rd & APERTA2_GPIO_CTRL_OEBF_MASK);
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

/**   GPIO Pin value
 *    This function used to set GPIO Pin value
 *
 *    @param phy                Phy access structure
 *    @param pin_no             GPIO Pin number
 *    @param value              Value 
 *
 *    @return PHYMOD_E_NONE
 */

int _plp_aperta2_phy_gpio_pin_value_set(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, int value)
{
    uint32_t  pad_ctrl_gpio_ctrl = 0;
    uint16_t  pull_up_dwn_data = ((value >> 0x1)& 0x1) ? 0x1 /* PULL UP */ : 0x2 /* PULL DOWN */ ;

    if ((pin_no >= APERTA2_GPIO_MIN_PIN) && (pin_no <= APERTA2_GPIO_MAX_PIN)) {
        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_direct_reg_read(phy, APERTA2_GPIO_CTRL1_BASE_ADDR + (pin_no * 4), &pad_ctrl_gpio_ctrl));
        pad_ctrl_gpio_ctrl &= ~(3 << 5);
        pad_ctrl_gpio_ctrl |= ((value &1) ? (3 << 5) : 0);
        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_direct_reg_write(phy, APERTA2_GPIO_CTRL1_BASE_ADDR + (pin_no * 4), pad_ctrl_gpio_ctrl));

        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_direct_reg_read(phy, APERTA2_GPIO_CTRL0_BASE_ADDR + (pin_no * 4), &pad_ctrl_gpio_ctrl));
        if (pull_up_dwn_data == 1) { /* PULL UP*/
            pad_ctrl_gpio_ctrl &= ~(3);
            pad_ctrl_gpio_ctrl |= 1;
        }
        if (pull_up_dwn_data == 2) { /* PULL Down*/
            pad_ctrl_gpio_ctrl &= ~(3);
            pad_ctrl_gpio_ctrl |= 2;
        }
        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_direct_reg_write(phy, APERTA2_GPIO_CTRL0_BASE_ADDR + (pin_no * 4), pad_ctrl_gpio_ctrl));
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Invalid GPIO pin selected")));
    }
    return PHYMOD_E_NONE;
}

/**   GPIO Pin value get
 *    This function used to get GPIO Pin value
 *
 *    @param phy                Phy access structure
 *    @param pin_no             GPIO Pin number
 *    @param value              Value 
 *
 *    @return PHYMOD_E_NONE
 */
int _plp_aperta2_phy_gpio_pin_value_get(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, int* value)
{
    uint32_t pad_ctrl_gpio_sts = 0;
    uint32_t pull_up_dwn_data = 0 ;
    uint16_t cfg_value = 0;
    
    if((pin_no >= APERTA2_GPIO_MIN_PIN) && (pin_no <= APERTA2_GPIO_MAX_PIN)) {
        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_direct_reg_read(phy, APERTA2_GPIO_CTRL0_BASE_ADDR + (pin_no * 4), &pull_up_dwn_data));
        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_direct_reg_read(phy, APERTA2_GPIO_STS_BASE_ADDR + (pin_no * 4), &pad_ctrl_gpio_sts));

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

/**   I2C Module command
 *    This function used to set Module Command
 *
 *    @param phy                Phy access structure
 *    @param xfer_addr          Read/write address
 *    @param slv_addr           slave to access
 *    @param xfer_cnt           Transfer count 
 *    @param cmd                Module command
 *
 *    @return PHYMOD_E_NONE
 */
int _plp_aperta2_set_module_command(const plp_aperta2_phymod_phy_access_t* phy, uint16_t xfer_addr, uint32_t slv_addr, unsigned char xfer_cnt, aperta2_i2c_module_cmd_t cmd)
{
    BCM_APERTA2_DIRECT_MODULE_CNTRL_CONTROLr_t           mod_ctrl;
    BCM_APERTA2_DIRECT_MODULE_CNTRL_ADDRESSr_t           mod_add;
    BCM_APERTA2_DIRECT_MODULE_CNTRL_STATUSr_t            mod_ctrl_sts;
    BCM_APERTA2_DIRECT_MODULE_CNTRL_XFER_COUNTr_t        mod_xfer_cnt;
    BCM_APERTA2_DIRECT_MODULE_CNTRL_XFER_ADDRESSr_t      mod_xfer_add;

    uint16_t retry_count = 1000, data = 0;
    uint32_t wait_timeout_us = 0;
    wait_timeout_us = ((2*(xfer_cnt+1))*100)/5;

    PHYMOD_MEMSET(&mod_ctrl,     0, sizeof(BCM_APERTA2_DIRECT_MODULE_CNTRL_CONTROLr_t     ));
    PHYMOD_MEMSET(&mod_add,      0, sizeof(BCM_APERTA2_DIRECT_MODULE_CNTRL_ADDRESSr_t     ));
    PHYMOD_MEMSET(&mod_ctrl_sts, 0, sizeof(BCM_APERTA2_DIRECT_MODULE_CNTRL_STATUSr_t      ));
    PHYMOD_MEMSET(&mod_xfer_cnt, 0, sizeof(BCM_APERTA2_DIRECT_MODULE_CNTRL_XFER_COUNTr_t  ));
    PHYMOD_MEMSET(&mod_xfer_add, 0, sizeof(BCM_APERTA2_DIRECT_MODULE_CNTRL_XFER_ADDRESSr_t));

    if (cmd == APERTA2_FLUSH) {
        BCM_APERTA2_DIRECT_MODULE_CNTRL_CONTROLr_SET(mod_ctrl, 0xC000);
        PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_MODULE_CNTRL_CONTROLr(&phy->access, mod_ctrl));
    } else {
        BCM_APERTA2_DIRECT_MODULE_CNTRL_XFER_ADDRESSr_SET(mod_xfer_add, xfer_addr);
        BCM_APERTA2_DIRECT_MODULE_CNTRL_XFER_COUNTr_XFER_CNTf_SET(mod_xfer_cnt, xfer_cnt);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_MODULE_CNTRL_XFER_ADDRESSr(&phy->access, mod_xfer_add));
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_MODULE_CNTRL_XFER_COUNTr(&phy->access, mod_xfer_cnt));
        if (cmd == APERTA2_CURRENT_ADDRESS_READ) {
            BCM_APERTA2_DIRECT_MODULE_CNTRL_CONTROLr_SET(mod_ctrl, 0x8001);
            PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_MODULE_CNTRL_CONTROLr(&phy->access, mod_ctrl));
        } else if (cmd == APERTA2_RANDOM_ADDRESS_READ ) {
            BCM_APERTA2_DIRECT_MODULE_CNTRL_ADDRESSr_SET(mod_add, slv_addr);
            BCM_APERTA2_DIRECT_MODULE_CNTRL_CONTROLr_SET(mod_ctrl, 0x8003);
            PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_MODULE_CNTRL_ADDRESSr(&phy->access, mod_add));
            PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_MODULE_CNTRL_CONTROLr(&phy->access, mod_ctrl));
        } else {
            BCM_APERTA2_DIRECT_MODULE_CNTRL_ADDRESSr_SET(mod_add, slv_addr);
            BCM_APERTA2_DIRECT_MODULE_CNTRL_CONTROLr_SET(mod_ctrl, 0x8022);
            PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_MODULE_CNTRL_ADDRESSr(&phy->access, mod_add));
            PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_MODULE_CNTRL_CONTROLr(&phy->access, mod_ctrl));
        }
    }
    PHYMOD_USLEEP(500);
    if ((cmd == APERTA2_CURRENT_ADDRESS_READ) || (cmd == APERTA2_RANDOM_ADDRESS_READ) || (cmd == APERTA2_I2C_WRITE)) {
        PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_MODULE_CNTRL_XFER_COUNTr(&phy->access, &mod_xfer_cnt));
        if (BCM_APERTA2_DIRECT_MODULE_CNTRL_XFER_COUNTr_XFER_CNTf_GET(mod_xfer_cnt) == (xfer_cnt+1)) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_RESOURCE, (_PHYMOD_MSG("ERROR: Module not Available")));
        }
        do {
            PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_MODULE_CNTRL_STATUSr(&phy->access, &mod_ctrl_sts));
            data = BCM_APERTA2_DIRECT_MODULE_CNTRL_STATUSr_XACTION_DONEf_GET(mod_ctrl_sts);
            PHYMOD_USLEEP(wait_timeout_us);
        } while((data == 0) && --retry_count);
        if(!retry_count) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_TIMEOUT, (_PHYMOD_MSG("ERROR: I2C transaction failed")));
        }
    }
    BCM_APERTA2_DIRECT_MODULE_CNTRL_CONTROLr_SET(mod_ctrl, 0x3);
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_MODULE_CNTRL_CONTROLr(&phy->access, mod_ctrl));

    return PHYMOD_E_NONE;
}

/**   I2C Read
 *    This function used to read data from the start_addr of the Module 
 *
 *    @param phy                Phy access structure
 *    @param slv_dev_addr       I2C Slave address
 *    @param start_addr         Start offset 
 *    @param no_of_bytes        Number of bytes to read 
 *    @param read_data          Data  
 *
 *    @return PHYMOD_E_NONE
 */
int _plp_aperta2_phy_i2c_read(const plp_aperta2_phymod_phy_access_t* phy, uint32_t slv_dev_addr, uint32_t start_addr, uint32_t no_of_bytes, uint8_t* read_data)
{
    BCM_APERTA2_DIRECT_MODULE_CNTRL_DEV_IDr_t            dev_id ;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_t        gen_ctrl1;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL3r_t        gen_ctrl3;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_t mod_ram_mdio_ctrl ;
    uint32_t lower_page_start_addr = 0;
    uint32_t upper_page_start_addr = 0;
    uint32_t lower_page_bytes = 0;
    uint32_t upper_page_bytes = 0;
    uint32_t lower_page_flag  = 0;
    uint32_t upper_page_flag  = 0;
    uint16_t START_OF_NVRAM   = 0;
    uint32_t index     = 0;
    uint32_t rd_data   = 0;
    uint8_t sys_side   = 0;
    uint8_t octal      = 0, nvram = 0;

    PHYMOD_MEMSET(&dev_id,            0, sizeof(BCM_APERTA2_DIRECT_MODULE_CNTRL_DEV_IDr_t           ));
    PHYMOD_MEMSET(&gen_ctrl1,         0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_t       ));
    PHYMOD_MEMSET(&gen_ctrl3,         0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL3r_t       ));
    PHYMOD_MEMSET(&mod_ram_mdio_ctrl, 0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_t));

    sys_side = APERTA2_IS_SYSTEM_SIDE(phy) ? 1 : 0;
    octal = APERTA2_GET_OCTAL(phy->access.lane_mask);

    /* qsfp mode or legacy mode */
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL3r(&phy->access, &gen_ctrl3));
    gen_ctrl3.v[0] &= ~(3) ;

    /* Select Module type and NVRAM based on the side and 
     * octal */
    if (sys_side) {
        if (octal == APERTA2_PM_OCTAL1) {
            gen_ctrl3.v[0] |= 0 ;
            nvram = 0;
        } else {
            gen_ctrl3.v[0] |= 2 ;
            nvram = 2;
        }
    } else {
        if (octal == APERTA2_PM_OCTAL1) {
            gen_ctrl3.v[0] |= 1;
            nvram = 1;
        } else {
            gen_ctrl3.v[0] |= 3;
            nvram = 3;
        }
    }
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL3r_UCSPI_SLOWf_SET(gen_ctrl3,0);
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL3r(&phy->access, gen_ctrl3));

    /* qsfp reset at beginning */
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, &gen_ctrl1));
    gen_ctrl1.v[0] &= ~(1 << 6);
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, gen_ctrl1));
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, &gen_ctrl1));
    gen_ctrl1.v[0] |= (1 << 6);
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, gen_ctrl1));

    /* Select QSFP module and associated NVRAM */
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr(&phy->access, &mod_ram_mdio_ctrl));
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_MD_EXTMOD_SELECTf_SET(mod_ram_mdio_ctrl, nvram);
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr(&phy->access, mod_ram_mdio_ctrl));

    /* Configure the slave device ID default is 0x50 */
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_MODULE_CNTRL_DEV_IDr(&phy->access, &dev_id));
    BCM_APERTA2_DIRECT_MODULE_CNTRL_DEV_IDr_SL_DEV_ADDf_SET(dev_id, slv_dev_addr);
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_MODULE_CNTRL_DEV_IDr(&phy->access, dev_id));

    if(no_of_bytes == 0) {
        /* Perform module controller reset and FLUSH */
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_set_module_command(phy, 0, 0, 0, APERTA2_FLUSH));
    }

    if ((no_of_bytes + start_addr) >= 256) {
        no_of_bytes = 255 - start_addr + 1;
    }

    /* To determine page to be written is lower page or upper page or
     * both lower and upper page
     */
    if ((start_addr+no_of_bytes - 1) > 127) {
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

    if (lower_page_flag) {
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_set_module_command(phy, 0, 0, 0, APERTA2_FLUSH));
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_set_module_command(phy, (APERTA2_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM + lower_page_start_addr),
                                                        lower_page_start_addr, lower_page_bytes - 1, APERTA2_RANDOM_ADDRESS_READ));
        lower_page_flag = 0;
    }

    if (upper_page_flag) {
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_set_module_command(phy, 0, 0, 0, APERTA2_FLUSH));
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_set_module_command(phy, (APERTA2_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM + upper_page_start_addr),
                                                        upper_page_start_addr, upper_page_bytes - 1, APERTA2_RANDOM_ADDRESS_READ));
        upper_page_flag = 0;
    }

   /* Read data from NVRAM using I2C */
    for (index = 0; index < no_of_bytes; index++) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_read(phy, (0x10000 + APERTA2_MODULE_CNTRL_RAM_NVR0_ADR + start_addr + index),  &rd_data));
       read_data[index] = (unsigned char) (rd_data & 0xff);
    }
    return PHYMOD_E_NONE ;
}

/**   I2C Write
 *    This function used to write data to Module at the start_addr
 *
 *    @param phy                Phy access structure
 *    @param slv_dev_addr       I2C Slave address
 *    @param start_addr         Start offset 
 *    @param no_of_bytes        Number of bytes to write
 *    @param read_data          Data  
 *
 *    @return PHYMOD_E_NONE
 */
int _plp_aperta2_phy_i2c_write(const plp_aperta2_phymod_phy_access_t* phy, uint32_t slv_dev_addr, uint32_t start_addr, uint32_t no_of_bytes, const uint8_t* write_data)
{
    BCM_APERTA2_DIRECT_MODULE_CNTRL_DEV_IDr_t            dev_id ;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_t        gen_ctrl1;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL3r_t        gen_ctrl3;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_t mod_ram_mdio_ctrl ;
    uint32_t lower_page_start_addr = 0;
    uint32_t upper_page_start_addr = 0;
    uint32_t lower_page_bytes = 0;
    uint32_t upper_page_bytes = 0;
    uint32_t lower_page_flag  = 0;
    uint32_t upper_page_flag  = 0;
    uint16_t START_OF_NVRAM   = 0;
    uint32_t index     = 0;
    uint8_t sys_side   = 0;
    uint8_t octal      = 0, nvram = 0;

    PHYMOD_MEMSET(&dev_id,            0, sizeof(BCM_APERTA2_DIRECT_MODULE_CNTRL_DEV_IDr_t           ));
    PHYMOD_MEMSET(&gen_ctrl1,         0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_t       ));
    PHYMOD_MEMSET(&gen_ctrl3,         0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL3r_t       ));
    PHYMOD_MEMSET(&mod_ram_mdio_ctrl, 0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_t));

    sys_side = APERTA2_IS_SYSTEM_SIDE(phy) ? 1 : 0;
    octal = APERTA2_GET_OCTAL(phy->access.lane_mask);

    /* qsfp mode or legacy mode */
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL3r(&phy->access, &gen_ctrl3));
    gen_ctrl3.v[0] &= ~(3) ;

    /* Select Module type and NVRAM based on the side and 
     * octal */
    if (sys_side) {
        if (octal == APERTA2_PM_OCTAL1) {
            gen_ctrl3.v[0] |= 0 ;
            nvram = 0;
        } else {
            gen_ctrl3.v[0] |= 2 ;
            nvram = 2;
        }
    } else {
        if (octal == APERTA2_PM_OCTAL1) {
            gen_ctrl3.v[0] |= 1;
            nvram = 1;
        } else {
            gen_ctrl3.v[0] |= 3;
            nvram = 3;
        }
    }
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL3r_UCSPI_SLOWf_SET(gen_ctrl3,0);
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL3r(&phy->access, gen_ctrl3));

    /* qsfp reset at beginning */
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, &gen_ctrl1));
    gen_ctrl1.v[0] &= ~(1 << 6);
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, gen_ctrl1));
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, &gen_ctrl1));
    gen_ctrl1.v[0] |= (1 << 6);
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&phy->access, gen_ctrl1));

    /* Select QSFP module and associated NVRAM */
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr(&phy->access, &mod_ram_mdio_ctrl));
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_MD_EXTMOD_SELECTf_SET(mod_ram_mdio_ctrl, nvram);
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr(&phy->access, mod_ram_mdio_ctrl));

    /* Configure the slave device ID default is 0x50 */
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_READ_MODULE_CNTRL_DEV_IDr(&phy->access, &dev_id));
    BCM_APERTA2_DIRECT_MODULE_CNTRL_DEV_IDr_SL_DEV_ADDf_SET(dev_id, slv_dev_addr);
    PHYMOD_IF_ERR_RETURN(BCM_APERTA2_DIRECT_WRITE_MODULE_CNTRL_DEV_IDr(&phy->access, dev_id));

    if(no_of_bytes == 0) {
        /* Perform module controller reset and FLUSH */
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_set_module_command(phy, 0, 0, 0, APERTA2_FLUSH));
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
            lower_page_flag  = 1;
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
        lower_page_flag  = 1;
        lower_page_start_addr = start_addr;
    }

    /* Write data to NVRAM */
    for (index = 0; index < no_of_bytes; index++) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_write(phy, (0x10000+APERTA2_MODULE_CNTRL_RAM_NVR0_ADR+START_OF_NVRAM+start_addr+index), write_data[index]));
    }

    if(lower_page_flag) {
        for (index = 0; index < (lower_page_bytes / 4); index ++) {
            PHYMOD_IF_ERR_RETURN(_plp_aperta2_set_module_command(phy, APERTA2_MODULE_CNTRL_RAM_NVR0_ADR+START_OF_NVRAM+lower_page_start_addr+(4*index),
                                                            lower_page_start_addr+(4*index),3, APERTA2_I2C_WRITE));
        }
        if ((lower_page_bytes % 4) > 0) {
            PHYMOD_IF_ERR_RETURN(_plp_aperta2_set_module_command(phy, APERTA2_MODULE_CNTRL_RAM_NVR0_ADR+START_OF_NVRAM+lower_page_start_addr+(4*index),
                                                            lower_page_start_addr+(4*index), ((lower_page_bytes%4)-1), APERTA2_I2C_WRITE));
        }
        lower_page_flag = 0;
    }

    if(upper_page_flag) {
        for (index = 0; index < (upper_page_bytes / 4); index++) {
            PHYMOD_IF_ERR_RETURN(_plp_aperta2_set_module_command(phy, (APERTA2_MODULE_CNTRL_RAM_NVR0_ADR+START_OF_NVRAM+upper_page_start_addr+(4*index)),
                                                            upper_page_start_addr + (4*index), 3, APERTA2_I2C_WRITE));
        }
        if ((upper_page_bytes%4) > 0) {
            PHYMOD_IF_ERR_RETURN(_plp_aperta2_set_module_command(phy, (APERTA2_MODULE_CNTRL_RAM_NVR0_ADR+START_OF_NVRAM+upper_page_start_addr+(4*index)),
                                                            upper_page_start_addr + (4*index), ((upper_page_bytes%4)-1), APERTA2_I2C_WRITE));
        }
        upper_page_flag = 0;
    }
    return PHYMOD_E_NONE ;
}


