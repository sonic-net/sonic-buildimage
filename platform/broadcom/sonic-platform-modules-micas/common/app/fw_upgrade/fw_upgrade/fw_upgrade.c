#include <sys/io.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "fw_upgrade.h"

static flash_info_t flash_info[] = {
    {
        .flash_name = "M25L6433F",
        .flash_size = M32,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = MX25L6433F,
        .block_size = STEP_64,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "S25FL512S",
        .flash_size = M64,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = S25FL512S,
        .block_size = STEP_256,
        .full_erase = 0,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "MX25l512",
        .flash_size = M64,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = MX25l512,
        .block_size = STEP_64,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "STM25P64",
        .flash_size = M12,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = STM25P64,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "STM25P128",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = STM25P128,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "N25Q256",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = N25Q256,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "N25Q512",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = N25Q512,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "W25X16",
        .flash_size = M3,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = W25X16,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "W25X64",
        .flash_size = M12,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = W25X64,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "W25Q64BV",
        .flash_size = M12,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = W25Q64BV,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "W25Q128BV",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = W25Q128BV,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "W25Q256FV",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = W25Q256FV,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "MX25L1605D",
        .flash_size = M32,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = MX25L1605D,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "MX25L12805D",
        .flash_size = M32,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = MX25L12805D,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "MX66L1G45G",
        .flash_size = M128,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = MX66L1G45G,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "GD25Q256",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = GD25Q256,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
};

static int debug_on;

static void help(void)
{
    printf("------------------------------BMC Upgrade Tool--------------------------------\n");
    printf("Program Flash:\n");
    printf("\tfw_upgrade upgrade [file name] [chip select: 0 | 1 | 2] ");
    printf("[erase type: full | block]\n");
    printf("\t[file name] if file is not located at /home/admin, path should be added\n");
    printf("\t[chip select] 0:master, 1:slave, 2:both\n");
    printf("\t[erase type] choose a way to erase chip, full erase would be faster\n");
    printf("Read BMC Reg:\n");
    printf("\tfw_upgrade rd [address] [length]\n");
    printf("\t[address(Hexadecimal)] register address of BMC\n");
    printf("\t[length(decimal)] length of read data, should be times of 4\n");

    return;
}

static int set_ioport_rw_access(void)
{

    if ( iopl(3) < 0) {
        printf("Can't get access to /dev/port \n");
        return -1;
    }

    return 0;
}

static int get_file_size(char *file_name)
{
    FILE * pFile;
    int size;

    pFile = fopen(file_name,"rb");
    if (pFile == NULL) {
        printf("Error opening file\n");
        return -1;
    }
    fseek (pFile, 0, SEEK_END);
    size = ftell(pFile);
    fclose (pFile);
    return size;
}

static uint8_t _read(uint16_t addr)
{
    return inb(addr);
}

static void _write(uint16_t addr, uint8_t val)
{
    outb(val, addr);

    return;
}

static void write_addr_port(uint8_t addr_val, uint16_t addr_port)
{
    _write(addr_port, addr_val);

    return;
}

static void write_data_port(uint8_t val, uint16_t data_port)
{
    _write(data_port, val);

    return;
}

static uint8_t read_data_port(uint16_t data_port)
{
    return _read(data_port);
}

static void write_ilpc2ahb_addr(uint32_t addr)
{
    int i;

    for (i = 0; i < 4; i++) {
        write_addr_port(SUPERIO_REG0 + i, LPC_ADDR_PORT);
        write_data_port((addr >> (8 * (3 - i))) & MASK, LPC_DATA_PORT);
    }

    return;
}

static void write_ilpc2ahb_data(uint32_t data)
{
    int i;

    for (i = 0; i < 4; i++) {
        write_addr_port(SUPERIO_REG4 + i, LPC_ADDR_PORT);
        write_data_port((data >> (8 * (3 - i))) & MASK, LPC_DATA_PORT);
    }

    return;
}

static uint32_t read_ilpc2ahb_data(void)
{
    int i, tmp;
    uint32_t res;

    res = 0;
    for (i = 0; i < 4; i++) {
        write_addr_port(SUPERIO_REG4 + i, LPC_ADDR_PORT);
        tmp = read_data_port(LPC_DATA_PORT);
        res |= (tmp << (8 * (3 - i)));
    }

    return res;
}

static void trigger_ilpc2ahb_read(void)
{
    write_addr_port(SUPERIO_FE, LPC_ADDR_PORT);
    read_data_port(LPC_DATA_PORT);

    return;
}

static void trigger_ilpc2ahb_write(void)
{
    write_addr_port(SUPERIO_FE, LPC_ADDR_PORT);
    write_data_port(TOGGLE_WRITE, LPC_DATA_PORT);

    return;
}

static uint32_t read_bmc_reg(uint32_t addr)
{
    uint32_t res;

    write_ilpc2ahb_addr(addr);
    trigger_ilpc2ahb_read();
    res = read_ilpc2ahb_data();

    return res;
}

static void write_bmc_reg(uint32_t addr, uint32_t val)
{
    write_ilpc2ahb_addr(addr);
    write_ilpc2ahb_data(val);
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
        write_addr_port(SUPERIO_REG4 + i, LPC_ADDR_PORT);
        write_data_port((addr >> (8 * i)) & MASK, LPC_DATA_PORT);
    }

    trigger_ilpc2ahb_write();

    return;
}

static void enable_bytes(int byte)
{
    write_addr_port(SUPERIO_REG8, LPC_ADDR_PORT);
    switch (byte) {
    case BYTE1:
        write_data_port(SUPERIO_A0 + BYTE1_VAL, LPC_DATA_PORT);
        break;
    case BYTE2:
        write_data_port(SUPERIO_A0 + BYTE2_VAL, LPC_DATA_PORT);
        break;
    case BYTE4:
        write_data_port(SUPERIO_A0 + BYTE4_VAL, LPC_DATA_PORT);
        break;
    default:
        write_data_port(SUPERIO_A0 + BYTE_RESERVED, LPC_DATA_PORT);
        break;
    }

    return;
}

static void pull_ce_down(flash_info_t* info)
{
    write_bmc_reg(info->ce_control_reg, USER_MODE_PULL_CE_DOWN);

    return;
}

static void pull_ce_up(flash_info_t* info)
{
    write_bmc_reg(info->ce_control_reg, USER_MODE_PULL_CE_UP);

    return;
}

static void send_cmd(uint32_t flash_base_addr, int cmd)
{
    write_ilpc2ahb_addr(flash_base_addr);
    enable_bytes(1);
    write_addr_port(SUPERIO_REG7, LPC_ADDR_PORT);
    write_data_port(cmd & MASK, LPC_DATA_PORT);
    trigger_ilpc2ahb_write();
    enable_bytes(4);

    return;
}

static void send_cmd_to_flash(flash_info_t* info, int cmd)
{
    pull_ce_down(info);
    send_cmd(info->flash_base_addr, cmd);
    pull_ce_up(info);

    return;
}

static void check_data_length(void)
{
    uint8_t tmp;
    /* Data length check, 4 bytes */
    write_addr_port(SUPERIO_REG8, LPC_ADDR_PORT);
    tmp = read_data_port(LPC_DATA_PORT);
    if (tmp != SUPERIO_A2) {
        write_data_port(SUPERIO_A2, LPC_DATA_PORT);
    }

    return;
}

static void enable_ilpc2ahb(void)
{
    /* Write 0xAA then write 0xA5 twice to enable super IO*/
    write_addr_port(DISABLE_LPC, LPC_ADDR_PORT);
    write_addr_port(ENABLE_LPC, LPC_ADDR_PORT);
    write_addr_port(ENABLE_LPC, LPC_ADDR_PORT);

    /* Enable iLPC2AHB */
    write_addr_port(SUPERIO_07, LPC_ADDR_PORT);
    write_data_port(LPC_TO_AHB, LPC_DATA_PORT);
    write_addr_port(SUPERIO_30, LPC_ADDR_PORT);
    write_data_port(ENABLE_LPC_TO_AHB, LPC_DATA_PORT);

    /* Data length */
    check_data_length();

    return;
}

static void disable_ilpc2ahb(void)
{
    /* disable ilpc2ahb */
    write_addr_port(SUPERIO_30, LPC_ADDR_PORT);
    write_data_port(DISABLE_LPC_TO_AHB, LPC_DATA_PORT);
    /* disable super IO */
    write_addr_port(DISABLE_LPC, LPC_ADDR_PORT);

    return;
}

/* Enable CPU */
static void enable_cpu(void)
{
    /* unlock SCU register */
    write_bmc_reg(SCU_ADDR, UNLOCK_SCU_KEY);
    /* enable ARM */
    write_bmc_reg(REBOOT_CPU_REGISTER, SET_BMC_CPU_BOOT);
    /* lock SCU register */
    write_bmc_reg(SCU_ADDR, LOCK_SCU_KEY);

    return;
}

/* diasble CPU */
static void disable_cpu(void)
{
    uint32_t scu_hw_strap_val;

    /* unlock SCU register */
    write_bmc_reg(SCU_ADDR, UNLOCK_SCU_KEY);
    /* disable ARM */
    scu_hw_strap_val = read_bmc_reg(HARDWARE_STRAP_REGISTER);
    write_bmc_reg(HARDWARE_STRAP_REGISTER, scu_hw_strap_val |0x01);
    /* lock SCU register */
    write_bmc_reg(SCU_ADDR, LOCK_SCU_KEY);

    return;
}

static void enable_upgrade(void)
{

    enable_ilpc2ahb();
    /* diasble CPU */
    disable_cpu();
    /* init CE control register */
    write_bmc_reg(CE0_CONTROL_REGISTER, 0);
    write_bmc_reg(CE1_CONTROL_REGISTER, 0);
    /* disable WDT2 */
    write_bmc_reg(WATCHDOG2_CONTROL, DISABLE_WATCHDOG);

    return;
}

static void disable_upgrade(void)
{
    enable_cpu();
    dbg_print(debug_on, "DEBUG 0x%x\n", read_bmc_reg(HARDWARE_STRAP_REGISTER));
    disable_ilpc2ahb();

    return;
}

static void watchdog_status_debug(void)
{
    uint32_t watchdog_reg;

    /* Watchdog Control Register */
    watchdog_reg = read_bmc_reg(WATCHDOG2_CONTROL);
    dbg_print(debug_on,"Watchdog Control Register: 0x%x\n", watchdog_reg);
    dbg_print(debug_on,"Watchdog Enable Signal: 0x%x\n", watchdog_reg & BIT1);
    dbg_print(debug_on,"Watchdog Reset SyS En: 0x%x\n", (watchdog_reg & BIT2) >> 1);
    dbg_print(debug_on,"Watchdog Reset Mode: 0x%x\n", (watchdog_reg & (BIT6 | BIT7)) >> 5);
    switch (watchdog_reg & (BIT6 | BIT7)) {
    case SOC_SYS:
        dbg_print(debug_on,"\tReset Mode En: SoC System\n");
        break;
    case FULL_CHIP:
        dbg_print(debug_on,"\tReset Mode En: Full Chip\n");
        break;
    case ARM_CPU:
        dbg_print(debug_on,"\tReset Mode En: ARM Cpu\n");
        break;
    default:
        break;
    }

    /* Watchdog Timeout Status Register */
    watchdog_reg = read_bmc_reg(WATCHDOG2_TSR);
    dbg_print(debug_on,"Watchdog Timeout Occur: 0x%x\n", watchdog_reg & BIT1);
    dbg_print(debug_on,"Watchdog Boot from: CD%d\n", watchdog_reg & BIT2);
    dbg_print(debug_on,"Watchdog Interrupt Occur: 0x%x\n", watchdog_reg & BIT3);

    return;
}

/* CE Type Setting Register */
static void ce_type_setting_debug(void)
{
    uint32_t fmc_reg;

    fmc_reg = read_bmc_reg(FMC_CE_TYPE_SETTING_REG);
    if ((fmc_reg & CE0_SPI_TYPE) == SPI) {
        dbg_print(debug_on,"CE0 Type Seeting: 0x%x, Type: SPI\n", fmc_reg & CE0_SPI_TYPE);
    } else {
        dbg_print(debug_on,"CE0 Type Seeting: 0x%x, Type: Unknown\n", fmc_reg & CE0_SPI_TYPE);
    }
    if (((fmc_reg & CE1_SPI_TYPE) >> BIT2) == SPI) {
        dbg_print(debug_on,"CE1 Type Seeting: 0x%x, Type: SPI\n", (fmc_reg & CE1_SPI_TYPE) >> BIT2);
    } else {
        dbg_print(debug_on,"CE1 Type Seeting: 0x%x, Type: Unknown\n", (fmc_reg & CE1_SPI_TYPE) >> BIT2);
    }

    return;
}
/* CE Control Register */
static void ce_control_debug(void)
{
    uint32_t fmc_reg;

    fmc_reg = read_bmc_reg(CE_CONTROL_REGISTER);
    dbg_print(debug_on,"CE0 Address Mode: 0x%x, Mode: %d Bytes\n",
        fmc_reg & BIT1, (fmc_reg & BIT1) + 3);
    dbg_print(debug_on,"CE1 Address Mode: 0x%x, Mode: %d Bytes\n",
        (fmc_reg & BIT2) >> 1, ((fmc_reg & BIT2) >> 1)  + 3);

    return;
}

/* Interrupt Control & Status Register */
static void irq_control_status_debug(void)
{
    uint32_t fmc_reg;

    fmc_reg = read_bmc_reg(INR_STATUS_CONTROL_REGISTER);
    dbg_print(debug_on,"SPI Write Address Protected Interrupt EN: 0x%x\n", fmc_reg & BIT2);
    dbg_print(debug_on,"SPI Command Abort Interrupt EN: 0x%x\n", fmc_reg & BIT3);
    dbg_print(debug_on,"SPI Write Address Protected Status: 0x%x, Status: %s\n",
        RIGHT_SHIFT_8(fmc_reg) & BIT2, (RIGHT_SHIFT_8(fmc_reg) & BIT2) == BIT2 ? "Occur" : "Normal");
    dbg_print(debug_on,"SPI Command Abort Status: 0x%x, Status: %s\n",
        RIGHT_SHIFT_8(fmc_reg) & BIT3, (RIGHT_SHIFT_8(fmc_reg) & BIT3) == BIT3 ? "Occur" : "Normal");
    /*Clear Abnormal Status*/
    if ((RIGHT_SHIFT_8(fmc_reg) & BIT3) || (RIGHT_SHIFT_8(fmc_reg) & BIT2)) {
        write_bmc_reg(INR_STATUS_CONTROL_REGISTER, CLEAR_INR_STATUS_CONTROL);
    }

    return;
}

/* Command Control Register */
static void command_control_debug(void)
{
    uint32_t fmc_reg;

    fmc_reg = read_bmc_reg(COMMAND_CONTROL_REGISTER);
    dbg_print(debug_on,"Data Byte Line 0: %s\n", ((fmc_reg & BIT4) != 0) ? "Disable" : "Enable");
    dbg_print(debug_on,"Data Byte Line 1: %s\n", ((fmc_reg & BIT3) != 0) ? "Disable" : "Enable");
    dbg_print(debug_on,"Data Byte Line 2: %s\n", ((fmc_reg & BIT2) != 0) ? "Disable" : "Enable");
    dbg_print(debug_on,"Data Byte Line 3: %s\n", ((fmc_reg & BIT1) != 0) ? "Disable" : "Enable");

    dbg_print(debug_on,"Address Byte Line 0: %s\n", ((fmc_reg & BIT8) != 0) ? "Disable" : "Enable");
    dbg_print(debug_on,"Address Byte Line 1: %s\n", ((fmc_reg & BIT7) != 0) ? "Disable" : "Enable");
    dbg_print(debug_on,"Address Byte Line 2: %s\n", ((fmc_reg & BIT6) != 0) ? "Disable" : "Enable");
    dbg_print(debug_on,"Address Byte Line 3: %s\n", ((fmc_reg & BIT5) != 0) ? "Disable" : "Enable");

    return;
}

static void ce_control_reg_debug(void)
{
    uint32_t fmc_reg;

    /* CE0 Control Register */
    fmc_reg = read_bmc_reg(CE0_CONTROL_REGISTER);
    switch (fmc_reg & (BIT1 | BIT2)){
    case NORMAL_READ:
        dbg_print(debug_on,"CE0 Command Mode: Normal Read\n");
        break;
    case READ_MODE:
        dbg_print(debug_on,"CE0 Command Mode: Read Command\n");
        break;
    case WRITE_MODE:
        dbg_print(debug_on,"CE0 Command Mode: Write Command\n");
        break;
    case USER_MODE:
        dbg_print(debug_on,"CE0 Command Mode: User Mode\n");
        break;
    default:
        break;
    }
    switch((RIGHT_SHIFT_24(fmc_reg) & (BIT5 | BIT6 | BIT7))){
    case 0:
        dbg_print(debug_on,"CE0 IO Mode: Single Mode\n");
        break;
    case 2:
    case 3:
        dbg_print(debug_on,"CE0 IO Mode: Dual Mode\n");
        break;
    default:
        break;
    }

    dbg_print(debug_on,"CE0 Inactive Pulse Width: %d HCLK\n",
        DEFAULT_WIDTH - (RIGHT_SHIFT_24(fmc_reg) & (BIT1 | BIT2 | BIT3 | BIT4)));
    dbg_print(debug_on,"CE0 Data Input Mode: %s Mode\n", (fmc_reg & BIT4) == 0 ? "Single" : "Dual");
    dbg_print(debug_on,"CE0 MSB | LSB: %s First\n", (fmc_reg & BIT6) == 0 ? "MSB" : "LSB");

    /* CE1 Control Register */
    fmc_reg = read_bmc_reg(CE1_CONTROL_REGISTER);
    switch (fmc_reg & (BIT1 | BIT2)){
    case NORMAL_READ:
        dbg_print(debug_on,"CE1 Command Mode: Normal Read\n");
        break;
    case READ_MODE:
        dbg_print(debug_on,"CE1 Command Mode: Read Command\n");
        break;
    case WRITE_MODE:
        dbg_print(debug_on,"CE1 Command Mode: Write Command\n");
        break;
    case USER_MODE:
        dbg_print(debug_on,"CE1 Command Mode: User Mode\n");
        break;
    default:
        break;
    }
    switch((RIGHT_SHIFT_24(fmc_reg) & (BIT5 | BIT6 | BIT7))){
    case 0:
        dbg_print(debug_on,"CE1 IO Mode: Single Mode\n");
        break;
    case 2:
    case 3:
        dbg_print(debug_on,"CE1 IO Mode: Dual Mode\n");
        break;
    default:
        break;
    }

    dbg_print(debug_on,"CE1 Inactive Pulse Width: %d HCLK\n",
        DEFAULT_WIDTH - (RIGHT_SHIFT_24(fmc_reg) & (BIT1 | BIT2 | BIT3 | BIT4)));
    dbg_print(debug_on,"CE1 Data Input Mode: %s Mode\n", (fmc_reg & BIT4) == 0 ? "Single" : "Dual");
    dbg_print(debug_on,"CE1 MSB | LSB: %s First\n", (fmc_reg & BIT6) == 0 ? "MSB" : "LSB");

    return;
}

static void fmc_debug(void)
{
    ce_type_setting_debug();
    ce_control_debug();
    irq_control_status_debug();
    command_control_debug();
    ce_control_reg_debug();

    return;
}

/* Enable WatchDog to reset BMC*/
static void enable_watchdog(int cs)
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
    enable_watchdog(cs);
    watchdog_status_debug();
    disable_upgrade();
    printf("Upgrade-Complete, BMC rebooting...\n");

    return;
}

static int get_current_bmc(void)
{
    return (read_bmc_reg(WATCHDOG2_TSR) & 0x02) >> 1;
}

static void get_flash_base_and_ce_ctrl(int current_bmc, int cs, uint32_t *flash_base_addr, uint32_t *ce_ctrl_addr)
{
    uint32_t ce0_addr_range_reg_val, ce0_decode_addr;
    uint32_t ce1_addr_range_reg_val, ce1_decode_addr;

    ce0_addr_range_reg_val = read_bmc_reg(CE0_ADDRESS_RANGE_REGISTER);
    ce0_decode_addr = SEGMENT_ADDR_START(ce0_addr_range_reg_val);
    ce1_addr_range_reg_val = read_bmc_reg(CE1_ADDRESS_RANGE_REGISTER);
    ce1_decode_addr = SEGMENT_ADDR_START(ce1_addr_range_reg_val);
    dbg_print(debug_on,"CE0 addr decode range reg value:0x%08x, decode addr:0x%08x.\n",
        ce0_addr_range_reg_val, ce0_decode_addr);
    dbg_print(debug_on,"CE1 addr decode range reg value:0x%08x, decode addr:0x%08x.\n",
        ce1_addr_range_reg_val, ce1_decode_addr);

    if (((current_bmc == CURRENT_MASTER) && (cs ==CE0)) || ((current_bmc == CURRENT_SLAVE) && (cs ==CE1))) {
        *ce_ctrl_addr = CE0_CONTROL_REGISTER;
        *flash_base_addr = ce0_decode_addr;
    } else {
        *ce_ctrl_addr = CE1_CONTROL_REGISTER;
        *flash_base_addr = ce1_decode_addr;
    }

    return;
}

static int get_flash_id(uint32_t flash_base_addr, uint32_t ce_ctrl_addr)
{
    uint32_t origin_flash_id, flash_id;

    write_bmc_reg(ce_ctrl_addr, USER_MODE_PULL_CE_DOWN);
    send_cmd(flash_base_addr, READID);
    origin_flash_id = read_bmc_flash_data();
    write_bmc_reg(ce_ctrl_addr, USER_MODE_PULL_CE_UP);
    flash_id = origin_flash_id & 0xFFFFFF;
    dbg_print(debug_on,"origin flash id:0x%x, flash id:0x%x\n", origin_flash_id, flash_id);

    return flash_id;
}

static uint8_t get_flash_status(flash_info_t* info)
{
    uint8_t flash_status;

    pull_ce_down(info);

    send_cmd(info->flash_base_addr, READ_FLASH_STATUS);

    flash_status = read_bmc_flash_data() & MASK;
    pull_ce_up(info);

    dbg_print(debug_on,"get_flash_status:0x%x\n", flash_status);
    return flash_status;
}

static int check_flash_write_enable(flash_info_t* info)
{
    uint8_t flash_status;
    int i, count;

    count = FLASH_WEL_TIMEOUT / FLASH_WEL_SLEEP_TIME;
    for (i = 0; i <= count; i++) {
        flash_status = get_flash_status(info);
        if ((flash_status & FLASH_WRITE_ENABLE_MASK) != FLASH_WRITE_ENABLE_MASK) {
            usleep(FLASH_WEL_SLEEP_TIME);
        } else {
            dbg_print(debug_on,"Check flash WEL success, RDSR:0x%x\n", flash_status);
            return 0;
        }
    }
    printf("Check flash WEL timeout, RDSR:0x%x\n", flash_status);
    return -1;
}

static int check_flash_write_process(flash_info_t* info, int timeout, int sleep_time)
{
    int i, count;
    uint8_t flash_status;

    count = timeout / sleep_time;
    for (i = 0; i <= count; i++) {
        flash_status = get_flash_status(info);
        if ((flash_status & FLASH_WIP_MASK) != 0) {
            usleep(sleep_time);
        } else {
            dbg_print(debug_on,"Check flash WIP success, RDSR:0x%x\n", flash_status);
            return 0;
        }
    }
    printf("Check flash WIP timeout, RDSR:0x%x.\n", flash_status);
    return -1;
}

static int flash_write_enable(flash_info_t* info)
{
    int ret;

    send_cmd_to_flash(info, WRITE_ENABLE_FLASH);
    ret = check_flash_write_enable(info);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

static void send_block_erase_cmd(flash_info_t* info, uint32_t block_addr)
{
    pull_ce_down(info);
    send_cmd(info->flash_base_addr, info->erase_block_command);
    write_bmc_flash_addr(block_addr); /* Erase Block addr */
    pull_ce_up(info);

    return;
}

static void send_chip_erase_cmd(flash_info_t* info)
{
    send_cmd_to_flash(info, CHIP_ERASE_FLASH);

    return;
}

static int write_bmc_flash_page(flash_info_t* info, uint32_t page_addr, uint8_t *p, int len)
{
    int pos;

    if (len % 4) {
        printf("Page size %d invalid.\n", len);
        return -1;
    }

    pos = 0;
    pull_ce_down(info);
    send_cmd(info->flash_base_addr, info->page_program);
    write_bmc_flash_addr(page_addr); /* page address */
    while (len) {
        write_bmc_flash_data((*(uint32_t *)(p + pos)));
        pos += 4;
        len -= 4;
    }
    pull_ce_up(info);

    return 0;
}

static int erase_chip_full(flash_info_t* info)
{
    time_t timep;
    int ret;

    if (info->full_erase == 0) {
        printf("Flash not support full erase function.\n");
        return -1;
    }

    ret = flash_write_enable(info);
    if(ret < 0) {
        printf("Chip erase, enable flash write error.\n");
        return -1;
    }

    time(&timep);
    printf("Full chip erasing, please wait...\n");
    dbg_print(debug_on,"Erase Start-%s\n",asctime(gmtime(&timep)));
    send_chip_erase_cmd(info);
    ret = check_flash_write_process(info, CHIP_ERASE_TIMEOUT, CHIP_ERASE_SLEEP_TIME);
    if (ret < 0) {
        printf("Chip erase timeout.\n");
        return -1;
    }
    time(&timep);
    dbg_print(debug_on,"Erase Finish-%s\n",asctime(gmtime(&timep)));
    printf("Erase Finish\n");
    printf("=========================================\n");
    return 0;
}

static int erase_chip_block(flash_info_t* info)
{
    uint32_t block_addr, end_addr;
    time_t timep;
    int ret;

    printf("Block erasing...\n");
    time (&timep);
    dbg_print(debug_on,"Erase-Start-%s\n", asctime(gmtime(&timep)));
    end_addr = info->flash_base_addr + info->flash_size;
    block_addr = info->flash_base_addr;
    while (1) {
        /* Enable write */
        ret = flash_write_enable(info);
        if(ret < 0) {
            printf("Block erase, enable flash write error, block addr:0x%x\n", block_addr);
            return -1;
        }

        send_block_erase_cmd(info, block_addr);
        /* Erase Block(64KB) MAX time 650ms*/
        ret = check_flash_write_process(info, BLOCK_ERASE_TIMEOUT, BLOCK_ERASE_SLEEP_TIME);
        if (ret < 0) {
            printf("Block erase, check write status error, block addr:0x%x\n", block_addr);
            return -1;
        }
        printf("\r0x%x", block_addr);
        fflush(stdout);
        if (block_addr >= end_addr) {
            time(&timep);
            printf("\r\nErase Finish\n");
            printf("=========================================\n");
            dbg_print(debug_on,"\nEnd-Earse-%s\n",asctime(gmtime(&timep)));
            break;
        }
        block_addr += info->block_size;
    }
    return 0;
}

static int program_chip(uint32_t file_size, uint8_t *p, flash_info_t* info)
{
    time_t timep;
    uint32_t page_addr, end_addr;
    int ret, page_size;

    page_addr = info->flash_base_addr;
    page_size = info->page_size;
    end_addr = file_size + info->flash_base_addr;
    time (&timep);
    printf("Programming...\n");
    dbg_print(debug_on,"Program Start-%s\n",asctime(gmtime(&timep)));
    /* Debug info */
    fmc_debug();
    while (1) {
        /* Write enable */
        ret = flash_write_enable(info);
        if(ret < 0) {
            printf("Page program, enable flash write error, page addr:0x%x\n", page_addr);
            return -1;
        }
        ret = write_bmc_flash_page(info, page_addr, p, page_size);
        if (ret < 0) {
             printf("Page program, write bmc flash page error, page addr:0x%x\n", page_addr);
             return -1;
         }
        /* page program MAX time 1.5ms */
        ret = check_flash_write_process(info, PAGE_PROGRAM_TIMEOUT, PAGE_PROGRAM_SLEEP_TIME);
        if (ret < 0) {
            printf("Page program, check write status error, page addr:0x%x\n", page_addr);
            return -1;
        }
        page_addr += page_size;
        p += page_size;
        if ((page_addr % 0x10000) == 0) {
            printf("\r0x%x", page_addr);
            fflush(stdout);
        }

        if (page_addr >= end_addr) {
            printf("\nProgram Finish\n");
            printf("=========================================\n");
            time(&timep);
            dbg_print(debug_on,"\nProgram-End-%s\n",asctime(gmtime(&timep)));
            break;
        }
    } /* End of while (1) */
    return 0;
}

static int check_chip(uint32_t file_size, uint8_t *p, flash_info_t* info)
{
    time_t timep;
    uint32_t offset_addr, rd_val, end_addr;
    int pos;

    offset_addr = info->flash_base_addr;
    end_addr = file_size + info->flash_base_addr;
    pos=0;
    /* Checking */
    time(&timep);
    printf("Checking...\n");
    dbg_print(debug_on,"Checking-Start-%s\n",asctime(gmtime(&timep)));

    pull_ce_down(info);
    send_cmd(info->flash_base_addr, COMMON_FLASH_READ);
    write_bmc_flash_addr(info->flash_base_addr);
    while (1) {
        if (offset_addr >= end_addr) {
            break;
        }
        rd_val = read_bmc_flash_data();
        if (rd_val != (*(uint32_t *)(p + pos))) {
            printf("Check Error at 0x%08x\n", offset_addr);
            printf("READ:0x%08x VALUE:0x%08x\n", rd_val, (*(uint32_t *)(p + pos)));
            pull_ce_up(info);
            return -1;
        }
        if ((offset_addr % 0x10000) == 0) {
            printf("\r0x%x ", offset_addr);
            fflush(stdout);
        }
        offset_addr += 4;
        pos += 4;
    }
    pull_ce_up(info);
    printf("\r\nFlash Checked\n");
    printf("=========================================\n");
    time(&timep);
    dbg_print(debug_on,"Checking-End-%s\n",asctime(gmtime(&timep)));
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
    printf("Cannot get flash info, cs:%d, flash base addr:0x%x, ce control addr:0x%x, flash_id:0x%x.\n",
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

static int upgrade_bmc_core(char *file_name, int erase_type, flash_info_t* info)
{
    int file_size, fp, ret;
    uint8_t *p;

    file_size = get_file_size(file_name);
    if (file_size < 0) {
        printf("file size %d Error\n", file_size);
        return -1;
    }

    fp = open(file_name, O_RDWR);
    if (fp < 0) {
        printf("Cannot open %s.\n", file_name);
        return -1;
    }

    p = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fp, 0);
    if (p == MAP_FAILED) {
        printf("Could not mmap %s, error(%s).\n", file_name, strerror(errno));
        close(fp);
        return -1;
    }

    printf("* CE%d FLASH TYPE: SPI FLASH\n", info->cs);
    printf("* FLASH NAME: %s\n", info->flash_name);
    printf("* File Size:%d, 0x%x\n", file_size, file_size);
    printf("=========================================\n");

    /* Select erase type */
    switch (erase_type) {
    case FULL_ERASE:
        ret = erase_chip_full(info);
        break;
    case BLOCK_ERASE:
        ret = erase_chip_block(info);
        break;
    default:
        printf("Unsupport earse type:%d\n", erase_type);
        goto exit;
        break;
    }

    if (ret < 0) {
        printf("Erase Chip Error\n");
        goto exit;
    }

    /* Program the flash */
    ret = program_chip(file_size, p, info);
    if(ret < 0) {
        printf("Program Chip Error\n");
        goto exit;
    }
    /* Check */
    ret = check_chip(file_size, p, info);
    if(ret < 0) {
        printf("Check Chip Error\n");
        goto exit;
    }

    munmap(p, file_size);
    close(fp);
    return 0;
exit:
    munmap(p, file_size);
    close(fp);
    return -1;
}

static int upgrade_bmc_flash(char *filename, int current_bmc, int cs, int erase_type)
{
    int ret;
    flash_info_t* info;

    info = get_flash_info(current_bmc, cs);
    if(info == NULL) {
        return -1;
    }

    init_flash(info);

    ret = upgrade_bmc_core(filename, erase_type, info);

    return ret;
}

static int upgrade_both_flash(char *filename, int erase_type)
{
    int ret, current_bmc;

    enable_upgrade();

    current_bmc = get_current_bmc();
    if (current_bmc == CURRENT_MASTER) {
        printf("* Current Bmc Default Boot: CE0\n");
    } else {
        printf("* Current Bmc Default Boot: CE1\n");
    }

    ret = upgrade_bmc_flash(filename, current_bmc, CE0, erase_type);
    if (ret < 0) {
        printf("Upgrade master bmc flash failed, stop upgrade.\n");
        goto err;
    }
    printf("Upgrade master bmc flash success.\n");

    ret = upgrade_bmc_flash(filename, current_bmc, CE1, erase_type);
    if (ret < 0) {
        printf("Upgrade slave bmc flash failed.\n");
        goto err;
    }
    printf("Upgrade slave bmc flash success.\n");

    bmc_reboot(CE0);
    return 0;
err:
    disable_upgrade();
    return -1;
}

static int upgrade_single_flash(char *filename, int cs, int erase_type)
{
    int ret, current_bmc;

    enable_upgrade();

    current_bmc = get_current_bmc();
    if (current_bmc == CURRENT_MASTER) {
        printf("* Current Bmc Default Boot: CE0\n");
    } else {
        printf("* Current Bmc Default Boot: CE1\n");
    }

    ret = upgrade_bmc_flash(filename, current_bmc, cs, erase_type);
    if (ret < 0) {
        printf("Upgrade %s bmc flash failed.\n", cs == 0 ? "master":"slave");
        goto err;
    }
    printf("Upgrade %s bmc flash success.\n", cs == 0 ? "master":"slave");

    bmc_reboot(cs);
    return 0;
err:
    disable_upgrade();
    return -1;
}

static int upgrade_bmc(char *filename, int cs, int erase_type)
{
    int ret;

    if (access(filename, F_OK) < 0) {
        printf("Can't find file\n");
        help();
        return -1;
    }

    ret = set_ioport_rw_access();
    if (ret < 0) {
        printf("IO ERROR\n");
        return -1;
    }

    switch(cs) {
    /* Single */
    case CE0:
    case CE1:
        ret = upgrade_single_flash(filename, cs, erase_type);
        break;
    /* Both */
    case BOTHFLASH:
        ret = upgrade_both_flash(filename, erase_type);
        break;
    default:
        ret = -1;
        printf("Unsupport cs:%d\n", cs);
        break;
    }

    return ret;
}

static int read_single_bmc_flash(flash_info_t* info, uint32_t start_addr, int read_size, int is_print)
{
    uint32_t res, flash_start_addr, flash_end_addr;
    char filename[MAX_FILENAME_LENGTH];
    int fd, ret;

    flash_start_addr = info->flash_base_addr + start_addr;
    flash_end_addr = flash_start_addr + read_size;
    ret = 0;
    fd = 0;
    if (!is_print) {
        mem_clear(filename, MAX_FILENAME_LENGTH);
        snprintf(filename, MAX_FILENAME_LENGTH, "/tmp/image-bmc%d", info->cs);
        fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXG|S_IRWXU|S_IRWXO);
        if (fd < 0) {
            printf("open file %s fail(err:%d)!\r\n", filename, errno);
            return -1;
        }
    }

    printf("* CE%d FLASH TYPE: SPI FLASH\n", info->cs);
    printf("* FLASH NAME: %s\n", info->flash_name);
    printf("* Read flash addr:0x%x, size:0x%x\n", flash_start_addr, read_size);
    printf("=========================================\n");
    printf("Reading...\n");

    pull_ce_down(info);
    send_cmd(info->flash_base_addr, COMMON_FLASH_READ);
    write_bmc_flash_addr(flash_start_addr);
    while (1) {
        if (flash_start_addr >= flash_end_addr) {
            break;
        }
        res = read_bmc_flash_data();
        if (is_print) {
            printf("addr:0x%08x, val:0x%08x\n", flash_start_addr, res);
        } else {
            ret = write(fd, &res, sizeof(res));
            if (ret < 0) {
                printf("write failed (errno: %d).\n", errno);
                ret = -1;
                goto exit;
            }
        }
        if (((flash_start_addr % 0x10000) == 0) && (!is_print)) {
            printf("\r0x%x ", flash_start_addr);
            fflush(stdout);
        }
        flash_start_addr += 4;
    }
    printf("\r\nRead Finish\n");
    printf("=========================================\n");
exit:
    pull_ce_up(info);
    if (fd > 0) {
        close(fd);
    }
    return ret;
}

static int read_bmc_flash(int cs, uint32_t start_addr, int read_size, int is_print)
{
    int ret, current_bmc;
    flash_info_t* info;

    ret = set_ioport_rw_access();
    if (ret < 0) {
        printf("IO ERROR\n");
        return -1;
    }

    enable_upgrade();

    current_bmc = get_current_bmc();
    if (current_bmc == CURRENT_MASTER) {
        printf("* Current Bmc Default Boot: CE0\n");
    } else {
        printf("* Current Bmc Default Boot: CE1\n");
    }

    info = get_flash_info(current_bmc, cs);
    if(info == NULL) {
        goto err;
    }

    if (start_addr >= info->flash_size) {
        printf("start_addr 0x%x out of range.\n", start_addr);
        goto err;
    }

    if ((start_addr + read_size) > info->flash_size) {
        printf("read size %d exceed flash size.\n", read_size);
        read_size = info->flash_size - start_addr;
    }

    init_flash(info);

    ret = read_single_bmc_flash(info, start_addr, read_size, is_print);
    if (ret < 0) {
        printf("Read %s bmc flash failed.\n", cs == 0 ? "master" : "slave");
        goto err;
    }
    disable_upgrade();
    return 0;
err:
    disable_upgrade();
    return -1;
}

static int read_bmc_reg_main(int argc, char* argv[])
{
    uint32_t start_addr, read_val;
    int read_size, ret;
    char *stopstring;

    if (argc != 4) {
        printf("Input invalid.\n");
        help();
        return -1;
    }

    start_addr = strtoul(argv[2], &stopstring, 16);
    read_size = strtol(argv[3], &stopstring, 10);

    if (read_size <= 0) {
        printf("read length %d invalid\n", read_size);
        return -1;
    }

    if (((start_addr % 4) != 0) || ((read_size % 4) != 0)) {
        printf("Params invalid, start_addr:0x%08x, read_size:%d\n", start_addr, read_size);
        printf("Please input address/length times of 4\n");
        return -1;
    }

    ret = set_ioport_rw_access();
    if (ret < 0) {
        printf("IO ERROR\n");
        return -1;
    }

    enable_ilpc2ahb();

    printf("read bcm reg, start_addr:0x%08x, read length:%d\n", start_addr, read_size);
    printf("===Addr=== | ===Cont===\n");
    while (read_size) {
        read_val = read_bmc_reg(start_addr);
        printf("0x%08x | 0x%08x\n", start_addr, read_val);
        start_addr += 4;
        read_size -= 4;
    }

    disable_ilpc2ahb();
    return 0;
}

static int write_bmc_reg_main(int argc, char* argv[])
{
    uint32_t addr, wr_val;
    int ret;
    char *stopstring;

    if (argc != 4) {
        printf("Input invalid.\n");
        help();
        return -1;
    }

    addr = strtoul(argv[2], &stopstring, 16);
    wr_val = strtoul(argv[3], &stopstring, 16);

    if (((addr & MASK_BYTE) != REGISTER_HEAD) || ((addr % 4) != 0)) {
        printf("Address[0x%08x] invalid, address should be register address and times of 4.\n", addr);
        return -1;
    }

    ret = set_ioport_rw_access();
    if (ret < 0) {
        printf("IO ERROR\n");
        return -1;
    }

    printf("write bcm reg, addr:0x%08x, val:0x%08x\n", addr, wr_val);

    enable_ilpc2ahb();
    write_bmc_reg(addr, wr_val);
    disable_ilpc2ahb();

    return 0;
}

static int get_fmc_info_main(void)
{
    int ret;

    ret = set_ioport_rw_access();
    if (ret < 0) {
        printf("IO ERROR\n");
        return -1;
    }

    enable_ilpc2ahb();

    debug_on = 3;
    fmc_debug();
    debug_on = 0;

    disable_ilpc2ahb();
    return 0;
}

static int program_flash_main(int argc, char* argv[])
{
    int cs, erase_way, ret;
    char *stopstring;
    char tmp[128];

    if (argc != 5) {
        printf("Input invalid.\n");
        help();
        return -1;
    }

    cs = strtol(argv[3], &stopstring, 10);
    if ((strlen(stopstring) != 0) || cs < 0 || cs > 2) {
        snprintf(tmp, sizeof(tmp), "%s", argv[3]);
        printf("Incorrect chip select %s\n", tmp);
        help();
        return -1;
    }

    if (strcmp(argv[4], "full") == 0) {
        erase_way = FULL_ERASE;
    } else if (strcmp(argv[4], "block") == 0) {
        erase_way = BLOCK_ERASE;
    } else {
        snprintf(tmp, sizeof(tmp), "%s", argv[4]);
        printf("Incorrect erase type %s\n", tmp);
        help();
        return -1;
    }

    printf("============BMC Upgrade Tool=============\n");
    ret = upgrade_bmc(argv[2], cs, erase_way);
    return ret;
}

static int read_bmc_flash_main(int argc, char* argv[])
{
    int cs, ret, read_size, is_print;
    uint32_t start_addr;
    char *stopstring;
    char tmp[128];

    if (argc != 6) {
        printf("Input invalid.\n");
        help();
        return -1;
    }

    cs = strtol(argv[2], &stopstring, 10);
    if ((strlen(stopstring) != 0) || cs < 0 || cs > 1) {
        snprintf(tmp, sizeof(tmp), "%s", argv[2]);
        printf("Incorrect chip select %s\n", tmp);
        help();
        return -1;
    }

    start_addr = strtoul(argv[3], &stopstring, 16);
    read_size = strtol(argv[4], &stopstring, 10);

    if (read_size <= 0) {
        printf("read length %d invalid\n", read_size);
        return -1;
    }

    if (((start_addr % 4) != 0) || ((read_size % 4) != 0)) {
        printf("Params invalid, start_addr:0x%08x, read_size:%d\n", start_addr, read_size);
        printf("Please input address/length times of 4\n");
        return -1;
    }

    if (strcmp(argv[5], "print") == 0) {
        is_print = 1;
    } else {
        is_print = 0;
    }

    printf("============READ BMC FLASH=============\n");
    ret = read_bmc_flash(cs, start_addr, read_size, is_print);
    return ret;
}

int main(int argc, char *argv[])
{
    int ret;

    debug_on = fw_upgrade_debug();

    if (argc < 2) {
        help();
        return -1;
    }

    if (argc == 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            help();
            return 0;
        }
    }

    if (strcmp(argv[1], "rd") == 0) {
        ret = read_bmc_reg_main(argc, argv);
        if (ret < 0) {
            printf("Read Failed\n");
        }
        return ret;
    }

    if (strcmp(argv[1], "wr") == 0 && debug_on == 3) {
        ret = write_bmc_reg_main(argc, argv);
        if (ret < 0) {
            printf("Write Failed\n");
        }
        return ret;
    }

    if (strcmp(argv[1], "info") == 0) {
        ret = get_fmc_info_main();
        if (ret < 0) {
            printf("Get fmc info Failed\n");
        }
        return ret;
    }

    if (strcmp(argv[1], "upgrade") == 0) {
        ret = program_flash_main(argc, argv);
        if (ret < 0) {
            printf("Upgrade BMC failed.\n");
        }
        return ret;
    }

    if (strcmp(argv[1], "read_bmc_flash") == 0) {
        ret = read_bmc_flash_main(argc, argv);
        if (ret < 0) {
            printf("Read BMC flash failed.\n");
        }
        return ret;
    }

    printf("Input invalid.\n");
    help();

    return -1;
}
