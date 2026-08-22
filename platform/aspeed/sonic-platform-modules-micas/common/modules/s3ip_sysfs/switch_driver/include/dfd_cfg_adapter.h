#ifndef __DFD_CFG_ADAPTER_H__
#define __DFD_CFG_ADAPTER_H__

#define DFD_KO_CPLD_I2C_RETRY_SLEEP            (10)  /* ms */

#define DFD_KO_CPLD_GET_SLOT(addr)             ((addr >> 24) & 0xff)
#define DFD_KO_CPLD_GET_ID(addr)               ((addr >> 16) & 0xff)
#define DFD_KO_CPLD_GET_INDEX(addr)            (addr & 0xffff)
#define DFD_KO_CPLD_MODE_I2C_STRING            "i2c"
#define DFD_KO_CPLD_MODE_LPC_STRING            "lpc"
#define DFD_KO_CPLD_MODE_LOGIC_DEV_STRING      "logic_dev"

#define DFD_KO_OTHER_I2C_GET_MAIN_ID(addr)     ((addr >> 24) & 0xff)
#define DFD_KO_OTHER_I2C_GET_INDEX(addr)       ((addr >> 16) & 0xff)
#define DFD_KO_OTHER_I2C_GET_OFFSET(addr)      (addr & 0xffff)
#define DFD_SYSFS_PATH_MAX_LEN                 (64)
#define DFD_SMBUS_BLOCK_MAX_LEN                (32)

typedef struct dfd_i2c_dev_s {
    int bus;        /* bus number */
    int addr;       /* Bus address */
} dfd_i2c_dev_t;

/* dfd_i2c_dev_t member macro */
typedef enum dfd_i2c_dev_mem_s {
    DFD_I2C_DEV_MEM_BUS,
    DFD_I2C_DEV_MEM_ADDR,
    DFD_I2C_DEV_MEM_END
} dfd_i2c_dev_mem_t;

typedef enum cpld_mode_e {
    DFD_CPLD_MODE_I2C,          /* I2C bus */
    DFD_CPLD_MODE_LPC,          /* LPC bus */
    DFD_CPLD_MODE_LOGIC_DEV,    /* logic dev */
} cpld_mode_t;

/* i2c access mode */
typedef enum i2c_mode_e {
    DFD_I2C_MODE_NORMAL_I2C,    /* I2C bus */
    DFD_I2C_MODE_SMBUS,         /* SMBUS bus */
} i2c_mode_t;

/* Global variable */
extern char *g_dfd_i2c_dev_mem_str[DFD_I2C_DEV_MEM_END];      /* dfd_i2c_dev_t member string */

/**
 * dfd_ko_cpld_read - cpld read operation
 * @addr: Offset address
 * @buf: data
 *
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_cpld_read(int32_t addr, uint8_t *buf);

/**
 * dfd_ko_cpld_write - cpld write operation
 * @addr: address
 * @data: data
 *
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_cpld_write(int32_t addr, uint8_t val);

/**
 * dfd_ko_i2c_read - I2C read operation
 * @bus: I2C bus
 * @addr: I2C device address
 * @offset:register offset
 * @buf:Read buffer
 * @size:Read length
 * @sysfs_name:sysfs attribute name
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_i2c_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size, const char *sysfs_name);

/**
 * dfd_ko_i2c_write - I2C write operation
 * @bus: I2C bus
 * @addr: I2C device address
 * @offset:register offset
 * @buf:write buffer
 * @size: write length
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_i2c_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size);

/**
 * dfd_ko_read_file - File read operation
 * @fpath: File path
 * @addr: address
 * @val: data
 * @read_bytes: length
 *
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_read_file(char *fpath, int32_t addr, uint8_t *val, int32_t read_bytes);

/**
 * dfd_ko_write_file - File write operation
 * @fpath: File path
 * @addr: address
 * @val:  data
 * @write_bytes: length
 *
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_write_file(char *fpath, int32_t addr, uint8_t *val, int32_t write_bytes);

/**
 * access_file_exist - Check if a file exists
 * @filepath: Path to the file
 *
 * @returns: true if the file exists, false otherwise
 */
bool access_file_exist(const char *filepath);

/**
 * dfd_ko_other_i2c_dev_read - other_i2c read operation
 * @addr: address
 * @val: data
 * @read_len: length
 *
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_other_i2c_dev_read(int32_t addr, uint8_t *value, int32_t read_len);

/**
 * dfd_ko_i2c_dev_smbus_read - Read data from an I2C device using SMBus protocol
 * @bus: I2C bus number
 * @addr: I2C device address (7-bit)
 * @offset: Register offset to read from
 * @buf: Buffer to store the read data
 * @size: Number of bytes to read
 *
 * This function reads data from the specified I2C device using the SMBus protocol.
 * It reads `size` bytes of data starting from the given register offset and stores
 * the result in the provided buffer.
 *
 * @returns: <0 Failed, others succeeded (number of bytes read)
 */
int32_t dfd_ko_i2c_dev_smbus_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size);

/**
 *   dfd_ko_extract_i2c_bus_number - Extract I2C bus number from device path
 *   @path: Device path string containing I2C bus information
 *   This function parses a device path string to extract the I2C bus number.
 *   Typically used to extract bus number from sysfs paths like:
 *   "i2c-1"
 *
 *   The function searches for the pattern "i2c-<number>" in the input string
 *   and returns the numeric bus number.
 *
 *    @returns: Extracted I2C bus number on success, negative error code on failure
 *        -EINVAL: Invalid input parameters (NULL path)
 */
int dfd_ko_extract_i2c_bus_number(const char *path);

/**
 * @brief SMBus block readfrom I2C device
 *
 * @bus: I2C bus number
 * @addr: I2C device address
 * @command_code: SMBus Command code
 * @block_length: SMBus Block length
 * @offset: Offset of Block data
 * @buf: Pointer to the buffer to store the read data
 * @size: Size of the buffer
 *
 * @return DFD_RV_OK on success, negative error code on failure
 */
int32_t dfd_ko_i2c_dev_smbus_block_offset_read(int bus, int addr, int command_code, int block_length, int offset, uint8_t *buf, uint32_t size);
int32_t dfd_ko_i2c_dev_smbus_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size);

#endif /* __DFD_CFG_ADAPTER_H__ */
