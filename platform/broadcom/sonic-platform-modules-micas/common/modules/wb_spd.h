#ifndef _WB_SPD_H_
#define _WB_SPD_H_

#define SMB_EEPROM                  (0)
#define SMB_TSOD                    (1)
/* Intel D1500 */
#define INTEL_D1500_SPD_DEVICE_ID   (0x6fa8)
#define INTEL_D1500_SELECT_EEPROM   (0xA0000000)
#define INTEL_D1500_SELECT_TSOD     (0x30000000)
#define INTEL_D1500_CMD_SEND        BIT(31)
#define INTEL_D1500_CMD_WORD_MODE   BIT(29)
#define INTEL_D1500_SMBUS_ERROR     BIT(29)
#define INTEL_D1500_CLOCK_OVERRIDE  BIT(27)
#define INTEL_D1500_SMBUS_RDO       BIT(31)
#define INTEL_D1500_DTI_WP_EEPROM   (0x06 << 28)
#define INTEL_D1500_CMD_SMB_READ    (0x00 << 27)
#define INTEL_D1500_CMD_SMB_WRITE   (0x01 << 27)
#define INTEL_D1500_EE_PAGE_SEL0    (0x06)
#define INTEL_D1500_EE_PAGE_SEL1    (0x07)

#define INTEL_D1500_SMB_CNTL        (0x188)
#define INTEL_D1500_SMB_CMD         (0x184)
#define INTEL_D1500_SMB_STAT        (0x180)
/* Intel D1700 */
#define INTEL_D1700_SPD_DEVICE_ID   (0x3448)
#define INTEL_D1700_SELECT_EEPROM   (0x00005000)
#define INTEL_D1700_SELECT_TSOD     (0x00001800)
#define INTEL_D1700_CLOCK_OVERRIDE  BIT(29)
#define INTEL_D1700_CMD_SEND        BIT(19)
#define INTEL_D1700_CMD_WORD_MODE   BIT(17)
#define INTEL_D1700_CMD_SMB_READ    (0x00 << 15)
#define INTEL_D1700_CMD_SMB_WRITE   (0x01 << 15)
#define INTEL_D1700_DTI_WP_EEPROM   (0x06 << 11)
#define INTEL_D1700_EE_PAGE_SEL0    (0x06)
#define INTEL_D1700_EE_PAGE_SEL1    (0x07)

#define INTEL_D1700_SMBUS_ERROR     BIT(1)
#define INTEL_D1700_SMBUS_BUSY      BIT(0)
#define INTEL_D1700_SMBUS_TSOD_EN   BIT(20)
#define INTEL_D1700_SMB_CMD_CFG     (0x80)
#define INTEL_D1700_SMB_STAT_CFG    (0x84)
#define INTEL_D1700_SMB_DATA_CFG    (0x88)

typedef enum cpu_type {
    TYPE_INTEL_D1500 = 0,
    TYPE_INTEL_D1700 = 1,
} cpu_type_t;

#define MASK_16         (0xFF)
#define MASK_32         (0xFFFF)
#define MAX_REG         (0x1FF)
#define WAIT_TIME       (100)
#define SLEEP_TIME      (50)

#define MEM_TEMP_REG                    (0x5)
#define MEM_TEMP_VALID_VALUE_MASK       (0x1FFC)
#define MEM_TEMP_SIGN_BIT               (0x400)
#define MEM_TEMP_DECIMAL_BITS_MASK      (0x03)
#define MEM_TEMP_INTEGER_BITS_MASK      (0x7F)
#define MEM_DATE_CODE_REG               (0x143)

#endif  /* _WB_SPD_H_ */
