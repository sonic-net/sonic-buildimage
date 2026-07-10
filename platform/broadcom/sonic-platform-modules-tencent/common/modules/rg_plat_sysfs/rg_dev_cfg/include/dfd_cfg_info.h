#ifndef __DFD_CFG_INFO_H__
#define __DFD_CFG_INFO_H__

#include <linux/types.h>

typedef int (*info_num_buf_to_value_f)(uint8_t *num_buf, int buf_len, int *num_val);

typedef int (*info_buf_to_buf_f)(uint8_t *buf, int buf_len, uint8_t *buf_new, int *buf_len_new);

#define IS_INFO_FRMT_BIT(frmt)      ((frmt) == INFO_FRMT_BIT)
#define IS_INFO_FRMT_BYTE(frmt)     (((frmt) == INFO_FRMT_BYTE) || ((frmt) == INFO_FRMT_NUM_BYTES))
#define IS_INFO_FRMT_NUM_STR(frmt)  ((frmt) == INFO_FRMT_NUM_STR)
#define IS_INFO_FRMT_NUM_BUF(frmt)  ((frmt) == INFO_FRMT_NUM_BUF)
#define IS_INFO_FRMT_BUF(frmt)      ((frmt) == INFO_FRMT_BUF)

#define INFO_INT_MAX_LEN            (32)
#define INFO_INT_LEN_VALAID(len)    (((len) > 0) && ((len) < INFO_INT_MAX_LEN))

#define INFO_BUF_MAX_LEN            (128)
#define INFO_BUF_LEN_VALAID(len)    (((len) > 0) && ((len) < INFO_BUF_MAX_LEN))

#define INFO_BIT_OFFSET_VALID(bit_offset)   (((bit_offset) >= 0) && ((bit_offset) < 8))

typedef enum info_ctrl_mode_e {
    INFO_CTRL_MODE_NONE,
    INFO_CTRL_MODE_CFG,
    INFO_CTRL_MODE_CONS,
    INFO_CTRL_MODE_TLV,
    INFO_CTRL_MODE_SRT_CONS,
    INFO_CTRL_MODE_END
} info_ctrl_mode_t;

typedef enum info_frmt_e {
    INFO_FRMT_NONE,
    INFO_FRMT_BIT,
    INFO_FRMT_BYTE,
    INFO_FRMT_NUM_BYTES,
    INFO_FRMT_NUM_STR,
    INFO_FRMT_NUM_BUF,
    INFO_FRMT_BUF,
    INFO_FRMT_END
} info_frmt_t;

typedef enum info_src_e {
    INFO_SRC_NONE,
    INFO_SRC_CPLD,
    INFO_SRC_FPGA,
    INFO_SRC_OTHER_I2C,
    INFO_SRC_FILE,
    INFO_SRC_END
} info_src_t;

typedef enum info_pola_e {
    INFO_POLA_NONE,
    INFO_POLA_POSI,
    INFO_POLA_NEGA,
    INFO_POLA_END
} info_pola_t;

#define INFO_FPATH_MAX_LEN     (128)
#define INFO_STR_CONS_MAX_LEN  (64)
typedef struct info_ctrl_s {
    info_ctrl_mode_t mode;
    int32_t int_cons;
    info_src_t src;
    info_frmt_t frmt;
    info_pola_t pola;
    char fpath[INFO_FPATH_MAX_LEN];
    int32_t addr;
    int32_t len;
    int32_t bit_offset;
    char str_cons[INFO_STR_CONS_MAX_LEN];
    int32_t int_extra1;
    int32_t int_extra2;
} info_ctrl_t;

typedef enum info_ctrl_mem_s {
    INFO_CTRL_MEM_MODE,
    INFO_CTRL_MEM_INT_CONS,
    INFO_CTRL_MEM_SRC,
    INFO_CTRL_MEM_FRMT,
    INFO_CTRL_MEM_POLA,
    INFO_CTRL_MEM_FPATH,
    INFO_CTRL_MEM_ADDR,
    INFO_CTRL_MEM_LEN,
    INFO_CTRL_MEM_BIT_OFFSET,
    INFO_CTRL_MEM_STR_CONS,
    INFO_CTRL_MEM_INT_EXTRA1,
    INFO_CTRL_MEM_INT_EXTRA2,
    INFO_CTRL_MEM_END
} info_ctrl_mem_t;

typedef int (*info_hwmon_buf_f)(uint8_t *buf, int buf_len, uint8_t *buf_new, int *buf_len_new, info_ctrl_t *info_ctrl);

extern char *g_info_ctrl_mem_str[INFO_CTRL_MEM_END];
extern char *g_info_src_str[INFO_SRC_END];
extern char *g_info_frmt_str[INFO_FRMT_END];
extern char *g_info_pola_str[INFO_POLA_END];
extern char *g_info_ctrl_mode_str[INFO_CTRL_MODE_END];

int dfd_info_get_int(int key, int *ret, info_num_buf_to_value_f pfun);

int dfd_info_get_buf(int key, uint8_t *buf, int buf_len, info_buf_to_buf_f pfun);

int dfd_info_set_int(int key, int val);

int dfd_info_get_sensor(uint32_t key, char *buf, int buf_len, info_hwmon_buf_f pfun);

#endif /* __DFD_CFG_INFO_H__ */
