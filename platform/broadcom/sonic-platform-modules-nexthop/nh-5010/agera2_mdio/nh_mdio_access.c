/*
 * Copyright (C) 2026 Nexthop Systems Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "nh_mdio_access.h"

#define MDIO_SYSFS_DIR  "/sys/class/mdio_bus"
#define MDIO_SYSFS_PATH MDIO_SYSFS_DIR "/pci-mdio-%u/mdio_access"
#define RESP_BUF_SIZE 128
#define CMD_BUF_SIZE 64

#define MDIO_MAX_BUSES 16

static int bus_fds[MDIO_MAX_BUSES] = { [0 ... MDIO_MAX_BUSES - 1] = -1 };

static int bus_open(uint16_t mdio_bus)
{
    char sysfs_path[256];

    snprintf(sysfs_path, sizeof(sysfs_path), MDIO_SYSFS_PATH, mdio_bus);
    bus_fds[mdio_bus] = open(sysfs_path, O_RDWR | O_CLOEXEC);
    if (bus_fds[mdio_bus] < 0) {
        syslog(LOG_ERR, "MDIO: failed to open %s: %s", sysfs_path, strerror(errno));
    }

    return bus_fds[mdio_bus];
}

/* Agera2 reads a register at a time, so an open per access was measurable. */
static void __attribute__((constructor)) bus_fds_open(void)
{
    struct dirent *ent;
    unsigned int bus;
    int opened = 0;
    DIR *dir;

    dir = opendir(MDIO_SYSFS_DIR);
    if (!dir) {
        syslog(LOG_ERR, "MDIO: failed to open %s: %s", MDIO_SYSFS_DIR, strerror(errno));
        return;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (sscanf(ent->d_name, "pci-mdio-%u", &bus) != 1 || bus >= MDIO_MAX_BUSES) {
            continue;
        }
        if (bus_open((uint16_t)bus) >= 0) {
            opened++;
        }
    }

    closedir(dir);

    if (opened != MDIO_MAX_BUSES) {
        syslog(LOG_ERR, "MDIO: opened %d of the %d FPGA MDIO buses",
               opened, MDIO_MAX_BUSES);
        return;
    }

    syslog(LOG_NOTICE, "MDIO: opened %d FPGA MDIO buses", opened);
}

/* Retries a bus that was not present when the library loaded. */
static int bus_fd(uint16_t mdio_bus)
{
    if (bus_fds[mdio_bus] < 0) {
        return bus_open(mdio_bus);
    }

    return bus_fds[mdio_bus];
}

nh_mdio_status_t mdio_read(uint64_t ctx, uint32_t mdio_addr,
                           uint32_t reg_addr, uint32_t number_of_registers, uint32_t *data)
{
    char cmd[CMD_BUF_SIZE];
    char buf[RESP_BUF_SIZE];
    uint16_t mdio_bus;
    uint32_t i;
    int fd, len;
    ssize_t n;

    if (!data || number_of_registers == 0) {
        syslog(LOG_ERR, "MDIO_READ: invalid parameter");
        return NH_MDIO_STATUS_INVALID_PARAMETER;
    }

    mdio_addr = (ctx >> 16) & 0xFFFF;
    mdio_bus = (uint16_t)(ctx & 0xFFFF);

    if (mdio_bus >= MDIO_MAX_BUSES) {
        syslog(LOG_ERR, "MDIO_READ: bus %u exceeds the %u supported buses",
               mdio_bus, MDIO_MAX_BUSES);
        return NH_MDIO_STATUS_INVALID_PARAMETER;
    }

    fd = bus_fd(mdio_bus);
    if (fd < 0) {
        return NH_MDIO_STATUS_FAILURE;
    }

    for (i = 0; i < number_of_registers; i++) {
        len = snprintf(cmd, sizeof(cmd), "read 0x%x 0x%x\n", mdio_addr, reg_addr + i);

        n = pwrite(fd, cmd, (size_t)len, 0);
        if (n != len) {
            syslog(LOG_ERR, "MDIO_READ: transaction rejected for reg=0x%x: %s",
                   reg_addr + i, strerror(errno));
            return NH_MDIO_STATUS_FAILURE;
        }

        n = pread(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            syslog(LOG_ERR, "MDIO_READ: failed to read response for reg=0x%x: %s",
                   reg_addr + i, strerror(errno));
            return NH_MDIO_STATUS_FAILURE;
        }
        buf[n] = '\0';

        if (sscanf(buf, "0x%x", &data[i]) != 1) {
            syslog(LOG_ERR, "MDIO_READ: failed to parse response: '%s'", buf);
            return NH_MDIO_STATUS_FAILURE;
        }
    }

    return NH_MDIO_STATUS_SUCCESS;
}

nh_mdio_status_t mdio_write(uint64_t ctx, uint32_t mdio_addr,
                            uint32_t reg_addr, uint32_t number_of_registers, const uint32_t *data)
{
    char cmd[CMD_BUF_SIZE];
    uint16_t mdio_bus;
    uint32_t i;
    int fd, len;
    ssize_t n;

    if (!data || number_of_registers == 0) {
        syslog(LOG_ERR, "MDIO_WRITE: invalid parameter");
        return NH_MDIO_STATUS_INVALID_PARAMETER;
    }

    mdio_addr = (ctx >> 16) & 0xFFFF;
    mdio_bus = (uint16_t)(ctx & 0xFFFF);

    if (mdio_bus >= MDIO_MAX_BUSES) {
        syslog(LOG_ERR, "MDIO_WRITE: bus %u exceeds the %u supported buses",
               mdio_bus, MDIO_MAX_BUSES);
        return NH_MDIO_STATUS_INVALID_PARAMETER;
    }

    fd = bus_fd(mdio_bus);
    if (fd < 0) {
        return NH_MDIO_STATUS_FAILURE;
    }

    for (i = 0; i < number_of_registers; i++) {
        len = snprintf(cmd, sizeof(cmd), "write 0x%x 0x%x 0x%x\n",
                       mdio_addr, reg_addr + i, data[i]);

        n = pwrite(fd, cmd, (size_t)len, 0);
        if (n != len) {
            syslog(LOG_ERR, "MDIO_WRITE: transaction rejected for reg=0x%x: %s",
                   reg_addr + i, strerror(errno));
            return NH_MDIO_STATUS_FAILURE;
        }
    }

    return NH_MDIO_STATUS_SUCCESS;
}
