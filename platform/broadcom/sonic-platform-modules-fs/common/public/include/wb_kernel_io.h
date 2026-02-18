#ifndef __WB_KERNEL_IO_H__
#define __WB_KERNEL_IO_H__

#include <linux/fs.h>
#include <linux/uio.h>
#include "wb_logic_dev_common.h"

/*
 * wb_io_fn_t - Function pointer type for read/write operations
 * @file: pointer to struct file representing the file
 * @buf: buffer to read from or write to
 * @count: number of bytes to read/write
 * @pos: pointer to file position offset
 *
 * Returns number of bytes read/written on success, negative error code on failure.
 */
typedef ssize_t (*wb_io_fn_t)(struct file *, char *, size_t, loff_t *);

/*
 * wb_iov_iter_read - Read data from file into iov_iter using buffer
 * @iocb: pointer to kiocb structure containing file and position info
 * @to: iov_iter representing destination of data (user or kernel space)
 * @rw_func: callback function to perform actual read operation
 *
 * Returns total bytes read on success, or negative error code on failure.
 */
static inline ssize_t wb_iov_iter_read(struct kiocb *iocb, struct iov_iter *to, wb_io_fn_t rw_func)
{
    char buffer[MAX_RW_LEN];
    ssize_t ret;
    size_t read_size;

    read_size = iov_iter_count(to);
    if (read_size > MAX_RW_LEN) {
        read_size = MAX_RW_LEN;
    }

    /* Call user-provided read function */
    ret = rw_func(iocb->ki_filp, buffer, read_size, &iocb->ki_pos);
    if (ret <= 0) {
        return ret;
    }

    return copy_to_iter(buffer, ret, to);
}

/*
 * wb_iov_iter_write - Write data from iov_iter to file using intermediate buffer
 * @iocb: pointer to kiocb structure containing file and position info
 * @from: iov_iter representing source of data (user or kernel space)
 * @rw_func: callback function to perform actual write operation
 *
 * Returns total bytes written on success, or negative error code on failure.
 */
static inline ssize_t wb_iov_iter_write(struct kiocb *iocb, struct iov_iter *from, 
                        wb_io_fn_t rw_func)
{
    char buffer[MAX_RW_LEN];
    ssize_t ret;
    size_t write_size;

    write_size = iov_iter_count(from);
    if (write_size > MAX_RW_LEN) {
        write_size = MAX_RW_LEN;
    }

    /*
     * Copy data from iov_iter to internal buffer.
     * copy_from_iter() returns bytes NOT copied.
     */
    ret = copy_from_iter(buffer, write_size, from);
    if (ret < 0) {
        return -EFAULT;
    }

    /* Call user-provided write function */
    return rw_func(iocb->ki_filp, buffer, write_size, &iocb->ki_pos);
}

#endif  /* __WB_KERNEL_IO_H__ */
