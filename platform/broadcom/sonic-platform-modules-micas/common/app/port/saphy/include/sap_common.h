/********************************************************************************
 * Copyright(C) 2020 Micas Network. All rights reserved.
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期        | 作者     |  修改记录
**===============================================================================
**| 2024/08/23  | zhoutenghui  |  创建该文件
**
*********************************************************************************/
#ifndef _SAP_COMMON_H_
#define _SAP_COMMON_H_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include "sap_api.h"

// #define SAP_LOG_LEVEL_ERROR   (0x1)
// #define SAP_LOG_LEVEL_WARN    (0x2)
// #define SAP_LOG_LEVEL_NOTICE  (0x3)
// #define SAP_LOG_LEVEL_INFO    (0x4)
// #define SAP_LOG_LEVEL_DEBUG   (0x5)
// #define SAP_LOG_LEVEL_ALL     (0xf)

typedef enum {
    SAP_LOG_LEVEL_DISABLE = 0,
    SAP_LOG_LEVEL_ERROR = 1,
    SAP_LOG_LEVEL_WARN = 2,
    SAP_LOG_LEVEL_NOTICE = 3,
    SAP_LOG_LEVEL_INFO = 4,
    SAP_LOG_LEVEL_DEBUG = 5,
    SAP_LOG_LEVEL_ALL = 0xf
} SAP_LOG_LEVEL_E;


#define SAP_LOG_LEVEL SAP_LOG_LEVEL_DEBUG

// #ifndef SAP_LOG_LEVEL
// // #define SAP_LOG_LEVEL SAP_LOG_LEVEL_INFO
// #define SAP_LOG_LEVEL SAP_LOG_LEVEL_DEBUG
// #endif
#define SAP_MAX_TABLE_COL 16
#define SAP_MAX_TABLE_BLOCK 16
#define SAP_MAX_TABLE_CELL 64
#define SAP_CFG_FILE_STR_MAX   (128)
#define SAP_CFG_FILE_ARR_MAX   (512)

#define SAP_FILEPATH_BUFF_SIZE (128)

#ifdef SYSLOG_SUPPORT
/*LOG_EMERG */
/*LOG_ALERT */
/*LOG_CRIT */
#define SAP_LOG_ERR(fmt, args...)     syslog(LOG_ERR, fmt, ##args)
#define SAP_LOG_WARN(fmt, args...)    syslog(LOG_WARNING, fmt, ##args)
#define SAP_LOG_NOTICE(fmt, args...)  syslog(LOG_NOTICE, fmt, ##args)
#define SAP_LOG_INFO(fmt, args...)    syslog(LOG_INFO, fmt, ##args)
#define SAP_LOG_DBG(fmt, args...)     syslog(LOG_DEBUG, fmt, ##args)
#else
#define SAP_LOG_ERR(fmt, args...)     sap_log_error("%s ERROR Func=[%s], Line=[%d] " fmt"\n", \
                                    getCurrentTime(), __FUNCTION__, __LINE__, ##args)
#define SAP_LOG_WARN(fmt, args...)    sap_log_warn("%s WARN Func=[%s], Line=[%d] " fmt"\n", \
                                    getCurrentTime(), __FUNCTION__, __LINE__, ##args)
#define SAP_LOG_NOTICE(fmt, args...)  sap_log_notice("%s NOTICE Func=[%s], Line=[%d] " fmt"\n", \
                                    getCurrentTime(), __FUNCTION__, __LINE__, ##args)
#define SAP_LOG_INFO(fmt, args...)    sap_log_info("%s INFO Func=[%s], Line=[%d] " fmt"\n", \
                                    getCurrentTime(), __FUNCTION__, __LINE__, ##args)
#define SAP_LOG_DBG(fmt, args...)     sap_log_debug("%s DEBUG Func=[%s], Line=[%d] " fmt"\n", \
                                    getCurrentTime(), __FUNCTION__, __LINE__, ##args)
#endif

#define SAP_CLI_LOG(fmt, args...) printf(fmt"\n", ##args)
#define SAP_CLI_LOG_NO_LF(fmt, args...) printf(fmt, ##args)

#define SAP_COMMON_MEM_FREE(ptr) \
    if (ptr != NULL) {    \
        free(ptr);        \
        ptr = NULL;       \
    }

typedef int (*sap_log_func_t)(const char *format, va_list args);

#define TX_MODE_NRZ_LP_3TAP  1
#define TX_MODE_NRZ_6TAP     2 
#define TX_MODE_PAM4_LP_3TAP 3
#define TX_MODE_PAM4_6TAP    4

typedef enum {
    SAP_PHY_TYPE_MILLENIO = 0,
    SAP_PHY_TYPE_CREDO
} sap_phy_type_e;

typedef struct {
    sap_fec_e sap_fec;
    int phy_fec;
} sap_phy_fec_map;

typedef struct {
    char type[32];
    int  (*sap_platform_init)(int unit, bool warm_boot);
    bool (*sap_port_support)(int unit, int port, int physical_port);
    int  (*sap_create_port)(int unit, int port, sap_port_resource_t *port_resource); 
    int  (*sap_remove_port)(int unit, int port); 
    int  (*sap_port_speed_set)(int unit, int port, int speed);
    int  (*sap_port_speed_get)(int unit, int port, int *speed);
    int  (*sap_port_fec_support)(int unit, int port);
    int  (*sap_port_fec_set)(int unit, int port, int if_side, sap_fec_e fec_type);
    int  (*sap_port_fec_get)(int unit, int port, int if_side, sap_fec_e *fec_type);
    int  (*sap_port_autoneg_set)(int unit, int port, int if_side, int enable);
    int  (*sap_port_autoneg_get)(int unit, int port, int if_side, int *enable);
    int  (*sap_port_linktrain_set)(int unit, int port, int if_side, int enable);
    int  (*sap_port_prbs_set)(int unit, int port, int poly, int inv, int if_side, int lane);
    int  (*sap_port_prbs_get)(int unit, int port, int if_side, sap_prbs_status_t *prbs_info);
    int  (*sap_port_prbs_clear)(int unit, int port, int if_side);
    int  (*sap_port_prbs_ber_get)(int unit, int port, int if_side, int time_v, sap_prbs_status_t *prbs_info);
    int  (*sap_squelch_set)(int unit, int port, int if_side, int tx_rx, int enable, int lane);
    int  (*sap_loopback_set)(int unit, int port, int if_side, int lb_dir, int enable);
    int  (*sap_loopback_get)(int unit, int port, int if_side, int lb_dir, int *enable);
    int  (*sap_polarity_set)(int unit, int port, int if_side, int tx_rx, uint32_t polarity, bool override);
    int  (*sap_polarity_get)(int unit, int port, int if_side, int tx_rx, uint32_t *polarity);
    int  (*sap_tx_fir_set)(int unit, int port, int if_side, int lane, sap_tx_fir_t tx_fir);
    int  (*sap_tx_fir_get)(int unit, int port, int if_side, int lane, sap_tx_fir_t *tx_fir);
    int  (*sap_phy_status_dump)(int unit, int port, int level);
    int  (*sap_dsc_dump)(int unit, int port, int if_side, int flag, int lane);
    int  (*sap_eyesacn_dump)(int unit, int port, int if_side, int lane);
    int  (*sap_shell_cmd_run)(int unit, int port, char *shell_cmd);
    int  (*sap_temperature_get)(int unit, int port, double *temp);
    int  (*sap_highest_temperature_get)(double *temp);
    int  (*sap_phy_reset)(int unit, int port, int flags);
    int  (*sap_reg_set)(int unit, int port, int if_side, int devaddr, int regaddr, int flag, uint64_t data);
    int  (*sap_reg_get)(int unit, int port, int if_side, int devaddr, int regaddr, int flag, uint64_t *data);
    void (*sap_techsupport)(int unit, int port);
    int  (*sap_mib_dump)(int unit, int port, int if_side);
    int  (*sap_channel_reach_set)(int unit, int port, int if_side, int lane, int nr_er);
    int  (*sap_eeprom_update)(int unit, int port, bool use_file);
    int  (*sap_fw_version_get)(int unit, int port, sap_version_t *version_info);
    // int  (*sap_port_status_get)(int unit, int port);
    int  (*sap_port_list_get)(sap_port_list_t *port_list);
    int  (*sap_port_status_get)(int unit, int port, sap_port_status_t *port_status);
} sap_apis_t;

int sap_common_init(void);
void sap_cust_log_reg(sap_log_func_t log_func);
void sap_diag_log_reg(sap_log_func_t log_func);
void sap_log_debug(const char *format, ...);
void sap_log_info(const char *format, ...);
void sap_log_notice(const char *format, ...);
void sap_log_warn(const char *format, ...);
void sap_log_error(const char *format, ...);
int sap_get_cfg_info(char *file_path, char *config_buf, int *val, bool is_arr);
int sap_get_cfg_info_str(char *file_path, char *config_buf, char *val);
void sap_str_to_int(char *str, int *val);
void sap_str_to_uint32(char *str, uint32_t *val);
void sap_str_to_uint64(char *str, uint64_t *val);

void sap_parse_int_list(const char *input, int *val, int val_size, int *list_len);

void print_table(char *table[][SAP_MAX_TABLE_COL], int start_row, int print_rows, int table_rows, char *header[][SAP_MAX_TABLE_COL], int header_rows, int header_cols);
void print_table_without_header(char *table[][SAP_MAX_TABLE_COL], int start_row, int print_rows, int table_rows, char *header[][SAP_MAX_TABLE_COL], int header_rows, int header_cols);
void print_table_header(char *table[][SAP_MAX_TABLE_COL], int table_rows, char *header[][SAP_MAX_TABLE_COL], int header_rows, int header_cols);
void table_memfree(char *table[][SAP_MAX_TABLE_COL], int table_rows, char *header[][SAP_MAX_TABLE_COL], int header_rows, int header_cols);

char* getCurrentTime(void);
int _parse_param(char *str);
char* _dump_int_array(int *input, int input_size, char *output, int output_size);
int tx_fir_cfg_parse(char *cfg_file, int phy_addr, int if_side, int lane, int serdes_mode, sap_medium_e medium, sap_tx_fir_t *tx_fir);
int fileExists(const char *filePath);
// char* _dump_int_array(int *input, int input_size, char output[], int output_size);

#endif /* _SAP_COMMON_H_ */
