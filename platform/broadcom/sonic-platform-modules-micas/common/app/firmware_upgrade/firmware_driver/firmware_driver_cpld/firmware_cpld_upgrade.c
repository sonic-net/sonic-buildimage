#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <firmware.h>
#include <firmware_cpld.h>
#include <jbi.h>

/* CPLD file parses the relevant parameters */
#define CPLD_HEX                 16
#define DEC_VAL                  10
#define CPLD_INIT_CNT            4
#define CPLD_UNIT_SZ             4
#define CPLD_HEAD_KEYWORD       "Header"
#define CPLD_NAME_KEYWORD       "Entity"
#define CPLD_INIT_KEYWORD       "INITIALIZE"
#define CPLD_REPEAT_KEYWORD     "REPEAT"
#define CPLD_END_CHAR           ','

/* TCK clock MAX 16MHz */
#define    TCK_DELAY                         (current_fmw_cpld->tck_delay)

/*
 * The instruction format of the MAX II CPLD is 10 bits
 * For shift_ir state machine use
 */
#define    BYPASS                            0x3FF
#define    EXTEST                            0xF
#define    SAMPLE                            0x5
#define    IDCODE                            0x6
#define    USERCODE                          0x7
#define    CLAMP                             0xA
#define    HIGHZ                             0xB

/* Following 7 instructions are IEEE 1532 instructions */
#define    ISC_ENABLE                        0x2CC
#define    ISC_DISABLE                       0x201
#define    ISC_PROGRAM                       0x2F4
#define    ISC_ERASE                         0x2F2
#define    ISC_ADDRESS_SHIFT                 0x203
#define    ISC_READ                          0x205
#define    ISC_NOOP                          0x210

/*
 * MAX II devices support the real-time in-system programmability (ISP)
 * feature that allows you to program the device while it is still in operation.
 * when there is either a power cycle to the device (powering down and powering
 * up again) or with the execution of certain ISP instructions to start the SRAM
 * download process when realtime ISP has completed.
 */
#define    RT_ISC_ENABLE                     0x199
#define    RT_ISC_DISABLE                    0x166

/* Chip ID */
#define    EPM240_G                          0x020A10DD
#define    EPM570_G                          0x020A20DD
#define    EPM1270_G                         0x020A30DD
#define    EPM2210_G                         0x020A40DD
#define    EPM240_Z                          0x020A50DD
#define    EPM570_Z                          0x020A60DD

/* The size of the output data for ID validation */
#define    VERIFY_IDCODE_SIZE                0x5

/* Erasure and programmatic delay handling */
#define    ERASE_DELAY                       0x1024
#define    PROGRAM_DELAY                     0x5

/* Chip instruction register */
#define    CPLD_INSTRUCTION_SIZE             10

/*
 * Currently, only two connectors are supported
 * The size of the instruction register needs to be changed
 * when more than two connectors are used
 */
#ifndef CPLD_MAX_CHIP
#define    CPLD_MAX_CHIP                     2
#endif

typedef struct cpld_chip_id {
    char    *name;
    uint    id;
    int     addr_register_length;
    int     data_register_length;
    int     eeprom_array_length;
    int     first_blank_check_length;
    int     second_blank_check_length;
    int     first_erase_addr;
    int     second_erase_addr;
    int     third_erase_addr;
    int     verify_idcode_addr;
} cpld_chip_id_t;

static cpld_chip_id_t cpld_id_table[] = {
    {"EPM240T100", EPM240_G, 13, 16, 4604, 3327, 511, 0x0, 0x1, 0x11, 0x89},
    {"EPM570T144", EPM570_G, 14, 16, 8700, 3327, 511, 0x0, 0x1, 0x21, 0x111},
    {"EPM1270F256", EPM1270_G, 15, 16, 16892, 16383, 511, 0x0, 0x1, 0x41, 0x221},
    {"5M240Z", EPM240_Z, 13, 16, 4604, 3327, 511, 0x0, 0x1, 0x11, 0x89},
    {"5M570Z", EPM570_Z, 14, 16, 8700, 3327, 511, 0x0, 0x1, 0x21, 0x111},
    {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static cpld_chip_id_t *chip_cpld_info = NULL;

/* The following variables are used when cascading multiple chips */
static int chip_num, current_chip_index;
static firmware_cpld_t *current_fmw_cpld;

static int TDI_PULL_UP(void);
static int TDI_PULL_DOWN(void);
static int TMS_PULL_UP(void);
static int TMS_PULL_DOWN(void);
static int TCK_PULL_UP(void);
static int TCK_PULL_DOWN(void);

/*
 * set_currrent_cpld_info
 * function: Save the current device information
 * @info: param[in] Information about the device to be updated
 */
static void set_currrent_cpld_info(firmware_cpld_t *info)
{
    current_fmw_cpld = info;
}

/*
 * firmware_upgrade_en
 * function: Upgrade access enabling switch
 * @flag: !0:enable 0:disable
 */
static int firmware_upgrade_en(int flag)
{
    int i;
    int ret;

    for (i = 0; i < current_fmw_cpld->gpio_en_info_num; i++) {
        if (flag) {
            ret = gpio_request(current_fmw_cpld->gpio_en_info[i].en_gpio, "cpld_upgrade");
            if (ret) {
                FIRMWARE_DRIVER_DEBUG_ERROR("Requesting cpld_ispvme_upgrade EN[%d] GPIO[%d] failed!\n",
                        i, current_fmw_cpld->gpio_en_info[i].en_gpio);
                goto free_gpio;
            }
            gpio_direction_output(current_fmw_cpld->gpio_en_info[i].en_gpio, current_fmw_cpld->gpio_en_info[i].en_level);
            current_fmw_cpld->gpio_en_info[i].flag = 1;
        } else {
            gpio_set_value(current_fmw_cpld->gpio_en_info[i].en_gpio, !current_fmw_cpld->gpio_en_info[i].en_level);
            gpio_free(current_fmw_cpld->gpio_en_info[i].en_gpio);
            current_fmw_cpld->gpio_en_info[i].flag = 0;
        }
    }
    return 0;
free_gpio:
    for (i = 0; i < current_fmw_cpld->gpio_en_info_num; i++) {
        if (current_fmw_cpld->gpio_en_info[i].flag == 1) {
            gpio_set_value(current_fmw_cpld->gpio_en_info[i].en_gpio, !current_fmw_cpld->gpio_en_info[i].en_level);
            gpio_free(current_fmw_cpld->gpio_en_info[i].en_gpio);
            current_fmw_cpld->gpio_en_info[i].flag = 0;
        } else {
            break;
        }
    }

    return -1;
}

/*
 * init_cpld
 * function:Initialize CPLD
 * return value: 0 success ; -1 fail
 */
static int init_cpld(void)
{
    int ret;

    if (current_fmw_cpld == NULL) {
        return -1;
    }
    mdelay(10);
    ret = gpio_request(current_fmw_cpld->tdi, "cpld_upgrade");
    if (ret) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Requesting cpld_upgrade TDI GPIO failed!\n");
        return ret;
    }
    ret = gpio_request(current_fmw_cpld->tck, "cpld_upgrade");
    if (ret) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Requesting cpld_upgrade TCK GPIO failed!\n");
        goto free_tdi;
    }
    ret = gpio_request(current_fmw_cpld->tms, "cpld_upgrade");
    if (ret) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Requesting cpld_upgrade TMS GPIO failed!\n");
        goto free_tck;
    }
    ret = gpio_request(current_fmw_cpld->tdo, "cpld_upgrade");
    if (ret) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Requesting cpld_upgrade TDO GPIO failed!\n");
        goto free_tms;
    }

    gpio_direction_output(current_fmw_cpld->tdi, 1);
    gpio_direction_output(current_fmw_cpld->tck, 1);
    gpio_direction_output(current_fmw_cpld->tms, 1);

    gpio_direction_input(current_fmw_cpld->tdo);
    ret = firmware_upgrade_en(1);
    if (ret) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: open firmware upgrade en failed, ret %d.\n", ret);
        goto free_tdo;
    }

    /* test GPIO */
    if (TDI_PULL_UP() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TDI_PULL_UP failed.\n");
        goto free_tdo;
    }
    if (TDI_PULL_DOWN() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TDI_PULL_DOWN failed.\n");
        goto free_tdo;
    }
    if (TMS_PULL_UP() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TMS_PULL_UP failed.\n");
        goto free_tdo;
    }
    if (TMS_PULL_DOWN() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TMS_PULL_DOWN failed.\n");
        goto free_tdo;
    }
    if (TCK_PULL_UP() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TCK_PULL_UP failed.\n");
        goto free_tdo;
    }
    if (TCK_PULL_DOWN() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TCK_PULL_DOWN failed.\n");
        goto free_tdo;
    }

    mdelay(10);
    return 0;

free_tdo:
    gpio_free(current_fmw_cpld->tdo);
free_tms:
    gpio_free(current_fmw_cpld->tms);
free_tck:
    gpio_free(current_fmw_cpld->tck);
free_tdi:
    gpio_free(current_fmw_cpld->tdi);
    return ret;
}

/*
 * finish_cpld
 * function: finish  CPLD upgrade operation
 * return value: 0 success ; -1 fail
 */
static int finish_cpld(void)
{
    int ret;

    if (current_fmw_cpld == NULL) {
        return -1;
    }
    mdelay(10);
    ret = firmware_upgrade_en(0);
    if (ret < 0){
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: close firmware upgrade en failed, ret %d.\r\n", ret);
    }

    gpio_free(current_fmw_cpld->tdi);
    gpio_free(current_fmw_cpld->tck);
    gpio_free(current_fmw_cpld->tms);
    gpio_free(current_fmw_cpld->tdo);
    mdelay(10);
    return 0;
}

/* Loop waiting for */
static int pull_wait(int gpio, int value) {
    int i, j;
    /* Timeout time is two seconds */
    for (i = 0; i < 20; i++) {
        for (j = 0; j < 100; j++) {
            if (!!gpio_get_value(gpio) == !!value ) {
                return 0;
            }
            /* The first loop does not delay, normally the first loop can immediately return the result */
            if (i) {
                mdelay(1);
            }
        }
        /* The CPU is released every 100ms */
        schedule();
    }
    /* timeout */
    FIRMWARE_DRIVER_DEBUG_ERROR("Error: Wait gpio %d pull to %d failed.\n", gpio, value);
    return -1;
}

/* TDI pull-up */
static int pull_tdi_up(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tdi, 1);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tdi, 1);
}

/* TDI pull-down */
static int pull_tdi_down(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tdi, 0);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tdi, 0);
}

/* TCK pull-up */
static int pull_tck_up(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tck, 1);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tck, 1);
}

/* TCK pull-down */
static int pull_tck_down(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tck, 0);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tck, 0);
}

/* TMS pull-up */
static int pull_tms_up(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tms, 1);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tms, 1);
}

/* TMS pull-down */
static int pull_tms_down(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tms, 0);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tms, 0);
}

/* Read TDO */
static int read_tdo(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    return gpio_get_value(current_fmw_cpld->tdo);
}

static firmware_cpld_function_t function_fmw_cpld = {
      .pull_tdi_up = pull_tdi_up,
      .pull_tdi_down = pull_tdi_down,
      .pull_tck_up = pull_tck_up,
      .pull_tck_down = pull_tck_down,
      .pull_tms_up = pull_tms_up,
      .pull_tms_down = pull_tms_down,
      .read_tdo = read_tdo,
      .init_cpld = init_cpld,
      .finish_cpld = finish_cpld,
};

/*
 * TDI_PULL_DOWN
 * function: Lower TDI
 */
static int TDI_PULL_DOWN(void)
{
    if ( function_fmw_cpld.pull_tdi_down != NULL) {
        return function_fmw_cpld.pull_tdi_down();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TDI_PULL_DOWN.\n");
        return -1;
    }
}

/*
 * TDI_PULL_UP
 * function: High TDI
 */
static int TDI_PULL_UP(void)
{
    if (function_fmw_cpld.pull_tdi_up != NULL) {
        return function_fmw_cpld.pull_tdi_up();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TDI_PULL_UP.\n");
        return -1;
    }
}

/*
 * TCK_PULL_DOWN
 * function: Lower TCK
 */
static int TCK_PULL_DOWN(void)
{
    if (function_fmw_cpld.pull_tck_down != NULL) {
        return function_fmw_cpld.pull_tck_down();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TCK_PULL_DOWN.\n");
        return -1;
    }
}

/*
 * TCK_PULL_UP
 * function: High TCK
 */
static int TCK_PULL_UP(void)
{
    if (function_fmw_cpld.pull_tck_up != NULL) {
        return function_fmw_cpld.pull_tck_up();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TCK_PULL_UP.\n");
        return -1;
    }
}

/*
 * TMS_PULL_DOWN
 * function: Lower TMS
 */
static int TMS_PULL_DOWN(void)
{
    if (function_fmw_cpld.pull_tms_down != NULL) {
        return function_fmw_cpld.pull_tms_down();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TMS_PULL_DOWN.\n");
        return -1;
    }
}

/*
 * TMS_PULL_UP
 * function: High TMS
 */
static int TMS_PULL_UP(void)
{
    if (function_fmw_cpld.pull_tms_up != NULL) {
        return function_fmw_cpld.pull_tms_up();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TMS_PULL_UP.\n");
        return -1;
    }
}

/*
 * TDO_READ
 * function:Read the TDO level
 */
static int TDO_READ(void)
{
    if (function_fmw_cpld.read_tdo != NULL) {
        return function_fmw_cpld.read_tdo();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TDO_READ.\n");
        return -1;
    }
}

/*
 * tap_test_logic_reset
 * function: reset JTAG
 * No matter what the original state of the controoler, it will enter
 * Test_Logic_Reset when TMS is held high for at least five rising
 * edges of TCK (16MHz)
 * The controller remains in this state while TMS is high
 */
static void tap_test_logic_reset(void)
{
    int i;
    TMS_PULL_UP();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);

    for (i = 0; i < 5; i++) {
        TCK_PULL_UP();
        ndelay(TCK_DELAY);
        TCK_PULL_DOWN();
        ndelay(TCK_DELAY);
    }
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

/*
 * tap_run_test_idle
 * function:  A controller state between scan operations.Once entered, the controller
 * will remain in the Run_Test/Idle state as long as TMS is held low.
 */
static void tap_run_test_idle(void)
{
    TMS_PULL_DOWN();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

/*
 * tap_select_dr_scan
 * function :This is a temporary controller state in which all test data registers
 * selected by the current instruction retain their previous state.
 */
static void tap_select_dr_scan(void)
{
    TMS_PULL_UP();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

/*
 * tap_capture_dr
 * function : In this controller state data may be parallel-loaded into test data
 * register selected by the current instruction on the rising edge of TCK
 */
static void tap_capture_dr(void)
{
    TMS_PULL_DOWN();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

/*
 * tap_shift_dr
 * function: In this controller state.the test data register connected between TDI
 * and TDO as a result of the current instruction shifts one stage
 * toward its serial output on each rising edge of TCK.
 */
static void tap_shift_dr(void)
{
    TMS_PULL_DOWN();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

/*
 * tap_exit1_dr
 * function: This is a temporary controller state.
 */
static void tap_exit1_dr(int data)
{
    int j;
    if (data) {
        TDI_PULL_UP();
    } else {
        TDI_PULL_DOWN();
    }

    /* need to idle here */
    for (j = 1; j < current_chip_index; j++) {
        TCK_PULL_DOWN();
        ndelay(TCK_DELAY);
        TCK_PULL_UP();
        ndelay(TCK_DELAY);
    }
    TMS_PULL_UP();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

/*
 * tap_update_dr
 * function : Some test data registers may be provided with a latched parallel output to
 * prevent changes at the parallel out-put while data is shifted in the
 * associated whift-register path in response to certain instructions.Data is
 * latched onto the parallel output of these test data registers from the
 * shift-register path on the falling edge of TCK in the Update-DR controler state.
 */
static void tap_update_dr(void)
{
    TMS_PULL_UP();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

/*
 * tap_select_ir_scan
 * function:This is a temporarily controler state in which all test data register selected
 * by the current instruction retain their previous state.
 */
static void tap_select_ir_scan(void)
{
    TMS_PULL_UP();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

/*
 * tap_capture_ir
 * function :In this controller state the shift-register contained in the instruction
 * register loads a pattern of fixed logic values on the rising edge of
 * TCK.design-specific data may be loaded into shift-register stages that
 * are not required to be set to fixed values.
 */
static void tap_capture_ir(void)
{
    TMS_PULL_DOWN();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

/*
 * tap_exit1_ir
 * function : enter exit1 ir state. This is a temporary controller state.
 */
static void tap_exit1_ir(int data)
{
    if (data) {
        TDI_PULL_UP();
    } else {
        TDI_PULL_DOWN();
    }
    TMS_PULL_UP();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

/*
 * tap_shift_ir
 * function: In this controller state the shift-register contained in the instruction
 * register is connected between TDI and TDO and shifts data one stage
 * toward its serial output on each rising edge of TCK.
 */
static void tap_shift_ir(void)
{
    TMS_PULL_DOWN();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

/*
 * The instruction shifted into the instruction register is latched onto the parallel output
 * from the shift-register path on the falling edge of TCK in this controller state.Once the
 * new instruction has been latched,it becomes the current instruction.
 *
 */
static void tap_update_ir(void)
{
    TMS_PULL_UP();
    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);
    TCK_PULL_UP();
    ndelay(TCK_DELAY);
}

static void tap_send_instruction(int instruction, int ins_len)
{
    int i;
    for (i = 0; i < (ins_len - 1); i++) {
        if (instruction & 0x1) {
            TDI_PULL_UP();
        } else {
            TDI_PULL_DOWN();
        }
        TCK_PULL_DOWN();
        ndelay(TCK_DELAY);
        TCK_PULL_UP();
        ndelay(TCK_DELAY);
        instruction = instruction >> 1;
    }
}

static void tap_send_data(int data, int data_len)
{
    int i;
    for (i = 0; i < (data_len - 1); i++) {
        if (data & 0x1) {
            TDI_PULL_UP();
        } else {
            TDI_PULL_DOWN();
        }
        TCK_PULL_DOWN();
        ndelay(TCK_DELAY);
        TCK_PULL_UP();
        ndelay(TCK_DELAY);
        data = data >> 1;
    }
}

/*
 * tap_rcv_byte
 * function  : Receive data from the device side
 * @data : param[out] Received data */
static void tap_rcv_byte(u8 *data)
{
    int i;
    u8 rec_data = 0;
    unsigned char tmp;
    ndelay(TCK_DELAY);
    for (i = 0; i < 8; i++) {
        TCK_PULL_DOWN();
        ndelay(TCK_DELAY);
        tmp = TDO_READ();
        rec_data |= (tmp << i);
        TCK_PULL_UP();
        ndelay(TCK_DELAY);
    }
    *data = rec_data;
}

/*
 * tap_idle
 * function :Used for state machine idling
 */
static void tap_idle(void)
{
    int i;
    for (i = 0; i < 0x100; i++) {
        TCK_PULL_DOWN();
        ndelay(TCK_DELAY);
        TCK_PULL_UP();
        ndelay(TCK_DELAY);

        /* Timely release of CPU   */
        schedule();
    }
}

/*
 * jtag_read_data
 * function :Read the JTAG output data
 * @size: param[in]  buffer size
 * @data: param[out] read data buffer
 */
static void jtag_read_data(u8 *buf, int size)
{
    int i, j;
    /* JTAG state switching */
    tap_run_test_idle();
    tap_select_dr_scan();
    tap_capture_dr();
    tap_shift_dr();
    for (j = current_chip_index; j < chip_num; j++) {
        TCK_PULL_DOWN();
        ndelay(TCK_DELAY);
        TCK_PULL_UP();
        ndelay(TCK_DELAY);
    }
    /* Receive data from the device side */
    for (i = 0; i < size; i++) {
        tap_rcv_byte(&buf[i]);
    }
    /* JTAG state switching */
    tap_exit1_dr(0);
    tap_update_dr();
    tap_run_test_idle();
}

/*
 * jtag_send_instruction
 * function :JTAG instruction sending interface
 * @instruction: param[in]  Instruction to be sent
 * @ins_length:  param[in]  Instruction length
 */
static void jtag_send_instruction(int instruction, int ins_length)
{
    int i, j;
    i = 1 << (ins_length - 1);
    /* JTAG state switching */
    tap_run_test_idle();
    tap_select_dr_scan();
    tap_select_ir_scan();
    tap_capture_ir();
    tap_shift_ir();

    for (j = chip_num; j > 1; j--) {
        if (j == current_chip_index) {
            tap_send_instruction(instruction, ins_length + 1);
        } else {
            tap_send_instruction(BYPASS, ins_length + 1);
        }
    }

    if (current_chip_index == 1) {
        tap_send_instruction(instruction, ins_length);
        /* Gets the highest bit of the instruction */
        tap_exit1_ir((instruction & i) >> (ins_length - 1));
    } else {
        tap_send_instruction(BYPASS, ins_length);
        /* Gets the highest bit of the instruction */
        tap_exit1_ir((BYPASS & i) >> (ins_length - 1));
    }

    /* JTAG state switching */
    tap_update_ir();
    tap_run_test_idle();
}

/*
 * jtag_send_data
 * function :JTAG data sending interface
 * @buf : param[in]    Data that needs to be sent
 * @data_length: param[in] Data length
 */
static void jtag_send_data(unsigned int buf, int data_length)
{
    int i;
    i = 1 << (data_length - 1);

    /* JTAG state switching */
    tap_run_test_idle();
    tap_select_dr_scan();
    tap_capture_dr();
    tap_shift_dr();
    tap_send_data(buf, data_length);
    /* Gets the highest bit of the instruction */
    tap_exit1_dr((buf & i) >> (data_length - 1));
    tap_update_dr();
    tap_run_test_idle();
}

/*
 * jtag_program_donebit
 * JTAG programming end point */
static void jtag_program_donebit(void)
{
    jtag_send_instruction(ISC_ADDRESS_SHIFT, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    jtag_send_data(0x0, chip_cpld_info->addr_register_length);
    tap_idle();

    switch (chip_cpld_info->id) {
    case EPM240_G:
    case EPM570_G:
        jtag_send_instruction(ISC_PROGRAM, CPLD_INSTRUCTION_SIZE);
        tap_idle();
        jtag_send_data(0x7BFF, chip_cpld_info->data_register_length);
        tap_idle();
        break;
    case EPM1270_G:
        jtag_send_instruction(ISC_PROGRAM, CPLD_INSTRUCTION_SIZE);
        tap_idle();
        jtag_send_data(0x7FFF, chip_cpld_info->data_register_length);
        tap_idle();

        jtag_send_instruction(ISC_PROGRAM, CPLD_INSTRUCTION_SIZE);
        tap_idle();
        jtag_send_data(0xFFFF, chip_cpld_info->data_register_length);
        tap_idle();

        jtag_send_instruction(ISC_PROGRAM, CPLD_INSTRUCTION_SIZE);
        tap_idle();
        jtag_send_data(0xFFBF, chip_cpld_info->data_register_length);
        tap_idle();

        jtag_send_instruction(ISC_PROGRAM, CPLD_INSTRUCTION_SIZE);
        tap_idle();
        jtag_send_data(0xFFFF, chip_cpld_info->data_register_length);
        tap_idle();
        break;
    default:
        break;
    } /* End of switch */
}

/*
 * jtag_rt_disable
 * JTAG Disable state machine under Real-Time ISP
 */
static void jtag_rt_disable(void)
{
    jtag_send_instruction(RT_ISC_DISABLE, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    jtag_send_instruction(BYPASS, CPLD_INSTRUCTION_SIZE);
    tap_idle();
}

/*
 * jtag_verify_idcode
 * function :JTAG internal ID reading
 */
static void jtag_verify_idcode(void)
{
    int data, i;
    u8 buf[2];

    jtag_send_instruction(ISC_ADDRESS_SHIFT, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    jtag_send_data(chip_cpld_info->verify_idcode_addr,
                   chip_cpld_info->addr_register_length);
    tap_idle();
    for (i = 0; i < VERIFY_IDCODE_SIZE; i++) {
        jtag_send_instruction(ISC_READ, CPLD_INSTRUCTION_SIZE);
        tap_idle();

        jtag_read_data(buf, 2);

        /* When validating the ID, the data is compared to the corresponding chip value,
           which is retrieved from the BSDL file*/
        data = (buf[1] << 8) | buf[0];
    }
}

/*
 * jtag_rt_enable
 * Enter Real-Time ISP mode; JTAG Enable State Machine under Real-Time ISP
 */
static void jtag_rt_enable(void)
{
    jtag_send_instruction(RT_ISC_ENABLE, CPLD_INSTRUCTION_SIZE);
    tap_idle();
}

/*
 * jtag_erase
 * JTAG erases the timing
 */
static void jtag_erase(void)
{
    int i;

    jtag_send_instruction(ISC_ADDRESS_SHIFT, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    jtag_send_data(chip_cpld_info->first_erase_addr,
                   chip_cpld_info->addr_register_length);
    tap_idle();
    jtag_send_instruction(ISC_ERASE, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    for (i = 0; i < ERASE_DELAY; i++) {
        tap_idle();
        tap_idle();
    }

    jtag_send_instruction(ISC_ADDRESS_SHIFT, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    jtag_send_data(chip_cpld_info->second_erase_addr,
                   chip_cpld_info->addr_register_length);
    tap_idle();
    jtag_send_instruction(ISC_ERASE, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    for (i = 0; i < ERASE_DELAY; i++) {
        tap_idle();
        tap_idle();
    }

    jtag_send_instruction(ISC_ADDRESS_SHIFT, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    jtag_send_data(chip_cpld_info->third_erase_addr,
                   chip_cpld_info->addr_register_length);
    tap_idle();
    jtag_send_instruction(ISC_ERASE, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    for (i = 0; i < ERASE_DELAY; i++) {
        tap_idle();
        tap_idle();
    }
}

/*
 * jtag_blank_check
 * JTAG blank detection */
static void jtag_blank_check(void)
{
    int j;
    int data;
    u8 buf[2];

    jtag_send_instruction(ISC_ADDRESS_SHIFT, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    jtag_send_data(0x0, chip_cpld_info->addr_register_length);
    tap_idle();
    for (j = 0; j < chip_cpld_info->first_blank_check_length; j++) {
        jtag_send_instruction(ISC_READ, CPLD_INSTRUCTION_SIZE);
        tap_idle();

        jtag_read_data(buf, 2);
        data = (buf[1] << 8) | buf[0];
    }

    jtag_send_instruction(ISC_ADDRESS_SHIFT, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    jtag_send_data(0x1, chip_cpld_info->addr_register_length);
    tap_idle();
    for (j = 0; j < chip_cpld_info->second_blank_check_length; j++) {
        jtag_send_instruction(ISC_READ, CPLD_INSTRUCTION_SIZE);
        tap_idle();

        jtag_read_data(buf, 2);
        data = (buf[1] << 8) | buf[0];
    }
}

/*
 * jtag_verify1
 * function :JTAG content validation
 * @buffer : param[in]  original data
 * return value  0  validation success; -1 validation failed
 */
static int jtag_verify1(unsigned int *buffer)
{
    int j, ret = 0;
    unsigned int data;
    u8 buf[2];

    jtag_send_instruction(ISC_ADDRESS_SHIFT, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    jtag_send_data(0x0, chip_cpld_info->addr_register_length);
    tap_idle();
    for (j = 0; j < chip_cpld_info->eeprom_array_length; j++) {
        jtag_send_instruction(ISC_READ, CPLD_INSTRUCTION_SIZE);
        tap_idle();

        jtag_read_data(buf, 2);
        data = (buf[1] << 8) | buf[0];

        if (data != buffer[j]) {
            FIRMWARE_DRIVER_DEBUG_ERROR("%d: %02x, %02x.\n", j, data, buffer[j]);
            ret = -1;
            break;
        }
    }
    return ret;
}

/*
 * jtag_read_buffer
 * function:JTAG internal data reading
 * @size: param[in]   Read size
 * @buffer: param[out]  Pointer to read data
 */
static void jtag_read_buffer(unsigned int *buffer, int size)
{
    int j;
    int data;
    u8 buf[2];

    jtag_send_instruction(ISC_ADDRESS_SHIFT, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    jtag_send_data(0x0, chip_cpld_info->addr_register_length);
    tap_idle();
    for (j = 0; j < size; j++) {
        jtag_send_instruction(ISC_READ, CPLD_INSTRUCTION_SIZE);
        tap_idle();

        jtag_read_data(buf, 2);
        data = (buf[1] << 8) | buf[0];
        buffer[j] = data;
    }
}

/*
 * jtag_program
 * function:JTAG programming timing
 * @buffer: param[in]  data pointer to program
 */
static void jtag_program(unsigned int *buffer)
{
    int i, j;

    jtag_send_instruction(ISC_ADDRESS_SHIFT, CPLD_INSTRUCTION_SIZE);
    tap_idle();
    jtag_send_data(0x0, chip_cpld_info->addr_register_length);
    tap_idle();
    for (j = 0; j < chip_cpld_info->eeprom_array_length; j++) {
        jtag_send_instruction(ISC_PROGRAM, CPLD_INSTRUCTION_SIZE);
        tap_idle();

        jtag_send_data(buffer[j], chip_cpld_info->data_register_length);
        for (i = 0; i < PROGRAM_DELAY; i++) {
            tap_idle();
            tap_idle();
        }
    }
}

/*
 * cpld_read_id
 * function: CPLD chip ID read
 * @chip: param[in] chip index
 * id : param[out] ID point */
static void cpld_read_id(int chip, unsigned int *id)
{
    u8 data[sizeof(int)];
    if (!chip_num || chip > chip_num) {
        return;
    }
    current_chip_index = chip;
    /* Send instructions */
    jtag_send_instruction(IDCODE, CPLD_INSTRUCTION_SIZE);
    /* Read Data */
    jtag_read_data(data, sizeof(int));
    *id = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
}

/*
 * chip_num_init
 * function:CPLD number of chips initialized */
static void chip_num_init(void)
{
    unsigned int i, id;
    unsigned char buf[sizeof(int) * CPLD_MAX_CHIP];
    chip_num = 0;

    /* JTAG state switching */
    tap_run_test_idle();
    tap_select_dr_scan();
    tap_capture_dr();
    tap_shift_dr();

    for (i = 0; i < sizeof(int) * CPLD_MAX_CHIP; i++) {
        tap_rcv_byte(&buf[i]);
    }

    /* JTAG state switching */
    tap_exit1_dr(0);
    tap_update_dr();
    tap_run_test_idle();

    for (i = 0; i < sizeof(int) * CPLD_MAX_CHIP; i += 4) {
        id = (buf[i + 3] << 24) | (buf[i + 2] << 16) | (buf[i + 1] << 8) | buf[i];
        FIRMWARE_DRIVER_DEBUG_VERBOSE("ID: %04x\n", id);
        if (id != 0xFFFFFFFF && id != 0) {
            chip_num++;
        }
    }
}

/*
 * cpld_reset
 * function: reset JTAG
 * @chip: param[in] chip index
 */
static void cpld_reset(int chip)
{
    unsigned int chip_type_id = 0;
    int i;
    /* JTAG enters the reset state */
    tap_test_logic_reset();
    /* Gets the number of chips in the CPLD */
    chip_num_init();
    if (!chip_num) {
        pr_notice("There is no CPLD chip or the chip is not supported!!\r\n");
        FIRMWARE_DRIVER_DEBUG_ERROR("chip_num == NULL.\n");
    } else {
        FIRMWARE_DRIVER_DEBUG_VERBOSE("enter cpld read id.\n");
        current_chip_index = chip;
        /* Read chip ID */
        cpld_read_id(current_chip_index, &chip_type_id);
        FIRMWARE_DRIVER_DEBUG_VERBOSE("get cpld id: 0x%x.\n", chip_type_id);
        for (i = 0; cpld_id_table[i].name != NULL; i++) {
            if (cpld_id_table[i].id == chip_type_id) {
                chip_cpld_info = &cpld_id_table[i];
                break;
            }
        }
    }
    current_chip_index = -1;
    tap_test_logic_reset();
}

/*
 * cpld_program
 * function: CPLD programming interface
 * @chip: param[in]    Chip serial number/chip index
 * @buffer: param[in]  data pointer to program
 * return value:      0 success; -1 fail
 */
static int cpld_program(int chip, unsigned int *buffer)
{
    int ret;
    int counte;

    if (!chip_num || chip > chip_num
        || chip_cpld_info == NULL) {
        return -1;
    }
    current_chip_index = chip;

    /* Enter Real-Time ISP mode */
    jtag_rt_enable();
    /* JTAG internal ID reading */
    jtag_verify_idcode();
    /* JTAG erases */
    jtag_erase();
    /* JTAG blank detection */
    jtag_blank_check();
    /* JTAG programming timing */
    jtag_program(buffer);

    /* In the process of upgrade, there is a problem with reading data,
     * which may occur in the process of reading. Some bit reading fails,
     * but the reason is not found.
     * Avoidance resolution: perform multiple checks */
    for (counte = 0; counte < 4; counte++) {
        ret = jtag_verify1(buffer);
        if (counte > 0) {
            pr_notice("Verify again(%d).\n", counte + 1);
        }

        if (ret == 0) {
            break;
        }
    }
    pr_notice("Write chip %d cpld success(%d).\n", chip, ret);
    jtag_program_donebit();

    /* JTAG Disable state machine under Real-Time ISP */
    jtag_rt_disable();

    return ret;
}

static void cpld_read_buffer(int chip, unsigned int *buffer, unsigned int size)
{
    if (!chip_num || chip > chip_num
        || chip_cpld_info == NULL) {
        return;
    }
    current_chip_index = chip;

    /* Enter Real-Time ISP mode */
    jtag_rt_enable();

    /* JTAG internal ID reading */
    jtag_verify_idcode();

    /* JTAG internal data reading */
    jtag_read_buffer(buffer, size);

    jtag_rt_disable();

}

/*
 * cpld_eeprom_size
 * function:CPLD chip capacity size
 * return value :Returns chip capacity on success, or 0 on failure
 */
static int cpld_eeprom_size(void)
{
    int ret;

    if (!chip_num || chip_cpld_info == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("chip_num:%d or chip_cpld_info == NULL.\n", chip_num);
        ret = 0;
    } else {
        ret = chip_cpld_info->eeprom_array_length;
        FIRMWARE_DRIVER_DEBUG_ERROR("chip_cpld_info->eeprom_array_length = %d.\n",
            chip_cpld_info->eeprom_array_length);
    }

    return ret;
}

/*
 * cpld_read_name
 * function: Gets the CPLD chip name
 * @chip: param[in] Chip serial number/chip index
 * return value :chip name */
static char *cpld_read_name(int chip)
{
    uint chip_type_id;
    int i;

    chip_type_id = 0;
    cpld_read_id(chip, &chip_type_id);
    for (i = 0; cpld_id_table[i].name != NULL; i++) {
         if (cpld_id_table[i].id == chip_type_id) {
             return cpld_id_table[i].name;
         }
    }

    return NULL;
}

/*
 * cpld_upgrade_init
 * function:Initialize GPIO and CPLD
 * return value: success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
static int cpld_upgrade_init(void)
{
    int ret;

    if (function_fmw_cpld.init_cpld != NULL) {
        ret = function_fmw_cpld.init_cpld();
        if (ret != FIRMWARE_SUCCESS) {
            return ret;
        }
    }

    return FIRMWARE_SUCCESS;
}

/*
 * cpld_upgrade_finish
 * function:Release GPIO and CPLD
 * return value: success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
static int cpld_upgrade_finish(void)
{
    int ret;

    if (function_fmw_cpld.finish_cpld != NULL) {
        ret = function_fmw_cpld.finish_cpld();
        if (ret != FIRMWARE_SUCCESS) {
            return ret;
        }
    }

    return FIRMWARE_SUCCESS;
}

static int cpld_str_hex_to_dec(char *str, char end_char)
{
    int i;
    int result;

    if (str == NULL) {
        return FIRMWARE_FAILED;
    }

    i = 0;
    result = 0;
    while (str[i] != end_char) {
        /* Check for hexadecimal characters:0123456789abcdef */
        if (!isxdigit(str[i]) || i >= CPLD_UNIT_SZ) {
            return FIRMWARE_FAILED;
        }
        /* Check for a number between 0 and 9 */
        if (isdigit(str[i])) {
            result = result * CPLD_HEX + str[i] - '0';
        }
        /* Check if the character is uppercase */
        else if (isupper(str[i])) {
            result = result * CPLD_HEX + str[i] - 'A' + DEC_VAL;
        } else {
            result = result * CPLD_HEX + str[i] - 'a' + DEC_VAL;
        }

        i++;
    }

    return result;
}

static int cpld_check_upgrade_data(char *src, int src_len, int *dst, int dst_len)
{
    int i, init_lcnt, tmp;
    char *ptr;
    int ret;

    if (src == NULL || dst == NULL) {
        return FIRMWARE_FAILED;
    }
    /* Pointers the ptr pointer to the data following the CPLD_INIT_KEYWORD */
    ret = FIRMWARE_SUCCESS;
    ptr = strstr(src, CPLD_INIT_KEYWORD);
    if (ptr == NULL) {
        return FIRMWARE_FAILED;
    } else {
        ptr += strlen(CPLD_INIT_KEYWORD);
        while (*ptr == '(' || *ptr == '\r' || *ptr == '\n') {
            ptr++;
        }
    }

    /* Converts a hexadecimal string to decimal, with 4 groups of 4 bytes each */
    i = 0;
    init_lcnt = 0;
    for (init_lcnt = 0; init_lcnt < CPLD_INIT_CNT; init_lcnt++) {
        tmp = cpld_str_hex_to_dec(ptr, CPLD_END_CHAR);
        if (tmp < 0) {
            ret = tmp;
            return ret;
        }
        /* int type is 4 bytes */
        dst[i++] = tmp;
        if (i >= dst_len) {
            return FIRMWARE_SUCCESS;
        }

        ptr += CPLD_UNIT_SZ + 1;

        while (*ptr == '\r' || *ptr == '\n') {
            ptr++;
        }
    }

    /* Point the ptr pointer to the data after CPLD_REPEAT_KEYWORD */
    ptr = strstr(src, CPLD_REPEAT_KEYWORD);
    if (ptr == NULL) {
        return FIRMWARE_FAILED;
    } else {
        ptr += strlen(CPLD_REPEAT_KEYWORD);
        while (*ptr == '(' || *ptr == '\r' || *ptr == '\n') {
            ptr++;
        }
    }

    while (1) {
        /* Converts the 4 bytes before ',' to base 10 */
        tmp = cpld_str_hex_to_dec(ptr, CPLD_END_CHAR);
        if (tmp < 0) {
            ret = tmp;
            break;
        }
        dst[i++] = tmp;
        if (i >= dst_len) {
            return FIRMWARE_SUCCESS;
        }

        ptr += CPLD_UNIT_SZ + 1;

        while (*ptr == '\r' || *ptr == '\n') {
            ptr++;
        }
    }

    return FIRMWARE_SUCCESS;
}

/**
 * fmw_cpld_upg_get_chip_name
 * function:get chip name
 * @chain: param[in]  chain
 * @cpld: param[in]  Device private data
 * @len:  param[in]  chip name length
 * @info: param[out] chip name
 * return value: success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int fmw_cpld_upg_get_chip_name(int chain, firmware_cpld_t *cpld, char *info, int len)
{
    int ret;
    char *name;

    /* Check the input and output parameters */
    if (chain < 0 || info == NULL || len <= 0) {
        return FIRMWARE_FAILED;
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Cpld driver to get chip name.\n");

    if (cpld == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to get gpio info.(chain = %d)\n", chain);
    } else {
        set_currrent_cpld_info(cpld);
    }

    if (chain != current_fmw_cpld->chain) {
        FIRMWARE_DRIVER_DEBUG_ERROR("The chain num is not fit."
                                    "(chain = %d, current chain = %d, current name: %s)\n",
                  chain, current_fmw_cpld->chain, current_fmw_cpld->devname);
    }

    /* Initialize GPIO and CPLD */
    ret = cpld_upgrade_init( );
    if (ret != FIRMWARE_SUCCESS) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error:Failed to get chip name when init upgrade.(chain = %d)\n",
            chain);
        return FIRMWARE_FAILED;
    }

    /* reset JTAG */
    cpld_reset(current_fmw_cpld->chip_index);
    /* Read chip name */
    name = cpld_read_name(current_fmw_cpld->chip_index);
    if (name == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to get chip name when read name.(chain %d, index %d)\n",
            chain, current_fmw_cpld->chip_index);
        cpld_upgrade_finish( );
        return FIRMWARE_FAILED;
    }

    /* Release GPIO */
    ret = cpld_upgrade_finish( );
    if (ret != FIRMWARE_SUCCESS) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to get chip name when finish upgrade.(chain = %d)\n",
            chain);
        return FIRMWARE_FAILED;
    }

    strncpy(info, name, len);

    return FIRMWARE_SUCCESS;
}

/**
 * fmw_cpld_upg_program
 * function:Upgrade CPLD(ISC file format)
 * @chain: param[in] chain
 * @cpld: param[in] Device private data
 * @info: param[in] Data to be written
 * @len:  param[in] Length of data to be written
 * return value: success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int fmw_cpld_upg_program(int chain, firmware_cpld_t *cpld, char *info, int len)
{
    int i;
    int time;
    int ret;
    int target_len;
    int *target_buf;

    /* Check the input parameters */
    if (chain < 0 || info == NULL || len <= 0) {
        return FIRMWARE_FAILED;
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Cpld driver to program chip.\n");

    if (cpld == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to get gpio info.(chain = %d)\n", chain);
    } else {
        set_currrent_cpld_info(cpld);
    }

    if (chain != current_fmw_cpld->chain) {
        FIRMWARE_DRIVER_DEBUG_ERROR("The chain num is not fit.(chain = %d, current chain = %d)\n",
                  chain, current_fmw_cpld->chain);
    }
    /* Initialize GPIO and CPLD */
    ret = cpld_upgrade_init( );
    if (ret != FIRMWARE_SUCCESS) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to program when init upgrade.(chain = %d)\n",
                  chain);
        return FIRMWARE_FAILED;
    }

    /* reset JTAG */
    cpld_reset(current_fmw_cpld->chip_index);
    /* CPLD chip capacity size */
    target_len = cpld_eeprom_size();
    if (target_len <= 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to get cpld size.(chain = %d)\n",
                  chain);
        cpld_upgrade_finish( );
        return FIRMWARE_FAILED;
    }

    target_buf = (int *) kzalloc(target_len * sizeof(int), GFP_KERNEL);
    if (target_buf == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to malloc target buffer.(chain = %d)\n",
                  chain);
        cpld_upgrade_finish( );
        return FIRMWARE_FAILED;
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("cpld_check_upgrade_data start.(chain = %d, %d)\n",
                  chain, target_len);
    /* Remove extraneous information */
    ret = cpld_check_upgrade_data(info, len, target_buf, target_len);
    if (ret < 0){
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to check data.(chain = %d)\n",
                  chain);
        kfree(target_buf);
        cpld_upgrade_finish( );
        return FIRMWARE_FAILED;
    }

    for (i = 0; i < 16 * 8; i += 8) {
        FIRMWARE_DRIVER_DEBUG_VERBOSE(" %x %x %x %x %x %x %x %x\n",
                  target_buf[i], target_buf[i + 1],
                  target_buf[i + 2], target_buf[i + 3],
                  target_buf[i + 4], target_buf[i + 5],
                  target_buf[i + 6], target_buf[i + 7]);
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("cpld_check_upgrade_data finish.(chain = %d)\n", chain);

    /* CPLD device writing */
    for (time = 0; time < 10; time++) {
        FIRMWARE_DRIVER_DEBUG_VERBOSE("Start upgrade cpld: %d.(chain = %d)\n", time, chain);
        ret = cpld_program(current_fmw_cpld->chip_index, target_buf);
        if (ret >= 0) {
            break;
        }
    }
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to program.(chain = %d)\n", chain);
        kfree(target_buf);
        cpld_upgrade_finish( );
        return FIRMWARE_FAILED;
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("SUCCESS PROGRAM.\n");

    /* Release GPIO */
    ret = cpld_upgrade_finish();
    if (ret != FIRMWARE_SUCCESS) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to program when finish upgrade.(chain = %d)\n",
                  chain);
    }

    kfree(target_buf);
    return FIRMWARE_SUCCESS;
}

/**
 * fmw_cpld_upg_program_jbi
 * function: Upgrade CPLD(JBI file format)
 * @chain: param[in] chain
 * @cpld: param[in] Device private data
 * @info: param[in] Data to be written
 * @len:  param[in] Length of data to be written
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int fmw_cpld_upg_program_jbi(int chain, firmware_cpld_t *cpld, char *info, int len)
{
    int time, ret;
    int argc = 3;
    char *argv[] = {
        "-r",
        "-aprogram",
        "-ddo_real_time_isp=1"
    };

    /* Check the input parameters */
    if (chain < 0 || info == NULL || len <= 0) {
        return FIRMWARE_FAILED;
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Cpld driver to program chip %d(%p,%p,%d).\n",
        chain, cpld, info, len);

    if (cpld == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to get gpio info.(chain = %d)\n", chain);
    } else {
        set_currrent_cpld_info(cpld);
    }

    if (chain != current_fmw_cpld->chain) {
        FIRMWARE_DRIVER_DEBUG_ERROR("The chain num is not fit.(chain = %d, current chain = %d)\n",
            chain, current_fmw_cpld->chain);
    }
    /* Initialize GPIO and CPLD */
    ret = cpld_upgrade_init( );
    if (ret != FIRMWARE_SUCCESS) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to program when init upgrade.(chain = %d)\n",
            chain);
        return FIRMWARE_FAILED;
    }

    /* reset JTAG */
    cpld_reset(current_fmw_cpld->chip_index);

    for (time = 0; time < 30; time++) {
        FIRMWARE_DRIVER_DEBUG_VERBOSE("Start upgrade cpld: %d.(chain = %d)\n", time, chain);
        ret = jbi_main((unsigned char *) info, (unsigned long) len, argc, argv);
        if (ret == 0) {
            break;
        }
    }
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to program.(chain = %d)\n", chain);
        cpld_upgrade_finish( );
        return FIRMWARE_FAILED;
    }
    FIRMWARE_DRIVER_DEBUG_VERBOSE("SUCCESS PROGRAM.\n");

    /* Release GPIO and CPLD */
    ret = cpld_upgrade_finish( );
    if (ret != FIRMWARE_SUCCESS) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to program when finish upgrade.(chain = %d)\n",
                  chain);
    }

    return FIRMWARE_SUCCESS;
}

/**
 * fmw_cpld_upg_get_version
 * function: get version
 * @chain: param[in]  chain
 * @cpld: param[in]  Device private data
 * @len:  param[in]  Data length
 * @info: param[out] Version information buffer
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int fmw_cpld_upg_get_version(int chain, firmware_cpld_t *cpld, char *info, int len)
{
    int ret;

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Cpld driver to get version.\n");
    if (cpld == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to get gpio info.(chain = %d)\n", chain);
    } else {
        set_currrent_cpld_info(cpld);
    }

    if (chain != current_fmw_cpld->chain) {
        FIRMWARE_DRIVER_DEBUG_ERROR("The chain num is not fit.(chain = %d, current chain = %d)\n",
                  chain, current_fmw_cpld->chain);
    }

    /* CPLD device can't get version */
    if (function_fmw_cpld.get_version != NULL) {
        ret = function_fmw_cpld.get_version(chain, info, len);
        if (ret < 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed get version in chain: %d.\n", chain);
            return FIRMWARE_FAILED;
        }

        return FIRMWARE_SUCCESS;
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("The get_version is NULL in chain: %d.\n", chain);
    }

    return FIRMWARE_FAILED;
}

/**
 * fmw_cpld_upg_get_chip_info
 * function: Get chip content
 * @chain:  param[in]  chain
 * @cpld:  param[in]  Device private data
 * @len:   param[in]  Data length
 * @info:  param[out] Read Data Buffer
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int fmw_cpld_upg_get_chip_info(int chain, firmware_cpld_t *cpld, void *info, int len)
{
    int i;
    int ret;
    int target_len;
    int *target_buf;

    /* Check input and output parameters */
    if (chain < 0 || info == NULL || len <= 0) {
        return FIRMWARE_FAILED;
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Cpld driver to read chip.\n");

    if (cpld == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to get gpio info.(chain = %d)\n", chain);
    } else {
        set_currrent_cpld_info(cpld);
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Cpld driver to read chip: %s.\n",current_fmw_cpld->devname);
    if (chain != current_fmw_cpld->chain) {
        FIRMWARE_DRIVER_DEBUG_ERROR("The chain num is not fit.(chain = %d, current chain = %d)\n",
                  chain, current_fmw_cpld->chain);
    }

    /* Initialize GPIO and CPLD */
    ret = cpld_upgrade_init( );
    if (ret != FIRMWARE_SUCCESS) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to program when init upgrade.(chain = %d)\n",
                  chain);
        return FIRMWARE_FAILED;
    }

    /* reset JTAG*/
    cpld_reset(current_fmw_cpld->chip_index);
    /* CPLD chip capacity size */
    target_len = cpld_eeprom_size();
    if (target_len <= 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to get cpld size.(chain = %d)\n",
                  chain);
        cpld_upgrade_finish( );
        return FIRMWARE_FAILED;
    }

    target_buf = (int *) kzalloc(target_len * sizeof(int), GFP_KERNEL);
    if (target_buf == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to malloc target buffer.(chain = %d)\n",
                  chain);
        cpld_upgrade_finish( );
        return FIRMWARE_FAILED;
    }
    /* Read chip */
    cpld_read_buffer(current_fmw_cpld->chip_index, target_buf, target_len);

    for (i = 0; i < 16 * 8; i += 8) {
        FIRMWARE_DRIVER_DEBUG_VERBOSE(" %x %x %x %x %x %x %x %x\n",
                  target_buf[i], target_buf[i + 1],
                  target_buf[i + 2], target_buf[i + 3],
                  target_buf[i + 4], target_buf[i + 5],
                  target_buf[i + 6], target_buf[i + 7]);
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Success Read.\n");

    /* Release GPIO and CPLD */
    ret = cpld_upgrade_finish( );
    if (ret != FIRMWARE_SUCCESS) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to program when finish upgrade.(chain = %d)\n",
                  chain);
    }

    if (copy_to_user(info, target_buf, (len > target_len) ? target_len : len)) {
        kfree(target_buf);
        return FIRMWARE_FAILED;
    }

    kfree(target_buf);
    return FIRMWARE_SUCCESS;
}

/**
 * jbi_jtag_io_
 * function: JBI GPIO operation
 * @tms: param[in]  TMS signal level
 * @tdi: param[in]  TDI signal level
 * @read_tdo:  param[in] Whether to read the level of the TDO
 * return value : tdo
 */
int __attribute__ ((weak)) jbi_jtag_io_(int tms, int tdi, int read_tdo)
{
    int tdo = 0;

    if (tms) {
        TMS_PULL_UP();
    } else {
        TMS_PULL_DOWN();
    }

    if (tdi) {
        TDI_PULL_UP();
    } else {
        TDI_PULL_DOWN();
    }

    TCK_PULL_UP();
    ndelay(TCK_DELAY);

    if (read_tdo) {
        tdo = TDO_READ();
    }

    TCK_PULL_DOWN();
    ndelay(TCK_DELAY);

    return tdo;
}
