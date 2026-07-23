/*
 * $Id: custom_led.c$
 * $Copyright: (c) 2022 Broadcom.
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

/* The led bitstream */
#define LED_BIT_BLINK                    0b00
#define LED_BIT_GREEN                    0b10
#define LED_BIT_AMBER                    0b01
#define LED_BIT_OFF                    0b11

/* Number of ports mapped to a port LED */
#define MAX_PORTS_PER_LED                4

/* The time window of activity LED displaying on */
#define ACT_TICKS                        2

/* OSFP ports defined software flag. */
#define LED_SW_LINK_UP                   0x1

/* mgnt ports defined software flag. */
#define LED_HW_LINK                      0x100

/* define the max physical port number used, include mgnt. */
#define MAX_LED_UC_PORT                  130

/* define the max physical port number for mgnt */
#define MAX_LED_MGNT_PORT                2

/* lport is mangment port or not */
#define IS_MGNT_PORT(lport)              (lport >= (MAX_LED_UC_PORT - MAX_LED_MGNT_PORT))

/* led bitstream base addr and width */
#define LED_SEND_DATA_WIDTH              2

/* define LED custom data get/set */
#define LED_CONTROL_DATA_LANE_SPEED_GET(led_control_data)  ((led_control_data >> 1) & 0x7)

/* The mapping table used to translate front port lane index to physical port number */
const uint16 dport_map[MAX_LED_UC_PORT]={   1,   2,   3,   4,   5,   6,   7,   8,
                                            9,  10,  11,  12,  13,  14,  15,  16,
                                           17,  18,  19,  20,  21,  22,  23,  24,
                                           25,  26,  27,  28,  29,  30,  31,  32,
                                           33,  34,  35,  36,  37,  38,  39,  40,
                                           41,  42,  43,  44,  45,  46,  47,  48,
                                           49,  50,  51,  52,  53,  54,  55,  56,
                                           57,  58,  59,  60,  61,  62,  63,  64,
                                          193, 194, 195, 196, 197, 198, 199, 200,
                                          201, 202, 203, 204, 205, 206, 207, 208,
                                          209, 210, 211, 212, 213, 214, 215, 216,
                                          217, 218, 219, 220, 221, 222, 223, 224,
                                          225, 226, 227, 228, 229, 230, 231, 232,
                                          233, 234, 235, 236, 237, 238, 239, 240,
                                          241, 242, 243, 244, 245, 246, 247, 248,
                                          249, 250, 251, 252, 253, 254, 255, 256,
                                          257, 258 };

/* buff used to save color */
static uint8 colors[MAX_LED_UC_PORT - MAX_LED_MGNT_PORT];

/*
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
    uint8 idx, color, led_ctrl=0;
    uint16 accu_val;
    uint16 i, lport, led_uc_port, pos, mgnt_pos;
    for (lport = 0; lport < MAX_LED_UC_PORT; lport ++) {
        /* led_uc_port = physical port number - constant. The constant is 1 for ESW chips */
        led_uc_port = dport_map[lport] - 1;
        if (lport == MAX_LED_UC_PORT - 1) {
            led_uc_port = dport_map[lport];
        }
        /* Read value from accumulation RAM */
        accu_val = LED_HW_RAM_READ16(ctrl->accu_ram_base, led_uc_port);
        led_ctrl = ctrl->led_control_data[led_uc_port];

        color = LED_BIT_OFF;

        //if ((accu_val & LED_HW_LINK_UP) && (led_ctrl & LED_SW_LINK_UP)) {
        if (accu_val & LED_HW_LINK) {
            color = LED_BIT_GREEN;
            if ((accu_val & (LED_HW_RX | LED_HW_TX)) && (cnt & ACT_TICKS)) {
                color = LED_BIT_BLINK;
            }
        }

        if (IS_MGNT_PORT(lport)) {
            mgnt_pos = lport - (MAX_LED_UC_PORT - MAX_LED_MGNT_PORT) + 32;
            if (color == LED_BIT_BLINK) {
                color = LED_BIT_OFF;
            }
            LED_HW_RAM_WRITE16(ctrl->pat_ram_base, mgnt_pos, color);
        } else {
            colors[lport] = color;
        }
    }

    for (lport = 0, pos = 0; lport < (MAX_LED_UC_PORT - MAX_LED_MGNT_PORT); lport += MAX_PORTS_PER_LED, pos++) {
        color = colors[lport];
        for (i = 1; i < MAX_PORTS_PER_LED; i++) {
            if (color == LED_BIT_BLINK) {
                break;
            }
            if (colors[lport + i] == LED_BIT_OFF) {
                continue;
            }
            color = colors[lport + i];
        }

        LED_HW_RAM_WRITE16(ctrl->pat_ram_base, pos, color);
    }

    /* Configure LED HW interfaces based on board configuration */
    for (idx = 0; idx < LED_HW_INTF_MAX_NUM; idx++) {
        soc_led_intf_ctrl_t *lic = &ctrl->intf_ctrl[idx];
        switch (idx) {
            case 0:

                /* Send the pattern over LED interface 0 for fornt LED 0 - 31 */
                lic->valid = 1;
                lic->start_row = 0;
                lic->end_row = 33;
                lic->pat_width = LED_SEND_DATA_WIDTH;
                break;
            case 1:

                /* Send the pattern over LED interface 1 for fornt LED 32 - 63 */
                lic->valid = 0;
                break;
            case 2:

                /* Send the pattern over LED interface 2 for fornt sfp28 LED */
                lic->valid = 0;
                break;
            default:
                /* Invalidate rest of the interfaces */
                lic->valid = 0;        
                break;
        }
    }

    return; 
}   
