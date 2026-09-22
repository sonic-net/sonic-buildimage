/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * General purpose logging, config parsing, and utility functions.
 ********************************************************************************/
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include "common.h"

/* ==========================================================================
 * Log system — internal variables
 * ========================================================================== */
#define LOG_FILE_PATH       "/var/log/physyslog"
#define MAX_FILE_PATH_LEN   256
#define MAX_LOG_FILE_SIZE   (1024 * 1024 * 5)
#define MAX_LOG_FILE_NUM    10
#define TIME_BUF_LEN        128

static FILE *g_log_file = NULL;
static char g_time_buf[TIME_BUF_LEN] = {0};

/* ==========================================================================
 * Log system — internal helpers
 * ========================================================================== */
static int compare_file_times(const char *file1, const char *file2)
{
    struct stat stat1, stat2;
    if (stat(file1, &stat1) != 0)
    {
        return -1;
    }
    if (stat(file2, &stat2) != 0)
    {
        return -1;
    }
    if (stat1.st_mtime < stat2.st_mtime)
    {
        return 2;
    }    
    if (stat1.st_mtime > stat2.st_mtime)
    {
        return 1;
    }
    return 0;
}

static void get_current_format_time(char *buffer, size_t bufferSize, char *format)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, bufferSize, format, timeinfo);
}

static void cust_log_file_init(void)
{
    struct stat file_stat;
    char buffer[MAX_FILE_PATH_LEN] = {0};
    char oldest_file[MAX_FILE_PATH_LEN] = {0};
    int i, slot_found = 0;

    while ((stat(LOG_FILE_PATH, &file_stat)) != -1) {
        if (file_stat.st_size < MAX_LOG_FILE_SIZE) {
            break;
        }
        /* Rotate: rename current log into the first free backup slot
         * (.1..MAX_LOG_FILE_NUM); if all slots are full, overwrite the
         * oldest backup instead. */
        for (i = 1; i <= MAX_LOG_FILE_NUM; i++) {
            snprintf(buffer, sizeof(buffer), "%s.%d", LOG_FILE_PATH, i);
            if ((stat(buffer, &file_stat)) != -1) {
                if (strlen(oldest_file) == 0) {
                    snprintf(oldest_file, sizeof(oldest_file), "%s", buffer);
                } else if (compare_file_times(buffer, oldest_file) == 2) {
                    snprintf(oldest_file, sizeof(oldest_file), "%s", buffer);
                }
            } else {
                if (rename(LOG_FILE_PATH, buffer) != 0) {
                    perror("log file rename failed");
                }
                slot_found = 1;
                break;
            }
        }
        if (!slot_found && (strlen(oldest_file) > 0)) {
            /* All backup slots are occupied -> overwrite the oldest one */
            if (rename(LOG_FILE_PATH, oldest_file) != 0) {
                perror("log file rename failed");
            }
        }
        break;
    }
    g_log_file = fopen(LOG_FILE_PATH, "a+");
    if (g_log_file == NULL) {
        perror("log file open failed");
        printf("WARNING: cannot open %s, logs will go to console only\n", LOG_FILE_PATH);
    }
    setbuf(g_log_file,NULL);
}

/* ==========================================================================
 * Log system — public functions
 * ========================================================================== */
char* get_current_time(void)
{
    get_current_format_time(g_time_buf, sizeof(g_time_buf), "%Y-%m-%d %H:%M:%S");
    return g_time_buf;
}

void log_info(const char *format, ...)
{
    va_list args;
    /* Also append to the log file when it was opened successfully. */
    if (g_log_file != NULL) {
        va_start(args, format);
        vfprintf(g_log_file, format, args);
        fflush(g_log_file);
        va_end(args);
    } else {
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

/* ==========================================================================
 * Init
 * ========================================================================== */
int common_init(void)
{
    cust_log_file_init();
    return STATUS_SUCCESS;
}

/* ==========================================================================
 * Config file parsing
 * ========================================================================== */
static int _c2int(const char *str, int *val)
{
    if (str == NULL) return STATUS_FAILURE;
    if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) {
        *val = (int)strtol(str, NULL, 16);
    } else {
        *val = atoi(str);
    }
    return STATUS_SUCCESS;
}

void parse_int_list(const char *input, int *val, int val_size, int *list_len)
{
    char *sp, *find, *buf, *outer_ptr = NULL, *inner_ptr = NULL;
    int index = 0;
    char temp[CFG_FILE_LINE_MAX];

    if (input == NULL || val == NULL) return;
    snprintf(temp, sizeof(temp), "%s", input);

    /* Remove brackets */
    find = strchr(temp, '[');
    if (find) {
        buf = temp;
        sp = strsep(&buf, "[");
        if (buf) {
            find = strchr(buf, ']');
            if (find) *find = '\0';
            sp = buf;
        } else {
            sp = temp;
        }
    } else {
        sp = temp;
    }

    /* Split by ',' and expand 'a-b' ranges (e.g. 0-3 -> 0,1,2,3) */
    sp = strtok_r(sp, ",", &outer_ptr);
    while (sp != NULL && index < val_size) {
        int arr_s = 0, arr_e = 0, i;
        while (*sp == ' ') sp++;
        find = strchr(sp, '-');
        if (find && (find != sp)) {
            /* range notation a-b */
            _c2int(strtok_r(sp, "-", &inner_ptr), &arr_s);
            _c2int(inner_ptr, &arr_e);
            for (i = arr_s; i <= arr_e; i++) {
                if (index >= val_size) break;
                val[index++] = i;
            }
        } else {
            _c2int(sp, &val[index++]);
        }
        sp = strtok_r(NULL, ",", &outer_ptr);
    }
    if (list_len) *list_len = index;
}

int get_cfg_info(const char *file_path, char *config_buf, int *val, bool is_arr)
{
    int arr_size;
    FILE *fp = NULL;
    char *sp = NULL;
    char *find = NULL;
    char config_line[CFG_FILE_LINE_MAX] = {0};
    char config_prefix[CFG_FILE_LINE_MAX] = {0};
    char *outer_ptr = NULL;

    if ((fp = fopen((char *)file_path, "r")) == NULL) {
        LOG_INFO("%s open fail.", file_path);
        return STATUS_FAILURE;
    }
    snprintf(config_prefix, sizeof(config_prefix), "%s=", config_buf);
    while (fgets(config_line, CFG_FILE_LINE_MAX, fp) != NULL) {
        if (config_line[0] == '#' || config_line[0] == '\r'
            || config_line[0] == '\n' || config_line[0] == ' ') {
            continue;
        }
        find = strchr(config_line, '\n');
        if (find) *find = '\0';
        find = strchr(config_line, '\r');
        if (find) *find = '\0';
        if (strncmp(config_line, config_prefix, strlen(config_prefix)) != 0) {
            continue;
        }
        if (is_arr) {
            char buf[CFG_FILE_LINE_MAX];
            snprintf(buf, sizeof(buf), "%s", config_line);
            sp = strtok_r(buf, "=", &outer_ptr);
            if (sp == NULL) {
                continue;
            }
            parse_int_list(outer_ptr, val, CFG_FILE_ARR_MAX, &arr_size);
        } else {
            char *temp = config_line;
            sp = strsep(&temp, "=");
            _c2int(temp, val);
        }
        fclose(fp);
        return STATUS_SUCCESS;
    }
    fclose(fp);
    LOG_INFO("get %s empty", config_buf);
    return STATUS_ITEM_NOT_FOUND;
}

/* ==========================================================================
 * General utility functions
 * ========================================================================== */
int file_exists(const char *filePath)
{
    if (filePath == NULL) {
        return 0;
    }
    if (access(filePath, F_OK) == 0){
        return 1;
    }
    return 0;
}
