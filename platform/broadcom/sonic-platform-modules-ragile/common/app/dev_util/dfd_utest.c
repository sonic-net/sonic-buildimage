
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <stdint.h>
#include <semaphore.h>
#include <linux/version.h>
#include <sys/mman.h>
#include <time.h>
#include "dfd_utest.h"

#define DFD_UTEST_MAX_RDWR_NUM            (256)
#define DFD_UTEST_DEFAULT_WR_NUM          (1)

#define DEV_MEM_NAME                    "/dev/mem"
#define DEV_KMEM_NAME                   "/dev/kmem"

#define WIDTH_1Byte                       (1)
#define WIDTH_2Byte                       (2)
#define WIDTH_4Byte                       (4)
#define DFD_UTEST_MAX_BIT_WIDTH           (4)

#ifdef DFD_UTEST_ITEM
#undef DFD_UTEST_ITEM
#endif
#define DFD_UTEST_ITEM(_id, _type_str, _help_info, _help_info_detail)    {_id, #_type_str, dfd_utest_##_type_str, _help_info, _help_info_detail},
static dfd_utest_t g_dfd_unit_test[] = {
    DFD_UTEST_ITEM_ALL
};

static int g_sys_page_size;
#define SYS_PAGE_SIZE                   g_sys_page_size
#define SYS_PAGE_MASK                   (~(SYS_PAGE_SIZE - 1))

void dfd_utest_print_cmd(int argc, char* argv[])
{
    int i;

    for (i = 1; i < argc; i++) {
        if (i != 1) {
            printf(" ");
        }
        printf("%s", argv[i]);
    }
    return;
}

void dfd_utest_print_all_help(void)
{
    int i, tbl_size;

    tbl_size = sizeof(g_dfd_unit_test) / sizeof(g_dfd_unit_test[0]);

    for (i = 0; i < tbl_size; i++) {
        printf("%-20s\t\t\t%s\r\n", g_dfd_unit_test[i].type_str, g_dfd_unit_test[i].help_info);
    }

    return;
}

void dfd_utest_printf_single_help(int utest_type)
{
    int i, tbl_size;

    tbl_size = sizeof(g_dfd_unit_test) / sizeof(g_dfd_unit_test[0]);
    for (i = 0; i < tbl_size; i++) {
        if (g_dfd_unit_test[i].utest_type == utest_type) {
            printf("%-20s\t\t\t%s\r\n", g_dfd_unit_test[i].type_str, g_dfd_unit_test[i].help_info_detail);
            return;
        }
    }

    DFD_DEBUG_DBG("type: %d not match.\n", utest_type);
    return;

}

void dfd_utest_printf_reg(uint8_t *buf, int buf_len, uint32_t offset_addr)
{
    int i, j, tmp;

    j = offset_addr % 16;
    tmp = j;
    offset_addr -= j;
    printf("\n            ");

    for (i = 0; i < 16; i++) {
        printf("%2x ", i);
    }

    for (i = 0; i < buf_len + j; i++) {
        if ((i % 16) == 0) {
            printf("\n0x%08x  ", offset_addr);
            offset_addr = offset_addr + 16;
        }
        if (tmp) {
            printf("   ");
            tmp--;
        } else {
            printf("%02x ", buf[i-j]);
        }
    }

    printf("\n");
    return;
}

#define I2C_RETRIES 0x0701
#define I2C_TIMEOUT 0x0702
#define I2C_RDWR 0x0707

#define I2C_SLAVE	0x0703	/* Use this slave address */

#define I2C_SLAVE_FORCE	0x0706	/* Use this slave address, even if it
				   is already in use by a driver! */
#define I2C_PEC		0x0708	/* != 0 to use PEC with SMBus */
#define I2C_SMBUS	0x0720	/* SMBus transfer */

struct i2c_msg
{
 unsigned short addr;
 unsigned short flags;
#define I2C_M_TEN 0x0010
#define I2C_M_RD 0x0001
 unsigned short len;
 unsigned char *buf;
};

struct i2c_rdwr_ioctl_data
{
 struct i2c_msg *msgs;
 int nmsgs;

};

#define DFD_I2C_SHORT_ADDR_TYPE 0
#define DFD_I2C_RETRY_SLEEP_TIME        (10000)   /* 10ms */
#define DFD_I2C_RETRY_TIME              (50000 / DFD_I2C_RETRY_SLEEP_TIME)
/* i2c_smbus_xfer read or write markers */
#define I2C_SMBUS_READ	1
#define I2C_SMBUS_WRITE	0

/* SMBus transaction types (size parameter in the above functions)
   Note: these no longer correspond to the (arbitrary) PIIX4 internal codes! */
#define I2C_SMBUS_QUICK		    0
#define I2C_SMBUS_BYTE		    1
#define I2C_SMBUS_BYTE_DATA	    2
#define I2C_SMBUS_WORD_DATA	    3
#define I2C_SMBUS_PROC_CALL	    4
#define I2C_SMBUS_BLOCK_DATA	    5
#define I2C_SMBUS_I2C_BLOCK_BROKEN  6
#define I2C_SMBUS_BLOCK_PROC_CALL   7		/* SMBus 2.0 */
#define I2C_SMBUS_I2C_BLOCK_DATA    8

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,36)
/*  fix tjm */

#ifndef __ASSEMBLY__
/*
 * __xx is ok: it doesn't pollute the POSIX namespace. Use these in the
 * header files exported to user space
 */
typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

typedef __signed__ long __s64;
typedef unsigned long __u64;

#endif /* __ASSEMBLY__ */

#else
/*  do noting add tjm */
#endif

/*
 * Data for SMBus Messages
 */
#define I2C_SMBUS_BLOCK_MAX	32	/* As specified in SMBus standard */
union i2c_smbus_data {
	__u8 byte;
	__u16 word;
	__u8 block[I2C_SMBUS_BLOCK_MAX + 2]; /* block[0] is used for length */
			       /* and one more for user-space compatibility */
};

/* This is the structure as used in the I2C_SMBUS ioctl call */
struct i2c_smbus_ioctl_data {
	__u8 read_write;
	__u8 command;
	__u32 size;
	union i2c_smbus_data  *data;
};
int32_t dfd_read_port_i2c_one_time_smbus(char *i2c_name, uint16_t dev_addr, uint16_t offset_addr,
                                   uint8_t *recv_buf, int32_t size, int addr_type)
{
     union i2c_smbus_data data;
     struct i2c_smbus_ioctl_data ioctl_data;
     unsigned long addr = dev_addr;
     int fd;
     int rc;
     int rv;
     int i;

    mem_clear(&ioctl_data, sizeof(struct i2c_smbus_ioctl_data));
    if (i2c_name == NULL || recv_buf == NULL) {
            DFD_DEBUG_ERROR("i2c_num = NULL, recv_buf = NULL\r\n");
            return -1;
    }

    DFD_DEBUG_DBG("i2c name: %s, dev_addr: 0x%x, offset_addr: 0x%x, size: %d, addr_type: %d.\n", i2c_name, dev_addr,
        offset_addr, size, addr_type);

    rv = 0;
    fd = open(i2c_name, O_RDWR | O_SYNC);
    if (fd < 0) {
        DFD_DEBUG_ERROR("i2c open fail fd %d\n", fd);
        rv = fd;
        goto err;
    }
    if (ioctl(fd, I2C_SLAVE_FORCE , addr) < 0) {
        DFD_DEBUG_ERROR("ioctl 2C_SLAVE_FORCE %d.\n", errno);
        rv =-1;
        goto fail;
    }
    for (i = 0 ;i < size; i++) {
         data.byte = 0;
         ioctl_data.read_write = I2C_SMBUS_READ;
         ioctl_data.command = (offset_addr + i);
         ioctl_data.size = I2C_SMBUS_BYTE_DATA;
         ioctl_data.data= &data;

         rc = ioctl(fd, I2C_SMBUS, &ioctl_data);
         if (rc < 0) {
             DFD_DEBUG_ERROR("read, I2C_SMBUS failed: %d.\n", errno);
             rv = -1;
             goto fail;
         }
         *(recv_buf + i) = data.byte;
     }
  fail:
     close(fd);
  err:
     return rv;

}

int32_t dfd_read_port_i2c_one_time(char *i2c_name, uint16_t dev_addr, uint16_t offset_addr,
                                   uint8_t *recv_buf, int32_t size, int addr_type)
{

        int32_t fd, rv;
        struct i2c_rdwr_ioctl_data ioctl_data;
        struct i2c_msg msgs[2];
        uint8_t buf[2];

        if (i2c_name == NULL || recv_buf == NULL) {
            DFD_DEBUG_ERROR("i2c_num = NULL, recv_buf = NULL\r\n");
            return -1;
        }

        DFD_DEBUG_DBG("i2c name %s, dev_addr 0x%x, offset_addr 0x%x, size %d, addr_type %d.\n", i2c_name, dev_addr,
            offset_addr, size, addr_type);

        rv = 0;
        fd = open(i2c_name, O_RDWR | O_SYNC);
        if (fd < 0) {
            DFD_DEBUG_ERROR("i2c open fail fd %d\n", fd);
            return -1;
        }
        mem_clear(&ioctl_data, sizeof(ioctl_data));
        mem_clear(msgs, sizeof(msgs));
        mem_clear(buf, sizeof(buf));
        if (ioctl(fd, I2C_SLAVE, dev_addr) < 0) {

            DFD_DEBUG_ERROR("%s %dioctl fail(ret:%d, errno:%s)!\r\n", __func__ , __LINE__, rv, strerror(errno));
            rv = -1;
            goto fail;
        }

         buf[0] = (uint8_t)(offset_addr);
         msgs[0].addr= dev_addr;
         msgs[0].len= 2;
         msgs[0].buf= buf;
         msgs[1].addr= dev_addr;
         msgs[1].flags|= I2C_M_RD;
         msgs[1].len= 1;
         msgs[1].buf= recv_buf;
         ioctl_data.nmsgs= 1;
         ioctl_data.msgs= msgs;

         rv = ioctl(fd, I2C_RDWR, &ioctl_data);
        if(rv < 0) {
            DFD_DEBUG_ERROR("%s %dioctl fail(ret:%d, errno:%s)!\r\n", __func__ , __LINE__, rv, strerror(errno));
            goto fail;
        }
        ioctl_data.msgs= &msgs[1];
        DFD_DEBUG_DBG("ioctlread, return :%d/n", ioctl(fd, I2C_RDWR, &ioctl_data));
        DFD_DEBUG_DBG("dfd_read_port_i2c addr: 0x%X, offset: 0x%X, value: 0x%X\n", dev_addr, offset_addr, *recv_buf);
    fail:
        close(fd);
        return rv;

}

int32_t dfd_read_port_i2c(char *i2c_name, uint16_t dev_addr, uint16_t offset_addr,
                          uint8_t *recv_buf, int32_t size)
{
    int i;
    int rv;

    for (i = 0; i < DFD_I2C_RETRY_TIME; i++) {
        rv = dfd_read_port_i2c_one_time_smbus(i2c_name, dev_addr, offset_addr, recv_buf, size, DFD_I2C_SHORT_ADDR_TYPE);
        if (rv < 0) {
            DFD_DEBUG_ERROR("(read times %d)i2c name %s, dev_addr 0x%X, offset_addr 0x%X, addr_type %d\n", i, i2c_name, dev_addr, offset_addr, DFD_I2C_SHORT_ADDR_TYPE);
            usleep(DFD_I2C_RETRY_SLEEP_TIME);
            continue;
        }
        break;
    }

    return rv;
}

int32_t dfd_write_port_i2c_one_time(char *i2c_name, uint16_t dev_addr, uint16_t offset_addr,
                                    uint8_t *write_buf, int32_t size,int addr_type)
{
    int32_t fd, rv;
    int index;
    struct i2c_smbus_ioctl_data ioctl_data;
    union  i2c_smbus_data data;
    uint8_t addr_buf[2];
    uint8_t write_buf_tmp[256];

    if (i2c_name == NULL || write_buf == NULL ) {
        DFD_DEBUG_ERROR("i2c_num = NULL \r\n");
        return -1;
    }

    if (size <= 0) {
        DFD_DEBUG_ERROR("error:size\n");
        return -1;
    }
    DFD_DEBUG_DBG("i2c name %s, dev_addr 0x%x, offset_addr 0x%x, size %d, addr_type %d\n",i2c_name, dev_addr,
        offset_addr, size, addr_type);
    mem_clear(&ioctl_data, sizeof(ioctl_data));
    mem_clear(addr_buf, sizeof(addr_buf));
    mem_clear(write_buf_tmp, sizeof(write_buf_tmp));

    rv = 0;

    fd = open(i2c_name, O_RDWR | O_SYNC);
    if (fd < 0) {
        DFD_DEBUG_ERROR("i2c open fail fd %d\n", fd);
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE_FORCE, dev_addr) < 0) {
        DFD_DEBUG_ERROR("ioctl, I2C_SLAVE failed: %d.\n", errno);
        rv = -1;
        goto fail;
     }

    for (index = 0; index < size; index++) {
            data.byte = *(write_buf + index);
            ioctl_data.read_write = I2C_SMBUS_WRITE;
            ioctl_data.command = (offset_addr + index);
            ioctl_data.size = I2C_SMBUS_BYTE_DATA;
            ioctl_data.data= &data;
            rv = ioctl(fd, I2C_SMBUS, (unsigned long)&ioctl_data);
            if(rv < 0) {
                DFD_DEBUG_ERROR("ioctl fail(ret:%d, errno:%s %d) !\r\n", rv, strerror(errno),errno);
                break;
            }
            DFD_DEBUG_DBG("ret:%d  value:0x%02x\n", rv, data.byte);
            usleep(5000);
    }

fail:
    close(fd);
    return rv;
}

int32_t dfd_write_port_i2c(char *i2c_name, uint16_t dev_addr, uint16_t offset_addr,
                           uint8_t *write_buf, int32_t size)
{
    int i;
    int rv;

    for (i = 0; i < DFD_I2C_RETRY_TIME; i++) {
        rv = dfd_write_port_i2c_one_time(i2c_name, dev_addr, offset_addr, write_buf,size, DFD_I2C_SHORT_ADDR_TYPE);
        if (rv < 0) {
            DFD_DEBUG_ERROR("(write times %d)i2c name %s, dev_addr 0x%X, offset_addr 0x%X, addr_type %d\n",
                i, i2c_name, dev_addr, offset_addr, DFD_I2C_SHORT_ADDR_TYPE);
            usleep(DFD_I2C_RETRY_SLEEP_TIME);
            continue;
        }
        break;
    }

    return rv;
}

static int dfd_read_io_port(uint16_t offset_addr, uint8_t *recv_buf, int32_t size)
{
    int fd;
    int ret;

    fd = open("/dev/port", O_RDWR);
    if (fd < 0) {
        printf("open failed ret %d.\n", fd);
        return -1;
    }

    ret = lseek(fd, offset_addr, SEEK_SET);
    if (ret < 0) {
        printf("lseek failed ret %d.\n", ret);
        goto exit;
    }

    ret = read(fd, recv_buf, size);
    if (ret != size) {
        printf("read failed ret %d size %d.\n", ret, size);
        ret = -1;
        goto exit;
    }

exit:
    close(fd);
    return ret;
}

static int dfd_write_io_port(uint16_t offset_addr, uint8_t *write_buf, int32_t size)
{
    int fd;
    int ret;

    fd = open("/dev/port", O_RDWR);
    if (fd < 0) {
        printf("open failed ret %d.\n", fd);
        return -1;
    }

    ret = lseek(fd, offset_addr, SEEK_SET);
    if (ret < 0) {
        printf("lseek failed ret %d.\n", ret);
        goto exit;
    }

    ret = write(fd, write_buf, size);
    if (ret != size) {
        printf("write failed ret %d size %d.\n", ret, size);
        ret = -1;
        goto exit;
    }

exit:
    close(fd);
    return ret;
}

static int dfd_process_mem(char *dev_name, char is_wr, char width, off_t offset, uint8_t *buf, int32_t size)
{
    int mfd, ret = 0;
    void *base;
    int i, j;
    unsigned int val;
    off_t map_offset;
    size_t map_size;

    if (size & (width - 1)) {
        printf("size %d invalid.\n", size);
        return -1;
    }

    mfd = open(dev_name, O_RDWR);
    if (mfd < 0) {
        printf("Cannot open %s.\n", dev_name);
        return -1;
    }

    g_sys_page_size = getpagesize();
    map_offset = offset & SYS_PAGE_MASK;
    map_size = size + offset - map_offset;
    base = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, mfd, map_offset);
    if (base == MAP_FAILED) {
        printf("mmap offset 0x%lx failed error(%s).\n", map_offset, strerror(errno));
        close(mfd);
        return -1;
    }
    printf("width %d map_offset 0x%lx, offset 0x%lx, mmap base %p, g_sys_page_size %d\n",
        width, map_offset, offset, base, g_sys_page_size);

    if (is_wr) {
        for (i = 0; i < size; i = i + width) {
            val = 0;
            for (j = 0; j < width; j++) {
                val |= buf[i + j] << (8 * j);
            }
            switch (width) {
            case 1:
                *((volatile unsigned char*)(base + i + offset - map_offset)) = val;
                break;
            case 2:
                *((volatile unsigned short*)(base + i + offset - map_offset)) = val;
                break;
            case 4:
                *((volatile unsigned int*)(base + i + offset - map_offset)) = val;
                break;
            default:
                ret = -1;
                printf("Not support width %d.\n", width);
                goto exit;
            }
        }
    } else {
        for (i = 0; i < size; i = i + width) {
            switch (width) {
            case 1:
                val = *((volatile unsigned char*)(base + i + offset - map_offset));
                break;
            case 2:
                val = *((volatile unsigned short*)(base + i + offset - map_offset));
                break;
            case 4:
                val = *((volatile unsigned int*)(base + i + offset - map_offset));
                break;
            default:
                ret = -1;
                printf("Not support width %d.\n", width);
                goto exit;
            }
            for (j = 0; j < width; j++) {
                buf[i + j] = (val >> (8 * j)) & 0xff;
            }
        }
    }
exit:
    munmap(base, map_size);
    close(mfd);
    return ret;
}

int32_t dfd_i2c_gen_read_one_time(char *i2c_path, uint32_t dev_addr, uint32_t addr_bitwidth,
                uint32_t offset_addr, uint8_t *recv_buf, int32_t rd_len)
{
    int32_t fd, rv, i;
    struct i2c_rdwr_ioctl_data ioctl_data;
    struct i2c_msg msgs[2];
    uint8_t buf[DFD_UTEST_MAX_BIT_WIDTH];

    fd = open(i2c_path, O_RDWR | O_SYNC);
    if (fd < 0) {
        DFD_DEBUG_ERROR("i2c open fail fd:%d\n", fd);
        return -1;
    }
    mem_clear(&ioctl_data, sizeof(ioctl_data));
    mem_clear(msgs, sizeof(msgs));
    mem_clear(buf, sizeof(buf));

    i = 0;

    switch (addr_bitwidth) {
    case WIDTH_4Byte:
        buf[i++] = (offset_addr >> 24) & 0xFF;
        buf[i++] = (offset_addr >> 16) & 0xFF;
        buf[i++] = (offset_addr >> 8) & 0xFF;
        buf[i++] = offset_addr & 0xFF;
        break;
    case WIDTH_2Byte:
        buf[i++] = (offset_addr >> 8) & 0xFF;
        buf[i++] = offset_addr & 0xFF;
        break;
    case WIDTH_1Byte:
        buf[i++] = offset_addr & 0xFF;
        break;
    default:
        DFD_DEBUG_ERROR("Only support 1,2,4 Byte Address Width,but set %u addr_bitwidth \n", addr_bitwidth);
        rv = -1;
        goto fail;
    }

    msgs[0].addr = dev_addr;
    msgs[0].flags = 0;
    msgs[0].len = addr_bitwidth;
    msgs[0].buf = buf;
    msgs[1].addr = dev_addr;
    msgs[1].flags |= I2C_M_RD;
    msgs[1].len = rd_len;
    msgs[1].buf = recv_buf;
    ioctl_data.nmsgs = 2;
    ioctl_data.msgs = msgs;

    rv = ioctl(fd, I2C_RDWR, &ioctl_data);
    if(rv < 0) {
        DFD_DEBUG_ERROR("%s %d Error: Sending messages failed:(ret:%d, errno:%s)!\n", __func__ , __LINE__, rv, strerror(errno));
        goto fail;
    }

fail:
    close(fd);
    return rv;
}

int32_t dfd_i2c_gen_read(char *i2c_path, uint32_t dev_addr, uint32_t addr_bitwidth,
                uint32_t offset_addr, uint8_t *recv_buf, int32_t rd_len)
{
    int i;
    int rv;

    for (i = 0; i < DFD_I2C_RETRY_TIME; i++) {
        rv = dfd_i2c_gen_read_one_time(i2c_path, dev_addr, addr_bitwidth, offset_addr, recv_buf, rd_len);
        if (rv < 0) {
            DFD_DEBUG_ERROR("(read times:%d) i2c_path:%s, dev_addr:0x%x, addr_bitwidth:%u, offset_addr:0x%x, rd_len:%u\n",
                i, i2c_path, dev_addr, addr_bitwidth, offset_addr, rd_len);
            usleep(DFD_I2C_RETRY_SLEEP_TIME);
            continue;
        }
        break;
    }

    return rv;
}

int dfd_utest_i2c_gen_rd(int argc, char* argv[])
{
    int ret;
    uint32_t i2c_bus, dev_addr, addr_bitwidth, offset_addr, data_bitwidth, rd_len, i, j;
    char *stopstring;
    char i2c_path[32];
    uint8_t tmp_value[DFD_UTEST_MAX_RDWR_NUM];
    uint8_t rd_value[DFD_UTEST_MAX_RDWR_NUM];

    if (argc != 8) {
        DFD_DEBUG_ERROR("params error\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_I2C_GEN_RD);
        goto exit;
    }

    i2c_bus =  strtol(argv[2], &stopstring, 10);
    dev_addr = strtol(argv[3], &stopstring, 16);
    addr_bitwidth = strtol(argv[4], &stopstring, 10);
    offset_addr = strtol(argv[5], &stopstring, 16);
    data_bitwidth = strtol(argv[6], &stopstring, 10);
    rd_len = strtol(argv[7], &stopstring, 10);

    if (rd_len > DFD_UTEST_MAX_RDWR_NUM) {
        DFD_DEBUG_ERROR("Input num %d exceed max.\n", rd_len);
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_I2C_GEN_RD);
        goto exit;
    }

    dfd_utest_print_cmd(argc, argv);
    printf(":\n");
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", i2c_bus);
    mem_clear(tmp_value, sizeof(tmp_value));
    ret = dfd_i2c_gen_read(i2c_path, dev_addr, addr_bitwidth, offset_addr, tmp_value, rd_len);
    if (ret < 0) {
        printf("read failed. ret:%d\n", ret);
        goto exit;
    }

    mem_clear(rd_value, sizeof(rd_value));
    if (data_bitwidth == WIDTH_1Byte) {
        memcpy(rd_value, tmp_value, rd_len);
    } else {
        for (i = 0; i < rd_len; i += data_bitwidth) {
            for (j = 0; (j < data_bitwidth) && (i + j < rd_len); j++) {
                rd_value[i + data_bitwidth - j - 1] = tmp_value[i + j];
            }
        }
    }

    dfd_utest_printf_reg(rd_value, rd_len, offset_addr);

exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int32_t dfd_i2c_gen_write_one_time(char *i2c_path, uint32_t dev_addr, uint32_t addr_bitwidth,
                uint32_t offset_addr, uint8_t *wr_value, uint32_t wr_len)
{
    int32_t fd, rv, i;
    struct i2c_rdwr_ioctl_data ioctl_data;
    struct i2c_msg msgs[1];
    uint8_t buf[DFD_UTEST_MAX_BIT_WIDTH + DFD_UTEST_MAX_RDWR_NUM];

    fd = open(i2c_path, O_RDWR | O_SYNC);
    if (fd < 0) {
        DFD_DEBUG_ERROR("i2c open fail fd %d\n", fd);
        return -1;
    }
    mem_clear(&ioctl_data, sizeof(ioctl_data));
    mem_clear(msgs, sizeof(msgs));
    mem_clear(buf, sizeof(buf));

    i = 0;

    switch (addr_bitwidth) {
    case WIDTH_4Byte:
        buf[i++] = (offset_addr >> 24) & 0xFF;
        buf[i++] = (offset_addr >> 16) & 0xFF;
        buf[i++] = (offset_addr >> 8) & 0xFF;
        buf[i++] = offset_addr & 0xFF;
        break;
    case WIDTH_2Byte:
        buf[i++] = (offset_addr >> 8) & 0xFF;
        buf[i++] = offset_addr & 0xFF;
        break;
    case WIDTH_1Byte:
        buf[i++] = offset_addr & 0xFF;
        break;
    default:
        DFD_DEBUG_ERROR("Only support 1,2,4 Byte Address Width,but set %u addr_bitwidth \r\n", addr_bitwidth);
        rv = -1;
        goto fail;
    }

    memcpy(buf + addr_bitwidth, wr_value, wr_len);

    msgs[0].addr= dev_addr;
    msgs[0].flags = 0;
    msgs[0].len= addr_bitwidth + wr_len;
    msgs[0].buf= buf;

    ioctl_data.nmsgs= 1;
    ioctl_data.msgs= msgs;

    rv = ioctl(fd, I2C_RDWR, &ioctl_data);
    if(rv < 0) {
        DFD_DEBUG_ERROR("%s %dError: Sending messages failed:(ret:%d, errno:%s)!\n", __func__ , __LINE__, rv, strerror(errno));
        goto fail;
    } else if (rv < ioctl_data.nmsgs) {
        DFD_DEBUG_ERROR("%s %dWarning: only %d/%d messages were sent\n", __func__ , __LINE__, rv, ioctl_data.nmsgs);
    }

fail:
    close(fd);
    return rv;
}

int32_t dfd_i2c_gen_write(char *i2c_path, uint32_t dev_addr, uint32_t addr_bitwidth,
                uint32_t offset_addr, uint8_t *wr_value, uint32_t wr_len)
{
    int i;
    int rv;

    for (i = 0; i < DFD_I2C_RETRY_TIME; i++) {
        rv = dfd_i2c_gen_write_one_time(i2c_path, dev_addr, addr_bitwidth, offset_addr, wr_value, wr_len);
        if (rv < 0) {
            DFD_DEBUG_ERROR("(write times:%d)i2c_path:%s, dev_addr:0x%x, addr_bitwidth:%u, offset_addr:0x%x, wr_len:%u\n",
                i, i2c_path, dev_addr, addr_bitwidth, offset_addr, wr_len);
            usleep(DFD_I2C_RETRY_SLEEP_TIME);
            continue;
        }
        break;
    }

    return rv;
}

int dfd_utest_i2c_gen_wr(int argc, char* argv[])
{
    int ret;
    uint32_t i2c_bus, dev_addr, addr_bitwidth, offset_addr, data_bitwidth, wr_len, tmp_data, para_len, i, j;
    char *stopstring;
    char i2c_path[32];
    uint8_t wr_value[DFD_UTEST_MAX_RDWR_NUM];

    if (argc < 8) {
        DFD_DEBUG_ERROR("Input invalid.\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_I2C_GEN_WR);
        goto exit;
    }

    i2c_bus = strtol(argv[2], &stopstring, 10);
    dev_addr = strtol(argv[3], &stopstring, 16);
    addr_bitwidth = strtol(argv[4], &stopstring, 10);
    offset_addr = strtol(argv[5], &stopstring, 16);
    data_bitwidth = strtol(argv[6], &stopstring, 10);

    para_len = argc - 7;
    wr_len = para_len * data_bitwidth;

    if (wr_len > DFD_UTEST_MAX_RDWR_NUM) {
        DFD_DEBUG_ERROR("Input num %d exceed max.\n", wr_len);
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_I2C_GEN_WR);
        goto exit;
    }

    if (data_bitwidth == WIDTH_1Byte) {
        for (i = 0; i < para_len; i++) {
            wr_value[i] = strtol(argv[7 + i], &stopstring, 16);
            DFD_DEBUG_DBG(" index :%d value 0x%x\n", i , wr_value[i]);
        }
    } else {
        for (i = 0; i < para_len; i++) {
            tmp_data = strtol(argv[7 + i], &stopstring, 16);
            DFD_DEBUG_DBG(" index :%d value 0x%x\n", i , tmp_data);
            for (j = 0; j < data_bitwidth; j++) {
                tmp_data = strtol(argv[7 + i], &stopstring, 16);
                wr_value[j + i * data_bitwidth] = (tmp_data >> (24 - 8 * j)) & 0xFF;
            }
        }
    }

    dfd_utest_print_cmd(argc, argv);

    printf(":\n");
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", i2c_bus);

    ret = dfd_i2c_gen_write(i2c_path, dev_addr, addr_bitwidth, offset_addr, wr_value, wr_len);
    if (ret < 0) {
        printf("write failed. ret:%d\n", ret);
    } else {
        printf("write success\n");
    }
exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int dfd_utest_i2c_rd(int argc, char* argv[])
{
    int ret;
    uint8_t value[DFD_UTEST_MAX_RDWR_NUM];
    uint16_t dev_addr, offset_addr;
    char *stopstring;
    int num, i2c_bus;
    char i2c_path[32];

    if (argc != 6) {
        DFD_DEBUG_ERROR("params error\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_I2C_RD);
        goto exit;
    }

    i2c_bus =  strtol(argv[2], &stopstring, 10);
    dev_addr = strtol(argv[3], &stopstring, 16);
    offset_addr = strtol(argv[4], &stopstring, 16);
    num = strtol(argv[5], &stopstring, 10);

    if (num > DFD_UTEST_MAX_RDWR_NUM) {
        DFD_DEBUG_ERROR("Input num %d exceed max.\n", num);
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_I2C_RD);
        goto exit;
    }

    dfd_utest_print_cmd(argc, argv);
    printf(":\n");
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", i2c_bus);
    mem_clear(value, sizeof(value));
    ret = dfd_read_port_i2c(i2c_path, dev_addr, offset_addr, value, num);
    if (ret < 0) {
        printf("failed ret %d\n", ret);
        goto exit;
    }

    dfd_utest_printf_reg(value, num, offset_addr);

exit:
    return DFD_RV_MODE_NOTSUPPORT;

}

int dfd_utest_i2c_wr(int argc, char* argv[])
{
    int ret;
    uint16_t dev_addr, offset_addr;
    char *stopstring;
    int i2c_bus;
    char i2c_path[32];
    uint8_t wr_len,i;
    uint8_t wr_value[DFD_UTEST_MAX_RDWR_NUM];

    if (argc < 6) {
        DFD_DEBUG_ERROR("Input invalid.\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_I2C_WR);
        goto exit;
    }

    wr_len = argc - 5;
    i2c_bus = strtol(argv[2], &stopstring, 10);
    dev_addr = strtol(argv[3], &stopstring, 16);
    offset_addr = strtol(argv[4], &stopstring, 16);

    for (i = 0; i < wr_len; i++) {
        wr_value[i] = strtol(argv[5+i], &stopstring, 16);
        DFD_DEBUG_DBG(" index :%d value %x\n", i , wr_value[i]);
    }

    dfd_utest_print_cmd(argc, argv);

    printf(":\n");
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", i2c_bus);

    ret = dfd_write_port_i2c(i2c_path, dev_addr, offset_addr, wr_value, wr_len);
    if (ret < 0) {
        printf("failed ret %d\n", ret);
    } else {
        printf("success\n");
    }
exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int dfd_utest_io_rd(int argc, char* argv[])
{
    int ret;
    uint8_t value[DFD_UTEST_MAX_RDWR_NUM];
    uint16_t offset_addr;
    char *stopstring;
    int num;

    if (argc != 4) {
        DFD_DEBUG_ERROR("params error\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_IO_RD);
        goto exit;
    }

    offset_addr = strtol(argv[2], &stopstring, 16);
    num = strtol(argv[3], &stopstring, 10);

    if (num > DFD_UTEST_MAX_RDWR_NUM) {
        DFD_DEBUG_ERROR("Input num %d exceed max.\n", num);
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_IO_RD);
        goto exit;
    }

    dfd_utest_print_cmd(argc, argv);
    printf(":\n");
    mem_clear(value, sizeof(value));
    ret = dfd_read_io_port(offset_addr, value, num);
    if (ret < 0) {
        printf("failed ret %d\n", ret);
        goto exit;
    }

    dfd_utest_printf_reg(value, num, offset_addr);

exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int dfd_utest_io_wr(int argc, char* argv[])
{
    int ret;
    uint16_t offset_addr;
    char *stopstring;
    int32_t wr_len,i;
    uint8_t wr_value[DFD_UTEST_MAX_RDWR_NUM];

    if (argc < 4) {
        DFD_DEBUG_ERROR("Input invalid.\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_IO_WR);
        goto exit;
    }

    wr_len = argc - 3;
    if (wr_len > DFD_UTEST_MAX_RDWR_NUM) {
        DFD_DEBUG_ERROR("Input num %d exceed max.\n", wr_len);
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_IO_WR);
        goto exit;
    }

    offset_addr = strtol(argv[2], &stopstring, 16);

    for (i = 0; i < wr_len; i++) {
        wr_value[i] = strtol(argv[3 + i], &stopstring, 16);
        DFD_DEBUG_DBG(" index :%d value %x\n", i , wr_value[i]);
    }

    dfd_utest_print_cmd(argc, argv);

    printf(":\n");
    ret = dfd_write_io_port(offset_addr, wr_value, wr_len);
    if (ret < 0) {
        printf("failed ret %d\n", ret);
    } else {
        printf("success\n");
    }
exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int dfd_utest_phymem_rd(int argc, char* argv[])
{
    int ret, width;
    uint8_t value[DFD_UTEST_MAX_RDWR_NUM];
    off_t offset_addr;
    char *stopstring;
    int num;

    if (argc != 5) {
        DFD_DEBUG_ERROR("params error\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_PHYMEM_RD);
        goto exit;
    }

    width = strtol(argv[2], &stopstring, 10);
    offset_addr = strtol(argv[3], &stopstring, 16);
    num = strtol(argv[4], &stopstring, 10);

    if (num > DFD_UTEST_MAX_RDWR_NUM) {
        DFD_DEBUG_ERROR("Input num %d exceed max.\n", num);
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_PHYMEM_RD);
        goto exit;
    }

    dfd_utest_print_cmd(argc, argv);
    printf(":\n");
    mem_clear(value, sizeof(value));
    ret = dfd_process_mem(DEV_MEM_NAME, 0, width, offset_addr, value, num);
    if (ret < 0) {
        printf("failed ret %d\n", ret);
        goto exit;
    }

    dfd_utest_printf_reg(value, num, offset_addr);

exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int dfd_utest_phymem_wr(int argc, char* argv[])
{
    int ret, width;
    off_t offset_addr;
    char *stopstring;
    int32_t wr_len,i;
    uint8_t wr_value[DFD_UTEST_MAX_RDWR_NUM];

    if (argc < 5) {
        DFD_DEBUG_ERROR("Input invalid.\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_PHYMEM_WR);
        goto exit;
    }

    wr_len = argc - 4;
    if (wr_len > DFD_UTEST_MAX_RDWR_NUM) {
        DFD_DEBUG_ERROR("Input num %d exceed max.\n", wr_len);
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_PHYMEM_WR);
        goto exit;
    }

    width = strtol(argv[2], &stopstring, 10);
    offset_addr = strtol(argv[3], &stopstring, 16);

    for (i = 0; i < wr_len; i++) {
        wr_value[i] = strtol(argv[4 + i], &stopstring, 16);
        DFD_DEBUG_DBG(" index :%d value %x\n", i , wr_value[i]);
    }

    dfd_utest_print_cmd(argc, argv);

    printf(":\n");
    ret = dfd_process_mem(DEV_MEM_NAME, 1, width, offset_addr, wr_value, wr_len);
    if (ret < 0) {
        printf("failed ret %d\n", ret);
    } else {
        printf("success\n");
    }
exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int dfd_utest_kmem_rd(int argc, char* argv[])
{
    int ret, width;
    uint8_t value[DFD_UTEST_MAX_RDWR_NUM];
    uint16_t offset_addr;
    char *stopstring;
    int num;

    if (argc != 5) {
        DFD_DEBUG_ERROR("params error\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_KMEM_RD);
        goto exit;
    }

    width = strtol(argv[2], &stopstring, 10);
    offset_addr = strtol(argv[3], &stopstring, 16);
    num = strtol(argv[4], &stopstring, 10);

    if (num > DFD_UTEST_MAX_RDWR_NUM) {
        DFD_DEBUG_ERROR("Input num %d exceed max.\n", num);
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_KMEM_RD);
        goto exit;
    }

    dfd_utest_print_cmd(argc, argv);
    printf(":\n");
    mem_clear(value, sizeof(value));
    ret = dfd_process_mem(DEV_KMEM_NAME, 0, width, offset_addr, value, num);
    if (ret < 0) {
        printf("failed ret %d\n", ret);
        goto exit;
    }

    dfd_utest_printf_reg(value, num, offset_addr);

exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int dfd_utest_kmem_wr(int argc, char* argv[])
{
    int ret;
    uint16_t offset_addr, width;
    char *stopstring;
    int32_t wr_len,i;
    uint8_t wr_value[DFD_UTEST_MAX_RDWR_NUM];

    if (argc < 5) {
        DFD_DEBUG_ERROR("Input invalid.\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_KMEM_WR);
        goto exit;
    }

    wr_len = argc - 4;
    if (wr_len > DFD_UTEST_MAX_RDWR_NUM) {
        DFD_DEBUG_ERROR("Input num %d exceed max.\n", wr_len);
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_KMEM_WR);
        goto exit;
    }

    width = strtol(argv[2], &stopstring, 10);
    offset_addr = strtol(argv[3], &stopstring, 16);

    for (i = 0; i < wr_len; i++) {
        wr_value[i] = strtol(argv[4 + i], &stopstring, 16);
        DFD_DEBUG_DBG(" index :%d value %x\n", i , wr_value[i]);
    }

    dfd_utest_print_cmd(argc, argv);

    printf(":\n");
    ret = dfd_process_mem(DEV_KMEM_NAME, 1, width, offset_addr, wr_value, wr_len);
    if (ret < 0) {
        printf("failed ret %d\n", ret);
    } else {
        printf("success\n");
    }
exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

static unsigned long dfd_utest_get_file_size(const char *path)
{
    unsigned long filesize;
    struct stat statbuff;

    if (stat(path, &statbuff) < 0) {
        filesize = -1;
    } else {
        filesize = statbuff.st_size;
    }

    return filesize;
}

int dfd_utest_i2c_file_wr(int argc, char* argv[])
{
    int ret;
    uint16_t dev_addr, offset_addr;
    char *stopstring;
    int i2c_bus;
    char i2c_path[32];
    char *file_name;
    unsigned long filesize;
    int fd;
    uint8_t wr_buf[DFD_UTEST_MAX_RDWR_NUM];
    int len;
    int bpt;    /* byte per times*/
    int page_left;

    if (argc != 7) {
        printf("Input invalid.\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_I2C_FILE_WR);
        goto exit;
    }

    i2c_bus = strtol(argv[2], &stopstring, 10);
    dev_addr = strtol(argv[3], &stopstring, 16);
    offset_addr = strtol(argv[4], &stopstring, 16);
    bpt = strtol(argv[5], &stopstring, 10);
    file_name = argv[6];

    if ((bpt <= 0) || (bpt > DFD_UTEST_MAX_RDWR_NUM)) {
        bpt = DFD_UTEST_MAX_RDWR_NUM;
    }

    if ((bpt & (bpt - 1)) != 0) {
        printf("Bytes per times %d isn't power of two.\n",bpt);
        goto exit;
    }

    filesize = dfd_utest_get_file_size(file_name);
    if (filesize <= 0) {
        printf("Input invalid file %s, filesize %lu.\n", file_name, filesize);
        goto exit;
    }

    fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        printf("open file[%s] fail.\n", file_name);
        goto exit;
    }

    dfd_utest_print_cmd(argc, argv);

    printf(":\n");
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", i2c_bus);

    while (filesize > 0) {
        mem_clear(wr_buf, DFD_UTEST_MAX_RDWR_NUM);
        len = bpt;
        if (offset_addr & (bpt - 1)) {
            page_left = bpt - (offset_addr & (bpt - 1));
            len  = len  > page_left ? page_left : len;
        }

        len = read(fd, wr_buf, len);

        ret = dfd_write_port_i2c(i2c_path, dev_addr, offset_addr, wr_buf, len);
        if (ret < 0) {
            break;
        }
        offset_addr += len;
        filesize -= len;
    }

    close(fd);

    if (ret < 0) {
        printf("failed ret %d\n", ret);
    } else {
        printf("success\n");
    }

exit:
    return DFD_RV_MODE_NOTSUPPORT;

}

/* compare with sys_flie_wr, One more step is read back verification */
int dfd_utest_sysfs_file_upg(int argc, char* argv[])
{
    int ret = 0;
    uint32_t offset_addr;
    char *file_name;
    char *sysfs_loc;
    char *stopstring;
    unsigned long filesize;
    int fd, file_fd;
    uint8_t wr_buf[DFD_UTEST_MAX_RDWR_NUM];
    int len, write_len, per_wr_len;
    int i;
    uint8_t reread_buf[DFD_UTEST_MAX_RDWR_NUM];
    int reback_len, reread_len;
    int j = 0;

    if (argc != 5 && argc != 6) {
        printf("Input invalid.\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_SYSFS_FILE_UPG);
        goto exit;
    }

    sysfs_loc = argv[2];
    offset_addr = strtol(argv[3], &stopstring, 16);
    file_name = argv[4];

    if (argc == 6) {
        per_wr_len = strtol(argv[5], &stopstring, 10);
        if (per_wr_len > DFD_UTEST_MAX_RDWR_NUM || per_wr_len <= 0) {
            printf("per_wr_byte %d invalid, not in range (0, 256]\n", per_wr_len);
            goto exit;
        }
    } else {
        per_wr_len = DFD_UTEST_DEFAULT_WR_NUM;
    }
    DFD_DEBUG_DBG("per_wr_byte: %d\n", per_wr_len);
    filesize = dfd_utest_get_file_size(file_name);
    if (filesize <= 0) {
        printf("Input invalid file %s, filesize %lu.\n", file_name, filesize);
        goto exit;
    }

    fd = open(sysfs_loc, O_RDWR | O_SYNC);
    if (fd < 0) {
        printf("open file[%s] fail.\n", sysfs_loc);
        goto exit;
    }

    file_fd = open(file_name, O_RDONLY);
    if (file_fd < 0) {
        printf("open file[%s] fail.\n", file_name);
        goto open_dev_err;
    }

    dfd_utest_print_cmd(argc, argv);

    ret = lseek(fd, offset_addr, SEEK_SET);
    if (ret < 0) {
        printf("lseek file[%s offset=%d] fail,\n", sysfs_loc, offset_addr);
        goto fail;
    }

    printf(":\n");
    while (filesize > 0) {
        if (filesize > (unsigned long)per_wr_len) {
            len = per_wr_len;
        } else {
            len = filesize;
        }

        mem_clear(wr_buf, DFD_UTEST_MAX_RDWR_NUM);
        for (i = 0; i < DFD_I2C_RETRY_TIME; i++) {
            len = read(file_fd, wr_buf, len);
            if (len < 0) {
                DFD_DEBUG_ERROR("read file[%s] fail, offset = 0x%x retrytimes = %d ret = %d\n",
                    sysfs_loc, offset_addr, i ,len);
                usleep(DFD_I2C_RETRY_SLEEP_TIME);
                continue;
            }
            break;
        }
        if (i == DFD_I2C_RETRY_TIME) {
            printf("read file[%s] fail, offset = 0x%x, ret = %d\n", sysfs_loc, offset_addr, len);
            goto fail;
        }

        for (i = 0; i < DFD_I2C_RETRY_TIME; i++) {
            write_len = write(fd, wr_buf, len);
            if (write_len != len) {
                DFD_DEBUG_ERROR("write file[%s] fail,offset = 0x%x retrytimes = %d len = %d,write_len =%d\n",
                    sysfs_loc, offset_addr, i ,len, write_len);
                usleep(DFD_I2C_RETRY_SLEEP_TIME);
                continue;
            }
            break;
        }
        if (i == DFD_I2C_RETRY_TIME) {
            printf("write file[%s] fail, offset = 0x%x, len = %d,write_len =%d\n",
                sysfs_loc, offset_addr, len, write_len);
            goto fail;
        }

        reback_len = write_len;
        ret = lseek(fd, -reback_len, SEEK_CUR);
        if (ret < 0) {
            printf("reread lseek file[%s offset=%d] fail,lseek len=%d\n",
                sysfs_loc, offset_addr, reback_len);
            goto fail;
        }

        mem_clear(reread_buf, DFD_UTEST_MAX_RDWR_NUM);
        for (i = 0; i < DFD_I2C_RETRY_TIME; i++) {
            reread_len = read(fd, reread_buf, reback_len);
            if (reread_len != reback_len) {
                DFD_DEBUG_ERROR("reread file[%s] fail,offset = 0x%x retrytimes = %d reread_len = %d,reback_len =%d\n",
                    sysfs_loc, offset_addr, i ,reread_len, reback_len);
                usleep(DFD_I2C_RETRY_SLEEP_TIME);
                continue;
            }
            break;
        }
        if (i == DFD_I2C_RETRY_TIME) {
            printf("reread file[%s] fail, offset = 0x%x, reread_len = %d,reback_len = %d\n",
                sysfs_loc, offset_addr, reread_len, reback_len);
            goto fail;
        }

        if (memcmp(reread_buf, wr_buf, reread_len) != 0) {
            if (j < DFD_I2C_RETRY_TIME) {
                DFD_DEBUG_ERROR("memcmp file[%s] fail,offset = 0x%x retrytimes = %d\n",
                                    sysfs_loc, offset_addr, j);
                j++;
                ret = lseek(file_fd, -len, SEEK_CUR);
                if (ret < 0) {
                    printf("retry file_fd lseek fail,lseek len=%d\n", len);
                    goto fail;
                }
                ret = lseek(fd, -write_len, SEEK_CUR);
                if (ret < 0) {
                    printf("retry fd lseek fail,lseek len=%d\n", write_len);
                    goto fail;
                }
                continue;
            }

            printf("upgrade file[%s] fail, offset = 0x%x.\n", sysfs_loc, offset_addr);
            printf("want to write buf :\n");
            for (i = 0; i < reread_len; i++) {
                printf("0x%x ", wr_buf[i]);
            }
            printf("\n");

            printf("actually reread buf :\n");
            for (i = 0; i < reread_len; i++) {
                printf("0x%x ", reread_buf[i]);
            }
            printf("\n");

            goto fail;
        }

        offset_addr += len;
        filesize -= len;
        usleep(5000);
    }

    printf("success\n");
    close(file_fd);
    close(fd);
    return DFD_RV_OK;

fail:
    close(file_fd);
open_dev_err:
    close(fd);
exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int dfd_utest_sysfs_file_wr(int argc, char* argv[])
{
    int ret = 0;
    uint32_t offset_addr;
    char *file_name;
    char *sysfs_loc;
    char *stopstring;
    unsigned long filesize;
    int fd, file_fd;
    uint8_t wr_buf[DFD_UTEST_MAX_RDWR_NUM];
    int len, write_len, per_wr_len;
    int i;

    if (argc != 5 && argc != 6) {
        printf("Input invalid.\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_SYSFS_FILE_WR);
        goto exit;
    }

    sysfs_loc = argv[2];
    offset_addr = strtol(argv[3], &stopstring, 16);
    file_name = argv[4];

    if (argc == 6) {
        per_wr_len = strtol(argv[5], &stopstring, 10);
        if (per_wr_len > DFD_UTEST_MAX_RDWR_NUM || per_wr_len <= 0) {
            printf("per_wr_byte %d invalid, not in range (0, 256]\n", per_wr_len);
            goto exit;
        }
    } else {
        per_wr_len = DFD_UTEST_DEFAULT_WR_NUM;
    }
    DFD_DEBUG_DBG("per_wr_byte: %d\n", per_wr_len);
    filesize = dfd_utest_get_file_size(file_name);
    if (filesize <= 0) {
        printf("Input invalid file %s, filesize %lu.\n", file_name, filesize);
        goto exit;
    }

    fd = open(sysfs_loc, O_RDWR | O_SYNC);
    if (fd < 0) {
        printf("open file[%s] fail.\n", sysfs_loc);
        goto exit;
    }

    file_fd = open(file_name, O_RDONLY);
    if (file_fd < 0) {
        printf("open file[%s] fail.\n", file_name);
        goto open_dev_err;
    }

    dfd_utest_print_cmd(argc, argv);

    ret = lseek(fd, offset_addr, SEEK_SET);
    if (ret < 0) {
        printf("lseek file[%s offset=%d] fail,\n", sysfs_loc, offset_addr);
        goto fail;
    }

    printf(":\n");
    while (filesize > 0) {
        if (filesize > (unsigned long)per_wr_len) {
            len = per_wr_len;
        } else {
            len = filesize;
        }

        mem_clear(wr_buf, DFD_UTEST_MAX_RDWR_NUM);
        for (i = 0; i < DFD_I2C_RETRY_TIME; i++) {
            len = read(file_fd, wr_buf, len);
            if (len < 0) {
                DFD_DEBUG_ERROR("read file[%s] fail, offset = 0x%x retrytimes = %d ret = %d\n",
                    sysfs_loc, offset_addr, i ,len);
                usleep(DFD_I2C_RETRY_SLEEP_TIME);
                continue;
            }
            break;
        }
        if (i == DFD_I2C_RETRY_TIME) {
            printf("read file[%s] fail, offset = 0x%x, ret = %d\n", sysfs_loc, offset_addr, len);
            goto fail;
        }
        for (i = 0; i < DFD_I2C_RETRY_TIME; i++) {
            write_len = write(fd, wr_buf, len);
            if (write_len != len) {
                DFD_DEBUG_ERROR("write file[%s] fail,offset = 0x%x retrytimes = %d len = %d,write_len =%d\n", sysfs_loc, offset_addr, i ,len, write_len);
                usleep(DFD_I2C_RETRY_SLEEP_TIME);
                continue;
            }
            break;
        }

        if(i == DFD_I2C_RETRY_TIME) {
            printf("write file[%s] fail, offset = 0x%x, len = %d,write_len =%d\n", sysfs_loc, offset_addr, len, write_len);
            ret = -1;
            goto fail;
        }
        offset_addr += len;
        filesize -= len;
        usleep(5000);
    }

    printf("success\n");
    close(file_fd);
    close(fd);
    return DFD_RV_OK;

fail:
    close(file_fd);
open_dev_err:
    close(fd);
exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int dfd_utest_sysfs_file_rd(int argc, char* argv[])
{
    int ret = 0;
    uint32_t offset_addr;
    char *sysfs_loc;
    char *stopstring;
    int fd;
    uint8_t rd_buf[DFD_UTEST_MAX_RDWR_NUM];
    int len, read_len;;

    if (argc != 5) {
        printf("Input invalid.\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_SYSFS_FILE_RD);
        goto exit;
    }

    sysfs_loc = argv[2];
    offset_addr = strtol(argv[3], &stopstring, 16);
    len = strtol(argv[4], &stopstring, 10);

    if (len > DFD_UTEST_MAX_RDWR_NUM) {
        printf("Input num %d exceed max 256.\n", len);
        goto exit;
    }

    fd = open(sysfs_loc, O_RDONLY);
    if (fd < 0) {
        printf("open file[%s] fail.\n", sysfs_loc);
        goto exit;
    }
    dfd_utest_print_cmd(argc, argv);

    printf(":\n");

    ret = lseek(fd, offset_addr, SEEK_SET);
    if (ret < 0) {
        printf("lseek failed ret %d.\n", ret);
        goto fail;
    }

    mem_clear(rd_buf, DFD_UTEST_MAX_RDWR_NUM);
    read_len = read(fd, rd_buf, len);
    if (read_len != len) {
        printf("read failed read_len %d len %d.\n", read_len, len);
        goto fail;
    }
    dfd_utest_printf_reg(rd_buf, read_len, offset_addr);
    close(fd);
    return DFD_RV_OK;

fail:
    close(fd);
exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int dfd_utest_msr_rd(int argc, char* argv[])
{
    int fd;
    char msr_file_name[64];
    uint64_t data;
    uint64_t read_result;
    char *stopstring;
    uint8_t cpu_index, width;
    uint64_t offset;

    if (argc != 5) {
        printf("rdmsr failed: Input invalid.\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_MSR_RD);
        goto exit;
    }

    cpu_index = strtol(argv[2], &stopstring, 10);
    offset = strtol(argv[3], &stopstring, 16);
    width = strtol(argv[4], &stopstring, 10);

    if (width != 8 && width != 16 && width != 32 && width != 64) {
        printf("rdmsr failed: width:%u Input invalid.only support 8 16 32 64\n", width);
        goto exit;
    }

    mem_clear(msr_file_name, sizeof(msr_file_name));
    sprintf(msr_file_name, "/dev/cpu/%u/msr", cpu_index);

    fd = open(msr_file_name, O_RDONLY);
    if (fd < 0) {
        if (errno == ENXIO) {
            fprintf(stderr, "rdmsr failed: No CPU %u\n", cpu_index);
        } else if (errno == EIO) {
            fprintf(stderr, "rdmsr failed: CPU %u doesn't support MSRs\n", cpu_index);
        } else if (errno == ENOENT) {
            fprintf(stderr, "rdmsr failed: can't find %s file, Please check if modprobe msr driver already\n", msr_file_name);
        } else {
            printf("rdmsr failed: %s open failed. errno:%d\n", msr_file_name, errno);
        }
        goto exit;
    }

    if (pread(fd, &data, sizeof(data), offset) != sizeof(data)) {
        fprintf(stderr, "rdmsr failed: CPU:%u offset:0x%lx read failed\n", cpu_index, offset);
        goto fail;
    }

    switch (width) {
        case 8:
            read_result = (volatile uint8_t)data;
            break;
        case 16:
            read_result = (volatile uint16_t)data;
            break;
        case 32:
            read_result = (volatile uint32_t)data;
            break;
        case 64:
            read_result = (volatile uint64_t)data;
            break;
        default:
            printf("rdmsr failed: width:%u illegal width.\n", width);
            goto fail;
    }

    printf("0x%lx\n", read_result);
    close(fd);
    return DFD_RV_OK;

fail:
    close(fd);
exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

int dfd_utest_sysfs_data_wr(int argc, char* argv[])
{
    uint32_t offset;
    char *sysfs_loc;
    char *stopstring;
    uint8_t wr_buf[DFD_UTEST_MAX_RDWR_NUM];
    int ret, i;
    int fd, len, write_len, index;

    if (argc < 5) {
        DFD_DEBUG_ERROR("Input invalid.\n");
        dfd_utest_printf_single_help(DFD_UTEST_ITEM_SYSFS_DATA_WR);
        goto exit;
    }

    dfd_utest_print_cmd(argc, argv);
    printf(":\n");

    sysfs_loc = argv[2];
    offset = strtol(argv[3], &stopstring, 16);
    len = argc - 4;
    mem_clear(wr_buf, sizeof(wr_buf));
    for (i = 0; i < len; i++) {
        wr_buf[i] = strtol(argv[4 + i], &stopstring, 16);
        DFD_DEBUG_DBG("index :%d value %x\n", i , wr_buf[i]);
    }

    fd = open(sysfs_loc, O_RDWR | O_SYNC);
    if (fd < 0) {
        printf("open file[%s] fail.\n", sysfs_loc);
        goto exit;
    }

    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek file[%s offset=%d] fail,\n", sysfs_loc, offset);
        goto fail;
    }
    index = 0;
    while (len > 0) {
        for (i = 0; i < DFD_I2C_RETRY_TIME; i++) {
            write_len = write(fd, &wr_buf[index], len);
            if (write_len < 0) {
                DFD_DEBUG_ERROR("write file[%s] fail, retrytimes: %d, offset: 0x%x, len: %d, write_len: %d\n",
                    sysfs_loc, offset, i, len, write_len);
                usleep(DFD_I2C_RETRY_SLEEP_TIME);
                continue;
            }
            if (write_len == 0) {
                DFD_DEBUG_ERROR("write file[%s] EOF, offset: 0x%x, len: %d, write_len: %d\n",
                    sysfs_loc, offset, len, write_len);
                goto fail;
            }
            break;
        }
        if(i == DFD_I2C_RETRY_TIME) {
            printf("write file[%s] fail, offset: 0x%x, len: %d, write_len: %d\n",
                sysfs_loc, offset, len, write_len);
            goto fail;
        }
        offset += write_len;
        index += write_len;
        len -= write_len;
        usleep(5000);
    }
    printf("success\n");
    close(fd);
    return DFD_RV_OK;
fail:
    close(fd);
exit:
    return DFD_RV_MODE_NOTSUPPORT;
}

dfd_utest_proc_fun dfd_utest_get_proc_func(char *type_str)
{
    int i, tbl_size;

    tbl_size = sizeof(g_dfd_unit_test) / sizeof(g_dfd_unit_test[0]);

    for (i = 0; i < tbl_size; i++) {
        if (!strncmp(g_dfd_unit_test[i].type_str, type_str, strlen(g_dfd_unit_test[i].type_str))) {
            return g_dfd_unit_test[i].utest_func;
        }
    }
    DFD_DEBUG_DBG("type: %s not match.\n", type_str);
    return NULL;
}

void dfd_utest_cmd_main(int argc, char* argv[])
{
    dfd_utest_proc_fun pfunc;
    int ret;

    if (argc < 2) {
        dfd_utest_print_all_help();
        return;
    }

    pfunc = dfd_utest_get_proc_func(argv[1]);
    if (pfunc == NULL) {
        DFD_DEBUG_DBG("utest type %s in not support.\n", argv[1]);
        dfd_utest_print_all_help();
        return;
    }
    ret = pfunc(argc, argv);
    if ((ret != DFD_RV_MODE_NOTSUPPORT) && (ret != DFD_RV_INDEX_INVALID)) {
        if (ret == DFD_RV_OK) {
            DFD_DEBUG_DBG(" [SUCCESS]\n");
        } else {
            DFD_DEBUG_DBG(" [FAIL(%d)]\n", ret);
        }
    }

    return;
}
