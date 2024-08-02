#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <linux/version.h>
#include <asm/byteorder.h>
#include <stdint.h>
#include <firmware_app.h>
#include "fw_upg_spi_logic_dev.h"

#define be32_to_cpus(p)  __be32_to_cpus(p)
#define le32_to_cpus(p)  __le32_to_cpus(p)
#define cpu_to_be32s(p)  __cpu_to_be32s(p)
#define cpu_to_le32s(p)  __cpu_to_le32s(p)

static void firmware_upgrade_printf_reg(uint8_t *buf, int buf_len, uint32_t offset_addr)
{
    int i, j, tmp;

    j = offset_addr % 16;
    tmp = j;
    offset_addr -= j;
    printf("\n            ");

    for (i = 0; i < 16; i++) {
        printf("%2x ", i);
    }

    for (i = 0; i < buf_len + j; i++) {
        if ((i % 16) == 0) {
            printf("\n0x%08x  ", offset_addr);
            offset_addr = offset_addr + 16;
        }
        if (tmp) {
            printf("   ");
            tmp--;
        } else {
            printf("%02x ", buf[i-j]);
        }
    }

    printf("\n");
    return;
}

static int firmware_upgrade_get_spi_logic_info(int fd, firmware_spi_logic_upg_t *current_upg_priv)
{
    int ret;
    firmware_spi_logic_info_t syfs_info;

    if (fd < 0) {
        dbg_print(is_debug_on, "Error: get spi logic info fd %d.\n", fd);
        return fd;
    }

    ret = 0;
    ret = ioctl(fd, FIRMWARE_SYSFS_SPI_INFO, &syfs_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "Failed to get upg flash dev info, ret=%d\n", ret);
        return -FW_SPI_FLASH_GET_INFO_ERR;
    }

    current_upg_priv->flash_base          = syfs_info.flash_base;
    current_upg_priv->ctrl_base           = syfs_info.ctrl_base;
    memcpy(current_upg_priv->dev_path, syfs_info.logic_dev_name, FIRMWARE_LOGIC_DEV_NAME_LEN - 1);
    current_upg_priv->status_reg          = syfs_info.ctrl_base + FPGA_UPG_STATUS_REG;
    current_upg_priv->spi_ctrl_reg        = syfs_info.ctrl_base + FPGA_UPG_SPI_CTRL_REG;
    current_upg_priv->wr_flash_status_reg = syfs_info.ctrl_base + FPGA_UPG_WR_FLASH_STATUS_REG;
    current_upg_priv->rd_flash_status_reg = syfs_info.ctrl_base + FPGA_UPG_RD_FLASH_STATUS_REG;
    current_upg_priv->instruction_reg     = syfs_info.ctrl_base + FPGA_UPG_INSTRUCTION_REG;
    current_upg_priv->addr_reg            = syfs_info.ctrl_base + FPGA_UPG_ADDR_REG;
    current_upg_priv->length_reg          = syfs_info.ctrl_base + FPGA_UPG_LENGTH_REG;
    current_upg_priv->device_id_reg       = syfs_info.ctrl_base + FPGA_UPG_DEVICE_ID_REG;
    current_upg_priv->drop_reg_num_reg    = syfs_info.ctrl_base + FPGA_UPG_DROP_REQ_NUM_REG;
    current_upg_priv->test_base           = syfs_info.test_base;
    current_upg_priv->test_size           = syfs_info.test_size;

    return 0;
}

static int firmware_upgrade_spi_logic_init(int fd)
{
    int ret;

    ret = 0;
    ret = ioctl(fd, FIRMWARE_SYSFS_INIT, NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "Failed to init spi logic, ret=%d\n", ret);
        return -1;
    }

    return 0;
}

static int firmware_upgrade_spi_logic_finish(int fd)
{
    int ret;

    if (fd < 0) {
        dbg_print(is_debug_on, "Error: get spi logic info fd %d.\n", fd);
        return -1;
    }

    ret = 0;
    ret = ioctl(fd, FIRMWARE_SYSFS_FINISH, NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "Failed to release spi logic, ret=%d\n", ret);
        return -1;
    }

    return 0;
}

/**
 * firmware_fpga_file_read -
 * function:Provide FPGA read-register interface (address must be 4-byte aligned)
 * @dev_name: Device file descriptor
 * @offset: device offset
 * @buf:  Read Data Buffer
 * @rd_len:  Read Data Length
 * return: 0--success; other--fail
 */
int firmware_fpga_file_read(char *dev_name, uint32_t offset, uint8_t *buf, uint32_t rd_len)
{
    int ret, fd;

    if ((dev_name == NULL) || (buf == NULL)) {
        dbg_print(is_debug_on, "upg_priv or read buf is null.\n");
        return -1;
    }

    if ((fd = open(dev_name, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        dbg_print(is_debug_on, "Error: Could not open file %s. Errno=%d\n", dev_name, errno);
        return -1;
    }

    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        dbg_print(is_debug_on, "read llseek failed, errno: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    ret = read(fd, buf, rd_len);
    if (ret  < 0) {
        dbg_print(is_debug_on, "read failed, err: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int firmware_fpga_read_word(char *dev_name, uint32_t addr, uint32_t *val)
{
    int ret;
    uint32_t retry;

    if (sizeof(int) < FIRMWARE_FPGA_WORD_LEN) {
        dbg_print(is_debug_on, "Error:dfd_fpga_read_word buf len %ld support len %d.\n",
                sizeof(int), FIRMWARE_FPGA_WORD_LEN);
        return -1;
    }

    retry = 0;
    *val = 0;
    while(retry < FIRMWARE_FPGA_UPG_RETRY_CNT) {
        ret = firmware_fpga_file_read(dev_name, addr, (uint8_t *)val, FIRMWARE_FPGA_WORD_LEN);
        if (ret) {
            retry++;
            dbg_print(is_debug_on, "firmware_fpga_file_read addr 0x%x retry %u failed ret %d.\n",
                addr, retry, ret);
            continue;
        } else {
            le32_to_cpus(val);
            return 0;
        }
    }

    dbg_print(is_debug_on, "dfd_fpga_read_word addr 0x%x retry %u failed ret %d.\n", addr, retry, ret);
    return -1;
}

static int firmware_fpga_read_buf(char *dev_name, uint32_t addr, uint8_t *buf, uint32_t rd_len)
{
    int ret;
    uint32_t retry;

    retry = 0;
    while(retry < FIRMWARE_FPGA_UPG_RETRY_CNT) {
        ret = firmware_fpga_file_read(dev_name, addr, buf, rd_len);
        if (ret) {
            retry++;
            dbg_print(is_debug_on, "firmware_fpga_file_read addr 0x%x rd_len %u i %d failed ret %d.\n",
                    addr, rd_len, retry, ret);
            continue;
        } else {
            return 0;
        }
    }

    dbg_print(is_debug_on, "firmware_fpga_file_read addr 0x%x rd_len %u retry %u failed ret %d.\n",
            addr, rd_len, retry, ret);
    return -1;
}

/**
 * firmware_fpga_file_write -
 * function:Provide FPGA write-register interface (address must be 4-byte aligned)
 * @dev_name: Device file descriptor
 * @offset: device offset
 * @buf:  Write Data Buffer
 * @wr_len:  Write Data Length
 * return: 0--success; other--fail
 */
int firmware_fpga_file_write(char *dev_name, uint32_t offset, uint8_t *buf, uint32_t wr_len)
{
    int ret, fd;

    if ((dev_name == NULL) || (buf == NULL)) {
        dbg_print(is_debug_on, "dev_name or write buf is null.\n");
        return -1;
    }

    if ((fd = open(dev_name, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        dbg_print(is_debug_on, "Error: Could not open file %s. Errno=%d\n", dev_name, errno);
        return -1;
    }

    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        dbg_print(is_debug_on, "write llseek failed, err: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    ret = write(fd, buf, wr_len);
    if (ret < 0 ) {
        dbg_print(is_debug_on, "write failed, err: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int firmware_fpga_write_word(char *dev_name, uint32_t addr, uint32_t val)
{
    int ret;
    uint32_t retry, tmp;

    retry = 0;
    tmp = val;
    cpu_to_le32s(&tmp);
    while(retry < FIRMWARE_FPGA_UPG_RETRY_CNT) {
        ret = firmware_fpga_file_write(dev_name, addr, (uint8_t *)&tmp, FIRMWARE_FPGA_WORD_LEN);
        if (ret) {
            retry++;
            dbg_print(is_debug_on, "firmware_fpga_file_write addr 0x%x val 0x%x retry %u failed ret %d.\n",
                addr, val, retry, ret);
            continue;
        } else {
            return 0;
        }
    }

    dbg_print(is_debug_on, "firmware_fpga_file_write addr 0x%x val 0x%x retry %u failed ret %d.\n",
        addr, val, retry, ret);
    return -1;
}

static int firmware_fpga_write_buf(char *dev_name, uint32_t addr, uint8_t *buf, uint32_t wr_len)
{
    int ret;
    uint32_t retry;

    retry = 0;
    while(retry < FIRMWARE_FPGA_UPG_RETRY_CNT) {
        ret = firmware_fpga_file_write(dev_name, addr, buf, wr_len);
        if (ret) {
            retry++;
            dbg_print(is_debug_on, "firmware_fpga_file_write addr 0x%x wr_len 0x%x retry %u failed ret %d.\n",
                addr, wr_len, retry, ret);
            continue;
        } else {
            return 0;
        }
    }

    dbg_print(is_debug_on, "dfd_fpga_buf_write addr 0x%x wr_len 0x%x retry %u failed ret %d.\n",
        addr, wr_len, retry, ret);

    return -1;
}

/* Whether the SPI port is idle, 0--idle, 1--busy */
static int firmware_fpga_get_status(firmware_spi_logic_upg_t *upg_priv, char *status)
{
    int ret;
    uint32_t addr, val;

    addr = upg_priv->status_reg;
    ret = firmware_fpga_read_word(upg_priv->dev_path, addr, &val);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_get_status addr 0x%x failed ret %d.\n", addr, ret);
        return -1;
    }

    *status = val & FPGA_UPG_STATUS_MASK;

    return 0;
}

/* Wait for the SPI port to become free again */
static int firmware_fpga_wait_ready(firmware_spi_logic_upg_t *upg_priv)
{
    int timeout;
    char status;
    int ret;

    timeout = FIRMWARE_UPG_RETRY_TIME_CNT;
    while (timeout--) {
        usleep(FIRMWARE_UPG_RETRY_SLEEP_TIME);
        ret = firmware_fpga_get_status(upg_priv, &status);
        if (ret) {
            dbg_print(is_debug_on, "firmware_fpga_get_status failed ret %d.\n", ret);
            continue;
        }

        /* Determine if it's idle */
        if (!status) {
            return 0;
        }
    }

    return -1;
}

/* Configure access */
static int firmware_fpga_set_access(firmware_spi_logic_upg_t *upg_priv, uint32_t cmd)
{
    int ret;
    uint32_t addr, val;

    addr = upg_priv->instruction_reg;
    val = cmd;
    ret = firmware_fpga_write_word(upg_priv->dev_path, addr, val);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -1;
    }

    addr = upg_priv->spi_ctrl_reg;
    val = FPGA_UPG_ACCESS_ENABLE;
    ret = firmware_fpga_write_word(upg_priv->dev_path, addr, val);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -1;
    }

    /* Wait for the SPI port on the FPGA to become free again*/
    ret = firmware_fpga_wait_ready(upg_priv);
    if (ret) {
        dbg_print(is_debug_on,"firmware_fpga_wait_ready failed ret %d.\n", ret);
        return -FW_SPI_FLASH_BUSY;
    }

    return 0;
}

/* Get SPI STATUS register */
static int firmware_fpga_get_spi_status(firmware_spi_logic_upg_t *upg_priv, char *status)
{
    int ret;
    uint32_t val, addr, cmd;

    cmd = FPGA_UPG_INSTRUTION_RDSR;
    ret = firmware_fpga_set_access(upg_priv, cmd);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_set_access cmd 0x%x failed ret %d.\n", cmd, ret);
        return -1;
    }

    addr = upg_priv->rd_flash_status_reg;
    ret = firmware_fpga_read_word(upg_priv->dev_path, addr, &val);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_read_word addr 0x%x failed ret %d.\n", addr, ret);
        return -1;
    }

    *status = val & FPGA_UPG_SPI_STATUS_MASK;

    return 0;
}

/* Wait for the SPI chip operation to complete */
static int firmware_fpga_wait_spi_ready(firmware_spi_logic_upg_t *upg_priv,
    uint32_t timeout, uint32_t usleep_time)
{
    char status;
    int ret;

    while (timeout--) {
        usleep(usleep_time);
        ret = firmware_fpga_get_spi_status(upg_priv, &status);
        if (ret) {
            dbg_print(is_debug_on, "firmware_fpga_get_spi_status failed ret %d.\n", ret);
            continue;
        }
        /* Determine if it's idle */
        if (!status) {
            return 0;
        }
    }

    return -FW_SPI_FLASH_SPI_BUSY;
}

/* Configure FPGA upgrade write enable */
static int firmware_fpga_set_wr_enable(firmware_spi_logic_upg_t *upg_priv)
{
    int ret;
    uint32_t cmd;

    cmd = FPGA_UPG_INSTRUTION_WREN;
    ret = firmware_fpga_set_access(upg_priv, cmd);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -1;
    }

    return 0;
}

#if 0
/* erase all flash */
static int firmware_fpga_upg_set_erase_all(firmware_spi_logic_upg_t *upg_priv)
{
    int ret;
    int cmd;

    /* Wait for the SPI port on the FPGA to become free */
    ret = firmware_fpga_wait_ready(upg_priv);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_wait_ready failed ret %d.\n", ret);
        return -1;
    }

    /* Configure FPGA upgrade write enable */
    ret = firmware_fpga_set_wr_enable(upg_priv);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_set_wr_enable failed ret %d.\n", ret);
        return -1;
    }

    cmd = FPGA_UPG_INSTRUTION_BE;
    ret = firmware_fpga_set_access(upg_priv, cmd);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -1;
    }

    /* Hardware requirements, delay of 1s */
    sleep(1);

    /* Wait for the SPI chip operation to complete, 1s check status once, max delay 300s */
    ret = firmware_fpga_wait_spi_ready(upg_priv, 300, (1 * 1000 * 1000));
    if (ret) {
        dbg_print(is_debug_on, "dfd_fpga_wait_spi_ready failed ret %d.\n", ret);
        return -1;
    }

    dbg_print(is_debug_on, "Success.\n");
    return 0;
}
#endif

/* Erase sectors (256 pages, 64K total) */
static int firmware_fpga_erase_sector(firmware_spi_logic_upg_t *upg_priv, uint32_t spi_addr)
{
    int ret;
    uint32_t val, addr, cmd;

    /* Wait for the SPI port on the FPGA to become free again */
    ret = firmware_fpga_wait_ready(upg_priv);
    if (ret < 0) {
        dbg_print(is_debug_on, "firmware_fpga_wait_ready failed ret %d.\n", ret);
        return -FW_SPI_FLASH_BUSY;
    }

    /* Enable write */
    ret = firmware_fpga_set_wr_enable(upg_priv);
    if (ret < 0) {
        dbg_print(is_debug_on, "firmware_fpga_set_wr_enable failed ret %d.\n", ret);
        return -FW_SPI_FLASH_WR_ENABLE_ERR;
    }

    /* Write erase address */
    val = spi_addr;
    addr = upg_priv->addr_reg;
    ret = firmware_fpga_write_word(upg_priv->dev_path, addr, val);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_write_word addr 0x%x val 0x%x failed ret %d.\n", addr, val, ret);
        return -FW_SPI_FLASH_ERASE_ADDR_ERR;
    }

    /* Enable sector erasure */
    cmd = FPGA_UPG_INSTRUTION_SE;
    ret = firmware_fpga_set_access(upg_priv, cmd);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -FW_SPI_FLASH_ERASE_SECTOR_ERR;
    }

    /* Hardware requirements, delay of 0.25s */
    usleep(250 * 1000);

    /* Wait for the SPI chip operation to complete, 1s check status once, max delay 10s */
    ret = firmware_fpga_wait_spi_ready(upg_priv, FPGA_UPG_WAIT_SPI_RETRY_CNT, (100 * 1000));
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_wait_spi_ready failed ret %d.\n", ret);
        return -FW_SPI_FLASH_SPI_BUSY;
    }

    return 0;
}

#if 0
int firmware_fpga_erase64_sector(firmware_spi_logic_upg_t *upg_priv, int offset)
{
    int ret;
    ret = -1;

    if ((offset % FIRMWARE_SPI_LOGIC_SECTOR_SIZE) == 0) {
        dbg_print(is_debug_on, "erase 64k area, offset 0x%x.\n", offset);
        ret = firmware_fpga_erase_sector(upg_priv, offset);
        if (ret) {
            dbg_print(is_debug_on, "firmware_fpga_erase_sector offset 0x%x failed ret %d.\n", offset, ret);
            return ret;
        }
    } else {
        dbg_print(is_debug_on, "Input para invalid, offset 0x%x.\n", offset);
    }

    return ret;
}
#endif

static int firmware_fpga_upg_program(firmware_spi_logic_upg_t *upg_priv,
    uint32_t spi_addr, uint8_t *buf, uint32_t len)
{
    int ret;
    uint32_t addr, val, cmd, wr_len;

    /* Write data to the Upgrade Content Register */
    addr = upg_priv->ctrl_base;
    wr_len = len;
    ret = firmware_fpga_write_buf(upg_priv->dev_path, addr, (uint8_t*)buf, wr_len);
    if (ret) {
        dbg_print(is_debug_on,"firmware_fpga_write_buf addr 0x%x wr_len %d failed ret %d.\n",
                    addr, len, ret);
        return -FW_SPI_FLASH_WR_ERR;
    }

    /* Write length register, FPGA is fixed 256 lengths */
    val = FFPGA_UPG_DATA_SIZE;
    addr = upg_priv->length_reg;
    ret = firmware_fpga_write_word(upg_priv->dev_path, addr, val);
    if (ret) {
        dbg_print(is_debug_on,"firmware_fpga_write_word addr 0x%x val 0x%x failed ret %d.\n",
                addr, val, ret);
        return -FW_SPI_FLASH_WR_LENGTH_ERR;
    }

    /* Write address register */
    val = spi_addr;
    addr = upg_priv->addr_reg;
    ret = firmware_fpga_write_word(upg_priv->dev_path, addr, val);
    if (ret) {
        dbg_print(is_debug_on,"firmware_fpga_write_word addr 0x%x val 0x%x failed ret %d.\n",
                addr, val, ret);
        return -FW_SPI_FLASH_WR_ADDR_ERR;
    }

    /* Start writing upgrade data to SPI */
    cmd = FPGA_UPG_INSTRUTION_PP;
    ret = firmware_fpga_set_access(upg_priv, cmd);
    if (ret) {
        dbg_print(is_debug_on,"firmware_fpga_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -FW_SPI_FLASH_SET_ACCESS_ERR;
    }

    /* min write wait 0.33ms */
    usleep(330);

    /* Wait for the SPI chip operation to complete, 100us check status once, max delay 10ms */
    ret = firmware_fpga_wait_spi_ready(upg_priv, FPGA_UPG_WAIT_SPI_RETRY_CNT, (100));
    if (ret) {
        dbg_print(is_debug_on,"firmware_fpga_wait_spi_ready failed ret %d.\n", ret);
        return -FW_SPI_FLASH_BUSY;
    }

    return 0;
}

/**
 * firmware_fpga_upg_write
 * function: write interface provided to the upgrade module
 * @upg_priv: Device information
 * @addr: upgrade addr
 * @buf:  Write Data Buffer
 * @len:  Write Data Length
 * return: 0--success; other--fail
 */
static int firmware_fpga_upg_write(firmware_spi_logic_upg_t *upg_priv,
    uint32_t addr, uint8_t *buf, uint32_t len)
{
    int ret;

    /* address must be 256 bytes aligned */
    if ((upg_priv == NULL) || (buf == NULL) || (addr & 0xff) || (len > 256)) {
        dbg_print(is_debug_on,"Input para invalid upg_priv %p buf %p addr 0x%x len %u.\n",
                upg_priv, buf, addr, len);
        return -FW_SPI_FLASH_PARAM_ERR;
    }

    /* Wait for the SPI port on the FPGA to become free again*/
    ret = firmware_fpga_wait_ready(upg_priv);
    if (ret) {
        dbg_print(is_debug_on,"firmware_fpga_wait_ready failed ret %d.\n", ret);
        return -FW_SPI_FLASH_BUSY;
    }

    /* Configure write enable */
    ret = firmware_fpga_set_wr_enable(upg_priv);
    if (ret) {
        dbg_print(is_debug_on,"firmware_fpga_set_wr_enable failed ret %d.\n", ret);
        return -FW_SPI_FLASH_WR_ENABLE_ERR;
    }

    /* Write upgrade data */
    ret = firmware_fpga_upg_program(upg_priv, addr, buf, len);
    if (ret) {
        dbg_print(is_debug_on,"dfd_fpga_upg_program addr 0x%x len %u failed ret %d.\n", addr, len, ret);
        return -FW_SPI_FLASH_UPG_ERR;
    }

    return 0;
}

static int firmware_fpga_fast_read(firmware_spi_logic_upg_t *upg_priv,
    uint32_t spi_addr, uint8_t *buf, uint32_t len)
{
    int ret;
    uint32_t val, addr, cmd;

    /* Clear register value */
    addr = upg_priv->ctrl_base;
    ret = firmware_fpga_write_buf(upg_priv->dev_path, addr, buf, len);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_write_buf addr 0x%x len %d failed ret %d.\n", addr, len, ret);
        return -FW_SPI_FLASH_WR_ERR;
    }
    /* Write length register */
    val = FFPGA_UPG_DATA_SIZE;
    addr = upg_priv->length_reg;
    ret = firmware_fpga_write_word(upg_priv->dev_path, addr, val);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_write_word addr 0x%x val 0x%x failed ret %d.\n",
                addr, val, ret);
        return -FW_SPI_FLASH_WR_LENGTH_ERR;
    }

    /* Write address register */
    val = spi_addr;
    addr = upg_priv->addr_reg;
    ret = firmware_fpga_write_word(upg_priv->dev_path, addr, val);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_write_word addr 0x%x val 0x%x failed ret %d.\n",
                    addr, val, ret);
        return -FW_SPI_FLASH_WR_ADDR_ERR;
    }

    /* Start reading SPI data */
    cmd = FPGA_UPG_INSTRUTION_FR;
    ret = firmware_fpga_set_access(upg_priv, cmd);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_set_access cmd %d failed ret %d.\n", cmd, ret);
        return -FW_SPI_FLASH_SET_ACCESS_ERR;
    }

    /* Read the upgraded content register to the buffer,
     * FPGA only supports 4 bytes of read and write */
    addr = upg_priv->ctrl_base;
    ret = firmware_fpga_read_buf(upg_priv->dev_path, addr, (uint8_t*)buf, len);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_read_buf addr 0x%x len %d failed ret %d.\n", addr, len, ret);
        return -FW_SPI_FLASH_RD_ERR;
    }

    return 0;
}

/**
 * firmware_fpga_upg_read
 * function: read interface provided to the upgrade module
 * @upg_priv: Device information
 * @addr: upgrade addr
 * @buf:  Read Data Buffer
 * @len:  Read Data Length
 * return: 0--success; other--fail
 */
static int firmware_fpga_upg_read(firmware_spi_logic_upg_t *upg_priv,
    uint32_t addr, uint8_t *buf, uint32_t len)
{
    int ret;

    /* address must be 256 bytes aligned */
    if ((upg_priv == NULL) || (buf == NULL) || (addr & 0xff) || (len > 256)) {
        dbg_print(is_debug_on, "Input para invalid upg_priv %p buf %p addr 0x%x len %u.\n",
                upg_priv, buf, addr, len);
        return -FW_SPI_FLASH_PARAM_ERR;
    }

    /* Wait for the SPI port on the FPGA to become free again */
    ret = firmware_fpga_wait_ready(upg_priv);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_wait_ready failed ret %d.\n", ret);
        return -FW_SPI_FLASH_BUSY;
    }

    /* Configure write enable */
    ret = firmware_fpga_set_wr_enable(upg_priv);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_set_wr_enable failed ret %d.\n", ret);
        return -FW_SPI_FLASH_WR_ENABLE_ERR;
    }

    /* Read upgrade data */
    ret = firmware_fpga_fast_read(upg_priv, addr, buf, len);
    if (ret) {
        dbg_print(is_debug_on, "firmware_fpga_fast_read addr 0x%x len %u failed ret %d.\n", addr, len, ret);
        return -FW_SPI_FLASH_RD_ERR;
    }

    return 0;

}

static int firmware_upgreade_fpga_onetime(firmware_spi_logic_upg_t *upg_priv,
        uint32_t flash_base, uint8_t *buf, uint32_t size)
{
    uint32_t offset, len, flash_addr, retry;
    int ret, res;
    uint8_t rbuf[FFPGA_UPG_DATA_SIZE];

    offset = 0;
    while(offset < size) {
        flash_addr = flash_base + offset;
        /* Erases a sector */
        if ((flash_addr % FIRMWARE_SPI_LOGIC_SECTOR_SIZE) == 0) {
            ret = firmware_fpga_erase_sector(upg_priv, flash_addr);
            if (ret < 0) {
                dbg_print(is_debug_on, "firmware_fpga_erase_sector flash_addr 0x%x failed ret %d.\n",
                        flash_addr, ret);
                goto exit;
            }
        }

        if (size > FFPGA_UPG_DATA_SIZE) {
            len = FFPGA_UPG_DATA_SIZE;
        } else {
            len = size;
        }

        /* first, Write data */
        ret = firmware_fpga_upg_write(upg_priv, flash_addr, buf + offset, len);
        if (ret) {
            dbg_print(is_debug_on, "firmware_fpga_upg_write addr 0x%x len 0x%x failed ret %d.\n",
                    flash_addr, len, ret);
            ret = -FW_SPI_FLASH_UPG_ERR;
            goto exit;
        }

        /* Read back the data and compare the correctness of the data */
        for (retry = 0; retry < FPGA_UPG_RETRY_TIMES; retry++) { /*retry 3 times*/
            mem_clear(rbuf, len);
            ret = firmware_fpga_upg_read(upg_priv, flash_addr, rbuf, len);
            res = memcmp(rbuf, buf + offset, len);
            if (ret || res) {
                usleep(1000);
                continue;
            }
            break;
        }

         if (ret) {
            dbg_print(is_debug_on, "firmware fpga read offset 0x%x len 0x%x failed ret %d.\n", flash_addr, len, ret);
            ret = -FW_SPI_FLASH_RD_ERR;
            goto exit;
        }

        if (res) {
            dbg_print(is_debug_on, "firmware fpga rbuf wbuf not equal, len 0x%x, check failed.\n", len);
            ret = -FW_SPI_FLASH_DATA_CMP_ERR;
            goto exit;
        }
        offset += len;
    }

    dbg_print(is_debug_on, "Update success.\n");
    return FIRMWARE_SUCCESS;
exit:
    dbg_print(is_debug_on, "Update failed.\n");
    return FIRMWARE_FAILED;
}

static int firmware_upgrade_do_spi_logic(firmware_spi_logic_upg_t *current_upg_priv,
        unsigned char *buf, uint32_t size)
{
    int i, ret;
    uint32_t retry;

    i = 0;
    retry = FIRMWARE_SPI_LOGIC_UPG_RETRY_CNT;

    ret = 0;
    while(i < retry) {
        ret = firmware_upgreade_fpga_onetime(current_upg_priv, current_upg_priv->flash_base, buf, size);
        if (ret) {
            i++;
            dbg_print(is_debug_on, "firmware_upgreade_fpga_onetime size 0x%x failed ret %d.\n", size, ret);
            continue;
        } else {
            dbg_print(is_debug_on, "firmware_upgreade_fpga_onetime success.\n");
            return 0;
        }
    }

    return ret;
}

/*
 * firmware_upgrade_spi_logic_dev
 * function: FPGA SPI FLASH Firmware upgrade handler function
 * @fd:   param[in] sysfs device descriptor
 * @buf:  param[in] Update data
 * @size: param[in] Update data size
 * @info: param[in] Upgrade file information
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int firmware_upgrade_spi_logic_dev(int fd, uint8_t *buf, uint32_t size, name_info_t *info)
{
    int ret;
    firmware_spi_logic_upg_t current_upg_priv;

    if ((fd < 0) || (buf == NULL) || (info == NULL)) {
        dbg_print(is_debug_on, "Error:firmware upgrade spi logic dev parameters failed.\n");
        return FIRMWARE_FAILED;
    }

    /* Gets the current logical device information */
    mem_clear(&current_upg_priv, sizeof(firmware_spi_logic_upg_t));
    ret = firmware_upgrade_get_spi_logic_info(fd, &current_upg_priv);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_get_spi_logic_info failed ret %d.\n", ret);
        return FIRMWARE_FAILED;
    }

    dbg_print(is_debug_on, "current_upg_priv dev_path[%s] flash_base[0x%0x] ctrl_base[0x%0x]\n",
                current_upg_priv.dev_path, current_upg_priv.flash_base,
                current_upg_priv.ctrl_base);

    /* Enable upgrade access */
    ret = firmware_upgrade_spi_logic_init(fd);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_spi_logic_init failed ret %d.\n", ret);
        return FIRMWARE_FAILED;
    }

    /* Upgrade logic device */
    ret = firmware_upgrade_do_spi_logic(&current_upg_priv, buf, size);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_do_spi_logic failed ret %d.\n", ret);
        goto fail;
    }

    /* disable upgrade access */
    ret = firmware_upgrade_spi_logic_finish(fd);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_spi_logic_finish failed ret %d.\n", ret);
    }

    return FIRMWARE_SUCCESS;
fail:
    /* disable upgrade access */
    ret = firmware_upgrade_spi_logic_finish(fd);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_spi_logic_finish failed ret %d.\n", ret);
    }

    return FIRMWARE_FAILED;
}

int firmware_fpga_upgrade_test(firmware_spi_logic_upg_t *current_upg_priv)
{
    int ret, i, j, num;
    uint8_t *wbuf;
    uint32_t retry;

    ret = FW_SPI_FLASH_RV_OK;
    wbuf = (uint8_t *) malloc(current_upg_priv->test_size);
    if (wbuf == NULL) {
        dbg_print(is_debug_on, "Error: Failed to malloc memory for test data buf, size=0x%x.\n", current_upg_priv->test_size);
        ret = -FW_SPI_FLASH_NOT_SUPPORT_TEST;
        goto exit;
    }
    mem_clear(wbuf, current_upg_priv->test_size);
    /* Get random data */
    for (j = 0; j < current_upg_priv->test_size; j++) {
        num = rand() % 256;
        wbuf[j] = num & 0xff;
    }

    i = 0;
    retry = FIRMWARE_SPI_LOGIC_UPG_RETRY_CNT;

    ret = 0;
    while(i < retry) {
        ret = firmware_upgreade_fpga_onetime(current_upg_priv, current_upg_priv->test_base, wbuf, current_upg_priv->test_size);
        if (ret) {
            i++;
            dbg_print(is_debug_on, "firmware_upgreade_fpga_onetime test size 0x%x failed ret %d.\n",
                        current_upg_priv->test_size, ret);
            continue;
        } else {
            dbg_print(is_debug_on, "firmware_upgreade_fpga_onetime test success.\n");
            break;
        }
    }
    free(wbuf);
exit:
    return ret;
}

/*
 * firmware_upgrade_spi_logic_dev_test
 * function: FPGA SPI FLASH Firmware upgrade test handler function
 * @fd:   param[in] sysfs device descriptor
 * @buf:  param[in] Update data
 * @size: param[in] Update data size
 * @info: param[in] Upgrade file information
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int firmware_upgrade_spi_logic_dev_test(int fd, name_info_t *info)
{
    int ret;
    firmware_spi_logic_upg_t current_upg_priv;

    if ((fd < 0) || (info == NULL)) {
        dbg_print(is_debug_on, "Error:firmware upgrade spi logic dev parameters failed.\n");
        return FIRMWARE_FAILED;
    }

    /* Gets the current logical device information */
    mem_clear(&current_upg_priv, sizeof(firmware_spi_logic_upg_t));
    ret = firmware_upgrade_get_spi_logic_info(fd, &current_upg_priv);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_get_spi_logic_info failed ret %d.\n", ret);
        return FIRMWARE_FAILED;
    }

    dbg_print(is_debug_on, "current_upg_priv dev_path[%s] test_base[0x%0x] test_size[0x%x]\n",
                current_upg_priv.dev_path, current_upg_priv.test_base, current_upg_priv.test_size);
    if (current_upg_priv.test_size <= 0) {
        dbg_print(is_debug_on, "Error: don't support flast test.\n");
        return FIRMWARE_NOT_SUPPORT;
    }

    /* Enable upgrade access */
    ret = firmware_upgrade_spi_logic_init(fd);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_spi_logic_init failed ret %d.\n", ret);
        return FIRMWARE_FAILED;
    }

    /* Upgrade logic device */
    ret = firmware_fpga_upgrade_test(&current_upg_priv);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_do_spi_logic failed ret %d.\n", ret);
        goto fail;
    }

    /* disable upgrade access */
    ret = firmware_upgrade_spi_logic_finish(fd);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_spi_logic_finish failed ret %d.\n", ret);
    }

    return FIRMWARE_SUCCESS;
fail:
    /* disable upgrade access */
    ret = firmware_upgrade_spi_logic_finish(fd);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_spi_logic_finish failed ret %d.\n", ret);
    }

    return FIRMWARE_FAILED;
}

static int firmware_upgreade_spi_logic_dump(firmware_spi_logic_upg_t *upg_priv,
        uint32_t offset, uint8_t *buf, uint32_t size)
{
    int ret, i;
    uint32_t addr, buf_page, retry, cnt, rd_len;

    buf_page = FFPGA_UPG_DATA_SIZE;           /* read data by BUF SIZE each time */

    cnt = size / FFPGA_UPG_DATA_SIZE;
    if (size % FFPGA_UPG_DATA_SIZE) {
        cnt++;
    }
    dbg_print(is_debug_on, "need read number of times:%d.\n", cnt);

    for (i = 0; i < cnt; i++) {
        addr = offset + i * FFPGA_UPG_DATA_SIZE;
        if (i == (cnt - 1)) {
            /* last time read remain size */
            rd_len = size - buf_page * i;
        } else {
            /* each time read buf page size */
            rd_len = buf_page;
        }

        for (retry = 0; retry < FPGA_UPG_RETRY_TIMES; retry++) {
            ret = firmware_fpga_upg_read(upg_priv, addr, buf, rd_len);
            if (ret < 0) {
                dbg_print(is_debug_on, "addr:0x%x read %d time failed. ret %d\n", addr, retry, ret);
                continue;
            }
            break;
        }

        if (ret < 0) {
            dbg_print(is_debug_on, "finally addr:0x%x read failed ret %d\n", addr, ret);
            return FIRMWARE_FAILED;
        }

        buf += rd_len;      /* buf pointer offset rd_len */
    }

    return FIRMWARE_SUCCESS;
}

static int firmware_fpga_dump_read(int fd, uint32_t offset, uint8_t *buf, uint32_t len)
{
    int ret;
    firmware_spi_logic_upg_t current_upg_priv;

    if ((fd < 0) || (buf == NULL)) {
        dbg_print(is_debug_on, "Error:firmware upgrade spi logic dev parameters failed.\n");
        return FIRMWARE_FAILED;
    }

    /* Gets the current logical device information */
    mem_clear(&current_upg_priv, sizeof(firmware_spi_logic_upg_t));
    ret = firmware_upgrade_get_spi_logic_info(fd, &current_upg_priv);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_get_spi_logic_info failed ret %d.\n", ret);
        return FIRMWARE_FAILED;
    }

    dbg_print(is_debug_on, "current_upg_priv dev_path[%s] flash_base[0x%0x] ctrl_base[0x%0x]\n",
                current_upg_priv.dev_path, current_upg_priv.flash_base,
                current_upg_priv.ctrl_base);

    /* Enable upgrade access */
    ret = firmware_upgrade_spi_logic_init(fd);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_spi_logic_init failed ret %d.\n", ret);
        return FIRMWARE_FAILED;
    }

    /* read logic device */
    ret = firmware_upgreade_spi_logic_dump(&current_upg_priv, offset, buf, len);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_do_spi_logic failed ret %d.\n", ret);
        goto fail;
    }

    /* disable upgrade access */
    ret = firmware_upgrade_spi_logic_finish(fd);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_spi_logic_finish failed ret %d.\n", ret);
    }

    return FIRMWARE_SUCCESS;

fail:
    /* disable upgrade access */
    ret = firmware_upgrade_spi_logic_finish(fd);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:firmware_upgrade_spi_logic_finish failed ret %d.\n", ret);
    }

    return FIRMWARE_FAILED;
}

int firmware_upgrade_spi_logic_dev_dump(char *dev_name, uint32_t offset,
        uint32_t len, char *record_file)
{
    int ret, dev_fd, file_fd;
    char save_file[FIRMWARE_LOGIC_DEV_NAME_LEN];
    uint8_t *buf;

    dev_fd = open(dev_name, O_RDWR);
    if (dev_fd < 0) {
        dbg_print(is_debug_on, "Error: Failed to open %s, errno:%d.\n", dev_name, errno);
        return FIRMWARE_FAILED;
    }

    dbg_print(is_debug_on, "open dev file %s succeeded.\n", dev_name);

    buf = (uint8_t *) malloc(len);
    if (buf == NULL) {
        dbg_print(is_debug_on, "Error: Failed to malloc memory read %s data.\n", dev_name);
        ret = FIRMWARE_FAILED;
        goto free_dev_fd;
    }

    mem_clear(buf, len);
    ret = firmware_fpga_dump_read(dev_fd, offset, buf, len);
    if (ret < 0) {
        dbg_print(is_debug_on, "addr 0x%x read 0x%x failed ret:%d\n", offset, len, ret);
        goto free_data;
    }

    dbg_print(is_debug_on, "dump data succeeded. offset:0x%x, len:0x%x\n", offset, len);

    if (strcmp(record_file, "print") != 0) {      /* record dump data on 'record_file' */
        mem_clear(save_file, FIRMWARE_LOGIC_DEV_NAME_LEN);
        strncpy(save_file, record_file, FIRMWARE_LOGIC_DEV_NAME_LEN - 1);
        file_fd = open(save_file, O_RDWR|O_CREAT|O_TRUNC, S_IRWXG|S_IRWXU|S_IRWXO);
        if (file_fd < 0) {
            dbg_print(is_debug_on, "open file %s fail, errno:%d.\n", save_file, errno);
            ret = -ENOENT;
            goto free_data;
        }

        dbg_print(is_debug_on, "open save file %s succeeded.\n", save_file);

        ret = write(file_fd, buf, len);
        if (ret < 0) {
            dbg_print(is_debug_on, "write failed (errno: %d).\n", errno);
            goto free_file_fd;
        }
        dbg_print(is_debug_on, "write save file %s succeeded.\n", save_file);
        ret = FIRMWARE_SUCCESS;
    } else {    /* print reg on terminal by format */
        firmware_upgrade_printf_reg((uint8_t*)buf, len, offset);
        ret = FIRMWARE_SUCCESS;
        goto free_data;
    }

free_file_fd:
    close(file_fd);
free_data:
    free(buf);
free_dev_fd:
    close(dev_fd);

    return ret;
}
