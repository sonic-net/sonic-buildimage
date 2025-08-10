#ifndef __FIRMWARE_CPLD_H__
#define __FIRMWARE_CPLD_H__

#define FIRMWARE_DEV_NAME_LEN     32
#define FIRMWARE_MAX_CPLD_NUM     16

typedef struct firmware_cpld_s {
    char devname[FIRMWARE_DEV_NAME_LEN];
    int slot;
    int chip_index;
    int is_used;                       /* 0:unused 1:used */
    u32 tck_delay;                     /* delay time */
    void (*pull_tdi_up)(void);         /* TDI pull up */
    void (*pull_tdi_down)(void);       /* TDI pull dowm */
    void (*pull_tck_up)(void);         /* TCK pull up */
    void (*pull_tck_down)(void);       /* TCK pull dowm */
    void (*pull_tms_up)(void);         /* TMS pull up */
    void (*pull_tms_down)(void);       /* TCK pull dowm */
    int (*read_tdo)(void);             /* read ?TDO */
    int (*init_cpld)(void);            /* CPLD upgrade initialization operation  */
    int (*init_chip)(int slot);        /* Chip related initialization operations */
    int (*finish_chip)(int slot);      /* Chip related completion operations */
    int (*finish_cpld)(void);          /* CPLD upgrade completion operation */
    int (*check_upgrade_data)(char *src, int src_len, int *dst, int dst_len);
    int (*touch_watch_dog)(void);      /* touch watch dog related operation */
    int (*keeplive)(void);             /* KEEPLIVE */
    int (*get_version)(int slot, char *ver, int len);
    int (*get_card_name)(char *name, int len); /* get card name */
} firmware_cpld_t;

typedef int (*firmware_set_gpio_info_func_t)(firmware_upg_gpio_info_t *info);

extern int fmw_cpld_upg_get_chip_name(int slot, firmware_cpld_t *cpld, char *info, int len);
extern int fmw_cpld_upg_get_card_name(int slot, firmware_cpld_t *cpld, char *info, int len);
extern int fmw_cpld_upg_program(int slot, firmware_cpld_t *cpld, char *info, int len);
extern int fmw_cpld_upg_get_version(int slot, firmware_cpld_t *cpld, char *info, int len);
extern firmware_cpld_t *fmw_cpld_upg_get_cpld(char *name);
extern int fmw_cpld_upg_init(void);
extern void fmw_cpld_upg_exit(void);
extern int fmw_cpld_upg_copy_firmware_info(firmware_cpld_t *info);
extern int fmw_cpld_upg_get_chip_info(int slot, firmware_cpld_t *cpld, void *info, int len);

extern void fwm_cpld_tdi_op(int value);
extern void fwm_cpld_tck_op(int value);
extern void fwm_cpld_tms_op(int value);
extern int fwm_cpld_tdo_op(void);
extern void firmware_cpld_upgrade_init(void);
extern void firmware_cpld_upgrade_finish(void);
extern int fmw_cpld_set_gpio_info(firmware_upg_gpio_info_t *info);
extern void fmw_cpld_reg_gpio_info_set_func(firmware_set_gpio_info_func_t func);

#endif /* __FIRMWARE_CPLD_H__ */