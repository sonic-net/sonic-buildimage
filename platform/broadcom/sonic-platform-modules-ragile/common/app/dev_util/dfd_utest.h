/* monitor_utest.h */
#ifndef __DFD_UTEST_H__
#define __DFD_UTEST_H__

#include <string.h>

extern int g_dfd_debug_sw;
extern int g_dfd_debugpp_sw;

#define DFD_UTEST_TRUE_FALSE_STRING(flag)                   ((flag == true) ? "true" : "false")

#define DFD_DEBUG_DBG(fmt, args...) do {                     \
    if (g_dfd_debug_sw) {               \
        printf("" fmt,\
            ##args);  \
    }                                                   \
} while (0)

#define DFD_DEBUG_ERROR(fmt, args...) do {                     \
    if (g_dfd_debugpp_sw) {               \
        printf("" fmt,\
            ##args);  \
    }                                                   \
} while (0)

#define mem_clear(data, size) memset((data), 0, (size))

typedef enum dfd_rv_s {
    DFD_RV_OK               = 0,
    DFD_RV_INIT_ERR         = 1,
    DFD_RV_SLOT_INVALID     = 2,
    DFD_RV_MODE_INVALID     = 3,
    DFD_RV_MODE_NOTSUPPORT  = 4,
    DFD_RV_TYPE_ERR         = 5,
    DFD_RV_DEV_NOTSUPPORT   = 6,
    DFD_RV_DEV_FAIL         = 7,
    DFD_RV_INDEX_INVALID    = 8,
    DFD_RV_NO_INTF          = 9,
    DFD_RV_NO_NODE          = 10,
    DFD_RV_NODE_FAIL        = 11,
} dfd_rv_t;

#define DFD_DEBUG_BUF_LEN               (32)
#define DFD_DEBUGP_DEBUG_FILE           "/sbin/.dfd_debugp_flag"
#define DFD_DEBUGPP_DEBUG_FILE          "/sbin/.dfd_debugpp_flag"

#define DFD_UTEST_MAX_PARA_NUM          (4)
#define DFD_UTEST_TYPE_STRING_LEN       (64)
#define DFD_UTEST_MATCH_STRING_LEN      (64)
#define DFD_UTEST_HELP_STRING_LEN       (256)
#define DFD_UTEST_INVALID_PARA          (-1)
#define DFD_UTEST_BUFF_LEN              (64)

typedef enum dfd_fpga_cpld_flag_e {
    DFD_CPLD_RW_FLAG = 0x00,
    DFD_FPGA_RW_FLAG = 0x01,
} dfd_fpga_cpld_flag_t;

typedef int (* dfd_utest_proc_fun)(int argc, char* argv[]);

#define DFD_UTEST_ITEM_ALL \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_I2C_RD, i2c_rd, "i2c_rd [i2c_bus] [slave_addr] [offset] [len]", "i2c_rd [i2c_bus] [slave_addr] [offset] [len]")    \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_I2C_WR, i2c_wr, "i2c_wr [i2c_bus] [slave_addr] [offset] [data0] ... [dataN]", "i2c_wr [i2c_bus] [slave_addr] [offset] [data0] ... [dataN]")   \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_IO_RD, io_rd, "io_rd [offset] [len]", "io_rd [offset] [len]")   \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_IO_WR, io_wr, "io_wr [offset] [data0]... [dataN]", "io_wr [offset] [data0]... [dataN]")   \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_PHYMEM_RD, phymem_rd, "phymem_rd [bit_width] [offset] [len]", "phymem_rd [bit_width] [offset] [len]")   \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_PHYMEM_WR, phymem_wr, "phymem_wr [bit_width] [offset] [data0]... [dataN]", "phymem_wr [bit_width] [offset] [data0]... [dataN]")   \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_KMEM_RD, kmem_rd, "kmem_rd [bit_width] [offset] [len]", "kmem_rd [bit_width] [offset] [len]")   \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_KMEM_WR, kmem_wr, "kmem_wr [bit_width][offset] [data0]... [dataN]", "kmem_wr [bit_width] [offset] [data0]... [dataN]")   \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_I2C_FILE_WR, i2c_file_wr, "i2c_file_wr [i2c_bus] [slave_addr] [offset] [bpt] [filename]", "i2c_file_wr [i2c_bus] [slave_addr] [offset] [bpt] [filename]\nbpt:bytes per times")   \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_SYSFS_FILE_WR, sysfs_file_wr, "sysfs_file_wr [sysfs_loc] [offset] [filename] [per_wr_byte]", "sysfs_file_wr [sysfs_loc] [offset]  [filename] [per_wr_byte]")   \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_SYSFS_FILE_RD, sysfs_file_rd, "sysfs_file_rd [sysfs_loc] [offset]  [len]", "sysfs_file_rd [sysfs_loc] [offset]  [len]")   \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_SYSFS_FILE_UPG, sysfs_file_upg, "sysfs_file_upg [sysfs_loc] [offset] [filename] [per_wr_byte]", "sysfs_file_upg [sysfs_loc] [offset]  [filename] [per_wr_byte]")   \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_I2C_GEN_RD, i2c_gen_rd, "i2c_gen_rd [i2c_bus] [slave_addr] [addr_bitwidth] [offset] [data_bitwidth] [len]", "i2c_gen_rd [i2c_bus] [slave_addr] [addr_bitwidth] [offset] [data_bitwidth] [len]")    \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_I2C_GEN_WR, i2c_gen_wr, "i2c_gen_wr [i2c_bus] [slave_addr] [addr_bitwidth] [offset] [data_bitwidth] [data0]... [dataN]", "i2c_gen_wr [i2c_bus] [slave_addr] [addr_bitwidth] [offset] [data_bitwidth] [data0]... [dataN]")    \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_MSR_RD, msr_rd, "msr_rd [cpu_index] [offset] [width]", "msr_rd [cpu_index] [offset] [width]")    \
    DFD_UTEST_ITEM(DFD_UTEST_ITEM_SYSFS_DATA_WR, sysfs_data_wr, "sysfs_data_wr [sysfs_loc] [offset] [data0] ... [dataN]", "sysfs_data_wr [sysfs_loc] [offset] [data0] ... [dataN]]")   \

#ifdef DFD_UTEST_ITEM
#undef DFD_UTEST_ITEM
#endif
#define DFD_UTEST_ITEM(_id, _type_str, _help_info, _help_info_detail)    _id,
typedef enum dfd_utest_item_id_s {
    DFD_UTEST_ITEM_ALL
} dfd_utest_item_id_t;

typedef struct {
    int utest_type;
    char type_str[DFD_UTEST_TYPE_STRING_LEN];
    dfd_utest_proc_fun utest_func;
    char help_info[DFD_UTEST_HELP_STRING_LEN];
    char help_info_detail[DFD_UTEST_HELP_STRING_LEN];
} dfd_utest_t;

void dfd_utest_cmd_main(int argc, char* argv[]);

#ifdef DFD_UTEST_ITEM
#undef DFD_UTEST_ITEM
#endif
#define DFD_UTEST_ITEM(_id, _type_str, _help_info, _help_info_detail)    int dfd_utest_##_type_str(int argc, char* argv[]);
DFD_UTEST_ITEM_ALL

#endif
