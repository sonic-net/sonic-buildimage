#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <firmware_app.h>
#include <firmware_common.h>
#include <firmware_upgrade_mtd.h>

int firmware_cmd_parse_fw_up_bios(firmware_app_cmd_t *cmd)
{
    int opt;
    int arg_right;
    char *abs_path;
    static struct option long_options[] = {
        {"file", required_argument, NULL, 'f'},
        {"uconf-skip", no_argument, NULL, 'u'},
        {"checked", no_argument, NULL, 'c'},
        {"test", no_argument, NULL, 't'},
        {0, 0, 0, 0},
    };

    if ((cmd == NULL) || (cmd->argv == NULL) || (cmd->argc <= 0)) {
        return FIRMWARE_FAILED;
    }

    cmd->bios.uconf_skip = 0;
    cmd->bios.checked = 0;
    cmd->bios.up_file_path = NULL;
    cmd->bios.remain_argc = 0;
    cmd->bios.remain_argv = NULL;
    arg_right = 0;

    opterr = 0;
    optind = 1;
    while ((opt = getopt_long(cmd->argc, cmd->argv, "+f:uct", long_options, NULL)) != -1) {
        switch (opt) {
        case 'u':
            cmd->bios.uconf_skip = 1;
            break;
        case 'c':
            cmd->bios.checked = 1;
            break;
        case 't':
            cmd->test_mode = 1;
            break;
        case 'f':
            /* Considering the BMC's small memory scenario, using the heap is more appropriate than consuming 4K of stack space for PATH_MAX */
            abs_path = realpath(optarg, NULL);
            if (abs_path == NULL) {
                dbg_print(is_debug_on,
                    "Error: fw_up_bios failed to resolve file path '%s', errno=%d(%s)\n",
                    optarg, errno, strerror(errno));
                return FIRMWARE_FAILED;
            }
            if (cmd->bios.up_file_path != NULL) {
                free(cmd->bios.up_file_path);
            }
            cmd->bios.up_file_path = abs_path;
            arg_right = 1;
            break;
        case '?':
        default:
            dbg_print(is_debug_on, "Error: fw_up_bios invalid option: %s\n", cmd->argv[optind - 1]);
            return FIRMWARE_FAILED;
        }
    }

    if (!arg_right) {
        dbg_print(is_debug_on, "Error: fw_up_bios requires -f/--file option.\n");
        return FIRMWARE_FAILED;
    }

    cmd->bios.remain_argc = cmd->argc - optind;
    cmd->bios.remain_argv = &cmd->argv[optind];
    return FIRMWARE_SUCCESS;
}

/*
 * firmware_cmd_bios_sort_info
 * function: Based on the BIOS header information, distribute the user's intentions (e.g., uconf_skip) to the lower layer
 * @info: param[in] name info
 * return value : success--FIRMWARE_SUCCESS, other fail return error code
 */
static int firmware_cmd_bios_sort_info(name_info_t *info, firmware_app_cmd_bios_t *bios)
{
    int ret;
    void *mtd_ctx;

    if (info == NULL || bios == NULL) {
        return FIRMWARE_FAILED;
    }

    mtd_ctx = NULL;
    /* In the future, different types of ctx can be obtained based on the type in info. Currently, only MTD is supported.
     * The MTD ctx is obtained to pass resources from the upper layer to the lower layer.
     * The design consideration is that this level absolutely does not handle low-level sensitive information such as MTD/I2C, only provides a general ctx interface.
     * The lower layer parses and obtains resources based on the information in the ctx.
     * If there are other types of upgrades in the future, the same design can be followed.
     */
    switch (info->file_type)
    {
    case FIRMWARE_MTD:
        dbg_print(is_debug_on, "Allocating mtd upgrade context.\n");
        ret = firmware_upgrade_mtd_ctx_alloc(&mtd_ctx);
        if (ret != FIRMWARE_SUCCESS) {
            dbg_print(is_debug_on, "Failed to alloc mtd upgrade context.\n");
            return ret;
        }

        ((firmware_mtd_upg_ctx_t *)mtd_ctx)->uconf_skip = bios->uconf_skip;
        break;
    default:
        printf("Error: fw_up_bios unsupported file type %d.\n", info->file_type);
        return FIRMWARE_FAILED;
    }

    info->priv_data = mtd_ctx;
    return FIRMWARE_SUCCESS;
}

int firmware_cmd_exec_fw_up_bios(firmware_app_cmd_t *cmd)
{
    int ret;
    char *file_name;
    name_info_t *header_info;
    firmware_app_cmd_bios_t *bios;

    ret = FIRMWARE_FAILED;
    file_name = NULL;
    header_info = NULL;
    bios = NULL;

    if (cmd == NULL) {
        goto exit;
    }

    bios = &cmd->bios;

    dbg_print(is_debug_on,
              "fw_up_bios parsed. -u:%d, -c:%d, remain_argc:%d (reserved command framework).\n",
              bios->uconf_skip, bios->checked, bios->remain_argc);

    /* The -c option indicates whether the board compatibility has been checked. The check is performed by upgrade.py.
     * It is possible to consider performing the check within the command itself, but to adapt to the existing framework,
     * the command does not perform product information verification for now. */
    if (!bios->checked) {
        printf("+=====================================+\n");
        printf("|  Product info has not been checked! |\n");
        printf("|  Please ensure the file is correct! |\n");
        printf("+=====================================+\n");
        ret = FIRMWARE_FAILED;
        goto exit;
    }

    if (cmd->bios.remain_argc > 0) {
        dbg_print(is_debug_on, "fw_up_bios remain argv[0]: %s\n", cmd->bios.remain_argv[0]);
    }

    /* 0. Get header information, mainly to obtain lower-level related information */
    header_info = (name_info_t *)malloc(sizeof(name_info_t));
    if (header_info == NULL) {
        dbg_print(is_debug_on, "Error: fw_up_bios failed to malloc memory for name_info_t.\n");
        ret = FIRMWARE_FAILED;
        goto exit;
    }
    mem_clear(header_info, sizeof(name_info_t));
    file_name = bios->up_file_path;
    if (file_name == NULL) {
        dbg_print(is_debug_on, "Error: fw_up_bios missing input file path.\n");
        ret = FIRMWARE_FAILED;
        goto exit;
    }

    ret = firmware_upgrade_read_header(file_name, header_info);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to get file header: %s\n", file_name);
        goto exit;
    }

    /* Currently, upgrades without headers are not supported. In the future, options like -f force and -m mtd-name may be added to directly flash the MTD for debugging purposes.
     * Upgrades of non-MTD types are not supported for now. Support may be added as needed. Currently, BIOS upgrades are all of the MTD type.
     */
    if (header_info->header_exist == FIRMWARE_WITHOUT_HEADER) {
        printf("+=====================================+\n");
        printf("|  Unsupported file!                   |\n");
        printf("|  Only support file with header       |\n");
        printf("+=====================================+\n");
        ret = FIRMWARE_FAILED;
        goto exit;
    }

    /* fw_up_bios currently only executes the upgrade process once, defaulting to the first chain in the header.
     * The validity of the chain_list has already been checked in firmware_upgrade_read_header, so it can be used directly.
     */
    if (header_info->chain_list[0] == FIRMWARE_INVALID_CHAIN) {
        printf("+=====================================+\n");
        printf("|  Unsupported file!                   |\n");
        printf("|  Header chain is missing             |\n");
        printf("+=====================================+\n");
        ret = FIRMWARE_FAILED;
        goto exit;
    }
    header_info->chain = header_info->chain_list[0];
    dbg_print(is_debug_on, "fw_up_bios use header chain_list[0]=%d as chain=%d\n",
              header_info->chain_list[0], header_info->chain);


    /* 1. Get the lower-level ctx, passing the top-level user intentions to the lower level, such as uconf_skip, to skip partition upgrades */
    ret = firmware_cmd_bios_sort_info(header_info, bios);
    if (ret != FIRMWARE_SUCCESS) {
        dbg_print(is_debug_on, "Failed to sort bios upgrade info for file %s.\n", file_name);
        goto exit;
    }

    /* 2. Execute the upgrade, writing the file to the target partition. 
     * When saving the configuration, the upgrade is segmented, and the segment information is passed through header_info->priv_data
     */
    if (cmd->test_mode) {
        printf("fw_up_bios is in test mode, skip actual upgrade.\n");
        ret = firmware_upgrade_test(file_name, header_info);
    } else {
        ret = firmware_upgrade(file_name, header_info);
    }
    printf("+=====================================+\n");
    if (ret == FIRMWARE_SUCCESS) {
        printf("|         Upgrade succeeded!        |\n");
    } else {
        printf("|         Upgrade failed!           |\n");
    }
    printf("+=====================================+\n");

exit:
    /* 3. Upgrade completed, release resources */
    if ((header_info != NULL) && (header_info->priv_data != NULL)) {
        /* Currently, only MTD upgrades have segment information, and the segment information is in the block_info field of the firmware_mtd_upg_ctx_t structure.
         * If there are other types of upgrades in the future, there may be different ctx structures. Design-wise, a general free function pointer can be added to firmware_mtd_upg_ctx_t,
         * or different free functions can be called in firmware_cmd_exec_fw_up_bios based on the file_type. For now, we simply distinguish MTD upgrades. */
        if (header_info->file_type == FIRMWARE_MTD) {
            firmware_upgrade_mtd_ctx_free(&header_info->priv_data);
        } else {
            free(header_info->priv_data);
            header_info->priv_data = NULL;
        }
    }

    if (header_info != NULL) {
        free(header_info);
    }

    return ret;
}

int firmware_cmd_finish_fw_up_bios(firmware_app_cmd_t *cmd)
{
    if (cmd == NULL) {
        return FIRMWARE_FAILED;
    }

    /* realpath may allocate memory for the file path, free it if it's not NULL */
    if (cmd->bios.up_file_path != NULL) {
        free(cmd->bios.up_file_path);
        cmd->bios.up_file_path = NULL;
    }

    dbg_print(is_debug_on, "fw_up_bios finish.\n");

    return FIRMWARE_SUCCESS;
}

void firmware_cmd_usage_fw_up_bios(void)
{
    printf("Usage:\n");
    printf(" fw_up_bios [OPTIONS]\n");
    printf("OPTIONS:\n");
    printf(" -f, --file <path>           Input firmware file path, resolved to absolute path.\n");
    printf(" -u, --uconf-skip            Skip upgrading user configuration.\n");
    printf(" -c, --checked               The input file has already passed header/env compatibility check.\n");
    printf(" -t, --test                  Upgrade test mode, will not perform actual upgrade.\n");
}
