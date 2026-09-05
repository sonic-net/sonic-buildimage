#ifndef __DFD_CFG_ADAPTER_H__
#define __DFD_CFG_ADAPTER_H__

#define DFD_KO_CPLD_I2C_RETRY_SLEEP            (10)  /* ms */
#define DFD_KO_CPLD_I2C_RETRY_TIMES            (50 / DFD_KO_CPLD_I2C_RETRY_SLEEP)

#define DFD_KO_CPLD_GET_SLOT(addr)             ((addr >> 24) & 0xff)
#define DFD_KO_CPLD_GET_ID(addr)               ((addr >> 16) & 0xff)
#define DFD_KO_CPLD_GET_INDEX(addr)            (addr & 0xffff)
#define DFD_KO_CPLD_MODE_I2C_STRING            "i2c"
#define DFD_KO_CPLD_MODE_LPC_STRING            "lpc"

typedef struct dfd_i2c_dev_s {
    int bus;
    int addr;
} dfd_i2c_dev_t;

typedef enum dfd_i2c_dev_mem_s {
    DFD_I2C_DEV_MEM_BUS,
    DFD_I2C_DEV_MEM_ADDR,
    DFD_I2C_DEV_MEM_END
} dfd_i2c_dev_mem_t;

typedef enum cpld_mode_e {
    DFD_CPLD_MODE_I2C,
    DFD_CPLD_MODE_LPC,
} cpld_mode_t;

typedef enum i2c_mode_e {
    DFD_I2C_MODE_NORMAL_I2C,
    DFD_I2C_MODE_SMBUS,
} i2c_mode_t;

extern char *g_dfd_i2c_dev_mem_str[DFD_I2C_DEV_MEM_END];

int32_t dfd_ko_cpld_read(int32_t addr, uint8_t *buf);

int32_t dfd_ko_cpld_write(int32_t addr, uint8_t val);

int32_t dfd_ko_i2c_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size);

int32_t dfd_ko_i2c_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size);

int32_t dfd_ko_read_file(char *fpath, int32_t addr, uint8_t *val, int32_t read_bytes);

#endif /* __DFD_CFG_ADAPTER_H__ */
