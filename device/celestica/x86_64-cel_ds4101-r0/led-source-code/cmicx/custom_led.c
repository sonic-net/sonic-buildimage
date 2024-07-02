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

/*! The time window of activity LED displaying on. */
#define ACT_TICKS        2


/*! Customer defined software flag. */
#define LED_SW_LINK_UP   0x1


#define FRONT_PORT_MAX 34

#define LED_AMBER_BICOLOR 0x1 //bit : 01
#define LED_GREEN_BICOLOR 0x2 //bit : 10
#define LED_OFF_BICOLOR   0x3 //bit : 11

uint16 phy_port[] = {0,   13,  9,   5,   1,  29,  25,  21,  17,  37, 33,   45,  41,  53,  49,  61,  57,
                         69,  65,  77,  73,  85,  81,  93,  89,  101, 97, 109, 105, 117, 113, 125, 121,
                        133, 129, 141, 137, 149, 145, 157, 153, 165, 161, 173, 169, 181, 177, 189, 185,
                        197, 193, 205, 201, 221, 217, 213, 209, 237, 233, 229, 225, 253, 249, 245, 241, 
                        258, 257 };

void
customer_led_handler(soc_led_custom_handler_ctrl_t *ctrl, uint32 cnt)
{
    uint16 front_port, physical_port;
    uint16 led_uc_port;
    uint8 led_control_data0, led_control_data1, led_control_data2, led_control_data3;
    uint16 accu_val0 = 0, accu_val1 =0 , accu_val2 =0, accu_val3 = 0;
    uint16 led_tx_rx0 = 0, led_tx_rx1 =0 , led_tx_rx2 =0, led_tx_rx3 = 0;
    uint16 pattern = 0;
    uint16 intf;
    uint8 idx;

    /* Process all fornt ports. */
   for(front_port = 1; front_port < (FRONT_PORT_MAX*2-1); front_port++) {

            physical_port = phy_port[front_port];
            if(physical_port == TH4G_MGMT2_LED_UC_PORT)
            {

                led_uc_port = physical_port;

            }
            else
            {

                led_uc_port = (physical_port - 1);

            }

            accu_val0 = LED_HW_RAM_READ16(ctrl->accu_ram_base, led_uc_port );
            accu_val1 = LED_HW_RAM_READ16(ctrl->accu_ram_base, (led_uc_port + 1));
            accu_val2 = LED_HW_RAM_READ16(ctrl->accu_ram_base, (led_uc_port + 2));
            accu_val3 = LED_HW_RAM_READ16(ctrl->accu_ram_base, (led_uc_port + 3));

            led_tx_rx0 = accu_val0 & (LED_HW_RX | LED_HW_TX);
            led_tx_rx1 = accu_val1 & (LED_HW_RX | LED_HW_TX);
            led_tx_rx2 = accu_val2 & (LED_HW_RX | LED_HW_TX);
            led_tx_rx3 = accu_val3 & (LED_HW_RX | LED_HW_TX);

            led_control_data0 = ctrl->led_control_data[(physical_port - 1)];
            led_control_data1 = ctrl->led_control_data[(physical_port - 1 + 1)];
            led_control_data2 = ctrl->led_control_data[(physical_port - 1 + 2)];
            led_control_data3 = ctrl->led_control_data[(physical_port - 1 + 3)];


            if((led_tx_rx0 | led_tx_rx1 | led_tx_rx2 | led_tx_rx3) && (cnt & ACT_TICKS) ){

                 pattern = LED_OFF_BICOLOR;
               
             }
            else if ((led_control_data0 | led_control_data1 | led_control_data2 | led_control_data3) & LED_SW_LINK_UP){

                 pattern = LED_GREEN_BICOLOR;

            }else{

                 pattern = LED_OFF_BICOLOR;

            }

            LED_HW_RAM_WRITE16(ctrl->pat_ram_base, (front_port -1), pattern);

    }


    /* Configure LED HW interfaces based on board configuration */
    for (idx = 0; idx < LED_HW_INTF_MAX_NUM; idx++) {
        soc_led_intf_ctrl_t *lic = &ctrl->intf_ctrl[idx];
        switch (idx) {
        case 0:
            lic->valid = 1;
            lic->start_row = 0;
            lic->end_row = (FRONT_PORT_MAX * 2 - 1);
            lic->pat_width = 2;
            break;
        case 1:
            lic->valid = 0;
            lic->start_row = 0;
            lic->end_row = 0;
            lic->pat_width = 0;
            break;
        default:
            lic->valid = 0;
            break;
        }
    }

    return;
}
