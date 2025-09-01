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
#include <signal.h>
#include <firmware_app.h>

int header_offset;

static firmware_file_name_t firmware_file_str[] = {
    {"VME",             FIRMWARE_VME},
    {"ISC",             FIRMWARE_ISC},
    {"JBI",             FIRMWARE_JBI},
    {"SPI-LOGIC-DEV",   FIRMWARE_SPI_LOGIC_DEV},
    {"SYSFS",           FIRMWARE_SYSFS_DEV},
    {"MTD",             FIRMWARE_MTD},
    {"VME-I2C",         FIRMWARE_ISPVME_I2C},
    {"SYSFS-XDPE132",   FIRMWARE_SYSFS_XDPE132},
};

/**
 * firmware_error_type
 * function:set error code
 * @action: param[in]  The stage where the error occurs
 * @info: param[in] Upgrade file information
 * return value: error code
 */
int firmware_error_type(int action, name_info_t *info)
{
    if (info == NULL) {
        return ERR_FW_UPGRADE;
    }

    if((info->type <= FIRMWARE_UNDEF_TYPE) || (info->type > FIRMWARE_OTHER)) {
        return ERR_FW_UPGRADE;
    }

    if (info->type == FIRMWARE_CPLD) {
        switch (action) {
        case FIRMWARE_ACTION_CHECK:
            return ERR_FW_CHECK_CPLD_UPGRADE;
        case FIRMWARE_ACTION_MATCH:
            return ERR_FW_MATCH_CPLD_UPGRADE;
        case FIRMWARE_ACTION_VERCHECK:
            return ERR_FW_SAMEVER_CPLD_UPGRADE;
        case FIRMWARE_ACTION_UPGRADE:
            return ERR_FW_DO_CPLD_UPGRADE;
        case FIRMWARE_ACTION_SUPPORT:
            return ERR_FW_DO_UPGRADE_NOT_SUPPORT;
        default:
            return ERR_FW_UPGRADE;
        }
    } else if (info->type == FIRMWARE_FPGA) {
        switch (action) {
        case FIRMWARE_ACTION_CHECK:
            return ERR_FW_CHECK_FPGA_UPGRADE;
        case FIRMWARE_ACTION_MATCH:
            return ERR_FW_MATCH_FPGA_UPGRADE;
        case FIRMWARE_ACTION_VERCHECK:
            return ERR_FW_SAMEVER_FPGA_UPGRADE;
        case FIRMWARE_ACTION_UPGRADE:
            return ERR_FW_DO_FPGA_UPGRADE;
        case FIRMWARE_ACTION_SUPPORT:
            return ERR_FW_DO_UPGRADE_NOT_SUPPORT;
        default:
            return ERR_FW_UPGRADE;
        }
    } else {
        switch (action) {
        case FIRMWARE_ACTION_CHECK:
            return ERR_FW_CHECK_UPGRADE;
        case FIRMWARE_ACTION_MATCH:
            return ERR_FW_MATCH_UPGRADE;
        case FIRMWARE_ACTION_VERCHECK:
            return ERR_FW_SAMEVER_UPGRADE;
        case FIRMWARE_ACTION_UPGRADE:
            return ERR_FW_DO_UPGRADE;
        case FIRMWARE_ACTION_SUPPORT:
            return ERR_FW_DO_UPGRADE_NOT_SUPPORT;
        default:
            return ERR_FW_UPGRADE;
        }
    }

}

/*
 * firmware_check_file_info
 * function:Check the file information to determine that the file is available for use on the device
 * @info: param[in] Upgrade file information
 * @main_type : param[in] main type
 * @sub_type : param[in] sub type
 * @slot : param[in] 0--main, sub slot starts at 1
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
static int firmware_check_file_info(name_info_t *info, int main_type, int sub_type, int slot)
{
    int i;

    dbg_print(is_debug_on, "Check file info.\n");
    /* Check the mainboard type */
    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (main_type == info->card_type[i]) {
            dbg_print(is_debug_on, "main type is 0x%x \n", main_type);
            break;
        }
    }
    if (i == MAX_DEV_NUM) {
        dbg_print(is_debug_on, "Error: The main type[0x%x] is not matched \n", main_type);
        return firmware_error_type(FIRMWARE_ACTION_MATCH, info);
    }

    /* Check the sub board type, if firwmare upgrade sub board, then sub_type must be 0 */
    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (sub_type == info->sub_type[i]) {
            dbg_print(is_debug_on, "sub type is 0x%x \n", sub_type);
            break;
        }
    }
    if (i == MAX_DEV_NUM) {
        dbg_print(is_debug_on, "Error: The sub type[0x%x] is not matched \n", sub_type);
        return firmware_error_type(FIRMWARE_ACTION_MATCH, info);
    }

    /* if firwmare upgrade main board,  then sub_type must be 0 and slot must be 0
    * if firwmare upgrade sub board, then sub_type must not be 0 and slot must not be 0 */
    if (((sub_type != 0) && (slot < 1)) || ((sub_type == 0) && (slot != 0))) {
        dbg_print(is_debug_on, "Error: The sub type[0x%x] is not match slot %d error.\n", sub_type, slot);
        return firmware_error_type(FIRMWARE_ACTION_MATCH, info);
    }

    dbg_print(is_debug_on, "Success check file info.\n");

    return FIRMWARE_SUCCESS;
}

/*
 * firmware_get_dev_file_name
 * function:Gets the name of the device file
 * @info: param[in] Upgrade file information
 * @len:  param[in] Device file name length
 * @file_name: param[out] Device file name
 */
static int firmware_get_dev_file_name(name_info_t *info, char *file_name, int len)
{
    int ret;

    ret = FIRMWARE_SUCCESS;
    switch(info->file_type) {
    case FIRMWARE_VME:
        snprintf(file_name, len, "/dev/firmware_cpld_ispvme%d", info->chain);
        break;
    case FIRMWARE_ISC:
    case FIRMWARE_JBI:
        snprintf(file_name, len, "/dev/firmware_cpld%d", info->chain);
        break;
    case FIRMWARE_SPI_LOGIC_DEV:
    case FIRMWARE_SYSFS_DEV:
    case FIRMWARE_MTD:
    case FIRMWARE_SYSFS_XDPE132:
        snprintf(file_name, len, "/dev/firmware_sysfs%d", info->chain);
        break;
    case FIRMWARE_ISPVME_I2C:
        (void)snprintf(file_name, len, "/dev/"ISPVME_I2C_MISCDEV_NAME_PREFIX"%u", info->chain);
        break;

    default:
        ret = FIRMWARE_FAILED;
        break;
    }

    return ret;
 }

/**
 * firmware_check_chip_verison
 * function: Check chip version
 * @fd:   param[in] Device file descriptor
 * @info: param[in] Upgrade file information
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
int firmware_check_chip_verison(int fd, name_info_t *info)
{
    int ret;
    cmd_info_t cmd_info;
    char version[FIRMWARE_NAME_LEN + 1];

    dbg_print(is_debug_on, "Check chip version.\n");
    mem_clear(version, sizeof(version));
    cmd_info.size = FIRMWARE_NAME_LEN;
    cmd_info.data = (void *) version;

    /* Ignore version checking */
    if (strncmp("v", info->version, 1) == 0) {
        dbg_print(is_debug_on, "Skip check chip version.\n");
        return FIRMWARE_SUCCESS;
    }

    /* Get the program version from the device file */
    ret = ioctl(fd, FIRMWARE_GET_VERSION, &cmd_info);
    if (ret < 0) {
        dbg_print(is_debug_on, "Error: Failed to get version(chain %d, version %s).\n",
                    info->chain, info->version);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, NULL);
    }
    dbg_print(is_debug_on, "Chip verion: %s, file chip verion: %s.\n", version, info->version);

    /* The device version is the same and does not upgrade */
    if (strcmp(version, info->version) == 0) {
        dbg_print(is_debug_on, "the file program version is same as the firmware version %s \n",
                    info->version);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
    dbg_print(is_debug_on, "Check version pass.\n");

    return FIRMWARE_SUCCESS;
}

/*
 * firmware_get_file_size
 * function: Gets the upgrade file size
 * @file_name: param[in]  Upgrade file name
 * @size: param[out] Upgrade file size
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
static int firmware_get_file_size(char *file_name, uint32_t *size)
{
    int ret;
    struct stat buf;

    ret = stat(file_name, &buf);
    if (ret < 0) {
        return FIRMWARE_FAILED;
    }

    if (buf.st_size < 0 || buf.st_size - header_offset < 0) {
        return FIRMWARE_FAILED;
    }
    /* Remove the upgrade file header information to actually upgrade the content size */
    *size = buf.st_size - header_offset;

    return FIRMWARE_SUCCESS;
}

/*
 * firmware_get_file_info
 * function: Gets the contents of the upgrade file
 * @file_name: param[in]   Upgrade file name
 * @size:  param[in]  Upgrade file size
 * @buf:   param[out] Upgrade the file content
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
static int firmware_get_file_info(char *file_name, uint8_t *buf, uint32_t size)
{
    FILE *fp;
    int len;
    int ret;

    fp = fopen(file_name, "r");
    if (fp == NULL) {
        return FIRMWARE_FAILED;
    }
    /* Removes the contents of the upgrade file header information */
    ret = fseek(fp, header_offset, SEEK_SET);
    if (ret < 0) {
        fclose(fp);
        return FIRMWARE_FAILED;
    }

    len = fread(buf, size, 1, fp);
    if (len < 0) {
        fclose(fp);
        return FIRMWARE_FAILED;
    }
    fclose(fp);

    return FIRMWARE_SUCCESS;
}

/*
* firmware_upgrade
* function: firmware upgrade
* @file_name:   param[in] Upgrade file name
* @info:        param[in] Upgrade file information
* return value : success--FIRMWARE_SUCCESS, other fail return error code
*/
static int firmware_upgrade(char *file_name, name_info_t *info)
{
    int ret;
    int fd;
    uint32_t upg_size;
    uint8_t *upg_buf;
    char dev_file_name[FIRMWARE_NAME_LEN];
    unsigned long crc;

    dbg_print(is_debug_on, "Upgrade firmware: %s.\n", file_name);
    mem_clear(dev_file_name, FIRMWARE_NAME_LEN);
    ret = firmware_get_dev_file_name(info, dev_file_name, FIRMWARE_NAME_LEN - 1);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to get dev file name.\n");
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    fd = open(dev_file_name, O_RDWR);
    if (fd < 0) {
        dbg_print(is_debug_on, "Error: Failed to open %s.\n", dev_file_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

#if 0
    /* check chip name */
    ret = firmware_check_chip_name(fd, info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to check chip name: %s.\n", dev_file_name);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
#endif

    /* Check chip version */
    ret = firmware_check_chip_verison(fd, info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to check chip version: %s.\n", dev_file_name);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    /* Gets the upgrade file size */
    ret = firmware_get_file_size(file_name, &upg_size);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to get file size: %s.\n", file_name);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    if (upg_size == 0) {
        dbg_print(is_debug_on, "Error: The upgrade file is empty \n");
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    upg_buf = (uint8_t *) malloc(upg_size + 1);
    if (upg_buf == NULL) {
        dbg_print(is_debug_on, "Error: Failed to malloc memory for upgrade file info: %s.\n",
                  dev_file_name);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    /* Gets the contents of the upgrade file */
    mem_clear(upg_buf, upg_size + 1);
    ret = firmware_get_file_info(file_name, upg_buf, upg_size);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to read file info: %s.\n", file_name);
        free(upg_buf);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    if (info->header_exist != FIRMWARE_WITHOUT_HEADER) {
        /* file crc32 check */
        crc = crc32(0, (const unsigned char *)upg_buf, (unsigned int)upg_size);
        if (crc != info->crc32) {
            dbg_print(is_debug_on, "Error: Failed to check file crc: %s.\n", file_name);
            dbg_print(is_debug_on, "the crc value is : %#08x.\n", (unsigned int)crc);
            free(upg_buf);
            close(fd);
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
    }

    dbg_print(is_debug_on, "Start upgrading firmware, wait...\n");

    /* Start firmware upgrade */
    switch (info->file_type) {
    case FIRMWARE_VME:
        dbg_print(is_debug_on, "start to ispvme upgrade: %s.\n", file_name);
        ret = firmware_upgrade_ispvme(fd, file_name, info);
        break;
    case FIRMWARE_ISC:
    case FIRMWARE_JBI:
        dbg_print(is_debug_on, "start to upgrade: %s.\n", file_name);
        ret = firmware_upgrade_jtag(fd, upg_buf, upg_size, info);
        break;
    case FIRMWARE_SPI_LOGIC_DEV:
        dbg_print(is_debug_on, "start to spi logic dev upgrade: %s.\n", file_name);
        ret = firmware_upgrade_spi_logic_dev(fd, upg_buf, upg_size, info);
        break;
    case FIRMWARE_SYSFS_DEV:
        dbg_print(is_debug_on, "start to sysfs upgrade: %s.\n", file_name);
        ret = firmware_upgrade_sysfs(fd, upg_buf, upg_size, info);
        break;
    case FIRMWARE_MTD:
        dbg_print(is_debug_on, "start to mtd device upgrade: %s.\n", file_name);
        ret = firmware_upgrade_mtd(fd, upg_buf, upg_size, info);
        break;
    case FIRMWARE_ISPVME_I2C:
        dbg_print(is_debug_on, "start to ispvme i2c device upgrade: %s.\n", file_name);
        ret = firmware_upgrade_ispvme_i2c(fd, file_name, header_offset);
        break;
    case FIRMWARE_SYSFS_XDPE132:
        dbg_print(is_debug_on, "start to sysfs xdpe132 upgrade: %s.\n", file_name);
        ret = firmware_upgrade_sysfs_xdpe132(fd, file_name, header_offset);
        break;
    default:
        dbg_print(is_debug_on, "Error: file type is not support: %s.\n", file_name);
        free(upg_buf);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_UPGRADE, info);
    }

    dbg_print(is_debug_on, "Completed.\n");
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to upgrade: %s.\n", dev_file_name);
        free(upg_buf);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_UPGRADE, info);
    }

    free(upg_buf);
    close(fd);

    return FIRMWARE_SUCCESS;
}

/*
* firmware_upgrade_test
* function: firmware upgrade test
* @file_name:   param[in] Upgrade file name
* @info:        param[in] Upgrade file information
* return value : success--FIRMWARE_SUCCESS, other fail return error code
*/
static int firmware_upgrade_test(char *file_name, name_info_t *info)
{
    int ret;
    int fd;
    uint32_t upg_size;
    uint8_t *upg_buf;
    char dev_file_name[FIRMWARE_NAME_LEN];
    unsigned long crc;

    dbg_print(is_debug_on, "Upgrade firmware test: %s.\n", file_name);
    mem_clear(dev_file_name, FIRMWARE_NAME_LEN);
    ret = firmware_get_dev_file_name(info, dev_file_name, FIRMWARE_NAME_LEN - 1);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to get dev file name.\n");
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    fd = open(dev_file_name, O_RDWR);
    if (fd < 0) {
        dbg_print(is_debug_on, "Error: Failed to open %s.\n", dev_file_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

#if 0
    /* check chip name */
    ret = firmware_check_chip_name(fd, info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to check chip name: %s.\n", dev_file_name);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
#endif

    /* Check chip version */
    ret = firmware_check_chip_verison(fd, info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to check chip version: %s.\n", dev_file_name);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    /* Gets the upgrade file size */
    ret = firmware_get_file_size(file_name, &upg_size);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to get file size: %s.\n", file_name);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    upg_buf = (uint8_t *) malloc(upg_size + 1);
    if (upg_buf == NULL) {
        dbg_print(is_debug_on, "Error: Failed to malloc memory for upgrade file info: %s.\n",
                  dev_file_name);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    /* Gets the contents of the upgrade file */
    mem_clear(upg_buf, upg_size + 1);
    ret = firmware_get_file_info(file_name, upg_buf, upg_size);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to read file info: %s.\n", file_name);
        free(upg_buf);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    /* file crc32 check */
    crc = crc32(0, (const unsigned char *)upg_buf, (unsigned int)upg_size);
    if (crc != info->crc32) {
        dbg_print(is_debug_on, "Error: Failed to check file crc: %s.\n", file_name);
        dbg_print(is_debug_on, "the crc value is : %#08x.\n", (unsigned int)crc);
        free(upg_buf);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    dbg_print(is_debug_on, "Start upgrading firmware test, wait...\n");

    /* Start firmware upgrade */
    switch (info->file_type) {
    case FIRMWARE_VME:
        dbg_print(is_debug_on, "start to ispvme upgrade test: %s.\n", file_name);
        /* WME upgrade link testing is the same as upgrading, using vme test file. */
        ret = firmware_upgrade_ispvme(fd, file_name, info);
        break;
    case FIRMWARE_ISC:
    case FIRMWARE_JBI:
        dbg_print(is_debug_on, "start to upgrade test: %s.\n", file_name);
        ret = firmware_upgrade_jtag_test(fd, upg_buf, upg_size, info);
        break;
    case FIRMWARE_SPI_LOGIC_DEV:
        dbg_print(is_debug_on, "start to spi logic dev upgrade test: %s.\n", file_name);
        ret = firmware_upgrade_spi_logic_dev_test(fd,info);
        break;
    case FIRMWARE_SYSFS_DEV:
        dbg_print(is_debug_on, "start to sysfs upgrade test: %s.\n", file_name);
        ret = firmware_upgrade_sysfs_test(fd, info);
        break;
    case FIRMWARE_MTD:
        dbg_print(is_debug_on, "start to mtd device upgrade test: %s.\n", file_name);
        ret = firmware_upgrade_mtd_test(fd, info);
        break;
    case FIRMWARE_ISPVME_I2C:
        dbg_print(is_debug_on, "start to ispvme i2c device upgrade: %s.\n", file_name);
        ret = firmware_upgrade_ispvme_i2c(fd, file_name, header_offset);
        break;
    default:
        dbg_print(is_debug_on, "Error: test file type is not support: %s.\n", file_name);
        free(upg_buf);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_UPGRADE, info);
    }

    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Error: Failed to upgrade test: %s ret=%d.\n", dev_file_name, ret);
        free(upg_buf);
        close(fd);
        if (ret == FIRMWARE_NOT_SUPPORT) {
            return firmware_error_type(FIRMWARE_ACTION_SUPPORT, info);
        } else {
            return firmware_error_type(FIRMWARE_ACTION_UPGRADE, info);
        }
    }

    free(upg_buf);
    close(fd);

    return FIRMWARE_SUCCESS;
}

/*
 * firmware_upgrade_file_type_map
 * function:Gets the corresponding upgrade file type from the upgrade file type list
 * @value : param[in] file type name
 * return value : file type, firmware_file_type_t
 */
static firmware_file_type_t firmware_upgrade_file_type_map(char *type_str)
{
    int type_num;
    int i, org_len, dst_len;

    type_num = (sizeof(firmware_file_str) /sizeof(firmware_file_str[0]));
    for (i = 0; i < type_num; i++) {
        org_len = strlen(firmware_file_str[i].firmware_file_name_str);
        dst_len = strlen(type_str);
        if (org_len != dst_len) {
            continue;
        }
        if (!strncmp(firmware_file_str[i].firmware_file_name_str, type_str, org_len)) {
            return firmware_file_str[i].firmware_file_type;
        }
    }

    dbg_print(is_debug_on, "firmware file type unknown\n");
    return FIRMWARE_NONE;
}

/*
 * firmware_upgrade_parse_kv
 * function:Parses the header information of the upgrade file based on the key and value
 * @key: param[in] key
 * @value : param[in] value
 * @info : param[out]  Upgrade file information
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
static int firmware_upgrade_parse_kv(const char *key, const char *value, name_info_t *info)
{
    int i;
    if (key == NULL || value == NULL) {
        dbg_print(is_debug_on, "Error: failed to get ther key or value.\n");
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    } else if (strcmp(key, FILEHEADER_DEVTYPE) == 0) {
        /* main board type */
        for (i = 0; i < MAX_DEV_NUM && info->card_type[i]; i++);
        if (i == MAX_DEV_NUM) {
            dbg_print(is_debug_on, "Error: card type is full for %s. \n", value);
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
        info->card_type[i] = strtoul(value, NULL, 0);
    } else if (strcmp(key, FILEHEADER_SUBTYPE) == 0) {
        /* sub board type */
        for (i = 0; i < MAX_DEV_NUM && info->sub_type[i]; i++);
        if (i == MAX_DEV_NUM) {
            dbg_print(is_debug_on, "Error: sub type is full for %s. \n", value);
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
        info->sub_type[i] = strtoul(value, NULL, 0);
    } else if (strcmp(key, FILEHEADER_TYPE) == 0) {
        /* Device type */
        if (strcmp(value, FIRMWARE_CPLD_NAME) == 0) {
            info->type = FIRMWARE_CPLD;
        } else if (strcmp(value, FIRMWARE_FPGA_NAME) == 0) {
            info->type = FIRMWARE_FPGA;
        } else {
            info->type = FIRMWARE_OTHER;
        }
    } else if (strcmp(key, FILEHEADER_CHAIN) == 0) {
        /* link num */
        for (i = 0; i < FIRMWARE_SLOT_MAX_NUM && (info->chain_list[i] != FIRMWARE_INVALID_CHAIN); i++);
        if (i == FIRMWARE_SLOT_MAX_NUM) {
            dbg_print(is_debug_on, "Error:chain is full for %s. \n", value);
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
        info->chain_list[i] = strtoul(value, NULL, 0);
    } else if (strcmp(key, FILEHEADER_CHIPNAME) == 0) {
        /* chip name */
        if (strlen(value) >= FIRMWARE_NAME_LEN) {
            dbg_print(is_debug_on, "Error: '%s' is too long for a chipname.\n", value);
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
        mem_clear(info->chip_name, sizeof(info->chip_name));
        snprintf(info->chip_name, sizeof(info->chip_name), "%s", value);
    } else if (strcmp(key, FILEHEADER_VERSION) == 0) {
        /* version */
        if (strlen(value) >= FIRMWARE_NAME_LEN) {
            dbg_print(is_debug_on, "Error: '%s' is too long for a version.\n", value);
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
        mem_clear(info->version, sizeof(info->version));
        snprintf(info->version, sizeof(info->version), "%s", value);
    } else if (strcmp(key, FILEHEADER_FILETYPE) == 0) {
        /* file type */
        info->file_type = firmware_upgrade_file_type_map((char *)value);
    } else if (strcmp(key, FILEHEADER_CRC) == 0) {
        /* file crc32 */
        info->crc32 = strtoul(value, NULL, 0);
    } else {
        dbg_print(is_debug_on, "Warning: key '%s' is unknown. Continue anyway.\n", key);
        return FIRMWARE_SUCCESS;
    }
    dbg_print(is_debug_on, "key %s is matched.\n", key);
    return FIRMWARE_SUCCESS;
 }

/*
 * firmware_upgrade_parse_check
 * function:Check the results of header parsing
 * @file_name: Upgrade file name
 * @info : Upgrade file information
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
static int firmware_upgrade_parse_check(char *file_name, name_info_t *info)
{
    int i;
    if (info->card_type[0] == 0) {
        dbg_print(is_debug_on, "Error: %s card type is missing.\n", file_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
    if (info->chain_list[0] == FIRMWARE_INVALID_CHAIN) {
        dbg_print(is_debug_on, "Error: %s chain is missing.\n", file_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
    if ((info->type <= FIRMWARE_UNDEF_TYPE) || (info->type > FIRMWARE_OTHER)) {
        dbg_print(is_debug_on, "Error: %s type is unknown.\n", file_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
    if (strlen(info->chip_name) == 0) {
        dbg_print(is_debug_on, "Error: %s chip_name is empty.\n", file_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
    if (strlen(info->version) == 0) {
        dbg_print(is_debug_on, "Error: %s version is empty.\n", file_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
    if ((info->file_type <= FIRMWARE_UNDEF_FILE_TYPE) || (info->file_type > FIRMWARE_NONE)) {
        dbg_print(is_debug_on, "Error: %s file type is unknown.\n", file_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
    dbg_print(is_debug_on, "The file header parse:(%s) \n" , file_name);
    dbg_print(is_debug_on, "    card type: ");
    for (i = 0; i < MAX_DEV_NUM && info->card_type[i]; i++){
        dbg_print(is_debug_on, "0x%x, ", info->card_type[i]);
     }
    dbg_print(is_debug_on, "\n"
                           "    sub type : ");
    for (i = 0; i < MAX_DEV_NUM && info->sub_type[i]; i++){
        dbg_print(is_debug_on, "0x%x, ", info->sub_type[i]);
    }
    dbg_print(is_debug_on, "    chain: ");
    for (i = 0; i < FIRMWARE_SLOT_MAX_NUM && (info->chain_list[i] != FIRMWARE_INVALID_CHAIN); i++){
        dbg_print(is_debug_on, "0x%x, ", info->chain_list[i]);
     }

    dbg_print(is_debug_on, "\n"
                           "    type     : %d, \n"
                           "    chip name: %s \n"
                           "    version  : %s \n"
                           "    file type: %d \n"
                           "    the crc32 value: %#x \n",
    info->type, info->chip_name, info->version, info->file_type, info->crc32);
    return FIRMWARE_SUCCESS;
}

/*
 * firmware_upgrade_read_header
 * function:Read the header information of the upgrade file
 * @file_name: param[in] Upgrade file name
 * @info : param[out]  Upgrade file information
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
static int firmware_upgrade_read_header(char *file_name, name_info_t *info)
{
    FILE *fp;
    char *charp;
    char *charn;
    char header_buffer[MAX_HEADER_SIZE];
    char header_key[MAX_HEADER_KV_SIZE];
    char header_var[MAX_HEADER_KV_SIZE];
    int ret, len, i;

    fp = fopen(file_name, "r");
    if (fp == NULL) {
        dbg_print(is_debug_on, "Error: Failed to open file: %s. \n", file_name);
        perror("fopen");
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    mem_clear(header_buffer, sizeof(header_buffer));
    len = fread(header_buffer, MAX_HEADER_SIZE - 1, 1, fp);
    fclose(fp);
    if (len < 0) {
        dbg_print(is_debug_on, "Error: Failed to read header : %s. \n", file_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
    header_buffer[MAX_HEADER_SIZE - 1] = 0;

    info->header_exist = FIRMWARE_WITH_HEADER;
    charp = strstr(header_buffer, "FILEHEADER(\n");
    if (charp == NULL) {
        info->header_exist = FIRMWARE_WITHOUT_HEADER;
        dbg_print(is_debug_on, "Parse: The file %s has no header. \n",  file_name);
        return FIRMWARE_SUCCESS;
    }
    charp += strlen("FILEHEADER(\n");

    dbg_print(is_debug_on, "File parse start.\n");
    mem_clear(info, sizeof(name_info_t));
    /* init chain_list value to FIRMWARE_INVALID_CHAIN */
    for (i = 0; i < FIRMWARE_SLOT_MAX_NUM; i++) {
        info->chain_list[i] = FIRMWARE_INVALID_CHAIN;
    }
    ret = 0;
    charn = charp;
    mem_clear(header_key, sizeof(header_key));
    while (*charn != ')') {
        charn = strpbrk(charp, "=,)\n");
        if (charn == NULL) {
            dbg_print(is_debug_on, "Error: The parser can't find mark.\n");
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
        if (charn - charp >= MAX_HEADER_KV_SIZE) {
            dbg_print(is_debug_on, "Error: The parser find a overflow mark.\n");
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
        switch (*charn) {
        case '=':
            mem_clear(header_key, sizeof(header_key));
            memcpy(header_key, charp, charn - charp);
            break;
        case '\n':
        case ',':
            mem_clear(header_var, sizeof(header_var));
            memcpy(header_var, charp, charn - charp);
            dbg_print(is_debug_on, "Parser: %s = %s .\n", header_key, header_var);
            firmware_upgrade_parse_kv(header_key, header_var, info);
            break;
        case ')':
            break;
        default:
            dbg_print(is_debug_on, "Error: The parser get unexpected mark '%c(0x%02X)'.\n", *charn, *charn);
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
        charp = (charn + 1);
    }

    ret = firmware_upgrade_parse_check(file_name, info);
    if (ret != FIRMWARE_SUCCESS) {
        return FIRMWARE_FAILED;
    }

    header_offset = charp + 1 - header_buffer; /* charp at '\n' */
    dbg_print(is_debug_on,"the header offset is  %d \n", header_offset);
    return FIRMWARE_SUCCESS;
}

/*
 * firmware_upgrade_one_file_get_info_argc7
 * function: get upgrade info for argc7: chain and file_type are specified
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
static int firmware_upgrade_one_file_get_info_argc7(int argc, char *argv[], name_info_t *info, int *chain_value)
{
    int ret, i;
    int main_type, sub_type, slot, chain, file_type;
    char *file_name, *file_type_str;

    if (info == NULL) {
        dbg_print(is_debug_on,
                "Failed firmware_upgrade_one_file parameter err.\n");
        return FIRMWARE_FAILED;
    }
    file_name = argv[1];
    main_type = strtoul(argv[2], NULL, 0);
    sub_type = strtoul(argv[3], NULL, 0);
    slot = strtoul(argv[4], NULL, 0);

    file_type_str = argv[5];
    chain = strtoul(argv[6], NULL, 0);
    if ((chain < 0) || (file_type_str == NULL)) {
        dbg_print(is_debug_on,
                "Failed firmware_upgrade_one_file parameter err.\n");
        return FIRMWARE_FAILED;
    }
    dbg_print(is_debug_on, "firmware upgrade %s 0x%x 0x%x %d %s 0x%x\n",
            file_name, main_type, sub_type, slot, file_type_str, chain);

    /* Read the header information of the upgrade file */
    mem_clear(info, sizeof(name_info_t));
    ret = firmware_upgrade_read_header(file_name, info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to get file header: %s\n", file_name);
        return ret;
    }

    /* argc=7 + without header:use input para */
    if (info->header_exist == FIRMWARE_WITHOUT_HEADER) {
        info->card_type[0] = main_type;
        info->sub_type[0] = sub_type;
        info->type = FIRMWARE_OTHER;
        *chain_value = chain;
        /* info->chip_name not set */
        snprintf(info->version, sizeof(info->version) - 1, "v1.0"); /* to skip version check */
        info->file_type = firmware_upgrade_file_type_map(file_type_str);
        /* info->crc32  not set */
        header_offset = 0;
    } else {
        /* argc=7 + with header:check input para and header. */
        file_type = firmware_upgrade_file_type_map(file_type_str);
        if (file_type != info->file_type) {
            dbg_print(is_debug_on,
                    "file_type in input para is not equal to the one in header. file_type in input para: %d, file_type in header: %d.\n",
                    file_type, info->file_type);
            return FIRMWARE_FAILED;
        }
        for (i = 0; i < FIRMWARE_SLOT_MAX_NUM; i++) {
            if (info->chain_list[i] == chain) {
                dbg_print(is_debug_on, "Match chain %d in header\n", chain);
                break;
            }
        }
        if (i == FIRMWARE_SLOT_MAX_NUM) {
            dbg_print(is_debug_on, "Can't find chind %d in header\n", chain);
            return FIRMWARE_FAILED;
        }
        *chain_value = chain;
    }

    return FIRMWARE_SUCCESS;
}

/*
 * firmware_upgrade_one_file_get_info_argc5
 * function: get upgrade info for argc5: chain and file_type are not specified
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
static int firmware_upgrade_one_file_get_info_argc5(int argc, char *argv[], name_info_t *info)
{
    int ret;
    int main_type, sub_type, slot;
    char *file_name;

    if (info == NULL) {
        dbg_print(is_debug_on,
                "Failed firmware_upgrade_one_file parameter err.\n");
        return FIRMWARE_FAILED;
    }
    file_name = argv[1];
    main_type = strtoul(argv[2], NULL, 0);
    sub_type = strtoul(argv[3], NULL, 0);
    slot = strtoul(argv[4], NULL, 0);

    dbg_print(is_debug_on, "firmware upgrade %s 0x%x 0x%x %d\n", file_name,
            main_type, sub_type, slot);

    /* Read the header information of the upgrade file */
    mem_clear(info, sizeof(name_info_t));
    ret = firmware_upgrade_read_header(file_name, info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to get file header: %s\n", file_name);
        return ret;
    }

    /* argc=5 + without header: err */
    if (info->header_exist == FIRMWARE_WITHOUT_HEADER) {
        dbg_print(is_debug_on,
                "It is not supported when %s without header and argument number is %d (chain and type is not specified).\n",
                file_name, argc);
        return FIRMWARE_FAILED;
    }

    return FIRMWARE_SUCCESS;
}

/*
 * firmware_upgrade_one_file
 * function: upgrade file
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
static int firmware_upgrade_one_file(int argc, char *argv[])
{
    int ret;
    name_info_t info;
    int main_type, sub_type, slot;
    char *file_name;
    int i, chain_value, totalerr;

    file_name = argv[1];
    main_type = strtoul(argv[2], NULL, 0);
    sub_type = strtoul(argv[3], NULL, 0);
    slot = strtoul(argv[4], NULL, 0);

    /* paramter check1 */
    if ((slot < 0) || (file_name == NULL)) {
        dbg_print(is_debug_on, "Failed firmware_upgrade_one_file parameter err.\n");
        return FIRMWARE_FAILED;
    }

    chain_value = FIRMWARE_INVALID_CHAIN;
    if (argc == 7) {
        ret = firmware_upgrade_one_file_get_info_argc7(argc, argv, &info, &chain_value);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "get_info_argc7 failed: %s.\n",
                    file_name);
            return ret;
        }
    } else if (argc == 5) {
        ret = firmware_upgrade_one_file_get_info_argc5(argc, argv, &info);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "get_info_argc5 failed: %s.\n",
                    file_name);
            return ret;
        }
    } else {
        dbg_print(is_debug_on,
                "Failed firmware_upgrade_one_file argument number: %d parameter err.\n",
                argc);
        return FIRMWARE_FAILED;
    }

    /* Check the file information to determine that the file is available for use on the device */
    ret = firmware_check_file_info(&info, main_type, sub_type, slot);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "File is not match with the device: %s.\n", file_name);
        return ret;
    }

    /* The link number corresponding to the upgrade file is calculated based on the slot number.
       16 links are reserved for each slot. main boade slot is 0. */
    if (chain_value != FIRMWARE_INVALID_CHAIN) {
        info.chain = chain_value + (slot * FIRMWARE_SLOT_MAX_NUM);
        dbg_print(is_debug_on, "Specify chain upgrade, file: %s, slot: %d, chain_value: %d, chain: %d\n",
            file_name, slot, chain_value, info.chain);
        ret = firmware_upgrade(file_name, &info);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "Failed to upgrade: %s, slot: %d, chain_value: %d, chain: %d\n",
                file_name, slot, chain_value, info.chain);
            return ret;
        }
        dbg_print(is_debug_on, "Specify chain upgrade success, file: %s, slot: %d, chain_value: %d, chain: %d\n",
            file_name, slot, chain_value, info.chain);
        return FIRMWARE_SUCCESS;
    }
    /* Traverse all chains for upgrade */
    totalerr = 0;
    for (i = 0; i < FIRMWARE_SLOT_MAX_NUM; i++) {
        if (info.chain_list[i] == FIRMWARE_INVALID_CHAIN) {
            dbg_print(is_debug_on, "End of chain_list, index: %d\n", i);
            break;
        }
        info.chain = info.chain_list[i] + (slot * FIRMWARE_SLOT_MAX_NUM);
        dbg_print(is_debug_on, "Traverse upgrade, file: %s, slot: %d, chain index: %d, chain_value: %d, chain: %d\n",
            file_name, slot, i, info.chain_list[i], info.chain);
        ret = firmware_upgrade(file_name, &info);
        if (ret != FIRMWARE_SUCCESS) {
            totalerr += 1;
            dbg_print(is_debug_on, "Failed to upgrade: %s, slot: %d, chain index: %d, chain_value: %d, chain: %d, ret: %d\n",
                file_name, slot, i, info.chain_list[i], info.chain, ret);
        } else {
            dbg_print(is_debug_on, "Traverse upgrade success, file: %s, slot: %d, chain index: %d, chain_value: %d, chain: %d\n",
                file_name, slot, i, info.chain_list[i], info.chain);
        }
    }

    if (totalerr == 0) {
        dbg_print(is_debug_on, "Traverse upgrade all success, file: %s, total chain number: %d\n", file_name, i);
        return FIRMWARE_SUCCESS;
    }
    dbg_print(is_debug_on, "Traverse upgrade failed, file: %s, total error: %d\n", file_name, totalerr);
    return FIRMWARE_FAILED;
}

/*
 * firmware_upgrade_file_test
 * function: upgrade file
 * @file_name: Upgrade file name
 * @main_type: main board type
 * @sub_type:  sub board type
 * @slot: 0--main, sub slot starts at 1
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
static int firmware_upgrade_file_test(int argc, char *argv[])
{
    int ret;
    name_info_t info;
    int main_type, sub_type, slot;
    char *file_name;
    int i, chain_value, totalerr;

    /* argv[0] is 'test' */
    file_name = argv[1];
    main_type = strtoul(argv[2], NULL, 0);
    sub_type = strtoul(argv[3], NULL, 0);
    slot = strtoul(argv[4], NULL, 0);

    /* paramter check1 */
    if ((slot < 0) || (file_name == NULL)) {
        dbg_print(is_debug_on, "Failed firmware_upgrade_one_file parameter err.\n");
        return FIRMWARE_FAILED;
    }

    chain_value = FIRMWARE_INVALID_CHAIN;
    if (argc == 7) {
        ret = firmware_upgrade_one_file_get_info_argc7(argc, argv, &info, &chain_value);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "get_info_argc7 failed: %s.\n",
                    file_name);
            return ret;
        }
    } else if (argc == 5) {
        ret = firmware_upgrade_one_file_get_info_argc5(argc, argv, &info);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "get_info_argc5 failed: %s.\n",
                    file_name);
            return ret;
        }
    } else {
        dbg_print(is_debug_on,
                "Failed firmware_upgrade_one_file argument number: %d parameter err.\n",
                argc);
        return FIRMWARE_FAILED;
    }

    /* Check the file information to determine that the file is available for use on the device */
    ret = firmware_check_file_info(&info, main_type, sub_type, slot);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "File is not match with the device: %s, ret=%d.\n", file_name, ret);
        return ret;
    }

    /* The link number corresponding to the upgrade file is calculated based on the slot number.
       16 links are reserved for each slot. main boade slot is 0. */
    if (chain_value != FIRMWARE_INVALID_CHAIN) {
        info.chain = chain_value + (slot * FIRMWARE_SLOT_MAX_NUM);
        dbg_print(is_debug_on, "Specify chain upgrade test, file: %s, slot: %d, chain_value: %d, chain: %d\n",
            file_name, slot, chain_value, info.chain);
        ret = firmware_upgrade_test(file_name, &info);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "Failed to upgrade test: %s, slot: %d, chain_value: %d, chain: %d\n",
                file_name, slot, chain_value, info.chain);
            return ret;
        }
        dbg_print(is_debug_on, "Specify chain upgrade test success, file: %s, slot: %d, chain_value: %d, chain: %d\n",
            file_name, slot, chain_value, info.chain);
        return FIRMWARE_SUCCESS;
    }

    /* Traverse all chains for upgrade test */
    totalerr = 0;
    for (i = 0; i < FIRMWARE_SLOT_MAX_NUM; i++) {
        if (info.chain_list[i] == FIRMWARE_INVALID_CHAIN) {
            dbg_print(is_debug_on, "End of chain_list, index: %d\n", i);
            break;
        }
        info.chain = info.chain_list[i] + (slot * FIRMWARE_SLOT_MAX_NUM);
        dbg_print(is_debug_on, "Traverse upgrade test, file: %s, slot: %d, chain index: %d, chain_value: %d, chain: %d\n",
            file_name, slot, i, info.chain_list[i], info.chain);
        ret = firmware_upgrade_test(file_name, &info);
        if (ret != FIRMWARE_SUCCESS) {
            totalerr += 1;
            dbg_print(is_debug_on, "Failed to upgrade test: %s, slot: %d, chain index: %d, chain_value: %d, chain: %d, ret: %d\n",
                file_name, slot, i, info.chain_list[i], info.chain, ret);
        } else {
            dbg_print(is_debug_on, "Traverse upgrade test success, file: %s, slot: %d, chain index: %d, chain_value: %d, chain: %d\n",
                file_name, slot, i, info.chain_list[i], info.chain);
        }
    }

    if (totalerr == 0) {
        dbg_print(is_debug_on, "Traverse upgrade test all success, file: %s, total chain number: %d\n", file_name, i);
        return FIRMWARE_SUCCESS;
    }
    dbg_print(is_debug_on, "Traverse upgrade test failed, file: %s, total error: %d\n", file_name, totalerr);
    return FIRMWARE_FAILED;
}

static int firmware_upgrade_data_dump(char *argv[])
{
    int ret;
    uint32_t offset, len;

    /* dump by type */
    if (strcmp(argv[2], "spi_logic_dev") == 0) {
        /* usag: firmware_upgrade dump spi_logic_dev dev_path offset size print/record_file_path */
        offset = strtoul(argv[4], NULL, 0);
        len = strtoul(argv[5], NULL, 0);
        /* offset needs align by 256 bytes */
        if ((offset & 0xff) || (len == 0)) {
            dbg_print(is_debug_on,"only support offset align by 256 bytes.\n");
            return FIRMWARE_FAILED;
        }
        dbg_print(is_debug_on, "start to dump %s data. offset:0x%x, len:0x%x\n", argv[2], offset, len);
        ret = firmware_upgrade_spi_logic_dev_dump(argv[3], offset, len, argv[6]);
    } else {
        dbg_print(is_debug_on, "Error: %s not support dump data.\n", argv[2]);
        return FIRMWARE_FAILED;
    }

    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to dump %s data. ret:%d\n", argv[3], ret);
        return FIRMWARE_FAILED;
    }

    return FIRMWARE_SUCCESS;
}

static void print_usage(void)
{
    printf("Use:\n");
    printf(" upgrade file with header : firmware_upgrade file main_type sub_type slot\n");
    printf(" upgrade file with/without heade: firmware_upgrade file main_type sub_type slot file_type chain\n");
    printf(" upgrade test with header: firmware_upgrade test file main_type sub_type slot\n");
    printf(" upgrade test with/without heade: firmware_upgrade test file main_type sub_type slot file_type chain\n");
    printf(" spi_logic_dev dump : firmware_upgrade dump spi_logic_dev dev_path offset size print/record_file_path\n");
    printf(" spi_logic_dev : firmware_upgrade spi_logic_dev dev_path get_flash_id\n");
    return;
}

int main(int argc, char *argv[])
{
    int ret;
    uint32_t flash_id;

    is_debug_on = firmware_upgrade_debug();

    signal(SIGTERM, SIG_IGN);   /* ignore kill signal */
    signal(SIGINT, SIG_IGN);    /* ignore ctrl+c signal */
    signal(SIGTSTP, SIG_IGN);   /* ignore ctrl+z signal */

    if ((argc != 4) && (argc != 5) && (argc != 6) && (argc != 7) && (argc != 8)) {
        print_usage();
        dbg_print(is_debug_on, "Failed to upgrade the number of argv: %d.\n", argc);
        return ERR_FW_UPGRADE;
    }
    if (argc == 4) {
        if ((strcmp(argv[1], "spi_logic_dev") == 0) && (strcmp(argv[3], "get_flash_id") == 0)) {
            ret = firmware_upgrade_spi_logic_dev_get_flash_id(argv[2], &flash_id);
            if (ret < 0) {
                printf("Failed to get spi_logic_dev %s flash id, ret: %d\n", argv[2], ret);
                return ret;
            }
            printf("flash id: 0x%x\n",flash_id);
            return 0;
        }
        print_usage();
        return ERR_FW_UPGRADE;
    }

    if ((argc == 7) && (strcmp(argv[1], "dump") == 0)) {
        /* print device data */
        ret = firmware_upgrade_data_dump(argv);
        if (ret == FIRMWARE_SUCCESS) {
            printf("dump data succeeded.\n");
            return FIRMWARE_SUCCESS;
        }
        printf("dump data failed. ret:%d\n", ret);
        return ret;
    }

    if ((argc == 5) || (argc == 7)) {
        printf("+================================+\n");
        printf("|Begin to upgrade, please wait...|\n");
        ret = firmware_upgrade_one_file(argc, argv);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "Failed to upgrade a firmware file: %s. (%d)\n", argv[1], ret);
            printf("|           Upgrade failed!      |\n");
            printf("+================================+\n");
            return ret;
        }

        printf("|          Upgrade succeeded!    |\n");
        printf("+================================+\n");
        dbg_print(is_debug_on, "Sucess to upgrade a firmware file: %s.\n", argv[1]);
        return FIRMWARE_SUCCESS;
    } else if (((argc == 6) || (argc == 8)) && (strcmp(argv[1], "test") == 0)) {
        printf("+=====================================+\n");
        printf("|Begin to upgrade test, please wait...|\n");
        /* Skip one parameter to make the argc and argv of the test command consistent with the upgrade command */
        ret = firmware_upgrade_file_test(argc - 1, argv + 1);
        if (ret == FIRMWARE_SUCCESS) {
            printf("|       Upgrade test succeeded!       |\n");
            printf("+=====================================+\n");
            dbg_print(is_debug_on, "Sucess to upgrade test a firmware file: %s.\n", argv[2]);
            return FIRMWARE_SUCCESS;
        } else if (ret == ERR_FW_DO_UPGRADE_NOT_SUPPORT) {
            dbg_print(is_debug_on, "do not support to upgrade test a firmware file: %s. (%d)\n", argv[2], ret);
            printf("|     Not support to upgrade test!    |\n");
            printf("+=====================================+\n");
            return ret;
        } else {
            dbg_print(is_debug_on, "Failed to upgrade test a firmware file: %s. (%d)\n", argv[2], ret);
            printf("|         Upgrade test failed!        |\n");
            printf("+=====================================+\n");
            return ret;
        }
    }

    printf("+=================+\n");
    printf("|  UPGRADE FAIL!  |\n");
    printf("+=================+\n");

    return ERR_FW_UPGRADE;
 }
