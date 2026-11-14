#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <asm/byteorder.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <linux/version.h>

#include <firmware_app.h>
#include "firmware_upgrade_mailbox.h"

static inline uint8_t get_uint8_bit(uint8_t value, int bit)
{
    return (value >> bit) & 1;
}

static inline uint8_t revert_bit(uint8_t data)
{
    uint8_t out;

    out = (get_uint8_bit(data, 0) << 7) | (get_uint8_bit(data, 1) << 6) |
          (get_uint8_bit(data, 2) << 5) | (get_uint8_bit(data, 3) << 4) |
          (get_uint8_bit(data, 4) << 3) | (get_uint8_bit(data, 5) << 2) |
          (get_uint8_bit(data, 6) << 1) | (get_uint8_bit(data, 7) << 0);
    return out;
}

static void revert_bit_buf(uint8_t *buf, uint32_t length)
{
    size_t i;

    for (i = 0; i < length; i++) {
        *buf = revert_bit(*buf);
        buf++;
    }
}

static int firmware_sysfs_get_spi_info(int fd, firmware_spi_logic_info_t *dev_info)
{
    int ret;

    ret = ioctl(fd, FIRMWARE_SYSFS_SPI_INFO, dev_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "Failed to get upg flash dev info.\n");
        return ret;
    }

    dbg_print(is_debug_on, "sysfs_name=%s flash_base=%u.\n", dev_info->dev_name, dev_info->flash_base);
    return 0;
}

static int fpga_mailbox_r32(firmware_spi_logic_info_t *logic_dev_info, uint32_t reg_offset, uint32_t *val)
{
    int ret;
    uint32_t addr;
    char *dev_name;

    dev_name = logic_dev_info->dev_name;
    addr = logic_dev_info->ctrl_base + reg_offset;
    ret = firmware_fpga_read_word(dev_name, addr, val);
    if (ret < 0) {
        dbg_print(is_debug_on, "fpga_mailbox_r32 read failed, dev_name: %s, addr 0x%x, ret %d.\n",
            dev_name, addr, ret);
        return MB_EFILEIO;
    }
    dbg_print(is_debug_on, "fpga_mailbox_r32 read success, dev_name: %s addr: 0x%x, read value: 0x%x.\n",
        dev_name, addr, *val);
    return MB_SUCCESS;
}

static int fpga_mailbox_w32(firmware_spi_logic_info_t *logic_dev_info, uint32_t reg_offset, uint32_t val)
{
    int ret;
    uint32_t addr;
    char *dev_name;

    dev_name = logic_dev_info->dev_name;
    addr = logic_dev_info->ctrl_base + reg_offset;
    ret = firmware_fpga_write_word(dev_name, addr, val);
    if (ret < 0) {
        dbg_print(is_debug_on, "fpga_mailbox_w32 write failed, dev_name: %s, addr 0x%x, val 0x%x, ret %d.\n",
            dev_name, addr, val, ret);
        return MB_EFILEIO;
    }
    dbg_print(is_debug_on, "fpga_mailbox_w32 write success, dev_name: %s addr: 0x%x, write value: 0x%x.\n",
        dev_name, addr, val);
    return MB_SUCCESS;
}

static int fpga_mailbox_wr_cmd(firmware_spi_logic_info_t *logic_dev_info, uint32_t cmd_len, uint32_t *cmd, bool write_flash)
{
    int ret;
    uint32_t fifo_space = 0;
    int i;

    /* wait enough fifo space */ 
    for (i = 0; i <= MAILBOX_READ_MAX; i++) {
        ret = fpga_mailbox_r32(logic_dev_info, MAILBOX_COMMAND_FIFO, &fifo_space);
        if ((ret == MB_SUCCESS) && (fifo_space > 0)) {
            break;
        }
    }
    if ((ret < 0) || (fifo_space <= 0)) {
        dbg_print(is_debug_on, "fpga_mailbox fifo space not enough, fifo_space: 0x%x, ret: %d\n", fifo_space, ret);
        return MB_ESIZE;
    }
  
    for (i = 0 ; i < cmd_len; i++) {
        ret = fpga_mailbox_w32(logic_dev_info, MAILBOX_COMMAND, *cmd);
        if (ret != MB_SUCCESS) {
            dbg_print(is_debug_on,"write cmd 0x%x to mailbox 0x%x failed, ret: %d\n", *cmd, MAILBOX_COMMAND, ret);
            return ret;
        }
        cmd++;
    }

    if (write_flash) {
        ret = fpga_mailbox_w32(logic_dev_info, MAILBOX_COMMAND, *cmd);
        if (ret != MB_SUCCESS) {
            dbg_print(is_debug_on,"write cmd 0x%x to mailbox 0x%x failed, ret: %d\n", *cmd, MAILBOX_COMMAND, ret);
            return ret;
        }
    } else {
        ret = fpga_mailbox_w32(logic_dev_info, MAILBOX_EOP, *cmd);
        if (ret != MB_SUCCESS) {
            dbg_print(is_debug_on,"write cmd 0x%x to mailbox 0x%x failed, ret: %d\n", *cmd, MAILBOX_COMMAND, ret);
            return ret;
        }
    }
    return MB_SUCCESS;
}

/* clean the fifo response cache */
static void cleanup_response_FIFO(firmware_spi_logic_info_t *logic_dev_info, uint32_t level_cnt)
{
    uint32_t i;
    __attribute__ ((unused)) uint32_t val;

    for (i = 0; i < level_cnt; i++) {
        /* drop val */
        fpga_mailbox_r32(logic_dev_info, MAILBOX_RESPONSE_DATA, &val);
    }
}

static int fpga_mailbox_read_response(firmware_spi_logic_info_t *logic_dev_info, uint32_t *response)
{
    int ret;
    uint32_t isr_reg = 0, fifo_level_reg = 0;
    int i;

    /* wait ISR right */
    for (i = 0; i <= MAILBOX_READ_MAX; i++) {
        ret = fpga_mailbox_r32(logic_dev_info, MAILBOX_ISR, &isr_reg);
        if ((ret == MB_SUCCESS) && (isr_reg == MAILBOX_ISR_RIGHT)) {
            break;
        }
    }

    if ((ret < 0) || (isr_reg != MAILBOX_ISR_RIGHT)) {
        dbg_print(is_debug_on, "fpga_mailbox ISR code err, ISR: 0x%x, ret: %d\n", isr_reg, ret);
        return MB_ECALLBACK;
    }

    /* wait SOP right */
    for (i = 0; i <= MAILBOX_READ_MAX; i++) {
        ret = fpga_mailbox_r32(logic_dev_info, MAILBOX_RESPONSE_FIFO, &fifo_level_reg);
        if ((ret == MB_SUCCESS) && (fifo_level_reg > 0)) {
            break;
        }
    }

    if ((ret < 0) || !(fifo_level_reg & MAILBOX_SOP_RIGHT)) {
        dbg_print(is_debug_on, "fpga_mailbox fifo level code err, fifo level: 0x%x, ret: %d\n", fifo_level_reg, ret);
        if ((fifo_level_reg >> 2) >= 1) {
            dbg_print(is_debug_on, "fpga_mailbox clean fifo \n");
            cleanup_response_FIFO(logic_dev_info, fifo_level_reg >> 2);
        }
        return MB_ECALLBACK;
    }

    /* read response */
    ret = fpga_mailbox_r32(logic_dev_info, MAILBOX_RESPONSE_DATA, response);
    if (ret != MB_SUCCESS){
        dbg_print(is_debug_on, "fpga_mailbox read response fail, reg: 0x%x, ret: %d\n", MAILBOX_RESPONSE_DATA, ret);
        return MB_EFILEIO;
    }
    dbg_print(is_debug_on, "fpga_mailbox read response success, reg: 0x%x, response: 0x%x\n", MAILBOX_RESPONSE_DATA, *response);
    return MB_SUCCESS;
}

static int qspi_operation(firmware_spi_logic_info_t *logic_dev_info, QSPI_OP_TYPE operation) 
{
    int ret;
    uint32_t command_length;
    uint32_t command[QSPI_CMD_MAX];
    uint32_t response_header = 0xFF;

    switch (operation) {
    case QSPI_OPEN:
        command_length = 0;
        command[0] = 0x32;
        ret = fpga_mailbox_wr_cmd(logic_dev_info, command_length, command, false);
        break;
    case QSPI_CLOSE:
        command_length = 0;
        command[0] = 0x33;
        ret = fpga_mailbox_wr_cmd(logic_dev_info, command_length, command, false);
        break;
    case QSPI_SET_CS:
        command_length = 1;
        command[0] = 0x1034;
        command[1] = 0;
        ret = fpga_mailbox_wr_cmd(logic_dev_info, command_length, command, false);
        break;
    default:
        break;
    }
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "qspi_operation write cmd error, operation: %d, ret: %d\n", operation, ret);
        return ret;
    }

    ret = fpga_mailbox_read_response(logic_dev_info, &response_header);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "qspi_operation read response error, operation: %d, ret: %d\n", operation, ret);
        return ret;
    }

    if (response_header) {
        dbg_print(is_debug_on, "qspi_operation response header error, operation: %d, response: 0x%x\n", operation, response_header);
        return MB_ECALLBACK;
    }
    dbg_print(is_debug_on, "qspi_operation response header right, operation: %d, response: 0x%x\n", operation, response_header);
    return MB_SUCCESS;
}

static int fpga_mailbox_erase_epcq_sect(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset)
{
    uint32_t response_header = 0xFF;
    uint32_t command_length;
    uint32_t command[QSPI_CMD_MAX];
    int rc, ret;
    
    /* open device */
    rc = qspi_operation(logic_dev_info, QSPI_OPEN);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_erase_epcq_sect qspi open device failed, ret: %d.\n", rc);
        goto error;
    }
    /* set cs */
    rc = qspi_operation(logic_dev_info, QSPI_SET_CS);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_erase_epcq_sect qspi set cs failed, ret: %d.\n", rc);
        goto error;
    }

    /* erase */
    command_length = 2;
    command[0] = 0x2038;
    command[1] = offset;
    command[2] = 0x4000;
    rc = fpga_mailbox_wr_cmd(logic_dev_info, command_length, command, false);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_erase_epcq_sect qspi erase flash error, offset: 0x%x, ret: %d.\n", offset, rc);
        goto error;
    }

    /* read response */
    rc = fpga_mailbox_read_response(logic_dev_info, &response_header);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_erase_epcq_sect read response error, ret: %d\n", rc);
        goto error;
    }

    if (response_header) {
        dbg_print(is_debug_on, "fpga_mailbox_erase_epcq_sect response header error, response: 0x%x\n", response_header);
        rc = MB_ECALLBACK;
        goto error;
    }
    /* close device */
    ret = qspi_operation(logic_dev_info, QSPI_CLOSE);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_erase_epcq_sect qspi close device failed, ret: %d.\n", rc);
        return ret;
    }
    return MB_SUCCESS;
error:
    ret = qspi_operation(logic_dev_info, QSPI_CLOSE);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_erase_epcq_sect qspi close device failed, ret: %d.\n", ret);
    }
    return rc;
}

static int fpga_mailbox_read_epcq_data_word(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset, uint32_t *buf, uint32_t size)
{
    uint32_t response_header = 0xFF;
    uint32_t command_length;
    uint32_t command[QSPI_CMD_MAX];
    uint32_t remain;
    uint32_t *buf_tmp;
    int rc, ret, i;
    uint32_t fifo_level_reg = 0;
    uint32_t fifo_level = 0;

    remain = size;
    buf_tmp = buf;

    /* open device */
    rc = qspi_operation(logic_dev_info, QSPI_OPEN);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_read_epcq_data_word qspi open device failed, ret: %d.\n", rc);
        goto error;
    }
    /* set cs */
    rc = qspi_operation(logic_dev_info, QSPI_SET_CS);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_read_epcq_data_word qspi set cs failed, ret: %d.\n", rc);
        goto error;
    }

    command_length = 2;
    command[0] = 0x203a;
    command[1] = offset;
    command[2] = remain;
    rc = fpga_mailbox_wr_cmd(logic_dev_info, command_length, command, false);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_read_epcq_data_word qspi write cmd error, offset: 0x%x, ret: %d.\n", offset, rc);
        goto error;
    }

    /* read response */
    rc = fpga_mailbox_read_response(logic_dev_info, &response_header);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_read_epcq_data_word read response error, ret: %d\n", rc);
        goto error;
    }

    if (response_header & 0x7ff) {
        dbg_print(is_debug_on, "fpga_mailbox_read_epcq_data_word response header error, response: 0x%x\n", response_header);
        rc = MB_ECALLBACK;
        goto error;
    }

    while (remain) {
        /* wait response fifo */ 
        for (i = 0; i <= MAILBOX_READ_MAX; i++) {
            rc = fpga_mailbox_r32(logic_dev_info, MAILBOX_RESPONSE_FIFO, &fifo_level_reg);
            if ((rc == MB_SUCCESS) && ((fifo_level_reg >> 2) > 0)) {
                break;
            }
        }
        if ((rc < 0) || ((fifo_level_reg >> 2) <= 0)) {
            dbg_print(is_debug_on, "fpga_mailbox_read_epcq_data_word response fifo empty, fifo_level_reg: 0x%x, ret: %d\n", fifo_level_reg, rc);
            goto error;
        }
        fifo_level = fifo_level_reg >> 2;
        while (fifo_level > 0 && remain > 0)
        {
            rc = fpga_mailbox_r32(logic_dev_info, MAILBOX_RESPONSE_DATA, buf_tmp);
            if (rc != MB_SUCCESS){
                dbg_print(is_debug_on, "fpga_mailbox_read_epcq_data_word read response fail, reg: 0x%x, ret: %d\n", MAILBOX_RESPONSE_DATA, rc);
                goto error;
            }
            buf_tmp++;
            remain--;
            fifo_level--;
        }   
    }
    
    /* close device */
    ret = qspi_operation(logic_dev_info, QSPI_CLOSE);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_read_epcq_data_word qspi close device failed, ret: %d.\n", ret);
        return ret;
    }
    return MB_SUCCESS;

error:
    ret = qspi_operation(logic_dev_info, QSPI_CLOSE);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_read_epcq_data_word qspi close device failed, ret: %d.\n", ret);
    }
    return rc;  
}

static int fpga_mailbox_write_flash_by_fifo(firmware_spi_logic_info_t *logic_dev_info, uint32_t *buf, uint32_t *remain)
{
    int i, ret;
    uint32_t fifo_space = 0;

    /* wait enough fifo space */ 
    for (i = 0; i <= MAILBOX_READ_MAX; i++) {
        ret = fpga_mailbox_r32(logic_dev_info, MAILBOX_COMMAND_FIFO, &fifo_space);
        if ((ret == MB_SUCCESS) && (fifo_space > 0)) {
            break;
        }
    }

    if ((ret < 0) || (fifo_space <= 0)) {
        dbg_print(is_debug_on, "fpga_mailbox_write_flash_by_fifo fifo space not enough, fifo_space: 0x%x, ret: %d\n", fifo_space, ret);
        return MB_ESIZE;
    }

    while ((*remain > 1) && (fifo_space > 0)) {
        ret = fpga_mailbox_w32(logic_dev_info, MAILBOX_COMMAND, *buf);
        if (ret != MB_SUCCESS) {
            dbg_print(is_debug_on, "fpga_mailbox_write_flash_by_fifo write buf to flash error, reg: 0x%x, ret: %d.\n", MAILBOX_COMMAND, ret);
            return ret;
        }
        (*remain)--;
        fifo_space--;
        buf++;
    }

    if ((*remain) == 1) {
        /* write last word */
        ret = fpga_mailbox_w32(logic_dev_info, MAILBOX_EOP, *buf);
        if (ret != MB_SUCCESS) {
            dbg_print(is_debug_on, "fpga_mailbox_write_flash_by_fifo write buf to flash error, reg: 0x%x, ret: %d.\n", MAILBOX_EOP, ret);
            return ret;
        }
        (*remain)--;
        buf++;
    }

    return ret;
}

static int fpga_mailbox_write_epcq_data_word(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset, uint32_t *buf, uint32_t size)
{
    uint32_t qspi_write;
    uint32_t response_header = 0xFF;
    uint32_t command_length;
    uint32_t command[QSPI_CMD_MAX];
    uint32_t remain, remain_temp;
    uint32_t *buf_temp;
    int rc, ret;

    qspi_write = 0x39;
    remain = size;
    buf_temp = buf;

    /* open device */
    rc = qspi_operation(logic_dev_info, QSPI_OPEN);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_write_epcq_data_word qspi open device failed, ret: %d.\n", rc);
        goto error;
    }
    /* set cs */
    rc = qspi_operation(logic_dev_info, QSPI_SET_CS);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_write_epcq_data_word qspi set cs failed, ret: %d.\n", rc);
        goto error;
    }

    qspi_write |= ((remain + 2) << 12);
    command_length = 2;
    command[0] = qspi_write;
    command[1] = offset;
    command[2] = remain;
    rc = fpga_mailbox_wr_cmd(logic_dev_info, command_length, command, true);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_write_epcq_data_word qspi write cmd error, offset: 0x%x, ret: %d.\n", offset, rc);
        goto error;
    }
    while (remain > 0) {
        remain_temp = remain;
        rc = fpga_mailbox_write_flash_by_fifo(logic_dev_info, buf_temp, &remain);
        if (rc != MB_SUCCESS) {
            dbg_print(is_debug_on, "fpga_mailbox_write_epcq_data_word write buf to flash error, ret: %d.\n", rc);
            goto error;
        }
        buf_temp += (remain_temp - remain);
    }
    
    /* read response */
    rc = fpga_mailbox_read_response(logic_dev_info, &response_header);
    if (rc != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_write_epcq_data_word read response error, ret: %d\n", rc);
        goto error;
    }

    if (response_header) {
        dbg_print(is_debug_on, "fpga_mailbox_write_epcq_data_word response header error, response: 0x%x\n", response_header);
        rc = MB_ECALLBACK;
        goto error;
    }
    /* close device */
    ret = qspi_operation(logic_dev_info, QSPI_CLOSE);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_write_epcq_data_word qspi close device failed, ret: %d.\n", ret);
        return ret;
    }
    return MB_SUCCESS;

error:
    ret = qspi_operation(logic_dev_info, QSPI_CLOSE);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga_mailbox_write_epcq_data_word qspi close device failed, ret: %d.\n", ret);
    }
    return rc;
}

static int fpga_mailbox_write_epcq_data(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset, uint8_t *buf, uint32_t size)
{
    uint32_t *tmp_buf;
    uint32_t tmp_offset;
    uint32_t word_remain;
    uint32_t count;
    int ret;

    revert_bit_buf(buf, size);
    tmp_buf = (uint32_t *)buf;
    tmp_offset = offset;
    word_remain = size / sizeof(uint32_t);

    while (word_remain) {
        count = word_remain < FLASH_PAGE_SIZE ? word_remain : FLASH_PAGE_SIZE;
  
        ret = fpga_mailbox_write_epcq_data_word(logic_dev_info, tmp_offset, tmp_buf, count);
        if (ret != MB_SUCCESS) {
            dbg_print(is_debug_on, "fpga_mailbox_write_epcq_data write data word failed, offset: 0x%x, write length: 0x%x, ret: %d\n",
                tmp_offset, count, ret);
            return ret;
        }
        word_remain -= count;
        tmp_offset += count * sizeof(uint32_t);
        tmp_buf += count;
    }
    revert_bit_buf(buf, size);
    return MB_SUCCESS;
}

static int fpga_mailbox_read_epcq_data(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset, uint8_t *buf, uint32_t size)
{
    uint32_t *tmp_buf;
    uint32_t tmp_offset;
    uint32_t word_remain;
    uint32_t count;
    int ret;

    tmp_buf = (uint32_t *)buf;
    tmp_offset = offset;
    word_remain = size / sizeof(uint32_t);

    while (word_remain) {
        count = word_remain < FLASH_PAGE_SIZE ? word_remain : FLASH_PAGE_SIZE;
        ret = fpga_mailbox_read_epcq_data_word(logic_dev_info, tmp_offset, tmp_buf, count);
        if (ret != MB_SUCCESS) {
            dbg_print(is_debug_on, "fpga_mailbox_read_epcq_data read data word failed, offset: 0x%x, write length: 0x%x, ret: %d\n",
                tmp_offset, count, ret);
            return ret;
        }
        word_remain -= count;
        tmp_offset += count * sizeof(uint32_t);
        tmp_buf += count;
    }
    revert_bit_buf(buf, size);
    return MB_SUCCESS;
}

static int fpga_mailbox_do_upgrade(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset, uint8_t *buf, uint32_t size)
{
    int ret;
    uint32_t ind;
    uint8_t *read_buff;
    clock_t t1, t2, t3;

    dbg_print(is_debug_on, "fpga_mailbox_do_upgrade offset: %x size: %x\n", offset, size);
    /* Flash eare */
    dbg_print(is_debug_on, "Erasing EPCQ flash...\n");
    t1 = clock();
    for (ind = offset ; ind < offset + size; ind += MAILBOX_INTEL_LOGIC_SECTOR_SIZE) {
        ret = fpga_mailbox_erase_epcq_sect(logic_dev_info, ind);
        if (ret != MB_SUCCESS) {
            dbg_print(is_debug_on,"Erase sector: 0x%x failed, ret: %d\n", ind, ret);
            return MB_EERASE;
        }
    }
    t1 = clock() - t1;
    dbg_print(is_debug_on, "Erase Time: %f seconds\n", ((double)t1)/CLOCKS_PER_SEC);
    dbg_print(is_debug_on, "Erase EPCQ flash complete.\n");

    /* Flash write */
    dbg_print(is_debug_on, "Programming EPCQ flash...\n");
    t2 = clock();
    ret = fpga_mailbox_write_epcq_data(logic_dev_info, offset, buf, size);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga write failed, offset: 0x%x, write length: 0x%x, ret: %d\n",
            offset, size, ret);
        return MB_EPROGRAM;
    }
    t2 = clock() - t2;
    dbg_print(is_debug_on, "Write Time: %f seconds\n", ((double)t2)/CLOCKS_PER_SEC);
    dbg_print(is_debug_on, "Programming EPCQ flash Complete...\n");

    /* Flash check */
    dbg_print(is_debug_on, "Verifying EPCQ flash ...\n");
    read_buff = malloc(size);
    if(!read_buff) {
        dbg_print(is_debug_on, "malloc read buffer size: 0x%x failed\n", size);
        return MB_ESIZE;
    }
    memset(read_buff, 0, size);
    t3 = clock();
    ret = fpga_mailbox_read_epcq_data(logic_dev_info, offset, read_buff, size);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "fpga read failed, offset: 0x%x, read length: 0x%x, ret: %d\n",
            offset, size, ret);
        free(read_buff);
        return MB_ECMP;
    }
    t3 = clock() - t3;
    dbg_print(is_debug_on, "Read Back Time: %f seconds\n", ((double)t3)/CLOCKS_PER_SEC);
    dbg_print(is_debug_on, "Read Back EPCQ flash Complete...\n");

    for (ind = 0 ; ind < size; ind += 4) {
        if ((*(uint32_t*)(read_buff + ind)) != (*(uint32_t*)(buf + ind))) {
            dbg_print(is_debug_on, "Image verification failed at 0x%08x: [EPCQ: 0x%08x Img_file: 0x%08x]\n",
                offset + ind, *(uint32_t*)(read_buff + ind), *(uint32_t*)(buf + ind));
            free(read_buff);
            return MB_EPROGRAM;
        }
    }
    dbg_print(is_debug_on, "Verify EPCQ flash Complete\n");
    free(read_buff);
    return MB_SUCCESS;
}

static int reset_mailbox(firmware_spi_logic_info_t *logic_dev_info)
{
    uint32_t reset_ops;
    int ret;

    ret = firmware_fpga_read_word(logic_dev_info->dev_name, CHIP_LEVEL_RESET, &reset_ops);
    if (ret < 0) {
        dbg_print(is_debug_on, "reset_mailbox read logic_dev failed, reg: 0x%x ret: %d\n",
            CHIP_LEVEL_RESET, ret);
        return MB_EPROGRAM;
    }
    reset_ops &= ~(RESET_MAILBOX_MASK);
    reset_ops |= RESET_MAILBOX_ON;
    ret = firmware_fpga_write_word(logic_dev_info->dev_name, CHIP_LEVEL_RESET, reset_ops);
    if (ret < 0) {
        dbg_print(is_debug_on, "fpga_mailbox_w32 write failed, reg 0x%x, val 0x%x, ret %d.\n",
            CHIP_LEVEL_RESET, reset_ops, ret);
        return MB_EPROGRAM;
    }
    sleep(1);
    ret = firmware_fpga_read_word(logic_dev_info->dev_name, CHIP_LEVEL_RESET, &reset_ops);
    if (ret < 0) {
        dbg_print(is_debug_on, "reset_mailbox read logic_dev failed, reg: 0x%x ret: %d\n",
            CHIP_LEVEL_RESET, ret);
        return MB_EPROGRAM;
    }
    reset_ops &= ~(RESET_MAILBOX_MASK);
    reset_ops |= RESET_MAILBOX_OFF;
    ret = firmware_fpga_write_word(logic_dev_info->dev_name, CHIP_LEVEL_RESET, reset_ops);
    if (ret < 0) {
        dbg_print(is_debug_on, "fpga_mailbox_w32 write failed, reg 0x%x, val 0x%x, ret %d.\n",
            CHIP_LEVEL_RESET, reset_ops, ret);
        return MB_EPROGRAM;
    }
    return MB_SUCCESS;
}


int firmware_upgrade_do_mailbox(int fd, uint8_t *buf, uint32_t size)
{
    int ret = 0;
    firmware_spi_logic_info_t dev_info;

    if (buf == NULL) {
        dbg_print(is_debug_on, "Input invalid error.\n");
        goto exit;
    }

    /* get sysfs information*/
    ret = firmware_sysfs_get_spi_info(fd, &dev_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "firmware_sysfs_get_dev_info failed, ret %d.\n", ret);
        goto exit;
    }

    /* enable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_INIT, NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "init dev logic faile\n");
        goto exit;
    }

    ret = firmware_upgrade_do_mailbox_func(&dev_info, buf, size);
    if (ret < 0) {
        dbg_print(is_debug_on, "init dev logic faile\n");
        goto fail;
    }

    dbg_print(is_debug_on, "firmware upgrade fpga mailbox success.\n");
    /* disable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_FINISH,NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "close dev logic en failed.\n");
    }
    return FIRMWARE_SUCCESS;

fail:
    /* disable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_FINISH, NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "close dev logic en failed.\n");
    }
exit:
    dbg_print(is_debug_on, "firmware upgrade sysfs fail.\n");
    return FIRMWARE_FAILED;
}

int firmware_upgrade_do_mailbox_func(firmware_spi_logic_info_t *logic_dev_info, uint8_t *buf, uint32_t size)
{
    int i, ret, retry;
    uint32_t img_off;

    if ((logic_dev_info == NULL) || (buf == NULL) || (size == 0)) {
        dbg_print(is_debug_on, "firmware_upgrade_do_mailbox params error, size: 0x%x, logic_dev_info or buf maybe is NULL.\n",
            size);
        return MB_EPROGRAM;
    }

    ret = reset_mailbox(logic_dev_info);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "reset mailbox fail, ret: %d\n", ret);
        return ret;
    }

    img_off = logic_dev_info->flash_base;
    if ((img_off % MAILBOX_INTEL_LOGIC_SECTOR_SIZE) != 0) {
        dbg_print(is_debug_on, "Upgrade base address: 0x%x not aligned, erase size: 0x%x\n",
            img_off, MAILBOX_INTEL_LOGIC_SECTOR_SIZE);
        return MB_EPROGRAM;
    }

    retry = FIRMWARE_SPI_LOGIC_UPG_RETRY_CNT;
    for (i = 0; i < retry; i++) {
        ret = fpga_mailbox_do_upgrade(logic_dev_info, img_off, buf, size);
        if (ret == MB_SUCCESS) {
            dbg_print(is_debug_on, "Upgrade fpga success, offset: 0x%x, size: 0x%x, retry: %d\n", img_off, size, i);
            return ret;
        }
        dbg_print(is_debug_on, "Upgrade fpga failed, offset: 0x%x, size: 0x%x, ret %d, retry: %d\n", img_off, size, ret, i);
    }
    dbg_print(is_debug_on, "Finally upgrade fpga failed, offset: 0x%x, size: 0x%x, ret %d, retry: %d\n", img_off, size, ret, i);
    return ret;
}

int firmware_upgrade_do_mailbox_test(int fd)
{
    int ret = 0;
    firmware_spi_logic_info_t dev_info;

    /* get sysfs information*/
    ret = firmware_sysfs_get_spi_info(fd, &dev_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "firmware_sysfs_get_dev_info failed, ret %d.\n", ret);
        goto exit;
    }

    /* enable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_INIT, NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "init dev logic faile\n");
        goto exit;
    }

    ret = firmware_fpga_mailbox_upgrade_test_func(&dev_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "init dev logic faile\n");
        goto fail;
    }

    dbg_print(is_debug_on, "firmware upgrade fpga mailbox success.\n");
    /* disable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_FINISH,NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "close dev logic en failed.\n");
    }
    return FIRMWARE_SUCCESS;

fail:
    /* disable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_FINISH, NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "close dev logic en failed.\n");
    }
exit:
    dbg_print(is_debug_on, "firmware upgrade sysfs fail.\n");
    return FIRMWARE_FAILED;
}

int firmware_fpga_mailbox_upgrade_test_func(firmware_spi_logic_info_t *logic_dev_info)
{
    int ret, num, i, retry;
    uint8_t *wbuf;
    uint32_t test_base, test_size;

    if (logic_dev_info == NULL) {
        dbg_print(is_debug_on, "firmware_fpga_mailbox_upgrade_test params error, logic_dev_info is NULL.\n");
        return MB_EPROGRAM;
    }

    ret = reset_mailbox(logic_dev_info);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "reset mailbox fail, ret: %d\n", ret);
        return ret;
    }

    test_base = logic_dev_info->test_base;
    test_size = logic_dev_info->test_size;
    if ((test_base % MAILBOX_INTEL_LOGIC_SECTOR_SIZE) != 0) {
        dbg_print(is_debug_on, "Error: test base = 0x%x, test size = 0x%x, sector size = 0x%x, not aligned\n",
            test_base, test_size, MAILBOX_INTEL_LOGIC_SECTOR_SIZE);
        return MB_EPROGRAM;
    }

    if (test_size == 0) {
        dbg_print(is_debug_on, "Error test base = 0x%x, test size is zero\n", test_base);
        return MB_EPROGRAM;
    }

    wbuf = (uint8_t *) malloc(test_size);
    if (wbuf == NULL) {
        dbg_print(is_debug_on, "Error: Failed to malloc memory for test write data buf, size=0x%x.\n", test_size);
        return MB_ESIZE;
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
        ret = fpga_mailbox_do_upgrade(logic_dev_info, test_base, wbuf, test_size);
        if (ret == MB_SUCCESS) {
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

int firmware_upgreade_mailbox_dump(firmware_spi_logic_info_t *logic_dev_info,
        uint32_t offset, uint8_t *buf, uint32_t rd_size)
{
    int ret;

    if ((logic_dev_info == NULL) || (buf == NULL)) {
        dbg_print(is_debug_on, "dump fpga params error, logic_dev_info or buf maybe is NULL.\n");
        return MB_EPROGRAM;
    }

    ret = reset_mailbox(logic_dev_info);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "reset mailbox fail, ret: %d\n", ret);
        return ret;
    }

    if (rd_size == 0) {
        dbg_print(is_debug_on, "dump fpga params error, rd_size is zero.\n");
        return MB_EPROGRAM;
    }

    if (offset % 4) {
        dbg_print(is_debug_on, "dump fpga params error, offset: 0x%x not 4 byte alignment.\n", offset);
        return MB_EPROGRAM;
    }

    ret= fpga_mailbox_read_epcq_data(logic_dev_info, offset, buf, rd_size);
    if (ret != MB_SUCCESS) {
        dbg_print(is_debug_on, "dump fpga read failed, offset: 0x%x, read size:0x%x, ret: %d\n", offset, rd_size, ret);
    } else {
        dbg_print(is_debug_on, "dump fpga read offset: 0x%x, read size:0x%x, success\n", offset, rd_size);
    }

    return ret;
}