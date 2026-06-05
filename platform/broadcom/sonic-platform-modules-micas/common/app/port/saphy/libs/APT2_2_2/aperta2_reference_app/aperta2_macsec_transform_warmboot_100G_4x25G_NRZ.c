/*
 * $Copyright: (c) 2024 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * bcm_pm_if APIs. This program might not work for all environments.
 *
 * Note: INITIALIZE/MEMSET EVERY FUNCTION PARAMETERS TO "0",
 *       TO AVOID FUTURE COMPATIBILITY ISSUES WITH THE DRIVER.
 */

/* Includes */
#include <aperta2_macsec_common.h>


/* -------------------------------------------------------------------------------- *
 *                         Aperta2 reference application                             *
 * -------------------------------------------------------------------------------- *
 * This sample application configures the Aperta2 chip for 100G (4x25G)              *
 * macsec transform mode as follows:                                                *
 *   1. Connect to the board.                                                       *
 *   2. Initialize the PHY and download the firmware(Warmboot mode)                 *
 *   3. Register Macsec Warmboot callbacks with the driver                          *
 *   4. Restore Macsec configuration from persistent memory on the host             *
 *   5. Run Macsec Traffic, dump statistics                                         *
 *   6. Close the connection to the board.                                          *
 * -------------------------------------------------------------------------------- *
 * Please Note : This sample application is only for 85343 parts.                   *
 * -------------------------------------------------------------------------------- */

/* 100G NRZ (4x25G) MACsec mode */
#define OP_MODE_STR    "100G NRZ (4x25G) MACsec Warmboot mode"
bcm_plp_cfye_vport_handle_t warm_vport_handle[MAX_NUM_OF_LANES][BCM_PLP_SECY_ROLE_EGRESS_INGRESS];
bcm_plp_cfye_rule_handle_t  warm_rule_handle[MAX_NUM_OF_LANES][BCM_PLP_SECY_ROLE_EGRESS_INGRESS];
bcm_plp_secy_sa_handle_t    warm_secy_sahandle[MAX_NUM_OF_LANES][BCM_PLP_SECY_ROLE_EGRESS_INGRESS];
unsigned int warm_vport_ids[MAX_NUM_OF_LANES][BCM_PLP_SECY_ROLE_EGRESS_INGRESS];

unsigned int sys_lane_map_list[] =  {
                                      0x000F, 0x00F0, 0x0F00, 0xF000
                                    };
unsigned int line_lane_map_list[] =  {
                                      0x000F, 0x00F0, 0x0F00, 0xF000
                                    };


#define APP_MACSEC_WARMBOOT_NOF_AREAS 20

FILE *fd[APP_MACSEC_WARMBOOT_NOF_AREAS] = {NULL};
FILE *mfptr = NULL;

bcm_plp_warmboot_status_t app_macsec_alloc(const bcm_plp_sec_phy_access_t *pa,
                                           const unsigned int storage_byte_count,
                                           unsigned int *const area_id_p);

bcm_plp_warmboot_status_t app_macsec_read(bcm_plp_sec_phy_access_t *pa,
                                          const unsigned int area_id,
                                          unsigned char *const data_p,
                                          const unsigned int byte_offset,
                                          const unsigned int byte_count);

bcm_plp_warmboot_status_t app_macsec_write(bcm_plp_sec_phy_access_t *pa,
                                           const unsigned int area_id,
                                           const unsigned char *const data_p,
                                           const unsigned int byte_offset,
                                           const unsigned int byte_count);

bcm_plp_warmboot_status_t app_macsec_free(bcm_plp_sec_phy_access_t *pa,
                                          const unsigned int area_id);

bcm_plp_warmboot_status_t app_macsec_warmboot_restore(const char *chip_name, bcm_plp_sec_phy_access_t *pa, int area_id);

extern char* itoa(int num, char* str, int base);

/* Main entry to application */
int main(int argc, char *argv[])
{
    int index;
    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int macsec_side = 0;
    int lane_map_index;
    int total_lane_maps;
    int port = 0;
    unsigned int fw_ver;
    unsigned int fw_crc;
    int area_id = 0;
    int paddr, mside, lmap;
    int fret = -1;
    int device_bypass_enable = 0;
    bcm_plp_sec_phy_access_t sec_info;
    bcm_plp_mac_access_t mac_info;
    bcm_plp_access_t phy_info;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_macsec_warmboot_callbacks_t macsec_cb;

    /* Initialize structure instances to zero */
    memset(&sec_info, 0, sizeof(bcm_plp_sec_phy_access_t));
    memset(&mac_info, 0, sizeof(bcm_plp_mac_access_t));
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type, 0, sizeof(bcm_plp_firmware_load_type_t));

    phy_info.platform_ctxt = &p_ctxt;

    /* -------------------------------------------------------------------------------- *
     * Connect to the board                                                             *
     * -------------------------------------------------------------------------------- */
    retval = device_open();
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to connect to the board (ret = %d)!\n", retval);
        return retval;
    }

    /* -------------------------------------------------------------------------------- *
     * Section 1: PHY, MACsec Initialization and MACsec Configuration                   *
     * -------------------------------------------------------------------------------- */

    /* -------------------------------------------------------------------------------- *
     * Initialize the PHYs and download firmware based on the test setup                *
     * -------------------------------------------------------------------------------- */
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;

    phy_info.if_side = 0;
    phy_info.lane_map = ALL_LANE_MAP;

    phy_info.flags = BCM_PLP_WARM_BOOT; /* Set Warmboot flag; Skip HW configuration */

    phy_info.phy_addr = PHY_ID;

    printf("Initializing PHY-%d with loading internal firmware...\n", phy_info.phy_addr);
    retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastNone);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: bcm_plp_init_fw_bcast failed to initialize PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
        goto _aperta2_init_error;
    }

    /* Read firmware info */
    retval = bcm_plp_firmware_info_get(CHIP_NAME, phy_info, &fw_ver, &fw_crc);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to get the firmware info for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
        goto _aperta2_init_error;
    } else {
        printf("Firmware info for PHY-%d: FW version 0x%x, FW CRC 0x%x\n", phy_info.phy_addr, fw_ver, fw_crc);
    }
    phy_info.flags = BCM_PLP_WARM_BOOT; /* Set Warmboot flag; Skip HW configuration */
    /* -------------------------------------------------------------------------------- *
     * Initialize MACsec CfyE and SecY                                                  *
     * -------------------------------------------------------------------------------- */
    phy_info.lane_map = OCTAL0_LANE_MAP;
    device_bypass_enable = 0;
    phy_info.phy_addr = PHY_ID;
    memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    for (macsec_side = 0; macsec_side < 2; macsec_side++) {
        sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
        /* device_bypass_enable is set to 0, to trigger macsec transformation & validation */
        retval = macsec_ipsec_initialize(sec_info, device_bypass_enable);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: MACSec Initialize for PHY-ID[%d], macsec_side[%d], lane_map [0x%x] return code [%d]\n",
                    phy_info.phy_addr, sec_info.macsec_side, phy_info.lane_map, retval);
            goto _aperta2_macsec_init_error;
        } else {
            printf("PASS: MACSec Initialize for PHY-ID[%d], macsec_side [%d] lane_map [0x%x] \n",
                    phy_info.phy_addr, sec_info.macsec_side, phy_info.lane_map);
        }
    }
    phy_info.lane_map = OCTAL1_LANE_MAP;
    device_bypass_enable = 0;
    phy_info.phy_addr = PHY_ID;
    memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    for (macsec_side = 0; macsec_side < 2; macsec_side++) {
        sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
        /* device_bypass_enable is set to 0, to trigger macsec transformation & validation */
        retval = macsec_ipsec_initialize(sec_info, device_bypass_enable);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: MACSec Initialize for PHY-ID[%d], macsec_side[%d], lane_map [0x%x] return code [%d]\n",
                    phy_info.phy_addr, sec_info.macsec_side, phy_info.lane_map, retval);
            goto _aperta2_macsec_init_error;
        } else {
            printf("PASS: MACSec Initialize for PHY-ID[%d], macsec_side [%d] lane_map [0x%x] \n",
                    phy_info.phy_addr, sec_info.macsec_side, phy_info.lane_map);
        }
    }
    /* -------------------------------------------------------------------------------- *
     * Register Warmboot Callbacks for Macsec                                           *
     * -------------------------------------------------------------------------------- */

    macsec_cb.alloc_cb = (bcm_plp_warmboot_alloc_callback_t)app_macsec_alloc;
    macsec_cb.free_cb = (bcm_plp_warmboot_free_callback_t)app_macsec_free;
    macsec_cb.read_cb = (bcm_plp_warmboot_read_callback_t)app_macsec_read;
    macsec_cb.write_cb = (bcm_plp_warmboot_write_callback_t)app_macsec_write;

    phy_info.lane_map = OCTAL0_LANE_MAP;
    phy_info.phy_addr = PHY_ID;
    memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    sec_info.macsec_side = 0; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
    retval = bcm_plp_macsec_warmboot_register(CHIP_NAME, &sec_info, &macsec_cb);
    if (retval) {
        printf("FAIL: bcm_plp_macsec_warmboot_register (ret = %d)!\n", retval);
        goto _aperta2_init_error;
    } else {
        printf("Success : bcm_plp_macsec_warmboot_register\n");
    }

    /*
     * Initialize the Persistent memory managing meta data to -1
     */
    for (index = 0; index < APP_MACSEC_WARMBOOT_NOF_AREAS; index++)
        fd[index] = NULL;

    /*
     * Open the meta file to read Area-ID to PHY/Ingress/Egress/Port mapping
     * For restoring Device specific Warmboot Areas
     */
    mfptr = fopen(WARMBOOT_MASTER_FILE, "r+");
    if (!mfptr) {
        printf("\n could not open %s\n", WARMBOOT_MASTER_FILE);
        goto _aperta2_init_error;
    }

    fret = fscanf(mfptr, "%1d\n%1d\n%1d\n%20x\n", &area_id, &paddr, &mside, &lmap);
    while (fret != EOF) {
        phy_info.lane_map = lmap;
        phy_info.phy_addr = paddr;
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        sec_info.macsec_side = mside;
        printf("\nStarting warmboot_restore for area:%d\n", area_id);
        retval = app_macsec_warmboot_restore(CHIP_NAME, &sec_info, area_id);
        if (retval) {
            printf("FAIL: app_macsec_warmboot_restore for PHY-%d (ret = %d) area:%d !\n", phy_info.phy_addr, retval, area_id);
            goto _aperta2_init_error;
        } else {
            printf("Success : app_macsec_warmboot_restore for PHY-%d: area:%d\n", phy_info.phy_addr, area_id);
        }
        fret = fscanf(mfptr, "%1d\n%1d\n%1d\n%20x\n", &area_id, &paddr, &mside, &lmap);
    }

    if (mfptr)
        fclose(mfptr);

    total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);
    /* -------------------------------------------------------------------------------- *
     * Restore Warmboot Config                                                          *
     * -------------------------------------------------------------------------------- */
    bcm_plp_secy_sa_handle_t nh, ch;
    bcm_plp_cfye_vport_handle_t cvh, nvh;
    bcm_plp_cfye_rule_handle_t crh, nrh;

    phy_info.phy_addr = PHY_ID;
    for (macsec_side = 0; macsec_side < 2; macsec_side++) {
        /* Filling phy_info */
        phy_info.if_side = LINE_SIDE;

        /*++++++++++++++++++++++++++++++++++++++++++++
         * warmboot restore vPort
         *+++++++++++++++++++++++++++++++++++++++++++++*/
        crh = NULL;
        cvh = NULL;
        nh.p = NULL; ch.p = NULL;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            phy_info.lane_map = line_lane_map_list[lane_map_index];
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            port = lanemap_to_portindex(sec_info.phy_info.lane_map);
            sec_info.macsec_side = macsec_side;
            retval = bcm_plp_cfye_vport_next_get(CHIP_NAME, &sec_info, cvh, &nvh);
            if(retval) {
                printf("FAIL: bcm_plp_cfye_vport_next_get API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                goto _aperta2_config_error;
            } else {
                printf("PASS : bcm_plp_cfye_vport_next_get API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
            }
            warm_vport_handle[port][macsec_side] = cvh = nvh;
            printf("\n Restored vport handle for port:%d side:%d is %p\n",port, macsec_side, cvh);
            if(warm_vport_handle[port][macsec_side]){
                warm_vport_ids[port][macsec_side] = bcm_plp_cfye_vport_id_get(CHIP_NAME, &sec_info, warm_vport_handle[port][macsec_side]);
                printf("PASS: Successfully got vPort ID = %d for lane-map 0x%x of PHY-%d in %s path!\n",
                        warm_vport_ids[port][macsec_side], sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
            /*++++++++++++++++++++++++++++++++++++++++++++
             * warmboot restore sa with transform record
             *+++++++++++++++++++++++++++++++++++++++++++++*/
            retval = bcm_plp_secy_sa_next_get(CHIP_NAME, &sec_info, ch, &nh);
            if(retval) {
                printf("FAIL: bcm_plp_secy_sa_next_get API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                goto _aperta2_config_error;
            } else {
                printf("PASS : bcm_plp_secy_sa_next_get API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
            }
            warm_secy_sahandle[port][macsec_side].p = ch.p = nh.p;
            printf("\n Restored SA handle for port:%d side:%d is %p\n",port, macsec_side, ch.p);
            retval = bcm_plp_secy_diag_sa_dump(CHIP_NAME, &sec_info, warm_secy_sahandle[port][macsec_side], 1);
            if(retval) {
                printf("FAIL: bcm_plp_secy_diag_sa_dump API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                goto _aperta2_config_error;
            } else {
                printf("PASS : bcm_plp_secy_diag_sa_dump API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
             * warmboot restore rule policy
             *+++++++++++++++++++++++++++++++++++++++++++++*/
            retval = bcm_plp_cfye_rule_next_get(CHIP_NAME, &sec_info, crh, &nrh);
            if(retval) {
                printf("bcm_plp_cfye_rule_next_get API FAILED for PHY_ID[%d], macsec_side[%d], ALL_LANE_MAP[0x%x], return code =[%d] \n",
                        sec_info.phy_info.phy_addr, macsec_side, phy_info.lane_map, retval);
                goto _aperta2_config_error;
            }
            printf("bcm_plp_cfye_rule_next_get API successful for PHY_ID[%d], macsec_side[%d], ALL_LANE_MAP[0x%x], return code =[%d] \n",
                    sec_info.phy_info.phy_addr, macsec_side, phy_info.lane_map, retval);
            warm_rule_handle[port][macsec_side] = crh = nrh;
            printf("\n Restored Rule handle for port:%d side:%d is %p\n",port, macsec_side, crh);
        }
    }
    /* -------------------------------------------------------------------------------- *
     * Dump all rules configured for both Octals                                        * 
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    phy_info.lane_map = OCTAL0_LANE_MAP;
    for (macsec_side = 0; macsec_side < 2; macsec_side++) {
        sec_info.macsec_side = macsec_side;
        phy_info.if_side = macsec_side ? BCM_LINE_SIDE : BCM_SYSTEM_SIDE; /* Mandatory field in case of Octal Xing*/
        phy_info.lane_map = OCTAL0_LANE_MAP;                              /* Mandatory field in case of Octal Xing*/
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t)); 
        retval = bcm_plp_cfye_diag_rule_dump(CHIP_NAME, &sec_info, NULL, 1);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to dump rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta2_config_error;
        } else {
            printf("PASS: Successfully dump rule for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
    } 
    phy_info.phy_addr = PHY_ID;
    phy_info.lane_map = OCTAL1_LANE_MAP;
    for (macsec_side = 0; macsec_side < 2; macsec_side++) {
        sec_info.macsec_side = macsec_side;
        phy_info.if_side = macsec_side ? BCM_LINE_SIDE : BCM_SYSTEM_SIDE; /* Mandatory field in case of Octal Xing*/
        phy_info.lane_map = OCTAL1_LANE_MAP;                              /* Mandatory field in case of Octal Xing*/
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t)); 
        retval = bcm_plp_cfye_diag_rule_dump(CHIP_NAME, &sec_info, NULL, 1);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to dump rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta2_config_error;
        } else {
            printf("PASS: Successfully dump rule for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
    } 


    /* -------------------------------------------------------------------------------- *
     * Ports are setup now and ready to test with traffic with dm=00:00:00:08:01:01     *
     * Macsec Packets will be allowed
     * -------------------------------------------------------------------------------- */

_aperta2_config_error:
_aperta2_macsec_init_error:
    /* -------------------------------------------------------------------------------- *
     * Do Not Uninitialize MACsec CfyE and SecY if you Warmboot later                   *
     * -------------------------------------------------------------------------------- */

_aperta2_init_error:
    /* Check for test result */
    if (retval == TEST_SUCCESS) {
        printf("Test for %s mode completed successfully\n", argv[0]);
    } else {
        printf("Test for %s mode failed!\n", argv[0]);
    }

    /* -------------------------------------------------------------------------------- *
     * Close the connections to the board                                               *
     * -------------------------------------------------------------------------------- */
    device_close();

    /* Return with test status */
    return retval;
}


/*
 * Sample alloc function creates files to persist macsec config to be saved
 * during a cold boot and restored during a warmboot
 */
bcm_plp_warmboot_status_t app_macsec_alloc(const bcm_plp_sec_phy_access_t *pa,
                                           const unsigned int storage_byte_count,
                                           unsigned int *const area_id_p)
{
    unsigned int i;
    char *data_p;

    for (i=0; i<APP_MACSEC_WARMBOOT_NOF_AREAS; i++) {
        if (fd[i] == NULL) {
            char buf[4] = {'\0'};
            char filename[FILENAME_MAX_LENGTH] = {'\0'};

            sprintf(filename, "%s%s", WARMBOOT_FILE_PREFIX, itoa(i,buf,10));
            fd[i] = fopen(filename, "r+");

            if (fd[i] == NULL) {
                /* Allocation failed. */
                printf("\n open of file %s failed with error \n", filename);
                perror("Error : ");
                return BCM_PLP_WARMBOOT_ERROR_ALLOCATION;
            } else {
                *area_id_p = i;
                /* Initialized the memory with default 0 */
                data_p = (char *)calloc(storage_byte_count,sizeof(char));
                if(data_p == NULL){
                    printf("memory allocation failed \n");
                    return BCM_PLP_WARMBOOT_ERROR_ALLOCATION;
                }
                fwrite(data_p, 1, storage_byte_count,  fd[i]);
                /* free temporary allocated memory */
                free(data_p);
                fprintf(mfptr, "%d\n%d\n%d\n%0x\n", i, pa->phy_info.phy_addr, pa->macsec_side, pa->phy_info.lane_map);
                return BCM_PLP_WARMBOOT_STATUS_OK;
            }
        }
    }
    return BCM_PLP_WARMBOOT_INTERNAL_ERROR;
}

/*
 * Sample read callback function reads from files to restore config
 * during a warmboot
 */
bcm_plp_warmboot_status_t app_macsec_read(bcm_plp_sec_phy_access_t *pa,
                                          const unsigned int area_id,
                                          unsigned char *const data_p,
                                          const unsigned int byte_offset,
                                          const unsigned int byte_count)
{
    int rv = 0;
    char filename[FILENAME_MAX_LENGTH] = {'\0'};
    if (area_id >= APP_MACSEC_WARMBOOT_NOF_AREAS) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }
    if (fd[area_id] == NULL) {
        sprintf(filename, "%s%d", WARMBOOT_FILE_PREFIX,area_id);
        fd[area_id] = fopen(filename, "r+");
    }
    if (fd[area_id] == NULL || data_p == NULL) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }


    fseek(fd[area_id], byte_offset, SEEK_SET);

    rv = fread(data_p, 1, byte_count, fd[area_id]);
    if(rv <= 0) {
        printf("File read failed error: %d \n", rv);
        return BCM_PLP_WARMBOOT_INTERNAL_ERROR;
    }
    return BCM_PLP_WARMBOOT_STATUS_OK;
}


/*
 * Sample read callback function writes into files to restore config
 * during a warmboot
 */
bcm_plp_warmboot_status_t app_macsec_write(bcm_plp_sec_phy_access_t *pa,
                                           const unsigned int area_id,
                                           const unsigned char *const data_p,
                                           const unsigned int byte_offset,
                                           const unsigned int byte_count)
{
    if (area_id >= APP_MACSEC_WARMBOOT_NOF_AREAS) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }

    if (fd[area_id] == NULL || data_p == NULL) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }
    fseek(fd[area_id], byte_offset, SEEK_SET);

    fwrite(data_p, 1, byte_count,  fd[area_id]);

    return BCM_PLP_WARMBOOT_STATUS_OK;
}

/*
 * Sample write callback function writes to files to save config
 * before a warmboot
 */
bcm_plp_warmboot_status_t app_macsec_free(bcm_plp_sec_phy_access_t *pa,
                                          const unsigned int area_id)
{
    if (area_id >= APP_MACSEC_WARMBOOT_NOF_AREAS) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }

    if (fd[area_id] == NULL) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }
    fflush(fd[area_id]);

    fclose(fd[area_id]);
    fd[area_id] = NULL;
    return BCM_PLP_WARMBOOT_STATUS_OK;
}


bcm_plp_warmboot_status_t app_macsec_warmboot_restore(const char *chip_name, bcm_plp_sec_phy_access_t *pa, int area_id)
{
    bcm_plp_warmboot_status_t rc;

    rc = bcm_plp_macsec_warmboot_restore (chip_name, pa, area_id);
    if (rc != BCM_PLP_WARMBOOT_STATUS_OK) {
        printf("WarmBoot_restore failed for area-id[%d]\n", area_id);
        return BCM_PLP_WARMBOOT_INTERNAL_ERROR;
    } else
        printf("WarmBoot_restore passed for area-id[%d]\n", area_id);

    return BCM_PLP_WARMBOOT_STATUS_OK;
}

