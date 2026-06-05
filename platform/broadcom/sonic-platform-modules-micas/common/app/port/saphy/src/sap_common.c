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
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "sap_common.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdint.h>

#define SAP_LOG_FILE_PATH    "/var/log/saplog"
#define MAX_FILE_PATH_LEN 256
#define MAX_LOG_FILE_SIZE 1024*1024*5
#define MAX_LOG_FILE_NUM 10
#define TIME_BUF_LEN 128
#define SAP_CFG_FILE_LINE_MAX   (2000)

static void getCurrentFormatTime(char *buffer, size_t bufferSize, char *format);

static sap_log_func_t g_diag_log_func = NULL;
static sap_log_func_t g_cust_log_func = NULL;

static FILE* sap_log_file = NULL;
pthread_mutex_t time_mutex;
static int time_mutex_inited = 0;
static char time_buf[TIME_BUF_LEN] = {0};

char* getCurrentTime()
{
    if (time_mutex_inited == 0) {
        time_mutex_inited = 1;
        pthread_mutex_init(&time_mutex, NULL);
    }
    pthread_mutex_lock(&time_mutex);
    getCurrentFormatTime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S");
    pthread_mutex_unlock(&time_mutex);
    return time_buf;
}

void sap_diag_log_reg(sap_log_func_t log_func)
{
    g_diag_log_func = log_func;
}

void sap_log_debug(const char *format, ...)
{
    /* TODO: 实现动态调整日志级别之后，需要在函数起始位置进行过滤 */
    va_list args;
    va_start(args, format);
    if (g_diag_log_func != NULL) {
        g_diag_log_func(format, args);
        va_start(args, format);
    }
    if (SAP_LOG_LEVEL < SAP_LOG_LEVEL_DEBUG) {
        return;
    }
    if (g_cust_log_func != NULL) {
        g_cust_log_func(format, args);
        va_start(args, format);
    }
    if (sap_log_file != NULL) {
        vfprintf(sap_log_file, format, args);
        fflush(sap_log_file);
    }
    va_end(args);
}

void sap_log_info(const char *format, ...)
{
    /* TODO: 实现动态调整日志级别之后，需要在函数起始位置进行过滤 */
    va_list args;
    va_start(args, format);
    if (g_diag_log_func != NULL) {
        g_diag_log_func(format, args);
        va_start(args, format);
    }
    if (SAP_LOG_LEVEL < SAP_LOG_LEVEL_INFO) {
        return;
    }
    if (g_cust_log_func != NULL) {
        g_cust_log_func(format, args);
        va_start(args, format);
    }
    if (sap_log_file != NULL) {
        vfprintf(sap_log_file, format, args);
        fflush(sap_log_file);
    }
    va_end(args);
}

void sap_log_notice(const char *format, ...)
{
    /* TODO: 实现动态调整日志级别之后，需要在函数起始位置进行过滤 */
    va_list args;
    va_start(args, format);
    if (g_diag_log_func != NULL) {
        g_diag_log_func(format, args);
        va_start(args, format);
    }
    if (SAP_LOG_LEVEL < SAP_LOG_LEVEL_NOTICE) {
        return;
    }
    if (g_cust_log_func != NULL) {
        g_cust_log_func(format, args);
        va_start(args, format);
    }
    if (sap_log_file != NULL) {
        vfprintf(sap_log_file, format, args);
        fflush(sap_log_file);
    }
    va_end(args);
}

void sap_log_warn(const char *format, ...)
{
    /* TODO: 实现动态调整日志级别之后，需要在函数起始位置进行过滤 */
    va_list args;
    va_start(args, format);
    if (g_diag_log_func != NULL) {
        g_diag_log_func(format, args);
        va_start(args, format);
    }
    if (SAP_LOG_LEVEL < SAP_LOG_LEVEL_WARN) {
        return;
    }
    if (g_cust_log_func != NULL) {
        g_cust_log_func(format, args);
        va_start(args, format);
    }
    if (sap_log_file != NULL) {
        vfprintf(sap_log_file, format, args);
        fflush(sap_log_file);
    }
    va_end(args);
}

void sap_log_error(const char *format, ...)
{
    /* TODO: 实现动态调整日志级别之后，需要在函数起始位置进行过滤 */
    va_list args;
    va_start(args, format);
    if (g_diag_log_func != NULL) {
        g_diag_log_func(format, args);
        va_start(args, format);
    }
    if (SAP_LOG_LEVEL < SAP_LOG_LEVEL_ERROR) {
        return;
    }
    if (g_cust_log_func != NULL) {
        g_cust_log_func(format, args);
        va_start(args, format);
    }
    if (sap_log_file != NULL) {
        vfprintf(sap_log_file, format, args);
        fflush(sap_log_file);
    }
    va_end(args);
}

void sap_cust_log_reg(sap_log_func_t log_func)
{
    g_cust_log_func = log_func;
}

/**
 * 根据二维数组内容和表头计算每列的最大宽度
 * @param table 二维字符串数组，表示数据内容
 * @param rows 表格的数据行数
 * @param cols 表格的列数
 * @param header 表头的二维字符串数组
 * @param header_rows 表头的行数
 * @param column_width 用于存储每列宽度的数组
 */
static void calculate_column_width(char *table[][SAP_MAX_TABLE_COL], int table_rows, char *header[][SAP_MAX_TABLE_COL], int header_rows, int header_cols, int column_width[]) {
    for (int col = 0; col < header_cols; col++) {
        column_width[col] = 0; /* 初始化列宽为0 */
        for (int row = 0; row < header_rows; row++) {
            if (header[row][col] != NULL) {
                int header_length = strlen(header[row][col]);
                if (header_length > column_width[col]) {
                    column_width[col] = header_length; /* 更新列宽度为表头内容的最大长度 */
                }
            }
        }
        for (int row = 0; row < table_rows; row++) {
            if (table[row][col] != NULL) {
                int cell_length = strlen(table[row][col]);
                if (cell_length > column_width[col]) {
                    column_width[col] = cell_length; /* 更新列宽度为数据内容的最大长度 */
                }
            }
        }
    }
}

/**
 * 打印表格的一行（居中显示）
 * @param content 当前单元格内容
 * @param width 当前列宽度
 */
static void print_centered_cell(const char *content, int width) {
    int content_length = content ? strlen(content) : 4; /* NULL 被显示为 "NULL"，长4 */
    int padding_left = (width - content_length) / 2;
    int padding_right = width - content_length - padding_left;

    for (int i = 0; i < padding_left; i++) {
        SAP_CLI_LOG_NO_LF(" ");
    }
    SAP_CLI_LOG_NO_LF("%s", content ? content : "NULL");
    for (int i = 0; i < padding_right; i++) {
        SAP_CLI_LOG_NO_LF(" ");
    }
}

/**
 * 打印二维字符串数组为表格格式
 * @param table 二维字符串数组，表示数据内容
 * @param rows 表格的数据行数
 * @param cols 表格的列数
 * @param header 表头的二维字符串数组
 * @param header_rows 表头的行数
 */

void print_table_without_header(char *table[][SAP_MAX_TABLE_COL], int start_row, int print_rows, int table_rows, char *header[][SAP_MAX_TABLE_COL], int header_rows, int header_cols)
{
    int column_width[SAP_MAX_TABLE_COL] = {0}; /* 存储每列的宽度 */
    calculate_column_width(table, table_rows, header, header_rows, header_cols, column_width); /* 计算列宽度 */

    /* 打印每行数据 */
    for (int row = start_row; row < print_rows + start_row; row++) {
        SAP_CLI_LOG_NO_LF("|");
        for (int col = 0; col < header_cols; col++) {
            SAP_CLI_LOG_NO_LF(" ");
            if (table[row][col] != NULL) {
                print_centered_cell(table[row][col], column_width[col]);
            } else {
                print_centered_cell("NULL", column_width[col]);
            }
            SAP_CLI_LOG_NO_LF(" |");
        }
        SAP_CLI_LOG_NO_LF("\n");
    }

    /* 打印表格底线 */
    SAP_CLI_LOG_NO_LF("+");
    for (int i = 0; i < header_cols; i++) {
        for (int j = 0; j < column_width[i] + 2; j++) {
            SAP_CLI_LOG_NO_LF("-");
        }
        SAP_CLI_LOG_NO_LF("+");
    }
    SAP_CLI_LOG_NO_LF("\n");
}

void print_table_header(char *table[][SAP_MAX_TABLE_COL], int table_rows, char *header[][SAP_MAX_TABLE_COL], int header_rows, int header_cols)
{
    int column_width[SAP_MAX_TABLE_COL] = {0}; /* 存储每列的宽度 */
    calculate_column_width(table, table_rows, header, header_rows, header_cols, column_width); /* 计算列宽度 */

    /* 打印表格顶线 */
    SAP_CLI_LOG_NO_LF("+");
    for (int i = 0; i < header_cols; i++) {
        for (int j = 0; j < column_width[i] + 2; j++) {
            SAP_CLI_LOG_NO_LF("-");
        }
        SAP_CLI_LOG_NO_LF("+");
    }
    SAP_CLI_LOG_NO_LF("\n");

    /* 打印表头 */
    for (int row = 0; row < header_rows; row++) {
        SAP_CLI_LOG_NO_LF("|");
        for (int col = 0; col < header_cols; col++) {
            SAP_CLI_LOG_NO_LF(" ");
            print_centered_cell(header[row][col], column_width[col]);
            SAP_CLI_LOG_NO_LF(" |");
        }
        SAP_CLI_LOG_NO_LF("\n");
    }

    /* 打印表格分隔线 */
    SAP_CLI_LOG_NO_LF("+");
    for (int i = 0; i < header_cols; i++) {
        for (int j = 0; j < column_width[i] + 2; j++) {
            SAP_CLI_LOG_NO_LF("-");
        }
        SAP_CLI_LOG_NO_LF("+");
    }
    SAP_CLI_LOG_NO_LF("\n");
}

void print_table(char *table[][SAP_MAX_TABLE_COL], int start_row, int print_rows, int table_rows, char *header[][SAP_MAX_TABLE_COL], int header_rows, int header_cols)
{
    int column_width[SAP_MAX_TABLE_COL] = {0}; /* 存储每列的宽度 */
    calculate_column_width(table, table_rows, header, header_rows, header_cols, column_width); /* 计算列宽度 */

    /* 打印表格顶线 */
    SAP_CLI_LOG_NO_LF("+");
    for (int i = 0; i < header_cols; i++) {
        for (int j = 0; j < column_width[i] + 2; j++) {
            SAP_CLI_LOG_NO_LF("-");
        }
        SAP_CLI_LOG_NO_LF("+");
    }
    SAP_CLI_LOG_NO_LF("\n");

    /* 打印表头 */
    for (int row = 0; row < header_rows; row++) {
        SAP_CLI_LOG_NO_LF("|");
        for (int col = 0; col < header_cols; col++) {
            SAP_CLI_LOG_NO_LF(" ");
            print_centered_cell(header[row][col], column_width[col]);
            SAP_CLI_LOG_NO_LF(" |");
        }
        SAP_CLI_LOG_NO_LF("\n");
    }

    /* 打印表格分隔线 */
    SAP_CLI_LOG_NO_LF("+");
    for (int i = 0; i < header_cols; i++) {
        for (int j = 0; j < column_width[i] + 2; j++) {
            SAP_CLI_LOG_NO_LF("-");
        }
        SAP_CLI_LOG_NO_LF("+");
    }
    SAP_CLI_LOG_NO_LF("\n");

    /* 打印每行数据 */
    for (int row = start_row; row < start_row + print_rows; row++) {
        SAP_CLI_LOG_NO_LF("|");
        for (int col = 0; col < header_cols; col++) {
            SAP_CLI_LOG_NO_LF(" ");
            if (table[row][col] != NULL) {
                print_centered_cell(table[row][col], column_width[col]);
            } else {
                print_centered_cell("NULL", column_width[col]);
            }
            SAP_CLI_LOG_NO_LF(" |");
        }
        SAP_CLI_LOG_NO_LF("\n");
    }

    /* 打印表格底线 */
    SAP_CLI_LOG_NO_LF("+");
    for (int i = 0; i < header_cols; i++) {
        for (int j = 0; j < column_width[i] + 2; j++) {
            SAP_CLI_LOG_NO_LF("-");
        }
        SAP_CLI_LOG_NO_LF("+");
    }
    SAP_CLI_LOG_NO_LF("\n");
}

void table_memfree(char *table[][SAP_MAX_TABLE_COL], int table_rows, char *header[][SAP_MAX_TABLE_COL], int header_rows, int header_cols)
{
    for (int row = 0; row < table_rows; row++) {
        for (int col = 0; col < header_cols; col++) {
            if (table[row][col] != NULL) {
                free(table[row][col]);
                table[row][col] = NULL;
            }
        }
    }
}

static void getCurrentFormatTime(char *buffer, size_t bufferSize, char *format) {
    time_t current_time;
    struct tm *local_time;
    char *default_format = "%Y_%m_%d_%H_%M_%S";
    if (format == NULL) {
        format = default_format;
    }
    /* 获取当前时间戳 */
    current_time = time(NULL);
    if (current_time == (time_t)-1) {
        return;
    }
    /* 转换为本地时间 */
    local_time = localtime(&current_time);
    if (local_time == NULL) {
        return;
    }
    /* 格式化时间字符串 */
    memset(buffer, 0, bufferSize);
    size_t len = strftime(buffer, bufferSize, format, local_time);
    if (len == 0) {
        return;
    }
}

static void _c2int(char *str, int *val)
{
    int base, flag;
    if (str == NULL) {
        SAP_LOG_ERR("in parameter check fail, exist nullpointer");
        return;
    }
    if (val == NULL) {
        SAP_LOG_ERR("in parameter check fail, exist nullpointer");
        return;
    }
    base = 0;
    flag = 1;
    if (val) {
        if (*str == '-') {
            flag = -1;
            str += 1;
        }
        if (*str == '0') {
            if (str[1] == 'b' || str[1] == 'B') {
                base = 2;
                str += 2;
            } else if (str[1] == 'x' || str[1] == 'X') {
                base = 16;
                str += 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
        *val = flag * (int)strtol(str, NULL, base);
    }
}

void sap_str_to_int(char *str, int *val)
{
    _c2int(str, val);
}

void sap_str_to_uint32(char *str, uint32_t *val)
{
    int _val;
    _c2int(str, &_val);
    *val = (uint32_t)_val;
}

void sap_str_to_uint64(char *str, uint64_t *val)
{
    int _val;
    _c2int(str, &_val);
    *val = (uint32_t)_val;
}

int _parse_param(char *str)
{
    char *param_val;
    int value;
    value = 0;
    param_val = strrchr(str, '=');
    param_val++;
    _c2int(param_val, &value);
    return value;
}

/* 检查文件是否存在的函数 */
int fileExists(const char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (file != NULL) {
        fclose(file);
        return 1;
    } else {
        /* 文件不存在 */
        return 0;
    }
}

void sap_parse_int_list(const char *input, int *val, int input_len, int *output_len)
{
    int i;
    char *sp = NULL;
    char *buf = NULL;
    char *outer_ptr = NULL;
    char *inner_ptr = NULL;
    char *find = NULL;
    int arr_s, arr_e;
    char *intput_buff = NULL;
    char *tmp;
    *output_len = 0;
    if (input == NULL) {
        SAP_LOG_ERR("in parameter check fail, exist nullpointer");
        return;
    }
    if (val == NULL) {
        SAP_LOG_ERR("in parameter check fail, exist nullpointer");
        return;
    }
    if (output_len == NULL) {
        SAP_LOG_ERR("in parameter check fail, exist nullpointer");
        return;
    }
    intput_buff = strdup(input);
    tmp = intput_buff;
    while ((buf = strtok_r(intput_buff, ",", &outer_ptr)) != NULL)
    {
        if (*output_len >= input_len) {
            break;
        }
        find = strchr(buf, '-');
        if (find && (find != buf)) {
            sp = strtok_r(buf, "-", &inner_ptr);
            _c2int(sp, &arr_s);
            _c2int(inner_ptr, &arr_e);
            for (i = arr_s; i<=arr_e; i++) {
                *(val++) = i;
                (*output_len)++;
                if (*output_len >= input_len) {
                    break;
                }
            }
        } else {
            _c2int(buf, val++);
            (*output_len)++;
        }
        intput_buff = outer_ptr;
    }
    free(tmp);
}

int sap_get_cfg_info(char *file_path, char *config_buf, int *val, bool is_arr)
{
    int arr_size;
    FILE *fp = NULL;
    char *sp = NULL;
    char *find = NULL;
    char *buf = NULL;
    char *out_buf = NULL;
    char config_line[SAP_CFG_FILE_LINE_MAX] = {0};
    char config_prefix[SAP_CFG_FILE_LINE_MAX] = {0};
    char *outer_ptr = NULL;
    char *inner_ptr = NULL;
    int arr_s, arr_e;
    if ((fp = fopen((char *)file_path, "r")) == NULL) {
        SAP_LOG_ERR("%s open fail.", file_path);
        return SAP_STATUS_FAILURE;
    }
    memset(config_line, 0, sizeof(config_line));
    memset(config_prefix, 0, sizeof(config_prefix));

    snprintf(config_prefix, sizeof(config_prefix), "%s=", config_buf);
    while (fgets(config_line, SAP_CFG_FILE_LINE_MAX, fp) != NULL) {
        if (config_line[0] == '#' || config_line[0] == '\r'
            || config_line[0] == '\n' || config_line[0] == ' ') {
            continue;
        }
        find = strchr(config_line, '\n');
        if (find) {
            *find = '\0';
        }
        find = strchr(config_line, '\r');
        if (find) {
            *find = '\0';
        }
        if (strncmp(config_line, config_prefix, strlen(config_prefix)) != 0) {
            continue;
        }
        if (is_arr == true) {
            buf = config_line;
            sp = strtok_r(buf, "=", &out_buf);
            if (sp == NULL) {
                continue;
            }
            sap_parse_int_list(out_buf, val, SAP_CFG_FILE_ARR_MAX, &arr_size);
        } else {
            char *temp = config_line;
            sp = strsep(&temp, "=");
            _c2int(temp, val);
        }
        fclose(fp);
        return SAP_STATUS_SUCCESS;
    }
    fclose(fp);
    SAP_LOG_DBG("get %s empty", config_buf);
    return SAP_STATUS_ITEM_NOT_FOUND;
}

int sap_get_cfg_info_str(char *file_path, char *config_buf, char *val)
{
    FILE *fp = NULL;
    char *sp = NULL;
    char *find = NULL;
    char config_line[SAP_CFG_FILE_LINE_MAX] = {0};
    char config_prefix[SAP_CFG_FILE_LINE_MAX] = {0};

    if ((fp = fopen((char *)file_path, "r")) == NULL) {
        SAP_LOG_DBG("%s open fail", file_path);
        return SAP_STATUS_FAILURE;
    }
    memset(config_line, 0, sizeof(config_line));
    memset(config_prefix, 0, sizeof(config_prefix));

    snprintf(config_prefix, sizeof(config_prefix), "%s=", config_buf);
    while (fgets(config_line, SAP_CFG_FILE_LINE_MAX, fp) != NULL) {
        if (config_line[0] == '#' || config_line[0] == '\r'
            || config_line[0] == '\n' || config_line[0] == ' ') {
            continue;
        }
        find = strchr(config_line, '\n');
        if (find) {
            *find = '\0';
        }
        find = strchr(config_line, '\r');
        if (find) {
            *find = '\0';
        }
        if (strncmp(config_line, config_prefix, strlen(config_prefix)) == 0) {
            sp = strtok(config_line, "=");
            sp = strtok(NULL, "=");
            // SAP_LOG_DBG("value : %s\n", sp);
            memcpy(val, sp, strlen(sp));
            fclose(fp);
            return SAP_STATUS_SUCCESS;
        }
    }
    fclose(fp);
    return SAP_STATUS_ITEM_NOT_FOUND;
}

char* _dump_int_array(int *input, int input_size, char *output, int output_size)
{
    int i;
    char number_str[32];
    memset(output, 0, output_size);
    for (i = 0; i<input_size; i++) {
        if (i != 0) {
            strncat(output, ",", strlen(output));
        }
        snprintf(number_str, sizeof(number_str), "%d", input[i]);
        strncat(output, number_str, strlen(number_str));
    }
    return output;
}

int tx_fir_cfg_parse(char *cfg_file, int phy_addr, int if_side, int lane, int serdes_mode, sap_medium_e medium, sap_tx_fir_t *tx_fir)
{
    int rv;
    int vals[SAP_CFG_FILE_ARR_MAX];
    char config_prefix[SAP_CFG_FILE_STR_MAX];
    char serdes_mode_string[16];
    char dump_string[SAP_CFG_FILE_STR_MAX];

    if (tx_fir == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    switch (serdes_mode) {
    case TX_MODE_PAM4_6TAP:
        snprintf(serdes_mode_string, sizeof(serdes_mode_string), "%s", "pam4_6tap");
        break;
    case TX_MODE_PAM4_LP_3TAP:
        snprintf(serdes_mode_string, sizeof(serdes_mode_string), "%s", "pam4_3tap");
        break;
    case TX_MODE_NRZ_6TAP:
        snprintf(serdes_mode_string, sizeof(serdes_mode_string), "%s", "nrz_6tap");
        break;
    case TX_MODE_NRZ_LP_3TAP:
        snprintf(serdes_mode_string, sizeof(serdes_mode_string), "%s", "nrz_3tap");
        break;
    default:
        return SAP_STATUS_INVALID_PARAMETER;
    }
    snprintf(config_prefix, sizeof(config_prefix), "serdes_pree_%s_lane%d_%s:0x%x", 
                            (if_side == PLP_SYS_IF_SIDE)?"sys":"line", lane, serdes_mode_string, phy_addr);
    rv = sap_get_cfg_info(cfg_file, config_prefix, vals, true);
    if (rv != SAP_STATUS_SUCCESS) {
        // SAP_LOG_NOTICE("%s get empty.", config_prefix);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    switch (serdes_mode) {
    case TX_MODE_PAM4_6TAP:
    case TX_MODE_NRZ_6TAP:
        SAP_LOG_DBG("%s: %s", config_prefix, _dump_int_array(vals, 7, dump_string, sizeof(dump_string)));
        tx_fir->pre3  = vals[0];
        tx_fir->pre2  = vals[1];
        tx_fir->pre   = vals[2];
        tx_fir->main  = vals[3];
        tx_fir->post  = vals[4];
        tx_fir->post2 = vals[5];
        tx_fir->post3 = vals[6];
        break;
    case TX_MODE_PAM4_LP_3TAP:
    case TX_MODE_NRZ_LP_3TAP:
        break;
    }
    return SAP_STATUS_SUCCESS;
}

static int compare_file_times(const char *file1, const char *file2) {
    struct stat stat1, stat2;
    if (stat(file1, &stat1) == -1) {
        perror("获取文件状态失败");
        return 0;
    }
    if (stat(file2, &stat2) == -1) {
        perror("获取文件状态失败");
        return 0;
    }
    /* 比较最后修改时间 */
    if (stat1.st_mtime > stat2.st_mtime) {
        /* 文件1最新 */
        return 1;
    } else if (stat1.st_mtime < stat2.st_mtime) {
        /* 文件2最新 */
        return 2;
    } else {
        return 0;
    }
}

static void cust_log_file_init()
{
    int i;
    FILE *fp = NULL;
    struct stat file_stat;
    char buffer[MAX_FILE_PATH_LEN] = {0};
    char oldest_file[MAX_FILE_PATH_LEN] = {0};

    while((stat(SAP_LOG_FILE_PATH, &file_stat)) != -1) {
        if (file_stat.st_size < MAX_LOG_FILE_SIZE) {
            break;
        }
        /* 当前日志文件大小超过限制，转存日志文件 */
        for (i = 1; i<=MAX_LOG_FILE_NUM; i++) {
            snprintf(buffer, sizeof(buffer), "%s.%d", SAP_LOG_FILE_PATH, i);
            if ((stat(buffer, &file_stat)) != -1) {
                /* 旧文件存在，判断是否是日期最早的文件 */
                if (strlen(oldest_file) == 0) {
                    snprintf(oldest_file, sizeof(oldest_file), "%s", buffer);
                } else if (compare_file_times(buffer, oldest_file) == 2) {
                    snprintf(oldest_file, sizeof(oldest_file), "%s", buffer);
                }
            } else {
                /* 有文件空位，直接转存 */
                if (rename(SAP_LOG_FILE_PATH, buffer) != 0) {
                    perror("日志文件移动失败");
                }
                break;
            }
        }
        /* 没有文件空位，替换日期最早的日志文件 */
        if (i > MAX_LOG_FILE_NUM) {
            if (rename(SAP_LOG_FILE_PATH, oldest_file) != 0) {
                perror("日志文件移动失败");
            }
        }
        break;
    }
    sap_log_file = fopen(SAP_LOG_FILE_PATH, "a+");
    if (sap_log_file == NULL) {
        perror("日志文件创建失败");
    }
}

int sap_common_init(void)
{
    cust_log_file_init();
    return SAP_STATUS_SUCCESS;
}
