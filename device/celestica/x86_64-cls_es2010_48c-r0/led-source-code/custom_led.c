/*
 * $Id: custom_led.c$
 * $Copyright: (c) 2024 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 * File:        custom_led.c
 * Purpose:     Customer CMICx LED bit pattern composer.
 * Requires:
 */

/******************************************************************************
 *
 * The CMICx LED interface has two RAM Banks as shown below, Bank0
 * (Accumulation RAM) for accumulation of status from ports and Bank1
 * (Pattern RAM) for writing LED pattern. Both Bank0 and Bank1 are of
 * 1024x16-bit, each row representing one port.
 *
 *           Accumulation RAM (Bank 0)        Pattern RAM (Bank1)
 *          15                       0     15                          0
 *         ----------------------------   ------------------------------
 * Row 0   |  led_uc_port 0 status    |   | led_uc_port 0 LED Pattern   |
 *         ----------------------------   ------------------------------
 * Row 1   |  led_uc_port 1 status    |   | led_uc_port 1 LED Pattern   |
 *         ----------------------------   ------------------------------
 *         |                          |   |                             |
 *         ----------------------------   ------------------------------
 *         |                          |   |                             |
 *         ----------------------------   ------------------------------
 *         |                          |   |                             |
 *         ----------------------------   ------------------------------
 *         |                          |   |                             |
 *         ----------------------------   ------------------------------
 *         |                          |   |                             |
 *         ----------------------------   ------------------------------
 * Row 127 |  led_uc_port 128 status  |   | led_uc_port 128 LED Pattern |
 *         ----------------------------   ------------------------------
 * Row 128 |                          |   |                             |
 *         ----------------------------   ------------------------------
 *         |                          |   |                             |
 *         ----------------------------   ------------------------------
 *         |                          |   |                             |
 *         ----------------------------   ------------------------------
 * Row x   |  led_uc_port (x+1) status|   | led_uc_port(x+1) LED Pattern|
 *         ----------------------------   ------------------------------
 *         |                          |   |                             |
 *         ----------------------------   ------------------------------
 *         |                          |   |                             |
 *         ----------------------------   ------------------------------
 * Row 1022|  led_uc_port 1022 status |   | led_uc_port 1022 LED Pattern|
 *         ----------------------------   ------------------------------
 * Row 1023|  led_uc_port 1023 status |   | led_uc_port 1023 LED Pattern|
 *         ----------------------------   ------------------------------
 *
 * Format of Accumulation RAM:
 *
 * Bits   15:9       8        7         6        5      4:3     2    1    0
 *    ------------------------------------------------------------------------
 *    | Reserved | Link  | Link-up |  Flow  | Duplex | Speed | Col | Tx | Rx |
 *    |          | Enable| Status  | Control|        |       |     |    |    |
 *    ------------------------------------------------------------------------
 *
 * Where Speed 00 - 10 Mbps
 *             01 - 100 Mbps
 *             10 - 1 Gbps
 *             11 - Above 1 Gbps
 *
 * The customer handler in this file should read the port status from
 * the HW Accumulation RAM or "led_control_data" array, then form the required
 * LED bit pattern in the Pattern RAM at the corresponding location.
 *
 * The "led_control_data" is a 1024 bytes array, application user can use BCM LED API
 * to exchange port information with LED FW.
 *
 * Typically, led_uc_port = physical port number - constant.
 * The constant is 1 for ESW chips, 0 for DNX/DNXF chips and 2 for Firelight.
 * For those ports that do not meet the above rule, they will be listed in
 * "include/shared/cmicfw/cmicx_led_public.h".
 *
 * There are five LED interfaces in CMICx-based devices, and although
 * a single interface can be used to output LED patterns for all
 * ports, it is possible to use more than one interface, e.g. the LEDs
 * for some ports are connected to LED interface-0, while the rest of
 * the ports are connected to LED interface-1. Accordingly, the custom
 * handler MUST fill in start-port, end-port and pattern-width in the
 * soc_led_custom_handler_ctrl_t structure passed to the custom
 * handler.
 *
 * The example custom handler provided in this file has reference code
 * for forming two different LED patterns. Please refer to these
 * patterns before writing your own custom handler code.
 *
 * The led_customer_t structure definition is available in
 * include/shared/cmicfw/cmicx_led_public.h.
 *
 ******************************************************************************/

#include <shared/cmicfw/cmicx_led_public.h>

/*****************************************
 *  Customer defintion.
 *****************************************/

/*! The time window of activity LED displaying on. */
#define ACT_TICKS        2

/*! Customer defined software flag. */
#define LED_SW_LINK_UP   0x1

/*
*Format for pattern format definition for per RJ45 2xLED, low level light
*Per led have 2 bits
*Bits 3           2           1           0
*  -------------------------------------------------
*  | Right LED | Right LED | Left LED  | Left LED |
*  | YELLOW    | GREEN     | YELLOW    | GREEN    |
*  -------------------------------------------------
* Set bit (0 for ON, 1 for OFF), 0xe mean Left green on, Right LED off.

/*
*Format for pattern format definition for per QSFP28 4xLED, low level light
*Per led have 2 bits
*Bits  7       6        5       4      3         2       1       0
*  ----------------------------------------------------------------------
*  | LED4   | LED4  | LED3   | LED3  | LED2   | LED2  | LED1   | LED1  |
*  | YELLOW | GREEN | YELLOW | GREEN | YELLOW | GREEN | YELLOW | GREEN |
*  ----------------------------------------------------------------------
* Set bit (0 for ON, 1 for OFF), 0xfe mean LED1 green on, LED2, LED3, LED4 off.
*/


#define RJ45_PORT_MAX         48
#define QSFP28_PORT_MAX         6

#define RJ45_ALL_LED_OFF  0xf
#define RJ45_LEFT_LED_GREEN_ON  0xe
#define RJ45_LEFT_LED_AMBER_ON  0xd

/*The right LED will BLINK only when the left LED is ON.*/
#define RJ45_RIGHT_LED_GREEN_BLINK  0xa
#define RJ45_RIGHT_LED_AMBER_BLINK  0x5

#define QSFP28_ALL_LED_OFF  0xff
#define QSFP28_LED1_GREEN_ON  0xfe

unsigned short phyPortMap[] = {
     54,  53,  56,  55,  58,  57,  60,  59,
     62,  61,  64,  63,  66,  65,  68,  67,
     70,  69,  72,  71,  74,  73,  76,  75,
     78,  77,  80,  79,  2,   1,   4,   3,
     6,   5,   8,   7,   10,  9,   12,  11,
     14,  13,  16,  15,  18,  17,  20,  19,
     25,  21,  29,  33,  41,  45
};

/*!
 * \brief Function for LED bit pattern generator.
 *
 * Customer can compose the LED bit pattern to control serial LED
 * according to link/traffic information.
 *
 * \param [in,out] ctrl Data structure indicating the locations of the
 *                      port status and serial LED bit pattern RAM.
 * \param [in] cnt 30Hz counter.
 *
 */
void
customer_led_handler(soc_led_custom_handler_ctrl_t *ctrl, uint32 cnt)
{
    uint16 led_uc_port, front_port = 0;
    uint16 intf;
    uint16 accu_val = 0, pattern = 0;
    uint8 idx;
    uint16 tmp_port;
    uint8 led_control_data = 0;

    /* Process all fornt ports. */
    /* for RJ45_PORT_MAX, one port two led */
    for (front_port = 1; front_port <= RJ45_PORT_MAX ; front_port++) {
        pattern = 0;
        led_uc_port = phyPortMap[front_port-1] - 1;

        led_control_data = ctrl->led_control_data[(led_uc_port)];

        if (led_control_data & LED_SW_LINK_UP) {// link up
                /* Read value from accumulation RAM */
            accu_val = LED_HW_RAM_READ16(ctrl->accu_ram_base, led_uc_port);

            pattern = RJ45_LEFT_LED_GREEN_ON;
            if (cnt & ACT_TICKS) {
                pattern = RJ45_LEFT_LED_GREEN_ON;
            }else if ((accu_val & LED_HW_RX) || (accu_val & LED_HW_TX)) {
                pattern = RJ45_RIGHT_LED_GREEN_BLINK;
            }else {
                pattern = RJ45_LEFT_LED_GREEN_ON;
            }

        }else{ // Link down -> All Off
            pattern = RJ45_ALL_LED_OFF;
        }
        /* Write value to pattern RAM */
        LED_HW_RAM_WRITE16(ctrl->pat_ram_base, front_port, pattern);
    }

    /* for QSFP28 port*/
    for ( tmp_port = 0; tmp_port < QSFP28_PORT_MAX ; tmp_port++) {
        pattern = 0;
        front_port = (RJ45_PORT_MAX + 1) + tmp_port;
        led_uc_port = phyPortMap[front_port-1] - 1;
        led_control_data = ctrl->led_control_data[(led_uc_port)];

        if (led_control_data & LED_SW_LINK_UP) {// link up
                /* Read value from accumulation RAM */
            accu_val = LED_HW_RAM_READ16(ctrl->accu_ram_base, led_uc_port);

            pattern = QSFP28_LED1_GREEN_ON;

            // activity is always green
            if ((accu_val & LED_HW_RX) || (accu_val & LED_HW_TX)) {
                pattern = QSFP28_LED1_GREEN_ON;
                if (cnt & ACT_TICKS) {
                    pattern = QSFP28_ALL_LED_OFF;
                }
            }

        }else{
            pattern = QSFP28_ALL_LED_OFF;
        }

        /* Write value to pattern RAM */
        LED_HW_RAM_WRITE16(ctrl->pat_ram_base, front_port, pattern);
    }

    /* Configure LED HW interfaces based on board configuration */
    for (idx = 0; idx < LED_HW_INTF_MAX_NUM; idx++) {
        soc_led_intf_ctrl_t *lic = &ctrl->intf_ctrl[idx];
        switch (idx) {
        case 0:
            lic->valid = 1;
            lic->start_row = 1;
            lic->end_row = 48;
            lic->pat_width = 4;
            break;
        case 1:
            lic->valid = 1;
            lic->start_row = 49;
            lic->end_row = 54;
            lic->pat_width = 8;
            break;
        default:
            /* Invalidate rest of the interfaces */
            lic->valid = 0;
            break;
        }
    }

    return;
}
