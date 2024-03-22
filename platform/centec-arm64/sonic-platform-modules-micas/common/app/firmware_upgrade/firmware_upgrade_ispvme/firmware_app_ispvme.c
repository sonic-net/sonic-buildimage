/*
 * firmware_app.c

 * firmware upgrade
 * v1.0    support <support> 2013-10-25  Initial version.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>

#if 0
#include <ssa/lib/ssa_dfd_intf.h>
#include <autoconf.h>
#endif
#include <firmware_app_ispvme.h>
static firmware_card_info_t g_card_info[] = {
    {
        .dev_type      = M2_W6010_48GT4X_FA,
        .slot_num      = 1,
        .card_name     = "M2_W6010_48GT4X_FA",
        .gpio_info     = {
            /* slot 0 */
            {
                .tdi       = 507,
                .tck       = 505,
                .tms       = 506,
                .tdo       = 508,
                .jtag_en   = 504,
                .select           = -1,
                .jtag_5    = GPIO(-1, 0,1)
                .jtag_4    = GPIO(-1, 0,1)
                .jtag_3    = GPIO(-1, 1,1)
                .jtag_2    = GPIO(-1, 0,1)
                .jtag_1    = GPIO(-1, 1,1)
            },
        },

    },

    {
        .dev_type      = M2_W6010_48GT4X_RA,
        .slot_num      = 1,
        .card_name     = "M2_W6010_48GT4X_RA",
        .gpio_info     = {
            /* slot 0 */
            {
                .tdi       = 507,
                .tck       = 505,
                .tms       = 506,
                .tdo       = 508,
                .jtag_en   = 504,
                .select           = -1,
                .jtag_5    = GPIO(-1, 0,1)
                .jtag_4    = GPIO(-1, 0,1)
                .jtag_3    = GPIO(-1, 1,1)
                .jtag_2    = GPIO(-1, 0,1)
                .jtag_1    = GPIO(-1, 1,1)
            },
        },
    },
};

static int is_debug_on;
static int dfd_my_type = 0;

#if 0
#if defined(CONFIG_FRM_PRODUCT_NAME)
#define CONFIG_PRODUCT_NAME CONFIG_FRM_PRODUCT_NAME
#else
#define CONFIG_PRODUCT_NAME "card"
#endif
#endif

#define DFD_TYPE_FILE	"/sys/module/platform_common/parameters/dfd_my_type"
#define DFD_TYPE_BUFF_SIZE	(256)

static int is_vme_file(char *file_name)
{
    char *tmp;

    tmp = strchr(file_name, '.');
    if (strcmp(tmp, ".bin") == 0) {
        return 0;
    } else if (strcmp(tmp, ".vme") == 0) {
        return 1;
    } else {
        return -1;
    }
}

int drv_get_my_dev_type(void)
{
    int type;
	int fd;
	char rbuf[DFD_TYPE_BUFF_SIZE];
	int read_len;

    if (dfd_my_type != 0) {
        dbg_print(is_debug_on, "my_type = 0x%x\r\n", dfd_my_type);
        return dfd_my_type;
    }

	fd = open(DFD_TYPE_FILE, O_RDONLY);
    if (fd < 0) {
        dbg_print(is_debug_on, "can't open device %s.\r\n", DFD_TYPE_FILE);
        return M2_W6010_48GT4X_FA;	/* Avoid 6010 to obtain different device types */
    }

	mem_clear(rbuf, DFD_TYPE_BUFF_SIZE);
	type = 0;
    read_len = read(fd, rbuf, DFD_TYPE_BUFF_SIZE - 1);
	if (read_len > 0) {
		type = strtoul(rbuf, NULL, 0);
	}
	close(fd);

    dfd_my_type = type;

	dbg_print(is_debug_on, "read dfd type file is %s read_len %d, dfd_my_type 0x%x\n", rbuf, read_len, dfd_my_type);
    return dfd_my_type;
}

firmware_card_info_t* firmware_get_card_info(int dev_type)
{
    int i;
    int size;

    size = (sizeof(g_card_info) /sizeof((g_card_info)[0]));

    dbg_print(is_debug_on, "Enter dev_type 0x%x size %d.\n", dev_type, size);
    for (i = 0; i < size; i++) {
        if (g_card_info[i].dev_type == dev_type) {
            dbg_print(is_debug_on, "match dev_type 0x%x.\n", dev_type);
            return &g_card_info[i];
        }
    }

    dbg_print(is_debug_on, "dismatch dev_type 0x%x.\n", dev_type);
    return NULL;
}

int firmware_get_card_name(char *name, int len)
{
    int dev_type;
    firmware_card_info_t *info;

    dbg_print(is_debug_on, "Enter len %d.\n", len);
    dev_type = drv_get_my_dev_type();
    if (dev_type < 0) {
        dbg_print(is_debug_on, "drv_get_my_dev_type failed ret %d.\n", dev_type);
        return FIRMWARE_FAILED;
    }

    info = firmware_get_card_info(dev_type);
    if (info == NULL) {
        dbg_print(is_debug_on, "firmware_get_card_info dev_type %d failed.\n", dev_type );
        return FIRMWARE_FAILED;
    }

    strncpy(name, info->card_name, len - 1);
    dbg_print(is_debug_on, "Leave dev_type 0x%x name %s, info->name %s.\n", dev_type,
        name, info->card_name);
    return FIRMWARE_SUCCESS;
}

int get_debug_value(void)
{
    return is_debug_on;
}

#if 0
/* each device implements its own corresponding interface */
int __attribute__ ((weak)) firmware_get_card_name(char *name, int len)
{
    strncpy(name, CONFIG_PRODUCT_NAME, len - 1);
    return FIRMWARE_SUCCESS;
}
#endif

static int firmware_check_file_is_dir(char *dir, char *file_name)
{
    int ret;
    struct stat buf;
    char tmp[FIRMWARE_FILE_DIR_LEN];

    if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
        return -1;
    }

    mem_clear(tmp, FIRMWARE_FILE_DIR_LEN);
    snprintf(tmp, FIRMWARE_FILE_DIR_LEN - 1, "%s/%s", dir, file_name);
    ret = stat(tmp, &buf);
    if (ret < 0) {
        return -1;
    }

    if (S_ISDIR(buf.st_mode)) {
        return 1;
    }

    return 0;
}

static inline int firmware_error_type(int action, name_info_t *info)
{
    if (info == NULL) {
        return ERR_FW_UPGRADE;
    }
    if (info->type == FIRMWARE_CPLD) {
        switch (action) {
        case FIRMWARE_ACTION_CHECK:
            return ERR_FW_CHECK_CPLD_UPGRADE;
        case FIRMWARE_ACTION_UPGRADE:
            return ERR_FW_DO_CPLD_UPGRADE;
        default:
            return ERR_FW_UPGRADE;
        }
    } else if (info->type == FIRMWARE_FPGA) {
        switch (action) {
        case FIRMWARE_ACTION_CHECK:
            return ERR_FW_CHECK_FPGA_UPGRADE;
        case FIRMWARE_ACTION_UPGRADE:
            return ERR_FW_DO_FPGA_UPGRADE;
        default:
            return ERR_FW_UPGRADE;
        }
    } else {
        return ERR_FW_UPGRADE;
    }
}

/* analyze file's name,Name rule: card name_firmware type_slot number_firmware chip name.bin*/
static int firmware_parse_file_name(char *name, name_info_t *info)
{
    int i;
    char *tmp, *start;
    char slot[FIRMWARE_NAME_LEN];

    dbg_print(is_debug_on, "Parse file name: %s\n", name);

    start = name;
    /* card name */
    tmp = strchr(start, '_');
    if (tmp == NULL) {
        dbg_print(is_debug_on, "Failed to get card name form file name: %s.\n", name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, NULL);
    }

    strncpy(info->card_name, start,
            (tmp - start > FIRMWARE_NAME_LEN - 1) ? (FIRMWARE_NAME_LEN - 1) : (tmp - start));

    /* firmware type */
    start = tmp + 1;
    tmp = strchr(start, '_');
    if (tmp == NULL) {
        dbg_print(is_debug_on, "Failed to get upgrade type form file name: %s.\n", name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, NULL);
    }

    if (strncmp(start, FIRMWARE_CPLD_NAME, tmp - start) == 0) {
        info->type = FIRMWARE_CPLD;
    } else if (strncmp(start, FIRMWARE_FPGA_NAME, tmp - start) == 0) {
        info->type = FIRMWARE_FPGA;
    } else {
        info->type = FIRMWARE_OTHER;
    }

    /* slot number */
    start = tmp + 1;
    tmp = strchr(start, '_');
    if (tmp == NULL) {
        dbg_print(is_debug_on, "Failed to get slot form file name: %s.\n", name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    mem_clear(slot, FIRMWARE_NAME_LEN);
    strncpy(slot, start,
            ((tmp - start > FIRMWARE_NAME_LEN - 1) ? FIRMWARE_NAME_LEN - 1 : tmp - start));

    for (i = 0; i < FIRMWARE_NAME_LEN && slot[i] != '\0'; i++) {
        if (!isdigit(slot[i])) {
            return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
        }
    }

    dbg_print(is_debug_on, "get slot info: %s.\n", name);
    info->slot = strtoul(slot, NULL, 10);
    dbg_print(is_debug_on, "get slot info slot: %d.\n", info->slot);

    /* firmware chip name */
    start = tmp + 1;
    tmp = strchr(start, '_');
    if (tmp == NULL) {
        dbg_print(is_debug_on, "Failed to get chip name form file name: %s.\n", name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
    strncpy(info->chip_name, start,
            (tmp - start > FIRMWARE_NAME_LEN - 1) ? (FIRMWARE_NAME_LEN - 1) : (tmp - start));

    /* version */
    start = tmp + 1;
    tmp = strstr(start, ".vme");
    if (tmp == NULL) {
        dbg_print(is_debug_on, "Failed to get chip version form file name: %s.\n", name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }
    strncpy(info->version, start,
            (tmp - start > FIRMWARE_NAME_LEN - 1) ? (FIRMWARE_NAME_LEN - 1) : (tmp - start));

    /* finish checking */
    if (strcmp(tmp, ".vme") != 0) {
        dbg_print(is_debug_on, "The file format is wrong: %s.\n", name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    return FIRMWARE_SUCCESS;
}

/* check the file information to determine whether the file can be used in the device */
static int firmware_check_file_info(name_info_t *info)
{
    int ret;
    char card_name[FIRMWARE_NAME_LEN];

    dbg_print(is_debug_on, "Check file info.\n");

    /* get card name */
    mem_clear(card_name, FIRMWARE_NAME_LEN);
    ret = firmware_get_card_name(card_name, FIRMWARE_NAME_LEN);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to get card name.(%s)\n", info->card_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    /* check card name */
    dbg_print(is_debug_on, "The card name: %s, the file card name: %s.\n",
              card_name, info->card_name);
    if (strcmp(card_name, info->card_name) != 0) {
        dbg_print(is_debug_on, "The file card name %s is wrong.(real: %s)\n",
                  info->card_name, card_name);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    /* check type */
    if (info->type != FIRMWARE_CPLD && info->type != FIRMWARE_FPGA) {
        dbg_print(is_debug_on, "The file type %d is wrong.(cpld %d, fpga %d)\n",
                  info->type, FIRMWARE_CPLD, FIRMWARE_FPGA);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    /* check slot */
    if (info->slot < 1  || info->slot > FIRMWARE_MAX_SLOT_NUM) {
        dbg_print(is_debug_on, "The file slot %d is wrong.\n", info->slot);
        return firmware_error_type(FIRMWARE_ACTION_CHECK, info);
    }

    dbg_print(is_debug_on, "Success check file info.\n");
    return FIRMWARE_SUCCESS;
}

static void firmware_get_dev_file_name(name_info_t *info, char *file_name, int len)
{
    if (info->type == FIRMWARE_CPLD) {
        snprintf(file_name, len, "/dev/firmware_cpld_ispvme%d", info->slot - 1);
    } else if (info->type == FIRMWARE_FPGA) {
        snprintf(file_name, len, "/dev/firmware_fpga_ispvme%d", info->slot - 1);
    } else {
        snprintf(file_name, len, "/dev/firmware_ispvme%d", info->slot - 1);
    }
}

static void firmware_set_driver_debug(int fd)
{
    int ret;

    if (is_debug_on == DEBUG_ALL_ON || is_debug_on == DEBUG_KERN_ON) {
        ret = ioctl(fd, FIRMWARE_SET_DEBUG_ON, NULL);
        if (ret < 0) {
            dbg_print(is_debug_on, "Failed to set driver debug on.\n");
        } else {
            dbg_print(is_debug_on, "Open driver debug.\n");
        }
    } else if (is_debug_on == DEBUG_OFF) {
        ret = ioctl(fd, FIRMWARE_SET_DEBUG_OFF, NULL);
        if (ret < 0) {
            dbg_print(is_debug_on, "Failed to set driver debug off.\n");
        } else {
            dbg_print(is_debug_on, "OFF driver debug.\n");
        }
    } else {
        dbg_print(is_debug_on, "Ignore driver debug.\n");
    }
}

static int firmware_check_chip_name(int fd, name_info_t *info)
{
    return FIRMWARE_SUCCESS;
}

static int firmware_check_chip_verison(int fd, name_info_t *info)
{
    return FIRMWARE_SUCCESS;
}

static int firmware_get_file_size(char *file_name, int *size)
{
    int ret;
    struct stat buf;

    ret = stat(file_name, &buf);
    if (ret < 0) {
        return FIRMWARE_FAILED;
    }

    if (buf.st_size < 0) {
        return FIRMWARE_FAILED;
    }

    *size = buf.st_size;

    return FIRMWARE_SUCCESS;
}

static int firmware_get_file_info(char *file_name, char *buf, int size)
{
    FILE *fp;
    int len;

    fp = fopen(file_name, "r");
    if (fp == NULL) {
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

static int firmware_upgrade_info(int fd, char *buf, int size)
{
    return FIRMWARE_SUCCESS;
}

/* upgrade CPLD */
static int firmware_upgrade(char *dir, char *file_name, name_info_t *info)
{
    return FIRMWARE_SUCCESS;
}

static int firmware_upgrade_file(char *dir, char *file_name)
{
    int ret, argc;
    name_info_t info;
    char *argv[2];
    char tmp_file[FIRMWARE_FILE_DIR_LEN];

    mem_clear(&info, sizeof(name_info_t));
    ret = firmware_parse_file_name(file_name, &info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to parse file name: %s\n", file_name);
        return ret;
    }

    dbg_print(is_debug_on, "The file name parse:(%s) \n"
                         "    card name: %s, \n"
                         "    type     : %d, \n"
                         "    slot     : %d, \n"
                         "    chip name: %s \n"
                         "    version  : %s \n",
              file_name, info.card_name, info.type, info.slot, info.chip_name, info.version);

    ret = firmware_check_file_info(&info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to check file name: %s\n", file_name);
        return ret;
    }

    /* get the information of upgrade file */
    mem_clear(tmp_file, FIRMWARE_FILE_DIR_LEN);
    if (dir != NULL) {
        snprintf(tmp_file, FIRMWARE_FILE_DIR_LEN - 1, "%s/%s", dir, file_name);
    } else {
        snprintf(tmp_file, FIRMWARE_FILE_DIR_LEN - 1, "%s", file_name);
    }

    argc = 2;
    argv[1] = tmp_file;
    ret = ispvme_main(argc, (char **)&argv);
    if (ret != FIRMWARE_SUCCESS){
        dbg_print(is_debug_on, "Failed to upgrade file name: %s\n", file_name);
    }

    return ret;
}

static int firmware_upgrade_set_gpio_info(int slot)
{
	firmware_card_info_t *hw_info;
	cmd_info_t cmd_info;
	firmware_upg_gpio_info_t *gpio_info;
	int ret;
	int fd;
	int dev_type;
	int my_slot;

	ret = 0;

	dev_type = drv_get_my_dev_type();
	fd = open("/dev/firmware_cpld_ispvme0", O_RDWR);
    if (fd < 0) {
        dbg_print(is_debug_on, "%s can't open device\r\n", __func__);
        return -1;
    }

	hw_info = firmware_get_card_info(dev_type);

	if (hw_info == NULL) {
		dbg_print(is_debug_on, "card type 0x%x don't support firmware.\n", dev_type);
		ret = -1;
		goto gpio_info_err;
	}

	if (slot >= hw_info->slot_num) {
		dbg_print(is_debug_on, "slot %d is too large, support slot num %d.\n", slot, hw_info->slot_num);
		ret = -1;
		goto gpio_info_err;
	}

	gpio_info = &(hw_info->gpio_info[slot]);
    cmd_info.size = sizeof(firmware_upg_gpio_info_t);
    cmd_info.data = (void *)gpio_info;

	dbg_print(is_debug_on, "slot = %d, gpio_info[jtag_en:%d select:%d tck:%d tdi:%d tdo=%d tms=%d]\n",slot, \
		gpio_info->jtag_en, gpio_info->select,gpio_info->tck,gpio_info->tdi,gpio_info->tdo,gpio_info->tms);

	ret = ioctl(fd, FIRMWARE_SET_GPIO_INFO, &cmd_info);

gpio_info_err:
	if (ret < 0) {
		dbg_print(is_debug_on, "Failed due to:set gpio info.\n");
	}

	close(fd);

	return ret;
}

/**
 * argv[1]: file name
 * argv[2]: type
 * argv[3]: slot number
 * argv[4]: chip name
 */
static int firmware_upgrade_one_file(int argc, char *argv[])
{
    int ret;
    name_info_t info;
    char tmp[FIRMWARE_FILE_DIR_LEN];

	mem_clear(&info, sizeof(name_info_t));

    info.slot = strtoul(argv[3], NULL, 10);
    strncpy(info.chip_name, argv[4], FIRMWARE_NAME_LEN - 1);

    if (strcmp(argv[2], FIRMWARE_CPLD_NAME) == 0) {     /* CPLD upgrade */
        if(is_vme_file(argv[1]) == 1){  /* VME upgrade file */
            dbg_print(is_debug_on, "vme file\n");
            info.type = FIRMWARE_CPLD;
            ret = firmware_upgrade_set_gpio_info(info.slot);
            if (ret < 0) {
            goto upgrade_err;
            }
            ret = ispvme_main(2, argv);
        }
        else if(is_vme_file(argv[1]) == 0){ /* bin upgrade file */
            dbg_print(is_debug_on, "bin file\n");
            mem_clear(tmp, FIRMWARE_FILE_DIR_LEN);
            snprintf(tmp, FIRMWARE_FILE_DIR_LEN, "firmware_upgrade_bin %s %s %s %s", argv[1], argv[2], argv[3], argv[4]);
            ret = system(tmp);
        }
        else{
            dbg_print(is_debug_on, "unknow file\n");
            return FIRMWARE_FAILED;
        }
    }
    else if (strcmp(argv[2], FIRMWARE_FPGA_NAME) == 0) {  /* FPGA upgrade */
        info.type = FIRMWARE_FPGA;
        ret = dfd_fpga_upgrade_do_upgrade(argv[1]);
    } else {
        dbg_print(is_debug_on, "Failed to get upgrade type: %s.\n", argv[2]);
        return ERR_FW_UPGRADE;
    }

upgrade_err:
    if (ret != FIRMWARE_SUCCESS){
        dbg_print(is_debug_on, "Failed to upgrade: %s.\n", argv[1]);
    }

    return ret;
}

static void firmware_get_err_type(int err, int *real_err)
{
    int tmp_err;

    tmp_err = *real_err;

    if (tmp_err == err) {
        return;
    }
    switch (err) {
    case ERR_FW_DO_CPLD_UPGRADE:
    case ERR_FW_DO_FPGA_UPGRADE:
        if (tmp_err == ERR_FW_CHECK_CPLD_UPGRADE
            || tmp_err == ERR_FW_CHECK_FPGA_UPGRADE
            || tmp_err == FIRMWARE_SUCCESS || tmp_err == ERR_FW_UPGRADE) {
            tmp_err = err;
        }
        break;
    case ERR_FW_CHECK_CPLD_UPGRADE:
    case ERR_FW_CHECK_FPGA_UPGRADE:
        if (tmp_err == FIRMWARE_SUCCESS || tmp_err == ERR_FW_UPGRADE) {
            tmp_err = err;
        }
        break;
    case ERR_FW_UPGRADE:
        if (tmp_err == FIRMWARE_SUCCESS) {
            tmp_err = err;
        }
        break;
    case FIRMWARE_SUCCESS:
        break;
    default:
        return;
    }

    *real_err = tmp_err;

    return;
}

/* argv[1]: Pathname */
static int firmware_upgrade_one_dir(int argc, char *argv[])
{
    int ret, real_ret;
    int flag;
    DIR *dirp;
    struct dirent *dp;
    char *dir;

    dir = argv[1];
    dirp = opendir(dir);
    if (dirp  == NULL) {
        dbg_print(is_debug_on, "Failed to open the dir: %s.\n", dir);
        return ERR_FW_UPGRADE;
    }

    ret = ERR_FW_UPGRADE;
    real_ret = FIRMWARE_SUCCESS;
    flag = 0;
    for (;;) {
        /* read the pathname of the file */
        dp = readdir(dirp);
        if (dp == NULL) {
            break;
        }

        dbg_print(is_debug_on, "The file name: %s.\n", dp->d_name);
        /* check whether it is a file */
        if (firmware_check_file_is_dir(dir, dp->d_name) != 0) {
            continue;
        }

        dbg_print(is_debug_on, "\n=========== Start: %s ===========\n", dp->d_name);

        /* upgrade a upgrade file */
        ret = firmware_upgrade_file(dir, dp->d_name);
        if (ret != FIRMWARE_SUCCESS) {
            firmware_get_err_type(ret, &real_ret);
            dbg_print(is_debug_on, "Failed to upgrade the file: %s.(%d, %d)\n", dp->d_name, ret, real_ret);
        } else {
            flag = 1;
            dbg_print(is_debug_on, "Upgrade the file: %s success.\n", dp->d_name);
        }

        dbg_print(is_debug_on, "=========== End: %s ===========\n", dp->d_name);
    }

    if (flag == 1 && (real_ret == ERR_FW_CHECK_CPLD_UPGRADE || real_ret == ERR_FW_CHECK_FPGA_UPGRADE)) {
        real_ret = FIRMWARE_SUCCESS;
    }

    if (real_ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to upgrade: %s.\n", dir);
    } else {
        dbg_print(is_debug_on, "Upgrade success: %s.\n", dir);
    }

    closedir(dirp);
    return real_ret;
}

/**
 * argv[1]: file name
 * argv[2]: type
 * argv[3]: slot number
 */
static int firmware_upgrade_read_chip(int argc, char *argv[])
{

    return FIRMWARE_SUCCESS;
}

static int firmware_upgrade_test_fpga(int argc, char *argv[])
{
    int ret;
    char tmp1[128];
    char tmp2[128];

    if ((strcmp(argv[1], FIRMWARE_FPGA_NAME) != 0)
        || (strcmp(argv[2], FIRMWARE_FPGA_TEST) != 0)) {
        snprintf(tmp1, sizeof(tmp1), "%s", argv[1]);
        snprintf(tmp2, sizeof(tmp2), "%s", argv[2]);
        printf( "fpga test:Failed to Input ERR Parm, argv[1]:%s, agrv[2]:%s\n", tmp1,tmp2);
        return FIRMWARE_FAILED;
    }
    ret = dfd_fpga_upgrade_test();
    return ret;
}

static int firmware_upgrade_test_chip(int argc, char *argv[])
{
    int ret,dev_type,slot;
    int err_ret=0;
    firmware_card_info_t *hw_info;
    char tmp1[128];
    char tmp2[128];

    if ((strcmp(argv[1], FIRMWARE_CPLD_NAME) != 0)
        || (strcmp(argv[2], FIRMWARE_CPLD_TEST) != 0)) {
        snprintf(tmp1, sizeof(tmp1), "%s", argv[1]);
        snprintf(tmp2, sizeof(tmp2), "%s", argv[2]);
        printf( "gpio test:Failed to Input ERR Parm, argv[1]:%s, agrv[2]:%s\n", tmp1, tmp2);
        return FIRMWARE_FAILED;
    }

    dev_type = drv_get_my_dev_type();            /* get the type of card first */
    if (dev_type < 0) {
        printf("gpio test:drv_get_my_dev_type failed ret 0x%x.\n", dev_type);
        return FIRMWARE_FAILED;
    }

    hw_info = firmware_get_card_info(dev_type);    /* get the detail information of card */
    if (hw_info == NULL) {
        printf( "gpio test:card type 0x%x don't support firmware.\n", dev_type);
        return FIRMWARE_FAILED;
    }

    for(slot = 0; slot < hw_info->slot_num; slot++){
        ret = firmware_upgrade_set_gpio_info(slot);    /* set GPIO information */
        if(ret < 0){
            err_ret++;
            printf( "gpio test:Failed to set gpio info,dev_type 0x%x,slot %d\n", dev_type, slot);
            continue;
        }
        ret = ispvme_test();                        /* GPIO path test */
        if(ret != 0){
            err_ret++;
            printf("gpio test:ispvme_test failed,dev_type 0x%x,slot %d\n", dev_type, slot);
        }
    }
    if(err_ret != 0)
        return FIRMWARE_FAILED;
    return FIRMWARE_SUCCESS;
}

int main(int argc, char *argv[])
{
    int ret;

    is_debug_on = firmware_upgrade_debug();

    if (argc != 2 && argc != 5 && argc != 4 && argc != 3 && argc != 6) {
        printf("Use:\n");
        printf(" upgrade dir  : firmware_upgrade_ispvme dir\n");
        printf(" upgrade file : firmware_upgrade_ispvme file type slot chip_name\n");
        printf(" read chip    : firmware_upgrade_ispvme file type slot\n");
        dbg_print(is_debug_on, "Failed to upgrade the number of argv: %d.\n", argc);
        return ERR_FW_UPGRADE;
    }
#if 0
    ret = dev_drv_init();
    if (ret) {
        dbg_print(is_debug_on, "failed to init dfd ret %d.", ret);
        return ERR_FW_UPGRADE;
    }
#endif
    /* dump fpga flash operation */
    if (argc == 6) {
        if (strcmp(argv[1], "fpga_dump_flash") == 0) {
            ret = dfd_fpga_upgrade_dump_flash(argc, argv);
            printf("fpga_dump_flash ret %d.\n", ret);
            return FIRMWARE_SUCCESS;
        } else {
            printf("Not support, please check your cmd.\n");
            return FIRMWARE_SUCCESS;
        }
    }

    /* upgrade individual files */
    if (argc == 5) {
        printf("+================================+\n");
        printf("|Begin to upgrade, please wait...|\n");
        ret = firmware_upgrade_one_file(argc, argv);
        if (ret != FIRMWARE_SUCCESS) {
            if(strcmp(argv[2], FIRMWARE_CPLD_NAME) == 0)
                printf("|      CPLD Upgrade failed!      |\n");
            else if(strcmp(argv[2], FIRMWARE_FPGA_NAME) == 0)
                printf("|      FPGA Upgrade failed!      |\n");
            else
                printf("|   Failed to get upgrade type!  |\n");
            printf("+================================+\n");
            dbg_print(is_debug_on, "Failed to upgrade a firmware file: %s.\n", argv[1]);
            return ret;
        }
        if(strcmp(argv[2], FIRMWARE_CPLD_NAME) == 0)
            printf("|     CPLD Upgrade succeeded!    |\n");
        else
            printf("|     FPGA Upgrade succeeded!    |\n");
        printf("+================================+\n");
        return FIRMWARE_SUCCESS;
    }

    if (argc == 2) {
        printf("+================================+\n");
        printf("|Begin to upgrade, please wait...|\n");

        ret = firmware_upgrade_one_dir(argc, argv);
        if (ret != FIRMWARE_SUCCESS) {
            printf("|         Upgrade failed!        |\n");
            printf("+================================+\n");
            dbg_print(is_debug_on, "Failed to upgrade a firmware dir: %s.\n", argv[1]);
            return ret;
        }
        printf("|       Upgrade succeeded!       |\n");
        printf("+================================+\n");
        return FIRMWARE_SUCCESS;
    }

    if (argc == 4) {
        ret = firmware_upgrade_read_chip(argc, argv);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "Failed to read chip: %s.\n", argv[1]);
            return ret;
        }

        return FIRMWARE_SUCCESS;
    }

    if (argc == 3) {
        if (strcmp(argv[1], FIRMWARE_FPGA_NAME) == 0) {
            ret = firmware_upgrade_test_fpga(argc, argv);
            if (ret != FIRMWARE_SUCCESS) {
                printf("+=================+\n");
                printf("| FPGA TEST FAIL! |\n");
                printf("+=================+\n");
                return FIRMWARE_FAILED;
            } else {
                printf("+=================+\n");
                printf("| FPGA TEST PASS! |\n");
                printf("+=================+\n");
                return FIRMWARE_SUCCESS;
            }
        } else {
            ret = firmware_upgrade_test_chip(argc, argv);
            if (ret != FIRMWARE_SUCCESS) {
                printf("+=================+\n");
                printf("| GPIO TEST FAIL! |\n");
                printf("+=================+\n");
                return FIRMWARE_FAILED;
            } else {
                printf("+=================+\n");
                printf("| GPIO TEST PASS! |\n");
                printf("+=================+\n");
                return FIRMWARE_SUCCESS;
            }
        }
    }

    return ERR_FW_UPGRADE;
}