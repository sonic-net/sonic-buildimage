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
#include <hw_misc/hw_misc.h>
#include <hw_misc/dram_driver.h>

static void dram_help(char *name)
{
    fprintf(stderr,
            "Usage: %s  reg_addr [reg_data]                                         \r\n"
            "  reg_addr                                                             \r\n"
            "  reg_data                                                             \r\n",
            name);
    exit(1);
}

static void phy_help(char *name)
{
    fprintf(stderr,
            "Usage: %s  phy_index(dec) regnum(hex) [regval(hex)]                    \r\n"
            "  phy_index     phydev index                                           \r\n"
            "  regnum        phydev register address                                \r\n"
            "  regval        phydev register value                                  \r\n",
            name);
    exit(1);
}

static void mdio_help(char *name)
{
    fprintf(stderr,
            "Usage: %s  mdio_index(dec) phyaddr(hex) regnum(hex) [regval(hex)]      \r\n"
            "  mdio_index    mdiodev index                                          \r\n"
            "  phyaddr       phydev address                                         \r\n"
            "  regnum        phydev register address                                \r\n"
            "  regval        phydev register value                                  \r\n",
            name);
    exit(1);
}

static int dram_arg_parse(int argc, char **argv,
                          u64 *reg_addr, u64 *reg_data,
                          int min_arg)
{
    u64 reg_addr_tmp, reg_data_tmp = 0;
    char *end;
    int flags = 0;

    if (argc < min_arg) {
        return -EINVAL;
    }

    reg_addr_tmp = strtoul(argv[flags + 1], &end, 0);
    if (*end) {
        fprintf(stderr, "Error: reg addr invalid!\n");
        return -EINVAL;
    }

    if (argc > 2) {
        reg_data_tmp = strtoul(argv[flags + 2], &end, 0);
        if (*end) {
            fprintf(stderr, "Error: reg data invalid!\n");
            return -EINVAL;
        }
    }

    *reg_addr = reg_addr_tmp;
    *reg_data = reg_data_tmp;

    return 0;
}

static int phydev_arg_parse(int argc, char **argv,
                                int *phy_index, u32 *regnum, u32 *regval,
                                int min_arg)
{

    unsigned long index, regaddr, value;
    char *end;

    if (argc < min_arg) {
        return -EINVAL;
    }

    index = strtoul(argv[1], &end, 0);
    if (*end) {
        fprintf(stderr, "Error: index invalid!\n");
        return -EINVAL;
    }

    regaddr = strtoul(argv[2], &end, 16);
    if (*end || regaddr > 0xffff) {
        fprintf(stderr, "Error: regaddr invalid!\n");
        return -EINVAL;
    }

    if (argc > 3) {
        value = strtoul(argv[3], &end, 16);
        if (*end) {
            fprintf(stderr, "Error: reg data invalid!\n");
            return -EINVAL;
        }
        
        *regval = (u32)value;
    }

    *phy_index     =  (u32)index;
    *regnum        =  (u32)regaddr;

    return 0;
}

static int mdiodev_arg_parse(int argc, char **argv,
                                int *mdio_index, int *phyaddr, u32 *regnum, u32 *regval,
                                int min_arg)
{

    unsigned long index, addr, regaddr, value;
    char *end;

    if (argc < min_arg) {
        return -EINVAL;
    }

    index = strtoul(argv[1], &end, 0);
    if (*end) {
        fprintf(stderr, "Error: index invalid!\n");
        return -EINVAL;
    }

    addr = strtoul(argv[2], &end, 16);
    if (*end  || addr > 0x1f) {
        fprintf(stderr, "Error: phyaddr invalid!\n");
        return -EINVAL;
    }
    
    regaddr = strtoul(argv[3], &end, 16);
    if (*end || regaddr > 0xffff) {
        fprintf(stderr, "Error: regaddr invalid!\n");
        return -EINVAL;
    }

    if (argc > 4) {
        value = strtoul(argv[4], &end, 16);
        if (*end) {
            fprintf(stderr, "Error: reg data invalid!\n");
            return -EINVAL;
        }
        
        *regval = (u32)value;
    }

    *mdio_index    =  (u32)index;
    *phyaddr       =  (int)addr;
    *regnum        =  (u32)regaddr;

    return 0;
}                                

int reload_main(int argc, char **argv)
{
    printf("%s, %d, %d, %s\r\n", __FUNCTION__, __LINE__, argc, argv[0]);
    return 0;
}

int sys_info_main(int argc, char **argv)
{
    printf("%s, %d, %d, %s\r\n", __FUNCTION__, __LINE__, argc, argv[0]);
    return 0;
}

int mem_dump_main(int argc, char **argv)
{
    printf("%s, %d, %d, %s\r\n", __FUNCTION__, __LINE__, argc, argv[0]);
    return 0;
}

int cache_flush_main(int argc, char **argv)
{
    printf("%s, %d, %d, %s\r\n", __FUNCTION__, __LINE__, argc, argv[0]);
    return 0;
}

int reg_wr64_main(int argc, char **argv)
{
    u64 reg_addr, reg_data;
    unsigned int reg_data1, reg_data2;
    int fd;
    long int ret;
    unsigned char buf[MAX_DATA_SIZE];

    ret = dram_arg_parse(argc, argv, &reg_addr, &reg_data, 3);
    if (ret < 0) {
        dram_help("reg_wr64");
        return -1;
    }

    if ((fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    if ((ret = lseek(fd, reg_addr, SEEK_SET)) < 0) {
        fprintf(stderr, "Error: Could not llseek :%s\n", strerror(errno));
        close(fd);
        return  -1;
    }
    reg_data2 = (unsigned int)(((u64)reg_data) >> 32) & 0xFFFFFFFF;
    reg_data1 = (unsigned int)reg_data & 0xFFFFFFFF;
    buf[0] = (reg_data1 >> 0) & 0xFF;
    buf[1] = (reg_data1 >> 8) & 0xFF;
    buf[2] = (reg_data1 >> 16) & 0xFF;
    buf[3] = (reg_data1 >> 24) & 0xFF;

    buf[4] = (reg_data2 >> 0) & 0xFF;
    buf[5] = (reg_data2 >> 8) & 0xFF;
    buf[6] = (reg_data2 >> 16) & 0xFF;
    buf[7] = (reg_data2 >> 24) & 0xFF;

    if ((ret = write(fd, buf, 8)) < 0) {
        fprintf(stderr, "Error: Could not write file : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);

    printf("Write quad-word 0x%16llX to address 0x%16llX.\n\n", reg_data, reg_addr);
    return 0;
}

int reg_rd64_main(int argc, char **argv)
{
    u64 reg_addr, reg_data;
    unsigned int reg_data1, reg_data2;
    int fd;
    long int ret;
    unsigned char buf[MAX_DATA_SIZE];

    ret = dram_arg_parse(argc, argv, &reg_addr, &reg_data, 2);
    if (ret < 0) {
        dram_help("reg_rd64");
        return -1;
    }

    if ((fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    if ((ret = lseek(fd, reg_addr, SEEK_SET)) < 0) {
        fprintf(stderr, "Error: Could not llseek : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    if ((ret = read(fd, buf, 8)) < 0) {
        fprintf(stderr, "Error: Could not read file : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);
    reg_data1 = (buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
    reg_data2 = (buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24));

    printf("Read quad-word from address 0x%16llX: 0x%08X%08X\n\n", reg_addr, reg_data2, reg_data1);
    return 0;
}

int reg_wr32_main(int argc, char **argv)
{
    u64 reg_addr, reg_data;
    unsigned int reg_data1, reg_data2;
    int fd;
    long int ret;
    unsigned char buf[MAX_DATA_SIZE];

    ret = dram_arg_parse(argc, argv, &reg_addr, &reg_data, 3);
    if (ret < 0) {
        dram_help("reg_wr32");
        return -1;
    }

    if ((fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    if ((ret = lseek(fd, reg_addr, SEEK_SET)) < 0) {
        fprintf(stderr, "Error: Could not llseek :%s\n", strerror(errno));
        close(fd);
        return  -1;
    }
    reg_data2 = (unsigned int)(((u64)reg_data) >> 32) & 0xFFFFFFFF;
    reg_data1 = (unsigned int)reg_data & 0xFFFFFFFF;
    buf[0] = (reg_data1 >> 0) & 0xFF;
    buf[1] = (reg_data1 >> 8) & 0xFF;
    buf[2] = (reg_data1 >> 16) & 0xFF;
    buf[3] = (reg_data1 >> 24) & 0xFF;

    buf[4] = (reg_data2 >> 0) & 0xFF;
    buf[5] = (reg_data2 >> 8) & 0xFF;
    buf[6] = (reg_data2 >> 16) & 0xFF;
    buf[7] = (reg_data2 >> 24) & 0xFF;

    if ((ret = write(fd, buf, 4)) < 0) {
        fprintf(stderr, "Error: Could not write file : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);

    printf("Write word 0x%08X to address 0x%16llX.\n\n", reg_data1, reg_addr);
    return 0;
}

int reg_rd32_main(int argc, char **argv)
{
    u64 reg_addr, reg_data;
    unsigned int reg_data1;
    int fd;
    long int ret;
    unsigned char buf[MAX_DATA_SIZE];

    ret = dram_arg_parse(argc, argv, &reg_addr, &reg_data, 2);
    if (ret < 0) {
        dram_help("reg_rd32");
        return -1;
    }

    if ((fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    if ((ret = lseek(fd, reg_addr, SEEK_SET)) < 0) {
        fprintf(stderr, "Error: Could not llseek : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    if ((ret = read(fd, buf, 4)) < 0) {
        fprintf(stderr, "Error: Could not read file : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);
    reg_data1 = (buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));

    printf("Read word from address 0x%16llX: 0x%08X\n\n", reg_addr, reg_data1);
    return 0;
}

int reg_wr16_main(int argc, char **argv)
{
    u64 reg_addr, reg_data;
    unsigned int reg_data1, reg_data2;
    int fd;
    long int ret;
    unsigned char buf[MAX_DATA_SIZE];

    ret = dram_arg_parse(argc, argv, &reg_addr, &reg_data, 3);
    if (ret < 0) {
        dram_help("reg_wr16");
        return -1;
    }

    if ((fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    if ((ret = lseek(fd, reg_addr, SEEK_SET)) < 0) {
        fprintf(stderr, "Error: Could not llseek :%s\n", strerror(errno));
        close(fd);
        return  -1;
    }
    reg_data2 = (unsigned int)(((u64)reg_data) >> 32) & 0xFFFFFFFF;
    reg_data1 = (unsigned int)reg_data & 0xFFFFFFFF;
    buf[0] = (reg_data1 >> 0) & 0xFF;
    buf[1] = (reg_data1 >> 8) & 0xFF;
    buf[2] = (reg_data1 >> 16) & 0xFF;
    buf[3] = (reg_data1 >> 24) & 0xFF;

    buf[4] = (reg_data2 >> 0) & 0xFF;
    buf[5] = (reg_data2 >> 8) & 0xFF;
    buf[6] = (reg_data2 >> 16) & 0xFF;
    buf[7] = (reg_data2 >> 24) & 0xFF;

    if ((ret = write(fd, buf, 2)) < 0) {
        fprintf(stderr, "Error: Could not write file : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);

    printf("Write half-word 0x%02X%02X to address 0x%16llX.\n\n", buf[1], buf[0], reg_addr);
    return 0;
}

int reg_rd16_main(int argc, char **argv)
{
    u64 reg_addr, reg_data;
    int fd;
    long int ret;
    unsigned char buf[MAX_DATA_SIZE];

    ret = dram_arg_parse(argc, argv, &reg_addr, &reg_data, 2);
    if (ret < 0) {
        dram_help("reg_rd16");
        return -1;
    }

    if ((fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    if ((ret = lseek(fd, reg_addr, SEEK_SET)) < 0) {
        fprintf(stderr, "Error: Could not llseek : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    if ((ret = read(fd, buf, 2)) < 0) {
        fprintf(stderr, "Error: Could not read file : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);

    printf("Read half-word from address 0x%16llX: 0x%02X%02X\n\n", reg_addr, buf[1], buf[0]);
    return 0;
}

int reg_wr8_main(int argc, char **argv)
{
    u64 reg_addr, reg_data;
    unsigned int reg_data1, reg_data2;
    int fd;
    long int ret;
    unsigned char buf[MAX_DATA_SIZE];

    ret = dram_arg_parse(argc, argv, &reg_addr, &reg_data, 3);
    if (ret < 0) {
        dram_help("reg_wr8");
        return -1;
    }

    if ((fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    if ((ret = lseek(fd, reg_addr, SEEK_SET)) < 0) {
        fprintf(stderr, "Error: Could not llseek :%s\n", strerror(errno));
        close(fd);
        return  -1;
    }
    reg_data2 = (unsigned int)(((u64)reg_data) >> 32) & 0xFFFFFFFF;
    reg_data1 = (unsigned int)reg_data & 0xFFFFFFFF;
    buf[0] = (reg_data1 >> 0) & 0xFF;
    buf[1] = (reg_data1 >> 8) & 0xFF;
    buf[2] = (reg_data1 >> 16) & 0xFF;
    buf[3] = (reg_data1 >> 24) & 0xFF;

    buf[4] = (reg_data2 >> 0) & 0xFF;
    buf[5] = (reg_data2 >> 8) & 0xFF;
    buf[6] = (reg_data2 >> 16) & 0xFF;
    buf[7] = (reg_data2 >> 24) & 0xFF;

    if ((ret = write(fd, buf, 1)) < 0) {
        fprintf(stderr, "Error: Could not write file : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);

    printf("Write byte 0x%02X to address 0x%16llX.\n\n", buf[0], reg_addr);
    return 0;
}

int reg_rd8_main(int argc, char **argv)
{
    u64 reg_addr, reg_data;
    int fd;
    long int ret;
    unsigned char buf[MAX_DATA_SIZE];

    ret = dram_arg_parse(argc, argv, &reg_addr, &reg_data, 2);
    if (ret < 0) {
        dram_help("reg_rd8");
        return -1;
    }

    if ((fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    if ((ret = lseek(fd, reg_addr, SEEK_SET)) < 0) {
        fprintf(stderr, "Error: Could not llseek : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    if ((ret = read(fd, buf, 1)) < 0) {
        fprintf(stderr, "Error: Could not read file : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);

    printf("Read byte from address 0x%16llX: 0x%02X\n\n", reg_addr, buf[0]);
    return 0;
}

int phydev_list_main(int argc, char **argv)
{
    int fd;

    argc = argc;
    argv = argv;
    fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);   
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    (void)ioctl(fd, CMD_PHY_LIST, NULL);

    close(fd);

    return 0;
}

int phydev_rd_main(int argc, char **argv)
{
    struct phydev_user_info  phy_info;
    int fd;
    long int ret;

    ret = phydev_arg_parse(argc, argv, &phy_info.phy_index, &phy_info.regnum, &phy_info.regval, 3);   
    if (ret < 0) {
        phy_help("phydev_rd");
        return -1;
    }

    fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);   
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, CMD_PHY_READ, &phy_info);
    if (ret < 0) {
        fprintf(stderr, "Error: phy read error : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);

    printf("Read success --- phydev%d regnum: 0x%x, value: 0x%x\n",phy_info.phy_index, phy_info.regnum, phy_info.regval);

    return 0;
}

int phydev_wr_main(int argc, char **argv)
{
    struct phydev_user_info  phy_info;
    int fd;
    long int ret;

    ret = phydev_arg_parse(argc, argv, &phy_info.phy_index, &phy_info.regnum, &phy_info.regval, 4);   
    if (ret < 0) {
        phy_help("phydev_wr");
        return -1;
    }

    fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);   
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, CMD_PHY_WRITE, &phy_info);
    if (ret < 0) {
        fprintf(stderr, "Error: phy write error : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);

    printf("write success --- phydev%d regnum: 0x%x, value: 0x%x\n",phy_info.phy_index, phy_info.regnum, phy_info.regval);

    return 0;
}

int mdiodev_list_main(int argc, char **argv)
{
    int fd;

    argc = argc;
    argv = argv;
    fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);   
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    (void)ioctl(fd, CMD_MDIO_LIST, NULL);

    close(fd);

    return 0;
}

int mdiodev_rd_main(int argc, char **argv)
{
    struct mdio_dev_user_info  mdio_info;
    int fd;
    long int ret;

    ret = mdiodev_arg_parse(argc, argv, &mdio_info.mdio_index, &mdio_info.phyaddr, &mdio_info.regnum, &mdio_info.regval, 4);   
    if (ret < 0) {
        mdio_help("mdiodev_rd");
        return -1;
    }

    fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);   
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, CMD_MDIO_READ, &mdio_info);
    if (ret < 0) {
        fprintf(stderr, "Error: mdio read error : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);

    printf("Read success\n mdio_index  phyaddr     regnum      value\n");
    printf("  %-10d  %#-10x  %#-10x  %#-10x\n", mdio_info.mdio_index, mdio_info.phyaddr, mdio_info.regnum, mdio_info.regval);

    return 0;
}

int mdiodev_wr_main(int argc, char **argv)
{
    struct mdio_dev_user_info  mdio_info;
    int fd;
    long int ret;

    ret = mdiodev_arg_parse(argc, argv, &mdio_info.mdio_index, &mdio_info.phyaddr, &mdio_info.regnum, &mdio_info.regval, 5);    
    if (ret < 0) {
        mdio_help("mdiodev_wr");
        return -1;
    }

    fd = open("/dev/dram_test", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);   
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open file "
                "/dev/dram: %s\n", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, CMD_MDIO_WRITE, &mdio_info);
    if (ret < 0) {
        fprintf(stderr, "Error: mdio write error : %s\n", strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);

    printf("write success\n mdio_index  phyaddr     regnum      value\n");
    printf("  %-10d  %#-10x  %#-10x  %#-10x\n", mdio_info.mdio_index, mdio_info.phyaddr, mdio_info.regnum, mdio_info.regval);

    return 0;
}
