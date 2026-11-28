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
#include "firmware_common.h"

int header_offset;

static const char *firmware_app_basename(const char *path)
{
    const char *name;

    if (path == NULL) {
        return NULL;
    }

    name = strrchr(path, '/');
    return (name == NULL) ? path : (name + 1);
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

static void firmware_cmd_usage_legacy(void)
{
    printf("Use:\n");
    printf(" upgrade file with header : firmware_upgrade file main_type sub_type slot\n");
    printf(" upgrade file with/without heade: firmware_upgrade file main_type sub_type slot file_type chain\n");
    printf(" upgrade test with header: firmware_upgrade test file main_type sub_type slot\n");
    printf(" upgrade test with/without heade: firmware_upgrade test file main_type sub_type slot file_type chain\n");
    printf(" spi_logic_dev dump : firmware_upgrade dump spi_logic_dev dev_path offset size print/record_file_path\n");
    printf(" spi_logic_dev : firmware_upgrade spi_logic_dev dev_path get_flash_id\n");
    printf(" fw_up_bios reserved cmd : fw_up_bios [-u] [args ...]\n");
    return;
}

/* Backward compatible API reserved */
static void print_usage(void)
{
    return firmware_cmd_usage_legacy();
}

int firmware_upgrade_legacy(int argc, char *argv[])
{
    int ret;
    uint32_t flash_id;

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

 static int firmware_cmd_parse_legacy(firmware_app_cmd_t *cmd)
{
    if ((cmd == NULL) || (cmd->argv == NULL) || (cmd->argc <= 0)) {
        return FIRMWARE_FAILED;
    }
    return FIRMWARE_SUCCESS;
}

static int firmware_cmd_exec_legacy(firmware_app_cmd_t *cmd)
{
    return firmware_upgrade_legacy(cmd->argc, cmd->argv);
}


/* Hierarchical framework:
 *  top：[user level]cmd parse and parameter passing, handling user intentions and input data, mainly firmware_app.c firmware_app_up_bios.c;
 *  middle：[protocol layer]handling intermediate protocols, such as bin file header parsing, jtag vme data protocol, mainly firmware_common.c, etc.;
 *  bottom：[driver layer]executing specific upgrade logic based on user intentions passed from the middle layer, mainly firmware_upgrade_mtd.c, etc.;
 */

/* Future command extension decoupling, new commands can be created for reuse, bottom layer operations reused, top layer command parsing separated */ 
static const firmware_cmd_entry_t g_firmware_cmd_table[] = {
    {
        "firmware_upgrade", 
        FIRMWARE_APP_CMD_LEGACY,
        firmware_cmd_parse_legacy,
        firmware_cmd_exec_legacy,
        NULL,
        firmware_cmd_usage_legacy
    },
    {
        "fw_up_bios", 
        FIRMWARE_APP_CMD_UPGRADE_BIOS, 
        firmware_cmd_parse_fw_up_bios, 
        firmware_cmd_exec_fw_up_bios, 
        firmware_cmd_finish_fw_up_bios,
        firmware_cmd_usage_fw_up_bios
    },
    {},
};

static const firmware_cmd_entry_t *firmware_find_cmd_entry(const char *cmd_name)
{
    size_t i;
    size_t cmd_count = sizeof(g_firmware_cmd_table) / sizeof(g_firmware_cmd_table[0]);
    const firmware_cmd_entry_t *entry;

    if (cmd_name == NULL) {
        return NULL;
    }

    for (i = 0; i < cmd_count; ++i) {
        entry = &g_firmware_cmd_table[i];
        /* NULL is used to indicate the end of the command table */
        if (entry->name == NULL) {
            break;
        }

        if (strcmp(cmd_name, entry->name) == 0) {
            return entry;
        }
    }

    return NULL;
}


int main(int argc, char *argv[])
{
    int ret = ERR_FW_UPGRADE;
    const char *cmd_name;
    const firmware_cmd_entry_t *entry = NULL;
    firmware_app_cmd_t cmd;

    mem_clear(&cmd, sizeof(cmd));
    is_debug_on = firmware_upgrade_debug();

    signal(SIGTERM, SIG_IGN);   /* ignore kill signal */
    signal(SIGINT, SIG_IGN);    /* ignore ctrl+c signal */
    signal(SIGTSTP, SIG_IGN);   /* ignore ctrl+z signal */

    if ((argc <= 0) || (argv == NULL) || (argv[0] == NULL)) {
        dbg_print(is_debug_on, "Error: invalid argv input.\n");
        return ERR_FW_UPGRADE;
    }

    cmd.argc = argc;
    cmd.argv = argv;
    cmd_name = firmware_app_basename(argv[0]);
    cmd.cmd_name = cmd_name;

    entry = firmware_find_cmd_entry(cmd.cmd_name);
    if (entry == NULL) {
        dbg_print(is_debug_on, "Error: unsupported command name '%s'.\n", cmd.cmd_name);
        return ERR_FW_UPGRADE;
    }
    cmd.cmd_type = entry->type;

    ret = entry->parse(&cmd);
    if (ret != FIRMWARE_SUCCESS) {
        if (entry->usage != NULL) {
            entry->usage();
        } else {
            print_usage();
        }
        return ret;
    }

    ret = entry->exec(&cmd);
    if (ret != FIRMWARE_SUCCESS) {
        return ret;
    }

    if (entry->finish != NULL) {
        ret = entry->finish(&cmd);
        if (ret != FIRMWARE_SUCCESS) {
            return ret;
        }
    }

    return ret;

}
