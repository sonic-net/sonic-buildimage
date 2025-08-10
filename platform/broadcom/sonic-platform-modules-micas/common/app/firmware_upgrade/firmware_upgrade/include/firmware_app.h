#ifndef __FIRMWARE_APP_H__
#define __FIRMWARE_APP_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <debug.h>
#include <stdint.h>

#define ERR_FW_CHECK_CPLD_UPGRADE       (-601)   /* File validation error */
#define ERR_FW_CHECK_FPGA_UPGRADE       (-602)
#define ERR_FW_MATCH_CPLD_UPGRADE       (-603)   /* No matching upgrade file found */
#define ERR_FW_MATCH_FPGA_UPGRADE       (-604)
#define ERR_FW_SAMEVER_CPLD_UPGRADE     (-605)   /* the same version */
#define ERR_FW_SAMEVER_FPGA_UPGRADE     (-606)
#define ERR_FW_DO_CPLD_UPGRADE          (-607)   /* upgrade fail */
#define ERR_FW_DO_FPGA_UPGRADE          (-608)
#define ERR_FW_UPGRADE                  (-609)   /* other fail */
#define ERR_FW_CHECK_UPGRADE            (-610)   /* File validation error */
#define ERR_FW_MATCH_UPGRADE            (-611)   /* No matching upgrade file found */
#define ERR_FW_SAMEVER_UPGRADE          (-612)   /* the same version */
#define ERR_FW_DO_UPGRADE               (-613)   /* upgrade fail */
#define ERR_FW_DO_UPGRADE_NOT_SUPPORT   (-614)   /* upgrade fail */

#define FIRMWARE_NOT_SUPPORT         (-2)
#define FIRMWARE_FAILED              (-1)
#define FIRMWARE_SUCCESS             (0)

#define FIRMWARE_ACTION_CHECK        0
#define FIRMWARE_ACTION_MATCH        1
#define FIRMWARE_ACTION_VERCHECK     2
#define FIRMWARE_ACTION_UPGRADE      3
#define FIRMWARE_ACTION_SUPPORT      4

#define FIRMWARE_UPGRADE_RETRY_CNT   (10)
#define FIRMWARE_NAME_LEN            (48)
#define FIRMWARE_SLOT_MAX_NUM        (16)           /* Maximum number of links supported by board cards */

/* Upgrade file headers */
#define MAX_DEV_NUM                     10           /* Maximum number of devices to which the upgrade file is applicable */
#define INSMOD_DRIVER                   1            /* insmod driver */
#define RMMOD_DRIVER                    0            /* rmmod driver */
#define MAX_HEADER_SIZE                 1000         /* Upgrade the maximum length of file header information */
#define MAX_HEADER_KV_SIZE              64           /* Upgrade the maximum length of the file header key value */

/* Upgrade file header key values */
#define FILEHEADER_DEVTYPE                "DEVTYPE"
#define FILEHEADER_SUBTYPE                "SUBTYPE"
#define FILEHEADER_TYPE                   "TYPE"
#define FILEHEADER_CHAIN                  "CHAIN"
#define FILEHEADER_CHIPNAME               "CHIPNAME"
#define FILEHEADER_VERSION                "VERSION"
#define FILEHEADER_FILETYPE               "FILETYPE"
#define FILEHEADER_CRC                    "CRC"

#define FIRMWARE_CPLD_NAME           "cpld"
#define FIRMWARE_FPGA_NAME           "fpga"

/* ioctl publi command, the same as driver */
#define FIRMWARE_COMMON_TYPE 'C'
#define FIRMWARE_GET_CHIPNAME            _IOR(FIRMWARE_COMMON_TYPE, 0, char)    /* get the chip name */
#define FIRMWARE_GET_VERSION             _IOR(FIRMWARE_COMMON_TYPE, 2, int)     /* get version */
#define FIRMWARE_SET_DEBUG_ON            _IOW(FIRMWARE_COMMON_TYPE, 3, int)     /* debug on */
#define FIRMWARE_SET_DEBUG_OFF           _IOW(FIRMWARE_COMMON_TYPE, 4, int)     /* debug off */

/* firmware cpld driver ioctl command, the same as "firmware_driver\firmware_driver\include\firmware.h" */
#define FIRMWARE_TYPE 'J'
#define FIRMWARE_PROGRAM                 _IOW(FIRMWARE_TYPE, 1, char)    /* firmware upgrade ISC */
#define FIRMWARE_READ_CHIP               _IOR(FIRMWARE_TYPE, 5, int)     /* read the contents of the chip */
#define FIRMWARE_PROGRAM_JBI             _IOW(FIRMWARE_TYPE, 6, char)    /* firmware upgrade JBI */

/* firmware cpld ispvme driver ioctl command, the same as "firmware_driver\firmware_driver_ispvme\include\firmware_ispvme.h" */
#define FIRMWARE_VME_TYPE 'V'
#define FIRMWARE_JTAG_TDI                _IOR(FIRMWARE_VME_TYPE, 0, char)
#define FIRMWARE_JTAG_TDO                _IOR(FIRMWARE_VME_TYPE, 1, char)
#define FIRMWARE_JTAG_TCK                _IOR(FIRMWARE_VME_TYPE, 2, char)
#define FIRMWARE_JTAG_TMS                _IOR(FIRMWARE_VME_TYPE, 3, char)
#define FIRMWARE_JTAG_EN                 _IOR(FIRMWARE_VME_TYPE, 4, char)
#define FIRMWARE_JTAG_INIT               _IOR(FIRMWARE_VME_TYPE, 7, char)   /* enable upgrade access */
#define FIRMWARE_JTAG_FINISH             _IOR(FIRMWARE_VME_TYPE, 8, char)   /* disable upgrade access */

/* firmware sysfs driver ioctl command, the same as "firmware_driver\firmware_driver_sysfs\include\firmware_sysfs.h" */
#define FIRMWARE_SYSFS_TYPE 'S'
#define FIRMWARE_SYSFS_INIT               _IOR(FIRMWARE_SYSFS_TYPE, 0, char)   /* enable upgrade access */
#define FIRMWARE_SYSFS_FINISH             _IOR(FIRMWARE_SYSFS_TYPE, 1, char)   /* disable upgrade access */
#define FIRMWARE_SYSFS_SPI_INFO           _IOR(FIRMWARE_SYSFS_TYPE, 2, char)   /* spi flash upgrade */
#define FIRMWARE_SYSFS_DEV_FILE_INFO      _IOR(FIRMWARE_SYSFS_TYPE, 3, char)   /* sysfs upgrade */
#define FIRMWARE_SYSFS_MTD_INFO           _IOR(FIRMWARE_SYSFS_TYPE, 4, char)   /* sysfs mtd upgrade */

/* VME file, used to distinguish the JTAG signal that needs to operate */
#define JTAG_TDO         1
#define JTAG_TCK         2
#define JTAG_TDI         3
#define JTAG_TMS         4
#define JTAG_ENABLE      5
#define JTAG_TRST        6

typedef struct name_info_s {
    int card_type[MAX_DEV_NUM];                      /* main board type */
    int sub_type[MAX_DEV_NUM];                       /* sub board type */
    int type;                                        /* device type */
    int chain;                                        /* chain num */
    char chip_name[FIRMWARE_NAME_LEN];               /* chip name */
    char version[FIRMWARE_NAME_LEN];                 /* version */
    int file_type;                                   /* file type */
    unsigned int crc32;                              /* 4 byte CRC values */
} name_info_t;

typedef struct cmd_info_s {
    uint32_t size;
    void *data;
} cmd_info_t;

enum firmware_type_s {
    FIRMWARE_UNDEF_TYPE = 0,
    FIRMWARE_CPLD,
    FIRMWARE_FPGA,
    FIRMWARE_SYSFS,
    FIRMWARE_OTHER,
};

typedef enum firmware_file_type_s {
    FIRMWARE_UNDEF_FILE_TYPE = 0,
    FIRMWARE_VME,           /* ispvme cpld, GPIO simulates JTAG */
    FIRMWARE_ISC,           /* cpld, GPIO simulates JTAG */
    FIRMWARE_JBI,
    FIRMWARE_SPI_LOGIC_DEV, /* FPGA SPI upgrde register upgrade flash */
    FIRMWARE_SYSFS_DEV,     /* write file upgrade eeprom */
    FIRMWARE_MTD,           /* upgrade mtd device */
    FIRMWARE_NONE,
} firmware_file_type_t;

typedef struct firmware_file_name_s {
    char firmware_file_name_str[MAX_HEADER_KV_SIZE];
    int firmware_file_type;
} firmware_file_name_t;

extern int header_offset;

/* CRC32 calculation */
extern unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned int len);
/* VME file upgrade */
extern int firmware_upgrade_ispvme(int file_fd, char *upgrade_file_name, name_info_t *info);
extern void writePort(unsigned char a_ucPins, unsigned char a_ucValue);
extern unsigned char readPort();
extern void sclock();
extern void ispVMStateMachine(signed char NextState);

/* spi flash upgrade */
extern int firmware_upgrade_spi_logic_dev(int fd, uint8_t *buf, uint32_t size, name_info_t *info);
/* spi flash upgrade test*/
extern int firmware_upgrade_spi_logic_dev_test(int fd, name_info_t *info);
/* spi flash data print*/
extern int firmware_upgrade_spi_logic_dev_dump(char *dev_name, uint32_t offset, uint32_t size, char *record_file);

/* sysfs upgrade */
extern int firmware_upgrade_sysfs(int fd, uint8_t *buf, uint32_t size, name_info_t *info);
/* sysfs upgrade test*/
extern int firmware_upgrade_sysfs_test(int fd, name_info_t *info);

/* isc upgrade */
extern int firmware_upgrade_jtag(int fd, uint8_t *buf, uint32_t size, name_info_t *info);
/* isc upgrade test */
extern int firmware_upgrade_jtag_test(int fd, uint8_t *buf, uint32_t size, name_info_t *info);

/* mtd upgrade */
extern int firmware_upgrade_mtd(int fd, uint8_t *buf, uint32_t size, name_info_t *info);
/* mtd upgrade test */
extern int firmware_upgrade_mtd_test(int fd, name_info_t *info);

#endif /* End of __FIRMWARE_APP_H__ */
