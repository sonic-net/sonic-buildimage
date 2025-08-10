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
#include <stdlib.h>
#include <unistd.h>
#include <firmware_app.h>
#include "firmware_upgrade_mtd.h"
#include "mtd-abi.h"

static int get_mtdnum_from_name(char *name, int *mtdnum)
{
    FILE *fp;
    int ret;
    char buf[PATH_LEN];
    char *start;
    char *end;
    char *key_w = "mtd";

    if (name == NULL || mtdnum == NULL) {
        dbg_print(is_debug_on, "Input invalid error.\n");
        return -EINVAL;
    }
    ret = 0;
    *mtdnum = -1;
    fp = fopen("/proc/mtd", "r");
    if (fp == NULL) {
        dbg_print(is_debug_on, "Not find mtd device.\n");
        return -FIRWMARE_MTD_PART_INFO_ERR;
    }

    mem_clear(buf, sizeof(buf));
    while(fgets(buf, sizeof(buf), fp)) {
        if (strstr(buf, name) != NULL) {
            start = strstr(buf, key_w);
            if (start == NULL) {
                dbg_print(is_debug_on, "/proc/mtd don't find %s.\n", key_w);
                ret = -FIRWMARE_MTD_PART_INFO_ERR;
                goto exit;
            }
            start += strlen(key_w);
            end = strchr(start, ':');
            if (end == NULL) {
                dbg_print(is_debug_on, "/proc/mtd don't find %c.\n", ':');
                ret = -FIRWMARE_MTD_PART_INFO_ERR;
                goto exit;
            }

            *end = '\0';
            *mtdnum = atoi(start);
            if (*mtdnum < 0) {
                dbg_print(is_debug_on, "Not get mtd num.\n");
                ret = -FIRWMARE_MTD_PART_INFO_ERR;
                goto exit;
            }
        }
    }

    if (*mtdnum == -1) {
        ret = -FIRWMARE_MTD_PART_INFO_ERR;
        goto exit;
    }
exit:
    if (fp != NULL) {
        fclose(fp);
    }

    return ret;
}

static int firmware_sysfs_get_dev_info(int fd, firmware_mtd_info_t *dev_info)
{
    int ret;

    ret = ioctl(fd, FIRMWARE_SYSFS_MTD_INFO, dev_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "Failed to get upg device file info.\n");
        return ret;
    }

    dbg_print(is_debug_on, "mtd_name=%s flash_base=0x%x test_base=0x%x test_size=%d.\n",
            dev_info->mtd_name, dev_info->flash_base, dev_info->test_base, dev_info->test_size);
    return 0;
}

/*
 * MEMGETINFO
 */
static int getmeminfo(int fd, struct mtd_info_user *mtd)
{
    return ioctl(fd, MEMGETINFO, mtd);
}

/*
 * MEMERASE
 */
static int memerase(int fd, struct erase_info_user *erase)
{
    return ioctl(fd, MEMERASE, erase);
}

static int erase_flash(int fd, uint32_t offset, uint32_t bytes)
{
    int err;
    struct erase_info_user erase;
    erase.start = offset;
    erase.length = bytes;
    err = memerase(fd, &erase);
    if (err < 0) {
        dbg_print(is_debug_on, "Error: memerase failed, err=%d\n", err);
        return -FIRWMARE_MTD_MEMERASE;
    }
    dbg_print(is_debug_on, "Erased %d bytes from address 0x%.8x in flash\n", bytes, offset);
    return 0;
}

/*
 * firmware_upgrade_mtd_block
 * function: upgrade mtd device block
 * @dev_info:   param[in] Device file descriptor
 * @buf:  param[in] Upgrade the file content
 * @size: param[in] Upgrade file size
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
static int firmware_upgrade_mtd_block(int mtd_fd, uint32_t offset,
                uint8_t *buf, uint32_t size, uint32_t erasesize)
{
    int ret;
    int i;
    uint8_t *reread_buf;
    uint32_t cmp_retry, reread_len, write_len;

    /* Read back data */
    reread_buf = (uint8_t *) malloc(size);
    if (reread_buf == NULL) {
        dbg_print(is_debug_on, "Error: Failed to malloc memory for read back data buf, size=%d.\n", size);
        return FIRMWARE_FAILED;
    }

    for (cmp_retry = 0; cmp_retry < FW_SYSFS_RETRY_TIME; cmp_retry++) {
        for (i = 0; i < FW_SYSFS_RETRY_TIME; i++) {
            if (offset != lseek(mtd_fd, offset, SEEK_SET)) {
                dbg_print(is_debug_on, "Error:lseek mtd offset=%x retrytimes=%d failed.\n", offset, i);
                usleep(FW_SYSFS_RETRY_SLEEP_TIME);
                continue;
            }

            dbg_print(is_debug_on, "erase mtd offset=0x%x erasesize=%d retrytimes=%d.\n",
                        offset, erasesize, i);
            ret = erase_flash(mtd_fd, offset, erasesize);
            if (ret < 0) {
                dbg_print(is_debug_on, "Error:erase mtd offset=%x size=%d retrytimes=%d failed, ret=%d\n",
                            offset, size, i, ret);
                usleep(FW_SYSFS_RETRY_SLEEP_TIME);
                continue;
            }

            dbg_print(is_debug_on, "write mtd offset=0x%x size=%d retrytimes=%d.\n",
                        offset, size, i);
            write_len = write(mtd_fd, buf, size);
            if (write_len != size) {
                dbg_print(is_debug_on, "Error:write mtd offset=0x%x size=%d write_len=%d retrytimes=%d.\n",
                             offset, size, write_len, i);
                usleep(FW_SYSFS_RETRY_SLEEP_TIME);
                continue;
            }
            break;
        }
        if (i == FW_SYSFS_RETRY_TIME) {
            dbg_print(is_debug_on, "Error: upgrade mtd fail, offset = 0x%x, size = %d\n", offset, size);
            free(reread_buf);
            return FIRMWARE_FAILED;
        }

        usleep(FW_SYSFS_RETRY_SLEEP_TIME);
        dbg_print(is_debug_on, "Reread mtd offset=0x%x size=%d\n", offset, size);
        for (i = 0; i < FW_SYSFS_RETRY_TIME; i++) {
            if (offset != lseek(mtd_fd, offset, SEEK_SET)) {
                dbg_print(is_debug_on, "Error:lseek mtd offset=%x retrytimes=%d failed.\n", offset, i);
                usleep(FW_SYSFS_RETRY_SLEEP_TIME);
                continue;
            }

            reread_len = read(mtd_fd, reread_buf, size);
            if (reread_len != size) {
                dbg_print(is_debug_on, "Error:reread mtd offset=0x%x size=%d reread_len=%d retrytimes=%d.\n",
                             offset, size, reread_len, i);
                usleep(FW_SYSFS_RETRY_SLEEP_TIME);
                continue;
            }
            break;
        }
        if (i == FW_SYSFS_RETRY_TIME) {
            dbg_print(is_debug_on, "Error: reread mtd fail, offset = 0x%x size = %d\n", offset, size);
            free(reread_buf);
            return FIRMWARE_FAILED;
        }

        /* Check data */
        if (memcmp(reread_buf, buf, size) != 0) {
            dbg_print(is_debug_on, "memcmp mtd fail,offset = 0x%x retrytimes = %d\n", offset, cmp_retry);
        } else {
            break;
        }
    }
    if (cmp_retry >= FW_SYSFS_RETRY_TIME) {
        dbg_print(is_debug_on, "upgrade mtd fail, offset = 0x%x.\n", offset);
        dbg_print(is_debug_on, "want to write buf :\n");
        for (i = 0; i < size; i++) {
            dbg_print(is_debug_on, "0x%x ", buf[i]);
            if (((i + 1) % 16) == 0) {
                dbg_print(is_debug_on, "\n");
            }
        }
        dbg_print(is_debug_on, "\n");

        dbg_print(is_debug_on, "actually reread buf :\n");
        for (i = 0; i < size; i++) {
            dbg_print(is_debug_on, "0x%x ", reread_buf[i]);
            if (((i + 1) % 16) == 0) {
                dbg_print(is_debug_on, "\n");
            }
        }
        dbg_print(is_debug_on, "\n");

        free(reread_buf);
        return FIRMWARE_FAILED;
    }

    free(reread_buf);
    dbg_print(is_debug_on, "firmware upgrade mtd block offset[0x%.8x] success.\n", offset);
    return FIRMWARE_SUCCESS;
}

/*
 * firmware_upgrade_mtd_program
 * function: upgrade mtd device
 * @dev_info:   param[in] Device file descriptor
 * @flash_base: param[in] Upgrade the flash start address
 * @buf:  param[in] Upgrade the file content
 * @size: param[in] Upgrade file size
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
static int firmware_upgrade_mtd_program(firmware_mtd_info_t *dev_info,
                int flash_base, uint8_t *buf, uint32_t size)
{
    int ret;
    int mtdnum;
    char dev_mtd[PATH_LEN];
    int mtd_fd;
    uint32_t offset, len, block_size;
    struct mtd_info_user mtd_info;
    uint8_t *data_point;

    ret = get_mtdnum_from_name(dev_info->mtd_name, &mtdnum);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:not find %s mtd num.\n", dev_info->mtd_name);
        return FIRMWARE_FAILED;
    }

    mem_clear(dev_mtd, sizeof(dev_mtd));
    snprintf(dev_mtd, sizeof(dev_mtd) - 1, "/dev/mtd%d", mtdnum);

    mtd_fd = open(dev_mtd, O_SYNC | O_RDWR);
    if (mtd_fd < 0) {
        dbg_print(is_debug_on, "Error:open %s failed.\n", dev_mtd);
        goto err;
    }

    ret = getmeminfo(mtd_fd, &mtd_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:get mtd info failed, ret=%d.\n", ret);
        goto failed;
    }

    offset = flash_base;
    if (offset >= mtd_info.size) {
        dbg_print(is_debug_on, "Error: offset[0x%.8x] over size[0x%.8x]\n", offset, size);
        goto failed;
    }

    len = size;
    data_point = buf;
    while ((offset < mtd_info.size) && (len > 0)) {
        if (len > mtd_info.erasesize) {
            block_size = mtd_info.erasesize;
        } else {
            block_size = len;
        }
        dbg_print(is_debug_on, "upgrade mtd[%s] block offset[0x%.8x] size[%d] relen[%d].\n", dev_mtd, offset, size, len);
        ret = firmware_upgrade_mtd_block(mtd_fd, offset, data_point, block_size, mtd_info.erasesize);
        if (ret < 0) {
            dbg_print(is_debug_on, "Error: mt block offset[0x%.8x] size[0x%.8x] failed.\n", offset, block_size);
            goto failed;
        }
        len -= block_size;
        data_point += block_size;
        offset += block_size;
        usleep(FW_MTD_BLOCK_SLEEP_TIME);
    }

    if (close(mtd_fd) < 0) {
        dbg_print(is_debug_on, "Error:close %s failed.\n", dev_mtd);
    }
    dbg_print(is_debug_on, "firmware upgrade mtd device success.\n");
    return FIRMWARE_SUCCESS;

failed:
    if (close(mtd_fd) < 0) {
        dbg_print(is_debug_on, "Error:close %s failed.\n", dev_mtd);
    }

err:
    dbg_print(is_debug_on, "firmware upgrade mtd device fail.\n");
    return FIRMWARE_FAILED;
}

/*
 * firmware_upgrade_mtd
 * function: Determine whether to upgrade ISC or JBI
 * @fd:   param[in] Device file descriptor
 * @buf:  param[in] Upgrade the file content
 * @size: param[in] Upgrade file size
 * @info: param[in] Upgrade file information
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int firmware_upgrade_mtd(int fd, uint8_t *buf, uint32_t size, name_info_t *info)
{
    int ret;
    firmware_mtd_info_t dev_info;

    if ((buf == NULL) || (info == NULL)) {
        dbg_print(is_debug_on, "Input invalid error.\n");
        return FIRMWARE_FAILED;
    }

    /* get sysfs information*/
    ret = firmware_sysfs_get_dev_info(fd, &dev_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "firmware_sysfs_get_dev_info failed, ret %d.\n", ret);
        return FIRMWARE_FAILED;
    }

    /* enable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_INIT, NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "init dev logic faile\n");
        return FIRMWARE_FAILED;
    }

    ret = firmware_upgrade_mtd_program(&dev_info, dev_info.flash_base, buf, size);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:mtd device program failed, ret=%d.\n", ret);
        goto failed;
    }

    /* disable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_FINISH, NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "close dev logic en failed.\n");
    }

    return FIRMWARE_SUCCESS;

failed:
    /* disable upgrade access */
    ret = ioctl(fd, FIRMWARE_SYSFS_FINISH,NULL);
    if (ret < 0) {
        dbg_print(is_debug_on, "close dev logic en failed.\n");
    }

    return FIRMWARE_FAILED;
}

/*
 * firmware_upgrade_mtd_test
 * function: Determine whether to upgrade ISC or JBI
 * @fd:   param[in] Device file descriptor
 * @info: param[in] Upgrade file information
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int firmware_upgrade_mtd_test(int fd, name_info_t *info)
{
    int ret, rv;
    firmware_mtd_info_t dev_info;
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
        dbg_print(is_debug_on, "Error: get flash size:%d, not support.\n", dev_info.test_size);
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

    ret = firmware_upgrade_mtd_program(&dev_info, dev_info.test_base, data_buf, dev_info.test_size);
    /* disable upgrade access */
    rv = ioctl(fd, FIRMWARE_SYSFS_FINISH, NULL);
    if (rv < 0) {
        dbg_print(is_debug_on, "close dev logic en failed.\n");
    }
    free(data_buf);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:mtd device program failed, ret=%d.\n", ret);
        return FIRMWARE_FAILED;
    }
    return FIRMWARE_SUCCESS;
}
