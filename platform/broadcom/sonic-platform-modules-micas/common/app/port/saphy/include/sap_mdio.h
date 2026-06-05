/********************************************************************************
 * Copyright(C) 2020 Micas Network. All rights reserved.
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期        | 作者     |  修改记录
**===============================================================================
**| 2025/04/17  | zhoutenghui  |  创建该文件
**
*********************************************************************************/
#ifndef _SAP_MDIO_H_
#define _SAP_MDIO_H_

#define MAX_MDIO_TYPE_STR_LEN 32

typedef int (*mdio_read_func_t)(void *user_acc, uint8_t mdio_index, uint32_t phy_addr, uint32_t reg_addr, uint32_t *data);
typedef int (*mdio_write_func_t)(void *user_acc, uint8_t mdio_index, uint32_t phy_addr, uint32_t reg_addr, uint32_t data);

typedef struct
{
    char type[MAX_MDIO_TYPE_STR_LEN];
    mdio_read_func_t read_func;
    mdio_write_func_t write_func;
} sap_mdio_info_t;

int sap_mdio_init(void);

/* 新接口 */
int sap_mdio_read_func_get(char *type, mdio_read_func_t *func);
int sap_mdio_write_func_get(char *type, mdio_write_func_t *func);
int sap_mdio_type_reg(char *type, mdio_read_func_t read_func, mdio_write_func_t write_func);


/* 旧接口,准备废弃 */
typedef int (*mdio_read_func)(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int *data);
typedef int (*mdio_write_func)(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int data);
typedef enum {
    SAP_MDIO_TYPE_MAC = 0,
    SAP_MDIO_TYPE_SYSFS = 1,
    SAP_MDIO_TYPE_SYSFS_CUST = 2,
    SAP_MDIO_TYPE_MAX
} sap_mdio_type_e;
int sap_get_mdio_read_func(sap_mdio_type_e type, mdio_read_func *func);
int sap_get_mdio_write_func(sap_mdio_type_e type, mdio_write_func *func);
int sap_mdio_func_reg(sap_mdio_type_e type, mdio_read_func read_func, mdio_write_func write_func);

#endif
