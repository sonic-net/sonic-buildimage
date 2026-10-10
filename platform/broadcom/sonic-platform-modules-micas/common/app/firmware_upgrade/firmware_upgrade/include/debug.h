/*
 *
 * debug.h
 * firmware upgrade debug switch control
 */

#ifndef __FIRMWARE_UPGRADE_DEBUG_H__
#define __FIRMWARE_UPGRADE_DEBUG_H__
#include <string.h>

#define mem_clear(data, size) memset((data), 0, (size))

#define DEBUG_INFO_LEN  20
#define DEBUG_FILE      "/tmp/.firmware_upgrade_debug"
#define DEBUG_ON_ALL    "3"
#define DEBUG_ON_INFO   "1"
#define DEBUG_OFF_INFO  "0"

enum debug_s {
    DEBUG_OFF = 0,                  /* off debug  */
    DEBUG_APP_ON,                   /* open app debug */
    DEBUG_ALL_ON,                   /* open all debug */
    DEBUG_IGNORE,                   /* ignore debug */
};

#define dbg_print(debug, fmt, arg...)  \
    if (debug == DEBUG_APP_ON || debug == DEBUG_ALL_ON) \
        { do{printf(fmt,##arg);} while(0); }

/* firmware upgrade debug switch */
extern int firmware_upgrade_debug(void);
extern int is_debug_on;

#endif /* End of __FIRMWARE_UPGRADE_DEBUG_H__ */
