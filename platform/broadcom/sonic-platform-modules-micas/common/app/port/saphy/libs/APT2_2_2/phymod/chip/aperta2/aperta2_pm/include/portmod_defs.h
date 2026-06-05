/*
 *         
 * $Id:$
 * 
 * $Copyright: (c) 2013 Broadcom Corp. All Rights Reserved.$
 *         
 *     
 *
 */

#ifndef _PORTMOD_DEFS_H__H_
#define _PORTMOD_DEFS_H__H_

#ifdef PHYMOD_SUPPORT 
#include <phymod/phymod.h>
#endif /* PHYMOD_SUPPORT */
#include <include/plp_soc_portmod.h>


#define MAX_LANES_PER_PORT (24)
#define MAX_PHYN (3)
/* ILKN PM doesn't manage phymod objects directly, 
    instead it take over other PMs and control them using portmod APIs.
    Each ILKN PM controls up to 6 PM*/
#define PORTMOD_MAX_ILKN_AGGREGATED_PMS (6)

#define MAX_NUM_CORES PORTMOD_MAX_ILKN_AGGREGATED_PMS
#define PORTMOD_PBMP_CLEAR(x) (x) = 0;
typedef int ilkn_retransmit_config_t;
typedef int phymod_firmware_mode_t;
typedef soc_port_ability_t portmod_port_ability_t;
typedef soc_port_if_t portmod_port_if_t;
typedef unsigned int portmod_port_mode_t;

typedef  unsigned int portmod_pa_encap_t;


typedef struct portmod_lanes_assign_info_s{
    uint32 valid_entries;
    uint32 lane_assign[MAX_LANES_PER_PORT];
}portmod_lanes_assign_info_t;



extern int portmod_pbmp_bmnull(portmod_pbmp_t *bmp);
extern int portmod_pbmp_bmeq(portmod_pbmp_t *bmp1, portmod_pbmp_t *bmp2);

#define PORTMOD_PBMP_PORTS_RANGE_ADD(bm, first_port, range) \
    do {\
        uint32 _mask_;\
        int _first_port_, _range_;\
        _first_port_ = first_port; _range_ = range;\
        while (_range_ > 0) {\
            _mask_ = ~0;\
            if (_range_ < _SHR_PBMP_WORD_WIDTH) _mask_ >>= (_SHR_PBMP_WORD_WIDTH - _range_);\
            _mask_ <<= (_first_port_ % _SHR_PBMP_WORD_WIDTH);\
            _SHR_PBMP_ENTRY(bm, _first_port_) |= _mask_; \
            _range_ += (_first_port_ % _SHR_PBMP_WORD_WIDTH) - _SHR_PBMP_WORD_WIDTH;\
            _first_port_ +=  _SHR_PBMP_WORD_WIDTH - (_first_port_ % _SHR_PBMP_WORD_WIDTH);\
        }\
    } while (0); 



#endif /*_PORTMOD_DEFS_H__H_*/
