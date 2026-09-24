/*
 * wb_logic_dev_common.c
 * ko provide universal methods to logic_dev module
 */

#include <wb_logic_dev_common.h>
#include <wb_bsp_kernel_debug.h>

#define LOGIC_DEV_DUMP_LINE_BYTES (16)

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

static int check_mode_data_process(uint32_t check_mode, uint8_t *data, int data_len)
{
    int ret;
    int i;

    ret = 0;
    switch (check_mode) {
    case WB_LOGIC_NORMAL:
        break;
    case WB_LOGIC_NAGATIVE_MODE:
        for (i = 0; i < data_len; i++) {
            data[i] = ~data[i];
        }
        break;
    default:
        return -EINVAL;
    }

    return ret;
}

int dev_rw_check(uint8_t *rd_buf, uint8_t *wr_buf, uint32_t len, uint32_t type, uint32_t check_mode)
{
    int i;
    int ret;

    if (rd_buf == NULL || wr_buf == NULL) {
        DEBUG_ERROR("Error: buf is NULL\n");
        return -EINVAL;
    }

    if ((len == 0) ||(len > MAX_DATA_WIDTH)) {
        DEBUG_ERROR("Invalid len: %u.\n", len);
        return -EINVAL;
    }

    if (check_mode >= WB_LOGIC_SUPPORT_CHECK_MODE_MAX) {
        DEBUG_ERROR("Invalid check_mode: 0x%x.\n", check_mode);
        return -EINVAL;
    }

    ret = check_mode_data_process(check_mode, rd_buf, len);
    if (ret < 0) {
        DEBUG_ERROR("check mode check_mode_data_process fail, check_mode: %d, ret:%d.\n", check_mode, ret);
        return ret;
    }

    switch (type) {
    case STATUS_CHECK_SELFTEST:
    case STATUS_CHECK_SCRATCH:
    case STATUS_CHECK_SEU:
    case STATUS_CHECK_CRAM:
        if (memcmp(rd_buf, wr_buf, len)) {
            DEBUG_ERROR("type: %u, status check result failed.\n", type);
            for (i = 0; i < len; i++) {
                DEBUG_INFO("rd_buf[%d]: 0x%x, wr_buf[%d]: 0x%x\n",
                    i, rd_buf[i], i, wr_buf[i]);
            }
            return -EIO;
        }
        break;
    default:
        DEBUG_ERROR("Failed: status check type:%d not support.\n", type);
        return -EINVAL;
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

    if (!*write_intf_addr || !*read_intf_addr) {
        DEBUG_ERROR("Fail: func mode %u rw symbol undefined\n", mode);
        return -ENOSYS;
    }

    return 0;
}
EXPORT_SYMBOL_GPL(find_intf_addr);

int find_cs_intf_addr(unsigned long *cs_enable_intf_addr, unsigned long *cs_disable_intf_addr)
{
    *cs_enable_intf_addr = (unsigned long)kallsyms_lookup_name_fun("chip_select_enable");
    *cs_disable_intf_addr = (unsigned long)kallsyms_lookup_name_fun("chip_select_disable");

    if (!*cs_enable_intf_addr || !*cs_disable_intf_addr) {
        DEBUG_ERROR("Fail: cs enable or disable symbol undefined\n");
        return -ENOSYS;
    }

    return 0;
}
EXPORT_SYMBOL_GPL(find_cs_intf_addr);


static int logic_dev_read(logic_dev_dump_t *dump_logic_dev, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_read pfunc;

    if (!dump_logic_dev->logic_dev_read_intf_addr) {
        return -ENOSYS;
    }

    pfunc = (device_func_read)dump_logic_dev->logic_dev_read_intf_addr;
    return pfunc(dump_logic_dev->logic_dev_name, pos, val, size);
}

static void logic_dev_dump_buf_seg(struct device *dev, uint32_t base, const uint8_t *buf, size_t bytes)
{
    int i, j, pos;
    char print_line[MAX_RW_LEN];

    for (i = 0; i < bytes; i += LOGIC_DEV_DUMP_LINE_BYTES) {
        pos = snprintf(print_line, sizeof(print_line), "0x%04x:", base + i);
        for (j = 0; j < LOGIC_DEV_DUMP_LINE_BYTES && (i + j) < bytes; j++) {
            pos += snprintf(print_line + pos, sizeof(print_line) - pos, " %02x", buf[i + j]);
        }
        dev_info(dev, "%s\n", print_line);
    }
}

void logic_dev_dump_regs(logic_dev_dump_info_t *dump_info, struct device *dev, const char *log_tag)
{
    uint8_t dump_buf[LOGIC_DEV_DUMP_LINE_BYTES];
    uint32_t addr, bytes, i;
    int ret;
    logic_dev_dump_t *dump_logic_dev;
    const char *tag;

    if (!dump_info || !dev) {
        return;
    }

    tag = log_tag ? log_tag : "logic_dev";
    for (i = 0; i < dump_info->dump_logic_dev_num; i++) {
        dump_logic_dev = &dump_info->dump_logic_dev_cfg[i];
        dev_info(dev,
            "[%s] :dump logic dev regs, dev:%s, range:[0x%x, 0x%x]\n",
            tag, dump_logic_dev->logic_dev_name,
            dump_logic_dev->logic_dev_reg_start, dump_logic_dev->logic_dev_reg_end);

        for (addr = dump_logic_dev->logic_dev_reg_start;
             addr <= dump_logic_dev->logic_dev_reg_end;
             addr += LOGIC_DEV_DUMP_LINE_BYTES) {
            bytes = min_t(uint32_t, LOGIC_DEV_DUMP_LINE_BYTES,
                dump_logic_dev->logic_dev_reg_end - addr + 1);
            mem_clear(dump_buf, sizeof(dump_buf));

            ret = logic_dev_read(dump_logic_dev, addr, dump_buf, bytes);
            if (ret < 0) {
                dev_err(dev,
                    "read logic dev reg failed, dev:%s, addr:0x%x, len:%u, ret:%d\n",
                    dump_logic_dev->logic_dev_name, addr, bytes, ret);
                break;
            }
            logic_dev_dump_buf_seg(dev, addr, dump_buf, bytes);
        }
        dev_info(dev, "===== %s: End dump from logic dev regs =====\n", tag);
    }
}
EXPORT_SYMBOL_GPL(logic_dev_dump_regs);

static int logic_dev_dump_init(logic_dev_dump_info_t *dump_info, struct device *dev, uint32_t cfg_index,
    const logic_dev_dump_cfg_t *dump_logic_dev_cfg)
{
    int ret;
    logic_dev_dump_t *dump_logic_dev;

    if (!dump_info || !dev || !dump_logic_dev_cfg) {
        dev_err(dev, "invalid param, dump_info:%p, dev:%p, dump_logic_dev_cfg:%p\n",
            dump_info, dev, dump_logic_dev_cfg);
        return -EINVAL;
    }

    if (!dump_logic_dev_cfg->logic_dev_name[0]) {
        dev_err(dev, "invalid logic dev dump name, index:%u\n", cfg_index);
        return -EINVAL;
    }

    if (cfg_index >= LOGIC_DEV_DUMP_MAX_NUM) {
        dev_err(dev, "logic dev dump index %u out of range, max:%u\n",
            cfg_index, LOGIC_DEV_DUMP_MAX_NUM);
        return -EINVAL;
    }

    if (dump_logic_dev_cfg->logic_dev_reg_end < dump_logic_dev_cfg->logic_dev_reg_start) {
        dev_err(dev,
            "invalid logic dev dump range, dev:%s, start:0x%x, end:0x%x\n",
            dump_logic_dev_cfg->logic_dev_name,
            dump_logic_dev_cfg->logic_dev_reg_start,
            dump_logic_dev_cfg->logic_dev_reg_end);
        return -EINVAL;
    }

    dump_logic_dev = &dump_info->dump_logic_dev_cfg[cfg_index];
    dump_logic_dev->logic_dev_name = kstrdup_const(dump_logic_dev_cfg->logic_dev_name, GFP_KERNEL);
    if (!dump_logic_dev->logic_dev_name) {
        dev_err(dev, "alloc logic dev dump name failed, dev:%s\n",
            dump_logic_dev_cfg->logic_dev_name);
        return -ENOMEM;
    }
    dump_logic_dev->logic_dev_func_mode = dump_logic_dev_cfg->logic_dev_func_mode;
    dump_logic_dev->logic_dev_reg_start = dump_logic_dev_cfg->logic_dev_reg_start;
    dump_logic_dev->logic_dev_reg_end = dump_logic_dev_cfg->logic_dev_reg_end;

    ret = find_intf_addr(&dump_logic_dev->logic_dev_write_intf_addr,
        &dump_logic_dev->logic_dev_read_intf_addr, dump_logic_dev->logic_dev_func_mode);
    if (ret) {
        dev_err(dev,
            "find_intf_addr for logic dev dump failed, mode:%u, ret:%d\n",
            dump_logic_dev->logic_dev_func_mode, ret);
        return ret;
    }

    if (!dump_logic_dev->logic_dev_write_intf_addr || !dump_logic_dev->logic_dev_read_intf_addr) {
        dev_err(dev,
            "logic dev dump rw symbol undefined, mode:%u\n",
            dump_logic_dev->logic_dev_func_mode);
        return -ENOSYS;
    }

    LOGIC_DEV_INFO("logic dev dump init success, index:%u, dev:%s, mode:%u, range:[0x%x, 0x%x]\n",
        cfg_index, dump_logic_dev->logic_dev_name, dump_logic_dev->logic_dev_func_mode,
        dump_logic_dev->logic_dev_reg_start, dump_logic_dev->logic_dev_reg_end);

    return 0;
}

int logic_dev_dump_of_node_init(logic_dev_dump_info_t *dump_info, struct device *dev)
{
    int ret;
    int rv;
    int dump_logic_dev_name_num;
    uint32_t i;
    uint32_t dump_logic_dev_num;
    uint32_t dump_logic_dev_func_mode;
    uint32_t dump_logic_dev_reg_start;
    uint32_t dump_logic_dev_reg_end;
    const char *dump_logic_dev_name;
    logic_dev_dump_cfg_t dump_logic_dev_cfg;

    if (!dump_info || !dev || !dev->of_node) {
        dev_err(dev, "invalid param, dump_info:%p, dev:%p, of_node:%p\n",
            dump_info, dev, dev ? dev->of_node : NULL);
        return -EINVAL;
    }

    dump_logic_dev_name_num = of_property_count_strings(dev->of_node, "dump_logic_dev_name");
    if (dump_logic_dev_name_num <= 0) {
        return 0;
    }

    if (dump_logic_dev_name_num > LOGIC_DEV_DUMP_MAX_NUM) {
        dev_err(dev, "invalid logic dev dump num:%d, max:%u\n",
            dump_logic_dev_name_num, LOGIC_DEV_DUMP_MAX_NUM);
        return -EINVAL;
    }

    dump_logic_dev_num = dump_logic_dev_name_num;
    for (i = 0; i < dump_logic_dev_num; i++) {
        rv = of_property_read_string_index(dev->of_node, "dump_logic_dev_name", i, &dump_logic_dev_name);
        if (rv != 0) {
            dev_err(dev, "dump_logic_dev_name[%u] read failed, rv:%d\n", i, rv);
            return rv;
        }

        if (!dump_logic_dev_name || !dump_logic_dev_name[0]) {
            dev_err(dev, "dump_logic_dev_name[%u] is empty\n", i);
            return -EINVAL;
        }

        rv = of_property_read_u32_index(dev->of_node, "dump_logic_dev_func_mode", i,
            &dump_logic_dev_func_mode);
        if (rv != 0) {
            dev_err(dev,
                "dump_logic_dev_name[%u] configured but dump_logic_dev_func_mode[%u] is missing.\n",
                i, i);
            return rv;
        }

        dump_logic_dev_reg_start = 0;
        dump_logic_dev_reg_end = LOGIC_DEV_DUMP_REG_END_DEFAULT;
        of_property_read_u32_index(dev->of_node, "dump_logic_dev_reg_start", i, &dump_logic_dev_reg_start);
        of_property_read_u32_index(dev->of_node, "dump_logic_dev_reg_end", i, &dump_logic_dev_reg_end);

        mem_clear(&dump_logic_dev_cfg, sizeof(dump_logic_dev_cfg));
        strscpy(dump_logic_dev_cfg.logic_dev_name, dump_logic_dev_name,
            sizeof(dump_logic_dev_cfg.logic_dev_name));
        dump_logic_dev_cfg.logic_dev_func_mode = dump_logic_dev_func_mode;
        dump_logic_dev_cfg.logic_dev_reg_start = dump_logic_dev_reg_start;
        dump_logic_dev_cfg.logic_dev_reg_end = dump_logic_dev_reg_end;

        ret = logic_dev_dump_init(dump_info, dev, i, &dump_logic_dev_cfg);
        if (ret) {
            return ret;
        }
    }

    dump_info->dump_logic_dev_num = dump_logic_dev_num;

    return 0;
}
EXPORT_SYMBOL_GPL(logic_dev_dump_of_node_init);

int logic_dev_dump_cfg_info_init(logic_dev_dump_info_t *dump_info, struct device *dev,
    const logic_dev_dump_cfg_info_t *dump_logic_dev_cfg_info)
{
    int ret;
    uint32_t i;
    uint32_t dump_logic_dev_num;
    logic_dev_dump_cfg_t dump_logic_dev_cfg;

    if (!dump_info || !dev || !dump_logic_dev_cfg_info) {
        dev_err(dev, "invalid param, dump_info:%p, dev:%p, dump_logic_dev_cfg_info:%p\n",
            dump_info, dev, dump_logic_dev_cfg_info);
        return -EINVAL;
    }

    dump_logic_dev_num = dump_logic_dev_cfg_info->dump_logic_dev_num;
    if (dump_logic_dev_num > LOGIC_DEV_DUMP_MAX_NUM) {
        dev_err(dev, "invalid logic dev dump num:%u, max:%u\n",
            dump_logic_dev_num, LOGIC_DEV_DUMP_MAX_NUM);
        return -EINVAL;
    }

    for (i = 0; i < dump_logic_dev_num; i++) {
        mem_clear(&dump_logic_dev_cfg, sizeof(dump_logic_dev_cfg));
        dump_logic_dev_cfg = dump_logic_dev_cfg_info->dump_logic_dev_cfg[i];
        if (!dump_logic_dev_cfg.logic_dev_name[0]) {
            dev_err(dev, "dump_logic_dev_cfg[%u].logic_dev_name is empty\n", i);
            return -EINVAL;
        }

        if (dump_logic_dev_cfg.logic_dev_reg_end == 0) {
            dump_logic_dev_cfg.logic_dev_reg_end = LOGIC_DEV_DUMP_REG_END_DEFAULT;
        }

        ret = logic_dev_dump_init(dump_info, dev, i, &dump_logic_dev_cfg);
        if (ret) {
            return ret;
        }
    }

    dump_info->dump_logic_dev_num = dump_logic_dev_num;

    return 0;
}
EXPORT_SYMBOL_GPL(logic_dev_dump_cfg_info_init);

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

static int logic_dev_status_check_read_func(struct device_status_check *status_check,
                                                        uint32_t reg,
                                                        uint8_t *data_buf,
                                                        uint32_t read_len)
{
    int ret;

    if (status_check == NULL) {
        DEBUG_ERROR("status_check is NULL\n");
        return -EINVAL;
    }

    if (status_check->dev_bus_ops.read == NULL) {
        DEBUG_ERROR("status_check read func is NULL\n");
        return -EINVAL;
    }

    ret = status_check->dev_bus_ops.read(status_check->dev_name,
                        reg,
                        data_buf,
                        read_len);

    return ret;
}

static int logic_dev_status_check_write_func(struct device_status_check *status_check,
                                                        uint32_t reg,
                                                        uint8_t *data_buf,
                                                        uint32_t read_len)
{
    int ret;

    if (status_check == NULL) {
        DEBUG_ERROR("status_check is NULL\n");
        return -EINVAL;
    }

    if (status_check->dev_bus_ops.write == NULL) {
        DEBUG_ERROR("status_check write func is NULL\n");
        return -EINVAL;
    }

    ret = status_check->dev_bus_ops.write(status_check->dev_name,
                        reg,
                        data_buf,
                        read_len);

    return ret;
}

static int status_check_read_check(struct device_status_check *status_check,
                                  uint32_t data_byte_len,
                                  uint32_t reg,
                                  uint8_t *check_val,
                                  uint8_t *mask,
                                  uint32_t type)
{
    int ret;
    uint8_t val[MAX_DATA_WIDTH];
    u32 j;

    mem_clear(val, sizeof(val));
    ret = logic_dev_status_check_read_func(status_check, reg, val, data_byte_len);
    if (ret < 0) {
        DEBUG_ERROR("Soft fail read failed at reg 0x%x: read=0x%x expect=0x%x mask=0x%x\n",
               reg,
               val[0],
               check_val[0],
               mask[0]);

        return ret;
    }

    for (j = 0; j < data_byte_len; j++) {
        DEBUG_VERBOSE("reg 0x%x: read=0x%x expect=0x%x mask=0x%x\n", reg, val[j], check_val[j], mask[j]);
        val[j] &= mask[j];
    }

    ret = dev_rw_check(val, check_val, data_byte_len, type, WB_LOGIC_NORMAL);
    if (ret < 0) {
        DEBUG_ERROR("STATUS_NOT_OK result check failed, ret:%d.\n", ret);
        DEBUG_ERROR("Soft fail check failed at reg 0x%x: read=0x%x expect=0x%x mask=0x%x\n",
               reg,
               val[0],
               check_val[0],
               mask[0]);

        return ret;
    }

    DEBUG_VERBOSE("STATUS_OK rw one time result check success. reg 0x%x: read=0x%x expect=0x%x mask=0x%x\n",
           reg,
           val[0],
           check_val[0],
           mask[0]);

    return 0;
}

static int check_seu(struct device_status_check *status_check,
                                  uint32_t data_byte_len)
{
    int ret;
    u32 i;

    if (!(status_check->status_check_type_bmp & STATUS_CHECK_SEU)) {
        return 0;
    }

    for (i = 0; i < status_check->seu_check_num; i++) {
        ret = status_check_read_check(status_check, data_byte_len, status_check->seu_checks[i].reg,
                                        status_check->seu_checks[i].value, status_check->seu_checks[i].mask,
                                        STATUS_CHECK_SEU);
        if (ret < 0) {
            DEBUG_ERROR("status_check_read_check failed\n");
            return ret;
        }
    }

    DEBUG_VERBOSE("STATUS_OK rw one time result check success.");
    return 0;
}

static int check_selftest_status(struct device_status_check *status_check,
                                  uint32_t data_byte_len)
{
    int ret;
    uint8_t val[MAX_DATA_WIDTH];
    u32 i;

    if (!(status_check->status_check_type_bmp & STATUS_CHECK_SELFTEST)) {
        return 0;
    }

    for (i = 0; i < status_check->selftest_check_num; i++) {
        ret = logic_dev_status_check_write_func(status_check,
                             status_check->selftest_checks[i].reg,
                             status_check->selftest_checks[i].write_value,
                             data_byte_len);
        if (ret < 0) {
            DEBUG_ERROR("selftest fail write failed at reg 0x%x: wr_value=0x%x\n",
                   status_check->selftest_checks[i].reg,
                   status_check->selftest_checks[i].write_value[0]);
            return ret;
        }

        mem_clear(val, sizeof(val));
        ret = logic_dev_status_check_read_func(status_check,
                            status_check->selftest_checks[i].reg,
                            val,
                            data_byte_len);
        if (ret < 0) {
            DEBUG_ERROR("selftest fail read failed at reg 0x%x\n",
                   status_check->selftest_checks[i].reg);

            return ret;
        }

        ret = dev_rw_check(val, status_check->selftest_checks[i].write_value, data_byte_len, STATUS_CHECK_SELFTEST,
                            status_check->selftest_checks[i].check_mode);
        if (ret < 0) {
            DEBUG_ERROR("STATUS_NOT_OK result check failed, ret:%d.\n", ret);
            DEBUG_ERROR("selftest fail check failed at reg 0x%x: read=0x%x expect=0x%x check_mode=0x%x\n",
                   status_check->selftest_checks[i].reg,
                   val[0],
                   status_check->selftest_checks[i].write_value[0],
                   status_check->selftest_checks[i].check_mode);
            return ret;
        }

        DEBUG_ERROR("STATUS_OK rw one time result check success. reg 0x%x: read=0x%x expect=0x%x check_type=0x%x\n",
               status_check->selftest_checks[i].reg,
               val[0],
               status_check->selftest_checks[i].write_value[0],
               status_check->selftest_checks[i].check_mode);
    }

    DEBUG_VERBOSE("STATUS_OK rw one time result check success.");
    return 0;
}

static int check_scratch_status(struct device_status_check *status_check,
                                     uint32_t data_byte_len)
{
    int ret;
    uint8_t val[MAX_DATA_WIDTH];
    u32 i;

    if (!(status_check->status_check_type_bmp & STATUS_CHECK_SCRATCH)) {
        return 0;
    }

    for (i = 0; i < status_check->scratch_num; i++) {
        mem_clear(val, sizeof(val));
        ret = logic_dev_status_check_read_func(status_check,
                            status_check->scratch_checks[i].reg,
                            val,
                            data_byte_len);
        if (ret < 0) {
            DEBUG_ERROR("check scratch fail, read failed at reg 0x%x\n",
                   status_check->scratch_checks[i].reg);

            return ret;
        }

        ret = dev_rw_check(val, status_check->scratch_checks[i].value, data_byte_len, STATUS_CHECK_SCRATCH,
                            status_check->scratch_checks[i].check_mode);
        if (ret < 0) {
            DEBUG_ERROR("STATUS_NOT_OK result check failed, ret:%d.\n", ret);
            DEBUG_ERROR("Soft fail check failed at reg 0x%x: read=0x%x expect=0x%x check_mode=0x%x\n",
                   status_check->scratch_checks[i].reg,
                   val[0],
                   status_check->scratch_checks[i].value[0],
                   status_check->scratch_checks[i].check_mode);

            return ret;
        }

        DEBUG_VERBOSE("STATUS_OK rw one time result check success. reg 0x%x: read=0x%x expect=0x%x\n",
               status_check->scratch_checks[i].reg,
               val[0],
               status_check->scratch_checks[i].value[0]);

    }

    DEBUG_VERBOSE("STATUS_OK rw one time result check success.");
    return 0;
}

static uint64_t combine_bytes_to_value(uint8_t *buf, uint32_t buf_len, uint32_t width) {
    uint64_t val;
    int j;

    val = 0;
    for (j = 0; (j < width) && (j < buf_len); j++) {
        val |= (uint64_t)(buf[j] & 0xff) << (8 * j);
    }

    return val;
}

static int check_cram(struct device_status_check *status_check,
                                  uint32_t data_byte_len,
                                  uint32_t *status_value)
{
    int ret;
    uint8_t val[MAX_DATA_WIDTH];

    *status_value = CRAM_ALL_BITS_SET;
    if (!(status_check->status_check_type_bmp & STATUS_CHECK_CRAM)) {
        return 0;
    }

    if (data_byte_len < WIDTH_4Byte || data_byte_len > MAX_DATA_WIDTH) {
        DEBUG_ERROR("data_byte_len to short. len:%d.\n", data_byte_len);
        return -EINVAL;
    }

    mem_clear(val, sizeof(val));
    ret = logic_dev_status_check_read_func(status_check,
                        status_check->cram_checks.reg,
                        val,
                        data_byte_len);
    if (ret < 0) {
        DEBUG_ERROR("check cram fail, read failed at reg 0x%x\n",
               status_check->cram_checks.reg);

        return ret;
    }

    *status_value = CRAM_STATUS_FROM_BYTE(val);
    if (*status_value != CRAM_STATUS_OK) {
        DEBUG_ERROR("status_value: 0x%x, ok value 0x%x, status check result failed.\n", *status_value, CRAM_STATUS_OK);
        return -EIO;
    }

    DEBUG_VERBOSE("STATUS_OK cram result check success. reg 0x%x: status_value=0x%x expect=0x%x\n",
           status_check->cram_checks.reg,
           *status_value,
           CRAM_STATUS_OK);

    return 0;
}

static int dev_status_check(struct device_status_check *status_check,
                                     uint32_t data_byte_len,
                                     uint32_t type,
                                     uint32_t *status_value)
{
    int ret, i;

    ret = 0;
    for (i = 0; i < LOGIC_DEV_RETRY_TIME; i++) {
        if (type == STATUS_CHECK_SEU) {
            ret = check_seu(status_check, data_byte_len);
        } else if (type == STATUS_CHECK_SELFTEST) {
            ret = check_selftest_status(status_check, data_byte_len);
        } else if (type == STATUS_CHECK_SCRATCH) {
            ret = check_scratch_status(status_check, data_byte_len);
        } else if (type == STATUS_CHECK_CRAM) {
            ret = check_cram(status_check, data_byte_len, status_value);
        } else {
            ret = -1;
            break;
        }

        if (ret < 0) {
            DEBUG_ERROR("check STATUS_NOT_OK time: %d, type: 0x%x, ret: %d\n",
                i, type, ret);
            msleep(LOGIC_DEV_RETRY_DELAY_MS);
            continue;
        }
        break;
    }

    return ret;
}

static int cram_fail_format_device_status_message(struct device_status_check *status_check,
                                         uint32_t data_byte_len,
                                         uint32_t type,
                                         char *buf,
                                         uint32_t buf_len,
                                         uint32_t status_value)
{
    int ret;
    uint8_t val[MAX_DATA_WIDTH];
    u32 i;
    uint64_t value_tmp;
    int count;
    const char *detail;
    uint32_t output_status_value;

    DEBUG_VERBOSE("cram_fail_format_device_status_message start.");

    /* Normalize single+multi bit error to multi-bit error. */
    output_status_value = (status_value == CRAM_STATUS_SINGLE_MULTI_BIT_ERROR) ?
                          CRAM_STATUS_MULTI_BIT_ERROR : status_value;

    /* Return a read-failure detail when CRAM status cannot be parsed. */
    if (output_status_value == CRAM_ALL_BITS_SET) {
        return snprintf(buf, buf_len, "0x%x %s\n", output_status_value, CRAM_DETAIL_LOGICDEV_ERROR);
    }

    if (output_status_value > CRAM_STATUS_MULTI_BIT_ERROR) {
        DEBUG_ERROR("cram_fail_format_device_status_message invalid status value: 0x%x\n", output_status_value);
        return -EINVAL;
    }

    count = snprintf(buf, buf_len, "0x%x", output_status_value);

    for (i = 0; i < status_check->cram_checks.cram_err_addr_reg_num; i++) {
        mem_clear(val, sizeof(val));
        ret = logic_dev_status_check_read_func(status_check,
                            status_check->cram_checks.cram_err_addr_reg[i],
                            val,
                            data_byte_len);
        if (ret < 0) {
            DEBUG_ERROR("cram_err_addr_reg read fail, read failed at reg 0x%x data_byte_len %d\n",
                   status_check->cram_checks.cram_err_addr_reg[i], data_byte_len);
            count += snprintf(buf + strlen(buf), buf_len - strlen(buf), " fail");

            continue;
        }

        value_tmp = combine_bytes_to_value(val, sizeof(val), data_byte_len);
        value_tmp &= status_check->cram_checks.cram_err_addr_reg_mask[i];
        value_tmp = value_tmp >> CRAM_ERR_ADDR_DATA_OFFSET;

        count += snprintf(buf + strlen(buf), buf_len - strlen(buf), " 0x%" PRIX64 "", value_tmp);

        DEBUG_VERBOSE("cram_fail_format_device_status_message reg 0x%x: read=0x%" PRIX64 " data_byte_len %d mask 0x%x\n",
               status_check->cram_checks.cram_err_addr_reg[i],
               value_tmp,
               data_byte_len,
               status_check->cram_checks.cram_err_addr_reg_mask[i]);

    }

    detail = NULL;
    switch (output_status_value) {
    case CRAM_STATUS_SINGLE_BIT_ERROR:
        detail = CRAM_DETAIL_SINGLE_BIT_ERROR;
        break;
    case CRAM_STATUS_MULTI_BIT_ERROR:
        detail = CRAM_DETAIL_MULTI_BIT_ERROR;
        break;
    default:
        break;
    }

    if (detail != NULL) {
        count += snprintf(buf + strlen(buf), buf_len - strlen(buf), " %s", detail);
    }

    DEBUG_VERBOSE("cram_fail_format_device_status_message end.");
    count += snprintf(buf + strlen(buf), buf_len - strlen(buf), "\n");

    return count;

}

static int format_device_status_message(struct device_status_check *status_check,
                                         uint32_t data_byte_len,
                                         uint32_t type,
                                         char *buf,
                                         uint32_t buf_len,
                                         uint32_t status,
                                         uint32_t status_value)
{
    int ret;

    ret = 0;
    switch (type) {
    case STATUS_CHECK_SEU:
    case STATUS_CHECK_SELFTEST:
    case STATUS_CHECK_SCRATCH:
        return snprintf(buf, buf_len, "0x%x\n", status);
        break;
    case STATUS_CHECK_CRAM:
        if (status == LOGIC_DEV_STATUS_OK) {
            return snprintf(buf, buf_len, "0x%x\n", status);
        }

        return cram_fail_format_device_status_message(status_check, data_byte_len, type,
                                                        buf, buf_len, status_value);

        break;
    default:
        DEBUG_ERROR("not found init func, check type 0x%x\n", type);
        return -EINVAL;
    }

    return ret;

}


static int dev_status_check_message(struct device_status_check *status_check,
                                             uint32_t data_byte_len,
                                             uint32_t type,
                                             char *buf,
                                             uint32_t buf_len)
{
    int ret;
    u32 status;
    uint32_t status_value;

    status = 0;
    ret = dev_status_check(status_check, data_byte_len, type, &status_value);
    if (ret != 0) {
        status = LOGIC_DEV_STATUS_NOT_OK;
    }

    ret = format_device_status_message(status_check, data_byte_len, type, buf, buf_len, status, status_value);

    return ret;
}

static int wb_logic_check_scratch_status_init(struct device_status_check *status_check,
                                     uint32_t data_byte_len)
{
    int ret;
    u32 i;

    if (status_check == NULL) {
        DEBUG_ERROR("param is NULL\n");
        return -EINVAL;
    }

    if (data_byte_len > MAX_DATA_WIDTH || data_byte_len == 0) {
        DEBUG_ERROR("data_byte_len is error.(data_byte_len: %d)\n", data_byte_len);
        return -EINVAL;
    }

    for (i = 0; i < status_check->scratch_num; i++) {
        ret = logic_dev_status_check_write_func(status_check,
                             status_check->scratch_checks[i].reg,
                             status_check->scratch_checks[i].value,
                             data_byte_len);
        if (ret < 0) {
            return ret;
        }

        status_check->scratch_checks[i].initialized = true;
    }

    DEBUG_VERBOSE("STATUS_OK rw one time result check success.");
    return 0;
}

static int wb_logic_check_status_hw_init_with_type(struct device_status_check *status_check,
                                    uint32_t data_byte_len,
                                    uint32_t check_type)
{
    int ret;

    ret = 0;
    switch (check_type) {
    case STATUS_CHECK_SEU:
        break;
    case STATUS_CHECK_SELFTEST:
        break;
    case STATUS_CHECK_CRAM:
        break;
    case STATUS_CHECK_SCRATCH:
        ret = wb_logic_check_scratch_status_init(status_check, data_byte_len);
        if (ret < 0) {
            DEBUG_ERROR("wb_logic_check_scratch_status_init fail. ret %d\n", ret);
        }
        break;
    default:
        DEBUG_ERROR("not found init func, check type 0x%x\n", check_type);
        return -EINVAL;
    }

    return ret;
}


int wb_logic_check_status_hw_init(struct device_status_check *status_check,
                                     uint32_t data_byte_len)
{
    int ret;
    int check_type;
    int i;
    int fail_count;

    fail_count = 0;
    if (status_check == NULL) {
        DEBUG_ERROR("param is NULL\n");
        return -EINVAL;
    }

    ret = 0;
    for (i = 0; i < STATUS_CHECK_TYPE_MAX; i++) {
        check_type = STATUS_CHECK_BITS(i);
        if (status_check->status_check_type_bmp & check_type) {
            ret = wb_logic_check_status_hw_init_with_type(status_check, data_byte_len, check_type);
            if (ret < 0) {
                fail_count++;
                DEBUG_ERROR("wb_logic_check_status_hw_init_with_type init fail, check type 0x%x ret %d\n", check_type, ret);
            }
        } else {
            DEBUG_INFO("unsupport type skip hw init.(0x%x)\n", check_type);
        }
    }

    if (fail_count != 0) {
        return -EINVAL;
    }

    return 0;
}
EXPORT_SYMBOL_GPL(wb_logic_check_status_hw_init);

int wb_logic_dev_get_status(struct device_status_check *status_check,
                                     uint32_t data_byte_len,
                                     char *buf,
                                     uint32_t buf_len)
{
    int ret;
    u32 val;
    u32 check_type;
    u32 bit;
    uint32_t status_value;

    if (status_check == NULL || buf == NULL) {
        DEBUG_ERROR("param is NULL.(status_check: %p, buf: %p)\n", status_check, buf);
        return -EINVAL;
    }

    if (data_byte_len > MAX_DATA_WIDTH || data_byte_len == 0) {
        DEBUG_ERROR("data_byte_len is error.(data_byte_len: %d)\n", data_byte_len);
        return -EINVAL;
    }

    if (status_check->status_check_type_bmp == 0) {
        DEBUG_ERROR("unsupport dev status check.\n");
        return -EOPNOTSUPP;
    }

    val = 0;
    for (bit = 0; bit < STATUS_CHECK_TYPE_MAX; bit++) {
        check_type = STATUS_CHECK_BITS(bit);
        if (status_check->status_check_type_bmp & check_type) {
            ret = dev_status_check(status_check, data_byte_len, check_type, &status_value);
            if (ret != 0) {
                val |= check_type;
            }
        } else {
            DEBUG_INFO("unsupport type skip check.(0x%x)\n", check_type);
        }
    }

    status_check->dev_status = val;

    return snprintf(buf, buf_len, "0x%x\n", status_check->dev_status);
}
EXPORT_SYMBOL_GPL(wb_logic_dev_get_status);

uint32_t wb_logic_status_type_get_number(uint32_t type_bmp)
{
    int i;
    u32 check_type;
    uint8_t support_check_number;

    support_check_number = 0;
    for (i = 0; i < STATUS_CHECK_TYPE_MAX; i++) {
        check_type = STATUS_CHECK_BITS(i);

        if (type_bmp & check_type) {
            support_check_number++;
        }
    }

    return support_check_number;
}
EXPORT_SYMBOL_GPL(wb_logic_status_type_get_number);

static int wb_logic_get_check_type_bit(uint32_t type_bmp)
{
    int i;
    u32 check_type_tmp;
    uint8_t check_type;

    check_type = -1;
    for (i = 0; i < STATUS_CHECK_TYPE_MAX; i++) {
        check_type_tmp = STATUS_CHECK_BITS(i);

        if (type_bmp & check_type_tmp) {
            check_type = i;
            break;
        }
    }

    return check_type;
}

static int set_last_dev_status_with_type(struct device_status_check *status_check,
                                    uint32_t type,
                                    uint32_t status)
{
    int status_tmp;
    int bit_number;

    if (status == LOGIC_DEV_STATUS_OK) {
        status_tmp = LOGIC_DEV_STATUS_OK;
    } else {
        status_tmp = LOGIC_DEV_STATUS_NOT_OK;
    }

    bit_number = wb_logic_get_check_type_bit(type);
    status_check->type_dev_status[bit_number] = status_tmp;

    return status_tmp;
}

int wb_logic_dev_get_status_with_type(struct device_status_check *status_check,
                                     uint32_t data_byte_len,
                                     char *buf,
                                     uint32_t buf_len,
                                     uint32_t type)
{
    int ret;
    u32 val;
    int check_type_number;

    if (status_check == NULL || buf == NULL) {
        DEBUG_ERROR("param is NULL.(status_check: %p, buf: %p)\n", status_check, buf);
        return -EINVAL;
    }

    if (data_byte_len > MAX_DATA_WIDTH || data_byte_len == 0) {
        DEBUG_ERROR("data_byte_len is error.(data_byte_len: %d)\n", data_byte_len);

        return -EINVAL;
    }

    val = 0;
    /* Input parameter detection, only one type */
    check_type_number = wb_logic_status_type_get_number(type);
    if (check_type_number != 1) {
        DEBUG_ERROR("check_type_number not 1: 0x%x.\n", type);
        return -EINVAL;
    }

    if (status_check->status_check_type_bmp & type) {
        ret = dev_status_check_message(status_check, data_byte_len, type, buf, buf_len);
    } else {
        DEBUG_ERROR("unsupport dev status check.\n");
        return -EOPNOTSUPP;
    }

    set_last_dev_status_with_type(status_check, type, val);

    return ret;
}
EXPORT_SYMBOL_GPL(wb_logic_dev_get_status_with_type);

/* return: 0:not cache, -x: fail, +x: use cache status */
int wb_logic_get_cache_status_with_type(struct device_status_check *status_check,
                                                                uint32_t check_type,
                                                                uint32_t status_cache_ms,
                                                                char *buf,
                                                                uint32_t buf_len)
{
    int check_type_number;
    int bit_number;

    if (status_check == NULL ||  buf == NULL) {
        DEBUG_ERROR("param is NULL.(status_check: %p, buf: %p)\n", status_check, buf);
        return -EINVAL;
    }

    /* Input parameter detection, only one type */
    check_type_number = wb_logic_status_type_get_number(check_type);
    if (check_type_number != 1) {
        DEBUG_ERROR("check_type_number not 1: 0x%x.\n", check_type);
        return -EINVAL;
    }

    /* get bit_number 100% success */
    bit_number = wb_logic_get_check_type_bit(check_type);
    if (time_before(jiffies, status_check->type_last_jiffies[bit_number] + msecs_to_jiffies(status_cache_ms))) {
        /* Within the time range of status_cache_ms, directly return the last result */
        DEBUG_VERBOSE("time before last time %d ms return last status: %d\n",
            status_cache_ms, status_check->type_dev_status[bit_number]);
        return snprintf(buf, buf_len, "%u\n", status_check->type_dev_status[bit_number]);
    }

    status_check->type_last_jiffies[bit_number] = jiffies;

    return 0;
}
EXPORT_SYMBOL_GPL(wb_logic_get_cache_status_with_type);

static void set_seu_value_buf(int data_bus_width, int data_num, struct device_status_check *status_check, uint32_t *reg_data_buf)
{
    int i;
    int j;

    if (data_num == data_bus_width) {
        for (i = 0; i < status_check->seu_check_num; i++) {
            for (j = 0; j < data_bus_width; j++) {
                status_check->seu_checks[i].value[j] = reg_data_buf[j];
            }
        }
    } else {
        for (i = 0; i < status_check->seu_check_num; i++) {
            for (j = 0; j < data_bus_width; j++) {
                status_check->seu_checks[i].value[j] = reg_data_buf[j + i * data_bus_width];
            }
        }
    }
}

static void set_seu_mask_buf_def(int data_bus_width, struct device_status_check *status_check)
{
    int i;
    int j;

    for (i = 0; i < status_check->seu_check_num; i++) {
        for (j = 0; j < data_bus_width; j++) {
            status_check->seu_checks[i].mask[j] = 0xff;
        }
    }
}

static void set_seu_mask_buf(int data_bus_width, int data_num, struct device_status_check *status_check, uint32_t *reg_data_buf)
{
    int i;
    int j;

    if (data_num == data_bus_width) {
        for (i = 0; i < status_check->seu_check_num; i++) {
            for (j = 0; j < data_bus_width; j++) {
                status_check->seu_checks[i].mask[j] = reg_data_buf[j];
            }
        }
    } else {
        for (i = 0; i < status_check->seu_check_num; i++) {
            for (j = 0; j < data_bus_width; j++) {
                status_check->seu_checks[i].mask[j] = reg_data_buf[j + i * data_bus_width];
            }
        }
    }
}

static void set_cram_mask_buf_def(struct device_status_check *status_check)
{
    int i;

    for (i = 0; i < status_check->cram_checks.cram_err_addr_reg_num; i++) {
        status_check->cram_checks.cram_err_addr_reg_mask[i] = CRAM_ALL_BITS_SET;
    }
}

static void set_cram_mask_buf(int data_num, struct device_status_check *status_check, uint32_t *reg_data_buf)
{
    int i;

    if (data_num == CRAM_CONFIG_MASK_MIN_COUNT) {
        for (i = 0; i < status_check->cram_checks.cram_err_addr_reg_num; i++) {
            status_check->cram_checks.cram_err_addr_reg_mask[i] = reg_data_buf[0];
        }
    } else {
        for (i = 0; i < status_check->cram_checks.cram_err_addr_reg_num; i++) {
            status_check->cram_checks.cram_err_addr_reg_mask[i] = reg_data_buf[i];
        }
    }
}

static void set_selftest_value_buf(int data_bus_width, int data_num, struct device_status_check *status_check, uint32_t *reg_data_buf)
{
    int i;
    int j;

    if (data_num == data_bus_width) {
        for (i = 0; i < status_check->selftest_check_num; i++) {
            for (j = 0; j < data_bus_width; j++) {
                status_check->selftest_checks[i].write_value[j] = reg_data_buf[j];
            }
        }
    } else {
        for (i = 0; i < status_check->selftest_check_num; i++) {
            for (j = 0; j < data_bus_width; j++) {
                status_check->selftest_checks[i].write_value[j] = reg_data_buf[j + i * data_bus_width];
            }
        }
    }
}

static void wb_logic_check_type_status_and_jiffies_init(struct device_status_check *status_check)
{
    int i;

    status_check->last_jiffies = jiffies;
    status_check->dev_status = 0;

    for (i = 0; i < STATUS_CHECK_TYPE_MAX; i++) {
        status_check->type_last_jiffies[i] = jiffies;
        status_check->type_dev_status[i] = 0;
    }

    return;
}


static void set_scratch_value_buf(int data_bus_width, int data_num, struct device_status_check *status_check, uint32_t *reg_data_buf)
{
    int i;
    int j;

    if (data_num == data_bus_width) {
        for (i = 0; i < status_check->scratch_num; i++) {
            for (j = 0; j < data_bus_width; j++) {
                status_check->scratch_checks[i].value[j] = reg_data_buf[j];
            }
        }
    } else {
        for (i = 0; i < status_check->scratch_num; i++) {
            for (j = 0; j < data_bus_width; j++) {
                status_check->scratch_checks[i].value[j] = reg_data_buf[j + i * data_bus_width];
            }
        }
    }
}

static void cram_checks_value_init_def(struct device_status_check *status_check)
{
    status_check->cram_checks.cram_err_addr_reg_num = CRAM_ERR_ADDR_NUM_DEF;
    status_check->cram_checks.cram_err_addr_reg[0] = status_check->cram_checks.reg;
    status_check->cram_checks.cram_err_addr_reg_mask[0] = CRAM_ERR_ADDR_REG_MASK_DEF;
}

static int of_dev_seu_status_check_config_init(struct device *dev,
                                                struct device_status_check *status_check,
                                                uint32_t data_byte_len)
{
    int ret;
    uint32_t reg_buf[MAX_DATA_WIDTH];
    uint32_t reg_data_buf[MAX_REG_DATA_LEN];
    int data_bus_width;
    int data_num;
    int i;

    ret = of_property_read_u32(dev->of_node, "seu_check_num", &status_check->seu_check_num);
    if (ret != 0) {
        dev_err(dev, "Failed to get seu_check_num config, ret: %d\n", ret);
        return -ENXIO;
    }

    if ((status_check->seu_check_num == 0) || (status_check->seu_check_num > TEST_REG_MAX_NUM)) {
        dev_err(dev, "Invalid seu_check_num: %u\n", status_check->seu_check_num);
        return -EINVAL;
    }

    /* get reg */
    ret = of_property_read_u32_array(dev->of_node, "seu_reg", reg_buf, status_check->seu_check_num);
    if(ret != 0) {
        dev_err(dev, "Failed to get seu_reg config, ret: %d\n", ret);
        return -ENXIO;
    }

    for (i = 0; i < status_check->seu_check_num; i++) {
        status_check->seu_checks[i].reg = reg_buf[i];
    }

    data_bus_width = data_byte_len;
    /* get seu reg data */
    data_num = of_property_count_elems_of_size(dev->of_node, "seu_reg_data", sizeof(u32));
    if (data_num <= 0) {
        dev_err(dev, "Invalid seu_reg_data num: %u\n", data_num);
        return -EINVAL;
    }

    if (data_num != data_bus_width && data_num != (data_bus_width * status_check->seu_check_num)) { 
        dev_err(dev, "Fail seu_reg_data num: %u\n", data_num);
        return -EINVAL;
    }

    for (i = 0; i < data_num; i++) {
        ret = of_property_read_u32_index(dev->of_node, "seu_reg_data", i, &reg_data_buf[i]);
        if (ret < 0) {
            dev_err(dev, "Failed to get seu_reg_data config, ret: %d\n", ret);
            return -ENXIO;
        }
    }

    /* set seu value */
    set_seu_value_buf(data_bus_width, data_num, status_check, reg_data_buf);

    /* get seu reg data mask */
    data_num = of_property_count_elems_of_size(dev->of_node, "seu_reg_mask", sizeof(u32));
    if (data_num <= 0) {
        set_seu_mask_buf_def(data_bus_width, status_check);
        goto mask_out;
    }

    if (data_num != data_bus_width && data_num != (data_bus_width * status_check->seu_check_num)) { 
        dev_err(dev, "Fail seu_reg_mask num: %u\n", data_num);
        return -EINVAL;
    }

    for (i = 0; i < data_num; i++) {
        ret = of_property_read_u32_index(dev->of_node, "seu_reg_mask", i, &reg_data_buf[i]);
        if (ret < 0) {
            dev_err(dev, "Fail to get seu_reg_mask value: %d\n", ret);
            return -EINVAL;
        }
    }

    set_seu_mask_buf(data_bus_width, data_num, status_check, reg_data_buf);

mask_out:
    return 0;
}

static int of_dev_selftest_status_check_config_init(struct device *dev,
                                                struct device_status_check *status_check,
                                                uint32_t data_byte_len)
{
    int ret;
    uint32_t reg_buf[MAX_DATA_WIDTH];
    uint32_t reg_data_buf[MAX_REG_DATA_LEN];
    int data_bus_width;
    int data_num;
    int i;

    ret = of_property_read_u32(dev->of_node, "selftest_check_num", &status_check->selftest_check_num);
    if (ret != 0) {
        dev_err(dev, "Failed to get selftest_check_num config, ret: %d\n", ret);
        return -ENXIO;
    }
    
    if ((status_check->selftest_check_num == 0) || (status_check->selftest_check_num > TEST_REG_MAX_NUM)) {
        dev_err(dev, "Invalid selftest_check_num: %u\n", status_check->selftest_check_num);
        return -EINVAL;
    }
    
    ret = of_property_read_u32_array(dev->of_node, "selftest_reg", reg_buf, status_check->selftest_check_num);
    if(ret != 0) {
        dev_err(dev, "Failed to get selftest_reg config, ret: %d\n", ret);
        return -ENXIO;
    }

    for (i = 0; i < status_check->selftest_check_num; i++) {
        status_check->selftest_checks[i].reg = reg_buf[i];
    }

    ret = of_property_read_u32_array(dev->of_node, "selftest_check_mode", reg_buf, status_check->selftest_check_num);
    if(ret != 0) {
        /* defalut use 0 */
        for (i = 0; i < status_check->selftest_check_num; i++) {
            status_check->selftest_checks[i].check_mode = WB_LOGIC_NORMAL;
        }
    } else {
        for (i = 0; i < status_check->selftest_check_num; i++) {
            status_check->selftest_checks[i].check_mode = reg_buf[i];
        }
    }

    data_bus_width = data_byte_len;
    /* get selftest reg data */
    data_num = of_property_count_elems_of_size(dev->of_node, "selftest_reg_data", sizeof(u32));
    if (data_num <= 0) {
        dev_err(dev, "Invalid selftest_reg_data num: %u\n", data_num);
        return -EINVAL;
    }

    if (data_num != data_bus_width && data_num != (data_bus_width * status_check->selftest_check_num)) { 
        dev_err(dev, "Fail selftest_reg_data num: %u\n", data_num);
        return -EINVAL;
    }

    for (i = 0; i < data_num; i++) {
        ret = of_property_read_u32_index(dev->of_node, "selftest_reg_data", i, &reg_data_buf[i]);
        if (ret < 0) {
            dev_err(dev, "Failed to get selftest_reg_data config, ret: %d\n", ret);
            return -ENXIO;
        }
    }

    /* set selftest value */
    set_selftest_value_buf(data_bus_width, data_num, status_check, reg_data_buf);

    return 0;
}

static int of_dev_scratch_status_check_config_init(struct device *dev,
                                                struct device_status_check *status_check,
                                                uint32_t data_byte_len)
{
    int ret;
    uint32_t reg_buf[MAX_DATA_WIDTH];
    uint32_t reg_data_buf[MAX_REG_DATA_LEN];
    int data_bus_width;
    int data_num;
    int i;

    ret = of_property_read_u32(dev->of_node, "scratch_check_num", &status_check->scratch_num);
    if (ret != 0) {
        dev_err(dev, "Failed to get scratch_check_num config, ret: %d\n", ret);
        return -ENXIO;
    }
    
    if ((status_check->scratch_num == 0) || (status_check->scratch_num > TEST_REG_MAX_NUM)) {
        dev_err(dev, "Invalid scratch_num: %u\n", status_check->scratch_num);
        return -EINVAL;
    }
    
    ret = of_property_read_u32_array(dev->of_node, "scratch_reg", reg_buf, status_check->scratch_num);
    if(ret != 0) {
        dev_err(dev, "Failed to get scratch_reg config, ret: %d\n", ret);
        return -ENXIO;
    }

    for (i = 0; i < status_check->scratch_num; i++) {
        status_check->scratch_checks[i].reg = reg_buf[i];
    }

    ret = of_property_read_u32_array(dev->of_node, "scratch_check_mode", reg_buf, status_check->scratch_num);
    if(ret != 0) {
        /* defalut use 0 */
        for (i = 0; i < status_check->scratch_num; i++) {
            status_check->scratch_checks[i].check_mode = WB_LOGIC_NORMAL;
        }
    } else {
        for (i = 0; i < status_check->scratch_num; i++) {
            status_check->scratch_checks[i].check_mode = reg_buf[i];
        }
    }

    data_bus_width = data_byte_len;
    /* get selftest reg data */
    data_num = of_property_count_elems_of_size(dev->of_node, "scratch_reg_data", sizeof(u32));
    if (data_num <= 0) {
        dev_err(dev, "Invalid scratch_reg_data num: %u\n", data_num);
        return -EINVAL;
    }
    
    if (data_num != data_bus_width && data_num != (data_bus_width * status_check->scratch_num)) { 
        dev_err(dev, "Fail scratch_reg_data num: %u\n", data_num);
        return -EINVAL;
    }

    for (i = 0; i < data_num; i++) {
        ret = of_property_read_u32_index(dev->of_node, "scratch_reg_data", i, &reg_data_buf[i]);
        if (ret < 0) {
            dev_err(dev, "Failed to get scratch_reg_data config, ret: %d\n", ret);
            return -ENXIO;
        }
    }

    /* set scratch value */
    set_scratch_value_buf(data_bus_width, data_num, status_check, reg_data_buf);

    return 0;
}

static int of_dev_cram_status_check_config_init(struct device *dev,
                                                struct device_status_check *status_check,
                                                uint32_t data_byte_len)
{
    int ret;
    int i;
    int data_num;
    int data_bus_width;
    uint32_t reg_data_buf[MAX_REG_DATA_LEN];

    ret = of_property_read_u32(dev->of_node, "cram_reg", &status_check->cram_checks.reg);
    if(ret != 0) {
        dev_err(dev, "Failed to get cram_reg config, ret: %d\n", ret);
        return -ENXIO;
    }

    ret = of_property_read_u32(dev->of_node, "cram_err_addr_reg_num", &status_check->cram_checks.cram_err_addr_reg_num);
    if (ret == 0) {
        if ((status_check->cram_checks.cram_err_addr_reg_num == 0) || (status_check->cram_checks.cram_err_addr_reg_num > TEST_REG_MAX_NUM)) {
            dev_err(dev, "Invalid cram_err_addr_reg_num: %u\n", status_check->cram_checks.cram_err_addr_reg_num);
            return -EINVAL;
        }

        ret = of_property_read_u32_array(dev->of_node, "cram_err_addr_reg", status_check->cram_checks.cram_err_addr_reg, status_check->cram_checks.cram_err_addr_reg_num);
        if(ret != 0) {
            dev_err(dev, "Failed to get cram_err_addr_reg config, ret: %d\n", ret);
            return -ENXIO;
        }

        /* get seu reg data mask */
        data_bus_width = data_byte_len;
        data_num = of_property_count_elems_of_size(dev->of_node, "cram_err_addr_reg_mask", sizeof(u32));
        if (data_num <= 0) {
            /* use def mask CRAM_ALL_BITS_SET */
            set_cram_mask_buf_def(status_check);
        } else {
            /* must config one mask or everyone mask */
            if (data_num != CRAM_CONFIG_MASK_MIN_COUNT && data_num != status_check->cram_checks.cram_err_addr_reg_num) { 
                dev_err(dev, "Fail cram_err_addr_reg_mask num: %u\n", data_num);
                return -EINVAL;
            }

            for (i = 0; i < data_num; i++) {
                ret = of_property_read_u32_index(dev->of_node, "cram_err_addr_reg_mask", i, &reg_data_buf[i]);
                if (ret < 0) {
                    dev_err(dev, "Fail to get seu_reg_mask value: %d\n", ret);
                    return -EINVAL;
                }
            }

            set_cram_mask_buf(data_num, status_check, reg_data_buf);
        }
    } else {
        cram_checks_value_init_def(status_check);
        DEBUG_VERBOSE("use def value.\n");
    }

    return 0;
}

static int of_dev_status_check_one_type_init(struct device *dev,
                                    struct device_status_check *status_check,
                                    uint32_t data_byte_len,
                                    uint32_t check_type)
{
    int ret;

    switch (check_type) {
    case STATUS_CHECK_SEU:
        ret = of_dev_seu_status_check_config_init(dev, status_check, data_byte_len);
        if (ret != 0) {
            dev_err(dev, "seu_status_check_config_init fail: %d\n", ret);
            return ret;
        }
        break;
    case STATUS_CHECK_SELFTEST:
        ret = of_dev_selftest_status_check_config_init(dev, status_check, data_byte_len);
        if (ret != 0) {
            dev_err(dev, "selftest_status_check_config_init fail: %d\n", ret);
            return ret;
        }
        break;
    case STATUS_CHECK_SCRATCH:
        ret = of_dev_scratch_status_check_config_init(dev, status_check, data_byte_len);
        if (ret != 0) {
            dev_err(dev, "scratch_status_check_config_init fail: %d\n", ret);
            return ret;
        }
        break;
    case STATUS_CHECK_CRAM:
        ret = of_dev_cram_status_check_config_init(dev, status_check, data_byte_len);
        if (ret != 0) {
            dev_err(dev, "dev_cram_status_check_config_init fail: %d\n", ret);
            return ret;
        }
        break;
    default:
        dev_err(dev, "not found init func, check type 0x%x\n", check_type);
        return -EINVAL;
    }

    return 0;
}


static int of_dev_status_check_init(struct device *dev,
                                    struct device_status_check *status_check,
                                    uint32_t data_byte_len)
{
    int ret;
    int i;
    u32 check_type;

    for (i = 0; i < STATUS_CHECK_TYPE_MAX; i++) {
        check_type = STATUS_CHECK_BITS(i);
        if (status_check->status_check_type_bmp & check_type) {
            ret = of_dev_status_check_one_type_init(dev, status_check, data_byte_len, check_type);
            if (ret != 0) {
                dev_err(dev, "of_dev_status_check_one_type_init fail, check_type 0x%x ret %d\n", check_type, ret);
                return ret;
            }
        } else {
            DEBUG_INFO("unsupport type skip init.(0x%x)\n", check_type);
        }
    }

    return 0;
}

int of_dev_status_check_config_init(struct device *dev,
                                                struct device_status_check *status_check,
                                                uint32_t data_byte_len,
                                                device_func_write read,
                                                device_func_write write,
                                                const char *dev_name)
{
    int ret;
    uint32_t type;
    int check_type_number;

    if (dev == NULL) {
        PRINT_ERROR("dev_status_check_config_init fail, dev is NULL\n");
        return -EINVAL;
    }

    if (status_check == NULL || dev_name == NULL) {
        dev_err(dev, "param is NULL.(status_check: %p, dev_name = %p)\n", status_check, dev_name);
        return -EINVAL;
    }

    if (data_byte_len > MAX_DATA_WIDTH || data_byte_len == 0) {
        dev_err(dev, "data_byte_len is error.(data_byte_len: %d)\n", data_byte_len);
        return -EINVAL;
    }

    snprintf(status_check->dev_name, sizeof(status_check->dev_name),"/dev/%s", dev_name);

    ret = of_property_read_u32(dev->of_node, "status_check_type_bmp", &status_check->status_check_type_bmp);
    if (ret == 0) {
        type = status_check->status_check_type_bmp;
        /* Check dts configuration status_check_type_bmp valid */
        check_type_number = wb_logic_status_type_get_number(type);
        if (check_type_number == 0) {
            dev_err(dev, "Invalid status_check_type: %u\n", type);
            return -EINVAL;
        }

        if (read == NULL || write == NULL) {
            dev_err(dev, "Invalid read or write func\n");
            return -EINVAL;
        }

        status_check->dev_bus_ops.read = read;
        status_check->dev_bus_ops.write = write;
        wb_logic_check_type_status_and_jiffies_init(status_check);
        DEBUG_VERBOSE("status_check_type: %u\n", status_check->status_check_type_bmp);
        ret = of_dev_status_check_init(dev, status_check, data_byte_len);
        if (ret != 0) {
            dev_err(dev, "of_dev_status_check_init fail: %d\n", ret);
            return ret;
        }
    } else {
        DEBUG_VERBOSE("not support dev status check sysfs.\n");
    }

    return 0;
}
EXPORT_SYMBOL_GPL(of_dev_status_check_config_init);

static int platform_dev_seu_status_check_config_init(struct device *dev,
                                                struct device_status_check *status_check,
                                                uint32_t data_byte_len,
                                                struct device_status_check *status_check_data)
{
    int data_bus_width;
    int i;
    int j;

    status_check->seu_check_num = status_check_data->seu_check_num;
    if ((status_check->seu_check_num == 0) || (status_check->seu_check_num > TEST_REG_MAX_NUM)) {
        dev_err(dev, "Invalid seu_check_num: %u\n", status_check->seu_check_num);
        return -EINVAL;
    }

    data_bus_width = data_byte_len;
    for (i = 0; i < status_check->seu_check_num; i++) {
        status_check->seu_checks[i].reg = status_check_data->seu_checks[i].reg;
        for (j = 0; j < data_bus_width; j++) {
            status_check->seu_checks[i].value[j] = status_check_data->seu_checks[i].value[j];
            if (status_check_data->seu_checks[i].mask[j] == 0) {
                status_check->seu_checks[i].mask[j] = 0xff;
            } else {
                status_check->seu_checks[i].mask[j] = status_check_data->seu_checks[i].mask[j];
            }
        }
    }

    return 0;
}

static int platform_dev_selftest_status_check_config_init(struct device *dev,
                                                struct device_status_check *status_check,
                                                uint32_t data_byte_len,
                                                struct device_status_check *status_check_data)
{
    int data_bus_width;
    int i;
    int j;

    status_check->selftest_check_num = status_check_data->selftest_check_num;
    if ((status_check->selftest_check_num == 0) || (status_check->selftest_check_num > TEST_REG_MAX_NUM)) {
        dev_err(dev, "Invalid selftest_check_num: %u\n", status_check->selftest_check_num);
        return -EINVAL;
    }

    data_bus_width = data_byte_len;
    for (i = 0; i < status_check->selftest_check_num; i++) {
        status_check->selftest_checks[i].reg = status_check_data->selftest_checks[i].reg;
        status_check->selftest_checks[i].check_mode = status_check_data->selftest_checks[i].check_mode;
        for (j = 0; j < data_bus_width; j++) {
            status_check->selftest_checks[i].write_value[j] = status_check_data->selftest_checks[i].write_value[j];
        }
    }

    return 0;
}

static int platform_dev_scratch_status_check_config_init(struct device *dev,
                                                struct device_status_check *status_check,
                                                uint32_t data_byte_len,
                                                struct device_status_check *status_check_data)
{
    int data_bus_width;
    int i;
    int j;

    status_check->scratch_num = status_check_data->scratch_num;
    if ((status_check->scratch_num == 0) || (status_check->scratch_num > TEST_REG_MAX_NUM)) {
        dev_err(dev, "Invalid scratch_num: %u\n", status_check->scratch_num);
        return -EINVAL;
    }

    data_bus_width = data_byte_len;
    for (i = 0; i < status_check->scratch_num; i++) {
        status_check->scratch_checks[i].reg = status_check_data->scratch_checks[i].reg;
        status_check->scratch_checks[i].check_mode = status_check_data->scratch_checks[i].check_mode;
        for (j = 0; j < data_bus_width; j++) {
            status_check->scratch_checks[i].value[j] = status_check_data->scratch_checks[i].value[j];
        }
    }

    return 0;
}

static int platform_dev_cram_status_check_config_init(struct device *dev,
                                                struct device_status_check *status_check,
                                                uint32_t data_byte_len,
                                                struct device_status_check *status_check_data)
{
    int i;

    status_check->cram_checks.reg = status_check_data->cram_checks.reg;
    status_check->cram_checks.cram_err_addr_reg_num = status_check_data->cram_checks.cram_err_addr_reg_num;
    if ((status_check->cram_checks.cram_err_addr_reg_num == 0) || (status_check->cram_checks.cram_err_addr_reg_num > TEST_REG_MAX_NUM)) {
        /* use def value */
        cram_checks_value_init_def(status_check);
        return 0;
    }

    for (i = 0; i < status_check->cram_checks.cram_err_addr_reg_num; i++) {
        status_check->cram_checks.cram_err_addr_reg[i] = status_check_data->cram_checks.cram_err_addr_reg[i];
        
        if (status_check_data->cram_checks.cram_err_addr_reg_mask[i] == 0) {
            status_check->cram_checks.cram_err_addr_reg_mask[i] = CRAM_ALL_BITS_SET;
        } else {
            status_check->cram_checks.cram_err_addr_reg_mask[i] = status_check_data->cram_checks.cram_err_addr_reg_mask[i];
        }
    }

    return 0;
}

static int platform_dev_status_check_one_type_init(struct device *dev,
                                                                struct device_status_check *status_check,
                                                                uint32_t data_byte_len,
                                                                struct device_status_check *status_check_data,
                                                                uint32_t check_type)
{
    int ret;

    switch (check_type) {
    case STATUS_CHECK_SEU:
        ret = platform_dev_seu_status_check_config_init(dev, status_check, data_byte_len, status_check_data);
        if (ret != 0) {
            dev_err(dev, "platform seu_status_check_config_init fail: %d\n", ret);
            return ret;
        }
        break;
    case STATUS_CHECK_SELFTEST:
        ret = platform_dev_selftest_status_check_config_init(dev, status_check, data_byte_len, status_check_data);
        if (ret != 0) {
            dev_err(dev, "platform selftest_status_check_config_init fail: %d\n", ret);
            return ret;
        }
        break;
    case STATUS_CHECK_SCRATCH:
        ret = platform_dev_scratch_status_check_config_init(dev, status_check, data_byte_len, status_check_data);
        if (ret != 0) {
            dev_err(dev, "platform scratch_status_check_config_init fail: %d\n", ret);
            return ret;
        }
        break;
    case STATUS_CHECK_CRAM:
        ret = platform_dev_cram_status_check_config_init(dev, status_check, data_byte_len, status_check_data);
        if (ret != 0) {
            dev_err(dev, "platform cram_status_check_config_init fail: %d\n", ret);
            return ret;
        }
        break;

    default:
        dev_err(dev, "not found init func, check type 0x%x\n", check_type);
        return -EINVAL;
    }

    return 0;
}


static int platform_dev_status_check_init(struct device *dev,
                                                    struct device_status_check *status_check,
                                                    uint32_t data_byte_len,
                                                    struct device_status_check *status_check_data)
{
    int ret;
    int i;
    u32 check_type;

    for (i = 0; i < STATUS_CHECK_TYPE_MAX; i++) {
        check_type = STATUS_CHECK_BITS(i);
        if (status_check->status_check_type_bmp & check_type) {
            ret = platform_dev_status_check_one_type_init(dev, status_check, data_byte_len, status_check_data, check_type);
            if (ret != 0) {
                dev_err(dev, "platform_dev_status_check_one_type_init fail, check_type 0x%x ret %d\n", check_type, ret);
                return ret;
            }
        } else {
            DEBUG_INFO("unsupport type skip init.(0x%x)\n", check_type);
        }
    }

    return 0;
}


int platform_dev_status_check_config_init(struct device *dev,
                                                        struct device_status_check *status_check,
                                                        uint32_t data_byte_len,
                                                        struct device_status_check *status_check_data,
                                                        device_func_write read,
                                                        device_func_write write,
                                                        const char *dev_name)
{
    int ret;
    int check_type_number;

    if (dev == NULL) {
        PRINT_ERROR("dev_status_check_config_init fail, dev is NULL\n");
        return -EINVAL;
    }

    if (status_check == NULL || status_check_data == NULL) {
        dev_err(dev, "param is NULL.(status_check: %p, status_check_data: %p)\n", status_check, status_check_data);
        return -EINVAL;
    }

    if (data_byte_len > MAX_DATA_WIDTH || data_byte_len == 0) {
        dev_err(dev, "data_byte_len is error.(data_byte_len: %d)\n", data_byte_len);
        return -EINVAL;
    }

    snprintf(status_check->dev_name, sizeof(status_check->dev_name), "/dev/%s", dev_name);

    status_check->status_check_type_bmp = status_check_data->status_check_type_bmp;
    /* Check platform configuration status_check_type_bmp valid */
    check_type_number = wb_logic_status_type_get_number(status_check->status_check_type_bmp);
    if (check_type_number == 0) {
        DEBUG_INFO("Invalid status_check_type: %u\n", status_check->status_check_type_bmp);
        return 0;
    }

    if (read == NULL || write == NULL) {
        dev_err(dev, "Invalid read or write func\n");
        return -EINVAL;
    }

    status_check->dev_bus_ops.read = read;
    status_check->dev_bus_ops.write = write;
    wb_logic_check_type_status_and_jiffies_init(status_check);

    DEBUG_VERBOSE("status_check_type: %u\n", status_check->status_check_type_bmp);
    ret = platform_dev_status_check_init(dev, status_check, data_byte_len, status_check_data);
    if (ret != 0) {
        dev_err(dev, "platform dev_status_check_init fail: %d\n", ret);
        return ret;
    }

    return 0;
}
EXPORT_SYMBOL_GPL(platform_dev_status_check_config_init);

static int wb_logic_scratch_clear_status(struct device_status_check *status_check,
                                     uint32_t data_byte_len)
{
    int ret;

    ret = wb_logic_check_scratch_status_init(status_check, data_byte_len);
    if (ret < 0) {
        DEBUG_ERROR("scratch status clear fail. ret %d\n", ret);
    }

    return ret;
}

static int wb_logic_clear_status_with_type(struct device_status_check *status_check,
                                    uint32_t data_byte_len,
                                    uint32_t check_type)
{
    int ret;

    ret = 0;
    switch (check_type) {
    case STATUS_CHECK_SEU:
        break;
    case STATUS_CHECK_SELFTEST:
        break;
    case STATUS_CHECK_CRAM:
        break;
    case STATUS_CHECK_SCRATCH:
        ret = wb_logic_scratch_clear_status(status_check, data_byte_len);
        if (ret < 0) {
            DEBUG_ERROR("wb_logic_scratch_clear_status fail. ret %d\n", ret);
        }
        break;
    default:
        DEBUG_ERROR("not found clear func, check type 0x%x\n", check_type);
        return -EINVAL;
    }

    return ret;
}

int wb_logic_clear_status(struct device_status_check *status_check,
                                     uint32_t data_byte_len,
                                     uint32_t type_bmp)
{
    int ret;
    int check_type;
    int i;
    int fail_count;

    fail_count = 0;
    if (status_check == NULL) {
        DEBUG_ERROR("param is NULL\n");
        return -EINVAL;
    }

    if (wb_logic_status_type_get_number(type_bmp) == 0) {
        DEBUG_ERROR("check_type_number is 0\n");
        return -EINVAL;
    }

    if (data_byte_len > MAX_DATA_WIDTH || data_byte_len == 0) {
        DEBUG_ERROR("data_byte_len is error.(data_byte_len: %d)\n", data_byte_len);
        return -EINVAL;
    }


    ret = 0;
    for (i = 0; i < STATUS_CHECK_TYPE_MAX; i++) {
        check_type = STATUS_CHECK_BITS(i);
        if ((type_bmp & check_type) == 0) {
            DEBUG_INFO("do nothing. (type_bmp 0x%x, check_type 0x%x)\n", type_bmp, check_type);
            continue;
        }

        if (status_check->status_check_type_bmp & check_type) {
            ret = wb_logic_clear_status_with_type(status_check, data_byte_len, check_type);
            if (ret < 0) {
                fail_count++;
                DEBUG_ERROR("wb_logic_clear_status_with_type fail, check type 0x%x ret %d\n", check_type, ret);
            }
        } else {
            DEBUG_INFO("unsupport type skip hw init.(0x%x)\n", check_type);
        }
    }

    if (fail_count != 0) {
        return -EINVAL;
    }

    return 0;
}
EXPORT_SYMBOL_GPL(wb_logic_clear_status);

void sleep_by_lock_mode(int sleep_time, uint32_t lock_mode)
{
    if (lock_mode == WB_MUTEX_LOCK_MODE) {
        usleep_range(sleep_time, sleep_time + 1);
    } else {
        udelay(sleep_time);
    }
}
EXPORT_SYMBOL_GPL(sleep_by_lock_mode);

int wb_logic_lock_mode_check(uint32_t lock_mode, uint32_t func_mode)
{
    if(lock_mode == WB_SPIN_LOCK_MODE) {
        switch (func_mode){
            case SYMBOL_I2C_DEV_MODE:
            case FILE_MODE:
            case SYMBOL_SPI_DEV_MODE:
            case SYMBOL_INDIRECT_DEV_MODE:
            case SYMBOL_PLATFORM_I2C_DEV_MODE:
                DEBUG_ERROR("func mode not support spin lock mode.\n");
                return -EINVAL;
            default:
                break;
        }
    }

    return 0;
}
EXPORT_SYMBOL_GPL(wb_logic_lock_mode_check);

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
