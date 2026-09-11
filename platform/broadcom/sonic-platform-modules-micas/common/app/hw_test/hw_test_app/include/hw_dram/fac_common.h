#ifndef _FAC_AC_COMMON_H_
#define _FAC_AC_COMMON_H_

#define FAC_TEST_OK                  0
#define FAC_TEST_FAIL                1
#define GRTD_LOG_DEBUG               2
#define GRTD_LOG_ERR                 1
#define GRTD_LOG_NONE                0
#define FAC_MEM_SIZE_BUF_LEN         1024               /* Save the length of the memory size */
#define FAC_MEM_TEST_SIZE            (1024*1024*32)     /* Memory test size */
#define FAC_MEM_AUTOTEST_SIZE        (1024*1024*2048ULL) /* Memory test size */
#define FAC_MEM_MAX_SIZE             (1024*1024*4096ULL) /* Memory test size */
#define FAC_MEM_TEST_PAGESIZE        8192           /* pagesize for memory tests */
#define FAC_FILENAME_LEN             128            /* Device file name length */
#define FAC_FILE_LINE_LEN            128            /* The length of each line of the file */
#define FAC_FILE_SIZE_LEN            16             /* Length that represents size in a file */
#define GRTD_SDRAM_ECC_ERR           1              /* ECC check error */
#define GRTD_SDRAM_WR_ERR            2              /* Read/write erro */
#define GRTD_SDRAM_GET_MEM_SIZE_ERR  3              /* Failed to obtain the memory capacity */
#define GRTD_SDRAM_UNKNOW_ERR        4              /* Other unknown errors */

extern int platform_fac_dbg;
/* The production test module tracks debugging information */
#define FAC_LOG_DBG(dbg, fmt, arg...)                       \
    do {                                                    \
        if (dbg <= platform_fac_dbg) {                      \
            printf("[FACTORY <%s>:<%d>] " fmt,          \
                 __FUNCTION__, __LINE__, ##arg); \
        }                                                   \
    } while (0)

#endif /* _FAC_AC_COMMON_H_ */
