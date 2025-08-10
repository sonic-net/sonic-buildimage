/* register offset define */
#define CPLD_VER_REG            0x20

#define WDT_EN_REG              0xa0
#define WDT_EN_ENABLE           0x10
#define WDT_EN_DISABLE          0xef

#define EEPROM_WP_REG           0xa0
#define EEPROM_WP_ENABLE        0x10
#define EEPROM_WP_DISABLE       0xef

#define USB_EN_REG              0xa0
#define USB_EN_ENABLE           0x01
#define USB_EN_DISABLE          0xfe

#define MAC_RESET_REG           0xa1
#define MAC_RESET               0x00

#define SHUTDOWN_REG            0xa1
#define SHUTDOWN                0x10

#define BMC_EN_REG              0xa3
#define BMC_EN_ENABLE           0x01
#define BMC_EN_DISABLE          0x00

#define SW_ALARM_REG            0xc2
#define SW_ALERT_MASK_REG       0xc3

#define THERMAL_INT_REG         0xc0
#define THERMAL_INT_MASK_REG    0xc1

#define SENSOR_INT_REG          0xd0
#define SENSOR_INT_MASK_REG     0xd1

#define MODULE_RESET_REG        0xa2
#define MODULE_PRESENT_REG      0xa3
#define MODULE_POWER_REG        0xa4
#define MODULE_ENABLE_REG       0xa5

#define SWITCH_INT_REG          0xd0
#define SWITCH_INT_MASK_REG     0xd1

#define SFP_SELECT_REG          0x60
#define SFP_NON_SELECT          0
#define SFP_PORT_1              1
#define SFP_PORT_2              2
#define SFP_PORT_MGM            3

#define SFP_TX_REG              0x70
#define SFP_PRESENT_REG         0x80
#define SFP_RX_REG              0x90
#define SFP_INT_REG             0xd0

#define SFP_PORT_1_ON           1
#define SFP_PORT_1_OFF          2
#define SFP_PORT_2_ON           3
#define SFP_PORT_2_OFF          4
#define SFP_PORT_MGM_ON         5
#define SFP_PORT_MGM_OFF        6

#define PSU_INT_REG             0xd0
#define SYS_INT_MASK_REG        0xd1

/* end of register offset define */