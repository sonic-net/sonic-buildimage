/*
 * dfd_fpga_upg.c
 *
 * FPGA upgrade related interface
 * v1.0    support <support> 2013-10-25  Initial version.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/time.h>

#include "dfd_fpga_pkt.h"
#include "dfd_fpga_debug.h"

typedef struct dfd_fpga_upg_node_s {
    int sslot;              /* Expansion card slot number */
    int extype;             /* Expansion card type */
    int fpga_ver;           /* Expansion card FPGA version number */
} dfd_fpga_upg_node_t;

/* FPGA upgrade related registers */
#define FPGA_UPG_CONTENT_BASE_REG_A00         (0xa00)
#define FPGA_UPG_CONTENT_BASE_REG_E00         (0xe00)

#define FPGA_UPG_STATUS_REG                   (0x180)
#define FPGA_UPG_SPI_CTRL_REG                 (0x184)
#define FPGA_UPG_WR_FLASH_STATUS_REG          (0x188)
#define FPGA_UPG_RD_FLASH_STATUS_REG          (0x18C)
#define FPGA_UPG_INSTRUCTION_REG              (0x190)

#define FPGA_UPG_ADDR_REG                     (0x194)
#define FPGA_UPG_LENGTH_REG                   (0x198)
#define FPGA_UPG_DEVICE_ID_REG                (0x19C)

#define FPGA_UPG_DROP_REQ_NUM_REG             (0x1A8)

#define FPGA_VER_ADDRESS                      (0x00000000)

#define FPGA_VER_MASK                         (0xFFFF)
#define FPGA_VERSION(ver)                     ((ver & FPGA_VER_MASK) >> 8)

/* define FPGA upgrade related instructions */
#define FPGA_UPG_INSTRUTION_SE                (0xD8)
#define FPGA_UPG_INSTRUTION_RDID              (0x9F)
#define FPGA_UPG_INSTRUTION_WRSR              (0x01)
#define FPGA_UPG_INSTRUTION_RDSR              (0x05)
#define FPGA_UPG_INSTRUTION_WREN              (0x06)
#define FPGA_UPG_INSTRUTION_WRDI              (0x04)
#define FPGA_UPG_INSTRUTION_BE                (0xC7)
#define FPGA_UPG_INSTRUTION_PP                (0x02)
#define FPGA_UPG_INSTRUTION_FR                (0x0B)
#define FPGA_UPG_INSTRUTION_P4E               (0x20)

#define FPGA_UPG_CONTENT_LENGTH               (256)

#define FPGA_UPG_STATUS_MASK                  (0x1)
#define FPGA_UPG_ACCESS_ENABLE                (0x3)
#define FPGA_UPG_STATUS_RESET                 (0x0)

#define FPGA_UPG_SPI_STATUS_MASK              (0x1)

#define FPGA_UPG_RETRY_SLEEP_TIME             (10)   /* 10us */
#define FPGA_UPG_RETRY_CNT                    (1000)

#define FPGA_UPG_PKT_RETRY_CNT                (100)

#define DFD_FPGA_VERSION_REG                  (0x10D0)

#define DFD_FPGA_UPGRADE_BUFF_SIZE            (256)
#define DFD_FPGA_UPGADE_RETRY_CNT             (10)
#define DFD_FPGA_UPGRADE_CMD_BUFF_SIZE        (100)
#define DFD_FPGA_UPGRADE_MAX_NODE             (16)

#define DFD_FPGA_UPDATE_BOOT_ADDR             (0x1A0000)    /* UPDATE area start address */
#define DFD_FPGA_UPDATE_BOOT_ADDR_FIX         (0x2F0000)
#define DFD_FPGA_SPI_SECTOR_SIZE              (0x10000)     /* One sector is 64k */
#define DFD_FPGA_UPDATE_FLASH_SIZE            (0x4000000)
#define DFD_FPGA_BASE_TEST_ADD                (DFD_FPGA_UPDATE_FLASH_SIZE - DFD_FPGA_SPI_SECTOR_SIZE)

#define DFD_FPGA_CRITICAL_SWITCH_PAGE_ADDR    (0xF00)    /* CRITICAL SWITCH page address */
#define DFD_FPGA_CRITICAL_SWITCH_PAGE_OFFSET  (0xFC)     /* CRITICAL SWITCH in-page offset address */
#define DFD_FPGA_CRITICAL_SWITCH_WORD         (0xAA995566)  /* CRITICAL SWITCH value */

#define DFD_FPGA_ERASE_P4E_SIZE               (0x1000)/* erase p4e,4k at a time*/

#define FPGA_UPG_WAIT_SPI_RETRY_CNT           (1000)
#define FPGA_UPG_WAIT_SPI_RETRY_SLEEP_TIME    (1000 * 10)   /* 10ms */
#define FPGA_RETRY_TIMES                      (3)

static dfd_pci_dev_priv_t  default_pci_priv = {
    .pcibus = 8,
    .slot = 0,
    .fn = 0,
    .bar = 0,
    .align = 4,
};

static dfd_pci_dev_priv_t *current_pci_priv = NULL;

static void dfd_utest_printf_reg(uint8_t *buf, int buf_len)
{
    int i;

    for (i = 0; i < buf_len; i++) {
        if ((i % 16) == 0) {
            printf("\n");
        }
        printf("%02x ", buf[i]);
    }

    printf("\n");
    return;
}

static int dfd_fpga_upg_write_word(dfd_pci_dev_priv_t *pci_priv, int addr, int val)
{
    int ret;
    int i;
    int cnt;

    i = 0;
    cnt = FPGA_UPG_PKT_RETRY_CNT;
    while(i < cnt) {
        ret = dfd_fpga_write_word(pci_priv, addr, val);
        if (ret) {
            i++;
            DFD_VERBOS("dfd_fpga_write_word addr 0x%x val 0x%x i %d failed ret %d.\n", addr, val, i,
                ret);
            continue;
        } else {
            DFD_VERBOS("dfd_fpga_write_word addr 0x%x val 0x%x success.\n", addr, val);
            return 0;
        }
    }

    DFD_VERBOS("dfd_fpga_upg_write_word addr 0x%x val 0x%x i %d failed ret %d.\n", addr, val, i, ret);
    return -1;
}

static int dfd_fpga_upg_read_word(dfd_pci_dev_priv_t *pci_priv, int addr, int *val)
{
    int ret;
    int i;
    int cnt;

    i = 0;
    cnt = FPGA_UPG_PKT_RETRY_CNT;
    while(i < cnt) {
        ret = dfd_fpga_read_word(pci_priv, addr, val);
        if (ret) {
            i++;
            DFD_VERBOS("dfd_fpga_read_word addr 0x%x i %d failed ret %d.\n", addr, i,
                ret);
            continue;
        } else {
            DFD_VERBOS("dfd_fpga_read_word addr 0x%x val 0x%x success.\n", addr, *val);
            return 0;
        }
    }

    DFD_VERBOS("dfd_fpga_read_word addr 0x%x i %d failed ret %d.\n", addr, i, ret);
    return -1;
}

static int dfd_fpga_upg_buf_write(dfd_pci_dev_priv_t *pci_priv, int addr, uint8_t *buf, int wr_len)
{
    int ret;
    int i;
    int cnt;

    i = 0;
    cnt = FPGA_UPG_PKT_RETRY_CNT;
    while(i < cnt) {
        ret = dfd_fpga_pci_write(pci_priv, addr, buf, wr_len);
        if (ret) {
            i++;
            DFD_VERBOS("dfd_fpga_buf_write addr 0x%x wr_len %d i %d failed ret %d.\n", addr, wr_len, i,
                ret);
            continue;
        } else {
            DFD_VERBOS("dfd_fpga_buf_write addr 0x%x wr_len %d success.\n", addr, wr_len);
            return 0;
        }
    }

    DFD_VERBOS("dfd_fpga_buf_write addr 0x%x wr_len %d i %d failed ret %d.\n", addr, wr_len, i, ret);
    return -1;
}

static int dfd_fpga_upg_buf_read(dfd_pci_dev_priv_t *pci_priv, int addr, uint8_t *buf, int rd_len)
{
    int ret;
    int i;
    int cnt;

    i = 0;
    cnt = FPGA_UPG_PKT_RETRY_CNT;
    while(i < cnt) {
        ret = dfd_fpga_pci_read(pci_priv, addr, buf, rd_len);
        if (ret) {
            i++;
            DFD_VERBOS("dfd_fpga_buf_read addr 0x%x rd_len %d i %d failed ret %d.\n", addr, rd_len, i,
                ret);
            continue;
        } else {
            DFD_VERBOS("dfd_fpga_buf_read addr 0x%x rd_len %d success.\n", addr, rd_len);
            return 0;
        }
    }

    DFD_VERBOS("dfd_fpga_buf_read addr 0x%x rd_len %d i %d failed ret %d.\n", addr, rd_len, i, ret);
    return -1;
}

/* Configuring boot Access */
static int dfd_fpga_upg_set_access(dfd_pci_dev_priv_t *pci_priv, int cmd)
{
    int ret;
    int val;
    int addr;

    addr = pci_priv->fpga_upg_base + FPGA_UPG_INSTRUCTION_REG;
    val = cmd;
    ret = dfd_fpga_upg_write_word(pci_priv, addr, val);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -1;
    }

    addr = pci_priv->fpga_upg_base + FPGA_UPG_SPI_CTRL_REG;
    val = FPGA_UPG_ACCESS_ENABLE;
    ret = dfd_fpga_upg_write_word(pci_priv, addr, val);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -1;
    }

    DFD_VERBOS("Success: cmd %d.\n", cmd);
    return 0;
}

static int dfd_fpga_upg_reset(dfd_pci_dev_priv_t *pci_priv)
{
    int ret;
    int val;
    int addr;

    addr = pci_priv->fpga_upg_base + FPGA_UPG_SPI_CTRL_REG;
    val = FPGA_UPG_STATUS_RESET;
    ret = dfd_fpga_upg_write_word(pci_priv, addr, val);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -1;
    }

    DFD_VERBOS("Success: reset %d.\n", val);
    return 0;
}

/* Whether the SPI port is idle. 0 indicates idle, and 1 indicates busy */
static int dfd_fpga_upg_get_status(dfd_pci_dev_priv_t *pci_priv, char *status)
{
    int ret;
    int val;
    int addr;

    addr = pci_priv->fpga_upg_base + FPGA_UPG_STATUS_REG;
    ret = dfd_fpga_upg_read_word(pci_priv, addr, &val);
    DFD_VERBOS("dfd_fpga_upg_read_word return.\n");
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_read_word addr 0x%x failed ret %d.\n", addr, ret);
        return -1;
    }

    *status = val & FPGA_UPG_STATUS_MASK;
    DFD_VERBOS("Success: val %d status %d.\n", val, *status);
    return 0;
}

/* Wait for the SPI port to become idle */
static int dfd_fpga_upg_wait_ready(dfd_pci_dev_priv_t *pci_priv)
{
    int timeout;
    char status;
    int ret;

    timeout = FPGA_UPG_RETRY_CNT;
    while (timeout--) {
        DFD_VERBOS("timeout %d.\n", timeout);
        ret = dfd_fpga_upg_get_status(pci_priv, &status);
        if (ret) {
            DFD_ERROR("dfd_fpga_upg_get_status failed ret %d.\n", ret);
            continue;
        }

        DFD_VERBOS("timeout %d status %d.\n", timeout, status);
        /* Determine whether to be idle */
        if (!status) {
            DFD_VERBOS("FPGA SPI READY.\n");
            return 0;
        }
        usleep(FPGA_UPG_RETRY_SLEEP_TIME);
    }

    return -2;
}

/* Configure the FPGA upgrade write function */
static int dfd_fpga_upg_set_wr_enable(dfd_pci_dev_priv_t *pci_priv)
{
    int ret;
    int cmd;

    cmd = FPGA_UPG_INSTRUTION_WREN;
    ret = dfd_fpga_upg_set_access(pci_priv, cmd);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -1;
    }

    DFD_VERBOS("Success.\n");
    return 0;
}

/* get SPI's STATUS register */
static int dfd_fpga_upg_get_spi_status(dfd_pci_dev_priv_t *pci_priv, char *status)
{
    int ret;
    int val;
    int addr;
    int cmd;

    cmd = FPGA_UPG_INSTRUTION_RDSR;
    ret = dfd_fpga_upg_set_access(pci_priv, cmd);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -1;
    }

    addr = pci_priv->fpga_upg_base + FPGA_UPG_RD_FLASH_STATUS_REG;
    ret = dfd_fpga_upg_read_word(pci_priv, addr, &val);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_read_word addr 0x%x failed ret %d.\n", addr, ret);
        return -1;
    }

    ret = dfd_fpga_upg_reset(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_reset failed ret %d.\n", ret);
        return -1;
    }

    *status = val & FPGA_UPG_SPI_STATUS_MASK;
    DFD_VERBOS("Success: val %d spi_status %d.\n", val, *status);
    return 0;
}

/* waiting for SPI chip opreation to complete */
static int dfd_fpga_wait_spi_ready(dfd_pci_dev_priv_t *pci_priv)
{
    int timeout;
    char status;
    int ret;

    timeout = FPGA_UPG_WAIT_SPI_RETRY_CNT;
    while (timeout--) {
        DFD_VERBOS("timeout %d.\n", timeout);
        ret = dfd_fpga_upg_get_spi_status(pci_priv, &status);
        if (ret) {
            DFD_ERROR("dfd_fpga_upg_get_spi_status failed ret %d.\n", ret);
            continue;
        }
        DFD_VERBOS("timeout %d status %d.\n", timeout, status);
        /* assert whether it is free */
        if (!status) {
            DFD_VERBOS("SPI CHIP READY.\n");
            return 0;
        }
        usleep(FPGA_UPG_RETRY_SLEEP_TIME);
    }

    return -2;
}

/* Erase the entire chip */
static int dfd_fpga_upg_set_erase_all(dfd_pci_dev_priv_t *pci_priv)
{
    int ret;
    int cmd;

    /* waiting for FPGA's SPI port to become free */
    ret = dfd_fpga_upg_wait_ready(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_wait_ready failed ret %d.\n", ret);
        return -1;
    }

    /* configure write enable */
    ret = dfd_fpga_upg_set_wr_enable(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_wr_enable failed ret %d.\n", ret);
        return -1;
    }

    cmd = FPGA_UPG_INSTRUTION_BE;
    ret = dfd_fpga_upg_set_access(pci_priv, cmd);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -1;
    }

    /* Hardware requirements, delay 1s */
    sleep(1);

    /* Waiting for the SPI chip operation to complete */
    ret = dfd_fpga_wait_spi_ready(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_wait_ready sslot %d unit %d failed ret %d.\n", ret);
        return -1;
    }

    ret = dfd_fpga_upg_reset(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_reset failed ret %d.\n", ret);
        return -1;
    }

    DFD_VERBOS("Success.\n");
    return 0;
}

/* Erase sector (256 pages, 64k in total) */
int dfd_fpga_upg_set_erase_sector(dfd_pci_dev_priv_t *pci_priv, int spi_addr)
{
    int ret;
    int cmd, val, addr;

    DFD_VERBOS("Enter spi_addr 0x%x.\n", spi_addr);

    /* waiting for FPGA's SPI port to become free*/
    ret = dfd_fpga_upg_wait_ready(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_wait_ready failed ret %d.\n", ret);
        return -1;
    }

    /* Start write enable */
    ret = dfd_fpga_upg_set_wr_enable(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_wr_enable failed ret %d.\n", ret);
        return -1;
    }

    /* Write erase address */
    val = spi_addr;
    addr = pci_priv->fpga_upg_base + FPGA_UPG_ADDR_REG;
    ret = dfd_fpga_upg_write_word(pci_priv, addr, val);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -1;
    }

    /* Start sector erase? */
    cmd = FPGA_UPG_INSTRUTION_SE;
    ret = dfd_fpga_upg_set_access(pci_priv, cmd);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -1;
    }

    /* hardware requirment ,delay 500ms */
    usleep(500 * 1000);

    /* waiting for SPI chip operation to complete */
    ret = dfd_fpga_wait_spi_ready(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_wait_ready failed ret %d.\n", ret);
        return -1;
    }

    ret = dfd_fpga_upg_reset(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_reset failed ret %d.\n", ret);
        return -1;
    }

    DFD_VERBOS("Success.\n");

    return 0;
}

/* eara 4k area */
int dfd_fpga_upg_set_erase_p4e(dfd_pci_dev_priv_t *pci_priv, int spi_addr)
{
    int ret;
    int cmd, val, addr;

    DFD_VERBOS("Enter spi_addr 0x%x.\n", spi_addr);

    /* waiting for FPGA's SPI port to become free */
    ret = dfd_fpga_upg_wait_ready(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_wait_ready failed ret %d.\n", ret);
        return -1;
    }

    /* start write enable */
    ret = dfd_fpga_upg_set_wr_enable(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_wr_enable failed ret %d.\n", ret);
        return -1;
    }

    /* write erase address */
    val = spi_addr;
    addr = pci_priv->fpga_upg_base + FPGA_UPG_ADDR_REG;
    ret = dfd_fpga_upg_write_word(pci_priv, addr, val);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -1;
    }

    /* start 4k erase */
    cmd = FPGA_UPG_INSTRUTION_P4E;
    ret = dfd_fpga_upg_set_access(pci_priv, cmd);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -1;
    }

    /* hardware requirment,delay 200ms */
    usleep(200 * 1000);

    /* waiting for SPI chip operation to complete */
    ret = dfd_fpga_wait_spi_ready(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_wait_ready failed ret %d.\n", ret);
        return -1;
    }

    ret = dfd_fpga_upg_reset(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_reset failed ret %d.\n", ret);
        return -1;
    }

    DFD_VERBOS("Success.\n");

    return 0;
}

static int dfd_fpga_upg_program(dfd_pci_dev_priv_t *pci_priv, int spi_addr, char *buf, int len)
{
    int ret;
    int addr;
    int val;
    int cmd;
    int step;
    int wr_len;

    /* Write data to upgrade content register */
    step = 1;
    #if 0
    /* FPGA temporarily only supports 4 bytes of read and write */
    for (i = 0; i < len; i += 4) {
        addr = pci_priv->fpga_upg_base + i;
        wr_len = ((i + 4) <= len) ? (4) : (len - i);
        DFD_VERBOS("dfd_fpga_buf_write sslot %d unit %d i %d addr 0x%x wr_len %d.\n",
            dst->sslot, dst->unit, i, addr, wr_len);
        ret = dfd_fpga_upg_buf_write(dst, addr, (uint8_t*)&buf[i], wr_len);
        if (ret) {
            DFD_ERROR("dfd_fpga_upg_buf_write addr 0x%x wr_len %d failed ret %d.\n", addr, len, ret);
            return -1;
        }
    }
    #else
    addr = pci_priv->fpga_upg_base;
    wr_len = len;
    DFD_VERBOS("dfd_fpga_buf_write addr 0x%x wr_len %d.\n", addr, wr_len);
    ret = dfd_fpga_upg_buf_write(pci_priv, addr, (uint8_t*)buf, wr_len);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_buf_write addr 0x%x wr_len %d failed ret %d.\n", addr, len, ret);
        return -1;
    }
    #endif

    /* Write length register */
    step++;
    val = FPGA_UPG_CONTENT_LENGTH;/* Fpga is always written in 256 length */
    addr = pci_priv->fpga_upg_base + FPGA_UPG_LENGTH_REG;
    ret = dfd_fpga_upg_write_word(pci_priv, addr, val);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -step;
    }

    /* write address register */
    step++;
    val = spi_addr;
    addr = pci_priv->fpga_upg_base + FPGA_UPG_ADDR_REG;
    ret = dfd_fpga_upg_write_word(pci_priv, addr, val);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -step;
    }

    /* Start writing upgrade data to SPI */
    step++;
    cmd = FPGA_UPG_INSTRUTION_PP;
    ret = dfd_fpga_upg_set_access(pci_priv, cmd);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -step;
    }

    /* Wait for the SPI port of FPGA to recover from idle */
    step++;
    ret = dfd_fpga_upg_wait_ready(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_wait_ready failed ret %d.\n", ret);
        return -step;
    }

    step++;
    ret = dfd_fpga_upg_reset(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_reset failed ret %d.\n", ret);
        return -step;
    }

    return 0;
}

/**
 * dfd_fpga_upg_write -write operation interface provided to upgrade module
 * @dst:  the data structure of sslot and unit corresponding to the target chip to be upgraded
 * @addr: write address (must ensure 256-byte alignment)
 * @buf:  write address buffer
 * @len:  Write length (when upgrading, 256 bytes of data must be written every time, only the last set of data can be less than 256)
 * return: return 0 if success,else return failure
 *
 */
int dfd_fpga_upg_write(dfd_pci_dev_priv_t *pci_priv, int addr, char *buf, int len)
{
    int ret;
    int step;

    /* address must be 256-byte alignment */
    step = 1;
    if ((pci_priv == NULL) || (buf == NULL) || (addr & 0xff) || (len > 256)) {
        DFD_ERROR("Input para invalid pci_priv %p buf %p addr 0x%x len %d.\n", pci_priv, buf, addr, len);
        return -step;
    }

    DFD_VERBOS("Enter: addr 0x%x len %d.\n", addr, len);

    /* waiting for FPGA's SPI port to become free */
    step++;
    ret = dfd_fpga_upg_wait_ready(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_wait_ready failed ret %d.\n", ret);
        return -step;
    }

    /* configure write enable */
    step++;
    ret = dfd_fpga_upg_set_wr_enable(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_wr_enable failed ret %d.\n", ret);
        return -step;
    }

    /* write upgrade data */
    step++;
    ret = dfd_fpga_upg_program(pci_priv, addr, buf, len);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_program addr 0x%x len %d failed ret %d.\n", addr, len, ret);
        return -step;
    }

    DFD_VERBOS("Success: addr 0x%x len %d.\n", addr, len);
    return 0;
}

static int dfd_fpga_upg_fast_read(dfd_pci_dev_priv_t *pci_priv, int spi_addr, char *buf, int len)
{
    int ret;
    uint32_t val;
    int addr;
    int cmd;
    int step;

    step = 0;

    /* clear register value */
    step++;
    addr = pci_priv->fpga_upg_base;
    ret = dfd_fpga_upg_buf_write(pci_priv, addr, buf, len);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_buf_write addr 0x%x len %d failed ret %d.\n", addr, len, ret);
        return -step;
    }
    /* write length register */
    step++;
    val = FPGA_UPG_CONTENT_LENGTH;
    addr = pci_priv->fpga_upg_base + FPGA_UPG_LENGTH_REG;
    ret = dfd_fpga_upg_write_word(pci_priv, addr, val);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -step;
    }

    /* write address register */
    step++;
    val = spi_addr;
    addr = pci_priv->fpga_upg_base + FPGA_UPG_ADDR_REG;
    ret = dfd_fpga_upg_write_word(pci_priv, addr, val);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -step;
    }

    /* start reading SPI data */
    step++;
    cmd = FPGA_UPG_INSTRUTION_FR;
    ret = dfd_fpga_upg_set_access(pci_priv, cmd);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -step;
    }

    /* waiting for FPGA's SPI port to become free */
    step++;
    ret = dfd_fpga_upg_wait_ready(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_wait_ready failed ret %d.\n", ret);
        return -step;
    }

    /* Read upgrade content register to buffer */
    step++;

    /* FPGA temporarily only supports 4 bytes of read and write */
    #if 1
    addr = pci_priv->fpga_upg_base;
    ret = dfd_fpga_upg_buf_read(pci_priv, addr, (uint8_t*)buf, len);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_buf_read addr 0x%x len %d failed ret %d.\n", addr, len, ret);
        return -step;
    }

    step++;
    ret = dfd_fpga_upg_reset(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_reset failed ret %d.\n", ret);
        return -step;
    }

    #else
    for (i = 0; i < len; i += 4) {
        addr = pci_priv->fpga_upg_base + i;
        rd_len = ((i + 4) <= len) ? (4) : (len - i);
        DFD_VERBOS("dfd_fpga_upg_buf_read sslot %d unit %d i %d addr 0x%x rd_len %d.\n",
            dst->sslot, dst->unit, i, addr, rd_len);
        ret = dfd_fpga_upg_buf_read(dst, addr, (uint8_t*)(&buf[i]), rd_len);
        if (ret) {
            DFD_ERROR("dfd_fpga_upg_buf_read addr 0x%x rd_len %d failed ret %d.\n", addr, rd_len, ret);
            return -step;
        }

        for (j = 0; j < rd_len; j++) {
            DFD_VERBOS("buf[%d]: 0x%x.\n", i, buf[i]);
        }
    }
    #endif

    return 0;
}

/**
 * dfd_fpga_upg_read -read operation interface provided to upgrade module
 * @dst:  the data structure of sslot and unit corresponding to the target chip
 * @addr: read address (must ensure 256-byte alignment)
 * @buf:  buffer for reading data
 * @len:  read length (the data read each time must be 256 bytes, only the last time can be less than 256 bytes)
 * return: return 0 if success,else return failure
 *
 */
int dfd_fpga_upg_read(dfd_pci_dev_priv_t *pci_priv, int addr, char *buf, int len)
{
    int ret;
    int step;

    /* address must be 256-byte alignment */
    step = 1;
    if ((pci_priv == NULL) || (buf == NULL) || (addr & 0xff) || (len > 256)) {
        DFD_ERROR("Input para invalid pci_priv %p buf %p addr 0x%x len %d.\n", pci_priv, buf, addr, len);
        return -step;
    }

    DFD_VERBOS("Enter: addr 0x%x len %d.\n", addr, len);

    /* waiting for FPGA's SPI port to become free */
    step++;
    ret = dfd_fpga_upg_wait_ready(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_wait_ready failed ret %d.\n", ret);
        return -step;
    }

    /* configure write enable */
    step++;
    ret = dfd_fpga_upg_set_wr_enable(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_wr_enable failed ret %d.\n", ret);
        return -step;
    }

    /* read upgrade data */
    step++;
    ret = dfd_fpga_upg_fast_read(pci_priv, addr, buf, len);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_fast_read addr 0x%x len %d failed ret %d.\n", addr, len, ret);
        return -step;
    }

    DFD_VERBOS("Success: addr 0x%x len %d.\n", addr, len);
    return 0;

}

/**
 * dfd_fpga_upg_hw_init -upgrade initialization interface provided to the upgrade module (call before starting the upgrade)
 * @dst:   the data structure of sslot and unit corresponding to the target chip to be upgraded
 * return: return 0 if success,else return failure
 * Before the interface returns, it will actively delay 1s (required by FPGA)
 */
int dfd_fpga_upg_hw_init(dfd_pci_dev_priv_t *pci_priv)
{
    int ret;

    ret = dfd_fpga_upg_set_erase_all(pci_priv);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_set_wr_enable failed ret %d.\n", ret);
        return -2;
    }

    DFD_VERBOS("Success.\n");
    return 0;
}

static int dfd_fpga_upgrade_get_fpga_version(dfd_pci_dev_priv_t *pci_priv, int *ver)
{
    int addr;
    int ret;
    int val;

    addr = DFD_FPGA_VERSION_REG;
    ret = dfd_fpga_upg_read_word(pci_priv, addr, &val);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_read_word addr 0x%x failed ret %d.\n", addr, ret);
        return -1;
    }

    *ver = val;
    DFD_VERBOS("ver 0x%x.\n", *ver);
    return 0;
}

unsigned long dfd_fpga_upg_get_file_size(const char *path)
{
    unsigned long filesize;
    struct stat statbuff;

    if(stat(path, &statbuff) < 0){
        filesize = -1;
    } else{
        filesize = statbuff.st_size;
    }

    DFD_VERBOS("file %s size is %lu.\n", path, filesize);
    return filesize;
}

static int dfd_fpga_address_init(void)
{
    int fw_version;
    int ret;

    ret = dfd_fpga_upg_read_word(current_pci_priv, FPGA_VER_ADDRESS, &fw_version);
    switch (FPGA_VERSION(fw_version)) {
    case FPGA_VER_00:
    case FPGA_VER_02:
    case FPGA_VER_03:
    case FPGA_VER_06:
        current_pci_priv->fpga_upg_base = FPGA_UPG_CONTENT_BASE_REG_A00;
        break;
    case FPGA_VER_05:
        current_pci_priv->fpga_upg_base = FPGA_UPG_CONTENT_BASE_REG_E00;
        break;
    default:
        current_pci_priv->fpga_upg_base = FPGA_UPG_CONTENT_BASE_REG_A00;
        break;
    }
    return ret;
}

static int dfd_fpga_device_init(void)
{
    current_pci_priv = &default_pci_priv;
    if (drv_get_my_dev_type() == 0x4075){
        current_pci_priv->pcibus = 1;  /* ATS48's pcie channel is 1 */
    }

    return dfd_fpga_address_init();
}

/**
 * dfd_fpga_pkt_init -DFD FPGA driver library initialization interface (if you need to use DFD FPGA driver, you must initialize first)
 * return: return 0 if success,else return failure
 */

int dfd_fpga_upg_init(void)
{
    int ret;
    static int flag;

    if (flag) {
        DFD_VERBOS("Already init.\n");
        return 0;
    }

    /* debug initialization */
    dfd_fpga_debug_init();

    ret = dfd_fpga_device_init();
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_init failed ret %d.\n", ret);
        return ret;
    }

    flag = 1;
    return 0;
}

int dfd_fpga_erase64_sector(dfd_pci_dev_priv_t *pci_priv, int offset)
{
    int ret;
    ret = -1;

    if ((offset % DFD_FPGA_SPI_SECTOR_SIZE) == 0) {
        DFD_VERBOS("erase 64k area, offset 0x%x.\n", offset);
        ret = dfd_fpga_upg_set_erase_sector(pci_priv, offset);
        if (ret) {
            DFD_ERROR("dfd_fpga_upg_set_erase_sector offset 0x%x failed ret %d.\n", offset, ret);
            return ret;
        }
    }
    DFD_ERROR("Input para invalid, offset 0x%x.\n", offset);
    return ret;
}

int dfd_fpga_upgrade_test(void)
{
    int ret, i, j, offset, num, len, res, retry;
    char wbuf[DFD_FPGA_SPI_SECTOR_SIZE];
    char rbuf[DFD_FPGA_UPGRADE_BUFF_SIZE];

    offset = DFD_FPGA_BASE_TEST_ADD;
    len = DFD_FPGA_UPGRADE_BUFF_SIZE;
    dfd_pci_dev_priv_t *pci_priv;
    dfd_fpga_upg_init();

    if (current_pci_priv == NULL) {
        printf("fpga test input para invalid pci_priv %p.\n", current_pci_priv);
        return -DFD_RV_INIT_ERR;
    }
    pci_priv = current_pci_priv;

    mem_clear(wbuf, DFD_FPGA_SPI_SECTOR_SIZE);
    /* get random data */
    for (j = 0; j < DFD_FPGA_SPI_SECTOR_SIZE; j++) {
        num = rand() % 256;
        wbuf[j] = num & 0xff;
    }
    ret = dfd_fpga_erase64_sector(pci_priv, offset);
    if (ret) {
        goto exit;
    }

    for (i = 0; i < DFD_FPGA_UPGRADE_BUFF_SIZE; i++) {
        mem_clear(rbuf, DFD_FPGA_UPGRADE_BUFF_SIZE);
        /* write data first */
        ret = dfd_fpga_upg_write(pci_priv, offset, &wbuf[i * DFD_FPGA_UPGRADE_BUFF_SIZE], len);
        if (ret) {
            DFD_ERROR("fpga upg write offset 0x%x len %d failed ret %d.\n", offset, len, ret);
            ret = -DFD_RV_DEV_FAIL;
            goto exit;
        } else {
            DFD_VERBOS("page %d upg write offset 0x%x len %d success.\n", i, offset, len);
        }

        /* go back to read the data*/
        for (retry = 0; retry < FPGA_RETRY_TIMES; retry++) { /*retry 3 times*/
            ret = dfd_fpga_upg_read(pci_priv, offset, rbuf, len);
            res = memcmp(rbuf, &wbuf[i * DFD_FPGA_UPGRADE_BUFF_SIZE], len);
            if (ret || res) {
                usleep(1000);
                continue;
            }
            break;
        }
        if (ret) {
            DFD_ERROR("fpga upg read offset 0x%x len %d failed ret %d.\n", offset, len, ret);
            ret = -DFD_RV_DEV_FAIL;
            goto exit;
        } else {
            DFD_VERBOS("page %d upg read offset 0x%x len %d success.\n", i, offset, len);
        }

        if (res) {
            DFD_ERROR("rbuf wbuf not equal, len %d, check failed.\n", len);
            DFD_ERROR("wbuf: \n");
            dfd_utest_printf_reg((uint8_t*)wbuf, len);
            DFD_ERROR("rbuf: \n");
            dfd_utest_printf_reg((uint8_t*)rbuf, len);
            ret = -DFD_RV_DEV_FAIL;
            goto exit;
        }

        offset += len;
    }

    offset = DFD_FPGA_BASE_TEST_ADD;
    ret = dfd_fpga_erase64_sector(pci_priv, offset);
    if (ret) {
        goto exit;
    }
exit:
    return ret;
}

static int dfd_fpga_upgrade_do_upgrade_onetime(dfd_pci_dev_priv_t *pci_priv, int fd, unsigned long filesize)
{
    int ret, res;
    int i, len, read_len, retry;
    char wbuf[DFD_FPGA_UPGRADE_BUFF_SIZE];
    char rbuf[DFD_FPGA_UPGRADE_BUFF_SIZE];
    int offset;

    if (current_pci_priv == NULL) {
        DFD_ERROR("Input para invalid pci_priv %p.\n", pci_priv);
        return -DFD_RV_INDEX_INVALID;
    }

#if 0
    /* handle before upgrading */
    ret = dfd_fpga_upg_do_pre(dst, filesize);
    if (ret) {
        DFD_ERROR("fpga hw init failed ret %d.\n", ret);
        ret = -DFD_RV_DEV_FAIL;
        goto exit;
    }
#endif

    i = 0;
    offset = DFD_FPGA_UPDATE_BOOT_ADDR;
    ret = lseek(fd, 0, SEEK_SET);
    if (ret == -1) {
        DFD_ERROR("seek file failed  ret %d.\n", ret);
        ret = -DFD_RV_DEV_FAIL;
        goto exit;
    }
    while(1) {
        len = DFD_FPGA_UPGRADE_BUFF_SIZE;
#if 0
        if ((offset % DFD_FPGA_ERASE_P4E_SIZE) == 0) {
            DFD_VERBOS("erase 4k area, offset 0x%x.\n", offset);
            ret = dfd_fpga_upg_set_erase_p4e(dst, offset);
            if (ret) {
                DFD_ERROR("dfd_fpga_upg_set_erase_p4e offset 0x%x failed ret %d.\n", offset, ret);
                goto exit;
            }
        }
#else
        if ((offset % DFD_FPGA_SPI_SECTOR_SIZE) == 0) {
            DFD_VERBOS("erase 64k area, offset 0x%x.\n", offset);
            ret = dfd_fpga_upg_set_erase_sector(pci_priv, offset);
            if (ret) {
                DFD_ERROR("dfd_fpga_upg_set_erase_sector offset 0x%x failed ret %d.\n", offset, ret);
                goto exit;
            }
        }
#endif

        mem_clear(wbuf, DFD_FPGA_UPGRADE_BUFF_SIZE);
        read_len = read(fd, wbuf, len);
        i++;
        if ((read_len > 0) && (read_len <= len)) {
            /* write data first */
            ret = dfd_fpga_upg_write(pci_priv, offset, wbuf, read_len);
            if (ret) {
                DFD_ERROR("fpga upg write offset 0x%x len %d failed ret %d.\n", offset, read_len, ret);
                ret = -DFD_RV_DEV_FAIL;
                goto exit;
            } else {
                DFD_VERBOS("page %d upg write offset 0x%x len %d success.\n", i, offset, read_len);
            }

            /* go back to read data */
            for (retry = 0; retry < FPGA_RETRY_TIMES; retry++) { /*retry 3 times*/
                mem_clear(rbuf, DFD_FPGA_UPGRADE_BUFF_SIZE);
                ret = dfd_fpga_upg_read(pci_priv, offset, rbuf, read_len);
                res = memcmp(rbuf, wbuf, read_len);
                if (ret || res) {
                    usleep(1000);
                    continue;
                }
                break;
            }
            if (ret) {
                DFD_ERROR("fpga upg read offset 0x%x len %d failed ret %d.\n", offset, read_len, ret);
                ret = -DFD_RV_DEV_FAIL;
                goto exit;
            } else {
                DFD_VERBOS("page %d upg read offset 0x%x len %d success.\n", i, offset, read_len);
            }

            if (res) {
                DFD_ERROR("rbuf wbuf not equal, read_len %d, check failed.\n", read_len);
                DFD_ERROR("wbuf: \n");
                dfd_utest_printf_reg((uint8_t*)wbuf, read_len);
                DFD_ERROR("rbuf: \n");
                dfd_utest_printf_reg((uint8_t*)rbuf, read_len);
                ret = -DFD_RV_DEV_FAIL;
                goto exit;
            }

            DFD_VERBOS("page %d upg check offset 0x%x len %d success.\n", i, offset, read_len);
            offset += read_len;
            if (read_len != len) {
                DFD_VERBOS("page %d read_len %d len %d, last page exit.\n", i, read_len, len);
                break;
            }
        } else if (read_len == 0) {
            DFD_VERBOS("read_len %d exit.\n", read_len);
            break;
        } else {
            DFD_ERROR("len %d read_len %d, read failed, errno(%s).\n", len, read_len, strerror(errno));
            ret = -DFD_RV_DEV_FAIL;
            goto exit;
        }
    }

#if 0
    ret = dfd_fpga_upg_do_post(dst);
    if (ret) {
        DFD_ERROR("dfd_fpga_upg_do_post dst->sslot %d dst->unit %d failed ret %d.\n", dst->sslot,
            dst->unit, ret);
        ret = -DFD_RV_DEV_FAIL;
        goto exit;
    }
#endif

    ret = 0;
    DFD_VERBOS("Update success.\n");
exit:
    return ret;
}

/* external FPGA upgrade interface */
int dfd_fpga_upgrade_do_upgrade(char* upg_file)
{
    int ret;
    int i;
    int cnt;
    unsigned long filesize;
    int fd;

    DFD_VERBOS("Enter.\n");

    if (upg_file == NULL) {
        DFD_ERROR("Input para invalid upg_file %p.\n", upg_file);
        return -DFD_RV_INDEX_INVALID;
    }

    dfd_fpga_upg_init();

    filesize = dfd_fpga_upg_get_file_size(upg_file);
    if (filesize <= 0) {
        DFD_ERROR("invalid filesize %lu.\n", filesize);
        return -DFD_RV_DEV_FAIL;
    }

    fd = open(upg_file, O_RDONLY);
    if (fd < 0) {
        DFD_ERROR("open file[%s] fail.\n", upg_file);
        return -DFD_RV_DEV_FAIL;
    }

    i = 0;
    cnt = DFD_FPGA_UPGADE_RETRY_CNT;
    while(i < cnt) {
        ret = dfd_fpga_upgrade_do_upgrade_onetime(current_pci_priv, fd, filesize);
        if (ret) {
            i++;
            DFD_ERROR("dfd_fpga_upgrade_do_upgrade_onetime upg_file %s failed ret %d.\n", upg_file, ret);
            continue;
        } else {
            DFD_ERROR("dfd_fpga_upgrade_do_upgrade_onetime upg_file %s success.\n", upg_file);
            close(fd);
            return 0;
        }
    }

    DFD_ERROR("upg_file %s failed ret %d.\n", upg_file, ret);
    close(fd);
    return ret;
}

static int dfd_fpga_upgrade_do_upgrade_onetime_all(dfd_pci_dev_priv_t *pci_priv, char* upg_file)
{
    int ret;
    int i, len, fd, read_len;
    char wbuf[DFD_FPGA_UPGRADE_BUFF_SIZE];
    char rbuf[DFD_FPGA_UPGRADE_BUFF_SIZE];
    int offset;

    DFD_VERBOS("Update upg_file: %s.\n", upg_file);

    if ((current_pci_priv == NULL) || (upg_file == NULL)) {
        DFD_ERROR("Input para invalid pci_priv %p upg_file %p.\n", pci_priv, upg_file);
        return -DFD_RV_INDEX_INVALID;
    }

    fd = open(upg_file, O_RDONLY);
    if (fd < 0) {
        DFD_ERROR("open file[%s] fail.\n", upg_file);
        return -DFD_RV_DEV_FAIL;
    }

    /* Before upgrading, first initialize the configuration and erase the entire SPI chip */
    ret = dfd_fpga_upg_hw_init(pci_priv);
    if (ret) {
        DFD_ERROR("fpga hw init failed ret %d.\n", ret);
        ret = -DFD_RV_DEV_FAIL;
        goto exit;
    }

    i = 0;
    offset = 0;
    while(1) {
        len = DFD_FPGA_UPGRADE_BUFF_SIZE;
        mem_clear(wbuf, DFD_FPGA_UPGRADE_BUFF_SIZE);
        read_len = read(fd, wbuf, len);
        i++;
        if ((read_len > 0) && (read_len <= len)) {
            /* write data first */
            ret = dfd_fpga_upg_write(pci_priv, offset, wbuf, read_len);
            if (ret) {
                DFD_ERROR("fpga upg write offset 0x%x len %d failed ret %d.\n", offset, read_len, ret);
                ret = -DFD_RV_DEV_FAIL;
                goto exit;
            } else {
                DFD_VERBOS("page %d upg write offset 0x%x len %d success.\n", i, offset, read_len);
            }

            /* go back to read data */
            mem_clear(rbuf, DFD_FPGA_UPGRADE_BUFF_SIZE);
            ret = dfd_fpga_upg_read(pci_priv, offset, rbuf, read_len);
            if (ret) {
                DFD_ERROR("fpga upg read offset 0x%x len %d failed ret %d.\n", offset, read_len, ret);
                ret = -DFD_RV_DEV_FAIL;
                goto exit;
            } else {
                DFD_VERBOS("page %d upg read offset 0x%x len %d success.\n", i, offset, read_len);
            }

            if (memcmp(rbuf, wbuf, read_len)) {
                DFD_ERROR("rbuf wbuf not equal, read_len %d, check failed.\n", read_len);
                ret = -DFD_RV_DEV_FAIL;
                goto exit;
            }

            DFD_VERBOS("page %d upg check offset 0x%x len %d success.\n", i, offset, read_len);
            offset += read_len;
            if (read_len != len) {
                DFD_VERBOS("page %d read_len %d len %d, last page exit.\n", i, read_len, len);
                break;
            }
        } else if (read_len == 0) {
            DFD_VERBOS("read_len %d exit.\n", read_len);
            break;
        } else {
            DFD_ERROR("len %d read_len %d, read failed.\n", len, read_len);
            ret = -DFD_RV_DEV_FAIL;
            goto exit;
        }
    }

    ret = 0;
    DFD_VERBOS("Update upg_file: %s success.\n", upg_file);
exit:
    close(fd);
    return ret;
}

int dfd_fpga_upgrade_do_upgrade_all(char* upg_file)
{
    int ret;
    int i;
    int cnt;

    DFD_VERBOS("Enter.\n");

    if (upg_file == NULL) {
        DFD_ERROR("Input para invalid upg_file %p.\n", upg_file);
        return -DFD_RV_INDEX_INVALID;
    }

    dfd_fpga_upg_init();

    i = 0;
    cnt = DFD_FPGA_UPGADE_RETRY_CNT;
    while(i < cnt) {
        ret = dfd_fpga_upgrade_do_upgrade_onetime_all(current_pci_priv, upg_file);
        if (ret) {
            i++;
            DFD_ERROR("dfd_fpga_upgrade_do_upgrade_onetime upg_file %s failed ret %d.\n", upg_file, ret);
            continue;
        } else {
            DFD_ERROR("dfd_fpga_upgrade_do_upgrade_onetime upg_file %s success.\n", upg_file);
            return 0;
        }
    }

    DFD_ERROR("upg_file %s failed ret %d.\n", upg_file, ret);
    return ret;
}

/* External fpga dump interface */
int dfd_fpga_upgrade_dump_flash(int argc, char* argv[])
{
    int offset, addr, len, dlen;
    int size, cnt, i;
    char *stopstring;
    int ret, fd;
    char filename[DFD_FPGA_UPGRADE_BUFF_SIZE];
    char tmp[DFD_FPGA_UPGRADE_BUFF_SIZE];
    char is_print;
    char buf[DFD_FPGA_UPGRADE_BUFF_SIZE];
    dfd_pci_dev_priv_t *pci_priv;

    ret = DFD_RV_OK;
    if (argc != 6) {
        printf("fpga dump flash Input invalid.\n");
        return -DFD_RV_INDEX_INVALID;
    }

    dfd_fpga_upg_init();

    if (current_pci_priv == NULL) {
        printf("fpga dump flash Input para invalid pci_priv %p.\n", current_pci_priv);
        return -DFD_RV_INIT_ERR;
    }

    offset = strtol(argv[3], &stopstring, 16);
    size = strtol(argv[4], &stopstring, 16);

    if (strcmp(argv[5], "print") != 0) {
        is_print = 0;
        mem_clear(filename, DFD_FPGA_UPGRADE_BUFF_SIZE);
        strncpy(filename, argv[5], (DFD_FPGA_UPGRADE_BUFF_SIZE - 1));
        fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXG|S_IRWXU|S_IRWXO);
        if (fd < 0) {
            snprintf(tmp, sizeof(tmp), "%s", filename);
            printf("open file %s fail(err:%d)!\r\n", tmp, errno);
            ret = -DFD_RV_DEV_FAIL;
            goto exit;
        }
    } else {
        is_print = 1;
    }

    pci_priv = current_pci_priv;
    cnt = size / DFD_FPGA_UPGRADE_BUFF_SIZE;
    if (size % DFD_FPGA_UPGRADE_BUFF_SIZE) {
        cnt++;
    }
    len = DFD_FPGA_UPGRADE_BUFF_SIZE;
    printf("cnt %d.\n", cnt);
    for (i = 0; i < cnt; i++) {
        mem_clear(buf, len);
        addr = offset + i * DFD_FPGA_UPGRADE_BUFF_SIZE;
        if (i == (cnt - 1)) {
            dlen = size - len * i;
            if (dlen > len) {
                printf("dlen %d len %d error.\n", dlen, len);
            }
        } else {
            dlen = len;
        }
        ret = dfd_fpga_upg_read(pci_priv, addr, buf, dlen);
        if (ret < 0) {
            printf("addr 0x%x failed ret %d\n", addr, ret);
            goto exit_close;
        }
        if (is_print) {
            dfd_utest_printf_reg((uint8_t*)buf, dlen);
        } else {
            ret = write(fd, buf, dlen);
            if (ret < 0) {
                printf("write failed (errno: %d).\n", errno);
                ret = -DFD_RV_DEV_FAIL;
                goto exit_close;
            }
        }
    }

exit_close:
    if (!is_print) {
        close(fd);
    }
exit:
    return ret;
}