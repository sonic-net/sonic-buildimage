#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <hw_mtd/hw_mtdflash.h>
#include <hw_mtd/mtd-abi.h>

static void mtdflash_help(char *name)
{
    fprintf(stderr,
            "Usage: %s  start_addr end_addr data1 data2 [times]                     \r\n"
            "  start_addr   Flash start address                                     \r\n"
            "  end_addr     Flash end address                                       \r\n"
            "  data1        Test data1(8bit)                                        \r\n"
            "  data2        Test data2(8bit)                                        \r\n"
            "  [times}                                                              \r\n",
            name);
    exit(1);
}

static int mtd_arg_get(int argc, char **argv, struct mtd_dev_priv *mtd_priv, int min_arg)
{
    unsigned int start_addr, end_addr, data1 = 0, data2 = 0, times = 1;
    char *end;
    int flags = 0;

    if (argc < min_arg) {
        return -EINVAL;
    }

    start_addr = strtol(argv[flags + 1], &end, 0);
    if (*end) {
        fprintf(stderr, "Error: start addr invalid!\n");
        return -EINVAL;
    }

    end_addr = strtol(argv[flags + 2], &end, 0);
    if (*end) {
        fprintf(stderr, "Error: end addr invalid!\n");
        return -EINVAL;
    }

    if (argc > 3) {
        data1 = strtol(argv[flags + 3], &end, 0);
        if (*end) {
            fprintf(stderr, "Error: data1 invalid!\n");
            return -EINVAL;
        }
    }

    if (argc > 4) {
        data2 = strtol(argv[flags + 4], &end, 0);
        if (*end) {
            fprintf(stderr, "Error: data2 invalid!\n");
            return -EINVAL;
        }
    }

    if (argc > 5) {
        times = strtol(argv[flags + 5], &end, 0);
        if (*end) {
            fprintf(stderr, "Error: times invalid!\n");
            return -EINVAL;
        }
    }

    mtd_priv->start_addr        = start_addr;
    mtd_priv->end_addr          = end_addr;
    mtd_priv->data1             = data1;
    mtd_priv->data2             = data2;
    mtd_priv->times             = times;
    return 0;
}

static void print_arg(struct mtd_dev_priv *mtd_priv)
{
    printf("Start: 0x%08X, End: 0x%08X\n",
           mtd_priv->start_addr, mtd_priv->end_addr);
    printf("Data1 = 0x%02X, Data2 = 0x%02X",
           mtd_priv->data1, mtd_priv->data2);

    printf(", Times = %u", mtd_priv->times);

    printf("\r\n");
}

static int open_mtd_dev(char *test_name)
{
    FILE *fd;
    int file, ret;
    char buffer[MAX_BUFFER_SIZE], filename[MAX_BUFFER_SIZE], *ptr;

    if ((fd = fopen("/proc/mtd", "r")) == NULL) {
        fprintf(stderr, "Error: Could not open file "
                "/proc/mtd: %s\n", strerror(errno));
        return -1;
    }

    while (fgets(buffer, MAX_BUFFER_SIZE, fd)) {
        if ((ptr = strstr(buffer, test_name)) != NULL) {
            break;
        }
    }
    fclose(fd);

    if ((ptr = strstr(buffer, ":")) == NULL) {
        return -1;
    }
    *ptr = '\0';

    ret = snprintf(filename, MAX_BUFFER_SIZE, "/dev/%s", buffer);
    filename[ret] = '\0';

    printf("....................%s\r\n", filename);

    if ((file = open(filename, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "`%s': %s\n", filename, strerror(errno));
    }

    return file;
}

static int mtd_arg_check(int fd,
                          struct mtd_dev_priv *mtd_priv,
                          unsigned int *nand_erasesize)
{
    int ret;
    struct mtd_info_user mtd;

    if ((ret = ioctl(fd, MEMGETINFO, &mtd)) < 0) {
        perror("MEMGETINFO");
        return -1;
    }

    if ((mtd.type != MTD_NANDFLASH && mtd.type != MTD_NORFLASH) ||
        (mtd.flags != MTD_CAP_NANDFLASH && mtd.flags != MTD_CAP_NORFLASH)) {
        printf("Error, Not NandFlash or NorFlash!\r\n");
        return -1;
    }

    if ((mtd_priv->start_addr & (mtd.writesize - 1)) ||
        (mtd_priv->end_addr & (mtd.writesize -1))) {
        printf("Error, Address must be %d bytes aligned!\r\n", mtd.writesize);
        return -1;
    }

    if (mtd_priv->end_addr > mtd.size) {
        printf("Error, Address out of test range!\r\n");
        return -1;
    }

    if (mtd_priv->start_addr >= mtd_priv->end_addr) {
        printf("Error, start address is higher than end address!\r\n");
        return -1;
    }

    *nand_erasesize = mtd.erasesize;

    return 0;
}

static int mtd_check_data(unsigned char *buffer,
                          struct mtd_dev_priv *mtd_priv,
                          unsigned long len)
{
    unsigned int i;
    len = mtd_priv->end_addr - mtd_priv->start_addr;
    for (i = 0; i < len; i++) {
        if (buffer[i] != mtd_priv->buffer[i]) {
            return (-1);
        }
    }

    return (0);
}

int nand_wr_main(int argc, char **argv)
{
    int ret, fd;
    unsigned long i, len;
    unsigned int nand_erasesize;
    struct mtd_dev_priv  nand_priv;
    struct erase_info_user erase;

    memset(&nand_priv, 0, sizeof(struct mtd_dev_priv));

    ret = mtd_arg_get(argc, argv, &nand_priv, 5);
    if (ret < 0) {
        mtdflash_help("nand_wr");
        return -1;
    }

    if ((fd = open_mtd_dev(NAND_TEST_PARTITION)) < 0) {
        return -1;
    }

    if ((ret = mtd_arg_check(fd, &nand_priv, &nand_erasesize)) < 0) {
        close(fd);
        return -1;
    }

    len = nand_priv.end_addr - nand_priv.start_addr;
    if (nand_priv.buffer == NULL) {
        nand_priv.buffer = malloc(len);
        if (nand_priv.buffer == NULL) {
            fprintf(stderr,
                "Error: Could not malloc memory %s\n",
                strerror(errno));
            close(fd);
            return -ENOMEM;
        }
    }

    printf("\r\n********************** NANDFLASH write test **********************\r\n");
    print_arg(&nand_priv);

    while (nand_priv.times--) {
        erase.start  = (nand_priv.start_addr / nand_erasesize) * nand_erasesize;
        erase.length = ((len + nand_erasesize) / nand_erasesize) * nand_erasesize;
        if ((ret = ioctl(fd, MEMERASE, &erase)) < 0) {
            perror("MEMERASE");
            goto error_out;
        }
        if (nand_priv.times % 2) {
            for (i = 0; i < len;) {
                nand_priv.buffer[i++] = nand_priv.data1;
                nand_priv.buffer[i++] = nand_priv.data2;
            }
        } else {
            for (i = 0; i < len;) {
                nand_priv.buffer[i++] = nand_priv.data2;
                nand_priv.buffer[i++] = nand_priv.data1;
            }
        }

        if ((ret = lseek(fd, nand_priv.start_addr, SEEK_SET)) < 0) {
            perror("lseek");
            goto error_out;
        }

        if ((ret = write(fd, nand_priv.buffer, len)) < 0) {
            fprintf(stderr, "%s: write, size %ld, %d\n", __func__, len, ret);
            perror("write");
            goto  error_out;
        }
    }

error_out:
    printf("\r\n******************** NANDFLASH write test End ********************\r\n\r\n");
    close(fd);
    if (nand_priv.buffer) {
        free(nand_priv.buffer);
    }
    return ret;
}

int nand_rd_main(int argc, char **argv)
{
    int ret, fd;
    unsigned long len;
    unsigned int nand_erasesize;
    struct mtd_dev_priv  nand_priv;

    memset(&nand_priv, 0, sizeof(struct mtd_dev_priv));

    ret = mtd_arg_get(argc, argv, &nand_priv, 3);
    if (ret < 0) {
        mtdflash_help("nand_rd");
        return -1;
    }

    if ((fd = open_mtd_dev(NAND_TEST_PARTITION)) < 0) {
        return -1;
    }

    if ((ret = mtd_arg_check(fd, &nand_priv, &nand_erasesize)) < 0) {
        close(fd);
        return -1;
    }

    len = nand_priv.end_addr - nand_priv.start_addr;
    if (nand_priv.buffer == NULL) {
        nand_priv.buffer = malloc(len);
        if (nand_priv.buffer == NULL) {
            fprintf(stderr,
                "Error: Could not malloc memory %s\n",
                strerror(errno));
            close(fd);
            return -ENOMEM;
        }
    }

    printf("\r\n********************** NANDFLASH read test **********************\r\n");
    print_arg(&nand_priv);

    while (nand_priv.times--) {
        if ((ret = lseek(fd, nand_priv.start_addr, SEEK_SET)) < 0) {
            perror("lseek");
            goto error_out;
        }
        if ((ret = read(fd, nand_priv.buffer, len)) < 0) {
            fprintf(stderr, "%s: read, size %ld, %d\n", __func__, len, ret);
            perror("read");
            goto  error_out;
        }
    }
#if 0
    printf("Last Read: \r\n");
    /* dump data */
    for (i = 0; i < len; i++) {
        printf("%02X ", nand_priv.buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\r\n");
        }
    }
#endif

error_out:
    printf("\r\n******************** NANDFLASH read test End ********************\r\n\r\n");
    close(fd);
    if (nand_priv.buffer) {
        free(nand_priv.buffer);
    }
    return ret;
}

int nand_chk_main(int argc, char **argv)
{
    int ret, fd;
    unsigned long i, len;
    unsigned int nand_erasesize;
    struct mtd_dev_priv  nand_priv;
    struct erase_info_user erase;
    unsigned char *buffer = NULL;

    memset(&nand_priv, 0, sizeof(struct mtd_dev_priv));

    ret = mtd_arg_get(argc, argv, &nand_priv, 5);
    if (ret < 0) {
        mtdflash_help("nand_chk");
        return -1;
    }

    if ((fd = open_mtd_dev(NAND_TEST_PARTITION)) < 0) {
        return -1;
    }

    if ((ret = mtd_arg_check(fd, &nand_priv, &nand_erasesize)) < 0) {
        close(fd);
        return -1;
    }

    len = nand_priv.end_addr - nand_priv.start_addr;
    if (nand_priv.buffer == NULL) {
        nand_priv.buffer = malloc(len);
        if (nand_priv.buffer == NULL) {
            fprintf(stderr,
                "Error: Could not malloc memory %s\n",
                strerror(errno));
            ret = -ENOMEM;
            goto error_out;
        }
    }
    buffer = malloc(len);
    if (buffer == NULL) {
        fprintf(stderr,
                "Error: Could not malloc memory %s\n",
                strerror(errno));
        ret = -ENOMEM;
        goto error_out;
    }
    memset(buffer, 0, len);

    printf("\r\n********************** NANDFLASH write test **********************\r\n");
    print_arg(&nand_priv);

    while (nand_priv.times--) {
        erase.start  = (nand_priv.start_addr / nand_erasesize) * nand_erasesize;
        erase.length = ((len + nand_erasesize) / nand_erasesize) * nand_erasesize;
        if ((ret = ioctl(fd, MEMERASE, &erase)) < 0) {
            perror("MEMERASE");
            goto error_out;
        }
        if (nand_priv.times % 2) {
            for (i = 0; i < len;) {
                nand_priv.buffer[i++] = nand_priv.data1;
                nand_priv.buffer[i++] = nand_priv.data2;
            }
        } else {
            for (i = 0; i < len;) {
                nand_priv.buffer[i++] = nand_priv.data2;
                nand_priv.buffer[i++] = nand_priv.data1;
            }
        }

        if ((ret = lseek(fd, nand_priv.start_addr, SEEK_SET)) < 0) {
            perror("lseek");
            goto error_out;
        }

        if ((ret = write(fd, nand_priv.buffer, len)) < 0) {
            fprintf(stderr, "%s: write, size %ld, %d\n", __func__, len, ret);
            perror("write");
            goto  error_out;
        }

        if ((ret = lseek(fd, nand_priv.start_addr, SEEK_SET)) < 0) {
            perror("lseek");
            goto error_out;
        }

        if ((ret = read(fd, buffer, len)) < 0) {
            fprintf(stderr, "%s: write, size %ld, %d\n", __func__, len, ret);
            perror("write");
            goto  error_out;
        }
        if ((ret = mtd_check_data(buffer, &nand_priv, len)) < 0) {
            nand_priv.check_test_errors++;
        }
    }

error_out:
    printf("Check test errors = %u\r\n", nand_priv.check_test_errors);
    printf("\r\n******************** NANDFLASH write test End ********************\r\n\r\n");
    close(fd);
    if (nand_priv.buffer) {
        free(nand_priv.buffer);
    }
    if (buffer) {
        free(buffer);
    }
    return ret;
}

int nor_wr_main(int argc, char **argv)
{
    int ret, fd;
    unsigned long i, len;
    unsigned int nor_erasesize;
    struct mtd_dev_priv  nor_priv;
    struct erase_info_user erase;

    memset(&nor_priv, 0, sizeof(struct mtd_dev_priv));

    ret = mtd_arg_get(argc, argv, &nor_priv, 5);
    if (ret < 0) {
        mtdflash_help("nor_wr");
        return -1;
    }

    if ((fd = open_mtd_dev(NOR_TEST_PARTITION)) < 0) {
        return -1;
    }

    if ((ret = mtd_arg_check(fd, &nor_priv, &nor_erasesize)) < 0) {
        close(fd);
        return -1;
    }

    len = nor_priv.end_addr - nor_priv.start_addr;
    if (nor_priv.buffer == NULL) {
        nor_priv.buffer = malloc(len);
        if (nor_priv.buffer == NULL) {
            fprintf(stderr,
                "Error: Could not malloc memory %s\n",
                strerror(errno));
            close(fd);
            return -ENOMEM;
        }
    }

    printf("\r\n********************** NORFLASH write test **********************\r\n");
    print_arg(&nor_priv);

    while (nor_priv.times--) {
        erase.start  = (nor_priv.start_addr / nor_erasesize) * nor_erasesize;
        erase.length = ((len + nor_erasesize) / nor_erasesize) * nor_erasesize;
        if ((ret = ioctl(fd, MEMERASE, &erase)) < 0) {
            perror("MEMERASE");
            goto error_out;
        }
        if (nor_priv.times % 2) {
            for (i = 0; i < len;) {
                nor_priv.buffer[i++] = nor_priv.data1;
                nor_priv.buffer[i++] = nor_priv.data2;
            }
        } else {
            for (i = 0; i < len;) {
                nor_priv.buffer[i++] = nor_priv.data2;
                nor_priv.buffer[i++] = nor_priv.data1;
            }
        }

        if ((ret = lseek(fd, nor_priv.start_addr, SEEK_SET)) < 0) {
            perror("lseek");
            goto error_out;
        }

        if ((ret = write(fd, nor_priv.buffer, len)) < 0) {
            fprintf(stderr, "%s: write, size %ld, %d\n", __func__, len, ret);
            perror("write");
            goto  error_out;
        }
    }

error_out:
    printf("\r\n******************** NORFLASH write test End ********************\r\n\r\n");
    close(fd);
    if (nor_priv.buffer) {
        free(nor_priv.buffer);
    }
    return ret;
}

int nor_rd_main(int argc, char **argv)
{
    int ret, fd;
    unsigned long i, len;
    unsigned int nor_erasesize;
    struct mtd_dev_priv  nor_priv;

    memset(&nor_priv, 0, sizeof(struct mtd_dev_priv));

    ret = mtd_arg_get(argc, argv, &nor_priv, 3);
    if (ret < 0) {
        mtdflash_help("nor_rd");
        return -1;
    }

    if ((fd = open_mtd_dev(NOR_TEST_PARTITION)) < 0) {
        return -1;
    }

    if ((ret = mtd_arg_check(fd, &nor_priv, &nor_erasesize)) < 0) {
        close(fd);
        return -1;
    }

    len = nor_priv.end_addr - nor_priv.start_addr;
    if (nor_priv.buffer == NULL) {
        nor_priv.buffer = malloc(len);
        if (nor_priv.buffer == NULL) {
            fprintf(stderr,
                "Error: Could not malloc memory %s\n",
                strerror(errno));
            close(fd);
            return -ENOMEM;
        }
    }

    printf("\r\n********************** NORFLASH read test **********************\r\n");
    print_arg(&nor_priv);

    while (nor_priv.times--) {
        if ((ret = lseek(fd, nor_priv.start_addr, SEEK_SET)) < 0) {
            perror("lseek");
            goto error_out;
        }
        if ((ret = read(fd, nor_priv.buffer, len)) < 0) {
            fprintf(stderr, "%s: read, size %ld, %d\n", __func__, len, ret);
            perror("read");
            goto  error_out;
        }
    }
#if 1
    printf("Last Read: \r\n");
    /* dump data */
    for (i = 0; i < len; i++) {
        printf("%02X ", nor_priv.buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\r\n");
        }
    }
#endif

error_out:
    printf("\r\n******************** NORFLASH read test End ********************\r\n\r\n");
    close(fd);
    if (nor_priv.buffer) {
        free(nor_priv.buffer);
    }
    return ret;
}

int nor_chk_main(int argc, char **argv)
{
    int ret, fd;
    unsigned long i, len;
    unsigned int nor_erasesize;
    struct mtd_dev_priv  nor_priv;
    struct erase_info_user erase;
    unsigned char *buffer = NULL;

    memset(&nor_priv, 0, sizeof(struct mtd_dev_priv));

    ret = mtd_arg_get(argc, argv, &nor_priv, 5);
    if (ret < 0) {
        mtdflash_help("nand_chk");
        return -1;
    }

    if ((fd = open_mtd_dev(NOR_TEST_PARTITION)) < 0) {
        return -1;
    }

    if ((ret = mtd_arg_check(fd, &nor_priv, &nor_erasesize)) < 0) {
        close(fd);
        return -1;
    }

    len = nor_priv.end_addr - nor_priv.start_addr;
    if (nor_priv.buffer == NULL) {
        nor_priv.buffer = malloc(len);
        if (nor_priv.buffer == NULL) {
            fprintf(stderr,
                "Error: Could not malloc memory %s\n",
                strerror(errno));
            ret = -ENOMEM;
            goto error_out;
        }
    }
    buffer = malloc(len);
    if (buffer == NULL) {
        fprintf(stderr,
                "Error: Could not malloc memory %s\n",
                strerror(errno));
        ret = -ENOMEM;
        goto error_out;
    }
    memset(buffer, 0, len);

    printf("\r\n********************** NORFLASH write test **********************\r\n");
    print_arg(&nor_priv);

    while (nor_priv.times--) {
        erase.start  = (nor_priv.start_addr / nor_erasesize) * nor_erasesize;
        erase.length = ((len + nor_erasesize) / nor_erasesize) * nor_erasesize;
        if ((ret = ioctl(fd, MEMERASE, &erase)) < 0) {
            perror("MEMERASE");
            goto error_out;
        }
        if (nor_priv.times % 2) {
            for (i = 0; i < len;) {
                nor_priv.buffer[i++] = nor_priv.data1;
                nor_priv.buffer[i++] = nor_priv.data2;
            }
        } else {
            for (i = 0; i < len;) {
                nor_priv.buffer[i++] = nor_priv.data2;
                nor_priv.buffer[i++] = nor_priv.data1;
            }
        }

        if ((ret = lseek(fd, nor_priv.start_addr, SEEK_SET)) < 0) {
            perror("lseek");
            goto error_out;
        }

        if ((ret = write(fd, nor_priv.buffer, len)) < 0) {
            fprintf(stderr, "%s: write, size %ld, %d\n", __func__, len, ret);
            perror("write");
            goto  error_out;
        }

        if ((ret = lseek(fd, nor_priv.start_addr, SEEK_SET)) < 0) {
            perror("lseek");
            goto error_out;
        }

        if ((ret = read(fd, buffer, len)) < 0) {
            fprintf(stderr, "%s: write, size %ld, %d\n", __func__, len, ret);
            perror("write");
            goto  error_out;
        }
        if ((ret = mtd_check_data(buffer, &nor_priv, len)) < 0) {
            nor_priv.check_test_errors++;
        }
    }

error_out:
    printf("Check test errors = %u\r\n", nor_priv.check_test_errors);
    printf("\r\n******************** NORFLASH write test End ********************\r\n\r\n");
    close(fd);
    if (nor_priv.buffer) {
        free(nor_priv.buffer);
    }
    if (buffer) {
        free(buffer);
    }
    return ret;
}
