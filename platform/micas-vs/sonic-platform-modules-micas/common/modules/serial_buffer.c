#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>  
#include <linux/platform_device.h>  
#include <linux/hwmon-sysfs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/cdev.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>

#define MIN_BUFFER_INDEX           (0)
#define MAX_BUFFER_INDEX           (47)
#define SERIAL_UART_NUM            (48)
#define NAME_SIZE                  (16)
#define SUPPORTED_BUFFER_NUM       (128)

typedef enum {
    SERIAL_BUFFER_EMPTY = 0,
    SERIAL_BUFFER_BUFFERING,
    SERIAL_BUFFER_FULL,
} buffer_status_t;

typedef enum {
    SERIAL_BUFFER_NOT_ALLOCCED = 0,
    SERIAL_BUFFER_ALLOCCED,
} serial_buf_status_t;

struct mc_circ_ring {
    unsigned char *buf;
    int head;
    int tail;
    int status;
    int index;
    int allocced;
    int buffer_size;
    struct mutex buf_lock;
};

struct dentry *tty_debugfs;
struct mc_circ_ring serial_buffer[SUPPORTED_BUFFER_NUM];

static struct mc_circ_ring* serial_get_buffer(int index)
{
    return &serial_buffer[index];
}

int serial_buffer_if_allocced(int index)
{
    struct mc_circ_ring *buf_ring;

    buf_ring = serial_get_buffer(index);

    return buf_ring->allocced;
}
EXPORT_SYMBOL(serial_buffer_if_allocced);

int serial_buffer_alloc(int index, int buffer_size)
{
    struct mc_circ_ring *buf_ring;

    if (index > MAX_BUFFER_INDEX || index < MIN_BUFFER_INDEX) {
        return -ENXIO;
    }
    buf_ring = serial_get_buffer(index);

    buf_ring->buffer_size = buffer_size;
    buf_ring->allocced = SERIAL_BUFFER_ALLOCCED;
    buf_ring->buf = kzalloc(sizeof(unsigned char) * buffer_size, GFP_KERNEL);
    if (!buf_ring->buf) {
        return  -ENOMEM;
    }

    return 0;
}
EXPORT_SYMBOL(serial_buffer_alloc);

int serial_buffer_release(int index)
{
    struct mc_circ_ring *buf_ring;

    if (index > MAX_BUFFER_INDEX || index < MIN_BUFFER_INDEX) {
        return -ENXIO;
    }
    buf_ring = serial_get_buffer(index);
    if (buf_ring->buf != NULL) {
        buf_ring->allocced = SERIAL_BUFFER_NOT_ALLOCCED;
        kfree(buf_ring->buf);
        buf_ring->buf = NULL;
    }

    return 0;
}
EXPORT_SYMBOL(serial_buffer_release);

int serial_buffer_realloc(int index, int new_buffer_size)
{
    unsigned char *tmp_buf;
    int cp_size;
    struct mc_circ_ring *buf_ring;

    if (index > MAX_BUFFER_INDEX || index < MIN_BUFFER_INDEX) {
        return -ENXIO;
    }

    buf_ring = serial_get_buffer(index);
    mutex_lock(&buf_ring->buf_lock);
    tmp_buf = kzalloc(sizeof(unsigned char) * buf_ring->buffer_size, GFP_KERNEL);
    if (!tmp_buf) {
        return -ENOMEM;
    }
    memcpy(tmp_buf, buf_ring->buf, buf_ring->buffer_size);
    kfree(buf_ring->buf);
    buf_ring->buf = NULL;
    buf_ring->buf = kzalloc(sizeof(unsigned char) * new_buffer_size, GFP_KERNEL);
    if (!buf_ring->buf) {
        kfree(tmp_buf);
        tmp_buf = NULL;
        mutex_unlock(&buf_ring->buf_lock);
        return -ENOMEM;
    }
    
    buf_ring->tail = (buf_ring->tail > new_buffer_size) ? new_buffer_size - 1 : buf_ring->tail;
    buf_ring->head = (buf_ring->head > new_buffer_size) ? new_buffer_size - 1 : buf_ring->head;
    cp_size = (new_buffer_size < buf_ring->buffer_size) ? new_buffer_size : buf_ring->buffer_size;
    buf_ring->buffer_size = new_buffer_size;
    memcpy(buf_ring->buf, tmp_buf, cp_size);
    mutex_unlock(&buf_ring->buf_lock);
    kfree(tmp_buf);
    tmp_buf = NULL;

    return 0;
}
EXPORT_SYMBOL(serial_buffer_realloc);

int serial_buffer_write(int index, unsigned char ch)
{
    struct mc_circ_ring *buf_ring;

    if (index > MAX_BUFFER_INDEX || index < MIN_BUFFER_INDEX) {
        return -ENXIO;
    }

    buf_ring = serial_get_buffer(index);
    mutex_lock(&buf_ring->buf_lock);
    buf_ring->buf[buf_ring->tail] = ch;
    buf_ring->tail = (buf_ring->tail + 1) % (buf_ring->buffer_size);
    buf_ring->status = (buf_ring->status == SERIAL_BUFFER_FULL) ? SERIAL_BUFFER_FULL :
        (buf_ring->tail == buf_ring->head) ? SERIAL_BUFFER_FULL : SERIAL_BUFFER_BUFFERING;
    mutex_unlock(&buf_ring->buf_lock);

    return 0;
}
EXPORT_SYMBOL(serial_buffer_write);

int serial_buffer_read(int index, unsigned char *ch)
{
    struct mc_circ_ring *buf_ring;
    if (index > MAX_BUFFER_INDEX || index < MIN_BUFFER_INDEX) {
        return -ENXIO;
    }

    buf_ring = serial_get_buffer(index);
    mutex_lock(&buf_ring->buf_lock);
    buf_ring->head = (buf_ring->status == SERIAL_BUFFER_FULL) ?
        buf_ring->tail : buf_ring->head;
    *ch = buf_ring->buf[buf_ring->head];
    buf_ring->buf[buf_ring->head] = 0;
    buf_ring->head = (buf_ring->head + 1) % (buf_ring->buffer_size);
    buf_ring->status = (buf_ring->tail == buf_ring->head) ? SERIAL_BUFFER_EMPTY : SERIAL_BUFFER_BUFFERING;
    mutex_unlock(&buf_ring->buf_lock);

    return 0;
}
EXPORT_SYMBOL(serial_buffer_read);

int serial_get_buffer_status(int index)
{
    struct mc_circ_ring *buf_ring;
    int ret;

    if (index > MAX_BUFFER_INDEX || index < MIN_BUFFER_INDEX) {
        return -ENXIO;
    }
    buf_ring = serial_get_buffer(index);
    mutex_lock(&buf_ring->buf_lock);
    ret = buf_ring->status;
    mutex_unlock(&buf_ring->buf_lock);
    return ret;
}
EXPORT_SYMBOL(serial_get_buffer_status);

int serial_get_buffer_size(int index)
{
    struct mc_circ_ring *buf_ring;
    int ret;

    if (index > MAX_BUFFER_INDEX || index < MIN_BUFFER_INDEX) {
        return -ENXIO;
    }
    buf_ring = serial_get_buffer(index);
    mutex_lock(&buf_ring->buf_lock);
    ret = buf_ring->buffer_size;
    mutex_unlock(&buf_ring->buf_lock);
    return ret;
}
EXPORT_SYMBOL(serial_get_buffer_size);

static int serial_show_buffer(struct seq_file *m, void *v)
{
    char *tmp_buf;
    int *index;
    struct mc_circ_ring *buf_ring;
    int head, tail, cur_size;

    index = m->private;

    if (*index > MAX_BUFFER_INDEX || *index < MIN_BUFFER_INDEX) {
        return -ENXIO;
    }
    buf_ring = serial_get_buffer(*index);
    mutex_lock(&buf_ring->buf_lock);
    head = buf_ring->head;
    tail = buf_ring->tail;
    if (buf_ring->status == SERIAL_BUFFER_EMPTY) {
        mutex_unlock(&buf_ring->buf_lock);
        goto no_data;
    }
    cur_size = (buf_ring->status == SERIAL_BUFFER_FULL) ? buf_ring->buffer_size : 
        (tail > head) ? (tail - head) : buf_ring->buffer_size + (tail - head);
    tmp_buf = kzalloc(sizeof(unsigned char) * cur_size, GFP_KERNEL);
    if (!tmp_buf) {
        mutex_unlock(&buf_ring->buf_lock);
        return -ENOMEM;
    }
    memcpy(tmp_buf, buf_ring->buf + head, cur_size);
    mutex_unlock(&buf_ring->buf_lock);
    seq_printf(m, "%s", tmp_buf);
    kfree(tmp_buf);
    tmp_buf = NULL;
no_data:

    return 0;
}

static int serial_buffer_debugfs_open(struct inode *inode, struct file *file)
{
    int *index;

    index = (int *)inode->i_private;
    return single_open(file, serial_show_buffer, index);
}

static const struct file_operations serial_buffer_debugfs_ops = {
    .owner          = THIS_MODULE,
    .open           = serial_buffer_debugfs_open,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static int serial_buffer_init(void)
{
    int i;
    char tmp[NAME_SIZE];
    struct mc_circ_ring *buf_ring;

    tty_debugfs = debugfs_create_dir("ttyMC_buffer", NULL);
    for (i = 0; i < SERIAL_UART_NUM; i++) {
        /* buffer init */
        buf_ring = serial_get_buffer(i);
        mutex_init(&buf_ring->buf_lock);
        buf_ring->head = 0;
        buf_ring->tail = 0;
        buf_ring->status = SERIAL_BUFFER_EMPTY;
        buf_ring->buf = NULL;
        buf_ring->buffer_size = 0;
        buf_ring->allocced = SERIAL_BUFFER_NOT_ALLOCCED;
        buf_ring->index = i;
        /* buffer sysfs init */
        snprintf(tmp, NAME_SIZE, "ttyMC_buf%d", i);
        debugfs_create_file(tmp, S_IRUGO, tty_debugfs, &buf_ring->index, &serial_buffer_debugfs_ops);
    }
    return 0;
}

static void serial_buffer_exit(void)
{
    int i;
    struct mc_circ_ring *buf_ring;

    for (i = 0; i < SERIAL_UART_NUM; i++) {
        buf_ring = serial_get_buffer(i);
        if (buf_ring->buf) {
            kfree(buf_ring->buf);
            buf_ring->buf = NULL;
        }
    }

    debugfs_remove_recursive(tty_debugfs);
    tty_debugfs = NULL;
}

static int __init serial_buffer_module_init(void)
{
    return serial_buffer_init();
}

static void __exit serial_buffer_module_exit(void)
{
    serial_buffer_exit();
    return;
}

module_init(serial_buffer_module_init);
module_exit(serial_buffer_module_exit);
MODULE_AUTHOR("Micas <rd@micas.com.cn>");
MODULE_DESCRIPTION("Serial Buffer Driver for mc-serial");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");
