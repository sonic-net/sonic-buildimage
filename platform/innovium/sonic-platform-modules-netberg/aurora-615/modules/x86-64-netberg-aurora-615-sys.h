/* register offset define */
#define CPLD_VER_REG            0x20
#define WDT_EN_REG              0xa0
#define WDT_EN_ENABLE           0x10
#define WDT_EN_DISABLE          0xef
#define EEPROM_WP_REG           0xa0
#define EEPROM_WP_ENABLE        0x04
#define EEPROM_WP_DISABLE       0xfB
#define USB_EN_REG              0xa0
#define USB_EN_ENABLE           0x02
#define USB_EN_DISABLE          0xfD
#define MAC_RESET_REG           0xa1
#define MAC_RESET               0x00
#define SHUTDOWN_REG            0xa1
#define SHUTDOWN                0x10
#define BMC_EN_REG              0xa4
#define BMC_EN_ENABLE           0x01
#define BMC_EN_DISABLE          0x00
#define THERMAL_INT_REG         0xc0
#define THERMAL_INT_MASK_REG    0xc1
#define SYS_INT_REG             0xd0
#define SYS_INT_MASK_REG        0xd1
/* end of register offset define */