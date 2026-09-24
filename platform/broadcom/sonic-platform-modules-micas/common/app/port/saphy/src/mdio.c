/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * MDIO read/write interface implementation.
 ********************************************************************************/
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdint.h>

#include "mdio.h"
#include "common.h"

/* ==========================================================================
 * Internal variables
 * ========================================================================== */
static mdio_read_func_t  g_mdio_read_funcs[MDIO_TYPE_MAX];
static mdio_write_func_t g_mdio_write_funcs[MDIO_TYPE_MAX];
static mdio_info_t g_mdio_info_list[MDIO_TYPE_MAX] = {0};
static uint8_t g_mdio_info_list_num = 0;

/* ==========================================================================
 * IOCTL commands
 * ========================================================================== */
struct mdio_dev_user_info {
    int mdio_index;
    int phyaddr;
    uint32_t regaddr;
    uint32_t regval;
};

#define CMD_MDIO_READ   _IOR('M', 1, struct mdio_dev_user_info)
#define CMD_MDIO_WRITE  _IOR('M', 2, struct mdio_dev_user_info)

/* ==========================================================================
 * Low-level device MDIO read/write
 * ========================================================================== */
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
        fprintf(stderr, "Error: Could not open /dev/dram_test: %s\n", strerror(errno));
        return -1;
    }
    ret = ioctl(fd, CMD_MDIO_READ, &mdio_info);
    if (ret < 0) {
        fprintf(stderr, "Error: mdio read error: %s\n", strerror(errno));
        close(fd);
        return -1;
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
        fprintf(stderr, "Error: Could not open /dev/dram_test: %s\n", strerror(errno));
        return -1;
    }
    ret = ioctl(fd, CMD_MDIO_WRITE, &mdio_info);
    if (ret < 0) {
        fprintf(stderr, "Error: mdio write error: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

/* ==========================================================================
 * Default MDIO read/write callbacks
 * ========================================================================== */
static int dfd_mdio_read(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int *data)
{
    int mdio_index;
    int bus_id = 2;
    mdio_index = ((phy_addr & 0xe0) >> 5) + ((phy_addr & 0xf00) >> 6) + bus_id;
    phy_addr = phy_addr & 0x1f;
    return dfd_utest_mdiodev_rd(mdio_index, phy_addr, reg_addr, data);
}

static int dfd_mdio_write(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int data)
{
    int mdio_index;
    int bus_id = 2;
    mdio_index = ((phy_addr & 0xe0) >> 5) + ((phy_addr & 0xf00) >> 6) + bus_id;
    phy_addr = phy_addr & 0x1f;
    return dfd_utest_mdiodev_wr(mdio_index, phy_addr, reg_addr, data);
}

static int dfd_mdio_read_cust(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int *data)
{
    int mdio_index, bus_id;
    bus_id = (phy_addr & 0x7c00) >> 10;
    mdio_index = (phy_addr & 0x3e0) >> 5;
    phy_addr = phy_addr & 0x1f;
    return dfd_utest_mdiodev_rd(mdio_index + bus_id, phy_addr, reg_addr, data);
}

static int dfd_mdio_write_cust(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int data)
{
    int mdio_index, bus_id;
    bus_id = (phy_addr & 0x7c00) >> 10;
    mdio_index = (phy_addr & 0x3e0) >> 5;
    phy_addr = phy_addr & 0x1f;
    return dfd_utest_mdiodev_wr(mdio_index + bus_id, phy_addr, reg_addr, data);
}

static int dfd_mdio_read_cust_ex(void *user_acc, uint8_t mdio_index, unsigned int phy_addr, unsigned int reg_addr, unsigned int *data)
{
    return dfd_utest_mdiodev_rd(mdio_index, phy_addr, reg_addr, data);
}

static int dfd_mdio_write_cust_ex(void *user_acc, uint8_t mdio_index, unsigned int phy_addr, unsigned int reg_addr, unsigned int data)
{
    return dfd_utest_mdiodev_wr(mdio_index, phy_addr, reg_addr, data);
}

/* Adapters matching mdio_read_func_t/mdio_write_func_t (4-arg) to the 5-arg
 * dfd *_ex callbacks. mdio_index is decoded from phy_addr the same way as
 * dfd_mdio_read_cust / dfd_mdio_write_cust. */
static int dfd_mdio_read_cust_ex_wrap(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int *data)
{
    uint8_t mdio_index = (uint8_t)(((phy_addr & 0x3e0) >> 5) + ((phy_addr & 0x7c00) >> 10));
    return dfd_mdio_read_cust_ex(user_acc, mdio_index, phy_addr & 0x1f, reg_addr, data);
}

static int dfd_mdio_write_cust_ex_wrap(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int data)
{
    uint8_t mdio_index = (uint8_t)(((phy_addr & 0x3e0) >> 5) + ((phy_addr & 0x7c00) >> 10));
    return dfd_mdio_write_cust_ex(user_acc, mdio_index, phy_addr & 0x1f, reg_addr, data);
}

/* ==========================================================================
 * Public functions
 * ========================================================================== */
int mdio_init(void)
{
    if (g_mdio_read_funcs[MDIO_TYPE_SYSFS] == NULL) {
        g_mdio_read_funcs[MDIO_TYPE_SYSFS] = dfd_mdio_read;
    }
    if (g_mdio_write_funcs[MDIO_TYPE_SYSFS] == NULL) {
        g_mdio_write_funcs[MDIO_TYPE_SYSFS] = dfd_mdio_write;
    }
    if (g_mdio_read_funcs[MDIO_TYPE_SYSFS_CUST] == NULL) {
        g_mdio_read_funcs[MDIO_TYPE_SYSFS_CUST] = dfd_mdio_read_cust;
    }
    if (g_mdio_write_funcs[MDIO_TYPE_SYSFS_CUST] == NULL) {
        g_mdio_write_funcs[MDIO_TYPE_SYSFS_CUST] = dfd_mdio_write_cust;
    }

    int rv = mdio_type_reg("dev_sysfile", dfd_mdio_read_cust_ex_wrap, dfd_mdio_write_cust_ex_wrap);
    if (rv) {
        LOG_INFO("dev_sysfile mdio func reg failed.");
    }
    return STATUS_SUCCESS;
}

int get_mdio_read_func(mdio_type_e type, mdio_read_func_t *func)
{
    if (type >= MDIO_TYPE_MAX) {
        return STATUS_FAILURE;
    }
    *func = g_mdio_read_funcs[type];
    return (*func == NULL) ? STATUS_FAILURE : STATUS_SUCCESS;
}

int get_mdio_write_func(mdio_type_e type, mdio_write_func_t *func)
{
    if (type >= MDIO_TYPE_MAX) {
        return STATUS_FAILURE;
    }
    *func = g_mdio_write_funcs[type];
    return (*func == NULL) ? STATUS_FAILURE : STATUS_SUCCESS;
}

int mdio_type_reg(char *type, mdio_read_func_t read_func, mdio_write_func_t write_func)
{
    if (g_mdio_info_list_num < MDIO_TYPE_MAX) {
        snprintf(g_mdio_info_list[g_mdio_info_list_num].type, MAX_MDIO_TYPE_STR_LEN, "%s", type);
        g_mdio_info_list[g_mdio_info_list_num].read_func = read_func;
        g_mdio_info_list[g_mdio_info_list_num].write_func = write_func;
        g_mdio_info_list_num++;
        return STATUS_SUCCESS;
    }
    return STATUS_FAILURE;
}