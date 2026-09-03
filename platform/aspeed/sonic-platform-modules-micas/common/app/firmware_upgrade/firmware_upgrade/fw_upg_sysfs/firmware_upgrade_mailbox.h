#ifndef __FIRMWARE_UPGRADE_MAILBOX__
#define __FIRMWARE_UPGRADE_MAILBOX__
#include "fw_upg_spi_logic_dev.h"

#define MB_SUCCESS         (0)                 /* Success */
#define MB_ELIB            (-1)                /* Error Library */
#define MB_ECFG            (-2)                /* Error Configuration */
#define MB_ESLOTNUM        (-3)                /* Error Slot Number */
#define MB_EFORMAT         (-4)                /* Error Format */
#define MB_EERASE          (-5)                /* Error Erase */
#define MB_EPROGRAM        (-6)                /* Error Program */
#define MB_ECMP            (-7)                /* Error Compare */
#define MB_ESIZE           (-8)                /* Error Size */
#define MB_ENAME           (-9)                /* Error Name */
#define MB_EFILEIO         (-10)               /* Error File IO */
#define MB_ECALLBACK       (-11)               /* Error Callback */
#define MB_ELOWLEVEL       (-12)               /* Error Low Level */
#define MB_EWRPROT         (-13)               /* Error Write Protection */
#define MB_EARGS           (-14)               /* Error Argument */
#define MB_ECORRUPTED_CPB  (-15)               /* Error Corrupted CPB */
#define MB_ECORRUPTED_SPT  (-16)               /* Error Corrupted SPT */

#define QSPI_CMD_MAX             (3)
#define FLASH_PAGE_SIZE          (0x400)  /* byte */
#define MAILBOX_READ_MAX         (2000000)

#define MAILBOX_ISR_RIGHT        (0x3)
#define MAILBOX_SOP_RIGHT        (0x1)

#define MAILBOX_COMMAND             (0x00)      /* base + 0 */
#define MAILBOX_EOP                 (0x04)      /* base + 1 */
#define MAILBOX_COMMAND_FIFO        (0x08)      /* base + 2 */
#define MAILBOX_RESPONSE_DATA       (0x14)      /* base + 5 */
#define MAILBOX_RESPONSE_FIFO       (0x18)      /* base + 6 */
#define MAILBOX_IER                 (0x1c)      /* base + 7 */
#define MAILBOX_ISR                 (0x20)      /* base + 8 */
#define MAILBOX_TIMER_1             (0x24)      /* base + 9 */
#define MAILBOX_TIMER_2             (0x28)      /* base + 10 */
#define MAILBOX_INTEL_LOGIC_SECTOR_SIZE    (0x10000)           /* One sector is 64K */
#define CHIP_LEVEL_RESET             (0x08)      /* Chip Level Reset Register. Each bit field is active high */
#define RESET_MAILBOX_MASK           (0x4)
#define RESET_MAILBOX_ON             (0x4)
#define RESET_MAILBOX_OFF            (0x0)

typedef enum {
    QSPI_OPEN,
    QSPI_CLOSE,
    QSPI_SET_CS,
} QSPI_OP_TYPE;

extern int firmware_upgrade_do_mailbox(int fd, uint8_t *buf, uint32_t size);
extern int firmware_upgrade_do_mailbox_test(int fd);
extern int firmware_fpga_mailbox_upgrade_test_func(firmware_spi_logic_info_t *logic_dev_info);
extern int firmware_upgrade_do_mailbox_func(firmware_spi_logic_info_t *logic_dev_info, uint8_t *buf, uint32_t size);
/* mailbox spi flash data print*/
extern int firmware_upgreade_mailbox_dump(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset, uint8_t *buf, uint32_t rd_size);

#endif