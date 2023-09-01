#ifndef __FIRMWARE_UPGRADE_DEBUG_H__
#define __FIRMWARE_UPGRADE_DEBUG_H__

#define DEBUG_INFO_LEN  20
#define DEBUG_FILE      "/.firmware_upgrade_debug"
#define DEBUG_ON_ALL    "3"
#define DEBUG_ON_KERN   "2"
#define DEBUG_ON_INFO   "1"
#define DEBUG_OFF_INFO  "0"

enum debug_s {
    DEBUG_OFF = 0,                  /* debug off */
    DEBUG_APP_ON,                   /* debug app on */
    DEBUG_KERN_ON,                  /* kernel debug on */
    DEBUG_ALL_ON,                   /* debug app and kernel debug on */
    DEBUG_IGNORE,                   /* ignore debug */
};

extern int firmware_upgrade_debug(void);

#endif /* End of __FIRMWARE_UPGRADE_DEBUG_H__ */