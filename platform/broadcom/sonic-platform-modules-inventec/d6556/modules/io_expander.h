/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef IO_EXPANDER_H
#define IO_EXPANDER_H

#include <linux/types.h>


/* IOEXP type define (SFP series) */
#define IOEXP_TYPE_MAGINOLIA_NAB      (10101)
#define IOEXP_TYPE_MAGINOLIA_4AB      (10102)
#define IOEXP_TYPE_CYPRESS_NABC       (10103)
#define IOEXP_TYPE_MAPLE_NABC         (10104)

/* IOEXP type define (QSFP series) */
#define IOEXP_TYPE_MAGINOLIA_7AB      (10201)
#define IOEXP_TYPE_REDWOOD_P01P08     (10202)
#define IOEXP_TYPE_REDWOOD_P09P16     (10203)
#define IOEXP_TYPE_HUDSON32IGA_P01P08 (10204)
#define IOEXP_TYPE_HUDSON32IGA_P09P16 (10205)
#define IOEXP_TYPE_SPRUCE_7AB         (10206)
#define IOEXP_TYPE_CYPRESS_7ABC       (10207)
#define IOEXP_TYPE_TAHOE_5A           (10208)
#define IOEXP_TYPE_TAHOE_6ABC         (10209)
#define IOEXP_TYPE_SEQUOIA_NABC       (10210)
#define IOEXP_TYPE_LAVENDER_P65       (10211)
#define IOEXP_TYPE_MAPLE_0ABC         (10212)

/* CPLD type define */
#define CPLD_TYPE_COTTONWOOD          (10301)

/* IOEXP mode define */
#define IOEXP_MODE_POLLING            (19000)
#define IOEXP_MODE_DIRECT             (19001)

/* IOEXP state define */
#define STATE_IOEXP_NORMAL                (0)
#define STATE_IOEXP_INIT                 (-1)
#define STATE_IOEXP_ABNORMAL             (-2)

/* IOEXP error code define */
#define ERR_IOEXP_NOTSUPPORT           (-100)
#define ERR_IOEXP_UNINIT               (-101)
#define ERR_IOEXP_BADCONF              (-102)
#define ERR_IOEXP_ABNORMAL             (-103)
#define ERR_IOEXP_NOSTATE              (-104)
#define ERR_IOEXP_BADINPUT             (-105)
#define ERR_IOEXP_UNEXCPT              (-199)

#define SWPS_INFO(fmt, args...) printk( KERN_INFO "[SWPS] " fmt, ##args)
#define SWPS_WARN(fmt, args...) printk( KERN_WARNING "[SWPS] " fmt, ##args)
#define SWPS_ERR(fmt, args...)  printk( KERN_ERR  "[SWPS] " fmt, ##args)

#ifdef DEBUG_SWPS
#    define SWPS_DEBUG(fmt, args...)  printk( KERN_DEBUG  "[SWPS] " fmt, ##args)
#else
#    define SWPS_DEBUG(fmt, args...)
#endif


struct ioexp_addr_s {
    int chan_id;
    int chip_addr;
    int read_offset[8];
    int write_offset[8];
    int conf_offset[8];
    uint8_t data_default[8]; 
    uint8_t conf_default[8]; 
};

struct ioexp_i2c_s {
    int chip_id;
    struct i2c_client  *i2c_client_p;
    struct ioexp_i2c_s *next;
};


struct ioexp_bitmap_s {
    int chip_id;        /* IOEXP chip id        */
    int ioexp_voffset;  /* IOEXP virtual offset */
    int bit_shift;
};

struct ioexp_map_s {
    int chip_amount;    /* Number of chips that IOEXP object content    */
    int data_width;     /* Number of (Read/Write/Config) bytes          */
    struct ioexp_addr_s   *map_addr;           /* Chip address info      */
    struct ioexp_bitmap_s  map_present[10];    /* IOEXP for SFP / QSFP   */
    struct ioexp_bitmap_s  map_tx_disable[10]; /* IOEXP for SFP          */
    struct ioexp_bitmap_s  map_tx_fault[10];   /* IOEXP for SFP          */
    struct ioexp_bitmap_s  map_rxlos[10];      /* IOEXP for SFP          */
    struct ioexp_bitmap_s  map_reset[10];      /* IOEXP for QSFP         */
    struct ioexp_bitmap_s  map_lpmod[10];      /* IOEXP for QSFP         */
    struct ioexp_bitmap_s  map_modsel[10];     /* IOEXP for QSFP         */
    struct ioexp_bitmap_s  map_hard_rs0[10];   /* IOEXP for QSFP         */
    struct ioexp_bitmap_s  map_hard_rs1[10];   /* IOEXP for QSFP         */
};

struct ioexp_data_s {
    uint8_t data[8];
};

struct ioexp_obj_s {

    /* ============================
     *  Object public property
     * ============================
     */
    int ioexp_id;
    int ioexp_type;

    /* ============================
     *  Object private property
     * ============================
     */
    struct ioexp_data_s chip_data[16];   /* Max: 8-ioexp in one virt-ioexp(ioexp_obj) */
    struct ioexp_map_s *ioexp_map_p;
    struct ioexp_obj_s *next;
    struct ioexp_i2c_s *i2c_head_p;
    struct mutex lock;
    int mode;
    int state;

    /* ===========================================
     *  Object public functions
     * ===========================================
     */
    int (*get_present)(struct ioexp_obj_s *self, int virt_offset);
    int (*get_tx_fault)(struct ioexp_obj_s *self, int virt_offset);
    int (*get_rxlos)(struct ioexp_obj_s *self, int virt_offset);
    int (*get_tx_disable)(struct ioexp_obj_s *self, int virt_offset);
    int (*get_reset)(struct ioexp_obj_s *self, int virt_offset);
    int (*get_lpmod)(struct ioexp_obj_s *self, int virt_offset);
    int (*get_modsel)(struct ioexp_obj_s *self, int virt_offset);
    int (*get_hard_rs0)(struct ioexp_obj_s *self, int virt_offset);
    int (*get_hard_rs1)(struct ioexp_obj_s *self, int virt_offset);
    int (*set_tx_disable)(struct ioexp_obj_s *self, int virt_offset, int input_val);
    int (*set_reset)(struct ioexp_obj_s *self, int virt_offset, int input_val);
    int (*set_lpmod)(struct ioexp_obj_s *self, int virt_offset, int input_val);
    int (*set_modsel)(struct ioexp_obj_s *self, int virt_offset, int input_val);
    int (*set_hard_rs0)(struct ioexp_obj_s *self, int virt_offset, int input_val);
    int (*set_hard_rs1)(struct ioexp_obj_s *self, int virt_offset, int input_val);

    /* ===========================================
     *  Object private functions
     * ===========================================
     */
    int (*init)(struct ioexp_obj_s *self);
    int (*check)(struct ioexp_obj_s *self);
    int (*update_all)(struct ioexp_obj_s *self, int show_err, char *caller_name);
    int (*fsm_4_direct)(struct ioexp_obj_s* self);
    int (*fsm_4_polling)(struct ioexp_obj_s* self);
};


struct ioexp_obj_s* get_ioexp_obj(int ioexp_id);
int  create_ioexp_obj(int ioexp_id,
                      int ioexp_type,
                      struct ioexp_addr_s *addr_map_p,
                      int run_mode);
int  init_ioexp_objs(void);
int  check_ioexp_objs(void);
void clean_ioexp_objs(void);

void unlock_ioexp_all(void);
int  lock_ioexp_all(void);

int  check_channel_tier_1(void);
int  resync_channel_tier_1(void);

/* Macro for bit control */
#define SWP_BIT_SET(byte_val,bit_shift)   ((byte_val) |= (1<<(bit_shift)))
#define SWP_BIT_CLEAR(byte_val,bit_shift) ((byte_val) &= ~(1<<(bit_shift)))


#endif /* IO_EXPANDER_H */






