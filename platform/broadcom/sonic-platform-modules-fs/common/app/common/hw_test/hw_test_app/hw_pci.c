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
#include <sys/mman.h>
#include <hw_pci/hw_pci.h>

static void pci_help(char *name)
{
    fprintf(stderr,
            "Usage: %s bus slot fn bar offset data [times]                          \r\n"
            "  bus          pci bus number                                          \r\n"
            "  slot         pci slot number                                         \r\n"
            "  fn           pci device function number                              \r\n"
            "  bar          pci resource bar number                                 \r\n"
            "  offset       pci register offset                                     \r\n"
            "  data         pci test data[32bit]                                     \r\n"
            "  [times]                                                              \r\n",
            name);
    exit(1);
}

static void pci_cfg_help(char *name)
{
    fprintf(stderr,
            "Usage: %s bus slot fn offset data [times]                              \r\n"
            "  bus          pci bus number                                          \r\n"
            "  slot         pci slot number                                         \r\n"
            "  fn           pci device function number                              \r\n"
            "  offset       pci register offset                                     \r\n"
            "  data         pci test data[8bit]                                     \r\n"
            "  [times]                                                              \r\n",
            name);
    exit(1);
}

static int pci_arg_parse(int argc, char **argv,
                         struct pci_dev_priv *pci_priv, int min_arg, int is_cfg)
{
    int pcibus, slot, fn, bar = 0, offset = 0, times = 1;
    unsigned long int data = 0;
    char *end;
    int flags = 0;

    if (argc < min_arg) {
        return -EINVAL;
    }

    pcibus = strtol(argv[flags + 1], &end, 0);
    if (*end || pcibus < 0 || pcibus > 255) {
        fprintf(stderr, "Error: pci bus number invalid!\n");
        return -EINVAL;
    }

    slot = strtol(argv[flags + 2], &end, 0);
    if (*end || slot < 0 || slot > 255) {
        fprintf(stderr, "Error: pci slot number invalid!\n");
        return -EINVAL;
    }

    fn = strtol(argv[flags + 3], &end, 0);
    if (*end || fn < 0 || fn > 32) {
        fprintf(stderr, "Error: pci device function number invalid!\n");
        return -EINVAL;
    }

    if (!is_cfg) {
        bar = strtol(argv[flags + 4], &end, 0);
        if (*end || bar < 0 || bar > 12) {
            fprintf(stderr, "Error: pci resource bar number invalid!\n");
            return -EINVAL;
        }
        flags++;
    }

    offset = strtol(argv[flags + 4], &end, 0);
    if (*end || offset < 0 || (offset & 0x3)) {
        fprintf(stderr, "Error: pci register offset invalid!\n");
        return -EINVAL;
    }

    if ((argc - flags) > 5) {
        data = strtoul(argv[flags + 5], &end, 0);
        if (*end) {
            fprintf(stderr, "Error: data invalid!\n");
            return -EINVAL;
        }
    }

    if ((argc - flags) > 6) {
        times = strtol(argv[flags + 6], &end, 0);
        if (*end || times < 0) {
            fprintf(stderr, "Error: times invalid!\n");
            return -EINVAL;
        }
    }

    pci_priv->pcibus        = pcibus;
    pci_priv->slot          = slot;
    pci_priv->fn            = fn;
    pci_priv->bar           = bar;
    pci_priv->offset        = offset;
    pci_priv->data          = (unsigned int)data;
    pci_priv->times         = times;
    return 0;
}

static void print_arg(struct pci_dev_priv *pci_priv, int is_cfg)
{
    printf("Bus: %d, Slot: %d/%d",
           pci_priv->pcibus, pci_priv->slot, pci_priv->fn);

    if (!is_cfg) {
        printf(", Bar: %d", pci_priv->bar);
    }
    printf(", offset: 0x%X", pci_priv->offset);

    if (pci_priv->data) {
        printf(", Data = 0x%08X", pci_priv->data);
    }

    if (pci_priv->times) {
        printf(", Times = %u", pci_priv->times);
    }

    printf("\r\n");
}

static int open_pci_dev(struct pci_dev_priv *pci_priv, int is_cfg)
{
    int file, ret;
    char filename[PCI_MAX_NAME_SIZE];

    if (is_cfg) {
        ret = snprintf(filename, PCI_MAX_NAME_SIZE,
                       "/sys/class/pci_bus/%04x:%02x/device/%04x:%02x:%02x.%d/config",
                       0, pci_priv->pcibus, 0, pci_priv->pcibus, pci_priv->slot, pci_priv->fn);
    } else {
        ret = snprintf(filename, PCI_MAX_NAME_SIZE,
                       "/sys/class/pci_bus/%04x:%02x/device/%04x:%02x:%02x.%d/resource%d",
                       0, pci_priv->pcibus, 0, pci_priv->pcibus, pci_priv->slot, pci_priv->fn,
                       pci_priv->bar);
    }

    filename[ret] = '\0';
    if ((file = open(filename, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "`%s': %s\n", filename, strerror(errno));
    }

    printf("file;%s\r\n", filename);
    return file;
}

int pci_wr_main(int argc, char **argv)
{
    int ret, fd;
    struct pci_dev_priv pci_priv;
    unsigned int *ptr;
    struct stat sb;

    memset(&pci_priv, 0, sizeof(struct pci_dev_priv));
    ret = pci_arg_parse(argc, argv, &pci_priv, 6, 0);
    if (ret < 0) {
        pci_help("pci_wr");
        return -1;
    }

    if ((fd = open_pci_dev(&pci_priv, 0)) < 0) {
        return -1;
    }

    if ((ret = fstat(fd, &sb)) == -1) {
        fprintf(stderr, "Error: Could not fstat : %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if (pci_priv.offset >= sb.st_size) {
        fprintf(stderr, "Error: offset is out of range\n");
        close(fd);
        return -1;
    }

    if ((ptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0)) == MAP_FAILED) {
        fprintf(stderr, "Error: Could not mmap : %s  or resource is IO\n", strerror(errno));
        close(fd);
        return -1;
    }

    printf("\r\n"
           "********************** PCI write test **********************\r\n");
    print_arg(&pci_priv, 1);

    while (pci_priv.times--) {
        *((volatile unsigned int *)(ptr + pci_priv.offset / sizeof(unsigned int))) = pci_priv.data;
    }
    printf("Write 0x%08X to offset 0x%08X.\n\n", pci_priv.data, pci_priv.offset);

    printf("\r\n******************** PCI write test End ********************\r\n\r\n");
    munmap(ptr, sb.st_size);
    close(fd);
    return ret;
}

int pci_rd_main(int argc, char **argv)
{
    int ret, fd;
    struct pci_dev_priv pci_priv;
    unsigned int data;
    unsigned int *ptr;
    struct stat sb;

    memset(&pci_priv, 0, sizeof(struct pci_dev_priv));
    ret = pci_arg_parse(argc, argv, &pci_priv, 6, 0);
    if (ret < 0) {
        pci_help("pci_rd");
        return -1;
    }

    if ((fd = open_pci_dev(&pci_priv, 0)) < 0) {
        return -1;
    }

    if ((ret = fstat(fd, &sb)) == -1) {
        fprintf(stderr, "Error: Could not fstat : %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if (pci_priv.offset >= sb.st_size) {
        fprintf(stderr, "Error: offset is out of range\n");
        close(fd);
        return -1;
    }

    if ((ptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0)) == MAP_FAILED) {
        fprintf(stderr, "Error: Could not mmap : %s  or resource is IO\n", strerror(errno));
        close(fd);
        return -1;
    }

    printf("\r\n"
           "********************** PCI read test **********************\r\n");
    print_arg(&pci_priv, 1);

    data = 0;
    while (pci_priv.times--) {
        data = *((volatile unsigned int *)(ptr + pci_priv.offset / sizeof(unsigned int)));
    }
    printf("Last Read: Data = 0x%08X\n", data);

    printf("\r\n******************** PCI read test End ********************\r\n\r\n");
    munmap(ptr, sb.st_size);
    close(fd);
    return ret;
}

int pci_cfg_wr_main(int argc, char **argv)
{
    int ret, fd;
    struct pci_dev_priv pci_priv;
    char buf[16];

    memset(&pci_priv, 0, sizeof(struct pci_dev_priv));
    ret = pci_arg_parse(argc, argv, &pci_priv, 6, 1);
    if (ret < 0) {
        pci_cfg_help("pci_cfg_wr");
        return -1;
    }

    if ((fd = open_pci_dev(&pci_priv, 1)) < 0) {
        return -1;
    }

    printf("\r\n"
           "********************** PCI config write test **********************\r\n");
    print_arg(&pci_priv, 1);

    while (pci_priv.times--) {
        if ((ret = lseek(fd, pci_priv.offset, SEEK_SET)) < 0) {
            fprintf(stderr, "Error: Could not llseek : %s\n", strerror(errno));
            goto error_out;
        }

        buf[0] = (pci_priv.data >> 0) & 0xFF;
        buf[1] = (pci_priv.data >> 8) & 0xFF;
        buf[2] = (pci_priv.data >> 16) & 0xFF;
        buf[3] = (pci_priv.data >> 24) & 0xFF;
        if ((ret = write(fd, buf, sizeof(unsigned int))) < 0) {
            fprintf(stderr, "Error: Could not read file : %s\n", strerror(errno));
            goto error_out;
        }
    }
    printf("Write 0x%08X to offset 0x%08X.\n\n", pci_priv.data, pci_priv.offset);

error_out:
    printf("\r\n******************** PCI config write test End ********************\r\n\r\n");
    close(fd);
    return ret;
}

int pci_cfg_rd_main(int argc, char **argv)
{
    int ret, fd;
    struct pci_dev_priv pci_priv;
    unsigned int data;

    memset(&pci_priv, 0, sizeof(struct pci_dev_priv));
    ret = pci_arg_parse(argc, argv, &pci_priv, 5, 1);
    if (ret < 0) {
        pci_cfg_help("pci_cfg_rd");
        return -1;
    }

    if ((fd = open_pci_dev(&pci_priv, 1)) < 0) {
        return -1;
    }

    printf("\r\n"
           "********************** PCI config read test **********************\r\n");
    print_arg(&pci_priv, 1);

    while (pci_priv.times--) {
        if ((ret = lseek(fd, pci_priv.offset, SEEK_SET)) < 0) {
            fprintf(stderr, "Error: Could not llseek : %s\n", strerror(errno));
            goto error_out;
        }

        if ((ret = read(fd, &data, sizeof(unsigned int))) < 0) {
            fprintf(stderr, "Error: Could not read file : %s\n", strerror(errno));
            goto error_out;
        }
    }
    printf("Last Read: Data = 0x%08X\n", data);

error_out:
    printf("\r\n******************** PCI config read test End ********************\r\n\r\n");
    close(fd);
    return ret;
}

int pci_chk_main(int argc, char **argv)
{
    int ret, fd;
    struct pci_dev_priv pci_priv;
    unsigned int data;
    unsigned int *ptr;
    struct stat sb;

    memset(&pci_priv, 0, sizeof(struct pci_dev_priv));
    ret = pci_arg_parse(argc, argv, &pci_priv, 6, 0);
    if (ret < 0) {
        pci_help("pci_wr");
        return -1;
    }

    if ((fd = open_pci_dev(&pci_priv, 0)) < 0) {
        return -1;
    }

    if ((ret = fstat(fd, &sb)) == -1) {
        fprintf(stderr, "Error: Could not fstat : %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if (pci_priv.offset >= sb.st_size) {
        fprintf(stderr, "Error: offset is out of range\n");
        close(fd);
        return -1;
    }

    if ((ptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0)) == MAP_FAILED) {
        fprintf(stderr, "Error: Could not mmap : %s  or resource is IO\n", strerror(errno));
        close(fd);
        return -1;
    }

    printf("\r\n"
           "********************** PCI Check test **********************\r\n");
    print_arg(&pci_priv, 1);

    while (pci_priv.times--) {
        *((volatile unsigned int *)(ptr + pci_priv.offset / sizeof(unsigned int))) = pci_priv.data;
        data = *((volatile unsigned int *)(ptr + pci_priv.offset / sizeof(unsigned int)));
        if (pci_priv.data != data) {
            pci_priv.check_test_errors++;
        }
    }
    printf("Check test errors = %u\r\n", pci_priv.check_test_errors);

    printf("\r\n******************** PCI Check test End ********************\r\n\r\n");
    munmap(ptr, sb.st_size);
    close(fd);
    return ret;
}

int pci_dump_main(int argc, char **argv)
{
    int ret, fd, i;
    struct pci_dev_priv pci_priv;
    unsigned int *ptr;
    unsigned char *p;
    struct stat sb;

    memset(&pci_priv, 0, sizeof(struct pci_dev_priv));
    ret = pci_arg_parse(argc, argv, &pci_priv, 6, 0);
    if (ret < 0) {
        pci_help("pci_dump");
        return -1;
    }

    if ((fd = open_pci_dev(&pci_priv, 0)) < 0) {
        return -1;
    }

    if ((ret = fstat(fd, &sb)) == -1) {
        fprintf(stderr, "Error: Could not fstat : %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if (pci_priv.offset >= sb.st_size) {
        fprintf(stderr, "Error: offset is out of range\n");
        close(fd);
        return -1;
    }

    if ((ptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0)) == MAP_FAILED) {
        fprintf(stderr, "Error: Could not mmap : %s  or resource is IO\n", strerror(errno));
        close(fd);
        return -1;
    }

    printf("\r\n"
           "********************** PCI dump test **********************\r\n");
    print_arg(&pci_priv, 1);

    printf(" Address |  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n");
    while (pci_priv.times--) {
        p = (unsigned char *)ptr;
        for (i = 0; i < pci_priv.offset; i++, p++) {
            if (i % 16 == 0) {
                printf("%08X | ", (unsigned int)((unsigned long)p - (unsigned long)ptr));
            }
            printf("%02X ", *p);
            if ((i + 1) % 16 == 0) {
                printf("\r\n");
            }
        }
    }

    printf("\r\n******************** PCI dump test End ********************\r\n\r\n");
    munmap(ptr, sb.st_size);
    close(fd);
    return ret;
}
