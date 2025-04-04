#include <asm/unistd.h>
#include <linux/uaccess.h>
#include <linux/stat.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "../include/dfd_cfg_file.h"
#include "../include/dfd_module.h"
#include "../../dev_sysfs/include/sysfs_common.h"

struct getdents_callback {
    struct dir_context ctx;
    const char *obj_name;
    char *match_name;
    int dir_len;
    int found;
};

int kfile_open(char *fname, kfile_ctrl_t *kfile_ctrl)
{
    int ret;
    struct file *filp;
    loff_t pos;

    if ((fname == NULL) || (kfile_ctrl == NULL)) {
        return KFILE_RV_INPUT_ERR;
    }

    filp = filp_open(fname, O_RDONLY, 0);
    if (IS_ERR(filp)){
        return KFILE_RV_OPEN_FAIL;
    }

    kfile_ctrl->size = filp->f_inode->i_size;

    kfile_ctrl->buf = kmalloc(kfile_ctrl->size, GFP_KERNEL);
    if (kfile_ctrl->buf == NULL) {
        ret = KFILE_RV_MALLOC_FAIL;
        goto close_fp;
    }
    mem_clear(kfile_ctrl->buf, kfile_ctrl->size);

    pos = 0;
    ret = kernel_read(filp, kfile_ctrl->buf, kfile_ctrl->size, &pos);
    if (ret < 0) {
        ret = KFILE_RV_RD_FAIL;
        goto free_buf;
    }

    kfile_ctrl->pos = 0;

    ret = KFILE_RV_OK;
    goto close_fp;

free_buf:
    kfree(kfile_ctrl->buf);
    kfile_ctrl->buf = NULL;

close_fp:
    filp_close(filp, NULL);
    return ret;
}

void kfile_close(kfile_ctrl_t *kfile_ctrl)
{
    if (kfile_ctrl == NULL) {
        return;
    }

    kfile_ctrl->size = 0;
    kfile_ctrl->pos = 0;
    if (kfile_ctrl->buf) {
        kfree(kfile_ctrl->buf);
        kfile_ctrl->buf = NULL;
    }
}

int kfile_gets(char *buf, int buf_size, kfile_ctrl_t *kfile_ctrl)
{
    int i;
    int has_cr = 0;

    if ((buf == NULL) || (buf_size <= 0) || (kfile_ctrl == NULL) || (kfile_ctrl->buf == NULL)
            || (kfile_ctrl->size <= 0)) {
        return KFILE_RV_INPUT_ERR;
    }

    mem_clear(buf, buf_size);
    for (i = 0; i < buf_size; i++) {

        if (kfile_ctrl->pos >= kfile_ctrl->size) {
            break;
        }

        if (has_cr) {
            break;
        }

        if (IS_CR(kfile_ctrl->buf[kfile_ctrl->pos])) {
            has_cr = 1;
        }

        buf[i] = kfile_ctrl->buf[kfile_ctrl->pos];
        kfile_ctrl->pos++;
    }

    return i;
}

int kfile_read(int32_t addr, char *buf, int buf_size, kfile_ctrl_t *kfile_ctrl)
{
    int i;

    if ((buf == NULL) || (buf_size <= 0) || (kfile_ctrl == NULL) || (kfile_ctrl->buf == NULL)
            || (kfile_ctrl->size <= 0)) {
        return KFILE_RV_INPUT_ERR;
    }

    if ((addr < 0) || (addr >= kfile_ctrl->size)) {
        return KFILE_RV_ADDR_ERR;
    }

    mem_clear(buf, buf_size);

    kfile_ctrl->pos = addr;
    for (i = 0; i < buf_size; i++) {

        if (kfile_ctrl->pos >= kfile_ctrl->size) {
            break;
        }

        buf[i] = kfile_ctrl->buf[kfile_ctrl->pos];
        kfile_ctrl->pos++;
    }

    return i;
}

static int kfile_filldir_one(struct dir_context *ctx, const char * name, int len,
            loff_t pos, u64 ino, unsigned int d_type)
{
    struct getdents_callback *buf ;
    int result;
    buf = container_of(ctx, struct getdents_callback, ctx);
    result = 0;
    if (strncmp(buf->obj_name, name, strlen(buf->obj_name)) == 0) {
        if (buf->dir_len < len) {
            DBG_DEBUG(DBG_ERROR, "match ok. dir name:%s, but buf_len %d small than dir len %d.\n",
                name, buf->dir_len, len);
            buf->found = 0;
            return -1;
        }
        mem_clear(buf->match_name, buf->dir_len);
        memcpy(buf->match_name, name, len);
        buf->found = 1;
        result = -1;
    }
    return result;
}

int kfile_iterate_dir(const char *dir_path, const char *obj_name, char *match_name, int len)
{
    int ret;
    struct file *dir;
    struct getdents_callback buffer = {
        .ctx.actor = kfile_filldir_one,
    };

    if(!dir_path || !obj_name || !match_name) {
        DBG_DEBUG(DBG_ERROR, "params error. \n");
        return KFILE_RV_INPUT_ERR;
    }
    buffer.obj_name = obj_name;
    buffer.match_name = match_name;
    buffer.dir_len = len;
    buffer.found = 0;

    dir = filp_open(dir_path, O_RDONLY, 0);
    if (IS_ERR(dir)) {
        DBG_DEBUG(DBG_ERROR, "filp_open error, dir path:%s\n", dir_path);
        return KFILE_RV_OPEN_FAIL;
    }
    ret = iterate_dir(dir, &buffer.ctx);
    if (buffer.found) {
        DBG_DEBUG(DBG_VERBOSE, "match ok, dir name:%s\n", match_name);
        filp_close(dir, NULL);
        return DFD_RV_OK;
    }
    filp_close(dir, NULL);
    return -DFD_RV_NODE_FAIL;
}

#if 0

int kfile_write(char *fpath, int32_t addr, char *buf, int buf_size)
{
    int ret = KFILE_RV_OK;
    struct file *filp;
    mm_segment_t old_fs;
    int wlen;

    if ((fpath == NULL) || (buf == NULL) || (buf_size <= 0)) {
        return KFILE_RV_INPUT_ERR;
    }

    if (addr < 0) {
        return KFILE_RV_ADDR_ERR;
    }

    filp = filp_open(fpath, O_RDWR, 0);
    if (IS_ERR(filp)){
        return KFILE_RV_OPEN_FAIL;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    filp->f_op->llseek(filp,0,0);
    filp->f_pos = addr;

    wlen = filp->f_op->write(filp, buf, buf_size, &(filp->f_pos));
    if (wlen < 0) {
        ret = KFILE_RV_WR_FAIL;
    }

    filp->f_op->llseek(filp,0,0);
    set_fs(old_fs);
    filp_close(filp, NULL);

    return ret;
}
#endif
