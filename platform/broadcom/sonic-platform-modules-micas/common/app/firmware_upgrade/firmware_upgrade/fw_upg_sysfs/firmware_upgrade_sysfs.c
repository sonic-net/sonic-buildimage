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
