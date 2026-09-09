/*
 * A header definition for dfd_module driver
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __DFD_MODULE_H__
#define __DFD_MODULE_H__

typedef enum dfd_rv_s {
    DFD_RV_OK               = 0,
    DFD_RV_INIT_ERR         = 1,
    DFD_RV_SLOT_INVALID     = 2,
    DFD_RV_MODE_INVALID     = 3,
    DFD_RV_MODE_NOTSUPPORT  = 4,
    DFD_RV_TYPE_ERR         = 5,
    DFD_RV_DEV_NOTSUPPORT   = 6,
    DFD_RV_DEV_FAIL         = 7,
    DFD_RV_INDEX_INVALID    = 8,
    DFD_RV_NO_INTF          = 9,
    DFD_RV_NO_NODE          = 10,
    DFD_RV_NODE_FAIL        = 11,
    DFD_RV_INVALID_VALUE    = 12,
    DFD_RV_NO_MEMORY        = 13,
} dfd_rv_t;

typedef enum {
    DBG_VERBOSE = 0x01,
    DBG_WARN    = 0x02,
    DBG_ERROR   = 0x04,
} dbg_level_t;

extern int g_dfd_dbg_level;
extern int g_dfd_fan_dbg_level;
extern int g_dfd_slot_dbg_level;
extern int g_dfd_sensor_dbg_level;
extern int g_dfd_psu_dbg_level;
extern int g_dfd_sff_dbg_level;

#define DBG_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_FAN_DEBUG(level, fmt, arg...) do { \
            if (g_dfd_fan_dbg_level & level) { \
                if(level >= DBG_ERROR) { \
                    printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
                } else { \
                    printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
                } \
            } \
} while (0)

#define DFD_SLOT_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_slot_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_SENSOR_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_sensor_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_PSU_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_psu_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_SFF_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_sff_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

int dfd_get_dev_number(unsigned int main_dev_id, unsigned int minor_dev_id);

#endif  /* __DFD_MODULE_H__ */
