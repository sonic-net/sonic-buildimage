/*
 * A header definition for dfd_cfg_file driver
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __DFD_CFG_FILE_H__
#define __DFD_CFG_FILE_H__

#include <linux/types.h>

#define KFILE_RV_OK             (0)
#define KFILE_RV_INPUT_ERR      (-1)
#define KFILE_RV_STAT_FAIL      (-2)
#define KFILE_RV_OPEN_FAIL      (-3)
#define KFILE_RV_MALLOC_FAIL    (-4)
#define KFILE_RV_RD_FAIL        (-5)
#define KFILE_RV_ADDR_ERR       (-6)
#define KFILE_RV_WR_FAIL        (-7)

#define IS_CR(c)  ((c) == '\n')

typedef struct kfile_ctrl_s {
    int32_t size;
    int32_t pos;
    char *buf;
} kfile_ctrl_t;

int kfile_open(char *fname, kfile_ctrl_t *kfile_ctrl);

void kfile_close(kfile_ctrl_t *kfile_ctrl);

int kfile_gets(char *buf, int buf_size, kfile_ctrl_t *kfile_ctrl);

int kfile_read(int32_t addr, char *buf, int buf_size, kfile_ctrl_t *kfile_ctrl);

int kfile_iterate_dir(const char *dir_path, const char *obj_name, char *match_name, int len);

#if 0

int kfile_write(char *fpath, int32_t addr, char *buf, int buf_size);
#endif
#endif /* __DFD_CFG_FILE_H__ */
