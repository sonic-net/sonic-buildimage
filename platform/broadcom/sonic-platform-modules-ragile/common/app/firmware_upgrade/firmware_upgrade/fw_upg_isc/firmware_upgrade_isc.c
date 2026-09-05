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

/*
 * firmware_upgrade_jtag
 * function: Determine whether to upgrade ISC or JBI
 * @fd:   param[in] Device file descriptor
 * @buf:  param[in] Upgrade the file content
 * @size: param[in] Upgrade file size
 * @info: param[in] Upgrade file information
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int firmware_upgrade_jtag(int fd, uint8_t *buf, uint32_t size, name_info_t *info)
{
    int ret;
    cmd_info_t cmd_info;

    cmd_info.size = size;
    cmd_info.data = buf;
    ret = 0;

    if (info->type == FIRMWARE_CPLD) {
        /* 0x4A,0x41,0x4D,0x01 is JBI file */
        if (buf[0] == 0x4A && buf[1] == 0x41 && buf[2] == 0x4D && buf[3] == 0x01) {
            dbg_print(is_debug_on, "Use jbi file.\n");
            ret = ioctl(fd, FIRMWARE_PROGRAM_JBI, &cmd_info);
        } else {
            dbg_print(is_debug_on, "Use isc file.\n");
            ret = ioctl(fd, FIRMWARE_PROGRAM, &cmd_info);
        }
    }

    if (info->type == FIRMWARE_FPGA) {
        ret = ioctl(fd, FIRMWARE_PROGRAM, &cmd_info);
    }

    if (ret < 0) {
        return FIRMWARE_FAILED;
    }

    return FIRMWARE_SUCCESS;
}

/*
 * firmware_upgrade_jtag_test
 * function: Determine whether to upgrade ISC or JBI
 * @fd:   param[in] Device file descriptor
 * @buf:  param[in] Upgrade the file content
 * @size: param[in] Upgrade file size
 * @info: param[in] Upgrade file information
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int firmware_upgrade_jtag_test(int fd, uint8_t *buf, uint32_t size, name_info_t *info)
{
    return FIRMWARE_SUCCESS;
}
