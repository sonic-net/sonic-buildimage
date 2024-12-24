/*
 * $Id: custom_led.c$
 * $Copyright: (c) 2019 Broadcom
 * Broadcom Proprietary and Confidential. All rights reserved.$
 */

/******************************************************************************
CMICX LED Interface has two RAM Banks, as shown below, Bank0(ACCUMULATION RAM)
for accumulation of status from ports and Bank1(PATTERN RAM) for writing
LED pattern. Both Bank0 and Bank1 are of 1024x16-bit, each row representing
one port.

         ACCUMULATION RAM (Bank 0)                Pattern RAM (Bank1)
         15                         0          15                         0
        ------------------------------         -----------------------------
Row 0   |  Port1 status              |         |  Port1 LED Pattern        |
        ------------------------------         -----------------------------
Row 1   |  Port2 status              |         |  Port2 LED Pattern        |
        ------------------------------         -----------------------------
        |                            |         |                           |
        ------------------------------         -----------------------------
        |                            |         |                           |
        ------------------------------         -----------------------------
        |                            |         |                           |
        ------------------------------         -----------------------------
        |                            |         |                           |
        ------------------------------         -----------------------------
        |                            |         |                           |
        ------------------------------         -----------------------------
Row 127 |  Port128 status            |         |  Port128 LED Pattern      |
        ------------------------------         -----------------------------
Row 128 |                            |         |                           |
        ------------------------------         -----------------------------
        |                            |         |                           |
        ------------------------------         -----------------------------
        |                            |         |                           |
        ------------------------------         -----------------------------
Row x   |  Port(x+1) status          |         |  Port(x+1) LED Pattern    |
        ------------------------------         -----------------------------
        |                            |         |                           |
        ------------------------------         -----------------------------
        |                            |         |                           |
        ------------------------------         -----------------------------
Row 1022|  Port1023 status           |         |  Port1023 LED Pattern     |
        ------------------------------         -----------------------------
Row 1023|  Port1024 status           |         |  Port1024 LED Pattern     |
        ------------------------------         -----------------------------

Format of Accumulation RAM:


Bits    15:9        8       7         6        5       4:3     2    1    0
     ------------------------------------------------------------------------
     | Reserved | Link  | Link Up |  Flow  | Duplex | Speed | Col | Tx | Rx |
     |          | Enable| Status  | Control|        |       |     |    |    |
     ------------------------------------------------------------------------

The custom handler in this file should read port status, for each port used,
from accumulation ram, and form required LED bit pattern in the Bank1 RAM
(pattern RAM) location corresponding to the port of interest. Note that
physical port numbers may differ from row number of LED RAM Banks. For
Trident3, Physical port numbers spread from 1 to 128 in 128x25G configuration
 and corresponding LED rows spread from Row 0 to Row 127.

There are five LED interfaces in CMICX based devices. Although single
interface can be used to output LED pattern for all ports, it is possible
that more than one interface can be used in the end system, e.g., LEDs for
some ports are connected to one LED interface-0 (i.e LED_CLK and LED_DATA),
while the rest of the ports are connected to LED interface-1. Accordingly,
custom handler MUST fill in start port, end port and width of pattern in the
soc_led_custom_handler_ctrl_t structure passsed to custom handler. The
example custom handler provided in this file has reference code for forming
two different LED patterns. Please refer to these patterns before writing your
own custom handler code.

The soc_led_custom_handler_ctrl_t structure definition is available in
$SDK/include/shared/cmicfw/cmicx_led_public.h

soc_led_custom_handler_ctrl_t structure also carries a point to array
port_speed[] of size equal to maximum ports in the system, e.g 128 in Trident3.
This array would have port speed for each port, as per bit mapping defined in
"soc_led_speed_t" in $SDK/include/shared/cmicfw/cmicx_led_public.h file.

Here is an exception, please keep in mind:
1. For TH3, port status/speed of  xe1 (physical port 258) is located in the
   accumulation entry/speed array  of physical port 259.

******************************************************************************/
#include <shared/cmicfw/cmicx_led_public.h>

#define ACTIVITY_TICKS 2

/*! Macro to calculate LED RAM address. */
#define LED_HW_RAM_ADDR(base, port) \
    (base + (port * sizeof(uint32)))

/*! Macro to read LED RAM. */
#define LED_HW_RAM_READ16(base, port) \
    *((uint16 *) LED_HW_RAM_ADDR(base, port))

/*! Macro to write LED RAM. */
#define LED_HW_RAM_WRITE16(base, port, val)  \
    *((uint16 *) LED_HW_RAM_ADDR(base, port)) = (val)

#define PORT_NUM_TOTAL 56

#define LED_GREEN_BICOLOR 0x2 //bit : 10
#define LED_AMBER_BICOLOR 0x1 //bit : 01
#define LED_OFF_BICOLOR   0x3 //bit : 11
#define LED_SW_LINK_UP    0x1

unsigned short portmap[] = {
     25,  26,  27,  28,  29,  30,  31,  32,
     33,  34,  35,  36,  37,  38,  39,  40,
     41,  42,  43,  44,  49,  50,  51,  52,
      1,   2,   3,   4,   5,   6,   7,   8,
      9,  10,  11,  12,  24,  23,  22,  21,
     20,  19,  18,  17,  16,  15,  14,  13,
     60,  58,  59,  57,  62,  64,  61,  63
};


/*
 * Function:
 *      custom_led_handler
 * Purpose:
 *      Timer event handler to accumulate, process and transmit led status
 * Parameters:
 *      param - parameter added while registering the timer event.
 * Returns:
 *      0 on success
 *      Error code on failure
 */
void custom_led_handler(soc_led_custom_handler_ctrl_t *ctrl,
                        uint32 activity_count)
{
    uint8 idx = 0;
    uint16 accu_val = 0, send_val = 0;
    uint16 uc_port = 0, physical_port = 0;

    /* Physical port numbers to be used */
    for(uc_port = 0; uc_port < PORT_NUM_TOTAL; uc_port++) {

	// change to zero-based
	physical_port = portmap[uc_port] - 1;

        /* Read value from led_ram bank0 */
        accu_val = LED_HW_RAM_READ16(ctrl->accu_ram_base, physical_port);

        send_val = LED_OFF_BICOLOR;

        if (((accu_val & LED_HW_RX) || (accu_val & LED_HW_TX)) && (activity_count & ACTIVITY_TICKS))
        {
            send_val = LED_OFF_BICOLOR;
        }
        else if (ctrl->led_control_data[physical_port] & LED_SW_LINK_UP)
        {
            send_val = LED_GREEN_BICOLOR;
        }
        else
        {
            send_val = LED_OFF_BICOLOR;
        }

	/* Write value to led_ram bank1 */
        LED_HW_RAM_WRITE16(ctrl->pat_ram_base, uc_port, send_val);
    } /* for */

    /* Configure LED HW interfaces based on board configuration */
    for (idx = 0; idx < LED_HW_INTF_MAX_NUM; idx++) {
        soc_led_intf_ctrl_t *lic = &ctrl->intf_ctrl[idx];
        switch (idx) {
        case 0:
	        lic->valid = 0;
            break;
        case 1:
	        lic->valid = 1;
            lic->start_row = 0;
            lic->end_row = 55;
            lic->pat_width = 2;
            break;
        default:

            /* Invalidate rest of the interfaces */
            lic->valid = 0;
            break;
        }
    }

    return;

}
