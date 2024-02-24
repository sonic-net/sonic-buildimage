#ifndef __FIRMWARE_APP_H__
#define __FIRMWARE_APP_H__

#include <sys/ioctl.h>
#include <debug_ispvme.h>
#include <string.h>

/* Agreement with PKG_MGMT module */
#define ERR_FW_CHECK_CPLD_UPGRADE    (440 - 256)
#define ERR_FW_CHECK_FPGA_UPGRADE    (441 - 256)
#define ERR_FW_DO_CPLD_UPGRADE       (442 - 256)
#define ERR_FW_DO_FPGA_UPGRADE       (443 - 256)
#define ERR_FW_UPGRADE               (444 - 256)

#define FIRMWARE_FAILED              -1
#define FIRMWARE_SUCCESS             0

#define FIRMWARE_ACTION_CHECK        0
#define FIRMWARE_ACTION_UPGRADE      1

#define mem_clear(data, size) memset((data), 0, (size))

#define dbg_print(debug, fmt, arg...)  \
    if (debug == DEBUG_APP_ON || debug == DEBUG_ALL_ON) \
        { do{printf(fmt,##arg);} while(0); }

#define FIRMWARE_FILE_DIR_LEN        128
#define FIRMWARE_NAME_LEN            48
#define FIRMWARE_MAX_SLOT_NUM        32

#define FIRMWARE_CPLD_NAME           "cpld"
#define FIRMWARE_FPGA_NAME           "fpga"

#define FIRMWARE_CPLD_EPM1270F256    "EPM1270F256"
#define FIRMWARE_CPLD_5M1270         "5M1270"
#define FIRMWARE_CPLD_TEST           "test"
#define FIRMWARE_FPGA_TEST           "test"

/* ioctl command */
#define FIRMWARE_TYPE 'F'

#define FIRMWARE_JTAG_TDI                _IOR(FIRMWARE_TYPE, 0, char)
#define FIRMWARE_JTAG_TDO                _IOR(FIRMWARE_TYPE, 1, char)
#define FIRMWARE_JTAG_TCK                _IOR(FIRMWARE_TYPE, 2, char)
#define FIRMWARE_JTAG_TMS                _IOR(FIRMWARE_TYPE, 3, char)
#define FIRMWARE_JTAG_EN                 _IOR(FIRMWARE_TYPE, 4, char)
#define FIRMWARE_SET_DEBUG_ON            _IOW(FIRMWARE_TYPE, 5, int)     /* debug on */
#define FIRMWARE_SET_DEBUG_OFF           _IOW(FIRMWARE_TYPE, 6, int)     /* debug off */
#define FIRMWARE_SET_GPIO_INFO           _IOR(FIRMWARE_TYPE, 7, int)     /* GPIO info */

#define FIRMWARE_MAX_SUB_SLOT_NUM    (8)
#define FIRMWARE_MAX_CARD_SLOT_NUM   ((FIRMWARE_MAX_SUB_SLOT_NUM) + (1))/* max slot num */
#define FIRMWARE_CARD_NAME_MAX_LEN      (64)
#define GPIO(p, v, d) {  \
                   .pin = p,  \
                   .val = v,  \
                   .dir = d, \
              },

enum firmware_type_s {
    FIRMWARE_CPLD = 0,
    FIRMWARE_FPGA,
    FIRMWARE_OTHER,
};

typedef struct name_info_s {
    char card_name[FIRMWARE_NAME_LEN];    /* card name */
    int type;                             /* upgrade file type */
    int slot;                             /* slot number correspoding to upgrade file  */
    char chip_name[FIRMWARE_NAME_LEN];    /* chip name */
    char version[FIRMWARE_NAME_LEN];      /* version */
} name_info_t;

typedef struct cmd_info_s {
    int size;
    void *data;
} cmd_info_t;

typedef struct gpio_group_s {
    int pin;
    int val;
    int dir;
} gpio_group_t;

typedef struct firmware_upg_gpio_info_s {
    int tdi;
    int tck;
    int tms;
    int tdo;
    int jtag_en;
	int select;
    gpio_group_t jtag_5;
    gpio_group_t jtag_4;
    gpio_group_t jtag_3;
    gpio_group_t jtag_2;
    gpio_group_t jtag_1;
} firmware_upg_gpio_info_t;

typedef struct firmware_card_info_s {
    int dev_type;                          /* the type of card */
	int slot_num;
	char card_name[FIRMWARE_CARD_NAME_MAX_LEN];
    firmware_upg_gpio_info_t gpio_info[FIRMWARE_MAX_CARD_SLOT_NUM];                        /* private data */
} firmware_card_info_t;

typedef enum card_type_e {
    M2_W6010_48GT4X_FA   = 0X4065,
    M2_W6010_48GT4X_RA   = 0X4065,
} card_type_t;

extern firmware_card_info_t* firmware_get_card_info(int dev_type);
extern int dfd_fpga_upgrade_do_upgrade(char* upg_file);
extern int dfd_fpga_upgrade_test(void);
extern int ispvme_test(void);
extern int ispvme_main(int argc, char *argv[]);
/* External fpga dump interface */
extern int dfd_fpga_upgrade_dump_flash(int argc, char* argv[]);

#endif /* End of __FIRMWARE_APP_H__ */