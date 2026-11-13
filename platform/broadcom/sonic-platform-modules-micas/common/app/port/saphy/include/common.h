/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * Common log, config parsing, and utility function header.
 ********************************************************************************/
#ifndef _PORT_COMMON_H_
#define _PORT_COMMON_H_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>

/* ==========================================================================
 * Status codes
 * ========================================================================== */
typedef enum {
    STATUS_SUCCESS                =  0,
    STATUS_FAILURE                = -1,
    STATUS_NOT_SUPPORTED          = -2,
    STATUS_NO_MEMORY              = -3,
    STATUS_INSUFFICIENT_RESOURCES = -4,
    STATUS_INVALID_PARAMETER      = -5,
    STATUS_ITEM_ALREADY_EXISTS    = -6,
    STATUS_ITEM_NOT_FOUND         = -7,
    STATUS_NOT_IMPLEMENTED        = -8,
} status_type_e;


#ifndef uint64_t
#define uint64_t unsigned long long
#endif

#ifndef uint32_t
#define uint32_t unsigned int
#endif

#ifndef uint16_t
#define uint16_t unsigned short
#endif

#ifndef uint8_t
#define uint8_t unsigned char
#endif
extern int hwsku_mode;
/* Log output functions */
void log_info(const char *format, ...);

#define LOG_INFO(fmt, args...)    log_info("%s INFO Func=[%s], Line=[%d] " fmt"\n", \
                                    get_current_time(), __FUNCTION__, __LINE__, ##args)

/* ==========================================================================
 * Init
 * ========================================================================== */
int  common_init(void);
char* get_current_time(void);

/* ==========================================================================
 * Config file parsing
 * ========================================================================== */
#define CFG_FILE_LINE_MAX   (2000)
#define CFG_FILE_STR_MAX    (128)
#define CFG_FILE_ARR_MAX    (512)

int  get_cfg_info(const char *file_path, char *config_buf, int *val, bool is_arr);
void parse_int_list(const char *input, int *val, int val_size, int *list_len);

int  file_exists(const char *filePath);

typedef struct {
    char type[32];
    int  (*phy_platform_init)(int unit);
} phy_apis_t;

#endif /* _PORT_COMMON_H_ */
