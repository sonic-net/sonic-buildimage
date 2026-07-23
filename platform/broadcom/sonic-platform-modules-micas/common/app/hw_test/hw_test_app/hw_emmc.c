#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <hw_emmc/hw_emmc.h>

#define EMMC_SYS_BLOCK_PATH     "/sys/block/emmcblk0/size"
#define EMMC_DEV_FACTEST_PATH   "/dev/emmcblk0p7"
#define EMMC_BLK_SIZE           512
#if 0
/* The test values are the same as nandflash, and the emmc factory_test partition is nearly 16MB */
static char emmc_test_ch[] = {
    0x00, 0xFF, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 
    0x40, 0x80, 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 
    0xBF, 0x7F
};
#endif
int emmc_test_main(int argc, char **argv)
{
#if 1
    argc = argc;
    argv = argv;
    return -1;
#else
    char *buf;
    int fd, i, j, ret;

    if (argc != 1) {
        fprintf(stderr, "Error: Wrong input!\n");
        return -1;
    }
    argv = argv;

    ret = posix_memalign((void **) &buf, EMMC_BLK_SIZE, EMMC_BLK_SIZE);
    if (ret != 0) {
        fprintf(stderr, "Error: Alloc memory failed(%d,%s)!\n", 
            ret, strerror(ret));
        return -1;
    }

    fd = open(EMMC_DEV_FACTEST_PATH, O_RDWR | O_DIRECT, S_IRWXU);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Open file[rw,%s] failed(%s)!\n", 
            EMMC_DEV_FACTEST_PATH, strerror(errno));
        goto out_free;
    }

    for (i = 0; i < (int) sizeof(emmc_test_ch); i++) {
        memset(buf, emmc_test_ch[i], EMMC_BLK_SIZE);
        ret = write(fd, buf, EMMC_BLK_SIZE);
        if (ret != EMMC_BLK_SIZE) {
            fprintf(stderr, "ERROR: Write file error(%d,%d,%s)!\n", 
                ret, i, strerror(errno));
            goto out_close;
        }
    }

    ret = fsync(fd);
    if (ret != 0) {
        fprintf(stderr, "ERROR: Fsync file error(%d,%s)!\n", 
            ret, strerror(errno));
        goto out_close;
    }
    
    close(fd);
   
    fd = open(EMMC_DEV_FACTEST_PATH, O_RDONLY | O_DIRECT, S_IRWXU);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Open file[rd,%s] failed(%s)!\n", 
            EMMC_DEV_FACTEST_PATH, strerror(errno));
        goto out_free;
    }

    for (i = 0; i < (int) sizeof(emmc_test_ch); i++) {
        memset(buf, 0x11, EMMC_BLK_SIZE); /* 0x11 does not belong to any test value */
        ret = read(fd, buf, EMMC_BLK_SIZE);
        if (ret != EMMC_BLK_SIZE) {
            fprintf(stderr, "ERROR: Read file error(%d,%d,%s)!\n", 
                ret, i, strerror(errno));
            goto out_close;
        }

        for (j = 0; j < EMMC_BLK_SIZE; j++) {
            if (buf[j] != emmc_test_ch[i]) {
                fprintf(stderr, "ERROR: Check error(%d,%d,%d,%d)!\n", 
                    i, j, buf[j], emmc_test_ch[i]);
                goto out_close;
            }
        }
    }

    close(fd);

    free(buf);

    printf("SUCCESS: eMMC test pass!\n");

    return 0;

out_close:
    close(fd);

out_free:
    free(buf);
    
    return -1;
#endif
}
