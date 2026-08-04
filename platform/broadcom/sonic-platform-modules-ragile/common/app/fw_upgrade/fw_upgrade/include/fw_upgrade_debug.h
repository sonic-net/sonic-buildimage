#ifndef __FW_UPGRADE_DEBUG_H__
#define __FW_UPGRADE_DEBUG_H__

#include <string.h>

#define DEBUG_INFO_LEN  20
#define DEBUG_FILE      "/tmp/.fw_upgrade_debug"
#define DEBUG_ON_ALL    "3"
#define DEBUG_ON_KERN   "2"
#define DEBUG_ON_INFO   "1"
#define DEBUG_OFF_INFO  "0"

#define mem_clear(data, size) memset((data), 0, (size))

enum debug_s {
    DEBUG_OFF = 0,
    DEBUG_APP_ON,
    DEBUG_KERN_ON,
    DEBUG_ALL_ON,
    DEBUG_IGNORE,
};

extern int fw_upgrade_debug(void);

#endif /* End of __FW_UPGRADE_DEBUG_H__ */
