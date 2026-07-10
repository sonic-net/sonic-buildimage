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
        snprintf(file_name, len, "/dev/firmware_sysfs%d", info->chain);
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
    mem_clear(version, FIRMWARE_NAME_LEN);
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

    /* file crc32 check */
    crc = crc32(0, (const unsigned char *)upg_buf, (unsigned int)upg_size);
    if (crc != info->crc32) {
        dbg_print(is_debug_on, "Error: Failed to check file crc: %s.\n", file_name);
        dbg_print(is_debug_on, "the crc value is : %#08x.\n", (unsigned int)crc);
        free(upg_buf);
        close(fd);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
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
    int i;

    type_num = (sizeof(firmware_file_str) /sizeof(firmware_file_str[0]));
    for (i = 0; i < type_num; i++) {
        if (!strncmp(firmware_file_str[i].firmware_file_name_str, type_str,
                strlen(firmware_file_str[i].firmware_file_name_str))) {
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
        info->chain = strtoul(value, NULL, 10);
    } else if (strcmp(key, FILEHEADER_CHIPNAME) == 0) {
        /* chip name */
        if (strlen(value) >= FIRMWARE_NAME_LEN) {
            dbg_print(is_debug_on, "Error: '%s' is too long for a chipname.\n", value);
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
        mem_clear(info->chip_name, sizeof(info->chip_name));
         snprintf(info->chip_name, sizeof(info->chip_name) - 1, "%s", value);
    } else if (strcmp(key, FILEHEADER_VERSION) == 0) {
        /* version */
        if (strlen(value) >= FIRMWARE_NAME_LEN) {
            dbg_print(is_debug_on, "Error: '%s' is too long for a version.\n", value);
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
        mem_clear(info->version, sizeof(info->version));
        snprintf(info->version, sizeof(info->version) - 1, "%s", value);
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
    dbg_print(is_debug_on, "\n"
                           "    type     : %d, \n"
                           "    chain     : %d, \n"
                           "    chip name: %s \n"
                           "    version  : %s \n"
                           "    file type: %d \n"
                           "    the crc32 value: %#x \n",
    info->type, info->chain, info->chip_name, info->version, info->file_type, info->crc32);
    return FIRMWARE_SUCCESS;
}

/*
 * firmware_upgrade_read_header
 * function:Read the header information of the upgrade file
 * @file_name: param[in] Upgrade file name
 * @info : param[out]  Upgrade file information
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
static int firmware_upgrade_read_header( char *file_name, name_info_t *info)
{
    FILE *fp;
    char *charp;
    char *charn;
    char header_buffer[MAX_HEADER_SIZE];
    char header_key[MAX_HEADER_KV_SIZE];
    char header_var[MAX_HEADER_KV_SIZE];
    int ret;
    int len;

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

    charp = strstr(header_buffer, "FILEHEADER(\n");
    if (charp == NULL) {
        dbg_print(is_debug_on, "Error: The file format %s is wrong. \n",  file_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
    charp += strlen("FILEHEADER(\n");

    dbg_print(is_debug_on, "File parse start.\n");
    mem_clear(info, sizeof(name_info_t));
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
 * firmware_upgrade_one_file
 * function: upgrade file
 * @file_name: Upgrade file name
 * @main_type: main board type
 * @sub_type:  sub board type
 * @slot: 0--main, sub slot starts at 1
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
static int firmware_upgrade_one_file(char *file_name, int main_type, int sub_type, int slot)
{
    int ret;
    name_info_t info;

    if ((slot < 0) || (file_name == NULL)) {
        dbg_print(is_debug_on, "Failed firmware_upgrade_one_file parameter err.\n");
        return FIRMWARE_FAILED;
    }

    dbg_print(is_debug_on, "firmware upgrade %s 0x%x 0x%x %d\n", file_name, main_type, sub_type, slot);
    /* Read the header information of the upgrade file */
    ret = firmware_upgrade_read_header(file_name, &info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to get file header: %s\n", file_name);
        return ret;
    }

    /* Check the file information to determine that the file is available for use on the device */
    ret = firmware_check_file_info(&info, main_type, sub_type, slot);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "File is not match with the device: %s.\n", file_name);
        return ret;
    }

    /* The link number corresponding to the upgrade file is calculated based on the slot number.
       16 links are reserved for each slot. main boade slot is 0. */
    info.chain += slot * FIRMWARE_SLOT_MAX_NUM;
    ret = firmware_upgrade(file_name, &info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to upgrade: %s.\n", file_name);
        return ret;
    }

    return FIRMWARE_SUCCESS;
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
static int firmware_upgrade_file_test(char *file_name, int main_type, int sub_type, int slot)
{
    int ret;
    name_info_t info;

    if ((slot < 0) || (file_name == NULL)) {
        dbg_print(is_debug_on, "Failed firmware_upgrade_one_file parameter err.\n");
        return FIRMWARE_FAILED;
    }

    dbg_print(is_debug_on, "firmware upgrade %s 0x%x 0x%x %d\n", file_name, main_type, sub_type, slot);
    /* Read the header information of the upgrade file */
    ret = firmware_upgrade_read_header(file_name, &info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to get file header: %s, ret=%d\n", file_name, ret);
        return ret;
    }

    /* Check the file information to determine that the file is available for use on the device */
    ret = firmware_check_file_info(&info, main_type, sub_type, slot);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "File is not match with the device: %s, ret=%d.\n", file_name, ret);
        return ret;
    }

    /* The link number corresponding to the upgrade file is calculated based on the slot number.
       16 links are reserved for each slot. main boade slot is 0. */
    info.chain += slot * FIRMWARE_SLOT_MAX_NUM;
    ret = firmware_upgrade_test(file_name, &info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to upgrade: %s, ret=%d\n", file_name, ret);
        return ret;
    }

    return FIRMWARE_SUCCESS;
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

int main(int argc, char *argv[])
{
    int ret;
    int main_type, sub_type, slot;

    is_debug_on = firmware_upgrade_debug();

    signal(SIGTERM, SIG_IGN);   /* ignore kill signal */
    signal(SIGINT, SIG_IGN);    /* ignore ctrl+c signal */
    signal(SIGTSTP, SIG_IGN);   /* ignore ctrl+z signal */

    if ((argc != 5) && (argc != 6) && (argc != 7)) {
        printf("Use:\n");
        printf(" upgrade file : firmware_upgrade file main_type sub_type slot\n");
        printf(" upgrade test : firmware_upgrade test file main_type sub_type slot\n");
        printf(" spi_logic_dev dump : firmware_upgrade dump spi_logic_dev dev_path offset size print/record_file_path\n");
        dbg_print(is_debug_on, "Failed to upgrade the number of argv: %d.\n", argc);
        return ERR_FW_UPGRADE;
    }

    if (argc == 5) {
        main_type = strtoul(argv[2], NULL, 16);
        sub_type = strtoul(argv[3], NULL, 16);
        slot = strtoul(argv[4], NULL, 10);
        printf("+================================+\n");
        printf("|Begin to upgrade, please wait...|\n");
        ret = firmware_upgrade_one_file(argv[1], main_type, sub_type, slot);
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
    } else if ((argc == 6) && (strcmp(argv[1], "test") == 0)) {
        main_type = strtoul(argv[3], NULL, 16);
        sub_type = strtoul(argv[4], NULL, 16);
        slot = strtoul(argv[5], NULL, 10);
        printf("+=====================================+\n");
        printf("|Begin to upgrade test, please wait...|\n");
        ret = firmware_upgrade_file_test(argv[2], main_type, sub_type, slot);
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
    } else if (strcmp(argv[1], "dump") == 0) {
        /* print device data */
        ret = firmware_upgrade_data_dump(argv);
        if (ret == FIRMWARE_SUCCESS) {
            printf("dump data succeeded.\n");
            return FIRMWARE_SUCCESS;
        } else {
            printf("dump data failed. ret:%d\n", ret);
            return ret;
        }
    }

    printf("+=================+\n");
    printf("|  UPGRADE FAIL!  |\n");
    printf("+=================+\n");

    return ERR_FW_UPGRADE;
 }
