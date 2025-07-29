/**
 * bininfo.c
 *
 * Copyright (C)2009. All rights reserved.
 * Description:
 *     Define the bininfo Structure
 *     Upgrade lib organization, separated from the original upgrade.c and bininfo.c.
 *     Use binary search to find bininfo, improving search efficiency.
 */
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <termios.h>
#include <ctype.h> 
#include <stdbool.h>
#include <limits.h>
#include <strings.h>
#include <mntent.h>
#include <sys/vfs.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include "bininfo.h"
#include "debug.h"

/* MTD partition information list. */
static char *device_mtd = "/proc/mtd";
static int is_debug_on;
#define MTD_INFO_BUF_LEN   128
#define SIZE_MTD_INFO   0x1000

/**
 * Byte order conversion for bininfo
 * @param dst   The target bininfo where the converted result will be saved
 * @param src   The source bininfo to be converted
 * @param type  Conversion direction - convert_tohost, convert_tonet.
 */
void bininfo_convert_byteorder(bininfo_t *dst, bininfo_t *src, convert_type type)
{
    if (dst == NULL || src == NULL) {
        dbg_print(is_debug_on, "Destination or source bininfo pointer NULL error!\n");
        return;
    }

    switch (type) {
    case convert_tohost:
        dst->magic1 = ntohl(src->magic1);
        dst->type = ntohl(src->type);
        dst->total_len = ntohl(src->total_len);
        dst->rawbin_size = ntohl(src->rawbin_size);
        dst->root_offset = ntohl(src->root_offset);
        dst->magic2 = ntohl(src->magic2);
        dst->crc = ntohl(src->crc);
        break;
    case convert_tonet:
        dst->magic1 = htonl(src->magic1);
        dst->type = htonl(src->type);
        dst->total_len = htonl(src->total_len);
        dst->rawbin_size = htonl(src->rawbin_size);
        dst->root_offset = htonl(src->root_offset);
        dst->magic2 = htonl(src->magic2);
        dst->crc = htonl(src->crc);
        break;
    default:
        /* never reach here */
        dbg_print(is_debug_on, "Specify an unknown convert type %d.\n", type);
        break;
    }
}


/**
 * Check the validity of the bininfo structure
 * @param bin   The bininfo to be checked
 * @return  0: Valid, -1: Invalid
 */
int bininfo_check_info(bininfo_t *bin)
{
    if (bin == NULL)
        return -1;

    /* Check Magic Number, Type, Size. */
    if ((bin->magic1 != BININFO_MAGIC1 || bin->magic2 != BININFO_MAGIC2)) {
        dbg_print(is_debug_on, "bininfo magic error: <magic1: 0x%08X>, <magic2: 0x%08X>\n",
            bin->magic1, bin->magic2);
        return -1;
    }
    if (bin->type != BININFO_TYPE_BOOT
        && bin->type != BININFO_TYPE_RBOOT
        && bin->type != BININFO_TYPE_MAIN) {
        dbg_print(is_debug_on, "bin type 0x%08X unknown error.\n", bin->type);
        return -1;
    }
    if (bin->rawbin_size != bin->total_len - sizeof(bininfo_t)) {
        dbg_print(is_debug_on, "bin file raw size %d beyond real size (%d - %d)\n",
            bin->rawbin_size, bin->total_len, (int)sizeof(bininfo_t));
        return -1;
    }
    return 0;
}

/**
 * Verify whether the CRC of the bin file stored in the buffer is correct
 * @param bin   The bininfo corresponding to the bin file
 * @param buf   The buffer containing the bin file content
 * @return  0: CRC correct, -1: CRC incorrect
 */
int bininfo_check_buffer_crc(bininfo_t *bin, void *buf)
{
    uint32_t crc = 0;

    if (bin == NULL || buf == NULL) {
        return -1;
    }

    /* check buffer crc. */
    crc = crc32(0, buf, bin->total_len - sizeof(uint32_t));
    /*printf("length:%d crc cal:0x%x read:0x%x \n", bin->total_len - sizeof(uint32_t), crc, bin->crc);*/
    if (crc == bin->crc) {
        return 0;
    } else {
        return -1;
    }
}

/**
 * Check the format of the bininfo
 * If the endian does not match, it will be automatically converted to the correct endian
 * @param info  The bininfo whose format is to be checked
 * @return  true: Format correct, false: Format incorrect
 *
 * @note
 * After checking, if the endian format is incorrect, it will have been automatically converted to
 * the correct endian format
 */
bool bininfo_parse(bininfo_t *info, bininfo_t *info_dsc, void *buf, int check_crc)
{
    if (info == NULL || info_dsc == NULL || buf == NULL) {
        return false;
    }

    if ((info->magic1 == BININFO_MAGIC1) || (info->magic1 == htonl(BININFO_MAGIC1))) {
        memcpy((void *)info_dsc, (void *)info, sizeof(bininfo_t));
        bininfo_convert_byteorder(info_dsc, info_dsc, convert_tonet);
    } else {
		return false;
	}
	
    /* printf("\r\nbin-info: magic=0x%x,type=0x%x, Length = %d \n",
         info_dsc->magic1, info_dsc->type, info_dsc->total_len); */

    if (bininfo_check_info(info_dsc) < 0) {
        return false;
    }

    if (check_crc && bininfo_check_buffer_crc(info_dsc, buf)) {
        return false;
    }
    
    return true;
}


/**
 * Display bininfo information
 * @param bin   The bininfo to be displayed
 */
void bininfo_show_info(bininfo_t *bin)
{
    if (bin == NULL) {
        printf("Program bininfo pointer NULL error!\n");
        return;
    }

    printf("Executable file information:\n");
    if (bin->type == BININFO_TYPE_BOOT) {
        printf("  Program type: boot\n");
    } else if (bin->type == BININFO_TYPE_RBOOT) {
        printf("  Program type: rboot\n");
    } else if (bin->type == BININFO_TYPE_MAIN) {
        printf("  Program type: main\n");
    } else {
        printf("  Program type: unknown\n");
    }

    printf("  Raw Bin Size: %d\n", bin->rawbin_size);
    printf("  Root Offset: %d\n", bin->root_offset);
    printf("  Total Length: %d\n", bin->total_len);
    printf("  Total CRC:    0x%08X\n", bin->crc);
}


/**
 * Search for the corresponding bininfo structure within the bin file content
 * @param buf   The buffer containing the bin file content
 * @param size  The size of the buffer
 * @param bin   The found bininfo will be stored here
 * @param check_crc Whether to check the CRC
 * @return  -1: Not found, other: Position of the bininfo
 */
int bininfo_lookup_buffer_info(void *buf, size_t size, bininfo_t *bin, int check_crc)
{
    int     ret = -1;
    void    *pos = buf + (size & BIN_ALIGN_MASK) - BIN_HEAD_SIZE;

    if (bin == NULL || buf == NULL) {
        return -1;
    }

    /* Lookup from buffer end. */
    while (pos > buf) {
        if (bininfo_parse((bininfo_t *)pos, bin, buf, check_crc)) {
            ret = (int)(pos - buf);
            break;
        }
        pos -= sizeof(uint32_t);
    }

    return ret;
}

/* from lib/kstrtox.c */
static const char *_parse_integer_fixup_radix(const char *s, unsigned int *base)
{
    if (*base == 0) {
        if (s[0] == '0') {
            if (tolower(s[1]) == 'x' && isxdigit(s[2]))
                *base = 16;
            else
                *base = 8;
        } else
            *base = 10;
    }
    if (*base == 16 && s[0] == '0' && tolower(s[1]) == 'x')
        s += 2;
    return s;
}

unsigned long simple_strtoul(const char *cp, char **endp,
                unsigned int base)
{
    unsigned long result = 0;
    unsigned long value;

    cp = _parse_integer_fixup_radix(cp, &base);

    while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
        ? toupper(*cp) : *cp)-'A'+10) < base) {
        result = result*base + value;
        cp++;
    }

    if (endp)
        *endp = (char *)cp;

    return result;
}

static int parse_mtd_part_info(char *buf, char *name, ulong *size)
{
    int i, j, index;
    char tmp[MTD_INFO_BUF_LEN];
    size_t len;

    /* Get the MTD name */
    j = 0;
    index = 0;
    len = strlen(buf);
    memset(tmp, 0, MTD_INFO_BUF_LEN);
    for (i = 0; i < (int)len; i++) {
        if (buf[i] == ':') {
            strcpy(name, tmp);
            index = i + 1;
            break;
        }
        tmp[j] = buf[i];
        j++;
    }

    while (buf[index] == ' ') {
        index++;
    }

    /* get the size */
    j = 0;
    memset(tmp, 0, MTD_INFO_BUF_LEN);
    for (i = index; i < (int)len; i++) {
        if (buf[i] == ' ') {
            *size = simple_strtoul(tmp, '\0', 16);
            index = i++;
            break;
        }

        tmp[j] = buf[i];
        j++;
    }

    while (buf[index] == ' ') {
        index++;
    }

    return 0;
}

static int get_mtd_info(char *buf, ulong *size, char *mtd_name)
{
    int ret;
    unsigned char *start_ptr, *end_ptr;
    int file_size;
    char mtd_buf[MTD_INFO_BUF_LEN];
    char tmp_info[MTD_INFO_BUF_LEN];
    char *mtd_part_buf;
    int fd;
    char *ptr ,*ptr_need ,*ptr_find;

    mtd_part_buf = malloc(SIZE_MTD_INFO);
    if (mtd_part_buf == NULL) {
        dbg_print(is_debug_on, "Alloc memory mtd_part_buf error!\r\n");
        return -1;
    }

    fd = open(device_mtd, O_RDONLY);
    if (fd < 0) {
        dbg_print(is_debug_on, "Failed to open %s !\r\n", device_mtd);
        free(mtd_part_buf);
        return -1;  
    }
    file_size = SIZE_MTD_INFO;
    ret = read(fd, mtd_part_buf, file_size);
    if (ret < 0) {
        dbg_print(is_debug_on, "Get file content fail!\r\n");
        perror("read");
        goto exit;
    }
    
    file_size = ret;
    start_ptr = (unsigned char *)mtd_part_buf;
    end_ptr = (unsigned char *)mtd_part_buf;
    while (file_size--) {
        if (*end_ptr == 0x0A) {  /* Find the partition */
            if (end_ptr - start_ptr <= 0 || end_ptr - start_ptr > (long int)sizeof(mtd_buf)) {
                dbg_print(is_debug_on, "size invalid.\r\n");
                goto exit;
            }
            memset(mtd_buf, 0, MTD_INFO_BUF_LEN);
            strncpy(mtd_buf, (const char *)start_ptr, end_ptr - start_ptr);
            start_ptr = end_ptr + 1;

            memset(tmp_info, 0, MTD_INFO_BUF_LEN);
            sprintf(tmp_info, "\"%s\"", mtd_name);
            ptr = strstr(mtd_buf, mtd_name);
            if (ptr != NULL) {
                ptr_need = strstr(mtd_buf, "other");
                ptr_find = strstr(mtd_name, "other");
                if ((ptr_need == NULL) && (ptr_find == NULL)) {
                    memset(tmp_info, 0, MTD_INFO_BUF_LEN);
                    parse_mtd_part_info(mtd_buf, tmp_info, size);
                    sprintf(buf, "/dev/%s", tmp_info);
                    close(fd);
                    free(mtd_part_buf);
                    return 0;
                }
                if ((ptr_need != NULL) && (ptr_find != NULL)) {
                    memset(tmp_info, 0, MTD_INFO_BUF_LEN);
                    parse_mtd_part_info(mtd_buf, tmp_info, size);
                    sprintf(buf, "/dev/%s", tmp_info);
                    close(fd);
                    free(mtd_part_buf);
                    return 0;
                }   
            }
        }
        end_ptr++;
    }

exit:
    close(fd);
    free(mtd_part_buf);
    return -1;
}

static int read_mtd(int fd, struct mtd_info_user *mtd_info, uint32_t offset, void *buf, uint32_t len)
{
    char *tmp_buf;
    uint32_t block_len = 0,tmp_len,left_len;
    int ret,is_badblock;
    int i;
    long long f_offset;
    ssize_t read_ret;
    
    /* When reading NAND partition, record the offset address in the block */
    long long start;
    
    int n,max_n;
    
    n = offset/mtd_info->erasesize;
    start = offset - n*mtd_info->erasesize;
    max_n = mtd_info->size/mtd_info->erasesize;
    
    tmp_buf = (char *)buf;
    left_len = len;
    if (mtd_info->type == MTD_NORFLASH) {
        f_offset = offset;
        block_len = left_len;
        ret = lseek(fd,f_offset,SEEK_SET);
        if (ret < 0) {
            dbg_print(is_debug_on, "lseek mtd failed.\r\n");
            return -1;
        }
        ret = read(fd, tmp_buf, block_len);
        if (ret < 0) {
            dbg_print(is_debug_on, "Failed to read\n");
            return -1;
        }
		tmp_len = (uint32_t) ret;
        left_len = left_len - tmp_len;
    }
    if (mtd_info->type == MTD_NANDFLASH) {
        for (i = n;i < max_n;i++) {
            f_offset = (long long)(i) * mtd_info->erasesize;
            ret = lseek(fd, f_offset + start, SEEK_SET);
            if (ret < 0) {
                dbg_print(is_debug_on, ": lseek mtd failed.\r\n");
                return -1;
            }
            is_badblock = ioctl(fd, MEMGETBADBLOCK, &f_offset);
            /* Bad block detected */
            if (is_badblock > 0) {
                dbg_print(is_debug_on, "\r\nBlock %d is bad, skip\n",i);
                continue;
            }
            /* 
             * After reading the first block, the read address is aligned to the erase size, 
             * with an offset of 0 
             */
            start = 0;
            if (left_len > mtd_info->erasesize) {
                /* The length of the read data is the end address minus the start address */
                block_len = (i + 1)*mtd_info->erasesize - f_offset;
            } else {
                /* Remaining bytes */
                block_len = left_len;
            }
            read_ret = read(fd, tmp_buf, block_len);
            if (read_ret < 0) {
                dbg_print(is_debug_on, "Failed to read \n");
                return -1;
            }
            tmp_len = (uint32_t)read_ret;
            left_len = left_len - tmp_len;
            tmp_buf = tmp_buf + tmp_len;
                   
            if(left_len < mtd_info->erasesize) {
                break;
            }
        }
    }
    
    return (len - left_len); 
}

int mem_cmp_00(register u_char  *src, register size_t len)
{
    register u_char *end;
    end = src + len;

    for (; src < end; src++) {
        if(*src != 0x00) {
            break;
        }
    }
    if (src < end) {
        return -1;
    } else {
        return 0;
    }
}

int mem_cmp_FF(register u_char  *src, register size_t len)
{
    register u_char *end;
    end = src + len;

    for (; src < end; src++) {
        if(*src != 0xFF) {
            break;
        }
    }
    if (src < end) {
        return -1;
    } else {
        return 0;
    }
}

/* Search for bininfo on NOR flash */
int bininfo_get_info(int fd, struct mtd_info_user *mtd_info, bininfo_t *bin)
{
    u_char *buf_target = NULL;
    int left_target,right_target,mid_target;
    bininfo_t bin_info;
    ssize_t len;
    
    int count;

    unsigned int block_len;

    block_len = BIN_HEAD_SIZE;
    /* 
     * Maximum memory requirement: one erase block plus the tail BIN_HEAD_SIZE 
     * of the previous block 
     */
    buf_target = malloc(mtd_info->erasesize + block_len);
    if (buf_target == NULL) {
        dbg_print(is_debug_on, "Alloc memory buf_target error!\r\n");
        return -1;
    }
    
    memset((void *)buf_target, 0xFF, block_len);
    /* read bininfo from first 256 byte */
    len = read_mtd(fd, mtd_info, 0, buf_target, block_len);
    if (len < 0) {
        goto read_failed;
    }

    /* 
     * Read bininfo. Since only the read-only part of the bin file is accessed, 
     * CRC check cannot be enabled 
     */

    if (bininfo_parse((bininfo_t *)buf_target, &bin_info, buf_target, 0)) {
        free(buf_target);
        memcpy(bin,&bin_info,BIN_HEAD_SIZE);
        return 0;
    }
    
    count = mtd_info->size/mtd_info->erasesize;

    left_target = 0;
    right_target = count;

    while (1) {
        mid_target = (left_target + right_target)/2;
        memset((void *)buf_target, 0xFF, block_len);
        len = read_mtd(fd, mtd_info, mid_target*mtd_info->erasesize, buf_target, block_len);
        if (len < 0) {
            goto read_failed;
        }

        if (mem_cmp_FF(buf_target,block_len) == 0) {
            right_target = mid_target;
        } else {
            left_target = mid_target;
        }
        if ((right_target - left_target) < 2) {
            break;
        }
    }

    /* Skip all-zero blocks */
    while (left_target >= 0) {
        len = read_mtd(fd, mtd_info, left_target * mtd_info->erasesize, buf_target + block_len,
                mtd_info->erasesize);
        if (len < 0) {
            goto read_failed;
        }
        if (mem_cmp_00(buf_target + block_len, mtd_info->erasesize) != 0) {
            break;
        }
        left_target--;
    }

    /* BININFO may be located between two data blocks, so read at most the length of one BININFO */
    if (left_target > 0) {
        len = read_mtd(fd, mtd_info, left_target * mtd_info->erasesize - block_len, buf_target,
                block_len);
        if (len < 0) {
            goto read_failed;
        }

    } else {
        memset((void *)buf_target, 0xFF, block_len);
    }

    /* 
     * Read bininfo. Since only the read-only part of the bin file is accessed, 
     * CRC check cannot be enabled 
     */
    if (bininfo_lookup_buffer_info(buf_target, 
          (int)mtd_info->erasesize + block_len, &bin_info, 0) < 0) {
        dbg_print(is_debug_on, "bininfo read failed.\n");
        free(buf_target);
        return -1;
    }
    free(buf_target);
    memcpy(bin, &bin_info, sizeof(bininfo_t));
    
    return 0;

read_failed:
    free(buf_target);
    dbg_print(is_debug_on, "Failed to read\n");
    return -1;
}

/* Binary search for the region where bininfo is located */
int get_version(char *mtd_name)
{
    bininfo_t bin_info;
    int fd;
    int ret;
    unsigned long size = 0;
    char path[MTD_INFO_BUF_LEN];
    struct mtd_info_user mtdinfo;

    /* Get partition information */
    ret = get_mtd_info(path, &size, mtd_name);
    if (ret != 0) {
        dbg_print(is_debug_on, "Failed to get %s part info.\n", mtd_name);
        return ret;
    }

    /* Get partition content */
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        dbg_print(is_debug_on, "Failed to open %s \n", path);
        return -1;
    }

    /* get mtd info first */
    ret = ioctl(fd, MEMGETINFO, (unsigned long) &mtdinfo);
    if (ret < 0) {
        dbg_print(is_debug_on, "%s: get mtd info failed.\r\n", mtd_name);
        close(fd);
        return -1;
    }

    /*printf("%s program information:\n", mtd_name);*/
    if (bininfo_get_info(fd, &mtdinfo, &bin_info)) {
        dbg_print(is_debug_on, "%s bininfo read failed.\n", mtd_name);
        close(fd);
        return -1;
    }

    printf("%s\n", bin_info.version);
    close(fd);

    return 0;
}

int main(int argc, char *argv[])
{
    int ret;

    is_debug_on = show_version_debug();
     
    if (argc != 3) {
        dbg_print(is_debug_on, "Use:\n");
        dbg_print(is_debug_on, " curent masterboot  : show_version curent master\n");
        dbg_print(is_debug_on, " curent slaveboot   : show_version curent slave\n");
        dbg_print(is_debug_on, " other masterboot   : show_version other master\n");
        dbg_print(is_debug_on, " other slaveboot    : show_version other slave\n");
        dbg_print(is_debug_on, " partition partition_name : show_version partition xxx\n");
        dbg_print(is_debug_on, "    The command 'cat /proc/mtd' is used to obtain the"   \
                               "    partition names\n");
        return -1;   
    }
    
    ret = 0;
    /* Current boot version information */
    if ((strcmp(argv[1], "current") == 0) && (strcmp(argv[2], "master") == 0)) { 
        ret = get_version("master_bootloader");     
    }
    if ((strcmp(argv[1], "current") == 0) && (strcmp(argv[2], "slave") == 0)) { 
        ret = get_version("slave_bootloader");      
    }

    /* Other boot version information */
    if ((strcmp(argv[1], "other") == 0) && (strcmp(argv[2], "master") == 0)){ 
        ret = get_version("master_bootloader_other");     
    }
    if ((strcmp(argv[1], "other") == 0) && (strcmp(argv[2], "slave") == 0)) { 
        ret = get_version("slave_bootloader_other");        
    }

    if ((strcmp(argv[1], "partition") == 0) && (argc == 3)) {
        ret = get_version(argv[2]);     
    } 

    if (ret != 0) {
        dbg_print(is_debug_on, "Get %s version fail!\r\n", argv[1]);
        /*printf("%s program information:\n", argv[1]);*/
        printf("N/A\n");
        return -1;
    }
    
    return 0;
}   
