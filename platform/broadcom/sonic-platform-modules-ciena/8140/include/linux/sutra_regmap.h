#ifndef SUTRA_REGMAP_H
#define SUTRA_REGMAP_H

#ifdef __KERNEL__
#ifdef CONFIG_REGMAP_MMIO
#include <linux/regmap.h>
#endif
#else
#include <stddef.h>
#include <inttypes.h>
#endif

struct Sutra_dev_reg
{
    uint8_t    SUTRA_BASE_PID;             /* Product ID - 0x03 = CES product */
    uint8_t    SUTRA_BASE_DID;             /* Device ID - 0x39 = Sutra */
    uint8_t    SUTRA_BASE_MJR;             /* Major revision number - */
    uint8_t    SUTRA_BASE_MNR;             /* Minor revision number - */
    uint8_t    SUTRA_BASE_BLD;             /* Build revision number - */
    uint8_t    SUTRA_BASE_DATE;            /* Day the load was built (0x1-0x31 .. no hex alphas) */
    uint8_t    SUTRA_BASE_MONTH;           /* Month the load was built (0x1-0x12 .. no hex alphas) */
    uint8_t    SUTRA_BASE_SCR;             /* Scratch pad */
    uint8_t    SUTRA_BASE_RESERVED;        /* Unused */
    uint8_t    SUTRA_BASE_MPU_BRD_RESET;   /* Board Power Sequencing Trigger */
    uint8_t    SUTRA_BASE_ENDIANESS;       /* Endianess of Data */
    uint8_t    SUTRA_BASE_BRD_ID;          /* Board ID/Revision & load indicator, used to identify which board is using the Sutra design */
    uint8_t    SUTRA_BASE_LOAD_TYPE;       /* Reserved for hardware uses */
    uint8_t pad1[239];

    uint8_t    SUTRA_BASE_THEEND;          /* Reserved */
    uint8_t pad2[3];

    uint8_t    SUTRA_ABR_BMC_ABR_RESERVED_1; /* RESERVED_1 */
    uint8_t    SUTRA_ABR_BMC_ABR_RESERVED_2; /* RESERVED_2 */
    uint8_t    SUTRA_ABR_BMC_ABR_RESERVED_3; /* RESERVED_3 */
    uint8_t pad3[253];

    uint8_t    SUTRA_GLUE_SOFT_RESET;      /* Software Requested Register Map reset */
    uint8_t    SUTRA_GLUE_RESET_MASK;      /* Mask Register for card reset sources */
    uint8_t    SUTRA_GLUE_RESET_BUTTON;    /* Reset Button Monitor & SW test */
    uint8_t    SUTRA_GLUE_GENERAL_STATUS;  /* General Board Status */
    uint8_t    SUTRA_GLUE_PWR_CTL_1;       /* S/W controlled Power controls. For DEBUG use only. */
    uint8_t    SUTRA_GLUE_PWR_CTL_2;       /* S/W controlled Power controls. For DEBUG use only. */
    uint8_t    SUTRA_GLUE_PWR_CTL_3;       /* S/W controlled Power controls. For DEBUG use only. */
    uint8_t    SUTRA_GLUE_PWR_CTL_4;       /* S/W controlled Power controls. For DEBUG use only. */
    uint8_t    SUTRA_GLUE_DEVICE_RESET;    /* Reset Controls. */
    uint8_t    SUTRA_GLUE_GENERAL_CTL;     /* General Board Controls */
    uint8_t    SUTRA_GLUE_LED_SYS_STATUS_0; /* Front Panel LED controls */
    uint8_t    SUTRA_GLUE_LED_SYS_STATUS_1; /* Power Supply LEDs controls. Set bit 2 of LED_SYS_STATUS_0 to enable these OVERRIDE controls. */
    uint8_t    SUTRA_GLUE_LED_SYS_STATUS_2; /* Alarm and GNSS LEDs controls */
    uint8_t    SUTRA_GLUE_LED_SYS_STATUS_3; /* Sync and Status LED controls */
    uint8_t    SUTRA_GLUE_RESERVED_1;      /* Main regmap Interrupt Control Register */
    uint8_t    SUTRA_GLUE_IST_MASTER_EVENT; /* Interrupt Test Debug register */
    uint8_t    SUTRA_GLUE_ISR_MASTER_EVENT; /* Master Interrupt Event Register */
    uint8_t    SUTRA_GLUE_ISM_MASTER_EVENT; /* Master Interrupt Mask Register */
    uint8_t    SUTRA_GLUE_ISR_SW_I2C;      /* Interrupt from State change for SW I2C Masters */
    uint8_t    SUTRA_GLUE_ISM_SW_I2C;      /* Interrupt Mask Register for SW I2C Masters */
    uint8_t    SUTRA_GLUE_ISR_MISC;        /* Interrupt from State change for Miscellaneous Elements. Note: STATUS_MISC is later in regmap */
    uint8_t    SUTRA_GLUE_ISM_MISC;        /* Interrupt Mask Register for Miscellaneous Elements */
    uint8_t    SUTRA_GLUE_ISR_PWR;         /* Interrupt from State change for STATUS_PWR */
    uint8_t    SUTRA_GLUE_ISM_PWR;         /* Interrupt Mask Register for STATUS_PWR */
    uint8_t    SUTRA_GLUE_STATUS_PWR;      /* Power Entry Board Status */
    uint8_t    SUTRA_GLUE_I2C_SW_IF_SEL;   /* SW I2C Interface Select */
    uint8_t    SUTRA_GLUE_I2C_SW_MISC_CTRL_0; /* SW I2C clock Control and Interface Select */
    uint8_t    SUTRA_GLUE_I2C_SW_MISC_CTRL_1; /* SW I2C clock Control and Interface Select */
    uint8_t    SUTRA_GLUE_ISR_BUTTON;      /* Interrupt from Board Reset Button */
    uint8_t    SUTRA_GLUE_ISM_BUTTON;      /* Interrupt Mask Register for Board Reset Button */
    uint8_t    SUTRA_GLUE_STATUS_BUTTON;   /* Current Status of Board Reset Button */
    uint8_t    SUTRA_GLUE_STATUS_MISC;     /* Status for Miscellaneous Elements. */
    uint8_t    SUTRA_GLUE_PWR_INPUT_DEBOUNCE; /* HW TEST ONLY! This value allows variable debounce filtering of the power supply input signals. */
    uint8_t    SUTRA_GLUE_IST_SW_I2C;      /* Interrupt Test Debug register */
    uint8_t    SUTRA_GLUE_IST_MISC;        /* Interrupt Test Debug register */
    uint8_t    SUTRA_GLUE_IST_PWR;         /* Interrupt Test Debug register */
    uint8_t    SUTRA_GLUE_IST_BUTTON;      /* Interrupt Test Debug register */
    uint8_t    SUTRA_GLUE_DEBUG_CPU_WDT;   /* Debug Test mode only. Allows debug of the cpu Hardware Watchdog Timer */
    uint8_t    SUTRA_GLUE_SW_TEST_PWR_0;   /* SW Test overrides/controls for Power Entry Board inputs */
    uint8_t    SUTRA_GLUE_SW_TEST_PWR_1;   /* SW Test overrides/controls for Power Entry Board inputs */
    uint8_t    SUTRA_GLUE_UART_SEL;        /* Uart from faceplate connected to 1 of 7 uarts */
    uint8_t    SUTRA_GLUE_PWRGOOD_STATUS_0; /* Power good for pwr_en */
    uint8_t    SUTRA_GLUE_PWRGOOD_STATUS_1; /* Power good for pwr_en */
    uint8_t    SUTRA_GLUE_PWRGOOD_STATUS_2; /* Power good for pwr_en */
    uint8_t    SUTRA_GLUE_PWRGOOD_STATUS_3; /* Power good for pwr_en */
    uint8_t    SUTRA_GLUE_MISC_CTRL;       /* Register for miscellaneous control */
    uint8_t    SUTRA_GLUE_MISC_STATUS;     /* Status Register for Miscellaneous signals */
    uint8_t    SUTRA_GLUE_UART_SWITCHING_STATUS; /* Uart switching algo status register */
    uint8_t    SUTRA_GLUE_UART_STATUS;     /* Uart switching algo status register */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA0; /* Data received from BMC over the SGPIO interface. */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA1; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA2; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA3; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA4; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA5; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA6; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA7; /*  */
    uint8_t    SUTRA_GLUE_RESERVED_2;      /* Reserved for future use */
    uint8_t    SUTRA_GLUE_TEST_SGPIO_CTL;  /* Control register to test SGPIO interface. */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG0; /* Data sent from Sutra to BMC over the SGPIO interface. */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG1; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG2; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG3; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG4; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG5; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG6; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG7; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA0; /* Data received from RUDRA over the SGPIO interface. */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA1; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA2; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA3; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA4; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA5; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA6; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA7; /*  */
    uint8_t    SUTRA_GLUE_RESERVED_3;      /* Reserved for future use */
    uint8_t    SUTRA_GLUE_RESERVED_4;      /* Reserved for future use */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG0; /* Data sent from Sutra to RUDRA over the SGPIO interface. */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG1; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG2; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG3; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG4; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG5; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG6; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG7; /*  */
    uint8_t    SUTRA_GLUE_SFP_TX_DISABLE;  /* SFP[3:0] Transmit Disable */
    uint8_t    SUTRA_GLUE_ISR_SFP_TX_FAULT; /* Interrupt from State change for STATUS_SFP_TX_FAULT */
    uint8_t    SUTRA_GLUE_ISM_SFP_TX_FAULT; /* Interrupt Mask Register for STATUS_SFP_TX_FAULT */
    uint8_t    SUTRA_GLUE_STATUS_SFP_TX_FAULT; /* Status of SFP+ Tx Fault signal */
    uint8_t    SUTRA_GLUE_IST_SFP_TX_FAULT; /* Interrupt Test Debug register */
    uint8_t    SUTRA_GLUE_PS_I2C_MUX_SEL;  /* Mux Selection Register for PS I2C Bus */
    uint8_t    SUTRA_GLUE_PSU_CONTROL;     /* Drive PSU A and B Control Pins */
    uint8_t    SUTRA_GLUE_PSU_STATUS;      /* Status bits information of Power Supply A and B */
    uint8_t    SUTRA_GLUE_DEBUG_REG1;      /* Debug register */
    uint8_t    SUTRA_GLUE_DEBUG_REG2;      /* Debug register */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER0; /* Data received from BMC over the SGPIO interface. Higher 64 bits. */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER1; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER2; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER3; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER4; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER5; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER6; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER7; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG0; /* Data sent from Sutra to BMC over the SGPIO interface. Higher 64 bits. */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG1; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG2; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG3; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG4; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG5; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG6; /*  */
    uint8_t    SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG7; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER0; /* Data received from RUDRA over the SGPIO interface. Higher 64 bits. */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER1; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER2; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER3; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER4; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER5; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER6; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER7; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG0; /* Data sent from Sutra to RUDRA over the SGPIO interface. Higher 64 bits. */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG1; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG2; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG3; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG4; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG5; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG6; /*  */
    uint8_t    SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG7; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0; /* Test frame to be sent on SGPIO interface */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO4; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO5; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO6; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO7; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO8; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO9; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO10; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO11; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO12; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO13; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO14; /*  */
    uint8_t    SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO15; /*  */
    uint8_t    SUTRA_GLUE_SPI_ENABLE;      /* SPI Transaction Enable Trigger */
    uint8_t    SUTRA_GLUE_THEEND;          /* Reserved */
    uint8_t pad4[367];

    uint8_t    SUTRA_MAIN_I2C_SW_CTRL;     /* I2C Bus Control Register for Mainboard ADC, IDP EEPROM and USB Mux */
    uint8_t    SUTRA_MAIN_I2C_SW_STAT;     /* I2C Bus Status Register */
    uint8_t    SUTRA_MAIN_I2C_SW_WRITE_DATA; /* I2C Bus Write Data Register */
    uint8_t    SUTRA_MAIN_I2C_SW_READ_DATA; /* I2C Bus Read Data Register */
    uint8_t    SUTRA_MAIN_I2C_SW_DONE;     /* I2C Bus Completion Status Register */
    uint8_t    SUTRA_MAIN_I2C_DIAG_CTRL;   /* : I2C Bus Control Register for Mainboard ADC, IDP EEPROM and USB Mux */
    uint8_t    SUTRA_MAIN_I2C_DIAG_4X;     /* clock divider factor (for HW use only) based on 50mhz clock source */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DEVADDR; /* Device Address Register used for I2C cycle, bit 0 is the Page bit for page operations on the AT24CM01 on this bus */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DATAADDR_0; /* Data Address Register used for I2C cycle. */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DATAADDR_1; /* Data Address Register used for I2C cycle. */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DATA_VLD_0; /* Byte Valid Register */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DEBUG_0; /* Debug register */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DATA_WR0; /* Write Data Registers */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DATA_WR1; /*  */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DATA_WR2; /*  */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DATA_WR3; /*  */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DATA_RD0; /* Read Data Registers */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DATA_RD1; /*  */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DATA_RD2; /*  */
    uint8_t    SUTRA_MAIN_I2C_DIAG_DATA_RD3; /*  */
    uint8_t pad5[492];

    uint8_t    SUTRA_MORE_I2C_PS_SW_CTRL;  /* I2C SW control Register for accessing Power Sppplies PMBUS and its EEPROM */
    uint8_t    SUTRA_MORE_I2C_PS_SW_STAT;  /* I2C Bus Status Register */
    uint8_t    SUTRA_MORE_I2C_PS_SW_WRITE_DATA; /* I2C Bus Write Data Register */
    uint8_t    SUTRA_MORE_I2C_PS_SW_READ_DATA; /* I2C Bus Read Data Register */
    uint8_t    SUTRA_MORE_I2C_PS_SW_DONE;  /* I2C Bus Completion Status Register */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_CTRL; /* I2C Diag control Register for accessing Power Sppplies PMBUS and its EEPROM */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_4X;  /* clock divider factor (for HW use only) based on 50mhz clock source */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DEVADDR; /* Device Address Register used for I2C cycle, bit 0 is the Page bit for page operations on the AT24CM01 on this bus */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DATAADDR_0; /* Data Address Register used for I2C cycle. */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DATAADDR_1; /* Data Address Register used for I2C cycle. */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DATA_VLD_0; /* Byte Valid Register */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DEBUG_0; /* Debug register */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DATA_WR0; /* Write Data Registers */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DATA_WR1; /*  */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DATA_WR2; /*  */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DATA_WR3; /*  */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DATA_RD0; /* Read Data Registers */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DATA_RD1; /*  */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DATA_RD2; /*  */
    uint8_t    SUTRA_MORE_I2C_PS_DIAG_DATA_RD3; /*  */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_SW_CTRL; /* I2C Bus Control Register for accessing PCIE Switch I2C */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_SW_STAT; /* I2C Bus Status Register */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_SW_WRITE_DATA; /* I2C Bus Write Data Register */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_SW_READ_DATA; /* I2C Bus Read Data Register */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_SW_DONE; /* I2C Bus Completion Status Register */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL; /* I2C Bus Control Register for accessing PCIE Switch I2C */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_4X; /* clock divider factor (for HW use only) based on 50mhz clock source */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DEVADDR; /* Device Address Register used for I2C cycle, bit 0 is the Page bit for page operations on the AT24CM01 on this bus */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATAADDR_0; /* Data Address Register used for I2C cycle. */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATAADDR_1; /* Data Address Register used for I2C cycle. */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_VLD_0; /* Byte Valid Register */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DEBUG_0; /* Debug register */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR0; /* Write Data Registers */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR1; /*  */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR2; /*  */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR3; /*  */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD0; /* Read Data Registers */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD1; /*  */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD2; /*  */
    uint8_t    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD3; /*  */
    uint8_t    SUTRA_MORE_I2C_PMBUS_SW_CTRL; /* I2C SW control Register for accessing Power Sppplies PMBUS and its EEPROM */
    uint8_t    SUTRA_MORE_I2C_PMBUS_SW_STAT; /* I2C Bus Status Register */
    uint8_t    SUTRA_MORE_I2C_PMBUS_SW_WRITE_DATA; /* I2C Bus Write Data Register */
    uint8_t    SUTRA_MORE_I2C_PMBUS_SW_READ_DATA; /* I2C Bus Read Data Register */
    uint8_t    SUTRA_MORE_I2C_PMBUS_SW_DONE; /* I2C Bus Completion Status Register */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_CTRL; /* I2C Diag control Register for accessing Power Sppplies PMBUS and its EEPROM */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_4X; /* clock divider factor (for HW use only) based on 50mhz clock source */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DEVADDR; /* Device Address Register used for I2C cycle, bit 0 is the Page bit for page operations on the AT24CM01 on this bus */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DATAADDR_0; /* Data Address Register used for I2C cycle. */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DATAADDR_1; /* Data Address Register used for I2C cycle. */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DATA_VLD_0; /* Byte Valid Register */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DEBUG_0; /* Debug register */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR0; /* Write Data Registers */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR1; /*  */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR2; /*  */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR3; /*  */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD0; /* Read Data Registers */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD1; /*  */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD2; /*  */
    uint8_t    SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD3; /*  */
    uint8_t    SUTRA_MORE_I2C_THEEND;      /* Reserved */
} __attribute__ ((__packed__, __aligned__(1)));

#define SUTRA_REG_PTR(base, reg)           (&(base)->reg)
#define SUTRA_REG_OFFSET(reg)              offsetof(struct Sutra_dev_reg, reg)
#define SUTRA_REG_INDEX(reg)               (SUTRA_REG_OFFSET(reg)/sizeof(uint8_t))
#define SUTRA_REG_WIDTH(reg)               SUTRA_JOIN(reg,, _WIDTH)
#define SUTRA_REG_TYPE(reg)                SUTRA_JOIN(reg,, _TYPE)
#define SUTRA_REG_VALUE(reg, val)          SUTRA_JOIN(reg,, val)
#define SUTRA_FIELD_MASK(reg, field)       SUTRA_JOIN(reg, field, _MASK)
#define SUTRA_FIELD_SHIFT(reg, field)      SUTRA_JOIN(reg, field, _SHIFT)
#define SUTRA_FIELD_VALUE(reg, field, val) SUTRA_JOIN(reg, field, val)
#define SUTRA_JOIN(reg, field, suffix) reg ## _ ## field ## __ ## suffix

#define SUTRA_GET_BITFIELD(regval, mask, shift) \
    ( ((regval)&(mask)) >> (shift) )

#define SUTRA_SET_BITFIELD(regval, mask, shift, value) \
    ( ((regval) & ~(mask)) | (((value)<<(shift)) & (mask)) )


/* ---- SUTRA_BASE_PID ---- */
#define SUTRA_BASE_PID____WIDTH	8
#define SUTRA_BASE_PID____TYPE 	uint8_t

#define SUTRA_BASE_PID____REGMASK	UINT8_C(0)

/* ---- SUTRA_BASE_DID ---- */
#define SUTRA_BASE_DID____WIDTH	8
#define SUTRA_BASE_DID____TYPE 	uint8_t

#define SUTRA_BASE_DID____REGMASK	UINT8_C(0)

/* ---- SUTRA_BASE_MJR ---- */
#define SUTRA_BASE_MJR____WIDTH	8
#define SUTRA_BASE_MJR____TYPE 	uint8_t

#define SUTRA_BASE_MJR____REGMASK	UINT8_C(0)

/* ---- SUTRA_BASE_MNR ---- */
#define SUTRA_BASE_MNR____WIDTH	8
#define SUTRA_BASE_MNR____TYPE 	uint8_t

#define SUTRA_BASE_MNR____REGMASK	UINT8_C(0)

/* ---- SUTRA_BASE_BLD ---- */
#define SUTRA_BASE_BLD____WIDTH	8
#define SUTRA_BASE_BLD____TYPE 	uint8_t

#define SUTRA_BASE_BLD____REGMASK	UINT8_C(0)

/* ---- SUTRA_BASE_DATE ---- */
#define SUTRA_BASE_DATE____WIDTH	8
#define SUTRA_BASE_DATE____TYPE 	uint8_t

#define SUTRA_BASE_DATE____REGMASK	UINT8_C(0)

/* ---- SUTRA_BASE_MONTH ---- */
#define SUTRA_BASE_MONTH____WIDTH	8
#define SUTRA_BASE_MONTH____TYPE 	uint8_t

#define SUTRA_BASE_MONTH____REGMASK	UINT8_C(0)

/* ---- SUTRA_BASE_SCR ---- */
#define SUTRA_BASE_SCR____WIDTH	8
#define SUTRA_BASE_SCR____TYPE 	uint8_t

#define SUTRA_BASE_SCR____REGMASK	UINT8_C(0)

/* ---- SUTRA_BASE_RESERVED ---- */
#define SUTRA_BASE_RESERVED____WIDTH	8
#define SUTRA_BASE_RESERVED____TYPE 	uint8_t

#define SUTRA_BASE_RESERVED____REGMASK	UINT8_C(0)

/* ---- SUTRA_BASE_MPU_BRD_RESET ---- */
#define SUTRA_BASE_MPU_BRD_RESET____WIDTH	8
#define SUTRA_BASE_MPU_BRD_RESET____TYPE 	uint8_t

#define SUTRA_BASE_MPU_BRD_RESET_board_reset_trigger___MASK 	UINT8_C(0xff)
#define SUTRA_BASE_MPU_BRD_RESET_board_reset_trigger___SHIFT	0
#define SUTRA_BASE_MPU_BRD_RESET____REGMASK	UINT8_C(255)

/* ---- SUTRA_BASE_ENDIANESS ---- */
#define SUTRA_BASE_ENDIANESS____WIDTH	8
#define SUTRA_BASE_ENDIANESS____TYPE 	uint8_t

#define SUTRA_BASE_ENDIANESS_Unused_1___MASK    	UINT8_C(0xfe)
#define SUTRA_BASE_ENDIANESS_Unused_1___SHIFT   	1
#define SUTRA_BASE_ENDIANESS_SWAP_ENDIAN___MASK 	UINT8_C(0x1)
#define SUTRA_BASE_ENDIANESS_SWAP_ENDIAN___SHIFT	0
#define SUTRA_BASE_ENDIANESS____REGMASK	UINT8_C(1)

/* ---- SUTRA_BASE_BRD_ID ---- */
#define SUTRA_BASE_BRD_ID____WIDTH	8
#define SUTRA_BASE_BRD_ID____TYPE 	uint8_t

#define SUTRA_BASE_BRD_ID_brd_rev___MASK 	UINT8_C(0xc0)
#define SUTRA_BASE_BRD_ID_brd_rev___SHIFT	6
#define SUTRA_BASE_BRD_ID_brd_id___MASK  	UINT8_C(0x3f)
#define SUTRA_BASE_BRD_ID_brd_id___SHIFT 	0
#define SUTRA_BASE_BRD_ID____REGMASK	UINT8_C(255)

/* ---- SUTRA_BASE_LOAD_TYPE ---- */
#define SUTRA_BASE_LOAD_TYPE____WIDTH	8
#define SUTRA_BASE_LOAD_TYPE____TYPE 	uint8_t

#define SUTRA_BASE_LOAD_TYPE____REGMASK	UINT8_C(0)

/* ---- SUTRA_BASE_THEEND ---- */
#define SUTRA_BASE_THEEND____WIDTH	8
#define SUTRA_BASE_THEEND____TYPE 	uint8_t

#define SUTRA_BASE_THEEND____REGMASK	UINT8_C(0)

/* ---- SUTRA_ABR_BMC_ABR_RESERVED_1 ---- */
#define SUTRA_ABR_BMC_ABR_RESERVED_1____WIDTH	8
#define SUTRA_ABR_BMC_ABR_RESERVED_1____TYPE 	uint8_t

#define SUTRA_ABR_BMC_ABR_RESERVED_1____REGMASK	UINT8_C(0)

/* ---- SUTRA_ABR_BMC_ABR_RESERVED_2 ---- */
#define SUTRA_ABR_BMC_ABR_RESERVED_2____WIDTH	8
#define SUTRA_ABR_BMC_ABR_RESERVED_2____TYPE 	uint8_t

#define SUTRA_ABR_BMC_ABR_RESERVED_2____REGMASK	UINT8_C(0)

/* ---- SUTRA_ABR_BMC_ABR_RESERVED_3 ---- */
#define SUTRA_ABR_BMC_ABR_RESERVED_3____WIDTH	8
#define SUTRA_ABR_BMC_ABR_RESERVED_3____TYPE 	uint8_t

#define SUTRA_ABR_BMC_ABR_RESERVED_3____REGMASK	UINT8_C(0)

/* ---- SUTRA_GLUE_SOFT_RESET ---- */
#define SUTRA_GLUE_SOFT_RESET____WIDTH	8
#define SUTRA_GLUE_SOFT_RESET____TYPE 	uint8_t

#define SUTRA_GLUE_SOFT_RESET_Unused_1___MASK 	UINT8_C(0xfe)
#define SUTRA_GLUE_SOFT_RESET_Unused_1___SHIFT	1
#define SUTRA_GLUE_SOFT_RESET_RST___MASK      	UINT8_C(0x1)
#define SUTRA_GLUE_SOFT_RESET_RST___SHIFT     	0
#define SUTRA_GLUE_SOFT_RESET____REGMASK	UINT8_C(1)

/* ---- SUTRA_GLUE_RESET_MASK ---- */
#define SUTRA_GLUE_RESET_MASK____WIDTH	8
#define SUTRA_GLUE_RESET_MASK____TYPE 	uint8_t

#define SUTRA_GLUE_RESET_MASK_MASK_CPU_PLTRST_FOR_SGPIO_THERMTRIP___MASK 	UINT8_C(0x80)
#define SUTRA_GLUE_RESET_MASK_MASK_CPU_PLTRST_FOR_SGPIO_THERMTRIP___SHIFT	7
#define SUTRA_GLUE_RESET_MASK_MASK_CPU_PLTRST_FOR_PCIESW_RST___MASK      	UINT8_C(0x40)
#define SUTRA_GLUE_RESET_MASK_MASK_CPU_PLTRST_FOR_PCIESW_RST___SHIFT     	6
#define SUTRA_GLUE_RESET_MASK_Unused_5___MASK                            	UINT8_C(0x20)
#define SUTRA_GLUE_RESET_MASK_Unused_5___SHIFT                           	5
#define SUTRA_GLUE_RESET_MASK_HW_WD_TIMER_SHORT___MASK                   	UINT8_C(0x10)
#define SUTRA_GLUE_RESET_MASK_HW_WD_TIMER_SHORT___SHIFT                  	4
#define SUTRA_GLUE_RESET_MASK_HW_WD_TIMER_MASK___MASK                    	UINT8_C(0x8)
#define SUTRA_GLUE_RESET_MASK_HW_WD_TIMER_MASK___SHIFT                   	3
#define SUTRA_GLUE_RESET_MASK_MASK_RUDRA_THERMAL_BIT_SHUTDOWN___MASK     	UINT8_C(0x4)
#define SUTRA_GLUE_RESET_MASK_MASK_RUDRA_THERMAL_BIT_SHUTDOWN___SHIFT    	2
#define SUTRA_GLUE_RESET_MASK_PB_RESET_TO_SGPIO_MASK___MASK              	UINT8_C(0x2)
#define SUTRA_GLUE_RESET_MASK_PB_RESET_TO_SGPIO_MASK___SHIFT             	1
#define SUTRA_GLUE_RESET_MASK_PB_RESET_MASK___MASK                       	UINT8_C(0x1)
#define SUTRA_GLUE_RESET_MASK_PB_RESET_MASK___SHIFT                      	0
#define SUTRA_GLUE_RESET_MASK____REGMASK	UINT8_C(223)

/* ---- SUTRA_GLUE_RESET_BUTTON ---- */
#define SUTRA_GLUE_RESET_BUTTON____WIDTH	8
#define SUTRA_GLUE_RESET_BUTTON____TYPE 	uint8_t

#define SUTRA_GLUE_RESET_BUTTON_SW_CAUSED_RESET___MASK             	UINT8_C(0x80)
#define SUTRA_GLUE_RESET_BUTTON_SW_CAUSED_RESET___SHIFT            	7
#define SUTRA_GLUE_RESET_BUTTON_PB_RESET_SW_TRIGGER___MASK         	UINT8_C(0x40)
#define SUTRA_GLUE_RESET_BUTTON_PB_RESET_SW_TRIGGER___SHIFT        	6
#define SUTRA_GLUE_RESET_BUTTON_HW_WD_TIMER_CLEAR___MASK           	UINT8_C(0x20)
#define SUTRA_GLUE_RESET_BUTTON_HW_WD_TIMER_CLEAR___SHIFT          	5
#define SUTRA_GLUE_RESET_BUTTON_Unused_4___MASK                    	UINT8_C(0x10)
#define SUTRA_GLUE_RESET_BUTTON_Unused_4___SHIFT                   	4
#define SUTRA_GLUE_RESET_BUTTON_Reserved___MASK                    	UINT8_C(0x8)
#define SUTRA_GLUE_RESET_BUTTON_Reserved___SHIFT                   	3
#define SUTRA_GLUE_RESET_BUTTON_PB_RESET_PRESS_DETECT___MASK       	UINT8_C(0x4)
#define SUTRA_GLUE_RESET_BUTTON_PB_RESET_PRESS_DETECT___SHIFT      	2
#define SUTRA_GLUE_RESET_BUTTON_PB_RESET_HELD_FOR_3_SECONDS___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_RESET_BUTTON_PB_RESET_HELD_FOR_3_SECONDS___SHIFT	1
#define SUTRA_GLUE_RESET_BUTTON_PB_RESET_REAL_TIME_STATE___MASK    	UINT8_C(0x1)
#define SUTRA_GLUE_RESET_BUTTON_PB_RESET_REAL_TIME_STATE___SHIFT   	0
#define SUTRA_GLUE_RESET_BUTTON____REGMASK	UINT8_C(239)

/* ---- SUTRA_GLUE_GENERAL_STATUS ---- */
#define SUTRA_GLUE_GENERAL_STATUS____WIDTH	8
#define SUTRA_GLUE_GENERAL_STATUS____TYPE 	uint8_t

#define SUTRA_GLUE_GENERAL_STATUS_BIOS_BOOT_SELECT_FINAL_STATUS___MASK 	UINT8_C(0x80)
#define SUTRA_GLUE_GENERAL_STATUS_BIOS_BOOT_SELECT_FINAL_STATUS___SHIFT	7
#define SUTRA_GLUE_GENERAL_STATUS_ALL_PWR_GOOD___MASK                  	UINT8_C(0x40)
#define SUTRA_GLUE_GENERAL_STATUS_ALL_PWR_GOOD___SHIFT                 	6
#define SUTRA_GLUE_GENERAL_STATUS_Unused_1___MASK                      	UINT8_C(0x3e)
#define SUTRA_GLUE_GENERAL_STATUS_Unused_1___SHIFT                     	1
#define SUTRA_GLUE_GENERAL_STATUS_PCIEX_CPLD_AVS_ENB___MASK            	UINT8_C(0x1)
#define SUTRA_GLUE_GENERAL_STATUS_PCIEX_CPLD_AVS_ENB___SHIFT           	0
#define SUTRA_GLUE_GENERAL_STATUS____REGMASK	UINT8_C(193)

/* ---- SUTRA_GLUE_PWR_CTL_1 ---- */
#define SUTRA_GLUE_PWR_CTL_1____WIDTH	8
#define SUTRA_GLUE_PWR_CTL_1____TYPE 	uint8_t

#define SUTRA_GLUE_PWR_CTL_1_Unused_5___MASK   	UINT8_C(0xe0)
#define SUTRA_GLUE_PWR_CTL_1_Unused_5___SHIFT  	5
#define SUTRA_GLUE_PWR_CTL_1_reserved4___MASK  	UINT8_C(0x10)
#define SUTRA_GLUE_PWR_CTL_1_reserved4___SHIFT 	4
#define SUTRA_GLUE_PWR_CTL_1_SW_CTL_EN___MASK  	UINT8_C(0x8)
#define SUTRA_GLUE_PWR_CTL_1_SW_CTL_EN___SHIFT 	3
#define SUTRA_GLUE_PWR_CTL_1_reserved3___MASK  	UINT8_C(0x4)
#define SUTRA_GLUE_PWR_CTL_1_reserved3___SHIFT 	2
#define SUTRA_GLUE_PWR_CTL_1_reserved2___MASK  	UINT8_C(0x2)
#define SUTRA_GLUE_PWR_CTL_1_reserved2___SHIFT 	1
#define SUTRA_GLUE_PWR_CTL_1_PWRGD_DRAM___MASK 	UINT8_C(0x1)
#define SUTRA_GLUE_PWR_CTL_1_PWRGD_DRAM___SHIFT	0
#define SUTRA_GLUE_PWR_CTL_1____REGMASK	UINT8_C(31)

/* ---- SUTRA_GLUE_PWR_CTL_2 ---- */
#define SUTRA_GLUE_PWR_CTL_2____WIDTH	8
#define SUTRA_GLUE_PWR_CTL_2____TYPE 	uint8_t

#define SUTRA_GLUE_PWR_CTL_2_PWRGD_NAC_NACHOST_LVC3___MASK 	UINT8_C(0x80)
#define SUTRA_GLUE_PWR_CTL_2_PWRGD_NAC_NACHOST_LVC3___SHIFT	7
#define SUTRA_GLUE_PWR_CTL_2_PWRGD_CPU_IN_LVC3___MASK      	UINT8_C(0x40)
#define SUTRA_GLUE_PWR_CTL_2_PWRGD_CPU_IN_LVC3___SHIFT     	6
#define SUTRA_GLUE_PWR_CTL_2_PWRGD_PCH_PWROK_INT___MASK    	UINT8_C(0x20)
#define SUTRA_GLUE_PWR_CTL_2_PWRGD_PCH_PWROK_INT___SHIFT   	5
#define SUTRA_GLUE_PWR_CTL_2_VCCIN_EN___MASK               	UINT8_C(0x10)
#define SUTRA_GLUE_PWR_CTL_2_VCCIN_EN___SHIFT              	4
#define SUTRA_GLUE_PWR_CTL_2_PVCCNA_EN___MASK              	UINT8_C(0x8)
#define SUTRA_GLUE_PWR_CTL_2_PVCCNA_EN___SHIFT             	3
#define SUTRA_GLUE_PWR_CTL_2_FPGA_USB3_PWR_EN_L___MASK     	UINT8_C(0x4)
#define SUTRA_GLUE_PWR_CTL_2_FPGA_USB3_PWR_EN_L___SHIFT    	2
#define SUTRA_GLUE_PWR_CTL_2_PCIEX_0V9_LDO_EN___MASK       	UINT8_C(0x2)
#define SUTRA_GLUE_PWR_CTL_2_PCIEX_0V9_LDO_EN___SHIFT      	1
#define SUTRA_GLUE_PWR_CTL_2_P0V6_VTT_AB_EN___MASK         	UINT8_C(0x1)
#define SUTRA_GLUE_PWR_CTL_2_P0V6_VTT_AB_EN___SHIFT        	0
#define SUTRA_GLUE_PWR_CTL_2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_PWR_CTL_3 ---- */
#define SUTRA_GLUE_PWR_CTL_3____WIDTH	8
#define SUTRA_GLUE_PWR_CTL_3____TYPE 	uint8_t

#define SUTRA_GLUE_PWR_CTL_3_PCIEX_1V8_LDO_EN___MASK   	UINT8_C(0x80)
#define SUTRA_GLUE_PWR_CTL_3_PCIEX_1V8_LDO_EN___SHIFT  	7
#define SUTRA_GLUE_PWR_CTL_3_VDDQ_AB_EN___MASK         	UINT8_C(0x40)
#define SUTRA_GLUE_PWR_CTL_3_VDDQ_AB_EN___SHIFT        	6
#define SUTRA_GLUE_PWR_CTL_3_PCIEX_0V9_DPOL_EN___MASK  	UINT8_C(0x20)
#define SUTRA_GLUE_PWR_CTL_3_PCIEX_0V9_DPOL_EN___SHIFT 	5
#define SUTRA_GLUE_PWR_CTL_3_FPGA_VPP_AB_EN___MASK     	UINT8_C(0x10)
#define SUTRA_GLUE_PWR_CTL_3_FPGA_VPP_AB_EN___SHIFT    	4
#define SUTRA_GLUE_PWR_CTL_3_FPGA_3V3_OPTICS_EN___MASK 	UINT8_C(0x8)
#define SUTRA_GLUE_PWR_CTL_3_FPGA_3V3_OPTICS_EN___SHIFT	3
#define SUTRA_GLUE_PWR_CTL_3_FPGA_1V8_NAC_EN___MASK    	UINT8_C(0x4)
#define SUTRA_GLUE_PWR_CTL_3_FPGA_1V8_NAC_EN___SHIFT   	2
#define SUTRA_GLUE_PWR_CTL_3_P1V05_EN___MASK           	UINT8_C(0x2)
#define SUTRA_GLUE_PWR_CTL_3_P1V05_EN___SHIFT          	1
#define SUTRA_GLUE_PWR_CTL_3_PVNN_PCH_EN___MASK        	UINT8_C(0x1)
#define SUTRA_GLUE_PWR_CTL_3_PVNN_PCH_EN___SHIFT       	0
#define SUTRA_GLUE_PWR_CTL_3____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_PWR_CTL_4 ---- */
#define SUTRA_GLUE_PWR_CTL_4____WIDTH	8
#define SUTRA_GLUE_PWR_CTL_4____TYPE 	uint8_t

#define SUTRA_GLUE_PWR_CTL_4_PVNN_NAC_EN___MASK     	UINT8_C(0x80)
#define SUTRA_GLUE_PWR_CTL_4_PVNN_NAC_EN___SHIFT    	7
#define SUTRA_GLUE_PWR_CTL_4_P1V8_AUX_EN___MASK     	UINT8_C(0x40)
#define SUTRA_GLUE_PWR_CTL_4_P1V8_AUX_EN___SHIFT    	6
#define SUTRA_GLUE_PWR_CTL_4_P3V3_CPU_EN___MASK     	UINT8_C(0x20)
#define SUTRA_GLUE_PWR_CTL_4_P3V3_CPU_EN___SHIFT    	5
#define SUTRA_GLUE_PWR_CTL_4_P1V0_BMC_AUX_EN___MASK 	UINT8_C(0x10)
#define SUTRA_GLUE_PWR_CTL_4_P1V0_BMC_AUX_EN___SHIFT	4
#define SUTRA_GLUE_PWR_CTL_4_P1V2_BMC_AUX_EN___MASK 	UINT8_C(0x8)
#define SUTRA_GLUE_PWR_CTL_4_P1V2_BMC_AUX_EN___SHIFT	3
#define SUTRA_GLUE_PWR_CTL_4_P3V3_RGM_EN___MASK     	UINT8_C(0x4)
#define SUTRA_GLUE_PWR_CTL_4_P3V3_RGM_EN___SHIFT    	2
#define SUTRA_GLUE_PWR_CTL_4_P1V8_BMC_AUX_EN___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_PWR_CTL_4_P1V8_BMC_AUX_EN___SHIFT	1
#define SUTRA_GLUE_PWR_CTL_4_P3V3_EN___MASK         	UINT8_C(0x1)
#define SUTRA_GLUE_PWR_CTL_4_P3V3_EN___SHIFT        	0
#define SUTRA_GLUE_PWR_CTL_4____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_DEVICE_RESET ---- */
#define SUTRA_GLUE_DEVICE_RESET____WIDTH	8
#define SUTRA_GLUE_DEVICE_RESET____TYPE 	uint8_t

#define SUTRA_GLUE_DEVICE_RESET_Unused_7___MASK          	UINT8_C(0x80)
#define SUTRA_GLUE_DEVICE_RESET_Unused_7___SHIFT         	7
#define SUTRA_GLUE_DEVICE_RESET_PCIE_QSPI_RESET_N___MASK 	UINT8_C(0x40)
#define SUTRA_GLUE_DEVICE_RESET_PCIE_QSPI_RESET_N___SHIFT	6
#define SUTRA_GLUE_DEVICE_RESET_TPM_RESET_N___MASK       	UINT8_C(0x20)
#define SUTRA_GLUE_DEVICE_RESET_TPM_RESET_N___SHIFT      	5
#define SUTRA_GLUE_DEVICE_RESET_RST_RTC_N___MASK         	UINT8_C(0x10)
#define SUTRA_GLUE_DEVICE_RESET_RST_RTC_N___SHIFT        	4
#define SUTRA_GLUE_DEVICE_RESET_RST_PERSTM2___MASK       	UINT8_C(0x8)
#define SUTRA_GLUE_DEVICE_RESET_RST_PERSTM2___SHIFT      	3
#define SUTRA_GLUE_DEVICE_RESET_MGMT_NIC_RST_N___MASK    	UINT8_C(0x4)
#define SUTRA_GLUE_DEVICE_RESET_MGMT_NIC_RST_N___SHIFT   	2
#define SUTRA_GLUE_DEVICE_RESET_CPLD_PCIEX_RSTB___MASK   	UINT8_C(0x2)
#define SUTRA_GLUE_DEVICE_RESET_CPLD_PCIEX_RSTB___SHIFT  	1
#define SUTRA_GLUE_DEVICE_RESET_BMC_EXTRST_N___MASK      	UINT8_C(0x1)
#define SUTRA_GLUE_DEVICE_RESET_BMC_EXTRST_N___SHIFT     	0
#define SUTRA_GLUE_DEVICE_RESET____REGMASK	UINT8_C(127)

/* ---- SUTRA_GLUE_GENERAL_CTL ---- */
#define SUTRA_GLUE_GENERAL_CTL____WIDTH	8
#define SUTRA_GLUE_GENERAL_CTL____TYPE 	uint8_t

#define SUTRA_GLUE_GENERAL_CTL_BIOS_SPI_INTERFACE_SEL___MASK      	UINT8_C(0x80)
#define SUTRA_GLUE_GENERAL_CTL_BIOS_SPI_INTERFACE_SEL___SHIFT     	7
#define SUTRA_GLUE_GENERAL_CTL_BMC_FWSPI_INTERFACE_SEL___MASK     	UINT8_C(0x40)
#define SUTRA_GLUE_GENERAL_CTL_BMC_FWSPI_INTERFACE_SEL___SHIFT    	6
#define SUTRA_GLUE_GENERAL_CTL_FPGA_IDP_WP___MASK                 	UINT8_C(0x20)
#define SUTRA_GLUE_GENERAL_CTL_FPGA_IDP_WP___SHIFT                	5
#define SUTRA_GLUE_GENERAL_CTL_disable_uart_switching_algo___MASK 	UINT8_C(0x10)
#define SUTRA_GLUE_GENERAL_CTL_disable_uart_switching_algo___SHIFT	4
#define SUTRA_GLUE_GENERAL_CTL_I210_DEV_OFF_N___MASK              	UINT8_C(0x8)
#define SUTRA_GLUE_GENERAL_CTL_I210_DEV_OFF_N___SHIFT             	3
#define SUTRA_GLUE_GENERAL_CTL_BMC_FLASH_SELECT___MASK            	UINT8_C(0x4)
#define SUTRA_GLUE_GENERAL_CTL_BMC_FLASH_SELECT___SHIFT           	2
#define SUTRA_GLUE_GENERAL_CTL_PCIEX_CPLD_AVS_ENB___MASK          	UINT8_C(0x2)
#define SUTRA_GLUE_GENERAL_CTL_PCIEX_CPLD_AVS_ENB___SHIFT         	1
#define SUTRA_GLUE_GENERAL_CTL_BIOS_BOOT_SELECT___MASK            	UINT8_C(0x1)
#define SUTRA_GLUE_GENERAL_CTL_BIOS_BOOT_SELECT___SHIFT           	0
#define SUTRA_GLUE_GENERAL_CTL____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_LED_SYS_STATUS_0 ---- */
#define SUTRA_GLUE_LED_SYS_STATUS_0____WIDTH	8
#define SUTRA_GLUE_LED_SYS_STATUS_0____TYPE 	uint8_t

#define SUTRA_GLUE_LED_SYS_STATUS_0_Unused_4___MASK            	UINT8_C(0xf0)
#define SUTRA_GLUE_LED_SYS_STATUS_0_Unused_4___SHIFT           	4
#define SUTRA_GLUE_LED_SYS_STATUS_0_unused___MASK              	UINT8_C(0x8)
#define SUTRA_GLUE_LED_SYS_STATUS_0_unused___SHIFT             	3
#define SUTRA_GLUE_LED_SYS_STATUS_0_sw_override_fp_leds___MASK 	UINT8_C(0x4)
#define SUTRA_GLUE_LED_SYS_STATUS_0_sw_override_fp_leds___SHIFT	2
#define SUTRA_GLUE_LED_SYS_STATUS_0_blink_all_leds___MASK      	UINT8_C(0x2)
#define SUTRA_GLUE_LED_SYS_STATUS_0_blink_all_leds___SHIFT     	1
#define SUTRA_GLUE_LED_SYS_STATUS_0_enable_all_leds___MASK     	UINT8_C(0x1)
#define SUTRA_GLUE_LED_SYS_STATUS_0_enable_all_leds___SHIFT    	0
#define SUTRA_GLUE_LED_SYS_STATUS_0____REGMASK	UINT8_C(15)

/* ---- SUTRA_GLUE_LED_SYS_STATUS_1 ---- */
#define SUTRA_GLUE_LED_SYS_STATUS_1____WIDTH	8
#define SUTRA_GLUE_LED_SYS_STATUS_1____TYPE 	uint8_t

#define SUTRA_GLUE_LED_SYS_STATUS_1_psa_ok_led_red_blink_en___MASK 	UINT8_C(0x80)
#define SUTRA_GLUE_LED_SYS_STATUS_1_psa_ok_led_red_blink_en___SHIFT	7
#define SUTRA_GLUE_LED_SYS_STATUS_1_psa_ok_led_red___MASK          	UINT8_C(0x40)
#define SUTRA_GLUE_LED_SYS_STATUS_1_psa_ok_led_red___SHIFT         	6
#define SUTRA_GLUE_LED_SYS_STATUS_1_psa_ok_led_grn_blink_en___MASK 	UINT8_C(0x20)
#define SUTRA_GLUE_LED_SYS_STATUS_1_psa_ok_led_grn_blink_en___SHIFT	5
#define SUTRA_GLUE_LED_SYS_STATUS_1_psa_ok_led_grn___MASK          	UINT8_C(0x10)
#define SUTRA_GLUE_LED_SYS_STATUS_1_psa_ok_led_grn___SHIFT         	4
#define SUTRA_GLUE_LED_SYS_STATUS_1_psb_ok_led_red_blink_en___MASK 	UINT8_C(0x8)
#define SUTRA_GLUE_LED_SYS_STATUS_1_psb_ok_led_red_blink_en___SHIFT	3
#define SUTRA_GLUE_LED_SYS_STATUS_1_psb_ok_led_red___MASK          	UINT8_C(0x4)
#define SUTRA_GLUE_LED_SYS_STATUS_1_psb_ok_led_red___SHIFT         	2
#define SUTRA_GLUE_LED_SYS_STATUS_1_psb_ok_led_grn_blink_en___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_LED_SYS_STATUS_1_psb_ok_led_grn_blink_en___SHIFT	1
#define SUTRA_GLUE_LED_SYS_STATUS_1_psb_ok_led_grn___MASK          	UINT8_C(0x1)
#define SUTRA_GLUE_LED_SYS_STATUS_1_psb_ok_led_grn___SHIFT         	0
#define SUTRA_GLUE_LED_SYS_STATUS_1____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_LED_SYS_STATUS_2 ---- */
#define SUTRA_GLUE_LED_SYS_STATUS_2____WIDTH	8
#define SUTRA_GLUE_LED_SYS_STATUS_2____TYPE 	uint8_t

#define SUTRA_GLUE_LED_SYS_STATUS_2_Unused_6___MASK              	UINT8_C(0xc0)
#define SUTRA_GLUE_LED_SYS_STATUS_2_Unused_6___SHIFT             	6
#define SUTRA_GLUE_LED_SYS_STATUS_2_alarm_led_blink_en___MASK    	UINT8_C(0x20)
#define SUTRA_GLUE_LED_SYS_STATUS_2_alarm_led_blink_en___SHIFT   	5
#define SUTRA_GLUE_LED_SYS_STATUS_2_alarm_led_yellow___MASK      	UINT8_C(0x10)
#define SUTRA_GLUE_LED_SYS_STATUS_2_alarm_led_yellow___SHIFT     	4
#define SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_red_blink_en___MASK 	UINT8_C(0x8)
#define SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_red_blink_en___SHIFT	3
#define SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_red___MASK          	UINT8_C(0x4)
#define SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_red___SHIFT         	2
#define SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_grn_blink_en___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_grn_blink_en___SHIFT	1
#define SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_grn___MASK          	UINT8_C(0x1)
#define SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_grn___SHIFT         	0
#define SUTRA_GLUE_LED_SYS_STATUS_2____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_LED_SYS_STATUS_3 ---- */
#define SUTRA_GLUE_LED_SYS_STATUS_3____WIDTH	8
#define SUTRA_GLUE_LED_SYS_STATUS_3____TYPE 	uint8_t

#define SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_blink_en___MASK   	UINT8_C(0x80)
#define SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_blink_en___SHIFT  	7
#define SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_blue___MASK       	UINT8_C(0x40)
#define SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_blue___SHIFT      	6
#define SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_red___MASK        	UINT8_C(0x20)
#define SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_red___SHIFT       	5
#define SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_grn___MASK        	UINT8_C(0x10)
#define SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_grn___SHIFT       	4
#define SUTRA_GLUE_LED_SYS_STATUS_3_status_led_blue___MASK     	UINT8_C(0x8)
#define SUTRA_GLUE_LED_SYS_STATUS_3_status_led_blue___SHIFT    	3
#define SUTRA_GLUE_LED_SYS_STATUS_3_status_led_red___MASK      	UINT8_C(0x4)
#define SUTRA_GLUE_LED_SYS_STATUS_3_status_led_red___SHIFT     	2
#define SUTRA_GLUE_LED_SYS_STATUS_3_status_led_blink_en___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_LED_SYS_STATUS_3_status_led_blink_en___SHIFT	1
#define SUTRA_GLUE_LED_SYS_STATUS_3_status_led_green___MASK    	UINT8_C(0x1)
#define SUTRA_GLUE_LED_SYS_STATUS_3_status_led_green___SHIFT   	0
#define SUTRA_GLUE_LED_SYS_STATUS_3____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RESERVED_1 ---- */
#define SUTRA_GLUE_RESERVED_1____WIDTH	8
#define SUTRA_GLUE_RESERVED_1____TYPE 	uint8_t

#define SUTRA_GLUE_RESERVED_1____REGMASK	UINT8_C(0)

/* ---- SUTRA_GLUE_IST_MASTER_EVENT ---- */
#define SUTRA_GLUE_IST_MASTER_EVENT____WIDTH	8
#define SUTRA_GLUE_IST_MASTER_EVENT____TYPE 	uint8_t

#define SUTRA_GLUE_IST_MASTER_EVENT_TST___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_IST_MASTER_EVENT_TST___SHIFT	0
#define SUTRA_GLUE_IST_MASTER_EVENT____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_ISR_MASTER_EVENT ---- */
#define SUTRA_GLUE_ISR_MASTER_EVENT____WIDTH	8
#define SUTRA_GLUE_ISR_MASTER_EVENT____TYPE 	uint8_t

#define SUTRA_GLUE_ISR_MASTER_EVENT_Unused_7___MASK   	UINT8_C(0x80)
#define SUTRA_GLUE_ISR_MASTER_EVENT_Unused_7___SHIFT  	7
#define SUTRA_GLUE_ISR_MASTER_EVENT_ANY_I2C___MASK    	UINT8_C(0x40)
#define SUTRA_GLUE_ISR_MASTER_EVENT_ANY_I2C___SHIFT   	6
#define SUTRA_GLUE_ISR_MASTER_EVENT_ANY_MISC___MASK   	UINT8_C(0x20)
#define SUTRA_GLUE_ISR_MASTER_EVENT_ANY_MISC___SHIFT  	5
#define SUTRA_GLUE_ISR_MASTER_EVENT_Unused_4___MASK   	UINT8_C(0x10)
#define SUTRA_GLUE_ISR_MASTER_EVENT_Unused_4___SHIFT  	4
#define SUTRA_GLUE_ISR_MASTER_EVENT_ANY_SFP_TX___MASK 	UINT8_C(0x8)
#define SUTRA_GLUE_ISR_MASTER_EVENT_ANY_SFP_TX___SHIFT	3
#define SUTRA_GLUE_ISR_MASTER_EVENT_Unused_2___MASK   	UINT8_C(0x4)
#define SUTRA_GLUE_ISR_MASTER_EVENT_Unused_2___SHIFT  	2
#define SUTRA_GLUE_ISR_MASTER_EVENT_ANY_BUTTON___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_ISR_MASTER_EVENT_ANY_BUTTON___SHIFT	1
#define SUTRA_GLUE_ISR_MASTER_EVENT_ANY_POWER___MASK  	UINT8_C(0x1)
#define SUTRA_GLUE_ISR_MASTER_EVENT_ANY_POWER___SHIFT 	0
#define SUTRA_GLUE_ISR_MASTER_EVENT____REGMASK	UINT8_C(107)

/* ---- SUTRA_GLUE_ISM_MASTER_EVENT ---- */
#define SUTRA_GLUE_ISM_MASTER_EVENT____WIDTH	8
#define SUTRA_GLUE_ISM_MASTER_EVENT____TYPE 	uint8_t

#define SUTRA_GLUE_ISM_MASTER_EVENT_Unused_7___MASK   	UINT8_C(0x80)
#define SUTRA_GLUE_ISM_MASTER_EVENT_Unused_7___SHIFT  	7
#define SUTRA_GLUE_ISM_MASTER_EVENT_ANY_I2C___MASK    	UINT8_C(0x40)
#define SUTRA_GLUE_ISM_MASTER_EVENT_ANY_I2C___SHIFT   	6
#define SUTRA_GLUE_ISM_MASTER_EVENT_ANY_MISC___MASK   	UINT8_C(0x20)
#define SUTRA_GLUE_ISM_MASTER_EVENT_ANY_MISC___SHIFT  	5
#define SUTRA_GLUE_ISM_MASTER_EVENT_Unused_4___MASK   	UINT8_C(0x10)
#define SUTRA_GLUE_ISM_MASTER_EVENT_Unused_4___SHIFT  	4
#define SUTRA_GLUE_ISM_MASTER_EVENT_ANY_SFP_TX___MASK 	UINT8_C(0x8)
#define SUTRA_GLUE_ISM_MASTER_EVENT_ANY_SFP_TX___SHIFT	3
#define SUTRA_GLUE_ISM_MASTER_EVENT_Unused_2___MASK   	UINT8_C(0x4)
#define SUTRA_GLUE_ISM_MASTER_EVENT_Unused_2___SHIFT  	2
#define SUTRA_GLUE_ISM_MASTER_EVENT_ANY_BUTTON___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_ISM_MASTER_EVENT_ANY_BUTTON___SHIFT	1
#define SUTRA_GLUE_ISM_MASTER_EVENT_ANY_POWER___MASK  	UINT8_C(0x1)
#define SUTRA_GLUE_ISM_MASTER_EVENT_ANY_POWER___SHIFT 	0
#define SUTRA_GLUE_ISM_MASTER_EVENT____REGMASK	UINT8_C(107)

/* ---- SUTRA_GLUE_ISR_SW_I2C ---- */
#define SUTRA_GLUE_ISR_SW_I2C____WIDTH	8
#define SUTRA_GLUE_ISR_SW_I2C____TYPE 	uint8_t

#define SUTRA_GLUE_ISR_SW_I2C_Unused_4___MASK                 	UINT8_C(0xf0)
#define SUTRA_GLUE_ISR_SW_I2C_Unused_4___SHIFT                	4
#define SUTRA_GLUE_ISR_SW_I2C_PMBUS_SW_I2C_MASTER_DONE___MASK 	UINT8_C(0x8)
#define SUTRA_GLUE_ISR_SW_I2C_PMBUS_SW_I2C_MASTER_DONE___SHIFT	3
#define SUTRA_GLUE_ISR_SW_I2C_PCIE_SW_I2C_MASTER_DONE___MASK  	UINT8_C(0x4)
#define SUTRA_GLUE_ISR_SW_I2C_PCIE_SW_I2C_MASTER_DONE___SHIFT 	2
#define SUTRA_GLUE_ISR_SW_I2C_PS_SW_I2C_MASTER_DONE___MASK    	UINT8_C(0x2)
#define SUTRA_GLUE_ISR_SW_I2C_PS_SW_I2C_MASTER_DONE___SHIFT   	1
#define SUTRA_GLUE_ISR_SW_I2C_MB_SW_I2C_MASTER_DONE___MASK    	UINT8_C(0x1)
#define SUTRA_GLUE_ISR_SW_I2C_MB_SW_I2C_MASTER_DONE___SHIFT   	0
#define SUTRA_GLUE_ISR_SW_I2C____REGMASK	UINT8_C(15)

/* ---- SUTRA_GLUE_ISM_SW_I2C ---- */
#define SUTRA_GLUE_ISM_SW_I2C____WIDTH	8
#define SUTRA_GLUE_ISM_SW_I2C____TYPE 	uint8_t

#define SUTRA_GLUE_ISM_SW_I2C_Unused_4___MASK                 	UINT8_C(0xf0)
#define SUTRA_GLUE_ISM_SW_I2C_Unused_4___SHIFT                	4
#define SUTRA_GLUE_ISM_SW_I2C_PMBUS_SW_I2C_MASTER_DONE___MASK 	UINT8_C(0x8)
#define SUTRA_GLUE_ISM_SW_I2C_PMBUS_SW_I2C_MASTER_DONE___SHIFT	3
#define SUTRA_GLUE_ISM_SW_I2C_PCIE_SW_I2C_MASTER_DONE___MASK  	UINT8_C(0x4)
#define SUTRA_GLUE_ISM_SW_I2C_PCIE_SW_I2C_MASTER_DONE___SHIFT 	2
#define SUTRA_GLUE_ISM_SW_I2C_PS_SW_I2C_MASTER_DONE___MASK    	UINT8_C(0x2)
#define SUTRA_GLUE_ISM_SW_I2C_PS_SW_I2C_MASTER_DONE___SHIFT   	1
#define SUTRA_GLUE_ISM_SW_I2C_MB_SW_I2C_MASTER_DONE___MASK    	UINT8_C(0x1)
#define SUTRA_GLUE_ISM_SW_I2C_MB_SW_I2C_MASTER_DONE___SHIFT   	0
#define SUTRA_GLUE_ISM_SW_I2C____REGMASK	UINT8_C(15)

/* ---- SUTRA_GLUE_ISR_MISC ---- */
#define SUTRA_GLUE_ISR_MISC____WIDTH	8
#define SUTRA_GLUE_ISR_MISC____TYPE 	uint8_t

#define SUTRA_GLUE_ISR_MISC_Unused_6___MASK               	UINT8_C(0xc0)
#define SUTRA_GLUE_ISR_MISC_Unused_6___SHIFT              	6
#define SUTRA_GLUE_ISR_MISC_CPU_BOARD_HOT_ALERT_N___MASK  	UINT8_C(0x20)
#define SUTRA_GLUE_ISR_MISC_CPU_BOARD_HOT_ALERT_N___SHIFT 	5
#define SUTRA_GLUE_ISR_MISC_CPU_BOARD_WARM_ALERT_N___MASK 	UINT8_C(0x10)
#define SUTRA_GLUE_ISR_MISC_CPU_BOARD_WARM_ALERT_N___SHIFT	4
#define SUTRA_GLUE_ISR_MISC_CPU_QUAD1_INT_N___MASK        	UINT8_C(0x8)
#define SUTRA_GLUE_ISR_MISC_CPU_QUAD1_INT_N___SHIFT       	3
#define SUTRA_GLUE_ISR_MISC_TPM_IRQ___MASK                	UINT8_C(0x4)
#define SUTRA_GLUE_ISR_MISC_TPM_IRQ___SHIFT               	2
#define SUTRA_GLUE_ISR_MISC_USB_IRQ___MASK                	UINT8_C(0x2)
#define SUTRA_GLUE_ISR_MISC_USB_IRQ___SHIFT               	1
#define SUTRA_GLUE_ISR_MISC_Reserved___MASK               	UINT8_C(0x1)
#define SUTRA_GLUE_ISR_MISC_Reserved___SHIFT              	0
#define SUTRA_GLUE_ISR_MISC____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_ISM_MISC ---- */
#define SUTRA_GLUE_ISM_MISC____WIDTH	8
#define SUTRA_GLUE_ISM_MISC____TYPE 	uint8_t

#define SUTRA_GLUE_ISM_MISC_Unused_6___MASK               	UINT8_C(0xc0)
#define SUTRA_GLUE_ISM_MISC_Unused_6___SHIFT              	6
#define SUTRA_GLUE_ISM_MISC_CPU_BOARD_HOT_ALERT_N___MASK  	UINT8_C(0x20)
#define SUTRA_GLUE_ISM_MISC_CPU_BOARD_HOT_ALERT_N___SHIFT 	5
#define SUTRA_GLUE_ISM_MISC_CPU_BOARD_WARM_ALERT_N___MASK 	UINT8_C(0x10)
#define SUTRA_GLUE_ISM_MISC_CPU_BOARD_WARM_ALERT_N___SHIFT	4
#define SUTRA_GLUE_ISM_MISC_CPU_QUAD1_INT_N___MASK        	UINT8_C(0x8)
#define SUTRA_GLUE_ISM_MISC_CPU_QUAD1_INT_N___SHIFT       	3
#define SUTRA_GLUE_ISM_MISC_TPM_IRQ___MASK                	UINT8_C(0x4)
#define SUTRA_GLUE_ISM_MISC_TPM_IRQ___SHIFT               	2
#define SUTRA_GLUE_ISM_MISC_USB_IRQ___MASK                	UINT8_C(0x2)
#define SUTRA_GLUE_ISM_MISC_USB_IRQ___SHIFT               	1
#define SUTRA_GLUE_ISM_MISC_Rsvd___MASK                   	UINT8_C(0x1)
#define SUTRA_GLUE_ISM_MISC_Rsvd___SHIFT                  	0
#define SUTRA_GLUE_ISM_MISC____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_ISR_PWR ---- */
#define SUTRA_GLUE_ISR_PWR____WIDTH	8
#define SUTRA_GLUE_ISR_PWR____TYPE 	uint8_t

#define SUTRA_GLUE_ISR_PWR_Unused_6___MASK      	UINT8_C(0xc0)
#define SUTRA_GLUE_ISR_PWR_Unused_6___SHIFT     	6
#define SUTRA_GLUE_ISR_PWR_PSB_ACOK_H___MASK    	UINT8_C(0x20)
#define SUTRA_GLUE_ISR_PWR_PSB_ACOK_H___SHIFT   	5
#define SUTRA_GLUE_ISR_PWR_PSA_ACOK_H___MASK    	UINT8_C(0x10)
#define SUTRA_GLUE_ISR_PWR_PSA_ACOK_H___SHIFT   	4
#define SUTRA_GLUE_ISR_PWR_PSB_PWR_OK___MASK    	UINT8_C(0x8)
#define SUTRA_GLUE_ISR_PWR_PSB_PWR_OK___SHIFT   	3
#define SUTRA_GLUE_ISR_PWR_PSA_PWR_OK___MASK    	UINT8_C(0x4)
#define SUTRA_GLUE_ISR_PWR_PSA_PWR_OK___SHIFT   	2
#define SUTRA_GLUE_ISR_PWR_PSB_PRESENT_L___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_ISR_PWR_PSB_PRESENT_L___SHIFT	1
#define SUTRA_GLUE_ISR_PWR_PSA_PRESENT_L___MASK 	UINT8_C(0x1)
#define SUTRA_GLUE_ISR_PWR_PSA_PRESENT_L___SHIFT	0
#define SUTRA_GLUE_ISR_PWR____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_ISM_PWR ---- */
#define SUTRA_GLUE_ISM_PWR____WIDTH	8
#define SUTRA_GLUE_ISM_PWR____TYPE 	uint8_t

#define SUTRA_GLUE_ISM_PWR_Unused_6___MASK      	UINT8_C(0xc0)
#define SUTRA_GLUE_ISM_PWR_Unused_6___SHIFT     	6
#define SUTRA_GLUE_ISM_PWR_PSB_ACOK_H___MASK    	UINT8_C(0x20)
#define SUTRA_GLUE_ISM_PWR_PSB_ACOK_H___SHIFT   	5
#define SUTRA_GLUE_ISM_PWR_PSA_ACOK_H___MASK    	UINT8_C(0x10)
#define SUTRA_GLUE_ISM_PWR_PSA_ACOK_H___SHIFT   	4
#define SUTRA_GLUE_ISM_PWR_PSB_PWR_OK___MASK    	UINT8_C(0x8)
#define SUTRA_GLUE_ISM_PWR_PSB_PWR_OK___SHIFT   	3
#define SUTRA_GLUE_ISM_PWR_PSA_PWR_OK___MASK    	UINT8_C(0x4)
#define SUTRA_GLUE_ISM_PWR_PSA_PWR_OK___SHIFT   	2
#define SUTRA_GLUE_ISM_PWR_PSB_PRESENT_L___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_ISM_PWR_PSB_PRESENT_L___SHIFT	1
#define SUTRA_GLUE_ISM_PWR_PSA_PRESENT_L___MASK 	UINT8_C(0x1)
#define SUTRA_GLUE_ISM_PWR_PSA_PRESENT_L___SHIFT	0
#define SUTRA_GLUE_ISM_PWR____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_STATUS_PWR ---- */
#define SUTRA_GLUE_STATUS_PWR____WIDTH	8
#define SUTRA_GLUE_STATUS_PWR____TYPE 	uint8_t

#define SUTRA_GLUE_STATUS_PWR_Unused_6___MASK      	UINT8_C(0xc0)
#define SUTRA_GLUE_STATUS_PWR_Unused_6___SHIFT     	6
#define SUTRA_GLUE_STATUS_PWR_PSB_ACOK_H___MASK    	UINT8_C(0x20)
#define SUTRA_GLUE_STATUS_PWR_PSB_ACOK_H___SHIFT   	5
#define SUTRA_GLUE_STATUS_PWR_PSA_ACOK_H___MASK    	UINT8_C(0x10)
#define SUTRA_GLUE_STATUS_PWR_PSA_ACOK_H___SHIFT   	4
#define SUTRA_GLUE_STATUS_PWR_PSB_PWR_OK___MASK    	UINT8_C(0x8)
#define SUTRA_GLUE_STATUS_PWR_PSB_PWR_OK___SHIFT   	3
#define SUTRA_GLUE_STATUS_PWR_PSA_PWR_OK___MASK    	UINT8_C(0x4)
#define SUTRA_GLUE_STATUS_PWR_PSA_PWR_OK___SHIFT   	2
#define SUTRA_GLUE_STATUS_PWR_PSB_PRESENT_L___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_STATUS_PWR_PSB_PRESENT_L___SHIFT	1
#define SUTRA_GLUE_STATUS_PWR_PSA_PRESENT_L___MASK 	UINT8_C(0x1)
#define SUTRA_GLUE_STATUS_PWR_PSA_PRESENT_L___SHIFT	0
#define SUTRA_GLUE_STATUS_PWR____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_I2C_SW_IF_SEL ---- */
#define SUTRA_GLUE_I2C_SW_IF_SEL____WIDTH	8
#define SUTRA_GLUE_I2C_SW_IF_SEL____TYPE 	uint8_t

#define SUTRA_GLUE_I2C_SW_IF_SEL_Unused_5___MASK         	UINT8_C(0xe0)
#define SUTRA_GLUE_I2C_SW_IF_SEL_Unused_5___SHIFT        	5
#define SUTRA_GLUE_I2C_SW_IF_SEL_pmbus_sel_sw_i2c___MASK 	UINT8_C(0x10)
#define SUTRA_GLUE_I2C_SW_IF_SEL_pmbus_sel_sw_i2c___SHIFT	4
#define SUTRA_GLUE_I2C_SW_IF_SEL_pcie_sel_sw_i2c___MASK  	UINT8_C(0x8)
#define SUTRA_GLUE_I2C_SW_IF_SEL_pcie_sel_sw_i2c___SHIFT 	3
#define SUTRA_GLUE_I2C_SW_IF_SEL_psb_sel_sw_i2c___MASK   	UINT8_C(0x4)
#define SUTRA_GLUE_I2C_SW_IF_SEL_psb_sel_sw_i2c___SHIFT  	2
#define SUTRA_GLUE_I2C_SW_IF_SEL_psa_sel_sw_i2c___MASK   	UINT8_C(0x2)
#define SUTRA_GLUE_I2C_SW_IF_SEL_psa_sel_sw_i2c___SHIFT  	1
#define SUTRA_GLUE_I2C_SW_IF_SEL_mb_sel_sw_i2c___MASK    	UINT8_C(0x1)
#define SUTRA_GLUE_I2C_SW_IF_SEL_mb_sel_sw_i2c___SHIFT   	0
#define SUTRA_GLUE_I2C_SW_IF_SEL____REGMASK	UINT8_C(31)

/* ---- SUTRA_GLUE_I2C_SW_MISC_CTRL_0 ---- */
#define SUTRA_GLUE_I2C_SW_MISC_CTRL_0____WIDTH	8
#define SUTRA_GLUE_I2C_SW_MISC_CTRL_0____TYPE 	uint8_t

#define SUTRA_GLUE_I2C_SW_MISC_CTRL_0_mb_i2c_half_period___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_I2C_SW_MISC_CTRL_0_mb_i2c_half_period___SHIFT	0
#define SUTRA_GLUE_I2C_SW_MISC_CTRL_0____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_I2C_SW_MISC_CTRL_1 ---- */
#define SUTRA_GLUE_I2C_SW_MISC_CTRL_1____WIDTH	8
#define SUTRA_GLUE_I2C_SW_MISC_CTRL_1____TYPE 	uint8_t

#define SUTRA_GLUE_I2C_SW_MISC_CTRL_1_mb_i2c_half_period___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_I2C_SW_MISC_CTRL_1_mb_i2c_half_period___SHIFT	0
#define SUTRA_GLUE_I2C_SW_MISC_CTRL_1____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_ISR_BUTTON ---- */
#define SUTRA_GLUE_ISR_BUTTON____WIDTH	8
#define SUTRA_GLUE_ISR_BUTTON____TYPE 	uint8_t

#define SUTRA_GLUE_ISR_BUTTON_Unused_2___MASK                    	UINT8_C(0xfc)
#define SUTRA_GLUE_ISR_BUTTON_Unused_2___SHIFT                   	2
#define SUTRA_GLUE_ISR_BUTTON_PB_RESET_HELD_FOR_3_SECONDS___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_ISR_BUTTON_PB_RESET_HELD_FOR_3_SECONDS___SHIFT	1
#define SUTRA_GLUE_ISR_BUTTON_PB_RESET_PRESS_DETECT___MASK       	UINT8_C(0x1)
#define SUTRA_GLUE_ISR_BUTTON_PB_RESET_PRESS_DETECT___SHIFT      	0
#define SUTRA_GLUE_ISR_BUTTON____REGMASK	UINT8_C(3)

/* ---- SUTRA_GLUE_ISM_BUTTON ---- */
#define SUTRA_GLUE_ISM_BUTTON____WIDTH	8
#define SUTRA_GLUE_ISM_BUTTON____TYPE 	uint8_t

#define SUTRA_GLUE_ISM_BUTTON_Unused_2___MASK                    	UINT8_C(0xfc)
#define SUTRA_GLUE_ISM_BUTTON_Unused_2___SHIFT                   	2
#define SUTRA_GLUE_ISM_BUTTON_PB_RESET_HELD_FOR_3_SECONDS___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_ISM_BUTTON_PB_RESET_HELD_FOR_3_SECONDS___SHIFT	1
#define SUTRA_GLUE_ISM_BUTTON_PB_RESET_PRESS_DETECT___MASK       	UINT8_C(0x1)
#define SUTRA_GLUE_ISM_BUTTON_PB_RESET_PRESS_DETECT___SHIFT      	0
#define SUTRA_GLUE_ISM_BUTTON____REGMASK	UINT8_C(3)

/* ---- SUTRA_GLUE_STATUS_BUTTON ---- */
#define SUTRA_GLUE_STATUS_BUTTON____WIDTH	8
#define SUTRA_GLUE_STATUS_BUTTON____TYPE 	uint8_t

#define SUTRA_GLUE_STATUS_BUTTON_Unused_2___MASK                    	UINT8_C(0xfc)
#define SUTRA_GLUE_STATUS_BUTTON_Unused_2___SHIFT                   	2
#define SUTRA_GLUE_STATUS_BUTTON_PB_RESET_HELD_FOR_3_SECONDS___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_STATUS_BUTTON_PB_RESET_HELD_FOR_3_SECONDS___SHIFT	1
#define SUTRA_GLUE_STATUS_BUTTON_PB_RESET_REAL_TIME_STATE___MASK    	UINT8_C(0x1)
#define SUTRA_GLUE_STATUS_BUTTON_PB_RESET_REAL_TIME_STATE___SHIFT   	0
#define SUTRA_GLUE_STATUS_BUTTON____REGMASK	UINT8_C(3)

/* ---- SUTRA_GLUE_STATUS_MISC ---- */
#define SUTRA_GLUE_STATUS_MISC____WIDTH	8
#define SUTRA_GLUE_STATUS_MISC____TYPE 	uint8_t

#define SUTRA_GLUE_STATUS_MISC_Unused_6___MASK               	UINT8_C(0xc0)
#define SUTRA_GLUE_STATUS_MISC_Unused_6___SHIFT              	6
#define SUTRA_GLUE_STATUS_MISC_CPU_BOARD_HOT_ALERT_N___MASK  	UINT8_C(0x20)
#define SUTRA_GLUE_STATUS_MISC_CPU_BOARD_HOT_ALERT_N___SHIFT 	5
#define SUTRA_GLUE_STATUS_MISC_CPU_BOARD_WARM_ALERT_N___MASK 	UINT8_C(0x10)
#define SUTRA_GLUE_STATUS_MISC_CPU_BOARD_WARM_ALERT_N___SHIFT	4
#define SUTRA_GLUE_STATUS_MISC_CPU_QUAD1_INT_N___MASK        	UINT8_C(0x8)
#define SUTRA_GLUE_STATUS_MISC_CPU_QUAD1_INT_N___SHIFT       	3
#define SUTRA_GLUE_STATUS_MISC_TPM_IRQ_N___MASK              	UINT8_C(0x4)
#define SUTRA_GLUE_STATUS_MISC_TPM_IRQ_N___SHIFT             	2
#define SUTRA_GLUE_STATUS_MISC_USB_IRQ_N___MASK              	UINT8_C(0x2)
#define SUTRA_GLUE_STATUS_MISC_USB_IRQ_N___SHIFT             	1
#define SUTRA_GLUE_STATUS_MISC_Rsvd___MASK                   	UINT8_C(0x1)
#define SUTRA_GLUE_STATUS_MISC_Rsvd___SHIFT                  	0
#define SUTRA_GLUE_STATUS_MISC____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_PWR_INPUT_DEBOUNCE ---- */
#define SUTRA_GLUE_PWR_INPUT_DEBOUNCE____WIDTH	8
#define SUTRA_GLUE_PWR_INPUT_DEBOUNCE____TYPE 	uint8_t

#define SUTRA_GLUE_PWR_INPUT_DEBOUNCE_IVAL___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_PWR_INPUT_DEBOUNCE_IVAL___SHIFT	0
#define SUTRA_GLUE_PWR_INPUT_DEBOUNCE____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_IST_SW_I2C ---- */
#define SUTRA_GLUE_IST_SW_I2C____WIDTH	8
#define SUTRA_GLUE_IST_SW_I2C____TYPE 	uint8_t

#define SUTRA_GLUE_IST_SW_I2C_Unused_4___MASK 	UINT8_C(0xf0)
#define SUTRA_GLUE_IST_SW_I2C_Unused_4___SHIFT	4
#define SUTRA_GLUE_IST_SW_I2C_TST___MASK      	UINT8_C(0xf)
#define SUTRA_GLUE_IST_SW_I2C_TST___SHIFT     	0
#define SUTRA_GLUE_IST_SW_I2C____REGMASK	UINT8_C(15)

/* ---- SUTRA_GLUE_IST_MISC ---- */
#define SUTRA_GLUE_IST_MISC____WIDTH	8
#define SUTRA_GLUE_IST_MISC____TYPE 	uint8_t

#define SUTRA_GLUE_IST_MISC_Unused_6___MASK 	UINT8_C(0xc0)
#define SUTRA_GLUE_IST_MISC_Unused_6___SHIFT	6
#define SUTRA_GLUE_IST_MISC_TST___MASK      	UINT8_C(0x3f)
#define SUTRA_GLUE_IST_MISC_TST___SHIFT     	0
#define SUTRA_GLUE_IST_MISC____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_IST_PWR ---- */
#define SUTRA_GLUE_IST_PWR____WIDTH	8
#define SUTRA_GLUE_IST_PWR____TYPE 	uint8_t

#define SUTRA_GLUE_IST_PWR_Unused_6___MASK 	UINT8_C(0xc0)
#define SUTRA_GLUE_IST_PWR_Unused_6___SHIFT	6
#define SUTRA_GLUE_IST_PWR_TST___MASK      	UINT8_C(0x3f)
#define SUTRA_GLUE_IST_PWR_TST___SHIFT     	0
#define SUTRA_GLUE_IST_PWR____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_IST_BUTTON ---- */
#define SUTRA_GLUE_IST_BUTTON____WIDTH	8
#define SUTRA_GLUE_IST_BUTTON____TYPE 	uint8_t

#define SUTRA_GLUE_IST_BUTTON_Unused_2___MASK 	UINT8_C(0xfc)
#define SUTRA_GLUE_IST_BUTTON_Unused_2___SHIFT	2
#define SUTRA_GLUE_IST_BUTTON_TST___MASK      	UINT8_C(0x3)
#define SUTRA_GLUE_IST_BUTTON_TST___SHIFT     	0
#define SUTRA_GLUE_IST_BUTTON____REGMASK	UINT8_C(3)

/* ---- SUTRA_GLUE_DEBUG_CPU_WDT ---- */
#define SUTRA_GLUE_DEBUG_CPU_WDT____WIDTH	8
#define SUTRA_GLUE_DEBUG_CPU_WDT____TYPE 	uint8_t

#define SUTRA_GLUE_DEBUG_CPU_WDT_Unused_1___MASK 	UINT8_C(0xfe)
#define SUTRA_GLUE_DEBUG_CPU_WDT_Unused_1___SHIFT	1
#define SUTRA_GLUE_DEBUG_CPU_WDT_ENABLE___MASK   	UINT8_C(0x1)
#define SUTRA_GLUE_DEBUG_CPU_WDT_ENABLE___SHIFT  	0
#define SUTRA_GLUE_DEBUG_CPU_WDT____REGMASK	UINT8_C(1)

/* ---- SUTRA_GLUE_SW_TEST_PWR_0 ---- */
#define SUTRA_GLUE_SW_TEST_PWR_0____WIDTH	8
#define SUTRA_GLUE_SW_TEST_PWR_0____TYPE 	uint8_t

#define SUTRA_GLUE_SW_TEST_PWR_0_Unused_6___MASK             	UINT8_C(0xc0)
#define SUTRA_GLUE_SW_TEST_PWR_0_Unused_6___SHIFT            	6
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSB_ACOK_H___MASK    	UINT8_C(0x20)
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSB_ACOK_H___SHIFT   	5
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSA_ACOK_H___MASK    	UINT8_C(0x10)
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSA_ACOK_H___SHIFT   	4
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSB_PWR_OK___MASK    	UINT8_C(0x8)
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSB_PWR_OK___SHIFT   	3
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSA_PWR_OK___MASK    	UINT8_C(0x4)
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSA_PWR_OK___SHIFT   	2
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSB_PRESENT_L___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSB_PRESENT_L___SHIFT	1
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSA_PRESENT_L___MASK 	UINT8_C(0x1)
#define SUTRA_GLUE_SW_TEST_PWR_0_SW_OVR_PSA_PRESENT_L___SHIFT	0
#define SUTRA_GLUE_SW_TEST_PWR_0____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_SW_TEST_PWR_1 ---- */
#define SUTRA_GLUE_SW_TEST_PWR_1____WIDTH	8
#define SUTRA_GLUE_SW_TEST_PWR_1____TYPE 	uint8_t

#define SUTRA_GLUE_SW_TEST_PWR_1_Unused_6___MASK         	UINT8_C(0xc0)
#define SUTRA_GLUE_SW_TEST_PWR_1_Unused_6___SHIFT        	6
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSB_ACOK_H___MASK    	UINT8_C(0x20)
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSB_ACOK_H___SHIFT   	5
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSA_ACOK_H___MASK    	UINT8_C(0x10)
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSA_ACOK_H___SHIFT   	4
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSB_PWR_OK___MASK    	UINT8_C(0x8)
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSB_PWR_OK___SHIFT   	3
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSA_PWR_OK___MASK    	UINT8_C(0x4)
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSA_PWR_OK___SHIFT   	2
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSB_PRESENT_L___MASK 	UINT8_C(0x2)
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSB_PRESENT_L___SHIFT	1
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSA_PRESENT_L___MASK 	UINT8_C(0x1)
#define SUTRA_GLUE_SW_TEST_PWR_1_SW_PSA_PRESENT_L___SHIFT	0
#define SUTRA_GLUE_SW_TEST_PWR_1____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_UART_SEL ---- */
#define SUTRA_GLUE_UART_SEL____WIDTH	8
#define SUTRA_GLUE_UART_SEL____TYPE 	uint8_t

#define SUTRA_GLUE_UART_SEL_Unused_6___MASK     	UINT8_C(0xc0)
#define SUTRA_GLUE_UART_SEL_Unused_6___SHIFT    	6
#define SUTRA_GLUE_UART_SEL_uart_pcie___MASK    	UINT8_C(0x20)
#define SUTRA_GLUE_UART_SEL_uart_pcie___SHIFT   	5
#define SUTRA_GLUE_UART_SEL_uart_fpga___MASK    	UINT8_C(0x10)
#define SUTRA_GLUE_UART_SEL_uart_fpga___SHIFT   	4
#define SUTRA_GLUE_UART_SEL_uart_bmc_sol___MASK 	UINT8_C(0x8)
#define SUTRA_GLUE_UART_SEL_uart_bmc_sol___SHIFT	3
#define SUTRA_GLUE_UART_SEL_uart_bmc_con___MASK 	UINT8_C(0x4)
#define SUTRA_GLUE_UART_SEL_uart_bmc_con___SHIFT	2
#define SUTRA_GLUE_UART_SEL_uart_cpu1___MASK    	UINT8_C(0x2)
#define SUTRA_GLUE_UART_SEL_uart_cpu1___SHIFT   	1
#define SUTRA_GLUE_UART_SEL_uart_cpu0___MASK    	UINT8_C(0x1)
#define SUTRA_GLUE_UART_SEL_uart_cpu0___SHIFT   	0
#define SUTRA_GLUE_UART_SEL____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_PWRGOOD_STATUS_0 ---- */
#define SUTRA_GLUE_PWRGOOD_STATUS_0____WIDTH	8
#define SUTRA_GLUE_PWRGOOD_STATUS_0____TYPE 	uint8_t

#define SUTRA_GLUE_PWRGOOD_STATUS_0_pwr_good_2___MASK 	UINT8_C(0xf8)
#define SUTRA_GLUE_PWRGOOD_STATUS_0_pwr_good_2___SHIFT	3
#define SUTRA_GLUE_PWRGOOD_STATUS_0_unused___MASK     	UINT8_C(0x4)
#define SUTRA_GLUE_PWRGOOD_STATUS_0_unused___SHIFT    	2
#define SUTRA_GLUE_PWRGOOD_STATUS_0_pwr_good_1___MASK 	UINT8_C(0x3)
#define SUTRA_GLUE_PWRGOOD_STATUS_0_pwr_good_1___SHIFT	0
#define SUTRA_GLUE_PWRGOOD_STATUS_0____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_PWRGOOD_STATUS_1 ---- */
#define SUTRA_GLUE_PWRGOOD_STATUS_1____WIDTH	8
#define SUTRA_GLUE_PWRGOOD_STATUS_1____TYPE 	uint8_t

#define SUTRA_GLUE_PWRGOOD_STATUS_1_Unused_7___MASK   	UINT8_C(0x80)
#define SUTRA_GLUE_PWRGOOD_STATUS_1_Unused_7___SHIFT  	7
#define SUTRA_GLUE_PWRGOOD_STATUS_1_pwr_good_2___MASK 	UINT8_C(0x78)
#define SUTRA_GLUE_PWRGOOD_STATUS_1_pwr_good_2___SHIFT	3
#define SUTRA_GLUE_PWRGOOD_STATUS_1_unused___MASK     	UINT8_C(0x4)
#define SUTRA_GLUE_PWRGOOD_STATUS_1_unused___SHIFT    	2
#define SUTRA_GLUE_PWRGOOD_STATUS_1_pwr_good_1___MASK 	UINT8_C(0x3)
#define SUTRA_GLUE_PWRGOOD_STATUS_1_pwr_good_1___SHIFT	0
#define SUTRA_GLUE_PWRGOOD_STATUS_1____REGMASK	UINT8_C(127)

/* ---- SUTRA_GLUE_PWRGOOD_STATUS_2 ---- */
#define SUTRA_GLUE_PWRGOOD_STATUS_2____WIDTH	8
#define SUTRA_GLUE_PWRGOOD_STATUS_2____TYPE 	uint8_t

#define SUTRA_GLUE_PWRGOOD_STATUS_2_pwr_good___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_PWRGOOD_STATUS_2_pwr_good___SHIFT	0
#define SUTRA_GLUE_PWRGOOD_STATUS_2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_PWRGOOD_STATUS_3 ---- */
#define SUTRA_GLUE_PWRGOOD_STATUS_3____WIDTH	8
#define SUTRA_GLUE_PWRGOOD_STATUS_3____TYPE 	uint8_t

#define SUTRA_GLUE_PWRGOOD_STATUS_3_Unused_5___MASK       	UINT8_C(0xe0)
#define SUTRA_GLUE_PWRGOOD_STATUS_3_Unused_5___SHIFT      	5
#define SUTRA_GLUE_PWRGOOD_STATUS_3_PG_3V3_M___MASK       	UINT8_C(0x10)
#define SUTRA_GLUE_PWRGOOD_STATUS_3_PG_3V3_M___SHIFT      	4
#define SUTRA_GLUE_PWRGOOD_STATUS_3_reserved___MASK       	UINT8_C(0x8)
#define SUTRA_GLUE_PWRGOOD_STATUS_3_reserved___SHIFT      	3
#define SUTRA_GLUE_PWRGOOD_STATUS_3_PG_2V5_BMC_AUX___MASK 	UINT8_C(0x4)
#define SUTRA_GLUE_PWRGOOD_STATUS_3_PG_2V5_BMC_AUX___SHIFT	2
#define SUTRA_GLUE_PWRGOOD_STATUS_3_PG_5V0___MASK         	UINT8_C(0x2)
#define SUTRA_GLUE_PWRGOOD_STATUS_3_PG_5V0___SHIFT        	1
#define SUTRA_GLUE_PWRGOOD_STATUS_3_PG_5V5___MASK         	UINT8_C(0x1)
#define SUTRA_GLUE_PWRGOOD_STATUS_3_PG_5V5___SHIFT        	0
#define SUTRA_GLUE_PWRGOOD_STATUS_3____REGMASK	UINT8_C(31)

/* ---- SUTRA_GLUE_MISC_CTRL ---- */
#define SUTRA_GLUE_MISC_CTRL____WIDTH	8
#define SUTRA_GLUE_MISC_CTRL____TYPE 	uint8_t

#define SUTRA_GLUE_MISC_CTRL_reserved3___MASK   	UINT8_C(0x80)
#define SUTRA_GLUE_MISC_CTRL_reserved3___SHIFT  	7
#define SUTRA_GLUE_MISC_CTRL_reserved2___MASK   	UINT8_C(0x40)
#define SUTRA_GLUE_MISC_CTRL_reserved2___SHIFT  	6
#define SUTRA_GLUE_MISC_CTRL_usb_p5v_en___MASK  	UINT8_C(0x20)
#define SUTRA_GLUE_MISC_CTRL_usb_p5v_en___SHIFT 	5
#define SUTRA_GLUE_MISC_CTRL_usb_p5v_val___MASK 	UINT8_C(0x10)
#define SUTRA_GLUE_MISC_CTRL_usb_p5v_val___SHIFT	4
#define SUTRA_GLUE_MISC_CTRL_ss_mux_en___MASK   	UINT8_C(0x8)
#define SUTRA_GLUE_MISC_CTRL_ss_mux_en___SHIFT  	3
#define SUTRA_GLUE_MISC_CTRL_ss_mux_val___MASK  	UINT8_C(0x4)
#define SUTRA_GLUE_MISC_CTRL_ss_mux_val___SHIFT 	2
#define SUTRA_GLUE_MISC_CTRL_reserved___MASK    	UINT8_C(0x3)
#define SUTRA_GLUE_MISC_CTRL_reserved___SHIFT   	0
#define SUTRA_GLUE_MISC_CTRL____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_MISC_STATUS ---- */
#define SUTRA_GLUE_MISC_STATUS____WIDTH	8
#define SUTRA_GLUE_MISC_STATUS____TYPE 	uint8_t

#define SUTRA_GLUE_MISC_STATUS_Unused_3___MASK      	UINT8_C(0xf8)
#define SUTRA_GLUE_MISC_STATUS_Unused_3___SHIFT     	3
#define SUTRA_GLUE_MISC_STATUS_usb_direction___MASK 	UINT8_C(0x4)
#define SUTRA_GLUE_MISC_STATUS_usb_direction___SHIFT	2
#define SUTRA_GLUE_MISC_STATUS_usb_id___MASK        	UINT8_C(0x2)
#define SUTRA_GLUE_MISC_STATUS_usb_id___SHIFT       	1
#define SUTRA_GLUE_MISC_STATUS_vconn_fault___MASK   	UINT8_C(0x1)
#define SUTRA_GLUE_MISC_STATUS_vconn_fault___SHIFT  	0
#define SUTRA_GLUE_MISC_STATUS____REGMASK	UINT8_C(7)

/* ---- SUTRA_GLUE_UART_SWITCHING_STATUS ---- */
#define SUTRA_GLUE_UART_SWITCHING_STATUS____WIDTH	8
#define SUTRA_GLUE_UART_SWITCHING_STATUS____TYPE 	uint8_t

#define SUTRA_GLUE_UART_SWITCHING_STATUS_Unused_1___MASK       	UINT8_C(0xfe)
#define SUTRA_GLUE_UART_SWITCHING_STATUS_Unused_1___SHIFT      	1
#define SUTRA_GLUE_UART_SWITCHING_STATUS_uart_switching___MASK 	UINT8_C(0x1)
#define SUTRA_GLUE_UART_SWITCHING_STATUS_uart_switching___SHIFT	0
#define SUTRA_GLUE_UART_SWITCHING_STATUS____REGMASK	UINT8_C(1)

/* ---- SUTRA_GLUE_UART_STATUS ---- */
#define SUTRA_GLUE_UART_STATUS____WIDTH	8
#define SUTRA_GLUE_UART_STATUS____TYPE 	uint8_t

#define SUTRA_GLUE_UART_STATUS_Unused_1___MASK    	UINT8_C(0xfe)
#define SUTRA_GLUE_UART_STATUS_Unused_1___SHIFT   	1
#define SUTRA_GLUE_UART_STATUS_uart_status___MASK 	UINT8_C(0x1)
#define SUTRA_GLUE_UART_STATUS_uart_status___SHIFT	0
#define SUTRA_GLUE_UART_STATUS____REGMASK	UINT8_C(1)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA0 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA0____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA0____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA0_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA0____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA1 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA1____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA1____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA1_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA1____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA2 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA2____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA2____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA2_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA3 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA3____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA3____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA3_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA3____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA4 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA4____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA4____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA4_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA4_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA4____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA5 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA5____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA5____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA5_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA5_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA5____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA6 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA6____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA6____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA6_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA6_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA6____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA7 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA7____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA7____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA7_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA7_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA7____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RESERVED_2 ---- */
#define SUTRA_GLUE_RESERVED_2____WIDTH	8
#define SUTRA_GLUE_RESERVED_2____TYPE 	uint8_t

#define SUTRA_GLUE_RESERVED_2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RESERVED_2_DATA___SHIFT	0
#define SUTRA_GLUE_RESERVED_2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_SGPIO_CTL ---- */
#define SUTRA_GLUE_TEST_SGPIO_CTL____WIDTH	8
#define SUTRA_GLUE_TEST_SGPIO_CTL____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_SGPIO_CTL_Unused_4___MASK                	UINT8_C(0xf0)
#define SUTRA_GLUE_TEST_SGPIO_CTL_Unused_4___SHIFT               	4
#define SUTRA_GLUE_TEST_SGPIO_CTL_send_test_data_to_bmc___MASK   	UINT8_C(0x8)
#define SUTRA_GLUE_TEST_SGPIO_CTL_send_test_data_to_bmc___SHIFT  	3
#define SUTRA_GLUE_TEST_SGPIO_CTL_send_test_data_to_rudra___MASK 	UINT8_C(0x4)
#define SUTRA_GLUE_TEST_SGPIO_CTL_send_test_data_to_rudra___SHIFT	2
#define SUTRA_GLUE_TEST_SGPIO_CTL_Reserved___MASK                	UINT8_C(0x2)
#define SUTRA_GLUE_TEST_SGPIO_CTL_Reserved___SHIFT               	1
#define SUTRA_GLUE_TEST_SGPIO_CTL_Unused_0___MASK                	UINT8_C(0x1)
#define SUTRA_GLUE_TEST_SGPIO_CTL_Unused_0___SHIFT               	0
#define SUTRA_GLUE_TEST_SGPIO_CTL____REGMASK	UINT8_C(14)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG0 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG0____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG0____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG0_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG0____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG1 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG1____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG1____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG1_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG1____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG2 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG2____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG2____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG2_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG3 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG3____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG3____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG3_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG3____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG4 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG4____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG4____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG4_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG4_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG4____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG5 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG5____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG5____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG5_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG5_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG5____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG6 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG6____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG6____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG6_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG6_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG6____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG7 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG7____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG7____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG7_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG7_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_DEBUG7____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA0 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA0____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA0____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA0_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA0____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA1 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA1____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA1____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA1_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA1____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA2 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA2____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA2____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA2_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA3 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA3____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA3____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA3_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA3____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA4 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA4____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA4____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA4_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA4_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA4____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA5 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA5____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA5____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA5_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA5_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA5____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA6 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA6____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA6____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA6_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA6_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA6____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA7 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA7____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA7____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA7_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA7_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA7____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RESERVED_3 ---- */
#define SUTRA_GLUE_RESERVED_3____WIDTH	8
#define SUTRA_GLUE_RESERVED_3____TYPE 	uint8_t

#define SUTRA_GLUE_RESERVED_3____REGMASK	UINT8_C(0)

/* ---- SUTRA_GLUE_RESERVED_4 ---- */
#define SUTRA_GLUE_RESERVED_4____WIDTH	8
#define SUTRA_GLUE_RESERVED_4____TYPE 	uint8_t

#define SUTRA_GLUE_RESERVED_4____REGMASK	UINT8_C(0)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG0 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG0____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG0____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG0_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG0____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG1 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG1____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG1____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG1_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG1____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG2 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG2____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG2____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG2_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG3 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG3____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG3____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG3_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG3____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG4 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG4____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG4____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG4_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG4_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG4____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG5 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG5____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG5____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG5_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG5_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG5____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG6 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG6____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG6____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG6_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG6_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG6____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG7 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG7____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG7____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG7_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG7_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_DEBUG7____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_SFP_TX_DISABLE ---- */
#define SUTRA_GLUE_SFP_TX_DISABLE____WIDTH	8
#define SUTRA_GLUE_SFP_TX_DISABLE____TYPE 	uint8_t

#define SUTRA_GLUE_SFP_TX_DISABLE_Unused_4___MASK 	UINT8_C(0xf0)
#define SUTRA_GLUE_SFP_TX_DISABLE_Unused_4___SHIFT	4
#define SUTRA_GLUE_SFP_TX_DISABLE_disable___MASK  	UINT8_C(0xf)
#define SUTRA_GLUE_SFP_TX_DISABLE_disable___SHIFT 	0
#define SUTRA_GLUE_SFP_TX_DISABLE____REGMASK	UINT8_C(15)

/* ---- SUTRA_GLUE_ISR_SFP_TX_FAULT ---- */
#define SUTRA_GLUE_ISR_SFP_TX_FAULT____WIDTH	8
#define SUTRA_GLUE_ISR_SFP_TX_FAULT____TYPE 	uint8_t

#define SUTRA_GLUE_ISR_SFP_TX_FAULT_Unused_4___MASK 	UINT8_C(0xf0)
#define SUTRA_GLUE_ISR_SFP_TX_FAULT_Unused_4___SHIFT	4
#define SUTRA_GLUE_ISR_SFP_TX_FAULT_CHG___MASK      	UINT8_C(0xf)
#define SUTRA_GLUE_ISR_SFP_TX_FAULT_CHG___SHIFT     	0
#define SUTRA_GLUE_ISR_SFP_TX_FAULT____REGMASK	UINT8_C(15)

/* ---- SUTRA_GLUE_ISM_SFP_TX_FAULT ---- */
#define SUTRA_GLUE_ISM_SFP_TX_FAULT____WIDTH	8
#define SUTRA_GLUE_ISM_SFP_TX_FAULT____TYPE 	uint8_t

#define SUTRA_GLUE_ISM_SFP_TX_FAULT_Unused_4___MASK 	UINT8_C(0xf0)
#define SUTRA_GLUE_ISM_SFP_TX_FAULT_Unused_4___SHIFT	4
#define SUTRA_GLUE_ISM_SFP_TX_FAULT_MASK___MASK     	UINT8_C(0xf)
#define SUTRA_GLUE_ISM_SFP_TX_FAULT_MASK___SHIFT    	0
#define SUTRA_GLUE_ISM_SFP_TX_FAULT____REGMASK	UINT8_C(15)

/* ---- SUTRA_GLUE_STATUS_SFP_TX_FAULT ---- */
#define SUTRA_GLUE_STATUS_SFP_TX_FAULT____WIDTH	8
#define SUTRA_GLUE_STATUS_SFP_TX_FAULT____TYPE 	uint8_t

#define SUTRA_GLUE_STATUS_SFP_TX_FAULT_Unused_4___MASK 	UINT8_C(0xf0)
#define SUTRA_GLUE_STATUS_SFP_TX_FAULT_Unused_4___SHIFT	4
#define SUTRA_GLUE_STATUS_SFP_TX_FAULT_STAT___MASK     	UINT8_C(0xf)
#define SUTRA_GLUE_STATUS_SFP_TX_FAULT_STAT___SHIFT    	0
#define SUTRA_GLUE_STATUS_SFP_TX_FAULT____REGMASK	UINT8_C(15)

/* ---- SUTRA_GLUE_IST_SFP_TX_FAULT ---- */
#define SUTRA_GLUE_IST_SFP_TX_FAULT____WIDTH	8
#define SUTRA_GLUE_IST_SFP_TX_FAULT____TYPE 	uint8_t

#define SUTRA_GLUE_IST_SFP_TX_FAULT_Unused_4___MASK 	UINT8_C(0xf0)
#define SUTRA_GLUE_IST_SFP_TX_FAULT_Unused_4___SHIFT	4
#define SUTRA_GLUE_IST_SFP_TX_FAULT_TST___MASK      	UINT8_C(0xf)
#define SUTRA_GLUE_IST_SFP_TX_FAULT_TST___SHIFT     	0
#define SUTRA_GLUE_IST_SFP_TX_FAULT____REGMASK	UINT8_C(15)

/* ---- SUTRA_GLUE_PS_I2C_MUX_SEL ---- */
#define SUTRA_GLUE_PS_I2C_MUX_SEL____WIDTH	8
#define SUTRA_GLUE_PS_I2C_MUX_SEL____TYPE 	uint8_t

#define SUTRA_GLUE_PS_I2C_MUX_SEL____REGMASK	UINT8_C(0)

/* ---- SUTRA_GLUE_PSU_CONTROL ---- */
#define SUTRA_GLUE_PSU_CONTROL____WIDTH	8
#define SUTRA_GLUE_PSU_CONTROL____TYPE 	uint8_t

#define SUTRA_GLUE_PSU_CONTROL_Unused_3___MASK      	UINT8_C(0xf8)
#define SUTRA_GLUE_PSU_CONTROL_Unused_3___SHIFT     	3
#define SUTRA_GLUE_PSU_CONTROL_PSB_PSON_L___MASK    	UINT8_C(0x4)
#define SUTRA_GLUE_PSU_CONTROL_PSB_PSON_L___SHIFT   	2
#define SUTRA_GLUE_PSU_CONTROL_PSA_PSON_L___MASK    	UINT8_C(0x2)
#define SUTRA_GLUE_PSU_CONTROL_PSA_PSON_L___SHIFT   	1
#define SUTRA_GLUE_PSU_CONTROL_PSU_EEPROM_WP___MASK 	UINT8_C(0x1)
#define SUTRA_GLUE_PSU_CONTROL_PSU_EEPROM_WP___SHIFT	0
#define SUTRA_GLUE_PSU_CONTROL____REGMASK	UINT8_C(7)

/* ---- SUTRA_GLUE_PSU_STATUS ---- */
#define SUTRA_GLUE_PSU_STATUS____WIDTH	8
#define SUTRA_GLUE_PSU_STATUS____TYPE 	uint8_t

#define SUTRA_GLUE_PSU_STATUS_Unused_6___MASK      	UINT8_C(0xc0)
#define SUTRA_GLUE_PSU_STATUS_Unused_6___SHIFT     	6
#define SUTRA_GLUE_PSU_STATUS_PSB_PRESENT_L___MASK 	UINT8_C(0x20)
#define SUTRA_GLUE_PSU_STATUS_PSB_PRESENT_L___SHIFT	5
#define SUTRA_GLUE_PSU_STATUS_PSA_PRESENT_L___MASK 	UINT8_C(0x10)
#define SUTRA_GLUE_PSU_STATUS_PSA_PRESENT_L___SHIFT	4
#define SUTRA_GLUE_PSU_STATUS_PSB_PWOK_H___MASK    	UINT8_C(0x8)
#define SUTRA_GLUE_PSU_STATUS_PSB_PWOK_H___SHIFT   	3
#define SUTRA_GLUE_PSU_STATUS_PSA_PWOK_H___MASK    	UINT8_C(0x4)
#define SUTRA_GLUE_PSU_STATUS_PSA_PWOK_H___SHIFT   	2
#define SUTRA_GLUE_PSU_STATUS_PSB_ACOK_H___MASK    	UINT8_C(0x2)
#define SUTRA_GLUE_PSU_STATUS_PSB_ACOK_H___SHIFT   	1
#define SUTRA_GLUE_PSU_STATUS_PSA_ACOK_H___MASK    	UINT8_C(0x1)
#define SUTRA_GLUE_PSU_STATUS_PSA_ACOK_H___SHIFT   	0
#define SUTRA_GLUE_PSU_STATUS____REGMASK	UINT8_C(63)

/* ---- SUTRA_GLUE_DEBUG_REG1 ---- */
#define SUTRA_GLUE_DEBUG_REG1____WIDTH	8
#define SUTRA_GLUE_DEBUG_REG1____TYPE 	uint8_t

#define SUTRA_GLUE_DEBUG_REG1____REGMASK	UINT8_C(0)

/* ---- SUTRA_GLUE_DEBUG_REG2 ---- */
#define SUTRA_GLUE_DEBUG_REG2____WIDTH	8
#define SUTRA_GLUE_DEBUG_REG2____TYPE 	uint8_t

#define SUTRA_GLUE_DEBUG_REG2____REGMASK	UINT8_C(0)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER0 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER0____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER0____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER0_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER0____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER1 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER1____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER1____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER1_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER1____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER2 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER2____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER2____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER2_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER3 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER3____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER3____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER3_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER3____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER4 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER4____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER4____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER4_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER4_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER4____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER5 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER5____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER5____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER5_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER5_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER5____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER6 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER6____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER6____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER6_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER6_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER6____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER7 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER7____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER7____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER7_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER7_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER7____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG0 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG0____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG0____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG0_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG0____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG1 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG1____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG1____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG1_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG1____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG2 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG2____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG2____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG2_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG3 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG3____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG3____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG3_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG3____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG4 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG4____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG4____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG4_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG4_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG4____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG5 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG5____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG5____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG5_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG5_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG5____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG6 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG6____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG6____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG6_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG6_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG6____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG7 ---- */
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG7____WIDTH	8
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG7____TYPE 	uint8_t

#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG7_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG7_DATA___SHIFT	0
#define SUTRA_GLUE_BMC_SGPIO_DATA_HIGHER_DEBUG7____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER0 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER0____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER0____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER0_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER0____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER1 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER1____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER1____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER1_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER1____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER2 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER2____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER2____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER2_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER3 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER3____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER3____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER3_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER3____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER4 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER4____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER4____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER4_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER4_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER4____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER5 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER5____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER5____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER5_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER5_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER5____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER6 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER6____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER6____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER6_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER6_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER6____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER7 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER7____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER7____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER7_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER7_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER7____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG0 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG0____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG0____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG0_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG0____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG1 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG1____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG1____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG1_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG1____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG2 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG2____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG2____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG2_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG3 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG3____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG3____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG3_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG3____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG4 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG4____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG4____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG4_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG4_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG4____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG5 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG5____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG5____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG5_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG5_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG5____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG6 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG6____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG6____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG6_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG6_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG6____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG7 ---- */
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG7____WIDTH	8
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG7____TYPE 	uint8_t

#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG7_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG7_DATA___SHIFT	0
#define SUTRA_GLUE_RUDRA_SGPIO_DATA_HIGHER_DEBUG7____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO0____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO1____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO2____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO3____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO4 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO4____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO4____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO4_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO4_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO4____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO5 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO5____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO5____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO5_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO5_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO5____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO6 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO6____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO6____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO6_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO6_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO6____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO7 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO7____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO7____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO7_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO7_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO7____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO8 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO8____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO8____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO8_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO8_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO8____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO9 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO9____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO9____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO9_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO9_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO9____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO10 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO10____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO10____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO10_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO10_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO10____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO11 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO11____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO11____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO11_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO11_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO11____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO12 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO12____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO12____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO12_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO12_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO12____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO13 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO13____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO13____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO13_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO13_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO13____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO14 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO14____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO14____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO14_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO14_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO14____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO15 ---- */
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO15____WIDTH	8
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO15____TYPE 	uint8_t

#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO15_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO15_DATA___SHIFT	0
#define SUTRA_GLUE_TEST_REGISTER_TO_SEND_ON_SGPIO15____REGMASK	UINT8_C(255)

/* ---- SUTRA_GLUE_SPI_ENABLE ---- */
#define SUTRA_GLUE_SPI_ENABLE____WIDTH	8
#define SUTRA_GLUE_SPI_ENABLE____TYPE 	uint8_t

#define SUTRA_GLUE_SPI_ENABLE_Unused_1___MASK           	UINT8_C(0xfe)
#define SUTRA_GLUE_SPI_ENABLE_Unused_1___SHIFT          	1
#define SUTRA_GLUE_SPI_ENABLE_spi_transaction_en___MASK 	UINT8_C(0x1)
#define SUTRA_GLUE_SPI_ENABLE_spi_transaction_en___SHIFT	0
#define SUTRA_GLUE_SPI_ENABLE____REGMASK	UINT8_C(1)

/* ---- SUTRA_GLUE_THEEND ---- */
#define SUTRA_GLUE_THEEND____WIDTH	8
#define SUTRA_GLUE_THEEND____TYPE 	uint8_t

#define SUTRA_GLUE_THEEND____REGMASK	UINT8_C(0)

/* ---- SUTRA_MAIN_I2C_SW_CTRL ---- */
#define SUTRA_MAIN_I2C_SW_CTRL____WIDTH	8
#define SUTRA_MAIN_I2C_SW_CTRL____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_SW_CTRL_Unused_6___MASK 	UINT8_C(0xc0)
#define SUTRA_MAIN_I2C_SW_CTRL_Unused_6___SHIFT	6
#define SUTRA_MAIN_I2C_SW_CTRL_READACK___MASK  	UINT8_C(0x20)
#define SUTRA_MAIN_I2C_SW_CTRL_READACK___SHIFT 	5
#define SUTRA_MAIN_I2C_SW_CTRL_READNACK___MASK 	UINT8_C(0x10)
#define SUTRA_MAIN_I2C_SW_CTRL_READNACK___SHIFT	4
#define SUTRA_MAIN_I2C_SW_CTRL_WRITE___MASK    	UINT8_C(0x8)
#define SUTRA_MAIN_I2C_SW_CTRL_WRITE___SHIFT   	3
#define SUTRA_MAIN_I2C_SW_CTRL_STOP___MASK     	UINT8_C(0x4)
#define SUTRA_MAIN_I2C_SW_CTRL_STOP___SHIFT    	2
#define SUTRA_MAIN_I2C_SW_CTRL_START___MASK    	UINT8_C(0x2)
#define SUTRA_MAIN_I2C_SW_CTRL_START___SHIFT   	1
#define SUTRA_MAIN_I2C_SW_CTRL_RESET___MASK    	UINT8_C(0x1)
#define SUTRA_MAIN_I2C_SW_CTRL_RESET___SHIFT   	0
#define SUTRA_MAIN_I2C_SW_CTRL____REGMASK	UINT8_C(63)

/* ---- SUTRA_MAIN_I2C_SW_STAT ---- */
#define SUTRA_MAIN_I2C_SW_STAT____WIDTH	8
#define SUTRA_MAIN_I2C_SW_STAT____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_SW_STAT_STATE___MASK 	UINT8_C(0xf0)
#define SUTRA_MAIN_I2C_SW_STAT_STATE___SHIFT	4
#define SUTRA_MAIN_I2C_SW_STAT_SDA___MASK   	UINT8_C(0x8)
#define SUTRA_MAIN_I2C_SW_STAT_SDA___SHIFT  	3
#define SUTRA_MAIN_I2C_SW_STAT_SCL___MASK   	UINT8_C(0x4)
#define SUTRA_MAIN_I2C_SW_STAT_SCL___SHIFT  	2
#define SUTRA_MAIN_I2C_SW_STAT_ACKR___MASK  	UINT8_C(0x2)
#define SUTRA_MAIN_I2C_SW_STAT_ACKR___SHIFT 	1
#define SUTRA_MAIN_I2C_SW_STAT_STAT___MASK  	UINT8_C(0x1)
#define SUTRA_MAIN_I2C_SW_STAT_STAT___SHIFT 	0
#define SUTRA_MAIN_I2C_SW_STAT____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_SW_WRITE_DATA ---- */
#define SUTRA_MAIN_I2C_SW_WRITE_DATA____WIDTH	8
#define SUTRA_MAIN_I2C_SW_WRITE_DATA____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_SW_WRITE_DATA_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_SW_WRITE_DATA_DATA___SHIFT	0
#define SUTRA_MAIN_I2C_SW_WRITE_DATA____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_SW_READ_DATA ---- */
#define SUTRA_MAIN_I2C_SW_READ_DATA____WIDTH	8
#define SUTRA_MAIN_I2C_SW_READ_DATA____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_SW_READ_DATA_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_SW_READ_DATA_DATA___SHIFT	0
#define SUTRA_MAIN_I2C_SW_READ_DATA____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_SW_DONE ---- */
#define SUTRA_MAIN_I2C_SW_DONE____WIDTH	8
#define SUTRA_MAIN_I2C_SW_DONE____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_SW_DONE_Unused_1___MASK 	UINT8_C(0xfe)
#define SUTRA_MAIN_I2C_SW_DONE_Unused_1___SHIFT	1
#define SUTRA_MAIN_I2C_SW_DONE_CMP___MASK      	UINT8_C(0x1)
#define SUTRA_MAIN_I2C_SW_DONE_CMP___SHIFT     	0
#define SUTRA_MAIN_I2C_SW_DONE____REGMASK	UINT8_C(1)

/* ---- SUTRA_MAIN_I2C_DIAG_CTRL ---- */
#define SUTRA_MAIN_I2C_DIAG_CTRL____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_CTRL____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_CTRL_RESET_MASTER___MASK     	UINT8_C(0x80)
#define SUTRA_MAIN_I2C_DIAG_CTRL_RESET_MASTER___SHIFT    	7
#define SUTRA_MAIN_I2C_DIAG_CTRL_SLAVE_ADDRS_NACK___MASK 	UINT8_C(0x40)
#define SUTRA_MAIN_I2C_DIAG_CTRL_SLAVE_ADDRS_NACK___SHIFT	6
#define SUTRA_MAIN_I2C_DIAG_CTRL_NO_DATAADDR_MODE___MASK 	UINT8_C(0x20)
#define SUTRA_MAIN_I2C_DIAG_CTRL_NO_DATAADDR_MODE___SHIFT	5
#define SUTRA_MAIN_I2C_DIAG_CTRL_DATAADDR_MODE___MASK    	UINT8_C(0x10)
#define SUTRA_MAIN_I2C_DIAG_CTRL_DATAADDR_MODE___SHIFT   	4
#define SUTRA_MAIN_I2C_DIAG_CTRL_RD_WRn___MASK           	UINT8_C(0x8)
#define SUTRA_MAIN_I2C_DIAG_CTRL_RD_WRn___SHIFT          	3
#define SUTRA_MAIN_I2C_DIAG_CTRL_ABORT___MASK            	UINT8_C(0x4)
#define SUTRA_MAIN_I2C_DIAG_CTRL_ABORT___SHIFT           	2
#define SUTRA_MAIN_I2C_DIAG_CTRL_FINISH___MASK           	UINT8_C(0x2)
#define SUTRA_MAIN_I2C_DIAG_CTRL_FINISH___SHIFT          	1
#define SUTRA_MAIN_I2C_DIAG_CTRL_START___MASK            	UINT8_C(0x1)
#define SUTRA_MAIN_I2C_DIAG_CTRL_START___SHIFT           	0
#define SUTRA_MAIN_I2C_DIAG_CTRL____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_DIAG_4X ---- */
#define SUTRA_MAIN_I2C_DIAG_4X____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_4X____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_4X_CLK_DIV___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_DIAG_4X_CLK_DIV___SHIFT	0
#define SUTRA_MAIN_I2C_DIAG_4X____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_DIAG_DEVADDR ---- */
#define SUTRA_MAIN_I2C_DIAG_DEVADDR____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DEVADDR____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DEVADDR____REGMASK	UINT8_C(0)

/* ---- SUTRA_MAIN_I2C_DIAG_DATAADDR_0 ---- */
#define SUTRA_MAIN_I2C_DIAG_DATAADDR_0____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DATAADDR_0____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DATAADDR_0____REGMASK	UINT8_C(0)

/* ---- SUTRA_MAIN_I2C_DIAG_DATAADDR_1 ---- */
#define SUTRA_MAIN_I2C_DIAG_DATAADDR_1____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DATAADDR_1____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DATAADDR_1____REGMASK	UINT8_C(0)

/* ---- SUTRA_MAIN_I2C_DIAG_DATA_VLD_0 ---- */
#define SUTRA_MAIN_I2C_DIAG_DATA_VLD_0____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DATA_VLD_0____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DATA_VLD_0____REGMASK	UINT8_C(0)

/* ---- SUTRA_MAIN_I2C_DIAG_DEBUG_0 ---- */
#define SUTRA_MAIN_I2C_DIAG_DEBUG_0____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DEBUG_0____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DEBUG_0_ABORT_LOC___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_DIAG_DEBUG_0_ABORT_LOC___SHIFT	0
#define SUTRA_MAIN_I2C_DIAG_DEBUG_0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_DIAG_DATA_WR0 ---- */
#define SUTRA_MAIN_I2C_DIAG_DATA_WR0____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DATA_WR0____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DATA_WR0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_DIAG_DATA_WR0_DATA___SHIFT	0
#define SUTRA_MAIN_I2C_DIAG_DATA_WR0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_DIAG_DATA_WR1 ---- */
#define SUTRA_MAIN_I2C_DIAG_DATA_WR1____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DATA_WR1____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DATA_WR1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_DIAG_DATA_WR1_DATA___SHIFT	0
#define SUTRA_MAIN_I2C_DIAG_DATA_WR1____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_DIAG_DATA_WR2 ---- */
#define SUTRA_MAIN_I2C_DIAG_DATA_WR2____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DATA_WR2____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DATA_WR2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_DIAG_DATA_WR2_DATA___SHIFT	0
#define SUTRA_MAIN_I2C_DIAG_DATA_WR2____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_DIAG_DATA_WR3 ---- */
#define SUTRA_MAIN_I2C_DIAG_DATA_WR3____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DATA_WR3____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DATA_WR3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_DIAG_DATA_WR3_DATA___SHIFT	0
#define SUTRA_MAIN_I2C_DIAG_DATA_WR3____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_DIAG_DATA_RD0 ---- */
#define SUTRA_MAIN_I2C_DIAG_DATA_RD0____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DATA_RD0____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DATA_RD0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_DIAG_DATA_RD0_DATA___SHIFT	0
#define SUTRA_MAIN_I2C_DIAG_DATA_RD0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_DIAG_DATA_RD1 ---- */
#define SUTRA_MAIN_I2C_DIAG_DATA_RD1____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DATA_RD1____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DATA_RD1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_DIAG_DATA_RD1_DATA___SHIFT	0
#define SUTRA_MAIN_I2C_DIAG_DATA_RD1____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_DIAG_DATA_RD2 ---- */
#define SUTRA_MAIN_I2C_DIAG_DATA_RD2____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DATA_RD2____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DATA_RD2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_DIAG_DATA_RD2_DATA___SHIFT	0
#define SUTRA_MAIN_I2C_DIAG_DATA_RD2____REGMASK	UINT8_C(255)

/* ---- SUTRA_MAIN_I2C_DIAG_DATA_RD3 ---- */
#define SUTRA_MAIN_I2C_DIAG_DATA_RD3____WIDTH	8
#define SUTRA_MAIN_I2C_DIAG_DATA_RD3____TYPE 	uint8_t

#define SUTRA_MAIN_I2C_DIAG_DATA_RD3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MAIN_I2C_DIAG_DATA_RD3_DATA___SHIFT	0
#define SUTRA_MAIN_I2C_DIAG_DATA_RD3____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_SW_CTRL ---- */
#define SUTRA_MORE_I2C_PS_SW_CTRL____WIDTH	8
#define SUTRA_MORE_I2C_PS_SW_CTRL____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_SW_CTRL_Unused_6___MASK 	UINT8_C(0xc0)
#define SUTRA_MORE_I2C_PS_SW_CTRL_Unused_6___SHIFT	6
#define SUTRA_MORE_I2C_PS_SW_CTRL_READACK___MASK  	UINT8_C(0x20)
#define SUTRA_MORE_I2C_PS_SW_CTRL_READACK___SHIFT 	5
#define SUTRA_MORE_I2C_PS_SW_CTRL_READNACK___MASK 	UINT8_C(0x10)
#define SUTRA_MORE_I2C_PS_SW_CTRL_READNACK___SHIFT	4
#define SUTRA_MORE_I2C_PS_SW_CTRL_WRITE___MASK    	UINT8_C(0x8)
#define SUTRA_MORE_I2C_PS_SW_CTRL_WRITE___SHIFT   	3
#define SUTRA_MORE_I2C_PS_SW_CTRL_STOP___MASK     	UINT8_C(0x4)
#define SUTRA_MORE_I2C_PS_SW_CTRL_STOP___SHIFT    	2
#define SUTRA_MORE_I2C_PS_SW_CTRL_START___MASK    	UINT8_C(0x2)
#define SUTRA_MORE_I2C_PS_SW_CTRL_START___SHIFT   	1
#define SUTRA_MORE_I2C_PS_SW_CTRL_RESET___MASK    	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PS_SW_CTRL_RESET___SHIFT   	0
#define SUTRA_MORE_I2C_PS_SW_CTRL____REGMASK	UINT8_C(63)

/* ---- SUTRA_MORE_I2C_PS_SW_STAT ---- */
#define SUTRA_MORE_I2C_PS_SW_STAT____WIDTH	8
#define SUTRA_MORE_I2C_PS_SW_STAT____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_SW_STAT_STATE___MASK 	UINT8_C(0xf0)
#define SUTRA_MORE_I2C_PS_SW_STAT_STATE___SHIFT	4
#define SUTRA_MORE_I2C_PS_SW_STAT_SDA___MASK   	UINT8_C(0x8)
#define SUTRA_MORE_I2C_PS_SW_STAT_SDA___SHIFT  	3
#define SUTRA_MORE_I2C_PS_SW_STAT_SCL___MASK   	UINT8_C(0x4)
#define SUTRA_MORE_I2C_PS_SW_STAT_SCL___SHIFT  	2
#define SUTRA_MORE_I2C_PS_SW_STAT_ACKR___MASK  	UINT8_C(0x2)
#define SUTRA_MORE_I2C_PS_SW_STAT_ACKR___SHIFT 	1
#define SUTRA_MORE_I2C_PS_SW_STAT_STAT___MASK  	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PS_SW_STAT_STAT___SHIFT 	0
#define SUTRA_MORE_I2C_PS_SW_STAT____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_SW_WRITE_DATA ---- */
#define SUTRA_MORE_I2C_PS_SW_WRITE_DATA____WIDTH	8
#define SUTRA_MORE_I2C_PS_SW_WRITE_DATA____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_SW_WRITE_DATA_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_SW_WRITE_DATA_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PS_SW_WRITE_DATA____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_SW_READ_DATA ---- */
#define SUTRA_MORE_I2C_PS_SW_READ_DATA____WIDTH	8
#define SUTRA_MORE_I2C_PS_SW_READ_DATA____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_SW_READ_DATA_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_SW_READ_DATA_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PS_SW_READ_DATA____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_SW_DONE ---- */
#define SUTRA_MORE_I2C_PS_SW_DONE____WIDTH	8
#define SUTRA_MORE_I2C_PS_SW_DONE____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_SW_DONE_Unused_1___MASK 	UINT8_C(0xfe)
#define SUTRA_MORE_I2C_PS_SW_DONE_Unused_1___SHIFT	1
#define SUTRA_MORE_I2C_PS_SW_DONE_CMP___MASK      	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PS_SW_DONE_CMP___SHIFT     	0
#define SUTRA_MORE_I2C_PS_SW_DONE____REGMASK	UINT8_C(1)

/* ---- SUTRA_MORE_I2C_PS_DIAG_CTRL ---- */
#define SUTRA_MORE_I2C_PS_DIAG_CTRL____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_CTRL____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_CTRL_RESET_MASTER___MASK     	UINT8_C(0x80)
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_RESET_MASTER___SHIFT    	7
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_Unused_6___MASK         	UINT8_C(0x40)
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_Unused_6___SHIFT        	6
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_NO_DATAADDR_MODE___MASK 	UINT8_C(0x20)
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_NO_DATAADDR_MODE___SHIFT	5
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_DATAADDR_MODE___MASK    	UINT8_C(0x10)
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_DATAADDR_MODE___SHIFT   	4
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_RD_WRn___MASK           	UINT8_C(0x8)
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_RD_WRn___SHIFT          	3
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_ABORT___MASK            	UINT8_C(0x4)
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_ABORT___SHIFT           	2
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_FINISH___MASK           	UINT8_C(0x2)
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_FINISH___SHIFT          	1
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_START___MASK            	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PS_DIAG_CTRL_START___SHIFT           	0
#define SUTRA_MORE_I2C_PS_DIAG_CTRL____REGMASK	UINT8_C(191)

/* ---- SUTRA_MORE_I2C_PS_DIAG_4X ---- */
#define SUTRA_MORE_I2C_PS_DIAG_4X____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_4X____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_4X_CLK_DIV___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_DIAG_4X_CLK_DIV___SHIFT	0
#define SUTRA_MORE_I2C_PS_DIAG_4X____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DEVADDR ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DEVADDR____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DEVADDR____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DEVADDR____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DATAADDR_0 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DATAADDR_0____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DATAADDR_0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DATAADDR_0____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DATAADDR_1 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DATAADDR_1____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DATAADDR_1____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DATAADDR_1____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DATA_VLD_0 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DATA_VLD_0____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DATA_VLD_0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DATA_VLD_0____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DEBUG_0 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DEBUG_0____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DEBUG_0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DEBUG_0_ABORT_LOC___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_DIAG_DEBUG_0_ABORT_LOC___SHIFT	0
#define SUTRA_MORE_I2C_PS_DIAG_DEBUG_0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DATA_WR0 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR0____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR0_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DATA_WR1 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR1____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR1____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR1_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR1____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DATA_WR2 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR2____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR2____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR2_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR2____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DATA_WR3 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR3____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR3____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR3_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PS_DIAG_DATA_WR3____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DATA_RD0 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD0____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD0_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DATA_RD1 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD1____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD1____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD1_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD1____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DATA_RD2 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD2____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD2____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD2_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD2____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PS_DIAG_DATA_RD3 ---- */
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD3____WIDTH	8
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD3____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD3_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PS_DIAG_DATA_RD3____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_SW_CTRL ---- */
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_Unused_6___MASK 	UINT8_C(0xc0)
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_Unused_6___SHIFT	6
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_READACK___MASK  	UINT8_C(0x20)
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_READACK___SHIFT 	5
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_READNACK___MASK 	UINT8_C(0x10)
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_READNACK___SHIFT	4
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_WRITE___MASK    	UINT8_C(0x8)
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_WRITE___SHIFT   	3
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_STOP___MASK     	UINT8_C(0x4)
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_STOP___SHIFT    	2
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_START___MASK    	UINT8_C(0x2)
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_START___SHIFT   	1
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_RESET___MASK    	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL_RESET___SHIFT   	0
#define SUTRA_MORE_I2C_PCIE_SW_SW_CTRL____REGMASK	UINT8_C(63)

/* ---- SUTRA_MORE_I2C_PCIE_SW_SW_STAT ---- */
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT_STATE___MASK 	UINT8_C(0xf0)
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT_STATE___SHIFT	4
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT_SDA___MASK   	UINT8_C(0x8)
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT_SDA___SHIFT  	3
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT_SCL___MASK   	UINT8_C(0x4)
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT_SCL___SHIFT  	2
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT_ACKR___MASK  	UINT8_C(0x2)
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT_ACKR___SHIFT 	1
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT_STAT___MASK  	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT_STAT___SHIFT 	0
#define SUTRA_MORE_I2C_PCIE_SW_SW_STAT____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_SW_WRITE_DATA ---- */
#define SUTRA_MORE_I2C_PCIE_SW_SW_WRITE_DATA____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_SW_WRITE_DATA____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_SW_WRITE_DATA_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_SW_WRITE_DATA_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_SW_WRITE_DATA____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_SW_READ_DATA ---- */
#define SUTRA_MORE_I2C_PCIE_SW_SW_READ_DATA____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_SW_READ_DATA____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_SW_READ_DATA_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_SW_READ_DATA_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_SW_READ_DATA____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_SW_DONE ---- */
#define SUTRA_MORE_I2C_PCIE_SW_SW_DONE____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_SW_DONE____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_SW_DONE_Unused_1___MASK 	UINT8_C(0xfe)
#define SUTRA_MORE_I2C_PCIE_SW_SW_DONE_Unused_1___SHIFT	1
#define SUTRA_MORE_I2C_PCIE_SW_SW_DONE_CMP___MASK      	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PCIE_SW_SW_DONE_CMP___SHIFT     	0
#define SUTRA_MORE_I2C_PCIE_SW_SW_DONE____REGMASK	UINT8_C(1)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_RESET_MASTER___MASK     	UINT8_C(0x80)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_RESET_MASTER___SHIFT    	7
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_Unused_6___MASK         	UINT8_C(0x40)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_Unused_6___SHIFT        	6
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_NO_DATAADDR_MODE___MASK 	UINT8_C(0x20)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_NO_DATAADDR_MODE___SHIFT	5
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_DATAADDR_MODE___MASK    	UINT8_C(0x10)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_DATAADDR_MODE___SHIFT   	4
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_RD_WRn___MASK           	UINT8_C(0x8)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_RD_WRn___SHIFT          	3
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_ABORT___MASK            	UINT8_C(0x4)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_ABORT___SHIFT           	2
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_FINISH___MASK           	UINT8_C(0x2)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_FINISH___SHIFT          	1
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_START___MASK            	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL_START___SHIFT           	0
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL____REGMASK	UINT8_C(191)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_4X ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_4X____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_4X____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_4X_CLK_DIV___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_4X_CLK_DIV___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_4X____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DEVADDR ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DEVADDR____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DEVADDR____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DEVADDR____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DATAADDR_0 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATAADDR_0____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATAADDR_0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATAADDR_0____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DATAADDR_1 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATAADDR_1____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATAADDR_1____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATAADDR_1____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_VLD_0 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_VLD_0____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_VLD_0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_VLD_0____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DEBUG_0 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DEBUG_0____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DEBUG_0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DEBUG_0_ABORT_LOC___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DEBUG_0_ABORT_LOC___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DEBUG_0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR0 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR0____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR0_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR1 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR1____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR1____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR1_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR1____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR2 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR2____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR2____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR2_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR2____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR3 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR3____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR3____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR3_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR3____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD0 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD0____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD0_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD1 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD1____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD1____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD1_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD1____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD2 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD2____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD2____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD2_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD2____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD3 ---- */
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD3____WIDTH	8
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD3____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD3_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD3____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_SW_CTRL ---- */
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_Unused_6___MASK 	UINT8_C(0xc0)
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_Unused_6___SHIFT	6
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_READACK___MASK  	UINT8_C(0x20)
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_READACK___SHIFT 	5
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_READNACK___MASK 	UINT8_C(0x10)
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_READNACK___SHIFT	4
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_WRITE___MASK    	UINT8_C(0x8)
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_WRITE___SHIFT   	3
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_STOP___MASK     	UINT8_C(0x4)
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_STOP___SHIFT    	2
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_START___MASK    	UINT8_C(0x2)
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_START___SHIFT   	1
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_RESET___MASK    	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL_RESET___SHIFT   	0
#define SUTRA_MORE_I2C_PMBUS_SW_CTRL____REGMASK	UINT8_C(63)

/* ---- SUTRA_MORE_I2C_PMBUS_SW_STAT ---- */
#define SUTRA_MORE_I2C_PMBUS_SW_STAT____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_SW_STAT____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_SW_STAT_STATE___MASK 	UINT8_C(0xf0)
#define SUTRA_MORE_I2C_PMBUS_SW_STAT_STATE___SHIFT	4
#define SUTRA_MORE_I2C_PMBUS_SW_STAT_SDA___MASK   	UINT8_C(0x8)
#define SUTRA_MORE_I2C_PMBUS_SW_STAT_SDA___SHIFT  	3
#define SUTRA_MORE_I2C_PMBUS_SW_STAT_SCL___MASK   	UINT8_C(0x4)
#define SUTRA_MORE_I2C_PMBUS_SW_STAT_SCL___SHIFT  	2
#define SUTRA_MORE_I2C_PMBUS_SW_STAT_ACKR___MASK  	UINT8_C(0x2)
#define SUTRA_MORE_I2C_PMBUS_SW_STAT_ACKR___SHIFT 	1
#define SUTRA_MORE_I2C_PMBUS_SW_STAT_STAT___MASK  	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PMBUS_SW_STAT_STAT___SHIFT 	0
#define SUTRA_MORE_I2C_PMBUS_SW_STAT____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_SW_WRITE_DATA ---- */
#define SUTRA_MORE_I2C_PMBUS_SW_WRITE_DATA____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_SW_WRITE_DATA____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_SW_WRITE_DATA_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_SW_WRITE_DATA_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_SW_WRITE_DATA____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_SW_READ_DATA ---- */
#define SUTRA_MORE_I2C_PMBUS_SW_READ_DATA____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_SW_READ_DATA____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_SW_READ_DATA_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_SW_READ_DATA_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_SW_READ_DATA____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_SW_DONE ---- */
#define SUTRA_MORE_I2C_PMBUS_SW_DONE____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_SW_DONE____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_SW_DONE_Unused_1___MASK 	UINT8_C(0xfe)
#define SUTRA_MORE_I2C_PMBUS_SW_DONE_Unused_1___SHIFT	1
#define SUTRA_MORE_I2C_PMBUS_SW_DONE_CMP___MASK      	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PMBUS_SW_DONE_CMP___SHIFT     	0
#define SUTRA_MORE_I2C_PMBUS_SW_DONE____REGMASK	UINT8_C(1)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_CTRL ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_RESET_MASTER___MASK     	UINT8_C(0x80)
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_RESET_MASTER___SHIFT    	7
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_Unused_6___MASK         	UINT8_C(0x40)
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_Unused_6___SHIFT        	6
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_NO_DATAADDR_MODE___MASK 	UINT8_C(0x20)
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_NO_DATAADDR_MODE___SHIFT	5
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_DATAADDR_MODE___MASK    	UINT8_C(0x10)
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_DATAADDR_MODE___SHIFT   	4
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_RD_WRn___MASK           	UINT8_C(0x8)
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_RD_WRn___SHIFT          	3
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_ABORT___MASK            	UINT8_C(0x4)
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_ABORT___SHIFT           	2
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_FINISH___MASK           	UINT8_C(0x2)
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_FINISH___SHIFT          	1
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_START___MASK            	UINT8_C(0x1)
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL_START___SHIFT           	0
#define SUTRA_MORE_I2C_PMBUS_DIAG_CTRL____REGMASK	UINT8_C(191)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_4X ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_4X____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_4X____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_4X_CLK_DIV___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_DIAG_4X_CLK_DIV___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_DIAG_4X____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DEVADDR ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DEVADDR____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DEVADDR____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DEVADDR____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DATAADDR_0 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATAADDR_0____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATAADDR_0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DATAADDR_0____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DATAADDR_1 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATAADDR_1____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATAADDR_1____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DATAADDR_1____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DATA_VLD_0 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_VLD_0____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_VLD_0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_VLD_0____REGMASK	UINT8_C(0)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DEBUG_0 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DEBUG_0____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DEBUG_0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DEBUG_0_ABORT_LOC___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_DIAG_DEBUG_0_ABORT_LOC___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_DIAG_DEBUG_0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR0 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR0____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR0_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR1 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR1____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR1____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR1_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR1____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR2 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR2____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR2____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR2_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR2____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR3 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR3____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR3____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR3_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR3____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD0 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD0____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD0____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD0_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD0_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD0____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD1 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD1____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD1____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD1_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD1_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD1____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD2 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD2____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD2____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD2_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD2_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD2____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD3 ---- */
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD3____WIDTH	8
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD3____TYPE 	uint8_t

#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD3_DATA___MASK 	UINT8_C(0xff)
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD3_DATA___SHIFT	0
#define SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD3____REGMASK	UINT8_C(255)

/* ---- SUTRA_MORE_I2C_THEEND ---- */
#define SUTRA_MORE_I2C_THEEND____WIDTH	8
#define SUTRA_MORE_I2C_THEEND____TYPE 	uint8_t

#define SUTRA_MORE_I2C_THEEND____REGMASK	UINT8_C(0)

#ifdef __KERNEL__
#ifdef CONFIG_REGMAP_MMIO

static const struct regmap_config sutra_regmap_config = {
        .reg_bits     = 8,
        .val_bits     = 8,
        .reg_stride   = 1,
        .max_register = sizeof(struct Sutra_dev_reg),
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
