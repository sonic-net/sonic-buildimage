/*
 *         
 * $Id: bcm_pm_if.c $
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *         
 *     
 */

#include "bcm_pm_if.h"

bcm_if_phymod_ctrl_t plp_barchetta_phy_ctrl;
bcm_if_phymod_phy_id_t _plp_barchetta_phyid_list[BCM_PM_IF_MAX_PHY];

int _bcm_plp_barchetta_if_phymod_phy_create(_bcm_if_phymod_phy_t **phy)
{
    _bcm_if_phymod_phy_t *new_phy;
    
    new_phy = PHYMOD_IF_MALLOC(sizeof(_bcm_if_phymod_phy_t));
    if (new_phy == NULL) {
        PHYMOD_DEBUG_ERROR(("Failed to allocate memory for _bcm_if_phymod_phy_t\n"));
        return BCM_PM_IF_MEMORY;
    }
    *phy = new_phy;
    PHYMOD_IF_MEMSET(*phy,0,sizeof(_bcm_if_phymod_phy_t));
    (*phy)->valid = 1;
    
    return BCM_PM_IF_SUCCESS;    
}

int _bcm_plp_barchetta_if_phymod_core_create(_bcm_if_phymod_core_t **core)
{
    _bcm_if_phymod_core_t *new_core;
    
    new_core = PHYMOD_IF_MALLOC(sizeof(_bcm_if_phymod_core_t));
    if (new_core == NULL) {
        PHYMOD_DEBUG_ERROR(("Failed to allocate memory for _bcm_if_phymod_core_t\n"));
        return BCM_PM_IF_MEMORY;
    }
    *core = new_core;
    return BCM_PM_IF_SUCCESS;    
}


void _bcm_plp_barchetta_pm_if_core_init(_bcm_if_phymod_core_t *core,
                   plp_barchetta_phymod_bus_t *core_bus, uint32_t phy_addr, void *user_acc)
{
    plp_barchetta_phymod_core_access_t *pm_core;
    plp_barchetta_phymod_access_t *pm_acc;

    core->unit = 0;
    core->port = 0 + phy_addr;
    core->read = core_bus->read;
    core->write = core_bus->write;
    core->wrmask = NULL;

    pm_core = &core->pm_core;
    plp_barchetta_phymod_core_access_t_init(pm_core);
    pm_acc = &pm_core->access;
    plp_barchetta_phymod_access_t_init(pm_acc);
    PHYMOD_ACC_USER_ACC(pm_acc) = user_acc;
    PHYMOD_ACC_BUS(pm_acc) = core_bus;
    PHYMOD_ACC_ADDR(pm_acc) = phy_addr;
  #if defined(PHYMOD_XGBASET_SUPPORT)
    PHYMOD_ACC_MISC(pm_acc) = &(_plp_barchetta_phyid_list[phy_addr]);
  #endif  /*  XGBASET_SUPPORT  */
    return;
}

#if defined(PHYMOD_XGBASET_SUPPORT)  /* * * * * * * * * * * * * * * * * * * * * */

void _bcm_pm_if_init_phy_id_idx(uint32_t phy_id_combo)
{
    /* invalid the _plp_barchetta_phyid_list entry before initialization */
    _plp_barchetta_phyid_list[BCM_PHY_ADDR_LOGICAL(phy_id_combo)].valid &= ~BCM_PHY_ID_VALID;
}

void _bcm_plp_barchetta_pm_if_get_phy_id_idx(uint32_t phy_id_combo, uint32_t *phy_id_idx, uint32_t *exist_phy)
{
    unsigned int  found  = FALSE;
    uint32_t      phy_id = BCM_PHY_ADDR_LOGICAL(phy_id_combo);
    *phy_id_idx = phy_id;

    if ( (BCM_PHY_ID_VALID & _plp_barchetta_phyid_list[phy_id].valid) && (_plp_barchetta_phyid_list[phy_id].phy_id == phy_id) ) {
       found      = TRUE;
       *exist_phy = TRUE;
       return;
    }
    /* New PHYID, register the physical & logical PHY address */
    if ( ! found ) {
        if ( ! (BCM_PHY_ID_VALID & _plp_barchetta_phyid_list[phy_id].valid) ) {
            if ( BCM_PHY_IS_PHYSICAL_ADDR_GIVEN_BY_APP(phy_id_combo) ) {
                _plp_barchetta_phyid_list[phy_id].valid        |=  BCM_PHY_PHYSICAL_ADDR_GIVEN_BY_APP;
                _plp_barchetta_phyid_list[phy_id].physical_addr =  BCM_PHY_ADDR_PHYSICAL(phy_id_combo);
            } else {
                _plp_barchetta_phyid_list[phy_id].valid        &= ~BCM_PHY_PHYSICAL_ADDR_GIVEN_BY_APP;
                _plp_barchetta_phyid_list[phy_id].physical_addr =  phy_id;
            }

            if ( BCM_PHY_IS_MASTER_PORT(phy_id_combo) ) {
                _plp_barchetta_phyid_list[phy_id].valid        |=  BCM_PHY_MASTER_PORT;
            } else {
                _plp_barchetta_phyid_list[phy_id].valid        &= ~BCM_PHY_MASTER_PORT;
            }

            _plp_barchetta_phyid_list[phy_id].phy_id            = phy_id;            /* logical PHY address */
            _plp_barchetta_phyid_list[phy_id].valid            |= BCM_PHY_ID_VALID;  /* validate this entry */
            *exist_phy = FALSE;
        }
    }
}

int _bcm_pm_if_phy_id_is_master(uint32_t phy_id)
{
    return (BCM_PHY_MASTER_PORT & _plp_barchetta_phyid_list[BCM_PHY_ADDR_LOGICAL(phy_id)].valid) ? TRUE : FALSE;
}

#else  /* ! XGBASET_SUPPORT  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */

void _bcm_plp_barchetta_pm_if_get_phy_id_idx(uint32_t phy_id, uint32_t *phy_id_idx, uint32_t *exist_phy) 
{
    unsigned int found = 0;
    *phy_id_idx = phy_id;

    if (_plp_barchetta_phyid_list[phy_id].valid && (_plp_barchetta_phyid_list[phy_id].phy_id == phy_id)) {
           found = 1;
           *exist_phy = 1;
    }
    if (!found) { /* New PHYID*/
        if (!_plp_barchetta_phyid_list[phy_id].valid) {
            _plp_barchetta_phyid_list[phy_id].phy_id = phy_id;
            _plp_barchetta_phyid_list[phy_id].valid = 1;
                 *exist_phy = 0;
        }
    }
}

#endif  /*  XGBASET_SUPPORT  * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef PHYMOD_GALLARDO28_SUPPORT

int _gallardo28_get_single_pmd_mode(bcm_plp_access_t phy_info,
                                            int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val),  unsigned int *pmd_mode)
{
    int rv=0;
    unsigned int pmd_ctrl=0;
    unsigned int saved_phy_id;
	unsigned int id0, id1;
    saved_phy_id = phy_info.phy_addr;
    saved_phy_id = (saved_phy_id & ~0x3);

    rv = read(phy_info.platform_ctxt,saved_phy_id, 0x10002, &id0);
    if(rv !=0){
        return rv;
    }

    rv = read(phy_info.platform_ctxt, saved_phy_id, 0x10003, &id1);
    if(rv !=0){
        return rv;
    }
	if(id0 == 0xae02 && id1 == 0x5210){

       rv = read(phy_info.platform_ctxt, saved_phy_id, 0x1ca86, &pmd_ctrl);
       if(rv !=0){
          return rv;
       }
	}else{
        PHYMOD_DEBUG_ERROR(("Invalid PHY\n"));
        return BCM_PM_IF_INVALID_PHY;
	}

    if(pmd_ctrl & 0x0080){ /* If it is single PMD mode */
        *pmd_mode = 1;
    }else{
        *pmd_mode = 0; /* If it is single PMD only base phy id is valid */
    }
    return rv;
}
#endif

int _bcm_plp_barchetta_pm_phy_init(_bcm_if_phymod_phy_t *phy, bcm_pm_firmware_load_method_t firmware_load_method, void *init_param)
{
    _bcm_if_phymod_core_t *core;
    int rv = 0;

    plp_barchetta_phymod_core_status_t core_sts;
    plp_barchetta_phymod_core_info_t core_info;

    core = phy->core;
    if (core == NULL)
        return -1;
    phy->init_config.polarity.rx_polarity = 0x0;
    phy->init_config.polarity.tx_polarity = 0x0;
    
    /*Initializing analog value only for lane 0 */
    phy->init_config.tx[0].pre = 0x0;
    phy->init_config.tx[0].main = 0x0;
    phy->init_config.tx[0].post = 0x0;
    phy->init_config.tx[0].post2 = 0x0;
    phy->init_config.tx[0].post3 = 0x0;
    phy->init_config.tx[0].amp = 0x0;
    phy->init_config.cl72_en = 0;
    phy->init_config.an_en = 0x0;
    phy->init_config.interface.interface_type = phymodInterfaceSR;
    phy->init_config.interface.data_rate = 10000;
    phy->init_config.interface.ref_clock = phymodRefClk156Mhz;
#if defined(PHYMOD_APERTA_SUPPORT) || defined (PHYMOD_AGERA_SUPPORT) || defined(PHYMOD_AGERALITE_SUPPORT)
    phy->init_config.interface.device_aux_modes = init_param;
#else
    phy->init_config.interface.device_aux_modes = phy->static_config;
#endif
	/* coverity[mixed_enums] */
    core->init_config.firmware_load_method = firmware_load_method;
    core->init_config.interface.interface_type = phymodInterfaceSR;
    core->init_config.interface.data_rate = 10000;
    core->init_config.interface.ref_clock = phymodRefClk156Mhz;
    core->init_config.interface.device_aux_modes = phy->static_config;
    /* Default op mode set to repeater and datapath as normal path */
    phy->init_config.op_mode = phymodOperationModeRepeater;
	core->init_config.op_datapath = phymodDatapathNormal;
#if !defined(PHYMOD_MILLENIO_SUPPORT) && !defined(PHYMOD_AGERA_SUPPORT) && !defined(PHYMOD_AGERALITE_SUPPORT)
    if (phy->static_config) {
        core->init_config.op_datapath = ((plp_static_config_t*)phy->static_config)->ull_dp;
    }
#endif

    core_sts.pmd_active = 0;
    if (core->init == FALSE  && PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_GET(&core->init_config)) {
        rv = plp_barchetta_phymod_core_init(&core->pm_core, &core->init_config, &core_sts);
       
        if (rv != 0) {
            return rv;
        }
        PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_CLR(&core->init_config);

        /* bcm_plp_barchetta_init needs to be multithreaded, so for PHY's other than
         * Q28 we are not enabling broadcast*/
#ifdef PHYMOD_QUADRA28_SUPPORT        
        if (core->pm_core.type == phymodDispatchTypeQuadra28) {
        PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_SET(&core->init_config);
        rv = plp_barchetta_phymod_core_init(&core->pm_core, &core->init_config, &core_sts);
        if (rv != 0) {
            return rv;
        }
        PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_CLR(&core->init_config);
        }
#endif        
        PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD_SET(&core->init_config);
        
        rv = plp_barchetta_phymod_core_init(&core->pm_core, &core->init_config, &core_sts);
       
        if (rv != 0) {
            return rv;
        }
        PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD_CLR(&core->init_config);
        PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_SET(&core->init_config);
        rv = plp_barchetta_phymod_core_init(&core->pm_core, &core->init_config, &core_sts);
       
        if (rv != 0) {
            return rv;
        }
        PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_CLR(&core->init_config);

        PHYMOD_CORE_INIT_F_FW_LOAD_END_SET(&core->init_config);
        rv = plp_barchetta_phymod_core_init(&core->pm_core, &core->init_config, &core_sts);
       
        if (rv != 0) {
            return rv;
        }
        core->init = TRUE;
        PHYMOD_CORE_INIT_F_FW_LOAD_END_CLR(&core->init_config);
        
    } else if (core->init == FALSE) {
       
        rv = plp_barchetta_phymod_core_init(&core->pm_core, &core->init_config, &core_sts);
        if (rv != 0) {
            return rv;
        }
        core->init = TRUE;
    }
    
    rv = plp_barchetta_phymod_phy_init(&phy->pm_phy, &phy->init_config);
    if (rv != 0) {
        return rv;
    }
    
    /* read serdes id info */
    rv = plp_barchetta_phymod_core_info_get(&core->pm_core, &core_info); 
    if (rv != 0) {
        return rv;
    }
    PHYMOD_DEBUG_INFO(("Core Ver:%d\n", core_info.core_version));
    /* Configure Default Interface Mode HERE*/
    return BCM_PM_IF_SUCCESS;
}

int _bcm_plp_barchetta_phy_init_bcast(_bcm_if_phymod_phy_t *phy, bcm_plp_firmware_load_type_t *firmware_load_type)
{
    _bcm_if_phymod_core_t *core;
    int rv = 0;

    plp_barchetta_phymod_core_status_t core_sts;
    plp_barchetta_phymod_core_info_t core_info;

    core = phy->core;
    if (core == NULL) {
        return -1;
    }
    phy->init_config.polarity.rx_polarity = 0x0;
    phy->init_config.polarity.tx_polarity = 0x0;

    /*Initializing analog value only for lane 0 */
    phy->init_config.tx[0].pre = 0x0;
    phy->init_config.tx[0].main = 0x0;
    phy->init_config.tx[0].post = 0x0;
    phy->init_config.tx[0].post2 = 0x0;
    phy->init_config.tx[0].post3 = 0x0;
    phy->init_config.tx[0].amp = 0x0;
    phy->init_config.cl72_en = 0;
    phy->init_config.an_en = 0x0;
    phy->init_config.interface.interface_type = phymodInterfaceSR;
    phy->init_config.interface.data_rate = 10000;
    phy->init_config.interface.ref_clock = phymodRefClk156Mhz;
#if defined( PHYMOD_APERTA_SUPPORT)|| defined(PHYMOD_EVORA_SUPPORT) || defined (PHYMOD_AGERA_SUPPORT) || defined(PHYMOD_AGERALITE_SUPPORT)
    phy->init_config.interface.device_aux_modes = firmware_load_type->fw_init_params;
#else
    phy->init_config.interface.device_aux_modes = phy->static_config;
#endif
	/* coverity[mixed_enums] */
    core->init_config.firmware_load_method = firmware_load_type->firmware_load_method;
    core->init_config.interface.interface_type = phymodInterfaceSR;
    core->init_config.interface.data_rate = 10000;
    core->init_config.interface.ref_clock = phymodRefClk156Mhz;
    core->init_config.interface.device_aux_modes = phy->static_config;
    if(firmware_load_type->firmware_load_method == bcmpmFirmwareLoadMethodExternal){
        if(firmware_load_type->firmware_loader !=NULL){
            core->init_config.firmware_loader = firmware_load_type->firmware_loader;
        }else{
            PHYMOD_DEBUG_ERROR(("Invalid firmware loader\n"));
            return BCM_PM_IF_PARAM;
        }
    }

    /* Default op mode set to repeater and datapath as normal path */
    phy->init_config.op_mode = phymodOperationModeRepeater;
	core->init_config.op_datapath = phymodDatapathNormal;
#if !defined(PHYMOD_MILLENIO_SUPPORT) && !defined(PHYMOD_BARCHETTA2_SUPPORT) && !defined(PHYMOD_AGERA_SUPPORT) && !defined(PHYMOD_AGERALITE_SUPPORT)
    if (phy->static_config) {
        core->init_config.op_datapath = ((plp_static_config_t*)phy->static_config)->ull_dp;
    }
#endif

    core_sts.pmd_active = 0;
    if (core->init == FALSE) {
        rv = plp_barchetta_phymod_core_init(&core->pm_core, &core->init_config, &core_sts);
        if (rv != 0) {
            return rv;
        }
        if (PHYMOD_CORE_INIT_F_FW_LOAD_END_GET(&core->init_config)) {
            core->init = TRUE;
            rv = plp_barchetta_phymod_phy_init(&phy->pm_phy, &phy->init_config);
            if (rv != 0) {
                return rv;
            }
            /* read serdes id info */
            rv = plp_barchetta_phymod_core_info_get(&core->pm_core, &core_info);
            if (rv != 0) {
                return rv;
            }
            PHYMOD_DEBUG_INFO(("Core Ver:%d\n", core_info.core_version));
            /* Configure Default Interface Mode HERE*/
        }
    }
    return BCM_PM_IF_SUCCESS;
}

int _bcm_plp_barchetta_phy_user_phymod_ability(unsigned int tech_ability, unsigned int fec_ability, unsigned short pause_ability, bcm_plp_an_config_t an_config, plp_barchetta_phymod_autoneg_ability_t* ability)
{
    ability->an_cap = 0;
    ability->an_cap_ext = 0;
    ability->an_fec = 0;

    if (tech_ability & bcmplpAnCap1G_KX) {
        PHYMOD_AN_CAP_1G_KX_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap10G_KX4) {
        PHYMOD_AN_CAP_10G_KX4_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap10G_KR) {
        PHYMOD_AN_CAP_10G_KR_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap40G_KR4) {
        PHYMOD_AN_CAP_40G_KR4_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap40G_CR4) {
        PHYMOD_AN_CAP_40G_CR4_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap100G_CR10) {
        PHYMOD_AN_CAP_100G_CR10_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap100G_CR4) {
        PHYMOD_AN_CAP_100G_CR4_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap100G_KR4) {
        PHYMOD_AN_CAP_100G_KR4_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap25G_KRS1) {
        PHYMOD_AN_CAP_25G_KRS1_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap25G_CRS1) {
        PHYMOD_AN_CAP_25G_CRS1_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap25G_KR) {
        PHYMOD_AN_CAP_25G_KR_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap25G_CR) {
        PHYMOD_AN_CAP_25G_CR_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap50G_KR2) {
        PHYMOD_AN_CAP_50G_KR2_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap50G_CR2) {
        PHYMOD_AN_CAP_50G_CR2_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap25G_KR1) {
        PHYMOD_AN_CAP_25G_KR1_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap25G_CR1) {
        PHYMOD_AN_CAP_25G_CR1_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap50G_CR_KR) {
        PHYMOD_AN_CAP_50G_CR_KR_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap50G_CR1_KR1) {
        PHYMOD_AN_CAP_50G_CR1_KR1_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap100G_CR2_KR2) {
        PHYMOD_AN_CAP_100G_CR2_KR2_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap200G_CR4_KR4) {
        PHYMOD_AN_CAP_200G_CR4_KR4_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap50G_KP) {
        PHYMOD_AN_CAP_50G_KP1_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap100G_KP2) {
        PHYMOD_AN_CAP_100G_KP2_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap200G_KP4) {
        PHYMOD_AN_CAP_200G_KP4_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCap400G_CR8_KR8) {
        PHYMOD_AN_CAP_400G_CR8_KR8_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCapBAM_50G_CR1_KR1) {
        PHYMOD_AN_CAP_BAM_50G_CR1_KR1_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCapBAM_50G_CR2_KR2) {
        PHYMOD_AN_CAP_BAM_50G_CR2_KR2_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCapBAM_100G_CR2_KR2) {
        PHYMOD_AN_CAP_BAM_100G_CR2_KR2_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCapBAM_100G_CR4_KR4) {
        PHYMOD_AN_CAP_BAM_100G_CR4_KR4_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCapBAM_200G_CR4_KR4) {
        PHYMOD_AN_CAP_BAM_200G_CR4_KR4_SET(ability->an_cap);
    }
    if (tech_ability & bcmplpAnCapBAM_400G_CR8_KR8) {
        PHYMOD_AN_CAP_BAM_400G_CR8_KR8_SET(ability->an_cap);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapBAM_20G_KR1) {
        PHYMOD_AN_CAP1_BAM_20G_CR1_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapBAM_20G_KR1) {
        PHYMOD_AN_CAP1_BAM_20G_KR1_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapBAM_20G_CR2) {
        PHYMOD_AN_CAP1_BAM_20G_CR2_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapBAM_20G_KR2) {
        PHYMOD_AN_CAP1_BAM_20G_KR2_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapBAM_25G_CR1) {
        PHYMOD_AN_CAP1_BAM_25G_CR1_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapBAM_25G_KR1) {
        PHYMOD_AN_CAP1_BAM_25G_KR1_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapBAM_40G_CR2) {
        PHYMOD_AN_CAP1_BAM_40G_CR2_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapBAM_40G_KR2) {
        PHYMOD_AN_CAP1_BAM_40G_KR2_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapBAM_50G_CR4) {
        PHYMOD_AN_CAP1_BAM_50G_CR4_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapBAM_50G_KR4) {
        PHYMOD_AN_CAP1_BAM_50G_KR4_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapBAM_100G_CR1_KR1) {
        PHYMOD_AN_CAP1_BAM_100G_CR1_KR1_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCap100G_CR1_KR1) {
        PHYMOD_AN_CAP1_100G_CR1_KR1_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCap200G_CR2_KR2) {
        PHYMOD_AN_CAP1_200G_CR2_KR2_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCap400G_CR4_KR4) {
        PHYMOD_AN_CAP1_400G_CR4_KR4_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapMSA_100G_CR2_KR2) {
        PHYMOD_AN_CAP1_MSA_100G_CR2_KR2_SET(ability->an_cap_ext);
    }
    if (an_config.tech_ability_ext & bcmplpAnCapMSA_200G_CR4_KR4) {
        PHYMOD_AN_CAP1_MSA_200G_CR4_KR4_SET(ability->an_cap_ext);
    }

    if (fec_ability == bcmplpHardwareDefault) {
        PHYMOD_AN_FEC_OFF_SET(ability->an_fec);
    }
    if (fec_ability & bcmplpFecAbility) {
        PHYMOD_AN_FEC_CL74_SET(ability->an_fec);
    }
    if (fec_ability & bcmplpFecRequested) {
        PHYMOD_AN_FEC_CL74_REQUESTED_SET(ability->an_fec);
    }
    if (fec_ability & bcmplpFec91RSFecAbility) {
        PHYMOD_AN_FEC_CL91_SET(ability->an_fec);
    }
    if (fec_ability & bcmplpFec91RSFecRequested) {
        PHYMOD_AN_FEC_CL91_REQUESTED_SET(ability->an_fec);
    }
    if (fec_ability & bcmplpFecRS528) {
        PHYMOD_AN_FEC_RS528_SET(ability->an_fec);
    }
    if (fec_ability & bcmplpFecRS544) {
        PHYMOD_AN_FEC_RS544_SET(ability->an_fec);
    }
    if (fec_ability & bcmplpFecRS272) {
        PHYMOD_AN_FEC_RS272_SET(ability->an_fec);
    }
    if (fec_ability & bcmplpFecNone) {
        PHYMOD_AN_FEC_NONE_SET(ability->an_fec);
    }

    if (pause_ability & bcmplpSymmPause) {
        PHYMOD_AN_CAP_SYMM_PAUSE_SET(ability);
    }
    if (pause_ability & bcmplpAsymPause) {
        PHYMOD_AN_CAP_ASYM_PAUSE_SET(ability);
    }
    if (pause_ability & bcmplpAsymSymmPause) {
        PHYMOD_AN_CAP_ASYM_PAUSE_SET(ability);
        PHYMOD_AN_CAP_SYMM_PAUSE_SET(ability);
    }

    return BCM_PM_IF_SUCCESS;
}

int _bcm_plp_barchetta_phy_phymod_user_ability(plp_barchetta_phymod_autoneg_ability_t ability, unsigned short *fec_ability, unsigned short *pause_ability, bcm_plp_an_config_t* an_config)
{
    an_config->tech_ability = 0;
    an_config->tech_ability_ext = 0;
    *fec_ability = 0;

    if (PHYMOD_AN_CAP_1G_KX_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap1G_KX;
    }
    if (PHYMOD_AN_CAP_10G_KX4_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap10G_KX4;
    }
    if (PHYMOD_AN_CAP_10G_KR_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap10G_KR;
    }
    if (PHYMOD_AN_CAP_40G_KR4_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap40G_KR4;
    }
    if (PHYMOD_AN_CAP_40G_CR4_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap40G_CR4;
    }
    if (PHYMOD_AN_CAP_100G_CR10_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap100G_CR10;
    }
    if (PHYMOD_AN_CAP_100G_CR4_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap100G_CR4;
    }
    if (PHYMOD_AN_CAP_BAM_100G_CR4_KR4_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCapBAM_100G_CR4_KR4;
    }
    if (PHYMOD_AN_CAP_100G_KR4_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap100G_KR4;
    }
    if (PHYMOD_AN_CAP_25G_KRS1_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap25G_KRS1;
    }
    if (PHYMOD_AN_CAP_25G_CRS1_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap25G_CRS1;
    }
    if (PHYMOD_AN_CAP_25G_KR_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap25G_KR;
    }
    if (PHYMOD_AN_CAP_25G_CR_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap25G_CR;
    }
    if (PHYMOD_AN_CAP_50G_KR2_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap50G_KR2;
    }
    if (PHYMOD_AN_CAP_50G_CR2_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap50G_CR2;
    }
    if (PHYMOD_AN_CAP_25G_KR1_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap25G_KR1;
    }
    if (PHYMOD_AN_CAP_25G_CR1_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap25G_CR1;
    }
    if (PHYMOD_AN_CAP_50G_CR_KR_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap50G_CR_KR;
    }
    if (PHYMOD_AN_CAP_50G_CR1_KR1_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap50G_CR1_KR1;
    }
    if (PHYMOD_AN_CAP_100G_CR2_KR2_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap100G_CR2_KR2;
    }
    if (PHYMOD_AN_CAP_200G_CR4_KR4_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap200G_CR4_KR4;
    }
    if (PHYMOD_AN_CAP_50G_KP1_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap50G_KP;
    }
    if (PHYMOD_AN_CAP_100G_KP2_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap100G_KP2;
    }
    if (PHYMOD_AN_CAP_200G_KP4_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap200G_KP4;
    }
    if (PHYMOD_AN_CAP_400G_CR8_KR8_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCap400G_CR8_KR8;
    }
    if (PHYMOD_AN_CAP_BAM_50G_CR1_KR1_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCapBAM_50G_CR1_KR1;
    }
    if (PHYMOD_AN_CAP_BAM_50G_CR2_KR2_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCapBAM_50G_CR2_KR2;
    }
    if (PHYMOD_AN_CAP_BAM_100G_CR2_KR2_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCapBAM_100G_CR2_KR2;
    }
    if (PHYMOD_AN_CAP_BAM_100G_CR4_KR4_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCapBAM_100G_CR4_KR4;
    }
    if (PHYMOD_AN_CAP_BAM_200G_CR4_KR4_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCapBAM_200G_CR4_KR4;
    }
    if (PHYMOD_AN_CAP_BAM_400G_CR8_KR8_GET(ability.an_cap)) {
        an_config->tech_ability |= bcmplpAnCapBAM_400G_CR8_KR8;
    }
    if (PHYMOD_AN_CAP1_BAM_20G_CR1_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapBAM_20G_KR1;
    }
    if (PHYMOD_AN_CAP1_BAM_20G_KR1_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapBAM_20G_KR1;
    }
    if (PHYMOD_AN_CAP1_BAM_20G_CR2_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapBAM_20G_CR2;
    }
    if (PHYMOD_AN_CAP1_BAM_20G_KR2_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapBAM_20G_KR2;
    }
    if (PHYMOD_AN_CAP1_BAM_25G_CR1_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapBAM_25G_CR1;
    }
    if (PHYMOD_AN_CAP1_BAM_25G_KR1_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapBAM_25G_KR1;
    }
    if (PHYMOD_AN_CAP1_BAM_40G_CR2_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapBAM_40G_CR2;
    }
    if (PHYMOD_AN_CAP1_BAM_40G_KR2_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapBAM_40G_KR2;
    }
    if (PHYMOD_AN_CAP1_BAM_50G_CR4_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapBAM_50G_CR4;
    }
    if (PHYMOD_AN_CAP1_BAM_50G_KR4_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapBAM_50G_KR4;
    }
    if (PHYMOD_AN_CAP1_BAM_100G_CR1_KR1_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapBAM_100G_CR1_KR1;
    }
    if (PHYMOD_AN_CAP1_100G_CR1_KR1_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCap100G_CR1_KR1;
    }
    if (PHYMOD_AN_CAP1_200G_CR2_KR2_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCap200G_CR2_KR2;
    }
    if (PHYMOD_AN_CAP1_400G_CR4_KR4_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCap400G_CR4_KR4;
    }
    if (PHYMOD_AN_CAP1_MSA_100G_CR2_KR2_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapMSA_100G_CR2_KR2;
    }
    if (PHYMOD_AN_CAP1_MSA_200G_CR4_KR4_GET(ability.an_cap_ext)) {
        an_config->tech_ability_ext |= bcmplpAnCapMSA_200G_CR4_KR4;
    }

    if (PHYMOD_AN_FEC_OFF_GET(ability.an_fec)) {
        *fec_ability |= bcmplpHardwareDefault;
    }
    if (PHYMOD_AN_FEC_CL74_GET(ability.an_fec)) {
        *fec_ability |= bcmplpFecAbility;
    }
    if (PHYMOD_AN_FEC_CL74_REQUESTED_GET(ability.an_fec)) {
        *fec_ability |= bcmplpFecRequested;
    }
    if (PHYMOD_AN_FEC_CL91_GET(ability.an_fec)) {
        *fec_ability |= bcmplpFec91RSFecAbility;
    }
    if (PHYMOD_AN_FEC_CL91_REQUESTED_GET(ability.an_fec)) {
        *fec_ability |= bcmplpFec91RSFecRequested;
    }
    if (PHYMOD_AN_FEC_RS528_GET(ability.an_fec)) {
        *fec_ability |= bcmplpFecRS528;
    }
    if (PHYMOD_AN_FEC_RS544_GET(ability.an_fec)) {
        *fec_ability |= bcmplpFecRS544;
    }
    if (PHYMOD_AN_FEC_RS272_GET(ability.an_fec)) {
        *fec_ability |= bcmplpFecRS272;
    }
    if (PHYMOD_AN_FEC_NONE_GET(ability.an_fec)) {
        *fec_ability |= bcmplpFecNone;
    }

    if (PHYMOD_AN_CAP_SYMM_PAUSE_GET(&ability) && !PHYMOD_AN_CAP_ASYM_PAUSE_GET(&ability)) {
        *pause_ability = bcmplpSymmPause;
    }
    if (PHYMOD_AN_CAP_ASYM_PAUSE_GET(&ability) && !PHYMOD_AN_CAP_SYMM_PAUSE_GET(&ability)) {
        *pause_ability = bcmplpAsymPause;
    }
    if (PHYMOD_AN_CAP_ASYM_PAUSE_GET(&ability) && PHYMOD_AN_CAP_SYMM_PAUSE_GET(&ability)) {
        *pause_ability = bcmplpAsymSymmPause;
    }
    if (PHYMOD_AN_CAP_SGMII_GET(&ability)) {
        if (ability.sgmii_speed == phymod_CL37_SGMII_10M) {
            an_config->tech_ability |= 0x40000000;
        }
        if (ability.sgmii_speed == phymod_CL37_SGMII_100M) {
            an_config->tech_ability |= 0x80000000;
        }
        if (ability.sgmii_speed == phymod_CL37_SGMII_1000M) {
            an_config->tech_ability_ext |= 0x40000000;
        }
    }
    if (PHYMOD_AN_CAP_CL37_GET(&ability)) {
        an_config->tech_ability_ext |= 0x80000000;
    }
    return BCM_PM_IF_SUCCESS;
}

int bcm_plp_barchetta_phy_get_phy_type (bcm_plp_access_t phy_info, plp_barchetta_phymod_dispatch_type_t *phy_type,
                         int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val))
{
    unsigned int id0 = 0, id1 = 0;
    int rv = 0;
#if defined(PHYMOD_GALLARDO28_SUPPORT)
    unsigned int saved_phy_id = 0;
    unsigned int pmd_mode = 0;
#endif
#if defined(PHYMOD_MILLENIOB_SUPPORT)
    unsigned int rev_id = 0;
#endif

    *phy_type = 0xFFFF;
#if defined (PHYMOD_GALLARDO28_SUPPORT)
    if (phy_info.flags & BCM_PLP_WARM_BOOT){
        saved_phy_id = phy_info.phy_addr;
        rv = _gallardo28_get_single_pmd_mode(phy_info, read, &pmd_mode);
        if (rv != 0) {
            PHYMOD_DEBUG_ERROR(("Invalid PHY ID\n"));
            goto ERR;
        }
        if(pmd_mode){ /* If it is single PMD mode only base phy address is accessible */
            phy_info.phy_addr = (saved_phy_id & ~0x3);
        }
        BCM_PM_IF_ERR_RETURN(
                read(phy_info.platform_ctxt,phy_info.phy_addr, 0x10002, &id0));
        BCM_PM_IF_ERR_RETURN(
                read(phy_info.platform_ctxt, phy_info.phy_addr, 0x10003, &id1));
    } else {
        BCM_PM_IF_ERR_RETURN(
                read(phy_info.platform_ctxt, phy_info.phy_addr, 0x10002, &id0));
        BCM_PM_IF_ERR_RETURN(
                read(phy_info.platform_ctxt, phy_info.phy_addr, 0x10003, &id1));
    }
#else
    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x10002, &id0));
    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x10003, &id1));
#endif

#ifdef PHYMOD_QUADRA28_SUPPORT
    if (id0 == 0xae02 && id1 == 0x5250) {
        PHYMOD_DEBUG_INFO(("Inside QUADRA28_SUPPORT\n"));
        *phy_type = phymodDispatchTypeQuadra28;
    }
#endif

#ifdef PHYMOD_GALLARDO28_SUPPORT
    if (id0 == 0xae02 && id1 == 0x5210) {
        PHYMOD_DEBUG_INFO(("Inside GALLARDO28_SUPPORT\n"));
        *phy_type = phymodDispatchTypeGallardo28;
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
        *phy_type = phymodDispatchTypeEvora;
    }
#else
    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b00, &id0));

    PHYMOD_DEBUG_INFO(("EVORA chip-ID LSB:%x for PHY:%x\n", id0, phy_info.phy_addr));

    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b01, &id1));

    PHYMOD_DEBUG_INFO(("EVORA chip ID MSB and REV:0x%x\n", id1));
    if (id0 == 0x2392 || id0 == 0x2391 || id0 == 0x2394) {
        *phy_type = phymodDispatchTypeEvora;
    }
#endif
#endif
#ifdef PHYMOD_DINO_SUPPORT
    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b00, &id0));

    PHYMOD_DEBUG_INFO(("DINO chip-ID LSB:%x for PHY:%x\n", id0, phy_info.phy_addr));

    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b01, &id1));

    PHYMOD_DEBUG_INFO(("DINO chip ID MSB and REV:0x%x\n", id1));
    if (id0 == 0x2332 || id0 == 0x2793 || id0 == 0x2795) {
        *phy_type = phymodDispatchTypeDino;
    }
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b00, &id0));

    PHYMOD_DEBUG_INFO(("Miura chip-ID LSB:%x for PHY:%x\n", id0, phy_info.phy_addr));

    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b01, &id1));

    PHYMOD_DEBUG_INFO(("Miura chip ID MSB and REV:0x%x\n", id1));
    if (id0 == 0x2756 || id0 == 0x2757 || id0 == 0x2759 || id0 == 0x2755) {
        *phy_type = phymodDispatchTypeMiura;
    }
#endif
#ifdef PHYMOD_ESTOQUE_SUPPORT
    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1e1800, &id0));

    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1e1801, &id1));

    if (id0 == 0x1141 && id1 == 0x8) {
        *phy_type = phymodDispatchTypeEstoque;
    }
#endif
#ifdef PHYMOD_MILLENIO_SUPPORT
#ifdef PHYMOD_MILLENIOB_SUPPORT
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1EB2E9, &id0));
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1EB2EA, &id1));
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1EB2CA, &rev_id));

    if (id0 == 0x1358 && id1 == 0x8 && rev_id == 0xB0) {
        *phy_type = phymodDispatchTypeMillenio;
    }
#else
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1EB2E9, &id0));
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1EB2EA, &id1));

    if (id0 == 0x1358 && id1 == 0x8) {
        *phy_type = phymodDispatchTypeMillenio;
    }
#endif
#endif
#ifdef PHYMOD_BARCHETTA2_SUPPORT
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1EC400, &id0));
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x1EC401, &id1));

    PHYMOD_DEBUG_INFO(("Barchetta2 chip-ID LSB:%x for PHY:%x\n", id0, phy_info.phy_addr));
    PHYMOD_DEBUG_INFO(("Barchetta2 chip ID MSB and REV:0x%x\n", id1));
    if (id0 == 0x7328 || id0 == 0x7326 || id0 == 0x7728) {
        *phy_type = phymodDispatchTypeBarchetta2;
    }
#endif
#ifdef PHYMOD_BARCHETTA_SUPPORT
    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18500, &id0));
    PHYMOD_DEBUG_INFO(("Barchetta chip-ID LSB:%x for PHY:%x\n", id0, phy_info.phy_addr));
    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18501, &id1));
    PHYMOD_DEBUG_INFO(("Barchetta chip ID MSB and REV:0x%x\n", id1));
    if (id0 == 0x1381 || id0 == 0x1337 || id0 == 0x1338 || id0 == 0x1321|| id0 == 0x1764) {
        *phy_type = phymodDispatchTypeBarchetta;
    }
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b00, &id0));
    PHYMOD_DEBUG_INFO(("Aperta CHIPID:%x for PHY:%x\n", id0, phy_info.phy_addr));
    BCM_PM_IF_ERR_RETURN(
            read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b01, &id1));
    PHYMOD_DEBUG_INFO(("Aperta chip ID and REV:0x%x\n", id1));
    if (id0 == 0x1388 || id0 == 0x1343 || id0 == 0x1384 || id0 == 0x1385
            || id0 == 0x1394 || id0 == 0x1398 || id0 == 0x1392) {
        *phy_type = phymodDispatchTypeAperta;
    }
#endif
#ifdef PHYMOD_AGERA_SUPPORT
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b00, &id0));
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b01, &id1));
    if (id0 == 0x7330 || id0 == 0x7360) {
         PHYMOD_DEBUG_INFO(("AGERA_SUPPORT\n"));
        *phy_type = phymodDispatchTypeAgera;
    }
#endif
#ifdef PHYMOD_AGERALITE_SUPPORT
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b00, &id0));
    BCM_PM_IF_ERR_RETURN(read(phy_info.platform_ctxt, phy_info.phy_addr, 0x18b01, &id1));
    if (id0 == 0x7350) {
         PHYMOD_DEBUG_INFO(("AGERALITE_SUPPORT\n"));
        *phy_type = phymodDispatchTypeAgeralite;
    }
#endif
#ifdef PHYMOD_KOI_SUPPORT       /* 84858/58R/56R */
    if ( (id0 == 0x600d) && ((id1 == 0x8562) || (id1 == 0x8552)) ) {
        PHYMOD_DEBUG_INFO(("KOI_SUPPORT\n"));
        *phy_type = phymodDispatchTypePhy848xx;
    }
#endif

/* ID1 Mapping for ORCA
 * PHY           A0       B0
 * BCM84888    0x5140    0x5141
 * BCM84884    0x5148    0x5149
 * BCM84887    0x5144    0x5145
 * BCM84881    0x5150    0x5151
 * BCM84880    0x5158    0x5159
 * BCM84888E   0x5160    0x5161
 * BCM84884E   0x5168    0x5169
 * BCM84888S   0x5174    0x5175
 * BCM84885    0x5178    0x5179
 * BCM84886    0x5170    0x5171
 */
#ifdef PHYMOD_ORCA_SUPPORT      /* 8488x */
    if ( (id0 == 0xae02) && ((id1 >= 0x5140) && (id1 < 0x5180)) ) {
        PHYMOD_DEBUG_INFO(("ORCA_SUPPORT\n"));
        *phy_type = phymodDispatchTypePhy848xx;
    }
#endif

/* ID1 Mapping for SHORTFIN
 * Part       A0     B0
 * BCM84898   0x5000 0x5001
 * BCM84896   0x5004 0x5005
 * BCM54998S  0x5008 0x5009
 * BCM54998ES 0x500C 0x500D
 * BCM54998   0x5010 0x5011
 * BCM54998E  0x5014 0x5015
 * BCM54994L         0x5019
 * BCM54994EL        0x501D
 */
#ifdef PHYMOD_SHORTFIN_SUPPORT  /* 8489x/5499x */
    if ( (id0 == 0x3590) &&
            ((id1 == 0x5001) || (id1 == 0x5005) || (id1 == 0x5009) ||
             (id1 == 0x500D) || (id1 == 0x5011) || (id1 == 0x5015) ||
                                (id1 == 0x5019) || (id1 == 0x501D)  ) ) {
        PHYMOD_DEBUG_INFO(("SHORTFIN CHIPID id0:0x%x id1:0x%x for PHY:%x\n",
                           id0, id1, phy_info.phy_addr));
        *phy_type = phymodDispatchTypeXgbaset;
    }
#endif

/* ID1 Mapping for BLACKFIN
 * Part       A0       B0
 * BCM84891   0x5090   0x5091
 * BCM54991   0x5094   0x5095
 * BCM54991E  0x5098   0x5099
 * BCM84891L  0x5080   0x5081
 * BCM54991L  0x5084   0x5085
 * BCM54991EL 0x5088   0x5089
 * BCM84892   0x50A0   0x50A1
 * BCM54992   0x50A4   0x50A5
 * BCM54992E  0x50A8   0x50A9
 * BCM84894   0x50B0   0x50B1
 * BCM54994   0x50B4   0x50B5
 * BCM54994E  0x50B8   0x50B9
 * BCM50991EL 0x50C8   0x50C9
 * BCM54991H  0x50D0   0x50D1
 * BCM54994H  0x50F0   0x50F1
 * BCM50994E  0x50F8   0x50F9
 */
#ifdef PHYMOD_BLACKFIN_SUPPORT  /* 8489x/5499x */
    if ( (id0 == 0x3590) && ((id1 >= 0x5080) && (id1 <= 0x50ff)) ) {
        PHYMOD_DEBUG_INFO(("BLACKFIN CHIPID id0:0x%x id1:0x%x for PHY:%x\n",
                           id0, id1, phy_info.phy_addr));
        *phy_type = phymodDispatchTypeXgbaset;
    }
#endif

/* ID1 Mapping for LONGFIN
 * Part         A0
 * BCM84891LM   0x5180
 * BCM54991LM   0x5184
 * BCM54991ELM  0x5188
 * BCM84891M    0x5190
 * BCM54991M    0x5194
 * BCM54991EM   0x5198
 * BCM84892M    0x51A0
 * BCM54992M    0x51A4
 * BCM54992EM   0x51A8
 * BCM84894M    0x51B0
 * BCM54994M    0x51B4
 * BCM54994EM   0x51B8
 * BCM54991H    0x51D0
 * BCM54994H    0x51F0
 */
#ifdef PHYMOD_LONGFIN_SUPPORT   /* 8489xM/5499xM */
    if ( (id0 == 0x3590) && ((id1 >= 0x5180) && (id1 <= 0x51ff)) ) {
        PHYMOD_DEBUG_INFO(("LONGFIN CHIPID id0:0x%x id1:0x%x for PHY:%x\n",
                           id0, id1, phy_info.phy_addr));
        *phy_type = phymodDispatchTypeXgbaset;
    }
#endif

/* ID1 Mapping for Whitetip
 * Part         A0
 * BCM84901L    0x5280
 * BCM84891LP   0x5284
 * BCM54991LP   0x5288
 * BCM54991ELP  0x528C
 * BCM84901     0x5290
 * BCM84891P    0x5294
 * BCM54991P    0x5298
 * BCM54991EP   0x529C
 * BCM84902     0x52A0
 * BCM84892P    0x52A4
 * BCM54992P    0x52A8
 * BCM54992EP   0x52AC
 * BCM84904     0x52B0
 * BCM84894P    0x52B4
 * BCM54994P    0x52B8
 * BCM54994EP   0x52BC
 * BCM54991HP   0x52E0
 * BCM54994HP   0x52F0
 */
#ifdef PHYMOD_WHITETIP_SUPPORT   /* 8490x */
    if ( (id0 == 0x3590) && ((id1 >= 0x5280) && (id1 <= 0x52ff)) ) {
        PHYMOD_DEBUG_INFO(("WHITETIP CHIPID id0:0x%x id1:0x%x for PHY:%x\n",
                           id0, id1, phy_info.phy_addr));
        *phy_type = phymodDispatchTypeXgbaset;
    }
#endif

/* ID1 Mapping for BROADFIN
 * Part         A0
 * BCM54998ESM  0x500C
 * BCM54998EM   0x5014
 * BCM54998SM   0x5008
 * BCM54998M    0x5010
 * BCM84898M    0x5000
 */
#ifdef PHYMOD_BROADFIN_SUPPORT   /* 8489xM/5499xM */
    if ( (id0 == 0x3590) &&
            ((id1 == 0x5000) || (id1 == 0x5008) || (id1 == 0x500C) ||
             (id1 == 0x5010) || (id1 == 0x5014) ) ) {
        PHYMOD_DEBUG_INFO(("BROADFIN CHIPID id0:0x%x id1:0x%x for PHY:%x\n",
                           id0, id1, phy_info.phy_addr));
        *phy_type = phymodDispatchTypeXgbaset;
    }
#endif

    /* ID1 Mapping for COWBOYS PHY
     * 0x84A2 - PHY54210E
     * 0x84A6 - PHY50210E
     * 0x84AA - PHY54216E
     * 0x84AE - PHY54214E
     */
#ifdef PHYMOD_COWBOYS_SUPPORT
    if ((id0 == 0x600d) && ((id1 == 0x84A2) || (id1 == 0x84A6) || (id1 == 0x84AA) || (id1 == 0x84AE) )) {
        PHYMOD_DEBUG_INFO(("COWBOYS_SUPPORT\n"));
        *phy_type = phymodDispatchTypePhy542xx;
    }
#endif

ERR:
    return rv;
}
