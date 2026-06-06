#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/mman.h>
#include "cpld_i2c.h"
#include "cpld_i2c_port.h"
#include "xo2_dev.h"
#include "xo2_api.h"
#include "debug.h"
#include <firmware_ispvme_i2c.h>

#define FIRMWARE_UPGRADE_TMP_FILE   "/tmp/ispvme_i2c_pid_%d.jed"
#define FILE_PATH_MAX_LEN           (256)
#define I2C_SLAVE_ADDR_MAX          (0xFF)

extern int ispvme_main_i2c(OC_I2C_t *cpld_i2c);


static int fw_upg_get_ispvme_i2c_info(int fd, fw_ispvme_i2c_info_t *p_i2c_info)
{
    int ret;

    ret = ioctl(fd, FIRMWARE_ISPVME_I2C_INFO, p_i2c_info);
    if (ret < 0) {
        CPLD_I2C_ERROR("get ispvme i2c info fail, ret:%d, fd:%d\n", ret, fd);
        return ret;
    }

    return 0;
}

static int fw_upg_ispvme_i2c_main(char *file_path, unsigned int bus_num,
                                        unsigned int slave_addr, unsigned int cpld_type)
{
    int ret;
    OC_I2C_t oc_i2c;
    char filename[I2C_MAX_NAME_SIZE];

    /* 1. judge input para */
    if (file_path == NULL) {
        CPLD_I2C_ERROR("file_path is null\n");
        return -1;
    }

    if (access(file_path, F_OK) < 0) {
        CPLD_I2C_ERROR("file:%s is not exist\n", file_path);
        return -1;
    }

    memset(filename, 0, sizeof(filename));
    (void)snprintf(filename, I2C_MAX_NAME_SIZE, "/dev/i2c-%u", bus_num);
    if (access(filename, F_OK) < 0) {
        CPLD_I2C_ERROR("file:%s is not exist\n", filename);
        return -1;
    }

    if (slave_addr > I2C_SLAVE_ADDR_MAX) {
        CPLD_I2C_ERROR("slave_addr:0x%x is invalid\n", slave_addr);
        return -1;
    }

    if (cpld_type < MachXO2_256 || cpld_type >= Mach_END) {
        CPLD_I2C_ERROR("cpld_type:%u is invalid\n", cpld_type);
        return -1;
    }

    /* 2. init oc_i2c */
    memset(&oc_i2c, 0, sizeof(oc_i2c));
    oc_i2c.masterBus = bus_num;
    oc_i2c.slaveAddr = slave_addr;
    oc_i2c.cpld_type = cpld_type;
    oc_i2c.file_path = file_path;
    oc_i2c.ProgramMode = XO2ECA_PROGRAM_TRANSPARENT;
    oc_i2c.VerifyMode  = XO2ECA_PROGRAM_VERIFY;

    /* 3. do cpld upgrade by call vendor main func interface: ispvme_main_i2c */
    ret = ispvme_main_i2c(&oc_i2c);
    if (ret != CPLD_I2C_OK) {
        CPLD_I2C_ERROR("CPLD I2C Program Failed %d\n", ret);
        return ret;
    }

    return 0;
}

static int separate_upg_file_from_file_with_header(const char *input_file_path,
                                          const char *output_file_path, size_t headerSize)
{
    char buffer[1024];
    size_t bytes_read;
    FILE *in_fp, *out_fp;

    in_fp = fopen(input_file_path, "rb");
    if (!in_fp) {
        CPLD_I2C_ERROR("can not open file:%s\n", input_file_path);
        return -1;
    }

    out_fp= fopen(output_file_path, "wb");
    if (!out_fp) {
        CPLD_I2C_ERROR("can not open file:%s\n", input_file_path);
        fclose(in_fp);
        return -1;
    }

    fseek(in_fp, headerSize, SEEK_SET);

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), in_fp)) > 0) {
        fwrite(buffer, 1, bytes_read, out_fp);
    }

    fclose(in_fp);
    fclose(out_fp);

    return 0;
}

int firmware_upgrade_ispvme_i2c(int fd, char *file_name, size_t header_size)
{
    int ret;
    fw_ispvme_i2c_info_t i2c_info;
    char tmp_file[FILE_PATH_MAX_LEN];

    /* 0. debug init */
    cpld_i2c_debug_init();

    CPLD_I2C_VERBOS("begin\n");

    /* 1. judge input para */
    if (fd < 0) {
        CPLD_I2C_ERROR("fd:%d is invalid\n", fd);
        return -1;
    }

    if (file_name == NULL) {
        CPLD_I2C_ERROR("file_name is null\n");
        return -1;
    }

    if (access((const char *)file_name, F_OK) < 0) {
        CPLD_I2C_ERROR("file:%s is not exist\n", file_name);
        return -1;
    }
    CPLD_I2C_VERBOS("fd:%d, file_name:%s, header_size:%zu\n", fd, file_name, header_size);

    /* 2. get ispvme i2c info */
    memset(&i2c_info, 0, sizeof(i2c_info));
    ret = fw_upg_get_ispvme_i2c_info(fd, &i2c_info);
    if (ret != 0) {
        CPLD_I2C_ERROR("get i2c info fail, ret:%d\n", ret);
        return -1;
    }
    CPLD_I2C_VERBOS("bus_num:%u, slave_addr:%u, cpld_type:%u\n",
                     i2c_info.bus_num, i2c_info.slave_addr, i2c_info.cpld_type);

    /* 3. separate the upgrade file from the file with the header info */
    if (header_size > 0) {
        memset(tmp_file, 0, sizeof(tmp_file));
        (void)snprintf(tmp_file, sizeof(tmp_file), FIRMWARE_UPGRADE_TMP_FILE, getpid());
        ret = separate_upg_file_from_file_with_header((const char *)file_name, (const char *)tmp_file, header_size);
        if (ret != 0) {
            CPLD_I2C_ERROR("separate upgrade file fail, ret:%d, file_name:%s, header_size:%zx\n",
                    ret, file_name, header_size);
            return -1;
        }
    }

    /* 4. firmware upgrade cpld that is attched to i2c bus */
    ret = fw_upg_ispvme_i2c_main(tmp_file, i2c_info.bus_num,
                                 i2c_info.slave_addr, i2c_info.cpld_type);
    if (ret != 0) {
        CPLD_I2C_ERROR("do fw_upg_ispvme_i2c_main fail, ret:%d, bus_num:%u, slave_addr:%u, cpld_type:%u\n",
                ret, i2c_info.bus_num, i2c_info.slave_addr, i2c_info.cpld_type);
        (void)remove((const char *)tmp_file);
        return -1;
    }

    /* 5. del tmp file */
    (void)remove((const char *)tmp_file);

    CPLD_I2C_VERBOS("end\n");
    return 0;
}
