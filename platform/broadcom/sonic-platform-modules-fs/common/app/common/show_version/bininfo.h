/*
 * Copyright (C) 2006.
 * All rights reserved.
 *
 * /include/lib/bininfo.h
 *
 * Description:
 * 
 * Utilities for NGSA Binaries file, included 'BOOT', 'CTRL', 'MAIN'.
 * 
 * Here is the layout for NGSA binary file format:
 * 
 *  32 Bytes aligned -->|      32 Bytes aligned -->|<-- 256 Bytes -->|
 * +--------------------+--------------------------+-----------------+
 * |        RawBin      |      Other Data          | bininfo_t       |
 * +--------------------+--------------------------+-----------------+
 *
 * 'RawBin' is the original excutable binary file.
 * 'Other Data' is appended data such as 'ConfData' & 'InstallPackge'.
 * 'bininfo_t' is the description of the binary file format, see below.
 * 
 */
#ifndef _BIN_HEAD_H_
#define _BIN_HEAD_H_


/* The addresses of the bin file segment and bininfo are both 32-byte aligned */
#define BIN_ALIGN               32
#define BIN_ALIGN_MASK          (~0x1f)
#define BIN_HEAD_SIZE           sizeof(bininfo_t)

#define BININFO_MAGIC1          0x90EAA98F
#define BININFO_MAGIC2          0x8CEAB59D

/* 
 *  #define BININFO_TYPE_BIN        0x4D41494E
 *  #define BININFO_TYPE_ROM        0x424F4F54
 */
#define BININFO_TYPE_BOOT        0x424F4F54
#define BININFO_TYPE_RBOOT      0x4354524C
#define BININFO_TYPE_MAIN       0x4D41494E

/* XXX, define some error numbers */
#define CRC_CHECK_ERROR         0x1    /* Main program is corrupted */
#define MAIN_NOT_EXIST          0x2    /* No main program on the line card */
#define BIN_NOT_FIT             0x3    /* Not suitable for this product */
#define BIN_TYPE_UNKNOWN        0x4    /* Unknown bin type */

#define CONFIG_SYS_BOOT_SIZE       (0x100000)
#define CONFIG_SYS_RBOOT_SIZE      (0xdb0000)

#define VERSION_LEN_BIN             0x40   /* Length of version */

typedef enum {
    convert_tohost,     /* Convert network byte order to host byte order */
    convert_tonet       /* Convert host byte order to network byte order */
} convert_type;

#define BINECC_HEAD_MAGIC1  0x15b9ac0d
#define BINECC_HEAD_MAGIC2  0xc4e718ba

typedef struct bininfo_s {
    uint32_t     magic1;             /* Magic1 Number: 0x90EAA98F (NGSA^0xDEADFACE) */
    uint32_t     type;               /* Identifies the image file type:
                                       BININFO_TYPE_BOOT BININFO_TYPE_RBOOT  BININFO_TYPE_MAIN 
                                    */
    uint32_t     total_len;          /* Total length of the image file (including the info itself). */
    uint32_t     rawbin_size;        /* Length of the raw program file data, excluding bininfo_t */
    uint32_t     root_offset;        /* Main root offset address */
    unsigned char version[VERSION_LEN_BIN];         /* Version number */
    uint32_t     reserve[41];                    /* Unused, all zeroed */
    uint32_t     magic2;             /* Magic2 Number: 0x8CEAB59D (RGOS^0xDEADFACE). */
    uint32_t     crc;                /* Calculation range: all image file data except for the CRC32 field itself. */
} bininfo_t;

int bininfo_lookup_buffer_info(void *buf, size_t size, bininfo_t *bin, int check_crc);
void bininfo_show_info(bininfo_t *bin);
unsigned long crc32 (unsigned long, const unsigned char *, unsigned int);
unsigned long simple_strtoul(const char *cp, char **endp,
				unsigned int base);
#endif  /* _BIN_HEAD_H_ */

