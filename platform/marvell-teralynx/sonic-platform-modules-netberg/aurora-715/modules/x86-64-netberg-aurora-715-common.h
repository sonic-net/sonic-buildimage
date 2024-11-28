/* register offset define */
#define ENABLE          1
#define DISABLE         0

#define PASSED          1
#define FAILED          0

#define TRUE            1
#define FALSE           0

#define ABNORMAL        1
#define NORMAL          0

#define BIT_0_MASK      0x01
#define BIT_1_MASK      0x02
#define BIT_2_MASK      0x04
#define BIT_3_MASK      0x08
#define BIT_4_MASK      0x10
#define BIT_5_MASK      0x20
#define BIT_6_MASK      0x40
#define BIT_7_MASK      0x80
/* end of register offset define */

int bmc_enable(void);
int read_8bit_temp(u8 sign,u8 value);