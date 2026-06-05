/*
 *         
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *         
 *     
 */

/*
 *         
 * $Id: phymod.c,v 1.2.2.13 Broadcom SDK $
 * 
 * $Copyright: 
 * All Rights Reserved.$
 *         
 *     
 */

#include <phymod/phymod.h>
#include <phymod/phymod_dispatch.h>
#include <phymod/phymod_acc.h>

int plp_aperta2_phymod_core_probe(const plp_aperta2_phymod_access_t* access, plp_aperta2_phymod_dispatch_type_t* type, int* is_probed)
{
    uint32_t is_identified;
    plp_aperta2_phymod_dispatch_type_t idx;
    plp_aperta2_phymod_core_access_t core;
    uint32_t core_id;
    int rv=PHYMOD_E_NONE;

    if(is_probed == NULL) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("is_probed NULL parameter")));
    }
    *is_probed = 0;

    /*parameters validation*/
    if(PHYMOD_E_OK != plp_aperta2_phymod_access_t_validate(access)) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("access validation failed")));
    }

    if(type == NULL) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("type NULL parameter")));
    }

    core_id = 0;
    
    /*Identify phy type & driver*/
    PHYMOD_MEMCPY(&core.access, access, sizeof(core.access));
    core.type = phymodDispatchTypeCount;
    is_identified = 0;
    for(idx = 0 ; idx < phymodDispatchTypeCount ; idx++) {
        if(NULL != __plp_aperta2_phymod__dispatch__[idx]->f_phymod_core_identify) {
            rv = __plp_aperta2_phymod__dispatch__[idx]->f_phymod_core_identify(&core, core_id, &is_identified);
            if (rv == PHYMOD_E_NONE && is_identified) {
                (*type) = idx;
                *is_probed = 1;
                break;
            }
        }
    }
         
    return PHYMOD_E_NONE;
}


int plp_aperta2_phymod_osr_mode_to_actual_os(plp_aperta2_phymod_osr_mode_t osr_mode, uint32_t* os_int, uint32_t* os_remainder)
{
    if(PHYMOD_E_OK != plp_aperta2_phymod_osr_mode_t_validate(osr_mode)) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("osr_mode validation failed")));
    }

    if(os_int == NULL) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("os_int NULL parameter")));
    }
    if(os_remainder == NULL) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("os_remainder NULL parameter")));
    }

    *os_int = 0;
    *os_remainder = 0;

    switch (osr_mode) {
        case phymodOversampleMode1: 
            *os_int = 1;
            break;
        case phymodOversampleMode2: 
            *os_int = 2; 
            break;
        case phymodOversampleMode3: 
            *os_int = 3; 
            break;
        case phymodOversampleMode3P3: 
            *os_int = 3;
            *os_remainder = 300;
            break;
        case phymodOversampleMode4: 
            *os_int = 4; 
            break;
        case phymodOversampleMode5: 
            *os_int = 5; 
            break;
        case phymodOversampleMode7P5: 
            *os_int = 7;
            *os_remainder = 500;
            break;
        case phymodOversampleMode8: 
            *os_int = 8; 
            break;
        case phymodOversampleMode8P25:
            *os_int = 8;
            *os_remainder = 250;
            break;
        case phymodOversampleMode10: 
            *os_int = 10; 
            break;
        case phymodOversampleMode16P5:
            *os_int = 16;
            *os_remainder = 500;
            break;
        case phymodOversampleMode20P625:
            *os_int = 20;
            *os_remainder = 625;
            break;
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("OS mode not supported")));
    }
         
    return PHYMOD_E_NONE;

}




