 /*
 *         
 * $Id: bcm_pm_if_api.c $
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *         
 *     
 */

#include "bcm_pm_if.h"
#include "bcm_pm_if_api.h"

#ifdef BCM_PLP_MAC_SUPPORT
#include "bcm_plp_mac_api.h"
#endif

#ifdef BARCHETTA_DEBUG_SUPPORT
#include "barchetta_cfg_seq.h"
#endif

bcm_plp_phy_static_config_t bcm_plp_aperta_phy_static_config[PHYMOD_IF_CONFIG_MAX_PHYS] = {{0xff,NULL},}; 

/*! \brief Set mutex information for a PHY.
 *
 *  This API sets mutex information for a given PHY. This API enables/ensures
 *  multi-threading support for all the other APIs
 *  User needs to call this API before calling 
 *  bcm_pm_if_static_config_set and bcm_pm_if_init to enable
 *  multithreading support for all the APIs except bcm_pm_if_cleanup.
 *
 *  @param phy_info        Represents PHY access
 *  @param mutex_info      Structure that contains the function pointer
 *                         for lock and unlock mutex callbacks for a given PHY
 *
 *    @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_mutex_info_set(bcm_plp_access_t phy_info, bcm_plp_mutex_info_t *mutex_info)
{
    int rv = BCM_PM_IF_SUCCESS;

    PHYMOD_NULL_CHECK(mutex_info);
    BCM_PM_CHK_INIT_DONE(phy_info.phy_addr);

    /* Create PHY object */
    if ((plp_aperta_phy_ctrl.phy[phy_info.phy_addr] == NULL) || (plp_aperta_phy_ctrl.phy[phy_info.phy_addr]->valid != 1)) { 
        rv = _bcm_plp_aperta_if_phymod_phy_create(&plp_aperta_phy_ctrl.phy[phy_info.phy_addr]);
        if (rv != 0) {
            return rv;
        }
    }

    _plp_aperta_phyid_list[phy_info.phy_addr].phy_id = phy_info.phy_addr; 
    plp_aperta_phy_ctrl.phy[phy_info.phy_addr]->mutex_info.mutex_give = mutex_info->mutex_give;
    plp_aperta_phy_ctrl.phy[phy_info.phy_addr]->mutex_info.mutex_take = mutex_info->mutex_take;

    return rv; 
}

/*! \brief Static configurations.
 *
 *  This API initializes the software database with static configurations
 *  specified by the user for the specified PHY ID. Static configurations
 *  are one time configurations. User needs to call this api before
 *  calling bcm_pm_if_init. But user can still skip calling this function,
 *      In that case default configurations are applied to the device.
 *
 *  @param phy_info  <pre>Represents PHY access\n
 *  @param bcm_static_config  static config structure.</pre> 
 *
 *  @return SUCCESS
 */
int
bcm_plp_aperta_static_config_set(bcm_plp_access_t phy_info, void* bcm_static_config)
{
    unsigned int glb_arr_idx = 0;
    unsigned int index = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    for(glb_arr_idx = 0; glb_arr_idx < PHYMOD_IF_CONFIG_MAX_PHYS; glb_arr_idx++) {
        if(bcm_plp_aperta_phy_static_config[glb_arr_idx].phy_id == 0xff) {
            if(glb_arr_idx == 0) {
                /* Initialize all entries from index 1 to 1023 */
                for (index = 1; index < PHYMOD_IF_CONFIG_MAX_PHYS; index ++) {
                    if(bcm_plp_aperta_phy_static_config[index].bcm_static_config == NULL) {
                        bcm_plp_aperta_phy_static_config[index].phy_id = 0xff;
                    }
                }    
            }
            if(bcm_static_config) {
                bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config = PHYMOD_IF_MALLOC(sizeof(plp_static_config_t));
                if(bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config == NULL) {
                    BCM_PM_RETURN_WITH_ERR(BCM_PM_IF_MEMORY, (_PHYMOD_MSG("Null parameter")));
                }
                PHYMOD_IF_MEMSET(bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config, 0, sizeof(plp_static_config_t));
            } 
            bcm_plp_aperta_phy_static_config[glb_arr_idx].phy_id = phy_info.phy_addr;
            if(bcm_static_config) {
                PHYMOD_IF_MEMCPY(bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config, bcm_static_config, sizeof(plp_static_config_t));
            } 
            break;
        } else if (bcm_plp_aperta_phy_static_config[glb_arr_idx].phy_id == phy_info.phy_addr) {
            if(bcm_static_config) {
                PHYMOD_IF_MEMCPY(bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config, bcm_static_config, sizeof(plp_static_config_t));
            } 
            break;
        }
    }
    if(glb_arr_idx >= PHYMOD_IF_CONFIG_MAX_PHYS) {
        PHYMOD_DEBUG_ERROR(("Static config array index reached MAX PHY number\n"));
        rv = BCM_PM_IF_INTERNAL;
        goto ERR;
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! brief static configurations
 *
 *  This API retrieves static configurations from software database
 *  for the PHY ID specified by the user.
 *  If no configurations specified by user through bcm_pm_if_static_config_set,
 *      this API returns default configurations.
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param bcm_static_config  static config structure. </pre>
 *
 *
 *  @return SUCCESS
 */
int
bcm_plp_aperta_static_config_get(bcm_plp_access_t phy_info, void* bcm_static_config)
{
    unsigned int glb_arr_idx = 0;
    int rv = BCM_PM_IF_SUCCESS;

    PHYMOD_NULL_CHECK(bcm_static_config);
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    PHYMOD_IF_MEMSET(bcm_static_config, 0, sizeof(plp_static_config_t));

    for(glb_arr_idx = 0; glb_arr_idx < PHYMOD_IF_CONFIG_MAX_PHYS; glb_arr_idx++) {
        if(phy_info.phy_addr == bcm_plp_aperta_phy_static_config[glb_arr_idx].phy_id) {
            if (bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config) {
                PHYMOD_IF_MEMCPY(bcm_static_config, bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config, sizeof(plp_static_config_t));
            } else {
                PHYMOD_IF_MEMSET(bcm_static_config, 0, sizeof(plp_static_config_t));
            }
            break;
        }
    }
    if(glb_arr_idx >= PHYMOD_IF_CONFIG_MAX_PHYS) {
        PHYMOD_IF_MEMSET(bcm_static_config, 0, sizeof(plp_static_config_t));
        PHYMOD_DEBUG_ERROR(("Static config array index reached MAX PHY number\n"));
        rv = BCM_PM_IF_SUCCESS;
        goto ERR;
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}


/*! \brief Initialize PHY.
 *
 *	This API initialize the specified PHYID by creating software database and 
 *	downloading firmware for the specified PHYID, This API needs to be called for each PHYID.
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param read	                 User defined Function pointer for reading register.
 *  @param write                 User defined Function pointer for writing register.
 *  @param firmware_load_method  Represents Firmware download method to be followed 
 *                               during initialization.<br>
 *                               0 - Don't download the firmware (*)<br>
 *                               1 - FW download through MDIO 
 *                               2 - Load FW by a given function, This is for future use (*)<br>
 *                               3 - Load FW through MDIO and flash it on to EEPROM (*)<br>
 *                               (*) means not supported by all PHYs</pre>
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_init(bcm_plp_access_t phy_info,   
               int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val), 
               int (*write)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int val), 
               bcm_pm_firmware_load_method_t firmware_load_method)
{
    plp_aperta_phymod_bus_t core_bus;
    plp_aperta_phymod_core_access_t *pm_core;
    plp_aperta_phymod_phy_access_t *pm_phy;
    unsigned int id0 = 0, id1 = 0;
    _bcm_if_phymod_core_t core_probe;
    _bcm_if_phymod_core_t *core;
    _bcm_if_phymod_phy_t *phy = NULL;
    plp_aperta_phymod_access_t *pm_acc = NULL;
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t chip_found = 0, is_exist_phy = FALSE;
#ifdef BCM_PLP_BASE_T_PHY
    int lane_map = phy_info.lane_map;
#else
    int lane_map = 0xF; /*By default it always 40G*/
#endif
    plp_aperta_phymod_dispatch_type_t phy_type = 0xFFFF;
    bcm_if_firmare_load_type_t firmware_load_type = bcm_if_load_type_unicast;
    uint32_t phy_id_idx = 0;
    unsigned int glb_arr_idx = 0; 

#if defined(PHYMOD_MADURA_SUPPORT) || defined (PHYMOD_HURACAN_SUPPORT)
    char *warm_boot_env = NULL;
    int warm_boot_enable = 0;
#endif
    plp_aperta_phymod_diag_print_func = printf ;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    _bcm_plp_aperta_pm_if_get_phy_id_idx(phy_info.phy_addr, &phy_id_idx, &is_exist_phy);
    if (is_exist_phy == 1) {
        PHYMOD_DEBUG_ERROR(("Existing PHY\n"));
        rv = BCM_PM_IF_PHY_EXISTING; 
        goto ERR;
    }
	/* coverity[cond_const] */
    if (phy_id_idx == BCM_PM_IF_MAX_PHY) {
        PHYMOD_DEBUG_ERROR(("MAX PHY reached\n"));
        rv = BCM_PM_IF_INTERNAL; 
        goto ERR;
    }

    BCM_PM_IF_ERR_RETURN(
           read(phy_info.platform_ctxt, phy_info.phy_addr, 0x10002, &id0));
    BCM_PM_IF_ERR_RETURN(
           read(phy_info.platform_ctxt, phy_info.phy_addr, 0x10003, &id1));

#ifdef PHYMOD_FURIA_SUPPORT
    if (id0 == 0xae02 && id1 == 0x5230) {
        phy_type = phymodDispatchTypeFuria;
    }
#endif 
#ifdef PHYMOD_SESTO_SUPPORT
    if (id0 == 0xae02 && id1 == 0x5290) {
        phy_type = phymodDispatchTypeSesto;
    }
#endif
#ifdef PHYMOD_MADURA_SUPPORT
    if (id0 == 0xae02 && (( id1 == 0x52b0) || ( id1 == 0x5290 ))) {
        phy_type = phymodDispatchTypeMadura;
	    firmware_load_type = bcm_if_load_type_broadcast;
    }
#endif
#ifdef PHYMOD_DINO_SUPPORT
    BCM_PM_IF_ERR_RETURN(
        read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b00, &id0));

    PHYMOD_DEBUG_INFO(("DINO chip-ID LSB:%x for PHY:%x\n", id0, phy_info.phy_addr));

    BCM_PM_IF_ERR_RETURN(
        read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b01, &id1));

    PHYMOD_DEBUG_INFO(("DINO chip ID MSB and REV:0x%x\n", id1));
    if (id0 == 0x2332 || id0 == 0x2793 || id0 == 0x2795) {
        phy_type = phymodDispatchTypeDino;
        if (firmware_load_method != bcmpmFirmwareLoadMethodNone) {
            firmware_load_type = bcm_if_load_type_broadcast;
        }
    }
#endif
#ifdef PHYMOD_KOI_SUPPORT       /* 84858/58R/56R */
    if ( (id0 == 0x600d) && ((id1 == 0x8562) || (id1 == 0x8552)) ) {
        PHYMOD_DEBUG_INFO(("KOI_SUPPORT\n"));
        phy_type = phymodDispatchTypePhy848xx;
    }
#endif

/* ID1 Mapping for ORCA
 * PHY           A0       B0
 * BCM84880    0x5158    0x5159
 * BCM84881    0x5150    0x5151
 * BCM84884    0x5148    0x5149
 * BCM84884E   0x5168    0x5169
 * BCM84885    0x5178    0x5179
 * BCM84886    0x5170    0x5171
 * BCM84887    0x5144    0x5145
 * BCM84888    0x5140    0x5141
 * BCM84888E   0x5160    0x5161
 * Note: Orca chip family A0 parts listed above are not supported
 */
#ifdef PHYMOD_ORCA_SUPPORT      /* 8488x */
    if ( (id0 == 0xae02) && ((id1 >= 0x5140) && (id1 < 0x5180)) ) {
        PHYMOD_DEBUG_INFO(("ORCA_SUPPORT\n"));
        phy_type = phymodDispatchTypePhy848xx;
    }
#endif

/* ID1 Mapping for SHORTFIN
 * Part       A0     B0
 * BCM84898   0x5000 0x5001 
 * BCM84896   0x5004 0x5005
 * BCM50998S         0x5009
 * BCM54998S  0x5008 0x5009
 * BCM50998ES        0x500D
 * BCM54998ES 0x500C 0x500D
 * BCM54998   0x5010 0x5011
 * BCM54998E  0x5014 0x5015
 * BCM54994EL        0x501D
 * BCM50998E         0x501D
 * Note: Shortfin chip family A0 parts listed above are not supported
 */
#ifdef PHYMOD_SHORTFIN_SUPPORT  /* 8489x/5499x */
    if ( (id0 == 0x3590) &&
        ((id1 == 0x5001) || (id1 == 0x5005) || (id1 == 0x5009) ||
         (id1 == 0x500D) || (id1 == 0x5011) || (id1 == 0x5015) ||
                            (id1 == 0x5019) || (id1 == 0x501D)  ) ) {
        PHYMOD_DEBUG_INFO(("SHORTFIN_SUPPORT\n"));
        phy_type = phymodDispatchTypeXgbaset;
    }
#endif

/* ID1 Mapping for BLACKFIN
 * Part       A0       B0
 * BCM84891   0x5090   0x5091 
 * BCM54991   0x5094   0x5095
 * BCM54991E  0x5098   0x5099
 * BCM84891L  0x5080   0x5081
 * BCM50991L  0x5084   0x5085
 * BCM54991L  0x5084   0x5085
 * BCM54991EL 0x5088   0x5089
 * BCM84892   0x50A0   0x50A1
 * BCM54992   0x50A4   0x50A5
 * BCM54992E  0x50A8   0x50A9
 * BCM84894   0x50B0   0x50B1
 * BCM54994   0x50B4   0x50B5
 * BCM54994E  0x50B8   0x50B9
 * BCM54991H  0x50D1
 * BCM54994H  0x50F1
 * Note: Blackfin chip family A0 parts listed above are not supported
 */
#ifdef PHYMOD_BLACKFIN_SUPPORT  /* 8489x/5499x */
    if ( (id0 == 0x3590) && ((id1 >= 0x5080) && (id1 <= 0x50ff)) ) {
        PHYMOD_DEBUG_INFO(("BLACKFIN_SUPPORT\n"));
        phy_type = phymodDispatchTypeXgbaset;
    }
#endif

/* ID1 Mapping for LONGFIN
 * Part         A0       B0  
 * BCM84891LM   0x5180   0x5181  
 * BCM54991LM   0x5184   0x5185  
 * BCM54991ELM  0x5188    
 * BCM84891M    0x5190   0x5191
 * BCM54991M    0x5194   0x5195  
 * BCM54991EM   0x5198   0x5199  
 * BCM84892M    0x51A0   0x51A1  
 * BCM54992M    0x51A4   0x51A5  
 * BCM54992EM   0x51A8   0x51A9  
 * BCM84894M    0x51B0   0x51B1  
 * BCM54994M    0x51B4   0x51B5
 * BCM54994EM   0x51B8   0x51B9
 * BCM54991H    0x51D0   
 * BCM54994H    0x51F0   
 * BCM50991ELM           0x518D
 * BCM54991SK            0x51D5
 * BCM5494SK             0x51F5
 * BCM54991H             0x51D0
 * BCM54994H             0x51F0
 */
#ifdef PHYMOD_LONGFIN_SUPPORT   /* 8489xM/5499xM */
    if ( (id0 == 0x3590) && ((id1 >= 0x5180) && (id1 <= 0x51ff)) ) {
        PHYMOD_DEBUG_INFO(("LONGFIN_SUPPORT\n"));
        phy_type = phymodDispatchTypeXgbaset;
    }
#endif

/* ID1 Mapping for SEAHAWKS (COMBO means copper+fiber)
 * Part         A0       B0    ---I/F-----#Port---Media--
 * BCM54190 - 0x5100  0x5101     SGMII    Octal   copper
 * BCM54192 - 0x5110  0x5111    QSGMII    Octal   copper
 * BCM54194 - 0x5108  0x5109     SGMII    Quad    COMBO
 * BCM54195 - 0x5118  0x5119    QSGMII    Octal   COMBO
 *
 * Note: Seahawks chip family A0 parts listed above are not supported
 */
#ifdef PHYMOD_SEAHAWKS_SUPPORT
    if ( (id0 == 0xae02) && ((id1 >= 0x5100) && (id1 <= 0x511f)) ) {
        PHYMOD_DEBUG_INFO(("SEAHAWKS_SUPPORT\n"));
        phy_type = phymodDispatchTypePhy542xx;
    }
#endif

/* ID1 Mapping for BRONCOS     ---I/F-----#Port---Media--
 * BCM54180/B501800 - 0x5001     SGMII    Octal   copper
 * BCM54140/B501400 - 0x5009     SGMII    Quad    COMBO
 * BCM54182/B501820 - 0x5011    QSGMII    Ocatl   copper
 * BCM54185/B501850 - 0x5019    QSGMII    Octal   COMBO
 */
#ifdef PHYMOD_BRONCOS_SUPPORT
    if ( (id0 == 0xae02) && ((id1 == 0x5001) || (id1 == 0x5002) ||
                             (id1 == 0x5009) || (id1 == 0x500a) ||
                             (id1 == 0x5011) || (id1 == 0x5012) ||
                             (id1 == 0x5019) || (id1 == 0x501a)  ) ) {
        PHYMOD_DEBUG_INFO(("BRONCOS_SUPPORT\n"));
        phy_type = phymodDispatchTypePhy542xx;
    }
#endif

/* ID1 Mapping for RAVENS 
 * BCM54290 - 0x8529
 * BCM54291 - 0x8531
 * BCM54292 - 0x8531
 * BCM54293 - 0x8531 
 * BCM54294 - 0x8529
 * BCM54295 - 0x8531
 * BCM54296 - 0x8531
 * B50290   - 0x8529
 * B50291   - 0x8531
 * B50292   - 0x8531
 * B50294   - 0x8529
 * B50295   - 0x8531
 * B50296   - 0x8531
 */

#ifdef PHYMOD_RAVENS_SUPPORT
    if ((id0 == 0x600d) && ((id1 == 0x8521) || (id1 == 0x8529) ||
                            (id1 == 0x8531) || (id1 == 0x8539) || (id1 == 0x8538) )) {
        PHYMOD_DEBUG_INFO(("RAVENS_SUPPORT\n"));
        phy_type = phymodDispatchTypePhy542xx;
    }
#endif
 
/* ID1 Mapping for GIANTS 
 * BCM54280/B50280 - 0x8443
 * BCM54282/B50282 - 0x845b
 * BCM54285/B50285 - 0x8453
 * BCM54240/B50240 - 0x8463
 */
#ifdef PHYMOD_GIANTS_SUPPORT
    if ((id0 == 0x600d) && ((id1 == 0x845b) || (id1 == 0x8443) || (id1 == 0x8463) || (id1 == 0x8453) )) {
        PHYMOD_DEBUG_INFO(("GIANTS_SUPPORT\n"));
        phy_type = phymodDispatchTypePhy542xx;
    }
#endif

/* ID1 Mapping for RAMS
 * B50210     - 0x8599
 * B50210F    - 0x8595
 * B50210S    - 0x8595
 * B50220     - 0x8589
 * B50220S    - 0x8585
 * BCM54210   - 0x8599
 * BCM54210S  - 0x8595
 * BCM54219S  - 0x8595
 * BCM54210SE - 0x8591
 * BCM54219SE - 0x8591
 * BCM54220   - 0x8589
 * BCM54220S  - 0x8585
 * BCM54220SE - 0x8581
 */
#ifdef PHYMOD_RAMS_SUPPORT
    if ((id0 == 0x600d) && ((id1 >= 0x8581) && (id1 <= 0x8599))) {
        PHYMOD_DEBUG_INFO(("RAMS_SUPPORT\n"));
        phy_type = phymodDispatchTypePhy542xx;
    }
#endif
/* ID1 Mapping for COWBOYS
 * B50210E    - 0x84A6
 * B50212E    - 0x84A6
 * B50212PE   - 0x84A6
 * BCM54210E  - 0x84A2
 * BCM54210PE - 0x84A2
 * BCM54213PE - 0x84A2
 * BCM54214E  - 0x84AE
 * BCM54216E  - 0x84AA
 * BCM54216EC - 0x84AA
 */
#ifdef PHYMOD_COWBOYS_SUPPORT
    if ((id0 == 0x600d) && ((id1 >= 0x84A0) && (id1 <= 0x84AF) )) {
        PHYMOD_DEBUG_INFO(("COWBOYS_SUPPORT\n"));
        phy_type = phymodDispatchTypePhy542xx;
    }
#endif


#ifdef PHYMOD_EVORA_SUPPORT
#ifdef PHYMOD_EUROPA_SUPPORT
    BCM_PM_IF_ERR_RETURN(
           read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b00, &id0));

    PHYMOD_DEBUG_INFO(("EUROPA chip-ID LSB:%x for PHY:%x\n", id0, phy_info.phy_addr));

    BCM_PM_IF_ERR_RETURN(
    read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b01, &id1));

    PHYMOD_DEBUG_INFO(("EUROPA chip ID MSB and REV:0x%x\n", id1));
    if (id0 == 0x2396 || id0 == 0x2397 || id0 == 0x2398 || id0 == 0x2399) {
        phy_type = phymodDispatchTypeEvora;
        if (firmware_load_method != bcmpmFirmwareLoadMethodNone) {
            /* The reason for selecting broadcast fw load is to follow broadcast fw load procedure.
               This does not mean that we are doing FW upgrade on multiple PHY chips.
               FW upgrade will happen on a Single PHY only. */
            firmware_load_type = bcm_if_load_type_broadcast;
        }
    }
#else
    BCM_PM_IF_ERR_RETURN(
           read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b00, &id0));

        PHYMOD_DEBUG_INFO(("EVORA chip-ID LSB:%x for PHY:%x\n", id0, phy_info.phy_addr));

        BCM_PM_IF_ERR_RETURN(
           read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b01, &id1));

        PHYMOD_DEBUG_INFO(("EVORA chip ID MSB and REV:0x%x\n", id1));
    if (id0 == 0x2392 || id0 == 0x2391 || id0 == 0x2394) {
        phy_type = phymodDispatchTypeEvora;
        if (firmware_load_method != bcmpmFirmwareLoadMethodNone) {
            firmware_load_type = bcm_if_load_type_broadcast;
        }
    }
#endif
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    BCM_PM_IF_ERR_RETURN(
           read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b00, &id0));
    BCM_PM_IF_ERR_RETURN(
           read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b01, &id1));

    if (id0 == 0x2756 || id0 == 0x2757 || id0 == 0x2759 || id0 == 0x2755) {
        PHYMOD_DEBUG_INFO(("bcm_plp_init is not supported for MIURA, Please use bcm_plp_init_fw_bcast\n"));
        return BCM_PM_IF_UNAVAIL; 
        /*phy_type = phymodDispatchTypeMiura;
        if (firmware_load_method != bcmpmFirmwareLoadMethodNone) {
            firmware_load_type = bcm_if_load_type_broadcast;
        }*/
    }
#endif
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PM_IF_ERR_RETURN(
           read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1e1800, &id0));

    BCM_PM_IF_ERR_RETURN(
           read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1e1801, &id1));

    if (id0 == 0x1141 && id1 == 0x8) {
        phy_type = phymodDispatchTypeEstoque;
        if (firmware_load_method != bcmpmFirmwareLoadMethodNone) {
            firmware_load_type = bcm_if_load_type_broadcast;
        }
    }
#endif

#ifdef PHYMOD_MILLENIO_SUPPORT
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1EB2E9, &id0));
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1EB2EA, &id1));

    if (id0 == 0x1358 && id1 == 0x8) {
        phy_type = phymodDispatchTypeMillenio;
        if (firmware_load_method != bcmpmFirmwareLoadMethodNone) {
            firmware_load_type = bcm_if_load_type_broadcast;
        }
    }
#endif

#ifdef PHYMOD_QUADRA28_SUPPORT
    if (id0 == 0xae02 && id1 == 0x5250) {
        PHYMOD_DEBUG_INFO(("Inside QUADRA28_SUPPORT\n"));
        phy_type = phymodDispatchTypeQuadra28;
        firmware_load_type = bcm_if_load_type_broadcast;
    }
#endif
#ifdef PHYMOD_GALLARDO28_SUPPORT
    if (id0 == 0xae02 && id1 == 0x5210) {
        PHYMOD_DEBUG_INFO(("Inside GALLARDO28_SUPPORT\n"));
        phy_type = phymodDispatchTypeGallardo28;
        firmware_load_type = bcm_if_load_type_broadcast;
    }
#endif
#ifdef PHYMOD_BARCHETTA_SUPPORT
    BCM_PM_IF_ERR_RETURN(
           read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18500, &id0));
        BCM_PM_IF_ERR_RETURN(
           read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18501, &id1));
    if (id0 == 0x1381 || id0 == 0x1337 || id0 == 0x1338 || id0 == 0x1321|| id0 == 0x1764) {
        PHYMOD_DEBUG_ERROR(("bcm_plp_init is not supported for barchetta, Please use bcm_plp_init_fw_bcast\n"));
        return BCM_PM_IF_UNAVAIL; 
        /*phy_type = phymodDispatchTypeBarchetta;
        if (firmware_load_method != bcmpmFirmwareLoadMethodNone) {
            firmware_load_type = bcm_if_load_type_broadcast;
        }*/
    }
#endif
    if (phy_type == 0xFFFF) {
        BCM_PM_IF_ERR_RETURN(
               read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18000, &id0));
        switch(id0) {
#ifdef PHYMOD_HURACAN_SUPPORT
            case 0x2108:
            case 0x2109:
            case 0x2112:
                phy_type = phymodDispatchTypeHuracan;
                break;
#endif
            case 0xFFFF:
            default:
                /* return if not Hurican */
                PHYMOD_DEBUG_ERROR(("bcm_plp_init(): Invalid PHY\n"))
                rv = BCM_PM_IF_INVALID_PHY;
                _plp_aperta_phyid_list[phy_info.phy_addr].valid = 0;
                goto ERR;
        }
    } 

    /* coverity[dead_error_begin:FALSE] */
    rv = plp_aperta_phymod_bus_t_init(&core_bus);
    if (rv != 0) {
        goto ERR;
    }
    core_bus.bus_name = "BCM-PM-IF"; 
    core_bus.read = read; 
    core_bus.write = write;
    _bcm_plp_aperta_pm_if_core_init(&core_probe, &core_bus,
                           phy_info.phy_addr, phy_info.platform_ctxt);
    pm_core = &core_probe.pm_core;
    pm_core->type = phy_type; 

    rv = plp_aperta_phymod_core_identify(pm_core, 0 , &chip_found);
    if (rv != 0) {
        PHYMOD_DEBUG_ERROR(("Core Identify error\n"));
        goto ERR;
    } 
    if (!chip_found) {
        PHYMOD_DEBUG_ERROR(("Chip not  found\n"));
        rv = BCM_PM_NOT_FOUND;
        goto ERR;
    }
    PHYMOD_CRIT_INFO(("Chip found!!! Initializing PHY....\n"));

    /* Create PHY object */

    if ((plp_aperta_phy_ctrl.phy[phy_id_idx] == NULL) || (plp_aperta_phy_ctrl.phy[phy_id_idx]->valid != 1)) { 
        rv = _bcm_plp_aperta_if_phymod_phy_create(&plp_aperta_phy_ctrl.phy[phy_id_idx]);
        if (rv != 0) {
            goto ERR;
        }
    }
    pm_phy = &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy;
    plp_aperta_phymod_phy_access_t_init(pm_phy);

    /*create associated core object */
    rv = _bcm_plp_aperta_if_phymod_core_create(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core);
    if (rv != 0) {
        goto ERR;
    }
    PHYMOD_IF_MEMSET(plp_aperta_phy_ctrl.phy[phy_id_idx]->core,0,sizeof(_bcm_if_phymod_core_t));
    core = plp_aperta_phy_ctrl.phy[phy_id_idx]->core;
    /* Initialize core object if newly created */
    if (core->ref_cnt == 0) {
        PHYMOD_IF_MEMCPY(&core->pm_bus,
                &core_bus, sizeof(core->pm_bus));
        _bcm_plp_aperta_pm_if_core_init(core, &core->pm_bus,
                           phy_info.phy_addr, phy_info.platform_ctxt);
         /* Set dispatch type */
        core->pm_core.type = phy_type;
    }
    core->ref_cnt++;

    /* Initialize phy access based on associated core */
    pm_acc = &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access;
    PHYMOD_IF_MEMCPY(pm_acc, &core->pm_core.access, sizeof(*pm_acc));
    plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.type = phy_type;
    PHYMOD_ACC_LANE_MASK(pm_acc) = lane_map;
    phy = plp_aperta_phy_ctrl.phy[phy_id_idx];
    /* coverity[dead_error_condition:FALSE] */
    if(firmware_load_type == bcm_if_load_type_broadcast) {
        /* coverity[dead_error_line] */
        if(firmware_load_method == bcmpmFirmwareLoadMethodAuto){
           PHYMOD_CORE_INIT_F_FW_AUTO_DOWNLOAD_SET(&(phy->core->init_config));
        } else {
           PHYMOD_CORE_INIT_F_FW_FORCE_DOWNLOAD_SET(&(phy->core->init_config));
        }
        PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_SET(&(phy->core->init_config)); 
    }

    for(glb_arr_idx = 0; glb_arr_idx < PHYMOD_IF_CONFIG_MAX_PHYS; glb_arr_idx++) {
        if(phy_info.phy_addr == bcm_plp_aperta_phy_static_config[glb_arr_idx].phy_id) {
            break;
        }
    }
    if(glb_arr_idx < PHYMOD_IF_CONFIG_MAX_PHYS) {
        if (bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config != NULL) {
            phy->static_config = bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config;
        } else {
            PHYMOD_DEBUG_INFO(("No static configurations available for the requested PHY\n"));
        }
    }
#if defined(PHYMOD_MADURA_SUPPORT) || defined (PHYMOD_HURACAN_SUPPORT)
    /*Read the environment variable to determine warm boot support is enabled or not */
    warm_boot_env = PHYMOD_GETENV("BCM_PLP_WARM_BOOT");
    if (warm_boot_env){ 
        warm_boot_enable = PHYMOD_ATOI(warm_boot_env);
    }
    if (warm_boot_enable) { /* Warm boot environment variable is set to 1 */
        /* Do nothing */
    } else { /* Warm boot environment variable is set to 0 */
        rv = _bcm_plp_aperta_pm_phy_init(phy, firmware_load_method, NULL);
        if (rv != 0) {
            PHYMOD_DEBUG_ERROR(("_bcm_plp_aperta_pm_phy_init rv:%d\n",rv));
            goto ERR;
        }
    }
#else 
    #ifdef BCM_PLP_WARM_BOOT_SUPPORT
        if (phy_info.flags & BCM_PLP_WARM_BOOT) { /* Warm boot flag is set */
            /* Do nothing */
        #ifdef BCM_PLP_MAC_SUPPORT
            {
                bcm_plp_mac_access_t mac_access;
                PHYMOD_MEMCPY(&mac_access.phy_info, &phy_info, sizeof(bcm_plp_access_t));
                rv = bcm_plp_aperta_warmboot_init(mac_access);
                if (rv != 0) {
                    PHYMOD_DEBUG_ERROR(("_bcm_warmboot_init rv:%d\n",rv));
                    goto ERR;
                }
            }
        #endif
        #if defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_PHY542XX_SUPPORT)
            {
                int  dummy = 0;  /* dummy value for the 2nd argument of plp_aperta_phymod_warmboot_init *\
                                 \*       since PHYMOD doesn't allow null pointer arguments  */
                core->pm_core.access.lane_mask = lane_map;
                rv = plp_aperta_phymod_warmboot_init(&(core->pm_core), &dummy);
                if ( rv != 0 ) {
                    PHYMOD_DEBUG_ERROR(("WARM BOOT : plp_aperta_phymod_warmboot_init rv:%d\n",rv));
                    goto ERR;
                }
            }
        #endif
        } else { /* Warm boot flag is cleared */
            rv = _bcm_plp_aperta_pm_phy_init(phy, firmware_load_method, NULL);
            if (rv != 0) {
                PHYMOD_DEBUG_ERROR(("_bcm_plp_aperta_pm_phy_init rv:%d\n",rv));
                goto ERR;
            }
        }
    #else /* Cold boot sequence*/ 
        rv = _bcm_plp_aperta_pm_phy_init(phy, firmware_load_method, NULL);
        if (rv != 0) {
            PHYMOD_DEBUG_ERROR(("_bcm_plp_aperta_pm_phy_init rv:%d\n",rv));
            goto ERR;
        }
    #endif
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
/*! \brief Initialize PHY.
 *
 *  This API initialize the specified PHYID by creating software database and
 *  downloading firmware for the specified PHYID, This API needs to be called for each PHYID.
 *
 *
 *  @param phy_info      <pre> Represent PHY access \n
 *  @param read                  User defined Function pointer for reading register.
 *  @param write                 User defined Function pointer for writing register.
 *  @param firmware_load_type  Represents Firmware download method and force download option to be followed
 *                               during initialization. \n
 *                              firmware_load_type.firmware_load_method
 *                              0 - bcmpmFirmwareLoadMethodNone
 *                              1 - bcmpmFirmwareLoadMethodInternal
 *                              2 - bcmpmFirmwareLoadMethodExternal
 *                              3 - bcmpmFirmwareLoadMethodProgEEPROM 
 *                              4 - bcmpmFirmwareLoadMethodAuto
 *                              5 - bcmpmFirmwareLoadMethodExternalNVM  \n
 *                              firmware_load_type.force_load_method
 *                              0 - bcmpmFirmwareLoadSkip
 *                              1 - bcmpmFirmwareLoadForce
 *                              2 - bcmpmFirmwareLoadAuto \n
 *                              3 - bcmpmFirmwareLoadEEPROMUpgrade \n
 *  @param broadcast_method  Represents firmware broadcast sequence to be followed in subsequent call to this function \n
 *                              0 - bcmpmFirmwareBroadcastNone                Firmware downloaded as unicast for each phy device in mdio bus\n
 *                              1 - bcmpmFirmwareBroadcastCoreReset           Reset the core for all phy id in mdio bus \n
 *                              2 - bcmpmFirmwareBroadcastEnable              Enable the broadcast for all phy id in mdio bus \n
 *                              3 - bcmpmFirmwareBroadcastFirmwareExecute     Load the FW for only one phy_id, internally it will broadcast firmware
 *                                                                              of similar type of phys on same mdio bus \n
 *                              4 - bcmpmFirmwareBroadcastFirmwareVerify      Disable the broadcast and FW load verify for all phy id in mdio bus \n 
 *                              5 - bcmpmFirmeareBroadcastEnd                 End of the broadcast sequence for all phy id in mdio bus </pre> \n
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_init_fw_bcast(bcm_plp_access_t phy_info,
                  int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val),
                  int (*write)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int val),
                  bcm_plp_firmware_load_type_t *firmware_load_type,
                  bcm_pm_firmware_broadcast_method_t broadcast_method)
{
    plp_aperta_phymod_bus_t core_bus;
    plp_aperta_phymod_core_access_t *pm_core;
    plp_aperta_phymod_phy_access_t *pm_phy;
    _bcm_if_phymod_core_t core_probe;
    _bcm_if_phymod_core_t *core = NULL;
    _bcm_if_phymod_phy_t *phy = NULL;
    plp_aperta_phymod_access_t *pm_acc = NULL;
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t chip_found = 0, is_exist_phy;
    int lane_map = 0xF; /*By default it always 40G*/
    plp_aperta_phymod_dispatch_type_t phy_type = 0xFFFF;
    uint32_t phy_id_idx = 0;
    unsigned int glb_arr_idx = 0;
#if defined(PHYMOD_MADURA_SUPPORT) || defined (PHYMOD_HURACAN_SUPPORT)
    char *warm_boot_env = NULL;
    int warm_boot_enable = 0;
#endif
#if defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_PHY848XX_SUPPORT)
    uint32_t  firmware_load_master_port = 0;
  #if defined(PHYMOD_XGBASET_SUPPORT)
    uint32_t  phy_addr_combo = 0;
  #endif
#endif
    /* - - end of variable declarations - - - - - - */

#if defined(PHYMOD_XGBASET_SUPPORT)
    firmware_load_master_port = BCM_PHY_ADDR_LOGICAL_MAX;
    phy_addr_combo = phy_info.phy_addr;   /* physical & logical PHY addresses */
    phy_info.phy_addr &= BCM_PHY_ADDR_LOGICAL_MASK; /* LSB 10 bits: logical PHY address */
#endif

    plp_aperta_phymod_diag_print_func = printf;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);

#if defined(PHYMOD_XGBASET_SUPPORT)   /* Whitetip/Broadfin/Longfin/Shortfin/Blackfin */
        if (        bcmpmFirmwareBroadcastCoreReset == broadcast_method ) {  /* 1st step of init */
            _bcm_pm_if_init_phy_id_idx(phy_info.phy_addr);      /* init the _plp_aperta_phyid_list[] entry  */
            if ( BCM_PHY_IS_MASTER_PORT(phy_addr_combo) ) {       /* MASTER port in 1st init step  */
                /* record MASTER port's logical PHY address */
                firmware_load_master_port = BCM_PHY_ADDR_LOGICAL(phy_addr_combo);
                /* mark MASTER flag for the _plp_aperta_phyid_list[] entry */
                phy_addr_combo |=  BCM_PHY_MASTER_PORT;
            } else {    /* not a MASTER */
                firmware_load_master_port = BCM_PHY_ADDR_LOGICAL_MAX;
                phy_addr_combo &= ~BCM_PHY_MASTER_PORT;
            }
        } else if ( bcmpmFirmwareBroadcastEnable    == broadcast_method ) {  /* 2nd step of init */
            /* parse phy_addr -- bit[28:24] = MASTER port's physical PHY address */
            firmware_load_master_port = BCM_PHY_ADDR_MASTER(phy_addr_combo);
        } else if ( BCM_PHY_IS_MASTER_PORT(phy_addr_combo) )      {  /* MASTER port */
            /* record MASTER port's logical PHY address */
            firmware_load_master_port = BCM_PHY_ADDR_LOGICAL( phy_addr_combo);
        } else {    /* not a MASTER */
            firmware_load_master_port = BCM_PHY_ADDR_LOGICAL_MAX;
        }
        /* Newer xgbase-t PHYs (other than Longfin/Shortfin/Blackfin) always use PHYID combo */
        phy_addr_combo |=  BCM_PHY_PHYSICAL_ADDR_GIVEN_BY_APP;
    /* register physical/logical PHY address and MASTER info */
    _bcm_plp_aperta_pm_if_get_phy_id_idx(phy_addr_combo   , &phy_id_idx, &is_exist_phy);

#elif defined(PHYMOD_PHY848XX_SUPPORT)   /* Orca/Koi */
    /* parse the phy_addr -- msb 16 bits: MASTER port address */
    firmware_load_master_port = (phy_info.phy_addr) >> BCM_PHY_ADDR_MASTER_SHIFT;
    phy_info.phy_addr &=  ~(BCM_PHY_ADDR_MASTER_MASK);    /* lower 16 bits: phy address */
    _bcm_plp_aperta_pm_if_get_phy_id_idx(phy_info.phy_addr, &phy_id_idx, &is_exist_phy);

#else   /* HSIP and 1G PHYs */
    _bcm_plp_aperta_pm_if_get_phy_id_idx(phy_info.phy_addr, &phy_id_idx, &is_exist_phy);
#endif

    if (!is_exist_phy) {
		/* coverity[cond_const] */
        if (phy_id_idx == BCM_PM_IF_MAX_PHY) {
            PHYMOD_DEBUG_ERROR(("MAX PHY reached\n"));
            rv = BCM_PM_IF_INTERNAL;
            goto ERR;
        }
        BCM_PM_IF_ERR_RETURN(
                bcm_plp_aperta_phy_get_phy_type (phy_info, &phy_type, read, write));
        if (phy_type == 0xFFFF) {
            PHYMOD_DEBUG_ERROR(("bcm_plp_init_fw_bcast(): Invalid PHY\n"));
            rv = BCM_PM_IF_INVALID_PHY;
            _plp_aperta_phyid_list[phy_info.phy_addr].valid = 0;
            goto ERR;
        }
        /* coverity[dead_error_begin:FALSE] */
        rv = plp_aperta_phymod_bus_t_init(&core_bus);
        if (rv != 0) {
            goto ERR;
        }
        core_bus.bus_name = "BCM-PM-IF";
        core_bus.read = read;
        core_bus.write = write;
        _bcm_plp_aperta_pm_if_core_init(&core_probe, &core_bus,
                phy_info.phy_addr, phy_info.platform_ctxt);
        pm_core = &core_probe.pm_core;
        pm_core->type = phy_type;
#if defined(PHYMOD_APERTA_SUPPORT) || defined (PHYMOD_APERTA2_SUPPORT)
        if (phy_info.flags & BCM_PLP_WARM_BOOT) {
            pm_core->access.flags |= (BCM_PLP_WARM_BOOT);
        }
#endif
        rv = plp_aperta_phymod_core_identify(pm_core, 0 , &chip_found);
        if (rv != 0) {
            PHYMOD_DEBUG_ERROR(("Core Identify error\n"));
            goto ERR;
        }
#if defined(PHYMOD_APERTA_SUPPORT) || defined (PHYMOD_APERTA2_SUPPORT)
        if (phy_info.flags & BCM_PLP_WARM_BOOT) {
            pm_core->access.flags &= ~(BCM_PLP_WARM_BOOT);
        }
#endif
        if (!chip_found) {
            PHYMOD_DEBUG_ERROR(("Chip not found\n"));
            rv = BCM_PM_NOT_FOUND;
            goto ERR;
        }
        if(!(chip_found & 0x80000000)){
            PHYMOD_DEBUG_ERROR(("Broadcast FW download not supported\n"));
            rv = BCM_PM_IF_UNAVAIL;
            goto ERR;
        }
        PHYMOD_CRIT_INFO(("Chip found!!! Initializing PHY (address 0x%x) ....\n", phy_info.phy_addr));
#if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT)
        if ((firmware_load_type->force_load_method == bcmpmFirmwareLoadEEPROMUpgrade) && (!(phy_info.flags & BCM_PLP_WARM_BOOT))) {
            PHYMOD_DEBUG_ERROR(("EEPROM Firmware Upgrade supported only with WARM BOOT\n"));
            rv = BCM_PM_IF_UNAVAIL;
            goto ERR;
        }
#endif

        /* Create PHY object */
        if ((plp_aperta_phy_ctrl.phy[phy_id_idx] == NULL) || (plp_aperta_phy_ctrl.phy[phy_id_idx]->valid != 1)){
          rv = _bcm_plp_aperta_if_phymod_phy_create(&plp_aperta_phy_ctrl.phy[phy_id_idx]);
          if (rv != 0) {
              goto ERR;
          }
        }
        pm_phy = &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy;
        plp_aperta_phymod_phy_access_t_init(pm_phy);

        /*create associated core object */
        rv = _bcm_plp_aperta_if_phymod_core_create(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core);
        if (rv != 0) {
            goto ERR;
        }
        PHYMOD_IF_MEMSET(plp_aperta_phy_ctrl.phy[phy_id_idx]->core,0,sizeof(_bcm_if_phymod_core_t));
        core = plp_aperta_phy_ctrl.phy[phy_id_idx]->core;
        /* Initialize core object if newly created */
        if (core->ref_cnt == 0) {
            PHYMOD_IF_MEMCPY(&core->pm_bus,
                    &core_bus, sizeof(core->pm_bus));
            _bcm_plp_aperta_pm_if_core_init(core, &core->pm_bus,
                               phy_info.phy_addr, phy_info.platform_ctxt);
             /* Set dispatch type */
            core->pm_core.type = phy_type;
        }
        core->ref_cnt++;

        /* Initialize phy access based on associated core */

        pm_acc = &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access;
        PHYMOD_IF_MEMCPY(pm_acc, &core->pm_core.access, sizeof(*pm_acc));
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.type = phy_type;
        PHYMOD_ACC_LANE_MASK(pm_acc) = lane_map;
        phy = plp_aperta_phy_ctrl.phy[phy_id_idx];

        if (firmware_load_type->force_load_method == bcmpmFirmwareLoadAuto) {
            PHYMOD_CORE_INIT_F_FW_AUTO_DOWNLOAD_SET(&(phy->core->init_config));
        } else if (firmware_load_type->force_load_method == bcmpmFirmwareLoadForce) {
            PHYMOD_CORE_INIT_F_FW_FORCE_DOWNLOAD_SET(&(phy->core->init_config));
        } else if (firmware_load_type->force_load_method == bcmpmFirmwareLoadEEPROMUpgrade) {
            PHYMOD_CORE_INIT_F_FW_EEPROM_UPGRADE_SET(&(phy->core->init_config));
        }

#if ! defined (PHYMOD_XGBASET_SUPPORT)
        if ( ((firmware_load_type->force_load_method == bcmpmFirmwareLoadForce) ||
              (firmware_load_type->force_load_method == bcmpmFirmwareLoadAuto)) &&
             (firmware_load_type->firmware_load_method == bcmpmFirmwareLoadMethodNone) ){
            rv = BCM_PM_IF_PARAM ;
            PHYMOD_DEBUG_ERROR(("Invalid combination of Firmware Load Method and Force Load Method \n"));
            goto ERR;
        }
#endif

        for(glb_arr_idx = 0; glb_arr_idx < PHYMOD_IF_CONFIG_MAX_PHYS; glb_arr_idx++) {
            if(phy_info.phy_addr == bcm_plp_aperta_phy_static_config[glb_arr_idx].phy_id) {
                break;
            }
        }
        if(glb_arr_idx < PHYMOD_IF_CONFIG_MAX_PHYS) {
            if (bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config != NULL) {
                phy->static_config = bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config;
            } else {
                PHYMOD_DEBUG_INFO(("No static configurations available for the requested PHY\n"));
            }
        }
    } else {   /* is_exist_phy */
            phy = plp_aperta_phy_ctrl.phy[phy_id_idx];
    }

#if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined (PHYMOD_APERTA_SUPPORT) || defined(PHYMOD_EVORA_SUPPORT) || defined (PHYMOD_AGERA_SUPPORT) \
	                              || defined (PHYMOD_AGERALITE_SUPPORT) || defined (PHYMOD_AGERA2_SUPPORT) || defined (PHYMOD_APERTA2_SUPPORT)
    phy->static_config = firmware_load_type->fw_init_params;
#endif

#if defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_PHY848XX_SUPPORT)
    /* pass the MASTER port address by plp_aperta_phymod_core_init_config_s structure */
    phy->core->init_config.firmware_load_master_port = firmware_load_master_port;
#endif
#if defined(PHYMOD_MADURA_SUPPORT) || defined (PHYMOD_HURACAN_SUPPORT)
    /*Read the environment variable to determine warm boot support is enabled or not */
    warm_boot_env = PHYMOD_GETENV("BCM_PLP_WARM_BOOT");
    warm_boot_enable = PHYMOD_ATOI(warm_boot_env);
    if (warm_boot_enable) { /* Warm boot environment variable is set to 1 */
        /* Do nothing */
    } else { /* Warm boot environment variable is set to 0 */
        goto COLDBOOT;
    }
#else 
    #ifdef BCM_PLP_WARM_BOOT_SUPPORT
        if (phy_info.flags & BCM_PLP_WARM_BOOT) { /* Warm boot flag is set */
            /* Do nothing */
        #ifdef BCM_PLP_MAC_SUPPORT
            {
                bcm_plp_mac_access_t mac_access;
                PHYMOD_MEMCPY(&mac_access.phy_info, &phy_info, sizeof(bcm_plp_access_t));
                rv = bcm_plp_aperta_warmboot_init(mac_access);
                if (rv != 0) {
                    PHYMOD_DEBUG_ERROR(("_bcm_warmboot_init rv:%d\n",rv));
                    goto ERR;
                }
            }
        #endif
        #if defined (PHYMOD_XGBASET_SUPPORT) || defined (PHYMOD_GALLARDO28_SUPPORT) || defined (PHYMOD_BARCHETTA_SUPPORT) || \
            defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined (PHYMOD_AGERA_SUPPORT) || defined (PHYMOD_AGERA2_SUPPORT) || defined (PHYMOD_AGERALITE_SUPPORT)
            {
                int  dummy = 0;  /* dummy value for the 2nd argument of plp_aperta_phymod_warmboot_init *\
                                 \*        since PHYMOD doesn't allow null pointer arguments */
                rv = plp_aperta_phymod_warmboot_init(&(phy->core->pm_core), &dummy);
                if ( rv != 0 ) {
                    PHYMOD_DEBUG_ERROR(("WARM BOOT : plp_aperta_phymod_warmboot_init rv:%d\n",rv));
                    goto ERR;
                }
            }
        #endif
        #if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT)
            if (firmware_load_type->force_load_method == bcmpmFirmwareLoadEEPROMUpgrade) {
                PHYMOD_CORE_INIT_F_FW_EEPROM_UPGRADE_SET(&(phy->core->init_config));
                rv = _bcm_plp_aperta_phy_init_bcast(phy, firmware_load_type);
                if (rv != 0) {
                    PHYMOD_DEBUG_ERROR(("_bcm_plp_aperta_phy_init_bcast rv:%d\n",rv));
                    goto ERR;
                }
                PHYMOD_CORE_INIT_F_FW_EEPROM_UPGRADE_CLR(&(phy->core->init_config));
            }
        #endif
        } else { /* Warm boot flag is cleared */
            goto COLDBOOT;
        }
    #else /* Cold boot sequence*/ 
        goto COLDBOOT;
    #endif
#endif
    /* coverity[unreachable] */
    rv = BCM_PM_IF_SUCCESS;
    goto ERR;

COLDBOOT:
    switch(broadcast_method){
        case bcmpmFirmwareBroadcastCoreReset:
            PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_SET(&(phy->core->init_config));
            rv = _bcm_plp_aperta_phy_init_bcast(phy, firmware_load_type);
            if (rv != 0) {
                PHYMOD_DEBUG_ERROR(("_bcm_plp_aperta_phy_init_bcast() ERROR!! bcast_method=%d  rv=%d\n", broadcast_method, rv));
                goto ERR;
            }
            PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_CLR(&(phy->core->init_config));
            break;
        case bcmpmFirmwareBroadcastEnable:
            PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_SET(&(phy->core->init_config));
            rv = _bcm_plp_aperta_phy_init_bcast(phy, firmware_load_type);
            if (rv != 0) {
                PHYMOD_DEBUG_ERROR(("_bcm_plp_aperta_phy_init_bcast() ERROR!! bcast_method=%d  rv=%d\n", broadcast_method, rv));
                goto ERR;
            }
            PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_CLR(&(phy->core->init_config));
            break;
        case bcmpmFirmwareBroadcastFirmwareExecute:
            PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD_SET(&(phy->core->init_config));
            rv = _bcm_plp_aperta_phy_init_bcast(phy, firmware_load_type);
            if (rv != 0) {
                PHYMOD_DEBUG_ERROR(("_bcm_plp_aperta_phy_init_bcast() ERROR!! bcast_method=%d  rv=%d\n", broadcast_method, rv));
                goto ERR;
            }
            PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD_CLR(&(phy->core->init_config));
            break;
        case bcmpmFirmwareBroadcastFirmwareVerify:
            PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_SET(&(phy->core->init_config));
            rv = _bcm_plp_aperta_phy_init_bcast(phy, firmware_load_type);
            if (rv != 0) {
                PHYMOD_DEBUG_ERROR(("_bcm_plp_aperta_phy_init_bcast() ERROR!! bcast_method=%d  rv=%d\n", broadcast_method, rv));
                goto ERR;
            }
            PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_CLR(&(phy->core->init_config));
            break;
        case bcmpmFirmwareBroadcastEnd:
            PHYMOD_CORE_INIT_F_FW_LOAD_END_SET(&(phy->core->init_config));
            rv = _bcm_plp_aperta_phy_init_bcast(phy, firmware_load_type);
            if (rv != 0) {
                PHYMOD_DEBUG_ERROR(("_bcm_plp_aperta_phy_init_bcast() ERROR!! bcast_method=%d  rv=%d\n", broadcast_method, rv));
                goto ERR;
            }
            PHYMOD_CORE_INIT_F_FW_LOAD_END_CLR(&(phy->core->init_config));
            break;
        case bcmpmFirmwareBroadcastNone:
#if defined (PHYMOD_XGBASET_SUPPORT)
            if ( (firmware_load_type->force_load_method == bcmpmFirmwareLoadSkip) ||
                 (firmware_load_type->force_load_method == bcmpmFirmwareLoadForce)) {
                PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_CLR(&(phy->core->init_config));
            } else {
                PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_SET(&(phy->core->init_config));
            }
#else
            if (firmware_load_type->force_load_method == bcmpmFirmwareLoadSkip) {
                PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_CLR(&(phy->core->init_config));
            } else {
              PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_SET(&(phy->core->init_config));
            }

            if ((firmware_load_type->firmware_load_method == bcmpmFirmwareLoadMethodExternal) ||
                (firmware_load_type->firmware_load_method == bcmpmFirmwareLoadMethodExternalNVM)) {
                if (firmware_load_type->firmware_loader !=NULL) {
                    phy->core->init_config.firmware_loader = firmware_load_type->firmware_loader;
                } else{
                    PHYMOD_DEBUG_ERROR(("Invalid firmware loader\n"))
                    rv = BCM_PM_IF_PARAM;
                    goto ERR;
                }
            }
#endif
            rv = _bcm_plp_aperta_pm_phy_init(phy, firmware_load_type->firmware_load_method, firmware_load_type->fw_init_params);
            if (rv != 0) {
                PHYMOD_DEBUG_ERROR(("_bcm_plp_aperta_pm_phy_init rv:%d\n",rv));
                goto ERR;
            }
        break;
        default:
            PHYMOD_DEBUG_ERROR(("PHY not available\n"))
            rv = BCM_PM_IF_PHY_NA;
            goto ERR;
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    if (rv != 0) {
        PHYMOD_DEBUG_ERROR(("Init Failed with error:%d!!! Cleaning Up\n", rv))
        bcm_plp_aperta_cleanup(phy_info);
    }

    return rv;
}
/*! \brief Cleanup the PHY.
 *
 *	This API cleanup the allocated SW database and invalidated the mentioned PHY-ID.
 *	This API is not thread safe and cannot be called from different threads.
 *
 * @param phy_info  <pre> Represents PHY access\n
 * </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_cleanup(bcm_plp_access_t phy_info)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    unsigned int glb_arr_idx = 0;
    int rv = BCM_PM_IF_SUCCESS;

    if (phy_info.phy_addr >= BCM_PM_IF_MAX_PHY) {
        PHYMOD_DEBUG_ERROR(("bcm_plp_aperta_cleanup(): Invalid PHY\n"))
        return BCM_PM_IF_INVALID_PHY;
    }

    _bcm_plp_aperta_pm_if_get_phy_id_idx(phy_info.phy_addr, &phy_id_idx, &exist_phy);
    if (exist_phy != 1) {
        _plp_aperta_phyid_list[phy_info.phy_addr].valid = 0;
        PHYMOD_DEBUG_ERROR(("PHY does not exist\n"));
        return BCM_PM_IF_PHY_NA;
    }
    if (phy_id_idx >= PHYMOD_IF_CONFIG_MAX_PHYS) {
        PHYMOD_DEBUG_ERROR(("MAX PHY reached\n"));
        return BCM_PM_IF_INTERNAL; 
    }
    if (plp_aperta_phy_ctrl.phy[phy_id_idx] && plp_aperta_phy_ctrl.phy[phy_id_idx]->static_config) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->static_config = NULL;
    }
#ifdef PHYMOD_PHY542XX_SUPPORT
    if (plp_aperta_phy_ctrl.phy[phy_id_idx]) {
        BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
        /* Call internal driver cleanup function */
        rv = plp_aperta_phymod_phy_cleanup(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
        if(rv){
            PHYMOD_DEBUG_INFO(("driver cleanup failed rv =%d \n",rv));
        }
    }
#endif

    for(glb_arr_idx = 0; glb_arr_idx < PHYMOD_IF_CONFIG_MAX_PHYS; glb_arr_idx++) {
        if((phy_info.phy_addr == bcm_plp_aperta_phy_static_config[glb_arr_idx].phy_id) &&
           (bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config != NULL)) {
            PHYMOD_IF_FREE(bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config);
            bcm_plp_aperta_phy_static_config[glb_arr_idx].phy_id = 0xff;
            bcm_plp_aperta_phy_static_config[glb_arr_idx].bcm_static_config = NULL;
        }
    }
    if (plp_aperta_phy_ctrl.phy[phy_id_idx]) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->mutex_info.mutex_give = NULL;
        plp_aperta_phy_ctrl.phy[phy_id_idx]->mutex_info.mutex_take = NULL;
        if (plp_aperta_phy_ctrl.phy[phy_id_idx]->core) {
            PHYMOD_IF_FREE(plp_aperta_phy_ctrl.phy[phy_id_idx]->core);
        }
        PHYMOD_IF_FREE(plp_aperta_phy_ctrl.phy[phy_id_idx]);
        plp_aperta_phy_ctrl.phy[phy_id_idx] = NULL;
    }
    _plp_aperta_phyid_list[phy_id_idx].phy_id = 0;
    _plp_aperta_phyid_list[phy_id_idx].valid = 0;
    PHYMOD_DEBUG_INFO(("Cleaning up phyid:%d\n", phy_info.phy_addr));

    return rv;
}

/*! \brief Retrives the link status.
 *
 *	This API retrives link status of the specified PHY. If PHY supports PCS it will 
 *	return PCS live link status, if not PMD link status will be returned.
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param link_status     [OUT] Retrives PMD/PCS link status </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int
bcm_plp_aperta_link_status_get(bcm_plp_access_t phy_info,  unsigned int *link_status)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    
    rv = plp_aperta_phymod_phy_link_status_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, link_status);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Mode Configuration.
 *
 *	This API configures speed, interface, reference clock and auxilary modes on the specified PHY-id. 
 *  @param phy_info        Represents PHY access\n
 *  @param speed           Represents datarate of a port(mentioned through lane_map) in PHY in Mbps
 *	                       ex: speed = 40000 for 40G
 *  @param if_type         Represents electrical interface type of the PHY, interface are as follows,
 *	                       0 - bcm_pm_InterfaceBypass, 1  - bcm_pm_InterfaceSR, 2 - bcm_pm_InterfaceSR4, 3 - bcm_pm_InterfaceKX,\n
 *	                       4 - bcm_pm_InterfaceKX4, 5 - bcm_pm_InterfaceKR,  6 - bcm_pm_InterfaceKR2,  7 - bcm_pm_InterfaceKR4, \n
 *	                       8 - bcm_pm_InterfaceCX, 9 - bcm_pm_InterfaceCX2, 10 - bcm_pm_InterfaceCX4, 11 - bcm_pm_InterfaceCR, \n
 *	                       12 - bcm_pm_InterfaceCR2, 13 - bcm_pm_InterfaceCR4, 14 - bcm_pm_InterfaceCR10, 15 - bcm_pm_InterfaceXFI,\n
 *	                       16 - bcm_pm_InterfaceSFI, 17 - bcm_pm_InterfaceSFPDAC 18 - bcm_pm_InterfaceXGMII, 19 - bcm_pm_Interface1000X,\n
 *	                       20 - bcm_pm_InterfaceSGMII, 21 - bcm_pm_InterfaceXAUI, 22 - bcm_pm_InterfaceRXAUI, 23 - bcm_pm_InterfaceX2,  24 - bcm_pm_InterfaceXLAUI,\n
 *	                       25 - bcm_pm_InterfaceXLAUI2, 26 - bcm_pm_InterfaceCAUI, 27 - bcm_pm_interfaceQSGMII, 28 - bcm_pm_InterfaceLR4, 29 - bcm_pm_InterfaceLR \n
 *	                       30 - bcm_pm_InterfaceLR2, 31 - bcm_pm_InterfaceER, 32 - bcm_pm_InterfaceER2, 33 - bcm_pm_InterfaceER4, 34 - bcm_pm_InterfaceSR2 \n
 *	                       35 - bcm_pm_InterfaceSR10, 36 - bcm_pm_InterfaceCAUI4, 37 - bcm_pm_InterfaceVSR, 38 - bcm_pm_InterfaceLR10, 39 - bcm_pm_InterfaceKR10 \n
 *	                       40 - bcm_pm_InterfaceCAUI4_C2C, 41 - bcm_pm_InterfaceCAUI4_C2M, 42 - bcm_pm_InterfaceZR, 43 - bcm_pm_InterfaceLRM, 44 - bcm_pm_InterfaceXLPPI \n
 *	                       45 - bcm_pm_InterfaceOTN, 46 - bcm_pm_InterfaceAUI_C2C, 47 - bcm_pm_InterfaceAUI_C2M
 *
 *  @param ref_clk         Represents reference clock of the PHY\n
 *	                       0 - bcm_pm_RefClk156Mhz(156.25MHz) , 1 - bcm_pm_RefClk125Mhz \n
 *	                       2 - bcm_pm_RefClk106Mhz , 3 - bcm_pm_RefClk161Mhz \n
 *	                       4 - bcm_pm_RefClk174Mhz , 5 - bcm_pm_RefClk312Mhz \n
 *	                       6 - bcm_pm_RefClk322Mhz , 7 - bcm_pm_RefClk349Mhz \n
 *	                       8 - bcm_pm_RefClk644Mhz , 9 - bcm_pm_RefClk698Mhz \n
 *	                       9 - bcm_pm_RefClk155Mhz , 10 - bcm_pm_RefClk156P6Mhz \n
 *	                       11 - bcm_pm_RefClk157Mhz , 12 - bcm_pm_RefClk158Mhz \n
 *	                       13 - bcm_pm_RefClk159Mhz , 14 - bcm_pm_RefClk168Mhz \n
 *	                       15 - bcm_pm_RefClk172Mhz , 16 - bcm_pm_RefClk173Mhz \n
 *
 *  @param interface_mode  Represents mode of the PHY\n
 *                         Interface mode can be set by user in three ways.
 *                         i) Basic mode support for the software backward compatibility
 *                             0 - bcm_pm_Interface_mode_IEEE\n
 *                             1 - bcm_pm_Interface_mode_HIGIG \n
 *                             2 - bcm_pm_Interface_mode_OTN \n
 *                             11 - bcm_pm_Interface_mode_proprietary \n
 *
 *                         ii) Basic and enhanced mode support
 *                             0x100000 - bcmplpInterfaceModeIEEE\n
 *                             0x10 - bcmplpInterfaceModeHIGIG\n
 *                             0x100 - bcmplpInterfaceModeFIBER\n
 *                             0x200 - bcmplpInterfaceModeTRIPLECORE\n
 *                             0x400 - bcmplpInterfaceModeTC343\n
 *                             0x800 - bcmplpInterfaceModeTC442\n
 *                             0x1000 - bcmplpInterfaceModeTC244\n
 *                             0x8000 - bcmplpInterfaceModeOTN\n
 *                             0x20000 - bcmplpInterfaceModeUNRELIABLELOS\n
 *
 *                         iii) 10GBase-T PHY support
 *                             0x00 - USXGMII disabled
 *                             0x01 - USXGMII Single  mode
 *                             0x02 - USXGMII Dual    mode
 *                             0x04 - USXGMII Quad    mode
 *                             0x08 - USXGMII Octal   mode
 *                             0x10 - USXGMII AutoNeg mode
 *
 *                         Legacy software users can use method i) and new software users can use the method ii)
 *
 *  @param device_aux_modes Structure that contains chip specific mode information such as pass-through or gear box\n
 *                          For details see phymod/chip/{chip name}/tier2/{chip name}.c file
 *
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_mode_config_set(bcm_plp_access_t phy_info,  int speed, int if_type,
                    int ref_clk, int interface_mode, void* device_aux_modes)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0, flags = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_phy_inf_config_t mode_config;
    bcm_plp_device_aux_modes_t *aux_mode = (bcm_plp_device_aux_modes_t*) device_aux_modes;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    
    if (aux_mode == NULL) {
        PHYMOD_DEBUG_ERROR(("AUX MODE PARAMETER IS NULL\n"));
        rv = BCM_PM_IF_MEMORY;
        goto ERR;
    }

#if defined(BCM_PLP_BASE_T_PHY) || defined(PHYMOD_ESTOQUE_SUPPORT) || defined(PHYMOD_EVORA_SUPPORT) || \
    defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined(PHYMOD_APERTA_SUPPORT) \
    || defined(PHYMOD_AGERA_SUPPORT) || defined (PHYMOD_AGERALITE_SUPPORT)|| defined (PHYMOD_APERTA2_SUPPORT) || defined (PHYMOD_AGERA2_SUPPORT)
    /* do nothing here */
#else
    if (aux_mode->pass_thru) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.device_op_mode = 0x0;
    } else {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.device_op_mode = 0x1;
    }
#endif

    PHYMOD_IF_MEMSET(&mode_config, 0, sizeof(plp_aperta_phymod_phy_inf_config_t));

#if defined(BCM_PLP_BASE_T_PHY)
    mode_config.interface_modes = (unsigned int) interface_mode;
#else
    /* bcm interface mode enum to phymod interface mode value */
    if (interface_mode & 0xf) {
    switch(interface_mode) {
        case bcm_pm_Interface_mode_IEEE:
            mode_config.interface_modes = 0x0;
        break;
        case bcm_pm_Interface_mode_HIGIG:
            mode_config.interface_modes = PHYMOD_INTF_MODES_HIGIG;
        break;
        case bcm_pm_Interface_mode_OTN:
            mode_config.interface_modes = PHYMOD_INTF_MODES_OTN;
        break;
        case bcm_pm_Interface_mode_FIBER:
            mode_config.interface_modes = PHYMOD_INTF_MODES_FIBER;
        break;
        default:
            PHYMOD_DEBUG_ERROR(("Invalid interface mode\n"));
            rv = BCM_PM_IF_INTERNAL;
            goto ERR;
        }
    }
    if (speed >= 100000) {
        if ((!plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.device_op_mode) ||
            (plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.port_loc == phymodPortLocSys)) { /* 100G Pass-through */
            if (plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.lane_mask == 0x3ff) {
                interface_mode |= bcmplpInterfaceModeTRIPLECORE;
                interface_mode |= bcmplpInterfaceModeTC442;
            } else if (plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.lane_mask == 0x7fe) {
                interface_mode |= bcmplpInterfaceModeTRIPLECORE;
                interface_mode |= bcmplpInterfaceModeTC343;
            } else if (plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.lane_mask == 0xffc) {
                interface_mode |= bcmplpInterfaceModeTRIPLECORE;
                interface_mode |= bcmplpInterfaceModeTC244;
            }
        } else { /* 100G Gearbox */
            interface_mode |= bcmplpInterfaceModeTRIPLECORE;
        }
    }
    if (interface_mode & 0xfffffff0) {
        mode_config.interface_modes = (unsigned int) (interface_mode >> 4);
    }
#endif

    mode_config.interface_type = if_type;
    mode_config.data_rate = speed;
    mode_config.ref_clock = ref_clk;
    mode_config.device_aux_modes = device_aux_modes;
    mode_config.otn_type = 0;
#ifdef PHYMOD_MIURA_SUPPORT
   flags = phy_info.flags; 
#endif
    rv = plp_aperta_phymod_phy_interface_config_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, &mode_config);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Retrive the configured mode.
 *
 *	This API retrives speed, interface, refclock and auxilary mode of specified PHY-id. 
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param speed            [OUT] Represents datarate of the PHY \n
 *  @param if_type          [OUT] Represents interface of the PHY, interface are as follows,\n
 *  @param ref_clk          [OUT] Represents reference clock of the PHY (not supported by all PHYs)\n
 *  @param interface_mode   [OUT] Represents interface mode of the PHY\n
 *  @param device_aux_modes [OUT] Structure that contains chip specific mode information\n </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_mode_config_get(bcm_plp_access_t phy_info, int *speed, int *if_type,
                    int *ref_clk, int *interface_mode, void *device_aux_modes)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
#ifndef BCM_PLP_BASE_T_PHY
    uint32_t lane_data_rate = 0;
    uint8_t  num_of_lanes = 0;
#endif
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_phy_inf_config_t mode_config;

#ifdef BCM_PLP_BASE_T_PHY
    bcm_plp_base_t_device_aux_modes_t *p_aux_mode =
                                      (bcm_plp_base_t_device_aux_modes_t*) device_aux_modes;
#else
    bcm_plp_device_aux_modes_t        *p_aux_mode =
                                      (bcm_plp_device_aux_modes_t*) device_aux_modes;
#endif
    if (p_aux_mode == NULL) {
        PHYMOD_DEBUG_ERROR(("AUX MODE PARAM is NULL\n"));
        rv = BCM_PM_IF_MEMORY;
        goto ERR;
    }

    PHYMOD_IF_MEMSET(&mode_config, 0, sizeof(plp_aperta_phymod_phy_inf_config_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    mode_config.device_aux_modes = device_aux_modes;
#ifdef BCM_PLP_BASE_T_PHY
    rv = plp_aperta_phymod_phy_interface_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                            p_aux_mode->ctrl_select, 0,         &mode_config);
#else
    /* Initialize ref_clk. It is not an input for this API */
    *ref_clk = 0;
    rv = plp_aperta_phymod_phy_interface_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                            0,                       *ref_clk,  &mode_config);
#endif
    if (rv != 0) {
        goto ERR;
    }

#if defined(BCM_PLP_BASE_T_PHY)
    *interface_mode |= (int) mode_config.interface_modes;
#else
    /* phymod interface mode to bcm interface mode enum */
    switch(mode_config.interface_modes & 0xF) {
        case 0x0:
            *interface_mode = bcm_pm_Interface_mode_IEEE;
        break;
        case PHYMOD_INTF_MODES_HIGIG:
            *interface_mode = bcm_pm_Interface_mode_HIGIG;
        break;
        /* coverity[dead_error_begin] */
        case PHYMOD_INTF_MODES_OTN:
            *interface_mode = bcm_pm_Interface_mode_OTN;
        break;
        /* coverity[dead_error_begin] */
        case PHYMOD_INTF_MODES_FIBER:
            *interface_mode = bcm_pm_Interface_mode_FIBER;
        break;
        default:
        PHYMOD_DEBUG_ERROR(("Invalid interface mode\n"));
        rv = BCM_PM_IF_INTERNAL;
        goto ERR;
    }
    *interface_mode |= (int) (mode_config.interface_modes << 4);
    if (!(*interface_mode & bcmplpInterfaceModeHIGIG)
     && !(*interface_mode & bcmplpInterfaceModeOTN)) {
        *interface_mode |= (bcmplpInterfaceModeIEEE);
        BCM_PLP_PHY_GET_NUM_OF_LANES(phy_info.lane_map, num_of_lanes);
        BCM_PLP_PHY_GET_PER_LANE_DATARATE(mode_config.data_rate, num_of_lanes, lane_data_rate);
        /* If FIBER */
        if(BCM_PLP_PHY_IS_FC_LANE_DATARATE(lane_data_rate)) {
            /* Clear IEEE Bit */
            *interface_mode &= ~(bcmplpInterfaceModeIEEE);
        }
    } else {
        *interface_mode &= ~(bcmplpInterfaceModeIEEE);
    }
#endif

    *if_type = mode_config.interface_type;
    *speed = mode_config.data_rate;
    *ref_clk = mode_config.ref_clock;

#ifdef PHYMOD_ESTOQUE_SUPPORT
    goto ERR;
#endif
#ifdef PHYMOD_MILLENIO_SUPPORT
    goto ERR;
#endif
#ifdef PHYMOD_BARCHETTA2_SUPPORT
    goto ERR;
#endif
#ifdef PHYMOD_BARCHETTA_SUPPORT
    goto ERR;
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    goto ERR;
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    goto ERR;
#endif

#ifdef PHYMOD_AGERA_SUPPORT
    goto ERR;
#endif
#ifdef PHYMOD_AGERALITE_SUPPORT
    goto ERR;
#endif
#ifdef PHYMOD_AGERA2_SUPPORT
    goto ERR;
#endif

#ifdef BCM_PLP_BASE_T_PHY
    PHYMOD_IF_MEMCPY(device_aux_modes, mode_config.device_aux_modes,
                                       sizeof(bcm_plp_base_t_device_aux_modes_t));
#else
    /* coverity[unreachable] */
    if (plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.device_op_mode == 0) {
        p_aux_mode->pass_thru = 1;
    } else {
        p_aux_mode->pass_thru = 0;
    }
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief SW version.
 *
 *	This API used to retrives SW version 
 *
 *  @param chip_ver        [OUT] Retrives chip version number 
 *  @param api_ver	       [OUT] Retrives api version number
 *  @param enahan_ver      [OUT] Retrives Enhancement version number
 *
 */
void
bcm_plp_aperta_version_get(unsigned short *chip_ver, unsigned short *api_ver, unsigned short *enahan_ver)
{
    *chip_ver = BCM_PM_IF_CHIP_VER_NO;
    *api_ver  =  BCM_PM_IF_API_VER_NO; 
    *enahan_ver = BCM_PM_IF_ENAHAN_VER_NO;
}

/*! \brief PRBS configuration.
 *
 *	This API configures PRBS and enable/disable generator(TX) and checker(Rx) for PRBS operation. 
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param tx_rx           Represents Transmit, receive or both\n 
 *	                       0 = Both transmit and receive PRBS \n 1 = Receive side PRBS\n 2 = Transmit side PRBS
 *  @param poly            Represents PRBS polynomial type \n
 *	                       0 = PRBS polynomial 7 \n
 *	                       1 = PRBS polynomial 9 \n
 *	                       2 = PRBS polynomial 11\n
 *	                       3 = PRBS polynomial 15\n
 *                         4 = PRBS polynomial 23\n
 *                         5 = PRBS polynomial 31\n
 *                         6 = PRBS polynomial 58\n
 *                         7 = PRBS polynomial 49\n
 *                         8 = PRBS polynomial 10\n
 *                         9 = PRBS polynomial 20\n
 *                         10 = PRBS polynomial 13
 *  @param invert          Represents PRBS inversion \n
 *	                       1 = Set invert,\n 0 = reset invert
 *  @param loopback        Reserved for future use only
 *  @param ena_dis         Represents PRBS Enable/Disable\n
 *	                       1 - Enable PRBS\n
 *	                       0 - Disable PRBS </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_prbs_set(bcm_plp_access_t phy_info,  unsigned int tx_rx, 
               unsigned int poly, unsigned int invert, unsigned int loopback, unsigned int ena_dis)
{
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_diag_prbs_set_args_t params;
    uint32_t phy_id_idx = 0xff;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_diag_prbs_args_t prbs_diag_args;
 
    params.prbs_options.poly = poly;
    params.prbs_options.invert = invert;
    params.enable = ena_dis;
    params.loopback = loopback;
    params.flags = tx_rx;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION(phy_info, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
    
    prbs_diag_args.prbs_cmd = PhymodDiagPrbsSet;
    PHYMOD_IF_MEMCPY(&prbs_diag_args.args.set_params, &params, sizeof(plp_aperta_phymod_diag_prbs_set_args_t));

    rv = plp_aperta_phymod_diag_prbs(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, 1, &prbs_diag_args);
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION_CLR(plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief PRBS configuration get.
 *
 *	This API retives the configured TX/RX PRBS. 
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param tx_rx           Represents Transmit, receive or both\n 
 *	                       0 = Both transmit and receive PRBS \n 1 = Receive side PRBS\n 2 = Transmit side PRBS
 *  @param poly            [OUT] Represents PRBS polynomial type \n
 *	                       0 = polynomial 7 \n
 *	                       1 = polynomial 9 \n
 *	                       2 = polynomial 11\n 
 *	                       3 = polynomial 15\n
 *                         4 = polynomial 23\n
 *                         5 = polynomial 31\n
 *                         6 = polynomial 58\n
 *                         7 = polynomial 49\n
 *                         8 = polynomial 10\n
 *                         9 = polynomial 20\n
 *                         10 = polynomial 13
 *  @param invert          [OUT] Represents PRBS inversion \n
 *	                       1 = Invert got enable \n 0 = Invert got not enable
 *  @param loopback        Reserved for future use only
 *  @param ena_dis         [OUT] Represents PRBS Enable/Disable \n
 *	                       1 - Enable PRBS \n
 *	                       0 - Disable PRBS</pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_prbs_get(bcm_plp_access_t phy_info,  unsigned int tx_rx, 
               unsigned int *poly, unsigned int *invert, unsigned int *loopback, unsigned int *ena_dis)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    uint32_t enable = 0xFF;
    plp_aperta_phymod_prbs_t prbs_conf_param;
 
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION(phy_info, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
    
    rv = plp_aperta_phymod_phy_prbs_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, tx_rx, &enable);
    if (rv != 0) {
        goto ERR;
    } 
    *ena_dis = enable;
    rv = plp_aperta_phymod_phy_prbs_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, tx_rx, &prbs_conf_param);
    if (rv != 0) {
        goto ERR;
    } 
    *poly = prbs_conf_param.poly; 
    *invert = prbs_conf_param.invert;
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION_CLR(plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Dump PRBS Status.
 *
 *	This API dump PRBS Checker(RX) Status. 
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param time            Represents delay between the PRBS RX status read's
 *	                       i.e. Read Rx STATUS;wait till 'time' expires; Re-read Rx status </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_prbs_rx_stat(bcm_plp_access_t phy_info,  unsigned int time)
{
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_diag_prbs_get_args_t params;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_diag_prbs_args_t prbs_diag_args;

    params.time = time;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION(phy_info, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
    prbs_diag_args.prbs_cmd = PhymodDiagPrbsGet;
    PHYMOD_IF_MEMCPY(&prbs_diag_args.args.get_params, &params, sizeof(plp_aperta_phymod_diag_prbs_get_args_t));

    rv = plp_aperta_phymod_diag_prbs(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, 1, &prbs_diag_args);
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION_CLR(plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
    
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Clear PRBS 
 *
 *	This API disable PRBS Generator(TX) / Checker(Rx) . 
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param tx_rx           Represents Tx/Rx PRBS\n
 *	                       1 = Receive side PRBS \n 2 = Transmit side PRBS\n
 *                         0 = Both Tranmit and Receive side </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_prbs_clear(bcm_plp_access_t phy_info,  unsigned int tx_rx)
{
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_diag_prbs_clear_args_t params;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_diag_prbs_args_t prbs_diag_args;
 
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION(phy_info, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
    params.flags = tx_rx;
    prbs_diag_args.prbs_cmd = PhymodDiagPrbsClear;
    PHYMOD_IF_MEMCPY(&prbs_diag_args.args.clear_params, &params, sizeof(plp_aperta_phymod_diag_prbs_clear_args_t));
    rv = plp_aperta_phymod_diag_prbs(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, 1, &prbs_diag_args);
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION_CLR(plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Get PRBS configuration.
 *
 *	This API used to retrieve TX/RX PRBS configurations for the specified lane
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param tx_rx           Represents Tx/Rx PRBS\n
 *	                       1 = Receive side PRBS \n 2 = Transmit side PRBS\n
 *                         0 = Both Tranmit and Receive side 
 *  @param poly            [OUT] Represents PRBS polynomial type\n
 *	                       0 = polynomial 7\n
 *	                       1 = polynomial 9\n
 *	                       2 = polynomial 11\n
 *	                       3 = polynomial 15\n
 *                         4 = polynomial 23\n
 *                         5 = polynomial 31\n
 *                         6 = polynomial 58\n
 *  @param invert          [OUT] Represents PRBS inversion\n
 *	                       1 = invert\n 0 = no inversion </pre>\n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_prbs_config_get(bcm_plp_access_t phy_info,  unsigned int tx_rx, unsigned int *poly, unsigned int *invert)
{
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_prbs_t prbs_conf_param;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION(phy_info, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
    rv = plp_aperta_phymod_phy_prbs_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, tx_rx, &prbs_conf_param);
    
    *poly = prbs_conf_param.poly; 
    *invert = prbs_conf_param.invert;
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION_CLR(plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Get PRBS Status.
 *
 *  This API retrives Rx checker status of the specified lane. It
 *  also provide lock lost and error count.
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param prbs_lock       [OUT] Represents whether PRBS is currently locked
 *  @param prbs_lock_loss  [OUT] Represents PRBS was unlocked since last call
 *  @param error_count     [OUT] Represents PRBS errors count </pre>
 *
 */

int
bcm_plp_aperta_prbs_status_get(bcm_plp_access_t phy_info,  unsigned int *prbs_lock, unsigned int *prbs_lock_loss, unsigned int *error_count)
{
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_prbs_status_t prbs_status;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION(phy_info, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
    rv = plp_aperta_phymod_phy_prbs_status_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, 0, &prbs_status);

    *prbs_lock = prbs_status.prbs_lock;
    *prbs_lock_loss = prbs_status.prbs_lock_loss;
    *error_count = prbs_status.error_count;
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION_CLR(plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Register Write 
 *
 *	This API is used to perform register write. 
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param devaddr         device address\n 1 - PMA/PMD \n 3 - PCS \n 7 - AN \n 30 - user
 *  @param regaddr         Register address of the device
 *  @param data            Value to be written to Register    </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_reg_value_set(bcm_plp_access_t phy_info,  unsigned int devaddr, unsigned int regaddr, unsigned int data)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
 
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.devad = devaddr;
#if defined(PHYMOD_BARCHETTA_SUPPORT) || defined (PHYMOD_AGERA_SUPPORT) || defined (PHYMOD_AGERALITE_SUPPORT)
    if (phy_info.data_path_dir == bcmplpDataPathDirectionEgress) {
        regaddr |= ((bcmplpDataPathDirectionEgress) << 28);
    }
    if (phy_info.data_path_dir == bcmplpDataPathDirectionIngress) {
        regaddr |= ((bcmplpDataPathDirectionIngress) << 28);
    }
#endif
#if defined(PHYMOD_AGERA2_SUPPORT)
    if (phy_info.data_path_dir == bcmplpDataPathDirectionEgress) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= ((bcmplpDataPathDirectionEgress) << 28);
    }
    if (phy_info.data_path_dir == bcmplpDataPathDirectionIngress) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= ((bcmplpDataPathDirectionIngress) << 28);
    }
#endif
#if defined(PHYMOD_SEAHAWKS_SUPPORT)
		if (phy_info.flags & BCM_PLP_SGMII_REG_ACCESS_MODE) {
			plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_SGMII_REG_ACCESS_MODE;
		}
#endif

    rv = plp_aperta_phymod_phy_reg_write(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, data);

#if defined(PHYMOD_AGERA2_SUPPORT)
    if (phy_info.data_path_dir == bcmplpDataPathDirectionEgress) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= ~((bcmplpDataPathDirectionEgress) << 28);
    }
    if (phy_info.data_path_dir == bcmplpDataPathDirectionIngress) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= ~((bcmplpDataPathDirectionIngress) << 28);
    }
#endif
#if defined(PHYMOD_SEAHAWKS_SUPPORT)
    if (phy_info.flags & BCM_PLP_SGMII_REG_ACCESS_MODE) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= ~(BCM_PLP_SGMII_REG_ACCESS_MODE);
    }
#endif


ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}


/*! \brief Register Read
 *
 *	This API is used to read the specified register address.
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param devaddr         device address\n 1 - PMA/PMD \n 3 - PCS \n 7 - AN \n 30 - user
 *  @param regaddr         Register address to read
 *  @param data            [OUT] Value of the specified register    </pre>
 *	       
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_reg_value_get(bcm_plp_access_t phy_info,  unsigned int devaddr, unsigned int regaddr, unsigned int *data)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
 
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.devad = devaddr;
#if defined(PHYMOD_BARCHETTA_SUPPORT) || defined (PHYMOD_AGERA_SUPPORT) || defined (PHYMOD_AGERALITE_SUPPORT)
    if (phy_info.data_path_dir == bcmplpDataPathDirectionEgress) {
        regaddr |= ((bcmplpDataPathDirectionEgress) << 28);
    }
    if (phy_info.data_path_dir == bcmplpDataPathDirectionIngress) {
        regaddr |= ((bcmplpDataPathDirectionIngress) << 28);
    }
#endif
#if defined(PHYMOD_AGERA2_SUPPORT)
    if (phy_info.data_path_dir == bcmplpDataPathDirectionEgress) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= ((bcmplpDataPathDirectionEgress) << 28);
    }
    if (phy_info.data_path_dir == bcmplpDataPathDirectionIngress) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= ((bcmplpDataPathDirectionIngress) << 28);
    }
#endif
#if defined(PHYMOD_SEAHAWKS_SUPPORT)
    if (phy_info.flags & BCM_PLP_SGMII_REG_ACCESS_MODE) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_SGMII_REG_ACCESS_MODE;
    }
#endif

    rv = plp_aperta_phymod_phy_reg_read(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, data);

#if defined(PHYMOD_AGERA2_SUPPORT)
    if (phy_info.data_path_dir == bcmplpDataPathDirectionEgress) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= ~((bcmplpDataPathDirectionEgress) << 28);
    }
    if (phy_info.data_path_dir == bcmplpDataPathDirectionIngress) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= ~((bcmplpDataPathDirectionIngress) << 28);
    }
#endif
#if defined(PHYMOD_SEAHAWKS_SUPPORT)
    if (phy_info.flags & BCM_PLP_SGMII_REG_ACCESS_MODE) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= ~(BCM_PLP_SGMII_REG_ACCESS_MODE);
    }
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Polarity set
 *
 *  This API is used to set the polarity of the specified lane 
 *
 *  @param phy_info        Represents PHY access\n
 *  @param tx_pol          Tx Polarity\n
 *                         Polarity bitmaps. The bitmap refers to number of lanes in access structure. 
 *                         For example if the structure contains 2 lanes (at any position), two first bits of these bitmaps are relevant.
 *  @param rx_pol          Rx Polarity\n
 *                         Polarity bitmaps. The bitmap refers to number of lanes in access structure. 
 *                         For example if the structure contains 2 lanes (at any position), two first bits of these bitmaps are relevant.
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_polarity_set(bcm_plp_access_t phy_info,  unsigned int tx_pol, unsigned int rx_pol)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_polarity_t phymod_polarity;
 
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    
    phymod_polarity.rx_polarity = rx_pol;
    phymod_polarity.tx_polarity = tx_pol;

    rv = plp_aperta_phymod_phy_polarity_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phymod_polarity); 

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Polarity get 
 *
 *  This API is used to get the polarity of a specified lane
 *          
 * @param phy_info         Represents PHY access\n
 *  @param tx_pol          [OUT] Tx polarity\n
 *                         Polarity bitmaps. The bitmap refers to number of lanes in access structure. 
 *                         For example if the structure contains 2 lanes (at any position), two first bits of these bitmaps are relevant.
 *  @param rx_pol          [OUT] Rx Polarity \n
 *                         Polarity bitmaps. The bitmap refers to number of lanes in access structure. 
 *                         For example if the structure contains 2 lanes (at any position), two first bits of these bitmaps are relevant.
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_polarity_get(bcm_plp_access_t phy_info,  unsigned int *tx_pol, unsigned int *rx_pol)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_polarity_t phymod_polarity;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_polarity_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phymod_polarity);

    *rx_pol = phymod_polarity.rx_polarity;
    *tx_pol = phymod_polarity.tx_polarity;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Get RX PMD lock  
 *
 *  This API is used to get RX PMD live lock status 
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param rx_pmd_locked   [OUT]Status of Rx PMD, When lane map is multicast, 
 *	                       rx_pmd_locked consists 'AND' of all the lane status. </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_rx_pmd_lock_get(bcm_plp_access_t phy_info,  unsigned int* rx_pmd_locked)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    uint32_t rx_pmd_lock = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    
    rv =  plp_aperta_phymod_phy_rx_pmd_locked_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &rx_pmd_lock);

    *rx_pmd_locked = rx_pmd_lock;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Power Set 
 *
 *	This API is used to set the power of a transmitter or receiver of the specified lane.
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param power_rx        Represent Rx power  \n
 *                         0 - PowerOff \n
 *                         1 - PowerOn \n
 *                         2 - PowerOffOn\n
 *                         3 - PowerNoChange, 
 *  @param power_tx        Represent Tx power \n
 *                         0 - PowerOff \n
 *                         1 - PowerOn \n
 *                         2 - PowerOffOn\n
 *                         3 - PowerNoChange, </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_power_set(bcm_plp_access_t phy_info,  unsigned int power_rx, unsigned int power_tx)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_phy_power_t phymod_power;
 
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    
    phymod_power.rx = power_rx;
    phymod_power.tx = power_tx;

    rv = plp_aperta_phymod_phy_power_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phymod_power);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Power get 
 *
 *	This API is used to get the power status of a specified lane from
 *	specified interface side  
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param power_rx        [OUT] Receiver power status\n
 *                         0 - PowerOff \n
 *                         1 - PowerOn 
 *  @param power_tx        [OUT] Transmitter power status\n
 *                         0 - PowerOff \n
 *                         1 - PowerOn </pre>
 * 
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_power_get(bcm_plp_access_t phy_info,  unsigned int *power_rx, unsigned int *power_tx)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_phy_power_t phymod_power;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    
    rv = plp_aperta_phymod_phy_power_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phymod_power);

    *power_rx = phymod_power.rx;
    *power_tx = phymod_power.tx; 

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Chip revision ID 
 *
 *  This API is used to retrieve revision id of the specified PHY 
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param rev_id          [OUT] Chip Revision ID </pre>
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_rev_id(bcm_plp_access_t phy_info,  unsigned int* rev_id)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    uint32_t chip_rev_id = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv =  plp_aperta_phymod_phy_rev_id(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &chip_rev_id);

    *rev_id = chip_rev_id;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Loopback set
 *
 *  This API is used to set the Remote or Digital loopback
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param lb_mode         Represents loopback mode.  See bcm_plp_loopback_mode_t for further usage info\n
 *                         1 - Digital PMD loopback
 *                         2 - Remote PMD loopback \n
 *                         3 - Analog internal loopback \n
 *  @param enable          Represents Enable/Disable\n
 *                         0 - disable\n
 *                         1 - enable </pre>
 *
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_loopback_set(bcm_plp_access_t phy_info, unsigned int lb_mode, unsigned int enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

#if defined (PHYMOD_XGBASET_SUPPORT)
    /* 10G Base-T PHYs don't do the conversion, lb_mode indicates the speed mode */
#elif defined(PHYMOD_PHY542XX_SUPPORT)
    /* 1G Base-T only supports PHY and Remote Loopback */
    lb_mode = (lb_mode == (unsigned int) bcmplpLpbkModeRemoteLoopback ) ? phymodLoopbackRemotePMD :
              (lb_mode == (unsigned int) bcmplpLpbkModeDigitalLoopback) ? phymodLoopbackGlobalPMD : lb_mode;
#else
    lb_mode = (lb_mode == (unsigned int) bcmplpLpbkModeAnalogInternalLoopback) ? phymodLoopbackAnalogInternal :
              (lb_mode == (unsigned int) bcmplpLpbkModeRemoteLoopback        ) ? phymodLoopbackRemotePMD :
              (lb_mode == (unsigned int) bcmplpLpbkModeDigitalLoopback       ) ? phymodLoopbackGlobalPMD : lb_mode;
#endif

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_loopback_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (plp_aperta_phymod_loopback_mode_t)lb_mode, enable);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Loopback get 
 *
 *  This API is used to get the status of specified loopback whether or not enabled
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param lb_mode         Represents loopback mode.  See bcm_plp_loopback_mode_t for further usage info\n
 *                         1 - Digital PMD loopback
 *                         2 - Remote PMD loopback \n
 *                         3 - Analog internal loopback \n
 *  @param enable [OUT]    Represents whether specified loopback is enabled or not.\n
 *                         0 - Loopback disabled\n
 *                         1 - Loopback enabled </pre>
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_loopback_get(bcm_plp_access_t phy_info,  unsigned int lb_mode, unsigned int *enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    *enable = 0;

#if defined(PHYMOD_XGBASET_SUPPORT)
    /* 10G Base-T PHYs don't do the conversion, lb_mode indicates the speed mode */
#else
    lb_mode = (lb_mode == (unsigned int) bcmplpLpbkModeAnalogInternalLoopback) ? phymodLoopbackAnalogInternal :
              (lb_mode == (unsigned int) bcmplpLpbkModeRemoteLoopback        ) ? phymodLoopbackRemotePMD :
              (lb_mode == (unsigned int) bcmplpLpbkModeDigitalLoopback       ) ? phymodLoopbackGlobalPMD : lb_mode;
#endif

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_loopback_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (plp_aperta_phymod_loopback_mode_t) lb_mode, enable);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Set Tx analog values
 *
 *  This API is used to set Transmitter pre, main, post, post2 and post3 taps.
 *  It also set Tx current.
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param tx              Transmitter analog parameter\n
 *                          pre       Pretap value \n
 *                          main      Maintap value\n
 *                          post      Posttap value\n
 *                          post2     Post2tap value\n
 *                          post3     Post3tap value\n
 *                          amp       current value\n </pre>
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_tx_set(bcm_plp_access_t phy_info,  bcm_plp_tx_t* tx)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_tx_t tx_param;
    PHYMOD_MEMSET(&tx_param, 0, sizeof(plp_aperta_phymod_tx_t));
    tx_param.pre = tx->pre;
    tx_param.main = tx->main;
    tx_param.post = tx->post;
    tx_param.post2 = tx->post2;
    tx_param.post3 = tx->post3;
    tx_param.amp = tx->amp; 
#if defined(PHYMOD_XGBASET_SUPPORT)
    tx_param.serdes_speed = tx->serdes_speed;
#endif

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_tx_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &tx_param);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Get Tx analog values
 *
 *	This API is used to get Transmitter pre, main, post, post2 and post3 taps.
 *	It also get Tx current.
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param tx              [OUT]   Transmitter analog parameter 
 *                          pre       Pretap value \n
 *                          main      Maintap value\n
 *                          post      Posttap value\n
 *                          post2     Post2tap value\n
 *                          post3     Post3tap value\n
 *                          amp       current value\n </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_tx_get(bcm_plp_access_t phy_info,  bcm_plp_tx_t* tx)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_tx_t tx_param;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_tx_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &tx_param); 
   
    tx->pre = tx_param.pre;
    tx->main = tx_param.main;
    tx->post = tx_param.post;
    tx->post2 = tx_param.post2;
    tx->post3 = tx_param.post3;
    tx->amp = tx_param.amp;
 
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

 
/*! \brief Set PAM4 Tx analog values
 *
 *	This API is used to set Transmitter pre, main, post, post2 and post3 taps.
 *	It also set Tx current.
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param tx              Transmitter analog parameter\n
 *                          pre       Pretap value \n
 *                          main      Maintap value\n
 *                          post      Posttap value\n
 *                          post2     Post2tap value\n
 *                          post3     Post3tap value\n
 *                          amp       current value\n </pre>
 *                          drivermode drivermode value\n
 *                          pre2      Pre2tap value \n
 *                          serdes_tx_tap_mode Serdes tx tap mode.\n
 *                          tx_precode Independent mode, TX PRECODE option \n
 *                          pre3      Pre3tap value \n
 *                          tx_symbol_swap    Tx symbol swap for PAM4 mode: 0- No swap  1- Swap \n
 *                          tx_graycode       Tx gray code enable for PAM4 mode: 0- Disable; 1- Enable \n
 *
 *                         bcm_plp_serdes_tx_tap_mode_t  This enum is applicable only for PAM4 Supported Chips \n
 *                          bcmplpTapModeDefault        This default mode supports only for legacy phys \n
 *                          bcmplpTapModeNRZ_LP_3TAP    This mode configures the pre,main and post taps \n
 *                          bcmplpTapModeNRZ_6TAP       This mode configures the pre2,pre,main,post,post2 and post3 taps \n
 *                          bcmplpTapModePAM4_LP_3TAP   This mode configures the pre,main and post taps \n
 *                          bcmplpTapModePAM4_6TAP      This mode configures the pre,main and post taps \n
 *
 *                         bcm_plp_indep_tx_precode_t  This enum is applicable only for DSP SerDes \n
 *                          bcmplpIndepTxPrecodeDefault Chip default \n
 *                          bcmplpIndepTxPrecodeOff     Disable the Independent mode, TX PRECODE option \n
 *                          bcmplpIndepTxPrecodeOn      Enable the Independent mode, TX PRECODE option \n
 *
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_pam4_tx_set(bcm_plp_access_t phy_info,  bcm_plp_pam4_tx_t* tx)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_pam4_tx_t tx_param;

    PHYMOD_IF_MEMSET(&tx_param, 0, sizeof(phymod_pam4_tx_t));
    tx_param.pre = tx->pre;
    tx_param.main = tx->main;
    tx_param.post = tx->post;
    tx_param.post2 = tx->post2;
    tx_param.post3 = tx->post3;
    tx_param.amp = tx->amp; 
    tx_param.pre2 = tx->pre2;
    tx_param.pre3 = tx->pre3;
    tx_param.drivermode = tx->drivermode;
    /* coverity[mixed_enums] */
    tx_param.serdes_tx_tap_mode = tx->serdes_tx_tap_mode;
#if defined(PHYMOD_ESTOQUE_SUPPORT) || defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined(PHYMOD_AGERA_SUPPORT)\
    || defined(PHYMOD_AGERALITE_SUPPORT) || defined (PHYMOD_AGERA2_SUPPORT)
    /* coverity[mixed_enums] */
    tx_param.tx_precode = tx->tx_precode;
#endif

    tx_param.tx_symbol_swap = tx->tx_symbol_swap ;
    tx_param.tx_graycode = tx->tx_graycode ;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_pam4_tx_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &tx_param); 
    if (rv != PHYMOD_E_NONE) {
        goto ERR;
    }
#if defined (PHYMOD_APERTA2_SUPPORT)
    {
        plp_aperta_phymod_tx_override_t tx_pi_freq_override;
        PHYMOD_MEMCPY(&tx_pi_freq_override.phase_interpolator, &tx->tx_pi_freq_override, sizeof(bcm_plp_value_override_t)); 
        rv = plp_aperta_phymod_phy_tx_override_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &tx_pi_freq_override);
    }
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Get PAM4 Tx analog values
 *
 *	This API is used to get Transmitter pre, main, post, post2 and post3 taps.
 *	It also get Tx current.
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param tx               Transmitter analog parameter 
 *                          [OUT]pre                     Pretap value \n
 *                          [OUT]main                    Maintap value\n
 *                          [OUT]post                    Posttap value\n
 *                          [OUT]post2                   Post2tap value\n
 *                          [OUT]post3                   Post3tap value\n
 *                          [OUT]amp                     current value\n </pre>
 *                          [OUT]drivermode              drivermode value\n
 *                          [OUT]pre2                    Pre2tap value \n
 *                          [IN/OUT] serdes_tx_tap_mode  Serdes tx tap mode which provides correct\n
 *                                   tap mode in case input is different from chip configured mode.\n
 *                          [OUT]tx_precode              Independent mode, TX PRECODE option \n
 *                          [OUT]pre3                    Pre3tap value \n
 *                          [OUT]tx_symbol_swap          Tx symbol swap for PAM4 mode: 0- No swap  1- Swap \n
 *                          [OUT]tx_graycode             Tx gray code enable for PAM4 mode: 0- Disable; 1- Enable \n
 *
 *                         bcm_plp_serdes_tx_tap_mode_t  This enum is applicable only for PAM4 Supported Chips \n
 *                          bcmplpTapModeDefault         This default mode supports only for legacy phys \n
 *                          bcmplpTapModeNRZ_LP_3TAP     This mode configures the pre,main and post taps \n
 *                          bcmplpTapModeNRZ_6TAP        This mode configures the pre2,pre,main,post,post2 and post3 taps \n
 *                          bcmplpTapModePAM4_LP_3TAP    This mode configures the pre,main and post taps \n
 *                          bcmplpTapModePAM4_6TAP       This mode configures the pre,main and post taps \n
 *
 *                         bcm_plp_indep_tx_precode_t    This enum is applicable only for DSP SerDes \n
 *                          bcmplpIndepTxPrecodeDefault  Chip default \n
 *                          bcmplpIndepTxPrecodeOff      Disable the Independent mode, TX PRECODE option \n
 *                          bcmplpIndepTxPrecodeOn       Enable the Independent mode, TX PRECODE option \n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_pam4_tx_get(bcm_plp_access_t phy_info,  bcm_plp_pam4_tx_t* tx)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_pam4_tx_t tx_param;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_IF_MEMSET(&tx_param, 0, sizeof(phymod_pam4_tx_t));
    
    /* coverity[mixed_enums] */
    tx_param.serdes_tx_tap_mode = tx->serdes_tx_tap_mode;
    rv = plp_aperta_phymod_phy_pam4_tx_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &tx_param); 
    if (rv != PHYMOD_E_NONE) {
        goto ERR;
    }

    tx->pre = tx_param.pre;
    tx->main = tx_param.main;
    tx->post = tx_param.post;
    tx->post2 = tx_param.post2;
    tx->post3 = tx_param.post3;
    tx->amp = tx_param.amp;
    tx->drivermode = tx_param.drivermode;
    tx->pre2 = tx_param.pre2;
    tx->pre3 = tx_param.pre3;
    /* coverity[mixed_enums] */
    tx->serdes_tx_tap_mode = tx_param.serdes_tx_tap_mode;
#if defined(PHYMOD_ESTOQUE_SUPPORT) || defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined(PHYMOD_AGERA_SUPPORT)\
    || defined(PHYMOD_AGERALITE_SUPPORT) || defined (PHYMOD_AGERA2_SUPPORT)
    /* coverity[mixed_enums] */
    tx->tx_precode = tx_param.tx_precode;
#endif

    tx->tx_symbol_swap = tx_param.tx_symbol_swap;
    tx->tx_graycode = tx_param.tx_graycode;
#if defined (PHYMOD_APERTA2_SUPPORT)
    {
        plp_aperta_phymod_tx_override_t tx_pi_freq_override;
        rv = plp_aperta_phymod_phy_tx_override_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &tx_pi_freq_override);
        PHYMOD_MEMCPY(&tx->tx_pi_freq_override, &tx_pi_freq_override.phase_interpolator, sizeof(bcm_plp_value_override_t)); 
    }
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Set Rx analog values
 *
 *	This API is used to set value of the receiver analog paramters
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param rx              Receiver analog parameter\n
 *                         vga->enable                       vga->value will be programmed only if enable is set
 *                         0 - disable; 1 - enable\n
 *                         vga->value                        Value to be set to vga\n
 *                         num_of_dfe_taps                   Number of DFE to program \n
 *                         dfe[i]->enable                    This needs to be set for programming dfe[i]->value
 *                         0 - disable; 1 - enable\n
 *                         dfe[i]->value                     Value to set to DFE[i]\n
 *                         peaking_filter->enable            This has to be set for programming peaking_filter->value
 *                         0 - disable; 1 - enable\n
 *                         peaking_filter->value             Value to be programmed to peaking_filter\n
 *                         low_freq_peaking_filter->enable   This needs to be set for programming low_freq_peaking_filter->value
 *                         0 - disable; 1 - enable\n
 *                         low_freq_peaking_filter->value    Value to be set for low_freq_peaking_filter </pre> \n
 *                         high_freq_peaking_filter->enable  This needs to be set for programming high_freq_peaking_filter->value
 *                         0 - disable; 1 - enable\n
 *                         high_freq_peaking_filter->value   Value to be set for high_freq_peaking_filter  \n
 *                         ffe1->enable                      This needs to be set for programming ffe1->value.
 *                         0 - disable; 1 - enable\n
 *                         ffe1->value                       Value to be set for Feed Forward Equalization1  \n
 *                         ffe2->enable                      This needs to be set for programming ffe2->value.
 *                         0 - disable; 1 - enable\n
 *                         ffe2->value                       Value to be set for Feed Forward Equalization2  \n
 *                         kp_sweep->enable                  This needs to be set for programming kp_sweep->value.
 *                         0 - disable; 1 - enable\n
 *                         kp_sweep->value                   Enable KP sweep for Copper and PAM4 mode\n
 *                         0 - disable; 1 - enable\n
 *                         fga->enable                       This needs to be set for programming fga->value.
 *                         0 - disable; 1 - enable\n
 *                         fga->value                        This needs to be set for FGA (Fixed Gain Amplifier)\n
 *                         rx_symbol_swap                    Rx symbol swap for PAM4 mode: 0- No swap  1- Swap \n
 *                         rx_graycode                       Rx gray code enable for PAM4 mode: 0- Disable; 1- Enable \n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_rx_set(bcm_plp_access_t phy_info,  bcm_plp_rx_t* rx)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    uint32_t index = 0;
    plp_aperta_phymod_rx_t rx_param;
    PHYMOD_IF_MEMSET(&rx_param, 0, sizeof(plp_aperta_phymod_rx_t));
    rx_param.vga.enable = rx->vga.enable;
    rx_param.vga.value = rx->vga.value;
    rx_param.num_of_dfe_taps = rx->num_of_dfe_taps;
    for(index = 0; index < rx_param.num_of_dfe_taps; index++) {
        rx_param.dfe[index].enable = rx->dfe[index].enable;
        rx_param.dfe[index].value = rx->dfe[index].value;
    }
    rx_param.peaking_filter.enable = rx->peaking_filter.enable;
    rx_param.peaking_filter.value =  rx->peaking_filter.value;
    rx_param.low_freq_peaking_filter.enable = rx->low_freq_peaking_filter.enable;
    rx_param.low_freq_peaking_filter.value = rx->low_freq_peaking_filter.value;
    rx_param.high_freq_peaking_filter.enable = rx->high_freq_peaking_filter.enable;
    rx_param.high_freq_peaking_filter.value = rx->high_freq_peaking_filter.value;
    rx_param.ffe1.enable = rx->ffe1.enable;
    rx_param.ffe1.value = rx->ffe1.value;
    rx_param.ffe2.enable = rx->ffe2.enable;
    rx_param.ffe2.value = rx->ffe2.value;
    rx_param.kp_sweep.enable = rx->kp_sweep.enable;
    rx_param.kp_sweep.value = rx->kp_sweep.value;
    rx_param.fga.enable = rx->fga.enable;
    rx_param.fga.value = rx->fga.value;
    rx_param.rx_symbol_swap = rx->rx_symbol_swap ;
    rx_param.rx_graycode = rx->rx_graycode ;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_rx_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &rx_param);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Get Rx analog values
 *
 *	This API is used to get value of the receiver analog paramters
 *
 * @param phy_info <pre> Represents PHY access\n
 *  @param rx              [OUT] Receiver analog parameter 
 *                         vga->enable               
 *                         0 - disable; 1 - enable\n
 *                         vga->value                       VGA value progarammed\n
 *                         num_of_dfe_taps                  Number of DFE to read the configuration \n
 *                         dfe[i]->enable            
 *                         0 - disable; 1 - enable\n
 *                         dfe[i]->value                    Value set in DFE[i]\n
 *                         peaking_filter->enable    
 *                         0 - disable; 1 - enable\n
 *                         peaking_filter->value            Value programmed in peaking_filter\n
 *                         low_freq_peaking_filter->enable 
 *                         0 - disable; 1 - enable\n
 *                         low_freq_peaking_filter->value   Value  programmed in low_freq_peaking_filter</pre> 
 *                         high_freq_peaking_filter->enable This needs to be set for programming high_freq_peaking_filter->value
 *                         0 - disable; 1 - enable\n
 *                         high_freq_peaking_filter->value  Value to be set for high_freq_peaking_filter  \n
 *                         ffe1->enable                     This needs to be set for programming ffe1->value.
 *                         0 - disable; 1 - enable\n
 *                         ffe1->value                      Value to be set for Feed Forward Equalization1  \n
 *                         ffe2->enable                     This needs to be set for programming ffe2->value.
 *                         0 - disable; 1 - enable\n
 *                         ffe2->value                      Value to be set for Feed Forward Equalization2  \n
 *                         0 - disable; 1 - enable\n
 *                         kp_sweep->enable                 This needs to be set for programming kp_sweep->value.
 *                         0 - disable; 1 - enable\n
 *                         kp_sweep->value                  Enable PAM4 KP sweep for Copper and PAM4 mode\n
 *                         0 - disable; 1 - enable\n
 *                         fga->enable
 *                         0 - disable; 1 - enable\n
 *                         fga->value                       Value programmed in FGA (Fixed Gain Amplifier) \n
 *                         rx_symbol_swap                   Rx symbol swap for PAM4 mode: 0- No swap  1- Swap \n
 *                         rx_graycode                      Rx gray code enable for PAM4 mode: 0- Disable; 1- Enable \n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_rx_get(bcm_plp_access_t phy_info,  bcm_plp_rx_t* rx)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_rx_t rx_param;
    uint32_t index = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_IF_MEMSET(&rx_param, 0, sizeof(plp_aperta_phymod_rx_t));
    rv = plp_aperta_phymod_phy_rx_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &rx_param);   
    if(!rv) {
        rx->vga.enable = rx_param.vga.enable;
        rx->vga.value = rx_param.vga.value;
        rx->num_of_dfe_taps = rx_param.num_of_dfe_taps;
     
        for(index = 0; index < rx_param.num_of_dfe_taps; index++) {
            rx->dfe[index].enable = rx_param.dfe[index].enable;
            rx->dfe[index].value = rx_param.dfe[index].value;
        }

        rx->peaking_filter.enable = rx_param.peaking_filter.enable;
        rx->peaking_filter.value = rx_param.peaking_filter.value;
        rx->low_freq_peaking_filter.enable = rx_param.low_freq_peaking_filter.enable;
        rx->low_freq_peaking_filter.value = rx_param.low_freq_peaking_filter.value;
        rx->high_freq_peaking_filter.enable = rx_param.high_freq_peaking_filter.enable;
        rx->high_freq_peaking_filter.value = rx_param.high_freq_peaking_filter.value;
        rx->ffe1.enable = rx_param.ffe1.enable;
        rx->ffe1.value = rx_param.ffe1.value;
        rx->ffe2.enable = rx_param.ffe2.enable;
        rx->ffe2.value = rx_param.ffe2.value;
        rx->kp_sweep.enable = rx_param.kp_sweep.enable;
        rx->kp_sweep.value = rx_param.kp_sweep.value;
        rx->fga.enable = rx_param.fga.enable;
        rx->fga.value = rx_param.fga.value;
        rx->rx_symbol_swap = rx_param.rx_symbol_swap;
        rx->rx_graycode = rx_param.rx_graycode;
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Chip Reset
 *
 *	This API is used to do Hard/Soft reset on the specified PHY.
 *  Hard reset erases firmware along with the contents of PHY registers.
 *  Soft reset preserves firmware and erases the contents of PHY registers.
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param reset_mode      Reset modes\n
 *                         0 - Hard reset (not supported by all PHYs)\n
 *                         1 - Soft reset 
 *  @param reset_direction Reserved for future use </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_reset_set(bcm_plp_access_t phy_info,  unsigned int reset_mode, unsigned int reset_direction)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_CORE_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_core_reset_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core->pm_core, reset_mode, reset_direction); 

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief AFE Datapath Reset 
 *
 *  This API is used to reset Tx/Rx Datapath. Perlane datapath also supported.
 *
 * @param phy_info <pre> Represents PHY access\n
 * @param reset           TX/RX Reset direction, 
 *                         bcm_plp_pm_phy_reset_t\n
 *                         0 - In\n
 *                         1 - Out\n
 *                         2 - In Out (toggle)</pre>
 *
 * @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_phy_lane_reset_set(bcm_plp_access_t phy_info,  bcm_plp_pm_phy_reset_t* reset)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_phy_reset_t rst;
    rst.tx = reset->tx; 
    rst.rx = reset->rx; 
    
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_reset_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &rst);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Datapath Reset get
 *
 *  This API is used to get the reset that are programmmed using bcm_pm_if_phy_lane_reset_set
 *  
 * @param phy_info <pre> Represents PHY access\n
 * @param reset           [OUT] TX/RX Reset direction\n
 *                         bcm_pm_reset_direction_t
 *                         0 - In\n
 *                         1 - Out\n
 *                         2 - In Out (toggle) </pre>
 *
 * @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_phy_lane_reset_get(bcm_plp_access_t phy_info,  bcm_plp_pm_phy_reset_t* reset)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_phy_reset_t rst;
    
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_reset_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &rst);
    reset->tx = rst.tx;
    reset->rx = rst.rx;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Lane control set 
 *
 *	This API is used to perform Tx datapath reset/Traffic disable/Squelch for a specified lane and
 *  for a specified interface side. (system or line side) 
 *
 *  @param phy_info  <pre> Represents PHY access\n
 * @param tx_control       Represet Tx lane control structure \n
 *                         struct bcm_pm_phy_tx_lane_control_t,\n
 *                         0 - Traffic disable  \n
 *                         1 - Traffic enable \n
 *                         2 - Tx Datapath reset\n  
 *                         3 - Tx Squelch on\n
 *                         4 - Tx Squelch off\n </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_tx_lane_control_set(bcm_plp_access_t phy_info,  bcm_pm_phy_tx_lane_control_t tx_control)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    /* coverity[mixed_enums] */
    rv = plp_aperta_phymod_phy_tx_lane_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, tx_control);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}


/*! \brief Lane control get
 *
 *  This API is used to get the Tx datapath reset /Traffic disable/Squelch for a specified lane and
 *  for a specified interface side. (system or line side)
 *
 *  @param phy_info  <pre> Represents PHY access\n
 * @param tx_control      [IN/OUT] Pass Tx lane control structure and retrieve the correct status\n
 *                         struct bcm_pm_phy_tx_lane_control_t,\n
 *                         0 - Traffic disable\n
 *                         1 - Traffic enable\n
 *                         2 - Tx Datapath reset\n
 *                         3 - Tx Squelch on\n
 *                         4 - Tx Squelch off </pre>
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int
bcm_plp_aperta_tx_lane_control_get(bcm_plp_access_t phy_info,  bcm_pm_phy_tx_lane_control_t *tx_control)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_phy_tx_lane_control_t tx_ctrl = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    
    /* coverity[mixed_enums] */
    tx_ctrl = *tx_control;
    rv = plp_aperta_phymod_phy_tx_lane_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &tx_ctrl);
    /* coverity[mixed_enums] */
    *tx_control = tx_ctrl;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Rx Lane control set 
 *
 *	This API is used to perform Rx datapath reset /Traffic disable/Squelch for a specified lane and
 *  for a specified interface side. (system or line side) 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param rx_control      Represents Rx lane control struct\n
 *	                       struct bcm_pm_phy_rx_lane_control_t\n
 *                         0 - Rx datapath reset \n
 *                         1 - Rx squelch on\n
 *                         2 - Rx squelch off </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_rx_lane_control_set(bcm_plp_access_t phy_info,  bcm_pm_phy_rx_lane_control_t rx_control)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_rx_lane_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, rx_control);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}


/*! \brief Rx Lane control set
 *
 *  This API is used to perform Rx datapath reset /Traffic disable/Squelch for a specified lane and
 *  for a specified interface side. (system or line side)
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param rx_control      [OUT]Represents Rx lane control struct\n
 *                         struct bcm_pm_phy_rx_lane_control_t\n
 *                         0 - Rx datapath reset\n
 *                         1 - Rx squelch on\n
 *                         2 - Rx squelch off </pre>
 *                         [IN] Select Rx squelch on (1) or off(2) value to select squelch status 
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int
bcm_plp_aperta_rx_lane_control_get(bcm_plp_access_t phy_info,  bcm_pm_phy_rx_lane_control_t *rx_control)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_phy_rx_lane_control_t rx_ctrl = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    
    rx_ctrl = *rx_control;
    rv = plp_aperta_phymod_phy_rx_lane_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &rx_ctrl);
    *rx_control = rx_ctrl;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Perform cross switch map 
 *
 *	This API is used to perform cross switch mapping between the specified source and destination lane 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param tx_source_array     Represent Tx lane of specified side to be mapped to Rx of the other side </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_lane_cross_switch_map_set(bcm_plp_access_t phy_info, unsigned int* tx_source_array)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_lane_cross_switch_map_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, tx_source_array);    
    
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;

}

/*! \brief Get cross switch map 
 *
 *	This API is used to retrieve lane number to which the specified lane is mapped. 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param tx_source_array     Represent Tx lane of specified side to be mapped to Rx of the other side </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_lane_cross_switch_map_get(bcm_plp_access_t phy_info,  unsigned int* tx_source_array)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_lane_cross_switch_map_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, tx_source_array);    
    
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

#ifdef BCM_PLP_BASE_T_PHY

/*! \brief BASE-T Auto Negotiation Ability set 
 *
 *  This API is used to set BASE-T Auto Negotiation ability
 *  @param phy_info                 Represents PHY access
 *  @param ability                  Points to the structure containing ability information\n
 *  ability.speed_half_duplex :     Mask for setting speed abilities of half duplex mode\n
 *                                  (See bcm_plp_base_t_speed_t in common defines header file for all the options available)\n
 *  ability.speed_full_duplex :     Mask for setting speed abilities of full duplex mode\n
 *                                  (See bcm_plp_base_t_speed_t in common defines header file for all the options available)\n
 *  ability.pause             :     Mask for setting pause abilities\n
 *                                  (See bcm_plp_base_t_pause_t in common defines header file for all the options available)\n
 *  ability.eee               :     Mask for setting EEE abilities\n
 *                                  (See bcm_plp_base_t_eee_ability_t in common defines header file for all the options available)\n
 *  ability.speed_2pair       :     Mask for setting 2-pair ethernet mode abilities\n
 *                                  (See bcm_plp_base_t_speed_t in common defines header file for all the options available)\n
 *  ability.an_side           :     Not supported in bcm_plp_base_t_autoneg_ability_set\n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_base_t_autoneg_ability_set(bcm_plp_access_t phy_info, bcm_plp_base_t_ability_t *ability)
{
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_autoneg_ability_t  pm_ability;
    uint32_t  phy_id_idx = BCM_PM_IF_MAX_PHY, exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    PHYMOD_IF_MEMSET(&pm_ability, 0, sizeof(plp_aperta_phymod_autoneg_ability_t));
    pm_ability.speed_full_duplex =  ability->speed_full_duplex;
    pm_ability.speed_half_duplex =  ability->speed_half_duplex;
    pm_ability.eee               =  ability->eee;
    pm_ability.pause             =  ability->pause;
    pm_ability.speed_2pair       =  ability->speed_2pair;
    rv = plp_aperta_phymod_phy_autoneg_ability_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &pm_ability);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief BASE-T Auto Negotiation Ability get
 *
 *  This API is used to get BASE-T Auto Negotiation ability
 *  @param phy_info               Represents PHY access
 *  @param ability                Points to the structure containing ability information\n
 *  ability.speed_half_duplex  :  Current speed abilities of half duplex mode\n
 *                                (See bcm_plp_base_t_speed_t in common defines header file for all the options available)\n
 *  ability.speed_full_duplex  :  Current speed abilities of full duplex mode\n
 *                                (See bcm_plp_base_t_speed_t in common defines header file for all the options available)\n
 *  ability.pause              :  Current pause abilities\n
 *                                (See bcm_plp_base_t_pause_t in common defines header file for all the options available)\n
 *  ability.eee                :  Current EEE abilities\n
 *                                (See bcm_plp_base_t_eee_ability_t in common defines header file for all the options available)\n
 *  ability.speed_2pair        :  Current 2-pair ethernet mode abilities\n
 *                                (See bcm_plp_base_t_speed_t in common defines header file for all the options available)\n
 *  ability.an_side            :  Auto negotiation side, 0 -> Get local AN ability, 1 -> Get remote AN ability\n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_base_t_autoneg_ability_get(bcm_plp_access_t phy_info, bcm_plp_base_t_ability_t *ability)
{
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_autoneg_ability_t  pm_ability;
    uint32_t  phy_id_idx = BCM_PM_IF_MAX_PHY, exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    if (ability->an_side == 1) {
        rv = plp_aperta_phymod_phy_autoneg_remote_ability_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &pm_ability);
    } else {
        rv = plp_aperta_phymod_phy_autoneg_ability_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &pm_ability);
    }
    ability->speed_full_duplex =  pm_ability.speed_full_duplex;
    ability->speed_half_duplex =  pm_ability.speed_half_duplex;
    ability->eee               =  pm_ability.eee;
    ability->pause             =  pm_ability.pause;
    ability->speed_2pair       =  pm_ability.speed_2pair;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief BASE-T Auto Negotiation Enable/Disable
 *
 *  This API is used to enable/Disable BASE-T Auto Negotiation
 *
 *  @param phy_info       Represents PHY access
 *  @param enable         [IN] Represent enable/disable\n
 *	                           0 = Disable    1 = enable
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_base_t_autoneg_set(bcm_plp_access_t phy_info, int enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_autoneg_control_t  an_ctrl;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY, exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    PHYMOD_MEMSET(&an_ctrl, 0, sizeof(plp_aperta_phymod_autoneg_control_t));
    an_ctrl.enable = enable; 
    
    rv = plp_aperta_phymod_phy_autoneg_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &an_ctrl);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Get BASE-T Auto Negotiation completion state 
 *
 *  This API is used to get BASE-T Auto Negotiation completion state
 *
 *  @param phy_info       Represents PHY access
 *  @param enable          [OUT] Represent whether AN is enabled/disabled\n
 *	                             0 = Disabled    1 = enabled
 *  @param an_done         [OUT] Represent AN done state\n
 *	                             0 = AN in porgress    1 = AN done
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_base_t_autoneg_get(bcm_plp_access_t phy_info, int *enable, int *an_done)
{
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_autoneg_status_t  an_ststus;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY, exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_phy_autoneg_status_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &an_ststus);
    *enable  = an_ststus.enabled;
    *an_done = an_ststus.locked;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

#endif /* BCM_PLP_BASE_T_PHY */

/*! \brief CL73 Auto Negotiation Ability set 
 *
 *  This API is used to set CL73 Auto Negotiation ability
 *  @param phy_info <pre>           Represents PHY access\n
 *  @param tech_ability             Represents 16-bit tech ability\n
 *                                  1 = AN_CAP_1G_KX\n
 *                                  2 = AN_CAP_10G_KX4\n
 *                                  4 = AN_CAP_10G_KR\n
 *                                  8 = AN_CAP_40G_KR4\n
 *                                  0x10 = AN_CAP_40G_CR4\n
 *                                  0x20 = AN_CAP_100G_CR10\n
 *                                  0x40 = AN_CAP_100G_CR4\n
 *                                  0x80 = AN_CAP_100G_KR4\n
 *                                  0x1000 = AN_CAP_25G_KRS1[IEEE]\n
 *                                  0x2000 = AN_CAP_25G_CRS1[IEEE]\n
 *                                  0x4000 = AN_CAP_25G_KR[IEEE]\n
 *                                  0x8000 = AN_CAP_25G_CR[IEEE]\n
 *                                  0x100 = AN_CAP_50G_KR2[Consortium]\n
 *                                  0x200 = AN_CAP_50G_CR2[Consortium]\n
 *                                  0x400 = AN_CAP_25G_KR1[Consortium]\n
 *                                  0x800 = AN_CAP_25G_CR1[Consortium]\n
 *                                  Note 1: for 100M and 10M, set the mode as 100/10M and 
 *                                  pass tech ability as "0"(HARDWARE DEFAULT). 
 *                                  Note 2: For tech ability above 16 bits user needs to
 *                                  use an_config.tech_ability parameter
 *  @param fec_ability              Represent FEC ability\n
 *                                  0x0  = Hardware default \n
 *                                  0x1  = FEC Ability\n
 *                                  0x2  = FEC requested\n
 *                                  0x4  = FEC 91/RS FEC Ability/Capable \n
 *                                  0x8  = FEC 91/RS FEC requested \n
 *                                  0x10 = RS FEC 528
 *                                  0x20 = RS FEC 544
 *                                  0x40 = RS FEC 272
 *                                  0x80 = RS FEC 544_2XN
 *                                  0x100 = RS FEC 272_2XN
 *                                  0x8000 = NONE or NO FEC \n
 *                                  Note: CL74 and CL91 can be requested by OR'ing appropriate bits. This is supported on Evora/Europa family only.
 *  @param pause_ability            Represent pause ability\n
 *                                  0     = Hardware default \n 
 *                                  0x40  = AN_CAPABILITIES_SYMM_PAUSE \n
 *                                  0x80  = AN_CAPABILITIES_ASYM_PAUSE </pre>
 *  @param an_config                This parameter is used for following settings\n
 *  an_config.master_lane           master lane through which Autonegotiation takes place\n
 *                                  value can be between 0 to 11 based on the mode\n 
 *                                     10G - This parameter is not used\n
 *                                     40G PT - master lane can be between 0 to 3\n
 *                                     40G Demux - master lane can be between 0 to 3\n
 *                                     40G Mux - master lane can be between 0 to 1\n
 *                                     100G Gbox - master lane can be between 0 to 3\n
 *                                     100G PT0/ 100G PT1/ 100G PT2 - master lane can be between 0 to 9\n
 *  an_config.cl72_en               Enable or disable CL72 during the process of autonegotiation\n
 *                                     0 - Disable CL72\n
 *                                     1 - Enable CL72\n
 *                                  Note: For some PAM4 chips,\n
 *                                  User needs to construct "an_config.cl72_en" as follows\n
 *                                  bit 3:0 represents enable(1)/disable(0) training\n
 *                                  bit 7:4 represents training frame size, used only in case of PAM4\n
 *                                     0 : 4K Frame\n
 *                                     1 : 16K Frame \n
 *                                  bit 11:8 represents training init condition applicable for both NRZ and PAM4,\n
 *                                     0 : No link training init condition \n
 *                                     1 : Link training init condition enable\n
 *                                  bit 15:12 represents preset type for NRZ mode,\n
 *                                     0: No link training Preset Normal: normal operation\n
 *                                     1: Link training Preset Enable: Preset coefficients \n
 *                                  bit 19:16 represents restart,\n
 *                                     0: No restart\n
 *                                     1: restart training\n
 *                                  bit 23:20 represents Link Training timer timeout duration which is valid only when \n
 *                                  bit 24 set to 0: Enable LT timer
 *                                     0 : Link Training Timer default value: firmware default
 *                                     1 : Link Training Timer value: 500ms
 *                                     2 : Link Training Timer value: 1s
 *                                     3 : Link Training Timer value: 1.5s
 *                                     4 : Link Training Timer value: 2s
 *                                     5 : Link Training Timer value: 2.5s
 *                                     6 : Link Training Timer value: 3s
 *                                     7 : Link Training Timer value: 3.5s
 *                                     8 : Link Training Timer value: 4s
 *                                     9 : Link Training Timer value: 4.5s
 *                                     10: Link Training Timer value: 5s
 *                                     11: Link Training Timer value: 5.5s
 *                                     12: Link Training Timer value: 6s
 *                                     13: Link Training Timer value: 6.5
 *                                     14: Link Training Timer value: 7s
 *                                     15: Link Training Timer value: 7.5s
 *                                  bit 24 : Used to disable the link training timer \n 
 *                                     0: Enable LT timer
 *                                     1: Disable LT timer
 *                                  bit 31:25 : Reserved for future use 
 *
 *  an_config.tech_ability          Represents 32 bit tech ability\n
 *                                  1 = AN_CAP_1G_KX\n
 *                                  2 = AN_CAP_10G_KX4\n
 *                                  4 = AN_CAP_10G_KR\n
 *                                  8 = AN_CAP_40G_KR4\n
 *                                  0x10 = AN_CAP_40G_CR4\n
 *                                  0x20 = AN_CAP_100G_CR10\n
 *                                  0x40 = AN_CAP_100G_CR4\n
 *                                  0x80 = AN_CAP_100G_KR4\n
 *                                  0x1000 = AN_CAP_25G_KRS1[IEEE]\n
 *                                  0x2000 = AN_CAP_25G_CRS1[IEEE]\n
 *                                  0x4000 = AN_CAP_25G_KR[IEEE]\n
 *                                  0x8000 = AN_CAP_25G_CR[IEEE]\n
 *                                  0x100 = AN_CAP_50G_KR2[Consortium]\n
 *                                  0x200 = AN_CAP_50G_CR2[Consortium]\n
 *                                  0x400 = AN_CAP_25G_KR1[Consortium]\n
 *                                  0x800 = AN_CAP_25G_CR1[Consortium]\n
 *                                  0x10000 = AN_CAP_50G_CR_KR\n
 *                                  0x20000 = AN_CAP_100G_CR2_KR2\n
 *                                  0x40000 = AN_CAP_200G_CR4_KR4\n
 *                                  0x80000 = AN_CAP_50G_KP\n
 *                                  0x100000 = AN_CAP_100G_KP2\n
 *                                  0x200000 = AN_CAP_200G_KP4\n
 *                                  0x400000 = AN_CAP_400G_CR8_KR8\n
 *                                  0x800000 = AN_CAP_50G_CR1_KR1[Consortium] \n
 *                                  0x1000000 =  AN_CAP_BAM_50G_CR1_KR1[Proprietary] \n
 *                                  0x2000000 = AN_CAP_BAM_50G_CR2_KR2[Proprietary] \n
 *                                  0x4000000 = AN_CAP_BAM_100G_CR2_KR2[Proprietary] \n
 *                                  0x8000000 = AN_CAP_BAM_100G_CR4_KR4[Proprietary] \n
 *                                  0x10000000 = AN_CAP_BAM_200G_CR4_KR4[Proprietary] \n
 *                                  0x20000000 = AN_CAP_BAM_400G_CR8_KR8[Proprietary] \n
 *  Note : For 32-bit tech ability user has to use an_config.tech_ability parameter
 *
 *  an_config.tech_ability_ext      Represents 32-bit tech ability extension\n
 *                                  0x1    = AN_CAP_BAM_20G_CR1[Proprietary] \n
 *                                  0x2    = AN_CAP_BAM_20G_KR1[Proprietary] \n
 *                                  0x4    = AN_CAP_BAM_20G_CR2[Proprietary] \n
 *                                  0x8    = AN_CAP_BAM_20G_KR2[Proprietary] \n
 *                                  0x10   = AN_CAP_BAM_25G_CR1[Proprietary] \n
 *                                  0x20   = AN_CAP_BAM_25G_KR1[Proprietary] \n
 *                                  0x40   = AN_CAP_BAM_40G_CR2[Proprietary] \n
 *                                  0x80   = AN_CAP_BAM_40G_KR2[Proprietary] \n
 *                                  0x100  = AN_CAP_BAM_50G_CR4[Proprietary] \n
 *                                  0x200  = AN_CAP_BAM_50G_KR4[Proprietary] \n
 *                                  0x400  = AN_CAP_BAM_100G_CR1_KR1[Proprietary] \n
 *                                  0x800  = AN_CAP_100G_CR1_KR1[IEEE] \n
 *                                  0x1000 = AN_CAP_200G_CR2_KR2[IEEE] \n
 *                                  0x2000 = AN_CAP_400G_CR4_KR4[IEEE] \n
 *                                  0x4000 = AN_CAP_MSA_100G_CR2_KR2[Consortium] \n
 *                                  0x8000 = AN_CAP_MSA_200G_CR4_KR4[Consortium] \n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_cl73_ability_set(bcm_plp_access_t phy_info, unsigned short tech_ability,
		           unsigned short fec_ability, unsigned short pause_ability, bcm_plp_an_config_t an_config)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_autoneg_ability_t ability;

    PHYMOD_IF_MEMSET(&ability, 0, sizeof(ability));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

#ifdef BCM_PLP_32BIT_TECH_ABILITY
    PHYMOD_IF_ERR_RETURN(_bcm_plp_aperta_phy_user_phymod_ability((tech_ability | (an_config.tech_ability)), fec_ability, pause_ability, an_config, &ability));
#else
    PHYMOD_IF_ERR_RETURN(_bcm_plp_aperta_phy_user_phymod_ability(tech_ability, fec_ability, pause_ability, an_config, &ability));
#endif

#if !defined(PHYMOD_MILLENIO_SUPPORT) && !defined(PHYMOD_BARCHETTA2_SUPPORT) && !defined(PHYMOD_AGERA_SUPPORT) && !defined (PHYMOD_AGERALITE_SUPPORT) && !defined (PHYMOD_AGERA2_SUPPORT)
    ability.an_fec = fec_ability;
    ability.capabilities = pause_ability;
#endif

    /* CL73 Enabled by default */
    ability.capabilities |= phymod_AN_MODE_CL73;
    ability.an_master_lane = an_config.master_lane;
    ability.an_cl72 = an_config.cl72_en;

    rv = plp_aperta_phymod_phy_autoneg_ability_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &ability);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief CL73 AN Ability get 
 *
 *  This API is used to get configured CL73 Auto Negotiation ability  
 *
 *  @param phy_info <pre>       Represents PHY access\n
 *  @param tech_ability         [OUT] Represent 16-bit tech ability\n
 *                                  1 = AN_CAP_1G_KX\n
 *                                  2 = AN_CAP_10G_KX4\n
 *                                  4 = AN_CAP_10G_KR\n
 *                                  8 = AN_CAP_40G_KR4\n
 *                                  0x10 = AN_CAP_40G_CR4\n
 *                                  0x20 = AN_CAP_100G_CR10\n
 *                                  0x40 = AN_CAP_100G_CR4\n
 *                                  0x80 = AN_CAP_100G_KR4\n
 *                                  0x1000 = AN_CAP_25G_KRS1[IEEE]\n
 *                                  0x2000 = AN_CAP_25G_CRS1[IEEE]\n
 *                                  0x4000 = AN_CAP_25G_KR[IEEE]\n
 *                                  0x8000 = AN_CAP_25G_CR[IEEE]\n
 *                                  0x100 = AN_CAP_50G_KR2[Consortium]\n
 *                                  0x200 = AN_CAP_50G_CR2[Consortium]\n
 *                                  0x400 = AN_CAP_25G_KR1[Consortium]\n
 *                                  0x800 = AN_CAP_25G_CR1[Consortium]
 *  @param fec_ability          [OUT] Represent FEC ability\n
 *                              0x0  = Hardware default\n
 *                              0x1  = FEC Ability\n
 *                              0x2  = FEC requested\n
 *                              0x4  = FEC 91/RS FEC Ability/Capable\n
 *                              0x8  = FEC 91/RS FEC requested \n
 *                              0x10 = RS FEC 528\n
 *                              0x20 = RS FEC 544\n
 *                              0x40 = RS FEC 272\n
 *                              0x80 = RS FEC 544_2xN\n
 *                              0x100 = RS FEC 272_2XN
 *                              0x8000 = NONE or NO FEC \n
 *                              Note: Evora/Europa family returns fec_ability as FEC requested (0x2) if user configures either FEC Ability (0x1) or FEC requested (0x2).
 *                              Similarly, FEC 91/RS FEC requested (0x8) is returned when user configures FEC 91/RS FEC Ability/Capable (0x4) or FEC 91/RS FEC requested (0x8).
 *  @param pause_ability        [OUT] Represent pause ability\n
 *                              0   = Hardware default \n
 *                              0x40  = AN_CAPABILITIES_SYMM_PAUSE \n
 *                              0x80  = AN_CAPABILITIES_ASYM_PAUSE </pre>
 *  @param an_config            This parameter is used to get master lane and CL72 enable/disable status\n 
 *  an_config.master_lane       [OUT] master lane through which Autonegotiation takes place\n
 *                              value can be between 0 to 11 based on the mode\n 
 *                                 10G - This parameter is not used\n
 *                                 40G PT - master lane can be between 0 to 3\n
 *                                 40G Demux - master lane can be between 0 to 3\n
 *                                 40G Mux - master lane can be between 0 to 1\n
 *                                 100G Gbox - master lane can be between 0 to 3\n
 *                                 100G PT0/ 100G PT1/ 100G PT2 - master lane can be between 0 to 9\n
 *  an_config.cl72_en           [OUT] Enable or disable CL72 during the process of autonegotiation\n
 *                                 0 - Disable CL72\n
 *                                 1 - Enable CL72\n
 *                              Note: For PAM4 chips,\n
 *                              User needs to construct "enable" as follows\n
 *                              bit 3:0 represents enable(1)/disable(0) training\n
 *                              bit 7:4 represents training frame size, used only in case of PAM4\n
 *                                     0 : 4K Frame\n
 *                                     1 : 16K Frame \n
 *                              bit 11:8 represents training init condition applicable for both NRZ and PAM4,\n
 *                                     0 : No link training init condition \n
 *                                     1 : Link training init condition enable\n
 *                              bit 15:12 represents preset type for NRZ mode,\n
 *                                     0: No link training Preset Normal: normal operation\n
 *                                     1: Link training Preset Enable: Preset coefficients \n
 *                              bit 19:16 represents restart,\n
 *                                     0: No restart\n
 *                                     1: restart training\n
 *                              bit 23:20 represents Link Training timer timeout duration which is valid only when \n
 *                              bit 24 set to 0: Enable LT timer
 *                                     0 : Link Training Timer default value: firmware default
 *                                     1 : Link Training Timer value: 500ms
 *                                     2 : Link Training Timer value: 1s
 *                                     3 : Link Training Timer value: 1.5s
 *                                     4 : Link Training Timer value: 2s
 *                                     5 : Link Training Timer value: 2.5s
 *                                     6 : Link Training Timer value: 3s
 *                                     7 : Link Training Timer value: 3.5s
 *                                     8 : Link Training Timer value: 4s
 *                                     9 : Link Training Timer value: 4.5s
 *                                     10: Link Training Timer value: 5s
 *                                     11: Link Training Timer value: 5.5s
 *                                     12: Link Training Timer value: 6s
 *                                     13: Link Training Timer value: 6.5
 *                                     14: Link Training Timer value: 7s
 *                                     15: Link Training Timer value: 7.5s
 *                              bit 24 : Used to disable the link training timer \n 
 *                                     0: Enable LT timer
 *                                     1: Disable LT timer
 *                              bit 31:25 : Reserved for future use
 *
 * an_config.tech_ability       [OUT]   Represents 32-bit tech ability\n
 *                                  1 = AN_CAP_1G_KX\n
 *                                  2 = AN_CAP_10G_KX4\n
 *                                  4 = AN_CAP_10G_KR\n
 *                                  8 = AN_CAP_40G_KR4\n
 *                                  0x10 = AN_CAP_40G_CR4\n
 *                                  0x20 = AN_CAP_100G_CR10\n
 *                                  0x40 = AN_CAP_100G_CR4\n
 *                                  0x80 = AN_CAP_100G_KR4\n
 *                                  0x1000 = AN_CAP_25G_KRS1[IEEE]\n
 *                                  0x2000 = AN_CAP_25G_CRS1[IEEE]\n
 *                                  0x4000 = AN_CAP_25G_KR[IEEE]\n
 *                                  0x8000 = AN_CAP_25G_CR[IEEE]\n
 *                                  0x100 = AN_CAP_50G_KR2[Consortium]\n
 *                                  0x200 = AN_CAP_50G_CR2[Consortium]\n
 *                                  0x400 = AN_CAP_25G_KR1[Consortium]\n
 *                                  0x800 = AN_CAP_25G_CR1[Consortium]\n
 *                                  0x10000 = AN_CAP_50G_CR_KR\n
 *                                  0x20000 = AN_CAP_100G_CR2_KR2\n
 *                                  0x40000 = AN_CAP_200G_CR4_KR4\n
 *                                  0x80000 = AN_CAP_50G_KP\n
 *                                  0x100000 = AN_CAP_100G_KP2\n
 *                                  0x200000 = AN_CAP_200G_KP4\n
 *                                  0x400000 = AN_CAP_400G_CR8_KR8\n
 *                                  0x800000 = AN_CAP_50G_CR1_KR1[Consortium] \n
 *                                  0x1000000 =  AN_CAP_BAM_50G_CR1_KR1[Proprietary] \n
 *                                  0x2000000 = AN_CAP_BAM_50G_CR2_KR2[Proprietary] \n
 *                                  0x4000000 = AN_CAP_BAM_100G_CR2_KR2[Proprietary] \n
 *                                  0x8000000 = AN_CAP_BAM_100G_CR4_KR4[Proprietary] \n
 *                                  0x10000000 = AN_CAP_BAM_200G_CR4_KR4[Proprietary] \n
 *                                  0x20000000 = AN_CAP_BAM_400G_CR8_KR8[Proprietary] \n
 *  Note : For 32-bit tech ability user has to use an_config.tech_ability parameter
 *
 *  an_config.tech_ability_ext  [OUT]   Represents 32-bit tech ability extension\n
 *                                  0x1    = AN_CAP_BAM_20G_CR1[Proprietary] \n
 *                                  0x2    = AN_CAP_BAM_20G_KR1[Proprietary] \n
 *                                  0x4    = AN_CAP_BAM_20G_CR2[Proprietary] \n
 *                                  0x8    = AN_CAP_BAM_20G_KR2[Proprietary] \n
 *                                  0x10   = AN_CAP_BAM_25G_CR1[Proprietary] \n
 *                                  0x20   = AN_CAP_BAM_25G_KR1[Proprietary] \n
 *                                  0x40   = AN_CAP_BAM_40G_CR2[Proprietary] \n
 *                                  0x80   = AN_CAP_BAM_40G_KR2[Proprietary] \n
 *                                  0x100  = AN_CAP_BAM_50G_CR4[Proprietary] \n
 *                                  0x200  = AN_CAP_BAM_50G_KR4[Proprietary] \n
 *                                  0x400  = AN_CAP_BAM_100G_CR1_KR1[Proprietary] \n
 *                                  0x800  = AN_CAP_100G_CR1_KR1[IEEE] \n
 *                                  0x1000 = AN_CAP_200G_CR2_KR2[IEEE] \n
 *                                  0x2000 = AN_CAP_400G_CR4_KR4[IEEE] \n
 *                                  0x4000 = AN_CAP_MSA_100G_CR2_KR2[Consortium] \n
 *                                  0x8000 = AN_CAP_MSA_200G_CR4_KR4[Consortium] \n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_cl73_ability_get(bcm_plp_access_t phy_info, unsigned short *tech_ability,
		           unsigned short *fec_ability, unsigned short *pause_ability, bcm_plp_an_config_t* an_config)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_autoneg_ability_t ability;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    
    rv = plp_aperta_phymod_phy_autoneg_ability_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &ability);
    PHYMOD_IF_ERR_RETURN(_bcm_plp_aperta_phy_phymod_user_ability(ability, fec_ability, pause_ability, an_config));
    *tech_ability = (an_config->tech_ability & 0xFFFF);

#if !defined(PHYMOD_MILLENIO_SUPPORT) && !defined(PHYMOD_BARCHETTA2_SUPPORT) && !defined(PHYMOD_AGERA_SUPPORT) && !defined(PHYMOD_AGERALITE_SUPPORT) && !defined (PHYMOD_AGERA2_SUPPORT)
    *fec_ability = ability.an_fec;
    *pause_ability = ability.capabilities;
#endif

    an_config->master_lane = ability.an_master_lane;
    an_config->cl72_en = ability.an_cl72;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief AN Remote Ability get 
 *
 *  This API is used to get link partner AN ability  
 *
 *  @param phy_info <pre>       Represents PHY access\n
 *  @param fec_ability          [OUT] Represent FEC ability\n
 *                              0x0  = Hardware default \n
 *                              0x1  = FEC Ability\n
 *                              0x2  = FEC requested
 *                              0x4  = FEC 91/RS FEC Ability/Capable \n
 *                              0x8  = FEC 91/RS FEC requested \n
 *                              0x10 = RS FEC 528
 *                              0x20 = RS FEC 544
 *                              0x40 = RS FEC 272
 *	                        0x8000 = NONE or NO FEC \n
 *  @param pause_ability        [OUT] Represent pause ability\n
 *                              0   = Hardware default \n
 *                              0x40  = AN_CAPABILITIES_SYMM_PAUSE \n
 *                              0x80  = AN_CAPABILITIES_ASYM_PAUSE </pre>
 *  @param an_config            This parameter is used to get master lane and CL72 enable/disable status\n 
 *  an_config.master_lane        - NA - 
 *  an_config.cl72_en            - NA -
 *  an_config.tech_ability       [OUT]   Represents 32-bit tech ability\n
 *                               1 = AN_CAP_1G_KX\n
 *                               2 = AN_CAP_10G_KX4\n
 *                               4 = AN_CAP_10G_KR\n
 *                               8 = AN_CAP_40G_KR4\n
 *                               0x10 = AN_CAP_40G_CR4\n
 *                               0x20 = AN_CAP_100G_CR10\n
 *                               0x40 = AN_CAP_100G_CR4\n
 *                               0x80 = AN_CAP_100G_KR4\n
 *                               0x1000 = AN_CAP_25G_KRS1[IEEE]\n
 *                               0x2000 = AN_CAP_25G_CRS1[IEEE]\n
 *                               0x4000 = AN_CAP_25G_KR[IEEE]\n
 *                               0x8000 = AN_CAP_25G_CR[IEEE]\n
 *                               0x100 = AN_CAP_50G_KR2[Consortium]\n
 *                               0x200 = AN_CAP_50G_CR2[Consortium]\n
 *                               0x400 = AN_CAP_25G_KR1[Consortium]\n
 *                               0x800 = AN_CAP_25G_CR1[Consortium]\n
 *                               0x10000 = AN_CAP_50G_CR_KR\n
 *                               0x20000 = AN_CAP_100G_CR2_KR2\n
 *                               0x40000 = AN_CAP_200G_CR4_KR4\n
 *                               0x80000 = AN_CAP_50G_KP\n
 *                               0x100000 = AN_CAP_100G_KP2\n
 *                               0x200000 = AN_CAP_200G_KP4\n
 *                               0x400000 = AN_CAP_400G_CR8_KR8\n
 *                               0x800000 = AN_CAP_50G_CR1_KR1[Consortium] \n
 *                               0x1000000 =  AN_CAP_BAM_50G_CR1_KR1[Proprietary] \n
 *                               0x2000000 = AN_CAP_BAM_50G_CR2_KR2[Proprietary] \n
 *                               0x4000000 = AN_CAP_BAM_100G_CR2_KR2[Proprietary] \n
 *                               0x8000000 = AN_CAP_BAM_100G_CR4_KR4[Proprietary] \n
 *                               0x10000000 = AN_CAP_BAM_200G_CR4_KR4[Proprietary] \n
 *                               0x20000000 = AN_CAP_BAM_400G_CR8_KR8[Proprietary] \n
 *                               0x40000000 = CL37_10M \n  
 *                               0x80000000 = CL37_100M \n 
 *
 *  an_config.tech_ability_ext  [OUT]   Represents 32-bit tech ability extension\n
 *                               bcmplpAnCapBAM_20G_CR1 = 0x1 \n
 *                               bcmplpAnCapBAM_20G_KR1 = 0x2 \n
 *                               bcmplpAnCapBAM_20G_CR2 = 0x4 \n
 *                               bcmplpAnCapBAM_20G_KR2 = 0x8 \n
 *                               bcmplpAnCapBAM_25G_CR1 = 0x10 \n
 *                               bcmplpAnCapBAM_25G_KR1 = 0x20 \n
 *                               bcmplpAnCapBAM_40G_CR2 = 0x40 \n
 *                               bcmplpAnCapBAM_40G_KR2 = 0x80 \n
 *                               bcmplpAnCapBAM_50G_CR4 = 0x100 \n
 *                               bcmplpAnCapBAM_50G_KR4 = 0x200 \n
 *                               CL37_1000M_SGMII = 0x40000000 \n
 *                               CL37_1000M = 0x80000000 \n
 *
 *    @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_autoneg_remote_ability_get(bcm_plp_access_t phy_info, unsigned short *fec_ability, 
                               unsigned short *pause_ability, bcm_plp_an_config_t* an_config)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_autoneg_ability_t ability;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_phy_autoneg_remote_ability_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &ability);
    PHYMOD_IF_ERR_RETURN(_bcm_plp_aperta_phy_phymod_user_ability(ability, fec_ability, pause_ability, an_config));
#if !defined(PHYMOD_MILLENIO_SUPPORT) && !defined(PHYMOD_BARCHETTA2_SUPPORT) && !defined (PHYMOD_AGERA2_SUPPORT)
    *fec_ability = ability.an_fec;
    *pause_ability = ability.capabilities;
#endif
    an_config->master_lane = 0;
    an_config->cl72_en = 0;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief CL73 Enable/Disable 
 *
 *  This API is used to enable/Disable CL73   
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param ena_dis         Represent enable/disable\n
 *	                       0 = Disable\n  1 = enable\n  2 = Restart </pre>
 *	Note: To enable CL37 and to set SGMII master mode for Miura, please use 
 *        phy_info flags(BCM_PLP_AN_MODE_CL37, BCM_PLP_CL37_SGMII_MASTER_MODE) 
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_cl73_set(bcm_plp_access_t phy_info, unsigned short ena_dis)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_autoneg_control_t an;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&an, 0, sizeof(an));
    an.an_mode = phymod_AN_MODE_CL73;
    if (phy_info.flags & BCM_PLP_AN_MODE_CL37) {
        an.an_mode = phymod_AN_MODE_CL37;
    }
    if (phy_info.flags & BCM_PLP_CL37_SGMII_MASTER_MODE) {
        an.flags |= (1 << 31) ;
    } else {
        an.flags &= ~(1 << 31) ;
    }

    an.enable = ena_dis; 
    
#if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT)
    if (ena_dis == 2) { /* AN Restart */
        an.enable = 1;
        rv = plp_aperta_phymod_phy_autoneg_restart_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &an);
    } else {
#endif
    rv = plp_aperta_phymod_phy_autoneg_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &an);
#if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT)
    }
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Get CL73 completion state 
 *
 *  This API is used to get CL73 completion state
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param an              [OUT] Represent whether AN is enabled/disabled
 *  @param an_done         [OUT] Represent AN done state</pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_cl73_get(bcm_plp_access_t phy_info,  unsigned int *an, unsigned int *an_done)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_autoneg_control_t an_ctrl;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    
    rv = plp_aperta_phymod_phy_autoneg_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &an_ctrl, an_done);
    *an = an_ctrl.enable;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Eyescan 
 *
 *  This API is used to display eyescan for a given lane
 *  @param phy_info   Represents PHY access\n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
/*  ***************************************************************************\n
 *            Legend of Entries in display_lane_state  header\n
 *  ***************************************************************************\n
 *  \n
 *  LN               : lane index within IP core\n
 *  (CDRxN,UC_CFG)   : CDR type x OSR ratio, micro lane configuration variable\n
 *  SD               : signal detect\n
 *  LOCK             : pmd_rx_lock\n
 *  RXPPM            : Frequency offset of local reference clock with respect to RX data in ppm\n
 *  CLK90            : Delay of zero crossing slicer, m1, wrt to data in PI codes\n
 *  CLKP1            : Delay of diagnostic/lms slicer, p1, wrt to data in PI codes\n
 *  PF(M,L)          : Peaking Filter Main (0..15) and Low Frequency (0..7) settings\n
 *  VGA              : Variable Gain Amplifier settings (0..42)\n
 *  DCO              : DC offset DAC control value\n
 *  P1mV             : Vertical threshold voltage of p1 slicer\n
 *  DFE taps         : ISI correction taps in units of 2.35mV (for 1 & 2 even values are displayed, dcd = even-odd)\n"
 *  SLICER(ze,zo,pe,po,me,mo) : Slicer calibration control codes\n
 *  TXPPM            : Frequency offset of local reference clock with respect to TX data in ppm\n
 *  TXEQ(n1,m,p1,p2) : TX equalization FIR tap weights in units of 1Vpp/60 units\n
 *  EYE(L,R,U,D)     : Eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV\n
 *  LINK_TIME        : Link time in milliseconds\n
 *  \n
 *  ***************************************************************************\n
 */
int bcm_plp_aperta_display_eye_scan(bcm_plp_access_t phy_info)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    uint32_t mode = 0,flags=0x4; 

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_eyescan_run(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,flags,mode,0);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Firmware info 
 *
 *    This API is used to get the firmware version and CRC.
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param fw_version      [OUT] Firmware version
 *  @param fw_crc          [OUT] Firmware checksum </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_firmware_info_get(bcm_plp_access_t phy_info,  unsigned int *fw_version, unsigned int *fw_crc)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_core_firmware_info_t firmware_info;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_CORE_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_core_firmware_info_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core->pm_core, &firmware_info);
    *fw_version = firmware_info.fw_version;
    *fw_crc     = firmware_info.fw_crc;
    if ((firmware_info.fw_version & 0xF000) == 0xE000) {
        PHYMOD_DIAG_OUT(("WARNING:The current firmware 0x%x is not for production but evaluation purpose ONLY\n", firmware_info.fw_version));
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;

}

/*! \brief Firmware set
 *
 *	This API is used to program firmware to non-volatile memory. 
 *
 *  @param phy_info  <pre> Represents PHY access </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_firmware_set(bcm_plp_access_t phy_info)
{
    int rv = BCM_PM_IF_INVALID_PHY;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_core_init_config_t  init_config;
    plp_aperta_phymod_core_status_t       core_sts;
 
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_CORE_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&init_config, 0, sizeof(plp_aperta_phymod_core_init_config_t));
    PHYMOD_MEMSET(&core_sts   , 0, sizeof(plp_aperta_phymod_core_status_t));

    init_config.interface.interface_type = phymodInterfaceXFI;
    init_config.firmware_load_method     = phymodFirmwareLoadMethodProgEEPROM;

    rv = plp_aperta_phymod_core_init(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core->pm_core, &init_config, &core_sts);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Set MDIO pair swap 
 *
 *	This API is used to set the MDI pair swap
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param laneswap_map     MDI pair swap map</pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_rxtx_laneswap_set(bcm_plp_access_t phy_info, bcm_laneswap_map_t *laneswap_map)
{
    int      ii, rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_lane_map_t map_param;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_CORE_ACCESS(phy_info, phy_id_idx);

    map_param.num_of_lanes = laneswap_map->num_of_lanes;
    for ( ii = 0; ii < map_param.num_of_lanes; ii++) {
        map_param.lane_map_rx[ii] = laneswap_map->lanswap_map[ii];
        map_param.lane_map_tx[ii] = laneswap_map->lane_map_tx[ii];
    }
    rv = plp_aperta_phymod_core_lane_map_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core->pm_core, &map_param);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Get MDIO pair swap
 *
 *	This API is used to get the MDI pair swap map
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param laneswap_map     MDI pair swap map</pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int 
bcm_plp_aperta_rxtx_laneswap_get(bcm_plp_access_t phy_info, bcm_laneswap_map_t* laneswap_map)
{
    int rv = BCM_PM_IF_SUCCESS, ii;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_lane_map_t map_param;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_CORE_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_core_lane_map_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core->pm_core, &map_param);    
    if ( 0 == rv ) {
        for ( ii = 0; ii < map_param.num_of_lanes; ii++ ) {
            laneswap_map->lanswap_map[ii] = map_param.lane_map_rx[ii];
            laneswap_map->lane_map_tx[ii] = map_param.lane_map_tx[ii];
        }
        laneswap_map->num_of_lanes = map_param.num_of_lanes;
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief PLL sequencer restart 
 *
 *    This API is used to restart the PLL sequencer
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param flags           Reserved for future use
 *  @param operation       PLL operation to be performed\n
 *                         2 = bcmpmSeqOpRestart </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_pll_sequencer_restart(bcm_plp_access_t phy_info,  unsigned char flags, bcm_pm_sequencer_operation_t operation)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_CORE_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_core_pll_sequencer_restart(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core->pm_core, flags, operation);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Enable/Disable FEC 
 *
 *  This API is used to Enable/Disable FEC 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param enable          enable or disable CL74/CL91 FEC \n
 *                         0x00000 - disable CL91 FEC\n
 *                         0x00001 - enable CL91 FEC \n
 *                         0x10000 - disable CL74 FEC\n
 *                         0x10001 - enable CL74 FEC \n
 *                         0x20000 - disable CL108 FEC\n
 *                         0x20001 - enable CL108 FEC</pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_fec_enable_set(bcm_plp_access_t phy_info,  unsigned int enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    if ((enable & 0xF0000) == 0x10000) {
        /* FOR CL74*/
        PHYMOD_FEC_CL91_CLR(enable);
    } else if ((enable & 0xF0000) == 0x00000){
        /* FOR CL91*/
        PHYMOD_FEC_CL91_SET(enable);
    }

    rv = plp_aperta_phymod_phy_fec_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief FEC enable get 
 *
 *  This API is used to retrive FEC enable status
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param enable          (INOUT) Represent enabled status of CL74/CL91\n
 *                         bit 16-31 act as input and bit 0 to 16 act as output.
 *                         0x0xxxx : Get FEC enabled status for CL91
 *                         0x1xxxx : Get FEC enabled status for CL74
 *                         0x2xxxx : Get FEC enabled status for CL108</pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_fec_enable_get(bcm_plp_access_t phy_info,  unsigned int* enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    if (enable == NULL) {
        rv = BCM_PM_IF_MEMORY;
        goto ERR;
    }
    if (((*enable) & 0xF0000) == 0x10000) {
        /* FOR CL74*/
        PHYMOD_FEC_CL91_CLR(*enable);
    } else if (((*enable) & 0xF0000) == 0x00000){
        /* FOR CL91*/
        PHYMOD_FEC_CL91_SET(*enable);
    }

    rv = plp_aperta_phymod_phy_fec_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Configure JFEC 
 *
 *  This API is used to configure JFEC 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param jfec_config    Represent the configuration parameters of JFEC</pre>          
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_jfec_config_set(bcm_plp_access_t phy_info, bcm_plp_jfec_config_t jfec_config)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_jfec_config_t phy_jfec_config;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&phy_jfec_config, 0, sizeof(phymod_jfec_config_t));
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION(phy_info, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif

    /* coverity[mixed_enums] */
    phy_jfec_config.bit_flip_ena = jfec_config.bit_flip_ena;
    /* coverity[mixed_enums] */
    phy_jfec_config.bit2_swap_ena = jfec_config.bit2_swap_ena;
    /* coverity[mixed_enums] */
    phy_jfec_config.word_swap_dis = jfec_config.word_swap_dis;
    /* coverity[mixed_enums] */
    phy_jfec_config.data_swap_ena = jfec_config.data_swap_ena;
    /* coverity[mixed_enums] */
    phy_jfec_config.dp_mode_sel = jfec_config.dp_mode_sel;
    /* coverity[mixed_enums] */
    phy_jfec_config.fec_dec_bypass_ena = jfec_config.fec_dec_bypass_ena;
    /* coverity[mixed_enums] */
    phy_jfec_config.fec_ena_bypass_ena = jfec_config.fec_enc_bypass_ena;
    /* coverity[mixed_enums] */
    phy_jfec_config.fec_mode_sel = jfec_config.fec_mode_sel;
    phy_jfec_config.align_lock_chk_wdw = jfec_config.align_lock_chk_wdw;
    phy_jfec_config.align_lock_chk_thr = jfec_config.align_lock_chk_thr;
    phy_jfec_config.align_unlock_thr = jfec_config.align_unlock_thr;
    phy_jfec_config.fec_cerr_sel = jfec_config.fec_cerr_sel;
    /* coverity[mixed_enums] */
    phy_jfec_config.fec_uerr_ena = jfec_config.fec_uerr_ena;

    rv = plp_aperta_phymod_phy_jfec_config_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, phy_jfec_config);
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION_CLR(plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Get Configured JFEC 
 *
 *  This API is used to get configured JFEC 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param jfec_config    [out]Represent the configuration parameters of JFEC</pre>          
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_jfec_config_get(bcm_plp_access_t phy_info, bcm_plp_jfec_config_t *jfec_config)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_jfec_config_t phy_jfec_config;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&phy_jfec_config, 0, sizeof(phymod_jfec_config_t));
    PHYMOD_MEMSET(jfec_config, 0, sizeof(bcm_plp_jfec_config_t));
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION(phy_info, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif

    rv = plp_aperta_phymod_phy_jfec_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phy_jfec_config);
    
    /* coverity[mixed_enums] */
    jfec_config->bit_flip_ena = phy_jfec_config.bit_flip_ena ;
    /* coverity[mixed_enums] */
    jfec_config->bit2_swap_ena = phy_jfec_config.bit2_swap_ena;
    /* coverity[mixed_enums] */
    jfec_config->word_swap_dis = phy_jfec_config.word_swap_dis;
    /* coverity[mixed_enums] */
    jfec_config->data_swap_ena = phy_jfec_config.data_swap_ena;
    /* coverity[mixed_enums] */
    jfec_config->dp_mode_sel   = phy_jfec_config.dp_mode_sel ;
    /* coverity[mixed_enums] */
    jfec_config->fec_dec_bypass_ena = phy_jfec_config.fec_dec_bypass_ena;
    /* coverity[mixed_enums] */
    jfec_config->fec_enc_bypass_ena = phy_jfec_config.fec_ena_bypass_ena;
    /* coverity[mixed_enums] */
    jfec_config->fec_mode_sel = phy_jfec_config.fec_mode_sel ;
    jfec_config->align_lock_chk_wdw = phy_jfec_config.align_lock_chk_wdw;
    jfec_config->align_lock_chk_thr = phy_jfec_config.align_lock_chk_thr;
    jfec_config->align_unlock_thr = phy_jfec_config.align_unlock_thr;
    jfec_config->fec_cerr_sel= phy_jfec_config.fec_cerr_sel ;
    /* coverity[mixed_enums] */
    jfec_config->fec_uerr_ena = phy_jfec_config.fec_uerr_ena ;
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION_CLR(plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;

}

/*! \brief Configure KP4 FEC 
 *
 *  This API is used to configure KP4 FEC and KP4 PRBS
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param kp4_fec_config    Represent the configuration parameters of kp4</pre>          
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_kp4_fec_config_set(bcm_plp_access_t phy_info, bcm_plp_kp4_fec_config_t kp4_fec_config)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_kp4_config_t phy_kp4_fec_config;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&phy_kp4_fec_config, 0, sizeof(phymod_kp4_config_t));

    phy_kp4_fec_config.kp4_fec_config_enable = kp4_fec_config.kp4_fec_config_enable;
    phy_kp4_fec_config.kp4_50g_comp_mode = kp4_fec_config.kp4_50g_mode;
    phy_kp4_fec_config.kp4_fec_format = kp4_fec_config.kp4_fec_format;
    phy_kp4_fec_config.kp4_message_mode = kp4_fec_config.kp4_message_mode;
    phy_kp4_fec_config.kp4_term_encoder_bypass = kp4_fec_config.kp4_term_encoder_bypass;
    phy_kp4_fec_config.kp4_prbs = kp4_fec_config.kp4_prbs;
    phy_kp4_fec_config.kp4_fec_lane_convertor_enable = kp4_fec_config.fec_lane_converter_enable;
    phy_kp4_fec_config.kp4_fec_lane_convertor = kp4_fec_config.kp4_fec_lane_converter;

    /* coverity[mixed_enums] */
    phy_kp4_fec_config.kp4_prbs_config.poly = kp4_fec_config.kp4_prbs_config.poly;
    phy_kp4_fec_config.kp4_prbs_config.invert = kp4_fec_config.kp4_prbs_config.invert;
    phy_kp4_fec_config.kp4_prbs_config.enable = kp4_fec_config.kp4_prbs_config.enable;
    phy_kp4_fec_config.kp4_prbs_config.flags = kp4_fec_config.kp4_prbs_config.tx_rx;

    phy_kp4_fec_config.kp4_prbs_align_tx = kp4_fec_config.kp4_prbs_align_tx;
    phy_kp4_fec_config.kp4_prbs_align_rx = kp4_fec_config.kp4_prbs_align_rx;

#ifdef PHYMOD_ESTOQUE_SUPPORT
    phy_kp4_fec_config.kp4_prbs_init_tx = kp4_fec_config.kp4_prbs_init_tx;
    phy_kp4_fec_config.kp4_prbs_init_rx = kp4_fec_config.kp4_prbs_init_rx;
#endif
 
    rv = plp_aperta_phymod_phy_kp4_config_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, phy_kp4_fec_config);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;

}

/*! \brief Get Configured KP4 FEC and KP4 PRBS
 *
 *  This API is used to get configured KP4 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param kp4_fec_config    Represent the configuration parameters of kp4</pre>          
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_kp4_fec_config_get(bcm_plp_access_t phy_info, bcm_plp_kp4_fec_config_t *kp4_fec_config)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_kp4_config_t phy_kp4_fec_config;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&phy_kp4_fec_config, 0, sizeof(phymod_kp4_config_t));
    /* tx_rx is input default is zero */
    phy_kp4_fec_config.kp4_prbs_config.flags = kp4_fec_config->kp4_prbs_config.tx_rx;
    PHYMOD_MEMSET(kp4_fec_config, 0, sizeof(bcm_plp_kp4_fec_config_t));

    rv = plp_aperta_phymod_phy_kp4_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phy_kp4_fec_config);

    kp4_fec_config->kp4_50g_mode = phy_kp4_fec_config.kp4_50g_comp_mode;
    kp4_fec_config->kp4_fec_format = phy_kp4_fec_config.kp4_fec_format;
    kp4_fec_config->kp4_message_mode = phy_kp4_fec_config.kp4_message_mode;
    kp4_fec_config->kp4_term_encoder_bypass = phy_kp4_fec_config.kp4_term_encoder_bypass;
    kp4_fec_config->kp4_prbs = phy_kp4_fec_config.kp4_prbs;
    kp4_fec_config->fec_lane_converter_enable = phy_kp4_fec_config.kp4_fec_lane_convertor_enable;
    kp4_fec_config->kp4_fec_lane_converter = phy_kp4_fec_config.kp4_fec_lane_convertor;

    /* coverity[mixed_enums] */
    kp4_fec_config->kp4_prbs_config.poly = phy_kp4_fec_config.kp4_prbs_config.poly;
    kp4_fec_config->kp4_prbs_config.enable = phy_kp4_fec_config.kp4_prbs_config.enable;
    kp4_fec_config->kp4_prbs_config.invert = phy_kp4_fec_config.kp4_prbs_config.invert;
    kp4_fec_config->kp4_prbs_config.tx_rx = phy_kp4_fec_config.kp4_prbs_config.flags;

    kp4_fec_config->kp4_prbs_status.prbs_lock = phy_kp4_fec_config.kp4_prbs_status.prbs_lock;
    kp4_fec_config->kp4_prbs_status.prbs_lock_loss = phy_kp4_fec_config.kp4_prbs_status.prbs_lock_loss;
    kp4_fec_config->kp4_prbs_status.error_count = phy_kp4_fec_config.kp4_prbs_status.error_count;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;

}

/*! \brief Get KP4/JFEC status 
 *
 *  This API is used to get status of KP4 / JFEC
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param fec_status  [OUT] Status of KP4/JFEC fec</pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_fec_status_get(bcm_plp_access_t phy_info, bcm_plp_fec_status_t *fec_status)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0, index = 0;
    phymod_phy_fec_dump_status_t phy_fec_dump_sts;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&phy_fec_dump_sts, 0, sizeof(phymod_phy_fec_dump_status_t));
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION(phy_info, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif

    phy_fec_dump_sts.fec_status_clear = fec_status->fec_status_clear;
    /* coverity[mixed_enums] */
    phy_fec_dump_sts.fec_status_init = fec_status->fec_status_init;
    rv = plp_aperta_phymod_phy_pam4_fec_status_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phy_fec_dump_sts);
    /* coverity[mixed_enums] */
    fec_status->align_lol_sticky = phy_fec_dump_sts.align_lol_sticky;
    /* coverity[mixed_enums] */
    fec_status->align_lock = phy_fec_dump_sts.align_lock;
    fec_status->fec_err_cnt.ieee_uncorr_cnt        = phy_fec_dump_sts.fec_err_cnt.ieee_uncorr_cnt;
    fec_status->fec_err_cnt.ieee_symbols_corr_cnt  = phy_fec_dump_sts.fec_err_cnt.ieee_symbols_corr_cnt;
    fec_status->fec_err_cnt.sys_am_lock            = phy_fec_dump_sts.fec_err_cnt.sys_am_lock;
    fec_status->fec_err_cnt.line_am_lock           = phy_fec_dump_sts.fec_err_cnt.line_am_lock;
    fec_status->fec_err_cnt.ieee_kp4_dec_ctrl      = phy_fec_dump_sts.fec_err_cnt.ieee_kp4_dec_ctrl;
    fec_status->fec_err_cnt.ieee_kp4_stat_ctrl     = phy_fec_dump_sts.fec_err_cnt.ieee_kp4_stat_ctrl;
    fec_status->fec_err_cnt.ieee_fec_lane_mapping  = phy_fec_dump_sts.fec_err_cnt.ieee_fec_lane_mapping;
    fec_status->fec_err_cnt.ieee_kp4_hi_ser_th_sp  = phy_fec_dump_sts.fec_err_cnt.ieee_kp4_hi_ser_th_sp;
    fec_status->fec_err_cnt.tot_frame_rev_cnt = phy_fec_dump_sts.fec_err_cnt.total_frame_rev_cnt;
    fec_status->fec_err_cnt.tot_frame_corr_cnt = phy_fec_dump_sts.fec_err_cnt.total_frame_corr_cnt;
    fec_status->fec_err_cnt.tot_frame_uncorr_cnt = phy_fec_dump_sts.fec_err_cnt.total_frame_uncorr_cnt;
    fec_status->fec_err_cnt.tot_symbols_corr_cnt = phy_fec_dump_sts.fec_err_cnt.total_symbols_corr_cnt;
    fec_status->fec_err_cnt.tot_bits_corr_cnt[0] = phy_fec_dump_sts.fec_err_cnt.total_bits_corr_cnt[0];
    fec_status->fec_err_cnt.tot_bits_corr_cnt[1] = phy_fec_dump_sts.fec_err_cnt.total_bits_corr_cnt[1];
    for (index = 0; index < BCM_PLP_FEC_TOT_FRAMES_ERR_NUM; index ++) {
        fec_status->fec_err_cnt.bcm_plp_tot_frames_err_cnt[index] = phy_fec_dump_sts.fec_err_cnt.total_frames_err_cnt[index];
    }
#if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined(PHYMOD_AGERA2_SUPPORT)
    /* IEEE FEC status Note: Below are applicable only for millenio PHY family*/
    fec_status->fec_dump_status.ieee_fec_sts.ieee_rsfec_ctrl = phy_fec_dump_sts.fec_dump_status.ieee_fec_sts.ieee_rsfec_ctrl;
    fec_status->fec_dump_status.ieee_fec_sts.ieee_rsfec_stat = phy_fec_dump_sts.fec_dump_status.ieee_fec_sts.ieee_rsfec_stat;
    fec_status->fec_dump_status.ieee_fec_sts.ieee_fec_lane_mapping = phy_fec_dump_sts.fec_dump_status.ieee_fec_sts.ieee_fec_lane_mapping;
    fec_status->fec_dump_status.ieee_fec_sts.ieee_corr_cw_cnt = phy_fec_dump_sts.fec_dump_status.ieee_fec_sts.ieee_corr_cw_cnt;
    fec_status->fec_dump_status.ieee_fec_sts.ieee_uncorr_cw_cnt = phy_fec_dump_sts.fec_dump_status.ieee_fec_sts.ieee_uncorr_cw_cnt;
    PHYMOD_MEMCPY(&fec_status->fec_dump_status.ieee_fec_sts.ieee_symbols_corr_cnt_fln, phy_fec_dump_sts.fec_dump_status.ieee_fec_sts.ieee_symbols_corr_cnt_fln, 8 * sizeof(unsigned short));
    fec_status->fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_1 = phy_fec_dump_sts.fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_1;
    fec_status->fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_3 = phy_fec_dump_sts.fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_3;
    PHYMOD_MEMCPY(&fec_status->fec_dump_status.ieee_fec_sts.ieee_fecpcs_lane_mapping, phy_fec_dump_sts.fec_dump_status.ieee_fec_sts.ieee_fecpcs_lane_mapping, 8 * sizeof(unsigned short));
    /* bcm_plp_fec_rx_status_t; Note: Below are applicable only for Millenio PHY family*/
    fec_status->fec_dump_status.fec_sts.igbox_clsn_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.igbox_clsn_sticky;
    fec_status->fec_dump_status.fec_sts.am_lolock_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.am_lolock_sticky;
    fec_status->fec_dump_status.fec_sts.dgbox_clsn_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.dgbox_clsn_sticky;
    fec_status->fec_dump_status.fec_sts.hi_ser_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.hi_ser_sticky;
    fec_status->fec_dump_status.fec_sts.xdec_err_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.xdec_err_sticky;
    fec_status->fec_dump_status.fec_sts.fec_link_stat = phy_fec_dump_sts.fec_dump_status.fec_sts.fec_link_stat;
    fec_status->fec_dump_status.fec_sts.fec_link_stat_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.fec_link_stat_sticky;
    /* bcm_plp_fec_rx_status_t; Note: Below are applicable only for Barchetta2 PHY family*/
    fec_status->fec_dump_status.fec_sts.intf_gbox_clsn_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.intf_gbox_clsn_sticky;
    fec_status->fec_dump_status.fec_sts.am_lo_lock_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.am_lo_lock_sticky;
    fec_status->fec_dump_status.fec_sts.xdec_gbox_clsn_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.xdec_gbox_clsn_sticky;
    fec_status->fec_dump_status.fec_sts.xenc_gbox_clsn_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.xenc_gbox_clsn_sticky;
    fec_status->fec_dump_status.fec_sts.ogbox_clsn_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.ogbox_clsn_sticky;
    fec_status->fec_dump_status.fec_sts.fec_algn_lol_sticky = phy_fec_dump_sts.fec_dump_status.fec_sts.fec_algn_lol_sticky;
    fec_status->fec_dump_status.fec_sts.fec_algn_stat = phy_fec_dump_sts.fec_dump_status.fec_sts.fec_algn_stat;
    /* Below are common parameters*/
    /* Fec a error count */
    fec_status->fec_dump_status.fec_a_err_cnt.tot_frame_rev_cnt = phy_fec_dump_sts.fec_dump_status.fec_a_err_cnt.tot_frame_rev_cnt;
    fec_status->fec_dump_status.fec_a_err_cnt.tot_frame_corr_cnt = phy_fec_dump_sts.fec_dump_status.fec_a_err_cnt.tot_frame_corr_cnt;
    fec_status->fec_dump_status.fec_a_err_cnt.tot_frame_uncorr_cnt = phy_fec_dump_sts.fec_dump_status.fec_a_err_cnt.tot_frame_uncorr_cnt;
    fec_status->fec_dump_status.fec_a_err_cnt.tot_symbols_corr_cnt = phy_fec_dump_sts.fec_dump_status.fec_a_err_cnt.tot_symbols_corr_cnt;
    PHYMOD_MEMCPY(&fec_status->fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt, phy_fec_dump_sts.fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt, 2 * sizeof(unsigned long long));
    PHYMOD_MEMCPY(&fec_status->fec_dump_status.fec_a_err_cnt.tot_frames_err_cnt, phy_fec_dump_sts.fec_dump_status.fec_a_err_cnt.tot_frames_err_cnt, 16 * sizeof(unsigned long long));
    /* Fec b error count */
    fec_status->fec_dump_status.fec_b_err_cnt.tot_frame_rev_cnt = phy_fec_dump_sts.fec_dump_status.fec_b_err_cnt.tot_frame_rev_cnt;
    fec_status->fec_dump_status.fec_b_err_cnt.tot_frame_corr_cnt = phy_fec_dump_sts.fec_dump_status.fec_b_err_cnt.tot_frame_corr_cnt;
    fec_status->fec_dump_status.fec_b_err_cnt.tot_frame_uncorr_cnt = phy_fec_dump_sts.fec_dump_status.fec_b_err_cnt.tot_frame_uncorr_cnt;
    fec_status->fec_dump_status.fec_b_err_cnt.tot_symbols_corr_cnt = phy_fec_dump_sts.fec_dump_status.fec_b_err_cnt.tot_symbols_corr_cnt;
    PHYMOD_MEMCPY(&fec_status->fec_dump_status.fec_b_err_cnt.tot_bits_corr_cnt, phy_fec_dump_sts.fec_dump_status.fec_b_err_cnt.tot_bits_corr_cnt, 2 * sizeof(unsigned long long));
    PHYMOD_MEMCPY(&fec_status->fec_dump_status.fec_b_err_cnt.tot_frames_err_cnt, phy_fec_dump_sts.fec_dump_status.fec_b_err_cnt.tot_frames_err_cnt, 16 * sizeof(unsigned long long));
    /* Fec c error count */
    fec_status->fec_dump_status.fec_c_err_cnt.tot_frame_rev_cnt = phy_fec_dump_sts.fec_dump_status.fec_c_err_cnt.tot_frame_rev_cnt;
    fec_status->fec_dump_status.fec_c_err_cnt.tot_frame_corr_cnt = phy_fec_dump_sts.fec_dump_status.fec_c_err_cnt.tot_frame_corr_cnt;
    fec_status->fec_dump_status.fec_c_err_cnt.tot_frame_uncorr_cnt = phy_fec_dump_sts.fec_dump_status.fec_c_err_cnt.tot_frame_uncorr_cnt;
    fec_status->fec_dump_status.fec_c_err_cnt.tot_symbols_corr_cnt = phy_fec_dump_sts.fec_dump_status.fec_c_err_cnt.tot_symbols_corr_cnt;
    PHYMOD_MEMCPY(&fec_status->fec_dump_status.fec_c_err_cnt.tot_bits_corr_cnt, phy_fec_dump_sts.fec_dump_status.fec_c_err_cnt.tot_bits_corr_cnt, 2 * sizeof(unsigned long long));
    PHYMOD_MEMCPY(&fec_status->fec_dump_status.fec_c_err_cnt.tot_frames_err_cnt, phy_fec_dump_sts.fec_dump_status.fec_c_err_cnt.tot_frames_err_cnt, 16 * sizeof(unsigned long long));
    /* Fec d error count */
    fec_status->fec_dump_status.fec_d_err_cnt.tot_frame_rev_cnt = phy_fec_dump_sts.fec_dump_status.fec_d_err_cnt.tot_frame_rev_cnt;
    fec_status->fec_dump_status.fec_d_err_cnt.tot_frame_corr_cnt = phy_fec_dump_sts.fec_dump_status.fec_d_err_cnt.tot_frame_corr_cnt;
    fec_status->fec_dump_status.fec_d_err_cnt.tot_frame_uncorr_cnt = phy_fec_dump_sts.fec_dump_status.fec_d_err_cnt.tot_frame_uncorr_cnt;
    fec_status->fec_dump_status.fec_d_err_cnt.tot_symbols_corr_cnt = phy_fec_dump_sts.fec_dump_status.fec_d_err_cnt.tot_symbols_corr_cnt;
    PHYMOD_MEMCPY(&fec_status->fec_dump_status.fec_d_err_cnt.tot_bits_corr_cnt, phy_fec_dump_sts.fec_dump_status.fec_d_err_cnt.tot_bits_corr_cnt, 2 * sizeof(unsigned long long));
    PHYMOD_MEMCPY(&fec_status->fec_dump_status.fec_d_err_cnt.tot_frames_err_cnt, phy_fec_dump_sts.fec_dump_status.fec_d_err_cnt.tot_frames_err_cnt, 16 * sizeof(unsigned long long));
    /* Fec ber status */
    /* Fec ber status */
    /* Fec ber status */
    fec_status->fec_dump_status.fec_ber.pre_fec_ber = phy_fec_dump_sts.fec_dump_status.fec_ber.pre_fec_ber;
    fec_status->fec_dump_status.fec_ber.post_fec_ber = phy_fec_dump_sts.fec_dump_status.fec_ber.post_fec_ber;
    fec_status->fec_dump_status.fec_ber.post_fec_ber_proj = phy_fec_dump_sts.fec_dump_status.fec_ber.post_fec_ber_proj;
#endif
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PLP_DATA_PATH_DIRECTION_CLR(plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;

}

/*! \brief PHY status dump
 *
 *  This API is used to dump status about PHY and its lanes
 *  Supports following debug levels:
 *      - BCM_PLP_INTERNAL_DUMP
 *      - BCM_PLP_INTERNAL_DUMP_L1
 *      - BCM_PLP_INTERNAL_DUMP_L2
 *      - BCM_PLP_INTERNAL_DUMP_L3
 *      - BCM_PLP_CUSTOM_DIAG_DUMP
 *  The debug levels should be configured via "flags" parameter of the phy_info structure.
 *  @param phy_info  <pre> Represents PHY access\n </pre>
 *  
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_phy_status_dump(bcm_plp_access_t phy_info)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_phy_diagnostics_t phy_diag;
    PHYMOD_IF_MEMSET(&phy_diag, 0, sizeof(plp_aperta_phymod_phy_diagnostics_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    
    if (phy_info.flags & BCM_PLP_INTERNAL_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_INTERNAL_DUMP;
    }
    if (phy_info.flags & BCM_PLP_CUSTOM_DIAG_DUMP) {
#if defined(PHYMOD_XGBASET_SUPPORT)
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_CUSTOM_DIAG_DUMP;
#endif
        plp_aperta_phymod_custom_diag_dump_hdr(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
        rv = plp_aperta_phymod_phy_diagnostics_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phy_diag);
#if defined(PHYMOD_XGBASET_SUPPORT)
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_CUSTOM_DIAG_DUMP);
#endif
        if(!rv) {
            plp_aperta_phymod_custom_diag_dump_display(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phy_diag);
        }
        goto ERR;
    }
#if defined(PHYMOD_ESTOQUE_SUPPORT) || defined(PHYMOD_MILLENIO_SUPPORT)
    if (phy_info.flags & BCM_PLP_INTERNAL_CHIP_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_INTERNAL_CHIP_DUMP;
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_CORE_LANE_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_INTERNAL_CORE_LANE_DUMP;
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_CHIP_DSC_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_INTERNAL_CHIP_DSC_DUMP;
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_DIAG_REG_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_INTERNAL_DIAG_REG_DUMP;
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_EVENT_UC_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_INTERNAL_EVENT_UC_DUMP;
    }
#endif
#if defined(PHYMOD_ESTOQUE_SUPPORT) || defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || \
    defined(PHYMOD_APERTA_SUPPORT) || defined(PHYMOD_BARCHETTA_SUPPORT) || defined(PHYMOD_AGERA_SUPPORT) || defined(PHYMOD_AGERALITE_SUPPORT) ||\
    defined(PHYMOD_APERTA2_SUPPORT) || defined (PHYMOD_AGERA2_SUPPORT)
    if (phy_info.flags & BCM_PLP_INTERNAL_CHIP_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_INTERNAL_CHIP_DUMP;
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_DUMP_L1) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_INTERNAL_DUMP_L1;
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_DUMP_L2) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_INTERNAL_DUMP_L2;
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_DUMP_L3) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_INTERNAL_DUMP_L3;
    }
#endif
    rv = plp_aperta_phymod_phy_status_dump(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
    if (phy_info.flags & BCM_PLP_INTERNAL_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_INTERNAL_DUMP);
    }
#if defined(PHYMOD_ESTOQUE_SUPPORT) || defined(PHYMOD_MILLENIO_SUPPORT)
    if (phy_info.flags & BCM_PLP_INTERNAL_CHIP_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_INTERNAL_CHIP_DUMP);
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_CORE_LANE_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_INTERNAL_CORE_LANE_DUMP);
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_CHIP_DSC_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_INTERNAL_CHIP_DSC_DUMP);
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_DIAG_REG_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_INTERNAL_DIAG_REG_DUMP);
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_EVENT_UC_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_INTERNAL_EVENT_UC_DUMP);
    }
#endif
#if defined(PHYMOD_ESTOQUE_SUPPORT) || defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || \
    defined(PHYMOD_APERTA_SUPPORT) || defined(PHYMOD_BARCHETTA_SUPPORT) || defined(PHYMOD_AGERA_SUPPORT) || defined(PHYMOD_AGERALITE_SUPPORT) || \
    defined(PHYMOD_APERTA2_SUPPORT) || defined (PHYMOD_AGERA2_SUPPORT)
    if (phy_info.flags & BCM_PLP_INTERNAL_CHIP_DUMP) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_INTERNAL_CHIP_DUMP);
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_DUMP_L1) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_INTERNAL_DUMP_L1);
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_DUMP_L2) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_INTERNAL_DUMP_L2);
    }
    if (phy_info.flags & BCM_PLP_INTERNAL_DUMP_L3) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_INTERNAL_DUMP_L3);
    }
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief PHY DSC Diagnostics 
 *
 *  This API is used to retrieve PHY DSC Diagnostics for a given lane
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param diag            [OUT]Attributes for Lane based diagnosis </pre>
 * 
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_phy_diagnostics_get(bcm_plp_access_t phy_info,  bcm_plp_pm_phy_diagnostics_t* diag)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_phy_diagnostics_t phy_diag;

    PHYMOD_IF_MEMSET(&phy_diag, 0, sizeof(plp_aperta_phymod_phy_diagnostics_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

#if defined(PHYMOD_XGBASET_SUPPORT)
    plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= BCM_PLP_CUSTOM_DIAG_DUMP;
#endif

    rv = plp_aperta_phymod_phy_diagnostics_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phy_diag);

#if defined(PHYMOD_XGBASET_SUPPORT)
    plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= (~BCM_PLP_CUSTOM_DIAG_DUMP);
#endif

    if(!rv) {
        PHYMOD_IF_MEMCPY(diag, &phy_diag, sizeof(plp_aperta_phymod_phy_diagnostics_t));
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief PHY PAM4 DSC Diagnostics 
 *
 *  This API is used to retrieve PAM4 PHY DSC information for a given lane
 *
 *  @param phy_info        <pre> Represents PHY access\n
 *  @param diag       [OUT]Attributes for Lane based diagnosis </pre>
 * 
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_phy_pam4_diagnostics_get(bcm_plp_access_t phy_info,  bcm_plp_pm_phy_pam4_diagnostics_t* diag)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_phy_pam4_diagnostics_t phy_diag;

    PHYMOD_IF_MEMSET(&phy_diag, 0, sizeof(phymod_phy_pam4_diagnostics_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
        
    rv = plp_aperta_phymod_phy_pam4_diagnostics_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phy_diag);
    if(!rv) {
        PHYMOD_IF_MEMCPY(diag, &phy_diag, sizeof(phymod_phy_pam4_diagnostics_t));
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Interrupt status get 
 *
 *  This API is used to get interrupt status
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param intr_type       Type of interrupt \n
 *                         <a href = ../evora_pm_intr_table.jpg> Evora Interrupts </a>
 *                         <a href = ../miura_pm_intr_table.jpg> Miura Interrupts </a>
 *                         <a href = ../barchetta_pm_intr_table.jpg> Barchetta Interrupts </a>
 *                         Refer Aperta user specification document for supported interrupt/event types in Aperta.
 *
 *  @param intr_status     [OUT] Interrupt status </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_intr_status_get(bcm_plp_access_t phy_info, unsigned int intr_type, unsigned int* intr_status)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    *intr_status = intr_type;

    rv = plp_aperta_phymod_phy_intr_status_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, intr_status);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Event get 
 *
 *  This API is used to get HW event 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param event_type      Type of event \n
 *                         Specific to chip \n 
 *                         <a href = ../evora_pm_intr_table.jpg> Evora Interrupts </a>
 *                         <a href = ../miura_pm_intr_table.jpg> Miura Interrupts </a>
 *                         <a href = ../barchetta_pm_intr_table.jpg> Barchetta Interrupts </a>
 *                         Refer Aperta user specification document for supported interrupt/event types in Aperta.
 *
 *  @param event_status     [OUT] Event status </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_event_status_get(bcm_plp_access_t phy_info, unsigned int event_type, unsigned int* event_status)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    *event_status = (event_type) | (1 << 31);

    rv = plp_aperta_phymod_phy_intr_status_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, event_status);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Interrupt enable set 
 *
 *  This API is used to enable specified interrupt.
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param intr_type       Type of interrupt, \n
 *                         Interrupt type varies based on the chip. 
 *                         <a href = ../evora_pm_intr_table.jpg> Evora Interrupts </a>
 *                         <a href = ../miura_pm_intr_table.jpg> Miura Interrupts </a>
 *                         <a href = ../barchetta_pm_intr_table.jpg> Barchetta Interrupts </a>
 *                         Refer Aperta user specification document for supported interrupt/event types in Aperta.
 *  @param enable          Enable/Disable specified interrupt \n
 *                         1 - Enable specified interrupt \n
 *                         0 - Disable specified interrupt  </pre>
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_intr_enable_set(bcm_plp_access_t phy_info, unsigned int intr_type, unsigned int enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    unsigned int temp_enable = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

#if defined(PHYMOD_BRONCOS_SUPPORT) || defined(PHYMOD_SEAHAWKS_SUPPORT)
    temp_enable = enable;
#else
        temp_enable = intr_type;
    if ( enable & 0xF ) {
        temp_enable |= (1 << 31);
    } else {
        temp_enable &= ~(1 << 31);
    }
#endif
    rv = plp_aperta_phymod_phy_intr_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, temp_enable);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
 
/*! \brief Interrupt enable get 
 *
 *  This API is used to get enabled interrupt 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param intr_type       Type of interrupt, 
 *                         <a href = ../evora_pm_intr_table.jpg> Evora Interrupts </a>
 *                         <a href = ../miura_pm_intr_table.jpg> Miura Interrupts </a>
 *                         <a href = ../barchetta_pm_intr_table.jpg> Barchetta Interrupts </a>
 *                         Refer Aperta user specification document for supported interrupt/event types in Aperta.
 *  @param enable          Get Enabled status\n
 *                         1 - Enable\n
 *                         0 - Disable </pre>
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_intr_enable_get(bcm_plp_access_t phy_info, unsigned int intr_type, unsigned int* enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_phy_intr_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,  enable);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
 
/*! \brief Interrupt status clear 
 *
 *  This API is used to clear the interrupt status 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param intr_type       Type of interrupt,</pre>
 *                         <a href = ../evora_pm_intr_table.jpg> Evora Interrupts </a>
 *                         <a href = ../miura_pm_intr_table.jpg> Miura Interrupts </a>
 *                         <a href = ../barchetta_pm_intr_table.jpg> Barchetta Interrupts </a>
 *                         Refer Aperta user specification document for supported interrupt/event types in Aperta.
 *      
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_intr_status_clear(bcm_plp_access_t phy_info, unsigned int intr_type)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_phy_intr_status_clear(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, intr_type);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

#ifdef BCM_PLP_BASE_T_PHY

/*! \brief EEE set 
 *
 *  This API is used to config EEE (energy efficient ethernet) 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param eee_conf       point to the structure containing EEE config parameters</pre>
 *      
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_base_t_eee_set(bcm_plp_access_t phy_info, bcm_plp_base_t_eee_t *eee_conf)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY, exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_phy_eee_mode_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (phymod_eee_t*) eee_conf);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief EEE get 
 *
 *  This API is used to get EEE (energy efficient ethernet) config and statistics information
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param eee_conf       point to the structure containing EEE config parameters</pre>
 *      
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_base_t_eee_get(bcm_plp_access_t phy_info, bcm_plp_base_t_eee_t *eee_conf)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY, exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_phy_eee_mode_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (phymod_eee_t*) eee_conf);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Power Mode set 
 *
 *  This API is used to config Power Mode 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param power_mode     Power mode: 0 = Full
 *                                    1 = Low
 *                                    2 = Auto (not supported by all PHYs)</pre>
 *      
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_base_t_power_mode_set(bcm_plp_access_t phy_info, bcm_plp_base_t_power_mode_t power_mode)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY, exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_phy_power_mode_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                   (phymod_power_mode_t) power_mode);    
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Power Mode get 
 *
 *  This API is used to get Power Mode 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param power_mode     Power mode: 0 = Full
 *                                    1 = Low
 *                                    2 = Auto (not supported by all PHYs)</pre>
 *      
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_base_t_power_mode_get(bcm_plp_access_t phy_info, bcm_plp_base_t_power_mode_t *power_mode)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY, exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_phy_power_mode_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                   (phymod_power_mode_t*) power_mode);    
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief PHY Cable Diagnostics 
 *
 *  This API is used to retrieve PHY Cable Diagnostics information
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param cdiag           point to the structure containing Cable Diagnostics information
 *                         (for details see bcm_plp_base_t_cable_diag_t in common defines header file)</pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_base_t_cable_diag(bcm_plp_access_t phy_info, bcm_plp_base_t_cable_diag_t *cdiag)
{
    plp_aperta_phymod_phy_diagnostics_t  phymod_diag;
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY, exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    PHYMOD_MEMSET(&phymod_diag, 0, sizeof(plp_aperta_phymod_phy_diagnostics_t));
    phymod_diag.state_mfg_diag_op_cmd = bcmplpBaseTDiagCtrlCmdCableDiag;
    phymod_diag.state_mfg_diag_arg    = cdiag;

    rv = plp_aperta_phymod_phy_diagnostics_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phymod_diag);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief PHY State/Manufacturing Diagnostics 
 *
 *  This API is used to retrieve PHY State/Manufacturing Diagnostics information
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param inst            The specific device block
 *  @param op_type         Operation type
 *  @param op_cmd          Operation command code
 *  @param arg             Command argument</pre>
 * 
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_base_t_state_mfg_diag(bcm_plp_access_t phy_info, unsigned int inst,
                              bcm_plp_base_t_diag_ctrl_type_t op_type,
                              bcm_plp_base_t_diag_ctrl_cmd_t  op_cmd, void *arg)
{
    plp_aperta_phymod_phy_diagnostics_t  phymod_diag;
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY, exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    PHYMOD_MEMSET(&phymod_diag, 0, sizeof(plp_aperta_phymod_phy_diagnostics_t));
    if ( ! op_cmd ) {    /* op_cmd = 0  is reserved for Cable Diag */
        op_cmd = bcmplpBaseTDiagCtrlCmdMfgHybCanc;  /* Hybrid_Canc by default */
    }
    phymod_diag.state_mfg_diag_inst     = inst;
    phymod_diag.state_mfg_diag_op_type  = op_type;
    phymod_diag.state_mfg_diag_op_cmd   = op_cmd;
    phymod_diag.state_mfg_diag_arg      = arg;

    rv = plp_aperta_phymod_phy_diagnostics_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phymod_diag);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
#endif /* BCM_PLP_BASE_T_PHY */

/*! \brief Eye margin Projection 
 *
 *  This API is used to display eye margin for a given lane
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param rate              Line rate in MHz used to calculate BER
 *  @param ber_scan_mode     Represents BER scan mode:\n
                             0 - Fast mode \n
                             1 - LowBER mode\n
                             2 - BERProjection mode\n
 *  @param timer_control     Total measurement time in units of ~1.3 seconds
 *  @param max_error_control The error threshold it uses to step to next measurement in units of 16 </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
#ifdef SERDES_API_FLOATING_POINT
int bcm_plp_aperta_eye_margin_proj(bcm_plp_access_t phy_info,  double rate, unsigned char ber_scan_mode, unsigned char timer_control, unsigned char max_error_control)
#else
int bcm_plp_aperta_eye_margin_proj(bcm_plp_access_t phy_info,  int rate, unsigned char ber_scan_mode, unsigned char timer_control, unsigned char max_error_control)
#endif
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    uint32_t mode = phymodEyescanModeBERProj,flags = 0x4;
    plp_aperta_phymod_phy_eyescan_options_t eyescan_options;
    
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    eyescan_options.linerate_in_khz = rate;
    eyescan_options.ber_proj_scan_mode = ber_scan_mode;
    eyescan_options.ber_proj_timer_cnt = timer_control;
    eyescan_options.ber_proj_err_cnt = max_error_control;

    rv = plp_aperta_phymod_phy_eyescan_run(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,flags,mode, &eyescan_options);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
/*! \brief Repeater mode
 *
 *  This API is used to enable/Disable Repeater/Retimer
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param ena_dis         Represents enable/disable\n
 *               0 - retimer mode
 *               1 - repeater mode </pre>
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 *                     */
int
bcm_plp_aperta_repeater_mode_set(bcm_plp_access_t phy_info, unsigned int ena_dis)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    _bcm_if_phymod_phy_t *phy; 
    plp_aperta_phymod_phy_inf_config_t mode_config;
    int ref_clk = 0;

    PHYMOD_IF_MEMSET(&mode_config, 0, sizeof(plp_aperta_phymod_phy_inf_config_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_phy_interface_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, 0,
           ref_clk,  &mode_config);
    if (rv != 0) {
       PHYMOD_DEBUG_ERROR(("plp_aperta_phymod_phy_interface_config_get error rv=%d\n",rv));
       goto ERR;
    }
    phy= plp_aperta_phy_ctrl.phy[phy_id_idx];
    phy->init_config.interface.interface_type = mode_config.interface_type;
    phy->init_config.interface.data_rate = mode_config.data_rate;
    phy->init_config.interface.ref_clock = mode_config.ref_clock;
    if (ena_dis) {
        phy->init_config.op_mode = phymodOperationModeRepeater; /* Setting Defaults to repeater*/
    } else { /* Defaults for the case where static config is not set*/ 
        phy->init_config.op_mode = phymodOperationModeRetimer; /*retimer*/
    }

    rv = plp_aperta_phymod_phy_init(&phy->pm_phy, &phy->init_config);
    if (rv != 0) {
       PHYMOD_DEBUG_ERROR(("PHY_INIT error rv=%d\n",rv));
       goto ERR;
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Module Read
 *    This API is used read the data with I2C command on selected module
 *    on specified PHY-id.
 *
 *    @param phy_info <pre> Represents PHY access\n
 *    @param slv_dev_addr       Module slave address (for supported Module types)
 *    @param start_addr      Start address of i2c Slave to be accessed
 *    @param no_of_bytes     No of bytes to be read.
 *    @param read_data     [OUT] Contains array of bytes read from the module </pre>
 *    @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_module_read(bcm_plp_access_t phy_info, unsigned int slv_dev_addr,
                    unsigned int start_addr, unsigned int no_of_bytes,
                    unsigned char *read_data)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    uint32_t unused_flags = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_i2c_read(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, unused_flags, slv_dev_addr, start_addr, no_of_bytes, read_data);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Module Write
 *   This API is used write the data with I2C command on selected module
 *   on specified PHY-id.
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param slv_dev_addr       Module slave address (for supported Module types)
 *  @param start_addr      Start address of i2c Slave to be accessed
 *  @param no_of_bytes     No of bytes to be read.
 *  @param write_data      Contains array of bytes to be written to the module </pre>
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_module_write(bcm_plp_access_t phy_info, unsigned int slv_dev_addr,
                     unsigned int start_addr, unsigned int no_of_bytes,
                     unsigned char *write_data)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    uint32_t unused_flags = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_i2c_write(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, unused_flags, slv_dev_addr, start_addr, no_of_bytes, write_data);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief I/O Peripheral Device Control
 *   This API is used for setting/getting I/O device parameters/properties
 *
 *  @param phy_info             Represents PHY access\n
 *  @param peripheral_dev       I/O device type (I2c, MDIO, SPI, GPIO, etc.)
 *  @param peripheral_dev_ctrl  parameter or property to be controlled
 *  @param ctrl_data            pointer pointing to the structure containing control value
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_peripheral_device_control(bcm_plp_access_t phy_info,
                                  int peripheral_dev, int peripheral_dev_ctrl,
                                  bcm_plp_peripheral_dev_ctrl_data_t *ctrl_data)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_peripheral_dev_control(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                                 peripheral_dev, peripheral_dev_ctrl, ctrl_data);
  ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Config set for GPIO pins
 *  This set of APIs are used to configure the GPIO pins and
 *  drive the values (set) and obtain the set values (get)
 *  These generic APIs are  provided to the user to configure GPIOs based on need
 *  User needs to provide direction an the pin number for a given pin  
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param gpio_pin_number   GPIO pin number (0-37)
 *  @param cfg_direction   Configuration direction
 *                         1 - in
 *                         0 - out
 *  @param cfg_pull        Configure the pull up or pull down
 *                         1 -  pull up
 *                         0 -  pull down
 *  @param pin_value       Configure the pin value
 *                         1 -  High driven on Pin in o/p mode
 *                         0 -  Low driven on Pin in o/p mode </pre>
 *
 *                         <a href = "../gpio_table.jpg"> EVORA GPIO Table </a>
 *                         <a href = "../gpio_table.jpg"> MIURA GPIO Table </a>
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_cfg_gpio_pin_set(bcm_plp_access_t phy_info, 
                               unsigned int gpio_pin_number, unsigned int cfg_direction,
                               unsigned int cfg_pull, unsigned int pin_value)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_gpio_config_set(
            &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, 
            gpio_pin_number, 
            (plp_aperta_phymod_gpio_mode_t ) (cfg_direction + 1));

    if (rv) {
        goto ERR;
    }

    rv = plp_aperta_phymod_phy_gpio_pin_value_set(
            &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, 
            gpio_pin_number, 
            (cfg_pull << 1)|(pin_value & 1));

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Config get for GPIO pins
 *  Config get the GPIO pins
 *  This API is used to set the GPIO pins pull up or pull down
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param gpio_pin_number GPIO pin number (0-37)
 *  @param cfg_direction   Configuration direction
 *                          1 - in
 *                          0 - out
 *  @param cfg_pull         [OUT] Reserved for future use.
 *                           0 -  pull down
 *                           1 -  pull up
 *                           2 -  no pull up / no pull down
 *  @param pin_value       pin value
 *                           0 - Logic low
 *                           1 - Logic high </pre>
 *
 *                         <a href = "../gpio_table.jpg"> EVORA GPIO Table </a>
 *                         <a href = "../gpio_table.jpg"> MIURA GPIO Table </a>
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_cfg_gpio_pin_get(bcm_plp_access_t phy_info, 
                               unsigned int gpio_pin_number, unsigned int *cfg_direction,
                               unsigned int *cfg_pull, unsigned int *pin_value)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int value;
    plp_aperta_phymod_gpio_mode_t mode;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_gpio_config_get(
            &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
            gpio_pin_number, 
            &mode);

    if(rv) {
        goto ERR;
    }

    *cfg_direction = mode - 1;

    rv = plp_aperta_phymod_phy_gpio_pin_value_get(
            &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
            gpio_pin_number, 
            &value);

    if(!rv) {
        *cfg_pull  = value >> 1;
        *pin_value = value &  1;
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Perform Force Tx Training
 *
 *	This API is used enable or disable force tx training for a specified interface side. (system or line side) 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param enable          Enable/Disable link training\n
 *                         0 - disable \n
 *                         1 - enable  \n
 *                         Note: For some PAM4 chips,\n
 *                         User needs to construct "enable" as follows\n
 *                         bit 3:0 represents enable(1)/disable(0) training\n
 *                         bit 7:4 represents training frame size, used only in case of PAM4\n
 *                                 0 : 4K Frame\n
 *                                 1 : 16K Frame \n
 *                         bit 11:8 represents training init condition applicable for both NRZ and PAM4,\n
 *                                 0 : No link training init condition \n
 *                                 1 : Link training init condition enable\n
 *                         bit 15:12 represents preset type for NRZ mode,\n
 *                                 0: No link training Preset Normal: normal operation\n
 *                                 1: Link training Preset Enable: Preset coefficients \n
 *                         bit 19:16 represents restart,\n
 *                                 0: No restart\n
 *                                 1: restart training\n
 *                         bit 23:20 represents Link Training timer timeout duration which is valid only when \n
 *                                   bit 24 set to 0: Enable LT timer
 *                                   0 : Link Training Timer default value: firmware default
 *                                   1 : Link Training Timer value: 500ms
 *                                   2 : Link Training Timer value: 1s
 *                                   3 : Link Training Timer value: 1.5s
 *                                   4 : Link Training Timer value: 2s
 *                                   5 : Link Training Timer value: 2.5s
 *                                   6 : Link Training Timer value: 3s
 *                                   7 : Link Training Timer value: 3.5s
 *                                   8 : Link Training Timer value: 4s
 *                                   9 : Link Training Timer value: 4.5s
 *                                   10: Link Training Timer value: 5s
 *                                   11: Link Training Timer value: 5.5s
 *                                   12: Link Training Timer value: 6s
 *                                   13: Link Training Timer value: 6.5
 *                                   14: Link Training Timer value: 7s
 *                                   15: Link Training Timer value: 7.5s
 *                         bit 24 : Used to disable the link training timer \n 
 *                                 0: Enable LT timer
 *                                 1: Disable LT timer
 *                         bit 31:25 : Reserved for future use  </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_force_tx_training_set(bcm_plp_access_t phy_info, unsigned int enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_cl72_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Get Tx training state
 *
 *	This API is used to get force Tx Training configuration whether or not enabled
 *  for a specified interface side. (system or line side) 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param enable          [OUT] Represent tx training enabled status\n
 *                         0 - disabled \n
 *                         1 - enabled  
 *                         Note: For some PAM4 chips,
 *                         bit 3:0 represents enable(1)/disable(0) training\n
 *                         bit 7:4 represents training frame size, used only in case of PAM4\n
 *                                 0 : 4K Frame\n
 *                                 1 : 16K Frame \n
 *                         bit 11:8 represents training init condition applicable for both NRZ and PAM4,\n
 *                                 0 : No link training init condition \n
 *                                 1 : Link training init condition enable\n
 *                         bit 15:12 represents preset type for NRZ mode,\n
 *                                 0: No link training Preset Normal: normal operation\n
 *                                 1: Link training Preset Enable: Preset coeffiecients\n
 *                         bit 19:16 represents restart ,\n
 *                                 0: No restart\n
 *                                 1: restart training\n
 *                         bit 23:20 represents Link Training timer timeout duration which is valid only when \n
 *                                   bit 24 set to 0: Enable LT timer
 *                                   0 : Link Training Timer default value: firmware default
 *                                   1 : Link Training Timer value: 500ms
 *                                   2 : Link Training Timer value: 1s
 *                                   3 : Link Training Timer value: 1.5s
 *                                   4 : Link Training Timer value: 2s
 *                                   5 : Link Training Timer value: 2.5s
 *                                   6 : Link Training Timer value: 3s
 *                                   7 : Link Training Timer value: 3.5s
 *                                   8 : Link Training Timer value: 4s
 *                                   9 : Link Training Timer value: 4.5s
 *                                   10: Link Training Timer value: 5s
 *                                   11: Link Training Timer value: 5.5s
 *                                   12: Link Training Timer value: 6s
 *                                   13: Link Training Timer value: 6.5
 *                                   14: Link Training Timer value: 7s
 *                                   15: Link Training Timer value: 7.5s
 *                         bit 24 : Used to disable the link training timer  \n
 *                                 0: Enable LT timer
 *                                 1: Disable LT timer
 *                         bit 31:25 : Reserved for future use  </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_force_tx_training_get(bcm_plp_access_t phy_info, unsigned int *enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_cl72_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable);    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Force Tx training status get 
 *
 *	This API is used to get force Tx Training status for a specified interface side. 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param enable          [OUT] Represents Tx training enabled status\n
 *                         0 - disabled \n
 *                         1 - enabled \n
 *  @param training_failure [OUT] Represents Tx training status\n
 *                          0 - no failure detected\n
 *                          1 - failure detected\n
 *                          Note: For PAM4 chips, Training failure represents,\n
 *                          0 - No Error\n
 *                          1 - Frame lock error\n
 *                          2 - SNR lower then threshold \n
 *                          3 - Link training timeout\n
 *  @param trained         [OUT] Represents Rx status\n
 *                         0 - receiver not trained\n
 *                         1 - receiver trained </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_force_tx_training_status_get(bcm_plp_access_t phy_info, unsigned int *enable, unsigned int *training_failure, unsigned int *trained)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_cl72_status_t cl72_status;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_cl72_status_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &cl72_status);
    *enable = cl72_status.enabled;
#if defined(PHYMOD_ESTOQUE_SUPPORT) || defined(PHYMOD_MILLENIO_SUPPORT)
    *trained = (cl72_status.locked) & 0xF;
    *training_failure = (cl72_status.locked >> 4) & 0xF;
#else
    *trained = cl72_status.locked;
    *training_failure = !cl72_status.locked;
#endif    

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}


/*! \brief Lane config set 
 *
 *	This API is used to set lane config parameter such as DFE settings.
 *
 *  @param phy_info   <pre>Represents PHY access\n
 *  @param firmware_lane_config   Represents lane config parameters\n
 *                         firmware_mode\n
 *                         0 - default mode\n
 *                         1 - dfe mode\n
 *                         2 - osdfe mode\n
 *                         3 - baud rate dfe mode\n
 *                         4 - low power dfe mode\n
 *                         5 - media type sfp dac\n
 *                         6 - media type xlaui\n
 *                         7 - media type optical sr4\n
 *                         8 - bcm_pm_fw_firmware_mode_none\n
 *                         ena_dis\n
 *                         0 - disable\n
 *                         1 - enable\n 
 *                         LaneConfigFromPCS\n
 *                         1 - Lane config from PCS
 *                         AnEnabled\n
 *                         1 - Autonegotiation enabled\n
 *                         0 - Autonegotiation disabled\n
 *                         MediaType\n
 *                         0 - bmplpFirmwareMediaTypePcbTraceBackPlane\n
 *                         1 - bcmplpFirmwareMediaTypeCopperCable\n
 *                         2 - bcmplpFirmwareMediaTypeOptics\n
 *                         UnreliableLos\n
 *                         1 - UnReliable LOS\n
 *                         ScramblingDisable\n
 *                         1 - Scrambling Disable
 *                         Cl72AutoPolEn\n
 *                         1 - Forced CL72 AutoPolEn\n 
 *                         Cl72RestTO\n
 *                         1 - Forced CL72 RestTO</pre>
 *                         ForceES \n
 *                         1 - Force ES Mode
 *                         ForceNS \n
 *                         1 - Force NS Mode
 *                         LinkPartnerPrecoderEn \n
 *                         1 - Link Partner has Precoder Enabled
 *                         ForcePAM4Mode \n
 *                         1 - Override the PAM4 mode pin with PAM4_MODE
 *                         ForceNRZMode \n
 *                         1 - Override the PAM4 mode pin with NRZ_MODE
 *                         DbLoss \n
 *                             DB Loss value: Value range is as per data sheet
 *                         LinkTrainingReStartTimeOut \n
 *                             1 - Link Training restart timeout enable
 *                         OppositeCdrFirst \n
 *                             1 - Wait for opposite cdr_lock to be ready before starting link training
 *                             0 - Start link training first, then check opposite cdr_lock status
 *                        AnTimer \n
 *                             0 - IEEE standard 3s link inhibit timer
 *                             1 - 6s  link inhibit timer
 *                             Applicable only for PAM4 modes
 *                        DfeOn \n
 *                            0 - No dfe mode
 *                            1 - dfe mode
 *                        LpDfeOn\n
 *                            0 - No low power dfe mode
 *                            1 - low power dfe mode
 *                        ForceBrDfe\n
 *                            0 - No baud rate dfe mode
 *                            1 - baud rate dfe mode
 *                        TxSquelchOrPrbs \n
 *                            0 - Squelch TX
 *                            1 - Send PRBS58
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_firmware_lane_config_set(bcm_plp_access_t phy_info, bcm_plp_pm_firmware_lane_config_t* firmware_lane_config)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_firmware_lane_config_t fw_config;
 
    PHYMOD_IF_MEMSET(&fw_config, 0, sizeof(phymod_firmware_lane_config_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_firmware_lane_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &fw_config);
    if (rv) {
        goto ERR;
    }

#ifdef BCM_FIRMWARE_LANE_CONFIG_FIRMWARE_MODE
    firmware_lane_config->firmware_mode = bcm_pm_fw_firmware_mode_none;
#endif

    switch (firmware_lane_config->firmware_mode) {
        case bcm_pm_fw_default:
           /* Firmware lane config get options are set default */
        break;
        case bcm_pm_fw_dfe:
            if (firmware_lane_config->ena_dis) {
                fw_config.DfeOn = 1;
            } else {
                fw_config.DfeOn = 0;
                fw_config.ForceBrDfe = 0;
                fw_config.LpDfeOn = 0;
            }
        break;
        case bcm_pm_fw_osdfe:
            if (firmware_lane_config->ena_dis) {
                fw_config.DfeOn = 1;
                fw_config.ForceBrDfe = 0;
            } else {
                fw_config.DfeOn = 0;
            }
        break;
        case bcm_pm_fw_br_dfe:
            if (firmware_lane_config->ena_dis) {
               fw_config.ForceBrDfe = 1;
               fw_config.DfeOn = 1; 
            } else {
               fw_config.ForceBrDfe = 0;
            }
        break;
        case bcm_pm_fw_lp_dfe:
            if (firmware_lane_config->ena_dis) {
               fw_config.LpDfeOn = 1;
               fw_config.DfeOn = 1; 
            } else {
               fw_config.LpDfeOn = 0;
            }
        break;
        case bcm_pm_fw_sfp_dac:
            fw_config.MediaType = phymodFirmwareMediaTypeCopperCable;
        break;
        case bcm_pm_fw_xlaui:
            fw_config.MediaType = phymodFirmwareMediaTypePcbTraceBackPlane;
        break;
        case bcm_pm_fw_sfp_opt_sr4:
            fw_config.MediaType = phymodFirmwareMediaTypeOptics;
        break;
        case bcm_pm_fw_firmware_mode_none:
            fw_config.DfeOn = firmware_lane_config->DfeOn;
            fw_config.ForceBrDfe = firmware_lane_config->ForceBrDfe;
            fw_config.LpDfeOn = firmware_lane_config->LpDfeOn;
        break;
        default:
           /* Firmware lane config get options are set default */
        break;
    }
    
    fw_config.LaneConfigFromPCS = firmware_lane_config->LaneConfigFromPCS;
    fw_config.AnEnabled = firmware_lane_config->AnEnabled;
    fw_config.MediaType = firmware_lane_config->MediaType;
    fw_config.UnreliableLos = firmware_lane_config->UnreliableLos;
    fw_config.ScramblingDisable = firmware_lane_config->ScramblingDisable;
    fw_config.Cl72AutoPolEn = firmware_lane_config->Cl72AutoPolEn;
    fw_config.Cl72RestTO = firmware_lane_config->Cl72RestTO;
    fw_config.ForceES = firmware_lane_config->ForceES;
    fw_config.ForceNS = firmware_lane_config->ForceNS;
    fw_config.LinkPartnerPrecoderEn = firmware_lane_config->LinkPartnerPrecoderEnable;
    fw_config.ForcePAM4Mode = firmware_lane_config->ForcePAM4Mode;
    fw_config.ForceNRZMode = firmware_lane_config->ForceNRZMode;
    fw_config.DbLoss = firmware_lane_config->DbLoss;
    fw_config.LinkTrainingReStartTimeOut = firmware_lane_config->LinkTrainingReStartTimeOut;
    fw_config.OppositeCdrFirst = firmware_lane_config->OppositeCdrFirst;
    fw_config.AnTimer = firmware_lane_config->AnTimer;
    fw_config.TxSquelchOrPrbs = firmware_lane_config->TxSquelchOrPrbs;
    
    rv = plp_aperta_phymod_phy_firmware_lane_config_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, fw_config);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
/*! \brief firmware lane config get 
 *
 *	This API is used to get the lane config settings for the specified lane
 *
 *  @param phy_info  <pre>Represents PHY access\n
 *  @param firmware_lane_config     Represents lane config parameters\n
 *                         firmware_mode\n
 *                         0 - default mode\n
 *                         1 - dfe mode\n
 *                         2 - osdfe mode\n
 *                         3 - baud rate dfe mode\n
 *                         4 - low power dfe mode\n
 *                         5 - media type sfp dac\n
 *                         6 - media type xlaui\n
 *                         7 - media type optical sr4\n
 *                         8 - bcm_pm_fw_firmware_mode_none\n
 *                         ena_dis\n
 *                         0 - disable\n
 *                         1 - enable\n 
 *                         LaneConfigFromPCS\n
 *                         1 - Lane config from PCS
 *                         AnEnabled\n
 *                         1 - Autonegotiation enabled\n
 *                         0 - Autonegotiation disabled\n
 *                         MediaType\n
 *                         0 - bmplpFirmwareMediaTypePcbTraceBackPlane\n
 *                         1 - bcmplpFirmwareMediaTypeCopperCable\n
 *                         2 - bcmplpFirmwareMediaTypeOptics\n
 *                         UnreliableLos\n
 *                         1 - UnReliable LOS\n
 *                         ScramblingDisable\n
 *                         1 - Scrambling Disable
 *                         Cl72AutoPolEn\n
 *                         1 - Forced CL72 AutoPolEn\n 
 *                         Cl72RestTO\n
 *                         1 - Forced CL72 RestTO</pre>
 *                         ForceES \n
 *                         1 - Force ES Mode
 *                         ForceNS \n
 *                         1 - Force NS Mode
 *                         LinkPartnerPrecoderEn \n
 *                         1 - Link Partner has Precoder En
 *                         ForcePAM4Mode \n
 *                         1 - Override the PAM4 mode pin with PAM4_MODE
 *                         ForceNRZMode \n
 *                         1 - Override the PAM4 mode pin with NRZ_MODE
 *                         DbLoss \n
 *                             DB Loss value: Value ranage is as per data sheet
 *                         LinkTrainingReStartTimeOut \n
 *                             1 - Link Training restart timeout enable
 *                         OppositeCdrFirst \n
 *                             1 - Wait for opposite cdr_lock to be ready before starting link training
 *                             0 - Start link training first, then check opposite cdr_lock status
 *                        AnTimer \n
 *                             0 - IEEE standard 3s link inhibit timer
 *                             1 - 6s  link inhibit timer
 *                             Applicable only for PAM4 modes
 *                        DfeOn \n
 *                            0 - No dfe mode
 *                            1 - dfe mode
 *                        LpDfeOn\n
 *                            0 - No low power dfe mode
 *                            1 - low power dfe mode
 *                        ForceBrDfe\n
 *                            0 - No baud rate dfe mode
 *                            1 - baud rate dfe mode
 *                        TxSquelchOrPrbs \n
 *                            0 - Squelch TX
 *                            1 - Send PRBS58
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_firmware_lane_config_get(bcm_plp_access_t phy_info, bcm_plp_pm_firmware_lane_config_t* firmware_lane_config)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_firmware_lane_config_t fw_config;

    PHYMOD_IF_MEMSET(&fw_config, 0, sizeof(phymod_firmware_lane_config_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_firmware_lane_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &fw_config);
    if (rv) {
        goto ERR;
    }

    firmware_lane_config->ena_dis = 1;
    if ((fw_config.LpDfeOn) && (fw_config.DfeOn)) {
        firmware_lane_config->firmware_mode = bcm_pm_fw_lp_dfe;
    } else if ((fw_config.ForceBrDfe) && (fw_config.DfeOn)) {
        firmware_lane_config->firmware_mode = bcm_pm_fw_br_dfe;
    } else if (fw_config.MediaType == phymodFirmwareMediaTypeCopperCable) {
        firmware_lane_config->firmware_mode = bcm_pm_fw_sfp_dac;
    } else if (fw_config.MediaType == phymodFirmwareMediaTypePcbTraceBackPlane) {
        firmware_lane_config->firmware_mode = bcm_pm_fw_xlaui;
    } else if (fw_config.MediaType == phymodFirmwareMediaTypeOptics){
        firmware_lane_config->firmware_mode = bcm_pm_fw_sfp_opt_sr4;
    } else if ((fw_config.DfeOn) && (fw_config.ForceBrDfe == 0) && (fw_config.LpDfeOn == 0)) {
        firmware_lane_config->firmware_mode = bcm_pm_fw_dfe;
    } else if ((fw_config.DfeOn) && (fw_config.ForceBrDfe == 0)) {
        firmware_lane_config->firmware_mode = bcm_pm_fw_osdfe;
    } else {
        firmware_lane_config->firmware_mode = bcm_pm_fw_default;
        firmware_lane_config->ena_dis = 0;
    }

    firmware_lane_config->LaneConfigFromPCS = fw_config.LaneConfigFromPCS;
    firmware_lane_config->AnEnabled = fw_config.AnEnabled;
    firmware_lane_config->MediaType = fw_config.MediaType;
    firmware_lane_config->UnreliableLos = fw_config.UnreliableLos;
    firmware_lane_config->ScramblingDisable = fw_config.ScramblingDisable;
    firmware_lane_config->Cl72AutoPolEn = fw_config.Cl72AutoPolEn;
    firmware_lane_config->Cl72RestTO = fw_config.Cl72RestTO ;
    firmware_lane_config->ForceES = fw_config.ForceES;
    firmware_lane_config->ForceNS = fw_config.ForceNS;
    firmware_lane_config->LinkPartnerPrecoderEnable = fw_config.LinkPartnerPrecoderEn;
    firmware_lane_config->ForcePAM4Mode = fw_config.ForcePAM4Mode;
    firmware_lane_config->ForceNRZMode = fw_config.ForceNRZMode;
    firmware_lane_config->DbLoss = fw_config.DbLoss;
    firmware_lane_config->LinkTrainingReStartTimeOut = fw_config.LinkTrainingReStartTimeOut;
    firmware_lane_config->OppositeCdrFirst = fw_config.OppositeCdrFirst;
    firmware_lane_config->AnTimer = fw_config.AnTimer;
    firmware_lane_config->DfeOn = fw_config.DfeOn;
    firmware_lane_config->ForceBrDfe = fw_config.ForceBrDfe;
    firmware_lane_config->LpDfeOn = fw_config.LpDfeOn;
#ifdef BCM_FIRMWARE_LANE_CONFIG_FIRMWARE_MODE
    firmware_lane_config->firmware_mode = bcm_pm_fw_firmware_mode_none;
#endif
    firmware_lane_config->TxSquelchOrPrbs = fw_config.TxSquelchOrPrbs;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
/*! \brief Fiber channel/PCS checker enable 
 *
 *  This API is used to Enable/Diable fiber/PCS checker 
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param fcpcs_chkr_mode checker mode\n
 *                         PCS49_1x10G   = 0x0, 10G-KR/LR/SR\n
 *                         PCS82_4x10G   = 0x1, 40G-KR4/LR4/SR4/CR4\n
 *                         PCS82_2x25G   = 0x2, 50G-KR2\n
 *                         PCS82_4x25G   = 0x3, 100G-KR4/LR4/CR4\n
 *                         FC4           = 0x4,\n
 *                         FC8           = 0x5,\n
 *                         FC16          = 0x6,\n
 *                         FC32          = 0x7,\n
 * @param enable           Represents enable/disable fc/pcs checker \n
 *                         1 - Enable\n
 *                         0 - Disable  </pre>
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_fc_pcs_chkr_enable_set(bcm_plp_access_t phy_info, unsigned int fcpcs_chkr_mode, unsigned int enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
 
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_link_mon_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, fcpcs_chkr_mode, enable);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
/*! \brief FC PCS chkr enable get 
 *
 *   This API is used to get status whether FC PCS checker enabled
 *    or disabled
 *     
 *  @param phy_info <pre> Represents PHY access\n
 *  @param fcpcs_chkr_mode FC/PCS checker mode\n
 *                         PCS49_1x10G   = 0x0, 10G-KR/LR/SR\n
 *                         PCS82_4x10G   = 0x1, 40G-KR4/LR4/SR4/CR4\n
 *                         PCS82_2x25G   = 0x2, 50G-KR2\n
 *                         PCS82_4x25G   = 0x3, 100G-KR4/LR4/CR4\n
 *                         FC4           = 0x4,\n
 *                         FC8           = 0x5,\n
 *                         FC16          = 0x6,\n
 *                         FC32          = 0x7,
 *  @param enable          [OUT] Retrives enabled status\n
 *                         1 - Enable\n
 *                         0 - Disable  </pre>
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_fc_pcs_chkr_enable_get(bcm_plp_access_t phy_info, unsigned int fcpcs_chkr_mode, unsigned int* enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
 
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_link_mon_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, fcpcs_chkr_mode, enable);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief FC PCS chkr status get 
 *
 *    This API is used to get the status of PCS checker 
 *
 *  @param phy_info        Represents PHY access\n
 *  @param lock_status     [OUT] Represents lock status\n
 *                         1 - Locked\n
 *                         0 - Unlocked\n
 *  @param lock_lost_lh    [OUT] Represents loss of lock status\n
 *                         1 - Loss of lock set\n
 *                         0 - Loss of lock not set\n
 *  @param error_count     [OUT] Represents error count
 *             
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_fc_pcs_chkr_status_get(bcm_plp_access_t phy_info, unsigned int *lock_status, unsigned int* lock_lost_lh, unsigned int* error_count)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
 
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_link_mon_status_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, lock_status, lock_lost_lh, error_count);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \ failover mode set
 *
 *    This API is used to set failover mode
 *
 *  @param phy_info      Represents PHY access\n
 *  @param failover_mode Represents failovermode to set \n
 *                         0 - bcmFailovermodeNone\n
 *                         1 - bcmFailovermodeEnable\n
 *                  For Millenio Family Chips \n
 *                         0 - bcmplpFailovermodePrimary\n
 *                         1 - bcmplpFailovermodeSecondary\n
 *                  For Barchetta Family Chips \n
 *                         0x00 - bcmplpFailovermodeNone (Fail over mode not enabled. This option is not supported during set operation)\n
 *                         0x01 - bcmplpFailovermodeEnable (Performs state less switch over )\n
 *                         0x21 - bcmplpFailovermodeSwitchToPrimary (Performs switch over to primary path)\n
 *                         0x31 - bcmplpFailovermodeSwitchToSecondary (Performs switch over to secondary path)\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_failover_mode_set(bcm_plp_access_t phy_info,unsigned int failover_mode)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_failover_mode_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,failover_mode);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
/*! \ failover mode get
 *
 *    This API is used to get failover mode
 *
 *  @param phy_info       Represents PHY access\n
 *  @param failover_mode [OUT] Represents failover mode \n
 *                         0 - bcmFailovermodeNone\n
 *                         1 - bcmFailovermodeEnable\n
 *                  For Millenio Family Chips \n
 *                         0 - bcmplpFailovermodePrimary\n
 *                         1 - bcmplpFailovermodeSecondary\n
 *                  For Barchetta Family Chips \n
 *                         Bit[3:0] :- 0x00 : bcmplpFailovermodeNone\n
 *                                  :- 0x01 : bcmplpFailovermodeEnable\n
 *                         Bit[7:0] :- 0x21 : bcmplpFailovermodeSwitchToPrimary\n
 *                                  :- 0x31 : bcmplpFailovermodeSwitchToSecondary\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_failover_mode_get(bcm_plp_access_t phy_info,unsigned int *failover_mode)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_failover_mode_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,failover_mode);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \ failover config set
 *
 *  This API is used to set failover config
 *
 *  @param phy_info  Represents PHY access\n
 *  @param failover_config Represents failover config to set \n
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_failover_config_set(bcm_plp_access_t phy_info, bcm_plp_failover_config_t failover_config)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_failover_config_t failover_cfg;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    PHYMOD_MEMSET(&failover_cfg, 0, sizeof(phymod_failover_config_t));
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    /* coverity[mixed_enums] */
    failover_cfg.failover_mode = failover_config.failover_config_mode;
    /* coverity[mixed_enums] */
    failover_cfg.standby_mode = failover_config.standby_mode;
    /* coverity[mixed_enums] */
    failover_cfg.switch_mode = failover_config.switch_mode;
    rv = plp_aperta_phymod_failover_config_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, failover_cfg);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \ failover config set
 *
 *  This API is used to set failover config
 *
 *  @param phy_info  Represents PHY access\n
 *  @param failover_config Represents failover config to set \n
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_failover_config_get(bcm_plp_access_t phy_info, bcm_plp_failover_config_t *failover_config)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_failover_config_t failover_cfg;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    PHYMOD_MEMSET(&failover_cfg, 0, sizeof(phymod_failover_config_t));
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_failover_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &failover_cfg);
    /* coverity[mixed_enums] */
    failover_config->failover_config_mode = failover_cfg.failover_mode;
    /* coverity[mixed_enums] */
    failover_config->standby_mode = failover_cfg.standby_mode;
    /* coverity[mixed_enums] */
    failover_config->switch_mode = failover_cfg.switch_mode;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \ edc receiver select
 *
 *    This API is used to select edc receiver
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param edc_method    edc receiver configuration method \n
 *                         0 - bcmEdcConfigMethodNone \n
 *                         1 - bcmEdcConfigMethodHardware  EDC mode is set automatically by hardware  \n
 *                         2-  bcmEdcConfigMethodSoftware EDC mode is selected by driver software \n 
 *  @param edc_value      Device-specific EDC mode value (valid only when software configuration method is used) </pre> \n
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_edc_config_set(bcm_plp_access_t phy_info, unsigned int edc_method, unsigned int edc_value )
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_edc_config_t edc_config;
    edc_config.method=edc_method;
    edc_config.mode_val=edc_value;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv=plp_aperta_phymod_edc_config_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &edc_config);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
/*! \ edc receiver get configuration
 *
 *    This API is used to get edc receiver configuration
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param edc_method   [OUT] edc receiver configuration method \n
 *                         0 - bcmEdcConfigMethodNone \n
 *                         1 - bcmEdcConfigMethodHardware  EDC mode is set automatically by hardware  \n
 *                         2-  bcmEdcConfigMethodSoftware EDC mode is selected by driver software \n 
 *  @param edc_value     [OUT] Device-specific EDC mode value (valid only when software configuration method is used) </pre> \n
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_edc_config_get(bcm_plp_access_t phy_info, unsigned int *edc_method, unsigned int *edc_value )
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_edc_config_t edc_config;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv=plp_aperta_phymod_edc_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &edc_config);
    if(!rv){
        *edc_method=edc_config.method;
        *edc_value=edc_config.mode_val;
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Driver SW version.
 *
 *	This API is used to retrive SW version 
 *
 *  @param phy_info        Represents PHY access\n
 *  @param phy_chip_name   [OUT] Retrives phy chip name 
 *  @param major_ver       [OUT] Retrives major version number
 *  @param minor_ver       [OUT] Retrives minor version number
 *
 */
int
bcm_plp_aperta_driver_version_get(bcm_plp_access_t phy_info, char *phy_chip_name, unsigned short *major_ver, unsigned short *minor_ver)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_core_info_t core_info;

    PHYMOD_IF_MEMSET(&core_info, 0, sizeof(plp_aperta_phymod_core_info_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_CORE_ACCESS(phy_info, phy_id_idx);
    *major_ver = BCM_PM_IF_MAJOR_VER_NO; 
    *minor_ver = BCM_PM_IF_MINOR_VER_NO;
    if (phy_chip_name == NULL) {
        PHYMOD_DEBUG_ERROR(("Invalid chip name\n"));
        rv = BCM_PM_IF_PARAM;
        goto ERR;
    }
    rv = plp_aperta_phymod_core_info_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core->pm_core, &core_info);
    /* coverity[array_null] */
    if ((rv == 0) && (core_info.name[0] != '\0')) {
        PHYMOD_STRCPY(phy_chip_name, core_info.name);
    } else {
        PHYMOD_DIAG_OUT(("Driver version information not available\n"));
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief FEC corrected error counter.
 *
 *	This API is used to retrive FEC corrected code word counter.
 *  Counts once for each corrected FEC codeword processed when fec_align_status is true. 
 *
 *  @param phy_info        Represents PHY access\n
 *  @param fec_type        [IN] Represents FEC type,
 *                          0 - bcmplpFecType91
 *                          1 - bcmplpFecType74
 *                          2 - bcmplpFecType108
 *  @param count           [OUT] FEC corrected word counter
 *
 */
int
bcm_plp_aperta_fec_corrected_error_counter(bcm_plp_access_t phy_info, bcm_plp_fec_type_t fec_type, unsigned int *count)
{
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_fec_type_t pm_fec_type = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    if (fec_type == bcmplpFecType91) {
        pm_fec_type = phymod_fec_CL91;
    } else if (fec_type == bcmplpFecType74){
        pm_fec_type = phymod_fec_CL74;
    } else if (fec_type == bcmplpFecType108){
        pm_fec_type = phymod_fec_CL108;
    } else if (fec_type == bcmplpFecTypeRS544){
        pm_fec_type = phymod_fec_RS544;
    } else if (fec_type == bcmplpFecTypeRS272){
        pm_fec_type = phymod_fec_RS272;
    } else if (fec_type == bcmplpFecTypeRS544_2XN){
        pm_fec_type = phymod_fec_RS544_2XN;
    } else if (fec_type == bcmplpFecTypeRS272_2XN){
        pm_fec_type = phymod_fec_RS272_2XN;
    }else {
        PHYMOD_DEBUG_ERROR(("Invalid FEC type\n"));
        rv = BCM_PM_IF_PARAM;
        goto ERR;
    }
    rv = plp_aperta_phymod_phy_fec_correctable_counter_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pm_fec_type, count);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief FEC uncorrected error counter.
 *
 *	This API is used to retrive FEC uncorrected code word counter.
 *  Counts once for each uncorrected FEC codeword processed when fec_align_status is true. 
 *
 *  @param phy_info        Represents PHY access\n
 *  @param fec_type        [IN] Represents FEC type,
 *                          0 - bcmplpFecType91
 *                          1 - bcmplpFecType74
 *                          2 - bcmplpFecType108
 *  @param count           [OUT] FEC uncorrected word counter
 *
 */
int
bcm_plp_aperta_fec_uncorrected_error_counter(bcm_plp_access_t phy_info, bcm_plp_fec_type_t fec_type, unsigned int *count)
{
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_fec_type_t pm_fec_type = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    if (fec_type == bcmplpFecType91) {
        pm_fec_type = phymod_fec_CL91;
    } else if (fec_type == bcmplpFecType74){
        pm_fec_type = phymod_fec_CL74;
    } else if (fec_type == bcmplpFecType108){
        pm_fec_type = phymod_fec_CL108;
    } else if (fec_type == bcmplpFecTypeRS544){
        pm_fec_type = phymod_fec_RS544;
    } else if (fec_type == bcmplpFecTypeRS272){
        pm_fec_type = phymod_fec_RS272;
    } else if (fec_type == bcmplpFecTypeRS544_2XN){
        pm_fec_type = phymod_fec_RS544_2XN;
    } else if (fec_type == bcmplpFecTypeRS272_2XN){
        pm_fec_type = phymod_fec_RS272_2XN;
    } else {
        PHYMOD_DEBUG_ERROR(("Invalid FEC type\n"));
        rv = BCM_PM_IF_PARAM;
        goto ERR;
    }
    rv = plp_aperta_phymod_phy_fec_uncorrectable_counter_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pm_fec_type, count);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief 1588 TimeSync config set 
 *
 *  This API is used to configure PTP IEEE1588 TimeSync 
 *
 *  @param phy_info       Represents PHY access
 *  @param config         Points to the structure containing 1588 TimeSync config parameters
 *                        (for details see bcm_plp_timesync_config_t in common defines header file)
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_config_set(bcm_plp_access_t phy_info,
                                bcm_plp_timesync_config_t* config) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_timesync_config_t phymod_ts_config;

    PHYMOD_IF_MEMSET(&phymod_ts_config, 0, sizeof(plp_aperta_phymod_timesync_config_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    phymod_ts_config.capabilities = config->validity_mask;
    BCM_PLP_CONVERT_TS_CONFIG((&phymod_ts_config), config);
    rv = plp_aperta_phymod_timesync_config_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phymod_ts_config);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief 1588 TimeSync config get 
 *
 *  This API is used to get PTP IEEE1588 TimeSync config parameters and information
 *
 *  @param phy_info       Represents PHY access
 *  @param config         Points to the structure containing 1588 TimeSync config parameters
 *                        (for details see bcm_plp_timesync_config_t in common defines header file)
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_config_get(bcm_plp_access_t phy_info,
                                bcm_plp_timesync_config_t* config) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_timesync_config_t phymod_ts_config;

    PHYMOD_IF_MEMSET(&phymod_ts_config, 0, sizeof(plp_aperta_phymod_timesync_config_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_timesync_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phymod_ts_config);
    config->validity_mask = phymod_ts_config.capabilities;
    BCM_PLP_CONVERT_TS_CONFIG(config, (&phymod_ts_config));
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief 1588 TimeSync enable set 
 *
 *  This API is used to enable/disable PTP IEEE1588 TimeSync
 *  @param phy_info        Represents PHY access
 *  @param flags           See PHYMOD_TS_DIRECTION
 *  @param enable          Timesync enable/disable
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_enable_set(bcm_plp_access_t phy_info,
                                unsigned int flags, unsigned int enable) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                      flags, enable);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief 1588 TimeSync enable get 
 *
 *  This API is used to get PTP IEEE1588 TimeSync enable/disable status
 *  @param phy_info        Represents PHY access
 *  @param flags           See PHYMOD_TS_DIRECTION
 *  @param enable          Timesync enable/disable
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_enable_get(bcm_plp_access_t phy_info,
                                unsigned int flags, unsigned int* enable) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                      flags, enable);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_nco_addend_set
 *
 * @brief Set the frequency control word in nanoseconds
 *
 * @param phy_info        Represents PHY access
 * @param flags           See PHYMOD_TS_DIRECTION
 * @param freq_step       NCO frequency control word (device-specific)
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_nco_addend_set(bcm_plp_access_t phy_info,
                                unsigned int flags, unsigned int freq_step) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_nco_addend_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, freq_step);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_nco_addend_get
 *
 * @brief Get the frequency control word in nanoseconds
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           See PHYMOD_TS_DIRECTION
 * @param  freq_step       NCO frequency control word (device-specific)
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_nco_addend_get(bcm_plp_access_t phy_info, unsigned int flags, unsigned int* freq_step) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_nco_addend_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, freq_step);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_framesync_mode_set
 *
 * @brief Set framesync mode 
 *
 * @param phy_info        Represents PHY access
 * @param flags           See PHYMOD_TS_DIRECTION
 * @param framesync       Framesync mode
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_framesync_mode_set(bcm_plp_access_t phy_info, unsigned int flags,
                                    bcm_plp_timesync_framesync_t* framesync) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_timesync_framesync_t phymod_ts_fsync ;

    PHYMOD_IF_MEMSET(&phymod_ts_fsync, 0, sizeof(plp_aperta_phymod_timesync_framesync_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    BCM_PLP_CONVERT_TS_FSYNC((&phymod_ts_fsync), framesync);
    rv = plp_aperta_phymod_timesync_framesync_mode_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phymod_ts_fsync);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_framesync_mode_get
 *
 * @brief Get framesync mode 
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           See PHYMOD_TS_DIRECTION
 * @param  framesync       Framesync mode
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_framesync_mode_get(bcm_plp_access_t phy_info, unsigned int flags,
                                    bcm_plp_timesync_framesync_t* framesync) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_timesync_framesync_t phymod_ts_fsync ;

    PHYMOD_IF_MEMSET(&phymod_ts_fsync, 0, sizeof(plp_aperta_phymod_timesync_framesync_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_timesync_framesync_mode_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phymod_ts_fsync);
    BCM_PLP_CONVERT_TS_FSYNC(framesync, (&phymod_ts_fsync));
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_local_time_set
 *
 * @brief Set local time 
 *
 * @param phy_info        Represents PHY access
 * @param flags           See PHYMOD_TS_DIRECTION
 * @param local_time      Local time (device-specific)
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_local_time_set(bcm_plp_access_t phy_info,
                                unsigned int flags, plp_uint64_t local_time) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_local_time_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, local_time);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_local_time_get
 *
 * @brief Get local time 
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           See PHYMOD_TS_DIRECTION
 * @param  local_time      Local time (device-specific)
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_local_time_get(bcm_plp_access_t phy_info,
                                unsigned int flags, plp_uint64_t* local_time) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_local_time_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, local_time);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_load_ctrl_set
 *
 * @brief Set load mode for loading shadow registers
 *
 * @param phy_info        Represents PHY access
 * @param flags           See PHYMOD_TS_DIRECTION
 * @param load_once       Load control mask for one-time shadow register load
 * @param load_always     Load control mask for loading shadow register persistently across FrameSync)
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_load_ctrl_set(bcm_plp_access_t phy_info, unsigned int flags,
                               unsigned int load_once, unsigned int load_always) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_load_ctrl_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                         load_once, load_always);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_load_ctrl_get
 *
 * @brief Get load mode for loading shadow registers 
 *
 * @param phy_info        Represents PHY access
 * @param flags           See PHYMOD_TS_DIRECTION
 * @param load_once       Load control mask for one-time shadow register load
 * @param load_always     Load control mask for loading shadow register persistently across FrameSync)
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_load_ctrl_get(bcm_plp_access_t phy_info, unsigned int flags,
                               unsigned int* load_once, unsigned int* load_always) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_load_ctrl_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                         load_once, load_always);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_timing_control_set
 *
 * @brief Set TimeSync timing control timer/clock/frequency value
 *
 * @param phy_info              PHY access structure
 * @param flags                 reserved for future use
 * @param time_ctrl             .type       =  Timing control type\n
 *                              .time_value =  Timing control value
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_timing_control_set(bcm_plp_access_t phy_info, unsigned int flags,
                                    bcm_plp_timesync_time_value_t *time_ctrl)
{
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_timing_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                            time_ctrl->type, time_ctrl->time_value,
                                            time_ctrl->time_value_hi, time_ctrl->dpll_time_value,
                                            time_ctrl->dpll_time_value_hi);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_timing_control_get
 *
 * @brief Get TimeSync timing control timer/clock/frequency value
 *
 * @param phy_info               PHY access structure
 * @param flags                  reserved for future use
 * @param time_ctrl             .type       =  Timing control type\n
 *                              .time_value =  Timing control value\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_timing_control_get(bcm_plp_access_t phy_info, unsigned int flags,
                                    bcm_plp_timesync_time_value_t *time_ctrl)
{
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_timing_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                    time_ctrl->type, &(time_ctrl->time_value),
                                    &time_ctrl->time_value_hi, &time_ctrl->dpll_time_value,
                                    &time_ctrl->dpll_time_value_hi);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_link_delay_set
 *
 * @brief Set TimeSync Link Delay
 *
 * @param phy_info              PHY access structure
 * @param flags                 reserved for future use
 * @param linkdelay             .direction  =  1: Rx  (only Rx supported)\n
 *                              .op         =  1: add, 2: subtract\n
 *                              .type       =  PTP message types bitmap\n
 *                              .time_value =  TimeSync link delay value\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_link_delay_set(bcm_plp_access_t phy_info, unsigned int flags,
                                bcm_plp_timesync_time_value_t *linkdelay)
{
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_link_delay_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                        linkdelay->direction, linkdelay->op,
                                        linkdelay->type     , linkdelay->time_value);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_link_delay_get
 *
 * @brief Get TimeSync Link Delay
 *
 * @param phy_info              PHY access structure
 * @param flags                 reserved for future use
 * @param linkdelay             .direction  =  1: Rx  (only Rx supported)\n
 *                              .op         =  1: add, 2: subtract\n
 *                              .type       =  PTP message types bitmap\n
 *                              .time_value =  TimeSync link delay value\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_link_delay_get(bcm_plp_access_t phy_info, unsigned int flags,
                                bcm_plp_timesync_time_value_t *linkdelay)
{
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_link_delay_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                          linkdelay->direction, &(linkdelay->op),
                                        &(linkdelay->type)    , &(linkdelay->time_value));
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_timestamp_offset_set
 *
 * @brief Set TimeSync Tx/Rx AFE countstamp offset
 *
 * @param phy_info              PHY access structure
 * @param flags                 reserved for future use
 * @param ts_offset             .direction  =  0: Tx & Rx, 1: Rx, 2: Tx\n
 *                              .op         =  1: add, 2: subtract\n
 *                              .type       =  PTP message types bitmap\n
 *                              .time_value =  AFE countstamp offset in ns\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_timestamp_offset_set(bcm_plp_access_t phy_info, unsigned int flags,
                                      bcm_plp_timesync_time_value_t *ts_offset)
{
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_timestamp_offset_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                        ts_offset->direction, ts_offset->op,
                                        ts_offset->type     , ts_offset->time_value);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_timestamp_offset_get
 *
 * @brief Get TimeSync Tx/Rx AFE countstamp offset
 *
 * @param phy_info              PHY access structure
 * @param flags                 reserved for future use
 * @param ts_offset             .direction  =  0: Tx & Rx, 1: Rx, 2: Tx\n
 *                              .op         =  1: add, 2: subtract\n
 *                              .type       =  PTP message types bitmap\n
 *                              .time_value =  AFE countstamp offset in ns\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_timestamp_offset_get(bcm_plp_access_t phy_info, unsigned int flags,
                                      bcm_plp_timesync_time_value_t *ts_offset)
{
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_timestamp_offset_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                          ts_offset->direction, &(ts_offset->op),
                                        &(ts_offset->type)    , &(ts_offset->time_value));
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_tx_timestamp_offset_set
 *
 * @brief Set timesync Tx AFE countstamp offset 
 *
 * @param phy_info        Represents PHY access
 * @param flags           See PHYMOD_TS_DIRECTION
 * @param ts_offset       Tx AFE countstamp offset in ns
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_tx_timestamp_offset_set(bcm_plp_access_t phy_info,
                                         unsigned int flags, unsigned int ts_offset) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_tx_timestamp_offset_set(
                                &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, ts_offset);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_tx_timestamp_offset_get
 *
 * @brief Get timesync Tx AFE countstamp offset 
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           See PHYMOD_TS_DIRECTION
 * @param  ts_offset       Tx AFE countstamp offset in ns
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_tx_timestamp_offset_get(bcm_plp_access_t phy_info,
                                         unsigned int flags, unsigned int* ts_offset) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_tx_timestamp_offset_get(
                                &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, ts_offset);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_rx_timestamp_offset_set
 *
 * @brief Set timesync Rx AFE countstamp offset 
 *
 * @param phy_info        Represents PHY access
 * @param flags           See PHYMOD_TS_DIRECTION
 * @param ts_offset       Rx AFE countstamp offset in ns
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_rx_timestamp_offset_set(bcm_plp_access_t phy_info,
                                         unsigned int flags, unsigned int ts_offset) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_rx_timestamp_offset_set(
                                &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, ts_offset);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_rx_timestamp_offset_get
 *
 * @brief Get timesync Rx AFE countstamp offset 
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           See PHYMOD_TS_DIRECTION
 * @param  ts_offset       Rx AFE countstamp offset in ns
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_rx_timestamp_offset_get(bcm_plp_access_t phy_info,
                                         unsigned int flags, unsigned int* ts_offset) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_rx_timestamp_offset_get(
                                &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, ts_offset);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_time_code_set
 *
 * @brief Set TimeSync Time Code
 *
 * @param phy_info              PHY access structure
 * @param flags                 reserved for future use
 * @param timecode              .seconds     =  second value\n
 *                              .nanoseconds =  nanosecond value\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_time_code_set(bcm_plp_access_t phy_info, unsigned int flags,
                               bcm_plp_timesync_timespec_t *timecode) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy  = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_timesync_timespec_t tcode;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    COMPILER_64_COPY(tcode.seconds, timecode->seconds);
    tcode.nanoseconds = timecode->nanoseconds;
    tcode.syncref_delay.enable = timecode->syncref_delay.enable ;
    COMPILER_64_COPY(tcode.syncref_delay.value, timecode->syncref_delay.value);
    rv = plp_aperta_phymod_timesync_time_code_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                       flags, &tcode);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    return rv;
}

/*!
 * bcm_plp_aperta_timesync_time_code_get
 *
 * @brief Get TimeSync Time Code
 *
 * @param phy_info              PHY access structure
 * @param flags                 reserved for future use
 * @param timecode              .seconds     =  second value\n
 *                              .nanoseconds =  nanosecond value\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_time_code_get(bcm_plp_access_t phy_info, unsigned int flags,
                               bcm_plp_timesync_timespec_t *timecode) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy  = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_timesync_timespec_t tcode;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_time_code_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                       flags, &tcode);
    timecode->nanoseconds = tcode.nanoseconds;
    COMPILER_64_COPY(timecode->seconds, tcode.seconds);
    COMPILER_64_COPY(timecode->nanoseconds64, tcode.nanoseconds64);
    timecode->syncref_delay.enable = tcode.syncref_delay.enable ;
    COMPILER_64_COPY(timecode->syncref_delay.value, tcode.syncref_delay.value);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    return rv;
}

/*!
 * bcm_plp_aperta_timesync_sopmem_set
 *
 * @brief Set the properties of IEEE1588 SOP_FIFO/SOPmem (start of packet memory)
 *
 * @param phy_info      PHY access structure
 * @param flags         Reserved for future use
 * @param index         Reserved for future use
 * @param sopentry      Points to the structure containing SOPmem entry contents\n
 *                      - lookup_key_mask: select one of 4 PCH Timestamp Capture modes\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_sopmem_set(bcm_plp_access_t phy_info, unsigned int flags,
                            int index, bcm_plp_timesync_sopmem_t *sopentry)
{
    phymod_timesync_sopmem_t  sopmem_entry;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    PHYMOD_MEMCPY(&sopmem_entry, sopentry, sizeof(phymod_timesync_sopmem_t));
    PHYMOD_MEMCPY( sopmem_entry.src_ip   , sopentry->classified_data ,
                                           sizeof(sopmem_entry.src_ip));
    rv = plp_aperta_phymod_timesync_sopmem_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags,
                                    &sopmem_entry);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_sopmem_get
 *
 * @brief Retrieve an IEEE1588 SOP_FIFO/SOPmem (start of packet memory) entry
 *
 * @param phy_info              PHY access structure
 * @param flags                 1: lookup all entries, 2: clear one entry, 4: clear all
 * @param index                 index of SOPmem entry (0 - max number supported)
 * @param sopentry              Points to the structure containing SOPmem entry contents\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_sopmem_get(bcm_plp_access_t phy_info, unsigned int flags,
                            int index, bcm_plp_timesync_sopmem_t *sopentry)
{

#if !defined(PHYMOD_APERTA2_SUPPORT)
    phymod_timesync_sopmem_t  sopmem_entry;
#else
    phymod_timesync_sopmem_t  sopmem_entry[BCM_PLP_TIMESYNC_SOPMEM_SIZE];
#endif
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
#if !defined (PHYMOD_APERTA2_SUPPORT)
    PHYMOD_MEMCPY(&sopmem_entry, sopentry, sizeof(phymod_timesync_sopmem_t));
    if ( BCMPLP_TIMESYNC_SOPMEM_TO_BE_READ_BYTE == sopentry->classified_data[0] ) {
        PHYMOD_MEMCPY(sopmem_entry.src_ip, sopentry->classified_data,
                                      sizeof(sopmem_entry.src_ip));
    }
    rv = plp_aperta_phymod_timesync_sopmem_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags,
                                    index, &sopmem_entry);
    PHYMOD_MEMCPY(sopentry, &sopmem_entry, sizeof(phymod_timesync_sopmem_t));
    PHYMOD_MEMCPY(sopentry->classified_data, sopmem_entry.src_ip,
                                      sizeof(sopmem_entry.src_ip));
#else
    PHYMOD_MEMCPY(&sopmem_entry, sopentry, sizeof(sopmem_entry));
    rv = plp_aperta_phymod_timesync_sopmem_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, index, sopmem_entry);
    PHYMOD_MEMCPY(sopentry, &sopmem_entry, sizeof(sopmem_entry));
#endif
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_capture_timestamp_get
 *
 * @brief Capture timesync timestamp 
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           See PHYMOD_TS_DIRECTION
 * @param  cap_ts          Capture timestamp
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_capture_timestamp_get(bcm_plp_access_t phy_info,
                                       unsigned int flags, plp_uint64_t* cap_ts) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_capture_timestamp_get(
                                &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, cap_ts);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_heartbeat_timestamp_get
 *
 * @brief Get timesync heartbeat 
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           See PHYMOD_TS_DIRECTION
 * @param  hb_ts           Heartbeat timestamp
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_heartbeat_timestamp_get(bcm_plp_access_t phy_info,
                                         unsigned int flags, plp_uint64_t* hb_ts) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_heartbeat_timestamp_get(
                                &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, hb_ts);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_inband_filter_set
 *
 * @brief Set timesync inband filter
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           Rx/Ingress or Tx/Egress, set 1 for Rx, 2 for Tx
 * @param  index	   Filter Index, set one of 32 indices in range 0-31
 * @param  config	   Filter Config pointer, set using bcm_plp_timesync_inband_filter_ctrl_t
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_inband_filter_set(bcm_plp_access_t phy_info, unsigned int flags,
                                   int index, bcm_plp_timesync_inband_filter_ctrl_t *config) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_timesync_inband_filter_ctrl_t phymod_ts_ib_flt;

    PHYMOD_IF_MEMSET(&phymod_ts_ib_flt, 0, sizeof(phymod_timesync_inband_filter_ctrl_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    BCM_PLP_CONVERT_TS_INBAND_FILTER((&phymod_ts_ib_flt), config);
    rv = plp_aperta_phymod_timesync_inband_filter_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, index, &phymod_ts_ib_flt);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_inband_filter_get
 *
 * @brief Get timesync inband filter
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           Rx/Ingress or Tx/Egress, set 1 for Rx, 2 for Tx
 * @param  index	   Filter Index, set one of 32 indices in range 0-31
 * @param  config	   Filter Config pointer, set using bcm_plp_timesync_inband_filter_ctrl_t
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_inband_filter_get(bcm_plp_access_t phy_info, unsigned int flags,
                                   int index, bcm_plp_timesync_inband_filter_ctrl_t *config) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_timesync_inband_filter_ctrl_t phymod_ts_ib_flt;

    PHYMOD_IF_MEMSET(&phymod_ts_ib_flt, 0, sizeof(phymod_timesync_inband_filter_ctrl_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_timesync_inband_filter_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, index, &phymod_ts_ib_flt);
    BCM_PLP_CONVERT_TS_INBAND_FILTER(config, (&phymod_ts_ib_flt));
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_mpls_set
 *
 * @brief Set timesync MPLS 
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           MPLS lable matching direction (1=Rx, 2=Tx, 3=RxTx)
 * @param  enable          Enable/Disable MPLS
 * @param  mpls_ctrl       Pointer to the data structure containing MPLS parameters
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_mpls_set(bcm_plp_access_t phy_info, unsigned int flags,
                          int enable, bcm_plp_timesync_mpls_ctrl_t *mpls_ctrl) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_timesync_mpls_ctrl_t phymod_ts_mpls_ctrl;

    PHYMOD_IF_MEMSET(&phymod_ts_mpls_ctrl, 0, sizeof(plp_aperta_phymod_timesync_mpls_ctrl_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    if ( enable ) {
        mpls_ctrl->flags |=  bcmplpTimesyncMplsEnable;
    } else {
        mpls_ctrl->flags &= ~bcmplpTimesyncMplsEnable;
    }
    BCM_PLP_CONVERT_TS_MPLS_CTRL((&phymod_ts_mpls_ctrl), mpls_ctrl);
    rv = plp_aperta_phymod_timesync_mpls_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, &phymod_ts_mpls_ctrl);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! 
 * bcm_plp_aperta_timesync_mpls_get
 *
 * @brief Get timesync MPLS 
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           MPLS lable matching direction (1=Rx, 2=Tx, 3=RxTx)
 * @param  enable          MPLS Enable/Disable status
 * @param  mpls_ctrl       Pointer to the data structure containing MPLS parameters
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_mpls_get(bcm_plp_access_t phy_info, unsigned int flags,
                          int enable, bcm_plp_timesync_mpls_ctrl_t *mpls_ctrl) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    plp_aperta_phymod_timesync_mpls_ctrl_t phymod_ts_mpls_ctrl;

    PHYMOD_IF_MEMSET(&phymod_ts_mpls_ctrl, 0, sizeof(plp_aperta_phymod_timesync_mpls_ctrl_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_timesync_mpls_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, &phymod_ts_mpls_ctrl);
    BCM_PLP_CONVERT_TS_MPLS_CTRL(mpls_ctrl, (&phymod_ts_mpls_ctrl));
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_pch_set
 *
 * @brief Set timesync PCH (packet control header)
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           Reserved for future use
 * @param  pch             PCH parameters
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_pch_set(bcm_plp_access_t phy_info,
                         unsigned int flags, bcm_plp_timesync_pch_t *pch) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    phymod_timesync_pch_t phymod_ts_pch;

    PHYMOD_IF_MEMSET(&phymod_ts_pch, 0, sizeof(phymod_timesync_pch_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    BCM_PLP_CONVERT_TS_PCH((&phymod_ts_pch), pch);
    rv = plp_aperta_phymod_timesync_pch_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, &phymod_ts_pch);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_pch_get
 *
 * @brief Get the setting of timesync PCH (packet control header)
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           Reserved for future use
 * @param  pch             PCH parameters
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_pch_get(bcm_plp_access_t phy_info,
                         unsigned int flags, bcm_plp_timesync_pch_t *pch) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_timesync_pch_t phymod_ts_pch;

    PHYMOD_IF_MEMSET(&phymod_ts_pch, 0, sizeof(phymod_timesync_pch_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_timesync_pch_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, &phymod_ts_pch);
    BCM_PLP_CONVERT_TS_PCH(pch, (&phymod_ts_pch));
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timesync_do_sync
 *
 * @brief Force a framesync event 
 *
 * @param phy_info         Represents PHY access
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timesync_do_sync(bcm_plp_access_t phy_info) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_timesync_framesync_t framesync;
    plp_aperta_phymod_timesync_framesync_mode_t original_mode;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_framesync_mode_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &framesync);
    if ( rv != BCM_PM_IF_SUCCESS ) {
        goto ERR;
    }
    original_mode = framesync.mode;      /* remember current FrameSync mode */

    /* set FrameSync mode to be NONE */
    framesync.mode = phymodTimesyncFramsyncModeNone;
    rv = plp_aperta_phymod_timesync_framesync_mode_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &framesync);
    if ( rv != BCM_PM_IF_SUCCESS ) {
        goto ERR;
    }
    /* set FrameSync to be CPU mode to trigger a FrameSync pulse immediately */
    framesync.mode = phymodTimesyncFramsyncModeCpu;
    rv = plp_aperta_phymod_timesync_framesync_mode_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &framesync);
    if ( rv != BCM_PM_IF_SUCCESS ) {
        goto ERR;
    }

    if ( phymodTimesyncFramsyncModeCpu != original_mode ) {
        framesync.mode = original_mode;    /* resume the FrameSync mode*/
        rv = plp_aperta_phymod_timesync_framesync_mode_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &framesync);
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timestamp_univ_set
 *
 * @brief Set universal timestamp timers
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           Reserved for future use
 * @param  type            Timer type
 * @param  timeuniv        Timer value and operation
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timestamp_univ_set(bcm_plp_access_t phy_info, unsigned int flags,
                           int type, bcm_plp_timestamp_univ_t *timeuniv) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_timestamp_univ_t phymod_ts_univ ;

    PHYMOD_IF_MEMSET(&phymod_ts_univ, 0, sizeof(phymod_timestamp_univ_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    BCM_PLP_CONVERT_TS_UNIV((&phymod_ts_univ), timeuniv);
    rv = plp_aperta_phymod_timestamp_univ_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, type, &phymod_ts_univ);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_timestamp_univ_get
 *
 * @brief Get universal timestamp timers
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           Reserved for future use
 * @param  type            Timer type
 * @param  timeuniv        Timer value and operation
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_timestamp_univ_get(bcm_plp_access_t phy_info, unsigned int flags,
                           int type, bcm_plp_timestamp_univ_t *timeuniv) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_timestamp_univ_t phymod_ts_univ ;

    PHYMOD_IF_MEMSET(&phymod_ts_univ, 0, sizeof(phymod_timestamp_univ_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_timestamp_univ_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, type, &phymod_ts_univ);
    BCM_PLP_CONVERT_TS_UNIV(timeuniv, (&phymod_ts_univ));
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

#ifdef BCM_PLP_TIMESYNC_V2_1_SUPPORT

/*!
 * bcm_timesync_ptp_action_set
 *
 * @brief Set PTP lookup action mode
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           Reserved for future use
 * @param  direction       1=Rx  2=Tx
 * @param  mode            PTP lookup-action mode (see bcm_plp_timesync_ptp_action_mode_t)
 * @param  user_def        pointer to the data structure containing
 *                         user-defined actions for classified PTP messages
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_timesync_ptp_action_set(bcm_plp_access_t phy_info, unsigned int flags,
                                bcm_plp_timesync_txrx_t  direction,
                                bcm_plp_timesync_ptp_action_mode_t mode, void *user_def) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    PHYMOD_NULL_CHECK(user_def);
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_ptp_action_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags,
                                                       direction, mode, user_def);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_timesync_ptp_action_get
 *
 * @brief Get PTP lookup action mode
 *
 * @param  phy_info        Represents PHY access
 * @param  flags           Reserved for future use
 * @param  direction       1=Rx  2=Tx
 * @param  mode            PTP lookup-action mode (see bcm_plp_timesync_ptp_action_mode_t)
 * @param  user_def        pointer to the data structure containing
 *                         user-defined actions for classified PTP messages
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_timesync_ptp_action_get(bcm_plp_access_t phy_info, unsigned int flags,
                                bcm_plp_timesync_txrx_t  direction,
                                bcm_plp_timesync_ptp_action_mode_t *mode, void *user_def) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    PHYMOD_NULL_CHECK(mode);
    PHYMOD_NULL_CHECK(user_def);
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_timesync_ptp_action_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags,
                                                       direction, mode, user_def);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

#endif  /* BCM_PLP_TIMESYNC_V2_1_SUPPORT */

/*!
 * bcm_plp_aperta_l1_intr_status_get
 *
 * @brief L1 interrupt status
 *
 *  This API is used to read the Level 1 interrupts status of the specified PHYID.
 *
 *  @param phy_info       Represents PHY access
 *  @param [out] l1_intr_status  - Level 1 Interrupt Status
 *
 *  NOTE: This API gives status of Level 1 interrupts.
 *    Each bit of L1 interrupt status output gives interrupt status of one type of interrupt.
 *    It is expected user need to read more details from "External Interrupt Status" register
 *    to call particular L2 interrupts status API(If chip supported).
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_l1_intr_status_get(bcm_plp_access_t phy_info, bcm_plp_l1_intr_status_t *l1_intr_status) {
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

#ifdef PHYMOD_EVORA_SUPPORT
    plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.devad = 1;
    rv = plp_aperta_phymod_phy_reg_read(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, 0x1008bfb, &l1_intr_status->l1_intr_sts);
#else
    PHYMOD_DEBUG_ERROR(("API not available\n"));
    rv = BCM_PM_IF_UNAVAIL;
#endif

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Core Diagnostics
 *
 *  This API is used to retrieve Core Diagnostics of a core
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param core_diag      [OUT]Attributes for core diagnosis </pre>
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_core_diagnostics_get(bcm_plp_access_t phy_info,  bcm_plp_core_diagnostics_t* core_diag)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    plp_aperta_phymod_core_diagnostics_t phy_core_diag;

    PHYMOD_IF_MEMSET(&phy_core_diag, 0, sizeof(plp_aperta_phymod_core_diagnostics_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_CORE_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_core_diagnostics_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core->pm_core, &phy_core_diag);
    if ( BCM_PM_IF_SUCCESS == rv ) {
        core_diag->temperature = phy_core_diag.temperature;
        core_diag->voltage = phy_core_diag.voltage;
        core_diag->pll_range = phy_core_diag.pll_range;
    }

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

#if defined(PHYMOD_ESTOQUE_SUPPORT) || defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined(PHYMOD_APERTA2_SUPPORT) || defined(PHYMOD_AGERA2_SUPPORT)

/*!
 * bcm_avs_config_set
 *
 * @brief AVS configuration
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param [in]  avs_config      AVS configuration </pre>
 */
int bcm_avs_config_set(bcm_plp_access_t phy_info, bcm_plp_avs_config_t avs_config)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_avs_config_t phy_avs_config;
#if defined(PHYMOD_APERTA2_SUPPORT) || defined(PHYMOD_AGERA2_SUPPORT)
    int reg_idx = 0, chnl_idx = 0;
#endif
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&phy_avs_config, 0, sizeof(phymod_avs_config_t));

    phy_avs_config.enable =  avs_config.enable;
    phy_avs_config.avs_disable_type = avs_config.avs_disable_type;
    phy_avs_config.avs_ctrl = avs_config.avs_ctrl;
    phy_avs_config.avs_pkg_share = avs_config.avs_pkg_share;
    phy_avs_config.avs_dc_margin = avs_config.avs_dc_margin;
    phy_avs_config.avs_regulator = avs_config.avs_regulator;
    phy_avs_config.ref_clock = avs_config.ref_clk;
    PHYMOD_MEMCPY(phy_avs_config.i2c_slave_address, avs_config.i2c_slave_address, sizeof(phy_avs_config.i2c_slave_address));
    phy_avs_config.external_ctrl_step = avs_config.external_ctrl_step;
    phy_avs_config.regulator_i2c_address = avs_config.regulator_i2c_address;
    phy_avs_config.regulator_legs = avs_config.regulator_legs;
    phy_avs_config.internal_master_slave_package = avs_config.internal_master_slave_package;
#if defined(PHYMOD_APERTA2_SUPPORT) || defined(PHYMOD_AGERA2_SUPPORT)
    for (reg_idx = 0; reg_idx < BCM_PLP_AVS_REGULATORS_MAX; reg_idx++) {
        phy_avs_config.regulator_config[reg_idx].regulator_addr = avs_config.regulator_config[reg_idx].regulator_addr;
        phy_avs_config.regulator_config[reg_idx].regulator_type = avs_config.regulator_config[reg_idx].regulator_type;
        for (chnl_idx = 0; chnl_idx < BCM_PLP_AVS_REGULATOR_CHANNELS_MAX; chnl_idx++) {
            phy_avs_config.regulator_config[reg_idx].channel_info[chnl_idx] = avs_config.regulator_config[reg_idx].channel_info[chnl_idx];
        }
    }
#endif
    rv = plp_aperta_phymod_phy_avs_config_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, phy_avs_config);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
/*!
 * bcm_avs_config_get
 *
 * @brief AVS configuration
 *
 * @param phy_info <pre> Represents PHY access\n
 * @param [out]  avs_config      AVS configuration </pre>
 */
int bcm_avs_config_get(bcm_plp_access_t phy_info, bcm_plp_avs_config_t* avs_config)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_avs_config_t phy_avs_config;
#if defined(PHYMOD_APERTA2_SUPPORT) || defined(PHYMOD_AGERA2_SUPPORT)
    int reg_idx = 0, chnl_idx = 0;
#endif

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&phy_avs_config, 0, sizeof(phymod_avs_config_t));
    PHYMOD_MEMSET(avs_config, 0, sizeof(bcm_plp_avs_config_t));

    rv = plp_aperta_phymod_phy_avs_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phy_avs_config);
    avs_config->enable =  phy_avs_config.enable;
    avs_config->avs_disable_type =  phy_avs_config.avs_disable_type;
    avs_config->avs_ctrl = phy_avs_config.avs_ctrl;
    avs_config->avs_pkg_share = phy_avs_config.avs_pkg_share;
    avs_config->avs_dc_margin = phy_avs_config.avs_dc_margin;
    avs_config->avs_regulator = phy_avs_config.avs_regulator;
    avs_config->ref_clk = phy_avs_config.ref_clock;
    PHYMOD_MEMCPY(avs_config->i2c_slave_address, phy_avs_config.i2c_slave_address, sizeof(phy_avs_config.i2c_slave_address));
    avs_config->external_ctrl_step = phy_avs_config.external_ctrl_step;
    avs_config->regulator_i2c_address = phy_avs_config.regulator_i2c_address;
    avs_config->regulator_legs = phy_avs_config.regulator_legs;
    avs_config->internal_master_slave_package = phy_avs_config.internal_master_slave_package;
#if defined(PHYMOD_APERTA2_SUPPORT) || defined(PHYMOD_AGERA2_SUPPORT)
    for (reg_idx = 0; reg_idx < BCM_PLP_AVS_REGULATORS_MAX; reg_idx++) {
        avs_config->regulator_config[reg_idx].regulator_addr = phy_avs_config.regulator_config[reg_idx].regulator_addr;
        avs_config->regulator_config[reg_idx].regulator_type = phy_avs_config.regulator_config[reg_idx].regulator_type;
        for (chnl_idx = 0; chnl_idx < BCM_PLP_AVS_REGULATOR_CHANNELS_MAX; chnl_idx++) {
            avs_config->regulator_config[reg_idx].channel_info[chnl_idx] = phy_avs_config.regulator_config[reg_idx].channel_info[chnl_idx];
        }
    }
#endif
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
/*!
 * bcm_avs_status_get
 *
 * @brief AVS configuration
 *
 * @param phy_info <pre> Represents PHY access\n
 * @param [out]  avs_status      AVS configuration status </pre>
 */
int bcm_avs_status_get(bcm_plp_access_t phy_info, bcm_plp_avs_config_status_t* avs_status)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_avs_config_status_t phy_avs_status;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&phy_avs_status, 0, sizeof(phymod_avs_config_status_t));
    PHYMOD_MEMSET(avs_status, 0, sizeof(bcm_plp_avs_config_status_t));

    rv = plp_aperta_phymod_phy_avs_status_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phy_avs_status);
    avs_status->enable =  phy_avs_status.enable;
    avs_status->avs_status = phy_avs_status.avs_status;
    avs_status->avs_track_status = phy_avs_status.avs_track_status;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
/*!
 * bcm_prbs_error_inject_set
 *
 * @brief Set prbs error injection
 *
 * @param phy_info <pre> Represents PHY access\n
 * @param [in]  prbs_error_inject   - prbs error inject </pre>
 */

int bcm_prbs_error_inject_set(bcm_plp_access_t phy_info, bcm_plp_prbs_error_inject_t prbs_error_inject)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_prbs_error_inject_t error_inject;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    error_inject.no_of_error_bits = prbs_error_inject.no_of_error_bits;

    rv = plp_aperta_phymod_phy_prbs_error_inject_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, error_inject);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

#endif  /* PHYMOD_ESTOQUE_SUPPORT || PHYMOD_MILLENIO_SUPPORT */

/*!
 * bcm_plp_aperta_logical_lane_set
 *
 * @brief Map logical lanes to Physical lane Tx and Rx. User should
 *        call this API twice to map system and line lanes.
 *
 * @param phy_info <pre> Represents PHY access\n
 * @param [in] logical_lane - Represents logical lane mapping list</pre>
 */
int bcm_plp_aperta_logical_lane_set(bcm_plp_access_t phy_info, bcm_plp_logical_lane_map_t logical_lane)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_logical_lane_map_t phy_logical_lane;

    PHYMOD_IF_MEMSET(&phy_logical_lane, 0, sizeof(phymod_logical_lane_map_t));
    PHYMOD_MEMCPY(&phy_logical_lane, &logical_lane, sizeof(phymod_logical_lane_map_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_CORE_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_logical_lane_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core->pm_core, &phy_logical_lane);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_aperta_logical_lane_get
 *
 * @brief Get mapped Physical lane Tx and Rx.
 *
 *
 * @param phy_info <pre> Represents PHY access\n
 * @param [out] logical_lane - Represents logical lane mapping list</pre>
 */
int bcm_plp_aperta_logical_lane_get(bcm_plp_access_t phy_info, bcm_plp_logical_lane_map_t *logical_lane)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    phymod_logical_lane_map_t phy_logical_lane;

    PHYMOD_IF_MEMSET(&phy_logical_lane, 0, sizeof(phymod_logical_lane_map_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_CORE_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_logical_lane_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core->pm_core, &phy_logical_lane);
    PHYMOD_MEMCPY(logical_lane, &phy_logical_lane, sizeof(phymod_logical_lane_map_t));

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief Display PRBS Error Analyzer Projection 
 *
 *  The Error Analyzer Projection API observes the number of frame errors generated based on hist_threshold 
 *  value programmed for the given time duration,
 *
 *  NOTE: This feature is enabled only if PRBS Checker is configured and enabled
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param prbs_error_fec_size    Defines the size of the FEC frames in bits\n
 *  @param hist_errcnt_thresh     Histogram Error Threshold (range [3-8]). Counts the number of errors in frames for x, x+1,..,x+7 errors in the FEC frames\n
 *  @param timeout_s     Time (in seconds) for which the PRBS errors are accumulated\n</pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_prbs_error_analyzer_proj(bcm_plp_access_t phy_info, unsigned short prbs_error_fec_size, unsigned char hist_errcnt_thresh, unsigned int timeout_s)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);


    rv = plp_aperta_phymod_phy_prbs_error_analyzer_proj(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, prbs_error_fec_size, hist_errcnt_thresh, timeout_s);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief DSP SerDes Lane config set 
 *
 *  This API is used to set DSP SerDes lane config parameter such as DFE settings.
 *
 *  @param phy_info   Represents PHY access\n
 *  @param firmware_lane_config   Represents lane config parameters\n
 *
 *                 DfeOn \n
 *                     0 - No dfe mode
 *                     1 - dfe mode
 *                 AnEnabled\n
 *                     1 - Autonegotiation enabled\n
 *                     0 - Autonegotiation disabled\n
 *                 MediaType\n
 *                     0 - bmplpFirmwareMediaTypePcbTraceBackPlane\n
 *                     1 - bcmplpFirmwareMediaTypeCopperCable\n
 *                     2 - bcmplpFirmwareMediaTypeOptics\n
 *                 AutoPeakingEnable \n
 *                     1  - Enable, 0 - Disable : Auto peaking algorithm in copper mode link training
 *                 OppositeCdrFirst \n
 *                     0 - Start link training firstly, then check opposite cdr_lock status
 *                     1 - Wait for opposite cdr_lock is ready before link training start
 *                 DcWanderMu \n
 *                     Range is 0-4 : Step size for baseline wander on rx data path \n
 *                 ExSlicer \n
 *                     0 - No override (default)
 *                     1 - Use regular slicer
 *                     2 - Use extended slicer
 *                 LinkTrainingReStartTimeOut \n
 *                     1 - Link Training restart timeout enable
 *                 LinkTrainingCL72_CL93PresetEn \n
 *                     LT RX send cl72/cl93 preset request:
 *                     0:Disable (non-IEEE compatible mode)
 *                     1: Enable(IEEE compatible mode, default value)
 *                 LinkTraining802_3CDPresetEn \n
 *                     LT RX send 802.3cd preset1 request:
 *                     0:Disable (non-IEEE compatible mode)
 *                     1: Enable (IEEE compatible mode, default value)
 *                 LpDfeOn \n
 *                     0 - No Low power dfe mode
 *                     1 - Low power dfe mode
 *                 AnTimer \n
 *                     0 - IEEE standard 3s link inhibit timer
 *                     1 - 6s  link inhibit timer
 *                     Applicable only for PAM4 modes
 *                 TxSquelchOrPrbs \n
 *                     0 - Squelch TX
 *                     1 - Send PRBS58
 *
 * @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_dsrds_firmware_lane_config_set(bcm_plp_access_t phy_info, bcm_plp_dsrds_firmware_lane_config_t firmware_lane_config)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_dsrds_firmware_lane_config_t fw_config;

    PHYMOD_IF_MEMSET(&fw_config, 0, sizeof(phymod_dsrds_firmware_lane_config_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    fw_config.DfeOn = firmware_lane_config.DfeOn;
    fw_config.LpDfeOn = firmware_lane_config.LpDfeOn;
    fw_config.AnEnabled = firmware_lane_config.AnEnabled;
    fw_config.MediaType = firmware_lane_config.MediaType;
    fw_config.AutoPeakingEnable = firmware_lane_config.AutoPeakingEnable;
    fw_config.OppositeCdrFirst = firmware_lane_config.OppositeCdrFirst;
    fw_config.DcWanderMu = firmware_lane_config.DcWanderMu;
    fw_config.ExSlicer = firmware_lane_config.ExSlicer;
    fw_config.LinkTrainingReStartTimeOut = firmware_lane_config.LinkTrainingReStartTimeOut;
    fw_config.LinkTrainingCL72_CL93PresetEn = firmware_lane_config.LinkTrainingCL72_CL93PresetEn;
    fw_config.LinkTraining802_3CDPresetEn = firmware_lane_config.LinkTraining802_3CDPresetEn;
    fw_config.AnTimer = firmware_lane_config.AnTimer;
    fw_config.TxSquelchOrPrbs = firmware_lane_config.TxSquelchOrPrbs;

    rv = plp_aperta_phymod_phy_dsrds_firmware_lane_config_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, fw_config);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*! \brief DSP SerDes Lane config get
 *
 *  This API is used to set DSP SerDes lane config parameter such as DFE settings.
 *
 *  @param phy_info   Represents PHY access\n
 *  @param firmware_lane_config   Represents lane config parameters\n
 *
 *                 DfeOn \n
 *                     0 - No dfe mode
 *                     1 - dfe mode
 *                 AnEnabled\n
 *                     1 - Autonegotiation enabled\n
 *                     0 - Autonegotiation disabled\n
 *                 MediaType\n
 *                     0 - bmplpFirmwareMediaTypePcbTraceBackPlane\n
 *                     1 - bcmplpFirmwareMediaTypeCopperCable\n
 *                     2 - bcmplpFirmwareMediaTypeOptics\n
 *                 AutoPeakingEnable \n
 *                     1  - Enable, 0 - Disable : Auto peaking algorithm in copper mode link training
 *                 OppositeCdrFirst \n
 *                     0 - Start link training firstly, then check opposite cdr_lock status
 *                     1 - Wait for opposite cdr_lock is ready before link training start
 *                 DcWanderMu \n
 *                     Range is 0-4 : Step size for baseline wander on rx data path \n
 *                 ExSlicer \n
 *                     0 - No override (default)
 *                     1 - Use regular slicer
 *                     2 - Use extended slicer
 *                 LinkTrainingReStartTimeOut \n
 *                     1 - Link Training restart timeout enable
 *                 LinkTrainingCL72_CL93PresetEn \n
 *                     LT RX send cl72/cl93 preset request:
 *                     0:Disable (non-IEEE compatible mode)
 *                     1: Enable(IEEE compatible mode, default value)
 *                 LinkTraining802_3CDPresetEn \n
 *                     LT RX send 802.3cd preset1 request:
 *                     0:Disable (non-IEEE compatible mode)
 *                     1: Enable (IEEE compatible mode, default value)
 *                 LpDfeOn \n
 *                     0 - No Low power dfe mode
 *                     1 - Low power dfe mode
 *                 AnTimer \n
 *                     0 - IEEE standard 3s link inhibit timer
 *                     1 - 6s  link inhibit timer
 *                     Applicable only for PAM4 modes
 *                 TxSquelchOrPrbs \n
 *                     0 - Squelch TX
 *                     1 - Send PRBS58
 *
 * @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_dsrds_firmware_lane_config_get(bcm_plp_access_t phy_info, bcm_plp_dsrds_firmware_lane_config_t* firmware_lane_config)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_dsrds_firmware_lane_config_t fw_config;

    PHYMOD_IF_MEMSET(&fw_config, 0, sizeof(phymod_dsrds_firmware_lane_config_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    rv = plp_aperta_phymod_phy_dsrds_firmware_lane_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &fw_config);

    firmware_lane_config->DfeOn = fw_config.DfeOn;
    firmware_lane_config->LpDfeOn = fw_config.LpDfeOn;
    firmware_lane_config->AnEnabled = fw_config.AnEnabled;
    firmware_lane_config->MediaType = fw_config.MediaType;
    firmware_lane_config->AutoPeakingEnable = fw_config.AutoPeakingEnable;
    firmware_lane_config->OppositeCdrFirst = fw_config.OppositeCdrFirst;
    firmware_lane_config->DcWanderMu = fw_config.DcWanderMu;
    firmware_lane_config->ExSlicer = fw_config.ExSlicer;
    firmware_lane_config->LinkTrainingReStartTimeOut = fw_config.LinkTrainingReStartTimeOut;
    firmware_lane_config->LinkTrainingCL72_CL93PresetEn = fw_config.LinkTrainingCL72_CL93PresetEn;
    firmware_lane_config->LinkTraining802_3CDPresetEn = fw_config.LinkTraining802_3CDPresetEn;
    firmware_lane_config->AnTimer = fw_config.AnTimer;
    firmware_lane_config->TxSquelchOrPrbs = fw_config.TxSquelchOrPrbs;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
/*!
 *
 * @brief This API is used to set SyncE Configuration on the PHY
 *
 *  @param phy_info   Represents PHY access\n
 *  @param [in] synce_cfg  Represents SyncE config parameters\n
 *      clkGenSquelchCfg - Clock Generation and Squelch Mode Configruation.\n
 *          -bcmplpClkGenSquelchDisable - Disable Clk Gen and Squelch.\n
 *          -bcmplpClkGenEnSquelchNone - Enable Clk Gen only, no squelch needed (Clock is always sent out)\n
 *          -bcmplpClkGenEnSquelchLOS - Enable Clk Gen and Squelch clock on Loss of Signal (LOS)\n
 *          -bcmplpClkGenEnSquelchLOL - Enable Clk Gen with Squelch clock on Loss of Lock (LOL)\n
 *          -bcmplpClkGenSquelchLOSOrLOL - Enable Clk Gen with Squelch clock on Loss of Signal (LOS) or Loss of Lock (LOL)\n
 *          -bcmplpClkGenSquelchForce - Enable Clk Gen with force Squelch\n
 *          ** 10G copper PHYs support ClkGenSquelchDisable & ClkGenEnSquelchNone only\n
 *               and only need the parameter 'rclk_out_pin_sel' below. Other parameters are not supported.
 *      recoveredClkLane - Recovered Clock Lane selection.\n
 *          -bcmplpRecoveredClkLane0 to bcmplpRecoveredClkLane15 - Select Lane 0 - 15 for recovered Clock Generation.\n
 *      squelchMonitorLanemap - Package Lanes that need to be monitored for Loss of Signal or Loss of Line.\n
 *          -Valid values are 0x1 to 0xF.\n
 *      divider - Divider selection to apply on Line/Lane rate before outputting clock.\n
 *          -bcmplpDivider20   - Divide by 20\n
 *          -bcmplpDivider40   - Divide by 40\n
 *          -bcmplpDivider80   - Divide by 80\n
 *          -bcmplpDivider160  - Divide by 160\n
 *          -bcmplpDivider400  - Divide by 400\n
 *          -bcmplpDivider1000 - Divide by 1000\n
 *          -bcmplpDivider64   - Divide by 64 \n
 *          -bcmplpDivider128  - Divide by 128\n
 *          -bcmplpDivider256  - Divide by 256\n
 *          -bcmplpDivider512  - Divide by 512\n
 *          -bcmplpDivider1024 - Divide by 1024\n
 *          -bcmplpDivider2048 - Divide by 2048\n
 *          -bcmplpDivider4096 - Divide by 4096\n
 *          -bcmplpDivider8192 - Divide by 8192\n
 *          -bcmplpDivider32   - Divide by 32\n
 *          -bcmplpDivider120  - Divide by 120\n
 *          -bcmplpDivider240  - Divide by 240\n
 *          -bcmplpDivider520  - Divide by 520\n
 *          -bcmplpDivider2040 - Divide by 2080\n
 *          -bcmplpDivider4080 - Divide by 4080\n
 *          -bcmplpDivider32   - Divide by 32\n
 *          -bcmplpDivider120  - Divide by 120\n
 *          -bcmplpDivider240  - Divide by 240\n
 *          -bcmplpDivider520  - Divide by 520\n
 *          -bcmplpDivider2040 - Divide by 2040\n
 *          -bcmplpDivider4080 - Divide by 4080\n
 *          -bcmplpDivider1    - Divide by 1\n
 *          -bcmplpDivider2    - Divide by 2\n
 *          -bcmplpDivider4    - Divide by 4\n
 *          -bcmplpDivider8    - Divide by 8\n
 *          -bcmplpDivider16   - Divide by 16\n
 *          -bcmplpDivider66   - Divide by 66\n
 *          -bcmplpDivider82p  - Divide by 82.5\n
 *          -bcmplpDivider528  - Divide by 528\n
 *          -bcmplpDivider320  - Divide by 320\n
 *          -bcmplpDivider480  - Divide by 480\n
 *          -bcmplpDivider640  - Divide by 640\n
 *          -bcmplpDivider960  - Divide by 960\n
 *          -bcmplpDivider1040 - Divide by 1040\n
 *          -bcmplpDivider2000 - Divide by 2000\n
 *          -bcmplpDivider2080 - Divide by 2080\n
 *          -bcmplpDivider4000 - Divide by 4000\n
 *          -bcmplpDivider8160 - Divide by 8160\n
 *          -bcmplpDivider16320- Divide by 16320\n
 *          -bcmplpDivider32640- Divide by 32640\n
 *          ** Not all dividers are supported on each phy.\n
 *             - Dividers supported by Europa family are:\n
 *                  20, 40, 80, 160, 400, 1000\n
 *             - Dividers supported by Millenio family are:\n
 *                  Single-ended ports -> 512, 1024, 2048, 4096\n
 *                  Differential ports -> 32, 64, 128, 256, 512, 1024, 2048, 4096 \n
 *             - Dividers supported by Aperta family are:\n
 *                  Single-ended ports -> 80, 120, 240, 520, 1000, 2040, 4080\n
 *                  Differential ports -> 32, 64, 128, 256, 512, 1024, 2048, 4096\n
 *             - Dividers supported by Barchetta family are:\n
 *                   1, 2, 4, 8 (It further divides the recovered clock by 20)
 *             - Dividers supported by Miura family are:
 *                  10G LRM Interface    -> 66, 82.5, 528
 *                  All other Interfaces -> 16, 64, 66, 82.5, 528
 *             - Dividers supported by Quadra28 family are:
 *                  LRM interface        -> 66
 *                  All other interfaces -> 16, 20, 64, 66
 *             - Dividers supported by Aperta2 family are:\n
 *                  Single-ended ports -> 80, 120, 240, 480, 520, 960, 1000, 1040, 2000,\n
 *                                        2040, 2080, 4000, 4080, 8160, 16320, 32640 \n
 *                  Differential ports -> 20, 40, 80, 160, 320, 640\n
 *      rclk_out_pin_sel - Recovered Clock output pin selection\n
 *          -bcmplpRclkPin0 - rclk pin 0 \n
 *          -bcmplpRclkPin1 - rclk pin 1 \n
 *          -bcmplpRclkPin2 - rclk pin 2 \n
 *          -bcmplpRclkPin3 - rclk pin 3 \n
 *          -bcmplpRclkPin4 - rclk pin 4 \n
 *          -bcmplpRclkPin5 - rclk pin 5 \n
 *          ** Note: Aperta family supports Single Ended and Differential Outputs.\n
 *             - Single Ended Outputs can be configured using bcmplpRclkPin1 and bcmplpRclkPin2.\n
 *             - Differential Output can be configured using bcmplpRclkPin0.\n
 *          ** Note: Aperta2 family supports Single Ended and Differential Outputs.\n
 *             - Single Ended Outputs can be configured using bcmplpRclkPin1 and bcmplpRclkPin2  \n
 *               for octal 0 and bcmplpRclkPin4 and bcmplpRclkPin5 for octal 1.\n
 *             - Differential Output can be configured using bcmplpRclkPin0 for octal 0 and \n
 *               bcmplpRclkPin3 for octal 1.\n
 *          ** 10G copper PHYs support rclk pin 0 & 1 only.\n
 *      rclk_if_side - Indicates if the recovered clock lane is on line side  or sys side\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_synce_config_set(bcm_plp_access_t phy_info, bcm_plp_synce_cfg_t* synce_cfg)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    phymod_synce_cfg_t phy_synce_cfg;

    PHYMOD_MEMSET(&phy_synce_cfg, 0, sizeof(phymod_synce_cfg_t));

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    BCM_PLP_CONVERT_SYNCE_CONFIG((&phy_synce_cfg), synce_cfg);
    rv = plp_aperta_phymod_synce_config_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phy_synce_cfg);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    return rv;
}

/*!
 *
 * @brief This API is used to get SyncE Configuration from the PHY.
 *
 * @param phy_info  Represents PHY access\n
 * @param [out]  synce_cfg   Represents SyncE Config parameters\n
 *      clkGenSquelchCfg - Clock Generation and Squelch Mode Configruation.\n
 *          -bcmplpClkGenSquelchDisable - Disable Clk Gen and Squelch.\n
 *          -bcmplpClkGenEnSquelchNone - Enable Clk Gen only, no squelch needed (Clock is always sent out)\n
 *          -bcmplpClkGenEnSquelchLOS - Enable Clk Gen and Squelch clock on Loss of Signal (LOS)\n
 *          -bcmplpClkGenEnSquelchLOL - Enable Clk Gen with Squelch clock on Loss of Lock (LOL)\n
 *          -bcmplpClkGenSquelchLOSOrLOL - Enable Clk Gen with Squelch clock on Loss of Signal (LOS) or Loss of Lock (LOL)\n
 *          -bcmplpClkGenSquelchForce - Enable Clk Gen with force Squelch\n
 *      recoveredClkLane - Recovered Clock Lane selection.\n
 *          -bcmplpRecoveredClkLane0 to bcmplpRecoveredClkLane15 - Select Lane 0 - 15 for recovered Clock Generation.\n
 *      squelchMonitorLanemap - Package Lanes that need to be monitored for Loss of Signal or Loss of Line.\n
 *          -Valid values are 0x1 to 0xF.\n
 *      divider - Divider selection to apply on Line/Lane rate before outputting clock.\n
 *          -bcmplpDivider20 - Divide by 20\n
 *          -bcmplpDivider40 - Divide by 40\n
 *          -bcmplpDivider80 - Divide by 80\n
 *          -bcmplpDivider160 - Divide by 160\n
 *          -bcmplpDivider400 - Divide by 400\n
 *          -bcmplpDivider1000 - Divide by 1000\n
 *          -bcmplpDivider64  - Divide by 64 \n
 *          -bcmplpDivider128  - Divide by 128\n
 *          -bcmplpDivider256  - Divide by 256\n
 *          -bcmplpDivider512  - Divide by 512\n
 *          -bcmplpDivider1024 - Divide by 1024\n
 *          -bcmplpDivider2048 - Divide by 2048\n
 *          -bcmplpDivider4096 - Divide by 4096\n
 *          -bcmplpDivider8192 - Divide by 8192\n
 *          -bcmplpDivider32   - Divide by 32\n
 *          -bcmplpDivider120  - Divide by 120\n
 *          -bcmplpDivider240  - Divide by 240\n
 *          -bcmplpDivider520  - Divide by 520\n
 *          -bcmplpDivider2040 - Divide by 2080\n
 *          -bcmplpDivider4080 - Divide by 4080\n
 *          -bcmplpDivider32   - Divide by 32\n
 *          -bcmplpDivider120  - Divide by 120\n
 *          -bcmplpDivider240  - Divide by 240\n
 *          -bcmplpDivider520  - Divide by 520\n
 *          -bcmplpDivider2040 - Divide by 2040\n
 *          -bcmplpDivider4080 - Divide by 4080\n
 *          -bcmplpDivider1    - Divide by 1\n
 *          -bcmplpDivider2    - Divide by 2\n
 *          -bcmplpDivider4    - Divide by 4\n
 *          -bcmplpDivider8    - Divide by 8\n
 *          -bcmplpDivider16,  - Divide by 16\n
 *          -bcmplpDivider66,  - Divide by 66\n
 *          -bcmplpDivider82p5,- Divide by 82.5\n
 *          -bcmplpDivider528, - Divide by 528\n
 *          -bcmplpDivider320  - Divide by 320\n
 *          -bcmplpDivider480  - Divide by 480\n
 *          -bcmplpDivider640  - Divide by 640\n
 *          -bcmplpDivider960  - Divide by 960\n
 *          -bcmplpDivider1040 - Divide by 1040\n
 *          -bcmplpDivider2000 - Divide by 2000\n
 *          -bcmplpDivider2080 - Divide by 2080\n
 *          -bcmplpDivider4000 - Divide by 4000\n
 *          -bcmplpDivider8160 - Divide by 8160\n
 *          -bcmplpDivider16320- Divide by 16320\n
 *          -bcmplpDivider32640- Divide by 32640\n
 *      Not all dividers are supported on each phy.\n
 *      - Dividers supported by Europa family are:\n
 *          20, 40, 80, 160, 400, 1000\n
 *      - Dividers supported by Millenio family are:\n
 *          Single-ended ports -> 512, 1024, 2048, 4096\n
 *          Differential ports -> 32, 64, 128, 256, 512, 1024, 2048, 4096 \n
 *      - Dividers supported by Aperta family are:\n
 *          Single-ended ports -> 80, 120, 240, 520, 1000, 2040, 4080\n
 *          Differential ports -> 32, 64, 128, 256, 512, 1024, 2048, 4096\n
 *      - Dividers supported by Barchetta family are:\n
 *           1, 2, 4, 8 (It further divides the recovered clock by 20)
 *      - Dividers supported by Miura family are:
 *          10G LRM Interface    -> 66, 82.5, 528
 *          All other Interfaces -> 16, 64, 66, 82.5, 528
 *      - Dividers supported by Quadra28 family are:
 *          LRM interface        -> 66
 *          All other interfaces -> 16, 20, 64, 66
 *      - Dividers supported by Aperta2 family are:\n
 *                  Single-ended ports -> 80, 120, 240, 480, 520, 960, 1000, 1040, 2000,\n
 *                                        2040, 2080, 4000, 4080, 8160, 16320, 32640 \n
 *                  Differential ports -> 20, 40, 80, 160, 320, 640\n
 *      rclk_out_pin_sel - Recovered Clock output pin selection\n
 *          -bcmplpRclkPin0 - rclk pin 0 \n
 *          -bcmplpRclkPin1 - rclk pin 1 \n
 *          -bcmplpRclkPin2 - rclk pin 2 \n
 *          -bcmplpRclkPin3 - rclk pin 3 \n
 *          -bcmplpRclkPin4 - rclk pin 4 \n
 *          -bcmplpRclkPin5 - rclk pin 5 \n
 *      rclk_if_side - Indicates if the recovered clock lane is on line side  or sys side\n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_synce_config_get(bcm_plp_access_t phy_info, bcm_plp_synce_cfg_t* synce_cfg)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    phymod_synce_cfg_t phy_synce_cfg;

    PHYMOD_IF_MEMSET(&phy_synce_cfg, 0, sizeof(phymod_synce_cfg_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    phy_synce_cfg.rclk_out_pin_sel = synce_cfg->rclk_out_pin_sel;
#if defined(PHYMOD_PHY542XX_SUPPORT)
    phy_synce_cfg.clkGenSquelchCfg = synce_cfg->clkGenSquelchCfg;
#endif
    rv = plp_aperta_phymod_synce_config_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phy_synce_cfg);
    BCM_PLP_CONVERT_SYNCE_CONFIG(synce_cfg, (&phy_synce_cfg));
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    return rv;
}

#ifdef BARCHETTA_DEBUG_SUPPORT
int bcm_get_hwport_number(bcm_plp_access_t phy_info, int *port_number)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    barchetta_package_info_t pkg_info;

    PHYMOD_MEMSET(&pkg_info, 0 , sizeof(barchetta_package_info_t));
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    PHYMOD_IF_ERR_RETURN(_barchetta_get_package_info(&(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy)->access, &pkg_info));
    rv = _barchetta_retrieve_hardware_port_number_from_sw_database(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pkg_info, port_number);
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
#endif

#if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined(PHYMOD_APERTA_SUPPORT)
/*
 * @brief      bcm_plp_aperta_pcs_status_get
 * @details    This API is used to get PCS status dump
 *
 * @param  phy_info  Represents PHY access\n
 * @param  pcs_status: PCS status dump
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_pcs_status_get(bcm_plp_access_t phy_info, bcm_plp_pcs_status_t* pcs_status)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_pcs_status_t phymod_pcs_status;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&phymod_pcs_status, 0, sizeof(phymod_pcs_status_t));
    rv = plp_aperta_phymod_phy_pcs_status_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &phymod_pcs_status);
    PHYMOD_MEMCPY(pcs_status, &phymod_pcs_status, sizeof(phymod_pcs_status_t));

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}
#endif

/*!
 * bcm_plp_aperta_pattern_enable_set
 *
 * @brief Set pattern state
 *
 * @param [in]  phy_info        - phy access information
 * @param [in]  pattern         - Pattern Configuration
 *
 * @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_pattern_enable_set(bcm_plp_access_t phy_info, const bcm_plp_pattern_t *pattern)
{
#if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined(PHYMOD_BARCHETTA_SUPPORT) || defined(PHYMOD_APERTA_SUPPORT) \
	 || defined(PHYMOD_AGERA_SUPPORT) || defined(PHYMOD_AGERALITE_SUPPORT) || defined (PHYMOD_AGERA2_SUPPORT)
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_pattern_t phymod_pattern;

    if (pattern == NULL) {
        return BCM_PM_IF_PARAM;
    }
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&phymod_pattern, 0, sizeof(plp_aperta_phymod_pattern_t));
    phymod_pattern.pattern_len = pattern->pattern_len;
    phymod_pattern.pattern = pattern->pattern;
    rv = plp_aperta_phymod_phy_pattern_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pattern->enable, &phymod_pattern);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
#else
    return BCM_PM_IF_UNAVAIL;
#endif
}

/*!
 * bcm_plp_aperta_pattern_enable_get
 *
 * @brief Get pattern state
 *
 * @param [in]  phy_info       - phy access information
 * @param [in,out] pattern      - Pattern Configuration
 *
 * @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_pattern_enable_get(bcm_plp_access_t phy_info, bcm_plp_pattern_t *pattern)
{
#if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined(PHYMOD_BARCHETTA_SUPPORT) \
	|| defined(PHYMOD_APERTA_SUPPORT) || defined(PHYMOD_AGERA_SUPPORT) || defined(PHYMOD_AGERALITE_SUPPORT) || defined (PHYMOD_AGERA2_SUPPORT)
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    plp_aperta_phymod_pattern_t phymod_pattern;

    if (pattern == NULL) {
        return BCM_PM_IF_PARAM;
    }
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&phymod_pattern, 0, sizeof(plp_aperta_phymod_pattern_t));
    phymod_pattern.pattern = pattern->pattern;
    rv = plp_aperta_phymod_phy_pattern_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &pattern->enable, &phymod_pattern);
    pattern->pattern_len = phymod_pattern.pattern_len;

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
#else
    return BCM_PM_IF_UNAVAIL;
#endif
}


/*!
 * bcm_plp_aperta_fw_init_params_get
 *
 * @brief Get the firmware init params configured during bcm_plp_aperta_init_fw_bcast
 *
 * @param [in]  phy_info         - phy access information
 * @param [out] fw_init_params   - Firmware init params, void pointer specific to chip fw_init_params
 *
 * @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_fw_init_params_get(bcm_plp_access_t phy_info, void* const fw_init_params)
{

    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_CORE_ACCESS(phy_info, phy_id_idx);
    rv = plp_aperta_phymod_phy_fw_init_params_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->core->pm_core, fw_init_params);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    return rv;
}

/*!
 * bcm_plp_srds_diag_access_enable
 *
 * @brief Enables serdes diagnostics access.
 * @details This API can be used to read/write Registers and RAM variables which are not available directly.
 * It also provides a way to insert breakpoints in the program for debug purposes.
 *
 * @param phy_info        [in] Represents PHY access
 * @param diag_access_cfg [in] Pointer to a bcm_plp_srds_diag_access_cfg_t structure to enable serdes diag  access
 *
 * @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_srds_diag_access_enable(bcm_plp_access_t phy_info, bcm_plp_srds_diag_access_cfg_t *diag_access_cfg) {
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = BCM_PM_IF_MAX_PHY;
    uint32_t exist_phy = 0;
    phymod_srds_diag_access_cfg_t srds_diag_access_cfg;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
    PHYMOD_MEMSET(&srds_diag_access_cfg, 0, sizeof(phymod_srds_diag_access_cfg_t));
    PHYMOD_MEMCPY(&srds_diag_access_cfg, diag_access_cfg, sizeof(phymod_srds_diag_access_cfg_t));
    rv = plp_aperta_phymod_srds_diag_access_enable(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &srds_diag_access_cfg);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    return rv;
}

/*!
 * bcm_plp_phy_pai_info_get
 *
 * @brief Provides chip/PAI specific information
 * @details This API is used by BRCM PAI package to access chip specific information.
 *
 * @param phy_info        [in] Structure for phy access information
 * @param pai_info        [in] Structure for PAI information
 *
 * @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_phy_pai_info_get(bcm_plp_access_t phy_info, bcm_plp_pai_info_t *pai_info)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = 0;
    uint32_t exist_phy = 0;
    phymod_pai_phy_config_t phymod_pai_phy_config ;
    plp_aperta_phymod_dispatch_type_t phy_type = 0xFFFF;
    plp_aperta_phymod_phy_access_t phy_access;
    plp_aperta_phymod_bus_t bus;

    if (pai_info->pai_phy_config->config_type == PAI_PHY_INIT) {
        bcm_plp_pai_init_t *pai_init_info;
        pai_init_info = (bcm_plp_pai_init_t*)pai_info->pai_phy_config->config_data;
#if defined(PHYMOD_PHY542XX_SUPPORT)
        rv = bcm_plp_aperta_init(phy_info, pai_info->read, pai_info->write,
                  pai_init_info->firmware_load_type->firmware_load_method);
#else
        rv = bcm_plp_aperta_init_fw_bcast(phy_info, pai_info->read, pai_info->write,
                  pai_init_info->firmware_load_type,
                  pai_init_info->broadcast_method);
#endif
        return rv;
    } else {
        BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
        if (pai_info->pai_phy_config->config_type != PAI_PHY_CONFIG_TYPE_FW_INIT) {
            BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
            BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);
        }
        PHYMOD_MEMSET(&phy_access, 0, sizeof(phy_access));
        PHYMOD_MEMSET(&bus, 0, sizeof(plp_aperta_phymod_bus_t));

        if (pai_info->pai_phy_config->config_type == PAI_PHY_CONFIG_TYPE_FW_INIT) {
            rv = bcm_plp_aperta_phy_get_phy_type (phy_info, &phy_type, pai_info->read,pai_info->write);
            if (rv != 0) {
                goto ERR;
            }
            phy_access.port_loc = 0;
            phy_access.device_op_mode = 0;
            phy_access.type = phy_type;
            bus.bus_name="PREINIT";
            bus.read = pai_info->read;
            bus.write = pai_info->write;
            phy_access.access.bus = &bus;
            phy_access.access.user_acc = phy_info.platform_ctxt; 
            phy_access.access.lane_mask = phy_info.lane_map;
            phy_access.access.addr = phy_info.phy_addr; 
            phymod_pai_phy_config.config_type = phymodConfigTypeFw_init;
        } else {
            switch(pai_info->pai_phy_config->config_type) {
                case PAI_PHY_CONFIG_TYPE_PORT_CONFIG:
                    phymod_pai_phy_config.config_type = phymodConfigTypePort_config;
                    break;
                case PAI_PHY_CONFIG_TYPE_TRAINING:
                    phymod_pai_phy_config.config_type = phymodConfigTypeTraining;
                    break;
                default:
                    rv = BCM_PM_IF_PARAM ;
                    goto ERR;
            }
            PHYMOD_MEMCPY(&phy_access, &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, sizeof(plp_aperta_phymod_phy_access_t));
        }
        phymod_pai_phy_config.config_data = pai_info->pai_phy_config->config_data;

        rv = plp_aperta_phymod_phy_pai_info_get(&phy_access, (phymod_pai_phy_op_t)pai_info->operation, 
                &phymod_pai_phy_config, pai_info->epdm_data);
    }
ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    return rv;
}

/*!
 * bcm_plp_phy_stats
 *
 * @brief Get or Enable PHY statistics counters.
 * @details This API can be used to enable statistics and to get one or more PHY statistics from the PHY. Supported only on applicable PHYs
 *
 * @param phy_info        [in] PHY access structure
 * @param op_type         [in] PHY MIB stats operation
 * @param stat_id         [in] Array of statistics identifiers defined in bcm_plp_mib_stat_type_t
 * @param stats_count     [in] Number of statistics counters to be read.
 * @param stats_p         [out] Pointer to a memory location to save statistics values.
 *     It is user’s responsibility to provide valid memory to return requested statistics counters.
 *
 * @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_phy_stats(bcm_plp_access_t phy_info, bcm_plp_mib_stat_operation_t op_type, unsigned int *stat_id,
    unsigned int stats_count, plp_uint64_t *stats_p)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx = 0;
    uint32_t exist_phy = 0;

    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    BCM_PLP_PHY_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_FILL_PHY_ACCESS(phy_info, phy_id_idx);

    if ((stat_id == NULL) || (stats_count == 0) ||
        ((op_type == BCM_PLP_MIB_STAT_OP_STATS_GET) && (stats_p == NULL))) {
        rv = BCM_PM_IF_PARAM ;
        goto ERR;
    }
    rv = plp_aperta_phymod_phy_stats(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, op_type, stat_id, stats_count, stats_p);

ERR:
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    return rv;
}
