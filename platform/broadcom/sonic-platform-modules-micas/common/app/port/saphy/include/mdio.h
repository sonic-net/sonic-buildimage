/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * MDIO read/write interface header.
 ********************************************************************************/
#ifndef _PORT_MDIO_H_
#define _PORT_MDIO_H_

#include <stdint.h>

#define MAX_MDIO_TYPE_STR_LEN  (32)

/* MDIO access type enum */
typedef enum {
    MDIO_TYPE_MAC = 0,
    MDIO_TYPE_SYSFS,
    MDIO_TYPE_SYSFS_CUST,
    MDIO_TYPE_MAX
} mdio_type_e;

/* MDIO read/write function pointer types */
typedef int (*mdio_read_func_t)(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int *data);
typedef int (*mdio_write_func_t)(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int data);

/* MDIO info structure */
typedef struct {
    char type[MAX_MDIO_TYPE_STR_LEN];
    mdio_read_func_t read_func;
    mdio_write_func_t write_func;
} mdio_info_t;

/* ==========================================================================
 * Init
 * ========================================================================== */
int mdio_init(void);

/* ==========================================================================
 * Function registration & query
 * ========================================================================== */
int get_mdio_read_func(mdio_type_e type, mdio_read_func_t *func);
int get_mdio_write_func(mdio_type_e type, mdio_write_func_t *func);

/* ==========================================================================
 * String-based type registration/query (extended interface)
 * ========================================================================== */
int mdio_type_reg(char *type, mdio_read_func_t read_func,
                  mdio_write_func_t write_func);

#endif /* _PORT_MDIO_H_ */
