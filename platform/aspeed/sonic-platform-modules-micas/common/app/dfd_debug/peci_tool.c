/**
 * PECI Tool - Platform Environment Control Interface Utility
 *
 * This tool provides a command-line interface to interact with PECI devices,
 * allowing users to perform various PECI operations such as:
 * - Ping PECI devices
 * - Get device temperature
 * - Read/write package configuration
 * - Read/write IA MSRs
 * - Read/write PCI configuration
 * - Crash dump operations
 *
 * Usage: peci_tool <command> <address> [arguments...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

#include <peci-ioctl.h>

/**
 * Print usage information for the PECI tool
 *
 * Displays all available commands, their arguments, and usage examples
 */
static void print_usage(void)
{
    printf("Commands and Arguments:\n");
    printf("  xfer <addr> <tx_len> <rx_len> <tx_data_hex>...\n");
    printf("  ping <addr>\n");
    printf("  get_dib <addr>\n");
    printf("  get_temp <addr>\n");
    printf("  rd_pkg_cfg <addr> <index> <param> <rx_len>\n");
    printf("  wr_pkg_cfg <addr> <index> <param> <value>\n");
    printf("  rd_ia_msr <addr> <thread id> <address>\n");
    printf("  wr_ia_msr <addr> <thread id> <address> <value>\n");
    printf("  rd_ia_msrex <addr> <thread_id> <address>\n");
    printf("  rd_pci_cfg <addr> <bus> <device> <function> <reg>\n");
    printf("  wr_pci_cfg <addr> <bus> <device> <function> <reg> <tx_data>\n");
    printf("  rd_pci_cfg_local <addr> <bus> <device> <function> <reg> <rx_len>\n");
    printf("  wr_pci_cfg_local <addr> <bus> <device> <function> <reg> <tx_data>\n");
    printf("  rd_end_pt_cfg <addr> <msg_type> <seg> <bus> <device> <func> <reg> <rx_len> (PCI type)\n");
    printf("  wr_end_pt_cfg <addr> <msg_type> <seg> <bus> <device> <func> <reg> <value> (PCI type)\n");
    printf("  crashdump_disc <addr> <subopcode> <param0> <param1>\n");
    printf("  crashdump_get_frame <addr> <param0> <param1> <param2>\n");
}

/**
 * Perform PECI raw transfer operation
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments
 * @param argv - Argument list: <tx_len> <rx_len> [tx_data...]
 * @return 0 on success, negative error code on failure
 */
static int do_peci_xfer(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    unsigned int val;
    int i;

    /* xfer <addr> <tx_len> <rx_len> [tx_data...] */
    if (argc < 2 || argv == NULL) {
        print_usage();
        return -1;
    }

    info->cmd_info.xfer.addr = info->addr;
    (void)sscanf(argv[0], "%hhu", &info->cmd_info.xfer.tx_len);
    (void)sscanf(argv[1], "%hhu", &info->cmd_info.xfer.rx_len);

    if ((info->cmd_info.xfer.tx_len > 0) && (argc - 2 != info->cmd_info.xfer.tx_len)) {
        fprintf(stderr, "invalid parameter\n");
        return -1;
    }

    info->msg_len = sizeof(struct peci_xfer_msg);
    info->cmd_info.xfer.tx_buf = NULL;
    info->cmd_info.xfer.rx_buf = NULL;
    if (info->cmd_info.xfer.tx_len > 0) {
        info->cmd_info.xfer.tx_buf = malloc(info->cmd_info.xfer.tx_len);
        if (!info->cmd_info.xfer.tx_buf) { 
            fprintf(stderr, "malloc failed\n"); 
            return -1; 
        }
        
        for (i = 0; i < info->cmd_info.xfer.tx_len; i++) {
            (void)sscanf(argv[2 + i], "%x", &val);
            info->cmd_info.xfer.tx_buf[i] = (uint8_t)val;
        }
    }

    if (info->cmd_info.xfer.rx_len > 0) {
        info->cmd_info.xfer.rx_buf = malloc(info->cmd_info.xfer.rx_len);
        if (!info->cmd_info.xfer.rx_buf) { 
            free(info->cmd_info.xfer.tx_buf);
            fprintf(stderr, "malloc failed\n"); 
            return -1; 
        }
    }

    if (ioctl(fd, PECI_IOC_XFER, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_XFER failed\n");
        /* Safe to free NULL as well */
        free(info->cmd_info.xfer.tx_buf);
        free(info->cmd_info.xfer.rx_buf);
        return -1;
    } else {
        printf("=== Xfer Result ===\n");
        printf("Return: 0x%02x\n", info->cmd_ret);
        if (info->cmd_ret == 0) {
            if (info->cmd_info.xfer.tx_len > 0) {
                printf("TX Success. ");
            }

            if (info->cmd_info.xfer.rx_len > 0) {
                printf("RX Success.\n");
                printf("RX Data: ");
                for (i = 0; i < info->cmd_info.xfer.rx_len; i++) {
                    printf("%02x ", info->cmd_info.xfer.rx_buf[i]);
                }
            }
        } else {
            printf("do peci xfer fail.\n");
        }
        printf("\n");
    }
    
    /* Safe to free NULL as well */
    free(info->cmd_info.xfer.tx_buf);
    free(info->cmd_info.xfer.rx_buf);

    return 0;
}

/**
 * Perform PECI ping operation to check device availability
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 0)
 * @param argv - Argument list (not used)
 * @return 0 on success, negative error code on failure
 */
static int do_peci_ping(int fd, peci_tool_info_t *info, int argc,char *argv[])
{
    (void)argv;

    if (argc != 0) {
        print_usage();
        return -1;
    }

    info->cmd_info.ping.addr = info->addr;
    if (ioctl(fd, PECI_IOC_PING, info) < 0) {
        fprintf(stderr, "Failed to ping PECI device at address 0x%02x\n", info->addr);
        return -1;
    } else {
        printf("=== Ping Result ===\n");
        printf("Addr: 0x%02x, Return: %d (CC: 0x%02x)\n", 
                info->cmd_info.ping.addr, info->cmd_ret, info->cmd_ret);
    }

    return 0;
}

/**
 * Get Device ID Block (DIB) from PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 0)
 * @param argv - Argument list (not used)
 * @return 0 on success, negative error code on failure
 */
static int do_peci_get_dib(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    (void)argv;

    if (argc != 0) {
        print_usage();
        return -1;
    }

    info->cmd_info.dib.addr = info->addr;
    info->msg_len = sizeof(struct peci_get_dib_msg);
    if (ioctl(fd, PECI_IOC_GET_DIB, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_GET_DIB failed\n");
        return -1;
    } else {
        printf("=== GetDIB Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x, DIB: 0x%016llx\n", info->cmd_ret, 
            (unsigned long long)info->cmd_info.dib.dib);
    }

    return 0;
}

/**
 * Get temperature reading from PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 0)
 * @param argv - Argument list (not used)
 * @return 0 on success, negative error code on failure
 */
static int do_peci_get_temp(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    (void)argv;

    if (argc != 0) {
        print_usage();
        return -1;
    }

    info->cmd_info.temp.addr = info->addr;
    info->msg_len = sizeof(struct peci_get_temp_msg);
    if (ioctl(fd, PECI_IOC_GET_TEMP, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_GET_TEMP failed\n");
        return -1;
    } else {
        printf("=== GetTemp Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x, Raw Temp: 0x%04x (%d)\n", 
                info->cmd_ret, (unsigned short)info->cmd_info.temp.temp_raw, 
                (short)info->cmd_info.temp.temp_raw);
    }

    return 0;
}

/**
 * Read package configuration from PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 3)
 * @param argv - Argument list: <index> <param> <rx_len>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_rd_pkg_cfg(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    int i;

    /* argv: index param rx_len */
    if (argc != 3) {
        print_usage();
        return -1;
    }
    info->cmd_info.rd_pkg_cfg.addr = info->addr;
    (void)sscanf(argv[0], "%hhx", &info->cmd_info.rd_pkg_cfg.index);
    (void)sscanf(argv[1], "%hx",  &info->cmd_info.rd_pkg_cfg.param);
    (void)sscanf(argv[2], "%hhx",  &info->cmd_info.rd_pkg_cfg.rx_len);
    info->msg_len = sizeof(struct peci_rd_pkg_cfg_msg);
    if (ioctl(fd, PECI_IOC_RD_PKG_CFG, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_RD_PKG_CFG failed\n");
        return -1;
    } else {
        printf("=== RdPkgCfg Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x\n", info->cmd_info.rd_pkg_cfg.cc);
        printf("Data: ");
        /* Print up to 4 bytes of data, depending on rx_len */
        for (i = 0; i < info->cmd_info.rd_pkg_cfg.rx_len && i < 4; i++) {   
            printf("%02x ", info->cmd_info.rd_pkg_cfg.pkg_config[i]);
        }
        printf("\n");
    }

    return 0;
}

/**
 * Write package configuration to PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 3)
 * @param argv - Argument list: <index> <param> <value>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_wr_pkg_cfg(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    /* argv: index param value */
    if (argc != 3) {
        print_usage();
        return -1;
    }

    info->cmd_info.wr_pkg_cfg.addr = info->addr;
    (void)sscanf(argv[0], "%hhx", &info->cmd_info.wr_pkg_cfg.index);
    (void)sscanf(argv[1], "%hx",  &info->cmd_info.wr_pkg_cfg.param);
    (void)sscanf(argv[2], "%x", &info->cmd_info.wr_pkg_cfg.value);
    info->msg_len = sizeof(struct peci_wr_pkg_cfg_msg);
    /* tx len is 4 bytes */
    info->cmd_info.wr_pkg_cfg.tx_len = 4; 

    if (ioctl(fd, PECI_IOC_WR_PKG_CFG, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_WR_PKG_CFG failed\n");
        return -1;
    } else {
        printf("=== WrPkgCfg Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x\n", info->cmd_info.wr_pkg_cfg.cc);
    }

    return 0;
}

/**
 * Read IA MSR (Model Specific Register) from PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 2)
 * @param argv - Argument list: <thread_id> <address>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_rd_ia_msr(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    if (argc != 2) {
        print_usage();
        return -1;
    }
    info->cmd_info.rd_ia_msr.addr = info->addr;
    (void)sscanf(argv[0], "%hhx", &info->cmd_info.rd_ia_msr.thread_id);
    (void)sscanf(argv[1], "%hx",  &info->cmd_info.rd_ia_msr.address);
    info->msg_len = sizeof(struct peci_rd_ia_msr_msg);
    if (ioctl(fd, PECI_IOC_RD_IA_MSR, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_RD_IA_MSR failed\n");
        return -1;
    } else {
        printf("=== RdIaMsr Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x, Value: 0x%016llx\n", 
                info->cmd_info.rd_ia_msr.cc, 
                (unsigned long long)info->cmd_info.rd_ia_msr.value);
    }

    return 0;
}

/**
 * Write IA MSR (Model Specific Register) to PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 3)
 * @param argv - Argument list: <thread_id> <address> <value>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_wr_ia_msr(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    if (argc != 3) {
        print_usage();
        return -1;
    }
    info->cmd_info.wr_ia_msr.addr = info->addr;
    (void)sscanf(argv[0], "%hhx", &info->cmd_info.wr_ia_msr.thread_id);
    (void)sscanf(argv[1], "%hx",  &info->cmd_info.wr_ia_msr.address);
    (void)sscanf(argv[2], "%llx", &info->cmd_info.wr_ia_msr.value);
    info->cmd_info.wr_ia_msr.tx_len = 8;
    info->msg_len = sizeof(struct peci_wr_ia_msr_msg);

    if (ioctl(fd, PECI_IOC_WR_IA_MSR, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_WR_IA_MSR failed\n");
        return -1;
    } else {
        printf("=== WrIaMsr Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x\n", info->cmd_info.wr_ia_msr.cc);
    }

    return 0;
}

/**
 * Read IA MSR (Model Specific Register) with extended thread ID from PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 2)
 * @param argv - Argument list: <thread_id> <address>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_rd_ia_msrex(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    if (argc != 2) {
        print_usage();
        return -1;
    }

    info->cmd_info.rd_ia_msrex.addr = info->addr;
    (void)sscanf(argv[0], "%hu",  &info->cmd_info.rd_ia_msrex.thread_id);
    (void)sscanf(argv[1], "%hx",  &info->cmd_info.rd_ia_msrex.address);
    info->msg_len = sizeof(struct peci_rd_ia_msrex_msg);
    if (ioctl(fd, PECI_IOC_RD_IA_MSREX, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_RD_IA_MSREX failed\n");
        return -1;
    } else {
        printf("=== RdIaMsrEx Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x, Value: 0x%016llx\n", 
                info->cmd_info.rd_ia_msrex.cc, 
                (unsigned long long)info->cmd_info.rd_ia_msrex.value);
    }

    return 0;
}

/**
 * Read PCI configuration from PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 4)
 * @param argv - Argument list: <bus> <device> <function> <reg>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_rd_pci_cfg(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    int i;

    if (argc != 4) {
        print_usage();
        return -1;
    }

    info->cmd_info.rd_pci_cfg.addr = info->addr;
    (void)sscanf(argv[0], "%hhx", &info->cmd_info.rd_pci_cfg.bus);
    (void)sscanf(argv[1], "%hhx", &info->cmd_info.rd_pci_cfg.device);
    (void)sscanf(argv[2], "%hhx", &info->cmd_info.rd_pci_cfg.function);
    (void)sscanf(argv[3], "%hx",  &info->cmd_info.rd_pci_cfg.reg);
    info->msg_len = sizeof(struct peci_rd_pci_cfg_msg);
    if (ioctl(fd, PECI_IOC_RD_PCI_CFG, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_RD_PCI_CFG failed\n");
        return -1;
    } else {
        printf("=== RdPciCfg Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x\n", info->cmd_info.rd_pci_cfg.cc);
        printf("Data: ");
        for (i = 0; i < 4; i++) {   /* PCI config data is always 4 bytes */
            printf("%02x ", info->cmd_info.rd_pci_cfg.pci_config[i]);
        }
        printf("\n");
    }

    return 0;
}

/**
 * Write PCI configuration to PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 5)
 * @param argv - Argument list: <bus> <device> <function> <reg> <value>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_wr_pci_cfg(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    if (argc != 5) {
        print_usage();
        return -1;
    }

    info->cmd_info.wr_pci_cfg.addr = info->addr;
    (void)sscanf(argv[0], "%hhx", &info->cmd_info.wr_pci_cfg.bus);
    (void)sscanf(argv[1], "%hhx", &info->cmd_info.wr_pci_cfg.device);
    (void)sscanf(argv[2], "%hhx", &info->cmd_info.wr_pci_cfg.function);
    (void)sscanf(argv[3], "%hx",  &info->cmd_info.wr_pci_cfg.reg);
    (void)sscanf(argv[4], "%hhx",   &info->cmd_info.wr_pci_cfg.pci_config[0]);
    info->cmd_info.wr_pci_cfg.cc = 0;
    info->cmd_info.wr_pci_cfg.tx_len = 4; /* default 4 bytes */
    info->msg_len = sizeof(struct peci_wr_pci_cfg_msg);

    if (ioctl(fd, PECI_IOC_WR_PCI_CFG, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_WR_PCI_CFG failed\n");
        return -1;
    } else {
        printf("=== WrPciCfg Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x\n", info->cmd_info.wr_pci_cfg.cc);
    }

    return 0;
}

/**
 * Read local PCI configuration from PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 5)
 * @param argv - Argument list: <bus> <device> <function> <reg> <rx_len>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_rd_pci_cfg_local(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    int i;

    if (argc != 5) {
        print_usage();
        return -1;
    }
    info->cmd_info.rd_pci_cfg_local.addr = info->addr;
    (void)sscanf(argv[0], "%hhx", &info->cmd_info.rd_pci_cfg_local.bus);
    (void)sscanf(argv[1], "%hhx", &info->cmd_info.rd_pci_cfg_local.device);
    (void)sscanf(argv[2], "%hhx", &info->cmd_info.rd_pci_cfg_local.function);
    (void)sscanf(argv[3], "%hx",  &info->cmd_info.rd_pci_cfg_local.reg);
    (void)sscanf(argv[4], "%hhx", &info->cmd_info.rd_pci_cfg_local.rx_len);
    info->msg_len = sizeof(struct peci_rd_pci_cfg_local_msg);

    if (ioctl(fd, PECI_IOC_RD_PCI_CFG_LOCAL, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_RD_PCI_CFG_LOCAL failed\n");
        return -1;
    } else {
        printf("=== RdPciCfgLocal Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x\n", info->cmd_info.rd_pci_cfg_local.cc);
        printf("Data: ");
        for (i = 0; i < 4; i++) {   /* PCI config data is always 4 bytes */
            printf("%02x ", info->cmd_info.rd_pci_cfg_local.pci_config[i]);
        }
        printf("\n");
    }

    return 0;
}

/**
 * Write local PCI configuration to PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 5)
 * @param argv - Argument list: <bus> <device> <function> <reg> <value>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_wr_pci_cfg_local(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    if (argc != 5) {
        print_usage();
        return -1;
    }
    info->cmd_info.wr_pci_cfg_local.addr = info->addr;
    (void)sscanf(argv[0], "%hhx", &info->cmd_info.wr_pci_cfg_local.bus);
    (void)sscanf(argv[1], "%hhx", &info->cmd_info.wr_pci_cfg_local.device);
    (void)sscanf(argv[2], "%hhx", &info->cmd_info.wr_pci_cfg_local.function);
    (void)sscanf(argv[3], "%hx",  &info->cmd_info.wr_pci_cfg_local.reg);
    (void)sscanf(argv[4], "%x",   &info->cmd_info.wr_pci_cfg_local.value);
    info->cmd_info.wr_pci_cfg_local.tx_len = 4;
    info->msg_len = sizeof(struct peci_wr_pci_cfg_local_msg);

    if (ioctl(fd, PECI_IOC_WR_PCI_CFG_LOCAL, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_WR_PCI_CFG_LOCAL failed\n");
        return -1;
    } else {
        printf("=== WrPciCfgLocal Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x\n", info->cmd_info.wr_pci_cfg_local.cc);
    }

    return 0;
}

/**
 * Read endpoint configuration from PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 7)
 * @param argv - Argument list: <msg_type> <seg> <bus> <device> <func> <reg> <rx_len>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_rd_end_pt_cfg(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    int i;

    if (argc != 7) {
        print_usage();
        return -1;
    }

    info->cmd_info.rd_end_pt_cfg.addr = info->addr;
    (void)sscanf(argv[0], "%hhx", &info->cmd_info.rd_end_pt_cfg.msg_type);

    /* only support pci, not support mmio */
    if (info->cmd_info.rd_end_pt_cfg.msg_type == PECI_ENDPTCFG_TYPE_PCI ||
        info->cmd_info.rd_end_pt_cfg.msg_type == PECI_ENDPTCFG_TYPE_LOCAL_PCI) {
        sscanf(argv[1], "%hhx", &info->cmd_info.rd_end_pt_cfg.params.pci_cfg.seg);
        sscanf(argv[2], "%hhx", &info->cmd_info.rd_end_pt_cfg.params.pci_cfg.bus);
        sscanf(argv[3], "%hhx", &info->cmd_info.rd_end_pt_cfg.params.pci_cfg.device);
        sscanf(argv[4], "%hhx", &info->cmd_info.rd_end_pt_cfg.params.pci_cfg.function);
        sscanf(argv[5], "%hx",  &info->cmd_info.rd_end_pt_cfg.params.pci_cfg.reg);
        sscanf(argv[6], "%hhx", &info->cmd_info.rd_end_pt_cfg.rx_len);
        info->msg_len = sizeof(struct peci_rd_end_pt_cfg_msg);
    } else {
        fprintf(stderr, "Unsupported msg_type for this example\n");
        return -1;
    }

    if (ioctl(fd, PECI_IOC_RD_END_PT_CFG, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_RD_END_PT_CFG failed\n");
        return -1;
    } else {
        printf("=== RdEndPtCfg Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x\n", info->cmd_info.rd_end_pt_cfg.cc);
        printf("Data: ");
        /* Print up to 8 bytes of data, depending on rx_len */
        for (i = 0; i < info->cmd_info.rd_end_pt_cfg.rx_len && i < 8; i++) {
            printf("%02x ", info->cmd_info.rd_end_pt_cfg.data[i]);
        }
        printf("\n");
    }

    return 0;
}

/**
 * Write endpoint configuration to PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 7)
 * @param argv - Argument list: <msg_type> <seg> <bus> <device> <func> <reg> <value>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_wr_end_pt_cfg(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    if (argc != 7) {
        print_usage();
        return -1;
    }

    info->cmd_info.wr_end_pt_cfg.addr = info->addr;
    (void)sscanf(argv[0], "%hhx", &info->cmd_info.wr_end_pt_cfg.msg_type);

    /* only support pci, not support mmio */
    if (info->cmd_info.wr_end_pt_cfg.msg_type == PECI_ENDPTCFG_TYPE_PCI ||
        info->cmd_info.wr_end_pt_cfg.msg_type == PECI_ENDPTCFG_TYPE_LOCAL_PCI) {
        sscanf(argv[1], "%hhx", &info->cmd_info.wr_end_pt_cfg.params.pci_cfg.seg);
        sscanf(argv[2], "%hhx", &info->cmd_info.wr_end_pt_cfg.params.pci_cfg.bus);
        sscanf(argv[3], "%hhx", &info->cmd_info.wr_end_pt_cfg.params.pci_cfg.device);
        sscanf(argv[4], "%hhx", &info->cmd_info.wr_end_pt_cfg.params.pci_cfg.function);
        sscanf(argv[5], "%hx",  &info->cmd_info.wr_end_pt_cfg.params.pci_cfg.reg);
        sscanf(argv[6], "%llx", &info->cmd_info.wr_end_pt_cfg.value);
        info->msg_len = sizeof(struct peci_wr_end_pt_cfg_msg);
    } else {
        fprintf(stderr, "Unsupported msg_type for this example\n");
        return -1;
    }
    
    info->cmd_info.wr_end_pt_cfg.tx_len = 8;

    if (ioctl(fd, PECI_IOC_WR_END_PT_CFG, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_WR_END_PT_CFG failed\n");
        return -1;
    } else {
        printf("=== WrEndPtCfg Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x\n", info->cmd_info.wr_end_pt_cfg.cc);
    }

    return 0;
}

/**
 * Discover crash dump information from PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 3)
 * @param argv - Argument list: <subopcode> <param0> <param1>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_crashdump_disc(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    int i;

    if (argc != 3) {
        print_usage();
        return -1;
    }

    info->cmd_info.crashdump_disc.addr = info->addr;
    (void)sscanf(argv[0], "%hhx", &info->cmd_info.crashdump_disc.subopcode);
    (void)sscanf(argv[1], "%hhx", &info->cmd_info.crashdump_disc.param0);
    (void)sscanf(argv[2], "%hx",  &info->cmd_info.crashdump_disc.param1);
    info->msg_len = sizeof(struct peci_crashdump_disc_msg);
    if (ioctl(fd, PECI_IOC_CRASHDUMP_DISC, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_CRASHDUMP_DISC failed\n");
        return -1;
    } else {
        printf("=== CrashdumpDisc Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x\n", info->cmd_info.crashdump_disc.cc);
        printf("Data: ");
        for (i = 0; i < 8; i++) {
            printf("%02x ", info->cmd_info.crashdump_disc.data[i]);
        }
        printf("\n");
    }

    return 0;
}

/**
 * Get crash dump frame from PECI device
 *
 * @param fd - File descriptor for PECI device
 * @param info - PECI tool information structure
 * @param argc - Number of arguments (should be 3)
 * @param argv - Argument list: <param0> <param1> <param2>
 * @return 0 on success, negative error code on failure
 */
static int do_peci_crashdump_get_frame(int fd, peci_tool_info_t *info, int argc, char *argv[])
{
    int i;

    if (argc != 3) {
        print_usage();
        return -1;
    }

    info->cmd_info.crashdump_get_frame.addr = info->addr;
    (void)sscanf(argv[0], "%hx",  &info->cmd_info.crashdump_get_frame.param0);
    (void)sscanf(argv[1], "%hx",  &info->cmd_info.crashdump_get_frame.param1);
    (void)sscanf(argv[2], "%hx",  &info->cmd_info.crashdump_get_frame.param2);
    info->cmd_info.crashdump_get_frame.rx_len = 16;
    info->msg_len = sizeof(struct peci_crashdump_get_frame_msg);

    if (ioctl(fd, PECI_IOC_CRASHDUMP_GET_FRAME, info) < 0) {
        fprintf(stderr, "ioctl PECI_IOC_CRASHDUMP_GET_FRAME failed\n");
        return -1;
    } else {
        printf("=== CrashdumpGetFrame Result ===\n");
        printf("Return: %d.\n", info->cmd_ret);
        printf("CC: 0x%02x\n", info->cmd_info.crashdump_get_frame.cc);
        printf("Data: ");
        for (i = 0; i < info->cmd_info.crashdump_get_frame.rx_len; i++) {
            printf("%02x ", info->cmd_info.crashdump_get_frame.data[i]);
        }
        printf("\n");
    }

    return 0;
}

/**
 * PECI function pointer type for command handlers
 */
typedef int (* peci_func_f)(int, peci_tool_info_t *, int, char **);

/**
 * PECI function table entry structure
 */
typedef struct peci_func_table_s {
    char *cmd;        /**< Command name */
    peci_func_f func; /**< Command handler function */
} peci_func_table_t;

/**
 * PECI command function table mapping command names to handlers
 */
static peci_func_table_t peci_func_table[] = {
    {"xfer", do_peci_xfer},
    {"ping", do_peci_ping},
    {"get_dib", do_peci_get_dib},
    {"get_temp", do_peci_get_temp},
    {"rd_pkg_cfg", do_peci_rd_pkg_cfg},
    {"wr_pkg_cfg", do_peci_wr_pkg_cfg},
    {"rd_ia_msr", do_peci_rd_ia_msr},
    {"wr_ia_msr", do_peci_wr_ia_msr},
    {"rd_ia_msrex", do_peci_rd_ia_msrex},
    {"rd_pci_cfg", do_peci_rd_pci_cfg},
    {"wr_pci_cfg", do_peci_wr_pci_cfg},
    {"rd_pci_cfg_local", do_peci_rd_pci_cfg_local},
    {"wr_pci_cfg_local", do_peci_wr_pci_cfg_local},
    {"rd_end_pt_cfg", do_peci_rd_end_pt_cfg},
    {"wr_end_pt_cfg", do_peci_wr_end_pt_cfg},
    {"crashdump_disc", do_peci_crashdump_disc},
    {"crashdump_get_frame", do_peci_crashdump_get_frame},
};

static int peci_addr_valid(uint32_t addr)
{
    if (addr >= PECI_BASE_ADDR && addr < PECI_BASE_ADDR + PECI_OFFSET_MAX) {
        return 1;
    }

    return 0;
}

/**
 * Process PECI command and execute corresponding handler
 *
 * @param argc - Number of command line arguments
 * @param argv - Command line arguments
 * @return 0 on success, negative error code on failure
 */
int do_peci_cmd(int argc, char *argv[])
{
    int fd;
    int ret;
    int sub_argc;
    char *cmd;
    char **sub_argv;
    peci_tool_info_t info;
    int i, size, found;

    if (argc < 4) { /* peci_tool cmd_str device_addr ...*/
        print_usage();
        return -1;
    }

    memset(&info, 0, sizeof(info));
    info.controller_id = 0; /* default 0 */
    info.cmd_ret = -1;      /* default error */

    (void)sscanf(argv[3], "%hhx", (unsigned char *)(&info.addr));
    if (peci_addr_valid(info.addr) == 0) {
        fprintf(stderr, "Invalid PECI address, must be 0x30~0x37\n");
        return -1;
    }
    cmd = argv[2];

    fd = open(FULL_PECI_TOOL_PATH, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open device\n");
        return -1;
    }

    ret = -1;
    size =  sizeof(peci_func_table) / sizeof(peci_func_table[0]);
    found = 0;
    for (i = 0; i < size; i++) {
        if (strcmp(cmd, peci_func_table[i].cmd) == 0) {
            found = 1;
            /*
             * If argc == 4, there are no extra arguments for the command, 
             * so pass NULL as argv.Otherwise, 
             * pass the pointer to the extra arguments starting from argv[4].
             */
            sub_argc = argc - 4;
            if (sub_argc == 0) {
                sub_argv = NULL;
            } else {
                sub_argv = &argv[4];
            }
            ret = peci_func_table[i].func(fd, &info, sub_argc, sub_argv);
            break;
        }
    }
    close(fd);

    if (found == 0) {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        print_usage();
        return -1;
    }

    return ret;
}