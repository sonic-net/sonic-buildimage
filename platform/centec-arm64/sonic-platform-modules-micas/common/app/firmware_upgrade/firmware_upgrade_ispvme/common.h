#ifndef __COMMON_H__
#define __COMMON_H__

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define FIRMWARE_TYPE 'F'
#define FIRMWARE_JTAG_TDI                _IOR(FIRMWARE_TYPE, 0, char)
#define FIRMWARE_JTAG_TDO                _IOR(FIRMWARE_TYPE, 1, char)
#define FIRMWARE_JTAG_TCK                _IOR(FIRMWARE_TYPE, 2, char)
#define FIRMWARE_JTAG_TMS                _IOR(FIRMWARE_TYPE, 3, char)
#define FIRMWARE_JTAG_EN                 _IOR(FIRMWARE_TYPE, 4, char)
#define FIRMWARE_SET_DEBUG_ON            _IOW(FIRMWARE_TYPE, 5, int)     /* debug on */
#define FIRMWARE_SET_DEBUG_OFF           _IOW(FIRMWARE_TYPE, 6, int)     /* debug off */
#define FIRMWARE_SET_GPIO_INFO           _IOR(FIRMWARE_TYPE, 7, int)     /* set GPIO pin configuration */

#define    JTAG_TDI            (1)
#define    JTAG_TDO            (2)
#define    JTAG_TCK            (3)
#define    JTAG_TMS            (4)
#define    JTAG_ENABLE         (5)
#define    JTAG_TRST           (6)

#endif