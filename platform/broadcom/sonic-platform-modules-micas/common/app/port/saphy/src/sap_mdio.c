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
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdint.h>

#include "sap_mdio.h"
#include "sap_common.h"

#define CMD_MDIO_READ   _IOR('M', 1, struct mdio_dev_user_info)
#define CMD_MDIO_WRITE  _IOR('M', 2, struct mdio_dev_user_info)

static mdio_read_func g_mdio_read_funcs[SAP_MDIO_TYPE_MAX];
static mdio_write_func g_mdio_write_funcs[SAP_MDIO_TYPE_MAX];

static sap_mdio_info_t g_mdio_info_list[SAP_MDIO_TYPE_MAX] = {0};
static uint8_t g_mdio_info_list_num = 0;
struct mdio_dev_user_info {
    int mdio_index;
    int phyaddr;
    uint32_t regaddr;
    uint32_t regval;
};

int sap_mdio_read_func_get(char *type, mdio_read_func_t *func)
{
    int i;
    if (type == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    for (i = 0; i < g_mdio_info_list_num; i++) {
        if (strncmp(g_mdio_info_list[i].type, type, strlen(type)) != 0) {
            continue;
        }
        *func = g_mdio_info_list[i].read_func;
        return SAP_STATUS_SUCCESS;
    }
    return SAP_STATUS_FAILURE;
}

int sap_mdio_write_func_get(char *type, mdio_write_func_t *func)
{
    int i;
    if (type == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    for (i = 0; i < g_mdio_info_list_num; i++) {
        if (strncmp(g_mdio_info_list[i].type, type, strlen(type)) != 0) {
            continue;
        }
        *func = g_mdio_info_list[i].write_func;
        return SAP_STATUS_SUCCESS;
    }
    return SAP_STATUS_FAILURE;
}

int sap_mdio_type_reg(char *type, mdio_read_func_t read_func, mdio_write_func_t write_func) {
    if (g_mdio_info_list_num < SAP_MDIO_TYPE_MAX) {
        snprintf(g_mdio_info_list[g_mdio_info_list_num].type, MAX_MDIO_TYPE_STR_LEN, "%s", type);
        g_mdio_info_list[g_mdio_info_list_num].read_func = read_func;
        g_mdio_info_list[g_mdio_info_list_num].write_func = write_func;
        g_mdio_info_list_num++;
        return SAP_STATUS_SUCCESS;
    }
    return SAP_STATUS_FAILURE;
}

int sap_get_mdio_read_func(sap_mdio_type_e type, mdio_read_func *func)
{
    if (type >= SAP_MDIO_TYPE_MAX) {
        return SAP_STATUS_FAILURE;
    }
    *func = g_mdio_read_funcs[type];
    if (*func == NULL) {
        return SAP_STATUS_FAILURE;
    }
    return SAP_STATUS_SUCCESS;
}

int sap_get_mdio_write_func(sap_mdio_type_e type, mdio_write_func *func)
{
    if (type >= SAP_MDIO_TYPE_MAX) {
        return SAP_STATUS_FAILURE;
    }
    *func = g_mdio_write_funcs[type];
    if (*func == NULL) {
        return SAP_STATUS_FAILURE;
    }
    return SAP_STATUS_SUCCESS;
}

int sap_mdio_func_reg(sap_mdio_type_e type, mdio_read_func read_func, mdio_write_func write_func)
{
    if (type >= SAP_MDIO_TYPE_MAX) {
        return SAP_STATUS_FAILURE;
    }
    if (read_func != NULL) {
        g_mdio_read_funcs[type] = read_func;
    }
    if (write_func != NULL) {
        g_mdio_write_funcs[type] = write_func;
    }
    return SAP_STATUS_SUCCESS;
}

static int dfd_utest_mdiodev_rd(int mdio_index, int phy_addr, uint32_t regaddr, uint32_t *regval)
{
    struct mdio_dev_user_info mdio_info;
    long int ret;
    int fd;
    mdio_info.mdio_index = mdio_index;
    mdio_info.phyaddr = phy_addr;
    mdio_info.regaddr = regaddr | 0x40000000;
    mdio_info.regval = 0;
    fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram_test: %s\n", strerror(errno));
        return -1;
    }
    ret = ioctl(fd, CMD_MDIO_READ, &mdio_info);
    if (ret < 0) {
        fprintf(stderr, "Error: mdio read error : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }
    close(fd);
    *regval = mdio_info.regval;
    return 0;
}

static int dfd_utest_mdiodev_wr(int mdio_index, int phy_addr, uint32_t regaddr, uint32_t regval)
{
    struct mdio_dev_user_info mdio_info;
    long int ret;
    int fd;

    mdio_info.mdio_index = mdio_index;
    mdio_info.phyaddr = phy_addr;
    mdio_info.regaddr = regaddr | 0x40000000;
    mdio_info.regval = regval;
    fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram_test: %s\n", strerror(errno));
        return -1;
    }
    ret = ioctl(fd, CMD_MDIO_WRITE, &mdio_info);
    if (ret < 0) {
        fprintf(stderr, "Error: mdio read error : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }
    close(fd);
    return 0;
}

// static int dfd_mdio_read(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int *data)
// {
//     int mdio_index;
//     int bus_id = 2;
//     mdio_index = ((phy_addr & 0xe0)>>5) + ((phy_addr & 0xf00) >> 6) + bus_id;
//     phy_addr = phy_addr & 0x1f;
//     return dfd_utest_mdiodev_rd(1, phy_addr, reg_addr, data);
// }

// static int dfd_mdio_write(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int data)
// {
//     int mdio_index;
//     int bus_id = 2;
//     mdio_index = ((phy_addr & 0xe0)>>5) + ((phy_addr & 0xf00) >> 6) + bus_id;
//     phy_addr = phy_addr & 0x1f;
//     return dfd_utest_mdiodev_wr(1, phy_addr, reg_addr, data);
// }

/* 旧版接口，准备废弃 */
static int dfd_mdio_read(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int *data)
{
    int mdio_index;
    int bus_id = 2;
    mdio_index = ((phy_addr & 0xe0)>>5) + ((phy_addr & 0xf00) >> 6) + bus_id;
    phy_addr = phy_addr & 0x1f;
    return dfd_utest_mdiodev_rd(mdio_index, phy_addr, reg_addr, data);
}

/* 旧版接口，准备废弃 */
static int dfd_mdio_write(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int data)
{
    int mdio_index;
    int bus_id = 2;
    mdio_index = ((phy_addr & 0xe0)>>5) + ((phy_addr & 0xf00) >> 6) + bus_id;
    phy_addr = phy_addr & 0x1f;
    return dfd_utest_mdiodev_wr(mdio_index, phy_addr, reg_addr, data);
}

static int dfd_mdio_read_cust(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int *data)
{
    int mdio_index, bus_id;
    bus_id = (phy_addr & 0x7c00)>>10;
    mdio_index = (phy_addr & 0x3e0)>>5;
    phy_addr = phy_addr & 0x1f;
    return dfd_utest_mdiodev_rd(mdio_index+bus_id, phy_addr, reg_addr, data);
}

static int dfd_mdio_write_cust(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int data)
{
    int mdio_index, bus_id;
    bus_id = (phy_addr & 0x7c00)>>10;
    mdio_index = (phy_addr & 0x3e0)>>5;
    phy_addr = phy_addr & 0x1f;
    return dfd_utest_mdiodev_wr(mdio_index+bus_id, phy_addr, reg_addr, data);
}

static int dfd_mdio_read_cust_ex(void *user_acc, uint8_t mdio_index, unsigned int phy_addr, unsigned int reg_addr, unsigned int *data)
{
    return dfd_utest_mdiodev_rd(mdio_index, phy_addr, reg_addr, data);
}

static int dfd_mdio_write_cust_ex(void *user_acc, uint8_t mdio_index, unsigned int phy_addr, unsigned int reg_addr, unsigned int data)
{
    return dfd_utest_mdiodev_wr(mdio_index, phy_addr, reg_addr, data);
}

int sap_mdio_init(void)
{
    if (g_mdio_read_funcs[SAP_MDIO_TYPE_SYSFS] == NULL) {
        g_mdio_read_funcs[SAP_MDIO_TYPE_SYSFS] = dfd_mdio_read;
    }
    if (g_mdio_write_funcs[SAP_MDIO_TYPE_SYSFS] == NULL) {
        g_mdio_write_funcs[SAP_MDIO_TYPE_SYSFS] = dfd_mdio_write;
    }
    if (g_mdio_read_funcs[SAP_MDIO_TYPE_SYSFS_CUST] == NULL) {
        g_mdio_read_funcs[SAP_MDIO_TYPE_SYSFS_CUST] = dfd_mdio_read_cust;
    }
    if (g_mdio_write_funcs[SAP_MDIO_TYPE_SYSFS_CUST] == NULL) {
        g_mdio_write_funcs[SAP_MDIO_TYPE_SYSFS_CUST] = dfd_mdio_write_cust;
    }
    int rv;
    rv = sap_mdio_type_reg("dev_sysfile", dfd_mdio_read_cust_ex, dfd_mdio_write_cust_ex);
    if (rv) {
        SAP_LOG_ERR("dev_sysfile mdio func reg failed.");
    }
    return SAP_STATUS_SUCCESS;
}
