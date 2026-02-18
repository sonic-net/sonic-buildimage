#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <linux/version.h>
#include <stdlib.h>
#include <unistd.h>
#include <firmware_app.h>
#include "firmware_upgrade_sysfs.h"

static int firmware_sysfs_get_dev_info(int fd, firmware_dev_file_info_t *dev_info)
{
    int ret;

    ret = ioctl(fd, FIRMWARE_SYSFS_DEV_FILE_INFO, dev_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "Failed to get upg flash dev info.\n");
        return ret;
    }

    dbg_print(is_debug_on, "sysfs_name=%s per_len=%u.\n", dev_info->sysfs_name, dev_info->per_len);
    return 0;
}

/* sysfs upgrade program function */
int firmware_upgrade_sysfs_program(firmware_dev_file_info_t *dev_info, uint32_t dev_base,
        uint8_t *buf, uint32_t size)
{
    int ret = 0;
    uint32_t offset_addr, buf_offset, len;
    uint32_t write_len, cmp_retry, reread_len;
    int sysfs_fd;
    uint8_t *reread_buf;
    int i;

    if (dev_info->per_len > 0) {
        if (size % dev_info->per_len) {
            dbg_print(is_debug_on, "firmware sysfs upgrade size[%u] is width[%u] mismatch, ret %d.\n",
                    size, dev_info->per_len, ret);
            return FIRMWARE_FAILED;
        }
        len = dev_info->per_len;
    } else {
        /* Write to the maximum buffer if the length of each write is not configured */
        len = size;
    }

    /* Read back data */
    reread_buf = (uint8_t *) malloc(len);
    if (reread_buf == NULL) {
        dbg_print(is_debug_on, "Error: Failed to malloc memory for read back data buf, len=%u.\n", len);
        return FIRMWARE_FAILED;
    }

    sysfs_fd = open(dev_info->sysfs_name, O_RDWR | O_SYNC);
    if (sysfs_fd < 0) {
        dbg_print(is_debug_on, "open file[%s] fail.\n", dev_info->sysfs_name);
        free(reread_buf);
        return FIRMWARE_FAILED;
    }

    offset_addr = dev_base;
    buf_offset = 0;
    cmp_retry = 0;
    while (buf_offset < size) {
        /* Calibrate upgrade data length */
        if (buf_offset + len > size) {
            len = size - buf_offset;
        }

        for (i = 0; i < FW_SYSFS_RETRY_TIME; i++) {
            ret = lseek(sysfs_fd, offset_addr, SEEK_SET);
            if (ret < 0) {
                dbg_print(is_debug_on, "lseek file[%s offset=%u] fail.\n", dev_info->sysfs_name, offset_addr);
                close(sysfs_fd);
                free(reread_buf);
                return FIRMWARE_FAILED;
            }
            write_len = write(sysfs_fd, buf + buf_offset, len);
            if (write_len != len) {
                dbg_print(is_debug_on, "write file[%s] fail,offset = 0x%x retrytimes = %u len = %u, write_len =%u\n",
                    dev_info->sysfs_name, offset_addr, i ,len, write_len);
                usleep(FW_SYSFS_RETRY_SLEEP_TIME);
                continue;
            }
            break;
        }

        if (i == FW_SYSFS_RETRY_TIME) {
            dbg_print(is_debug_on, "write file[%s] fail, offset = 0x%x, len = %u, write_len =%u\n",
                dev_info->sysfs_name, offset_addr, len, write_len);
            close(sysfs_fd);
            free(reread_buf);
            return FIRMWARE_FAILED;
        }

        mem_clear(reread_buf, len);
        ret = lseek(sysfs_fd, offset_addr, SEEK_SET);
        if (ret < 0) {
            dbg_print(is_debug_on, "reread lseek file[%s offset=%u] fail.\n", dev_info->sysfs_name, offset_addr);
            close(sysfs_fd);
            free(reread_buf);
            return FIRMWARE_FAILED;
        }

        for (i = 0; i < FW_SYSFS_RETRY_TIME; i++) {
            reread_len = read(sysfs_fd, reread_buf, len);
            if (reread_len != len) {
                dbg_print(is_debug_on, "reread file[%s] fail,offset = 0x%x retrytimes = %u reread_len = %u, len =%u\n",
                    dev_info->sysfs_name, offset_addr, i ,reread_len, len);
                usleep(FW_SYSFS_RETRY_SLEEP_TIME);
                continue;
            }
            break;
        }
        if (i == FW_SYSFS_RETRY_TIME) {
            dbg_print(is_debug_on, "reread file[%s] fail, offset = 0x%x, reread_len = %u, len = %u\n",
                dev_info->sysfs_name, offset_addr, reread_len, len);
            close(sysfs_fd);
            free(reread_buf);
            return FIRMWARE_FAILED;
        }

        /* Check data */
        if (memcmp(reread_buf, buf + buf_offset, len) != 0) {
            if (cmp_retry < FW_SYSFS_RETRY_TIME) {
                dbg_print(is_debug_on, "memcmp file[%s] fail,offset = 0x%x retrytimes = %u\n",
                                    dev_info->sysfs_name, offset_addr, cmp_retry);
                cmp_retry++;
                continue;
            }

            dbg_print(is_debug_on, "upgrade file[%s] fail, offset = 0x%x.\n", dev_info->sysfs_name, offset_addr);
            dbg_print(is_debug_on, "want to write buf :\n");
            for (i = 0; i < len; i++) {
                dbg_print(is_debug_on, "0x%x ", buf[buf_offset + i]);
                if (((i + 1) % 16) == 0) {
                    dbg_print(is_debug_on, "\n");
                }
            }
            dbg_print(is_debug_on, "\n");

            dbg_print(is_debug_on, "actually reread buf :\n");
            for (i = 0; i < len; i++) {
                dbg_print(is_debug_on, "0x%x ", reread_buf[i]);
                if (((i + 1) % 16) == 0) {
                    dbg_print(is_debug_on, "\n");
                }
            }
            dbg_print(is_debug_on, "\n");

            close(sysfs_fd);
            free(reread_buf);
            return FIRMWARE_FAILED;
        }
        offset_addr += len;
        buf_offset += len;
        usleep(5000);
    }
    free(reread_buf);

    dbg_print(is_debug_on, "firmware upgrade sysfs success.\n");
    close(sysfs_fd);
    return FIRMWARE_SUCCESS;
}

/* sysfs upgrade function */
int firmware_upgrade_sysfs(int fd, uint8_t *buf, uint32_t size, name_info_t *info)
{
    int ret = 0;
    firmware_dev_file_info_t dev_info;

    if ((buf == NULL) || (info == NULL)) {
        dbg_print(is_debug_on, "Input invalid error.\n");
        goto exit;
    }

    /* get sysfs information*/
    ret = firmware_sysfs_get_dev_info(fd, &dev_info);
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

    ret = firmware_upgrade_sysfs_program(&dev_info, dev_info.dev_base, buf, size);
    if (ret < 0) {
        dbg_print(is_debug_on, "init dev logic faile\n");
        goto fail;
    }

    dbg_print(is_debug_on, "firmware upgrade sysfs success.\n");
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

/* sysfs upgrade test function */
int firmware_upgrade_sysfs_test(int fd, name_info_t *info)
{
    int ret, rv;
    firmware_dev_file_info_t dev_info;
    uint8_t *data_buf;
    uint8_t num;
    int j;

    if (info == NULL) {
        dbg_print(is_debug_on, "Input invalid error.\n");
        return FIRMWARE_FAILED;
    }

    /* get sysfs information*/
    ret = firmware_sysfs_get_dev_info(fd, &dev_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "firmware_sysfs_get_dev_info failed, ret %d.\n", ret);
        return FIRMWARE_FAILED;
    }

    if (dev_info.test_size == 0) {
        dbg_print(is_debug_on, "Error: get sysfs test size:%d, not support.\n", dev_info.test_size);
        return FIRMWARE_NOT_SUPPORT;
    }

    data_buf = (uint8_t *) malloc(dev_info.test_size);
    if (data_buf == NULL) {
        dbg_print(is_debug_on, "Error: Failed to malloc memory for test data buf, size=%d.\n", dev_info.test_size);
        return FIRMWARE_FAILED;
    }

    /* Get random data */
    for (j = 0; j < dev_info.test_size; j++) {
        num = (uint8_t) rand() % 256;
        data_buf[j] = num & 0xff;
    }

    /* enable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_INIT, NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "init dev logic faile\n");
        free(data_buf);
        return FIRMWARE_FAILED;
    }

    ret = firmware_upgrade_sysfs_program(&dev_info, dev_info.test_base, data_buf, dev_info.test_size);
    /* disable upgrade access */
    rv = ioctl(fd, FIRMWARE_SYSFS_FINISH,NULL);
    if (rv < 0) {
        dbg_print(is_debug_on, "close dev logic en failed.\n");
    }
    free(data_buf);

    if (ret < 0) {
        dbg_print(is_debug_on, "init dev logic faile\n");
        return FIRMWARE_FAILED;
    }

    dbg_print(is_debug_on, "firmware upgrade sysfs success.\n");
    return FIRMWARE_SUCCESS;
}

/**
 * firmware_upgrade_sysfs_get_upg_file_line_num
 * get upg file line num
 * @file_name: file name
 * @header_offset: header offset
 * @line_num: upg file line num
 *
 * @return: success return FIRMWARE_SUCCESS£¬failed return FIRMWARE_FAILED
 */
static int firmware_upgrade_sysfs_get_upg_file_line_num(char *file_name, int header_offset, int *line_num)
{
    FILE *fp;
    char line_info[FW_SYSFS_XDPE_ANALYSE_LINE_LEN_MAX];
    int count, ret;

    if (file_name == NULL || header_offset < 0 || line_num == NULL) {
        dbg_print(is_debug_on, "firmware_upgrade_sysfs_get_upg_file_line_num: Input invalid error.\n");
        return FIRMWARE_FAILED;
    }

    if ((fp = fopen(file_name, "r")) == NULL) {
        dbg_print(is_debug_on, "open config file[%s] fail, error=%s\n", file_name, strerror(errno));
        return FIRMWARE_FAILED;
    }

    /* skip header info area */
    ret = fseek(fp, header_offset, SEEK_SET);
    if (ret < 0) {
        fclose(fp);
        dbg_print(is_debug_on, "fseek failed, ret %d\n", ret);
        return FIRMWARE_FAILED;
    }

    memset(line_info, 0, sizeof(line_info));
    count = 0;
    while(fgets(line_info, sizeof(line_info), fp) != NULL){
        count++;
    }

    fclose(fp);
    *line_num = count;
    dbg_print(is_debug_on, "get %s line_num %d\n", file_name, *line_num);

    return FIRMWARE_SUCCESS;
}

/**
 * firmware_upgrade_sysfs_xdpe132_analyse_line
 * analyse upg file line
 * @line_info: line info str
 * @page: info: page
 * @addr: info: addr
 * @val: info: val
 * @mask: info: mask
 *
 * @return: success return FIRMWARE_SUCCESS£¬failed return FIRMWARE_FAILED
 */
static int firmware_upgrade_sysfs_xdpe132_analyse_line(char *line_info,
            uint8_t *page, uint8_t *addr, uint8_t *val, uint8_t *mask)
{
    char tmp_info[FW_SYSFS_XDPE_ANALYSE_LINE_INFO_LEN];

    if (line_info == NULL || page == NULL || addr == NULL || val == NULL || mask == NULL) {
        dbg_print(is_debug_on, " firmware_upgrade_sysfs_xdpe132_analyse_line: Input invalid error.\n");
        return FIRMWARE_FAILED;
    }

    if (strlen(line_info) < FW_SYSFS_XDPE_ANALYSE_LINE_LEN_MIN) {
        dbg_print(is_debug_on, "%s strlen is invalid.\n", line_info);
        return FIRMWARE_FAILED;
    }

    /**
     * upg file format:
     * PAGE(1 byte) + ADDR(1 byte) + Space + write_val(1 byte) + Space + mask(1 byte)
     * for example: [0030 40 7F] represents: [page 0, addr 0x30, write_val 0x40£¬mask 0x7f]
     */
    memset(tmp_info, 0, sizeof(tmp_info));
    sprintf(tmp_info, "%c%c", line_info[0], line_info[1]);
    *page = strtol(tmp_info, NULL, 16);

    memset(tmp_info, 0, sizeof(tmp_info));
    sprintf(tmp_info, "%c%c", line_info[2], line_info[3]);
    *addr = strtol(tmp_info, NULL, 16);

    memset(tmp_info, 0, sizeof(tmp_info));
    sprintf(tmp_info, "%c%c", line_info[5], line_info[6]);
    *val = strtol(tmp_info, NULL, 16);

    memset(tmp_info, 0, sizeof(tmp_info));
    sprintf(tmp_info, "%c%c", line_info[8], line_info[9]);
    *mask = strtol(tmp_info, NULL, 16);

    return FIRMWARE_SUCCESS;
}

/**
 * firmware_upgrade_sysfs_xdpe132_read_func sysfs-xdpe132
 * read interface
 * @sysfs_fd: sysfs_fd
 * @offset: offset
 * @val: get read val
 *
 * @return: success return FIRMWARE_SUCCESS£¬failed return FIRMWARE_FAILED
 */
static int firmware_upgrade_sysfs_xdpe132_read_func(int sysfs_fd, uint32_t offset, uint8_t *val)
{
    uint8_t read_buf[FW_SYSFS_XDPE_RW_LENGTH];
    int ret, read_len;

    if (val == NULL) {
        dbg_print(is_debug_on, " firmware_upgrade_sysfs_xdpe132_read_func: Input invalid error.\n");
        return FIRMWARE_FAILED;
    }

    ret = lseek(sysfs_fd, offset, SEEK_SET);
    if (ret < 0) {
        dbg_print(is_debug_on, "lseek file failed. offset 0x%x\n", offset);
        return FIRMWARE_FAILED;
    }

    memset(read_buf, 0, sizeof(read_buf));
    read_len = read(sysfs_fd, read_buf, FW_SYSFS_XDPE_RW_LENGTH);
    if (read_len != FW_SYSFS_XDPE_RW_LENGTH) {
        dbg_print(is_debug_on, "read failed. offset 0x%x\n", offset);
        return FIRMWARE_FAILED;
    }
    *val = read_buf[0];

    return FIRMWARE_SUCCESS;
}

/**
 * firmware_upgrade_sysfs_xdpe132_write_func sysfs-xdpe132
 * write interface
 * @sysfs_fd: sysfs_fd
 * @offset: offset
 * @val: write val
 *
 * @return: success return FIRMWARE_SUCCESS£¬failed return FIRMWARE_FAILED
 */
static int firmware_upgrade_sysfs_xdpe132_write_func(int sysfs_fd, uint32_t offset, uint8_t val)
{
    uint8_t write_buf[FW_SYSFS_XDPE_RW_LENGTH];
    int ret, write_len;

    ret = lseek(sysfs_fd, offset, SEEK_SET);
    if (ret < 0) {
        dbg_print(is_debug_on, "lseek file failed. offset 0x%x\n", offset);
        return FIRMWARE_FAILED;
    }

    memset(write_buf, 0, sizeof(write_buf));
    write_buf[0] = val;
    write_len = write(sysfs_fd, write_buf, FW_SYSFS_XDPE_RW_LENGTH);
    if (write_len != FW_SYSFS_XDPE_RW_LENGTH) {
        dbg_print(is_debug_on, "write failed. offset 0x%x\n", offset);
        return FIRMWARE_FAILED;
    }

    return FIRMWARE_SUCCESS;
}

/**
 * firmware_upgrade_sysfs_xdpe132: 
 *  xdpe132 sysfs upg
 *
 * Upgrade process: 
 * Parse the device file and store it into a buffer array, 
 * then take data from the array one by one to 
 * write to the sysfs file and perform read-back verification.
 *
 * @fd: upg fd
 * @file_name: upg file
 * @header_offset: upg file header offset
 * @return: success return FIRMWARE_SUCCESS£¬failed return FIRMWARE_FAILED
 */
int firmware_upgrade_sysfs_xdpe132(int fd, char *file_name, int header_offset)
{
    firmware_dev_file_info_t dev_info;
    firmware_xdpe_file_info_t *upg_file_info;
    char line_info[FW_SYSFS_XDPE_ANALYSE_LINE_LEN_MAX];
    char *str_tmp;
    uint32_t offset_addr;
    uint8_t page, addr, val, mask, read_back_val;
    FILE *fp;
    int sysfs_fd, config_num, line_num, upg_fail_cnt;
    int ret, i;

    /* input param check */
    if ((file_name == NULL) || (header_offset < 0)) {
        dbg_print(is_debug_on, "firmware_upgrade_sysfs_xdpe132. Input invalid error.\n");
        goto exit;
    }
    dbg_print(is_debug_on, "Start firmware_upgrade_sysfs_xdpe132, file_name %s\n", file_name);

    /* get upg file line num */
    line_num = 0;
    ret = firmware_upgrade_sysfs_get_upg_file_line_num(file_name, header_offset, &line_num);
    if (ret) {
        dbg_print(is_debug_on, "get upgrade file line_num failed.\n");
        goto exit;
    }

    /* malloc one array save file info depend on upg file line num*/
    upg_file_info = (firmware_xdpe_file_info_t *)malloc(sizeof(firmware_xdpe_file_info_t) * line_num);
    if (upg_file_info == NULL) {
        dbg_print(is_debug_on, "Error: Failed to malloc memory for upg_file_info, line_num %d.\n", line_num);
        goto exit;
    }
    memset(upg_file_info, 0, sizeof(firmware_xdpe_file_info_t) * line_num);
    sysfs_fd = -1;

    /* open upg file */
    if ((fp = fopen(file_name, "r")) == NULL) {
        dbg_print(is_debug_on, "open config file[%s] failed\n", file_name);
        goto failed;
    }

    /* skip upg file header offset */
    ret = fseek(fp, header_offset, SEEK_SET);
    if (ret < 0) {
        dbg_print(is_debug_on, "fseek failed, ret %d\n", ret);
        goto failed;
    }

    /* analyse file and save to array£¬array num is :config_num */
    memset(line_info, 0, sizeof(line_info));
    config_num = 0;
    while(fgets(line_info, sizeof(line_info), fp) != NULL){
        /* Lines containing the character '/' are non-upgrade content and should be skipped. */
        str_tmp = strchr(line_info, '/');
        if (str_tmp != NULL) {
            continue;
        }
        ret = firmware_upgrade_sysfs_xdpe132_analyse_line(line_info, &page, &addr, &val, &mask);
        if (ret != FIRMWARE_SUCCESS) {
            goto failed;
        }

        /**
         * Only offset addresses within the following range need to be upgraded.
         * page 0: 0x30~0x83
         * page 4: 0x20~0x57
         * page 6: 0x00~0x41 0x44~0xff
         * page 7: 0x00~0xff
         * page 8: 0x20~0x57
         * page a: 0x00~0x41 0x44~0xff
         * page b: 0x00~0xff
         */
        if ((page == 0x0 && (addr >= 0x30 && addr <= 0x83))
                || (page == 0x4 && (addr >= 0x20 && addr <= 0x57))
                || (page == 0x6 && ((addr >= 0x00 && addr <= 0x41) || (addr >= 0x44 && addr <= 0xff)))
                || (page == 0x7 && (addr >= 0x00 && addr <= 0xff))
                || (page == 0x8 && (addr >= 0x20 && addr <= 0x57))
                || (page == 0xa && ((addr >= 0x00 && addr <= 0x41) || (addr >= 0x44 && addr <= 0xff)))
                || (page == 0xb && (addr >= 0x00 && addr <= 0xff))) {
            upg_file_info[config_num].page = page;
            upg_file_info[config_num].addr = addr;
            upg_file_info[config_num].val = val;
            upg_file_info[config_num].mask = mask;
            config_num++;
            memset(line_info, 0, sizeof(line_info));
        }
    }
    fclose(fp);

    /* get sysfs fd£¬ioctl enable upg */
    ret = firmware_sysfs_get_dev_info(fd, &dev_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "firmware_sysfs_get_dev_info failed, ret %d.\n", ret);
        goto failed;
    }
    ret = ioctl(fd, FIRMWARE_SYSFS_INIT, NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "init dev logic fail\n");
        goto failed;
    }
    sysfs_fd = open(dev_info.sysfs_name, O_RDWR | O_SYNC);
    if (sysfs_fd < 0) {
        dbg_print(is_debug_on, "open file[%s] fail.\n", dev_info.sysfs_name);
        goto disable_ioctl;
    }

    upg_fail_cnt = 0;
    /* Traverse the above cached array, writing to xdpe-sysfs one by one and performing read-back verification. */
    for (i = 0; i < config_num; i++) {
        /* The 'offset' format consists of setting bit 16 to 1, 
         * followed by 8 bits for the page and 8 bits for the offset address; 
         * page switching is handled within the driver. */
        offset_addr = FW_SYSFS_XDPE_RW_SET_PAGE_MASK | (upg_file_info[i].page << 8) | upg_file_info[i].addr;

        /* §Õ */
        ret = firmware_upgrade_sysfs_xdpe132_write_func(sysfs_fd, offset_addr, upg_file_info[i].val);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "upgrade_sysfs_xdpe132 write failed. offset_addr 0x%x\n", offset_addr);
            goto disable_ioctl;
        }

        /* read back and check */
        ret = firmware_upgrade_sysfs_xdpe132_read_func(sysfs_fd, offset_addr, &read_back_val);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "upgrade_sysfs_xdpe132 read_back failed. offset_addr 0x%x\n", offset_addr);
            goto disable_ioctl;
        }

        /* If the read-back value differs and the mask is not zero, it is considered an error. */
        if ((read_back_val != upg_file_info[i].val) && (upg_file_info[i].mask != 0)) {
            dbg_print(is_debug_on, "upgrade_sysfs_xdpe132 write check failed. offset_addr 0x%x\n", offset_addr);
            upg_fail_cnt++;
        }
    }
    if (upg_fail_cnt > 0) {
        dbg_print(is_debug_on, "upgrade_sysfs_xdpe132 upgrade failed. upg_fail_cnt %d\n", upg_fail_cnt);
        goto disable_ioctl;
    }
    /* disable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_FINISH,NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "close dev logic en failed.\n");
        goto failed;
    }
    close(sysfs_fd);
    free(upg_file_info);

    dbg_print(is_debug_on, "firmware upgrade sysfs success.\n");

    return FIRMWARE_SUCCESS;

disable_ioctl:
    /* disable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_FINISH, NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "close dev logic en failed.\n");
    }

failed:
    if (upg_file_info) {
        free(upg_file_info);
    }
    if (sysfs_fd >= 0) {
        close(sysfs_fd);
    }
    if (fp != NULL) {
        fclose(fp);
    }

exit:
    dbg_print(is_debug_on, "firmware upgrade sysfs-xdpe132 failed.\n");
    return FIRMWARE_FAILED;
}