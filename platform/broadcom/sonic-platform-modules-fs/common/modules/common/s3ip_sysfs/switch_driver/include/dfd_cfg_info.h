#ifndef __DFD_CFG_INFO_H__
#define __DFD_CFG_INFO_H__

#include <linux/types.h>

/* num buf format data to convert to a numeric function pointer */
typedef int (*info_num_buf_to_value_f)(uint8_t *num_buf, int buf_len, int *num_val);

/* num buf format data to convert to a numeric function pointer */
typedef int (*info_buf_to_buf_f)(uint8_t *buf, int buf_len, uint8_t *buf_new, int *buf_len_new);

/* Information format judgment macro */
#define IS_INFO_FRMT_BIT(frmt)      ((frmt) == INFO_FRMT_BIT)
#define IS_INFO_FRMT_BYTE(frmt)     (((frmt) == INFO_FRMT_BYTE) || ((frmt) == INFO_FRMT_NUM_BYTES))
#define IS_INFO_FRMT_NUM_STR(frmt)  ((frmt) == INFO_FRMT_NUM_STR)
#define IS_INFO_FRMT_NUM_BUF(frmt)  ((frmt) == INFO_FRMT_NUM_BUF)
#define IS_INFO_FRMT_BUF(frmt)      ((frmt) == INFO_FRMT_BUF)

/* INT Validity judgment of information length */
#define INFO_INT_MAX_LEN            (32)
#define INFO_INT_LEN_VALAID(len)    (((len) > 0) && ((len) < INFO_INT_MAX_LEN))

/* buf Validity judgment of information length */
#define INFO_BUF_MAX_LEN            (128)
#define INFO_BUF_LEN_VALAID(len)    (((len) > 0) && ((len) < INFO_BUF_MAX_LEN))

/* Determine the validity of information bit offset */
#define INFO_BIT_OFFSET_VALID(bit_offset)   (((bit_offset) >= 0) && ((bit_offset) < 31))

#define DFD_HWMON_NAME              "hwmon"

/* Information control mode */
typedef enum info_ctrl_mode_e {
    INFO_CTRL_MODE_NONE,
    INFO_CTRL_MODE_CFG,      /* Configuration mode */
    INFO_CTRL_MODE_CONS,     /* macromode */
    INFO_CTRL_MODE_TLV,      /* TLV mode */
    INFO_CTRL_MODE_SRT_CONS, /* String constant*/
    INFO_CTRL_MODE_END
} info_ctrl_mode_t;

/* Information format */
typedef enum info_frmt_e {
    INFO_FRMT_NONE,
    INFO_FRMT_BIT,          /* Single or multiple bits, not more than 8 bits */
    INFO_FRMT_BYTE,         /* Single byte */
    INFO_FRMT_NUM_BYTES,    /* Multiple byte values, up to sizeof(int) */
    INFO_FRMT_NUM_STR,      /* String value */
    INFO_FRMT_NUM_BUF,      /* String value */
    INFO_FRMT_BUF,          /* Multiple bytes */
    INFO_FRMT_END
} info_frmt_t;

/* Information source */
typedef enum info_src_e {
    INFO_SRC_NONE,
    INFO_SRC_CPLD,          /* CPLD equipment */
    INFO_SRC_FPGA,          /* FPGA equipment */
    INFO_SRC_OTHER_I2C,     /* other i2c equipment */
    INFO_SRC_FILE,          /* file */
    INFO_SRC_LOGIC_FILE,    /* logic file */
    INFO_SRC_END
} info_src_t;

/* Polarity of information */
typedef enum info_pola_e {
    INFO_POLA_NONE,
    INFO_POLA_POSI,         /* Positive polarity bit value 1 Valid value high bytes saved in the source low address space */
    INFO_POLA_NEGA,         /* Negative polarity bit value 0 Effective value high bytes saved in the source high address space */
    INFO_POLA_END
} info_pola_t;

/* value type of information */
typedef enum info_val_type_e {
    INFO_VAL_TYPE_NORMAL = 0,  /* Normal value. To be compatible with old products, it must be 0 */
    INFO_VAL_TYPE_FIXED_KEY,   /* The high 7 bits of value are fixed keys */
    INFO_VAL_TYPE_NEGA_KEY,    /* The high 7 bits of value represent the key, which is the inverse of the high 7 bits of the address */
    INFO_VAL_TYPE_END
} info_val_type_t;

/* Information control structure */
#define INFO_FPATH_MAX_LEN     (128)  /* Maximum length of the file source path */
#define INFO_STR_CONS_MAX_LEN  (64)   /* Maximum length of a string constant */
#define INFO_INT_RW_MAX_LEN    (4)    /* Maximum length of a int data */

typedef struct info_ctrl_s {
    info_ctrl_mode_t mode;          /* mode */
    int32_t int_cons;               /* Only the int type is supported */
    info_src_t src;                 /* source */
    info_frmt_t frmt;               /* format */
    info_pola_t pola;               /* polarity */
    info_pola_t val_pola;           /* Multi-byte data endianness */
    char fpath[INFO_FPATH_MAX_LEN]; /* File path, only the file source information */
    int32_t addr;                   /* address */
    int32_t len;                    /* Length, bit length, or byte length */
    int32_t val_len;                /* Multi-byte data length */
    int32_t bit_offset;             /* Offset number of bits in the address */
    char str_cons[INFO_STR_CONS_MAX_LEN]; /* String constant */
    info_val_type_t val_type;       /* value type */
    int32_t int_extra1;             /* int type reserved */
    int32_t int_extra2;
    int32_t int_extra3;             /* cpld voltage mode */
} info_ctrl_t;

/* info_ctrl_t member macro */
typedef enum info_ctrl_mem_s {
    INFO_CTRL_MEM_MODE,
    INFO_CTRL_MEM_INT_CONS,
    INFO_CTRL_MEM_SRC,
    INFO_CTRL_MEM_FRMT,
    INFO_CTRL_MEM_POLA,
    INFO_CTRL_MEM_VAL_POLA,
    INFO_CTRL_MEM_FPATH,
    INFO_CTRL_MEM_ADDR,
    INFO_CTRL_MEM_LEN,
    INFO_CTRL_MEM_VAL_LEN,
    INFO_CTRL_MEM_BIT_OFFSET,
    INFO_CTRL_MEM_STR_CONS,
    INFO_CTRL_VAL_TYPE,
    INFO_CTRL_MEM_INT_EXTRA1,
    INFO_CTRL_MEM_INT_EXTRA2,
    INFO_CTRL_MEM_INT_EXTRA3,
    INFO_CTRL_MEM_END
} info_ctrl_mem_t;

/* sensor data format */
typedef enum sensor_format_mem_s {
    LINEAR11 = 1,
    LINEAR16 = 2,
    TMP464   = 3,
    MAC_TH5  = 4,
    MAC_TH4  = 5,
    MAC_TD5  = 6,
    MAC_TD3  = 7,
    MAC_TH6  = 8,
} sensor_format_mem_t;

/* hwmon data format conversion */
typedef int (*info_hwmon_buf_f)(uint8_t *buf, int buf_len, uint8_t *buf_new, int *buf_len_new,
                info_ctrl_t *info_ctrl, int coefficient, int addend);

/* Global variable */
extern char *g_info_ctrl_mem_str[INFO_CTRL_MEM_END];  /* info_ctrl_t member string */
extern char *g_info_src_str[INFO_SRC_END];            /* info_src_t enumeration string */
extern char *g_info_frmt_str[INFO_FRMT_END];          /* info_frmt_t enumeration string */
extern char *g_info_pola_str[INFO_POLA_END];          /* info_pola_t enumeration string */
extern char *g_info_ctrl_mode_str[INFO_CTRL_MODE_END];/* info_ctrl_mode_t enumeration string */
extern char *g_info_val_type_str[INFO_VAL_TYPE_END];  /* info_val_type_t enumeration string */

/**
 * dfd_info_get_int - Get int type information
 * @key: Search keyword of the configuration item
 * @ret: int type information
 * @pfun: num Data conversion function of type buf
 *
 * @returns: 0 succeeds, <0 fails
 */
int dfd_info_get_int(uint64_t key, int *ret, info_num_buf_to_value_f pfun);

/**
 * dfd_info_get_buf - Get buf type information
 * @key: Search keyword of the configuration item
 * @buf: Information buf
 * @buf_len: buf length, which must be no less than info_ctrl->len
 * @pfun: Data conversion function pointer
 *
 * @returns: 0 succeeds, <0 fails
 */
int dfd_info_get_buf(uint64_t key, uint8_t *buf, int buf_len, info_buf_to_buf_f pfun);

/**
 * dfd_info_set_int - Set the int type information
 * @key: Search keyword of the configuration item
 * @val: int type information
 *
 * @returns: 0 succeeds, <0 fails
 */
int dfd_info_set_int(uint64_t key, int val);

/**
 * dfd_info_get_sensor - Get sensors
 * @key: HWMON Configures the key
 * @buf: Result storage
 * @buf_len: buf length
 *
 * @returns: <0 Failed, others succeeded
 */
int dfd_info_get_sensor(uint64_t key, char *buf, int buf_len, info_hwmon_buf_f pfun);

/**
 * @buf:Input and result store
 *
 */
void dfd_info_del_no_print_string(char *buf);
#endif /* __DFD_CFG_INFO_H__ */
