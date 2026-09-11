#ifndef __DFD_CFG_FILE_H__
#define __DFD_CFG_FILE_H__

#include <linux/types.h>

/* Returned value */
#define KFILE_RV_OK             (0)
#define KFILE_RV_INPUT_ERR      (-1)    /* Entry error */
#define KFILE_RV_STAT_FAIL      (-2)    /* Failed to obtain file properties. Procedure */
#define KFILE_RV_OPEN_FAIL      (-3)    /* Failed to open file */
#define KFILE_RV_MALLOC_FAIL    (-4)    /* Failed to allocate memory */
#define KFILE_RV_RD_FAIL        (-5)    /* Read failure */
#define KFILE_RV_ADDR_ERR       (-6)    /* Address error */
#define KFILE_RV_WR_FAIL        (-7)    /* Address error */

/* Whether it is a newline character */
#define IS_CR(c)  ((c) == '\n')

/* File operation control structure */
typedef struct kfile_ctrl_s {
    int32_t size;       /* File size */
    int32_t pos;        /* Current position */
    char *buf;          /* File cache */
} kfile_ctrl_t;

/*
 * Open file
 * @fname: filename
 * @kfile_ctrl: File control variable
 *
 * @returns: 0 Succeeded, others failed
 */
int kfile_open(char *fname, kfile_ctrl_t *kfile_ctrl);

/*
 * Close file
 * @kfile_ctrl: File control variable
 */
void kfile_close(kfile_ctrl_t *kfile_ctrl);

/*
 * Close file
 * @kfile_ctrl: File control variable
 *
 * @returns: >=0 Succeeded. Others failed
 */
int kfile_gets(char *buf, int buf_size, kfile_ctrl_t *kfile_ctrl);

/*
 * Read data
 * @buf: buf Buffer area
 * @buf_size: buf size
 * @kfile_ctrl: File control variable
 *
 * @returns: >=0 Succeeded. Others failed
 */
int kfile_read(int32_t addr, char *buf, int buf_size, kfile_ctrl_t *kfile_ctrl);

/*
 * Read data
 * @buf: buf Buffer area
 * @buf_size: buf size
 * @kfile_ctrl: File control variable
 *
 * @returns: >=0 Succeeded. Others failed
 */
int kfile_iterate_dir(const char *dir_path, const char *obj_name, char *match_name, int len);

#if 0
/*
 * Write data
 * @fname: filename
 * @addr: Offset address of the file written to
 * @buf: Write data
 * @buf_size: Data size
 *
 * @returns: >=0 Succeeded. Others failed
 */
int kfile_write(char *fpath, int32_t addr, char *buf, int buf_size);
#endif
#endif /* __DFD_CFG_FILE_H__ */
