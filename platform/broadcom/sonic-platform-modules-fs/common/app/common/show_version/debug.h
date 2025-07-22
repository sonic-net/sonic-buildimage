/*
 * Copyright(C) 2018 Ruijie Network. All rights reserved.
 */
/*
 * debug.h
 * Original Author : zhangdandan(zhangdandan@ruijie.com.cn) 2018-09-11 
 *
 * show versionµÄdebug¿ØÖÆ
 *
 * History
 *
 *    v1.0    zhangdandan(zhangdandan@ruijie.com.cn)  2018-09-11  Initial version.
 *
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG_INFO_LEN  20
#define DEBUG_FILE      "/var/tmp/.show_version_debug"
#define DEBUG_ON_INFO   "1"
#define DEBUG_OFF_INFO  "0"

#define dbg_print(debug, fmt, arg...)  \
    if (debug == DEBUG_ON) \
        { do{printf(fmt,##arg);} while(0); }
		
enum debug_s {
    DEBUG_OFF = 0,                  /* Turn off debug */
    DEBUG_ON,                       /* Turn on debug */
    DEBUG_IGNORE,                   /* Ignore debug */
};

/* Handle the debug switch */
extern int show_version_debug(void);

#endif /* End of __DEBUG_H__ */
