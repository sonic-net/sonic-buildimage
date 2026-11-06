#ifndef _SUTRA_CIC_H
#define _SUTRA_CIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>

#define UINT8_C(c)  __UINT8_C(c)

/*
 * The SUTRA supports many interrupt sources, which are routed to an interrupt
 * request line of the MPIC.
 */

typedef enum {
	SUTRA_INT_MIN,

	// ---- SUTRA_GLUE_ISR_SW_I2C ----			//  bit hwirq
	SUTRA_INT_SW_I2C_MB_DONE = SUTRA_INT_MIN,		//    0     0
	SUTRA_INT_SW_I2C_PS_DONE,				//    1     1
	SUTRA_INT_SW_I2C_PCIE_DONE,				//    2     2
	SUTRA_INT_SW_I2C_PMBUS_DONE,				//    3     3
	SUTRA_INT_SW_I2C_UNUSED_4,				//    4     4
	SUTRA_INT_SW_I2C_UNUSED_5,				//    5     5
	SUTRA_INT_SW_I2C_UNUSED_6,				//    6     6
	SUTRA_INT_SW_I2C_UNUSED_7,				//    7     7

	// ---- SUTRA_GLUE_ISR_MISC ----			//  bit hwirq
	SUTRA_INT_MISC_UNUSED_0,				//    0     8
	SUTRA_INT_MISC_USB,					//    1     9
	SUTRA_INT_MISC_TPM,					//    2    10
	SUTRA_INT_MISC_CPU_QUAD1,				//    3    11
	SUTRA_INT_MISC_CPU_BOARD_WARM_ALERT,			//    4    12
	SUTRA_INT_MISC_CPU_BOARD_HOT_ALERT,			//    5    13
	SUTRA_INT_MISC_UNUSED_6,				//    6    14
	SUTRA_INT_MISC_UNUSED_7,				//    7    15

	// ---- SUTRA_GLUE_ISR_PWR ----				//  bit hwirq
	SUTRA_INT_PWR_PSA_ABS,					//    0    16
	SUTRA_INT_PWR_PSB_ABS,					//    1    17
	SUTRA_INT_PWR_PSA_PWR_OK,				//    2    18
	SUTRA_INT_PWR_PSB_PWR_OK,				//    3    19
	SUTRA_INT_PWR_PSA_ACOK_H,				//    4    20
	SUTRA_INT_PWR_PSB_ACOK_H,				//    5    21
	SUTRA_INT_PWR_UNUSED_6,					//    6    22
	SUTRA_INT_PWR_UNUSED_7,					//    7    23

	// ---- SUTRA_GLUE_ISR_BUTTON ----			//  bit hwirq
	SUTRA_INT_PB_RESET_PRESS_DETECT,			//    0    24
	SUTRA_INT_PB_RESET_HELD_FOR_3_SECONDS,			//    1    25
	SUTRA_INT_BUTTON_UNUSED_2,				//    2    26
	SUTRA_INT_BUTTON_UNUSED_3,				//    3    27
	SUTRA_INT_BUTTON_UNUSED_4,				//    4    28
	SUTRA_INT_BUTTON_UNUSED_5,				//    5    29
	SUTRA_INT_BUTTON_UNUSED_6,				//    6    30
	SUTRA_INT_BUTTON_UNUSED_7,				//    7    31

	SUTRA_INT_MAX
} siril_pin;

#define CIENA_GPIO_ACTIVE_LOW sutra_cic_active_low

#ifdef __cplusplus
}
#endif
#endif /* _SUTRA_CIC_H */
