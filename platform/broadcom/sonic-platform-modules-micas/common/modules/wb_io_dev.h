#ifndef __WB_IO_DEV_H__
#define __WB_IO_DEV_H__
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))
#define IO_DEV_NAME_MAX_LEN (64)

typedef struct io_dev_device_s {
    char io_dev_name[IO_DEV_NAME_MAX_LEN];
    uint32_t io_base;
    uint32_t io_len;
    uint32_t indirect_addr;
    uint32_t wr_data;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t rd_data;
    uint32_t opt_ctl;
    int device_flag;
} io_dev_device_t;

#endif
