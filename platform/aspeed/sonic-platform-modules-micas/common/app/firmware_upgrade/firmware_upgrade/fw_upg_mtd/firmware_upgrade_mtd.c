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
#include <fnmatch.h>
#include <linux/version.h>
#include <stdlib.h>
#include <unistd.h>
#include <firmware_app.h>
#include "firmware_upgrade_mtd.h"
#include "mtd-abi.h"

#define SYS_MTD_PATH "/sys/class/mtd"
#define FIRMWARE_MTD_CTX_MAGIC (0x4D544443U)

static firmware_mtd_upg_ctx_t *fw_up_mtd_ctx_from_priv(void *priv_data)
{
    firmware_mtd_upg_ctx_t *ctx;

    if (priv_data == NULL) {
        dbg_print(is_debug_on, "Error: Invalid parameter for fw_up_mtd_ctx_from_priv.\n");
        return NULL;
    }

    ctx = (firmware_mtd_upg_ctx_t *)priv_data;
    if (ctx->magic != FIRMWARE_MTD_CTX_MAGIC) {
        dbg_print(is_debug_on, "Error: Invalid mtd upgrade context magic, possible memory corruption or misuse of priv_data.\n");
        return NULL;
    }

    return ctx;
}

int firmware_upgrade_mtd_ctx_alloc(void **ctx_out)
{
    firmware_mtd_upg_ctx_t *ctx;

    if (ctx_out == NULL) {
        dbg_print(is_debug_on, "Error: Invalid parameter for firmware_upgrade_mtd_ctx_alloc.\n");
        return -EINVAL;
    }

    ctx = (firmware_mtd_upg_ctx_t *)malloc(sizeof(*ctx));
    if (ctx == NULL) {
        dbg_print(is_debug_on, "Error: Failed to allocate memory for mtd upgrade context.\n");
        return -ENOMEM;
    }

    mem_clear(ctx, sizeof(*ctx));
    ctx->magic = FIRMWARE_MTD_CTX_MAGIC;
    ctx->uconf_skip = 0;
    ctx->block_info = NULL;
    *ctx_out = (void *)ctx;
    return FIRMWARE_SUCCESS;
}

void firmware_upgrade_mtd_ctx_free(void **ctx_inout)
{
    firmware_mtd_upg_ctx_t *ctx;

    if ((ctx_inout == NULL) || (*ctx_inout == NULL)) {
        dbg_print(is_debug_on, "Error: Invalid parameter for firmware_upgrade_mtd_ctx_free.\n");
        return;
    }

    ctx = fw_up_mtd_ctx_from_priv(*ctx_inout);
    if (ctx == NULL) {
        dbg_print(is_debug_on, "Error: Invalid mtd upgrade context in firmware_upgrade_mtd_ctx_free, possible memory corruption or misuse of priv_data.\n");
        *ctx_inout = NULL;
        return;
    }

    firmware_upgrade_mtd_topo_blk_free(&ctx->block_info);
    ctx->magic = 0;
    free(ctx);
    *ctx_inout = NULL;
}

int firmware_upgrade_mtd_get_dev_name(const char *label_name, char *mtd_dev_name, int len)
{
    FILE *fp;
    int ret;
    char buf[PATH_LEN];
    char *start;
    char *end;
    char *key_w = "mtd";
    int mtdnum;

    if ((label_name == NULL) || (mtd_dev_name == NULL) || (len <= 0)) {
        dbg_print(is_debug_on, "Error: Invalid parameter for firmware_upgrade_mtd_get_dev_name.\n");
        return -EINVAL;
    }

    ret = 0;
    mtdnum = -1;
    fp = fopen("/proc/mtd", "r");
    if (fp == NULL) {
        dbg_print(is_debug_on, "Not find mtd device.\n");
        return -FIRWMARE_MTD_PART_INFO_ERR;
    }

    mem_clear(buf, sizeof(buf));
    while (fgets(buf, sizeof(buf), fp)) {
        if (strstr(buf, label_name) != NULL) {
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
            mtdnum = atoi(start);
            if (mtdnum < 0) {
                dbg_print(is_debug_on, "Not get mtd num.\n");
                ret = -FIRWMARE_MTD_PART_INFO_ERR;
                goto exit;
            }
            break;
        }
        mem_clear(buf, sizeof(buf));
    }

    if (mtdnum == -1) {
        dbg_print(is_debug_on, "Not find mtd device with label name %s.\n", label_name);
        ret = -FIRWMARE_MTD_PART_INFO_ERR;
        goto exit;
    }

    if (snprintf(mtd_dev_name, len, "mtd%d", mtdnum) >= len) {
        dbg_print(is_debug_on, "mtd dev name buffer is too small.\n");
        ret = -EINVAL;
    }

exit:
    fclose(fp);
    return ret;
}

/* Only match subpartition directories with "mtd" + digits, ignore suffixes like "mtdXXro" */
static int fw_up_is_child_mtd_entry(const char *entry_name, const char *parent_name)
{
    int i;

    if ((entry_name == NULL) || (parent_name == NULL)) {
        return 0;
    }

    /* Check if the entry name starts with "mtd" */
    if (strncmp(entry_name, "mtd", 3) != 0) {
        return 0;
    }

    /* Skip the parent partition itself */
    if (strcmp(entry_name, parent_name) == 0) {
        return 0;
    }

    /* Check if all characters after "mtd" are digits, and there is at least one digit */
    for (i = 3; entry_name[i] != '\0'; i++) {
        if (!isdigit((unsigned char)entry_name[i])) {
            return 0;
        }
    }

    return (i > 3);
}

static int fw_up_read_mtd_attr(const char *mtd, const char *attr, char *buf, int buf_len)
{
    char path[PATH_LEN];
    FILE *fp;
    int len;

    if ((mtd == NULL) || (attr == NULL) || (buf == NULL) || (buf_len == 0)) {
        return -EINVAL;
    }

    (void)snprintf(path, sizeof(path), "%s/%s/%s", SYS_MTD_PATH, mtd, attr);
    fp = fopen(path, "r");
    if (fp == NULL) {
        dbg_print(is_debug_on, "Error: Failed to open mtd attribute file: %s. errno=%d.\n", path, errno);
        return -EIO;
    }

    if (fgets(buf, buf_len, fp) == NULL) {
        dbg_print(is_debug_on, "Error: Failed to read mtd attribute file: %s. errno=%d.\n", path, errno);
        fclose(fp);
        return -EIO;
    }

    len = strlen(buf);
    /* Filter out newline, carriage return, and space characters */
    while ((len > 0) && ((buf[len - 1] == '\n') || (buf[len - 1] == '\r') || (buf[len - 1] == ' '))) {
        buf[len - 1] = '\0';
        len--;
    }

    fclose(fp);
    return 0;
}

/* Read mtd attribute and parse it into uint32 value for topology fields like offset/size. */
static int fw_up_read_mtd_attr_u32(const char *mtd, const char *attr, uint32_t *value_out)
{
    int ret;
    char *endp;
    unsigned long val;
    char attr_buf[FIRMWARE_DEV_NAME_LEN];

    if ((mtd == NULL) || (attr == NULL) || (value_out == NULL)) {
        dbg_print(is_debug_on, "Error: Invalid parameter for fw_up_read_mtd_attr_u32.\n");
        return -EINVAL;
    }

    mem_clear(attr_buf, sizeof(attr_buf));
    ret = fw_up_read_mtd_attr(mtd, attr, attr_buf, sizeof(attr_buf));
    if (ret != 0) {
        dbg_print(is_debug_on, "Error: read %s for mtd %s failed.\n", attr, mtd);
        return -FIRWMARE_MTD_PART_INFO_ERR;
    }

    errno = 0;
    endp = NULL;
    val = strtoul(attr_buf, &endp, 0);
    if ((errno != 0) || (endp == attr_buf) || (*endp != '\0' && *endp != '\n')) {
        dbg_print(is_debug_on, "Error: parse %s for mtd %s failed.\n", attr, mtd);
        return -FIRWMARE_MTD_PART_INFO_ERR;
    }

    if (val > UINT32_MAX) {
        dbg_print(is_debug_on, "Error: value of %s for mtd %s is too large.\n", attr, mtd);
        return -FIRWMARE_MTD_PART_INFO_ERR;
    }

    *value_out = (uint32_t)val;
    return FIRMWARE_SUCCESS;
}

static int fw_up_mtd_mark_uconf_skip(const char *mtd_name, firmware_mtd_block_t *block_info)
{
    int i;
    int hit_count;
    char attr_buf[FIRMWARE_DEV_NAME_LEN];
    char pattern[FIRMWARE_DEV_NAME_LEN];

    if ((mtd_name == NULL) || (block_info == NULL)) {
        dbg_print(is_debug_on, "Error: Invalid parameter for fw_up_mtd_mark_uconf_skip.\n");
        return -EINVAL;
    }

    /* find target partition using wildcard, for example, if the target partition name is BIOS,
     * then BIOS_uconf0, BIOS_uconf1, etc. will be marked as skip
     */
    mem_clear(pattern, sizeof(pattern));
    if (snprintf(pattern, sizeof(pattern), "%s_uconf*", mtd_name) >= (int) sizeof(pattern)) {
        dbg_print(is_debug_on, "Error: mtd name %s is too long for uconf pattern.\n", mtd_name);
        printf("Error: mtd name %s is too long.\n", mtd_name);
        return -FIRWMARE_MTD_PART_INFO_ERR;
    }

    hit_count = 0;
    for (i = 0; i < block_info->block_num; i++) {
        block_info->block[i].skip_flag = 0;
        mem_clear(attr_buf, sizeof(attr_buf));
        if (fw_up_read_mtd_attr(block_info->block[i].name, "name", attr_buf, sizeof(attr_buf)) != 0) {
            continue;
        }

        if (fnmatch(pattern, attr_buf, 0) == 0) {
            block_info->block[i].skip_flag = 1;
            hit_count++;
            dbg_print(is_debug_on, "Mark skip partition %s (label: %s).\n",
                      block_info->block[i].name, attr_buf);
        }
    }

    if (hit_count == 0) {
        dbg_print(is_debug_on, "Error: no uconf partition matched by pattern %s.\n", pattern);
        printf("Error: cannot find uconf partition matched by pattern %s.\n", pattern);
        return -FIRWMARE_MTD_PART_INFO_ERR;
    }

    return FIRMWARE_SUCCESS;
}

/*
* firmware_upgrade_mtd_alloc_partition_topo
* function: Get all partition information of the target MTD, the obtained block_info resource needs to be released actively
* @mtd_name: param[in]  mtd device name
* @block_info: param[out] mtd partition topology info
* return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
*/
int firmware_upgrade_mtd_topo_blk_alloc(const char *mtd_name, firmware_mtd_block_t **block_info)
{
    int ret;
    DIR *dir;
    uint32_t count;
    size_t alloc_size;
    struct dirent *entry;
    char mtd_path[PATH_LEN];
    firmware_mtd_block_t *topo;

    if ((mtd_name == NULL) || (block_info == NULL)) {
        dbg_print(is_debug_on, "Error: Invalid parameter for firmware_upgrade_mtd_topo_blk_alloc.\n");
        return -EINVAL;
    }

    *block_info = NULL;

    mem_clear(mtd_path, sizeof(mtd_path));
    snprintf(mtd_path, sizeof(mtd_path), "%s/%s", SYS_MTD_PATH, mtd_name);
    dir = opendir(mtd_path);
    if (dir == NULL) {
        dbg_print(is_debug_on, "Error: opendir %s failed, errno=%d.\n", mtd_path, errno);
        return -EIO;
    }

    count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (fw_up_is_child_mtd_entry(entry->d_name, mtd_name)) {
            count++;
        }
    }

    /* if no child partition found, still allocate an empty topo structure, the upper layer needs to check block_num */
    if (count == 0) {
        closedir(dir);
        alloc_size = sizeof(firmware_mtd_block_t);
        topo = (firmware_mtd_block_t *)malloc(alloc_size);
        if (topo == NULL) {
            dbg_print(is_debug_on, "Error: malloc mtd topology failed, count=%u.\n", count);
            return -ENOMEM;
        }
        topo->block_num = 0;
        
        dbg_print(is_debug_on, "No child partition found for mtd %s.\n", mtd_name);
        *block_info = topo;
        return FIRWMARE_MTD_SUCCESS;
    }

    if (count > UINT8_MAX) {
        dbg_print(is_debug_on, "Error: too many child partitions for mtd %s, count=%u exceeds max %u.\n",
                  mtd_name, count, UINT8_MAX);
        closedir(dir);
        return -EINVAL;
    }

    alloc_size = sizeof(firmware_mtd_block_t) + (sizeof(topo->block[0]) * count);
    topo = (firmware_mtd_block_t *)malloc(alloc_size);
    if (topo == NULL) {
        closedir(dir);
        dbg_print(is_debug_on, "Error: malloc mtd topology failed, count=%u.\n", count);
        return -ENOMEM;
    }
    mem_clear(topo, alloc_size);
    topo->block_num = (uint8_t)count;

    rewinddir(dir);
    count = 0;
    ret = 0;
    /* get every partition information, including partition name, upgrade data offset and size, etc., for subsequent upgrade use */
    while ((entry = readdir(dir)) != NULL) {
        if (!fw_up_is_child_mtd_entry(entry->d_name, mtd_name)) {
            continue;
        }

        snprintf(topo->block[count].name, sizeof(topo->block[count].name), "%s", entry->d_name);

        /* parse offset and size from sysfs, if failed, return error, because these info are necessary for upgrade */
        ret = fw_up_read_mtd_attr_u32(entry->d_name, "offset", &topo->block[count].offset);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "Error: read offset attribute failed for mtd %s.\n", entry->d_name);
            break;
        }

        ret = fw_up_read_mtd_attr_u32(entry->d_name, "size", &topo->block[count].size);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "Error: read size attribute failed for mtd %s.\n", entry->d_name);
            break;
        }

        count++;
        if (count >= topo->block_num) {
            dbg_print(is_debug_on, "Count of child partitions for mtd %s exceeds expected block_num %u at child %s, stop parsing further.\n",
                      mtd_name, topo->block_num, entry->d_name);
            break;
        }
    }

    if (ret != 0) {
        free(topo);
        closedir(dir);
        return ret;
    }

    closedir(dir);
    *block_info = topo;
    return FIRMWARE_SUCCESS;
}

void firmware_upgrade_mtd_topo_blk_free(firmware_mtd_block_t **block_info)
{
    if ((block_info == NULL) || (*block_info == NULL)) {
        dbg_print(is_debug_on, "Error: Invalid parameter for firmware_upgrade_mtd_topo_blk_free.\n");
        return;
    }

    free(*block_info);
    *block_info = NULL;
}

static int firmware_sysfs_get_dev_info(int fd, firmware_mtd_info_t *dev_info)
{
    int ret;

    ret = ioctl(fd, FIRMWARE_SYSFS_MTD_INFO, dev_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error: Failed to get upg device file info.\n");
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

    /* TODO：optimization suggestion, process data according to erase block size, first read and compare data, skip if the data is the same, erase and write if different
     * This can reduce the number of flash erases, extend flash life, and flash read performance is usually much higher than erase and write performance. In the BIOS upgrade scenario,
     * a large part of the MTD data does not change frequently during version upgrades, such as INTEL firmware, microcode firmware, etc.
     */
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
    char mtd_dev_name[FIRMWARE_DEV_NAME_LEN];
    char dev_mtd[PATH_LEN];
    int mtd_fd;
    uint32_t offset, len, block_size;
    struct mtd_info_user mtd_info;
    uint8_t *data_point;

    mem_clear(mtd_dev_name, sizeof(mtd_dev_name));
    ret = firmware_upgrade_mtd_get_dev_name(dev_info->mtd_name, mtd_dev_name, sizeof(mtd_dev_name));
    if (ret < 0) {
        dbg_print(is_debug_on, "Error: get mtd device name failed, ret=%d.\n", ret);
        return FIRMWARE_FAILED;
    }

    mem_clear(dev_mtd, sizeof(dev_mtd));
    snprintf(dev_mtd, sizeof(dev_mtd) - 1, "/dev/%s", mtd_dev_name);

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

static int firmware_upgrade_mtd_program_fragment(firmware_mtd_info_t *dev_info, firmware_mtd_block_t *block_info)
{
    int ret;
    uint32_t offset, size;
    uint8_t *data_buf;

    if (block_info->org_data_buf == NULL || block_info->org_data_size == 0) {
        dbg_print(is_debug_on, "Error: Invalid input for program fragment.\n");
        return FIRMWARE_FAILED;
    }

    /* according to partition information, perform segmented upgrade. 
     * The upgrade only operates on the entire MTD device, not on sub-partitions, and can reuse the old API
     */
    for (int i = 0; i < block_info->block_num; i++) {
        if (block_info->block[i].skip_flag) {
            dbg_print(is_debug_on, "Skip partition %s offset=0x%x size=0x%x.\n",
                        block_info->block[i].name, block_info->block[i].offset, block_info->block[i].size);
            continue;
        }

        offset = dev_info->flash_base + block_info->block[i].offset;
        size = block_info->block[i].size;
        data_buf = block_info->org_data_buf + (offset - dev_info->flash_base);
        if ((offset + size) > (dev_info->flash_base + block_info->org_data_size)) {
            dbg_print(is_debug_on, "Error: partition %s offset and size exceed data buffer, offset=0x%x size=0x%x data_buf_size=0x%x.\n",
                        block_info->block[i].name, offset, size, block_info->org_data_size);
            return FIRMWARE_FAILED;
        }

        dbg_print(is_debug_on, "Program partition %s offset=0x%x size=0x%x.\n",
                    block_info->block[i].name, offset, size);

        ret = firmware_upgrade_mtd_program(dev_info, offset, data_buf, size);
        if (ret < 0) {
            dbg_print(is_debug_on, "Error: program mtd fragment failed for partition %s.\n", block_info->block[i].name);
            return FIRMWARE_FAILED;
        }
    }

    return FIRMWARE_SUCCESS;
}

/* Keep user configuration partitions by marking uconf blocks as skip and upgrading fragments. */
static int firmware_upgrade_mtd_program_keep_uconf(firmware_mtd_info_t *dev_info,
                firmware_mtd_upg_ctx_t *mtd_ctx, uint8_t *buf, uint32_t size)
{
    int ret;
    char topo_mtd_name[FIRMWARE_DEV_NAME_LEN];
    firmware_mtd_block_t *block_info;

    if ((dev_info == NULL) || (mtd_ctx == NULL) || (buf == NULL)) {
        dbg_print(is_debug_on, "Error: Invalid input for keep-uconf upgrade.\n");
        return FIRMWARE_FAILED;
    }

    block_info = mtd_ctx->block_info;
    if (block_info == NULL) {
        mem_clear(topo_mtd_name, sizeof(topo_mtd_name));
        ret = firmware_upgrade_mtd_get_dev_name(dev_info->mtd_name, topo_mtd_name, sizeof(topo_mtd_name));
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "Error: get real mtd dev name failed for label %s.\n", dev_info->mtd_name);
            return FIRMWARE_FAILED;
        }

        ret = firmware_upgrade_mtd_topo_blk_alloc(topo_mtd_name, &block_info);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "Error: get mtd topology failed for %s.\n", topo_mtd_name);
            return FIRMWARE_FAILED;
        }

        mtd_ctx->block_info = block_info;
    }

    ret = fw_up_mtd_mark_uconf_skip(dev_info->mtd_name, block_info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: mark uconf partitions failed for %s.\n", dev_info->mtd_name);
        return FIRMWARE_FAILED;
    }

    block_info->org_data_buf = buf;
    block_info->org_data_size = size;
    ret = firmware_upgrade_mtd_program_fragment(dev_info, block_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error:mtd device program failed, ret=%d.\n", ret);
        return FIRMWARE_FAILED;
    }

    return FIRMWARE_SUCCESS;
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
    int uconf_skip;
    firmware_mtd_info_t dev_info;
    firmware_mtd_upg_ctx_t *mtd_ctx;

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

    mtd_ctx = fw_up_mtd_ctx_from_priv(info->priv_data);
    if (mtd_ctx != NULL) {
        uconf_skip = mtd_ctx->uconf_skip;
    } else {
        /* For backward compatibility, if there is no context info, do not skip user config partitions, perform a full upgrade,
         * to avoid upgrade failure due to missing context
         */
        dbg_print(is_debug_on, "No mtd upgrade context found, will not skip uconf partition for upgrade.\n");
        uconf_skip = 0;
    }

    /* user want to perform multi-partition upgrade, preserve user configuration partitions */
    if (uconf_skip) {
        ret = firmware_upgrade_mtd_program_keep_uconf(&dev_info, mtd_ctx, buf, size);
        if (ret != FIRMWARE_SUCCESS) {
            goto failed;
        }
    } else {
        ret = firmware_upgrade_mtd_program(&dev_info, dev_info.flash_base, buf, size);
        if (ret < 0) {
            dbg_print(is_debug_on, "Error:mtd device program failed, ret=%d.\n", ret);
            goto failed;
        }
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
    char mtd_dev_name[FIRMWARE_DEV_NAME_LEN];
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
        /* If the test_size not configure, check whether the mtd device exists */
        mem_clear(mtd_dev_name, sizeof(mtd_dev_name));
        ret = firmware_upgrade_mtd_get_dev_name(dev_info.mtd_name, mtd_dev_name, sizeof(mtd_dev_name));
        if (ret < 0) {
            dbg_print(is_debug_on, "Error:not find %s mtd num.\n", dev_info.mtd_name);
            return FIRMWARE_FAILED;
        }
        return FIRMWARE_SUCCESS;
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
