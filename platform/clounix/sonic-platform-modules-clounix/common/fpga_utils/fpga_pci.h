#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <getopt.h>
#ifndef _FPGA_H //{ _FPGA_H
#define _FPGA_H

#ifdef __cplusplus
extern "C" {
#endif

#define PRINT(rc)   do {                                                    \
		fprintf(stderr, "Error at line %d, file %s (%d) [%s], rc=%d\n",     \
		__LINE__, __FILE__, errno, strerror(errno), rc);                    \
	} while(0)

#define FATAL  do {                                                         \
		PRINT(-1);                                                          \
        exit(1);                                                            \
	} while(0)

#define CHECK_RC(rc) do {                       \
    int __ret = rc;                             \
    if (__ret) {                                \
        PRINT(__ret);                           \
        return(__ret);                          \
    }                                           \
} while(0)

#define DBG(fmt, ...) do {	                                                   \
    if (verbose) {                                                             \
        fprintf(stderr, "%s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    }                                                                          \
} while(0)
#define FALSE                         (0)
#define TRUE                          (1)
#define CLX_SUCCESS                         (0)
#define CLX_FAILURE                         (-1)
#define CLX_DEVICE_NOT_FOUND                (-2)
#define FPGA_REG_WIDTH (4) 

enum bus_master{
    DEV_PCI_MEM = 0,
    //DEV_I2C = 1,
    //DEV_SPI = 2,
    DEV_FPGA = 3,
    //DEV_SMBUS = 4,
    DEV_CPLD = 5,
    DEV_LAST = 6
};

enum _access_mode{
    ONE_BYTE   = 1,
    TWO_BYTE   = 2,
    FOUR_BYTE  = 4,
    EIGHT_BYTE = 8,
};
struct func_map {
	const char *name;
    int (*init)(uint32_t region,void **driver);
    void (*exit)(uint32_t region);
};
struct cpld_fn_if {
    int page_size;
    int (*program_image)(uint8_t *image, size_t size, uint8_t update_cpld_golden_flag);
    int (*verify_image)(uint8_t *image, size_t size,uint32_t loops);
    int (*cmdxfer)(uint8_t *SendBufPtr,uint32_t wcnt,uint8_t *RecvBufPtr,uint32_t rcnt);
    
};
struct fpga_fn_if {
    int page_size;
    int (*program_image)(uint8_t *image, size_t size,uint32_t region,uint8_t erase_flag);
    int (*verify_image)(uint8_t *image, size_t size,uint32_t region,uint32_t loops);
};
#define MAX_PLATFORM_NAME_LEN 60 
#define CLX8000_PLATFORM_STRING "x86_64-clounix_clx8000_48c8d-r0"
#define CLX12800_PLATFORM_STRING "clounix_clx12800_32d-r0"
#define CLX25600_H_PLATFORM_STRING "x86_64-clounix_clx25600_48d8e-r0"
#define CLX25600_BRB_PLATFORM_STRING "x86_64-clounix_clx25600_brb-r0"
#define CLX25600_SLT_PLATFORM_STRING "x86_64-clounix_clx25600_slt-r0"
#define CLX25600_L_PLATFORM_STRING "x86_64-clounix_clx12800_48d8e-r0"
typedef uint32_t UINTPTR;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

extern uint32_t Spi_In32(uint32_t reg);
extern void Spi_Out32(uint32_t reg, uint32_t value);

extern int pci_reg_read(uint32_t reg,void *data);
extern int pci_reg_write(uint32_t reg, void *value);

#define Clx_AssertNonvoid(rc) do {                 \
    int __ret = rc;                                \
    if (!__ret) {                                  \
        FATAL;                                     \
    }                                              \
} while(0)


#define Clx_AssertVoid(rc) do {                    \
    int __ret = rc;                                \
    if (!__ret) {                                  \
        FATAL;                                     \
    }                                              \
} while(0)

#define Clx_AssertVoidAlways() do {                \
        FATAL;                                     \
} while(0)


#ifdef __cplusplus
}
#endif

#endif //_FPGA_H }
