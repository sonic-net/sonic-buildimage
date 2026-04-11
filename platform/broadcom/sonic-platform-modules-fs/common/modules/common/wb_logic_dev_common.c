/*
 * wb_logic_dev_common.c
 * ko provide universal methods to logic_dev module
 */
#include <linux/uio.h>
#include <wb_logic_dev_common.h>
#include <wb_bsp_kernel_debug.h>

/* Use the wb_bsp_kernel_debug header file must define debug variable */
static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define LOGIC_DEV_INFO(fmt, args...) do {                                        \
    printk(KERN_INFO "[LOGIC_DEV][VER][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
} while (0)

static int noop_pre(struct kprobe *p, struct pt_regs *regs) { return 0; }
static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name",
};
unsigned long (*kallsyms_lookup_name_fun)(const char *name) = NULL;

struct kobject *logic_dev_kobj;

/* Call kprobe to find the address location of kallsyms_lookup_name */
static int find_kallsyms_lookup_name(void)
{
    int ret = -1;

    kp.pre_handler = noop_pre;
    ret = register_kprobe(&kp);
    if (ret < 0) {
        DEBUG_ERROR("register_kprobe failed, error:%d\n", ret);
        return ret;
    }
    DEBUG_INFO("kallsyms_lookup_name addr: %p\n", kp.addr);
    kallsyms_lookup_name_fun = (void*)kp.addr;
    unregister_kprobe(&kp);

    return ret;
}

EXPORT_SYMBOL_GPL(kallsyms_lookup_name_fun);
EXPORT_SYMBOL_GPL(logic_dev_kobj);

static int wb_bsp_log_file_backup(char *src_path, struct file *src_fp, int src_file_size)
{
    int ret;
    struct file *dst_fp;
    char dst_path[BSP_LOG_DEV_NAME_MAX_LEN];
    char *buffer;
    ssize_t bytes_read, bytes_written;
    loff_t src_offset, dst_offset;

    buffer = kzalloc(src_file_size, GFP_KERNEL);
    if (!buffer) {
        DEBUG_ERROR("Failed to allocate memory for buffer size %d\n", src_file_size);
        return -ENOMEM;
    }

    mem_clear(dst_path, sizeof(dst_path));
    snprintf(dst_path, sizeof(dst_path), "%s_bak", src_path);
    dst_fp = filp_open(dst_path, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
    if (IS_ERR(dst_fp)) {
        DEBUG_ERROR("Open %s failed, errno = %ld\n", dst_path, -PTR_ERR(dst_fp));
        ret = PTR_ERR(dst_fp);
        goto out_free_buffer;
    }

    /* read origin file */
    src_offset = 0;
    bytes_read = kernel_read(src_fp, buffer, src_file_size, &src_offset);
    if (bytes_read < 0) {
        DEBUG_ERROR("Read %s failed, src_file_size: %d, ret: %zu\n", src_path, src_file_size, bytes_read);
        ret = bytes_read;
        goto out_close_dst;
    }
    /* write to backup file */
    dst_offset = 0;
    bytes_written = kernel_write(dst_fp, buffer, bytes_read, &dst_offset);
    if (bytes_written < 0) {
        DEBUG_ERROR("Write %s failed, src_file_size: %d, ret: %zu\n", src_path, src_file_size, bytes_written);
        ret = bytes_written;
        goto out_close_dst;
    }

    ret = 0;
    (void)vfs_fsync(dst_fp, 1);
    DEBUG_VERBOSE("Backup %s to %s success, src_file_size: %d\n", src_path, dst_path, src_file_size);
out_close_dst:
    filp_close(dst_fp, NULL);
out_free_buffer:
    kfree(buffer);
    return ret;
}

/**
 * wb_bsp_log_file_without_ts -- Log a message without generating a timestamp.
 * @log: The log message to be recorded.
 *
 * This function is used in kernel threads.
 */
static void wb_bsp_log_file_without_ts(char *path, int file_max_size, char *log, int size)
{
    struct file *fp;
    ssize_t ret;
    loff_t tmp_pos;
    struct inode *inode;

    fp = filp_open(path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (IS_ERR(fp)) {
        DEBUG_ERROR("open %s failed, errno = %ld\n", path, -PTR_ERR(fp));
        return;
    }

    /* get file size */
    inode = fp->f_inode;
    tmp_pos = i_size_read(inode);

    DEBUG_VERBOSE("%s file size: %lld, write len: %d, file_max_size: %d\n", path, tmp_pos, size, file_max_size);

    if (tmp_pos + size >= file_max_size) {
        DEBUG_VERBOSE("%s file write file offset: %lld, write len: %d, exceed max file size: %d, backup file\n",
            path, tmp_pos, size, file_max_size);
        ret = wb_bsp_log_file_backup(path, fp, tmp_pos);
        if (ret == 0) {
            DEBUG_VERBOSE("Backup %s success, truncate the file and start writing from the beginning\n", path);
            (void)filp_close(fp, NULL);
            /* Reopen file (truncate) */
            fp = filp_open(path, O_CREAT | O_RDWR | O_TRUNC, S_IRWXG | S_IRWXO);
            if (IS_ERR(fp)) {
                DEBUG_ERROR("open %s failed, errno = %ld\n", path, -PTR_ERR(fp));
                return;
            }
        } else {
            DEBUG_ERROR("Backup %s failed, ret: %zu, do not truncate the file, write it from the beginning\n", path, ret);
        }
        tmp_pos = 0;
    }

    ret = kernel_write(fp, log, size, &tmp_pos);
    if (ret < 0) {
        DEBUG_ERROR("Write %s failed, offset: %lld, size: %d, ret: %zu\n", path, tmp_pos, size, ret);
        goto finish;
    }

    vfs_fsync(fp, 1);
    DEBUG_VERBOSE("Write %s success, write len: %d, pos: %lld\n", path, size,tmp_pos);
finish:
    (void)filp_close(fp, NULL);
    return;
}

/**
 * wb_bsp_create_timestamp -- Generate a timestamp.
 * @ts: Output parameter to store the generated timestamp string.
 * @size: The size of the ts buffer.
 *
 * return: The length of the timestamp string stored in ts.
 */
static int wb_bsp_create_timestamp(char *buf, int size)
{
    int offset;
    struct tm tm;
    struct timespec64 ts;

    /* get timestamp_sec */
    mem_clear(&tm, sizeof(tm));
    mem_clear(&ts, sizeof(ts));
    ktime_get_real_ts64(&ts);
    time64_to_tm(ts.tv_sec, 0, &tm);
    mem_clear(buf, size);
    offset = snprintf(buf, size, "%-4ld-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900,
                tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return offset;
}

/**
 * wb_bsp_log_file_with_ts -- Log a message with an automatically generated timestamp.
 * @log: The log message to record.
 *
 * To be used within a kernel thread.
 */
static void wb_bsp_log_file_with_ts(char *path, int file_max_size, char *log, struct mutex *file_lock)
{
    char ts[BSP_LOG_TS_BUFF_SIZE];
    char wr_buf[BSP_KEY_DEVICE_LOG_SIZE+BSP_LOG_TS_BUFF_SIZE];

    mem_clear(ts, sizeof(ts));
    mem_clear(wr_buf, sizeof(wr_buf));
    (void)wb_bsp_create_timestamp(ts, BSP_LOG_TS_BUFF_SIZE);
    snprintf(wr_buf, sizeof(wr_buf), "%s%s", ts, log);
    mutex_lock(file_lock);
    wb_bsp_log_file_without_ts(path, file_max_size, wr_buf, strlen(wr_buf));
    mutex_unlock(file_lock);
    return;
}

int wb_bsp_key_device_log(char *dev_name, char *log_name, int log_size,
        wb_bsp_key_device_log_node_t *log_node, uint32_t offset, uint8_t *buf, size_t size)
{
    int i, j, addr, len, flags, log_len, tmp_offset;
    char log[BSP_KEY_DEVICE_LOG_SIZE];

    if (dev_name == NULL || log_name == NULL || log_node == NULL || buf == NULL ||
        size <= 0 || log_size <= 0) {
        DEBUG_ERROR("Invalid param! dev_name = %p, log_name = %p, log_node = %p, buf = %p, size = %zu, offset = 0x%x, log_size=%d\n",
            dev_name, log_name, log_node, buf, size, offset, log_size);
        return -EINVAL;
    }

    DEBUG_VERBOSE("log_name: %s, write type: %s, offset = 0x%02x size = %zu,\n", log_name, dev_name, offset, size);
    mem_clear(log, BSP_KEY_DEVICE_LOG_SIZE);
    log_len = 0;
    log_len += snprintf(log + log_len, BSP_KEY_DEVICE_LOG_SIZE - log_len, "%s: write register - value:\n", dev_name);
    /* Check if the address is the key register to be logged */
    flags = 0;
    for (j = 0; j < size; j++) {
        for (i = 0; i < log_node->log_num; i++) {
            addr = BSP_KEY_DEV_INDEX_TO_ADDR(log_node->log_index[i]);
            len = BSP_KEY_DEV_INDEX_TO_SIZE(log_node->log_index[i]);
            DEBUG_VERBOSE("log_reg[%d] addr = 0x%02x, len = 0x%02x\n", i, addr, len);
            /**
             *If the next log entry exceeds the buffer length, it will be written to the file first.
             * The magic number is related to the log format '0x%04x:0x%02x'.
             */
            if (log_len + 13 > BSP_KEY_DEVICE_LOG_SIZE) {
                /* Interrupt and crash handling processes output directly using printk without writing to a file. */
                if (unlikely(in_interrupt() || oops_in_progress)) {
                    /* Data concatenation already includes line breaks; no additional line breaks are added here */
                    printk("%s:%s", log_name, log);
                } else {
                    wb_bsp_log_file_with_ts(log_name, log_size, log, &log_node->file_lock);
                }
                mem_clear(log, BSP_KEY_DEVICE_LOG_SIZE);
                log_len = 0;
            }
            tmp_offset = offset + j;
            if ((tmp_offset >= addr) && (tmp_offset < addr + len)) {
                log_len += snprintf(log + log_len, BSP_KEY_DEVICE_LOG_SIZE - log_len, "0x%04x:0x%02x\n",
                    tmp_offset, buf[j]);
                flags = 1;
                break;
            }
        }
    }

    if (flags == 0) {
        DEBUG_VERBOSE("Key reg can not match!\n");
        return 0;
    }

    /* Interrupt and crash handling processes output directly using printk without writing to a file. */
    if (unlikely(in_interrupt() || oops_in_progress)) {
        /* Data concatenation already includes line breaks; no additional line breaks are added here */
        printk("%s", log);
    } else {
        wb_bsp_log_file_with_ts(log_name, log_size, log, &log_node->file_lock);
    }

    return 0;
}
EXPORT_SYMBOL_GPL(wb_bsp_key_device_log);

int dev_rw_check(uint8_t *rd_buf, uint8_t *wr_buf, uint32_t len, uint32_t type)
{
    uint8_t tmp_wr_val[WIDTH_4Byte];
    int i;

    if (rd_buf == NULL || wr_buf == NULL) {
        DEBUG_ERROR("Error: buf is NULL\n");
        return -EINVAL;
    }

    if ((len == 0) ||(len > WIDTH_4Byte)) {
        DEBUG_ERROR("Invalid len: %u.\n", len);
        return -EINVAL;
    }

    switch (type) {
    case READ_BACK_CHECK:
        if (memcmp(rd_buf, wr_buf, len)) {
            DEBUG_ERROR("type: %u, status check result failed.\n", type);
            for (i = 0; i < len; i++) {
                DEBUG_INFO("rd_buf[%d]: 0x%x, wr_buf[%d]: 0x%x\n",
                    i, rd_buf[i], i, wr_buf[i]);
            }
            return -EIO;
        }
        break;
    case READ_BACK_NAGATIVE_CHECK:
        for (i = 0; i < len; i++) {
            tmp_wr_val[i] = ~wr_buf[i];
        }
        if (memcmp(rd_buf, tmp_wr_val, len)) {
            DEBUG_ERROR("type: %u, status check result failed.\n", type);
            for (i = 0; i < len; i++) {
                DEBUG_INFO("rd_buf[%d]: 0x%x, tmp_wr_val[%d]: 0x%x\n",
                    i, rd_buf[i], i, tmp_wr_val[i]);
            }
            return -EIO;
        }
        break;
    default:
        DEBUG_ERROR("Failed: status check type:%d not support.\n", type);
        return -EOPNOTSUPP;
    }

    return 0;
}
EXPORT_SYMBOL_GPL(dev_rw_check);

void logic_dev_dump_data(const char *dev_name, uint32_t offset, u8 *val, size_t count, bool read_flag)
{
    int i;
    uint8_t buf[DEBUG_BUF_MAX_LEN];
    uint8_t *point;

    if ((dev_name == NULL) || (val == NULL) || (count <= 0)) {
        DEBUG_ERROR("Invalid param, dev_name: %p, val buf: %p, count: %zu\n", dev_name, val, count);
        return;
    }

    mem_clear(buf, sizeof(buf));
    point = buf;
    printk(KERN_INFO "%s %s, offset=0x%x, count=%zu, data:\n", dev_name, read_flag ? "read":"write", offset, count);
    for (i = 0; i < count; i++) {
        snprintf(point, DEBUG_BUF_MAX_LEN - (point - buf), "0x%02x ", val[i]);
        /* Format length. */
        point += 5;
        if (((i + 1) % 16) == 0) {
            printk(KERN_INFO "%s\n", buf);
            point = buf;
            mem_clear(buf, sizeof(buf));
        }
    }

    if (point != buf) {
        printk(KERN_INFO "%s\n", buf);
    }

    return;
}
EXPORT_SYMBOL_GPL(logic_dev_dump_data);

static int wb_dev_file_write(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDWR, 777);
    if (IS_ERR(filp)) {
        DEBUG_ERROR("write open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }
    ret = 0;
    tmp_pos = (loff_t)pos;
    ret = kernel_write(filp, (void*)val, size, &tmp_pos);
    if (ret < 0) {
        DEBUG_ERROR("write kernel_write failed, ret=%d\r\n", ret);
        goto exit;
    }
    vfs_fsync(filp, 1);
    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;
}

static int wb_dev_file_read(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        DEBUG_ERROR("read open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }
    ret = 0;
    tmp_pos = (loff_t)pos;
    ret = kernel_read(filp, val, size, &tmp_pos);
    if (ret < 0) {
        DEBUG_ERROR("read kernel_read failed, ret=%d\r\n", ret);
        goto exit;
    }
    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;
}

int find_intf_addr(unsigned long *write_intf_addr, unsigned long *read_intf_addr, uint32_t mode)
{
    switch (mode) {
    case SYMBOL_I2C_DEV_MODE:
        *write_intf_addr = (unsigned long)kallsyms_lookup_name_fun("i2c_device_func_write");
        *read_intf_addr = (unsigned long)kallsyms_lookup_name_fun("i2c_device_func_read");
        break;
    case SYMBOL_SPI_DEV_MODE:
        *write_intf_addr = (unsigned long)kallsyms_lookup_name_fun("spi_device_func_write");
        *read_intf_addr = (unsigned long)kallsyms_lookup_name_fun("spi_device_func_read");
        break;
    case SYMBOL_SPI_DEV_ATOMIC_MODE:
        *write_intf_addr = (unsigned long)kallsyms_lookup_name_fun("spi_device_func_atomic_write");
        *read_intf_addr = (unsigned long)kallsyms_lookup_name_fun("spi_device_func_atomic_read");
        break;
    case SYMBOL_IO_DEV_MODE:
        *write_intf_addr = (unsigned long)kallsyms_lookup_name_fun("io_device_func_write");
        *read_intf_addr = (unsigned long)kallsyms_lookup_name_fun("io_device_func_read");
        break;
    case SYMBOL_PCIE_DEV_MODE:
        *write_intf_addr = (unsigned long)kallsyms_lookup_name_fun("pcie_device_func_write");
        *read_intf_addr = (unsigned long)kallsyms_lookup_name_fun("pcie_device_func_read");
        break;
    case SYMBOL_INDIRECT_DEV_MODE:
        *write_intf_addr = (unsigned long)kallsyms_lookup_name_fun("indirect_device_func_write");
        *read_intf_addr = (unsigned long)kallsyms_lookup_name_fun("indirect_device_func_read");
        break;
    case SYMBOL_PLATFORM_I2C_DEV_MODE:
        *write_intf_addr = (unsigned long)kallsyms_lookup_name_fun("platform_i2c_device_func_write");
        *read_intf_addr = (unsigned long)kallsyms_lookup_name_fun("platform_i2c_device_func_read");
        break;
    case FILE_MODE:
        *write_intf_addr = (unsigned long)wb_dev_file_write;
        *read_intf_addr = (unsigned long)wb_dev_file_read;
        break;
    default:
        DEBUG_ERROR("func mode %d don't support.\n", mode);
        return -EINVAL;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(find_intf_addr);

int cache_value_read(const char *mask_file_path, const char *cache_file_path, uint32_t offset, uint8_t *value, uint32_t count)
{
    int ret, i;
    u8 mask_value[MAX_RW_LEN];
    u8 cache_value[MAX_RW_LEN];
    uint32_t total_read, per_len;

    if ((mask_file_path == NULL) || (cache_file_path == NULL) || (value == NULL)) {
        DEBUG_ERROR("Invalid params, mask_file_path or cache_file_path or value in NULL\n");
        return -EINVAL;
    }

    if (count == 0) {
        DEBUG_ERROR("Invalid params, read count is 0.\n");
        return -EINVAL;
    }

    DEBUG_INFO("mask_file_path: %s, cache_file_path: %s, offset: 0x%x, count: %u\n",
        mask_file_path, cache_file_path, offset, count);

    total_read = 0;
    while (total_read < count) {
        mem_clear(mask_value, sizeof(mask_value));
        mem_clear(cache_value, sizeof(cache_value));
        per_len = (count - total_read) > MAX_RW_LEN ? MAX_RW_LEN : (count - total_read);

        ret = wb_dev_file_read(mask_file_path, offset, mask_value, per_len);
        if (ret < 0) {
            DEBUG_ERROR("mask_file read failed, mask_file_path: %s, offset: 0x%x, read_len: %u, ret: %d\n",
                mask_file_path, offset, per_len, ret);
            return ret;
        }

        DEBUG_INFO("mask_file read success, mask_file_path: %s, offset: 0x%x, read_len: %u\n",
            mask_file_path, offset, per_len);

        ret = wb_dev_file_read(cache_file_path, offset, cache_value, per_len);
        if (ret < 0) {
            DEBUG_ERROR("cache_file read failed, cache_file_path: %s, offset: 0x%x, read_len: %u, ret: %d\n",
                cache_file_path, offset, per_len, ret);
            return ret;
        }
        DEBUG_INFO("cache_file read success, cache_file_path: %s, offset: 0x%x, read_len: %u\n",
            cache_file_path, offset, per_len);

        for (i = 0; i < per_len; i++) {
            if (mask_value[i] > 0) {
                DEBUG_INFO("offset: 0x%x replace origin value: 0x%x to cache value: 0x%x\n",
                    offset + i, value[total_read + i], cache_value[i]);
                value[total_read + i] = cache_value[i];
            }
        }
        offset += per_len;
        total_read += per_len;
    }

    return total_read;
}
EXPORT_SYMBOL_GPL(cache_value_read);

int cache_value_write(const char *mask_file_path, const char *cache_file_path, uint32_t offset, uint8_t *value, uint32_t count)
{
    int ret, i;
    u8 mask_value[MAX_RW_LEN];
    u8 cache_value[MAX_RW_LEN];
    uint32_t total_write, per_len;

    if ((mask_file_path == NULL) || (cache_file_path == NULL) || (value == NULL)) {
        DEBUG_ERROR("Invalid params, mask_file_path or cache_file_path or value in NULL\n");
        return -EINVAL;
    }

    if (count == 0) {
        DEBUG_ERROR("Invalid params, write count is 0.\n");
        return -EINVAL;
    }

    DEBUG_INFO("mask_file_path: %s, cache_file_path: %s, offset: 0x%x, count: %u\n",
        mask_file_path, cache_file_path, offset, count);

    total_write = 0;
    while (total_write < count) {
        mem_clear(mask_value, sizeof(mask_value));
        mem_clear(cache_value, sizeof(cache_value));
        per_len = (count - total_write) > MAX_RW_LEN ? MAX_RW_LEN : (count - total_write);

        ret = wb_dev_file_read(mask_file_path, offset, mask_value, per_len);
        if (ret < 0) {
            DEBUG_ERROR("mask_file read failed, mask_file_path: %s, offset: 0x%x, read_len: %u, ret: %d\n",
                mask_file_path, offset, per_len, ret);
            return ret;
        }

        DEBUG_INFO("mask_file read success, mask_file_path: %s, offset: 0x%x, read_len: %u\n",
            mask_file_path, offset, per_len);

        ret = wb_dev_file_read(cache_file_path, offset, cache_value, per_len);
        if (ret < 0) {
            DEBUG_ERROR("cache_file read failed, cache_file_path: %s, offset: 0x%x, read_len: %u, ret: %d\n",
                cache_file_path, offset, per_len, ret);
            return ret;
        }
        DEBUG_INFO("cache_file read success, cache_file_path: %s, offset: 0x%x, read_len: %u\n",
            cache_file_path, offset, per_len);

        for (i = 0; i < per_len; i++) {
            if (mask_value[i] > 0) {
                DEBUG_INFO("cache file: %s offset: 0x%x replace value: 0x%x to value: 0x%x\n",
                    cache_file_path, offset + i, cache_value[i], value[i]);
                cache_value[i] = value[total_write + i];
            }
        }
        ret = wb_dev_file_write(cache_file_path, offset, cache_value, per_len);
        if (ret < 0) {
            DEBUG_ERROR("cache_file write failed, cache_file_path: %s, offset: 0x%x, write_len: %u, ret: %d\n",
                cache_file_path, offset, per_len, ret);
            return ret;
        }
        DEBUG_INFO("cache_file write success, cache_file_path: %s, offset: 0x%x, write_len: %u\n",
            cache_file_path, offset, per_len);

        offset += per_len;
        total_write += per_len;
    }

    return total_write;
}
EXPORT_SYMBOL_GPL(cache_value_write);

static int __init logic_dev_init(void)
{
    int ret;
    LOGIC_DEV_INFO("logic_dev_init...\n");
    logic_dev_kobj = kobject_create_and_add("logic_dev", NULL);
    if (!logic_dev_kobj) {
        DEBUG_ERROR("create logic_dev_kobj error.\n");
        return -ENOMEM;
	}

    ret = find_kallsyms_lookup_name();
    if (ret < 0) {
        DEBUG_ERROR("find kallsyms_lookup_name failed\n");
        kobject_put(logic_dev_kobj);
        logic_dev_kobj = NULL;
        return -ENXIO;
    }
    DEBUG_INFO("find kallsyms_lookup_name ok\n");
    LOGIC_DEV_INFO("logic_dev_init success.\n");
    return 0;
}

static void __exit logic_dev_exit(void)
{
    if (logic_dev_kobj) {
        kobject_put(logic_dev_kobj);
    }
    LOGIC_DEV_INFO("logic_dev_exit success.\n");
}

module_init(logic_dev_init);
module_exit(logic_dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("sonic logic_dev common methods");
