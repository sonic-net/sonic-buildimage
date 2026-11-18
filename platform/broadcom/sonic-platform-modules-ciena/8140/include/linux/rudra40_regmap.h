#ifndef RUDRA40_REGMAP_H
#define RUDRA40_REGMAP_H

#ifdef __KERNEL__
#ifdef CONFIG_REGMAP_MMIO
#include <linux/regmap.h>
#endif
#else
#include <stddef.h>
#include <inttypes.h>
#endif

struct Rudra40_dev_reg
{
    uint32_t    RUDRA40_BASE_FID;           /* RUDRA40 FPGA ID */
    uint32_t    RUDRA40_BASE_MJR;           /* Major revision number */
    uint32_t    RUDRA40_BASE_MNR;           /* Minor revision number */
    uint32_t    RUDRA40_BASE_BLD;           /* Build revision number */
    uint32_t    RUDRA40_BASE_SCRATCHPAD;    /* Scratchpad test register */
    uint32_t    RUDRA40_BASE_FPGA_DATE;     /* Used for development purposes. */
    uint32_t    RUDRA40_BASE_FPGA_RECONFIG; /* FPGA Reconfiguation Trigger control */
    uint32_t    RUDRA40_BASE_MPU_BRD_RESET; /* Board Power Sequencing Trigger */
    uint32_t    RUDRA40_BASE_MPU_ISSU_RESET_EN; /* ISSU Support */
    uint32_t    RUDRA40_BASE_ENDIANESS;     /* Endianess of Data */
    uint32_t    RUDRA40_BASE_BRD_ID;        /* Board ID/Revision & load indicator, used to identify which board is using the RUDRA40 design */
    uint32_t    RUDRA40_BASE_EXT_FPGA_CFG_STATUS; /* External FPGA Configuration Status Register */
    uint32_t    RUDRA40_BASE_EXT_FPGA_CFG_CTL; /* External FPGA Configuration Control Register */
    uint32_t    RUDRA40_BASE_CLK_DEBUG_0;   /* Measured Frequencies of Input Clocks 0 */
    uint32_t    RUDRA40_BASE_CLK_DEBUG_1;   /* Measured Frequencies of Input Clocks 1 */
    uint32_t    RUDRA40_BASE_REGMAP_RESET_DEBUG; /* High and Low Counts of the Regmap Reset Line */
    uint32_t    RUDRA40_BASE_PCIE_RESET_DEBUG; /* High and Low Counts of the PCIE Reset Line */
    uint32_t    RUDRA40_BASE_PLL_LOCKED_DEBUG; /* Stability counters for PLL Locked outputs post milli-second filter */
    uint32_t    RUDRA40_BASE_I2C_SCRATCHPAD; /* Scratchpad2 register. This Register is mirrored to scratchpad register(0x7) defined in i2c_regmap.in */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_0; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_0; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_1; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_1; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_2; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_2; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_3; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_3; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_4; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_4; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_5; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_5; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_6; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_6; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_7; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_7; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_8; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_8; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_9; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_9; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_10; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_10; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_11; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_11; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_12; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_12; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_13; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_13; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_14; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_14; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_15; /* Reset History - Reserved for future use */
    uint32_t    RUDRA40_BASE_RESET_HISTORY_TS_UPR_15; /* Reset History Timestamp, upper bits 48-16 - Reserved for future use */
    uint32_t    RUDRA40_BASE_CURRENT_UPTIME_UPR; /* Local Free-running Timer, upper bits 63-32, Reading this register latches the lower bits as well. (Reserved for future use) */
    uint32_t    RUDRA40_BASE_CURRENT_UPTIME_LWR; /* Local Free-running Timer, lower bits 31-0 (Reserved for future use) */
    uint8_t pad1[40];

    uint32_t    RUDRA40_BASE_THEEND;        /* Reserved */
    uint8_t pad2[512];

    uint32_t    RUDRA40_RESERVED_THESTART;  /* Reserved */
    uint32_t    RUDRA40_RESERVED_THEEND;    /* Reserved */
    uint8_t pad3[248];

    uint32_t    RUDRA40_GLUE_SOFT_RESET;    /* Software Requested Register Map reset */
    uint32_t    RUDRA40_GLUE_RESERVED2;     /* Reserved */
    uint32_t    RUDRA40_GLUE_RESERVED3;     /* Reserved */
    uint32_t    RUDRA40_GLUE_GENERAL_STATUS; /* General Board Status */
    uint32_t    RUDRA40_GLUE_PWR_CTL;       /* S/W controlled Power controls. For DEBUG use only. */
    uint32_t    RUDRA40_GLUE_PCIE_RESET_MASK; /* PCIe Reset Control Mask */
    uint32_t    RUDRA40_GLUE_DEVICE_RESET;  /* Reset Controls */
    uint32_t    RUDRA40_GLUE_GENERAL_CTL;   /* General Board Controls */
    uint32_t    RUDRA40_GLUE_LED_SYS_STATUS; /* Global and Daughter Card LED Controls, Note: RUDRA40 specific */
    uint32_t    RUDRA40_GLUE_RESERVED4;     /* reserved */
    uint32_t    RUDRA40_GLUE_RESERVED5;     /* reserved */
    uint32_t    RUDRA40_GLUE_RESERVED6;     /* reserved */
    uint32_t    RUDRA40_GLUE_RESERVED7;     /* reserved */
    uint32_t    RUDRA40_GLUE_MSI_CTRL;      /* MSI Control Register */
    uint32_t    RUDRA40_GLUE_IST_MASTER_EVENT; /* Interrupt Test Debug register */
    uint32_t    RUDRA40_GLUE_ISR_MASTER_EVENT; /* Master Interrupt Event Register */
    uint32_t    RUDRA40_GLUE_ISM_MASTER_EVENT; /* Master Interrupt Mask Register */
    uint32_t    RUDRA40_GLUE_ISR_SW_I2C;    /* Interrupt from State change for SW I2C Masters */
    uint32_t    RUDRA40_GLUE_ISM_SW_I2C;    /* Interrupt Mask Register for SW I2C Masters */
    uint32_t    RUDRA40_GLUE_ISR_MISC;      /* Interrupt from State change for Miscellaneous Elements. Note: STATUS_MISC is later in regmap */
    uint32_t    RUDRA40_GLUE_ISM_MISC;      /* Interrupt Mask Register for Miscellaneous Elements */
    uint32_t    RUDRA40_GLUE_STATUS_MISC;   /* Status for Miscellaneous Elements */
    uint32_t    RUDRA40_GLUE_I2C_SW_IF_SEL; /* SW I2C Interface Select */
    uint32_t    RUDRA40_GLUE_I2C_SW_MISC_CTRL; /* SW I2C clock Control and Interface Select */
    uint32_t    RUDRA40_GLUE_J2CA_SRD_REFCLK; /* Measured Frequencies of Input Reference Clocks */
    uint32_t    RUDRA40_GLUE_RESERVED;      /*  */
    uint32_t    RUDRA40_GLUE_IST_SW_I2C;    /* Interrupt Test Debug register */
    uint32_t    RUDRA40_GLUE_IST_MISC;      /* Interrupt Test Debug register */
    uint32_t    RUDRA40_GLUE_DEBUG_CPU_WDT; /* Debug Test mode only. Allows debug of the cpu Hardware Watchdog Timer */
    uint32_t    RUDRA40_GLUE_UART_SEL;      /* Uart from faceplate connected to 1 of 5 uarts */
    uint32_t    RUDRA40_GLUE_SPI_SEL;       /* Spi controller selection for configuring Flash */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RATE_1; /* QSFP Rate (100g/200g/400g) leds for QSFP PORTS [5:1], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RATE_2; /* QSFP Rate (100g/200g/400g) leds for QSFP PORTS [8:6], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RATE_3; /* QSFP Rate (100g/200g/400g) leds for QSFP PORTS [15:11], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RATE_4; /* QSFP Rate (100g/200g/400g) leds for QSFP PORTS [20:16], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RATE_5; /* QSFP Rate (100g/200g/400g) leds for QSFP PORTS [25:21], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RATE_6; /* QSFP Rate (100g/200g/400g) leds for QSFP PORTS [30:26], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RATE_7; /* QSFP Rate (100g/200g/400g) leds for QSFP PORTS [35:31], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RATE_8; /* QSFP Rate (100g/200g/400g) leds for QSFP PORTS [36], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RED_1; /* QSFP Activity red led for QSFP PORTS [8:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RED_2; /* QSFP Activity red led for QSFP PORTS [8:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RED_3; /* QSFP Activity red led 1 for QSFP PORTS [36:19]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_QSFP_RED_4; /* QSFP Activity red led 2 for QSFP PORTS [36:19]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_QSFP_GREEN_1; /* QSFP Activity GREEN led for QSFP PORTS [8:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_QSFP_GREEN_2; /* QSFP Activity GREEN led for QSFP PORTS [8:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_QSFP_GREEN_3; /* QSFP Activity GREEN led 1 for QSFP PORTS [36:19]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_QSFP_GREEN_4; /* QSFP Activity GREEN led 2 for QSFP PORTS [36:19]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_QSFP_BLUE_1; /* QSFP Activity BLUE led for QSFP PORTS [8:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_QSFP_BLUE_2; /* QSFP Activity BLUE led for QSFP PORTS [8:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_QSFP_BLUE_3; /* QSFP Activity BLUE led 1 for QSFP PORTS [36:19]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_QSFP_BLUE_4; /* QSFP Activity BLUE led 2 for QSFP PORTS [36:19]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_HW_TESTING_CTL; /* General Board Controls for HW testing */
    uint32_t    RUDRA40_GLUE_LED_FW_DATA0;  /* Led Data[95:0] coming from FW. For debug use. */
    uint32_t    RUDRA40_GLUE_LED_FW_DATA1;  /*  */
    uint32_t    RUDRA40_GLUE_LED_FW_DATA2;  /*  */
    uint32_t    RUDRA40_GLUE_LED_FW_DATA3;  /*  */
    uint32_t    RUDRA40_GLUE_LED_FW_DATA4;  /*  */
    uint32_t    RUDRA40_GLUE_LED_FW_DATA5;  /*  */
    uint32_t    RUDRA40_GLUE_LED_FW_DATA6;  /*  */
    uint32_t    RUDRA40_GLUE_LED_FW_DATA7;  /*  */
    uint32_t    RUDRA40_GLUE_LED_FW_DATA8;  /*  */
    uint32_t    RUDRA40_GLUE_PWR_GOOD_STATUS; /* On board power good values through the SIRIL_IOEXP_PG I2C bus using autonomous polling. This register retain its value when PWRGD_I2C_SW_OVERRIDE=1 */
    uint32_t    RUDRA40_GLUE_MISC_TESTING_CTL; /* Miscellaneous testing control bits */
    uint32_t    RUDRA40_GLUE_CPLD_SGPIO_DATA0; /* Data received from Sutra CPLD over the SGPIO interface. */
    uint32_t    RUDRA40_GLUE_CPLD_SGPIO_DATA1; /*  */
    uint32_t    RUDRA40_GLUE_CPLD_SGPIO_DATA2; /*  */
    uint32_t    RUDRA40_GLUE_CPLD_SGPIO_DATA3; /*  */
    uint32_t    RUDRA40_GLUE_RESERVED_SGPIO; /* Reserved for future use */
    uint32_t    RUDRA40_GLUE_TEST_SGPIO_CTL; /* Control register to test SGPIO interface. */
    uint32_t    RUDRA40_GLUE_RESERVED0;     /* Unused. */
    uint32_t    RUDRA40_GLUE_RESERVED1;     /*  */
    uint32_t    RUDRA40_GLUE_PMBUS1_VI_MON_DATA0; /* Read Data Register */
    uint32_t    RUDRA40_GLUE_PMBUS1_VI_MON_DATA1; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS1_VI_MON_DATA2; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS1_VI_MON_DATA3; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS1_VI_MON_DATA4; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS1_VI_MON_DATA5; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS1_VI_MON_DATA6; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS1_VI_MON_DATA7; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS1_VI_MON_DATA8; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS1_VI_MON_DATA9; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS1_VI_MON_DATA10; /*  */
    uint32_t    RUDRA40_GLUE_IOEXP_PLL_LOCKS; /* Read PLL LOCKS status from io-expander */
    uint32_t    RUDRA40_GLUE_ADC_SPI_CONV_VALUE0; /* 16-bit Conversion values of 16 ADC-SPI channels for J2C socket monitoring through real time polling */
    uint32_t    RUDRA40_GLUE_ADC_SPI_CONV_VALUE1; /*  */
    uint32_t    RUDRA40_GLUE_ADC_SPI_CONV_VALUE2; /*  */
    uint32_t    RUDRA40_GLUE_ADC_SPI_CONV_VALUE3; /*  */
    uint32_t    RUDRA40_GLUE_ADC_SPI_CONV_VALUE4; /*  */
    uint32_t    RUDRA40_GLUE_ADC_SPI_CONV_VALUE5; /*  */
    uint32_t    RUDRA40_GLUE_ADC_SPI_CONV_VALUE6; /*  */
    uint32_t    RUDRA40_GLUE_ADC_SPI_CONV_VALUE7; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS2_VI_MON_DATA0; /* Read Data Register */
    uint32_t    RUDRA40_GLUE_PMBUS2_VI_MON_DATA1; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS2_VI_MON_DATA2; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS2_VI_MON_DATA3; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS2_VI_MON_DATA4; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS2_VI_MON_DATA5; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS2_VI_MON_DATA6; /*  */
    uint32_t    RUDRA40_GLUE_PMBUS2_VI_MON_DATA7; /*  */
    uint32_t    RUDRA40_GLUE_HBM_SUPPLIES_RESEQ; /* Reserved for future use */
    uint32_t    RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG0; /* Data sent from RUDRA40 to BMC over the SGPIO interface. */
    uint32_t    RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG1; /*  */
    uint32_t    RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG2; /*  */
    uint32_t    RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG3; /*  */
    uint32_t    RUDRA40_GLUE_HBM_POWER_ENABLE_THRESHOLD; /* Reserved for future use */
    uint32_t    RUDRA40_GLUE_QSFPDD_CLKEN_0; /* Clk Enable for Qsfpdd(EPPS clock) */
    uint32_t    RUDRA40_GLUE_ETH_TRAFFIC_STATUS; /* Ethernet Traffic Status Register */
    uint32_t    RUDRA40_GLUE_OPTICS_ADC_EPPS_SPI_SELECT; /* Selection of SPI lines */
    uint32_t    RUDRA40_GLUE_GLUE_DEBUG_REG_0; /* Glue Debug Register 0 */
    uint32_t    RUDRA40_GLUE_GLUE_DEBUG_REG_1; /* Glue Debug Register 1 */
    uint32_t    RUDRA40_GLUE_GLUE_DEBUG_REG_2; /* Glue Debug Register 2 */
    uint32_t    RUDRA40_GLUE_GLUE_DEBUG_REG_3; /* Glue Debug Register 3 */
    uint32_t    RUDRA40_GLUE_GLUE_DEBUG_REG_4; /* Glue Debug Register 4 */
    uint32_t    RUDRA40_GLUE_GLUE_DEBUG_REG_5; /* Glue Debug Register 5 */
    uint32_t    RUDRA40_GLUE_GLUE_DEBUG_REG_6; /* Glue Debug Register 6 */
    uint32_t    RUDRA40_GLUE_GLUE_DEBUG_REG_7; /* Glue Debug Register 7 */
    uint32_t    RUDRA40_GLUE_GLUE_DEBUG_REG_8; /* Glue Debug Register 8 */
    uint32_t    RUDRA40_GLUE_GLUE_DEBUG_REG_9; /* Glue Debug Register 9 */
    uint32_t    RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG0; /* Data sent from Rudra40 to CPLD over the SGPIO interface. */
    uint32_t    RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG1; /*  */
    uint32_t    RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG2; /*  */
    uint32_t    RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG3; /*  */
    uint32_t    RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0; /* Test frame to be sent on SGPIO interface */
    uint32_t    RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1; /*  */
    uint32_t    RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2; /*  */
    uint32_t    RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3; /*  */
    uint32_t    RUDRA40_GLUE_LED_SFP_RATE_1; /* SFP Rate (10g/25g/50g/100g) leds for SFP PORTS [5:1], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_SFP_RATE_2; /* SFP Rate (10g/25g/50g/100g) leds for SFP PORTS [10:6], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_SFP_RATE_3; /* SFP Rate (10g/25g/50g/100g) leds for SFP PORTS [15:11], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_SFP_RATE_4; /* SFP Rate (10g/25g/50g/100g) leds for SFP PORTS [20:16], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_SFP_RATE_5; /* SFP Rate (10g/25g/50g/100g) leds for SFP PORTS [25:21], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_SFP_RATE_6; /* SFP Rate (10g/25g/50g/100g) leds for SFP PORTS [30:26], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_SFP_RATE_7; /* SFP Rate (10g/25g/50g/100g) leds for SFP PORTS [35:31], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_SFP_RATE_8; /* SFP Rate (10g/25g/50g/100g) leds for SFP PORTS [40:36], 6 bits per port for 2 LEDs per port */
    uint32_t    RUDRA40_GLUE_LED_SFP_RED_1; /* SFP Activity red led 1 for SFP PORTS [20:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_SFP_RED_2; /* SFP Activity red led 2 for SFP PORTS [20:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_SFP_RED_3; /* SFP Activity red led 1 for SFP PORTS [40:21]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_SFP_RED_4; /* SFP Activity red led 2 for SFP PORTS [40:21]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_SFP_GREEN_1; /* SFP Activity GREEN led 1 for SFP PORTS [20:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_SFP_GREEN_2; /* SFP Activity GREEN led 2 for SFP PORTS [20:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_SFP_GREEN_3; /* SFP Activity GREEN led 1 for SFP PORTS [40:21]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_SFP_GREEN_4; /* SFP Activity GREEN led 2 for SFP PORTS [40:21]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_SFP_BLUE_1; /* SFP Activity BLUE led 1 for SFP PORTS [20:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_SFP_BLUE_2; /* SFP Activity BLUE led 1 for SFP PORTS [20:1]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_SFP_BLUE_3; /* SFP Activity BLUE led 1 for SFP PORTS [40:21]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_LED_SFP_BLUE_4; /* SFP Activity BLUE led 2 for SFP PORTS [40:21]. Used for testing, override data coming from BCM FRAME when bit 26 of LED_SYS_STATUS is set */
    uint32_t    RUDRA40_GLUE_GEARBOX_MDC_SPEED; /* MDC Speed Controls */
    uint32_t    RUDRA40_GLUE_GEARBOX_MDIO_CMD; /* Clause 45 or 22 MDIO Command IF */
    uint32_t    RUDRA40_GLUE_GEARBOX_MDIO_RDAT; /* MDIO Data IF */
    uint32_t    RUDRA40_GLUE_THEEND;        /* Reserved */
    uint8_t pad4[424];

    uint32_t    RUDRA40_TIME_DIST_TIMING_STATUS; /* GPS/BITS/TOD Status register */
    uint32_t    RUDRA40_TIME_DIST_TIMING_CTL; /* GPS/BITS/TOD Control register */
    uint32_t    RUDRA40_TIME_DIST_TIMING_MUX_SEL; /* Multiplexer control for timing outputs */
    uint32_t    RUDRA40_TIME_DIST_J2CA_TIMECODE_0; /* ***NOTE*** Work In PROGRESS!! Captured ML time code bits[96:80] */
    uint32_t    RUDRA40_TIME_DIST_J2CA_TIMECODE_1; /* ***NOTE*** Work In PROGRESS!! Captured ML time code bits[79:48] */
    uint32_t    RUDRA40_TIME_DIST_J2CA_TIMECODE_2; /* ***NOTE*** Work In PROGRESS!! Captured ML time code bits[47:16] */
    uint32_t    RUDRA40_TIME_DIST_J2CA_TIMECODE_3; /* ***NOTE*** Work In PROGRESS!! Captured ML time code bits[15:0] */
    uint32_t    RUDRA40_TIME_DIST_TIMING_TEST_SIG_GEN; /* Timing Test Signal Generation */
    uint32_t    RUDRA40_TIME_DIST_TIMING_TEST_FREQ; /* Timing Test Frequency Reporting */
    uint32_t    RUDRA40_TIME_DIST_SQUELCH;  /* Enables the FPGA to squelch the recovered clock being sent to the ZL30603 */
    uint8_t pad5[88];

    uint32_t    RUDRA40_TIME_DIST_SHIFT_1PPS_REGS; /* 1pps shift logic Configuration RAM */
    uint8_t pad6[52];

    uint32_t    RUDRA40_TIME_DIST_GPS_10MHZ_FREQ; /* GPS 10MHZ input clock Frequency counter */
    uint32_t    RUDRA40_TIME_DIST_GPS_PPS_FREQ; /* GPS 1pps input Frequency counter */
    uint32_t    RUDRA40_TIME_DIST_RESERVED_0; /* ***NOTE*** Work In PROGRESS!! Captured ML time code bits[96:80] */
    uint32_t    RUDRA40_TIME_DIST_RESERVED_1; /* ***NOTE*** Work In PROGRESS!! Captured ML time code bits[79:48] */
    uint32_t    RUDRA40_TIME_DIST_RESERVED_2; /* ***NOTE*** Work In PROGRESS!! Captured ML time code bits[47:16] */
    uint32_t    RUDRA40_TIME_DIST_RESERVED_3; /* ***NOTE*** Work In PROGRESS!! Captured ML time code bits[15:0] */
    uint32_t    RUDRA40_TIME_DIST_THEEND;   /* Reserved */
    uint8_t pad7[44];

    uint32_t    RUDRA40_FLASH_SPI_DATA;     /* Processor to SPI data register */
    uint8_t pad8[4];

    uint32_t    RUDRA40_FLASH_SPI_CTRL;     /* Processor to SPI interface control register */
    uint32_t    RUDRA40_FLASH_SPI_WRITE_FIFO_FILL_LEVEL; /* number of words in Flash SPI write data FIFO */
    uint32_t    RUDRA40_FLASH_SPI_READ_FIFO_FILL_LEVEL; /* number of words in Flash SPI read data FIFO */
    uint32_t    RUDRA40_FLASH_SPI_ERROR;    /* Flash SPI interface ERROR register */
    uint8_t pad9[40];

    uint32_t    RUDRA40_ADC_SPI_DATA;       /* Processor to SPI data register */
    uint8_t pad10[4];

    uint32_t    RUDRA40_ADC_SPI_CTRL;       /* Processor to SPI interface control register */
    uint32_t    RUDRA40_ADC_SPI_WRITE_FIFO_FILL_LEVEL; /* number of words in Flash SPI write data FIFO */
    uint32_t    RUDRA40_ADC_SPI_READ_FIFO_FILL_LEVEL; /* number of words in Flash SPI read data FIFO */
    uint32_t    RUDRA40_ADC_SPI_ERROR;      /* Flash SPI interface ERROR register */
    uint8_t pad11[40];

    uint32_t    RUDRA40_SKT_MON_SPI_DATA;   /* Processor to SPI data register */
    uint8_t pad12[4];

    uint32_t    RUDRA40_SKT_MON_SPI_CTRL;   /* Processor to SPI interface control register */
    uint32_t    RUDRA40_SKT_MON_SPI_WRITE_FIFO_FILL_LEVEL; /* number of words in Flash SPI write data FIFO */
    uint32_t    RUDRA40_SKT_MON_SPI_READ_FIFO_FILL_LEVEL; /* number of words in Flash SPI read data FIFO */
    uint32_t    RUDRA40_SKT_MON_SPI_ERROR;  /* Flash SPI interface ERROR register */
    uint8_t pad13[40];

    uint32_t    RUDRA40_J2CA_SPI_DATA;      /* Processor to SPI data register */
    uint8_t pad14[4];

    uint32_t    RUDRA40_J2CA_SPI_CTRL;      /* Processor to SPI interface control register */
    uint32_t    RUDRA40_J2CA_SPI_WRITE_FIFO_FILL_LEVEL; /* number of words in Flash SPI write data FIFO */
    uint32_t    RUDRA40_J2CA_SPI_READ_FIFO_FILL_LEVEL; /* number of words in Flash SPI read data FIFO */
    uint32_t    RUDRA40_J2CA_SPI_ERROR;     /* Flash SPI interface ERROR register */
    uint8_t pad15[104];

    uint32_t    RUDRA40_EPPS_SPI_DATA;      /* Processor to SPI data register */
    uint8_t pad16[4];

    uint32_t    RUDRA40_EPPS_SPI_CTRL;      /* Processor to SPI interface control register */
    uint32_t    RUDRA40_EPPS_SPI_WRITE_FIFO_FILL_LEVEL; /* number of words in Flash SPI write data FIFO */
    uint32_t    RUDRA40_EPPS_SPI_READ_FIFO_FILL_LEVEL; /* number of words in Flash SPI read data FIFO */
    uint32_t    RUDRA40_EPPS_SPI_ERROR;     /* Flash SPI interface ERROR register */
    uint8_t pad17[424];

    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_OVERRIDE; /* IO Expander I2C control to switch b/w HW and SW */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0; /* I2C control Register for accessing IO Expander I2C interfaces */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_4X0; /* clock divider factor (for HW use only) based on 100mhz clock source */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_4X1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_4X2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_4X3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_4X4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_4X5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR0; /* Device Address used for I2C cycle, bit 0 is the Page bit for page operations */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR0; /* Data Address Register used for I2C cycle. */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD0; /* Byte Valid Register */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG0; /* Debug register */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR0; /* Write Data Register */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD0; /* Read Data Register */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0; /* I2C Bus Control Register */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_STAT0; /* I2C Bus Status Register */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_STAT1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_STAT2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_STAT3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_STAT4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_STAT5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA0; /* I2C Bus Write Data Register */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA0; /* I2C Bus Read Data Register */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_DONE0; /* I2C Bus Completion Status Register */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_DONE1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_DONE2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_DONE3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_DONE4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_DONE5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL0; /* SW I2C Interface Select */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL0; /* SW I2C clock Control and Interface Select */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_SW_CTRL;   /* I2C Bus Control Register */
    uint32_t    RUDRA40_MAIN_I2C_SW_STAT;   /* I2C Bus Status Register */
    uint32_t    RUDRA40_MAIN_I2C_SW_WRITE_DATA; /* I2C Bus Write Data Register */
    uint32_t    RUDRA40_MAIN_I2C_SW_READ_DATA; /* I2C Bus Read Data Register */
    uint32_t    RUDRA40_MAIN_I2C_SW_DONE;   /* I2C Bus Completion Status Register */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_CTRL; /* I2C control Register for accessing Mainboard I2C interfaces */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_4X;   /* clock divider factor (for HW use only) based on 50mhz clock source */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DEVADDR; /* Device Address Register used for I2C cycle, bit 0 is the Page bit for page operations on the AT24CM01 on this bus */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATAADDR; /* Data Address Register used for I2C cycle. */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_VLD; /* Byte Valid Register */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DEBUG; /* Debug register */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR0; /* Write Data Registers */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR6; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR7; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR8; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR9; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR10; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR11; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR12; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR13; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR14; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR15; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR16; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR17; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR18; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR19; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR20; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR21; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR22; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR23; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR24; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR25; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR26; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR27; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR28; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR29; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR30; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR31; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR32; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR33; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR34; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR35; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR36; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR37; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR38; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR39; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR40; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR41; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR42; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR43; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR44; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR45; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR46; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR47; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR48; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR49; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR50; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR51; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR52; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR53; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR54; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR55; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR56; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR57; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR58; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR59; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR60; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR61; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR62; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_WR63; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD0; /* Read Data Registers */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD1; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD2; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD3; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD4; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD5; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD6; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD7; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD8; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD9; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD10; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD11; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD12; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD13; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD14; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD15; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD16; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD17; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD18; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD19; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD20; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD21; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD22; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD23; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD24; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD25; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD26; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD27; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD28; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD29; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD30; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD31; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD32; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD33; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD34; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD35; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD36; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD37; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD38; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD39; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD40; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD41; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD42; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD43; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD44; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD45; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD46; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD47; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD48; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD49; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD50; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD51; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD52; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD53; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD54; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD55; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD56; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD57; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD58; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD59; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD60; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD61; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD62; /*  */
    uint32_t    RUDRA40_MAIN_I2C_DIAG_DATA_RD63; /*  */
    uint8_t pad18[100];

    uint32_t    RUDRA40_MAIN_I2C_THEEND;    /* Reserved */
    uint8_t pad19[256];

    uint32_t    RUDRA40_RJ45_UART_DATA;     /* Processor Write-to/Read-from UART data register */
    uint8_t pad20[4];

    uint32_t    RUDRA40_RJ45_UART_CTRL;     /* Control Register for SW access to RJ45 UART */
    uint32_t    RUDRA40_RJ45_UART_WRITE_FIFO_DEPTH; /* number of bytes in the 32-location deep Uart write data FIFO */
    uint32_t    RUDRA40_RJ45_UART_READ_FIFO_DEPTH; /* number of bytes in 32-location deep Uart read data FIFO */
    uint8_t pad21[44];

    uint32_t    RUDRA40_FREE_RUN_TOD_RTC;   /* Time of Day in Seconds - NTP format, 64 bits wide - will be copied into TOD counter on the next 1pps pulse */
    uint8_t pad22[4];

    uint32_t    RUDRA40_FREE_RUN_TOD_RTC_OFST3; /* RTC offset register [63:32] - See explanation for bits [31:00]. */
    uint32_t    RUDRA40_FREE_RUN_TOD_RTC_OFST1; /* RTC offset register [31:00] - When initializing or adjusting the time of the RTC, the magnitude of the desired offset from the current time is written to this and the previous register. */
    uint32_t    RUDRA40_FREE_RUN_TOD_RTC_OFST_CNT; /* RTC offset count register. */
    uint32_t    RUDRA40_FREE_RUN_TOD_RTC_EPO_OFST; /* NTP to PTP Epoch offset register [31:0] - 32bit value indicating the number of seconds between 1/1/1900 and 1/1/1970 */
    uint32_t    RUDRA40_FREE_RUN_TOD_RTC_DRFT; /* RTC drift adjust register - This register may be written at any time and the new drift correction will be applied at the next occurrence of the hardware's drift correction event, which is based on a free-running counter and happens every approx. 786uS. */
    uint32_t    RUDRA40_FREE_RUN_TOD_RTC_CTRL; /* RTC control register */
    uint32_t    RUDRA40_FREE_RUN_TOD_RTC_OFST_CTRL; /* RTC offset count register */
    uint8_t pad23[28];

    uint32_t    RUDRA40_SW_UART_DATA;       /* Processor Write-to/Read-from UART data register */
    uint8_t pad24[4];

    uint32_t    RUDRA40_SW_UART_CTRL;       /* Control Register for SW access to GPS UART */
    uint32_t    RUDRA40_SW_UART_WRITE_FIFO_DEPTH; /* number of words in the 32-location deep Uart write data FIFO */
    uint32_t    RUDRA40_SW_UART_READ_FIFO_DEPTH; /* number of words in 32-location deep Uart read data FIFO */
    uint8_t pad25[1644];

    uint32_t    RUDRA40_MORE_I2C_RESERVED_1; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_2; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_3; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_4; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_5; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_6; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_7; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_8; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_9; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_10; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_11; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_12; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_13; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_14; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_15; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_RESERVED_16; /* Reserved */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_SW_CTRL; /* I2C Bus Control Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_SW_STAT; /* I2C Bus Status Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_SW_WRITE_DATA; /* I2C Bus Write Data Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_SW_READ_DATA; /* I2C Bus Read Data Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_SW_DONE; /* I2C Bus Completion Status Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_SW_IF_SEL; /* SW I2C Interface Select */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_SW_MISC_CTRL; /* SW I2C clock Control and Interface Select */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL; /* I2C control Register for accessing Mainboard I2C interfaces */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_DIAG_4X; /* clock divider factor (for HW use only) based on 100mhz clock source */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_DIAG_DEVADDR; /* Device Address used for I2C cycle, bit 0 is the Page bit for page operations */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_DIAG_DATAADDR; /* Data Address Register used for I2C cycle. */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_VLD; /* Byte Valid Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_DIAG_DEBUG; /* Debug register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_WR; /* Write Data Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_RD; /* Read Data Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS1_I2C_SW_OVERRIDE; /* PMBUS1 I2C control to switch b/w HW and SW */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_SW_CTRL; /* I2C Bus Control Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_SW_STAT; /* I2C Bus Status Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_SW_WRITE_DATA; /* I2C Bus Write Data Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_SW_READ_DATA; /* I2C Bus Read Data Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_SW_DONE; /* I2C Bus Completion Status Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_SW_IF_SEL; /* SW I2C Interface Select */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_SW_MISC_CTRL; /* SW I2C clock Control and Interface Select */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL; /* I2C control Register for accessing Mainboard I2C interfaces */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_4X; /* clock divider factor (for HW use only) based on 100mhz clock source */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DEVADDR; /* Device Address used for I2C cycle, bit 0 is the Page bit for page operations */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATAADDR; /* Data Address Register used for I2C cycle. */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_VLD; /* Byte Valid Register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DEBUG; /* Debug register */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR0; /* Write Data Registers */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR1; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR2; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR3; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR4; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR5; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR6; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR7; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR8; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR9; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR10; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR11; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR12; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR13; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR14; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR15; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR16; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR17; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR18; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR19; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR20; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR21; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR22; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR23; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR24; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR25; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR26; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR27; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR28; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR29; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR30; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR31; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR32; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR33; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR34; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR35; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR36; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR37; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR38; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR39; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR40; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR41; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR42; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR43; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR44; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR45; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR46; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR47; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR48; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR49; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR50; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR51; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR52; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR53; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR54; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR55; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR56; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR57; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR58; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR59; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR60; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR61; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR62; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR63; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD0; /* Read Data Registers */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD1; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD2; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD3; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD4; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD5; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD6; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD7; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD8; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD9; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD10; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD11; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD12; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD13; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD14; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD15; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD16; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD17; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD18; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD19; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD20; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD21; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD22; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD23; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD24; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD25; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD26; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD27; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD28; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD29; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD30; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD31; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD32; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD33; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD34; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD35; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD36; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD37; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD38; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD39; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD40; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD41; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD42; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD43; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD44; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD45; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD46; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD47; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD48; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD49; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD50; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD51; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD52; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD53; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD54; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD55; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD56; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD57; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD58; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD59; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD60; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD61; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD62; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD63; /*  */
    uint32_t    RUDRA40_MORE_I2C_PMBUS2_I2C_SW_OVERRIDE; /* PMBUS2 I2C control to switch b/w HW and SW */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL; /* I2C Bus Control Register */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT; /* I2C Bus Status Register */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_SW_WRITE_DATA; /* I2C Bus Write Data Register */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_SW_READ_DATA; /* I2C Bus Read Data Register */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_SW_DONE; /* I2C Bus Completion Status Register */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_SW_IF_SEL; /* SW I2C Interface Select */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_SW_MISC_CTRL; /* SW I2C clock Control and Interface Select */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL; /* I2C control Register for accessing Mainboard I2C interfaces */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_4X; /* clock divider factor (for HW use only) based on 100mhz clock source */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEVADDR; /* Device Address used for I2C cycle, bit 0 is the Page bit for page operations */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATAADDR; /* Data Address Register used for I2C cycle. */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_VLD; /* Byte Valid Register */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEBUG; /* Debug register */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_WR; /* Write Data Register */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_RD; /* Read Data Register */
    uint32_t    RUDRA40_MORE_I2C_J2C_IOEXP_I2C_SW_OVERRIDE; /* J2C_IOEXP I2C control to switch b/w HW and SW */
    uint32_t    RUDRA40_MORE_I2C_POLLING_DLY; /* To change the autonomous polling delay value in hex in terms of number of 100us pulses for all expanders */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_SW_CTRL; /* I2C Bus Control Register */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_SW_STAT; /* I2C Bus Status Register */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_SW_WRITE_DATA; /* I2C Bus Write Data Register */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_SW_READ_DATA; /* I2C Bus Read Data Register */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_SW_DONE; /* I2C Bus Completion Status Register */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_SW_IF_SEL; /* SW I2C Interface Select */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_SW_MISC_CTRL; /* SW I2C clock Control and Interface Select */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL; /* I2C control Register for accessing Mainboard I2C interfaces */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_4X; /* clock divider factor (for HW use only) based on 100mhz clock source */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DEVADDR; /* Device Address used for I2C cycle, bit 0 is the Page bit for page operations */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATAADDR; /* Data Address Register used for I2C cycle. */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_VLD; /* Byte Valid Register */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DEBUG; /* Debug register */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR0; /* Write Data Register */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR1; /*  */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR2; /*  */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR3; /*  */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR4; /*  */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR5; /*  */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD0; /* Read Data Register */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD1; /*  */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD2; /*  */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD3; /*  */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD4; /*  */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD5; /*  */
    uint32_t    RUDRA40_MORE_I2C_PWRGD_I2C_SW_OVERRIDE; /* PWR GOOD IO Expander I2C control to switch b/w HW and SW */
    uint8_t pad26[156];

    uint32_t    RUDRA40_OPTICS_QSFP_LOW_PWR_0; /* QSFP low power select bits */
    uint32_t    RUDRA40_OPTICS_QSFP_LOW_PWR_1; /* QSFP low power select bits */
    uint32_t    RUDRA40_OPTICS_QSFP_RESET_0; /* QSFP reset control bits */
    uint32_t    RUDRA40_OPTICS_QSFP_RESET_1; /* QSFP reset control bits */
    uint32_t    RUDRA40_OPTICS_QSFP_PWR_EN_0; /* QSFP power enable control bits */
    uint32_t    RUDRA40_OPTICS_QSFP_PWR_EN_1; /* QSFP power enable control bits */
    uint32_t    RUDRA40_OPTICS_ISR_QSFP_PRESENT_0; /* Interrupt from State change for STATUS_QSFP_PRESENT */
    uint32_t    RUDRA40_OPTICS_ISR_QSFP_PRESENT_1; /* Interrupt from State change for STATUS_QSFP_PRESENT */
    uint32_t    RUDRA40_OPTICS_ISM_QSFP_PRESENT_0; /* Interrupt Mask Register for STATUS_QSFP_PRESENT */
    uint32_t    RUDRA40_OPTICS_ISM_QSFP_PRESENT_1; /* Interrupt Mask Register for STATUS_QSFP_PRESENT */
    uint32_t    RUDRA40_OPTICS_STATUS_QSFP_PRESENT_0; /* Status of QSFP+ Present signal */
    uint32_t    RUDRA40_OPTICS_STATUS_QSFP_PRESENT_1; /* Status of QSFP+ Present signal */
    uint32_t    RUDRA40_OPTICS_IST_QSFP_PRESENT_0; /* Interrupt Test Debug register */
    uint32_t    RUDRA40_OPTICS_IST_QSFP_PRESENT_1; /* Interrupt Test Debug register */
    uint32_t    RUDRA40_OPTICS_ISR_QSFP_LOS_0; /* Interrupt from State change for STATUS_QSFP_LOS */
    uint32_t    RUDRA40_OPTICS_ISR_QSFP_LOS_1; /* Interrupt from State change for STATUS_QSFP_LOS */
    uint32_t    RUDRA40_OPTICS_ISM_QSFP_LOS_0; /* Interrupt Mask Register for STATUS_QSFP_LOS */
    uint32_t    RUDRA40_OPTICS_ISM_QSFP_LOS_1; /* Interrupt Mask Register for STATUS_QSFP_LOS */
    uint32_t    RUDRA40_OPTICS_STATUS_QSFP_LOS_0; /* Status of ~QSFP28_PORT_INT_N */
    uint32_t    RUDRA40_OPTICS_STATUS_QSFP_LOS_1; /* Status of ~QSFP28_PORT_INT_N */
    uint32_t    RUDRA40_OPTICS_IST_QSFP_LOS_0; /* Interrupt Test Debug register */
    uint32_t    RUDRA40_OPTICS_IST_QSFP_LOS_1; /* Interrupt Test Debug register */
    uint32_t    RUDRA40_OPTICS_ISR_QSFP_PWR_GD_0; /* Interrupt from State change for STATUS_QSFP_PWR_GD */
    uint32_t    RUDRA40_OPTICS_ISR_QSFP_PWR_GD_1; /* Interrupt from State change for STATUS_QSFP_PWR_GD */
    uint32_t    RUDRA40_OPTICS_ISM_QSFP_PWR_GD_0; /* Interrupt Mask Register for STATUS_QSFP_PWR_GD */
    uint32_t    RUDRA40_OPTICS_ISM_QSFP_PWR_GD_1; /* Interrupt Mask Register for STATUS_QSFP_PWR_GD */
    uint32_t    RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_0; /* Status of STATUS_QSFP_PWR_GD */
    uint32_t    RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_1; /* Status of STATUS_QSFP_PWR_GD */
    uint32_t    RUDRA40_OPTICS_IST_QSFP_PWR_GD_0; /* Interrupt Test Debug register */
    uint32_t    RUDRA40_OPTICS_IST_QSFP_PWR_GD_1; /* Interrupt Test Debug register */
    uint32_t    RUDRA40_OPTICS_I2C_MUX_SEL; /* used to control I2C Muxing to QSFPDD on PCB. */
    uint32_t    RUDRA40_OPTICS_RESERVED0;   /* Reserved */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_CTRL; /* DIAGs I2C Master Control Registers for Optics only. */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_4X; /* clock divider factor (for HW use only) based on 100mhz clock source */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DEVADDR; /* Device Address Register used for I2C cycle */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATAADDR; /* Data Address Register used for I2C cycle. */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_VLD; /* Byte Valid Register */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DEBUG; /* Debug register */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR0; /* Write Data Registers */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR1; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR2; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR3; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR4; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR5; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR6; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR7; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR8; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR9; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR10; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR11; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR12; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR13; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR14; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR15; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR16; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR17; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR18; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR19; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR20; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR21; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR22; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR23; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR24; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR25; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR26; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR27; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR28; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR29; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR30; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR31; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR32; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR33; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR34; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR35; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR36; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR37; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR38; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR39; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR40; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR41; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR42; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR43; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR44; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR45; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR46; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR47; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR48; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR49; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR50; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR51; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR52; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR53; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR54; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR55; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR56; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR57; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR58; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR59; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR60; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR61; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR62; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_WR63; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD0; /* Read Data Registers */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD1; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD2; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD3; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD4; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD5; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD6; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD7; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD8; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD9; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD10; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD11; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD12; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD13; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD14; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD15; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD16; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD17; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD18; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD19; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD20; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD21; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD22; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD23; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD24; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD25; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD26; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD27; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD28; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD29; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD30; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD31; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD32; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD33; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD34; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD35; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD36; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD37; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD38; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD39; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD40; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD41; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD42; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD43; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD44; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD45; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD46; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD47; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD48; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD49; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD50; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD51; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD52; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD53; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD54; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD55; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD56; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD57; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD58; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD59; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD60; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD61; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD62; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_DIAG_DATA_RD63; /*  */
    uint32_t    RUDRA40_OPTICS_I2C0_SW_CTRL; /* SW I2C Bus Control Register */
    uint32_t    RUDRA40_OPTICS_I2C0_SW_STAT; /* I2C Bus Status Register */
    uint32_t    RUDRA40_OPTICS_I2C0_SW_WRITE_DATA; /* I2C Bus Write Data Register */
    uint32_t    RUDRA40_OPTICS_I2C0_SW_READ_DATA; /* I2C Bus Read Data Register */
    uint32_t    RUDRA40_OPTICS_I2C0_SW_DONE; /* I2C Bus Completion Status Register */
    uint32_t    RUDRA40_OPTICS_SFP_TX_DISABLE; /* SFP[31:0] Transmit Disable */
    uint32_t    RUDRA40_OPTICS_SFP_TX_DISABLE_2; /* SFP[39:32] Transmit Disable */
    uint32_t    RUDRA40_OPTICS_SFP_RATE_SELECT; /* SFP[31:0] Rate Select */
    uint32_t    RUDRA40_OPTICS_SFP_RATE_SELECT_2; /* SFP[39:32] Rate Select */
    uint32_t    RUDRA40_OPTICS_SFP_PWR_EN;  /* SFP[31:0] Power Enable */
    uint32_t    RUDRA40_OPTICS_SFP_PWR_EN_2; /* SFP[39:32] Power Enable */
    uint32_t    RUDRA40_OPTICS_SFPDD_TX_DISABLE; /* SFPDD[31:0] Transmit Disable */
    uint32_t    RUDRA40_OPTICS_SFPDD_TX_DISABLE_2; /* SFPDD[39:32] Transmit Disable */
    uint32_t    RUDRA40_OPTICS_SFPDD_RATE_SELECT; /* SFPDD[31:0] Rate Select */
    uint32_t    RUDRA40_OPTICS_SFPDD_RATE_SELECT_2; /* SFPDD[39:32] Rate Select */
    uint32_t    RUDRA40_OPTICS_ISR_SFP_TX_FAULT; /* Interrupt from State change for STATUS_SFP_TX_FAULT */
    uint32_t    RUDRA40_OPTICS_ISR_SFP_TX_FAULT_2; /* Interrupt from State change for STATUS_SFP_TX_FAULT */
    uint32_t    RUDRA40_OPTICS_ISM_SFP_TX_FAULT; /* Interrupt Mask Register for STATUS_SFP_TX_FAULT */
    uint32_t    RUDRA40_OPTICS_ISM_SFP_TX_FAULT_2; /* Interrupt Mask Register for STATUS_SFP_TX_FAULT */
    uint32_t    RUDRA40_OPTICS_STATUS_SFP_TX_FAULT; /* Status of SFP+ Tx Fault signal */
    uint32_t    RUDRA40_OPTICS_STATUS_SFP_TX_FAULT_2; /* Status of SFP+ Tx Fault signal */
    uint32_t    RUDRA40_OPTICS_IST_SFP_TX_FAULT; /* Interrupt Test Debug register SFP[31:0] */
    uint32_t    RUDRA40_OPTICS_IST_SFP_TX_FAULT_2; /* Interrupt Test Debug register SFP[39:32] */
    uint32_t    RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT; /* Interrupt from State change for STATUS_SFPDD_TX_FAULT */
    uint32_t    RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_2; /* Interrupt from State change for STATUS_SFPDD_TX_FAULT */
    uint32_t    RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT; /* Interrupt Mask Register for STATUS_SFPDD_TX_FAULT */
    uint32_t    RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT_2; /* Interrupt Mask Register for STATUS_SFPDD_TX_FAULT */
    uint32_t    RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT; /* Status of SFPDD Tx Fault signal */
    uint32_t    RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT_2; /* Status of SFPDD Tx Fault signal */
    uint32_t    RUDRA40_OPTICS_IST_SFPDD_TX_FAULT; /* Interrupt Test Debug register SFPDD[31:0] */
    uint32_t    RUDRA40_OPTICS_IST_SFPDD_TX_FAULT_2; /* Interrupt Test Debug register SFPDD[39:32] */
    uint32_t    RUDRA40_OPTICS_ISR_SFP_RX_LOS; /* Interrupt from State change for STATUS_SFP_RX_LOS */
    uint32_t    RUDRA40_OPTICS_ISR_SFP_RX_LOS_2; /* Interrupt from State change for STATUS_SFP_RX_LOS */
    uint32_t    RUDRA40_OPTICS_ISM_SFP_RX_LOS; /* Interrupt Mask Register for STATUS_SFP_RX_LOS */
    uint32_t    RUDRA40_OPTICS_ISM_SFP_RX_LOS_2; /* Interrupt Mask Register for STATUS_SFP_RX_LOS */
    uint32_t    RUDRA40_OPTICS_STATUS_SFP_RX_LOS; /* Status of SFP+ Rx Loss-of-signal */
    uint32_t    RUDRA40_OPTICS_STATUS_SFP_RX_LOS_2; /* Status of SFP+ Rx Loss-of-signal */
    uint32_t    RUDRA40_OPTICS_IST_SFP_RX_LOS; /* Interrupt Test Debug register for SFP[31:0] */
    uint32_t    RUDRA40_OPTICS_IST_SFP_RX_LOS_2; /* Interrupt Test Debug register for SFP[39:32] */
    uint32_t    RUDRA40_OPTICS_ISR_SFPDD_RX_LOS; /* Interrupt from State change for STATUS_SFPDD_RX_LOS */
    uint32_t    RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_2; /* Interrupt from State change for STATUS_SFPDD_RX_LOS */
    uint32_t    RUDRA40_OPTICS_ISM_SFPDD_RX_LOS; /* Interrupt Mask Register for STATUS_SFPDD_RX_LOS */
    uint32_t    RUDRA40_OPTICS_ISM_SFPDD_RX_LOS_2; /* Interrupt Mask Register for STATUS_SFPDD_RX_LOS */
    uint32_t    RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS; /* Status of SFPDD Rx Loss-of-signal */
    uint32_t    RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS_2; /* Status of SFPDD Rx Loss-of-signal */
    uint32_t    RUDRA40_OPTICS_IST_SFPDD_RX_LOS; /* Interrupt Test Debug register for SFPDD[31:0] */
    uint32_t    RUDRA40_OPTICS_IST_SFPDD_RX_LOS_2; /* Interrupt Test Debug register for SFPDD[39:32] */
    uint32_t    RUDRA40_OPTICS_ISR_SFP_PRESENT; /* Interrupt from State change for STATUS_SFP_PRESENT */
    uint32_t    RUDRA40_OPTICS_ISR_SFP_PRESENT_2; /* Interrupt from State change for STATUS_SFP_PRESENT */
    uint32_t    RUDRA40_OPTICS_ISM_SFP_PRESENT; /* Interrupt Mask Register for STATUS_SFP_PRESENT */
    uint32_t    RUDRA40_OPTICS_ISM_SFP_PRESENT_2; /* Interrupt Mask Register for STATUS_SFP_PRESENT */
    uint32_t    RUDRA40_OPTICS_STATUS_SFP_PRESENT; /* Status of SFP+ Present signal */
    uint32_t    RUDRA40_OPTICS_STATUS_SFP_PRESENT_2; /* Status of SFP+ Present signal */
    uint32_t    RUDRA40_OPTICS_IST_SFP_PRESENT; /* Interrupt Test Debug register for SFP[31:0] */
    uint32_t    RUDRA40_OPTICS_IST_SFP_PRESENT_2; /* Interrupt Test Debug register for SFP[39:32] */
    uint32_t    RUDRA40_OPTICS_ISR_SFP_PWR_GD; /* Interrupt from State change for STATUS_SFP_PWR_GD */
    uint32_t    RUDRA40_OPTICS_ISR_SFP_PWR_GD_2; /* Interrupt from State change for STATUS_SFP_PWR_GD */
    uint32_t    RUDRA40_OPTICS_ISM_SFP_PWR_GD; /* Interrupt Mask Register for STATUS_SFP_PWR_GD */
    uint32_t    RUDRA40_OPTICS_ISM_SFP_PWR_GD_2; /* Interrupt Mask Register for STATUS_SFP_PWR_GD */
    uint32_t    RUDRA40_OPTICS_STATUS_SFP_PWR_GD; /* Status of SFP+ Power Good signal */
    uint32_t    RUDRA40_OPTICS_STATUS_SFP_PWR_GD_2; /* Status of SFP+ Power Good signal */
    uint32_t    RUDRA40_OPTICS_IST_SFP_PWR_GD; /* Interrupt Test Debug register for SFP[31:0] */
    uint32_t    RUDRA40_OPTICS_IST_SFP_PWR_GD_2; /* Interrupt Test Debug register for SFP[39:32] */
    uint32_t    RUDRA40_OPTICS_SFP_LOW_PWR_0; /* SFP low power select bits */
    uint32_t    RUDRA40_OPTICS_SFP_LOW_PWR_1; /* SFP low power select bits */
    uint32_t    RUDRA40_OPTICS_SFP_RESET_0; /* SFP reset control bits */
    uint32_t    RUDRA40_OPTICS_SFP_RESET_1; /* SFP reset control bits */
    uint32_t    RUDRA40_OPTICS_THEEND;      /* Reserved */
} __attribute__ ((__packed__, __aligned__(4)));

#define RUDRA40_REG_PTR(base, reg)           (&(base)->reg)
#define RUDRA40_REG_OFFSET(reg)              offsetof(struct Rudra40_dev_reg, reg)
#define RUDRA40_REG_INDEX(reg)               (RUDRA40_REG_OFFSET(reg)/sizeof(uint32_t))
#define RUDRA40_REG_WIDTH(reg)               RUDRA40_JOIN(reg,, _WIDTH)
#define RUDRA40_REG_TYPE(reg)                RUDRA40_JOIN(reg,, _TYPE)
#define RUDRA40_REG_VALUE(reg, val)          RUDRA40_JOIN(reg,, val)
#define RUDRA40_FIELD_MASK(reg, field)       RUDRA40_JOIN(reg, field, _MASK)
#define RUDRA40_FIELD_SHIFT(reg, field)      RUDRA40_JOIN(reg, field, _SHIFT)
#define RUDRA40_FIELD_VALUE(reg, field, val) RUDRA40_JOIN(reg, field, val)
#define RUDRA40_JOIN(reg, field, suffix) reg ## _ ## field ## __ ## suffix

#define RUDRA40_GET_BITFIELD(regval, mask, shift) \
    ( ((regval)&(mask)) >> (shift) )

#define RUDRA40_SET_BITFIELD(regval, mask, shift, value) \
    ( ((regval) & ~(mask)) | (((value)<<(shift)) & (mask)) )


/* ---- RUDRA40_BASE_FID ---- */
#define RUDRA40_BASE_FID____WIDTH	32
#define RUDRA40_BASE_FID____TYPE 	uint32_t

#define RUDRA40_BASE_FID_Unused_16___MASK 	UINT32_C(0xffff0000)
#define RUDRA40_BASE_FID_Unused_16___SHIFT	16
#define RUDRA40_BASE_FID_PID___MASK       	UINT32_C(0xff00)
#define RUDRA40_BASE_FID_PID___SHIFT      	8
#define RUDRA40_BASE_FID_DID___MASK       	UINT32_C(0xff)
#define RUDRA40_BASE_FID_DID___SHIFT      	0
#define RUDRA40_BASE_FID____REGMASK	UINT32_C(65535)

/* ---- RUDRA40_BASE_MJR ---- */
#define RUDRA40_BASE_MJR____WIDTH	32
#define RUDRA40_BASE_MJR____TYPE 	uint32_t

#define RUDRA40_BASE_MJR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_BASE_MNR ---- */
#define RUDRA40_BASE_MNR____WIDTH	32
#define RUDRA40_BASE_MNR____TYPE 	uint32_t

#define RUDRA40_BASE_MNR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_BASE_BLD ---- */
#define RUDRA40_BASE_BLD____WIDTH	32
#define RUDRA40_BASE_BLD____TYPE 	uint32_t

#define RUDRA40_BASE_BLD____REGMASK	UINT32_C(0)

/* ---- RUDRA40_BASE_SCRATCHPAD ---- */
#define RUDRA40_BASE_SCRATCHPAD____WIDTH	32
#define RUDRA40_BASE_SCRATCHPAD____TYPE 	uint32_t

#define RUDRA40_BASE_SCRATCHPAD____REGMASK	UINT32_C(0)

/* ---- RUDRA40_BASE_FPGA_DATE ---- */
#define RUDRA40_BASE_FPGA_DATE____WIDTH	32
#define RUDRA40_BASE_FPGA_DATE____TYPE 	uint32_t

#define RUDRA40_BASE_FPGA_DATE_build_year___MASK  	UINT32_C(0xffff0000)
#define RUDRA40_BASE_FPGA_DATE_build_year___SHIFT 	16
#define RUDRA40_BASE_FPGA_DATE_build_month___MASK 	UINT32_C(0xff00)
#define RUDRA40_BASE_FPGA_DATE_build_month___SHIFT	8
#define RUDRA40_BASE_FPGA_DATE_build_day___MASK   	UINT32_C(0xff)
#define RUDRA40_BASE_FPGA_DATE_build_day___SHIFT  	0
#define RUDRA40_BASE_FPGA_DATE____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_FPGA_RECONFIG ---- */
#define RUDRA40_BASE_FPGA_RECONFIG____WIDTH	32
#define RUDRA40_BASE_FPGA_RECONFIG____TYPE 	uint32_t

#define RUDRA40_BASE_FPGA_RECONFIG_Unused_16___MASK               	UINT32_C(0xffff0000)
#define RUDRA40_BASE_FPGA_RECONFIG_Unused_16___SHIFT              	16
#define RUDRA40_BASE_FPGA_RECONFIG_reconfiguration_trigger___MASK 	UINT32_C(0xffff)
#define RUDRA40_BASE_FPGA_RECONFIG_reconfiguration_trigger___SHIFT	0
#define RUDRA40_BASE_FPGA_RECONFIG____REGMASK	UINT32_C(65535)

/* ---- RUDRA40_BASE_MPU_BRD_RESET ---- */
#define RUDRA40_BASE_MPU_BRD_RESET____WIDTH	32
#define RUDRA40_BASE_MPU_BRD_RESET____TYPE 	uint32_t

#define RUDRA40_BASE_MPU_BRD_RESET_Unused_16___MASK           	UINT32_C(0xffff0000)
#define RUDRA40_BASE_MPU_BRD_RESET_Unused_16___SHIFT          	16
#define RUDRA40_BASE_MPU_BRD_RESET_board_reset_trigger___MASK 	UINT32_C(0xffff)
#define RUDRA40_BASE_MPU_BRD_RESET_board_reset_trigger___SHIFT	0
#define RUDRA40_BASE_MPU_BRD_RESET____REGMASK	UINT32_C(65535)

/* ---- RUDRA40_BASE_MPU_ISSU_RESET_EN ---- */
#define RUDRA40_BASE_MPU_ISSU_RESET_EN____WIDTH	32
#define RUDRA40_BASE_MPU_ISSU_RESET_EN____TYPE 	uint32_t

#define RUDRA40_BASE_MPU_ISSU_RESET_EN_Unused_16___MASK   	UINT32_C(0xffff0000)
#define RUDRA40_BASE_MPU_ISSU_RESET_EN_Unused_16___SHIFT  	16
#define RUDRA40_BASE_MPU_ISSU_RESET_EN_enable_issu___MASK 	UINT32_C(0xffff)
#define RUDRA40_BASE_MPU_ISSU_RESET_EN_enable_issu___SHIFT	0
#define RUDRA40_BASE_MPU_ISSU_RESET_EN____REGMASK	UINT32_C(65535)

/* ---- RUDRA40_BASE_ENDIANESS ---- */
#define RUDRA40_BASE_ENDIANESS____WIDTH	32
#define RUDRA40_BASE_ENDIANESS____TYPE 	uint32_t

#define RUDRA40_BASE_ENDIANESS_Unused_1___MASK    	UINT32_C(0xfffffffe)
#define RUDRA40_BASE_ENDIANESS_Unused_1___SHIFT   	1
#define RUDRA40_BASE_ENDIANESS_SWAP_ENDIAN___MASK 	UINT32_C(0x1)
#define RUDRA40_BASE_ENDIANESS_SWAP_ENDIAN___SHIFT	0
#define RUDRA40_BASE_ENDIANESS____REGMASK	UINT32_C(1)

/* ---- RUDRA40_BASE_BRD_ID ---- */
#define RUDRA40_BASE_BRD_ID____WIDTH	32
#define RUDRA40_BASE_BRD_ID____TYPE 	uint32_t

#define RUDRA40_BASE_BRD_ID_Unused_9___MASK       	UINT32_C(0xfffffe00)
#define RUDRA40_BASE_BRD_ID_Unused_9___SHIFT      	9
#define RUDRA40_BASE_BRD_ID_fpga_load_type___MASK 	UINT32_C(0x100)
#define RUDRA40_BASE_BRD_ID_fpga_load_type___SHIFT	8
#define RUDRA40_BASE_BRD_ID_brd_rev___MASK        	UINT32_C(0xc0)
#define RUDRA40_BASE_BRD_ID_brd_rev___SHIFT       	6
#define RUDRA40_BASE_BRD_ID_brd_id___MASK         	UINT32_C(0x3f)
#define RUDRA40_BASE_BRD_ID_brd_id___SHIFT        	0
#define RUDRA40_BASE_BRD_ID____REGMASK	UINT32_C(511)

/* ---- RUDRA40_BASE_EXT_FPGA_CFG_STATUS ---- */
#define RUDRA40_BASE_EXT_FPGA_CFG_STATUS____WIDTH	32
#define RUDRA40_BASE_EXT_FPGA_CFG_STATUS____TYPE 	uint32_t

#define RUDRA40_BASE_EXT_FPGA_CFG_STATUS_Unused_3___MASK      	UINT32_C(0xfffffff8)
#define RUDRA40_BASE_EXT_FPGA_CFG_STATUS_Unused_3___SHIFT     	3
#define RUDRA40_BASE_EXT_FPGA_CFG_STATUS_DONE_OVERRIDE___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_EXT_FPGA_CFG_STATUS_DONE_OVERRIDE___SHIFT	2
#define RUDRA40_BASE_EXT_FPGA_CFG_STATUS_INIT_N___MASK        	UINT32_C(0x2)
#define RUDRA40_BASE_EXT_FPGA_CFG_STATUS_INIT_N___SHIFT       	1
#define RUDRA40_BASE_EXT_FPGA_CFG_STATUS_DONE___MASK          	UINT32_C(0x1)
#define RUDRA40_BASE_EXT_FPGA_CFG_STATUS_DONE___SHIFT         	0
#define RUDRA40_BASE_EXT_FPGA_CFG_STATUS____REGMASK	UINT32_C(7)

/* ---- RUDRA40_BASE_EXT_FPGA_CFG_CTL ---- */
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL____WIDTH	32
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL____TYPE 	uint32_t

#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_Unused_5___MASK   	UINT32_C(0xffffffe0)
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_Unused_5___SHIFT  	5
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_CS_N___MASK       	UINT32_C(0x10)
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_CS_N___SHIFT      	4
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_QSPI_WE_L___MASK  	UINT32_C(0x8)
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_QSPI_WE_L___SHIFT 	3
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_QSPI_ENB_L___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_QSPI_ENB_L___SHIFT	2
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_INIT_N___MASK     	UINT32_C(0x2)
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_INIT_N___SHIFT    	1
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_PROGRAM_N___MASK  	UINT32_C(0x1)
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL_PROGRAM_N___SHIFT 	0
#define RUDRA40_BASE_EXT_FPGA_CFG_CTL____REGMASK	UINT32_C(31)

/* ---- RUDRA40_BASE_CLK_DEBUG_0 ---- */
#define RUDRA40_BASE_CLK_DEBUG_0____WIDTH	32
#define RUDRA40_BASE_CLK_DEBUG_0____TYPE 	uint32_t

#define RUDRA40_BASE_CLK_DEBUG_0_Unused_24___MASK            	UINT32_C(0xff000000)
#define RUDRA40_BASE_CLK_DEBUG_0_Unused_24___SHIFT           	24
#define RUDRA40_BASE_CLK_DEBUG_0_fpga_refclk_250_freq___MASK 	UINT32_C(0xfff000)
#define RUDRA40_BASE_CLK_DEBUG_0_fpga_refclk_250_freq___SHIFT	12
#define RUDRA40_BASE_CLK_DEBUG_0_fpga_refclk_100_freq___MASK 	UINT32_C(0xfff)
#define RUDRA40_BASE_CLK_DEBUG_0_fpga_refclk_100_freq___SHIFT	0
#define RUDRA40_BASE_CLK_DEBUG_0____REGMASK	UINT32_C(16777215)

/* ---- RUDRA40_BASE_CLK_DEBUG_1 ---- */
#define RUDRA40_BASE_CLK_DEBUG_1____WIDTH	32
#define RUDRA40_BASE_CLK_DEBUG_1____TYPE 	uint32_t

#define RUDRA40_BASE_CLK_DEBUG_1_Unused_24___MASK            	UINT32_C(0xff000000)
#define RUDRA40_BASE_CLK_DEBUG_1_Unused_24___SHIFT           	24
#define RUDRA40_BASE_CLK_DEBUG_1_internal_25_freq___MASK     	UINT32_C(0xfff000)
#define RUDRA40_BASE_CLK_DEBUG_1_internal_25_freq___SHIFT    	12
#define RUDRA40_BASE_CLK_DEBUG_1_pcie_refclk_100_freq___MASK 	UINT32_C(0xfff)
#define RUDRA40_BASE_CLK_DEBUG_1_pcie_refclk_100_freq___SHIFT	0
#define RUDRA40_BASE_CLK_DEBUG_1____REGMASK	UINT32_C(16777215)

/* ---- RUDRA40_BASE_REGMAP_RESET_DEBUG ---- */
#define RUDRA40_BASE_REGMAP_RESET_DEBUG____WIDTH	32
#define RUDRA40_BASE_REGMAP_RESET_DEBUG____TYPE 	uint32_t

#define RUDRA40_BASE_REGMAP_RESET_DEBUG_high_count___MASK 	UINT32_C(0xffff0000)
#define RUDRA40_BASE_REGMAP_RESET_DEBUG_high_count___SHIFT	16
#define RUDRA40_BASE_REGMAP_RESET_DEBUG_low_count___MASK  	UINT32_C(0xffff)
#define RUDRA40_BASE_REGMAP_RESET_DEBUG_low_count___SHIFT 	0
#define RUDRA40_BASE_REGMAP_RESET_DEBUG____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_PCIE_RESET_DEBUG ---- */
#define RUDRA40_BASE_PCIE_RESET_DEBUG____WIDTH	32
#define RUDRA40_BASE_PCIE_RESET_DEBUG____TYPE 	uint32_t

#define RUDRA40_BASE_PCIE_RESET_DEBUG_high_count___MASK 	UINT32_C(0xffff0000)
#define RUDRA40_BASE_PCIE_RESET_DEBUG_high_count___SHIFT	16
#define RUDRA40_BASE_PCIE_RESET_DEBUG_low_count___MASK  	UINT32_C(0xffff)
#define RUDRA40_BASE_PCIE_RESET_DEBUG_low_count___SHIFT 	0
#define RUDRA40_BASE_PCIE_RESET_DEBUG____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_PLL_LOCKED_DEBUG ---- */
#define RUDRA40_BASE_PLL_LOCKED_DEBUG____WIDTH	32
#define RUDRA40_BASE_PLL_LOCKED_DEBUG____TYPE 	uint32_t

#define RUDRA40_BASE_PLL_LOCKED_DEBUG_Unused_4___MASK        	UINT32_C(0xfffffff0)
#define RUDRA40_BASE_PLL_LOCKED_DEBUG_Unused_4___SHIFT       	4
#define RUDRA40_BASE_PLL_LOCKED_DEBUG_core_locked_cnt___MASK 	UINT32_C(0xf)
#define RUDRA40_BASE_PLL_LOCKED_DEBUG_core_locked_cnt___SHIFT	0
#define RUDRA40_BASE_PLL_LOCKED_DEBUG____REGMASK	UINT32_C(15)

/* ---- RUDRA40_BASE_I2C_SCRATCHPAD ---- */
#define RUDRA40_BASE_I2C_SCRATCHPAD____WIDTH	32
#define RUDRA40_BASE_I2C_SCRATCHPAD____TYPE 	uint32_t

#define RUDRA40_BASE_I2C_SCRATCHPAD____REGMASK	UINT32_C(0)

/* ---- RUDRA40_BASE_RESET_HISTORY_0 ---- */
#define RUDRA40_BASE_RESET_HISTORY_0____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_0____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_0_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_0_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_0_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_0_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_0_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_0_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_0_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_0_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_0_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_0_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_0_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_0_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_0_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_0_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_0_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_0_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_0_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_0_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_0_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_0_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_0_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_0_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_0_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_0_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_0____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_0 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_0____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_0____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_0_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_0_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_1 ---- */
#define RUDRA40_BASE_RESET_HISTORY_1____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_1____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_1_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_1_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_1_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_1_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_1_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_1_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_1_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_1_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_1_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_1_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_1_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_1_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_1_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_1_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_1_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_1_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_1_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_1_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_1_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_1_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_1_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_1_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_1_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_1_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_1____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_1 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_1____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_1____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_1_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_1_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_2 ---- */
#define RUDRA40_BASE_RESET_HISTORY_2____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_2____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_2_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_2_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_2_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_2_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_2_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_2_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_2_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_2_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_2_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_2_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_2_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_2_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_2_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_2_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_2_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_2_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_2_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_2_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_2_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_2_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_2_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_2_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_2_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_2_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_2____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_2 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_2____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_2____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_2_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_2_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_3 ---- */
#define RUDRA40_BASE_RESET_HISTORY_3____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_3____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_3_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_3_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_3_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_3_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_3_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_3_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_3_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_3_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_3_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_3_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_3_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_3_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_3_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_3_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_3_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_3_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_3_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_3_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_3_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_3_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_3_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_3_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_3_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_3_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_3____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_3 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_3____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_3____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_3_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_3_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_4 ---- */
#define RUDRA40_BASE_RESET_HISTORY_4____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_4____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_4_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_4_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_4_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_4_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_4_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_4_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_4_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_4_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_4_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_4_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_4_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_4_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_4_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_4_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_4_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_4_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_4_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_4_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_4_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_4_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_4_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_4_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_4_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_4_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_4____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_4 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_4____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_4____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_4_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_4_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_5 ---- */
#define RUDRA40_BASE_RESET_HISTORY_5____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_5____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_5_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_5_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_5_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_5_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_5_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_5_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_5_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_5_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_5_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_5_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_5_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_5_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_5_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_5_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_5_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_5_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_5_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_5_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_5_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_5_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_5_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_5_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_5_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_5_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_5____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_5 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_5____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_5____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_5_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_5_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_6 ---- */
#define RUDRA40_BASE_RESET_HISTORY_6____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_6____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_6_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_6_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_6_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_6_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_6_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_6_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_6_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_6_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_6_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_6_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_6_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_6_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_6_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_6_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_6_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_6_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_6_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_6_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_6_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_6_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_6_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_6_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_6_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_6_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_6____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_6 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_6____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_6____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_6_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_6_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_6____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_7 ---- */
#define RUDRA40_BASE_RESET_HISTORY_7____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_7____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_7_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_7_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_7_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_7_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_7_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_7_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_7_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_7_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_7_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_7_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_7_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_7_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_7_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_7_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_7_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_7_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_7_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_7_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_7_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_7_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_7_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_7_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_7_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_7_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_7____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_7 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_7____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_7____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_7_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_7_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_7____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_8 ---- */
#define RUDRA40_BASE_RESET_HISTORY_8____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_8____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_8_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_8_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_8_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_8_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_8_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_8_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_8_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_8_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_8_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_8_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_8_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_8_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_8_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_8_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_8_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_8_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_8_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_8_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_8_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_8_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_8_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_8_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_8_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_8_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_8____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_8 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_8____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_8____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_8_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_8_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_8____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_9 ---- */
#define RUDRA40_BASE_RESET_HISTORY_9____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_9____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_9_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_9_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_9_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_9_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_9_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_9_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_9_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_9_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_9_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_9_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_9_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_9_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_9_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_9_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_9_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_9_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_9_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_9_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_9_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_9_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_9_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_9_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_9_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_9_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_9____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_9 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_9____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_9____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_9_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_9_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_9____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_10 ---- */
#define RUDRA40_BASE_RESET_HISTORY_10____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_10____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_10_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_10_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_10_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_10_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_10_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_10_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_10_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_10_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_10_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_10_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_10_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_10_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_10_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_10_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_10_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_10_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_10_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_10_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_10_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_10_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_10_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_10_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_10_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_10_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_10____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_10 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_10____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_10____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_10_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_10_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_10____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_11 ---- */
#define RUDRA40_BASE_RESET_HISTORY_11____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_11____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_11_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_11_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_11_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_11_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_11_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_11_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_11_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_11_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_11_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_11_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_11_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_11_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_11_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_11_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_11_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_11_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_11_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_11_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_11_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_11_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_11_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_11_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_11_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_11_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_11____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_11 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_11____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_11____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_11_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_11_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_11____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_12 ---- */
#define RUDRA40_BASE_RESET_HISTORY_12____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_12____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_12_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_12_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_12_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_12_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_12_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_12_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_12_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_12_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_12_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_12_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_12_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_12_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_12_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_12_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_12_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_12_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_12_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_12_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_12_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_12_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_12_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_12_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_12_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_12_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_12____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_12 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_12____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_12____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_12_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_12_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_12____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_13 ---- */
#define RUDRA40_BASE_RESET_HISTORY_13____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_13____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_13_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_13_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_13_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_13_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_13_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_13_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_13_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_13_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_13_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_13_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_13_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_13_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_13_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_13_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_13_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_13_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_13_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_13_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_13_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_13_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_13_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_13_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_13_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_13_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_13____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_13 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_13____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_13____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_13_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_13_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_13____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_14 ---- */
#define RUDRA40_BASE_RESET_HISTORY_14____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_14____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_14_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_14_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_14_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_14_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_14_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_14_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_14_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_14_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_14_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_14_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_14_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_14_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_14_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_14_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_14_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_14_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_14_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_14_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_14_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_14_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_14_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_14_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_14_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_14_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_14____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_14 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_14____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_14____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_14_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_14_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_14____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_RESET_HISTORY_15 ---- */
#define RUDRA40_BASE_RESET_HISTORY_15____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_15____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_15_TIMESTAMPL___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_BASE_RESET_HISTORY_15_TIMESTAMPL___SHIFT           	16
#define RUDRA40_BASE_RESET_HISTORY_15_Unused_12___MASK             	UINT32_C(0xf000)
#define RUDRA40_BASE_RESET_HISTORY_15_Unused_12___SHIFT            	12
#define RUDRA40_BASE_RESET_HISTORY_15_RSVD0___MASK                 	UINT32_C(0xe00)
#define RUDRA40_BASE_RESET_HISTORY_15_RSVD0___SHIFT                	9
#define RUDRA40_BASE_RESET_HISTORY_15_Unused_8___MASK              	UINT32_C(0x100)
#define RUDRA40_BASE_RESET_HISTORY_15_Unused_8___SHIFT             	8
#define RUDRA40_BASE_RESET_HISTORY_15_WD_TIMER_RESET___MASK        	UINT32_C(0x80)
#define RUDRA40_BASE_RESET_HISTORY_15_WD_TIMER_RESET___SHIFT       	7
#define RUDRA40_BASE_RESET_HISTORY_15_SOFT_REGMAP_RESET___MASK     	UINT32_C(0x40)
#define RUDRA40_BASE_RESET_HISTORY_15_SOFT_REGMAP_RESET___SHIFT    	6
#define RUDRA40_BASE_RESET_HISTORY_15_CPU_WD_TIMER_RESET___MASK    	UINT32_C(0x20)
#define RUDRA40_BASE_RESET_HISTORY_15_CPU_WD_TIMER_RESET___SHIFT   	5
#define RUDRA40_BASE_RESET_HISTORY_15_THERMAL_RESET___MASK         	UINT32_C(0x10)
#define RUDRA40_BASE_RESET_HISTORY_15_THERMAL_RESET___SHIFT        	4
#define RUDRA40_BASE_RESET_HISTORY_15_SYS_RST_OUTn___MASK          	UINT32_C(0x8)
#define RUDRA40_BASE_RESET_HISTORY_15_SYS_RST_OUTn___SHIFT         	3
#define RUDRA40_BASE_RESET_HISTORY_15_PB_RESET_PRESS_DETECT___MASK 	UINT32_C(0x4)
#define RUDRA40_BASE_RESET_HISTORY_15_PB_RESET_PRESS_DETECT___SHIFT	2
#define RUDRA40_BASE_RESET_HISTORY_15_PWR_SEQ_RESTART___MASK       	UINT32_C(0x2)
#define RUDRA40_BASE_RESET_HISTORY_15_PWR_SEQ_RESTART___SHIFT      	1
#define RUDRA40_BASE_RESET_HISTORY_15_POWERUP_RECONFIG___MASK      	UINT32_C(0x1)
#define RUDRA40_BASE_RESET_HISTORY_15_POWERUP_RECONFIG___SHIFT     	0
#define RUDRA40_BASE_RESET_HISTORY_15____REGMASK	UINT32_C(4294905599)

/* ---- RUDRA40_BASE_RESET_HISTORY_TS_UPR_15 ---- */
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_15____WIDTH	32
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_15____TYPE 	uint32_t

#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_15_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_15_TIMESTAMPU___SHIFT	0
#define RUDRA40_BASE_RESET_HISTORY_TS_UPR_15____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_CURRENT_UPTIME_UPR ---- */
#define RUDRA40_BASE_CURRENT_UPTIME_UPR____WIDTH	32
#define RUDRA40_BASE_CURRENT_UPTIME_UPR____TYPE 	uint32_t

#define RUDRA40_BASE_CURRENT_UPTIME_UPR_UPTIME64___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_CURRENT_UPTIME_UPR_UPTIME64___SHIFT	0
#define RUDRA40_BASE_CURRENT_UPTIME_UPR____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_CURRENT_UPTIME_LWR ---- */
#define RUDRA40_BASE_CURRENT_UPTIME_LWR____WIDTH	32
#define RUDRA40_BASE_CURRENT_UPTIME_LWR____TYPE 	uint32_t

#define RUDRA40_BASE_CURRENT_UPTIME_LWR_UPTIME32___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_BASE_CURRENT_UPTIME_LWR_UPTIME32___SHIFT	0
#define RUDRA40_BASE_CURRENT_UPTIME_LWR____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_BASE_THEEND ---- */
#define RUDRA40_BASE_THEEND____WIDTH	32
#define RUDRA40_BASE_THEEND____TYPE 	uint32_t

#define RUDRA40_BASE_THEEND____REGMASK	UINT32_C(0)

/* ---- RUDRA40_RESERVED_THESTART ---- */
#define RUDRA40_RESERVED_THESTART____WIDTH	32
#define RUDRA40_RESERVED_THESTART____TYPE 	uint32_t

#define RUDRA40_RESERVED_THESTART____REGMASK	UINT32_C(0)

/* ---- RUDRA40_RESERVED_THEEND ---- */
#define RUDRA40_RESERVED_THEEND____WIDTH	32
#define RUDRA40_RESERVED_THEEND____TYPE 	uint32_t

#define RUDRA40_RESERVED_THEEND____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_SOFT_RESET ---- */
#define RUDRA40_GLUE_SOFT_RESET____WIDTH	32
#define RUDRA40_GLUE_SOFT_RESET____TYPE 	uint32_t

#define RUDRA40_GLUE_SOFT_RESET_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_GLUE_SOFT_RESET_Unused_1___SHIFT	1
#define RUDRA40_GLUE_SOFT_RESET_RST___MASK      	UINT32_C(0x1)
#define RUDRA40_GLUE_SOFT_RESET_RST___SHIFT     	0
#define RUDRA40_GLUE_SOFT_RESET____REGMASK	UINT32_C(1)

/* ---- RUDRA40_GLUE_RESERVED2 ---- */
#define RUDRA40_GLUE_RESERVED2____WIDTH	32
#define RUDRA40_GLUE_RESERVED2____TYPE 	uint32_t

#define RUDRA40_GLUE_RESERVED2____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_RESERVED3 ---- */
#define RUDRA40_GLUE_RESERVED3____WIDTH	32
#define RUDRA40_GLUE_RESERVED3____TYPE 	uint32_t

#define RUDRA40_GLUE_RESERVED3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_GENERAL_STATUS ---- */
#define RUDRA40_GLUE_GENERAL_STATUS____WIDTH	32
#define RUDRA40_GLUE_GENERAL_STATUS____TYPE 	uint32_t

#define RUDRA40_GLUE_GENERAL_STATUS_Unused_25___MASK                           	UINT32_C(0xfe000000)
#define RUDRA40_GLUE_GENERAL_STATUS_Unused_25___SHIFT                          	25
#define RUDRA40_GLUE_GENERAL_STATUS_socket_monitoring_flag_J2CPA_masked___MASK 	UINT32_C(0x1000000)
#define RUDRA40_GLUE_GENERAL_STATUS_socket_monitoring_flag_J2CPA_masked___SHIFT	24
#define RUDRA40_GLUE_GENERAL_STATUS_Unused_23___MASK                           	UINT32_C(0x800000)
#define RUDRA40_GLUE_GENERAL_STATUS_Unused_23___SHIFT                          	23
#define RUDRA40_GLUE_GENERAL_STATUS_J2CPA_SI5345_156M_LOL_N___MASK             	UINT32_C(0x400000)
#define RUDRA40_GLUE_GENERAL_STATUS_J2CPA_SI5345_156M_LOL_N___SHIFT            	22
#define RUDRA40_GLUE_GENERAL_STATUS_J2CP_SI5345_50M_LOL_N___MASK               	UINT32_C(0x200000)
#define RUDRA40_GLUE_GENERAL_STATUS_J2CP_SI5345_50M_LOL_N___SHIFT              	21
#define RUDRA40_GLUE_GENERAL_STATUS_ANT_PWR_GD___MASK                          	UINT32_C(0x100000)
#define RUDRA40_GLUE_GENERAL_STATUS_ANT_PWR_GD___SHIFT                         	20
#define RUDRA40_GLUE_GENERAL_STATUS_Unused_18___MASK                           	UINT32_C(0xc0000)
#define RUDRA40_GLUE_GENERAL_STATUS_Unused_18___SHIFT                          	18
#define RUDRA40_GLUE_GENERAL_STATUS_J2CPA_SYNCE1_CLK_OUT_VALID___MASK          	UINT32_C(0x20000)
#define RUDRA40_GLUE_GENERAL_STATUS_J2CPA_SYNCE1_CLK_OUT_VALID___SHIFT         	17
#define RUDRA40_GLUE_GENERAL_STATUS_J2CPA_SYNCE0_CLK_OUT_VALID___MASK          	UINT32_C(0x10000)
#define RUDRA40_GLUE_GENERAL_STATUS_J2CPA_SYNCE0_CLK_OUT_VALID___SHIFT         	16
#define RUDRA40_GLUE_GENERAL_STATUS_BMC_LESS_DESIGN___MASK                     	UINT32_C(0x8000)
#define RUDRA40_GLUE_GENERAL_STATUS_BMC_LESS_DESIGN___SHIFT                    	15
#define RUDRA40_GLUE_GENERAL_STATUS_HW_PGM_ZL_DONE___MASK                      	UINT32_C(0x4000)
#define RUDRA40_GLUE_GENERAL_STATUS_HW_PGM_ZL_DONE___SHIFT                     	14
#define RUDRA40_GLUE_GENERAL_STATUS_BMC_STATUS1___MASK                         	UINT32_C(0x2000)
#define RUDRA40_GLUE_GENERAL_STATUS_BMC_STATUS1___SHIFT                        	13
#define RUDRA40_GLUE_GENERAL_STATUS_BMC_STATUS0___MASK                         	UINT32_C(0x1000)
#define RUDRA40_GLUE_GENERAL_STATUS_BMC_STATUS0___SHIFT                        	12
#define RUDRA40_GLUE_GENERAL_STATUS_CPU_BOARD_SEATED_N___MASK                  	UINT32_C(0x800)
#define RUDRA40_GLUE_GENERAL_STATUS_CPU_BOARD_SEATED_N___SHIFT                 	11
#define RUDRA40_GLUE_GENERAL_STATUS_BMC_SEATED_N___MASK                        	UINT32_C(0x400)
#define RUDRA40_GLUE_GENERAL_STATUS_BMC_SEATED_N___SHIFT                       	10
#define RUDRA40_GLUE_GENERAL_STATUS_ZL30603_PLL_HOLDOVER___MASK                	UINT32_C(0x200)
#define RUDRA40_GLUE_GENERAL_STATUS_ZL30603_PLL_HOLDOVER___SHIFT               	9
#define RUDRA40_GLUE_GENERAL_STATUS_ZL30603_PLL_LOCKED___MASK                  	UINT32_C(0x100)
#define RUDRA40_GLUE_GENERAL_STATUS_ZL30603_PLL_LOCKED___SHIFT                 	8
#define RUDRA40_GLUE_GENERAL_STATUS_Unused_7___MASK                            	UINT32_C(0x80)
#define RUDRA40_GLUE_GENERAL_STATUS_Unused_7___SHIFT                           	7
#define RUDRA40_GLUE_GENERAL_STATUS_ALL_PWR_GOOD___MASK                        	UINT32_C(0x40)
#define RUDRA40_GLUE_GENERAL_STATUS_ALL_PWR_GOOD___SHIFT                       	6
#define RUDRA40_GLUE_GENERAL_STATUS_Unused_3___MASK                            	UINT32_C(0x38)
#define RUDRA40_GLUE_GENERAL_STATUS_Unused_3___SHIFT                           	3
#define RUDRA40_GLUE_GENERAL_STATUS_J2CA_ROV___MASK                            	UINT32_C(0x7)
#define RUDRA40_GLUE_GENERAL_STATUS_J2CA_ROV___SHIFT                           	0
#define RUDRA40_GLUE_GENERAL_STATUS____REGMASK	UINT32_C(24379207)

/* ---- RUDRA40_GLUE_PWR_CTL ---- */
#define RUDRA40_GLUE_PWR_CTL____WIDTH	32
#define RUDRA40_GLUE_PWR_CTL____TYPE 	uint32_t

#define RUDRA40_GLUE_PWR_CTL_vddio_1v8_mux_pwr_en___MASK 	UINT32_C(0x80000000)
#define RUDRA40_GLUE_PWR_CTL_vddio_1v8_mux_pwr_en___SHIFT	31
#define RUDRA40_GLUE_PWR_CTL_GPS_POWER_EN___MASK         	UINT32_C(0x40000000)
#define RUDRA40_GLUE_PWR_CTL_GPS_POWER_EN___SHIFT        	30
#define RUDRA40_GLUE_PWR_CTL_dvdd_0v72_mux_en___MASK     	UINT32_C(0x20000000)
#define RUDRA40_GLUE_PWR_CTL_dvdd_0v72_mux_en___SHIFT    	29
#define RUDRA40_GLUE_PWR_CTL_Unused_28___MASK            	UINT32_C(0x10000000)
#define RUDRA40_GLUE_PWR_CTL_Unused_28___SHIFT           	28
#define RUDRA40_GLUE_PWR_CTL_J2CA_ENABLE___MASK          	UINT32_C(0x8000000)
#define RUDRA40_GLUE_PWR_CTL_J2CA_ENABLE___SHIFT         	27
#define RUDRA40_GLUE_PWR_CTL_avdd_0v8_mux_pwr_en___MASK  	UINT32_C(0x4000000)
#define RUDRA40_GLUE_PWR_CTL_avdd_0v8_mux_pwr_en___SHIFT 	26
#define RUDRA40_GLUE_PWR_CTL_QSFPDD_3V3_DCO_2_EN___MASK  	UINT32_C(0x2000000)
#define RUDRA40_GLUE_PWR_CTL_QSFPDD_3V3_DCO_2_EN___SHIFT 	25
#define RUDRA40_GLUE_PWR_CTL_QSFPDD_3V3_DCO_1_EN___MASK  	UINT32_C(0x1000000)
#define RUDRA40_GLUE_PWR_CTL_QSFPDD_3V3_DCO_1_EN___SHIFT 	24
#define RUDRA40_GLUE_PWR_CTL_Unused_22___MASK            	UINT32_C(0xc00000)
#define RUDRA40_GLUE_PWR_CTL_Unused_22___SHIFT           	22
#define RUDRA40_GLUE_PWR_CTL_J2CPA_HBM1_VDD1P2_EN___MASK 	UINT32_C(0x200000)
#define RUDRA40_GLUE_PWR_CTL_J2CPA_HBM1_VDD1P2_EN___SHIFT	21
#define RUDRA40_GLUE_PWR_CTL_J2CPA_HBM0_VDD1P2_EN___MASK 	UINT32_C(0x100000)
#define RUDRA40_GLUE_PWR_CTL_J2CPA_HBM0_VDD1P2_EN___SHIFT	20
#define RUDRA40_GLUE_PWR_CTL_Unused_18___MASK            	UINT32_C(0xc0000)
#define RUDRA40_GLUE_PWR_CTL_Unused_18___SHIFT           	18
#define RUDRA40_GLUE_PWR_CTL_J2CPA_HBM1_VPP2P5_EN___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_PWR_CTL_J2CPA_HBM1_VPP2P5_EN___SHIFT	17
#define RUDRA40_GLUE_PWR_CTL_J2CPA_HBM0_VPP2P5_EN___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_PWR_CTL_J2CPA_HBM0_VPP2P5_EN___SHIFT	16
#define RUDRA40_GLUE_PWR_CTL_Unused_15___MASK            	UINT32_C(0x8000)
#define RUDRA40_GLUE_PWR_CTL_Unused_15___SHIFT           	15
#define RUDRA40_GLUE_PWR_CTL_J2CPA_SRD_TVDD1P2_EN___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_PWR_CTL_J2CPA_SRD_TVDD1P2_EN___SHIFT	14
#define RUDRA40_GLUE_PWR_CTL_Unused_13___MASK            	UINT32_C(0x2000)
#define RUDRA40_GLUE_PWR_CTL_Unused_13___SHIFT           	13
#define RUDRA40_GLUE_PWR_CTL_J2CPA_RTVDD0P75_EN___MASK   	UINT32_C(0x1000)
#define RUDRA40_GLUE_PWR_CTL_J2CPA_RTVDD0P75_EN___SHIFT  	12
#define RUDRA40_GLUE_PWR_CTL_Unused_11___MASK            	UINT32_C(0x800)
#define RUDRA40_GLUE_PWR_CTL_Unused_11___SHIFT           	11
#define RUDRA40_GLUE_PWR_CTL_J2CPA_PVDD0P75_EN___MASK    	UINT32_C(0x400)
#define RUDRA40_GLUE_PWR_CTL_J2CPA_PVDD0P75_EN___SHIFT   	10
#define RUDRA40_GLUE_PWR_CTL_Unused_9___MASK             	UINT32_C(0x200)
#define RUDRA40_GLUE_PWR_CTL_Unused_9___SHIFT            	9
#define RUDRA40_GLUE_PWR_CTL_J2CPA_AVDD1P8_EN___MASK     	UINT32_C(0x100)
#define RUDRA40_GLUE_PWR_CTL_J2CPA_AVDD1P8_EN___SHIFT    	8
#define RUDRA40_GLUE_PWR_CTL_Unused_7___MASK             	UINT32_C(0x80)
#define RUDRA40_GLUE_PWR_CTL_Unused_7___SHIFT            	7
#define RUDRA40_GLUE_PWR_CTL_J2CPA_VDDC_EN___MASK        	UINT32_C(0x40)
#define RUDRA40_GLUE_PWR_CTL_J2CPA_VDDC_EN___SHIFT       	6
#define RUDRA40_GLUE_PWR_CTL_Unused_5___MASK             	UINT32_C(0x20)
#define RUDRA40_GLUE_PWR_CTL_Unused_5___SHIFT            	5
#define RUDRA40_GLUE_PWR_CTL_J2CPA_VDDO1P8_EN___MASK     	UINT32_C(0x10)
#define RUDRA40_GLUE_PWR_CTL_J2CPA_VDDO1P8_EN___SHIFT    	4
#define RUDRA40_GLUE_PWR_CTL_J2C_3V0_PWR_EN___MASK       	UINT32_C(0x8)
#define RUDRA40_GLUE_PWR_CTL_J2C_3V0_PWR_EN___SHIFT      	3
#define RUDRA40_GLUE_PWR_CTL_SYNC_1V8_PWR_EN___MASK      	UINT32_C(0x4)
#define RUDRA40_GLUE_PWR_CTL_SYNC_1V8_PWR_EN___SHIFT     	2
#define RUDRA40_GLUE_PWR_CTL_SYNC_3V3_PWR_EN___MASK      	UINT32_C(0x2)
#define RUDRA40_GLUE_PWR_CTL_SYNC_3V3_PWR_EN___SHIFT     	1
#define RUDRA40_GLUE_PWR_CTL_GLOBAL_5V_PWR_EN___MASK     	UINT32_C(0x1)
#define RUDRA40_GLUE_PWR_CTL_GLOBAL_5V_PWR_EN___SHIFT    	0
#define RUDRA40_GLUE_PWR_CTL____REGMASK	UINT32_C(4013118815)

/* ---- RUDRA40_GLUE_PCIE_RESET_MASK ---- */
#define RUDRA40_GLUE_PCIE_RESET_MASK____WIDTH	32
#define RUDRA40_GLUE_PCIE_RESET_MASK____TYPE 	uint32_t

#define RUDRA40_GLUE_PCIE_RESET_MASK_Unused_6___MASK           	UINT32_C(0xffffffc0)
#define RUDRA40_GLUE_PCIE_RESET_MASK_Unused_6___SHIFT          	6
#define RUDRA40_GLUE_PCIE_RESET_MASK_J2CPA_SYS_RST_MASK___MASK 	UINT32_C(0x20)
#define RUDRA40_GLUE_PCIE_RESET_MASK_J2CPA_SYS_RST_MASK___SHIFT	5
#define RUDRA40_GLUE_PCIE_RESET_MASK_Unused_4___MASK           	UINT32_C(0x10)
#define RUDRA40_GLUE_PCIE_RESET_MASK_Unused_4___SHIFT          	4
#define RUDRA40_GLUE_PCIE_RESET_MASK_J2CPA_RST_MASK___MASK     	UINT32_C(0x8)
#define RUDRA40_GLUE_PCIE_RESET_MASK_J2CPA_RST_MASK___SHIFT    	3
#define RUDRA40_GLUE_PCIE_RESET_MASK_unused3___MASK            	UINT32_C(0x4)
#define RUDRA40_GLUE_PCIE_RESET_MASK_unused3___SHIFT           	2
#define RUDRA40_GLUE_PCIE_RESET_MASK_unused2___MASK            	UINT32_C(0x2)
#define RUDRA40_GLUE_PCIE_RESET_MASK_unused2___SHIFT           	1
#define RUDRA40_GLUE_PCIE_RESET_MASK_unused1___MASK            	UINT32_C(0x1)
#define RUDRA40_GLUE_PCIE_RESET_MASK_unused1___SHIFT           	0
#define RUDRA40_GLUE_PCIE_RESET_MASK____REGMASK	UINT32_C(47)

/* ---- RUDRA40_GLUE_DEVICE_RESET ---- */
#define RUDRA40_GLUE_DEVICE_RESET____WIDTH	32
#define RUDRA40_GLUE_DEVICE_RESET____TYPE 	uint32_t

#define RUDRA40_GLUE_DEVICE_RESET_Unused_11___MASK               	UINT32_C(0xfffff800)
#define RUDRA40_GLUE_DEVICE_RESET_Unused_11___SHIFT              	11
#define RUDRA40_GLUE_DEVICE_RESET_RUDHRA_MUX_RESET_L___MASK      	UINT32_C(0x400)
#define RUDRA40_GLUE_DEVICE_RESET_RUDHRA_MUX_RESET_L___SHIFT     	10
#define RUDRA40_GLUE_DEVICE_RESET_J2CPA_PCIE_RST_N___MASK        	UINT32_C(0x200)
#define RUDRA40_GLUE_DEVICE_RESET_J2CPA_PCIE_RST_N___SHIFT       	9
#define RUDRA40_GLUE_DEVICE_RESET_Unused_8___MASK                	UINT32_C(0x100)
#define RUDRA40_GLUE_DEVICE_RESET_Unused_8___SHIFT               	8
#define RUDRA40_GLUE_DEVICE_RESET_J2CPA_SYS_RST_N___MASK         	UINT32_C(0x80)
#define RUDRA40_GLUE_DEVICE_RESET_J2CPA_SYS_RST_N___SHIFT        	7
#define RUDRA40_GLUE_DEVICE_RESET_Unused_6___MASK                	UINT32_C(0x40)
#define RUDRA40_GLUE_DEVICE_RESET_Unused_6___SHIFT               	6
#define RUDRA40_GLUE_DEVICE_RESET_J2CPA_QSPI_RESET_N___MASK      	UINT32_C(0x20)
#define RUDRA40_GLUE_DEVICE_RESET_J2CPA_QSPI_RESET_N___SHIFT     	5
#define RUDRA40_GLUE_DEVICE_RESET_SET_ROV_CORE_SUPPLY___MASK     	UINT32_C(0x10)
#define RUDRA40_GLUE_DEVICE_RESET_SET_ROV_CORE_SUPPLY___SHIFT    	4
#define RUDRA40_GLUE_DEVICE_RESET_J2CP_SI5345_50M_RST_N___MASK   	UINT32_C(0x8)
#define RUDRA40_GLUE_DEVICE_RESET_J2CP_SI5345_50M_RST_N___SHIFT  	3
#define RUDRA40_GLUE_DEVICE_RESET_Unused_2___MASK                	UINT32_C(0x4)
#define RUDRA40_GLUE_DEVICE_RESET_Unused_2___SHIFT               	2
#define RUDRA40_GLUE_DEVICE_RESET_J2CPA_SI5345_156M_RST_N___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_DEVICE_RESET_J2CPA_SI5345_156M_RST_N___SHIFT	1
#define RUDRA40_GLUE_DEVICE_RESET_ZL30633_RST_N___MASK           	UINT32_C(0x1)
#define RUDRA40_GLUE_DEVICE_RESET_ZL30633_RST_N___SHIFT          	0
#define RUDRA40_GLUE_DEVICE_RESET____REGMASK	UINT32_C(1723)

/* ---- RUDRA40_GLUE_GENERAL_CTL ---- */
#define RUDRA40_GLUE_GENERAL_CTL____WIDTH	32
#define RUDRA40_GLUE_GENERAL_CTL____TYPE 	uint32_t

#define RUDRA40_GLUE_GENERAL_CTL_rclk_mux_sel___MASK                          	UINT32_C(0x80000000)
#define RUDRA40_GLUE_GENERAL_CTL_rclk_mux_sel___SHIFT                         	31
#define RUDRA40_GLUE_GENERAL_CTL_adc5_skt_mon_spi_sw_override___MASK          	UINT32_C(0x40000000)
#define RUDRA40_GLUE_GENERAL_CTL_adc5_skt_mon_spi_sw_override___SHIFT         	30
#define RUDRA40_GLUE_GENERAL_CTL_adc4_optics_spi_sw_override___MASK           	UINT32_C(0x20000000)
#define RUDRA40_GLUE_GENERAL_CTL_adc4_optics_spi_sw_override___SHIFT          	29
#define RUDRA40_GLUE_GENERAL_CTL_adc3_optics_spi_sw_override___MASK           	UINT32_C(0x10000000)
#define RUDRA40_GLUE_GENERAL_CTL_adc3_optics_spi_sw_override___SHIFT          	28
#define RUDRA40_GLUE_GENERAL_CTL_adc2_optics_spi_sw_override___MASK           	UINT32_C(0x8000000)
#define RUDRA40_GLUE_GENERAL_CTL_adc2_optics_spi_sw_override___SHIFT          	27
#define RUDRA40_GLUE_GENERAL_CTL_adc1_optics_spi_sw_override___MASK           	UINT32_C(0x4000000)
#define RUDRA40_GLUE_GENERAL_CTL_adc1_optics_spi_sw_override___SHIFT          	26
#define RUDRA40_GLUE_GENERAL_CTL_disable_uart_switching_algo___MASK           	UINT32_C(0x2000000)
#define RUDRA40_GLUE_GENERAL_CTL_disable_uart_switching_algo___SHIFT          	25
#define RUDRA40_GLUE_GENERAL_CTL_tps_programming_enable___MASK                	UINT32_C(0x1000000)
#define RUDRA40_GLUE_GENERAL_CTL_tps_programming_enable___SHIFT               	24
#define RUDRA40_GLUE_GENERAL_CTL_Unused_23___MASK                             	UINT32_C(0x800000)
#define RUDRA40_GLUE_GENERAL_CTL_Unused_23___SHIFT                            	23
#define RUDRA40_GLUE_GENERAL_CTL_sw_socket_test_en___MASK                     	UINT32_C(0x400000)
#define RUDRA40_GLUE_GENERAL_CTL_sw_socket_test_en___SHIFT                    	22
#define RUDRA40_GLUE_GENERAL_CTL_sw_override_socket_test_en___MASK            	UINT32_C(0x200000)
#define RUDRA40_GLUE_GENERAL_CTL_sw_override_socket_test_en___SHIFT           	21
#define RUDRA40_GLUE_GENERAL_CTL_sw_cfpga_pmbus_mux_avs_en___MASK             	UINT32_C(0x100000)
#define RUDRA40_GLUE_GENERAL_CTL_sw_cfpga_pmbus_mux_avs_en___SHIFT            	20
#define RUDRA40_GLUE_GENERAL_CTL_sw_override_cfpga_pmbus_mux_avs_en___MASK    	UINT32_C(0x80000)
#define RUDRA40_GLUE_GENERAL_CTL_sw_override_cfpga_pmbus_mux_avs_en___SHIFT   	19
#define RUDRA40_GLUE_GENERAL_CTL_SIRIL_IDP_OPTIONAL_WP___MASK                 	UINT32_C(0x40000)
#define RUDRA40_GLUE_GENERAL_CTL_SIRIL_IDP_OPTIONAL_WP___SHIFT                	18
#define RUDRA40_GLUE_GENERAL_CTL_SIRIL_IDP_WP___MASK                          	UINT32_C(0x20000)
#define RUDRA40_GLUE_GENERAL_CTL_SIRIL_IDP_WP___SHIFT                         	17
#define RUDRA40_GLUE_GENERAL_CTL_Unused_15___MASK                             	UINT32_C(0x18000)
#define RUDRA40_GLUE_GENERAL_CTL_Unused_15___SHIFT                            	15
#define RUDRA40_GLUE_GENERAL_CTL_gps_buffer_en_l___MASK                       	UINT32_C(0x4000)
#define RUDRA40_GLUE_GENERAL_CTL_gps_buffer_en_l___SHIFT                      	14
#define RUDRA40_GLUE_GENERAL_CTL_Reserved0___MASK                             	UINT32_C(0x2000)
#define RUDRA40_GLUE_GENERAL_CTL_Reserved0___SHIFT                            	13
#define RUDRA40_GLUE_GENERAL_CTL_Unused_11___MASK                             	UINT32_C(0x1800)
#define RUDRA40_GLUE_GENERAL_CTL_Unused_11___SHIFT                            	11
#define RUDRA40_GLUE_GENERAL_CTL_force_socket_monitoring_check_to_fail___MASK 	UINT32_C(0x400)
#define RUDRA40_GLUE_GENERAL_CTL_force_socket_monitoring_check_to_fail___SHIFT	10
#define RUDRA40_GLUE_GENERAL_CTL_socket_monitoring_bypass_bit___MASK          	UINT32_C(0x200)
#define RUDRA40_GLUE_GENERAL_CTL_socket_monitoring_bypass_bit___SHIFT         	9
#define RUDRA40_GLUE_GENERAL_CTL_main_i2c_ioexp_diag_mux_sel___MASK           	UINT32_C(0x180)
#define RUDRA40_GLUE_GENERAL_CTL_main_i2c_ioexp_diag_mux_sel___SHIFT          	7
#define RUDRA40_GLUE_GENERAL_CTL_ant_pwr_enb___MASK                           	UINT32_C(0x40)
#define RUDRA40_GLUE_GENERAL_CTL_ant_pwr_enb___SHIFT                          	6
#define RUDRA40_GLUE_GENERAL_CTL_ant_pwr_p3v3_sel___MASK                      	UINT32_C(0x20)
#define RUDRA40_GLUE_GENERAL_CTL_ant_pwr_p3v3_sel___SHIFT                     	5
#define RUDRA40_GLUE_GENERAL_CTL_Unused_3___MASK                              	UINT32_C(0x18)
#define RUDRA40_GLUE_GENERAL_CTL_Unused_3___SHIFT                             	3
#define RUDRA40_GLUE_GENERAL_CTL_ZARL_PRG_DONE___MASK                         	UINT32_C(0x4)
#define RUDRA40_GLUE_GENERAL_CTL_ZARL_PRG_DONE___SHIFT                        	2
#define RUDRA40_GLUE_GENERAL_CTL_Unused_1___MASK                              	UINT32_C(0x2)
#define RUDRA40_GLUE_GENERAL_CTL_Unused_1___SHIFT                             	1
#define RUDRA40_GLUE_GENERAL_CTL_J2CPA_DISCONNECT_3V3___MASK                  	UINT32_C(0x1)
#define RUDRA40_GLUE_GENERAL_CTL_J2CPA_DISCONNECT_3V3___SHIFT                 	0
#define RUDRA40_GLUE_GENERAL_CTL____REGMASK	UINT32_C(4286474213)

/* ---- RUDRA40_GLUE_LED_SYS_STATUS ---- */
#define RUDRA40_GLUE_LED_SYS_STATUS____WIDTH	32
#define RUDRA40_GLUE_LED_SYS_STATUS____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SYS_STATUS_Unused_28___MASK            	UINT32_C(0xf0000000)
#define RUDRA40_GLUE_LED_SYS_STATUS_Unused_28___SHIFT           	28
#define RUDRA40_GLUE_LED_SYS_STATUS_sw_override_phy_leds___MASK 	UINT32_C(0x8000000)
#define RUDRA40_GLUE_LED_SYS_STATUS_sw_override_phy_leds___SHIFT	27
#define RUDRA40_GLUE_LED_SYS_STATUS_sw_override_all_leds___MASK 	UINT32_C(0x4000000)
#define RUDRA40_GLUE_LED_SYS_STATUS_sw_override_all_leds___SHIFT	26
#define RUDRA40_GLUE_LED_SYS_STATUS_blink_all_leds___MASK       	UINT32_C(0x2000000)
#define RUDRA40_GLUE_LED_SYS_STATUS_blink_all_leds___SHIFT      	25
#define RUDRA40_GLUE_LED_SYS_STATUS_enable_all_leds___MASK      	UINT32_C(0x1000000)
#define RUDRA40_GLUE_LED_SYS_STATUS_enable_all_leds___SHIFT     	24
#define RUDRA40_GLUE_LED_SYS_STATUS_Unused_20___MASK            	UINT32_C(0xf00000)
#define RUDRA40_GLUE_LED_SYS_STATUS_Unused_20___SHIFT           	20
#define RUDRA40_GLUE_LED_SYS_STATUS_Reserved___MASK             	UINT32_C(0xf8000)
#define RUDRA40_GLUE_LED_SYS_STATUS_Reserved___SHIFT            	15
#define RUDRA40_GLUE_LED_SYS_STATUS_debug_led_blink_en___MASK   	UINT32_C(0x4000)
#define RUDRA40_GLUE_LED_SYS_STATUS_debug_led_blink_en___SHIFT  	14
#define RUDRA40_GLUE_LED_SYS_STATUS_debug_led_glow___MASK       	UINT32_C(0x2000)
#define RUDRA40_GLUE_LED_SYS_STATUS_debug_led_glow___SHIFT      	13
#define RUDRA40_GLUE_LED_SYS_STATUS_Unused_0___MASK             	UINT32_C(0x1fff)
#define RUDRA40_GLUE_LED_SYS_STATUS_Unused_0___SHIFT            	0
#define RUDRA40_GLUE_LED_SYS_STATUS____REGMASK	UINT32_C(252698624)

/* ---- RUDRA40_GLUE_RESERVED4 ---- */
#define RUDRA40_GLUE_RESERVED4____WIDTH	32
#define RUDRA40_GLUE_RESERVED4____TYPE 	uint32_t

#define RUDRA40_GLUE_RESERVED4____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_RESERVED5 ---- */
#define RUDRA40_GLUE_RESERVED5____WIDTH	32
#define RUDRA40_GLUE_RESERVED5____TYPE 	uint32_t

#define RUDRA40_GLUE_RESERVED5____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_RESERVED6 ---- */
#define RUDRA40_GLUE_RESERVED6____WIDTH	32
#define RUDRA40_GLUE_RESERVED6____TYPE 	uint32_t

#define RUDRA40_GLUE_RESERVED6____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_RESERVED7 ---- */
#define RUDRA40_GLUE_RESERVED7____WIDTH	32
#define RUDRA40_GLUE_RESERVED7____TYPE 	uint32_t

#define RUDRA40_GLUE_RESERVED7____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_MSI_CTRL ---- */
#define RUDRA40_GLUE_MSI_CTRL____WIDTH	32
#define RUDRA40_GLUE_MSI_CTRL____TYPE 	uint32_t

#define RUDRA40_GLUE_MSI_CTRL_Unused_5___MASK     	UINT32_C(0xffffffe0)
#define RUDRA40_GLUE_MSI_CTRL_Unused_5___SHIFT    	5
#define RUDRA40_GLUE_MSI_CTRL_MSI_INTERVAL___MASK 	UINT32_C(0x1f)
#define RUDRA40_GLUE_MSI_CTRL_MSI_INTERVAL___SHIFT	0
#define RUDRA40_GLUE_MSI_CTRL____REGMASK	UINT32_C(31)

/* ---- RUDRA40_GLUE_IST_MASTER_EVENT ---- */
#define RUDRA40_GLUE_IST_MASTER_EVENT____WIDTH	32
#define RUDRA40_GLUE_IST_MASTER_EVENT____TYPE 	uint32_t

#define RUDRA40_GLUE_IST_MASTER_EVENT_Unused_15___MASK 	UINT32_C(0xffff8000)
#define RUDRA40_GLUE_IST_MASTER_EVENT_Unused_15___SHIFT	15
#define RUDRA40_GLUE_IST_MASTER_EVENT_TST___MASK       	UINT32_C(0x7fff)
#define RUDRA40_GLUE_IST_MASTER_EVENT_TST___SHIFT      	0
#define RUDRA40_GLUE_IST_MASTER_EVENT____REGMASK	UINT32_C(32767)

/* ---- RUDRA40_GLUE_ISR_MASTER_EVENT ---- */
#define RUDRA40_GLUE_ISR_MASTER_EVENT____WIDTH	32
#define RUDRA40_GLUE_ISR_MASTER_EVENT____TYPE 	uint32_t

#define RUDRA40_GLUE_ISR_MASTER_EVENT_Unused_15___MASK          	UINT32_C(0xffff8000)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_Unused_15___SHIFT         	15
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFPDD_TX_FAULT___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFPDD_TX_FAULT___SHIFT	14
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFPDD_RX_LOS___MASK   	UINT32_C(0x2000)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFPDD_RX_LOS___SHIFT  	13
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_QSFP_PWR_GD___MASK    	UINT32_C(0x1000)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_QSFP_PWR_GD___SHIFT   	12
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFP_PWR_GD___MASK     	UINT32_C(0x800)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFP_PWR_GD___SHIFT    	11
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_TEMP___MASK           	UINT32_C(0x400)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_TEMP___SHIFT          	10
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SW_I2C___MASK         	UINT32_C(0x200)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SW_I2C___SHIFT        	9
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_MISC___MASK           	UINT32_C(0x100)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_MISC___SHIFT          	8
#define RUDRA40_GLUE_ISR_MASTER_EVENT_Unused_7___MASK           	UINT32_C(0x80)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_Unused_7___SHIFT          	7
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_QSFP_LOS___MASK       	UINT32_C(0x40)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_QSFP_LOS___SHIFT      	6
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_QSFP_PRESENT___MASK   	UINT32_C(0x20)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_QSFP_PRESENT___SHIFT  	5
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFP_PRESENT___MASK    	UINT32_C(0x10)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFP_PRESENT___SHIFT   	4
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFP_TX_FAULT___MASK   	UINT32_C(0x8)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFP_TX_FAULT___SHIFT  	3
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFP_RX_LOS___MASK     	UINT32_C(0x4)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_ANY_SFP_RX_LOS___SHIFT    	2
#define RUDRA40_GLUE_ISR_MASTER_EVENT_Unused_0___MASK           	UINT32_C(0x3)
#define RUDRA40_GLUE_ISR_MASTER_EVENT_Unused_0___SHIFT          	0
#define RUDRA40_GLUE_ISR_MASTER_EVENT____REGMASK	UINT32_C(32636)

/* ---- RUDRA40_GLUE_ISM_MASTER_EVENT ---- */
#define RUDRA40_GLUE_ISM_MASTER_EVENT____WIDTH	32
#define RUDRA40_GLUE_ISM_MASTER_EVENT____TYPE 	uint32_t

#define RUDRA40_GLUE_ISM_MASTER_EVENT_Unused_15___MASK          	UINT32_C(0xffff8000)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_Unused_15___SHIFT         	15
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFPDD_TX_FAULT___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFPDD_TX_FAULT___SHIFT	14
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFPDD_RX_LOS___MASK   	UINT32_C(0x2000)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFPDD_RX_LOS___SHIFT  	13
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_QSFP_PWR_GD___MASK    	UINT32_C(0x1000)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_QSFP_PWR_GD___SHIFT   	12
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFP_PWR_GD___MASK     	UINT32_C(0x800)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFP_PWR_GD___SHIFT    	11
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_TEMP___MASK           	UINT32_C(0x400)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_TEMP___SHIFT          	10
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SW_I2C___MASK         	UINT32_C(0x200)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SW_I2C___SHIFT        	9
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_MISC___MASK           	UINT32_C(0x100)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_MISC___SHIFT          	8
#define RUDRA40_GLUE_ISM_MASTER_EVENT_Unused_7___MASK           	UINT32_C(0x80)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_Unused_7___SHIFT          	7
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_QSFP_LOS___MASK       	UINT32_C(0x40)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_QSFP_LOS___SHIFT      	6
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_QSFP_PRESENT___MASK   	UINT32_C(0x20)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_QSFP_PRESENT___SHIFT  	5
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFP_PRESENT___MASK    	UINT32_C(0x10)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFP_PRESENT___SHIFT   	4
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFP_TX_FAULT___MASK   	UINT32_C(0x8)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFP_TX_FAULT___SHIFT  	3
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFP_RX_LOS___MASK     	UINT32_C(0x4)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_ANY_SFP_RX_LOS___SHIFT    	2
#define RUDRA40_GLUE_ISM_MASTER_EVENT_Unused_0___MASK           	UINT32_C(0x3)
#define RUDRA40_GLUE_ISM_MASTER_EVENT_Unused_0___SHIFT          	0
#define RUDRA40_GLUE_ISM_MASTER_EVENT____REGMASK	UINT32_C(32636)

/* ---- RUDRA40_GLUE_ISR_SW_I2C ---- */
#define RUDRA40_GLUE_ISR_SW_I2C____WIDTH	32
#define RUDRA40_GLUE_ISR_SW_I2C____TYPE 	uint32_t

#define RUDRA40_GLUE_ISR_SW_I2C_Unused_4___MASK                     	UINT32_C(0xfffffff0)
#define RUDRA40_GLUE_ISR_SW_I2C_Unused_4___SHIFT                    	4
#define RUDRA40_GLUE_ISR_SW_I2C_PWRGD_SW_I2C_MASTER_DONE___MASK     	UINT32_C(0x8)
#define RUDRA40_GLUE_ISR_SW_I2C_PWRGD_SW_I2C_MASTER_DONE___SHIFT    	3
#define RUDRA40_GLUE_ISR_SW_I2C_J2C_IOEXP_SW_I2C_MASTER_DONE___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_ISR_SW_I2C_J2C_IOEXP_SW_I2C_MASTER_DONE___SHIFT	2
#define RUDRA40_GLUE_ISR_SW_I2C_SFP_SW_I2C_MASTER_DONE___MASK       	UINT32_C(0x2)
#define RUDRA40_GLUE_ISR_SW_I2C_SFP_SW_I2C_MASTER_DONE___SHIFT      	1
#define RUDRA40_GLUE_ISR_SW_I2C_MB_SW_I2C_MASTER_DONE___MASK        	UINT32_C(0x1)
#define RUDRA40_GLUE_ISR_SW_I2C_MB_SW_I2C_MASTER_DONE___SHIFT       	0
#define RUDRA40_GLUE_ISR_SW_I2C____REGMASK	UINT32_C(15)

/* ---- RUDRA40_GLUE_ISM_SW_I2C ---- */
#define RUDRA40_GLUE_ISM_SW_I2C____WIDTH	32
#define RUDRA40_GLUE_ISM_SW_I2C____TYPE 	uint32_t

#define RUDRA40_GLUE_ISM_SW_I2C_Unused_4___MASK                     	UINT32_C(0xfffffff0)
#define RUDRA40_GLUE_ISM_SW_I2C_Unused_4___SHIFT                    	4
#define RUDRA40_GLUE_ISM_SW_I2C_PWRGD_SW_I2C_MASTER_DONE___MASK     	UINT32_C(0x8)
#define RUDRA40_GLUE_ISM_SW_I2C_PWRGD_SW_I2C_MASTER_DONE___SHIFT    	3
#define RUDRA40_GLUE_ISM_SW_I2C_J2C_IOEXP_SW_I2C_MASTER_DONE___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_ISM_SW_I2C_J2C_IOEXP_SW_I2C_MASTER_DONE___SHIFT	2
#define RUDRA40_GLUE_ISM_SW_I2C_SFP_SW_I2C_MASTER_DONE___MASK       	UINT32_C(0x2)
#define RUDRA40_GLUE_ISM_SW_I2C_SFP_SW_I2C_MASTER_DONE___SHIFT      	1
#define RUDRA40_GLUE_ISM_SW_I2C_MB_SW_I2C_MASTER_DONE___MASK        	UINT32_C(0x1)
#define RUDRA40_GLUE_ISM_SW_I2C_MB_SW_I2C_MASTER_DONE___SHIFT       	0
#define RUDRA40_GLUE_ISM_SW_I2C____REGMASK	UINT32_C(15)

/* ---- RUDRA40_GLUE_ISR_MISC ---- */
#define RUDRA40_GLUE_ISR_MISC____WIDTH	32
#define RUDRA40_GLUE_ISR_MISC____TYPE 	uint32_t

#define RUDRA40_GLUE_ISR_MISC_Unused_31___MASK                  	UINT32_C(0x80000000)
#define RUDRA40_GLUE_ISR_MISC_Unused_31___SHIFT                 	31
#define RUDRA40_GLUE_ISR_MISC_MUX81356_INTR_N_1___MASK          	UINT32_C(0x40000000)
#define RUDRA40_GLUE_ISR_MISC_MUX81356_INTR_N_1___SHIFT         	30
#define RUDRA40_GLUE_ISR_MISC_MUX81356_INTR_N_0___MASK          	UINT32_C(0x20000000)
#define RUDRA40_GLUE_ISR_MISC_MUX81356_INTR_N_0___SHIFT         	29
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT6_N___MASK     	UINT32_C(0x10000000)
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT6_N___SHIFT    	28
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT5_N___MASK     	UINT32_C(0x8000000)
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT5_N___SHIFT    	27
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT4_N___MASK     	UINT32_C(0x4000000)
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT4_N___SHIFT    	26
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT3_N___MASK     	UINT32_C(0x2000000)
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT3_N___SHIFT    	25
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT2_N___MASK     	UINT32_C(0x1000000)
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT2_N___SHIFT    	24
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT1_N___MASK     	UINT32_C(0x800000)
#define RUDRA40_GLUE_ISR_MISC_I2C_STATUS2_BUS_INT1_N___SHIFT    	23
#define RUDRA40_GLUE_ISR_MISC_CPU_BOARD_SEATED_N___MASK         	UINT32_C(0x400000)
#define RUDRA40_GLUE_ISR_MISC_CPU_BOARD_SEATED_N___SHIFT        	22
#define RUDRA40_GLUE_ISR_MISC_MB_HOT_ALERT_N___MASK             	UINT32_C(0x200000)
#define RUDRA40_GLUE_ISR_MISC_MB_HOT_ALERT_N___SHIFT            	21
#define RUDRA40_GLUE_ISR_MISC_MB_WARM_ALERT_N___MASK            	UINT32_C(0x100000)
#define RUDRA40_GLUE_ISR_MISC_MB_WARM_ALERT_N___SHIFT           	20
#define RUDRA40_GLUE_ISR_MISC_ZL30603_IRQ___MASK                	UINT32_C(0x80000)
#define RUDRA40_GLUE_ISR_MISC_ZL30603_IRQ___SHIFT               	19
#define RUDRA40_GLUE_ISR_MISC_Unused_18___MASK                  	UINT32_C(0x40000)
#define RUDRA40_GLUE_ISR_MISC_Unused_18___SHIFT                 	18
#define RUDRA40_GLUE_ISR_MISC_HBM_THERM_ALARM_J2CPA___MASK      	UINT32_C(0x20000)
#define RUDRA40_GLUE_ISR_MISC_HBM_THERM_ALARM_J2CPA___SHIFT     	17
#define RUDRA40_GLUE_ISR_MISC_RJ45_UART_DATA_RCVD___MASK        	UINT32_C(0x10000)
#define RUDRA40_GLUE_ISR_MISC_RJ45_UART_DATA_RCVD___SHIFT       	16
#define RUDRA40_GLUE_ISR_MISC_Unused_15___MASK                  	UINT32_C(0x8000)
#define RUDRA40_GLUE_ISR_MISC_Unused_15___SHIFT                 	15
#define RUDRA40_GLUE_ISR_MISC_J2CA_TS_SYNC___MASK               	UINT32_C(0x4000)
#define RUDRA40_GLUE_ISR_MISC_J2CA_TS_SYNC___SHIFT              	14
#define RUDRA40_GLUE_ISR_MISC_pps_src_sel___MASK                	UINT32_C(0x2000)
#define RUDRA40_GLUE_ISR_MISC_pps_src_sel___SHIFT               	13
#define RUDRA40_GLUE_ISR_MISC_gps_siril_1pps___MASK             	UINT32_C(0x1000)
#define RUDRA40_GLUE_ISR_MISC_gps_siril_1pps___SHIFT            	12
#define RUDRA40_GLUE_ISR_MISC_shifted_pps_location___MASK       	UINT32_C(0x800)
#define RUDRA40_GLUE_ISR_MISC_shifted_pps_location___SHIFT      	11
#define RUDRA40_GLUE_ISR_MISC_SW_UART_DATA_RCVD___MASK          	UINT32_C(0x400)
#define RUDRA40_GLUE_ISR_MISC_SW_UART_DATA_RCVD___SHIFT         	10
#define RUDRA40_GLUE_ISR_MISC_unused2___MASK                    	UINT32_C(0x200)
#define RUDRA40_GLUE_ISR_MISC_unused2___SHIFT                   	9
#define RUDRA40_GLUE_ISR_MISC_unused1___MASK                    	UINT32_C(0x100)
#define RUDRA40_GLUE_ISR_MISC_unused1___SHIFT                   	8
#define RUDRA40_GLUE_ISR_MISC_CFPGA_IOEXP_PG_INT_N___MASK       	UINT32_C(0x80)
#define RUDRA40_GLUE_ISR_MISC_CFPGA_IOEXP_PG_INT_N___SHIFT      	7
#define RUDRA40_GLUE_ISR_MISC_CFPGA_IOEXP_LOCK_INT_N_3V3___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_ISR_MISC_CFPGA_IOEXP_LOCK_INT_N_3V3___SHIFT	6
#define RUDRA40_GLUE_ISR_MISC_Unused_5___MASK                   	UINT32_C(0x20)
#define RUDRA40_GLUE_ISR_MISC_Unused_5___SHIFT                  	5
#define RUDRA40_GLUE_ISR_MISC_J2CPA_SI5345_156M_INTR_N___MASK   	UINT32_C(0x10)
#define RUDRA40_GLUE_ISR_MISC_J2CPA_SI5345_156M_INTR_N___SHIFT  	4
#define RUDRA40_GLUE_ISR_MISC_J2CP_SI5345_50M_INTR_N___MASK     	UINT32_C(0x8)
#define RUDRA40_GLUE_ISR_MISC_J2CP_SI5345_50M_INTR_N___SHIFT    	3
#define RUDRA40_GLUE_ISR_MISC_Unused_2___MASK                   	UINT32_C(0x4)
#define RUDRA40_GLUE_ISR_MISC_Unused_2___SHIFT                  	2
#define RUDRA40_GLUE_ISR_MISC_J2CPA_INT_N___MASK                	UINT32_C(0x2)
#define RUDRA40_GLUE_ISR_MISC_J2CPA_INT_N___SHIFT               	1
#define RUDRA40_GLUE_ISR_MISC_Rsvd___MASK                       	UINT32_C(0x1)
#define RUDRA40_GLUE_ISR_MISC_Rsvd___SHIFT                      	0
#define RUDRA40_GLUE_ISR_MISC____REGMASK	UINT32_C(2147188699)

/* ---- RUDRA40_GLUE_ISM_MISC ---- */
#define RUDRA40_GLUE_ISM_MISC____WIDTH	32
#define RUDRA40_GLUE_ISM_MISC____TYPE 	uint32_t

#define RUDRA40_GLUE_ISM_MISC_Unused_31___MASK                  	UINT32_C(0x80000000)
#define RUDRA40_GLUE_ISM_MISC_Unused_31___SHIFT                 	31
#define RUDRA40_GLUE_ISM_MISC_MUX81356_INTR_N_1___MASK          	UINT32_C(0x40000000)
#define RUDRA40_GLUE_ISM_MISC_MUX81356_INTR_N_1___SHIFT         	30
#define RUDRA40_GLUE_ISM_MISC_MUX81356_INTR_N_0___MASK          	UINT32_C(0x20000000)
#define RUDRA40_GLUE_ISM_MISC_MUX81356_INTR_N_0___SHIFT         	29
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT6_N___MASK     	UINT32_C(0x10000000)
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT6_N___SHIFT    	28
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT5_N___MASK     	UINT32_C(0x8000000)
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT5_N___SHIFT    	27
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT4_N___MASK     	UINT32_C(0x4000000)
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT4_N___SHIFT    	26
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT3_N___MASK     	UINT32_C(0x2000000)
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT3_N___SHIFT    	25
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT2_N___MASK     	UINT32_C(0x1000000)
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT2_N___SHIFT    	24
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT1_N___MASK     	UINT32_C(0x800000)
#define RUDRA40_GLUE_ISM_MISC_I2C_STATUS2_BUS_INT1_N___SHIFT    	23
#define RUDRA40_GLUE_ISM_MISC_CPU_BOARD_SEATED_N___MASK         	UINT32_C(0x400000)
#define RUDRA40_GLUE_ISM_MISC_CPU_BOARD_SEATED_N___SHIFT        	22
#define RUDRA40_GLUE_ISM_MISC_MB_HOT_ALERT_N___MASK             	UINT32_C(0x200000)
#define RUDRA40_GLUE_ISM_MISC_MB_HOT_ALERT_N___SHIFT            	21
#define RUDRA40_GLUE_ISM_MISC_MB_WARM_ALERT_N___MASK            	UINT32_C(0x100000)
#define RUDRA40_GLUE_ISM_MISC_MB_WARM_ALERT_N___SHIFT           	20
#define RUDRA40_GLUE_ISM_MISC_ZL30603_IRQ___MASK                	UINT32_C(0x80000)
#define RUDRA40_GLUE_ISM_MISC_ZL30603_IRQ___SHIFT               	19
#define RUDRA40_GLUE_ISM_MISC_Unused_18___MASK                  	UINT32_C(0x40000)
#define RUDRA40_GLUE_ISM_MISC_Unused_18___SHIFT                 	18
#define RUDRA40_GLUE_ISM_MISC_HBM_THERM_ALARM_J2CPA___MASK      	UINT32_C(0x20000)
#define RUDRA40_GLUE_ISM_MISC_HBM_THERM_ALARM_J2CPA___SHIFT     	17
#define RUDRA40_GLUE_ISM_MISC_RJ45_UART_DATA_RCVD___MASK        	UINT32_C(0x10000)
#define RUDRA40_GLUE_ISM_MISC_RJ45_UART_DATA_RCVD___SHIFT       	16
#define RUDRA40_GLUE_ISM_MISC_Unused_15___MASK                  	UINT32_C(0x8000)
#define RUDRA40_GLUE_ISM_MISC_Unused_15___SHIFT                 	15
#define RUDRA40_GLUE_ISM_MISC_J2CA_TS_SYNC___MASK               	UINT32_C(0x4000)
#define RUDRA40_GLUE_ISM_MISC_J2CA_TS_SYNC___SHIFT              	14
#define RUDRA40_GLUE_ISM_MISC_pps_src_sel___MASK                	UINT32_C(0x2000)
#define RUDRA40_GLUE_ISM_MISC_pps_src_sel___SHIFT               	13
#define RUDRA40_GLUE_ISM_MISC_gps_siril_1pps___MASK             	UINT32_C(0x1000)
#define RUDRA40_GLUE_ISM_MISC_gps_siril_1pps___SHIFT            	12
#define RUDRA40_GLUE_ISM_MISC_shifted_pps_location___MASK       	UINT32_C(0x800)
#define RUDRA40_GLUE_ISM_MISC_shifted_pps_location___SHIFT      	11
#define RUDRA40_GLUE_ISM_MISC_SW_UART_DATA_RCVD___MASK          	UINT32_C(0x400)
#define RUDRA40_GLUE_ISM_MISC_SW_UART_DATA_RCVD___SHIFT         	10
#define RUDRA40_GLUE_ISM_MISC_Unused2___MASK                    	UINT32_C(0x200)
#define RUDRA40_GLUE_ISM_MISC_Unused2___SHIFT                   	9
#define RUDRA40_GLUE_ISM_MISC_Unused1___MASK                    	UINT32_C(0x100)
#define RUDRA40_GLUE_ISM_MISC_Unused1___SHIFT                   	8
#define RUDRA40_GLUE_ISM_MISC_CFPGA_IOEXP_PG_INT_N___MASK       	UINT32_C(0x80)
#define RUDRA40_GLUE_ISM_MISC_CFPGA_IOEXP_PG_INT_N___SHIFT      	7
#define RUDRA40_GLUE_ISM_MISC_CFPGA_IOEXP_LOCK_INT_N_3V3___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_ISM_MISC_CFPGA_IOEXP_LOCK_INT_N_3V3___SHIFT	6
#define RUDRA40_GLUE_ISM_MISC_Unused_5___MASK                   	UINT32_C(0x20)
#define RUDRA40_GLUE_ISM_MISC_Unused_5___SHIFT                  	5
#define RUDRA40_GLUE_ISM_MISC_J2CPA_SI5345_156M_INTR_N___MASK   	UINT32_C(0x10)
#define RUDRA40_GLUE_ISM_MISC_J2CPA_SI5345_156M_INTR_N___SHIFT  	4
#define RUDRA40_GLUE_ISM_MISC_J2CP_SI5345_50M_INTR_N___MASK     	UINT32_C(0x8)
#define RUDRA40_GLUE_ISM_MISC_J2CP_SI5345_50M_INTR_N___SHIFT    	3
#define RUDRA40_GLUE_ISM_MISC_Unused_2___MASK                   	UINT32_C(0x4)
#define RUDRA40_GLUE_ISM_MISC_Unused_2___SHIFT                  	2
#define RUDRA40_GLUE_ISM_MISC_J2CPA_INT_N___MASK                	UINT32_C(0x2)
#define RUDRA40_GLUE_ISM_MISC_J2CPA_INT_N___SHIFT               	1
#define RUDRA40_GLUE_ISM_MISC_Rsvd___MASK                       	UINT32_C(0x1)
#define RUDRA40_GLUE_ISM_MISC_Rsvd___SHIFT                      	0
#define RUDRA40_GLUE_ISM_MISC____REGMASK	UINT32_C(2147187931)

/* ---- RUDRA40_GLUE_STATUS_MISC ---- */
#define RUDRA40_GLUE_STATUS_MISC____WIDTH	32
#define RUDRA40_GLUE_STATUS_MISC____TYPE 	uint32_t

#define RUDRA40_GLUE_STATUS_MISC_Unused_31___MASK                  	UINT32_C(0x80000000)
#define RUDRA40_GLUE_STATUS_MISC_Unused_31___SHIFT                 	31
#define RUDRA40_GLUE_STATUS_MISC_MUX81356_INTR_N_1___MASK          	UINT32_C(0x40000000)
#define RUDRA40_GLUE_STATUS_MISC_MUX81356_INTR_N_1___SHIFT         	30
#define RUDRA40_GLUE_STATUS_MISC_MUX81356_INTR_N_0___MASK          	UINT32_C(0x20000000)
#define RUDRA40_GLUE_STATUS_MISC_MUX81356_INTR_N_0___SHIFT         	29
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT6_N___MASK     	UINT32_C(0x10000000)
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT6_N___SHIFT    	28
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT5_N___MASK     	UINT32_C(0x8000000)
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT5_N___SHIFT    	27
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT4_N___MASK     	UINT32_C(0x4000000)
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT4_N___SHIFT    	26
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT3_N___MASK     	UINT32_C(0x2000000)
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT3_N___SHIFT    	25
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT2_N___MASK     	UINT32_C(0x1000000)
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT2_N___SHIFT    	24
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT1_N___MASK     	UINT32_C(0x800000)
#define RUDRA40_GLUE_STATUS_MISC_I2C_STATUS2_BUS_INT1_N___SHIFT    	23
#define RUDRA40_GLUE_STATUS_MISC_CPU_BOARD_SEATED_N___MASK         	UINT32_C(0x400000)
#define RUDRA40_GLUE_STATUS_MISC_CPU_BOARD_SEATED_N___SHIFT        	22
#define RUDRA40_GLUE_STATUS_MISC_MB_HOT_ALERT_N___MASK             	UINT32_C(0x200000)
#define RUDRA40_GLUE_STATUS_MISC_MB_HOT_ALERT_N___SHIFT            	21
#define RUDRA40_GLUE_STATUS_MISC_MB_WARM_ALERT_N___MASK            	UINT32_C(0x100000)
#define RUDRA40_GLUE_STATUS_MISC_MB_WARM_ALERT_N___SHIFT           	20
#define RUDRA40_GLUE_STATUS_MISC_ZL30603_IRQ___MASK                	UINT32_C(0x80000)
#define RUDRA40_GLUE_STATUS_MISC_ZL30603_IRQ___SHIFT               	19
#define RUDRA40_GLUE_STATUS_MISC_Unused_18___MASK                  	UINT32_C(0x40000)
#define RUDRA40_GLUE_STATUS_MISC_Unused_18___SHIFT                 	18
#define RUDRA40_GLUE_STATUS_MISC_HBM_THERM_ALARM_J2CPA___MASK      	UINT32_C(0x20000)
#define RUDRA40_GLUE_STATUS_MISC_HBM_THERM_ALARM_J2CPA___SHIFT     	17
#define RUDRA40_GLUE_STATUS_MISC_RJ45_UART_DATA_RCVD___MASK        	UINT32_C(0x10000)
#define RUDRA40_GLUE_STATUS_MISC_RJ45_UART_DATA_RCVD___SHIFT       	16
#define RUDRA40_GLUE_STATUS_MISC_Unused_15___MASK                  	UINT32_C(0x8000)
#define RUDRA40_GLUE_STATUS_MISC_Unused_15___SHIFT                 	15
#define RUDRA40_GLUE_STATUS_MISC_J2CA_TS_SYNC___MASK               	UINT32_C(0x4000)
#define RUDRA40_GLUE_STATUS_MISC_J2CA_TS_SYNC___SHIFT              	14
#define RUDRA40_GLUE_STATUS_MISC_pps_src_sel___MASK                	UINT32_C(0x2000)
#define RUDRA40_GLUE_STATUS_MISC_pps_src_sel___SHIFT               	13
#define RUDRA40_GLUE_STATUS_MISC_gps_siril_1pps___MASK             	UINT32_C(0x1000)
#define RUDRA40_GLUE_STATUS_MISC_gps_siril_1pps___SHIFT            	12
#define RUDRA40_GLUE_STATUS_MISC_shifted_pps_location___MASK       	UINT32_C(0x800)
#define RUDRA40_GLUE_STATUS_MISC_shifted_pps_location___SHIFT      	11
#define RUDRA40_GLUE_STATUS_MISC_SW_UART_DATA_RCVD___MASK          	UINT32_C(0x400)
#define RUDRA40_GLUE_STATUS_MISC_SW_UART_DATA_RCVD___SHIFT         	10
#define RUDRA40_GLUE_STATUS_MISC_unused2___MASK                    	UINT32_C(0x200)
#define RUDRA40_GLUE_STATUS_MISC_unused2___SHIFT                   	9
#define RUDRA40_GLUE_STATUS_MISC_unused1___MASK                    	UINT32_C(0x100)
#define RUDRA40_GLUE_STATUS_MISC_unused1___SHIFT                   	8
#define RUDRA40_GLUE_STATUS_MISC_CFPGA_IOEXP_PG_INT_N___MASK       	UINT32_C(0x80)
#define RUDRA40_GLUE_STATUS_MISC_CFPGA_IOEXP_PG_INT_N___SHIFT      	7
#define RUDRA40_GLUE_STATUS_MISC_CFPGA_IOEXP_LOCK_INT_N_3V3___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_STATUS_MISC_CFPGA_IOEXP_LOCK_INT_N_3V3___SHIFT	6
#define RUDRA40_GLUE_STATUS_MISC_Unused_5___MASK                   	UINT32_C(0x20)
#define RUDRA40_GLUE_STATUS_MISC_Unused_5___SHIFT                  	5
#define RUDRA40_GLUE_STATUS_MISC_J2CPA_SI5345_156M_INTR_N___MASK   	UINT32_C(0x10)
#define RUDRA40_GLUE_STATUS_MISC_J2CPA_SI5345_156M_INTR_N___SHIFT  	4
#define RUDRA40_GLUE_STATUS_MISC_J2CP_SI5345_50M_INTR_N___MASK     	UINT32_C(0x8)
#define RUDRA40_GLUE_STATUS_MISC_J2CP_SI5345_50M_INTR_N___SHIFT    	3
#define RUDRA40_GLUE_STATUS_MISC_Unused_2___MASK                   	UINT32_C(0x4)
#define RUDRA40_GLUE_STATUS_MISC_Unused_2___SHIFT                  	2
#define RUDRA40_GLUE_STATUS_MISC_J2CPA_INT_N___MASK                	UINT32_C(0x2)
#define RUDRA40_GLUE_STATUS_MISC_J2CPA_INT_N___SHIFT               	1
#define RUDRA40_GLUE_STATUS_MISC_Rsvd___MASK                       	UINT32_C(0x1)
#define RUDRA40_GLUE_STATUS_MISC_Rsvd___SHIFT                      	0
#define RUDRA40_GLUE_STATUS_MISC____REGMASK	UINT32_C(2147188699)

/* ---- RUDRA40_GLUE_I2C_SW_IF_SEL ---- */
#define RUDRA40_GLUE_I2C_SW_IF_SEL____WIDTH	32
#define RUDRA40_GLUE_I2C_SW_IF_SEL____TYPE 	uint32_t

#define RUDRA40_GLUE_I2C_SW_IF_SEL_Unused_4___MASK       	UINT32_C(0xfffffff0)
#define RUDRA40_GLUE_I2C_SW_IF_SEL_Unused_4___SHIFT      	4
#define RUDRA40_GLUE_I2C_SW_IF_SEL_bit3_reserved___MASK  	UINT32_C(0x8)
#define RUDRA40_GLUE_I2C_SW_IF_SEL_bit3_reserved___SHIFT 	3
#define RUDRA40_GLUE_I2C_SW_IF_SEL_bit2_reserved___MASK  	UINT32_C(0x4)
#define RUDRA40_GLUE_I2C_SW_IF_SEL_bit2_reserved___SHIFT 	2
#define RUDRA40_GLUE_I2C_SW_IF_SEL_sfp_sel_sw_i2c___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_I2C_SW_IF_SEL_sfp_sel_sw_i2c___SHIFT	1
#define RUDRA40_GLUE_I2C_SW_IF_SEL_mb_sel_sw_i2c___MASK  	UINT32_C(0x1)
#define RUDRA40_GLUE_I2C_SW_IF_SEL_mb_sel_sw_i2c___SHIFT 	0
#define RUDRA40_GLUE_I2C_SW_IF_SEL____REGMASK	UINT32_C(15)

/* ---- RUDRA40_GLUE_I2C_SW_MISC_CTRL ---- */
#define RUDRA40_GLUE_I2C_SW_MISC_CTRL____WIDTH	32
#define RUDRA40_GLUE_I2C_SW_MISC_CTRL____TYPE 	uint32_t

#define RUDRA40_GLUE_I2C_SW_MISC_CTRL_Unused_24___MASK           	UINT32_C(0xff000000)
#define RUDRA40_GLUE_I2C_SW_MISC_CTRL_Unused_24___SHIFT          	24
#define RUDRA40_GLUE_I2C_SW_MISC_CTRL_sfp_i2c_half_period___MASK 	UINT32_C(0xfff000)
#define RUDRA40_GLUE_I2C_SW_MISC_CTRL_sfp_i2c_half_period___SHIFT	12
#define RUDRA40_GLUE_I2C_SW_MISC_CTRL_mb_i2c_half_period___MASK  	UINT32_C(0xfff)
#define RUDRA40_GLUE_I2C_SW_MISC_CTRL_mb_i2c_half_period___SHIFT 	0
#define RUDRA40_GLUE_I2C_SW_MISC_CTRL____REGMASK	UINT32_C(16777215)

/* ---- RUDRA40_GLUE_J2CA_SRD_REFCLK ---- */
#define RUDRA40_GLUE_J2CA_SRD_REFCLK____WIDTH	32
#define RUDRA40_GLUE_J2CA_SRD_REFCLK____TYPE 	uint32_t

#define RUDRA40_GLUE_J2CA_SRD_REFCLK_Unused_24___MASK             	UINT32_C(0xff000000)
#define RUDRA40_GLUE_J2CA_SRD_REFCLK_Unused_24___SHIFT            	24
#define RUDRA40_GLUE_J2CA_SRD_REFCLK_J2CA_srd_refclk2_freq___MASK 	UINT32_C(0xfff000)
#define RUDRA40_GLUE_J2CA_SRD_REFCLK_J2CA_srd_refclk2_freq___SHIFT	12
#define RUDRA40_GLUE_J2CA_SRD_REFCLK_J2CA_srd_refclk1_freq___MASK 	UINT32_C(0xfff)
#define RUDRA40_GLUE_J2CA_SRD_REFCLK_J2CA_srd_refclk1_freq___SHIFT	0
#define RUDRA40_GLUE_J2CA_SRD_REFCLK____REGMASK	UINT32_C(16777215)

/* ---- RUDRA40_GLUE_RESERVED ---- */
#define RUDRA40_GLUE_RESERVED____WIDTH	32
#define RUDRA40_GLUE_RESERVED____TYPE 	uint32_t

#define RUDRA40_GLUE_RESERVED_Unused_24___MASK 	UINT32_C(0xff000000)
#define RUDRA40_GLUE_RESERVED_Unused_24___SHIFT	24
#define RUDRA40_GLUE_RESERVED_Unused_0___MASK  	UINT32_C(0xffffff)
#define RUDRA40_GLUE_RESERVED_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_RESERVED____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_IST_SW_I2C ---- */
#define RUDRA40_GLUE_IST_SW_I2C____WIDTH	32
#define RUDRA40_GLUE_IST_SW_I2C____TYPE 	uint32_t

#define RUDRA40_GLUE_IST_SW_I2C_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_GLUE_IST_SW_I2C_Unused_4___SHIFT	4
#define RUDRA40_GLUE_IST_SW_I2C_TST___MASK      	UINT32_C(0xf)
#define RUDRA40_GLUE_IST_SW_I2C_TST___SHIFT     	0
#define RUDRA40_GLUE_IST_SW_I2C____REGMASK	UINT32_C(15)

/* ---- RUDRA40_GLUE_IST_MISC ---- */
#define RUDRA40_GLUE_IST_MISC____WIDTH	32
#define RUDRA40_GLUE_IST_MISC____TYPE 	uint32_t

#define RUDRA40_GLUE_IST_MISC_Unused_31___MASK 	UINT32_C(0x80000000)
#define RUDRA40_GLUE_IST_MISC_Unused_31___SHIFT	31
#define RUDRA40_GLUE_IST_MISC_TST___MASK       	UINT32_C(0x7fffffff)
#define RUDRA40_GLUE_IST_MISC_TST___SHIFT      	0
#define RUDRA40_GLUE_IST_MISC____REGMASK	UINT32_C(2147483647)

/* ---- RUDRA40_GLUE_DEBUG_CPU_WDT ---- */
#define RUDRA40_GLUE_DEBUG_CPU_WDT____WIDTH	32
#define RUDRA40_GLUE_DEBUG_CPU_WDT____TYPE 	uint32_t

#define RUDRA40_GLUE_DEBUG_CPU_WDT_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_GLUE_DEBUG_CPU_WDT_Unused_1___SHIFT	1
#define RUDRA40_GLUE_DEBUG_CPU_WDT_ENABLE___MASK   	UINT32_C(0x1)
#define RUDRA40_GLUE_DEBUG_CPU_WDT_ENABLE___SHIFT  	0
#define RUDRA40_GLUE_DEBUG_CPU_WDT____REGMASK	UINT32_C(1)

/* ---- RUDRA40_GLUE_UART_SEL ---- */
#define RUDRA40_GLUE_UART_SEL____WIDTH	32
#define RUDRA40_GLUE_UART_SEL____TYPE 	uint32_t

#define RUDRA40_GLUE_UART_SEL_Unused_7___MASK     	UINT32_C(0xffffff80)
#define RUDRA40_GLUE_UART_SEL_Unused_7___SHIFT    	7
#define RUDRA40_GLUE_UART_SEL_uart_mux_sel___MASK 	UINT32_C(0x7f)
#define RUDRA40_GLUE_UART_SEL_uart_mux_sel___SHIFT	0
#define RUDRA40_GLUE_UART_SEL____REGMASK	UINT32_C(127)

/* ---- RUDRA40_GLUE_SPI_SEL ---- */
#define RUDRA40_GLUE_SPI_SEL____WIDTH	32
#define RUDRA40_GLUE_SPI_SEL____TYPE 	uint32_t

#define RUDRA40_GLUE_SPI_SEL_Unused_8___MASK      	UINT32_C(0xffffff00)
#define RUDRA40_GLUE_SPI_SEL_Unused_8___SHIFT     	8
#define RUDRA40_GLUE_SPI_SEL_Rsvd___MASK          	UINT32_C(0xf8)
#define RUDRA40_GLUE_SPI_SEL_Rsvd___SHIFT         	3
#define RUDRA40_GLUE_SPI_SEL_Unused_2___MASK      	UINT32_C(0x4)
#define RUDRA40_GLUE_SPI_SEL_Unused_2___SHIFT     	2
#define RUDRA40_GLUE_SPI_SEL_j2cpa_spi_sel___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_SPI_SEL_j2cpa_spi_sel___SHIFT	1
#define RUDRA40_GLUE_SPI_SEL_spi_sel___MASK       	UINT32_C(0x1)
#define RUDRA40_GLUE_SPI_SEL_spi_sel___SHIFT      	0
#define RUDRA40_GLUE_SPI_SEL____REGMASK	UINT32_C(251)

/* ---- RUDRA40_GLUE_LED_QSFP_RATE_1 ---- */
#define RUDRA40_GLUE_LED_QSFP_RATE_1____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RATE_1____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RATE_1_Unused_30___MASK             	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_Unused_30___SHIFT            	30
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit5___MASK 	UINT32_C(0x20000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit5___SHIFT	29
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit4___MASK 	UINT32_C(0x10000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit4___SHIFT	28
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit3___MASK 	UINT32_C(0x8000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit3___SHIFT	27
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit2___MASK 	UINT32_C(0x4000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit2___SHIFT	26
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit1___MASK 	UINT32_C(0x2000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit1___SHIFT	25
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit0___MASK 	UINT32_C(0x1000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_5_bit0___SHIFT	24
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit5___MASK 	UINT32_C(0x800000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit5___SHIFT	23
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit4___MASK 	UINT32_C(0x400000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit4___SHIFT	22
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit3___MASK 	UINT32_C(0x200000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit3___SHIFT	21
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit2___MASK 	UINT32_C(0x100000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit2___SHIFT	20
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit1___MASK 	UINT32_C(0x80000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit1___SHIFT	19
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit0___MASK 	UINT32_C(0x40000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_4_bit0___SHIFT	18
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit5___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit5___SHIFT	17
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit4___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit4___SHIFT	16
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit3___MASK 	UINT32_C(0x8000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit3___SHIFT	15
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit2___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit2___SHIFT	14
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit1___MASK 	UINT32_C(0x2000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit1___SHIFT	13
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit0___MASK 	UINT32_C(0x1000)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_3_bit0___SHIFT	12
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit5___MASK 	UINT32_C(0x800)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit5___SHIFT	11
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit4___MASK 	UINT32_C(0x400)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit4___SHIFT	10
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit3___MASK 	UINT32_C(0x200)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit3___SHIFT	9
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit2___MASK 	UINT32_C(0x100)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit2___SHIFT	8
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit1___MASK 	UINT32_C(0x80)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit1___SHIFT	7
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit0___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_2_bit0___SHIFT	6
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit5___MASK 	UINT32_C(0x20)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit5___SHIFT	5
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit4___MASK 	UINT32_C(0x10)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit4___SHIFT	4
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit3___MASK 	UINT32_C(0x8)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit3___SHIFT	3
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit2___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit2___SHIFT	2
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit1___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit1___SHIFT	1
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit0___MASK 	UINT32_C(0x1)
#define RUDRA40_GLUE_LED_QSFP_RATE_1_qsfp_rate_port_1_bit0___SHIFT	0
#define RUDRA40_GLUE_LED_QSFP_RATE_1____REGMASK	UINT32_C(1073741823)

/* ---- RUDRA40_GLUE_LED_QSFP_RATE_2 ---- */
#define RUDRA40_GLUE_LED_QSFP_RATE_2____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RATE_2____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RATE_2_Unused_30___MASK             	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_Unused_30___SHIFT            	30
#define RUDRA40_GLUE_LED_QSFP_RATE_2_Reserved___MASK              	UINT32_C(0x3ffc0000)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_Reserved___SHIFT             	18
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit5___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit5___SHIFT	17
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit4___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit4___SHIFT	16
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit3___MASK 	UINT32_C(0x8000)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit3___SHIFT	15
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit2___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit2___SHIFT	14
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit1___MASK 	UINT32_C(0x2000)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit1___SHIFT	13
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit0___MASK 	UINT32_C(0x1000)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_8_bit0___SHIFT	12
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit5___MASK 	UINT32_C(0x800)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit5___SHIFT	11
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit4___MASK 	UINT32_C(0x400)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit4___SHIFT	10
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit3___MASK 	UINT32_C(0x200)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit3___SHIFT	9
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit2___MASK 	UINT32_C(0x100)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit2___SHIFT	8
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit1___MASK 	UINT32_C(0x80)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit1___SHIFT	7
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit0___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_7_bit0___SHIFT	6
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit5___MASK 	UINT32_C(0x20)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit5___SHIFT	5
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit4___MASK 	UINT32_C(0x10)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit4___SHIFT	4
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit3___MASK 	UINT32_C(0x8)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit3___SHIFT	3
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit2___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit2___SHIFT	2
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit1___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit1___SHIFT	1
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit0___MASK 	UINT32_C(0x1)
#define RUDRA40_GLUE_LED_QSFP_RATE_2_qsfp_rate_port_6_bit0___SHIFT	0
#define RUDRA40_GLUE_LED_QSFP_RATE_2____REGMASK	UINT32_C(1073741823)

/* ---- RUDRA40_GLUE_LED_QSFP_RATE_3 ---- */
#define RUDRA40_GLUE_LED_QSFP_RATE_3____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RATE_3____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RATE_3_Unused_30___MASK 	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_3_Unused_30___SHIFT	30
#define RUDRA40_GLUE_LED_QSFP_RATE_3_Unused_0___MASK  	UINT32_C(0x3fffffff)
#define RUDRA40_GLUE_LED_QSFP_RATE_3_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_LED_QSFP_RATE_3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_LED_QSFP_RATE_4 ---- */
#define RUDRA40_GLUE_LED_QSFP_RATE_4____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RATE_4____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RATE_4_Unused_30___MASK 	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_4_Unused_30___SHIFT	30
#define RUDRA40_GLUE_LED_QSFP_RATE_4_Unused_0___MASK  	UINT32_C(0x3fffffff)
#define RUDRA40_GLUE_LED_QSFP_RATE_4_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_LED_QSFP_RATE_4____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_LED_QSFP_RATE_5 ---- */
#define RUDRA40_GLUE_LED_QSFP_RATE_5____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RATE_5____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RATE_5_Unused_30___MASK 	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_5_Unused_30___SHIFT	30
#define RUDRA40_GLUE_LED_QSFP_RATE_5_Unused_0___MASK  	UINT32_C(0x3fffffff)
#define RUDRA40_GLUE_LED_QSFP_RATE_5_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_LED_QSFP_RATE_5____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_LED_QSFP_RATE_6 ---- */
#define RUDRA40_GLUE_LED_QSFP_RATE_6____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RATE_6____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RATE_6_Unused_30___MASK 	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_6_Unused_30___SHIFT	30
#define RUDRA40_GLUE_LED_QSFP_RATE_6_Unused_0___MASK  	UINT32_C(0x3fffffff)
#define RUDRA40_GLUE_LED_QSFP_RATE_6_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_LED_QSFP_RATE_6____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_LED_QSFP_RATE_7 ---- */
#define RUDRA40_GLUE_LED_QSFP_RATE_7____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RATE_7____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RATE_7_Unused_30___MASK 	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_QSFP_RATE_7_Unused_30___SHIFT	30
#define RUDRA40_GLUE_LED_QSFP_RATE_7_Unused_0___MASK  	UINT32_C(0x3fffffff)
#define RUDRA40_GLUE_LED_QSFP_RATE_7_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_LED_QSFP_RATE_7____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_LED_QSFP_RATE_8 ---- */
#define RUDRA40_GLUE_LED_QSFP_RATE_8____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RATE_8____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RATE_8_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_GLUE_LED_QSFP_RATE_8_Unused_6___SHIFT	6
#define RUDRA40_GLUE_LED_QSFP_RATE_8_Unused_0___MASK 	UINT32_C(0x3f)
#define RUDRA40_GLUE_LED_QSFP_RATE_8_Unused_0___SHIFT	0
#define RUDRA40_GLUE_LED_QSFP_RATE_8____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_LED_QSFP_RED_1 ---- */
#define RUDRA40_GLUE_LED_QSFP_RED_1____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RED_1____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RED_1_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_GLUE_LED_QSFP_RED_1_Unused_8___SHIFT	8
#define RUDRA40_GLUE_LED_QSFP_RED_1_qsfp_red___MASK 	UINT32_C(0xff)
#define RUDRA40_GLUE_LED_QSFP_RED_1_qsfp_red___SHIFT	0
#define RUDRA40_GLUE_LED_QSFP_RED_1____REGMASK	UINT32_C(255)

/* ---- RUDRA40_GLUE_LED_QSFP_RED_2 ---- */
#define RUDRA40_GLUE_LED_QSFP_RED_2____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RED_2____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RED_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_GLUE_LED_QSFP_RED_2_Unused_8___SHIFT	8
#define RUDRA40_GLUE_LED_QSFP_RED_2_qsfp_red___MASK 	UINT32_C(0xff)
#define RUDRA40_GLUE_LED_QSFP_RED_2_qsfp_red___SHIFT	0
#define RUDRA40_GLUE_LED_QSFP_RED_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_GLUE_LED_QSFP_RED_3 ---- */
#define RUDRA40_GLUE_LED_QSFP_RED_3____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RED_3____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RED_3_Unused_18___MASK 	UINT32_C(0xfffc0000)
#define RUDRA40_GLUE_LED_QSFP_RED_3_Unused_18___SHIFT	18
#define RUDRA40_GLUE_LED_QSFP_RED_3_Unused_0___MASK  	UINT32_C(0x3ffff)
#define RUDRA40_GLUE_LED_QSFP_RED_3_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_LED_QSFP_RED_3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_LED_QSFP_RED_4 ---- */
#define RUDRA40_GLUE_LED_QSFP_RED_4____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_RED_4____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_RED_4_Unused_18___MASK 	UINT32_C(0xfffc0000)
#define RUDRA40_GLUE_LED_QSFP_RED_4_Unused_18___SHIFT	18
#define RUDRA40_GLUE_LED_QSFP_RED_4_Unused_0___MASK  	UINT32_C(0x3ffff)
#define RUDRA40_GLUE_LED_QSFP_RED_4_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_LED_QSFP_RED_4____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_LED_QSFP_GREEN_1 ---- */
#define RUDRA40_GLUE_LED_QSFP_GREEN_1____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_GREEN_1____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_GREEN_1_Unused_8___MASK   	UINT32_C(0xffffff00)
#define RUDRA40_GLUE_LED_QSFP_GREEN_1_Unused_8___SHIFT  	8
#define RUDRA40_GLUE_LED_QSFP_GREEN_1_qsfp_GREEN___MASK 	UINT32_C(0xff)
#define RUDRA40_GLUE_LED_QSFP_GREEN_1_qsfp_GREEN___SHIFT	0
#define RUDRA40_GLUE_LED_QSFP_GREEN_1____REGMASK	UINT32_C(255)

/* ---- RUDRA40_GLUE_LED_QSFP_GREEN_2 ---- */
#define RUDRA40_GLUE_LED_QSFP_GREEN_2____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_GREEN_2____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_GREEN_2_Unused_8___MASK   	UINT32_C(0xffffff00)
#define RUDRA40_GLUE_LED_QSFP_GREEN_2_Unused_8___SHIFT  	8
#define RUDRA40_GLUE_LED_QSFP_GREEN_2_qsfp_GREEN___MASK 	UINT32_C(0xff)
#define RUDRA40_GLUE_LED_QSFP_GREEN_2_qsfp_GREEN___SHIFT	0
#define RUDRA40_GLUE_LED_QSFP_GREEN_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_GLUE_LED_QSFP_GREEN_3 ---- */
#define RUDRA40_GLUE_LED_QSFP_GREEN_3____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_GREEN_3____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_GREEN_3_Unused_18___MASK 	UINT32_C(0xfffc0000)
#define RUDRA40_GLUE_LED_QSFP_GREEN_3_Unused_18___SHIFT	18
#define RUDRA40_GLUE_LED_QSFP_GREEN_3_Unused_0___MASK  	UINT32_C(0x3ffff)
#define RUDRA40_GLUE_LED_QSFP_GREEN_3_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_LED_QSFP_GREEN_3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_LED_QSFP_GREEN_4 ---- */
#define RUDRA40_GLUE_LED_QSFP_GREEN_4____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_GREEN_4____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_GREEN_4_Unused_18___MASK 	UINT32_C(0xfffc0000)
#define RUDRA40_GLUE_LED_QSFP_GREEN_4_Unused_18___SHIFT	18
#define RUDRA40_GLUE_LED_QSFP_GREEN_4_Unused_0___MASK  	UINT32_C(0x3ffff)
#define RUDRA40_GLUE_LED_QSFP_GREEN_4_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_LED_QSFP_GREEN_4____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_LED_QSFP_BLUE_1 ---- */
#define RUDRA40_GLUE_LED_QSFP_BLUE_1____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_BLUE_1____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_BLUE_1_Unused_8___MASK  	UINT32_C(0xffffff00)
#define RUDRA40_GLUE_LED_QSFP_BLUE_1_Unused_8___SHIFT 	8
#define RUDRA40_GLUE_LED_QSFP_BLUE_1_qsfp_BLUE___MASK 	UINT32_C(0xff)
#define RUDRA40_GLUE_LED_QSFP_BLUE_1_qsfp_BLUE___SHIFT	0
#define RUDRA40_GLUE_LED_QSFP_BLUE_1____REGMASK	UINT32_C(255)

/* ---- RUDRA40_GLUE_LED_QSFP_BLUE_2 ---- */
#define RUDRA40_GLUE_LED_QSFP_BLUE_2____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_BLUE_2____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_BLUE_2_Unused_8___MASK  	UINT32_C(0xffffff00)
#define RUDRA40_GLUE_LED_QSFP_BLUE_2_Unused_8___SHIFT 	8
#define RUDRA40_GLUE_LED_QSFP_BLUE_2_qsfp_BLUE___MASK 	UINT32_C(0xff)
#define RUDRA40_GLUE_LED_QSFP_BLUE_2_qsfp_BLUE___SHIFT	0
#define RUDRA40_GLUE_LED_QSFP_BLUE_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_GLUE_LED_QSFP_BLUE_3 ---- */
#define RUDRA40_GLUE_LED_QSFP_BLUE_3____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_BLUE_3____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_BLUE_3_Unused_18___MASK 	UINT32_C(0xfffc0000)
#define RUDRA40_GLUE_LED_QSFP_BLUE_3_Unused_18___SHIFT	18
#define RUDRA40_GLUE_LED_QSFP_BLUE_3_Unused_0___MASK  	UINT32_C(0x3ffff)
#define RUDRA40_GLUE_LED_QSFP_BLUE_3_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_LED_QSFP_BLUE_3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_LED_QSFP_BLUE_4 ---- */
#define RUDRA40_GLUE_LED_QSFP_BLUE_4____WIDTH	32
#define RUDRA40_GLUE_LED_QSFP_BLUE_4____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_QSFP_BLUE_4_Unused_18___MASK 	UINT32_C(0xfffc0000)
#define RUDRA40_GLUE_LED_QSFP_BLUE_4_Unused_18___SHIFT	18
#define RUDRA40_GLUE_LED_QSFP_BLUE_4_Unused_0___MASK  	UINT32_C(0x3ffff)
#define RUDRA40_GLUE_LED_QSFP_BLUE_4_Unused_0___SHIFT 	0
#define RUDRA40_GLUE_LED_QSFP_BLUE_4____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_HW_TESTING_CTL ---- */
#define RUDRA40_GLUE_HW_TESTING_CTL____WIDTH	32
#define RUDRA40_GLUE_HW_TESTING_CTL____TYPE 	uint32_t

#define RUDRA40_GLUE_HW_TESTING_CTL_unused___MASK              	UINT32_C(0xffffff80)
#define RUDRA40_GLUE_HW_TESTING_CTL_unused___SHIFT             	7
#define RUDRA40_GLUE_HW_TESTING_CTL_Unused_4___MASK            	UINT32_C(0x70)
#define RUDRA40_GLUE_HW_TESTING_CTL_Unused_4___SHIFT           	4
#define RUDRA40_GLUE_HW_TESTING_CTL_sw_rov_values_j2cpa___MASK 	UINT32_C(0xe)
#define RUDRA40_GLUE_HW_TESTING_CTL_sw_rov_values_j2cpa___SHIFT	1
#define RUDRA40_GLUE_HW_TESTING_CTL_sw_override_rov___MASK     	UINT32_C(0x1)
#define RUDRA40_GLUE_HW_TESTING_CTL_sw_override_rov___SHIFT    	0
#define RUDRA40_GLUE_HW_TESTING_CTL____REGMASK	UINT32_C(4294967183)

/* ---- RUDRA40_GLUE_LED_FW_DATA0 ---- */
#define RUDRA40_GLUE_LED_FW_DATA0____WIDTH	32
#define RUDRA40_GLUE_LED_FW_DATA0____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_FW_DATA0_data___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_LED_FW_DATA0_data___SHIFT	0
#define RUDRA40_GLUE_LED_FW_DATA0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_LED_FW_DATA1 ---- */
#define RUDRA40_GLUE_LED_FW_DATA1____WIDTH	32
#define RUDRA40_GLUE_LED_FW_DATA1____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_FW_DATA1_data___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_LED_FW_DATA1_data___SHIFT	0
#define RUDRA40_GLUE_LED_FW_DATA1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_LED_FW_DATA2 ---- */
#define RUDRA40_GLUE_LED_FW_DATA2____WIDTH	32
#define RUDRA40_GLUE_LED_FW_DATA2____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_FW_DATA2_data___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_LED_FW_DATA2_data___SHIFT	0
#define RUDRA40_GLUE_LED_FW_DATA2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_LED_FW_DATA3 ---- */
#define RUDRA40_GLUE_LED_FW_DATA3____WIDTH	32
#define RUDRA40_GLUE_LED_FW_DATA3____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_FW_DATA3_data___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_LED_FW_DATA3_data___SHIFT	0
#define RUDRA40_GLUE_LED_FW_DATA3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_LED_FW_DATA4 ---- */
#define RUDRA40_GLUE_LED_FW_DATA4____WIDTH	32
#define RUDRA40_GLUE_LED_FW_DATA4____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_FW_DATA4_data___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_LED_FW_DATA4_data___SHIFT	0
#define RUDRA40_GLUE_LED_FW_DATA4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_LED_FW_DATA5 ---- */
#define RUDRA40_GLUE_LED_FW_DATA5____WIDTH	32
#define RUDRA40_GLUE_LED_FW_DATA5____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_FW_DATA5_data___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_LED_FW_DATA5_data___SHIFT	0
#define RUDRA40_GLUE_LED_FW_DATA5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_LED_FW_DATA6 ---- */
#define RUDRA40_GLUE_LED_FW_DATA6____WIDTH	32
#define RUDRA40_GLUE_LED_FW_DATA6____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_FW_DATA6_data___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_LED_FW_DATA6_data___SHIFT	0
#define RUDRA40_GLUE_LED_FW_DATA6____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_LED_FW_DATA7 ---- */
#define RUDRA40_GLUE_LED_FW_DATA7____WIDTH	32
#define RUDRA40_GLUE_LED_FW_DATA7____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_FW_DATA7_data___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_LED_FW_DATA7_data___SHIFT	0
#define RUDRA40_GLUE_LED_FW_DATA7____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_LED_FW_DATA8 ---- */
#define RUDRA40_GLUE_LED_FW_DATA8____WIDTH	32
#define RUDRA40_GLUE_LED_FW_DATA8____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_FW_DATA8_data___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_LED_FW_DATA8_data___SHIFT	0
#define RUDRA40_GLUE_LED_FW_DATA8____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PWR_GOOD_STATUS ---- */
#define RUDRA40_GLUE_PWR_GOOD_STATUS____WIDTH	32
#define RUDRA40_GLUE_PWR_GOOD_STATUS____TYPE 	uint32_t

#define RUDRA40_GLUE_PWR_GOOD_STATUS_vddio_1v8_mux_pwr_en___MASK 	UINT32_C(0x80000000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_vddio_1v8_mux_pwr_en___SHIFT	31
#define RUDRA40_GLUE_PWR_GOOD_STATUS_dvdd_0v72_mux_en___MASK     	UINT32_C(0x40000000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_dvdd_0v72_mux_en___SHIFT    	30
#define RUDRA40_GLUE_PWR_GOOD_STATUS_ANT_POWER_PG___MASK         	UINT32_C(0x20000000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_ANT_POWER_PG___SHIFT        	29
#define RUDRA40_GLUE_PWR_GOOD_STATUS_AUX_1V8___MASK              	UINT32_C(0x10000000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_AUX_1V8___SHIFT             	28
#define RUDRA40_GLUE_PWR_GOOD_STATUS_AUX_3V3___MASK              	UINT32_C(0x8000000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_AUX_3V3___SHIFT             	27
#define RUDRA40_GLUE_PWR_GOOD_STATUS_AUX_1V1___MASK              	UINT32_C(0x4000000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_AUX_1V1___SHIFT             	26
#define RUDRA40_GLUE_PWR_GOOD_STATUS_QSFPDD_DCO_2_3V3_EN___MASK  	UINT32_C(0x2000000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_QSFPDD_DCO_2_3V3_EN___SHIFT 	25
#define RUDRA40_GLUE_PWR_GOOD_STATUS_QSFPDD_DCO_1_3V3_EN___MASK  	UINT32_C(0x1000000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_QSFPDD_DCO_1_3V3_EN___SHIFT 	24
#define RUDRA40_GLUE_PWR_GOOD_STATUS_avdd_0v8_mux_pwr_en___MASK  	UINT32_C(0x800000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_avdd_0v8_mux_pwr_en___SHIFT 	23
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_22___MASK            	UINT32_C(0x400000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_22___SHIFT           	22
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_HBM1_VDD1P2_EN___MASK 	UINT32_C(0x200000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_HBM1_VDD1P2_EN___SHIFT	21
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_HBM0_VDD1P2_EN___MASK 	UINT32_C(0x100000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_HBM0_VDD1P2_EN___SHIFT	20
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_18___MASK            	UINT32_C(0xc0000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_18___SHIFT           	18
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_HBM1_VPP2P5_EN___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_HBM1_VPP2P5_EN___SHIFT	17
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_HBM0_VPP2P5_EN___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_HBM0_VPP2P5_EN___SHIFT	16
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_15___MASK            	UINT32_C(0x8000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_15___SHIFT           	15
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_SRD_TVDD1P2_EN___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_SRD_TVDD1P2_EN___SHIFT	14
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_13___MASK            	UINT32_C(0x2000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_13___SHIFT           	13
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_RTVDD0P75_EN___MASK   	UINT32_C(0x1000)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_RTVDD0P75_EN___SHIFT  	12
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_11___MASK            	UINT32_C(0x800)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_11___SHIFT           	11
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_PVDD0P75_EN___MASK    	UINT32_C(0x400)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_PVDD0P75_EN___SHIFT   	10
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_9___MASK             	UINT32_C(0x200)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_9___SHIFT            	9
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_AVDD1P8_EN___MASK     	UINT32_C(0x100)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_AVDD1P8_EN___SHIFT    	8
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_7___MASK             	UINT32_C(0x80)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_7___SHIFT            	7
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_VDDC_EN___MASK        	UINT32_C(0x40)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_VDDC_EN___SHIFT       	6
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_5___MASK             	UINT32_C(0x20)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_Unused_5___SHIFT            	5
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_VDDO1P8_EN___MASK     	UINT32_C(0x10)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2CPA_VDDO1P8_EN___SHIFT    	4
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2C_3V0_PWR_EN___MASK       	UINT32_C(0x8)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_J2C_3V0_PWR_EN___SHIFT      	3
#define RUDRA40_GLUE_PWR_GOOD_STATUS_SYNC_1V8_PWR_EN___MASK      	UINT32_C(0x4)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_SYNC_1V8_PWR_EN___SHIFT     	2
#define RUDRA40_GLUE_PWR_GOOD_STATUS_SYNC_3V3_PWR_EN___MASK      	UINT32_C(0x2)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_SYNC_3V3_PWR_EN___SHIFT     	1
#define RUDRA40_GLUE_PWR_GOOD_STATUS_GLOBAL_5V_PWR_EN___MASK     	UINT32_C(0x1)
#define RUDRA40_GLUE_PWR_GOOD_STATUS_GLOBAL_5V_PWR_EN___SHIFT    	0
#define RUDRA40_GLUE_PWR_GOOD_STATUS____REGMASK	UINT32_C(4289942879)

/* ---- RUDRA40_GLUE_MISC_TESTING_CTL ---- */
#define RUDRA40_GLUE_MISC_TESTING_CTL____WIDTH	32
#define RUDRA40_GLUE_MISC_TESTING_CTL____TYPE 	uint32_t

#define RUDRA40_GLUE_MISC_TESTING_CTL_Unused_2___MASK  	UINT32_C(0xfffffffc)
#define RUDRA40_GLUE_MISC_TESTING_CTL_Unused_2___SHIFT 	2
#define RUDRA40_GLUE_MISC_TESTING_CTL_Reserved2___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_MISC_TESTING_CTL_Reserved2___SHIFT	1
#define RUDRA40_GLUE_MISC_TESTING_CTL_Reserved1___MASK 	UINT32_C(0x1)
#define RUDRA40_GLUE_MISC_TESTING_CTL_Reserved1___SHIFT	0
#define RUDRA40_GLUE_MISC_TESTING_CTL____REGMASK	UINT32_C(3)

/* ---- RUDRA40_GLUE_CPLD_SGPIO_DATA0 ---- */
#define RUDRA40_GLUE_CPLD_SGPIO_DATA0____WIDTH	32
#define RUDRA40_GLUE_CPLD_SGPIO_DATA0____TYPE 	uint32_t

#define RUDRA40_GLUE_CPLD_SGPIO_DATA0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_CPLD_SGPIO_DATA0_DATA___SHIFT	0
#define RUDRA40_GLUE_CPLD_SGPIO_DATA0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_CPLD_SGPIO_DATA1 ---- */
#define RUDRA40_GLUE_CPLD_SGPIO_DATA1____WIDTH	32
#define RUDRA40_GLUE_CPLD_SGPIO_DATA1____TYPE 	uint32_t

#define RUDRA40_GLUE_CPLD_SGPIO_DATA1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_CPLD_SGPIO_DATA1_DATA___SHIFT	0
#define RUDRA40_GLUE_CPLD_SGPIO_DATA1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_CPLD_SGPIO_DATA2 ---- */
#define RUDRA40_GLUE_CPLD_SGPIO_DATA2____WIDTH	32
#define RUDRA40_GLUE_CPLD_SGPIO_DATA2____TYPE 	uint32_t

#define RUDRA40_GLUE_CPLD_SGPIO_DATA2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_CPLD_SGPIO_DATA2_DATA___SHIFT	0
#define RUDRA40_GLUE_CPLD_SGPIO_DATA2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_CPLD_SGPIO_DATA3 ---- */
#define RUDRA40_GLUE_CPLD_SGPIO_DATA3____WIDTH	32
#define RUDRA40_GLUE_CPLD_SGPIO_DATA3____TYPE 	uint32_t

#define RUDRA40_GLUE_CPLD_SGPIO_DATA3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_CPLD_SGPIO_DATA3_DATA___SHIFT	0
#define RUDRA40_GLUE_CPLD_SGPIO_DATA3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_RESERVED_SGPIO ---- */
#define RUDRA40_GLUE_RESERVED_SGPIO____WIDTH	32
#define RUDRA40_GLUE_RESERVED_SGPIO____TYPE 	uint32_t

#define RUDRA40_GLUE_RESERVED_SGPIO_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_RESERVED_SGPIO_DATA___SHIFT	0
#define RUDRA40_GLUE_RESERVED_SGPIO____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_TEST_SGPIO_CTL ---- */
#define RUDRA40_GLUE_TEST_SGPIO_CTL____WIDTH	32
#define RUDRA40_GLUE_TEST_SGPIO_CTL____TYPE 	uint32_t

#define RUDRA40_GLUE_TEST_SGPIO_CTL_Unused_4___MASK                	UINT32_C(0xfffffff0)
#define RUDRA40_GLUE_TEST_SGPIO_CTL_Unused_4___SHIFT               	4
#define RUDRA40_GLUE_TEST_SGPIO_CTL_send_test_data_to_bmc___MASK   	UINT32_C(0x8)
#define RUDRA40_GLUE_TEST_SGPIO_CTL_send_test_data_to_bmc___SHIFT  	3
#define RUDRA40_GLUE_TEST_SGPIO_CTL_send_test_data_to_sutra___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_TEST_SGPIO_CTL_send_test_data_to_sutra___SHIFT	2
#define RUDRA40_GLUE_TEST_SGPIO_CTL_Reserved___MASK                	UINT32_C(0x2)
#define RUDRA40_GLUE_TEST_SGPIO_CTL_Reserved___SHIFT               	1
#define RUDRA40_GLUE_TEST_SGPIO_CTL_Unused_0___MASK                	UINT32_C(0x1)
#define RUDRA40_GLUE_TEST_SGPIO_CTL_Unused_0___SHIFT               	0
#define RUDRA40_GLUE_TEST_SGPIO_CTL____REGMASK	UINT32_C(14)

/* ---- RUDRA40_GLUE_RESERVED0 ---- */
#define RUDRA40_GLUE_RESERVED0____WIDTH	32
#define RUDRA40_GLUE_RESERVED0____TYPE 	uint32_t

#define RUDRA40_GLUE_RESERVED0____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_RESERVED1 ---- */
#define RUDRA40_GLUE_RESERVED1____WIDTH	32
#define RUDRA40_GLUE_RESERVED1____TYPE 	uint32_t

#define RUDRA40_GLUE_RESERVED1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_PMBUS1_VI_MON_DATA0 ---- */
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA0____WIDTH	32
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA0____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA0_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS1_VI_MON_DATA1 ---- */
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA1____WIDTH	32
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA1____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA1_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS1_VI_MON_DATA2 ---- */
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA2____WIDTH	32
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA2____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA2_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS1_VI_MON_DATA3 ---- */
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA3____WIDTH	32
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA3____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA3_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS1_VI_MON_DATA4 ---- */
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA4____WIDTH	32
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA4____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA4_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS1_VI_MON_DATA5 ---- */
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA5____WIDTH	32
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA5____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA5_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS1_VI_MON_DATA6 ---- */
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA6____WIDTH	32
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA6____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA6_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA6_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA6____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS1_VI_MON_DATA7 ---- */
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA7____WIDTH	32
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA7____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA7_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA7_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA7____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS1_VI_MON_DATA8 ---- */
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA8____WIDTH	32
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA8____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA8_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA8_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA8____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS1_VI_MON_DATA9 ---- */
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA9____WIDTH	32
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA9____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA9_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA9_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA9____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS1_VI_MON_DATA10 ---- */
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA10____WIDTH	32
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA10____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA10_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA10_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS1_VI_MON_DATA10____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_IOEXP_PLL_LOCKS ---- */
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS____WIDTH	32
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS____TYPE 	uint32_t

#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_Unused_6___MASK           	UINT32_C(0xffffffc0)
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_Unused_6___SHIFT          	6
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_U_PLL_LOCK___MASK   	UINT32_C(0x20)
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_U_PLL_LOCK___SHIFT  	5
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_TS_PLL_LOCK___MASK  	UINT32_C(0x10)
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_TS_PLL_LOCK___SHIFT 	4
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_NIF_PLL_LOCK___MASK 	UINT32_C(0x8)
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_NIF_PLL_LOCK___SHIFT	3
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_FAB_PLL_LOCK___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_FAB_PLL_LOCK___SHIFT	2
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_C_PLL_LOCK___MASK   	UINT32_C(0x2)
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_C_PLL_LOCK___SHIFT  	1
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_BS_PLL_LOCK___MASK  	UINT32_C(0x1)
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS_J2CPA_BS_PLL_LOCK___SHIFT 	0
#define RUDRA40_GLUE_IOEXP_PLL_LOCKS____REGMASK	UINT32_C(63)

/* ---- RUDRA40_GLUE_ADC_SPI_CONV_VALUE0 ---- */
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE0____WIDTH	32
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE0____TYPE 	uint32_t

#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE0_CONVERSION_VALUE___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE0_CONVERSION_VALUE___SHIFT	0
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_ADC_SPI_CONV_VALUE1 ---- */
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE1____WIDTH	32
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE1____TYPE 	uint32_t

#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE1_CONVERSION_VALUE___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE1_CONVERSION_VALUE___SHIFT	0
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_ADC_SPI_CONV_VALUE2 ---- */
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE2____WIDTH	32
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE2____TYPE 	uint32_t

#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE2_CONVERSION_VALUE___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE2_CONVERSION_VALUE___SHIFT	0
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_ADC_SPI_CONV_VALUE3 ---- */
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE3____WIDTH	32
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE3____TYPE 	uint32_t

#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE3_CONVERSION_VALUE___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE3_CONVERSION_VALUE___SHIFT	0
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_ADC_SPI_CONV_VALUE4 ---- */
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE4____WIDTH	32
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE4____TYPE 	uint32_t

#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE4_CONVERSION_VALUE___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE4_CONVERSION_VALUE___SHIFT	0
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_ADC_SPI_CONV_VALUE5 ---- */
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE5____WIDTH	32
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE5____TYPE 	uint32_t

#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE5_CONVERSION_VALUE___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE5_CONVERSION_VALUE___SHIFT	0
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_ADC_SPI_CONV_VALUE6 ---- */
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE6____WIDTH	32
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE6____TYPE 	uint32_t

#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE6_CONVERSION_VALUE___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE6_CONVERSION_VALUE___SHIFT	0
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE6____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_ADC_SPI_CONV_VALUE7 ---- */
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE7____WIDTH	32
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE7____TYPE 	uint32_t

#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE7_CONVERSION_VALUE___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE7_CONVERSION_VALUE___SHIFT	0
#define RUDRA40_GLUE_ADC_SPI_CONV_VALUE7____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS2_VI_MON_DATA0 ---- */
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA0____WIDTH	32
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA0____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA0_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS2_VI_MON_DATA1 ---- */
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA1____WIDTH	32
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA1____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA1_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS2_VI_MON_DATA2 ---- */
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA2____WIDTH	32
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA2____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA2_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS2_VI_MON_DATA3 ---- */
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA3____WIDTH	32
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA3____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA3_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS2_VI_MON_DATA4 ---- */
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA4____WIDTH	32
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA4____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA4_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS2_VI_MON_DATA5 ---- */
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA5____WIDTH	32
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA5____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA5_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS2_VI_MON_DATA6 ---- */
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA6____WIDTH	32
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA6____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA6_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA6_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA6____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_PMBUS2_VI_MON_DATA7 ---- */
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA7____WIDTH	32
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA7____TYPE 	uint32_t

#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA7_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA7_DATA___SHIFT	0
#define RUDRA40_GLUE_PMBUS2_VI_MON_DATA7____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_HBM_SUPPLIES_RESEQ ---- */
#define RUDRA40_GLUE_HBM_SUPPLIES_RESEQ____WIDTH	32
#define RUDRA40_GLUE_HBM_SUPPLIES_RESEQ____TYPE 	uint32_t

#define RUDRA40_GLUE_HBM_SUPPLIES_RESEQ_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_GLUE_HBM_SUPPLIES_RESEQ_Unused_1___SHIFT	1
#define RUDRA40_GLUE_HBM_SUPPLIES_RESEQ_trigger___MASK  	UINT32_C(0x1)
#define RUDRA40_GLUE_HBM_SUPPLIES_RESEQ_trigger___SHIFT 	0
#define RUDRA40_GLUE_HBM_SUPPLIES_RESEQ____REGMASK	UINT32_C(1)

/* ---- RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG0 ---- */
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG0____WIDTH	32
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG0____TYPE 	uint32_t

#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG0_DATA___SHIFT	0
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG1 ---- */
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG1____WIDTH	32
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG1____TYPE 	uint32_t

#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG1_DATA___SHIFT	0
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG2 ---- */
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG2____WIDTH	32
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG2____TYPE 	uint32_t

#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG2_DATA___SHIFT	0
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG3 ---- */
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG3____WIDTH	32
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG3____TYPE 	uint32_t

#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG3_DATA___SHIFT	0
#define RUDRA40_GLUE_BMC_SGPIO_DATA_DEBUG3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_HBM_POWER_ENABLE_THRESHOLD ---- */
#define RUDRA40_GLUE_HBM_POWER_ENABLE_THRESHOLD____WIDTH	32
#define RUDRA40_GLUE_HBM_POWER_ENABLE_THRESHOLD____TYPE 	uint32_t

#define RUDRA40_GLUE_HBM_POWER_ENABLE_THRESHOLD_Unused_7___MASK    	UINT32_C(0xffffff80)
#define RUDRA40_GLUE_HBM_POWER_ENABLE_THRESHOLD_Unused_7___SHIFT   	7
#define RUDRA40_GLUE_HBM_POWER_ENABLE_THRESHOLD_temperature___MASK 	UINT32_C(0x7f)
#define RUDRA40_GLUE_HBM_POWER_ENABLE_THRESHOLD_temperature___SHIFT	0
#define RUDRA40_GLUE_HBM_POWER_ENABLE_THRESHOLD____REGMASK	UINT32_C(127)

/* ---- RUDRA40_GLUE_QSFPDD_CLKEN_0 ---- */
#define RUDRA40_GLUE_QSFPDD_CLKEN_0____WIDTH	32
#define RUDRA40_GLUE_QSFPDD_CLKEN_0____TYPE 	uint32_t

#define RUDRA40_GLUE_QSFPDD_CLKEN_0_Unused_1___MASK        	UINT32_C(0xfffffffe)
#define RUDRA40_GLUE_QSFPDD_CLKEN_0_Unused_1___SHIFT       	1
#define RUDRA40_GLUE_QSFPDD_CLKEN_0_QSFDD_ALL_CLKEN___MASK 	UINT32_C(0x1)
#define RUDRA40_GLUE_QSFPDD_CLKEN_0_QSFDD_ALL_CLKEN___SHIFT	0
#define RUDRA40_GLUE_QSFPDD_CLKEN_0____REGMASK	UINT32_C(1)

/* ---- RUDRA40_GLUE_ETH_TRAFFIC_STATUS ---- */
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS____WIDTH	32
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS____TYPE 	uint32_t

#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status31___MASK 	UINT32_C(0x80000000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status31___SHIFT	31
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status30___MASK 	UINT32_C(0x40000000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status30___SHIFT	30
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status29___MASK 	UINT32_C(0x20000000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status29___SHIFT	29
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status28___MASK 	UINT32_C(0x10000000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status28___SHIFT	28
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status27___MASK 	UINT32_C(0x8000000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status27___SHIFT	27
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status26___MASK 	UINT32_C(0x4000000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status26___SHIFT	26
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status25___MASK 	UINT32_C(0x2000000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status25___SHIFT	25
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status24___MASK 	UINT32_C(0x1000000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status24___SHIFT	24
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status23___MASK 	UINT32_C(0x800000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status23___SHIFT	23
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status22___MASK 	UINT32_C(0x400000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status22___SHIFT	22
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status21___MASK 	UINT32_C(0x200000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status21___SHIFT	21
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status20___MASK 	UINT32_C(0x100000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status20___SHIFT	20
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status19___MASK 	UINT32_C(0x80000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status19___SHIFT	19
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status18___MASK 	UINT32_C(0x40000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status18___SHIFT	18
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status17___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status17___SHIFT	17
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status16___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status16___SHIFT	16
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status15___MASK 	UINT32_C(0x8000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status15___SHIFT	15
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status14___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status14___SHIFT	14
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status13___MASK 	UINT32_C(0x2000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status13___SHIFT	13
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status12___MASK 	UINT32_C(0x1000)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status12___SHIFT	12
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status11___MASK 	UINT32_C(0x800)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status11___SHIFT	11
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status10___MASK 	UINT32_C(0x400)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status10___SHIFT	10
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status9___MASK  	UINT32_C(0x200)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status9___SHIFT 	9
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status8___MASK  	UINT32_C(0x100)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status8___SHIFT 	8
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status7___MASK  	UINT32_C(0x80)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status7___SHIFT 	7
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status6___MASK  	UINT32_C(0x40)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status6___SHIFT 	6
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status5___MASK  	UINT32_C(0x20)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status5___SHIFT 	5
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status4___MASK  	UINT32_C(0x10)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status4___SHIFT 	4
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status3___MASK  	UINT32_C(0x8)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status3___SHIFT 	3
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status2___MASK  	UINT32_C(0x4)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status2___SHIFT 	2
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status1___MASK  	UINT32_C(0x2)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status1___SHIFT 	1
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status0___MASK  	UINT32_C(0x1)
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS_status0___SHIFT 	0
#define RUDRA40_GLUE_ETH_TRAFFIC_STATUS____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_OPTICS_ADC_EPPS_SPI_SELECT ---- */
#define RUDRA40_GLUE_OPTICS_ADC_EPPS_SPI_SELECT____WIDTH	32
#define RUDRA40_GLUE_OPTICS_ADC_EPPS_SPI_SELECT____TYPE 	uint32_t

#define RUDRA40_GLUE_OPTICS_ADC_EPPS_SPI_SELECT_Unused_5___MASK                       	UINT32_C(0xffffffe0)
#define RUDRA40_GLUE_OPTICS_ADC_EPPS_SPI_SELECT_Unused_5___SHIFT                      	5
#define RUDRA40_GLUE_OPTICS_ADC_EPPS_SPI_SELECT_epps_buffer_spi_selection___MASK      	UINT32_C(0x1c)
#define RUDRA40_GLUE_OPTICS_ADC_EPPS_SPI_SELECT_epps_buffer_spi_selection___SHIFT     	2
#define RUDRA40_GLUE_OPTICS_ADC_EPPS_SPI_SELECT_optics_power_adc_spi_selection___MASK 	UINT32_C(0x3)
#define RUDRA40_GLUE_OPTICS_ADC_EPPS_SPI_SELECT_optics_power_adc_spi_selection___SHIFT	0
#define RUDRA40_GLUE_OPTICS_ADC_EPPS_SPI_SELECT____REGMASK	UINT32_C(31)

/* ---- RUDRA40_GLUE_GLUE_DEBUG_REG_0 ---- */
#define RUDRA40_GLUE_GLUE_DEBUG_REG_0____WIDTH	32
#define RUDRA40_GLUE_GLUE_DEBUG_REG_0____TYPE 	uint32_t

#define RUDRA40_GLUE_GLUE_DEBUG_REG_0____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_GLUE_DEBUG_REG_1 ---- */
#define RUDRA40_GLUE_GLUE_DEBUG_REG_1____WIDTH	32
#define RUDRA40_GLUE_GLUE_DEBUG_REG_1____TYPE 	uint32_t

#define RUDRA40_GLUE_GLUE_DEBUG_REG_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_GLUE_DEBUG_REG_2 ---- */
#define RUDRA40_GLUE_GLUE_DEBUG_REG_2____WIDTH	32
#define RUDRA40_GLUE_GLUE_DEBUG_REG_2____TYPE 	uint32_t

#define RUDRA40_GLUE_GLUE_DEBUG_REG_2____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_GLUE_DEBUG_REG_3 ---- */
#define RUDRA40_GLUE_GLUE_DEBUG_REG_3____WIDTH	32
#define RUDRA40_GLUE_GLUE_DEBUG_REG_3____TYPE 	uint32_t

#define RUDRA40_GLUE_GLUE_DEBUG_REG_3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_GLUE_DEBUG_REG_4 ---- */
#define RUDRA40_GLUE_GLUE_DEBUG_REG_4____WIDTH	32
#define RUDRA40_GLUE_GLUE_DEBUG_REG_4____TYPE 	uint32_t

#define RUDRA40_GLUE_GLUE_DEBUG_REG_4____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_GLUE_DEBUG_REG_5 ---- */
#define RUDRA40_GLUE_GLUE_DEBUG_REG_5____WIDTH	32
#define RUDRA40_GLUE_GLUE_DEBUG_REG_5____TYPE 	uint32_t

#define RUDRA40_GLUE_GLUE_DEBUG_REG_5____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_GLUE_DEBUG_REG_6 ---- */
#define RUDRA40_GLUE_GLUE_DEBUG_REG_6____WIDTH	32
#define RUDRA40_GLUE_GLUE_DEBUG_REG_6____TYPE 	uint32_t

#define RUDRA40_GLUE_GLUE_DEBUG_REG_6____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_GLUE_DEBUG_REG_7 ---- */
#define RUDRA40_GLUE_GLUE_DEBUG_REG_7____WIDTH	32
#define RUDRA40_GLUE_GLUE_DEBUG_REG_7____TYPE 	uint32_t

#define RUDRA40_GLUE_GLUE_DEBUG_REG_7____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_GLUE_DEBUG_REG_8 ---- */
#define RUDRA40_GLUE_GLUE_DEBUG_REG_8____WIDTH	32
#define RUDRA40_GLUE_GLUE_DEBUG_REG_8____TYPE 	uint32_t

#define RUDRA40_GLUE_GLUE_DEBUG_REG_8____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_GLUE_DEBUG_REG_9 ---- */
#define RUDRA40_GLUE_GLUE_DEBUG_REG_9____WIDTH	32
#define RUDRA40_GLUE_GLUE_DEBUG_REG_9____TYPE 	uint32_t

#define RUDRA40_GLUE_GLUE_DEBUG_REG_9____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG0 ---- */
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG0____WIDTH	32
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG0____TYPE 	uint32_t

#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG0_DATA___SHIFT	0
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG1 ---- */
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG1____WIDTH	32
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG1____TYPE 	uint32_t

#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG1_DATA___SHIFT	0
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG2 ---- */
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG2____WIDTH	32
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG2____TYPE 	uint32_t

#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG2_DATA___SHIFT	0
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG3 ---- */
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG3____WIDTH	32
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG3____TYPE 	uint32_t

#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG3_DATA___SHIFT	0
#define RUDRA40_GLUE_CPLD_SGPIO_DATA_DEBUG3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0 ---- */
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0____WIDTH	32
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0____TYPE 	uint32_t

#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0_DATA___SHIFT	0
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1 ---- */
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1____WIDTH	32
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1____TYPE 	uint32_t

#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1_DATA___SHIFT	0
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2 ---- */
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2____WIDTH	32
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2____TYPE 	uint32_t

#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2_DATA___SHIFT	0
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3 ---- */
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3____WIDTH	32
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3____TYPE 	uint32_t

#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3_DATA___SHIFT	0
#define RUDRA40_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_LED_SFP_RATE_1 ---- */
#define RUDRA40_GLUE_LED_SFP_RATE_1____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RATE_1____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RATE_1_Unused_30___MASK            	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_Unused_30___SHIFT           	30
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit5___MASK 	UINT32_C(0x20000000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit5___SHIFT	29
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit4___MASK 	UINT32_C(0x10000000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit4___SHIFT	28
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit3___MASK 	UINT32_C(0x8000000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit3___SHIFT	27
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit2___MASK 	UINT32_C(0x4000000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit2___SHIFT	26
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit1___MASK 	UINT32_C(0x2000000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit1___SHIFT	25
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit0___MASK 	UINT32_C(0x1000000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_5_bit0___SHIFT	24
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit5___MASK 	UINT32_C(0x800000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit5___SHIFT	23
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit4___MASK 	UINT32_C(0x400000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit4___SHIFT	22
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit3___MASK 	UINT32_C(0x200000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit3___SHIFT	21
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit2___MASK 	UINT32_C(0x100000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit2___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit1___MASK 	UINT32_C(0x80000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit1___SHIFT	19
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit0___MASK 	UINT32_C(0x40000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_4_bit0___SHIFT	18
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit5___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit5___SHIFT	17
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit4___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit4___SHIFT	16
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit3___MASK 	UINT32_C(0x8000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit3___SHIFT	15
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit2___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit2___SHIFT	14
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit1___MASK 	UINT32_C(0x2000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit1___SHIFT	13
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit0___MASK 	UINT32_C(0x1000)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_3_bit0___SHIFT	12
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit5___MASK 	UINT32_C(0x800)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit5___SHIFT	11
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit4___MASK 	UINT32_C(0x400)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit4___SHIFT	10
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit3___MASK 	UINT32_C(0x200)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit3___SHIFT	9
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit2___MASK 	UINT32_C(0x100)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit2___SHIFT	8
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit1___MASK 	UINT32_C(0x80)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit1___SHIFT	7
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit0___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_2_bit0___SHIFT	6
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit5___MASK 	UINT32_C(0x20)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit5___SHIFT	5
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit4___MASK 	UINT32_C(0x10)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit4___SHIFT	4
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit3___MASK 	UINT32_C(0x8)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit3___SHIFT	3
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit2___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit2___SHIFT	2
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit1___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit1___SHIFT	1
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit0___MASK 	UINT32_C(0x1)
#define RUDRA40_GLUE_LED_SFP_RATE_1_sfp_rate_port_1_bit0___SHIFT	0
#define RUDRA40_GLUE_LED_SFP_RATE_1____REGMASK	UINT32_C(1073741823)

/* ---- RUDRA40_GLUE_LED_SFP_RATE_2 ---- */
#define RUDRA40_GLUE_LED_SFP_RATE_2____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RATE_2____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RATE_2_Unused_30___MASK             	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_Unused_30___SHIFT            	30
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit5___MASK 	UINT32_C(0x20000000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit5___SHIFT	29
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit4___MASK 	UINT32_C(0x10000000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit4___SHIFT	28
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit3___MASK 	UINT32_C(0x8000000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit3___SHIFT	27
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit2___MASK 	UINT32_C(0x4000000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit2___SHIFT	26
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit1___MASK 	UINT32_C(0x2000000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit1___SHIFT	25
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit0___MASK 	UINT32_C(0x1000000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_10_bit0___SHIFT	24
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit5___MASK  	UINT32_C(0x800000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit5___SHIFT 	23
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit4___MASK  	UINT32_C(0x400000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit4___SHIFT 	22
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit3___MASK  	UINT32_C(0x200000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit3___SHIFT 	21
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit2___MASK  	UINT32_C(0x100000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit2___SHIFT 	20
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit1___MASK  	UINT32_C(0x80000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit1___SHIFT 	19
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit0___MASK  	UINT32_C(0x40000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_9_bit0___SHIFT 	18
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit5___MASK  	UINT32_C(0x20000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit5___SHIFT 	17
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit4___MASK  	UINT32_C(0x10000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit4___SHIFT 	16
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit3___MASK  	UINT32_C(0x8000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit3___SHIFT 	15
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit2___MASK  	UINT32_C(0x4000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit2___SHIFT 	14
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit1___MASK  	UINT32_C(0x2000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit1___SHIFT 	13
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit0___MASK  	UINT32_C(0x1000)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_8_bit0___SHIFT 	12
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit5___MASK  	UINT32_C(0x800)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit5___SHIFT 	11
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit4___MASK  	UINT32_C(0x400)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit4___SHIFT 	10
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit3___MASK  	UINT32_C(0x200)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit3___SHIFT 	9
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit2___MASK  	UINT32_C(0x100)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit2___SHIFT 	8
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit1___MASK  	UINT32_C(0x80)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit1___SHIFT 	7
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit0___MASK  	UINT32_C(0x40)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_7_bit0___SHIFT 	6
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit5___MASK  	UINT32_C(0x20)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit5___SHIFT 	5
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit4___MASK  	UINT32_C(0x10)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit4___SHIFT 	4
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit3___MASK  	UINT32_C(0x8)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit3___SHIFT 	3
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit2___MASK  	UINT32_C(0x4)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit2___SHIFT 	2
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit1___MASK  	UINT32_C(0x2)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit1___SHIFT 	1
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit0___MASK  	UINT32_C(0x1)
#define RUDRA40_GLUE_LED_SFP_RATE_2_sfp_rate_port_6_bit0___SHIFT 	0
#define RUDRA40_GLUE_LED_SFP_RATE_2____REGMASK	UINT32_C(1073741823)

/* ---- RUDRA40_GLUE_LED_SFP_RATE_3 ---- */
#define RUDRA40_GLUE_LED_SFP_RATE_3____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RATE_3____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RATE_3_Unused_30___MASK             	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_Unused_30___SHIFT            	30
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit5___MASK 	UINT32_C(0x20000000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit5___SHIFT	29
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit4___MASK 	UINT32_C(0x10000000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit4___SHIFT	28
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit3___MASK 	UINT32_C(0x8000000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit3___SHIFT	27
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit2___MASK 	UINT32_C(0x4000000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit2___SHIFT	26
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit1___MASK 	UINT32_C(0x2000000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit1___SHIFT	25
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit0___MASK 	UINT32_C(0x1000000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_15_bit0___SHIFT	24
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit5___MASK 	UINT32_C(0x800000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit5___SHIFT	23
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit4___MASK 	UINT32_C(0x400000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit4___SHIFT	22
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit3___MASK 	UINT32_C(0x200000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit3___SHIFT	21
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit2___MASK 	UINT32_C(0x100000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit2___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit1___MASK 	UINT32_C(0x80000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit1___SHIFT	19
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit0___MASK 	UINT32_C(0x40000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_14_bit0___SHIFT	18
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit5___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit5___SHIFT	17
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit4___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit4___SHIFT	16
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit3___MASK 	UINT32_C(0x8000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit3___SHIFT	15
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit2___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit2___SHIFT	14
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit1___MASK 	UINT32_C(0x2000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit1___SHIFT	13
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit0___MASK 	UINT32_C(0x1000)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_13_bit0___SHIFT	12
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit5___MASK 	UINT32_C(0x800)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit5___SHIFT	11
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit4___MASK 	UINT32_C(0x400)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit4___SHIFT	10
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit3___MASK 	UINT32_C(0x200)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit3___SHIFT	9
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit2___MASK 	UINT32_C(0x100)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit2___SHIFT	8
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit1___MASK 	UINT32_C(0x80)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit1___SHIFT	7
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit0___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_12_bit0___SHIFT	6
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit5___MASK 	UINT32_C(0x20)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit5___SHIFT	5
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit4___MASK 	UINT32_C(0x10)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit4___SHIFT	4
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit3___MASK 	UINT32_C(0x8)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit3___SHIFT	3
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit2___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit2___SHIFT	2
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit1___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit1___SHIFT	1
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit0___MASK 	UINT32_C(0x1)
#define RUDRA40_GLUE_LED_SFP_RATE_3_sfp_rate_port_11_bit0___SHIFT	0
#define RUDRA40_GLUE_LED_SFP_RATE_3____REGMASK	UINT32_C(1073741823)

/* ---- RUDRA40_GLUE_LED_SFP_RATE_4 ---- */
#define RUDRA40_GLUE_LED_SFP_RATE_4____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RATE_4____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RATE_4_Unused_30___MASK             	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_Unused_30___SHIFT            	30
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit5___MASK 	UINT32_C(0x20000000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit5___SHIFT	29
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit4___MASK 	UINT32_C(0x10000000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit4___SHIFT	28
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit3___MASK 	UINT32_C(0x8000000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit3___SHIFT	27
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit2___MASK 	UINT32_C(0x4000000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit2___SHIFT	26
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit1___MASK 	UINT32_C(0x2000000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit1___SHIFT	25
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit0___MASK 	UINT32_C(0x1000000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_20_bit0___SHIFT	24
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit5___MASK 	UINT32_C(0x800000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit5___SHIFT	23
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit4___MASK 	UINT32_C(0x400000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit4___SHIFT	22
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit3___MASK 	UINT32_C(0x200000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit3___SHIFT	21
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit2___MASK 	UINT32_C(0x100000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit2___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit1___MASK 	UINT32_C(0x80000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit1___SHIFT	19
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit0___MASK 	UINT32_C(0x40000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_19_bit0___SHIFT	18
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit5___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit5___SHIFT	17
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit4___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit4___SHIFT	16
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit3___MASK 	UINT32_C(0x8000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit3___SHIFT	15
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit2___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit2___SHIFT	14
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit1___MASK 	UINT32_C(0x2000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit1___SHIFT	13
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit0___MASK 	UINT32_C(0x1000)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_18_bit0___SHIFT	12
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit5___MASK 	UINT32_C(0x800)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit5___SHIFT	11
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit4___MASK 	UINT32_C(0x400)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit4___SHIFT	10
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit3___MASK 	UINT32_C(0x200)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit3___SHIFT	9
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit2___MASK 	UINT32_C(0x100)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit2___SHIFT	8
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit1___MASK 	UINT32_C(0x80)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit1___SHIFT	7
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit0___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_17_bit0___SHIFT	6
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit5___MASK 	UINT32_C(0x20)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit5___SHIFT	5
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit4___MASK 	UINT32_C(0x10)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit4___SHIFT	4
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit3___MASK 	UINT32_C(0x8)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit3___SHIFT	3
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit2___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit2___SHIFT	2
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit1___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit1___SHIFT	1
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit0___MASK 	UINT32_C(0x1)
#define RUDRA40_GLUE_LED_SFP_RATE_4_sfp_rate_port_16_bit0___SHIFT	0
#define RUDRA40_GLUE_LED_SFP_RATE_4____REGMASK	UINT32_C(1073741823)

/* ---- RUDRA40_GLUE_LED_SFP_RATE_5 ---- */
#define RUDRA40_GLUE_LED_SFP_RATE_5____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RATE_5____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RATE_5_Unused_30___MASK             	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_Unused_30___SHIFT            	30
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit5___MASK 	UINT32_C(0x20000000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit5___SHIFT	29
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit4___MASK 	UINT32_C(0x10000000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit4___SHIFT	28
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit3___MASK 	UINT32_C(0x8000000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit3___SHIFT	27
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit2___MASK 	UINT32_C(0x4000000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit2___SHIFT	26
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit1___MASK 	UINT32_C(0x2000000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit1___SHIFT	25
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit0___MASK 	UINT32_C(0x1000000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_25_bit0___SHIFT	24
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit5___MASK 	UINT32_C(0x800000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit5___SHIFT	23
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit4___MASK 	UINT32_C(0x400000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit4___SHIFT	22
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit3___MASK 	UINT32_C(0x200000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit3___SHIFT	21
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit2___MASK 	UINT32_C(0x100000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit2___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit1___MASK 	UINT32_C(0x80000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit1___SHIFT	19
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit0___MASK 	UINT32_C(0x40000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_24_bit0___SHIFT	18
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit5___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit5___SHIFT	17
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit4___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit4___SHIFT	16
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit3___MASK 	UINT32_C(0x8000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit3___SHIFT	15
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit2___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit2___SHIFT	14
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit1___MASK 	UINT32_C(0x2000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit1___SHIFT	13
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit0___MASK 	UINT32_C(0x1000)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_23_bit0___SHIFT	12
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit5___MASK 	UINT32_C(0x800)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit5___SHIFT	11
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit4___MASK 	UINT32_C(0x400)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit4___SHIFT	10
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit3___MASK 	UINT32_C(0x200)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit3___SHIFT	9
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit2___MASK 	UINT32_C(0x100)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit2___SHIFT	8
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit1___MASK 	UINT32_C(0x80)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit1___SHIFT	7
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit0___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_22_bit0___SHIFT	6
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit5___MASK 	UINT32_C(0x20)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit5___SHIFT	5
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit4___MASK 	UINT32_C(0x10)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit4___SHIFT	4
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit3___MASK 	UINT32_C(0x8)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit3___SHIFT	3
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit2___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit2___SHIFT	2
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit1___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit1___SHIFT	1
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit0___MASK 	UINT32_C(0x1)
#define RUDRA40_GLUE_LED_SFP_RATE_5_sfp_rate_port_21_bit0___SHIFT	0
#define RUDRA40_GLUE_LED_SFP_RATE_5____REGMASK	UINT32_C(1073741823)

/* ---- RUDRA40_GLUE_LED_SFP_RATE_6 ---- */
#define RUDRA40_GLUE_LED_SFP_RATE_6____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RATE_6____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RATE_6_Unused_30___MASK             	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_Unused_30___SHIFT            	30
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit5___MASK 	UINT32_C(0x20000000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit5___SHIFT	29
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit4___MASK 	UINT32_C(0x10000000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit4___SHIFT	28
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit3___MASK 	UINT32_C(0x8000000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit3___SHIFT	27
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit2___MASK 	UINT32_C(0x4000000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit2___SHIFT	26
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit1___MASK 	UINT32_C(0x2000000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit1___SHIFT	25
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit0___MASK 	UINT32_C(0x1000000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_30_bit0___SHIFT	24
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit5___MASK 	UINT32_C(0x800000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit5___SHIFT	23
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit4___MASK 	UINT32_C(0x400000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit4___SHIFT	22
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit3___MASK 	UINT32_C(0x200000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit3___SHIFT	21
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit2___MASK 	UINT32_C(0x100000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit2___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit1___MASK 	UINT32_C(0x80000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit1___SHIFT	19
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit0___MASK 	UINT32_C(0x40000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_29_bit0___SHIFT	18
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit5___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit5___SHIFT	17
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit4___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit4___SHIFT	16
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit3___MASK 	UINT32_C(0x8000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit3___SHIFT	15
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit2___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit2___SHIFT	14
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit1___MASK 	UINT32_C(0x2000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit1___SHIFT	13
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit0___MASK 	UINT32_C(0x1000)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_28_bit0___SHIFT	12
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit5___MASK 	UINT32_C(0x800)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit5___SHIFT	11
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit4___MASK 	UINT32_C(0x400)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit4___SHIFT	10
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit3___MASK 	UINT32_C(0x200)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit3___SHIFT	9
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit2___MASK 	UINT32_C(0x100)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit2___SHIFT	8
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit1___MASK 	UINT32_C(0x80)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit1___SHIFT	7
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit0___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_27_bit0___SHIFT	6
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit5___MASK 	UINT32_C(0x20)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit5___SHIFT	5
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit4___MASK 	UINT32_C(0x10)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit4___SHIFT	4
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit3___MASK 	UINT32_C(0x8)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit3___SHIFT	3
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit2___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit2___SHIFT	2
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit1___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit1___SHIFT	1
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit0___MASK 	UINT32_C(0x1)
#define RUDRA40_GLUE_LED_SFP_RATE_6_sfp_rate_port_26_bit0___SHIFT	0
#define RUDRA40_GLUE_LED_SFP_RATE_6____REGMASK	UINT32_C(1073741823)

/* ---- RUDRA40_GLUE_LED_SFP_RATE_7 ---- */
#define RUDRA40_GLUE_LED_SFP_RATE_7____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RATE_7____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RATE_7_Unused_30___MASK             	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_Unused_30___SHIFT            	30
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit5___MASK 	UINT32_C(0x20000000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit5___SHIFT	29
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit4___MASK 	UINT32_C(0x10000000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit4___SHIFT	28
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit3___MASK 	UINT32_C(0x8000000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit3___SHIFT	27
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit2___MASK 	UINT32_C(0x4000000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit2___SHIFT	26
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit1___MASK 	UINT32_C(0x2000000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit1___SHIFT	25
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit0___MASK 	UINT32_C(0x1000000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_35_bit0___SHIFT	24
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit5___MASK 	UINT32_C(0x800000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit5___SHIFT	23
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit4___MASK 	UINT32_C(0x400000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit4___SHIFT	22
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit3___MASK 	UINT32_C(0x200000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit3___SHIFT	21
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit2___MASK 	UINT32_C(0x100000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit2___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit1___MASK 	UINT32_C(0x80000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit1___SHIFT	19
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit0___MASK 	UINT32_C(0x40000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_34_bit0___SHIFT	18
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit5___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit5___SHIFT	17
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit4___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit4___SHIFT	16
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit3___MASK 	UINT32_C(0x8000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit3___SHIFT	15
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit2___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit2___SHIFT	14
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit1___MASK 	UINT32_C(0x2000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit1___SHIFT	13
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit0___MASK 	UINT32_C(0x1000)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_33_bit0___SHIFT	12
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit5___MASK 	UINT32_C(0x800)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit5___SHIFT	11
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit4___MASK 	UINT32_C(0x400)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit4___SHIFT	10
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit3___MASK 	UINT32_C(0x200)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit3___SHIFT	9
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit2___MASK 	UINT32_C(0x100)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit2___SHIFT	8
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit1___MASK 	UINT32_C(0x80)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit1___SHIFT	7
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit0___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_32_bit0___SHIFT	6
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit5___MASK 	UINT32_C(0x20)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit5___SHIFT	5
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit4___MASK 	UINT32_C(0x10)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit4___SHIFT	4
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit3___MASK 	UINT32_C(0x8)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit3___SHIFT	3
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit2___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit2___SHIFT	2
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit1___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit1___SHIFT	1
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit0___MASK 	UINT32_C(0x1)
#define RUDRA40_GLUE_LED_SFP_RATE_7_sfp_rate_port_31_bit0___SHIFT	0
#define RUDRA40_GLUE_LED_SFP_RATE_7____REGMASK	UINT32_C(1073741823)

/* ---- RUDRA40_GLUE_LED_SFP_RATE_8 ---- */
#define RUDRA40_GLUE_LED_SFP_RATE_8____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RATE_8____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RATE_8_Unused_30___MASK             	UINT32_C(0xc0000000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_Unused_30___SHIFT            	30
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit5___MASK 	UINT32_C(0x20000000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit5___SHIFT	29
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit4___MASK 	UINT32_C(0x10000000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit4___SHIFT	28
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit3___MASK 	UINT32_C(0x8000000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit3___SHIFT	27
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit2___MASK 	UINT32_C(0x4000000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit2___SHIFT	26
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit1___MASK 	UINT32_C(0x2000000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit1___SHIFT	25
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit0___MASK 	UINT32_C(0x1000000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_40_bit0___SHIFT	24
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit5___MASK 	UINT32_C(0x800000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit5___SHIFT	23
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit4___MASK 	UINT32_C(0x400000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit4___SHIFT	22
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit3___MASK 	UINT32_C(0x200000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit3___SHIFT	21
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit2___MASK 	UINT32_C(0x100000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit2___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit1___MASK 	UINT32_C(0x80000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit1___SHIFT	19
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit0___MASK 	UINT32_C(0x40000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_39_bit0___SHIFT	18
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit5___MASK 	UINT32_C(0x20000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit5___SHIFT	17
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit4___MASK 	UINT32_C(0x10000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit4___SHIFT	16
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit3___MASK 	UINT32_C(0x8000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit3___SHIFT	15
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit2___MASK 	UINT32_C(0x4000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit2___SHIFT	14
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit1___MASK 	UINT32_C(0x2000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit1___SHIFT	13
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit0___MASK 	UINT32_C(0x1000)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_38_bit0___SHIFT	12
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit5___MASK 	UINT32_C(0x800)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit5___SHIFT	11
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit4___MASK 	UINT32_C(0x400)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit4___SHIFT	10
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit3___MASK 	UINT32_C(0x200)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit3___SHIFT	9
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit2___MASK 	UINT32_C(0x100)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit2___SHIFT	8
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit1___MASK 	UINT32_C(0x80)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit1___SHIFT	7
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit0___MASK 	UINT32_C(0x40)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_37_bit0___SHIFT	6
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit5___MASK 	UINT32_C(0x20)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit5___SHIFT	5
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit4___MASK 	UINT32_C(0x10)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit4___SHIFT	4
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit3___MASK 	UINT32_C(0x8)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit3___SHIFT	3
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit2___MASK 	UINT32_C(0x4)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit2___SHIFT	2
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit1___MASK 	UINT32_C(0x2)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit1___SHIFT	1
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit0___MASK 	UINT32_C(0x1)
#define RUDRA40_GLUE_LED_SFP_RATE_8_sfp_rate_port_36_bit0___SHIFT	0
#define RUDRA40_GLUE_LED_SFP_RATE_8____REGMASK	UINT32_C(1073741823)

/* ---- RUDRA40_GLUE_LED_SFP_RED_1 ---- */
#define RUDRA40_GLUE_LED_SFP_RED_1____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RED_1____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RED_1_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_RED_1_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_RED_1_sfp_red___MASK   	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_RED_1_sfp_red___SHIFT  	0
#define RUDRA40_GLUE_LED_SFP_RED_1____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_LED_SFP_RED_2 ---- */
#define RUDRA40_GLUE_LED_SFP_RED_2____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RED_2____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RED_2_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_RED_2_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_RED_2_sfp_red___MASK   	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_RED_2_sfp_red___SHIFT  	0
#define RUDRA40_GLUE_LED_SFP_RED_2____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_LED_SFP_RED_3 ---- */
#define RUDRA40_GLUE_LED_SFP_RED_3____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RED_3____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RED_3_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_RED_3_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_RED_3_sfp_red___MASK   	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_RED_3_sfp_red___SHIFT  	0
#define RUDRA40_GLUE_LED_SFP_RED_3____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_LED_SFP_RED_4 ---- */
#define RUDRA40_GLUE_LED_SFP_RED_4____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_RED_4____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_RED_4_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_RED_4_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_RED_4_sfp_red___MASK   	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_RED_4_sfp_red___SHIFT  	0
#define RUDRA40_GLUE_LED_SFP_RED_4____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_LED_SFP_GREEN_1 ---- */
#define RUDRA40_GLUE_LED_SFP_GREEN_1____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_GREEN_1____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_GREEN_1_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_GREEN_1_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_GREEN_1_sfp_green___MASK 	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_GREEN_1_sfp_green___SHIFT	0
#define RUDRA40_GLUE_LED_SFP_GREEN_1____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_LED_SFP_GREEN_2 ---- */
#define RUDRA40_GLUE_LED_SFP_GREEN_2____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_GREEN_2____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_GREEN_2_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_GREEN_2_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_GREEN_2_sfp_green___MASK 	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_GREEN_2_sfp_green___SHIFT	0
#define RUDRA40_GLUE_LED_SFP_GREEN_2____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_LED_SFP_GREEN_3 ---- */
#define RUDRA40_GLUE_LED_SFP_GREEN_3____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_GREEN_3____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_GREEN_3_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_GREEN_3_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_GREEN_3_sfp_green___MASK 	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_GREEN_3_sfp_green___SHIFT	0
#define RUDRA40_GLUE_LED_SFP_GREEN_3____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_LED_SFP_GREEN_4 ---- */
#define RUDRA40_GLUE_LED_SFP_GREEN_4____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_GREEN_4____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_GREEN_4_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_GREEN_4_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_GREEN_4_sfp_green___MASK 	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_GREEN_4_sfp_green___SHIFT	0
#define RUDRA40_GLUE_LED_SFP_GREEN_4____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_LED_SFP_BLUE_1 ---- */
#define RUDRA40_GLUE_LED_SFP_BLUE_1____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_BLUE_1____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_BLUE_1_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_BLUE_1_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_BLUE_1_sfp_blue___MASK  	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_BLUE_1_sfp_blue___SHIFT 	0
#define RUDRA40_GLUE_LED_SFP_BLUE_1____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_LED_SFP_BLUE_2 ---- */
#define RUDRA40_GLUE_LED_SFP_BLUE_2____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_BLUE_2____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_BLUE_2_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_BLUE_2_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_BLUE_2_sfp_blue___MASK  	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_BLUE_2_sfp_blue___SHIFT 	0
#define RUDRA40_GLUE_LED_SFP_BLUE_2____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_LED_SFP_BLUE_3 ---- */
#define RUDRA40_GLUE_LED_SFP_BLUE_3____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_BLUE_3____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_BLUE_3_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_BLUE_3_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_BLUE_3_sfp_blue___MASK  	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_BLUE_3_sfp_blue___SHIFT 	0
#define RUDRA40_GLUE_LED_SFP_BLUE_3____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_LED_SFP_BLUE_4 ---- */
#define RUDRA40_GLUE_LED_SFP_BLUE_4____WIDTH	32
#define RUDRA40_GLUE_LED_SFP_BLUE_4____TYPE 	uint32_t

#define RUDRA40_GLUE_LED_SFP_BLUE_4_Unused_20___MASK 	UINT32_C(0xfff00000)
#define RUDRA40_GLUE_LED_SFP_BLUE_4_Unused_20___SHIFT	20
#define RUDRA40_GLUE_LED_SFP_BLUE_4_sfp_blue___MASK  	UINT32_C(0xfffff)
#define RUDRA40_GLUE_LED_SFP_BLUE_4_sfp_blue___SHIFT 	0
#define RUDRA40_GLUE_LED_SFP_BLUE_4____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_GLUE_GEARBOX_MDC_SPEED ---- */
#define RUDRA40_GLUE_GEARBOX_MDC_SPEED____WIDTH	32
#define RUDRA40_GLUE_GEARBOX_MDC_SPEED____TYPE 	uint32_t

#define RUDRA40_GLUE_GEARBOX_MDC_SPEED_Unused_2___MASK          	UINT32_C(0xfffffffc)
#define RUDRA40_GLUE_GEARBOX_MDC_SPEED_Unused_2___SHIFT         	2
#define RUDRA40_GLUE_GEARBOX_MDC_SPEED_GEARBOX_MDC_SPEED___MASK 	UINT32_C(0x3)
#define RUDRA40_GLUE_GEARBOX_MDC_SPEED_GEARBOX_MDC_SPEED___SHIFT	0
#define RUDRA40_GLUE_GEARBOX_MDC_SPEED____REGMASK	UINT32_C(3)

/* ---- RUDRA40_GLUE_GEARBOX_MDIO_CMD ---- */
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD____WIDTH	32
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD____TYPE 	uint32_t

#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_EXE___MASK   	UINT32_C(0x80000000)
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_EXE___SHIFT  	31
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_SFS___MASK   	UINT32_C(0x40000000)
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_SFS___SHIFT  	30
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_OP___MASK    	UINT32_C(0x30000000)
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_OP___SHIFT   	28
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_PHYAD___MASK 	UINT32_C(0xf800000)
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_PHYAD___SHIFT	23
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_REGAD___MASK 	UINT32_C(0x7c0000)
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_REGAD___SHIFT	18
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_TA___MASK    	UINT32_C(0x30000)
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_TA___SHIFT   	16
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_AD___MASK    	UINT32_C(0xffff)
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD_MDIO_AD___SHIFT   	0
#define RUDRA40_GLUE_GEARBOX_MDIO_CMD____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_GLUE_GEARBOX_MDIO_RDAT ---- */
#define RUDRA40_GLUE_GEARBOX_MDIO_RDAT____WIDTH	32
#define RUDRA40_GLUE_GEARBOX_MDIO_RDAT____TYPE 	uint32_t

#define RUDRA40_GLUE_GEARBOX_MDIO_RDAT____REGMASK	UINT32_C(0)

/* ---- RUDRA40_GLUE_THEEND ---- */
#define RUDRA40_GLUE_THEEND____WIDTH	32
#define RUDRA40_GLUE_THEEND____TYPE 	uint32_t

#define RUDRA40_GLUE_THEEND____REGMASK	UINT32_C(0)

/* ---- RUDRA40_TIME_DIST_TIMING_STATUS ---- */
#define RUDRA40_TIME_DIST_TIMING_STATUS____WIDTH	32
#define RUDRA40_TIME_DIST_TIMING_STATUS____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_TIMING_STATUS_Unused_12___MASK               	UINT32_C(0xfffff000)
#define RUDRA40_TIME_DIST_TIMING_STATUS_Unused_12___SHIFT              	12
#define RUDRA40_TIME_DIST_TIMING_STATUS_GPS_SYNC2_RX_LOS_DETECT___MASK 	UINT32_C(0x800)
#define RUDRA40_TIME_DIST_TIMING_STATUS_GPS_SYNC2_RX_LOS_DETECT___SHIFT	11
#define RUDRA40_TIME_DIST_TIMING_STATUS_RX_LOS_1PPS_DETECT___MASK      	UINT32_C(0x400)
#define RUDRA40_TIME_DIST_TIMING_STATUS_RX_LOS_1PPS_DETECT___SHIFT     	10
#define RUDRA40_TIME_DIST_TIMING_STATUS_TOD_RX_LOS_DETECT___MASK       	UINT32_C(0x200)
#define RUDRA40_TIME_DIST_TIMING_STATUS_TOD_RX_LOS_DETECT___SHIFT      	9
#define RUDRA40_TIME_DIST_TIMING_STATUS_GPS_SYNC1_RX_LOS_DETECT___MASK 	UINT32_C(0x100)
#define RUDRA40_TIME_DIST_TIMING_STATUS_GPS_SYNC1_RX_LOS_DETECT___SHIFT	8
#define RUDRA40_TIME_DIST_TIMING_STATUS_Unused_5___MASK                	UINT32_C(0xe0)
#define RUDRA40_TIME_DIST_TIMING_STATUS_Unused_5___SHIFT               	5
#define RUDRA40_TIME_DIST_TIMING_STATUS_J2CA_GPIO_2___MASK             	UINT32_C(0x10)
#define RUDRA40_TIME_DIST_TIMING_STATUS_J2CA_GPIO_2___SHIFT            	4
#define RUDRA40_TIME_DIST_TIMING_STATUS_J2CA_GPIO_1___MASK             	UINT32_C(0x8)
#define RUDRA40_TIME_DIST_TIMING_STATUS_J2CA_GPIO_1___SHIFT            	3
#define RUDRA40_TIME_DIST_TIMING_STATUS_J2CA_GPIO_0___MASK             	UINT32_C(0x4)
#define RUDRA40_TIME_DIST_TIMING_STATUS_J2CA_GPIO_0___SHIFT            	2
#define RUDRA40_TIME_DIST_TIMING_STATUS_ZPLL_HOLD___MASK               	UINT32_C(0x2)
#define RUDRA40_TIME_DIST_TIMING_STATUS_ZPLL_HOLD___SHIFT              	1
#define RUDRA40_TIME_DIST_TIMING_STATUS_ZPLL_LOCK___MASK               	UINT32_C(0x1)
#define RUDRA40_TIME_DIST_TIMING_STATUS_ZPLL_LOCK___SHIFT              	0
#define RUDRA40_TIME_DIST_TIMING_STATUS____REGMASK	UINT32_C(3871)

/* ---- RUDRA40_TIME_DIST_TIMING_CTL ---- */
#define RUDRA40_TIME_DIST_TIMING_CTL____WIDTH	32
#define RUDRA40_TIME_DIST_TIMING_CTL____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_TIMING_CTL_Unused_13___MASK         	UINT32_C(0xffffe000)
#define RUDRA40_TIME_DIST_TIMING_CTL_Unused_13___SHIFT        	13
#define RUDRA40_TIME_DIST_TIMING_CTL_BITS_TCK_OE___MASK       	UINT32_C(0x1000)
#define RUDRA40_TIME_DIST_TIMING_CTL_BITS_TCK_OE___SHIFT      	12
#define RUDRA40_TIME_DIST_TIMING_CTL_BITS_THZE_OE___MASK      	UINT32_C(0x800)
#define RUDRA40_TIME_DIST_TIMING_CTL_BITS_THZE_OE___SHIFT     	11
#define RUDRA40_TIME_DIST_TIMING_CTL_BITS_T8K_400_OE___MASK   	UINT32_C(0x400)
#define RUDRA40_TIME_DIST_TIMING_CTL_BITS_T8K_400_OE___SHIFT  	10
#define RUDRA40_TIME_DIST_TIMING_CTL_Unused_8___MASK          	UINT32_C(0x300)
#define RUDRA40_TIME_DIST_TIMING_CTL_Unused_8___SHIFT         	8
#define RUDRA40_TIME_DIST_TIMING_CTL_J2CA_GPIO1_ENABLE___MASK 	UINT32_C(0x80)
#define RUDRA40_TIME_DIST_TIMING_CTL_J2CA_GPIO1_ENABLE___SHIFT	7
#define RUDRA40_TIME_DIST_TIMING_CTL_J2CA_GPIO1_OUTPUT___MASK 	UINT32_C(0x40)
#define RUDRA40_TIME_DIST_TIMING_CTL_J2CA_GPIO1_OUTPUT___SHIFT	6
#define RUDRA40_TIME_DIST_TIMING_CTL_PPS_LOC_UPDATE___MASK    	UINT32_C(0x20)
#define RUDRA40_TIME_DIST_TIMING_CTL_PPS_LOC_UPDATE___SHIFT   	5
#define RUDRA40_TIME_DIST_TIMING_CTL_TX_EN_1PPS___MASK        	UINT32_C(0x10)
#define RUDRA40_TIME_DIST_TIMING_CTL_TX_EN_1PPS___SHIFT       	4
#define RUDRA40_TIME_DIST_TIMING_CTL_GPS_SYNC2_TX_EN_N___MASK 	UINT32_C(0x8)
#define RUDRA40_TIME_DIST_TIMING_CTL_GPS_SYNC2_TX_EN_N___SHIFT	3
#define RUDRA40_TIME_DIST_TIMING_CTL_GPS_SYNC1_TX_EN_N___MASK 	UINT32_C(0x4)
#define RUDRA40_TIME_DIST_TIMING_CTL_GPS_SYNC1_TX_EN_N___SHIFT	2
#define RUDRA40_TIME_DIST_TIMING_CTL_Unused_1___MASK          	UINT32_C(0x2)
#define RUDRA40_TIME_DIST_TIMING_CTL_Unused_1___SHIFT         	1
#define RUDRA40_TIME_DIST_TIMING_CTL_TOD_TX_EN___MASK         	UINT32_C(0x1)
#define RUDRA40_TIME_DIST_TIMING_CTL_TOD_TX_EN___SHIFT        	0
#define RUDRA40_TIME_DIST_TIMING_CTL____REGMASK	UINT32_C(7421)

/* ---- RUDRA40_TIME_DIST_TIMING_MUX_SEL ---- */
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL____WIDTH	32
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_zarlink_rcvd_clk1___MASK   	UINT32_C(0xe0000000)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_zarlink_rcvd_clk1___SHIFT  	29
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_zarlink_rcvd_clk0___MASK   	UINT32_C(0x1c000000)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_zarlink_rcvd_clk0___SHIFT  	26
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_Unused_25___MASK           	UINT32_C(0x2000000)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_Unused_25___SHIFT          	25
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_clk_tx_sel_2___MASK    	UINT32_C(0x1800000)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_clk_tx_sel_2___SHIFT   	23
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_sync2_sel___MASK       	UINT32_C(0x400000)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_sync2_sel___SHIFT      	22
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_sync2_clk___MASK       	UINT32_C(0x200000)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_sync2_clk___SHIFT      	21
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_pps_src_sel___MASK         	UINT32_C(0x1c0000)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_pps_src_sel___SHIFT        	18
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_qsfpdd_epps_clk_sel___MASK 	UINT32_C(0x30000)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_qsfpdd_epps_clk_sel___SHIFT	16
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_Unused_15___MASK           	UINT32_C(0x8000)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_Unused_15___SHIFT          	15
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_clk_tx_sel_1___MASK    	UINT32_C(0x6000)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_clk_tx_sel_1___SHIFT   	13
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_zarlink_sync_sel___MASK    	UINT32_C(0x1c00)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_zarlink_sync_sel___SHIFT   	10
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_pps_tx_sel___MASK          	UINT32_C(0x3c0)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_pps_tx_sel___SHIFT         	6
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_Unused_5___MASK            	UINT32_C(0x20)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_Unused_5___SHIFT           	5
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_sync1_sel___MASK       	UINT32_C(0x10)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_sync1_sel___SHIFT      	4
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_Unused_3___MASK            	UINT32_C(0x8)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_Unused_3___SHIFT           	3
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_sync1_clk___MASK       	UINT32_C(0x4)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_gps_sync1_clk___SHIFT      	2
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_zarlink_clk_sel___MASK     	UINT32_C(0x3)
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL_zarlink_clk_sel___SHIFT    	0
#define RUDRA40_TIME_DIST_TIMING_MUX_SEL____REGMASK	UINT32_C(4261380055)

/* ---- RUDRA40_TIME_DIST_J2CA_TIMECODE_0 ---- */
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_0____WIDTH	32
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_0____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_J2CA_TIMECODE_0_Unused_17___MASK 	UINT32_C(0xfffe0000)
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_0_Unused_17___SHIFT	17
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_0_lock___MASK      	UINT32_C(0x10000)
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_0_lock___SHIFT     	16
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_0_epoch___MASK     	UINT32_C(0xffff)
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_0_epoch___SHIFT    	0
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_0____REGMASK	UINT32_C(131071)

/* ---- RUDRA40_TIME_DIST_J2CA_TIMECODE_1 ---- */
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_1____WIDTH	32
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_1____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_J2CA_TIMECODE_1_seconds___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_1_seconds___SHIFT	0
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_TIME_DIST_J2CA_TIMECODE_2 ---- */
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_2____WIDTH	32
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_2____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_J2CA_TIMECODE_2_unused___MASK      	UINT32_C(0xc0000000)
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_2_unused___SHIFT     	30
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_2_nanoseconds___MASK 	UINT32_C(0x3fffffff)
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_2_nanoseconds___SHIFT	0
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_TIME_DIST_J2CA_TIMECODE_3 ---- */
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_3____WIDTH	32
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_3____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_J2CA_TIMECODE_3_Unused_16___MASK 	UINT32_C(0xffff0000)
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_3_Unused_16___SHIFT	16
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_3_accuracy___MASK  	UINT32_C(0xff00)
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_3_accuracy___SHIFT 	8
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_3_checksum___MASK  	UINT32_C(0xff)
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_3_checksum___SHIFT 	0
#define RUDRA40_TIME_DIST_J2CA_TIMECODE_3____REGMASK	UINT32_C(65535)

/* ---- RUDRA40_TIME_DIST_TIMING_TEST_SIG_GEN ---- */
#define RUDRA40_TIME_DIST_TIMING_TEST_SIG_GEN____WIDTH	32
#define RUDRA40_TIME_DIST_TIMING_TEST_SIG_GEN____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_TIMING_TEST_SIG_GEN_Unused_2___MASK             	UINT32_C(0xfffffffc)
#define RUDRA40_TIME_DIST_TIMING_TEST_SIG_GEN_Unused_2___SHIFT            	2
#define RUDRA40_TIME_DIST_TIMING_TEST_SIG_GEN_gen_tod_tx_to_pps_rx___MASK 	UINT32_C(0x2)
#define RUDRA40_TIME_DIST_TIMING_TEST_SIG_GEN_gen_tod_tx_to_pps_rx___SHIFT	1
#define RUDRA40_TIME_DIST_TIMING_TEST_SIG_GEN_gen_pps_tx_to_tod_rx___MASK 	UINT32_C(0x1)
#define RUDRA40_TIME_DIST_TIMING_TEST_SIG_GEN_gen_pps_tx_to_tod_rx___SHIFT	0
#define RUDRA40_TIME_DIST_TIMING_TEST_SIG_GEN____REGMASK	UINT32_C(3)

/* ---- RUDRA40_TIME_DIST_TIMING_TEST_FREQ ---- */
#define RUDRA40_TIME_DIST_TIMING_TEST_FREQ____WIDTH	32
#define RUDRA40_TIME_DIST_TIMING_TEST_FREQ____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_TIMING_TEST_FREQ_Unused_24___MASK             	UINT32_C(0xff000000)
#define RUDRA40_TIME_DIST_TIMING_TEST_FREQ_Unused_24___SHIFT            	24
#define RUDRA40_TIME_DIST_TIMING_TEST_FREQ_zl30603_tclk_ref_freq___MASK 	UINT32_C(0xff0000)
#define RUDRA40_TIME_DIST_TIMING_TEST_FREQ_zl30603_tclk_ref_freq___SHIFT	16
#define RUDRA40_TIME_DIST_TIMING_TEST_FREQ_tod_rx_freq___MASK           	UINT32_C(0xff00)
#define RUDRA40_TIME_DIST_TIMING_TEST_FREQ_tod_rx_freq___SHIFT          	8
#define RUDRA40_TIME_DIST_TIMING_TEST_FREQ_pps_rx_freq___MASK           	UINT32_C(0xff)
#define RUDRA40_TIME_DIST_TIMING_TEST_FREQ_pps_rx_freq___SHIFT          	0
#define RUDRA40_TIME_DIST_TIMING_TEST_FREQ____REGMASK	UINT32_C(16777215)

/* ---- RUDRA40_TIME_DIST_SQUELCH ---- */
#define RUDRA40_TIME_DIST_SQUELCH____WIDTH	32
#define RUDRA40_TIME_DIST_SQUELCH____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_SQUELCH_Unused_2___MASK                 	UINT32_C(0xfffffffc)
#define RUDRA40_TIME_DIST_SQUELCH_Unused_2___SHIFT                	2
#define RUDRA40_TIME_DIST_SQUELCH_srd_refclk2_squelch_J2CA___MASK 	UINT32_C(0x2)
#define RUDRA40_TIME_DIST_SQUELCH_srd_refclk2_squelch_J2CA___SHIFT	1
#define RUDRA40_TIME_DIST_SQUELCH_srd_refclk1_squelch_J2CA___MASK 	UINT32_C(0x1)
#define RUDRA40_TIME_DIST_SQUELCH_srd_refclk1_squelch_J2CA___SHIFT	0
#define RUDRA40_TIME_DIST_SQUELCH____REGMASK	UINT32_C(3)

/* ---- RUDRA40_TIME_DIST_SHIFT_1PPS_REGS ---- */
#define RUDRA40_TIME_DIST_SHIFT_1PPS_REGS____WIDTH	32
#define RUDRA40_TIME_DIST_SHIFT_1PPS_REGS____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_SHIFT_1PPS_REGS____REGMASK	UINT32_C(0)

/* ---- RUDRA40_TIME_DIST_GPS_10MHZ_FREQ ---- */
#define RUDRA40_TIME_DIST_GPS_10MHZ_FREQ____WIDTH	32
#define RUDRA40_TIME_DIST_GPS_10MHZ_FREQ____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_GPS_10MHZ_FREQ_FREQUENCY___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_TIME_DIST_GPS_10MHZ_FREQ_FREQUENCY___SHIFT	0
#define RUDRA40_TIME_DIST_GPS_10MHZ_FREQ____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_TIME_DIST_GPS_PPS_FREQ ---- */
#define RUDRA40_TIME_DIST_GPS_PPS_FREQ____WIDTH	32
#define RUDRA40_TIME_DIST_GPS_PPS_FREQ____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_GPS_PPS_FREQ_Unused_8___MASK  	UINT32_C(0xffffff00)
#define RUDRA40_TIME_DIST_GPS_PPS_FREQ_Unused_8___SHIFT 	8
#define RUDRA40_TIME_DIST_GPS_PPS_FREQ_FREQUENCY___MASK 	UINT32_C(0xff)
#define RUDRA40_TIME_DIST_GPS_PPS_FREQ_FREQUENCY___SHIFT	0
#define RUDRA40_TIME_DIST_GPS_PPS_FREQ____REGMASK	UINT32_C(255)

/* ---- RUDRA40_TIME_DIST_RESERVED_0 ---- */
#define RUDRA40_TIME_DIST_RESERVED_0____WIDTH	32
#define RUDRA40_TIME_DIST_RESERVED_0____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_RESERVED_0_Unused_17___MASK 	UINT32_C(0xfffe0000)
#define RUDRA40_TIME_DIST_RESERVED_0_Unused_17___SHIFT	17
#define RUDRA40_TIME_DIST_RESERVED_0_Unused_0___MASK  	UINT32_C(0x1ffff)
#define RUDRA40_TIME_DIST_RESERVED_0_Unused_0___SHIFT 	0
#define RUDRA40_TIME_DIST_RESERVED_0____REGMASK	UINT32_C(0)

/* ---- RUDRA40_TIME_DIST_RESERVED_1 ---- */
#define RUDRA40_TIME_DIST_RESERVED_1____WIDTH	32
#define RUDRA40_TIME_DIST_RESERVED_1____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_RESERVED_1_Unused_0___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_TIME_DIST_RESERVED_1_Unused_0___SHIFT	0
#define RUDRA40_TIME_DIST_RESERVED_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_TIME_DIST_RESERVED_2 ---- */
#define RUDRA40_TIME_DIST_RESERVED_2____WIDTH	32
#define RUDRA40_TIME_DIST_RESERVED_2____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_RESERVED_2_Unused_0___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_TIME_DIST_RESERVED_2_Unused_0___SHIFT	0
#define RUDRA40_TIME_DIST_RESERVED_2____REGMASK	UINT32_C(0)

/* ---- RUDRA40_TIME_DIST_RESERVED_3 ---- */
#define RUDRA40_TIME_DIST_RESERVED_3____WIDTH	32
#define RUDRA40_TIME_DIST_RESERVED_3____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_RESERVED_3_Unused_16___MASK 	UINT32_C(0xffff0000)
#define RUDRA40_TIME_DIST_RESERVED_3_Unused_16___SHIFT	16
#define RUDRA40_TIME_DIST_RESERVED_3_Unused_0___MASK  	UINT32_C(0xffff)
#define RUDRA40_TIME_DIST_RESERVED_3_Unused_0___SHIFT 	0
#define RUDRA40_TIME_DIST_RESERVED_3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_TIME_DIST_THEEND ---- */
#define RUDRA40_TIME_DIST_THEEND____WIDTH	32
#define RUDRA40_TIME_DIST_THEEND____TYPE 	uint32_t

#define RUDRA40_TIME_DIST_THEEND____REGMASK	UINT32_C(0)

/* ---- RUDRA40_FLASH_SPI_DATA ---- */
#define RUDRA40_FLASH_SPI_DATA____WIDTH	32
#define RUDRA40_FLASH_SPI_DATA____TYPE 	uint32_t

#define RUDRA40_FLASH_SPI_DATA____REGMASK	UINT32_C(0)

/* ---- RUDRA40_FLASH_SPI_CTRL ---- */
#define RUDRA40_FLASH_SPI_CTRL____WIDTH	32
#define RUDRA40_FLASH_SPI_CTRL____TYPE 	uint32_t

#define RUDRA40_FLASH_SPI_CTRL_Unused_16___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_FLASH_SPI_CTRL_Unused_16___SHIFT           	16
#define RUDRA40_FLASH_SPI_CTRL_flush_SPI_data_fifos___MASK 	UINT32_C(0x8000)
#define RUDRA40_FLASH_SPI_CTRL_flush_SPI_data_fifos___SHIFT	15
#define RUDRA40_FLASH_SPI_CTRL_Unused_13___MASK            	UINT32_C(0x6000)
#define RUDRA40_FLASH_SPI_CTRL_Unused_13___SHIFT           	13
#define RUDRA40_FLASH_SPI_CTRL_write_FIFO_empty___MASK     	UINT32_C(0x1000)
#define RUDRA40_FLASH_SPI_CTRL_write_FIFO_empty___SHIFT    	12
#define RUDRA40_FLASH_SPI_CTRL_read_FIFO_empty___MASK      	UINT32_C(0x800)
#define RUDRA40_FLASH_SPI_CTRL_read_FIFO_empty___SHIFT     	11
#define RUDRA40_FLASH_SPI_CTRL_Unused_1___MASK             	UINT32_C(0x7fe)
#define RUDRA40_FLASH_SPI_CTRL_Unused_1___SHIFT            	1
#define RUDRA40_FLASH_SPI_CTRL_chip_select___MASK          	UINT32_C(0x1)
#define RUDRA40_FLASH_SPI_CTRL_chip_select___SHIFT         	0
#define RUDRA40_FLASH_SPI_CTRL____REGMASK	UINT32_C(38913)

/* ---- RUDRA40_FLASH_SPI_WRITE_FIFO_FILL_LEVEL ---- */
#define RUDRA40_FLASH_SPI_WRITE_FIFO_FILL_LEVEL____WIDTH	32
#define RUDRA40_FLASH_SPI_WRITE_FIFO_FILL_LEVEL____TYPE 	uint32_t

#define RUDRA40_FLASH_SPI_WRITE_FIFO_FILL_LEVEL_Unused_9___MASK   	UINT32_C(0xfffffe00)
#define RUDRA40_FLASH_SPI_WRITE_FIFO_FILL_LEVEL_Unused_9___SHIFT  	9
#define RUDRA40_FLASH_SPI_WRITE_FIFO_FILL_LEVEL_fill_level___MASK 	UINT32_C(0x1ff)
#define RUDRA40_FLASH_SPI_WRITE_FIFO_FILL_LEVEL_fill_level___SHIFT	0
#define RUDRA40_FLASH_SPI_WRITE_FIFO_FILL_LEVEL____REGMASK	UINT32_C(511)

/* ---- RUDRA40_FLASH_SPI_READ_FIFO_FILL_LEVEL ---- */
#define RUDRA40_FLASH_SPI_READ_FIFO_FILL_LEVEL____WIDTH	32
#define RUDRA40_FLASH_SPI_READ_FIFO_FILL_LEVEL____TYPE 	uint32_t

#define RUDRA40_FLASH_SPI_READ_FIFO_FILL_LEVEL_Unused_9___MASK   	UINT32_C(0xfffffe00)
#define RUDRA40_FLASH_SPI_READ_FIFO_FILL_LEVEL_Unused_9___SHIFT  	9
#define RUDRA40_FLASH_SPI_READ_FIFO_FILL_LEVEL_fill_level___MASK 	UINT32_C(0x1ff)
#define RUDRA40_FLASH_SPI_READ_FIFO_FILL_LEVEL_fill_level___SHIFT	0
#define RUDRA40_FLASH_SPI_READ_FIFO_FILL_LEVEL____REGMASK	UINT32_C(511)

/* ---- RUDRA40_FLASH_SPI_ERROR ---- */
#define RUDRA40_FLASH_SPI_ERROR____WIDTH	32
#define RUDRA40_FLASH_SPI_ERROR____TYPE 	uint32_t

#define RUDRA40_FLASH_SPI_ERROR_Unused_2___MASK                     	UINT32_C(0xfffffffc)
#define RUDRA40_FLASH_SPI_ERROR_Unused_2___SHIFT                    	2
#define RUDRA40_FLASH_SPI_ERROR_SPI_write_data_FIFO_overflow___MASK 	UINT32_C(0x2)
#define RUDRA40_FLASH_SPI_ERROR_SPI_write_data_FIFO_overflow___SHIFT	1
#define RUDRA40_FLASH_SPI_ERROR_SPI_read_data_FIFO_underflow___MASK 	UINT32_C(0x1)
#define RUDRA40_FLASH_SPI_ERROR_SPI_read_data_FIFO_underflow___SHIFT	0
#define RUDRA40_FLASH_SPI_ERROR____REGMASK	UINT32_C(3)

/* ---- RUDRA40_ADC_SPI_DATA ---- */
#define RUDRA40_ADC_SPI_DATA____WIDTH	32
#define RUDRA40_ADC_SPI_DATA____TYPE 	uint32_t

#define RUDRA40_ADC_SPI_DATA____REGMASK	UINT32_C(0)

/* ---- RUDRA40_ADC_SPI_CTRL ---- */
#define RUDRA40_ADC_SPI_CTRL____WIDTH	32
#define RUDRA40_ADC_SPI_CTRL____TYPE 	uint32_t

#define RUDRA40_ADC_SPI_CTRL_Unused_16___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_ADC_SPI_CTRL_Unused_16___SHIFT           	16
#define RUDRA40_ADC_SPI_CTRL_flush_SPI_data_fifos___MASK 	UINT32_C(0x8000)
#define RUDRA40_ADC_SPI_CTRL_flush_SPI_data_fifos___SHIFT	15
#define RUDRA40_ADC_SPI_CTRL_Unused_13___MASK            	UINT32_C(0x6000)
#define RUDRA40_ADC_SPI_CTRL_Unused_13___SHIFT           	13
#define RUDRA40_ADC_SPI_CTRL_write_FIFO_empty___MASK     	UINT32_C(0x1000)
#define RUDRA40_ADC_SPI_CTRL_write_FIFO_empty___SHIFT    	12
#define RUDRA40_ADC_SPI_CTRL_read_FIFO_empty___MASK      	UINT32_C(0x800)
#define RUDRA40_ADC_SPI_CTRL_read_FIFO_empty___SHIFT     	11
#define RUDRA40_ADC_SPI_CTRL_Unused_1___MASK             	UINT32_C(0x7fe)
#define RUDRA40_ADC_SPI_CTRL_Unused_1___SHIFT            	1
#define RUDRA40_ADC_SPI_CTRL_chip_select___MASK          	UINT32_C(0x1)
#define RUDRA40_ADC_SPI_CTRL_chip_select___SHIFT         	0
#define RUDRA40_ADC_SPI_CTRL____REGMASK	UINT32_C(38913)

/* ---- RUDRA40_ADC_SPI_WRITE_FIFO_FILL_LEVEL ---- */
#define RUDRA40_ADC_SPI_WRITE_FIFO_FILL_LEVEL____WIDTH	32
#define RUDRA40_ADC_SPI_WRITE_FIFO_FILL_LEVEL____TYPE 	uint32_t

#define RUDRA40_ADC_SPI_WRITE_FIFO_FILL_LEVEL_Unused_9___MASK   	UINT32_C(0xfffffe00)
#define RUDRA40_ADC_SPI_WRITE_FIFO_FILL_LEVEL_Unused_9___SHIFT  	9
#define RUDRA40_ADC_SPI_WRITE_FIFO_FILL_LEVEL_fill_level___MASK 	UINT32_C(0x1ff)
#define RUDRA40_ADC_SPI_WRITE_FIFO_FILL_LEVEL_fill_level___SHIFT	0
#define RUDRA40_ADC_SPI_WRITE_FIFO_FILL_LEVEL____REGMASK	UINT32_C(511)

/* ---- RUDRA40_ADC_SPI_READ_FIFO_FILL_LEVEL ---- */
#define RUDRA40_ADC_SPI_READ_FIFO_FILL_LEVEL____WIDTH	32
#define RUDRA40_ADC_SPI_READ_FIFO_FILL_LEVEL____TYPE 	uint32_t

#define RUDRA40_ADC_SPI_READ_FIFO_FILL_LEVEL_Unused_9___MASK   	UINT32_C(0xfffffe00)
#define RUDRA40_ADC_SPI_READ_FIFO_FILL_LEVEL_Unused_9___SHIFT  	9
#define RUDRA40_ADC_SPI_READ_FIFO_FILL_LEVEL_fill_level___MASK 	UINT32_C(0x1ff)
#define RUDRA40_ADC_SPI_READ_FIFO_FILL_LEVEL_fill_level___SHIFT	0
#define RUDRA40_ADC_SPI_READ_FIFO_FILL_LEVEL____REGMASK	UINT32_C(511)

/* ---- RUDRA40_ADC_SPI_ERROR ---- */
#define RUDRA40_ADC_SPI_ERROR____WIDTH	32
#define RUDRA40_ADC_SPI_ERROR____TYPE 	uint32_t

#define RUDRA40_ADC_SPI_ERROR_Unused_2___MASK                     	UINT32_C(0xfffffffc)
#define RUDRA40_ADC_SPI_ERROR_Unused_2___SHIFT                    	2
#define RUDRA40_ADC_SPI_ERROR_SPI_write_data_FIFO_overflow___MASK 	UINT32_C(0x2)
#define RUDRA40_ADC_SPI_ERROR_SPI_write_data_FIFO_overflow___SHIFT	1
#define RUDRA40_ADC_SPI_ERROR_SPI_read_data_FIFO_underflow___MASK 	UINT32_C(0x1)
#define RUDRA40_ADC_SPI_ERROR_SPI_read_data_FIFO_underflow___SHIFT	0
#define RUDRA40_ADC_SPI_ERROR____REGMASK	UINT32_C(3)

/* ---- RUDRA40_SKT_MON_SPI_DATA ---- */
#define RUDRA40_SKT_MON_SPI_DATA____WIDTH	32
#define RUDRA40_SKT_MON_SPI_DATA____TYPE 	uint32_t

#define RUDRA40_SKT_MON_SPI_DATA____REGMASK	UINT32_C(0)

/* ---- RUDRA40_SKT_MON_SPI_CTRL ---- */
#define RUDRA40_SKT_MON_SPI_CTRL____WIDTH	32
#define RUDRA40_SKT_MON_SPI_CTRL____TYPE 	uint32_t

#define RUDRA40_SKT_MON_SPI_CTRL_Unused_16___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_SKT_MON_SPI_CTRL_Unused_16___SHIFT           	16
#define RUDRA40_SKT_MON_SPI_CTRL_flush_SPI_data_fifos___MASK 	UINT32_C(0x8000)
#define RUDRA40_SKT_MON_SPI_CTRL_flush_SPI_data_fifos___SHIFT	15
#define RUDRA40_SKT_MON_SPI_CTRL_Unused_13___MASK            	UINT32_C(0x6000)
#define RUDRA40_SKT_MON_SPI_CTRL_Unused_13___SHIFT           	13
#define RUDRA40_SKT_MON_SPI_CTRL_write_FIFO_empty___MASK     	UINT32_C(0x1000)
#define RUDRA40_SKT_MON_SPI_CTRL_write_FIFO_empty___SHIFT    	12
#define RUDRA40_SKT_MON_SPI_CTRL_read_FIFO_empty___MASK      	UINT32_C(0x800)
#define RUDRA40_SKT_MON_SPI_CTRL_read_FIFO_empty___SHIFT     	11
#define RUDRA40_SKT_MON_SPI_CTRL_Unused_1___MASK             	UINT32_C(0x7fe)
#define RUDRA40_SKT_MON_SPI_CTRL_Unused_1___SHIFT            	1
#define RUDRA40_SKT_MON_SPI_CTRL_chip_select___MASK          	UINT32_C(0x1)
#define RUDRA40_SKT_MON_SPI_CTRL_chip_select___SHIFT         	0
#define RUDRA40_SKT_MON_SPI_CTRL____REGMASK	UINT32_C(38913)

/* ---- RUDRA40_SKT_MON_SPI_WRITE_FIFO_FILL_LEVEL ---- */
#define RUDRA40_SKT_MON_SPI_WRITE_FIFO_FILL_LEVEL____WIDTH	32
#define RUDRA40_SKT_MON_SPI_WRITE_FIFO_FILL_LEVEL____TYPE 	uint32_t

#define RUDRA40_SKT_MON_SPI_WRITE_FIFO_FILL_LEVEL_Unused_9___MASK   	UINT32_C(0xfffffe00)
#define RUDRA40_SKT_MON_SPI_WRITE_FIFO_FILL_LEVEL_Unused_9___SHIFT  	9
#define RUDRA40_SKT_MON_SPI_WRITE_FIFO_FILL_LEVEL_fill_level___MASK 	UINT32_C(0x1ff)
#define RUDRA40_SKT_MON_SPI_WRITE_FIFO_FILL_LEVEL_fill_level___SHIFT	0
#define RUDRA40_SKT_MON_SPI_WRITE_FIFO_FILL_LEVEL____REGMASK	UINT32_C(511)

/* ---- RUDRA40_SKT_MON_SPI_READ_FIFO_FILL_LEVEL ---- */
#define RUDRA40_SKT_MON_SPI_READ_FIFO_FILL_LEVEL____WIDTH	32
#define RUDRA40_SKT_MON_SPI_READ_FIFO_FILL_LEVEL____TYPE 	uint32_t

#define RUDRA40_SKT_MON_SPI_READ_FIFO_FILL_LEVEL_Unused_9___MASK   	UINT32_C(0xfffffe00)
#define RUDRA40_SKT_MON_SPI_READ_FIFO_FILL_LEVEL_Unused_9___SHIFT  	9
#define RUDRA40_SKT_MON_SPI_READ_FIFO_FILL_LEVEL_fill_level___MASK 	UINT32_C(0x1ff)
#define RUDRA40_SKT_MON_SPI_READ_FIFO_FILL_LEVEL_fill_level___SHIFT	0
#define RUDRA40_SKT_MON_SPI_READ_FIFO_FILL_LEVEL____REGMASK	UINT32_C(511)

/* ---- RUDRA40_SKT_MON_SPI_ERROR ---- */
#define RUDRA40_SKT_MON_SPI_ERROR____WIDTH	32
#define RUDRA40_SKT_MON_SPI_ERROR____TYPE 	uint32_t

#define RUDRA40_SKT_MON_SPI_ERROR_Unused_2___MASK                     	UINT32_C(0xfffffffc)
#define RUDRA40_SKT_MON_SPI_ERROR_Unused_2___SHIFT                    	2
#define RUDRA40_SKT_MON_SPI_ERROR_SPI_write_data_FIFO_overflow___MASK 	UINT32_C(0x2)
#define RUDRA40_SKT_MON_SPI_ERROR_SPI_write_data_FIFO_overflow___SHIFT	1
#define RUDRA40_SKT_MON_SPI_ERROR_SPI_read_data_FIFO_underflow___MASK 	UINT32_C(0x1)
#define RUDRA40_SKT_MON_SPI_ERROR_SPI_read_data_FIFO_underflow___SHIFT	0
#define RUDRA40_SKT_MON_SPI_ERROR____REGMASK	UINT32_C(3)

/* ---- RUDRA40_J2CA_SPI_DATA ---- */
#define RUDRA40_J2CA_SPI_DATA____WIDTH	32
#define RUDRA40_J2CA_SPI_DATA____TYPE 	uint32_t

#define RUDRA40_J2CA_SPI_DATA____REGMASK	UINT32_C(0)

/* ---- RUDRA40_J2CA_SPI_CTRL ---- */
#define RUDRA40_J2CA_SPI_CTRL____WIDTH	32
#define RUDRA40_J2CA_SPI_CTRL____TYPE 	uint32_t

#define RUDRA40_J2CA_SPI_CTRL_Unused_16___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_J2CA_SPI_CTRL_Unused_16___SHIFT           	16
#define RUDRA40_J2CA_SPI_CTRL_flush_SPI_data_fifos___MASK 	UINT32_C(0x8000)
#define RUDRA40_J2CA_SPI_CTRL_flush_SPI_data_fifos___SHIFT	15
#define RUDRA40_J2CA_SPI_CTRL_Unused_13___MASK            	UINT32_C(0x6000)
#define RUDRA40_J2CA_SPI_CTRL_Unused_13___SHIFT           	13
#define RUDRA40_J2CA_SPI_CTRL_write_FIFO_empty___MASK     	UINT32_C(0x1000)
#define RUDRA40_J2CA_SPI_CTRL_write_FIFO_empty___SHIFT    	12
#define RUDRA40_J2CA_SPI_CTRL_read_FIFO_empty___MASK      	UINT32_C(0x800)
#define RUDRA40_J2CA_SPI_CTRL_read_FIFO_empty___SHIFT     	11
#define RUDRA40_J2CA_SPI_CTRL_Unused_1___MASK             	UINT32_C(0x7fe)
#define RUDRA40_J2CA_SPI_CTRL_Unused_1___SHIFT            	1
#define RUDRA40_J2CA_SPI_CTRL_chip_select___MASK          	UINT32_C(0x1)
#define RUDRA40_J2CA_SPI_CTRL_chip_select___SHIFT         	0
#define RUDRA40_J2CA_SPI_CTRL____REGMASK	UINT32_C(38913)

/* ---- RUDRA40_J2CA_SPI_WRITE_FIFO_FILL_LEVEL ---- */
#define RUDRA40_J2CA_SPI_WRITE_FIFO_FILL_LEVEL____WIDTH	32
#define RUDRA40_J2CA_SPI_WRITE_FIFO_FILL_LEVEL____TYPE 	uint32_t

#define RUDRA40_J2CA_SPI_WRITE_FIFO_FILL_LEVEL_Unused_9___MASK   	UINT32_C(0xfffffe00)
#define RUDRA40_J2CA_SPI_WRITE_FIFO_FILL_LEVEL_Unused_9___SHIFT  	9
#define RUDRA40_J2CA_SPI_WRITE_FIFO_FILL_LEVEL_fill_level___MASK 	UINT32_C(0x1ff)
#define RUDRA40_J2CA_SPI_WRITE_FIFO_FILL_LEVEL_fill_level___SHIFT	0
#define RUDRA40_J2CA_SPI_WRITE_FIFO_FILL_LEVEL____REGMASK	UINT32_C(511)

/* ---- RUDRA40_J2CA_SPI_READ_FIFO_FILL_LEVEL ---- */
#define RUDRA40_J2CA_SPI_READ_FIFO_FILL_LEVEL____WIDTH	32
#define RUDRA40_J2CA_SPI_READ_FIFO_FILL_LEVEL____TYPE 	uint32_t

#define RUDRA40_J2CA_SPI_READ_FIFO_FILL_LEVEL_Unused_9___MASK   	UINT32_C(0xfffffe00)
#define RUDRA40_J2CA_SPI_READ_FIFO_FILL_LEVEL_Unused_9___SHIFT  	9
#define RUDRA40_J2CA_SPI_READ_FIFO_FILL_LEVEL_fill_level___MASK 	UINT32_C(0x1ff)
#define RUDRA40_J2CA_SPI_READ_FIFO_FILL_LEVEL_fill_level___SHIFT	0
#define RUDRA40_J2CA_SPI_READ_FIFO_FILL_LEVEL____REGMASK	UINT32_C(511)

/* ---- RUDRA40_J2CA_SPI_ERROR ---- */
#define RUDRA40_J2CA_SPI_ERROR____WIDTH	32
#define RUDRA40_J2CA_SPI_ERROR____TYPE 	uint32_t

#define RUDRA40_J2CA_SPI_ERROR_Unused_2___MASK                     	UINT32_C(0xfffffffc)
#define RUDRA40_J2CA_SPI_ERROR_Unused_2___SHIFT                    	2
#define RUDRA40_J2CA_SPI_ERROR_SPI_write_data_FIFO_overflow___MASK 	UINT32_C(0x2)
#define RUDRA40_J2CA_SPI_ERROR_SPI_write_data_FIFO_overflow___SHIFT	1
#define RUDRA40_J2CA_SPI_ERROR_SPI_read_data_FIFO_underflow___MASK 	UINT32_C(0x1)
#define RUDRA40_J2CA_SPI_ERROR_SPI_read_data_FIFO_underflow___SHIFT	0
#define RUDRA40_J2CA_SPI_ERROR____REGMASK	UINT32_C(3)

/* ---- RUDRA40_EPPS_SPI_DATA ---- */
#define RUDRA40_EPPS_SPI_DATA____WIDTH	32
#define RUDRA40_EPPS_SPI_DATA____TYPE 	uint32_t

#define RUDRA40_EPPS_SPI_DATA____REGMASK	UINT32_C(0)

/* ---- RUDRA40_EPPS_SPI_CTRL ---- */
#define RUDRA40_EPPS_SPI_CTRL____WIDTH	32
#define RUDRA40_EPPS_SPI_CTRL____TYPE 	uint32_t

#define RUDRA40_EPPS_SPI_CTRL_Unused_16___MASK            	UINT32_C(0xffff0000)
#define RUDRA40_EPPS_SPI_CTRL_Unused_16___SHIFT           	16
#define RUDRA40_EPPS_SPI_CTRL_flush_SPI_data_fifos___MASK 	UINT32_C(0x8000)
#define RUDRA40_EPPS_SPI_CTRL_flush_SPI_data_fifos___SHIFT	15
#define RUDRA40_EPPS_SPI_CTRL_Unused_13___MASK            	UINT32_C(0x6000)
#define RUDRA40_EPPS_SPI_CTRL_Unused_13___SHIFT           	13
#define RUDRA40_EPPS_SPI_CTRL_write_FIFO_empty___MASK     	UINT32_C(0x1000)
#define RUDRA40_EPPS_SPI_CTRL_write_FIFO_empty___SHIFT    	12
#define RUDRA40_EPPS_SPI_CTRL_read_FIFO_empty___MASK      	UINT32_C(0x800)
#define RUDRA40_EPPS_SPI_CTRL_read_FIFO_empty___SHIFT     	11
#define RUDRA40_EPPS_SPI_CTRL_Unused_1___MASK             	UINT32_C(0x7fe)
#define RUDRA40_EPPS_SPI_CTRL_Unused_1___SHIFT            	1
#define RUDRA40_EPPS_SPI_CTRL_chip_select___MASK          	UINT32_C(0x1)
#define RUDRA40_EPPS_SPI_CTRL_chip_select___SHIFT         	0
#define RUDRA40_EPPS_SPI_CTRL____REGMASK	UINT32_C(38913)

/* ---- RUDRA40_EPPS_SPI_WRITE_FIFO_FILL_LEVEL ---- */
#define RUDRA40_EPPS_SPI_WRITE_FIFO_FILL_LEVEL____WIDTH	32
#define RUDRA40_EPPS_SPI_WRITE_FIFO_FILL_LEVEL____TYPE 	uint32_t

#define RUDRA40_EPPS_SPI_WRITE_FIFO_FILL_LEVEL_Unused_9___MASK   	UINT32_C(0xfffffe00)
#define RUDRA40_EPPS_SPI_WRITE_FIFO_FILL_LEVEL_Unused_9___SHIFT  	9
#define RUDRA40_EPPS_SPI_WRITE_FIFO_FILL_LEVEL_fill_level___MASK 	UINT32_C(0x1ff)
#define RUDRA40_EPPS_SPI_WRITE_FIFO_FILL_LEVEL_fill_level___SHIFT	0
#define RUDRA40_EPPS_SPI_WRITE_FIFO_FILL_LEVEL____REGMASK	UINT32_C(511)

/* ---- RUDRA40_EPPS_SPI_READ_FIFO_FILL_LEVEL ---- */
#define RUDRA40_EPPS_SPI_READ_FIFO_FILL_LEVEL____WIDTH	32
#define RUDRA40_EPPS_SPI_READ_FIFO_FILL_LEVEL____TYPE 	uint32_t

#define RUDRA40_EPPS_SPI_READ_FIFO_FILL_LEVEL_Unused_9___MASK   	UINT32_C(0xfffffe00)
#define RUDRA40_EPPS_SPI_READ_FIFO_FILL_LEVEL_Unused_9___SHIFT  	9
#define RUDRA40_EPPS_SPI_READ_FIFO_FILL_LEVEL_fill_level___MASK 	UINT32_C(0x1ff)
#define RUDRA40_EPPS_SPI_READ_FIFO_FILL_LEVEL_fill_level___SHIFT	0
#define RUDRA40_EPPS_SPI_READ_FIFO_FILL_LEVEL____REGMASK	UINT32_C(511)

/* ---- RUDRA40_EPPS_SPI_ERROR ---- */
#define RUDRA40_EPPS_SPI_ERROR____WIDTH	32
#define RUDRA40_EPPS_SPI_ERROR____TYPE 	uint32_t

#define RUDRA40_EPPS_SPI_ERROR_Unused_2___MASK                     	UINT32_C(0xfffffffc)
#define RUDRA40_EPPS_SPI_ERROR_Unused_2___SHIFT                    	2
#define RUDRA40_EPPS_SPI_ERROR_SPI_write_data_FIFO_overflow___MASK 	UINT32_C(0x2)
#define RUDRA40_EPPS_SPI_ERROR_SPI_write_data_FIFO_overflow___SHIFT	1
#define RUDRA40_EPPS_SPI_ERROR_SPI_read_data_FIFO_underflow___MASK 	UINT32_C(0x1)
#define RUDRA40_EPPS_SPI_ERROR_SPI_read_data_FIFO_underflow___SHIFT	0
#define RUDRA40_EPPS_SPI_ERROR____REGMASK	UINT32_C(3)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_OVERRIDE ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_OVERRIDE____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_OVERRIDE____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_OVERRIDE_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_OVERRIDE_Unused_1___SHIFT      	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_OVERRIDE_sw_override_en___MASK 	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_OVERRIDE_sw_override_en___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_OVERRIDE____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_Unused_8___SHIFT        	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_RESET_MASTER___SHIFT    	7
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_Unused_6___MASK         	UINT32_C(0x40)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_Unused_6___SHIFT        	6
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_RD_WRn___SHIFT          	3
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_ABORT___SHIFT           	2
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_FINISH___SHIFT          	1
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_START___MASK            	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0_START___SHIFT           	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL0____REGMASK	UINT32_C(191)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_Unused_8___SHIFT        	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_RESET_MASTER___SHIFT    	7
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_Unused_6___MASK         	UINT32_C(0x40)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_Unused_6___SHIFT        	6
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_RD_WRn___SHIFT          	3
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_ABORT___SHIFT           	2
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_FINISH___SHIFT          	1
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_START___MASK            	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1_START___SHIFT           	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL1____REGMASK	UINT32_C(191)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_Unused_8___SHIFT        	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_RESET_MASTER___SHIFT    	7
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_Unused_6___MASK         	UINT32_C(0x40)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_Unused_6___SHIFT        	6
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_RD_WRn___SHIFT          	3
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_ABORT___SHIFT           	2
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_FINISH___SHIFT          	1
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_START___MASK            	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2_START___SHIFT           	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL2____REGMASK	UINT32_C(191)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_Unused_8___SHIFT        	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_RESET_MASTER___SHIFT    	7
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_Unused_6___MASK         	UINT32_C(0x40)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_Unused_6___SHIFT        	6
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_RD_WRn___SHIFT          	3
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_ABORT___SHIFT           	2
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_FINISH___SHIFT          	1
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_START___MASK            	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3_START___SHIFT           	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL3____REGMASK	UINT32_C(191)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_Unused_8___SHIFT        	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_RESET_MASTER___SHIFT    	7
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_Unused_6___MASK         	UINT32_C(0x40)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_Unused_6___SHIFT        	6
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_RD_WRn___SHIFT          	3
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_ABORT___SHIFT           	2
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_FINISH___SHIFT          	1
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_START___MASK            	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4_START___SHIFT           	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL4____REGMASK	UINT32_C(191)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_Unused_8___SHIFT        	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_RESET_MASTER___SHIFT    	7
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_Unused_6___MASK         	UINT32_C(0x40)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_Unused_6___SHIFT        	6
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_RD_WRn___SHIFT          	3
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_ABORT___SHIFT           	2
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_FINISH___SHIFT          	1
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_START___MASK            	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5_START___SHIFT           	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_CTRL5____REGMASK	UINT32_C(191)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_4X0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X0_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X0_CLK_DIV___MASK  	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X0_CLK_DIV___SHIFT 	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_4X1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X1_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X1_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X1_CLK_DIV___MASK  	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X1_CLK_DIV___SHIFT 	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X1____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_4X2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X2_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X2_CLK_DIV___MASK  	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X2_CLK_DIV___SHIFT 	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_4X3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X3_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X3_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X3_CLK_DIV___MASK  	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X3_CLK_DIV___SHIFT 	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X3____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_4X4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X4_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X4_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X4_CLK_DIV___MASK  	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X4_CLK_DIV___SHIFT 	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X4____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_4X5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X5_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X5_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X5_CLK_DIV___MASK  	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X5_CLK_DIV___SHIFT 	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_4X5____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR0____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR2____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR4____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEVADDR5____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR0____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR2____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR4____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATAADDR5____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD0____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD2____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD4____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_VLD5____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG0_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG0_Unused_9___SHIFT 	9
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG0_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG0_ABORT_LOC___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG0____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG1_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG1_Unused_9___SHIFT 	9
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG1_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG1_ABORT_LOC___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG1____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG2_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG2_Unused_9___SHIFT 	9
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG2_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG2_ABORT_LOC___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG2____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG3_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG3_Unused_9___SHIFT 	9
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG3_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG3_ABORT_LOC___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG3____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG4_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG4_Unused_9___SHIFT 	9
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG4_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG4_ABORT_LOC___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG4____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG5_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG5_Unused_9___SHIFT 	9
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG5_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG5_ABORT_LOC___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DEBUG5____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR0_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR1_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR2_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR3_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR4_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR5_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_WR5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD0_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD1_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD2_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD3_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD4_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD5_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_DIAG_DATA_RD5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_Unused_6___SHIFT	6
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_READACK___SHIFT 	5
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_READNACK___SHIFT	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_WRITE___SHIFT   	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_STOP___SHIFT    	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_START___MASK    	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_START___SHIFT   	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0_RESET___SHIFT   	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL0____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_Unused_6___SHIFT	6
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_READACK___SHIFT 	5
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_READNACK___SHIFT	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_WRITE___SHIFT   	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_STOP___SHIFT    	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_START___MASK    	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_START___SHIFT   	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1_RESET___SHIFT   	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL1____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_Unused_6___SHIFT	6
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_READACK___SHIFT 	5
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_READNACK___SHIFT	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_WRITE___SHIFT   	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_STOP___SHIFT    	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_START___MASK    	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_START___SHIFT   	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2_RESET___SHIFT   	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL2____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_Unused_6___SHIFT	6
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_READACK___SHIFT 	5
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_READNACK___SHIFT	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_WRITE___SHIFT   	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_STOP___SHIFT    	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_START___MASK    	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_START___SHIFT   	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3_RESET___SHIFT   	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL3____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_Unused_6___SHIFT	6
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_READACK___SHIFT 	5
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_READNACK___SHIFT	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_WRITE___SHIFT   	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_STOP___SHIFT    	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_START___MASK    	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_START___SHIFT   	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4_RESET___SHIFT   	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL4____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_Unused_6___SHIFT	6
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_READACK___SHIFT 	5
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_READNACK___SHIFT	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_WRITE___SHIFT   	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_STOP___SHIFT    	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_START___MASK    	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_START___SHIFT   	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5_RESET___SHIFT   	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_CTRL5____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_STAT0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_STATE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_SDA___SHIFT     	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_SCL___SHIFT     	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_ACKR___SHIFT    	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0_STAT___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_STAT1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_STATE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_SDA___SHIFT     	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_SCL___SHIFT     	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_ACKR___SHIFT    	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1_STAT___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT1____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_STAT2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_STATE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_SDA___SHIFT     	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_SCL___SHIFT     	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_ACKR___SHIFT    	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2_STAT___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_STAT3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_STATE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_SDA___SHIFT     	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_SCL___SHIFT     	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_ACKR___SHIFT    	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3_STAT___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT3____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_STAT4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_STATE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_SDA___SHIFT     	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_SCL___SHIFT     	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_ACKR___SHIFT    	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4_STAT___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT4____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_STAT5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_STATE___SHIFT   	4
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_SDA___SHIFT     	3
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_SCL___SHIFT     	2
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_ACKR___SHIFT    	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5_STAT___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_STAT5____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA0_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA0_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA0_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA1_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA1_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA1_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA1_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA1____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA2_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA2_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA2_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA3_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA3_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA3_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA3_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA3____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA4_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA4_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA4_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA4_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA4____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA5_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA5_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA5_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA5_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_WRITE_DATA5____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA0_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA0_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA0_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA1_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA1_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA1_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA1_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA1____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA2_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA2_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA2_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA3_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA3_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA3_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA3_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA3____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA4_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA4_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA4_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA4_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA4____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA5_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA5_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA5_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA5_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_READ_DATA5____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_DONE0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE0_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE0_Unused_1___SHIFT	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE0_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE0_CMP___SHIFT     	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE0____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_DONE1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE1_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE1_Unused_1___SHIFT	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE1_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE1_CMP___SHIFT     	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE1____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_DONE2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE2_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE2_Unused_1___SHIFT	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE2_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE2_CMP___SHIFT     	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE2____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_DONE3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE3_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE3_Unused_1___SHIFT	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE3_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE3_CMP___SHIFT     	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE3____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_DONE4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE4_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE4_Unused_1___SHIFT	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE4_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE4_CMP___SHIFT     	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE4____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_DONE5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE5_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE5_Unused_1___SHIFT	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE5_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE5_CMP___SHIFT     	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_DONE5____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL0_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL0_Unused_1___SHIFT      	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL0_exp_sel_sw_i2c___MASK 	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL0_exp_sel_sw_i2c___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL0____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL1_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL1_Unused_1___SHIFT      	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL1_exp_sel_sw_i2c___MASK 	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL1_exp_sel_sw_i2c___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL1____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL2_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL2_Unused_1___SHIFT      	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL2_exp_sel_sw_i2c___MASK 	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL2_exp_sel_sw_i2c___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL2____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL3_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL3_Unused_1___SHIFT      	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL3_exp_sel_sw_i2c___MASK 	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL3_exp_sel_sw_i2c___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL3____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL4_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL4_Unused_1___SHIFT      	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL4_exp_sel_sw_i2c___MASK 	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL4_exp_sel_sw_i2c___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL4____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL5_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL5_Unused_1___SHIFT      	1
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL5_exp_sel_sw_i2c___MASK 	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL5_exp_sel_sw_i2c___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL5____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL0 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL0____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL0_Unused_12___MASK             	UINT32_C(0xfffff000)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL0_Unused_12___SHIFT            	12
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL0_ioexp_i2c_half_period___MASK 	UINT32_C(0xfff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL0_ioexp_i2c_half_period___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL0____REGMASK	UINT32_C(4095)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL1 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL1____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL1_Unused_12___MASK             	UINT32_C(0xfffff000)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL1_Unused_12___SHIFT            	12
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL1_ioexp_i2c_half_period___MASK 	UINT32_C(0xfff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL1_ioexp_i2c_half_period___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL1____REGMASK	UINT32_C(4095)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL2 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL2____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL2_Unused_12___MASK             	UINT32_C(0xfffff000)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL2_Unused_12___SHIFT            	12
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL2_ioexp_i2c_half_period___MASK 	UINT32_C(0xfff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL2_ioexp_i2c_half_period___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL2____REGMASK	UINT32_C(4095)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL3 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL3____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL3_Unused_12___MASK             	UINT32_C(0xfffff000)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL3_Unused_12___SHIFT            	12
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL3_ioexp_i2c_half_period___MASK 	UINT32_C(0xfff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL3_ioexp_i2c_half_period___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL3____REGMASK	UINT32_C(4095)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL4 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL4____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL4_Unused_12___MASK             	UINT32_C(0xfffff000)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL4_Unused_12___SHIFT            	12
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL4_ioexp_i2c_half_period___MASK 	UINT32_C(0xfff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL4_ioexp_i2c_half_period___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL4____REGMASK	UINT32_C(4095)

/* ---- RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL5 ---- */
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL5____WIDTH	32
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL5_Unused_12___MASK             	UINT32_C(0xfffff000)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL5_Unused_12___SHIFT            	12
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL5_ioexp_i2c_half_period___MASK 	UINT32_C(0xfff)
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL5_ioexp_i2c_half_period___SHIFT	0
#define RUDRA40_MAIN_I2C_IOEXP_SW_MISC_CTRL5____REGMASK	UINT32_C(4095)

/* ---- RUDRA40_MAIN_I2C_SW_CTRL ---- */
#define RUDRA40_MAIN_I2C_SW_CTRL____WIDTH	32
#define RUDRA40_MAIN_I2C_SW_CTRL____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_SW_CTRL_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_MAIN_I2C_SW_CTRL_Unused_6___SHIFT	6
#define RUDRA40_MAIN_I2C_SW_CTRL_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_SW_CTRL_READACK___SHIFT 	5
#define RUDRA40_MAIN_I2C_SW_CTRL_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_SW_CTRL_READNACK___SHIFT	4
#define RUDRA40_MAIN_I2C_SW_CTRL_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_SW_CTRL_WRITE___SHIFT   	3
#define RUDRA40_MAIN_I2C_SW_CTRL_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_SW_CTRL_STOP___SHIFT    	2
#define RUDRA40_MAIN_I2C_SW_CTRL_START___MASK    	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_SW_CTRL_START___SHIFT   	1
#define RUDRA40_MAIN_I2C_SW_CTRL_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_SW_CTRL_RESET___SHIFT   	0
#define RUDRA40_MAIN_I2C_SW_CTRL____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MAIN_I2C_SW_STAT ---- */
#define RUDRA40_MAIN_I2C_SW_STAT____WIDTH	32
#define RUDRA40_MAIN_I2C_SW_STAT____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_SW_STAT_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_SW_STAT_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_SW_STAT_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_MAIN_I2C_SW_STAT_STATE___SHIFT   	4
#define RUDRA40_MAIN_I2C_SW_STAT_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_SW_STAT_SDA___SHIFT     	3
#define RUDRA40_MAIN_I2C_SW_STAT_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_SW_STAT_SCL___SHIFT     	2
#define RUDRA40_MAIN_I2C_SW_STAT_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_SW_STAT_ACKR___SHIFT    	1
#define RUDRA40_MAIN_I2C_SW_STAT_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_SW_STAT_STAT___SHIFT    	0
#define RUDRA40_MAIN_I2C_SW_STAT____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_SW_WRITE_DATA ---- */
#define RUDRA40_MAIN_I2C_SW_WRITE_DATA____WIDTH	32
#define RUDRA40_MAIN_I2C_SW_WRITE_DATA____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_SW_WRITE_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_SW_WRITE_DATA_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_SW_WRITE_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_SW_WRITE_DATA_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_SW_WRITE_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_SW_READ_DATA ---- */
#define RUDRA40_MAIN_I2C_SW_READ_DATA____WIDTH	32
#define RUDRA40_MAIN_I2C_SW_READ_DATA____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_SW_READ_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_SW_READ_DATA_Unused_8___SHIFT	8
#define RUDRA40_MAIN_I2C_SW_READ_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MAIN_I2C_SW_READ_DATA_DATA___SHIFT    	0
#define RUDRA40_MAIN_I2C_SW_READ_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_SW_DONE ---- */
#define RUDRA40_MAIN_I2C_SW_DONE____WIDTH	32
#define RUDRA40_MAIN_I2C_SW_DONE____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_SW_DONE_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_MAIN_I2C_SW_DONE_Unused_1___SHIFT	1
#define RUDRA40_MAIN_I2C_SW_DONE_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_SW_DONE_CMP___SHIFT     	0
#define RUDRA40_MAIN_I2C_SW_DONE____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MAIN_I2C_DIAG_CTRL ---- */
#define RUDRA40_MAIN_I2C_DIAG_CTRL____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_CTRL____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_CTRL_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_MAIN_I2C_DIAG_CTRL_Unused_8___SHIFT        	8
#define RUDRA40_MAIN_I2C_DIAG_CTRL_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_MAIN_I2C_DIAG_CTRL_RESET_MASTER___SHIFT    	7
#define RUDRA40_MAIN_I2C_DIAG_CTRL_SLAVE_ADDRS_NACK___MASK 	UINT32_C(0x40)
#define RUDRA40_MAIN_I2C_DIAG_CTRL_SLAVE_ADDRS_NACK___SHIFT	6
#define RUDRA40_MAIN_I2C_DIAG_CTRL_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_MAIN_I2C_DIAG_CTRL_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_MAIN_I2C_DIAG_CTRL_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_MAIN_I2C_DIAG_CTRL_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_MAIN_I2C_DIAG_CTRL_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_MAIN_I2C_DIAG_CTRL_RD_WRn___SHIFT          	3
#define RUDRA40_MAIN_I2C_DIAG_CTRL_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_MAIN_I2C_DIAG_CTRL_ABORT___SHIFT           	2
#define RUDRA40_MAIN_I2C_DIAG_CTRL_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_MAIN_I2C_DIAG_CTRL_FINISH___SHIFT          	1
#define RUDRA40_MAIN_I2C_DIAG_CTRL_START___MASK            	UINT32_C(0x1)
#define RUDRA40_MAIN_I2C_DIAG_CTRL_START___SHIFT           	0
#define RUDRA40_MAIN_I2C_DIAG_CTRL____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MAIN_I2C_DIAG_4X ---- */
#define RUDRA40_MAIN_I2C_DIAG_4X____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_4X____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_4X_Unused_9___MASK 	UINT32_C(0xfffffe00)
#define RUDRA40_MAIN_I2C_DIAG_4X_Unused_9___SHIFT	9
#define RUDRA40_MAIN_I2C_DIAG_4X_CLK_DIV___MASK  	UINT32_C(0x1ff)
#define RUDRA40_MAIN_I2C_DIAG_4X_CLK_DIV___SHIFT 	0
#define RUDRA40_MAIN_I2C_DIAG_4X____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MAIN_I2C_DIAG_DEVADDR ---- */
#define RUDRA40_MAIN_I2C_DIAG_DEVADDR____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DEVADDR____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DEVADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATAADDR ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATAADDR____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATAADDR____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATAADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_VLD ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_VLD____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_VLD____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_VLD____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MAIN_I2C_DIAG_DEBUG ---- */
#define RUDRA40_MAIN_I2C_DIAG_DEBUG____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DEBUG____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DEBUG_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_MAIN_I2C_DIAG_DEBUG_Unused_9___SHIFT 	9
#define RUDRA40_MAIN_I2C_DIAG_DEBUG_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_MAIN_I2C_DIAG_DEBUG_ABORT_LOC___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DEBUG____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR0 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR0____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR0_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR1 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR1____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR1_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR2 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR2____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR2_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR3 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR3____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR3_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR4 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR4____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR4_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR5 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR5____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR5_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR6 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR6____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR6____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR6_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR6_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR6____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR7 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR7____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR7____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR7_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR7_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR7____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR8 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR8____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR8____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR8_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR8_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR8____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR9 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR9____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR9____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR9_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR9_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR9____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR10 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR10____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR10____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR10_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR10_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR10____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR11 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR11____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR11____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR11_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR11_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR11____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR12 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR12____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR12____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR12_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR12_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR12____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR13 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR13____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR13____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR13_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR13_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR13____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR14 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR14____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR14____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR14_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR14_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR14____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR15 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR15____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR15____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR15_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR15_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR15____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR16 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR16____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR16____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR16_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR16_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR16____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR17 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR17____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR17____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR17_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR17_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR17____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR18 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR18____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR18____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR18_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR18_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR18____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR19 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR19____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR19____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR19_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR19_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR19____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR20 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR20____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR20____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR20_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR20_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR20____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR21 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR21____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR21____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR21_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR21_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR21____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR22 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR22____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR22____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR22_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR22_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR22____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR23 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR23____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR23____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR23_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR23_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR23____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR24 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR24____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR24____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR24_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR24_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR24____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR25 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR25____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR25____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR25_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR25_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR25____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR26 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR26____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR26____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR26_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR26_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR26____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR27 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR27____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR27____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR27_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR27_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR27____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR28 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR28____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR28____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR28_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR28_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR28____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR29 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR29____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR29____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR29_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR29_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR29____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR30 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR30____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR30____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR30_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR30_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR30____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR31 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR31____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR31____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR31_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR31_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR31____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR32 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR32____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR32____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR32_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR32_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR32____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR33 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR33____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR33____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR33_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR33_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR33____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR34 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR34____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR34____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR34_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR34_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR34____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR35 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR35____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR35____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR35_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR35_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR35____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR36 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR36____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR36____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR36_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR36_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR36____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR37 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR37____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR37____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR37_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR37_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR37____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR38 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR38____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR38____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR38_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR38_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR38____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR39 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR39____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR39____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR39_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR39_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR39____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR40 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR40____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR40____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR40_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR40_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR40____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR41 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR41____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR41____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR41_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR41_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR41____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR42 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR42____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR42____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR42_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR42_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR42____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR43 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR43____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR43____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR43_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR43_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR43____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR44 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR44____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR44____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR44_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR44_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR44____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR45 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR45____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR45____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR45_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR45_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR45____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR46 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR46____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR46____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR46_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR46_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR46____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR47 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR47____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR47____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR47_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR47_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR47____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR48 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR48____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR48____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR48_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR48_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR48____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR49 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR49____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR49____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR49_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR49_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR49____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR50 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR50____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR50____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR50_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR50_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR50____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR51 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR51____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR51____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR51_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR51_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR51____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR52 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR52____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR52____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR52_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR52_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR52____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR53 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR53____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR53____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR53_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR53_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR53____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR54 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR54____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR54____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR54_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR54_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR54____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR55 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR55____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR55____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR55_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR55_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR55____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR56 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR56____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR56____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR56_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR56_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR56____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR57 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR57____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR57____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR57_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR57_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR57____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR58 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR58____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR58____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR58_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR58_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR58____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR59 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR59____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR59____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR59_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR59_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR59____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR60 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR60____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR60____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR60_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR60_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR60____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR61 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR61____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR61____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR61_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR61_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR61____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR62 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR62____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR62____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR62_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR62_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR62____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_WR63 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR63____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR63____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_WR63_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR63_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_WR63____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD0 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD0____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD0____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD0_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD1 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD1____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD1____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD1_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD2 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD2____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD2____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD2_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD3 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD3____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD3____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD3_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD4 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD4____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD4____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD4_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD5 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD5____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD5____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD5_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD6 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD6____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD6____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD6_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD6_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD6____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD7 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD7____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD7____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD7_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD7_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD7____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD8 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD8____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD8____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD8_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD8_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD8____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD9 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD9____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD9____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD9_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD9_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD9____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD10 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD10____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD10____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD10_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD10_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD10____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD11 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD11____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD11____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD11_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD11_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD11____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD12 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD12____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD12____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD12_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD12_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD12____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD13 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD13____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD13____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD13_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD13_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD13____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD14 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD14____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD14____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD14_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD14_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD14____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD15 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD15____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD15____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD15_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD15_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD15____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD16 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD16____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD16____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD16_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD16_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD16____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD17 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD17____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD17____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD17_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD17_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD17____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD18 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD18____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD18____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD18_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD18_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD18____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD19 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD19____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD19____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD19_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD19_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD19____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD20 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD20____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD20____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD20_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD20_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD20____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD21 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD21____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD21____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD21_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD21_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD21____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD22 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD22____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD22____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD22_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD22_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD22____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD23 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD23____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD23____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD23_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD23_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD23____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD24 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD24____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD24____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD24_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD24_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD24____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD25 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD25____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD25____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD25_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD25_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD25____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD26 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD26____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD26____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD26_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD26_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD26____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD27 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD27____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD27____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD27_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD27_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD27____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD28 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD28____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD28____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD28_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD28_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD28____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD29 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD29____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD29____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD29_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD29_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD29____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD30 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD30____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD30____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD30_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD30_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD30____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD31 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD31____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD31____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD31_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD31_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD31____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD32 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD32____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD32____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD32_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD32_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD32____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD33 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD33____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD33____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD33_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD33_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD33____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD34 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD34____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD34____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD34_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD34_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD34____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD35 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD35____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD35____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD35_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD35_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD35____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD36 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD36____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD36____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD36_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD36_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD36____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD37 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD37____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD37____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD37_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD37_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD37____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD38 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD38____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD38____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD38_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD38_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD38____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD39 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD39____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD39____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD39_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD39_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD39____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD40 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD40____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD40____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD40_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD40_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD40____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD41 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD41____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD41____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD41_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD41_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD41____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD42 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD42____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD42____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD42_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD42_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD42____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD43 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD43____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD43____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD43_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD43_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD43____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD44 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD44____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD44____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD44_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD44_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD44____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD45 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD45____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD45____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD45_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD45_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD45____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD46 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD46____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD46____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD46_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD46_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD46____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD47 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD47____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD47____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD47_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD47_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD47____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD48 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD48____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD48____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD48_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD48_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD48____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD49 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD49____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD49____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD49_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD49_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD49____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD50 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD50____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD50____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD50_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD50_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD50____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD51 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD51____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD51____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD51_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD51_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD51____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD52 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD52____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD52____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD52_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD52_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD52____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD53 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD53____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD53____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD53_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD53_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD53____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD54 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD54____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD54____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD54_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD54_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD54____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD55 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD55____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD55____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD55_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD55_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD55____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD56 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD56____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD56____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD56_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD56_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD56____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD57 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD57____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD57____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD57_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD57_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD57____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD58 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD58____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD58____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD58_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD58_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD58____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD59 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD59____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD59____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD59_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD59_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD59____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD60 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD60____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD60____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD60_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD60_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD60____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD61 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD61____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD61____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD61_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD61_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD61____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD62 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD62____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD62____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD62_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD62_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD62____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_DIAG_DATA_RD63 ---- */
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD63____WIDTH	32
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD63____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_DIAG_DATA_RD63_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD63_DATA___SHIFT	0
#define RUDRA40_MAIN_I2C_DIAG_DATA_RD63____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MAIN_I2C_THEEND ---- */
#define RUDRA40_MAIN_I2C_THEEND____WIDTH	32
#define RUDRA40_MAIN_I2C_THEEND____TYPE 	uint32_t

#define RUDRA40_MAIN_I2C_THEEND____REGMASK	UINT32_C(0)

/* ---- RUDRA40_RJ45_UART_DATA ---- */
#define RUDRA40_RJ45_UART_DATA____WIDTH	32
#define RUDRA40_RJ45_UART_DATA____TYPE 	uint32_t

#define RUDRA40_RJ45_UART_DATA____REGMASK	UINT32_C(0)

/* ---- RUDRA40_RJ45_UART_CTRL ---- */
#define RUDRA40_RJ45_UART_CTRL____WIDTH	32
#define RUDRA40_RJ45_UART_CTRL____TYPE 	uint32_t

#define RUDRA40_RJ45_UART_CTRL_Unused_17___MASK        	UINT32_C(0xfffe0000)
#define RUDRA40_RJ45_UART_CTRL_Unused_17___SHIFT       	17
#define RUDRA40_RJ45_UART_CTRL_LOOPBACK___MASK         	UINT32_C(0x10000)
#define RUDRA40_RJ45_UART_CTRL_LOOPBACK___SHIFT        	16
#define RUDRA40_RJ45_UART_CTRL_RESET_UART___MASK       	UINT32_C(0x8000)
#define RUDRA40_RJ45_UART_CTRL_RESET_UART___SHIFT      	15
#define RUDRA40_RJ45_UART_CTRL_RX_STOP_ERR___MASK      	UINT32_C(0x4000)
#define RUDRA40_RJ45_UART_CTRL_RX_STOP_ERR___SHIFT     	14
#define RUDRA40_RJ45_UART_CTRL_RX_START_ERR___MASK     	UINT32_C(0x2000)
#define RUDRA40_RJ45_UART_CTRL_RX_START_ERR___SHIFT    	13
#define RUDRA40_RJ45_UART_CTRL_RX_FIFO_OVERFLOW___MASK 	UINT32_C(0x1000)
#define RUDRA40_RJ45_UART_CTRL_RX_FIFO_OVERFLOW___SHIFT	12
#define RUDRA40_RJ45_UART_CTRL_TX_FIFO_OVERFLOW___MASK 	UINT32_C(0x800)
#define RUDRA40_RJ45_UART_CTRL_TX_FIFO_OVERFLOW___SHIFT	11
#define RUDRA40_RJ45_UART_CTRL_PARITY_ERROR___MASK     	UINT32_C(0x400)
#define RUDRA40_RJ45_UART_CTRL_PARITY_ERROR___SHIFT    	10
#define RUDRA40_RJ45_UART_CTRL_WRITE_FIFO_EMPTY___MASK 	UINT32_C(0x200)
#define RUDRA40_RJ45_UART_CTRL_WRITE_FIFO_EMPTY___SHIFT	9
#define RUDRA40_RJ45_UART_CTRL_READ_FIFO_EMPTY___MASK  	UINT32_C(0x100)
#define RUDRA40_RJ45_UART_CTRL_READ_FIFO_EMPTY___SHIFT 	8
#define RUDRA40_RJ45_UART_CTRL_PARITY___MASK           	UINT32_C(0xc0)
#define RUDRA40_RJ45_UART_CTRL_PARITY___SHIFT          	6
#define RUDRA40_RJ45_UART_CTRL_TX_XTRA_STOP___MASK     	UINT32_C(0x20)
#define RUDRA40_RJ45_UART_CTRL_TX_XTRA_STOP___SHIFT    	5
#define RUDRA40_RJ45_UART_CTRL_RX_XTRA_STOP___MASK     	UINT32_C(0x10)
#define RUDRA40_RJ45_UART_CTRL_RX_XTRA_STOP___SHIFT    	4
#define RUDRA40_RJ45_UART_CTRL_SEL_SW___MASK           	UINT32_C(0x8)
#define RUDRA40_RJ45_UART_CTRL_SEL_SW___SHIFT          	3
#define RUDRA40_RJ45_UART_CTRL_BAUD_SEL___MASK         	UINT32_C(0x7)
#define RUDRA40_RJ45_UART_CTRL_BAUD_SEL___SHIFT        	0
#define RUDRA40_RJ45_UART_CTRL____REGMASK	UINT32_C(131071)

/* ---- RUDRA40_RJ45_UART_WRITE_FIFO_DEPTH ---- */
#define RUDRA40_RJ45_UART_WRITE_FIFO_DEPTH____WIDTH	32
#define RUDRA40_RJ45_UART_WRITE_FIFO_DEPTH____TYPE 	uint32_t

#define RUDRA40_RJ45_UART_WRITE_FIFO_DEPTH_Unused_6___MASK   	UINT32_C(0xffffffc0)
#define RUDRA40_RJ45_UART_WRITE_FIFO_DEPTH_Unused_6___SHIFT  	6
#define RUDRA40_RJ45_UART_WRITE_FIFO_DEPTH_fill_level___MASK 	UINT32_C(0x3f)
#define RUDRA40_RJ45_UART_WRITE_FIFO_DEPTH_fill_level___SHIFT	0
#define RUDRA40_RJ45_UART_WRITE_FIFO_DEPTH____REGMASK	UINT32_C(63)

/* ---- RUDRA40_RJ45_UART_READ_FIFO_DEPTH ---- */
#define RUDRA40_RJ45_UART_READ_FIFO_DEPTH____WIDTH	32
#define RUDRA40_RJ45_UART_READ_FIFO_DEPTH____TYPE 	uint32_t

#define RUDRA40_RJ45_UART_READ_FIFO_DEPTH_Unused_6___MASK   	UINT32_C(0xffffffc0)
#define RUDRA40_RJ45_UART_READ_FIFO_DEPTH_Unused_6___SHIFT  	6
#define RUDRA40_RJ45_UART_READ_FIFO_DEPTH_fill_level___MASK 	UINT32_C(0x3f)
#define RUDRA40_RJ45_UART_READ_FIFO_DEPTH_fill_level___SHIFT	0
#define RUDRA40_RJ45_UART_READ_FIFO_DEPTH____REGMASK	UINT32_C(63)

/* ---- RUDRA40_FREE_RUN_TOD_RTC ---- */
#define RUDRA40_FREE_RUN_TOD_RTC____WIDTH	32
#define RUDRA40_FREE_RUN_TOD_RTC____TYPE 	uint32_t

#define RUDRA40_FREE_RUN_TOD_RTC____REGMASK	UINT32_C(0)

/* ---- RUDRA40_FREE_RUN_TOD_RTC_OFST3 ---- */
#define RUDRA40_FREE_RUN_TOD_RTC_OFST3____WIDTH	32
#define RUDRA40_FREE_RUN_TOD_RTC_OFST3____TYPE 	uint32_t

#define RUDRA40_FREE_RUN_TOD_RTC_OFST3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_FREE_RUN_TOD_RTC_OFST1 ---- */
#define RUDRA40_FREE_RUN_TOD_RTC_OFST1____WIDTH	32
#define RUDRA40_FREE_RUN_TOD_RTC_OFST1____TYPE 	uint32_t

#define RUDRA40_FREE_RUN_TOD_RTC_OFST1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_FREE_RUN_TOD_RTC_OFST_CNT ---- */
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CNT____WIDTH	32
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CNT____TYPE 	uint32_t

#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CNT_Unused_20___MASK    	UINT32_C(0xfff00000)
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CNT_Unused_20___SHIFT   	20
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CNT_rtc_ofst_cnt___MASK 	UINT32_C(0xfffff)
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CNT_rtc_ofst_cnt___SHIFT	0
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CNT____REGMASK	UINT32_C(1048575)

/* ---- RUDRA40_FREE_RUN_TOD_RTC_EPO_OFST ---- */
#define RUDRA40_FREE_RUN_TOD_RTC_EPO_OFST____WIDTH	32
#define RUDRA40_FREE_RUN_TOD_RTC_EPO_OFST____TYPE 	uint32_t

#define RUDRA40_FREE_RUN_TOD_RTC_EPO_OFST____REGMASK	UINT32_C(0)

/* ---- RUDRA40_FREE_RUN_TOD_RTC_DRFT ---- */
#define RUDRA40_FREE_RUN_TOD_RTC_DRFT____WIDTH	32
#define RUDRA40_FREE_RUN_TOD_RTC_DRFT____TYPE 	uint32_t

#define RUDRA40_FREE_RUN_TOD_RTC_DRFT_Unused_10___MASK     	UINT32_C(0xfffffc00)
#define RUDRA40_FREE_RUN_TOD_RTC_DRFT_Unused_10___SHIFT    	10
#define RUDRA40_FREE_RUN_TOD_RTC_DRFT_rtc_drft_sign___MASK 	UINT32_C(0x200)
#define RUDRA40_FREE_RUN_TOD_RTC_DRFT_rtc_drft_sign___SHIFT	9
#define RUDRA40_FREE_RUN_TOD_RTC_DRFT_rtc_drft___MASK      	UINT32_C(0x1ff)
#define RUDRA40_FREE_RUN_TOD_RTC_DRFT_rtc_drft___SHIFT     	0
#define RUDRA40_FREE_RUN_TOD_RTC_DRFT____REGMASK	UINT32_C(1023)

/* ---- RUDRA40_FREE_RUN_TOD_RTC_CTRL ---- */
#define RUDRA40_FREE_RUN_TOD_RTC_CTRL____WIDTH	32
#define RUDRA40_FREE_RUN_TOD_RTC_CTRL____TYPE 	uint32_t

#define RUDRA40_FREE_RUN_TOD_RTC_CTRL_Unused_2___MASK 	UINT32_C(0xfffffffc)
#define RUDRA40_FREE_RUN_TOD_RTC_CTRL_Unused_2___SHIFT	2
#define RUDRA40_FREE_RUN_TOD_RTC_CTRL_rtc_en___MASK   	UINT32_C(0x2)
#define RUDRA40_FREE_RUN_TOD_RTC_CTRL_rtc_en___SHIFT  	1
#define RUDRA40_FREE_RUN_TOD_RTC_CTRL_rtc_clr___MASK  	UINT32_C(0x1)
#define RUDRA40_FREE_RUN_TOD_RTC_CTRL_rtc_clr___SHIFT 	0
#define RUDRA40_FREE_RUN_TOD_RTC_CTRL____REGMASK	UINT32_C(3)

/* ---- RUDRA40_FREE_RUN_TOD_RTC_OFST_CTRL ---- */
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CTRL____WIDTH	32
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CTRL____TYPE 	uint32_t

#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CTRL_Unused_2___MASK      	UINT32_C(0xfffffffc)
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CTRL_Unused_2___SHIFT     	2
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CTRL_rtc_ofst_sign___MASK 	UINT32_C(0x2)
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CTRL_rtc_ofst_sign___SHIFT	1
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CTRL_rtc_ofst_go___MASK   	UINT32_C(0x1)
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CTRL_rtc_ofst_go___SHIFT  	0
#define RUDRA40_FREE_RUN_TOD_RTC_OFST_CTRL____REGMASK	UINT32_C(3)

/* ---- RUDRA40_SW_UART_DATA ---- */
#define RUDRA40_SW_UART_DATA____WIDTH	32
#define RUDRA40_SW_UART_DATA____TYPE 	uint32_t

#define RUDRA40_SW_UART_DATA____REGMASK	UINT32_C(0)

/* ---- RUDRA40_SW_UART_CTRL ---- */
#define RUDRA40_SW_UART_CTRL____WIDTH	32
#define RUDRA40_SW_UART_CTRL____TYPE 	uint32_t

#define RUDRA40_SW_UART_CTRL_Unused_17___MASK        	UINT32_C(0xfffe0000)
#define RUDRA40_SW_UART_CTRL_Unused_17___SHIFT       	17
#define RUDRA40_SW_UART_CTRL_LOOPBACK___MASK         	UINT32_C(0x10000)
#define RUDRA40_SW_UART_CTRL_LOOPBACK___SHIFT        	16
#define RUDRA40_SW_UART_CTRL_RESET_UART___MASK       	UINT32_C(0x8000)
#define RUDRA40_SW_UART_CTRL_RESET_UART___SHIFT      	15
#define RUDRA40_SW_UART_CTRL_RX_STOP_ERR___MASK      	UINT32_C(0x4000)
#define RUDRA40_SW_UART_CTRL_RX_STOP_ERR___SHIFT     	14
#define RUDRA40_SW_UART_CTRL_RX_START_ERR___MASK     	UINT32_C(0x2000)
#define RUDRA40_SW_UART_CTRL_RX_START_ERR___SHIFT    	13
#define RUDRA40_SW_UART_CTRL_RX_FIFO_OVERFLOW___MASK 	UINT32_C(0x1000)
#define RUDRA40_SW_UART_CTRL_RX_FIFO_OVERFLOW___SHIFT	12
#define RUDRA40_SW_UART_CTRL_TX_FIFO_OVERFLOW___MASK 	UINT32_C(0x800)
#define RUDRA40_SW_UART_CTRL_TX_FIFO_OVERFLOW___SHIFT	11
#define RUDRA40_SW_UART_CTRL_PARITY_ERROR___MASK     	UINT32_C(0x400)
#define RUDRA40_SW_UART_CTRL_PARITY_ERROR___SHIFT    	10
#define RUDRA40_SW_UART_CTRL_WRITE_FIFO_EMPTY___MASK 	UINT32_C(0x200)
#define RUDRA40_SW_UART_CTRL_WRITE_FIFO_EMPTY___SHIFT	9
#define RUDRA40_SW_UART_CTRL_READ_FIFO_EMPTY___MASK  	UINT32_C(0x100)
#define RUDRA40_SW_UART_CTRL_READ_FIFO_EMPTY___SHIFT 	8
#define RUDRA40_SW_UART_CTRL_PARITY___MASK           	UINT32_C(0xc0)
#define RUDRA40_SW_UART_CTRL_PARITY___SHIFT          	6
#define RUDRA40_SW_UART_CTRL_TX_XTRA_STOP___MASK     	UINT32_C(0x20)
#define RUDRA40_SW_UART_CTRL_TX_XTRA_STOP___SHIFT    	5
#define RUDRA40_SW_UART_CTRL_RX_XTRA_STOP___MASK     	UINT32_C(0x10)
#define RUDRA40_SW_UART_CTRL_RX_XTRA_STOP___SHIFT    	4
#define RUDRA40_SW_UART_CTRL_SEL_SW___MASK           	UINT32_C(0x8)
#define RUDRA40_SW_UART_CTRL_SEL_SW___SHIFT          	3
#define RUDRA40_SW_UART_CTRL_BAUD_SEL___MASK         	UINT32_C(0x7)
#define RUDRA40_SW_UART_CTRL_BAUD_SEL___SHIFT        	0
#define RUDRA40_SW_UART_CTRL____REGMASK	UINT32_C(131071)

/* ---- RUDRA40_SW_UART_WRITE_FIFO_DEPTH ---- */
#define RUDRA40_SW_UART_WRITE_FIFO_DEPTH____WIDTH	32
#define RUDRA40_SW_UART_WRITE_FIFO_DEPTH____TYPE 	uint32_t

#define RUDRA40_SW_UART_WRITE_FIFO_DEPTH_Unused_6___MASK   	UINT32_C(0xffffffc0)
#define RUDRA40_SW_UART_WRITE_FIFO_DEPTH_Unused_6___SHIFT  	6
#define RUDRA40_SW_UART_WRITE_FIFO_DEPTH_fill_level___MASK 	UINT32_C(0x3f)
#define RUDRA40_SW_UART_WRITE_FIFO_DEPTH_fill_level___SHIFT	0
#define RUDRA40_SW_UART_WRITE_FIFO_DEPTH____REGMASK	UINT32_C(63)

/* ---- RUDRA40_SW_UART_READ_FIFO_DEPTH ---- */
#define RUDRA40_SW_UART_READ_FIFO_DEPTH____WIDTH	32
#define RUDRA40_SW_UART_READ_FIFO_DEPTH____TYPE 	uint32_t

#define RUDRA40_SW_UART_READ_FIFO_DEPTH_Unused_6___MASK   	UINT32_C(0xffffffc0)
#define RUDRA40_SW_UART_READ_FIFO_DEPTH_Unused_6___SHIFT  	6
#define RUDRA40_SW_UART_READ_FIFO_DEPTH_fill_level___MASK 	UINT32_C(0x3f)
#define RUDRA40_SW_UART_READ_FIFO_DEPTH_fill_level___SHIFT	0
#define RUDRA40_SW_UART_READ_FIFO_DEPTH____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MORE_I2C_RESERVED_1 ---- */
#define RUDRA40_MORE_I2C_RESERVED_1____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_1____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_2 ---- */
#define RUDRA40_MORE_I2C_RESERVED_2____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_2____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_2____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_3 ---- */
#define RUDRA40_MORE_I2C_RESERVED_3____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_3____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_3____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_4 ---- */
#define RUDRA40_MORE_I2C_RESERVED_4____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_4____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_4____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_5 ---- */
#define RUDRA40_MORE_I2C_RESERVED_5____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_5____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_5____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_6 ---- */
#define RUDRA40_MORE_I2C_RESERVED_6____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_6____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_6____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_7 ---- */
#define RUDRA40_MORE_I2C_RESERVED_7____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_7____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_7____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_8 ---- */
#define RUDRA40_MORE_I2C_RESERVED_8____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_8____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_8____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_9 ---- */
#define RUDRA40_MORE_I2C_RESERVED_9____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_9____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_9____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_10 ---- */
#define RUDRA40_MORE_I2C_RESERVED_10____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_10____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_10____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_11 ---- */
#define RUDRA40_MORE_I2C_RESERVED_11____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_11____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_11____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_12 ---- */
#define RUDRA40_MORE_I2C_RESERVED_12____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_12____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_12____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_13 ---- */
#define RUDRA40_MORE_I2C_RESERVED_13____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_13____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_13____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_14 ---- */
#define RUDRA40_MORE_I2C_RESERVED_14____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_14____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_14____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_15 ---- */
#define RUDRA40_MORE_I2C_RESERVED_15____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_15____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_15____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_RESERVED_16 ---- */
#define RUDRA40_MORE_I2C_RESERVED_16____WIDTH	32
#define RUDRA40_MORE_I2C_RESERVED_16____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_RESERVED_16____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_PMBUS1_SW_CTRL ---- */
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_Unused_6___SHIFT	6
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_READACK___SHIFT 	5
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_READNACK___SHIFT	4
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_WRITE___SHIFT   	3
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_STOP___SHIFT    	2
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_START___MASK    	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_START___SHIFT   	1
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL_RESET___SHIFT   	0
#define RUDRA40_MORE_I2C_PMBUS1_SW_CTRL____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MORE_I2C_PMBUS1_SW_STAT ---- */
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_STATE___SHIFT   	4
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_SDA___SHIFT     	3
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_SCL___SHIFT     	2
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_ACKR___SHIFT    	1
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT_STAT___SHIFT    	0
#define RUDRA40_MORE_I2C_PMBUS1_SW_STAT____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PMBUS1_SW_WRITE_DATA ---- */
#define RUDRA40_MORE_I2C_PMBUS1_SW_WRITE_DATA____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_SW_WRITE_DATA____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_SW_WRITE_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PMBUS1_SW_WRITE_DATA_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_PMBUS1_SW_WRITE_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MORE_I2C_PMBUS1_SW_WRITE_DATA_DATA___SHIFT    	0
#define RUDRA40_MORE_I2C_PMBUS1_SW_WRITE_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PMBUS1_SW_READ_DATA ---- */
#define RUDRA40_MORE_I2C_PMBUS1_SW_READ_DATA____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_SW_READ_DATA____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_SW_READ_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PMBUS1_SW_READ_DATA_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_PMBUS1_SW_READ_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MORE_I2C_PMBUS1_SW_READ_DATA_DATA___SHIFT    	0
#define RUDRA40_MORE_I2C_PMBUS1_SW_READ_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PMBUS1_SW_DONE ---- */
#define RUDRA40_MORE_I2C_PMBUS1_SW_DONE____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_SW_DONE____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_SW_DONE_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_PMBUS1_SW_DONE_Unused_1___SHIFT	1
#define RUDRA40_MORE_I2C_PMBUS1_SW_DONE_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS1_SW_DONE_CMP___SHIFT     	0
#define RUDRA40_MORE_I2C_PMBUS1_SW_DONE____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MORE_I2C_PMBUS1_SW_IF_SEL ---- */
#define RUDRA40_MORE_I2C_PMBUS1_SW_IF_SEL____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_SW_IF_SEL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_SW_IF_SEL_Unused_1___MASK          	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_PMBUS1_SW_IF_SEL_Unused_1___SHIFT         	1
#define RUDRA40_MORE_I2C_PMBUS1_SW_IF_SEL_pmbus1_sel_sw_i2c___MASK 	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS1_SW_IF_SEL_pmbus1_sel_sw_i2c___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS1_SW_IF_SEL____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MORE_I2C_PMBUS1_SW_MISC_CTRL ---- */
#define RUDRA40_MORE_I2C_PMBUS1_SW_MISC_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_SW_MISC_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_SW_MISC_CTRL_Unused_12___MASK              	UINT32_C(0xfffff000)
#define RUDRA40_MORE_I2C_PMBUS1_SW_MISC_CTRL_Unused_12___SHIFT             	12
#define RUDRA40_MORE_I2C_PMBUS1_SW_MISC_CTRL_pmbus1_i2c_half_period___MASK 	UINT32_C(0xfff)
#define RUDRA40_MORE_I2C_PMBUS1_SW_MISC_CTRL_pmbus1_i2c_half_period___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS1_SW_MISC_CTRL____REGMASK	UINT32_C(4095)

/* ---- RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL ---- */
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_Unused_8___SHIFT        	8
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_RESET_MASTER___SHIFT    	7
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_Unused_6___MASK         	UINT32_C(0x40)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_Unused_6___SHIFT        	6
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_RD_WRn___SHIFT          	3
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_ABORT___SHIFT           	2
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_FINISH___SHIFT          	1
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_START___MASK            	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL_START___SHIFT           	0
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_CTRL____REGMASK	UINT32_C(191)

/* ---- RUDRA40_MORE_I2C_PMBUS1_DIAG_4X ---- */
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_4X____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_4X____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_DIAG_4X_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_4X_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_4X_CLK_DIV___MASK  	UINT32_C(0xff)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_4X_CLK_DIV___SHIFT 	0
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_4X____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PMBUS1_DIAG_DEVADDR ---- */
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DEVADDR____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DEVADDR____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DEVADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_PMBUS1_DIAG_DATAADDR ---- */
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATAADDR____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATAADDR____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATAADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_VLD ---- */
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_VLD____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_VLD____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_VLD____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_PMBUS1_DIAG_DEBUG ---- */
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DEBUG____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DEBUG____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DEBUG_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DEBUG_Unused_9___SHIFT 	9
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DEBUG_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DEBUG_ABORT_LOC___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DEBUG____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_WR ---- */
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_WR____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_WR____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_WR_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_WR_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_WR____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_RD ---- */
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_RD____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_RD____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_RD_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_RD_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS1_DIAG_DATA_RD____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS1_I2C_SW_OVERRIDE ---- */
#define RUDRA40_MORE_I2C_PMBUS1_I2C_SW_OVERRIDE____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS1_I2C_SW_OVERRIDE____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS1_I2C_SW_OVERRIDE_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_PMBUS1_I2C_SW_OVERRIDE_Unused_1___SHIFT      	1
#define RUDRA40_MORE_I2C_PMBUS1_I2C_SW_OVERRIDE_sw_override_en___MASK 	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS1_I2C_SW_OVERRIDE_sw_override_en___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS1_I2C_SW_OVERRIDE____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MORE_I2C_PMBUS2_SW_CTRL ---- */
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_Unused_6___SHIFT	6
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_READACK___SHIFT 	5
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_READNACK___SHIFT	4
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_WRITE___SHIFT   	3
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_STOP___SHIFT    	2
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_START___MASK    	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_START___SHIFT   	1
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL_RESET___SHIFT   	0
#define RUDRA40_MORE_I2C_PMBUS2_SW_CTRL____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MORE_I2C_PMBUS2_SW_STAT ---- */
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_STATE___SHIFT   	4
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_SDA___SHIFT     	3
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_SCL___SHIFT     	2
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_ACKR___SHIFT    	1
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT_STAT___SHIFT    	0
#define RUDRA40_MORE_I2C_PMBUS2_SW_STAT____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PMBUS2_SW_WRITE_DATA ---- */
#define RUDRA40_MORE_I2C_PMBUS2_SW_WRITE_DATA____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_SW_WRITE_DATA____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_SW_WRITE_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PMBUS2_SW_WRITE_DATA_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_PMBUS2_SW_WRITE_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MORE_I2C_PMBUS2_SW_WRITE_DATA_DATA___SHIFT    	0
#define RUDRA40_MORE_I2C_PMBUS2_SW_WRITE_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PMBUS2_SW_READ_DATA ---- */
#define RUDRA40_MORE_I2C_PMBUS2_SW_READ_DATA____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_SW_READ_DATA____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_SW_READ_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PMBUS2_SW_READ_DATA_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_PMBUS2_SW_READ_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MORE_I2C_PMBUS2_SW_READ_DATA_DATA___SHIFT    	0
#define RUDRA40_MORE_I2C_PMBUS2_SW_READ_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PMBUS2_SW_DONE ---- */
#define RUDRA40_MORE_I2C_PMBUS2_SW_DONE____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_SW_DONE____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_SW_DONE_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_PMBUS2_SW_DONE_Unused_1___SHIFT	1
#define RUDRA40_MORE_I2C_PMBUS2_SW_DONE_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS2_SW_DONE_CMP___SHIFT     	0
#define RUDRA40_MORE_I2C_PMBUS2_SW_DONE____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MORE_I2C_PMBUS2_SW_IF_SEL ---- */
#define RUDRA40_MORE_I2C_PMBUS2_SW_IF_SEL____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_SW_IF_SEL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_SW_IF_SEL_Unused_1___MASK          	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_PMBUS2_SW_IF_SEL_Unused_1___SHIFT         	1
#define RUDRA40_MORE_I2C_PMBUS2_SW_IF_SEL_pmbus1_sel_sw_i2c___MASK 	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS2_SW_IF_SEL_pmbus1_sel_sw_i2c___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_SW_IF_SEL____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MORE_I2C_PMBUS2_SW_MISC_CTRL ---- */
#define RUDRA40_MORE_I2C_PMBUS2_SW_MISC_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_SW_MISC_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_SW_MISC_CTRL_Unused_12___MASK              	UINT32_C(0xfffff000)
#define RUDRA40_MORE_I2C_PMBUS2_SW_MISC_CTRL_Unused_12___SHIFT             	12
#define RUDRA40_MORE_I2C_PMBUS2_SW_MISC_CTRL_pmbus1_i2c_half_period___MASK 	UINT32_C(0xfff)
#define RUDRA40_MORE_I2C_PMBUS2_SW_MISC_CTRL_pmbus1_i2c_half_period___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_SW_MISC_CTRL____REGMASK	UINT32_C(4095)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_Unused_8___SHIFT        	8
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_RESET_MASTER___SHIFT    	7
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_SLAVE_ADDRS_NACK___MASK 	UINT32_C(0x40)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_SLAVE_ADDRS_NACK___SHIFT	6
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_RD_WRn___SHIFT          	3
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_ABORT___SHIFT           	2
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_FINISH___SHIFT          	1
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_START___MASK            	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL_START___SHIFT           	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_CTRL____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_4X ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_4X____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_4X____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_4X_Unused_9___MASK 	UINT32_C(0xfffffe00)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_4X_Unused_9___SHIFT	9
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_4X_CLK_DIV___MASK  	UINT32_C(0x1ff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_4X_CLK_DIV___SHIFT 	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_4X____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DEVADDR ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DEVADDR____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DEVADDR____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DEVADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATAADDR ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATAADDR____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATAADDR____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATAADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_VLD ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_VLD____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_VLD____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_VLD____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DEBUG ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DEBUG____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DEBUG____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DEBUG_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DEBUG_Unused_9___SHIFT 	9
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DEBUG_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DEBUG_ABORT_LOC___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DEBUG____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR0 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR0____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR0____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR0_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR1 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR1____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR1____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR1_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR2 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR2____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR2____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR2_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR3 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR3____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR3____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR3_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR4 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR4____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR4____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR4_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR5 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR5____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR5____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR5_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR6 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR6____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR6____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR6_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR6_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR6____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR7 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR7____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR7____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR7_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR7_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR7____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR8 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR8____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR8____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR8_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR8_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR8____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR9 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR9____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR9____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR9_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR9_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR9____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR10 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR10____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR10____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR10_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR10_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR10____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR11 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR11____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR11____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR11_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR11_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR11____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR12 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR12____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR12____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR12_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR12_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR12____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR13 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR13____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR13____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR13_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR13_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR13____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR14 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR14____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR14____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR14_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR14_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR14____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR15 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR15____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR15____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR15_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR15_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR15____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR16 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR16____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR16____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR16_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR16_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR16____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR17 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR17____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR17____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR17_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR17_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR17____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR18 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR18____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR18____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR18_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR18_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR18____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR19 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR19____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR19____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR19_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR19_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR19____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR20 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR20____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR20____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR20_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR20_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR20____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR21 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR21____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR21____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR21_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR21_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR21____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR22 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR22____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR22____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR22_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR22_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR22____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR23 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR23____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR23____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR23_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR23_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR23____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR24 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR24____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR24____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR24_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR24_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR24____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR25 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR25____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR25____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR25_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR25_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR25____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR26 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR26____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR26____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR26_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR26_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR26____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR27 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR27____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR27____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR27_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR27_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR27____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR28 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR28____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR28____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR28_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR28_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR28____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR29 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR29____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR29____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR29_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR29_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR29____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR30 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR30____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR30____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR30_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR30_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR30____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR31 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR31____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR31____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR31_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR31_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR31____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR32 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR32____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR32____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR32_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR32_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR32____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR33 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR33____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR33____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR33_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR33_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR33____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR34 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR34____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR34____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR34_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR34_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR34____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR35 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR35____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR35____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR35_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR35_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR35____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR36 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR36____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR36____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR36_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR36_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR36____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR37 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR37____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR37____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR37_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR37_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR37____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR38 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR38____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR38____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR38_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR38_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR38____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR39 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR39____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR39____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR39_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR39_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR39____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR40 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR40____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR40____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR40_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR40_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR40____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR41 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR41____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR41____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR41_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR41_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR41____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR42 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR42____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR42____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR42_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR42_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR42____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR43 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR43____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR43____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR43_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR43_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR43____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR44 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR44____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR44____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR44_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR44_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR44____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR45 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR45____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR45____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR45_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR45_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR45____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR46 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR46____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR46____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR46_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR46_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR46____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR47 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR47____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR47____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR47_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR47_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR47____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR48 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR48____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR48____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR48_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR48_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR48____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR49 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR49____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR49____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR49_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR49_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR49____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR50 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR50____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR50____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR50_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR50_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR50____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR51 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR51____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR51____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR51_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR51_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR51____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR52 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR52____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR52____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR52_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR52_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR52____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR53 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR53____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR53____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR53_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR53_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR53____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR54 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR54____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR54____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR54_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR54_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR54____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR55 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR55____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR55____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR55_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR55_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR55____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR56 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR56____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR56____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR56_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR56_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR56____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR57 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR57____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR57____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR57_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR57_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR57____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR58 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR58____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR58____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR58_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR58_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR58____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR59 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR59____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR59____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR59_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR59_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR59____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR60 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR60____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR60____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR60_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR60_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR60____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR61 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR61____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR61____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR61_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR61_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR61____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR62 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR62____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR62____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR62_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR62_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR62____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR63 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR63____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR63____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR63_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR63_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_WR63____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD0 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD0____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD0____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD0_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD1 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD1____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD1____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD1_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD2 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD2____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD2____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD2_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD3 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD3____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD3____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD3_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD4 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD4____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD4____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD4_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD5 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD5____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD5____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD5_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD6 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD6____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD6____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD6_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD6_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD6____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD7 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD7____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD7____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD7_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD7_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD7____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD8 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD8____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD8____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD8_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD8_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD8____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD9 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD9____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD9____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD9_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD9_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD9____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD10 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD10____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD10____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD10_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD10_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD10____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD11 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD11____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD11____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD11_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD11_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD11____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD12 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD12____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD12____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD12_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD12_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD12____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD13 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD13____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD13____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD13_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD13_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD13____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD14 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD14____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD14____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD14_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD14_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD14____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD15 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD15____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD15____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD15_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD15_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD15____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD16 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD16____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD16____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD16_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD16_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD16____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD17 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD17____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD17____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD17_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD17_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD17____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD18 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD18____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD18____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD18_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD18_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD18____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD19 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD19____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD19____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD19_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD19_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD19____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD20 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD20____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD20____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD20_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD20_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD20____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD21 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD21____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD21____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD21_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD21_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD21____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD22 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD22____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD22____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD22_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD22_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD22____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD23 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD23____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD23____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD23_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD23_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD23____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD24 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD24____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD24____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD24_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD24_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD24____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD25 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD25____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD25____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD25_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD25_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD25____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD26 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD26____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD26____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD26_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD26_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD26____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD27 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD27____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD27____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD27_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD27_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD27____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD28 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD28____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD28____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD28_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD28_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD28____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD29 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD29____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD29____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD29_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD29_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD29____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD30 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD30____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD30____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD30_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD30_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD30____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD31 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD31____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD31____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD31_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD31_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD31____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD32 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD32____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD32____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD32_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD32_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD32____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD33 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD33____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD33____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD33_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD33_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD33____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD34 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD34____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD34____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD34_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD34_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD34____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD35 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD35____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD35____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD35_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD35_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD35____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD36 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD36____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD36____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD36_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD36_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD36____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD37 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD37____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD37____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD37_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD37_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD37____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD38 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD38____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD38____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD38_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD38_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD38____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD39 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD39____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD39____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD39_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD39_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD39____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD40 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD40____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD40____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD40_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD40_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD40____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD41 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD41____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD41____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD41_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD41_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD41____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD42 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD42____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD42____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD42_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD42_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD42____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD43 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD43____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD43____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD43_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD43_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD43____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD44 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD44____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD44____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD44_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD44_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD44____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD45 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD45____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD45____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD45_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD45_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD45____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD46 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD46____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD46____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD46_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD46_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD46____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD47 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD47____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD47____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD47_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD47_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD47____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD48 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD48____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD48____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD48_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD48_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD48____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD49 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD49____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD49____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD49_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD49_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD49____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD50 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD50____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD50____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD50_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD50_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD50____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD51 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD51____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD51____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD51_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD51_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD51____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD52 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD52____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD52____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD52_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD52_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD52____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD53 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD53____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD53____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD53_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD53_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD53____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD54 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD54____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD54____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD54_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD54_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD54____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD55 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD55____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD55____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD55_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD55_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD55____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD56 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD56____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD56____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD56_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD56_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD56____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD57 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD57____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD57____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD57_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD57_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD57____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD58 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD58____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD58____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD58_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD58_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD58____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD59 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD59____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD59____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD59_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD59_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD59____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD60 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD60____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD60____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD60_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD60_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD60____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD61 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD61____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD61____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD61_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD61_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD61____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD62 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD62____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD62____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD62_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD62_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD62____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD63 ---- */
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD63____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD63____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD63_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD63_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_DIAG_DATA_RD63____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PMBUS2_I2C_SW_OVERRIDE ---- */
#define RUDRA40_MORE_I2C_PMBUS2_I2C_SW_OVERRIDE____WIDTH	32
#define RUDRA40_MORE_I2C_PMBUS2_I2C_SW_OVERRIDE____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PMBUS2_I2C_SW_OVERRIDE_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_PMBUS2_I2C_SW_OVERRIDE_Unused_1___SHIFT      	1
#define RUDRA40_MORE_I2C_PMBUS2_I2C_SW_OVERRIDE_sw_override_en___MASK 	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PMBUS2_I2C_SW_OVERRIDE_sw_override_en___SHIFT	0
#define RUDRA40_MORE_I2C_PMBUS2_I2C_SW_OVERRIDE____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_Unused_6___SHIFT	6
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_READACK___SHIFT 	5
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_READNACK___SHIFT	4
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_WRITE___SHIFT   	3
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_STOP___SHIFT    	2
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_START___MASK    	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_START___SHIFT   	1
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL_RESET___SHIFT   	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_STATE___SHIFT   	4
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_SDA___SHIFT     	3
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_SCL___SHIFT     	2
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_ACKR___SHIFT    	1
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT_STAT___SHIFT    	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_STAT____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_SW_WRITE_DATA ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_WRITE_DATA____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_WRITE_DATA____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_WRITE_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_WRITE_DATA_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_WRITE_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_WRITE_DATA_DATA___SHIFT    	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_WRITE_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_SW_READ_DATA ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_READ_DATA____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_READ_DATA____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_READ_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_READ_DATA_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_READ_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_READ_DATA_DATA___SHIFT    	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_READ_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_SW_DONE ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_DONE____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_DONE____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_DONE_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_DONE_Unused_1___SHIFT	1
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_DONE_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_DONE_CMP___SHIFT     	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_DONE____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_SW_IF_SEL ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_IF_SEL____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_IF_SEL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_IF_SEL_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_IF_SEL_Unused_1___SHIFT      	1
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_IF_SEL_j2c_sel_sw_i2c___MASK 	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_IF_SEL_j2c_sel_sw_i2c___SHIFT	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_IF_SEL____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_SW_MISC_CTRL ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_MISC_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_MISC_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_MISC_CTRL_Unused_12___MASK              	UINT32_C(0xfffff000)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_MISC_CTRL_Unused_12___SHIFT             	12
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_MISC_CTRL_pmbus1_i2c_half_period___MASK 	UINT32_C(0xfff)
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_MISC_CTRL_pmbus1_i2c_half_period___SHIFT	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_SW_MISC_CTRL____REGMASK	UINT32_C(4095)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_Unused_8___SHIFT        	8
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_RESET_MASTER___SHIFT    	7
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_Unused_6___MASK         	UINT32_C(0x40)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_Unused_6___SHIFT        	6
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_RD_WRn___SHIFT          	3
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_ABORT___SHIFT           	2
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_FINISH___SHIFT          	1
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_START___MASK            	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL_START___SHIFT           	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_CTRL____REGMASK	UINT32_C(191)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_4X ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_4X____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_4X____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_4X_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_4X_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_4X_CLK_DIV___MASK  	UINT32_C(0xff)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_4X_CLK_DIV___SHIFT 	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_4X____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEVADDR ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEVADDR____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEVADDR____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEVADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATAADDR ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATAADDR____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATAADDR____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATAADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_VLD ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_VLD____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_VLD____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_VLD____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEBUG ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEBUG____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEBUG____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEBUG_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEBUG_Unused_9___SHIFT 	9
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEBUG_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEBUG_ABORT_LOC___SHIFT	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DEBUG____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_WR ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_WR____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_WR____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_WR_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_WR_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_WR____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_RD ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_RD____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_RD____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_RD_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_RD_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_DIAG_DATA_RD____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_J2C_IOEXP_I2C_SW_OVERRIDE ---- */
#define RUDRA40_MORE_I2C_J2C_IOEXP_I2C_SW_OVERRIDE____WIDTH	32
#define RUDRA40_MORE_I2C_J2C_IOEXP_I2C_SW_OVERRIDE____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_J2C_IOEXP_I2C_SW_OVERRIDE_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_J2C_IOEXP_I2C_SW_OVERRIDE_Unused_1___SHIFT      	1
#define RUDRA40_MORE_I2C_J2C_IOEXP_I2C_SW_OVERRIDE_sw_override_en___MASK 	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_J2C_IOEXP_I2C_SW_OVERRIDE_sw_override_en___SHIFT	0
#define RUDRA40_MORE_I2C_J2C_IOEXP_I2C_SW_OVERRIDE____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MORE_I2C_POLLING_DLY ---- */
#define RUDRA40_MORE_I2C_POLLING_DLY____WIDTH	32
#define RUDRA40_MORE_I2C_POLLING_DLY____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_POLLING_DLY_Unused_14___MASK   	UINT32_C(0xffffc000)
#define RUDRA40_MORE_I2C_POLLING_DLY_Unused_14___SHIFT  	14
#define RUDRA40_MORE_I2C_POLLING_DLY_timer_delay___MASK 	UINT32_C(0x3fff)
#define RUDRA40_MORE_I2C_POLLING_DLY_timer_delay___SHIFT	0
#define RUDRA40_MORE_I2C_POLLING_DLY____REGMASK	UINT32_C(16383)

/* ---- RUDRA40_MORE_I2C_PWRGD_SW_CTRL ---- */
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_Unused_6___SHIFT	6
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_READACK___SHIFT 	5
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_READNACK___SHIFT	4
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_WRITE___SHIFT   	3
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_STOP___SHIFT    	2
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_START___MASK    	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_START___SHIFT   	1
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL_RESET___SHIFT   	0
#define RUDRA40_MORE_I2C_PWRGD_SW_CTRL____REGMASK	UINT32_C(63)

/* ---- RUDRA40_MORE_I2C_PWRGD_SW_STAT ---- */
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_STATE___SHIFT   	4
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_SDA___SHIFT     	3
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_SCL___SHIFT     	2
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_ACKR___SHIFT    	1
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT_STAT___SHIFT    	0
#define RUDRA40_MORE_I2C_PWRGD_SW_STAT____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PWRGD_SW_WRITE_DATA ---- */
#define RUDRA40_MORE_I2C_PWRGD_SW_WRITE_DATA____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_SW_WRITE_DATA____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_SW_WRITE_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PWRGD_SW_WRITE_DATA_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_PWRGD_SW_WRITE_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MORE_I2C_PWRGD_SW_WRITE_DATA_DATA___SHIFT    	0
#define RUDRA40_MORE_I2C_PWRGD_SW_WRITE_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PWRGD_SW_READ_DATA ---- */
#define RUDRA40_MORE_I2C_PWRGD_SW_READ_DATA____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_SW_READ_DATA____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_SW_READ_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PWRGD_SW_READ_DATA_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_PWRGD_SW_READ_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_MORE_I2C_PWRGD_SW_READ_DATA_DATA___SHIFT    	0
#define RUDRA40_MORE_I2C_PWRGD_SW_READ_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PWRGD_SW_DONE ---- */
#define RUDRA40_MORE_I2C_PWRGD_SW_DONE____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_SW_DONE____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_SW_DONE_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_PWRGD_SW_DONE_Unused_1___SHIFT	1
#define RUDRA40_MORE_I2C_PWRGD_SW_DONE_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PWRGD_SW_DONE_CMP___SHIFT     	0
#define RUDRA40_MORE_I2C_PWRGD_SW_DONE____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MORE_I2C_PWRGD_SW_IF_SEL ---- */
#define RUDRA40_MORE_I2C_PWRGD_SW_IF_SEL____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_SW_IF_SEL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_SW_IF_SEL_Unused_1___MASK         	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_PWRGD_SW_IF_SEL_Unused_1___SHIFT        	1
#define RUDRA40_MORE_I2C_PWRGD_SW_IF_SEL_pwrgd_sel_sw_i2c___MASK 	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PWRGD_SW_IF_SEL_pwrgd_sel_sw_i2c___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_SW_IF_SEL____REGMASK	UINT32_C(1)

/* ---- RUDRA40_MORE_I2C_PWRGD_SW_MISC_CTRL ---- */
#define RUDRA40_MORE_I2C_PWRGD_SW_MISC_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_SW_MISC_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_SW_MISC_CTRL_Unused_12___MASK             	UINT32_C(0xfffff000)
#define RUDRA40_MORE_I2C_PWRGD_SW_MISC_CTRL_Unused_12___SHIFT            	12
#define RUDRA40_MORE_I2C_PWRGD_SW_MISC_CTRL_pwrgd_i2c_half_period___MASK 	UINT32_C(0xfff)
#define RUDRA40_MORE_I2C_PWRGD_SW_MISC_CTRL_pwrgd_i2c_half_period___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_SW_MISC_CTRL____REGMASK	UINT32_C(4095)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_Unused_8___SHIFT        	8
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_RESET_MASTER___SHIFT    	7
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_Unused_6___MASK         	UINT32_C(0x40)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_Unused_6___SHIFT        	6
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_RD_WRn___SHIFT          	3
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_ABORT___SHIFT           	2
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_FINISH___SHIFT          	1
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_START___MASK            	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL_START___SHIFT           	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_CTRL____REGMASK	UINT32_C(191)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_4X ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_4X____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_4X____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_4X_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_4X_Unused_8___SHIFT	8
#define RUDRA40_MORE_I2C_PWRGD_DIAG_4X_CLK_DIV___MASK  	UINT32_C(0xff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_4X_CLK_DIV___SHIFT 	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_4X____REGMASK	UINT32_C(255)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DEVADDR ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DEVADDR____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DEVADDR____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DEVADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATAADDR ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATAADDR____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATAADDR____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATAADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_VLD ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_VLD____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_VLD____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_VLD____REGMASK	UINT32_C(0)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DEBUG ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DEBUG____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DEBUG____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DEBUG_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DEBUG_Unused_9___SHIFT 	9
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DEBUG_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DEBUG_ABORT_LOC___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DEBUG____REGMASK	UINT32_C(511)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR0 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR0____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR0____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR0_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR1 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR1____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR1____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR1_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR2 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR2____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR2____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR2_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR3 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR3____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR3____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR3_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR4 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR4____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR4____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR4_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR5 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR5____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR5____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR5_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_WR5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD0 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD0____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD0____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD0_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD1 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD1____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD1____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD1_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD2 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD2____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD2____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD2_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD3 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD3____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD3____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD3_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD4 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD4____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD4____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD4_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD5 ---- */
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD5____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD5____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD5_DATA___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_DIAG_DATA_RD5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_MORE_I2C_PWRGD_I2C_SW_OVERRIDE ---- */
#define RUDRA40_MORE_I2C_PWRGD_I2C_SW_OVERRIDE____WIDTH	32
#define RUDRA40_MORE_I2C_PWRGD_I2C_SW_OVERRIDE____TYPE 	uint32_t

#define RUDRA40_MORE_I2C_PWRGD_I2C_SW_OVERRIDE_Unused_1___MASK       	UINT32_C(0xfffffffe)
#define RUDRA40_MORE_I2C_PWRGD_I2C_SW_OVERRIDE_Unused_1___SHIFT      	1
#define RUDRA40_MORE_I2C_PWRGD_I2C_SW_OVERRIDE_sw_override_en___MASK 	UINT32_C(0x1)
#define RUDRA40_MORE_I2C_PWRGD_I2C_SW_OVERRIDE_sw_override_en___SHIFT	0
#define RUDRA40_MORE_I2C_PWRGD_I2C_SW_OVERRIDE____REGMASK	UINT32_C(1)

/* ---- RUDRA40_OPTICS_QSFP_LOW_PWR_0 ---- */
#define RUDRA40_OPTICS_QSFP_LOW_PWR_0____WIDTH	32
#define RUDRA40_OPTICS_QSFP_LOW_PWR_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_QSFP_LOW_PWR_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_QSFP_LOW_PWR_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_QSFP_LOW_PWR_0_low___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_QSFP_LOW_PWR_0_low___SHIFT     	0
#define RUDRA40_OPTICS_QSFP_LOW_PWR_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_QSFP_LOW_PWR_1 ---- */
#define RUDRA40_OPTICS_QSFP_LOW_PWR_1____WIDTH	32
#define RUDRA40_OPTICS_QSFP_LOW_PWR_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_QSFP_LOW_PWR_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_QSFP_LOW_PWR_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_QSFP_LOW_PWR_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_QSFP_LOW_PWR_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_QSFP_LOW_PWR_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_QSFP_RESET_0 ---- */
#define RUDRA40_OPTICS_QSFP_RESET_0____WIDTH	32
#define RUDRA40_OPTICS_QSFP_RESET_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_QSFP_RESET_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_QSFP_RESET_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_QSFP_RESET_0_rstn___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_QSFP_RESET_0_rstn___SHIFT    	0
#define RUDRA40_OPTICS_QSFP_RESET_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_QSFP_RESET_1 ---- */
#define RUDRA40_OPTICS_QSFP_RESET_1____WIDTH	32
#define RUDRA40_OPTICS_QSFP_RESET_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_QSFP_RESET_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_QSFP_RESET_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_QSFP_RESET_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_QSFP_RESET_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_QSFP_RESET_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_QSFP_PWR_EN_0 ---- */
#define RUDRA40_OPTICS_QSFP_PWR_EN_0____WIDTH	32
#define RUDRA40_OPTICS_QSFP_PWR_EN_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_QSFP_PWR_EN_0_Unused_8___MASK   	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_QSFP_PWR_EN_0_Unused_8___SHIFT  	8
#define RUDRA40_OPTICS_QSFP_PWR_EN_0_pwr_enable___MASK 	UINT32_C(0xff)
#define RUDRA40_OPTICS_QSFP_PWR_EN_0_pwr_enable___SHIFT	0
#define RUDRA40_OPTICS_QSFP_PWR_EN_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_QSFP_PWR_EN_1 ---- */
#define RUDRA40_OPTICS_QSFP_PWR_EN_1____WIDTH	32
#define RUDRA40_OPTICS_QSFP_PWR_EN_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_QSFP_PWR_EN_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_QSFP_PWR_EN_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_QSFP_PWR_EN_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_QSFP_PWR_EN_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_QSFP_PWR_EN_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_ISR_QSFP_PRESENT_0 ---- */
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_0____WIDTH	32
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_0_CHG___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_0_CHG___SHIFT     	0
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISR_QSFP_PRESENT_1 ---- */
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_1____WIDTH	32
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_ISR_QSFP_PRESENT_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_ISM_QSFP_PRESENT_0 ---- */
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_0____WIDTH	32
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_0_MASK___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_0_MASK___SHIFT    	0
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISM_QSFP_PRESENT_1 ---- */
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_1____WIDTH	32
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_ISM_QSFP_PRESENT_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_STATUS_QSFP_PRESENT_0 ---- */
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_0____WIDTH	32
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_0_STAT___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_0_STAT___SHIFT    	0
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_STATUS_QSFP_PRESENT_1 ---- */
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_1____WIDTH	32
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_STATUS_QSFP_PRESENT_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_IST_QSFP_PRESENT_0 ---- */
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_0____WIDTH	32
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_QSFP_PRESENT_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_0_TST___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_0_TST___SHIFT     	0
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_IST_QSFP_PRESENT_1 ---- */
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_1____WIDTH	32
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_QSFP_PRESENT_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_IST_QSFP_PRESENT_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_ISR_QSFP_LOS_0 ---- */
#define RUDRA40_OPTICS_ISR_QSFP_LOS_0____WIDTH	32
#define RUDRA40_OPTICS_ISR_QSFP_LOS_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_QSFP_LOS_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISR_QSFP_LOS_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISR_QSFP_LOS_0_CHG___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISR_QSFP_LOS_0_CHG___SHIFT     	0
#define RUDRA40_OPTICS_ISR_QSFP_LOS_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISR_QSFP_LOS_1 ---- */
#define RUDRA40_OPTICS_ISR_QSFP_LOS_1____WIDTH	32
#define RUDRA40_OPTICS_ISR_QSFP_LOS_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_QSFP_LOS_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_ISR_QSFP_LOS_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_ISR_QSFP_LOS_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_ISR_QSFP_LOS_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_ISR_QSFP_LOS_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_ISM_QSFP_LOS_0 ---- */
#define RUDRA40_OPTICS_ISM_QSFP_LOS_0____WIDTH	32
#define RUDRA40_OPTICS_ISM_QSFP_LOS_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_QSFP_LOS_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISM_QSFP_LOS_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISM_QSFP_LOS_0_MASK___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISM_QSFP_LOS_0_MASK___SHIFT    	0
#define RUDRA40_OPTICS_ISM_QSFP_LOS_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISM_QSFP_LOS_1 ---- */
#define RUDRA40_OPTICS_ISM_QSFP_LOS_1____WIDTH	32
#define RUDRA40_OPTICS_ISM_QSFP_LOS_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_QSFP_LOS_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_ISM_QSFP_LOS_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_ISM_QSFP_LOS_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_ISM_QSFP_LOS_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_ISM_QSFP_LOS_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_STATUS_QSFP_LOS_0 ---- */
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_0____WIDTH	32
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_QSFP_LOS_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_0_STAT___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_0_STAT___SHIFT    	0
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_STATUS_QSFP_LOS_1 ---- */
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_1____WIDTH	32
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_QSFP_LOS_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_STATUS_QSFP_LOS_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_IST_QSFP_LOS_0 ---- */
#define RUDRA40_OPTICS_IST_QSFP_LOS_0____WIDTH	32
#define RUDRA40_OPTICS_IST_QSFP_LOS_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_QSFP_LOS_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_IST_QSFP_LOS_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_IST_QSFP_LOS_0_TST___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_IST_QSFP_LOS_0_TST___SHIFT     	0
#define RUDRA40_OPTICS_IST_QSFP_LOS_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_IST_QSFP_LOS_1 ---- */
#define RUDRA40_OPTICS_IST_QSFP_LOS_1____WIDTH	32
#define RUDRA40_OPTICS_IST_QSFP_LOS_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_QSFP_LOS_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_IST_QSFP_LOS_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_IST_QSFP_LOS_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_IST_QSFP_LOS_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_IST_QSFP_LOS_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_ISR_QSFP_PWR_GD_0 ---- */
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_0____WIDTH	32
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_0_CHG___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_0_CHG___SHIFT     	0
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISR_QSFP_PWR_GD_1 ---- */
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_1____WIDTH	32
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_ISR_QSFP_PWR_GD_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_ISM_QSFP_PWR_GD_0 ---- */
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_0____WIDTH	32
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_0_MASK___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_0_MASK___SHIFT    	0
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISM_QSFP_PWR_GD_1 ---- */
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_1____WIDTH	32
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_ISM_QSFP_PWR_GD_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_0 ---- */
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_0____WIDTH	32
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_0_STAT___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_0_STAT___SHIFT    	0
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_1 ---- */
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_1____WIDTH	32
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_IST_QSFP_PWR_GD_0 ---- */
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_0____WIDTH	32
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_0_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_0_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_0_TST___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_0_TST___SHIFT     	0
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_0____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_IST_QSFP_PWR_GD_1 ---- */
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_1____WIDTH	32
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_1_Unused_4___MASK 	UINT32_C(0xfffffff0)
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_1_Unused_4___SHIFT	4
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_1_Unused_0___MASK 	UINT32_C(0xf)
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_1_Unused_0___SHIFT	0
#define RUDRA40_OPTICS_IST_QSFP_PWR_GD_1____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_I2C_MUX_SEL ---- */
#define RUDRA40_OPTICS_I2C_MUX_SEL____WIDTH	32
#define RUDRA40_OPTICS_I2C_MUX_SEL____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C_MUX_SEL_Unused_7___MASK 	UINT32_C(0xffffff80)
#define RUDRA40_OPTICS_I2C_MUX_SEL_Unused_7___SHIFT	7
#define RUDRA40_OPTICS_I2C_MUX_SEL_CAGE___MASK     	UINT32_C(0x60)
#define RUDRA40_OPTICS_I2C_MUX_SEL_CAGE___SHIFT    	5
#define RUDRA40_OPTICS_I2C_MUX_SEL_SDA___MASK      	UINT32_C(0x18)
#define RUDRA40_OPTICS_I2C_MUX_SEL_SDA___SHIFT     	3
#define RUDRA40_OPTICS_I2C_MUX_SEL_SCL___MASK      	UINT32_C(0x7)
#define RUDRA40_OPTICS_I2C_MUX_SEL_SCL___SHIFT     	0
#define RUDRA40_OPTICS_I2C_MUX_SEL____REGMASK	UINT32_C(127)

/* ---- RUDRA40_OPTICS_RESERVED0 ---- */
#define RUDRA40_OPTICS_RESERVED0____WIDTH	32
#define RUDRA40_OPTICS_RESERVED0____TYPE 	uint32_t

#define RUDRA40_OPTICS_RESERVED0____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_CTRL ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_Unused_8___MASK         	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_Unused_8___SHIFT        	8
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_RESET_MASTER___MASK     	UINT32_C(0x80)
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_RESET_MASTER___SHIFT    	7
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_SLAVE_ADDRS_NACK___MASK 	UINT32_C(0x40)
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_SLAVE_ADDRS_NACK___SHIFT	6
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_NO_DATAADDR_MODE___MASK 	UINT32_C(0x20)
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_NO_DATAADDR_MODE___SHIFT	5
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_DATAADDR_MODE___MASK    	UINT32_C(0x10)
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_DATAADDR_MODE___SHIFT   	4
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_RD_WRn___MASK           	UINT32_C(0x8)
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_RD_WRn___SHIFT          	3
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_ABORT___MASK            	UINT32_C(0x4)
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_ABORT___SHIFT           	2
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_FINISH___MASK           	UINT32_C(0x2)
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_FINISH___SHIFT          	1
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_START___MASK            	UINT32_C(0x1)
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL_START___SHIFT           	0
#define RUDRA40_OPTICS_I2C0_DIAG_CTRL____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_4X ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_4X____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_4X____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_4X_Unused_9___MASK 	UINT32_C(0xfffffe00)
#define RUDRA40_OPTICS_I2C0_DIAG_4X_Unused_9___SHIFT	9
#define RUDRA40_OPTICS_I2C0_DIAG_4X_CLK_DIV___MASK  	UINT32_C(0x1ff)
#define RUDRA40_OPTICS_I2C0_DIAG_4X_CLK_DIV___SHIFT 	0
#define RUDRA40_OPTICS_I2C0_DIAG_4X____REGMASK	UINT32_C(511)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DEVADDR ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DEVADDR____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DEVADDR____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DEVADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATAADDR ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATAADDR____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATAADDR____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATAADDR____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_VLD ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_VLD____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_VLD____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_VLD____REGMASK	UINT32_C(0)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DEBUG ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DEBUG____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DEBUG____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DEBUG_Unused_9___MASK  	UINT32_C(0xfffffe00)
#define RUDRA40_OPTICS_I2C0_DIAG_DEBUG_Unused_9___SHIFT 	9
#define RUDRA40_OPTICS_I2C0_DIAG_DEBUG_ABORT_LOC___MASK 	UINT32_C(0x1ff)
#define RUDRA40_OPTICS_I2C0_DIAG_DEBUG_ABORT_LOC___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DEBUG____REGMASK	UINT32_C(511)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR0 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR0____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR0____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR0_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR1 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR1____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR1____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR1_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR2 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR2____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR2____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR2_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR3 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR3____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR3____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR3_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR4 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR4____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR4____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR4_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR5 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR5____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR5____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR5_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR6 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR6____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR6____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR6_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR6_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR6____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR7 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR7____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR7____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR7_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR7_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR7____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR8 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR8____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR8____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR8_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR8_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR8____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR9 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR9____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR9____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR9_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR9_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR9____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR10 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR10____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR10____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR10_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR10_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR10____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR11 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR11____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR11____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR11_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR11_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR11____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR12 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR12____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR12____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR12_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR12_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR12____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR13 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR13____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR13____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR13_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR13_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR13____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR14 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR14____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR14____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR14_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR14_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR14____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR15 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR15____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR15____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR15_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR15_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR15____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR16 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR16____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR16____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR16_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR16_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR16____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR17 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR17____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR17____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR17_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR17_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR17____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR18 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR18____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR18____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR18_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR18_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR18____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR19 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR19____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR19____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR19_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR19_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR19____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR20 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR20____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR20____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR20_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR20_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR20____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR21 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR21____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR21____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR21_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR21_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR21____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR22 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR22____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR22____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR22_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR22_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR22____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR23 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR23____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR23____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR23_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR23_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR23____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR24 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR24____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR24____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR24_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR24_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR24____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR25 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR25____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR25____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR25_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR25_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR25____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR26 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR26____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR26____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR26_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR26_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR26____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR27 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR27____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR27____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR27_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR27_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR27____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR28 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR28____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR28____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR28_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR28_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR28____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR29 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR29____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR29____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR29_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR29_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR29____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR30 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR30____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR30____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR30_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR30_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR30____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR31 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR31____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR31____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR31_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR31_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR31____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR32 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR32____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR32____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR32_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR32_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR32____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR33 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR33____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR33____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR33_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR33_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR33____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR34 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR34____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR34____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR34_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR34_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR34____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR35 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR35____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR35____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR35_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR35_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR35____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR36 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR36____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR36____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR36_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR36_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR36____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR37 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR37____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR37____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR37_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR37_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR37____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR38 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR38____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR38____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR38_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR38_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR38____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR39 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR39____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR39____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR39_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR39_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR39____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR40 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR40____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR40____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR40_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR40_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR40____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR41 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR41____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR41____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR41_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR41_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR41____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR42 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR42____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR42____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR42_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR42_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR42____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR43 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR43____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR43____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR43_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR43_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR43____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR44 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR44____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR44____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR44_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR44_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR44____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR45 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR45____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR45____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR45_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR45_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR45____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR46 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR46____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR46____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR46_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR46_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR46____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR47 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR47____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR47____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR47_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR47_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR47____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR48 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR48____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR48____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR48_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR48_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR48____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR49 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR49____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR49____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR49_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR49_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR49____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR50 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR50____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR50____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR50_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR50_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR50____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR51 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR51____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR51____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR51_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR51_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR51____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR52 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR52____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR52____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR52_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR52_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR52____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR53 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR53____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR53____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR53_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR53_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR53____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR54 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR54____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR54____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR54_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR54_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR54____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR55 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR55____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR55____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR55_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR55_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR55____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR56 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR56____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR56____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR56_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR56_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR56____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR57 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR57____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR57____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR57_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR57_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR57____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR58 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR58____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR58____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR58_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR58_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR58____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR59 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR59____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR59____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR59_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR59_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR59____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR60 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR60____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR60____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR60_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR60_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR60____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR61 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR61____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR61____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR61_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR61_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR61____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR62 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR62____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR62____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR62_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR62_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR62____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_WR63 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR63____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR63____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR63_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR63_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_WR63____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD0 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD0____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD0____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD0_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD0_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD1 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD1____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD1____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD1_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD1_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD1____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD2 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD2____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD2____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD2_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD2_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD2____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD3 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD3____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD3____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD3_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD3_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD3____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD4 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD4____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD4____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD4_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD4_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD4____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD5 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD5____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD5____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD5_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD5_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD5____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD6 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD6____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD6____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD6_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD6_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD6____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD7 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD7____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD7____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD7_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD7_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD7____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD8 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD8____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD8____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD8_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD8_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD8____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD9 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD9____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD9____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD9_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD9_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD9____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD10 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD10____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD10____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD10_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD10_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD10____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD11 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD11____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD11____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD11_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD11_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD11____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD12 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD12____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD12____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD12_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD12_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD12____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD13 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD13____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD13____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD13_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD13_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD13____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD14 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD14____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD14____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD14_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD14_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD14____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD15 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD15____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD15____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD15_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD15_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD15____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD16 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD16____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD16____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD16_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD16_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD16____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD17 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD17____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD17____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD17_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD17_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD17____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD18 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD18____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD18____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD18_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD18_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD18____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD19 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD19____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD19____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD19_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD19_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD19____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD20 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD20____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD20____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD20_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD20_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD20____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD21 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD21____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD21____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD21_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD21_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD21____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD22 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD22____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD22____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD22_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD22_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD22____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD23 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD23____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD23____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD23_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD23_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD23____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD24 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD24____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD24____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD24_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD24_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD24____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD25 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD25____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD25____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD25_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD25_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD25____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD26 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD26____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD26____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD26_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD26_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD26____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD27 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD27____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD27____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD27_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD27_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD27____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD28 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD28____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD28____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD28_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD28_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD28____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD29 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD29____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD29____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD29_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD29_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD29____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD30 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD30____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD30____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD30_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD30_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD30____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD31 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD31____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD31____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD31_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD31_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD31____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD32 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD32____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD32____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD32_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD32_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD32____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD33 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD33____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD33____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD33_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD33_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD33____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD34 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD34____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD34____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD34_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD34_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD34____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD35 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD35____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD35____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD35_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD35_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD35____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD36 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD36____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD36____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD36_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD36_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD36____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD37 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD37____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD37____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD37_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD37_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD37____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD38 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD38____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD38____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD38_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD38_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD38____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD39 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD39____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD39____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD39_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD39_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD39____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD40 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD40____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD40____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD40_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD40_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD40____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD41 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD41____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD41____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD41_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD41_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD41____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD42 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD42____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD42____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD42_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD42_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD42____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD43 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD43____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD43____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD43_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD43_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD43____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD44 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD44____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD44____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD44_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD44_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD44____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD45 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD45____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD45____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD45_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD45_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD45____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD46 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD46____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD46____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD46_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD46_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD46____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD47 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD47____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD47____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD47_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD47_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD47____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD48 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD48____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD48____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD48_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD48_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD48____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD49 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD49____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD49____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD49_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD49_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD49____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD50 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD50____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD50____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD50_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD50_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD50____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD51 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD51____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD51____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD51_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD51_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD51____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD52 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD52____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD52____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD52_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD52_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD52____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD53 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD53____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD53____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD53_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD53_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD53____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD54 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD54____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD54____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD54_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD54_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD54____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD55 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD55____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD55____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD55_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD55_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD55____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD56 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD56____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD56____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD56_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD56_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD56____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD57 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD57____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD57____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD57_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD57_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD57____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD58 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD58____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD58____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD58_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD58_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD58____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD59 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD59____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD59____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD59_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD59_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD59____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD60 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD60____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD60____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD60_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD60_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD60____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD61 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD61____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD61____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD61_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD61_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD61____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD62 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD62____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD62____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD62_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD62_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD62____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_DIAG_DATA_RD63 ---- */
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD63____WIDTH	32
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD63____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD63_DATA___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD63_DATA___SHIFT	0
#define RUDRA40_OPTICS_I2C0_DIAG_DATA_RD63____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_I2C0_SW_CTRL ---- */
#define RUDRA40_OPTICS_I2C0_SW_CTRL____WIDTH	32
#define RUDRA40_OPTICS_I2C0_SW_CTRL____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_SW_CTRL_Unused_6___MASK 	UINT32_C(0xffffffc0)
#define RUDRA40_OPTICS_I2C0_SW_CTRL_Unused_6___SHIFT	6
#define RUDRA40_OPTICS_I2C0_SW_CTRL_READACK___MASK  	UINT32_C(0x20)
#define RUDRA40_OPTICS_I2C0_SW_CTRL_READACK___SHIFT 	5
#define RUDRA40_OPTICS_I2C0_SW_CTRL_READNACK___MASK 	UINT32_C(0x10)
#define RUDRA40_OPTICS_I2C0_SW_CTRL_READNACK___SHIFT	4
#define RUDRA40_OPTICS_I2C0_SW_CTRL_WRITE___MASK    	UINT32_C(0x8)
#define RUDRA40_OPTICS_I2C0_SW_CTRL_WRITE___SHIFT   	3
#define RUDRA40_OPTICS_I2C0_SW_CTRL_STOP___MASK     	UINT32_C(0x4)
#define RUDRA40_OPTICS_I2C0_SW_CTRL_STOP___SHIFT    	2
#define RUDRA40_OPTICS_I2C0_SW_CTRL_START___MASK    	UINT32_C(0x2)
#define RUDRA40_OPTICS_I2C0_SW_CTRL_START___SHIFT   	1
#define RUDRA40_OPTICS_I2C0_SW_CTRL_RESET___MASK    	UINT32_C(0x1)
#define RUDRA40_OPTICS_I2C0_SW_CTRL_RESET___SHIFT   	0
#define RUDRA40_OPTICS_I2C0_SW_CTRL____REGMASK	UINT32_C(63)

/* ---- RUDRA40_OPTICS_I2C0_SW_STAT ---- */
#define RUDRA40_OPTICS_I2C0_SW_STAT____WIDTH	32
#define RUDRA40_OPTICS_I2C0_SW_STAT____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_SW_STAT_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_I2C0_SW_STAT_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_I2C0_SW_STAT_STATE___MASK    	UINT32_C(0xf0)
#define RUDRA40_OPTICS_I2C0_SW_STAT_STATE___SHIFT   	4
#define RUDRA40_OPTICS_I2C0_SW_STAT_SDA___MASK      	UINT32_C(0x8)
#define RUDRA40_OPTICS_I2C0_SW_STAT_SDA___SHIFT     	3
#define RUDRA40_OPTICS_I2C0_SW_STAT_SCL___MASK      	UINT32_C(0x4)
#define RUDRA40_OPTICS_I2C0_SW_STAT_SCL___SHIFT     	2
#define RUDRA40_OPTICS_I2C0_SW_STAT_ACKR___MASK     	UINT32_C(0x2)
#define RUDRA40_OPTICS_I2C0_SW_STAT_ACKR___SHIFT    	1
#define RUDRA40_OPTICS_I2C0_SW_STAT_STAT___MASK     	UINT32_C(0x1)
#define RUDRA40_OPTICS_I2C0_SW_STAT_STAT___SHIFT    	0
#define RUDRA40_OPTICS_I2C0_SW_STAT____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_I2C0_SW_WRITE_DATA ---- */
#define RUDRA40_OPTICS_I2C0_SW_WRITE_DATA____WIDTH	32
#define RUDRA40_OPTICS_I2C0_SW_WRITE_DATA____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_SW_WRITE_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_I2C0_SW_WRITE_DATA_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_I2C0_SW_WRITE_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_I2C0_SW_WRITE_DATA_DATA___SHIFT    	0
#define RUDRA40_OPTICS_I2C0_SW_WRITE_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_I2C0_SW_READ_DATA ---- */
#define RUDRA40_OPTICS_I2C0_SW_READ_DATA____WIDTH	32
#define RUDRA40_OPTICS_I2C0_SW_READ_DATA____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_SW_READ_DATA_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_I2C0_SW_READ_DATA_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_I2C0_SW_READ_DATA_DATA___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_I2C0_SW_READ_DATA_DATA___SHIFT    	0
#define RUDRA40_OPTICS_I2C0_SW_READ_DATA____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_I2C0_SW_DONE ---- */
#define RUDRA40_OPTICS_I2C0_SW_DONE____WIDTH	32
#define RUDRA40_OPTICS_I2C0_SW_DONE____TYPE 	uint32_t

#define RUDRA40_OPTICS_I2C0_SW_DONE_Unused_1___MASK 	UINT32_C(0xfffffffe)
#define RUDRA40_OPTICS_I2C0_SW_DONE_Unused_1___SHIFT	1
#define RUDRA40_OPTICS_I2C0_SW_DONE_CMP___MASK      	UINT32_C(0x1)
#define RUDRA40_OPTICS_I2C0_SW_DONE_CMP___SHIFT     	0
#define RUDRA40_OPTICS_I2C0_SW_DONE____REGMASK	UINT32_C(1)

/* ---- RUDRA40_OPTICS_SFP_TX_DISABLE ---- */
#define RUDRA40_OPTICS_SFP_TX_DISABLE____WIDTH	32
#define RUDRA40_OPTICS_SFP_TX_DISABLE____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFP_TX_DISABLE_disable___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_SFP_TX_DISABLE_disable___SHIFT	0
#define RUDRA40_OPTICS_SFP_TX_DISABLE____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_SFP_TX_DISABLE_2 ---- */
#define RUDRA40_OPTICS_SFP_TX_DISABLE_2____WIDTH	32
#define RUDRA40_OPTICS_SFP_TX_DISABLE_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFP_TX_DISABLE_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_SFP_TX_DISABLE_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_SFP_TX_DISABLE_2_disable___MASK  	UINT32_C(0xff)
#define RUDRA40_OPTICS_SFP_TX_DISABLE_2_disable___SHIFT 	0
#define RUDRA40_OPTICS_SFP_TX_DISABLE_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_SFP_RATE_SELECT ---- */
#define RUDRA40_OPTICS_SFP_RATE_SELECT____WIDTH	32
#define RUDRA40_OPTICS_SFP_RATE_SELECT____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFP_RATE_SELECT_rate___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_SFP_RATE_SELECT_rate___SHIFT	0
#define RUDRA40_OPTICS_SFP_RATE_SELECT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_SFP_RATE_SELECT_2 ---- */
#define RUDRA40_OPTICS_SFP_RATE_SELECT_2____WIDTH	32
#define RUDRA40_OPTICS_SFP_RATE_SELECT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFP_RATE_SELECT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_SFP_RATE_SELECT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_SFP_RATE_SELECT_2_rate___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_SFP_RATE_SELECT_2_rate___SHIFT    	0
#define RUDRA40_OPTICS_SFP_RATE_SELECT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_SFP_PWR_EN ---- */
#define RUDRA40_OPTICS_SFP_PWR_EN____WIDTH	32
#define RUDRA40_OPTICS_SFP_PWR_EN____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFP_PWR_EN_pwr_enable___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_SFP_PWR_EN_pwr_enable___SHIFT	0
#define RUDRA40_OPTICS_SFP_PWR_EN____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_SFP_PWR_EN_2 ---- */
#define RUDRA40_OPTICS_SFP_PWR_EN_2____WIDTH	32
#define RUDRA40_OPTICS_SFP_PWR_EN_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFP_PWR_EN_2_Unused_8___MASK   	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_SFP_PWR_EN_2_Unused_8___SHIFT  	8
#define RUDRA40_OPTICS_SFP_PWR_EN_2_pwr_enable___MASK 	UINT32_C(0xff)
#define RUDRA40_OPTICS_SFP_PWR_EN_2_pwr_enable___SHIFT	0
#define RUDRA40_OPTICS_SFP_PWR_EN_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_SFPDD_TX_DISABLE ---- */
#define RUDRA40_OPTICS_SFPDD_TX_DISABLE____WIDTH	32
#define RUDRA40_OPTICS_SFPDD_TX_DISABLE____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFPDD_TX_DISABLE_disable___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_SFPDD_TX_DISABLE_disable___SHIFT	0
#define RUDRA40_OPTICS_SFPDD_TX_DISABLE____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_SFPDD_TX_DISABLE_2 ---- */
#define RUDRA40_OPTICS_SFPDD_TX_DISABLE_2____WIDTH	32
#define RUDRA40_OPTICS_SFPDD_TX_DISABLE_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFPDD_TX_DISABLE_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_SFPDD_TX_DISABLE_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_SFPDD_TX_DISABLE_2_disable___MASK  	UINT32_C(0xff)
#define RUDRA40_OPTICS_SFPDD_TX_DISABLE_2_disable___SHIFT 	0
#define RUDRA40_OPTICS_SFPDD_TX_DISABLE_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_SFPDD_RATE_SELECT ---- */
#define RUDRA40_OPTICS_SFPDD_RATE_SELECT____WIDTH	32
#define RUDRA40_OPTICS_SFPDD_RATE_SELECT____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFPDD_RATE_SELECT_rate___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_SFPDD_RATE_SELECT_rate___SHIFT	0
#define RUDRA40_OPTICS_SFPDD_RATE_SELECT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_SFPDD_RATE_SELECT_2 ---- */
#define RUDRA40_OPTICS_SFPDD_RATE_SELECT_2____WIDTH	32
#define RUDRA40_OPTICS_SFPDD_RATE_SELECT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFPDD_RATE_SELECT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_SFPDD_RATE_SELECT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_SFPDD_RATE_SELECT_2_rate___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_SFPDD_RATE_SELECT_2_rate___SHIFT    	0
#define RUDRA40_OPTICS_SFPDD_RATE_SELECT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISR_SFP_TX_FAULT ---- */
#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT_CHG___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT_CHG___SHIFT	0
#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISR_SFP_TX_FAULT_2 ---- */
#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT_2____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT_2_CHG___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT_2_CHG___SHIFT     	0
#define RUDRA40_OPTICS_ISR_SFP_TX_FAULT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISM_SFP_TX_FAULT ---- */
#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT_MASK___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT_MASK___SHIFT	0
#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISM_SFP_TX_FAULT_2 ---- */
#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT_2____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT_2_MASK___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT_2_MASK___SHIFT    	0
#define RUDRA40_OPTICS_ISM_SFP_TX_FAULT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_STATUS_SFP_TX_FAULT ---- */
#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT_STAT___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT_STAT___SHIFT	0
#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_STATUS_SFP_TX_FAULT_2 ---- */
#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT_2____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT_2_STAT___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT_2_STAT___SHIFT    	0
#define RUDRA40_OPTICS_STATUS_SFP_TX_FAULT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_IST_SFP_TX_FAULT ---- */
#define RUDRA40_OPTICS_IST_SFP_TX_FAULT____WIDTH	32
#define RUDRA40_OPTICS_IST_SFP_TX_FAULT____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFP_TX_FAULT_TST___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_IST_SFP_TX_FAULT_TST___SHIFT	0
#define RUDRA40_OPTICS_IST_SFP_TX_FAULT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_IST_SFP_TX_FAULT_2 ---- */
#define RUDRA40_OPTICS_IST_SFP_TX_FAULT_2____WIDTH	32
#define RUDRA40_OPTICS_IST_SFP_TX_FAULT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFP_TX_FAULT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_IST_SFP_TX_FAULT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_IST_SFP_TX_FAULT_2_TST___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_IST_SFP_TX_FAULT_2_TST___SHIFT     	0
#define RUDRA40_OPTICS_IST_SFP_TX_FAULT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT ---- */
#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_CHG___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_CHG___SHIFT	0
#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_2 ---- */
#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_2____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_2_CHG___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_2_CHG___SHIFT     	0
#define RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT ---- */
#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT_MASK___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT_MASK___SHIFT	0
#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT_2 ---- */
#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT_2____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT_2_MASK___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT_2_MASK___SHIFT    	0
#define RUDRA40_OPTICS_ISM_SFPDD_TX_FAULT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT ---- */
#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT_STAT___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT_STAT___SHIFT	0
#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT_2 ---- */
#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT_2____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT_2_STAT___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT_2_STAT___SHIFT    	0
#define RUDRA40_OPTICS_STATUS_SFPDD_TX_FAULT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_IST_SFPDD_TX_FAULT ---- */
#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT____WIDTH	32
#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT_TST___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT_TST___SHIFT	0
#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_IST_SFPDD_TX_FAULT_2 ---- */
#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT_2____WIDTH	32
#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT_2_TST___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT_2_TST___SHIFT     	0
#define RUDRA40_OPTICS_IST_SFPDD_TX_FAULT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISR_SFP_RX_LOS ---- */
#define RUDRA40_OPTICS_ISR_SFP_RX_LOS____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFP_RX_LOS____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFP_RX_LOS_CHG___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISR_SFP_RX_LOS_CHG___SHIFT	0
#define RUDRA40_OPTICS_ISR_SFP_RX_LOS____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISR_SFP_RX_LOS_2 ---- */
#define RUDRA40_OPTICS_ISR_SFP_RX_LOS_2____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFP_RX_LOS_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFP_RX_LOS_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISR_SFP_RX_LOS_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISR_SFP_RX_LOS_2_CHG___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISR_SFP_RX_LOS_2_CHG___SHIFT     	0
#define RUDRA40_OPTICS_ISR_SFP_RX_LOS_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISM_SFP_RX_LOS ---- */
#define RUDRA40_OPTICS_ISM_SFP_RX_LOS____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFP_RX_LOS____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFP_RX_LOS_MASK___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISM_SFP_RX_LOS_MASK___SHIFT	0
#define RUDRA40_OPTICS_ISM_SFP_RX_LOS____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISM_SFP_RX_LOS_2 ---- */
#define RUDRA40_OPTICS_ISM_SFP_RX_LOS_2____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFP_RX_LOS_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFP_RX_LOS_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISM_SFP_RX_LOS_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISM_SFP_RX_LOS_2_MASK___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISM_SFP_RX_LOS_2_MASK___SHIFT    	0
#define RUDRA40_OPTICS_ISM_SFP_RX_LOS_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_STATUS_SFP_RX_LOS ---- */
#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS_STAT___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS_STAT___SHIFT	0
#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_STATUS_SFP_RX_LOS_2 ---- */
#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS_2____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS_2_STAT___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS_2_STAT___SHIFT    	0
#define RUDRA40_OPTICS_STATUS_SFP_RX_LOS_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_IST_SFP_RX_LOS ---- */
#define RUDRA40_OPTICS_IST_SFP_RX_LOS____WIDTH	32
#define RUDRA40_OPTICS_IST_SFP_RX_LOS____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFP_RX_LOS_TST___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_IST_SFP_RX_LOS_TST___SHIFT	0
#define RUDRA40_OPTICS_IST_SFP_RX_LOS____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_IST_SFP_RX_LOS_2 ---- */
#define RUDRA40_OPTICS_IST_SFP_RX_LOS_2____WIDTH	32
#define RUDRA40_OPTICS_IST_SFP_RX_LOS_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFP_RX_LOS_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_IST_SFP_RX_LOS_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_IST_SFP_RX_LOS_2_TST___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_IST_SFP_RX_LOS_2_TST___SHIFT     	0
#define RUDRA40_OPTICS_IST_SFP_RX_LOS_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISR_SFPDD_RX_LOS ---- */
#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_CHG___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_CHG___SHIFT	0
#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_2 ---- */
#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_2____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_2_CHG___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_2_CHG___SHIFT     	0
#define RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISM_SFPDD_RX_LOS ---- */
#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS_MASK___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS_MASK___SHIFT	0
#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISM_SFPDD_RX_LOS_2 ---- */
#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS_2____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS_2_MASK___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS_2_MASK___SHIFT    	0
#define RUDRA40_OPTICS_ISM_SFPDD_RX_LOS_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS ---- */
#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS_STAT___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS_STAT___SHIFT	0
#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS_2 ---- */
#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS_2____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS_2_STAT___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS_2_STAT___SHIFT    	0
#define RUDRA40_OPTICS_STATUS_SFPDD_RX_LOS_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_IST_SFPDD_RX_LOS ---- */
#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS____WIDTH	32
#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS_TST___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS_TST___SHIFT	0
#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_IST_SFPDD_RX_LOS_2 ---- */
#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS_2____WIDTH	32
#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS_2_TST___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS_2_TST___SHIFT     	0
#define RUDRA40_OPTICS_IST_SFPDD_RX_LOS_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISR_SFP_PRESENT ---- */
#define RUDRA40_OPTICS_ISR_SFP_PRESENT____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFP_PRESENT____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFP_PRESENT_CHG___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISR_SFP_PRESENT_CHG___SHIFT	0
#define RUDRA40_OPTICS_ISR_SFP_PRESENT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISR_SFP_PRESENT_2 ---- */
#define RUDRA40_OPTICS_ISR_SFP_PRESENT_2____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFP_PRESENT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFP_PRESENT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISR_SFP_PRESENT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISR_SFP_PRESENT_2_CHG___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISR_SFP_PRESENT_2_CHG___SHIFT     	0
#define RUDRA40_OPTICS_ISR_SFP_PRESENT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISM_SFP_PRESENT ---- */
#define RUDRA40_OPTICS_ISM_SFP_PRESENT____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFP_PRESENT____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFP_PRESENT_MASK___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISM_SFP_PRESENT_MASK___SHIFT	0
#define RUDRA40_OPTICS_ISM_SFP_PRESENT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISM_SFP_PRESENT_2 ---- */
#define RUDRA40_OPTICS_ISM_SFP_PRESENT_2____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFP_PRESENT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFP_PRESENT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISM_SFP_PRESENT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISM_SFP_PRESENT_2_MASK___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISM_SFP_PRESENT_2_MASK___SHIFT    	0
#define RUDRA40_OPTICS_ISM_SFP_PRESENT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_STATUS_SFP_PRESENT ---- */
#define RUDRA40_OPTICS_STATUS_SFP_PRESENT____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFP_PRESENT____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFP_PRESENT_STAT___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_STATUS_SFP_PRESENT_STAT___SHIFT	0
#define RUDRA40_OPTICS_STATUS_SFP_PRESENT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_STATUS_SFP_PRESENT_2 ---- */
#define RUDRA40_OPTICS_STATUS_SFP_PRESENT_2____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFP_PRESENT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFP_PRESENT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_STATUS_SFP_PRESENT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_STATUS_SFP_PRESENT_2_STAT___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_STATUS_SFP_PRESENT_2_STAT___SHIFT    	0
#define RUDRA40_OPTICS_STATUS_SFP_PRESENT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_IST_SFP_PRESENT ---- */
#define RUDRA40_OPTICS_IST_SFP_PRESENT____WIDTH	32
#define RUDRA40_OPTICS_IST_SFP_PRESENT____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFP_PRESENT_TST___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_IST_SFP_PRESENT_TST___SHIFT	0
#define RUDRA40_OPTICS_IST_SFP_PRESENT____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_IST_SFP_PRESENT_2 ---- */
#define RUDRA40_OPTICS_IST_SFP_PRESENT_2____WIDTH	32
#define RUDRA40_OPTICS_IST_SFP_PRESENT_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFP_PRESENT_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_IST_SFP_PRESENT_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_IST_SFP_PRESENT_2_TST___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_IST_SFP_PRESENT_2_TST___SHIFT     	0
#define RUDRA40_OPTICS_IST_SFP_PRESENT_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISR_SFP_PWR_GD ---- */
#define RUDRA40_OPTICS_ISR_SFP_PWR_GD____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFP_PWR_GD____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFP_PWR_GD_CHG___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISR_SFP_PWR_GD_CHG___SHIFT	0
#define RUDRA40_OPTICS_ISR_SFP_PWR_GD____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISR_SFP_PWR_GD_2 ---- */
#define RUDRA40_OPTICS_ISR_SFP_PWR_GD_2____WIDTH	32
#define RUDRA40_OPTICS_ISR_SFP_PWR_GD_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISR_SFP_PWR_GD_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISR_SFP_PWR_GD_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISR_SFP_PWR_GD_2_CHG___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISR_SFP_PWR_GD_2_CHG___SHIFT     	0
#define RUDRA40_OPTICS_ISR_SFP_PWR_GD_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_ISM_SFP_PWR_GD ---- */
#define RUDRA40_OPTICS_ISM_SFP_PWR_GD____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFP_PWR_GD____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFP_PWR_GD_MASK___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_ISM_SFP_PWR_GD_MASK___SHIFT	0
#define RUDRA40_OPTICS_ISM_SFP_PWR_GD____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_ISM_SFP_PWR_GD_2 ---- */
#define RUDRA40_OPTICS_ISM_SFP_PWR_GD_2____WIDTH	32
#define RUDRA40_OPTICS_ISM_SFP_PWR_GD_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_ISM_SFP_PWR_GD_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_ISM_SFP_PWR_GD_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_ISM_SFP_PWR_GD_2_MASK___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_ISM_SFP_PWR_GD_2_MASK___SHIFT    	0
#define RUDRA40_OPTICS_ISM_SFP_PWR_GD_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_STATUS_SFP_PWR_GD ---- */
#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD_STAT___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD_STAT___SHIFT	0
#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_STATUS_SFP_PWR_GD_2 ---- */
#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD_2____WIDTH	32
#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD_2_STAT___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD_2_STAT___SHIFT    	0
#define RUDRA40_OPTICS_STATUS_SFP_PWR_GD_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_IST_SFP_PWR_GD ---- */
#define RUDRA40_OPTICS_IST_SFP_PWR_GD____WIDTH	32
#define RUDRA40_OPTICS_IST_SFP_PWR_GD____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFP_PWR_GD_TST___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_IST_SFP_PWR_GD_TST___SHIFT	0
#define RUDRA40_OPTICS_IST_SFP_PWR_GD____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_IST_SFP_PWR_GD_2 ---- */
#define RUDRA40_OPTICS_IST_SFP_PWR_GD_2____WIDTH	32
#define RUDRA40_OPTICS_IST_SFP_PWR_GD_2____TYPE 	uint32_t

#define RUDRA40_OPTICS_IST_SFP_PWR_GD_2_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_IST_SFP_PWR_GD_2_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_IST_SFP_PWR_GD_2_TST___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_IST_SFP_PWR_GD_2_TST___SHIFT     	0
#define RUDRA40_OPTICS_IST_SFP_PWR_GD_2____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_SFP_LOW_PWR_0 ---- */
#define RUDRA40_OPTICS_SFP_LOW_PWR_0____WIDTH	32
#define RUDRA40_OPTICS_SFP_LOW_PWR_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFP_LOW_PWR_0_low___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_SFP_LOW_PWR_0_low___SHIFT	0
#define RUDRA40_OPTICS_SFP_LOW_PWR_0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_SFP_LOW_PWR_1 ---- */
#define RUDRA40_OPTICS_SFP_LOW_PWR_1____WIDTH	32
#define RUDRA40_OPTICS_SFP_LOW_PWR_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFP_LOW_PWR_1_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_SFP_LOW_PWR_1_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_SFP_LOW_PWR_1_low___MASK      	UINT32_C(0xff)
#define RUDRA40_OPTICS_SFP_LOW_PWR_1_low___SHIFT     	0
#define RUDRA40_OPTICS_SFP_LOW_PWR_1____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_SFP_RESET_0 ---- */
#define RUDRA40_OPTICS_SFP_RESET_0____WIDTH	32
#define RUDRA40_OPTICS_SFP_RESET_0____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFP_RESET_0_rstn___MASK 	UINT32_C(0xffffffff)
#define RUDRA40_OPTICS_SFP_RESET_0_rstn___SHIFT	0
#define RUDRA40_OPTICS_SFP_RESET_0____REGMASK	UINT32_C(4294967295)

/* ---- RUDRA40_OPTICS_SFP_RESET_1 ---- */
#define RUDRA40_OPTICS_SFP_RESET_1____WIDTH	32
#define RUDRA40_OPTICS_SFP_RESET_1____TYPE 	uint32_t

#define RUDRA40_OPTICS_SFP_RESET_1_Unused_8___MASK 	UINT32_C(0xffffff00)
#define RUDRA40_OPTICS_SFP_RESET_1_Unused_8___SHIFT	8
#define RUDRA40_OPTICS_SFP_RESET_1_rstn___MASK     	UINT32_C(0xff)
#define RUDRA40_OPTICS_SFP_RESET_1_rstn___SHIFT    	0
#define RUDRA40_OPTICS_SFP_RESET_1____REGMASK	UINT32_C(255)

/* ---- RUDRA40_OPTICS_THEEND ---- */
#define RUDRA40_OPTICS_THEEND____WIDTH	32
#define RUDRA40_OPTICS_THEEND____TYPE 	uint32_t

#define RUDRA40_OPTICS_THEEND____REGMASK	UINT32_C(0)

#ifdef __KERNEL__
#ifdef CONFIG_REGMAP_MMIO

static const struct regmap_config rudra40_regmap_config = {
        .reg_bits     = 32,
        .val_bits     = 32,
        .reg_stride   = 4,
        .max_register = sizeof(struct Rudra40_dev_reg),
};

#ifndef UINT16_C
#define UINT16_C(c)  __UINT16_C(c)
#endif
#ifndef UINT32_C
#define UINT32_C(c)  __UINT32_C(c)
#endif

#endif /* CONFIG_REGMAP_MMIO */
#endif /* __KERNEL__ */
#endif
