/*
 * wb_lpc_bmc.c
 * ko to register bmc misc dev to rd/wr bmc reg
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/kprobes.h>

#include "wb_lpc_bmc.h"
#include <wb_bsp_kernel_debug.h>
#include <wb_logic_dev_common.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

int lpc_addr_mode = 0;
module_param(lpc_addr_mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(lpc_addr_mode, "addr base of lpc [0: 0x4e, 1: 0x2e]");

static uint8_t lpc_addr_port = LPC_ADDR_PORT_4E;
static uint8_t lpc_data_port = LPC_DATA_PORT_4F;

typedef struct wb_lpc_bmc_s {
    struct kobject kobj;
    struct attribute_group *sysfs_group;
    struct miscdevice bmc_reg_misc;
    struct mutex update_lock;
    struct miscdevice bmc_master_flash;
    struct miscdevice bmc_slave_flash;
    int bmc_type;
    uint8_t flash_rw_enable;
    uint8_t flash_erase_full;
    uint32_t bmc_id;
    uint32_t flash_size;
    uint8_t flash_wr_buf[FLASH_WR_MAX_LEN];
    flash_info_t *master_flash_info;
    flash_info_t *slave_flash_info;
} wb_lpc_bmc_t;

static wb_lpc_bmc_t *g_wb_lpc_bmc;
static bmc_id_list_t g_bmc_id_list[] = {
    {
        .bmc_ids = {AST2500_A0, AST2510_A0, AST2520_A0, AST2530_A0, AST2500_A1, AST2510_A1, AST2520_A1,\
                     AST2530_A1, AST2500_A2, AST2510_A2, AST2520_A2, AST2530_A2},
        .bmc_type = TYPE_AST2500,
        .bmc_id_reg = AST25_SILICON_REVISION_ID_REGISTER,
    },
    {
        .bmc_ids = {AST2600_A0, AST2600_A1, AST2600_A2, AST2600_A3, AST2620_A1, AST2620_A2, AST2620_A3},
        .bmc_type = TYPE_AST2600,
        .bmc_id_reg = AST26_SILICON_REVISION_ID_REGISTER,
    }
};

static void write_addr_port(uint8_t addr_val, uint16_t addr_port)
{
    outb(addr_val, addr_port);

    return;
}

static void write_data_port(uint8_t val, uint16_t data_port)
{
    outb(val, data_port);

    return;
}

static uint8_t read_data_port(uint16_t data_port)
{
    return inb(data_port);
}

static void write_ilpc2ahb_addr(uint32_t addr)
{
    int i;

    for (i = 0; i < 4; i++) {
        write_addr_port(SUPERIO_REG0 + i, lpc_addr_port);
        write_data_port((addr >> (8 * (3 - i))) & MASK, lpc_data_port);
    }

    return;
}

static void write_ilpc2ahb_data(uint32_t data)
{
    int i;

    for (i = 0; i < 4; i++) {
        write_addr_port(SUPERIO_REG4 + i, lpc_addr_port);
        write_data_port((data >> (8 * (3 - i))) & MASK, lpc_data_port);
    }

    return;
}

static uint32_t read_ilpc2ahb_data(void)
{
    int i, tmp;
    uint32_t res;

    res = 0;
    for (i = 0; i < 4; i++) {
        write_addr_port(SUPERIO_REG4 + i, lpc_addr_port);
        tmp = read_data_port(lpc_data_port);
        res |= (tmp << (8 * (3 - i)));
    }

    return res;
}

static void trigger_ilpc2ahb_read(void)
{
    write_addr_port(SUPERIO_FE, lpc_addr_port);
    read_data_port(lpc_data_port);

    return;
}

static void trigger_ilpc2ahb_write(void)
{
    write_addr_port(SUPERIO_FE, lpc_addr_port);
    write_data_port(TOGGLE_WRITE, lpc_data_port);

    return;
}

static void check_data_length(void)
{
    uint8_t tmp;
    /* Data length check, 4 bytes */
    write_addr_port(SUPERIO_REG8, lpc_addr_port);
    tmp = read_data_port(lpc_data_port);
    if (tmp != SUPERIO_A2) {
        write_data_port(SUPERIO_A2, lpc_data_port);
    }

    return;
}

static void enable_ilpc2ahb(void)
{
    /* Write 0xAA then write 0xA5 twice to enable super IO*/
    write_addr_port(DISABLE_LPC, lpc_addr_port);
    write_addr_port(ENABLE_LPC, lpc_addr_port);
    write_addr_port(ENABLE_LPC, lpc_addr_port);

    /* Enable iLPC2AHB */
    write_addr_port(SUPERIO_07, lpc_addr_port);
    write_data_port(LPC_TO_AHB, lpc_data_port);
    write_addr_port(SUPERIO_30, lpc_addr_port);
    write_data_port(ENABLE_LPC_TO_AHB, lpc_data_port);

    /* Data length */
    check_data_length();

    return;
}

static void disable_ilpc2ahb(void)
{
    /* disable ilpc2ahb */
    write_addr_port(SUPERIO_30, lpc_addr_port);
    write_data_port(DISABLE_LPC_TO_AHB, lpc_data_port);
    /* disable super IO */
    write_addr_port(DISABLE_LPC, lpc_addr_port);

    return;
}

static uint32_t read_bmc_reg(uint32_t addr)
{
    uint32_t res;

    write_ilpc2ahb_addr(addr);
    trigger_ilpc2ahb_read();
    res = read_ilpc2ahb_data();

    return le32_to_cpu(res);
}

static void write_bmc_reg(uint32_t addr, uint32_t val)
{
    write_ilpc2ahb_addr(addr);
    write_ilpc2ahb_data(cpu_to_le32(val));
    trigger_ilpc2ahb_write();

    return;
}

static uint32_t read_bmc_flash_data(void)
{
    uint32_t res;

    trigger_ilpc2ahb_read();
    res = read_ilpc2ahb_data();

    return res;
}

static void write_bmc_flash_data(uint32_t data)
{
    write_ilpc2ahb_data(data);
    trigger_ilpc2ahb_write();

    return;
}

static void write_bmc_flash_addr(uint32_t addr)
{
    int i;

    for (i = 0; i < 4; i++) {
        write_addr_port(SUPERIO_REG4 + i, lpc_addr_port);
        write_data_port((addr >> (8 * i)) & MASK, lpc_data_port);
    }

    trigger_ilpc2ahb_write();

    return;
}

/* Configure the iLPC2AHB bus in 1,2, and 4-byte mode */
static void enable_bytes(int byte)
{
    write_addr_port(SUPERIO_REG8, lpc_addr_port);
    switch (byte) {
    case BYTE1:
        write_data_port(SUPERIO_A0 + BYTE1_VAL, lpc_data_port);
        break;
    case BYTE2:
        write_data_port(SUPERIO_A0 + BYTE2_VAL, lpc_data_port);
        break;
    case BYTE4:
        write_data_port(SUPERIO_A0 + BYTE4_VAL, lpc_data_port);
        break;
    default:
        write_data_port(SUPERIO_A0 + BYTE_RESERVED, lpc_data_port);
        break;
    }

    return;
}

/* Gated cs */
static void pull_ce_down(flash_info_t* info)
{
    write_bmc_reg(info->ce_control_reg, USER_MODE_PULL_CE_DOWN);

    return;
}

/* Release cs */
static void pull_ce_up(flash_info_t* info)
{
    write_bmc_reg(info->ce_control_reg, USER_MODE_PULL_CE_UP);

    return;
}

/* Send the flash operation command word */
static void send_cmd(uint32_t flash_base_addr, int cmd)
{
    write_ilpc2ahb_addr(flash_base_addr); /* Write flash address */
    enable_bytes(1);    /* The flash command word is only 1 byte. You need to configure the AHB to be in 1 byte mode */
    write_addr_port(SUPERIO_REG7, lpc_addr_port);
    write_data_port(cmd & MASK, lpc_data_port); /* Write the flash command word */
    trigger_ilpc2ahb_write();
    enable_bytes(4);

    return;
}

/*
 * The flash command word is sent in user mode
 * @info: flashStructure pointer
 * @cmd: flash Command word
*/
static void send_cmd_to_flash(flash_info_t* info, int cmd)
{
    pull_ce_down(info);
    send_cmd(info->flash_base_addr, cmd);
    pull_ce_up(info);

    return;
}

static int get_bmc_type(wb_lpc_bmc_t *wb_lpc_bmc)
{
    int i, j, size_1, size_2;
    uint32_t read_val;

    enable_ilpc2ahb();

    size_1 = sizeof(g_bmc_id_list) / sizeof((g_bmc_id_list)[0]);
    for (i = 0; i < size_1 ; i++) {
        read_val = read_bmc_reg(g_bmc_id_list[i].bmc_id_reg);
        DEBUG_VERBOSE("get_bmc_type: read reg: 0x%x\n", g_bmc_id_list[i].bmc_id_reg);
        DEBUG_VERBOSE("get_bmc_type: reg value 0x%x\n", read_val);
        size_2 = sizeof(g_bmc_id_list[i].bmc_ids) / sizeof((g_bmc_id_list[i].bmc_ids)[0]);
        for (j = 0; j < size_2; j++) {
            if ((read_val != 0) && (read_val == g_bmc_id_list[i].bmc_ids[j])) {
                wb_lpc_bmc->bmc_type = g_bmc_id_list[i].bmc_type;
                wb_lpc_bmc->bmc_id = read_val;
                DEBUG_VERBOSE("get_bmc_type: success, bmc_type = %d\n", wb_lpc_bmc->bmc_type);
                disable_ilpc2ahb();
                return 0;
            }
        }
    }

    disable_ilpc2ahb();
    return -ENXIO;
}

/* Enable CPU */
static void enable_cpu(void)
{
    if (g_wb_lpc_bmc->bmc_type == TYPE_AST2600) {
        /* unlock SCU register */
        write_bmc_reg(AST26_PROTECTION_KEY_REGISTER_1, UNLOCK_SCU_KEY);
        write_bmc_reg(AST26_PROTECTION_KEY_REGISTER_2, UNLOCK_SCU_KEY);
        /* enable ARM */
        write_bmc_reg(AST26_HARDWARE_STRAP_REGISTER_CLEAR, 0x1);
        /* lock SCU register */
        write_bmc_reg(AST26_PROTECTION_KEY_REGISTER_1, LOCK_SCU_KEY);
        write_bmc_reg(AST26_PROTECTION_KEY_REGISTER_2, LOCK_SCU_KEY);
    } else {
        /* unlock SCU register */
        write_bmc_reg(SCU_ADDR, UNLOCK_SCU_KEY);
        /* enable ARM */
        write_bmc_reg(AST25_REBOOT_CPU_REGISTER, SET_BMC_CPU_BOOT);
        /* lock SCU register */
        write_bmc_reg(SCU_ADDR, LOCK_SCU_KEY);
    }
    return;
}

/* diasble CPU */
static void disable_cpu(void)
{
    uint32_t scu_hw_strap_val;

    if (g_wb_lpc_bmc->bmc_type == TYPE_AST2600) {
        /* unlock SCU register */
        write_bmc_reg(AST26_PROTECTION_KEY_REGISTER_1, UNLOCK_SCU_KEY);
        write_bmc_reg(AST26_PROTECTION_KEY_REGISTER_2, UNLOCK_SCU_KEY);
        /* disable ARM */
        scu_hw_strap_val = read_bmc_reg(AST26_HARDWARE_STRAP_REGISTER);
        write_bmc_reg(AST26_HARDWARE_STRAP_REGISTER, scu_hw_strap_val |0x01);
        /* lock SCU register */
        write_bmc_reg(AST26_PROTECTION_KEY_REGISTER_1, LOCK_SCU_KEY);
        write_bmc_reg(AST26_PROTECTION_KEY_REGISTER_2, LOCK_SCU_KEY);
    } else {
        /* unlock SCU register */
        write_bmc_reg(SCU_ADDR, UNLOCK_SCU_KEY);
        /* disable ARM */
        scu_hw_strap_val = read_bmc_reg(AST25_HARDWARE_STRAP_REGISTER);
        write_bmc_reg(AST25_HARDWARE_STRAP_REGISTER, scu_hw_strap_val |0x01);
        /* lock SCU register */
        write_bmc_reg(SCU_ADDR, LOCK_SCU_KEY);
    }
    return;
}

/* Enable the BMC upgrade path */
static void enable_upgrade(void)
{
    /* Open iLPC to AHB bridge */
    enable_ilpc2ahb();
    /* diasble CPU */
    disable_cpu();
    /* init CE control register */
    write_bmc_reg(CE0_CONTROL_REGISTER, 0);
    write_bmc_reg(CE1_CONTROL_REGISTER, 0);
    /* disable WDT2 */
    if (g_wb_lpc_bmc->bmc_type == TYPE_AST2600) {
        write_bmc_reg(FMC_WDT2_CONTROL_STATUS_REGISTER, AST26_FMC_WATCHDOG_DISABLE);
        write_bmc_reg(AST26_WATCHDOG2_CONTROL, DISABLE_AST26_WATCHDOG);
    } else {
        write_bmc_reg(WATCHDOG2_CONTROL, DISABLE_WATCHDOG);
    }

    return;
}

/* disable bmc upgrade */
static void disable_upgrade(void)
{
    enable_cpu();
    if (g_wb_lpc_bmc->bmc_type == TYPE_AST2600) {
        DEBUG_VERBOSE( "DEBUG 0x%x\n", read_bmc_reg(AST26_HARDWARE_STRAP_REGISTER));
    } else {
        DEBUG_VERBOSE( "DEBUG 0x%x\n", read_bmc_reg(AST25_HARDWARE_STRAP_REGISTER));
    }
    disable_ilpc2ahb();

    return;
}

/* According to the bit1 of the WDT2 status register, 0: the main BMC starts, 1: the BMC starts */ 
static int get_current_bmc(void)
{
    if (g_wb_lpc_bmc->bmc_type == TYPE_AST2600) {
        return (read_bmc_reg(FMC_WDT2_CONTROL_STATUS_REGISTER) & 0x010) >> 4;
    } else {
        return (read_bmc_reg(WATCHDOG2_TSR) & 0x02) >> 1;
    }
}

#if 0
/* Enable WatchDog to reset 2600 BMC*/
static void enable_ast26_watchdog(int cs) {

    if (cs == get_current_bmc()) {
        /* reset to current chip by wdt2*/
        write_bmc_reg(AST26_WATCHDOG2_CLEAR_STATUS, CLEAR_WATCHDOG_STATUS);
        write_bmc_reg(AST26_WATCHDOG2_RESET_FUN_MASK_2, AST26_WATCHDOG_GATEMASK_2);
        write_bmc_reg(AST26_WATCHDOG2_RELOAD_VALUE, WATCHDOG_NEW_COUNT);
        write_bmc_reg(AST26_WATCHDOG2_COUNTER_RST, WATCHDOG_RELOAD_COUNTER);
        write_bmc_reg(AST26_WATCHDOG2_CONTROL, AST26_ENABLE_WATCH_CMD);
    } else {
        /* reset to another chip by fmt_wdt*/
        write_bmc_reg(FMC_WDT2_TIMER_RELOAD_VALUE_REGISTER, AST26_FMC_WATCHDOG_NEW_COUNT);
        write_bmc_reg(FMC_WDT2_CONTROL_STATUS_REGISTER, AST26_ENABLE_FMC_WATCH_CMD);
    }

    return;
}

#endif

/* Enable WatchDog to reset 2600 BMC*/
/* 2600 BMC WDT2 full chip reset is invalid , so use this funtion to reset bmc by fmc_wdt */
static void enable_ast26_watchdog(int cs) {
    DEBUG_VERBOSE( "enable_ast26_watchdog cs: %d\n", cs);
    write_bmc_reg(FMC_WDT2_TIMER_RELOAD_VALUE_REGISTER, AST26_FMC_WATCHDOG_NEW_COUNT);
    write_bmc_reg(FMC_WDT2_CONTROL_STATUS_REGISTER, AST26_ENABLE_FMC_WATCH_CMD);
    return;
}

/* Enable WatchDog to reset 2500 BMC*/
static void enable_ast25_watchdog(int cs)
{
    uint32_t enable_watch_cmd;

    enable_watch_cmd = (cs == CE0) ? ENABLE_WATCHDOG : ENABLE_WATCHDOG | BOOT_DEFAULT_MASK;
    write_bmc_reg(WATCHDOG2_CLEAR_STATUS, CLEAR_WATCHDOG_STATUS);
    write_bmc_reg(WATCHDOG2_RESET_FUN_MASK, WATCHDOG_GATEMASK);
    write_bmc_reg(WATCHDOG2_RELOAD_VALUE, WATCHDOG_NEW_COUNT);
    write_bmc_reg(WATCHDOG2_COUNTER_RST, WATCHDOG_RELOAD_COUNTER);
    write_bmc_reg(WATCHDOG2_CONTROL, enable_watch_cmd);

    return;
}

static void bmc_reboot(int cs)
{
    if (g_wb_lpc_bmc->bmc_type == TYPE_AST2600) {
        enable_ast26_watchdog(cs);
    } else {
        enable_ast25_watchdog(cs);
    } 
    DEBUG_VERBOSE("Upgrade-Complete, BMC rebooting...\n");

    return;
}

static void get_flash_base_and_ce_ctrl(int current_bmc, int cs, uint32_t *flash_base_addr, uint32_t *ce_ctrl_addr)
{
    uint32_t ce0_addr_range_reg_val, ce0_decode_addr;
    uint32_t ce1_addr_range_reg_val, ce1_decode_addr;

    /* Address Decoding Range Register get flash base addr */
    ce0_addr_range_reg_val = read_bmc_reg(CE0_ADDRESS_RANGE_REGISTER);
    ce1_addr_range_reg_val = read_bmc_reg(CE1_ADDRESS_RANGE_REGISTER);
    if (g_wb_lpc_bmc->bmc_type == TYPE_AST2600) {
        ce0_decode_addr = AST26_SEGMENT_ADDR_START(ce0_addr_range_reg_val);
        ce1_decode_addr = AST26_SEGMENT_ADDR_START(ce1_addr_range_reg_val);
    } else {
        ce0_decode_addr = AST25_SEGMENT_ADDR_START(ce0_addr_range_reg_val);
        ce1_decode_addr = AST25_SEGMENT_ADDR_START(ce1_addr_range_reg_val);
    }
    DEBUG_VERBOSE("CE0 addr decode range reg value:0x%08x, decode addr:0x%08x.\n",
        ce0_addr_range_reg_val, ce0_decode_addr);
    DEBUG_VERBOSE("CE1 addr decode range reg value:0x%08x, decode addr:0x%08x.\n",
        ce1_addr_range_reg_val, ce1_decode_addr);

    /* The current BMC uses CE0, and the other BMC uses CE1 */
    if (((current_bmc == CURRENT_MASTER) && (cs ==CE0)) || ((current_bmc == CURRENT_SLAVE) && (cs ==CE1))) {
        *ce_ctrl_addr = CE0_CONTROL_REGISTER;
        *flash_base_addr = ce0_decode_addr;
    } else {
        *ce_ctrl_addr = CE1_CONTROL_REGISTER;
        *flash_base_addr = ce1_decode_addr;
    }

    return;
}

static void init_fmc_register(void) {
    write_bmc_reg(WRITE_ADDR_FILTER_REGISTER, WRITE_ADDR_FILTER_ENABLE);
    write_bmc_reg(FMC_BASE_ADDR, BASE_SET);
    write_bmc_reg(WRITE_ADDR_FILTER_REGISTER, WRITE_ADDR_FILTER_DISABLE);
    write_bmc_reg(CE_CONTROL_REGISTER, CE_CONTROL_SET);
}

static void enable_flash_write_before_id(unsigned int ce_base, unsigned int addr)
{
    write_bmc_reg(COMMAND_CONTROL_REGISTER, DISABLE_ADDR_DATA_LINE);
    write_bmc_reg(ce_base, WRITE_ENABLE);
    write_bmc_reg(addr, DECODE_DATA);
    write_bmc_reg(COMMAND_CONTROL_REGISTER, ENABLE_ADDR_DATA_LINE);
}

/* read flash ID */
static int get_flash_id(uint32_t flash_base_addr, uint32_t ce_ctrl_addr)
{
    uint32_t origin_flash_id, flash_id;

    enable_flash_write_before_id(ce_ctrl_addr, flash_base_addr);
    usleep_range(FLASH_WEL_SLEEP_TIME, FLASH_WEL_SLEEP_TIME+1); /* wait previous command finished, just incase */
    init_fmc_register();
    write_bmc_reg(ce_ctrl_addr, USER_MODE_PULL_CE_DOWN);
    send_cmd(flash_base_addr, READID);
    origin_flash_id = read_bmc_flash_data();
    write_bmc_reg(ce_ctrl_addr, USER_MODE_PULL_CE_UP);
    flash_id = origin_flash_id & 0xFFFFFF;
    DEBUG_VERBOSE("origin flash id:0x%x, flash id:0x%x\n", origin_flash_id, flash_id);

    return flash_id;
}

/* Get flash status */
static uint8_t get_flash_status(flash_info_t* info)
{
    uint8_t flash_status;

    pull_ce_down(info);
    /* Send the read status command word */
    send_cmd(info->flash_base_addr, READ_FLASH_STATUS);
    /* Read status */
    flash_status = read_bmc_flash_data() & MASK;
    pull_ce_up(info);

    DEBUG_VERBOSE("get_flash_status:0x%x\n", flash_status);
    return flash_status;
}

/* Send the read flash status command to check whether the write function is enabled */
static int check_flash_write_enable(flash_info_t* info)
{
    uint8_t flash_status;
    int i, count;

    count = FLASH_WEL_TIMEOUT / FLASH_WEL_SLEEP_TIME;
    for (i = 0; i <= count; i++) {
        flash_status = get_flash_status(info);
        if ((flash_status & FLASH_WRITE_ENABLE_MASK) != FLASH_WRITE_ENABLE_MASK) {
            usleep_range(FLASH_WEL_SLEEP_TIME, FLASH_WEL_SLEEP_TIME+1);
        } else {
            DEBUG_VERBOSE("Check flash WEL success, RDSR:0x%x\n", flash_status);
            return 0;
        }
    }
    DEBUG_ERROR("Check flash WEL timeout, RDSR:0x%x\n", flash_status);
    return -ETIME;
}

/* Send the read flash status command to check whether the erase/write operation is complete */
static int check_flash_write_process(flash_info_t* info, int timeout, int sleep_time)
{
    int i, count;
    uint8_t flash_status;

    count = timeout / sleep_time;
    for (i = 0; i <= count; i++) {
        flash_status = get_flash_status(info);
        if ((flash_status & FLASH_WIP_MASK) != 0) {
            usleep_range(sleep_time, sleep_time+1);
        } else {
            DEBUG_VERBOSE("Check flash WIP success, RDSR:0x%x\n", flash_status);
            return 0;
        }
    }
    DEBUG_ERROR("Check flash WIP timeout, RDSR:0x%x.\n", flash_status);
    return -ETIME;
}

/* The command for enabling flash write was sent */
static int flash_write_enable(flash_info_t* info)
{
    int ret;

    send_cmd_to_flash(info, WRITE_ENABLE_FLASH);
    ret = check_flash_write_enable(info);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

/* The block erase command is sent */
static void send_block_erase_cmd(flash_info_t* info, uint32_t block_addr)
{
    pull_ce_down(info);
    send_cmd(info->flash_base_addr, info->erase_block_command);
    write_bmc_flash_addr(block_addr); /* Erase Block addr */
    pull_ce_up(info);

    return;
}

/* Write the bmc flash on page */
static int write_bmc_flash_page(flash_info_t* info, uint32_t page_addr, uint8_t *p, int len)
{
    int pos;

    if (len % 4) {
        DEBUG_ERROR("Page size %d invalid.\n", len);
        return -EINVAL;
    }
    /* Send the Page program command */
    pos = 0;
    pull_ce_down(info);
    send_cmd(info->flash_base_addr, info->page_program);
    write_bmc_flash_addr(page_addr); /* page address */
    while (len) {
        write_bmc_flash_data((*(uint32_t *)(p + pos))); /* Write 4 bytes at a time */
        pos += 4;
        len -= 4;
    }
    pull_ce_up(info);

    return 0;
}

static int erase_chip_block(flash_info_t* info, uint32_t offset, uint32_t count)
{
    uint32_t start_addr, end_addr;
    int ret;

    DEBUG_VERBOSE("Block erasing...\n");
    start_addr = info->flash_base_addr + offset;
    end_addr = start_addr + count;
    while (1) {
        /* Enable write */
        ret = flash_write_enable(info);
        if(ret < 0) {
            DEBUG_ERROR("Block erase, enable flash write error, block addr:0x%x\n", start_addr);
            return ret;
        }

        /* The block erase command is sent */
        send_block_erase_cmd(info, start_addr);
        /* Erase Block(64KB) MAX time 650ms*/
        ret = check_flash_write_process(info, BLOCK_ERASE_TIMEOUT, BLOCK_ERASE_SLEEP_TIME);
        if (ret < 0) {
            DEBUG_ERROR("Block erase, check write status error, block addr:0x%x\n", start_addr);
            return ret;
        }
        DEBUG_VERBOSE("\r0x%x", start_addr);
        start_addr += info->block_size;
        if (start_addr >= end_addr) {
            DEBUG_VERBOSE("\r\nErase Finish\n");
            DEBUG_VERBOSE("=========================================\n");
            break;
        }
    }
    return 0;
}

/* Send the whole chip erase command */
static void send_chip_erase_cmd(flash_info_t* info)
{
    send_cmd_to_flash(info, CHIP_ERASE_FLASH);

    return;
}

static int erase_chip_full(flash_info_t* info)
{
    int ret;

    if (info->full_erase == 0) {
        DEBUG_ERROR("Flash not support full erase function.\n");
        return -EOPNOTSUPP;
    }

    ret = flash_write_enable(info);
    if(ret < 0) {
        DEBUG_ERROR("Chip erase, enable flash write error.\n");
        return ret;
    }

    /* Send the Chip Erase command */
    DEBUG_VERBOSE("Full chip erasing, please wait...\n");
    send_chip_erase_cmd(info);
    ret = check_flash_write_process(info, CHIP_ERASE_TIMEOUT, CHIP_ERASE_SLEEP_TIME);
    if (ret < 0) {
        DEBUG_VERBOSE("Chip erase timeout.\n");
        return ret;
    }
    DEBUG_VERBOSE("Erase Finish\n");
    return 0;
}
static int program_chip(uint32_t count, uint8_t *p, flash_info_t* info, uint32_t offset)
{

    uint32_t start_addr, end_addr;
    int ret, page_size;

    start_addr = info->flash_base_addr + offset;
    page_size = info->page_size;
    end_addr = start_addr + count;

    DEBUG_VERBOSE("Programming...\n");

    while (1) {
        /* Write enable */
        ret = flash_write_enable(info);
        if(ret < 0) {
            DEBUG_ERROR("Page program, enable flash write error, page addr:0x%x\n", start_addr);
            return ret;
        }

        ret = write_bmc_flash_page(info, start_addr, p, page_size);
        if (ret < 0) {
             DEBUG_ERROR("Page program, write bmc flash page error, page addr:0x%x\n", start_addr);
             return ret;
         }
        /* page program MAX time 1.5ms */
        ret = check_flash_write_process(info, PAGE_PROGRAM_TIMEOUT, PAGE_PROGRAM_SLEEP_TIME);
        if (ret < 0) {
            DEBUG_ERROR("Page program, check write status error, page addr:0x%x\n", start_addr);
            return ret;
        }
        start_addr += page_size;
        p += page_size;
        if ((start_addr % 0x10000) == 0) {
            DEBUG_VERBOSE("\r0x%x", start_addr);
        }

        if (start_addr >= end_addr) {
            DEBUG_VERBOSE("\nProgram Finish\n");
            DEBUG_VERBOSE("=========================================\n");
            break;
        }
    } /* End of while (1) */
    return 0;
}

static int check_chip(uint32_t count, uint8_t *p, flash_info_t* info, uint32_t offset)
{

    uint32_t start_addr, rd_val, end_addr;
    int pos;

    start_addr = info->flash_base_addr  + offset;
    end_addr = start_addr + count;
    pos = 0;

    /* Checking */
    DEBUG_VERBOSE("Checking...\n");

    /* Send the READ flash command */
    pull_ce_down(info);
    send_cmd(info->flash_base_addr, COMMON_FLASH_READ); /* flash read command */
    write_bmc_flash_addr(start_addr); /* Start IP address of the flash read */
    while (1) {
        if (start_addr >= end_addr) {
            break;
        }
        rd_val = read_bmc_flash_data();
        if (rd_val != (*(uint32_t *)(p + pos))) {
            DEBUG_ERROR("Check Error at 0x%08x\n", start_addr);
            DEBUG_ERROR("READ:0x%08x VALUE:0x%08x\n", rd_val, (*(uint32_t *)(p + pos)));
            pull_ce_up(info);
            return -ENODATA;
        }
        if ((start_addr % 0x10000) == 0) {
            DEBUG_VERBOSE("\r0x%x ", start_addr);
        }
        start_addr += 4;
        pos += 4;
    }
    pull_ce_up(info);
    DEBUG_VERBOSE("\r\nFlash Checked\n");
    DEBUG_VERBOSE("=========================================\n");

    return 0;
}

flash_info_t* get_flash_info(int current_bmc, int cs)
{
    int i, size;
    uint32_t flash_base_addr, ce_ctrl_addr, flash_id;

    get_flash_base_and_ce_ctrl(current_bmc, cs, &flash_base_addr, &ce_ctrl_addr);

    size = (sizeof(flash_info) / sizeof((flash_info)[0]));

    flash_id = get_flash_id(flash_base_addr, ce_ctrl_addr);
    for (i = 0; i < size; i++) {
        if (flash_info[i].flash_id == flash_id) {
            flash_info[i].flash_base_addr = flash_base_addr;
            flash_info[i].ce_control_reg = ce_ctrl_addr;
            flash_info[i].cs = cs;
            return &flash_info[i];
        }
    }
    DEBUG_VERBOSE("Cannot get flash info, cs:%d, flash base addr:0x%x, ce control addr:0x%x, flash_id:0x%x.\n",
        cs, flash_base_addr, ce_ctrl_addr, flash_id);
    return NULL;
}

static void init_flash(flash_info_t* info)
{
    send_cmd_to_flash(info, RSTEN);
    send_cmd_to_flash(info, RST);
    send_cmd_to_flash(info, EXIT_OTP);
    send_cmd_to_flash(info, ENABLE_BYTE4);

    return;
}

static int read_single_bmc_flash(flash_info_t* info, uint32_t start_addr, int read_size, uint32_t *buf)
{
    uint32_t flash_start_addr, flash_end_addr;
    int ret;

    flash_start_addr = info->flash_base_addr + start_addr;
    flash_end_addr = flash_start_addr + read_size;
    ret = 0;

    DEBUG_VERBOSE("* CE%d FLASH TYPE: SPI FLASH\n", info->cs);
    DEBUG_VERBOSE("* FLASH NAME: %s\n", info->flash_name);
    DEBUG_VERBOSE("* Read flash addr:0x%x, size:0x%x\n", flash_start_addr, read_size);
    DEBUG_VERBOSE("=========================================\n");
    DEBUG_VERBOSE("Reading...\n");

    /* The flash read command was sent */
    pull_ce_down(info);
    send_cmd(info->flash_base_addr, COMMON_FLASH_READ); /* flash read command */
    write_bmc_flash_addr(flash_start_addr);
    while (1) {
        if (flash_start_addr >= flash_end_addr) {
            break;
        }
        *buf = read_bmc_flash_data();
        buf++;

        flash_start_addr += 4;
    }
    DEBUG_VERBOSE("\r\nRead Finish\n");
    DEBUG_VERBOSE("=========================================\n");
    pull_ce_up(info);
    return ret;
}

static ssize_t bmc_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int i;
    uint32_t *read_val;
    u8 buf_tmp[DEV_RDWR_MAX_LEN];

    if (count == 0) {
        DEBUG_ERROR("Invalid params, read count is 0.\n");
        return -EINVAL;
    }

    if (count > DEV_RDWR_MAX_LEN) {
        DEBUG_VERBOSE("read count %ld exceed max %d.\n", count, DEV_RDWR_MAX_LEN);
        count = sizeof(buf_tmp);
    }

    if (((*offset % DEV_WIDTH) != 0) || ((count % DEV_WIDTH) != 0)) {
        DEBUG_ERROR("Params invalid, start_addr:0x%llx, read_size:%ld\n", *offset, count);
        DEBUG_ERROR("Please input address/length times of 4\n");
        return -EINVAL;
    }

    if (count > DEV_REG_MAX_LEN - *offset) {
        DEBUG_VERBOSE("read count out of range. input len:%lu, read len:0x%llx.\n",
            count, DEV_REG_MAX_LEN - *offset);
        count = DEV_REG_MAX_LEN - *offset;
    }

    mem_clear(buf_tmp, sizeof(buf_tmp));

    mutex_lock(&g_wb_lpc_bmc->update_lock);
    enable_ilpc2ahb();

    read_val = (uint32_t *)buf_tmp;
    for (i = 0; i < count; i += DEV_WIDTH) {
        *read_val = read_bmc_reg(*offset + i);
        DEBUG_VERBOSE("bmc reg read, offset: 0x%llx, value: 0x%08x\n", *offset, *read_val);
        read_val++;
    }

    disable_ilpc2ahb();
    mutex_unlock(&g_wb_lpc_bmc->update_lock);

    if (access_ok(buf, count)) {
        DEBUG_VERBOSE("user space read, buf: %p, offset: 0x%llx, read count %ld.\n",
            buf, *offset, count);
        if (copy_to_user(buf, buf_tmp, count)) {
            DEBUG_ERROR("copy_to_user failed.\n");
            return -EFAULT;
        }
    } else {
        DEBUG_VERBOSE("kernel space read, buf: %p, offset: 0x%llx, read count %ld.\n",
            buf, *offset, count);
        memcpy(buf, buf_tmp, count);
    }
    *offset += count;

    return count;
}

static ssize_t bmc_dev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    int ret;

    DEBUG_VERBOSE("bmc_dev_read_iter, file: %p, count: %ld, offset: %lld\n",
        iocb->ki_filp, to->count, iocb->ki_pos);
    ret = bmc_dev_read(iocb->ki_filp, to->kvec->iov_base, to->count, &iocb->ki_pos);
    return ret;
}

static ssize_t read_bmc_flash(struct file *file, char __user *buf, size_t count, loff_t *offset, int cs)
{
    int ret, current_bmc;
    u8 buf_tmp[DEV_RDWR_MAX_LEN];
    flash_info_t* info;

    if (g_wb_lpc_bmc->flash_rw_enable <= 0) {
        DEBUG_ERROR("flash rw not enable , can not read flash.\n");
        return -EPERM;
    }

    if (count == 0) {
        DEBUG_ERROR("Invalid params, read count is 0.\n");
        return -EINVAL;
    }

    if (count > sizeof(buf_tmp)) {
        DEBUG_VERBOSE("read count %ld exceed max %d.\n", count, DEV_RDWR_MAX_LEN);
        count = sizeof(buf_tmp);
    }

    if (((*offset % DEV_WIDTH) != 0) || ((count % DEV_WIDTH) != 0)) {
        DEBUG_ERROR("Params invalid, start_addr:0x%llx, read_size:%ld\n", *offset, count);
        DEBUG_ERROR("Please input address/length times of 4\n");
        return -EINVAL;
    }

    mutex_lock(&g_wb_lpc_bmc->update_lock);

    current_bmc = get_current_bmc();
    DEBUG_VERBOSE("* Current Bmc : %d\n", current_bmc);

    info = get_flash_info(current_bmc, cs);
    if(info == NULL) {
        DEBUG_ERROR("get flash info fail.\n");
        ret = -ENODEV;
        goto err;
    }

    if (count > info->flash_size - *offset) {
        DEBUG_VERBOSE("read count out of range. input len:%lu, read len:0x%llx.\n",
            count, info->flash_size - *offset);
        count = info->flash_size - *offset;
    }

    init_flash(info);

    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = read_single_bmc_flash(info, *offset, count, (uint32_t *)buf_tmp);
    if (ret < 0) {
        DEBUG_VERBOSE("Read %s bmc flash failed.\n", cs == 0 ? "master" : "slave");
        goto err;
    }

    mutex_unlock(&g_wb_lpc_bmc->update_lock);

    if (access_ok(buf, count)) {
        DEBUG_VERBOSE("user space read, buf: %p, offset: 0x%llx, read count %ld.\n",
            buf, *offset, count);
        if (copy_to_user(buf, buf_tmp, count)) {
            DEBUG_ERROR("copy_to_user failed.\n");
            return -EFAULT;
        }
    } else {
        DEBUG_VERBOSE("kernel space read, buf: %p, offset: 0x%llx, read count %ld.\n",
            buf, *offset, count);
        memcpy(buf, buf_tmp, count);
    }

    *offset += count;
    return count;
err:
    mutex_unlock(&g_wb_lpc_bmc->update_lock);
    return ret;
}

static ssize_t bmc_master_flash_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    int ret;

    DEBUG_VERBOSE("bmc_master_flash_read_iter, file: %p, count: %ld, offset: %lld\n",
        iocb->ki_filp, to->count, iocb->ki_pos);
    ret = read_bmc_flash(iocb->ki_filp, to->kvec->iov_base, to->count, &iocb->ki_pos, CURRENT_MASTER);
    return ret;
}

static ssize_t bmc_slave_flash_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    int ret;

    DEBUG_VERBOSE("bmc_slave_flash_read_iter, file: %p, count: %ld, offset: %lld\n",
        iocb->ki_filp, to->count, iocb->ki_pos);
    ret = read_bmc_flash(iocb->ki_filp, to->kvec->iov_base, to->count, &iocb->ki_pos, CURRENT_SLAVE);
    return ret;
}

static ssize_t bmc_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    int i;
    u8 buf_tmp[DEV_RDWR_MAX_LEN];
    uint32_t *write_val;

    if (count == 0) {
        DEBUG_ERROR("Invalid params, write count is 0.\n");
        return -EINVAL;
    }

    if (count > DEV_RDWR_MAX_LEN) {
        DEBUG_VERBOSE("write count %lu exceed max %ld.\n", count, sizeof(buf_tmp));
        count = sizeof(buf_tmp);
    }

    if (((*offset % DEV_WIDTH) != 0)) {
        DEBUG_ERROR("Params invalid, write addr:0x%llx, write_size:%ld\n", *offset, count);
        DEBUG_ERROR("Please input address times of 4\n");
        return -EINVAL;
    }

    if (count > DEV_REG_MAX_LEN - *offset) {
        DEBUG_VERBOSE("write count out of range. input len:%lu, write len:0x%llx.\n",
            count, DEV_REG_MAX_LEN - *offset);
        count = DEV_REG_MAX_LEN - *offset;
    }

    mem_clear(buf_tmp, sizeof(buf_tmp));
    if (access_ok(buf, count)) {
        DEBUG_VERBOSE("user space write, buf: %p, offset: 0x%llx, write count %ld.\n",
            buf, *offset, count);
        if (copy_from_user(buf_tmp, buf, count)) {
            DEBUG_ERROR("copy_from_user failed.\n");
            return -EFAULT;
        }
    } else {
        DEBUG_VERBOSE("kernel space write, buf: %p, offset: 0x%llx, write count %ld.\n",
            buf, *offset, count);
        memcpy(buf_tmp, buf, count);
    }

    mutex_lock(&g_wb_lpc_bmc->update_lock);
    enable_ilpc2ahb();
    write_val = (uint32_t *)buf_tmp;
     for (i = 0; i < count; i += DEV_WIDTH) {
        write_bmc_reg(*offset + i, *write_val);
        write_val++;
     }

    disable_ilpc2ahb();
    mutex_unlock(&g_wb_lpc_bmc->update_lock);

    *offset += count;
    return count;
}

static ssize_t bmc_dev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    int ret = 0;

    DEBUG_VERBOSE("bmc_dev_write_iter, file: %p, count: %ld, offset: %lld\n",
        iocb->ki_filp, from->count, iocb->ki_pos);
    ret = bmc_dev_write(iocb->ki_filp, from->kvec->iov_base, from->count, &iocb->ki_pos);
    return ret;
}
static ssize_t write_bmc_flash(struct file *file, char __user *buf, size_t count, loff_t *offset, int cs)
{
    int ret, current_bmc;
    flash_info_t* info;
    size_t bytes_remaining = count;
    size_t chunk_size;
    size_t buf_offset = 0;

    if (g_wb_lpc_bmc->flash_rw_enable <= 0) {
        DEBUG_ERROR("flash rw not enable , can not write flash.\n");
        return -EPERM;
    }

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, write failed.\n");
        return -EINVAL;
    }

    if (count == 0) {
        DEBUG_ERROR("Invalid params, write count is 0.\n");
        return -EINVAL;
    }

    mutex_lock(&g_wb_lpc_bmc->update_lock);

    current_bmc = get_current_bmc();
    DEBUG_VERBOSE("* Current Bmc : %d\n", current_bmc);

    info = get_flash_info(current_bmc, cs);
    if(info == NULL) {
        DEBUG_ERROR("get flash info fail.\n");
        ret = -ENODEV;
        goto err;
    }

    if (*offset >= info->flash_size) {
        DEBUG_VERBOSE("write offset 0x%llx exceed max %d.\n", *offset, info->flash_size);
        ret = -EINVAL;
        goto err;
    }

    if (*offset % info->block_size) {
        DEBUG_ERROR("offset: 0x%llx not times of block_size: 0x%x.\n", *offset, info->block_size);
        ret = -EINVAL;
        goto err;
    }

    if (info->page_size > FLASH_WR_MAX_LEN) {
        DEBUG_ERROR("flash page_size: 0x%x exceed max support write len: 0x%x.\n", info->page_size, FLASH_WR_MAX_LEN);
        ret = -EINVAL;
        goto err;
    }

    init_flash(info);

    /* if flash_erase_full set and offset is 0, full erase flash */
    if ((g_wb_lpc_bmc->flash_erase_full == 1) && (*offset == 0)) {
        ret = erase_chip_full(info);
    } else {
        ret = erase_chip_block(info, *offset, count);
    }
    if (ret < 0) {
        DEBUG_ERROR("Erase flash Error\n");
        goto err;
    }

    while (bytes_remaining > 0) {
        chunk_size = (bytes_remaining > info->page_size) ? info->page_size : bytes_remaining;

        mem_clear(g_wb_lpc_bmc->flash_wr_buf, sizeof(g_wb_lpc_bmc->flash_wr_buf));
        if (access_ok(buf + buf_offset, chunk_size)) {
            DEBUG_VERBOSE("user space write, buf: %p, offset: 0x%llx, write count %ld.\n",
                buf + buf_offset, *offset, chunk_size);
            if (copy_from_user(g_wb_lpc_bmc->flash_wr_buf, buf + buf_offset, chunk_size)) {
                DEBUG_ERROR("copy_from_user failed.\n");
                ret = -EFAULT;
                goto err;
            }
        } else {
            DEBUG_VERBOSE("kernel space write, buf: %p, offset: 0x%llx, write count %ld.\n",
                buf + buf_offset, *offset, chunk_size);
            memcpy(g_wb_lpc_bmc->flash_wr_buf, buf + buf_offset, chunk_size);
        }

        ret = program_chip(chunk_size, g_wb_lpc_bmc->flash_wr_buf, info, *offset);
        if(ret < 0) {
            DEBUG_ERROR("Program Chip Error\n");
            goto err;
        }

        ret = check_chip(chunk_size, g_wb_lpc_bmc->flash_wr_buf, info, *offset);
        if(ret < 0) {
            DEBUG_ERROR("Check Chip Error\n");
            goto err;
        }

        *offset += chunk_size;
        buf_offset += chunk_size;
        bytes_remaining -= chunk_size;
    }

    mutex_unlock(&g_wb_lpc_bmc->update_lock);
    return count;
err:
    mutex_unlock(&g_wb_lpc_bmc->update_lock);
    return ret;
}

static ssize_t bmc_master_flash_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    int ret = 0;

    DEBUG_VERBOSE("bmc_master_flash_write_iter, file: %p, count: %ld, offset: %lld\n",
        iocb->ki_filp, from->count, iocb->ki_pos);
    ret = write_bmc_flash(iocb->ki_filp, from->kvec->iov_base, from->count, &iocb->ki_pos, CURRENT_MASTER);
    return ret;
}

static ssize_t bmc_slave_flash_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    int ret = 0;

    DEBUG_VERBOSE("bmc_slave_flash_write_iter, file: %p, count: %ld, offset: %lld\n",
        iocb->ki_filp, from->count, iocb->ki_pos);
    ret = write_bmc_flash(iocb->ki_filp, from->kvec->iov_base, from->count, &iocb->ki_pos, CURRENT_SLAVE);
    return ret;
}

static loff_t bmc_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            DEBUG_ERROR("SEEK_SET, offset:%lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > DEV_REG_MAX_LEN) {
            DEBUG_ERROR("SEEK_SET out of range, offset:%lld, bmc_len:0x%x.\n",
                offset, DEV_REG_MAX_LEN);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) >= DEV_REG_MAX_LEN) || ((file->f_pos + offset) < 0)) {
            DEBUG_ERROR("SEEK_CUR out of range, f_ops:%lld, offset:%lld., bmc_len:0x%x.\n",
                file->f_pos, offset, DEV_REG_MAX_LEN);
            ret = - EINVAL;
            break;
        }
        file->f_pos += offset;
        ret = file->f_pos;
        break;
    default:
        DEBUG_ERROR("unsupport llseek type:%d.\n", origin);
        ret = -EINVAL;
        break;
    }
    return ret;
}

static loff_t bmc_flash_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;
    flash_info_t* info;

    if (g_wb_lpc_bmc->flash_size == 0) {
        mutex_lock(&g_wb_lpc_bmc->update_lock);
        if (g_wb_lpc_bmc->flash_rw_enable == 0) {
            enable_upgrade();
        }

        info = get_flash_info(CURRENT_MASTER, CURRENT_MASTER);
        if(info== NULL) {
            DEBUG_ERROR("get bmc flash info failed.\n");
            if (g_wb_lpc_bmc->flash_rw_enable == 0) {
                disable_upgrade();
            }
            mutex_unlock(&g_wb_lpc_bmc->update_lock);
            return -ENODEV;
        }

        g_wb_lpc_bmc->flash_size = info->flash_size;

        if (g_wb_lpc_bmc->flash_rw_enable == 0) {
            disable_upgrade();
        }
        mutex_unlock(&g_wb_lpc_bmc->update_lock);
    }

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            DEBUG_ERROR("SEEK_SET, offset:%lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > DEV_REG_MAX_LEN) {
            DEBUG_ERROR("SEEK_SET out of range, offset:%lld, bmc_len:0x%x.\n",
                offset, DEV_REG_MAX_LEN);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) >= g_wb_lpc_bmc->flash_size) || ((file->f_pos + offset) < 0)) {
            DEBUG_ERROR("SEEK_CUR out of range, f_ops:%lld, offset:%lld., bmc_len:0x%x.\n",
                file->f_pos, offset, DEV_REG_MAX_LEN);
            ret = - EINVAL;
            break;
        }
        file->f_pos += offset;
        ret = file->f_pos;
        break;
    default:
        DEBUG_ERROR("unsupport llseek type:%d.\n", origin);
        ret = -EINVAL;
        break;
    }
    return ret;
}

static long bmc_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static const struct file_operations bmc_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = bmc_dev_llseek,
    .read_iter  = bmc_dev_read_iter,
    .write_iter = bmc_dev_write_iter,
    .unlocked_ioctl = bmc_dev_ioctl,
};

static const struct file_operations bmc_master_flash_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = bmc_flash_llseek,
    .read_iter  = bmc_master_flash_read_iter,
    .write_iter = bmc_master_flash_write_iter,
    .unlocked_ioctl = bmc_dev_ioctl,
};

static const struct file_operations bmc_slave_flash_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = bmc_flash_llseek,
    .read_iter  = bmc_slave_flash_read_iter,
    .write_iter = bmc_slave_flash_write_iter,
    .unlocked_ioctl = bmc_dev_ioctl,
};

static ssize_t lpc_bmc_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->show) {
        return -ENOSYS;
    }

    return attribute->show(kobj, attribute, buf);
}

static ssize_t lpc_bmc_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf,
                   size_t len)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->store) {
        return -ENOSYS;
    }

    return attribute->store(kobj, attribute, buf, len);
}

static const struct sysfs_ops lpc_bmc_sysfs_ops = {
    .show = lpc_bmc_attr_show,
    .store = lpc_bmc_attr_store,
};

static void lpc_bmc_obj_release(struct kobject *kobj)
{
    return;
}

static struct kobj_type lpc_bmc_ktype = {
    .sysfs_ops = &lpc_bmc_sysfs_ops,
    .release = lpc_bmc_obj_release,
};

static ssize_t bmc_id_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    memset(buf, 0, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%08x\n", g_wb_lpc_bmc->bmc_id);
}

static ssize_t flash_rw_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    memset(buf, 0, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%d\n", g_wb_lpc_bmc->flash_rw_enable);
}

static ssize_t flash_rw_enable_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    u8 val;
    int ret;

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }

    if ((val != 0) && (val != 1)) {
        DEBUG_ERROR("input value [%s] not equal 0 or 1\n", buf);
        return -EINVAL;
    }

    mutex_lock(&g_wb_lpc_bmc->update_lock);
    if (val > 0) {
        enable_upgrade();
        g_wb_lpc_bmc->flash_rw_enable = 1;
    } else {
        disable_upgrade();
        g_wb_lpc_bmc->flash_rw_enable = 0;
    }
    mutex_unlock(&g_wb_lpc_bmc->update_lock);

    return count;
}

static ssize_t flash_info_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret, offset, current_bmc;
    ssize_t buf_len;

    if ((g_wb_lpc_bmc->master_flash_info == NULL) || (g_wb_lpc_bmc->slave_flash_info == NULL)) {
        mutex_lock(&g_wb_lpc_bmc->update_lock);
        if (g_wb_lpc_bmc->flash_rw_enable == 0) {
            enable_upgrade();
        }

        current_bmc = get_current_bmc();
        DEBUG_VERBOSE("* Current Bmc : %d\n", current_bmc);

        g_wb_lpc_bmc->master_flash_info = get_flash_info(current_bmc, CURRENT_MASTER);
        if(g_wb_lpc_bmc->master_flash_info == NULL) {
            DEBUG_ERROR("get master bmc flash info failed.\n");
            ret = -ENODEV;
            goto err;
        }

        g_wb_lpc_bmc->slave_flash_info = get_flash_info(current_bmc, CURRENT_SLAVE);
        if(g_wb_lpc_bmc->slave_flash_info == NULL) {
            DEBUG_ERROR("get slave bmc flash info failed.\n");
            ret = -ENODEV;
            goto err;
        }

        if (g_wb_lpc_bmc->flash_rw_enable == 0) {
            disable_upgrade();
        }
        mutex_unlock(&g_wb_lpc_bmc->update_lock);
    }

    memset(buf, 0, PAGE_SIZE);
    offset = 0;
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%-23s: %s\n", "----------------------", "-----------------------");
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%-23s: %s\n", "master_flash_name", g_wb_lpc_bmc->master_flash_info->flash_name);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%-23s: 0x%x\n", "master_flash_id", g_wb_lpc_bmc->master_flash_info->flash_id);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%-23s: %d\n", "master_flash_type", g_wb_lpc_bmc->master_flash_info->flash_type);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%-23s: 0x%x\n", "master_flash_page_size", g_wb_lpc_bmc->master_flash_info->page_size);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%-23s: %s\n", "----------------------", "-----------------------");
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%-23s: %s\n", "slave_flash_name", g_wb_lpc_bmc->slave_flash_info->flash_name);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%-23s: 0x%x\n", "slave_flash_id", g_wb_lpc_bmc->slave_flash_info->flash_id);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%-23s: %d\n", "slave_flash_type", g_wb_lpc_bmc->slave_flash_info->flash_type);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%-23s: 0x%x\n", "slave_flash_page_size", g_wb_lpc_bmc->slave_flash_info->page_size);
    buf_len = strlen(buf);
    return buf_len;
err:
    if (g_wb_lpc_bmc->flash_rw_enable == 0) {
        disable_upgrade();
    }
    mutex_unlock(&g_wb_lpc_bmc->update_lock);
    return ret;
}

static ssize_t reset_bmc_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    u8 cs;
    int ret;

    cs = 0;
    ret = kstrtou8(buf, 0, &cs);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }

    if ((cs != 0) && (cs != 1)) {
        DEBUG_ERROR("input value [%s] not equal 0 or 1\n", buf);
        return -EINVAL;
    }

    if (g_wb_lpc_bmc->flash_rw_enable <= 0) {
        DEBUG_ERROR("flash rw not enable , can not reset bmc.\n");
        return -EPERM;
    }

    mutex_lock(&g_wb_lpc_bmc->update_lock);
    bmc_reboot(cs);
    mutex_unlock(&g_wb_lpc_bmc->update_lock);

    return count;
}

static ssize_t flash_erase_full_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    memset(buf, 0, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%d\n", g_wb_lpc_bmc->flash_erase_full);
}

static ssize_t flash_erase_full_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    u8 val;
    int ret;

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }

    if ((val != 0) && (val != 1)) {
        DEBUG_ERROR("input value [%s] not equal 0 or 1\n", buf);
        return -EINVAL;
    }

    mutex_lock(&g_wb_lpc_bmc->update_lock);
    if (val > 0) {
        g_wb_lpc_bmc->flash_erase_full = 1;
    } else {
        g_wb_lpc_bmc->flash_erase_full = 0;
    }
    mutex_unlock(&g_wb_lpc_bmc->update_lock);

    return count;
}

static struct kobj_attribute flash_rw_enable_attribute = __ATTR(flash_rw_enable, S_IRUGO | S_IWUSR, flash_rw_enable_show, flash_rw_enable_store);
static struct kobj_attribute bmc_id_attribute = __ATTR(bmc_id, S_IRUGO, bmc_id_show, NULL);
static struct kobj_attribute flash_info_attribute = __ATTR(flash_info, S_IRUGO, flash_info_show, NULL);
static struct kobj_attribute reset_bmc_attribute = __ATTR(reset_bmc, S_IWUSR, NULL, reset_bmc_store);
static struct kobj_attribute flash_erase_full_attribute = __ATTR(flash_erase_full, S_IRUGO | S_IWUSR, flash_erase_full_show, flash_erase_full_store);

static struct attribute *lpc_bmc_attrs[] = {
    &bmc_id_attribute.attr,
    &flash_rw_enable_attribute.attr,
    &flash_info_attribute.attr,
    &reset_bmc_attribute.attr,
    &flash_erase_full_attribute.attr,
    NULL,
};

static struct attribute_group lpc_bmc_attr_group = {
    .attrs = lpc_bmc_attrs,
};

static int register_bmc_dev(wb_lpc_bmc_t *wb_lpc_bmc)
{
    int ret;
    struct miscdevice *bmc_reg_misc;
    struct miscdevice *bmc_master_flash_misc;
    struct miscdevice *bmc_slave_flash_misc;

    bmc_reg_misc = &wb_lpc_bmc->bmc_reg_misc;
    bmc_reg_misc->minor = MISC_DYNAMIC_MINOR;
    bmc_reg_misc->name = BMC_REG_DEV_NAME;
    bmc_reg_misc->fops = &bmc_dev_fops;
    bmc_reg_misc->mode = 0666;

    bmc_master_flash_misc = &wb_lpc_bmc->bmc_master_flash;
    bmc_master_flash_misc->minor = MISC_DYNAMIC_MINOR;
    bmc_master_flash_misc->name = BMC_MASTER_FLASH_NAME;
    bmc_master_flash_misc->fops = &bmc_master_flash_dev_fops;
    bmc_master_flash_misc->mode = 0666;

    bmc_slave_flash_misc = &wb_lpc_bmc->bmc_slave_flash;
    bmc_slave_flash_misc->minor = MISC_DYNAMIC_MINOR;
    bmc_slave_flash_misc->name = BMC_SLAVE_FLASH_NAME;
    bmc_slave_flash_misc->fops = &bmc_slave_flash_dev_fops;
    bmc_slave_flash_misc->mode = 0666;

    if (misc_register(bmc_reg_misc) != 0) {
        pr_err("Failed to register %s device.\n", bmc_reg_misc->name);
        ret = -ENXIO;
        return ret;
    }

    if (misc_register(bmc_master_flash_misc) != 0) {
        pr_err("Failed to register %s device.\n", bmc_master_flash_misc->name);
        ret = -ENXIO;
        goto unregister_bmc_reg_misc;
    }

    if (misc_register(bmc_slave_flash_misc) != 0) {
        pr_err("Failed to register %s device.\n", bmc_slave_flash_misc->name);
        ret = -ENXIO;
        goto unregister_master_flash_misc;
    }

    return 0;

unregister_master_flash_misc:
    misc_deregister(&wb_lpc_bmc->bmc_master_flash);
unregister_bmc_reg_misc:
    misc_deregister(&wb_lpc_bmc->bmc_reg_misc);
    return ret;
}

static int __init wb_lpc_bmc_init(void)
{
    int ret;

    g_wb_lpc_bmc = kzalloc(sizeof(*g_wb_lpc_bmc), GFP_KERNEL);
    if (!g_wb_lpc_bmc) {
        pr_err("init g_wb_lpc_bmc fail");
        return -ENOMEM;
    }

    switch (lpc_addr_mode) {
    case MODE_2E:
        lpc_addr_port = LPC_ADDR_PORT_2E;
        lpc_data_port = LPC_DATA_PORT_2F;
        break;
    case MODE_4E:
    default:
        lpc_addr_port = LPC_ADDR_PORT_4E;
        lpc_data_port = LPC_DATA_PORT_4F;
        break;
    }

    ret = get_bmc_type(g_wb_lpc_bmc);
    if (ret) {
        pr_err("get_bmc_type fail.\n");
        goto free_lpc_bmc;
    }

    /* creat parent dir by dev name in /sys/logic_dev */
    ret = kobject_init_and_add(&g_wb_lpc_bmc->kobj, &lpc_bmc_ktype, logic_dev_kobj, "%s", BMC_REG_DEV_NAME);
    if (ret) {
        kobject_put(&g_wb_lpc_bmc->kobj);
        pr_err("creat parent dir %s fail", BMC_REG_DEV_NAME);
        goto free_lpc_bmc;
    }

    g_wb_lpc_bmc->flash_rw_enable = 0;
    g_wb_lpc_bmc->flash_erase_full = 0;
    g_wb_lpc_bmc->flash_size = 0;
    g_wb_lpc_bmc->master_flash_info = NULL;
    g_wb_lpc_bmc->slave_flash_info = NULL;
    mutex_init(&g_wb_lpc_bmc->update_lock);

    g_wb_lpc_bmc->sysfs_group = &lpc_bmc_attr_group;
    ret = sysfs_create_group(&g_wb_lpc_bmc->kobj, g_wb_lpc_bmc->sysfs_group);
    if (ret) {
        pr_err("sysfs_create_group %s error", BMC_REG_DEV_NAME);
        goto remove_parent_kobj;
    }

    ret = register_bmc_dev(g_wb_lpc_bmc);
    if (ret) {
        pr_err("register_bmc_dev fail.\n");
        goto remove_sysfs_group;
    }

    pr_info("register lpc bmc dev success\n");
    return 0;

remove_sysfs_group:
    sysfs_remove_group(&g_wb_lpc_bmc->kobj, (const struct attribute_group *)g_wb_lpc_bmc->sysfs_group);
remove_parent_kobj:
    kobject_put(&g_wb_lpc_bmc->kobj);
free_lpc_bmc:
    kfree(g_wb_lpc_bmc);
    return ret;
}

static void __exit wb_lpc_bmc_exit(void)
{
    if (g_wb_lpc_bmc) {
        pr_info("unregister lpc bmc dev\n");
        misc_deregister(&g_wb_lpc_bmc->bmc_reg_misc);
        misc_deregister(&g_wb_lpc_bmc->bmc_slave_flash);
        misc_deregister(&g_wb_lpc_bmc->bmc_master_flash);
        sysfs_remove_group(&g_wb_lpc_bmc->kobj, (const struct attribute_group *)g_wb_lpc_bmc->sysfs_group);
        kobject_put(&g_wb_lpc_bmc->kobj);
        kfree(g_wb_lpc_bmc);
    }
}

module_init(wb_lpc_bmc_init);
module_exit(wb_lpc_bmc_exit);
MODULE_DESCRIPTION("lpc bmc driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
