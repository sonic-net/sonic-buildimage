/*
 * firmware_upgrade_rpd.c
 * Original Author : whitebox support 2022-08-17
 *
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
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
#include <time.h>
#include <firmware_app.h>
#include "firmware_upgrade_mailbox.h"
#include "firmware_upgrade_rpd.h"

static int ifc_r32(firmware_spi_logic_info_t *logic_dev_info, uint32_t reg_offset, uint32_t *val)
{
    int ret;
    uint32_t addr;
    char *dev_name;

    dev_name = logic_dev_info->dev_name;
    addr = logic_dev_info->ctrl_base + reg_offset;
    ret = firmware_fpga_read_word(dev_name, addr, val);
    if (ret < 0) {
        dbg_print(is_debug_on, "ifc_r32 read failed, dev_name: %s, addr 0x%x, ret %d.\n",
            dev_name, addr, ret);
        return IFC_ERR_OP_FAILED;
    }
    dbg_print(is_debug_on, "ifc_r32 read success, dev_name: %s addr: 0x%x, read value: 0x%x.\n",
        dev_name, addr, *val);
    return IFC_SUCCESS;
}

static int ifc_w32(firmware_spi_logic_info_t *logic_dev_info, uint32_t reg_offset, uint32_t val)
{
    int ret;
    uint32_t addr;
    char *dev_name;

    dev_name = logic_dev_info->dev_name;
    addr = logic_dev_info->ctrl_base + reg_offset;
    ret = firmware_fpga_write_word(dev_name, addr, val);
    if (ret < 0) {
        dbg_print(is_debug_on, "ifc_w32 write failed, dev_name: %s, addr 0x%x, val 0x%x, ret %d.\n",
            dev_name, addr, val, ret);
        return IFC_ERR_OP_FAILED;
    }
    dbg_print(is_debug_on, "ifc_w32 write success, dev_name: %s addr: 0x%x, write value: 0x%x.\n",
        dev_name, addr, val);
    return IFC_SUCCESS;
}

static int ifc_rw_epcq_fifo_idle(firmware_spi_logic_info_t *logic_dev_info)
{
    int rc, i, count;
    uint32_t fifo_stat;

    /* Wait until the FIFO becomes free before starting to operate on the FIFO. */
    count = FPGA_QSPI_WAIT_FIFO_TIMEOUT / FPGA_QSPI_WAIT_FIFO_SLEEP;
    for (i = 0; i <= count; i++) {
        rc = ifc_r32(logic_dev_info, IFC_TOP_CPU_MEM_STATUS_REGISTER, &fifo_stat);
        if (rc != IFC_SUCCESS) {
            dbg_print(is_debug_on, "ifc_rw_epcq_fifo_idle read CPU_MEM_STATUS reg failed, ret: %d\n", rc);
            return rc;
        }
        if (fifo_stat == 0) {
            dbg_print(is_debug_on, "ifc_rw_epcq_fifo_idle fifo status idle, read count: %d\n", i);
            return IFC_SUCCESS;
        }
        usleep(FPGA_QSPI_WAIT_FIFO_SLEEP);
    }

    dbg_print(is_debug_on, "ifc_rw_epcq_fifo_idle wait fifo idle timeout, loop: %d\n", i);
    return IFC_ERR_FW_CMD_TIMEOUT;
}

static int ifc_read_epcq_fifo(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset,
               uint8_t *buf, uint32_t len)
{
    uint32_t isr_status, rsp_status, rd_fifo_level, read_words;
    int rc, i, count;

    /* Write the flash offset address to be read into the READADDR register */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_READADDR_REGISTER, offset);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo write offset addr: 0x%x failed, ret: %d\n", offset, rc);
        return rc;
    }

    /* The length of "read_words" is the actual number of bytes read divided by 4. A value of 1 indicates that 4 bytes were read. */
    read_words = len / 4;
    if (len % 4) {
        read_words++;
    }

    /* Write the length of the data to be read from the flash into the READWORDS register. */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_READWORDS_REGISTER, read_words);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo write READWORDS reg value: %d failed, ret: %d\n",
            read_words, rc);
        return rc;
    }

    /* Write 0x02 to the READOP register to clear the read data buffer. */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_READOP_REGISTER, 0x2);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo write READOP reg to flush read data FIFO failed, ret: %d\n", rc);
        return rc;
    }

    /* Write 0x01 to the READOP register to initiate the read flash operation. The data is then read from the flash and stored in the FIFO. */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_READOP_REGISTER, 0x1);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo write READOP reg to start read failed, ret: %d\n", rc);
        return rc;
    }

    /* The logic added in this optimization: Wait Rddata valid */
    count = FPGA_QSPI_WAIT_FIFO_TIMEOUT / FPGA_QSPI_WAIT_FIFO_SLEEP;
    for (i = 0; i <= count; i++) {
        rc = ifc_r32(logic_dev_info, IFC_TOP_CPU_ISR_REGISTER, &isr_status);
        if (rc != IFC_SUCCESS) {
            dbg_print(is_debug_on, "ifc_read_epcq_fifo read ISR_REGISTER reg failed, ret: %d\n", rc);
            return rc;
        }
        if (isr_status & 0x02) {
            dbg_print(is_debug_on, "ifc_read_epcq_fifo , read data is available: loop: %d\n", i);
            break;
        }
        usleep(FPGA_QSPI_WAIT_FIFO_SLEEP);
    }

    if (i > count) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo wait read data available timeout, loop: %d\n", i);
        rc = IFC_ERR_FW_CMD_TIMEOUT;
        return rc;
    }

    /* Read the status register to determine if an abnormality has occurred */
    rc = ifc_r32(logic_dev_info, IFC_TOP_CPU_CSR_STATUS_REGISTER, &rsp_status);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo read CSR_STATUS reg failed, ret: %d\n", rc);
        return rc;
    }

    if ((isr_status & 0x1) || rsp_status) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo status error, ISR value: 0x%08x, RSP status: 0x%08x\n",
            isr_status, rsp_status);
        rc = IFC_ERR_FW_CMD_STATUS;
        return rc;
    }

    /* Read the READFIFOLVL register to determine how many bytes are available for reading in the FIFO. */
    rc = ifc_r32(logic_dev_info, IFC_TOP_CPU_READFIFOLVL_REGISTER, &rd_fifo_level);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo read READFIFOLVL reg failed, ret: %d\n", rc);
        return rc;
    }
    if (rd_fifo_level != read_words) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo read fifo level: 0x%x not equal read_words: 0x%x\n",
            rd_fifo_level, read_words);
        rc = IFC_ERR_OP_FAILED;
        return rc;
    }
    /* The original code here is waiting for FIFO idle. After optimization, we use Rddata valid for judgment instead, so there is no need to wait for idle. */
    /* Start the FIFO reading operation */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_CONTROL_REGISTER0, (read_words << 8) & 0xffffff00);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo write CONTROL_REGISTER0 to read FIFO failed, ret: %d\n", rc);
        return rc;
    }

    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_CONTROL_REGISTER1, 1);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo write CONTROL_REGISTER1 reg to start fifo read failed, ret: %d\n", rc);
        return rc;
    }

    /* The logic added in this optimization: Wait for the FIFO read operation to complete */
    count = FPGA_QSPI_WAIT_FIFO_TIMEOUT / FPGA_QSPI_WAIT_FIFO_SLEEP;
    for (i = 0; i <= count; i++) {
        rc = ifc_r32(logic_dev_info, IFC_TOP_CPU_READFIFOLVL_REGISTER, &rd_fifo_level);
        if (rc != IFC_SUCCESS) {
            dbg_print(is_debug_on, "ifc_read_epcq_fifo read READFIFOLVL reg failed, ret: %d\n", rc);
            return rc;
        }
        if (rd_fifo_level == 0) {
            dbg_print(is_debug_on, "ifc_read_epcq_fifo, read_fifo_level is zero, read to fpga memory finish, loop: %d\n", i);
            break;
        }
        usleep(FPGA_QSPI_WAIT_FIFO_SLEEP);
    }

    if (i > count) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo, read_fifo_level: 0x%x loop: %d, read to fpga memory timeout\n", rd_fifo_level, i);
        rc = IFC_ERR_FW_CMD_TIMEOUT;
        return rc;
    }

    /* Read data from the FPGA memory space */
    rc = firmware_fpga_read_buf(logic_dev_info->dev_name , logic_dev_info->ctrl_base + IFC_TOP_CPU_FLASH_RDDATA,
            buf, len);
    if (rc < 0) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo read fifo data length: %u failed, ret: %d\n", len, rc);
        rc = IFC_ERR_OP_FAILED;
        return rc;
    }
#if 0
    /* Disable the FIFO read operation. Original factory code. Theoretically, it is not necessary. */
    /* The manual states that "auto clear after operation done", and theoretically there is no need to write "0". */
    ret = ifc_w32(logic_dev_info, IFC_TOP_CPU_CONTROL_REGISTER1, 0);
    if (ret != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo write CONTROL_REGISTER1 to stop read failed, ret: %d\n", ret);
    }
#endif
    dbg_print(is_debug_on, "ifc_read_epcq_fifo read flash offset: 0x%x rd_len: %u success\n", offset, len);
    return IFC_SUCCESS;
}

static int ifc_write_epcq_fifo(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset, uint8_t *buf, uint32_t len)
{
    uint32_t isr_status, rsp_status, wr_fifo_lvl, wr_fifo_words;
    int rc, i, count;

    /* Wait until the FIFO becomes free before starting to operate on the FIFO. */
    rc = ifc_rw_epcq_fifo_idle(logic_dev_info);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo wait fifo idle error, ret: %d\n", rc);
        return rc;
    }

    /* Enable writing */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_WREN_REGISTER, 0x1);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo write WREN reg failed, ret: %d\n", rc);
        return rc;
    }

    /* Empty the FIFO */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_WRITEOP_REGISTER, 0x2);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo write WRITEOP reg to flush write data FIFO failed, ret: %d\n", rc);
        return rc;
    }

    /* Fill the data to be written into the FIFO into the FPGA memory space. */
    rc = firmware_fpga_write_buf(logic_dev_info->dev_name , logic_dev_info->ctrl_base + IFC_TOP_CPU_FLASH_WRDATA,
            buf, len);
    if (rc < 0) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo write fifo data length: %u failed, ret: %d\n", len, rc);
        rc = IFC_ERR_OP_FAILED;
        return rc;
    }

    /* The length of the "write_fifo_words" is the actual number of bytes written to the FIFO divided by 4. A value of 1 indicates that 4 bytes are written. */
    wr_fifo_words = len / 4;
    if (len % 4) {
        wr_fifo_words++;
    }

    /* Start the writing operation of the FIFO. */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_CONTROL_REGISTER0, (wr_fifo_words << 8) | 1);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo write CONTROL_REGISTER0 to write FIFO failed, ret: %d\n", rc);
        return rc;
    }

    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_CONTROL_REGISTER1, 1);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo write CONTROL_REGISTER1 reg to start fifo write failed, ret: %d\n", rc);
        return rc;
    }

    /* Waiting for the FIFO write operation to complete */
    count = FPGA_QSPI_WAIT_FIFO_TIMEOUT / FPGA_QSPI_WAIT_FIFO_SLEEP;
    for (i = 0; i <= count; i++) {
        rc = ifc_r32(logic_dev_info, IFC_TOP_CPU_WRFIFOLVL_REGISTER, &wr_fifo_lvl);
        if (rc != IFC_SUCCESS) {
            dbg_print(is_debug_on, "ifc_write_epcq_fifo read WRFIFOLVL reg failed, ret: %d\n", rc);
            return rc;
        }
        if (wr_fifo_lvl == wr_fifo_words) {
            dbg_print(is_debug_on, "ifc_write_epcq_fifo, write_fifo_level: 0x%x equal write_fifo_words, loop: %d\n", wr_fifo_lvl, i);
            break;
        }
        usleep(FPGA_QSPI_WAIT_FIFO_SLEEP);
    }

    if (i > count) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo wait write fifo finish timeout, write_fifo_words: 0x%x, write_fifo_level: 0x%x loop: %d\n",
            wr_fifo_words, wr_fifo_lvl, i);
        rc = IFC_ERR_FW_CMD_TIMEOUT;
        return rc;
    }
    /* Start writing about flash operations */
    /* Write to flash offset */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_WRADDR_REGISTER, offset);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo write flash offset: 0x%x to WRADDR reg failed, ret: %d\n", offset, rc);
        return rc;
    }

    /* Initiate the flash writing operation */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_WRITEOP_REGISTER, 0x1);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo write WRITEOP reg to start flash write failed, ret: %d\n", rc);
        return rc;
    }

    /* The logic added in this optimization: Wait until all the data in the FIFO has been written to the flash. */
    count = FPGA_QSPI_WAIT_FIFO_TIMEOUT / FPGA_QSPI_WAIT_FIFO_SLEEP;
    for (i = 0; i <= count; i++) {
        rc = ifc_r32(logic_dev_info, IFC_TOP_CPU_WRFIFOLVL_REGISTER, &wr_fifo_lvl);
        if (rc != IFC_SUCCESS) {
            dbg_print(is_debug_on, "ifc_write_epcq_fifo read WRFIFOLVL reg failed, ret: %d\n", rc);
            return rc;
        }
        if (wr_fifo_lvl == 0) {
            dbg_print(is_debug_on, "ifc_write_epcq_fifo, write_fifo_level is zero, write to flash finish, loop: %d\n", i);
            break;
        }
        usleep(FPGA_QSPI_WAIT_FIFO_SLEEP);
    }

    if (i > count) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo, write_fifo_level: 0x%x loop: %d, write to flash timeout\n", wr_fifo_lvl, i);
        rc = IFC_ERR_FW_CMD_TIMEOUT;
        return rc;
    }

    /* Determine whether there are any errors in the ISR/CSR. */
    rc = ifc_r32(logic_dev_info, IFC_TOP_CPU_ISR_REGISTER, &isr_status);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo read ISR reg failed, ret: %d\n", rc);
        return rc;
    }
    rc = ifc_r32(logic_dev_info, IFC_TOP_CPU_CSR_STATUS_REGISTER, &rsp_status);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo read CSR_STATUS reg failed, ret: %d\n", rc);
        return rc;
    }
    if ((isr_status & 0x1) || rsp_status) {
        dbg_print(is_debug_on, "ifc_write_epcq_fifo status error, ISR value: 0x%08x, RSP status: 0x%08x\n",
            isr_status, rsp_status);
        rc = IFC_ERR_FW_CMD_STATUS;
        return rc;
    }
    dbg_print(is_debug_on, "ifc_write_epcq_fifo write flash offset: 0x%x wr_len: %u success\n", offset, len);

   return IFC_SUCCESS;
}

static int ifc_read_epcq_data(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset,
               uint8_t *buf, uint32_t size)
{
    int rc, ret, i, retry;
    uint32_t addr, fifo_size, cnt, rd_per_len;

    /* Turn on the device */
    ret = ifc_w32(logic_dev_info, IFC_TOP_CPU_OPEN_REGISTER, 0x1);
    if (ret != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_data open device failed, ret: %d.\n", ret);
        return ret;
    }

    /* Set chip select */
    ret = ifc_w32(logic_dev_info, IFC_TOP_CPU_CHIPSEL_REGISTER, 0x0);
    if (ret != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_data chip select failed, ret: %d.\n", ret);
        goto fail;
    }

    fifo_size = IFC_TOP_CPU_FIFO_SIZE;           /* read data by FIFO SIZE each time */
    cnt = size / fifo_size;
    if (size % fifo_size) {
        cnt++;
    }
    dbg_print(is_debug_on, "ifc_read_epcq_data need read number of times:%d.\n", cnt);

    for (i = 0; i < cnt; i++) {
        addr = offset + i * fifo_size;
        if (i == (cnt - 1)) {
            /* last time read remain size */
            rd_per_len = size - fifo_size * i;
        } else {
            /* each time read buf page size */
            rd_per_len = fifo_size;
        }

        for (retry = 0; retry < FPGA_UPG_RETRY_TIMES; retry++) {
            ret = ifc_read_epcq_fifo(logic_dev_info, addr, buf, rd_per_len);
            if (ret == IFC_SUCCESS) {
                dbg_print(is_debug_on, "ifc_read_epcq_data read addr: 0x%x, read length: 0x%x success, retry: %d\n",
                    addr, rd_per_len, retry);
                break;
            }
            dbg_print(is_debug_on, "ifc_read_epcq_data read addr:0x%x, read length: 0x%x, read %d time failed, ret %d\n",
                addr, rd_per_len, retry, ret);
            if ((retry + 1) < FPGA_UPG_RETRY_TIMES) {
                /* Turn off the equipment */
                ret = ifc_w32(logic_dev_info, IFC_TOP_CPU_CLOSE_REGISTER, 0x1);
                if (ret != IFC_SUCCESS) {
                    dbg_print(is_debug_on, "ifc_read_epcq_data read fifo retry close device failed, ret: %d.\n", ret);
                    goto fail;
                }
                usleep(FPGA_QSPI_RW_FIFO_RETRY_SLEEP);
                /* Reopen the device */
                ret = ifc_w32(logic_dev_info, IFC_TOP_CPU_OPEN_REGISTER, 0x1);
                if (ret != IFC_SUCCESS) {
                    dbg_print(is_debug_on, "ifc_read_epcq_data read fifo retry open device failed, ret: %d.\n", ret);
                    return ret;
                }
                /* Set chip select */
                ret= ifc_w32(logic_dev_info, IFC_TOP_CPU_CHIPSEL_REGISTER, 0x0);
                if (ret != IFC_SUCCESS) {
                    dbg_print(is_debug_on, "ifc_read_epcq_data read fifo retry chip select failed, ret: %d.\n", ret);
                    goto fail;
                }
            }
        }
        if (ret != IFC_SUCCESS) {
            dbg_print(is_debug_on, "ifc_read_epcq_data finally read addr: 0x%x,  read length: 0x%x,, read failed ret %d\n",
                addr, rd_per_len, ret);
            goto fail;
        }
        buf += rd_per_len;      /* buf pointer offset rd_per_len */
    }
fail:
    /* Turn off the equipment */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_CLOSE_REGISTER, 0x1);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_read_epcq_fifo close device failed, ret: %d.\n", rc);
    }
    return ret;
}

static int ifc_write_epcq_data(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset, uint8_t *buf, uint32_t size)
{
    int rc, ret, i, retry;
    uint32_t addr, fifo_size, cnt, wr_per_len;

    /* Turn on the device */
    ret = ifc_w32(logic_dev_info, IFC_TOP_CPU_OPEN_REGISTER, 0x1);
    if (ret != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_write_epcq_data open device failed, ret: %d.\n", ret);
        return ret;
    }

    /* Set chip select */
    ret = ifc_w32(logic_dev_info, IFC_TOP_CPU_CHIPSEL_REGISTER, 0x0);
    if (ret != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_write_epcq_data chip select failed, ret: %d.\n", ret);
        goto fail;
    }

    fifo_size = IFC_TOP_CPU_FIFO_SIZE;           /* write data by FIFO SIZE each time */
    cnt = size / fifo_size;
    if (size % fifo_size) {
        cnt++;
    }
    dbg_print(is_debug_on, "ifc_write_epcq_data offset: 0x%x, write size: 0x%x, need write number of times:%d.\n",
        offset, size, cnt);

    for (i = 0; i < cnt; i++) {
        addr = offset + i * fifo_size;
        if (i == (cnt - 1)) {
            /* last time write remain size */
            wr_per_len = size - fifo_size * i;
        } else {
            /* each time write buf page size */
            wr_per_len = fifo_size;
        }
        for (retry = 0; retry < FPGA_UPG_RETRY_TIMES; retry++) {
            ret = ifc_write_epcq_fifo(logic_dev_info, addr, buf, wr_per_len);
            if (ret == IFC_SUCCESS) {
                dbg_print(is_debug_on, "ifc_write_epcq_data write addr: 0x%x write length: %u success, retry: %d\n",
                    addr, wr_per_len, retry);
                break;
            }
            dbg_print(is_debug_on, "ifc_write_epcq_data write addr:0x%x, write length: 0x%x, write %d time failed, ret %d\n",
                addr, wr_per_len, retry, ret);
            if ((retry + 1) < FPGA_UPG_RETRY_TIMES) {
                /* Turn off the equipment */
                ret = ifc_w32(logic_dev_info, IFC_TOP_CPU_CLOSE_REGISTER, 0x1);
                if (ret != IFC_SUCCESS) {
                    dbg_print(is_debug_on, "ifc_write_epcq_data write fifo retry close device failed, ret: %d\n", ret);
                    goto fail;
                }
                usleep(FPGA_QSPI_RW_FIFO_RETRY_SLEEP);
                /* Reopen the device */
                ret = ifc_w32(logic_dev_info, IFC_TOP_CPU_OPEN_REGISTER, 0x1);
                if (ret != IFC_SUCCESS) {
                    dbg_print(is_debug_on, "ifc_write_epcq_data write fifo retry open device failed, ret: %d.\n", ret);
                    return ret;
                }
                /* Set chip select */
                ret = ifc_w32(logic_dev_info, IFC_TOP_CPU_CHIPSEL_REGISTER, 0x0);
                if (ret != IFC_SUCCESS) {
                    dbg_print(is_debug_on, "ifc_write_epcq_data write fifo retry chip select failed, ret: %d.\n", ret);
                    goto fail;
                }
            }
        }
        if (ret < 0) {
            dbg_print(is_debug_on, "ifc_write_epcq_data finally write addr: 0x%x,  write length: 0x%x,, write failed ret %d\n",
                addr, wr_per_len, ret);
            goto fail;
        }
        buf += wr_per_len;      /* buf pointer offset wr_per_len */
    }
fail:
   /* Turn off the equipment */
   rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_CLOSE_REGISTER, 0x1);
   if (rc != IFC_SUCCESS) {
       dbg_print(is_debug_on, "ifc_write_epcq_data close device failed, ret: %d.\n", rc);
   }
   return ret;
}

static int ifc_erase_epcq_sect(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset)
{
    uint32_t flash_status;
    int rc, ret, i, count;

    dbg_print(is_debug_on, "ifc_erase_epcq_sect start to erase sector: 0x%x.\n", offset);
    /* Turn on the device */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_OPEN_REGISTER, 0x1);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_erase_epcq_sect open device failed, ret: %d.\n", rc);
        return rc;
    }

    /* Set chip select */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_CHIPSEL_REGISTER, 0x0);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_erase_epcq_sect chip select failed, ret: %d.\n", rc);
        goto fail;
    }

    /* Enable writing */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_WREN_REGISTER, 0x1);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_erase_epcq_sect write enable failed, ret: %d.\n", rc);
        goto fail;
    }

    /* Write the flash address to be erased */
    rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_SE_REGISTER, offset);
    if (rc != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_erase_epcq_sect write erase addr: 0x%x failed, ret: %d.\n", offset, rc);
        goto fail;
    }

    /* By sending the read flash status command word (RDSR:05) through the control register, it is possible to confirm whether the erasure is complete. */
    count = FPGA_QSPI_SE_TIMEOUT / FPGA_QSPI_SE_SLEEP_TIME;
    for (i = 0; i <= count; i++) {
        /* Read 4 bytes of data */
        rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_NUMBYTES_REGISTER, 0x4);
        if (rc != IFC_SUCCESS) {
            dbg_print(is_debug_on, "ifc_erase_epcq_sect write NUMBYTES reg failed, ret: %d.\n", rc);
            goto fail;
        }
        /* Write the control register to initiate the read status operation */
        rc = ifc_w32(logic_dev_info, IFC_TOP_CPU_CONTROL_REGISTER, FPGA_QSPI_RDSR_OPCODE);
        if (rc != IFC_SUCCESS) {
            dbg_print(is_debug_on, "ifc_erase_epcq_sect write CONTROL reg value: 0x%x failed, ret: %d.\n",
                FPGA_QSPI_RDSR_OPCODE, rc);
            goto fail;
        }
        /* Read the flash status */
        rc = ifc_r32(logic_dev_info, IFC_TOP_CPU_RDDATA0_REGISTER, &flash_status);
        if (rc != IFC_SUCCESS) {
            dbg_print(is_debug_on, "ifc_erase_epcq_sect read RDDATA0 reg failed, ret: %d.\n", rc);
            goto fail;
        }
        dbg_print(is_debug_on, "ifc_erase_epcq_sect read RDDATA0 reg success, flash status: 0x%x.\n", flash_status);
        /* Check if it is still in the erasing process. */
        if ((flash_status != FPGA_RDDATA_INV_VALUE) && ((flash_status & FPGA_QSPI_SR_WIP) == 0)) {
            dbg_print(is_debug_on, "ifc_erase_epcq_sect erase sector: 0x%x success, loop: %d\n", offset, i);
            break;
        }
        usleep(FPGA_QSPI_SE_SLEEP_TIME);
    }

    if (i > count) {
        dbg_print(is_debug_on, "ifc_erase_epcq_sect 0x%x wait sector erase finish timeout, loop: %d\n", offset, i);
        rc = IFC_ERR_FW_CMD_TIMEOUT;
    }
fail:
    /* Turn off the equipment */
    ret= ifc_w32(logic_dev_info, IFC_TOP_CPU_CLOSE_REGISTER, 0x1);
    if (ret != IFC_SUCCESS) {
        dbg_print(is_debug_on, "ifc_erase_epcq_sect close device failed, ret: %d.\n", ret);
    }
    return rc;
}

static int ifc_do_upgrade(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset, uint8_t *buf, uint32_t size)
{
    int ret;
    uint32_t ind;
    uint8_t *read_buff;
    clock_t t1, t2, t3;

    dbg_print(is_debug_on, "ifc_do_upgrade offset: %x size: %x\n", offset, size);
    /* Flash erasure operation */
    dbg_print(is_debug_on, "Erasing EPCQ flash...\n");
    t1 = clock();
    for (ind = offset ; ind < offset + size; ind += FIRMWARE_SPI_INTEL_LOGIC_SECTOR_SIZE) {
        ret = ifc_erase_epcq_sect(logic_dev_info, ind);
        if (ret != IFC_SUCCESS) {
            dbg_print(is_debug_on,"Erase sector: 0x%x failed, ret: %d\n", ind, ret);
            return ret;
        }
    }
    t1 = clock() - t1;
    dbg_print(is_debug_on, "Erase Time: %f seconds\n", ((double)t1)/CLOCKS_PER_SEC);
    dbg_print(is_debug_on, "Erase EPCQ flash complete.\n");

    /* Flash write operation */
    dbg_print(is_debug_on, "Programming EPCQ flash...\n");
    t2 = clock();
    ret = ifc_write_epcq_data(logic_dev_info, offset, buf, size);
    if (ret != IFC_SUCCESS) {
        dbg_print(is_debug_on, "fpga write failed, offset: 0x%x, write length: 0x%x, ret: %d\n",
            offset, size, ret);
        return ret;
    }
    t2 = clock() - t2;
    dbg_print(is_debug_on, "Write Time: %f seconds\n", ((double)t2)/CLOCKS_PER_SEC);
    dbg_print(is_debug_on, "Programming EPCQ flash Complete...\n");

    /* Flash readback verification */
    dbg_print(is_debug_on, "Verifying EPCQ flash ...\n");
    read_buff = malloc(size);
    if(!read_buff) {
        dbg_print(is_debug_on, "malloc read buffer size: 0x%x failed\n", size);
        return IFC_ERR_NO_MEMORY;
    }
    memset(read_buff, 0, size);
    t3 = clock();
    ret = ifc_read_epcq_data(logic_dev_info, offset, read_buff, size);
    if (ret != IFC_SUCCESS) {
        dbg_print(is_debug_on, "fpga read failed, offset: 0x%x, read length: 0x%x, ret: %d\n",
            offset, size, ret);
        free(read_buff);
        return IFC_ERR_OP_FAILED;
    }
    t3 = clock() - t3;
    dbg_print(is_debug_on, "Read Back Time: %f seconds\n", ((double)t3)/CLOCKS_PER_SEC);
    dbg_print(is_debug_on, "Read Back EPCQ flash Complete...\n");

    for (ind = 0 ; ind < size; ind += 4) {
        if ((*(uint32_t*)(read_buff + ind)) != (*(uint32_t*)(buf + ind))) {
            dbg_print(is_debug_on, "Image verification failed at 0x%08x: [EPCQ: 0x%08x Img_file: 0x%08x]\n",
                offset + ind, *(uint32_t*)(read_buff + ind), *(uint32_t*)(buf + ind));
            free(read_buff);
            return IFC_ERR_OP_FAILED;
        }
    }
    dbg_print(is_debug_on, "Verify EPCQ flash Complete\n");
    free(read_buff);
    return IFC_SUCCESS;
}

int firmware_upgrade_do_spi_intel_logic(firmware_spi_logic_info_t *logic_dev_info, uint8_t *buf, uint32_t size)
{
    int i, ret, retry;
    uint32_t img_off;

    if ((logic_dev_info == NULL) || (buf == NULL) || (size == 0)) {
        dbg_print(is_debug_on, "firmware_upgrade_do_spi_intel_logic params error, size: 0x%x, logic_dev_info or buf maybe is NULL.\n",
            size);
        return IFC_ERR_PARAM;
    }

    img_off = logic_dev_info->flash_base;
    if ((img_off % FIRMWARE_SPI_INTEL_LOGIC_SECTOR_SIZE) != 0) {
        dbg_print(is_debug_on, "Upgrade base address: 0x%x not aligned, erase size: 0x%x\n",
            img_off, FIRMWARE_SPI_INTEL_LOGIC_SECTOR_SIZE);
        return IFC_ERR_PARAM;
    }

    retry = FIRMWARE_SPI_LOGIC_UPG_RETRY_CNT;
    for (i = 0; i < retry; i++) {
        ret = ifc_do_upgrade(logic_dev_info, img_off, buf, size);
        if (ret == IFC_SUCCESS) {
            dbg_print(is_debug_on, "Upgrade fpga success, offset: 0x%x, size: 0x%x, retry: %d\n", img_off, size, i);
            return ret;
        }
        dbg_print(is_debug_on, "Upgrade fpga failed, offset: 0x%x, size: 0x%x, ret %d, retry: %d\n", img_off, size, ret, i);
    }
    dbg_print(is_debug_on, "Finally upgrade fpga failed, offset: 0x%x, size: 0x%x, ret %d, retry: %d\n", img_off, size, ret, i);
    return ret;
}

int firmware_intel_fpga_upgrade_test(firmware_spi_logic_info_t *logic_dev_info)
{
    int ret, num, i, retry;
    uint8_t *wbuf;
    uint32_t test_base, test_size;

    if (logic_dev_info == NULL) {
        dbg_print(is_debug_on, "firmware_intel_fpga_upgrade_test params error, logic_dev_info is NULL.\n");
        return IFC_ERR_PARAM;
    }

    test_base = logic_dev_info->test_base;
    test_size = logic_dev_info->test_size;
    /* The flash for the test needs to be aligned according to the size of the erased sectors. */
    if ((test_base % FIRMWARE_SPI_INTEL_LOGIC_SECTOR_SIZE) != 0) {
        dbg_print(is_debug_on, "Error: test base = 0x%x, test size = 0x%x, sector size = 0x%x, not aligned\n",
            test_base, test_size, FIRMWARE_SPI_INTEL_LOGIC_SECTOR_SIZE);
        return IFC_ERR_PARAM;
    }

    if (test_size == 0) {
        dbg_print(is_debug_on, "Error test base = 0x%x, test size is zero\n", test_base);
        return IFC_ERR_PARAM;
    }

    wbuf = (uint8_t *) malloc(test_size);
    if (wbuf == NULL) {
        dbg_print(is_debug_on, "Error: Failed to malloc memory for test write data buf, size=0x%x.\n", test_size);
        return IFC_ERR_NO_MEMORY;
    }

    memset(wbuf, 0, test_size);
    /* Get random data */
    srand(time(NULL));
    for (i = 0; i < test_size; i++) {
        num = rand() % 256;
        wbuf[i] = num & 0xff;
    }
    dbg_print(is_debug_on, "Start to upgrade fpga test, test base: 0x%x, test size: 0x%x\n", test_base, test_size);

    retry = FIRMWARE_SPI_LOGIC_UPG_RETRY_CNT;
    for (i = 0; i < retry; i++) {
        ret = ifc_do_upgrade(logic_dev_info, test_base, wbuf, test_size);
        if (ret == IFC_SUCCESS) {
            dbg_print(is_debug_on, "Upgrade fpga test success, offset: 0x%x, size: 0x%x, retry: %d\n", test_base, test_size, i);
            free(wbuf);
            return ret;
        }
        dbg_print(is_debug_on, "Upgrade fpga test failed, offset: 0x%x, size: 0x%x, ret %d, retry: %d\n", test_base, test_size, ret, i);
    }
    dbg_print(is_debug_on, "Finally upgrade fpga test failed, offset: 0x%x, size: 0x%x, ret %d, retry: %d\n", test_base, test_size, ret, i);
    free(wbuf);
    return ret;
}

int firmware_upgreade_spi_intel_logic_dump(firmware_spi_logic_info_t *logic_dev_info,
        uint32_t offset, uint8_t *buf, uint32_t rd_size)
{
    int ret;

    if ((logic_dev_info == NULL) || (buf == NULL)) {
        dbg_print(is_debug_on, "dump fpga params error, logic_dev_info or buf maybe is NULL.\n");
        return IFC_ERR_PARAM;
    }

    if (rd_size == 0) {
        dbg_print(is_debug_on, "dump fpga params error, rd_size is zero.\n");
        return IFC_ERR_PARAM;
    }

    if (offset % 4) {
        dbg_print(is_debug_on, "dump fpga params error, offset: 0x%x not 4 byte alignment.\n", offset);
        return IFC_ERR_PARAM;
    }

    ret= ifc_read_epcq_data(logic_dev_info, offset, buf, rd_size);
    if (ret != IFC_SUCCESS) {
        dbg_print(is_debug_on, "dump fpga read failed, offset: 0x%x, read size:0x%x, ret: %d\n", offset, rd_size, ret);
    } else {
        dbg_print(is_debug_on, "dump fpga read offset: 0x%x, read size:0x%x, success\n", offset, rd_size);
    }

    return ret;
}
