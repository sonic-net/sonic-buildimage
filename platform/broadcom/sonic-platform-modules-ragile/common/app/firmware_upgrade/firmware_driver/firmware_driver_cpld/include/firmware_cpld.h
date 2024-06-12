#ifndef __FIRMWARE_CPLD_H__
#define __FIRMWARE_CPLD_H__

#define FIRMWARE_DEV_NAME_LEN     32
#define FIRMWARE_MAX_CPLD_NUM     16
#define FIRMWARE_TYPE_LEN         10
#define FIRMWARE_EN_INFO_MAX      16
#define FIRMWARE_EN_INFO_BUF      128

typedef struct firmware_gpio_jtag_en_s {
    uint32_t en_gpio;                                               /* GPIO enable pin */
    uint32_t en_level;                                              /* GPIO enable level */
    int flag;                                                       /* init flag; 1-init 0-not init */
} firmware_gpio_jtag_en_t;

typedef struct firmware_cpld_s {
    char devname[FIRMWARE_DEV_NAME_LEN];                        /* Device name */
    char type[FIRMWARE_TYPE_LEN];                               /* interface type */
    uint32_t tdi;                                               /* TDI signal corresponding to GPIO pin information */
    uint32_t tck;                                               /* TCK signal corresponding to GPIO pin information */
    uint32_t tms;                                               /* TMS signal corresponding to GPIO pin information */
    uint32_t tdo;                                               /* TDO signal corresponding to GPIO pin information */
    uint32_t chain;                                             /* chain num */
    uint32_t chip_index;                                        /* chip index */
    uint32_t tck_delay;                                         /* Delay time */
    uint32_t gpio_en_info_num;                                  /* GPIO Enable Number */
    firmware_gpio_jtag_en_t gpio_en_info[FIRMWARE_EN_INFO_MAX]; /* GPIO Enable Information */
} firmware_cpld_t;

typedef struct firmware_cpld_function_s{
    int (*pull_tdi_up)(void);                          /* TDI pull-up */
    int (*pull_tdi_down)(void);                        /* TDI pull-down */
    int (*pull_tck_up)(void);                          /* TCK pull-up */
    int (*pull_tck_down)(void);                        /* TCK pull-down */
    int (*pull_tms_up)(void);                          /* TMS pull-up */
    int (*pull_tms_down)(void);                        /* TCK pull-down */
    int (*read_tdo)(void);                             /* Read TDO */
    int (*init_cpld)(void);                            /* CPLD upgrade initializes the operation */
    int (*init_chip)(int chain);                       /* chip initializes the operation */
    int (*finish_chip)(int chain);                     /* chip completes the operation*/
    int (*finish_cpld)(void);                          /* CPLD upgrade completes the operation */
    int (*get_version)(int chain, char *ver, int len); /* get version */
}firmware_cpld_function_t;

/* get chip name */
extern int fmw_cpld_upg_get_chip_name(int chain, firmware_cpld_t *cpld, char *info, int len);
/* ISC firmware upgrad */
extern int fmw_cpld_upg_program(int chain, firmware_cpld_t *cpld, char *info, int len);
/* get version */
extern int fmw_cpld_upg_get_version(int chain, firmware_cpld_t *cpld, char *info, int len);
/* Read the contents of Chip */
extern int fmw_cpld_upg_get_chip_info(int chain, firmware_cpld_t *cpld, void *info, int len);
/* operate TDI */
extern int fwm_cpld_tdi_op(int value);
/* operate TCK */
extern int fwm_cpld_tck_op(int value);
/* operate TMS */
extern int fwm_cpld_tms_op(int value);
/* operate TDO */
extern int fwm_cpld_tdo_op(void);
/* JBI firmware upgrad */
extern int fmw_cpld_upg_program_jbi(int chain, firmware_cpld_t *cpld, char *info, int len);

#endif /* __FIRMWARE_CPLD_H__ */
