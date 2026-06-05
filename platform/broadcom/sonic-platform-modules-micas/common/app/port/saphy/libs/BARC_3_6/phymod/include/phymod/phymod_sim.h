/*
 * $Id: $
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 */

#ifndef __plp_barchetta_PHYMOD_SIM_H__
#define __plp_barchetta_PHYMOD_SIM_H__

#include <phymod/phymod_system.h>

typedef enum {
    phymodSimEventNone,
    phymodSimEventLinkUp,
    phymodSimEventLinkDown,
    phymodSimEventAutoneg100,
    phymodSimEventAutoneg1000,
    phymodSimEventAutoneg10000,
    phymodSimEventCount
} phymod_sim_event_t;

typedef struct plp_barchetta_phymod_sim_entry_s {
    uint32_t flags;
    uint32_t addr;
    uint32_t data;
} plp_barchetta_phymod_sim_entry_t;

typedef struct plp_barchetta_phymod_sim_data_s {
    plp_barchetta_phymod_sim_entry_t *entries;
    int num_entries;
    int entries_used;
} plp_barchetta_phymod_sim_data_t;

typedef struct plp_barchetta_phymod_sim_drv_s {
    int (*init)(plp_barchetta_phymod_sim_data_t *psim, int num_ent, plp_barchetta_phymod_sim_entry_t *ent);
    int (*reset)(plp_barchetta_phymod_sim_data_t *psim);
    int (*read)(plp_barchetta_phymod_sim_data_t *psim, uint32_t addr, uint32_t *data);
    int (*write)(plp_barchetta_phymod_sim_data_t *psim, uint32_t addr, uint32_t data);
    int (*event)(plp_barchetta_phymod_sim_data_t *psim, phymod_sim_event_t evt);
} plp_barchetta_phymod_sim_drv_t;

typedef struct plp_barchetta_phymod_sim_s {
    plp_barchetta_phymod_sim_data_t data;
    plp_barchetta_phymod_sim_drv_t *drv;
} plp_barchetta_phymod_sim_t;

extern int
plp_barchetta_phymod_sim_init(plp_barchetta_phymod_sim_t *pms, int num_ent, plp_barchetta_phymod_sim_entry_t *ent);

extern int
plp_barchetta_phymod_sim_reset(plp_barchetta_phymod_sim_t *pms);

extern int
plp_barchetta_phymod_sim_read(plp_barchetta_phymod_sim_t *pms, uint32_t addr, uint32_t *data);

extern int
plp_barchetta_phymod_sim_write(plp_barchetta_phymod_sim_t *pms, uint32_t addr, uint32_t data);

extern int
plp_barchetta_phymod_sim_event(plp_barchetta_phymod_sim_t *pms, phymod_sim_event_t event);

#endif /* __plp_barchetta_PHYMOD_SIM_H__ */
