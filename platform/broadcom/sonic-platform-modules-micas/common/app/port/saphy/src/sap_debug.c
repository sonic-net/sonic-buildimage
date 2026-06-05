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
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include "sap_api.h"
#include "sap_debug.h"
#include "sap_common.h"
#include "sap_socket.h"
#include "list.h"
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>

#define MAX_DEBUG_STR_LEN 512
#define MAX_PARAM_BUF_SIZE 32
#define MAX_PARAM_OPTION_NUM 16

#define MAX_DEBUG_ARG_NUM 32
#define SAPMSG_BUF                      "/tmp/sapcmd_output"
#define SAPMSG_BUF_SIZE_MAX             (10*1024*1024)
#define SAPCMD_SOCKET_PATH              "/tmp/sapcmd_socket"
#define PLP_CLI_PROCESS_OVER            "done\n"

static bool g_cli_debug_mode = false;

#define PLP_CLI(fmt, args...) \
    do { \
        printf(fmt"\n", ##args); \
    } while (0)

#define PLP_CLI_DEBUG(fmt, args...) \
    if (g_cli_debug_mode) { \
        PLP_CLI(fmt, ##args); \
    }
    
static sap_log_func_t g_diag_log_func = vprintf;

#ifdef SAPHY_BINARY
#define HELP_MSG_PREFIX "<unit> <port>"
#else
#define HELP_MSG_PREFIX "phy user_diag <port_range>"
#endif

#define PRBS_MAX_LANE_NUM 16
#define LINK_STRING(link_st) ((link_st) ? "up" : "down")
typedef int (*sap_cmd_func_t)(int unit, int port, char **args, int argc);


typedef struct {
    char *name;			/* User printable name of the function. */
    sap_cmd_func_t func;		/* Function to call to do the job. */
    char *doc;			/* Documentation for this function.  */
    char *annotation;
} sap_debug_command_t;

typedef struct {
    int unit;
    int port;
    int if_side;
    int timestamp;
    struct list_head node;
} sap_prbsstat_info_t;
typedef struct
{
    int unit;
    int port;
    int enable;
    int speed;
    int if_side;
    int poly;
    int lane;
    int lb_dir;
    int flag;
    int level;
    int tx_rx;
    int inv;
    int override;
    int nr_er;
    uint32_t polarity;
    uint32_t data;
    uint32_t devaddr;
    uint32_t regaddr;
    long long unsigned int data_64;
    sap_tx_fir_t tx_fir;
} sap_cli_params_t;

static sap_cli_params_t g_params;

typedef enum {
    VALUE_TYPE_INT = 0,
    VALUE_TYPE_UINT,
    VALUE_TYPE_UINT_64,
    VALUE_TYPE_STRING,
    VALUE_TYPE_ENUM,
    VALUE_TYPE_BOOL,
    VALUE_TYPE_MAX
} sap_value_type_e;
typedef struct
{
    char value_str[MAX_PARAM_BUF_SIZE];
    int value_int;
} sap_enum_info_t;

typedef struct {
    char key[MAX_PARAM_BUF_SIZE];
    sap_value_type_e value_type;
    void *value_ptr;
    sap_enum_info_t enum_info[MAX_PARAM_OPTION_NUM];
} sap_cli_param_info_t;

#define PARAM_VALUE_TYPE_BOOL(key, ptr) {key, VALUE_TYPE_BOOL, ptr}
#define PARAM_VALUE_TYPE_INT(key, ptr) {key, VALUE_TYPE_INT, ptr}
#define PARAM_VALUE_TYPE_UINT(key, ptr) {key, VALUE_TYPE_UINT, ptr}
#define PARAM_VALUE_TYPE_UINT64(key, ptr) {key, VALUE_TYPE_UINT_64, ptr}
#define PARAM_LIST(...) {__VA_ARGS__}
#define PARAM_VALUE_TYPE_ENUM(key, ptr, enum_map) {key, VALUE_TYPE_ENUM, ptr, enum_map}

/* */
#define SAP_INT_PARAM_SET(field, default, ptr) \
do { \
    int __rv__; \
    __rv__ = cli_params_parse_field(args, argc, (field), (ptr)); \
    if (__rv__ == SAP_STATUS_ITEM_NOT_FOUND) { \
        *(ptr) = default; \
    } else if (__rv__ != SAP_STATUS_SUCCESS) { \
        return __rv__; \
    } \
} while(0)

static void *server_message_handle(NET_CONN net_comm, char *data, uint32_t data_len);
static void *server_connect_handle(NET_CONN net_comm, NET_COMM_CONN_STATUS_E status);
static int redirect_stdout_to_sdkmsg();
static int redirect_stdout_back();

/* Forward declarations. */
static char *stripwhite();
static sap_debug_command_t *find_command(char *name);
static int execute_line(char *cmd, uint32_t cmd_len);
static uint32_t sap_timestamp_get();
static int sap_debug_prbsstat_update(int unit, int port, int if_side, sap_prbs_status_t *prbs_info);
static void sap_debug_func_exec(int unit, int port, char *diag_func_name, char **diag_params, int n_param);
static void cli_params_parse_option(char **diag_params, int n_param, const char *option_key, bool *found);
static int cli_params_parse_field(char **diag_params, int n_param, const char *field_key, void *field_val);
static void cli_params_parse_all(char **diag_params, int n_param);
static int cli_param_value_get(sap_cli_param_info_t *param_info, char *raw_value, void *output);
static int sap_debug_get_all_port(int unit, int *input_list, int input_len, int *output_len);

static int sap_cli_dbg_on(int unit, int port, char **args, int argc);
static int sap_cli_dbg_off(int unit, int port, char **args, int argc);
static int sap_cli_phy_shell(int unit, int port, char **args, int argc);
static int sap_cli_dsc(int unit, int port, char **args, int argc);
static int sap_cli_eyescan(int unit, int port, char **args, int argc);
static int sap_cli_speed_set(int unit, int port, char **args, int argc);
static int sap_cli_prbs_set(int unit, int port, char **args, int argc);
static int sap_cli_prbs_get(int unit, int port, char **args, int argc);
static int sap_cli_prbs_clear(int unit, int port, char **args, int argc);
static int sap_cli_prbsstat_start(int unit, int port, char **args, int argc);
static int sap_cli_prbsstat_ber(int unit, int port, char **args, int argc);
static int sap_cli_prbsstat_clear(int unit, int port, char **args, int argc);
static int sap_cli_phy_status(int unit, int port, char **args, int argc);
static int sap_cli_polarity_set(int unit, int port, char **args, int argc);
static int sap_cli_polarity_get(int unit, int port, char **args, int argc);
static int sap_cli_phy_reset(int unit, int port, char **args, int argc);
static int sap_cli_squelch_set(int unit, int port, char **args, int argc);
static int sap_cli_loopback_set(int unit, int port, char **args, int argc);
static int sap_cli_loopback_get(int unit, int port, char **args, int argc);
static int sap_cli_autoneg_set(int unit, int port, char **args, int argc);
static int sap_cli_autoneg_get(int unit, int port, char **args, int argc);
static int sap_cli_temperature_get(int unit, int port, char **args, int argc);
static int sap_cli_highest_temperature_get(int unit, int port, char **args, int argc);
static int sap_cli_linktrain_set(int unit, int port, char **args, int argc);
static int sap_cli_tx_fir_set(int unit, int port, char **args, int argc);
static int sap_cli_tx_fir_get(int unit, int port, char **args, int argc);
static int sap_cli_setreg(int unit, int port, char **args, int argc);
static int sap_cli_getreg(int unit, int port, char **args, int argc);
static int sap_cli_mib_dump(int unit, int port, char **args, int argc);
static int sap_cli_channel_reach(int unit, int port, char **args, int argc);
static int sap_cli_techsupport(int unit, int port, char **args, int argc);
static int sap_cli_fw_version(int unit, int port, char **args, int argc);
static int sap_cli_eeprom_update(int unit, int port, char **args, int argc);
static int sap_cli_show_techsupport(int unit, int port, char **args, int argc);
static int sap_cli_show_portmap(int unit, int port, char **args, int argc);
static int sap_cli_show_help(int unit, int port, char **args, int argc);
static int sap_cli_show_version(int unit, int port, char **args, int argc);
static int sap_cli_port_status(int unit, int port, char **args, int argc);
static int sap_cmd_param_help(const char *param_key);
static int sap_cmd_doc_help(char *doc);
static void remove_brackets(const char* src, char* dst);

struct list_head g_prbsstat_info_list_head;
static int prbsstat_inited = 0;
static pthread_mutex_t climsg_mutex;
/* 重定向sdk输出log */
static int fd_stdout, fd_redirect;
static int g_time_count = 0;
/* 防止同时执行命令行出现异常，加个锁 */
pthread_mutex_t debug_mutex = PTHREAD_MUTEX_INITIALIZER;
char *fec_str[] = {"INVALID", "NONE", "BASER", "RS", "RS544", "RS272", "RS206", "RS108", "RS545", "RS304", "RS544_2XN", "RS272_2XN"};

sap_debug_command_t commands[] = {
    { "dbg_on",                   sap_cli_dbg_on,                   "", "debug mode off"},
    { "dbg_off",                  sap_cli_dbg_off,                  "", "debug mode on"},
    { "phy_shell",                sap_cli_phy_shell,                "{port} ", "execute cmd using phy sdk shell"},
    { "dsc",                      sap_cli_dsc,                      "{port} if_side=<?> [lane=<1-8>] [flag=<?>]", "dump serdes config"},
    { "eyescan",                  sap_cli_eyescan,                  "{port} if_side=<?> [lane=<1-8>]", "dump serdes eyescan" },
    { "speed_set",                sap_cli_speed_set,                "{port} speed=<?>", "set port speed(TODO)"},
    { "prbs_set",                 sap_cli_prbs_set,                 "{port} if_side=<?> poly=<?> [inv=<?>] [lane=<1-8>]", "set port prbs start"},
    { "prbs_get",                 sap_cli_prbs_get,                 "{port} if_side=<?>", "get port prbs status"},
    { "prbs_clear",               sap_cli_prbs_clear,               "{port} if_side=<?>", "stop port prbs"},
    { "prbsstat_start",           sap_cli_prbsstat_start,           "{port} if_side=<?>", "set port prbs statist start"},
    { "prbsstat_ber",             sap_cli_prbsstat_ber,             "{port} if_side=<?>", "show port prbs-ber statistic"},
    { "prbsstat_clear",           sap_cli_prbsstat_clear,           "{port} if_side=<?>", "clear port prbs-ber statistic"},
    { "phy_status",               sap_cli_phy_status,               "{port} [level=<?>]", "dump phy status"},
    { "polarity_set",             sap_cli_polarity_set,             "{port} if_side=<?> tx_rx=<?> polarity=<?> [override=<?>]", "set serdes polarity invert"},
    { "polarity_get",             sap_cli_polarity_get,             "{port} if_side=<?> tx_rx=<?>", "get serdes polarity"},
    { "phy_reset",                sap_cli_phy_reset,                "{port} [flag=<?>]", "reset phy with soft-reset or hard-reset flag"},
    { "squelch_set",              sap_cli_squelch_set,              "{port} if_side=<?> tx_rx=<?> enable=<?> [lane=<1-8>]", "set serdes squelch enable/disable"},
    { "loopback_set",             sap_cli_loopback_set,             "{port} if_side=<?> lb_dir=<?> enable=<?>", "set port loopback mode"},
    { "loopback_get",             sap_cli_loopback_get,             "{port} if_side=<?> [lb_dir=<?>]", "get port loopback mode"},
    { "autoneg_set",              sap_cli_autoneg_set,              "{port} if_side=<?> enable=<?>", "set port autoneg mode"},
    { "autoneg_get",              sap_cli_autoneg_get,              "{port} if_side=<?> ", "get port autoneg mode"},
    { "temperature_get",          sap_cli_temperature_get,          "{port} ", "get phy temperature"},
    { "highest_temperature_get",  sap_cli_highest_temperature_get,  "", "get highest temperature among all phy"},
    { "linktrain_set",            sap_cli_linktrain_set,            "{port} if_side=<?> enable=<?>", "set serdes lintrain enable/disable"},
    { "tx_fir_set",               sap_cli_tx_fir_set,               "{port} if_side=<?> lane=<1-8> main=<?> pre=<?> post=<?> pre2=<?> post2=<?> pre3=<?> post3=<?>", "set serdes tx_fir value"},
    { "tx_fir_get",               sap_cli_tx_fir_get,               "{port} if_side=<?> lane=<1-8>", "get serdes tx_fir"},
    { "setreg",                   sap_cli_setreg,                   "{port} if_side=<?> [devaddr=<?>] regaddr=<?> data_64=<?>", "set phy register value"},
    { "getreg",                   sap_cli_getreg,                   "{port} if_side=<?> [devaddr=<?>] regaddr=<?>", "get phy register value"},
    { "mib_dump",                 sap_cli_mib_dump,                 "{port} if_side=<?>", "collect port mib logs"},
    { "channel_reach",            sap_cli_channel_reach,            "{port} if_side=<?> nr_er=<?> lane=<1-8>", "set serdes nr/er mode"},
    { "techsupport",              sap_cli_techsupport,              "{port} ", "collect port techsupport logs"},
    { "fw_version",               sap_cli_fw_version,               "{port} ", "show phy firmware version"},
    { "eeprom_update",            sap_cli_eeprom_update,            "", "update extphy flash" },
    { "show_techsupport",         sap_cli_show_techsupport,         "", "collect all port techsupport logs"},
    { "show_portmap",             sap_cli_show_portmap,             "", "display port map table"},
    { "port_status",              sap_cli_port_status,              "", "display port status table"},
    { "ps",                       sap_cli_port_status,              "", "same as port_status"},
    { "version",                  sap_cli_show_version,             "", "show saphy program version"},
    { "help",                     sap_cli_show_help,                "", "show help"},
    { "?",                        sap_cli_show_help,                "", "same as `help'"},
    { (char *)NULL,               (sap_cmd_func_t)NULL,              (char *)NULL }
};

static sap_cli_param_info_t params_info[] = {
    PARAM_VALUE_TYPE_INT("unit",      &g_params.unit),
    PARAM_VALUE_TYPE_INT("port",      &g_params.port),
    PARAM_VALUE_TYPE_INT("speed",     &g_params.speed),
    PARAM_VALUE_TYPE_INT("lane",      &g_params.lane),
    PARAM_VALUE_TYPE_INT("flag",      &g_params.flag),
    PARAM_VALUE_TYPE_INT("level",     &g_params.level),
    PARAM_VALUE_TYPE_INT("pre",       &g_params.tx_fir.pre),
    PARAM_VALUE_TYPE_INT("pre2",      &g_params.tx_fir.pre2),
    PARAM_VALUE_TYPE_INT("pre3",      &g_params.tx_fir.pre3),
    PARAM_VALUE_TYPE_INT("main",      &g_params.tx_fir.main),
    PARAM_VALUE_TYPE_INT("post",      &g_params.tx_fir.post),
    PARAM_VALUE_TYPE_INT("post2",     &g_params.tx_fir.post2),
    PARAM_VALUE_TYPE_INT("post3",     &g_params.tx_fir.post3),
    PARAM_VALUE_TYPE_BOOL("inv",      &g_params.inv),
    PARAM_VALUE_TYPE_BOOL("override", &g_params.override),
    PARAM_VALUE_TYPE_BOOL("enable",   &g_params.enable),
    PARAM_VALUE_TYPE_ENUM(
        "if_side",
        &g_params.if_side,
        PARAM_LIST(
            {"line",  SAP_PORT_SIDE_LINE},
            {"sys",   SAP_PORT_SIDE_SYS},
            {"host",  SAP_PORT_SIDE_SYS}
        )
    ),
    PARAM_VALUE_TYPE_ENUM(
        "lb_dir",
        &g_params.lb_dir,
        PARAM_LIST(
            {"local",  SAP_LOOPBACK_DIR_LOCAL},
            {"remote", SAP_LOOPBACK_DIR_REMOTE},
            {"analog", SAP_LOOPBACK_DIR_ANALOG}
        )
    ),
    PARAM_VALUE_TYPE_ENUM(
        "tx_rx",
        &g_params.tx_rx,
        PARAM_LIST(
            {"tx", SAP_CHANNEL_TX},
            {"rx", SAP_CHANNEL_RX}
        )
    ),
    PARAM_VALUE_TYPE_ENUM(
        "nr_er",
        &g_params.nr_er,
        PARAM_LIST(
            {"nr", SAP_CHANNEL_RANGE_NR},
            {"er", SAP_CHANNEL_RANGE_ER}
        )
    ),
    PARAM_VALUE_TYPE_ENUM(
        "poly",
        &g_params.poly,
        PARAM_LIST(
            {"p7",  SAP_PRBS_POLY_P7},
            {"p15", SAP_PRBS_POLY_P15},
            {"p23", SAP_PRBS_POLY_P23},
            {"p31", SAP_PRBS_POLY_P31},
            {"p9",  SAP_PRBS_POLY_P9},
            {"p11", SAP_PRBS_POLY_P11},
            {"p58", SAP_PRBS_POLY_P58},
            {"p49", SAP_PRBS_POLY_P49},
            {"p20", SAP_PRBS_POLY_P20},
            {"p13", SAP_PRBS_POLY_P13},
            {"p10", SAP_PRBS_POLY_P10}
        )
    ),
    PARAM_VALUE_TYPE_UINT("polarity",  &g_params.polarity),
    PARAM_VALUE_TYPE_UINT("data",      &g_params.data),
    PARAM_VALUE_TYPE_UINT("devaddr",   &g_params.devaddr),
    PARAM_VALUE_TYPE_UINT("regaddr",   &g_params.regaddr),
    PARAM_VALUE_TYPE_UINT64("data_64", &g_params.data_64),
};

/* 作为.so库集成到到MAC-SDK时使用 */
int sap_debug_cmd_exec(int unit, int port, char** args, int n_arg)
{
    char *diag_func_name;
    if (n_arg <= 0) {
        PLP_CLI("n_arg error, n_arg:%d", n_arg);
        sap_cli_show_help(0, 0, NULL, 0);
        return -1;
    }
    diag_func_name = args[0];
    if (diag_func_name == NULL) {
        PLP_CLI("diag_func_name is missing!");
        sap_cli_show_help(0, 0, NULL, 0);
        return -1;
    }
    sap_debug_func_exec(unit, port, diag_func_name, &args[1], (n_arg - 1));
    fflush(NULL);
    return 0;
}

/* 命令行处理接口, 前台执行命令时, 或者sapcmd执行命令时调用 */
int sap_debug_line_exec(char *cmd, uint32_t cmd_len)
{
    int unit = 0, port = -1, n_arg = 0;
    char *args[MAX_DEBUG_ARG_NUM];
    char cString[MAX_DEBUG_STR_LEN];
    char *saveptr;
    char delim[] = " ";
    char *func_name;

    pthread_mutex_lock(&debug_mutex);
    snprintf(cString, cmd_len, "%s", cmd);
    char *token = strtok_r(cString, delim, &saveptr);
    n_arg = 0;
    while (token != NULL) {
        args[n_arg] = token;
        token = strtok_r(NULL, delim, &saveptr);
        n_arg++;
    }
    if (n_arg == 1) {
        func_name = args[0];
        sap_debug_func_exec(unit, port, func_name, NULL, 0);
    } else if (n_arg >= 2) {
        func_name = args[0];
        sap_debug_func_exec(unit, port, func_name, &args[1], (n_arg - 1));
    } else {
        sap_cli_show_help(0, 0, NULL, 0);
    }
    pthread_mutex_unlock(&debug_mutex);
    return SAP_STATUS_SUCCESS;;
}

int sapcmd_service_init(void)
{
    SAP_LOG_INFO("sapcmd service init\n");
    pthread_mutex_init(&climsg_mutex, NULL); 
    net_comm_init();
    NET_COMM_SERVICE_INFO_T service_info = {
        .host_name    = SAPCMD_SOCKET_PATH,
        .service_name = "sapcmd_service",
        .text_key     = NULL,
        .text_value   = NULL,
        .ipv4_addr    = NULL,
        .port         = 12345
    };
    net_comm_server_publish(&service_info, server_message_handle, server_connect_handle);
    return SAP_STATUS_SUCCESS;
}

static uint32_t sap_timestamp_get()
{
    struct timeval time;  
    gettimeofday(&time, NULL );
    return time.tv_sec * 1000 + time.tv_usec/1000;
}

static int redirect_stdout_to_sdkmsg()
{
    int ret;
    pthread_mutex_lock(&climsg_mutex);
    fflush(NULL);
    /* 保存当前的标准输出fd */
    fd_stdout = dup(STDOUT_FILENO);
    if (fd_stdout < 0) {
        SAP_LOG_WARN("dup fd_stdout fail");
    }
    /* 打开重定向的fd */
    fd_redirect = open(SAPMSG_BUF, O_RDWR | O_CREAT, 0666);
    if (fd_redirect < 0) {
        SAP_LOG_WARN("SAPMSG_BUF open fail, errno %d \n", errno);
        /* 还原默认输出 */
        ret = dup2(fd_stdout, STDOUT_FILENO);
        if (ret < 0) {
            SAP_LOG_WARN("dup2 fd_stdout fail ret %d\n", ret);
        }
        /* 关闭临时文件 */
        close(fd_stdout);
    }
    ret = dup2(fd_redirect, STDOUT_FILENO);
    if (ret < 0) {
        SAP_LOG_WARN("dup2 fd_redirect fail ret %d\n", ret);
    }
    pthread_mutex_unlock(&climsg_mutex);
    return ret;
}

static int redirect_stdout_back()
{
    int ret;
    pthread_mutex_lock(&climsg_mutex);
    if (fd_redirect <= 0) {
        pthread_mutex_unlock(&climsg_mutex);
        return 0;
    }
    fflush(NULL);
    /* 还原默认输出 */
    ret = dup2(fd_stdout, STDOUT_FILENO);
    if (ret < 0) {
        SAP_LOG_WARN("dup2 fd_stdout fail ret %d\n", ret);
    }
    /* 关闭临时文件 */
    close(fd_stdout);
    int size = lseek(fd_redirect,0,SEEK_END);
    if (size > SAPMSG_BUF_SIZE_MAX) {
        ftruncate(fd_redirect, SAPMSG_BUF_SIZE_MAX);
        SAP_LOG_WARN("sdkmsg buf overflow! size=%d\n", size);
    }
    fflush(NULL);
    close(fd_redirect);
    pthread_mutex_unlock(&climsg_mutex);
    return 0;
}

static void *server_message_handle(NET_CONN net_comm, char *data, uint32_t data_len)
{   
    redirect_stdout_to_sdkmsg();
    sap_debug_line_exec(data, data_len);
    redirect_stdout_back();
    net_comm_message_send(net_comm, PLP_CLI_PROCESS_OVER, strlen(PLP_CLI_PROCESS_OVER) + 1);
    return NULL;
}

static void *server_connect_handle(NET_CONN net_comm, NET_COMM_CONN_STATUS_E status)
{
    switch (status) {
    case NET_COMM_CONN_STATUS_CONNECTED: {
        SAP_LOG_INFO("%s, id: %d", "client connected", net_comm);
        break;
    }
    case NET_COMM_CONN_STATUS_CONNECT_LOST: {
        SAP_LOG_INFO("%s, id: %d", "client connect lost", net_comm);
        break;
    }
    case NET_COMM_CONN_STATUS_MDNS_QUERY_TIMEOUT: {
        SAP_LOG_INFO("%s, id: %d", "client connect timeout", net_comm);
        break;
    }
    default:
        break;
    }
    return NULL;
}

static void cli_params_parse_option(char **diag_params, int n_param, const char *option_key, bool *found)
{
    int param_i = 0;
    char *p = diag_params[param_i];

    while ((p != NULL) && (param_i++ < n_param)) {
        if (!strncmp(option_key, p, strlen(p))) {
            *found = true;
            return;
        }
        p = diag_params[param_i];
    }
    *found = false;
}

static void cli_params_parse_all(char **diag_params, int n_param)
{
    int i;
    int param_i=0;
    char *enum_val;
    int params_len = sizeof(params_info)/sizeof(sap_cli_param_info_t);
    
    for (param_i = 0; param_i < n_param; param_i++) {
        enum_val = strrchr(diag_params[param_i], '=');
        if (enum_val == NULL) {
            /* TODO: it's a option param*/
            continue;
        }
        enum_val++;
        for (i = 0; i < params_len; i++) {
            if (strncmp(diag_params[param_i], params_info[i].key, strlen(params_info[i].key))) {
                continue;
            }
            cli_param_value_get(&params_info[i], enum_val, params_info[i].value_ptr);
            break;
        }
        if (i >= params_len) {
            PLP_CLI("Invalid param: %s", diag_params[param_i]);
        }
    }
}

static int cli_params_parse_field(char **diag_params, int n_param, const char *field_key, void *field_val)
{
    int i, rv;
    int param_i;
    char *param_value = NULL;
    char *p;
    int params_len = sizeof(params_info)/sizeof(sap_cli_param_info_t);

    for (i = 0; i < params_len; i++) {
        if (strcmp(field_key, params_info[i].key)) {
            continue;
        }
        for (param_i = 0; param_i < n_param; param_i++) {
            p = diag_params[param_i];
            param_value = strrchr(p, '=');
            if (param_value == NULL) {
                continue;
            }
            if (!strncmp(field_key, p, strlen(field_key))) {
                param_value++;
                break;
            }
        }
        if (param_value == NULL || param_i >= n_param) {
            return SAP_STATUS_ITEM_NOT_FOUND;
        }
        rv = cli_param_value_get(&params_info[i], param_value, field_val);
        if (rv == SAP_STATUS_INVALID_PARAMETER) {
            return SAP_STATUS_INVALID_PARAMETER;
        }
        break;
    }
    if (i >= params_len) {
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    return SAP_STATUS_SUCCESS;
}

static int cli_param_value_get(sap_cli_param_info_t *param_info, char *raw_value, void *output)
{
    int j;
    char temp_buffer[MAX_PARAM_BUF_SIZE];

    if (!strcmp(raw_value, "?")) {
        printf("Supported Params:\n");
        sap_cmd_param_help(param_info->key);
        return SAP_STATUS_INVALID_PARAMETER;
    }

    switch (param_info->value_type)
    {
        case VALUE_TYPE_UINT:
            sap_str_to_uint32(raw_value, (uint32_t *)(output));
            break;
        case VALUE_TYPE_UINT_64:
            sap_str_to_uint64(raw_value, (uint64_t *)(output));
            break;
        case VALUE_TYPE_INT:
            sap_str_to_int(raw_value, (int *)(output));
            break;
        case VALUE_TYPE_STRING:
            snprintf(output, MAX_PARAM_BUF_SIZE, "%s", raw_value);
            break;
        case VALUE_TYPE_ENUM:
            for (j=0; j<MAX_PARAM_OPTION_NUM; j++) {
                /* 允许直接传入枚举值 */
                snprintf(temp_buffer, sizeof(temp_buffer), "%d", param_info->enum_info[j].value_int);
                if (!strcmp(raw_value, temp_buffer)) {
                    sap_str_to_int(raw_value, (int *)output);
                    break;
                }
                /* 通过枚举类型获取枚举值 */
                if (strncmp(param_info->enum_info[j].value_str, raw_value, strlen(raw_value))) {
                    continue;
                }
                *(int *)(output) = param_info->enum_info[j].value_int;
                break;
            }
            /* 没有对应枚举类型 */
            if (j>=MAX_PARAM_OPTION_NUM) {
                PLP_CLI("Enum:%s is not supported\nSupported enum values:", raw_value);
                sap_cmd_param_help(param_info->key);
                return SAP_STATUS_INVALID_PARAMETER;
            }
            break;
        case VALUE_TYPE_BOOL:
            if (!strncmp((raw_value), "false", strlen(raw_value))) {
                *(int *)(output) = 0;
            }
            else if (!strncmp((raw_value), "0", strlen("0"))) {
                *(int *)(output) = 0;
            }
            else if (!strncmp((raw_value), "true", strlen(raw_value))) {
                *(int *)(output) = 1;
            }
            else if (!strncmp((raw_value), "1", strlen("1"))) {
                *(int *)(output) = 1;
            } else {
                sap_cmd_param_help(param_info->key);
                return SAP_STATUS_INVALID_PARAMETER;
            }
            break;
        default:
            PLP_CLI("Invalid param value type: %d", param_info->value_type);
            return SAP_STATUS_ITEM_NOT_FOUND;
            break;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_debug_get_all_port(int unit, int *input_list, int input_len, int *output_len)
{
    int i, rv;
    sap_port_list_t port_list[EXTPHY_APIS_NUMBER];
    sap_port_info_t *port_info;
    int port_id = 0;

    *output_len = 0;
    memset(input_list, 0, sizeof(int) * input_len);
    memset(&port_list, 0, sizeof(sap_port_list_t));
    rv = sap_port_list_get(port_list);
    if (rv != SAP_STATUS_SUCCESS) {
        PLP_CLI("sap_port_list_get failed, rv=%d", rv);
        return rv;
    }
    for (i=0; i< EXTPHY_APIS_NUMBER; i++) {
        if (port_list[i].list == NULL) {
            continue;
        }
        *output_len += port_list[i].port_count;
        SAP_PLIST_ITER(&port_list[i], port_info) {
            if (unit != port_info->unit) {
                continue;
            }
            input_list[port_id++] = port_info->port;
        }
    }
    return SAP_STATUS_SUCCESS;
}

static void sap_debug_func_exec(int unit, int port, char *diag_func_name, char **diag_params, int n_param)
{
    int i,rv;
    sap_debug_command_t *command;
    int ports[SAP_MAX_PORT_NUM];
    int port_num;

    if (prbsstat_inited != 1) {
        INIT_LIST_HEAD(&g_prbsstat_info_list_head);
        prbsstat_inited = 1;
    }
    command = find_command(diag_func_name);
    if (command == NULL) {
        sap_cli_show_help(0, 0, &diag_func_name, 1);
        return;
    }
    for (i = 0; i<n_param; i++) {
        PLP_CLI_DEBUG("args[%d]: %s", i, diag_params[i]);
    }
    if (port == -1 && n_param > 0) {
        if (strcmp(diag_params[0], "all") == 0) {
            rv = sap_debug_get_all_port(unit, ports, SAP_MAX_PORT_NUM, &port_num);
            if (rv != SAP_STATUS_SUCCESS) {
                PLP_CLI("get all ports failed, rv: %d", rv);
                return;
            }
        } else if (strcmp(diag_params[0], "?") == 0){
            sap_cli_show_help(0, 0, &diag_func_name, 1);
            return;
        } else {
            sap_parse_int_list(diag_params[0], ports, SAP_MAX_PORT_NUM, &port_num);
        }
    } else {
        port_num = 1;
        ports[0] = port;
    }
    // cli_params_parse_all(diag_params, n_param);
    port_num = (port_num > SAP_MAX_PORT_NUM) ? SAP_MAX_PORT_NUM : port_num;
    for (i = 0; i < port_num; i++) {
        port = ports[i];
        rv = command->func(unit, port, diag_params, n_param);
        if (rv != SAP_STATUS_NOT_SUPPORTED_PORT) {
            PLP_CLI("unit: %d, port: %d, %s %s, rv: %d", unit, port, diag_func_name, (rv == SAP_STATUS_SUCCESS) ? "success" : "failed", rv);
        } else {
            PLP_CLI("unit: %d, port: %d, %s not supported", unit, port, diag_func_name);
        }
    }
}

static int sap_cli_show_version(int unit, int port, char **args, int argc)
{
    PLP_CLI("SAPHY_PROGRAM_VERSION: %s", SAPHY_PROGRAM_VERSION);
    /* TODO: show phy sdk version */
    return SAP_STATUS_SUCCESS;
}

/* Print out help for ARG, or for all of the commands if ARG is not present. */
static int sap_cli_show_help(int unit, int port, char **args, int argc)
{
    int i;
    int printed = 0;
    if (args == NULL || argc <= 0) {
        printf("Commands:\n");
        for (i = 0; commands[i].name; i++) {
            printf("\t%-20s\t\t%s.\n", commands[i].name, commands[i].annotation);
        }
    } else {
        for (i = 0; commands[i].name; i++) {
            if (!args[0] || (strncmp (args[0], commands[i].name, strlen(args[0])) == 0)) {
                printf("Command:\n");
                printf("\t%-20s\t\t%s.\n", commands[i].name, commands[i].annotation);
                printf("Supported Params:\n");
                sap_cmd_doc_help(commands[i].doc);
                printf("Example:\n\t%s %s\n", commands[i].name, commands[i].doc);
                printed++;
                printf("----------------------------------------------------------------\n");
            }
        }
        if (!printed) {
            printf("No commands match `%s'.  Possibilities are:\n", args[0]);
            for (i = 0; commands[i].name; i++) {
                /* Print in six columns. */
                if (printed == 6) {
                    printed = 0;
                    printf("\n");
                }
                printf("%-20s\t", commands[i].name);
                printed++;
            }
            if (printed) {
                printf("\n");
            }
        }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_cmd_doc_help(char *doc)
{
    char *saveptr;
    char delim[] = " ";
    char string[256];
    char param_key[256];
    snprintf(string, sizeof(string), "%s", doc);
    char *key_value = strtok_r(string, delim, &saveptr);
    char *key;

    while (key_value != NULL) {
        key = strrchr(key_value, '=');
        if (key != NULL) {
            snprintf(param_key, key-key_value+1, "%s", key_value);
            remove_brackets(param_key, param_key);
            sap_cmd_param_help(param_key);
        }
        key_value = strtok_r(NULL, delim, &saveptr);
    }
    return SAP_STATUS_SUCCESS;
}

/* Print out help for ARG, or for all of the commands if ARG is not present. */
static int sap_cmd_param_help(const char *param_key)
{
    int i, j;
    int printed = 0;
    char temp_buffer[MAX_PARAM_BUF_SIZE*2];
    sap_cli_param_info_t *param_info;

    for (i = 0; i < sizeof(params_info)/sizeof(sap_cli_param_info_t); i++) {
        if (strcmp(param_key, params_info[i].key) != 0) {
            continue;
        }
        param_info = &params_info[i];
        printf("\t%s:\n", param_key);
        printed++;
        switch (params_info[i].value_type)
        {
        case VALUE_TYPE_INT:
            printf("\t\t32bit Interger\n");
            break;
        case VALUE_TYPE_UINT:
            printf("\t\t32bit Unsigned\n");
            break;
        case VALUE_TYPE_UINT_64:
            printf("\t\t64bit Unsigned\n");
            break;
        case VALUE_TYPE_STRING:
            printf("\t\tString\n");
            break;
        case VALUE_TYPE_ENUM:
            for (j = 0; j<MAX_PARAM_OPTION_NUM; j++) {
                if (strlen(param_info->enum_info[j].value_str) <= 0) {
                    continue;
                }
                snprintf(temp_buffer, sizeof(temp_buffer), "\t\tEnum: %-10s Value: %d\n", param_info->enum_info[j].value_str, param_info->enum_info[j].value_int);
                printf("%s", temp_buffer);
            }
            break;
        case VALUE_TYPE_BOOL:
            printf("\t\tEnum: %-10s Value: 0\n", "false");
            printf("\t\tEnum: %-10s Value: 1\n", "true");
            break;
        default:
            break;
        }
    }
    if (printed == 0) {
        printf("Invalid param key: %s\n", param_key);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_cli_dbg_on(int unit, int port, char **args, int argc)
{
    sap_diag_log_reg(g_diag_log_func);
    g_cli_debug_mode = true;
    PLP_CLI_DEBUG("DEBUG ON");
    return SAP_STATUS_SUCCESS;
}

static int sap_cli_dbg_off(int unit, int port, char **args, int argc)
{
    PLP_CLI_DEBUG("DEBUG OFF");
    g_cli_debug_mode = false;
    sap_diag_log_reg(NULL);
    return SAP_STATUS_SUCCESS;
}

static int sap_cli_phy_shell(int unit, int port, char **args, int argc)
{
    int rv;
    char *shell_cmd;
    if (argc == 0 || args[0] == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    shell_cmd = args[1];
    PLP_CLI_DEBUG("sap_cli_phy_shell(unit=%d, port=%d, shell_cmd=%s);", unit, port, shell_cmd);
    return sap_shell_cmd_run(unit, port, shell_cmd);
}

static int sap_cli_dsc(int unit, int port, char **args, int argc)
{
    int if_side, flag, lane;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("flag",    0, &flag);
    SAP_INT_PARAM_SET("lane",    0, &lane);

    PLP_CLI_DEBUG("sap_cli_dsc(unit=%d, port=%d, if_side=%d, flag=%d, lane=%d);", unit, port, if_side, flag, lane);
    return sap_dsc_dump(unit, port, if_side, flag, lane);
}

static int sap_cli_eyescan(int unit, int port, char **args, int argc)
{
    int if_side, flag, lane;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("lane",    0, &lane);

    PLP_CLI_DEBUG("sap_cli_eyescan(unit=%d, port=%d, if_side=%d, lane=%d);", unit, port, if_side, lane);
    return sap_eyescan_dump(unit, port, if_side, lane);
}

static int sap_cli_speed_set(int unit, int port, char **args, int argc)
{
    int speed;

    SAP_INT_PARAM_SET("speed", 0, &speed);

    PLP_CLI_DEBUG("sap_port_speed_set(unit=%d, port=%d, speed=%d);", unit, port, speed);

    return sap_port_speed_set(unit, port, speed);
}

static int sap_cli_prbs_set(int unit, int port, char **args, int argc)
{
    int poly, inv, if_side, lane;

    SAP_INT_PARAM_SET("poly",     0, &poly);
    SAP_INT_PARAM_SET("inv",      0, &inv);
    SAP_INT_PARAM_SET("if_side",  0, &if_side);
    SAP_INT_PARAM_SET("lane",     0, &lane);

    PLP_CLI_DEBUG("sap_port_prbs_set(unit=%d, port=%d, poly=%d, inv=%d, if_side=%d, lane=%d);", 
        unit, port, poly, inv, if_side, lane);

    return sap_port_prbs_set(unit, port, poly, inv, if_side, lane);
}

static int sap_debug_prbsstat_update(int unit, int port, int if_side, sap_prbs_status_t *prbs_info)
{
    int rv;
    bool entry_find = false;
    sap_prbsstat_info_t *prbsstat_info;
    // sap_prbs_status_t prbs_info[PRBS_MAX_LANE_NUM];

    if (!sap_port_support(unit, port)) {
        return SAP_STATUS_NOT_SUPPORTED_PORT;
    }
    list_for_each_entry(prbsstat_info, &g_prbsstat_info_list_head, node, sap_prbsstat_info_t) {
        if (prbsstat_info->unit != unit) {
            continue;
        }
        if (prbsstat_info->port != port) {
            continue;
        }
        if (prbsstat_info->if_side != if_side) {
            continue;
        }
        entry_find = true;
        break;
    }
    if (entry_find == false) {
        prbsstat_info = (sap_prbsstat_info_t *)malloc(sizeof(sap_prbsstat_info_t));
        if (prbsstat_info == NULL) {
            PLP_CLI("malloc failed!");
            return SAP_STATUS_NO_MEMORY;
        }
        list_add_tail(&prbsstat_info->node, &g_prbsstat_info_list_head);
    }
    prbsstat_info->unit = unit;
    prbsstat_info->port = port;
    prbsstat_info->if_side = if_side;
    memset(prbs_info, 0, sizeof(sap_prbs_status_t)*PRBS_MAX_LANE_NUM);
    rv = sap_port_prbs_get(unit, port, if_side, prbs_info);
    if (rv != 0) {
        PLP_CLI("sap_port_prbs_get failed!");
        return SAP_STATUS_FAILURE;
    }
    prbsstat_info->timestamp = sap_timestamp_get();
    return SAP_STATUS_SUCCESS;
}

/* 每次get都清除一次prbs统计数值 */
static int sap_cli_prbs_get(int unit, int port, char **args, int argc)
{
    int i, rv, if_side;
    sap_prbs_status_t prbs_info[PRBS_MAX_LANE_NUM];
    memset(prbs_info, 0, sizeof(sap_prbs_status_t)*PRBS_MAX_LANE_NUM);

    SAP_INT_PARAM_SET("if_side",  0, &if_side);
    PLP_CLI_DEBUG("sap_port_prbs_get(unit=%d, port=%d, if_side=%d)", unit, port, if_side);
    rv = sap_debug_prbsstat_update(unit, port, if_side, prbs_info);
    if (rv != SAP_STATUS_SUCCESS) {
        return rv;
    }
    for (i = 0; i < PRBS_MAX_LANE_NUM; i++) {
        if (prbs_info[i].is_get) {
            PLP_CLI("port: %d, lane: %d, side: %d, prbs_lock: %d, lock_loss: %d, lane_err_cnt: %d", port, i, if_side,
            prbs_info[i].prbs_lock, prbs_info[i].lock_loss, prbs_info[i].lane_err_cnt);
        }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_cli_prbs_clear(int unit, int port, char **args, int argc)
{
    int if_side;

    SAP_INT_PARAM_SET("if_side",  0, &if_side);
    PLP_CLI_DEBUG("sap_port_prbs_clear(unit=%d, port=%d, if_side=%d)", unit, port, if_side);
    if (sap_cli_prbsstat_clear(unit, port, args, argc) != 0) {
        PLP_CLI("sap_cli_prbsstat_clear failed!");
    }
    return sap_port_prbs_clear(unit, port, if_side);
}

static int sap_cli_prbsstat_start(int unit, int port, char **args, int argc)
{
    int i, ret, if_side;
    sap_prbs_status_t prbs_info[PRBS_MAX_LANE_NUM];
    SAP_INT_PARAM_SET("if_side",  0, &if_side);
    PLP_CLI_DEBUG("sap_cli_prbsstat_update(unit=%d, port=%d, if_side=%d)", unit, port, if_side);
    ret =  sap_debug_prbsstat_update(unit, port, if_side, prbs_info);

    for (i = 0; i < PRBS_MAX_LANE_NUM; i++) {
        if ((prbs_info[i].is_get) && (prbs_info[i].prbs_lock != 1)) {
            PLP_CLI("port: %d, lane: %d, side: %d, prbs not lock!", port, i, if_side);
        }
    }
    return ret;
}

static int sap_cli_prbsstat_ber(int unit, int port, char **args, int argc)
{
    int i, rv;
    int if_side;
    int time_passed;
    bool entry_find = false;
    sap_prbsstat_info_t *prbsstat_info;
    sap_prbs_status_t prbs_info[PRBS_MAX_LANE_NUM];

    SAP_INT_PARAM_SET("if_side",  0, &if_side);
    PLP_CLI_DEBUG("sap_cli_prbsstat_ber(unit=%d, port=%d, if_side=%d)", unit, port, if_side);

    if (!sap_port_support(unit, port)) {
        return SAP_STATUS_NOT_SUPPORTED_PORT;
    }
    list_for_each_entry(prbsstat_info, &g_prbsstat_info_list_head, node, sap_prbsstat_info_t) {
        if (prbsstat_info->unit != unit) {
            continue;
        }
        if (prbsstat_info->port != port) {
            continue;
        }
        if (prbsstat_info->if_side != if_side) {
            continue;
        }
        entry_find = true;
        break;
    }
    if (entry_find == false) {
        PLP_CLI("prbsstat entry not found!");
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    time_passed = sap_timestamp_get() - prbsstat_info->timestamp + 1;
    printf("timepassed: %dms, side: %s\n", time_passed, (if_side == PLP_SYS_IF_SIDE) ? "SYS": "LINE");
    memset(prbs_info, 0, sizeof(sap_prbs_status_t)*PRBS_MAX_LANE_NUM);
    rv =  sap_port_prbs_ber_get(unit, port, if_side, time_passed, prbs_info);
    if (rv != 0) {
        PLP_CLI("sap_port_prbs_ber_dump failed");
    }
    prbsstat_info->timestamp = sap_timestamp_get();
    PLP_CLI("====");
    for (i = 0; i < PRBS_MAX_LANE_NUM; i++) {
        if (prbs_info[i].is_get) {
            if (prbs_info[i].prbs_lock == 0) {
                PLP_CLI("%d[%d] : Nolock", port, i);
            } else if (prbs_info[i].lock_loss == 1) {
                PLP_CLI("%d[%d] : LossOfLock", port, i);
            } else if (prbs_info[i].ber == 0){
                PLP_CLI("%d[%d] : OK!", port, i);
            } else {
                PLP_CLI("%d[%d] : %4.2e", port, i, prbs_info[i].ber);
            }
        }
        //  else {
        //     PLP_CLI("%d[%d] : !chk_en", port, i);
        // }
    }
    PLP_CLI("====");
    return rv;
}

static int sap_cli_prbsstat_clear(int unit, int port, char **args, int argc)
{
    int if_side;
    bool entry_find = false;
    sap_prbsstat_info_t *prbsstat_info;

    SAP_INT_PARAM_SET("if_side",  0, &if_side);
    PLP_CLI_DEBUG("sap_cli_prbsstat_clear(unit=%d, port=%d, if_side=%d)", unit, port, if_side);

    if (!sap_port_support(unit, port)) {
        return SAP_STATUS_NOT_SUPPORTED_PORT;
    }
    list_for_each_entry(prbsstat_info, &g_prbsstat_info_list_head, node, sap_prbsstat_info_t) {
        if (prbsstat_info->unit != unit) {
            continue;
        }
        if (prbsstat_info->port != port) {
            continue;
        }
        if (prbsstat_info->if_side != if_side) {
            continue;
        }
        entry_find = true;
        break;
    }
    if (entry_find == true) {
        list_del_init(&prbsstat_info->node);
        free(prbsstat_info);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_cli_phy_status(int unit, int port, char **args, int argc)
{
    int level;

    SAP_INT_PARAM_SET("level",  0, &level);
    return sap_phy_status_dump(unit, port, level);
}

static int sap_cli_polarity_set(int unit, int port, char **args, int argc)
{
    int if_side, tx_rx, override;
    uint32_t polarity=0;

    SAP_INT_PARAM_SET("if_side",  0, &if_side);
    SAP_INT_PARAM_SET("tx_rx",    0, &tx_rx);
    SAP_INT_PARAM_SET("override", 0, &override);
    SAP_INT_PARAM_SET("polarity", 0, &polarity);

    PLP_CLI_DEBUG("sap_polarity_set(unit=%d, port=%d, if_side=%d, tx_rx=%d, polarity=0x%x, override=%d)", 
        unit, port, if_side, tx_rx, polarity, override);

    return sap_polarity_set(unit, port, if_side, tx_rx, polarity, override);
}

static int sap_cli_polarity_get(int unit, int port, char **args, int argc)
{
    int rv, if_side, tx_rx;
    uint32_t polarity=0;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("tx_rx",   0, &tx_rx);

    PLP_CLI_DEBUG("sap_polarity_get(unit=%d, port=%d, if_side=%d, tx_rx=%d)", unit, port, if_side, tx_rx);
    rv = sap_polarity_get(unit, port, if_side, tx_rx, &polarity);
    if (rv == SAP_STATUS_SUCCESS) {
        PLP_CLI("unit: %d, port: %d, %s_%s_polarity: 0x%x", unit, port, (if_side==1)? "sys": "line", (tx_rx==PLP_TX_DIR)? "tx": "rx", polarity);
    }
    return rv;
}

static int sap_cli_phy_reset(int unit, int port, char **args, int argc)
{
    int flag;

    SAP_INT_PARAM_SET("flag", 0, &flag);

    PLP_CLI_DEBUG("sap_phy_reset(unit=%d, port=%d, flag=%d)", unit, port, flag);
    return sap_phy_reset(unit, port, flag);
}

static int sap_cli_squelch_set(int unit, int port, char **args, int argc)
{
    int if_side, tx_rx, enable, lane;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("tx_rx",   0, &tx_rx);
    SAP_INT_PARAM_SET("enable",  0, &enable);
    SAP_INT_PARAM_SET("lane",    0, &lane);

    PLP_CLI_DEBUG("sap_squelch_set(unit=%d, port=%d, if_side=%d, tx_rx=%d, enable=%d, lane=%d)", 
        unit, port, if_side, tx_rx, enable, lane);
    return sap_squelch_set(unit, port, if_side, tx_rx, enable, lane);
}

static int sap_cli_loopback_set(int unit, int port, char **args, int argc)
{
    int lb_dir, enable, if_side;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("lb_dir",  0, &lb_dir);
    SAP_INT_PARAM_SET("enable",  0, &enable);

    PLP_CLI_DEBUG("sap_cli_loopback_set(unit=%d, port= %d, if_side=%d, lb_dir=%d, enable=%d", unit, port, if_side, lb_dir, enable);
    return sap_loopback_set(unit, port, if_side, lb_dir, enable);
}

static int sap_cli_loopback_get(int unit, int port, char **args, int argc)
{
    int rv;
    int lb_dir, enable, if_side;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("lb_dir",  0, &lb_dir);

    if (lb_dir == SAP_LOOPBACK_DIR_BOTH) {
        for (lb_dir = SAP_LOOPBACK_DIR_LOCAL; lb_dir <=SAP_LOOPBACK_DIR_MAX; lb_dir++) {
            rv = sap_loopback_get(unit, port, if_side, lb_dir, &enable);
            PLP_CLI("%s, %s loopback: %s, rv: %d", ((if_side == PLP_SYS_IF_SIDE) ? "system side" : "line side"), 
                        (lb_dir == SAP_LOOPBACK_DIR_LOCAL) ? "Digital PMD" : ((lb_dir == SAP_LOOPBACK_DIR_REMOTE) ? "Remote PMD" : "Analog internal"), ((enable) ? "enable" : "disable"), rv);
        }
    } else {
        rv = sap_loopback_get(unit, port, if_side, lb_dir, &enable);
        PLP_CLI("%s, %s loopback: %s", ((if_side == PLP_SYS_IF_SIDE) ? "system side" : "line side"), 
                    (lb_dir == SAP_LOOPBACK_DIR_LOCAL) ? "Digital PMD" : ((lb_dir == SAP_LOOPBACK_DIR_REMOTE) ? "Remote PMD" : "Analog internal"), ((enable) ? "enable" : "disable"));
    }

    return SAP_STATUS_SUCCESS;
}

static int sap_cli_autoneg_set(int unit, int port, char **args, int argc)
{
    int lb_dir, enable, if_side;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("enable",  0, &enable);

    PLP_CLI_DEBUG("sap_cli_autoneg_set(unit=%d, port= %d, if_side=%d, enable=%d", unit, port, if_side, enable);
    return sap_autoneg_set(unit, port, if_side, enable);
}

static int sap_cli_autoneg_get(int unit, int port, char **args, int argc)
{
    int rv;
    int lb_dir, enable, if_side;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);

    rv = sap_autoneg_get(unit, port, if_side, &enable);
    PLP_CLI("%s autoneg: %s, rv: %d", ((if_side == PLP_SYS_IF_SIDE) ? "system side" : "line side"), ((enable) ? "enable" : "disable"), rv);

    return SAP_STATUS_SUCCESS;
}

static int sap_cli_temperature_get(int unit, int port, char **args, int argc)
{
    int rv;
    double temp;
    rv = sap_temperature_get(unit, port, &temp);
    if (rv == SAP_STATUS_SUCCESS) {
        PLP_CLI("temperature: %f", temp);
    }
    return rv;
}

static int sap_cli_highest_temperature_get(int unit, int port, char **args, int argc)
{
    int rv;
    double temp;
    rv = sap_highest_temperature_get(&temp);
    if (rv == SAP_STATUS_SUCCESS) {
        PLP_CLI("highest_temperature: %f", temp);
    }
    return rv;
}

static int sap_cli_linktrain_set(int unit, int port, char **args, int argc)
{
    int if_side, enable;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("enable",  0, &enable);

    PLP_CLI_DEBUG("sap_cli_linktrain_set(unit=%d, port=%d, if_side=%d, enable=%d)", unit, port, if_side, enable);
    return sap_port_linktrain_set(unit, port, if_side, enable);
}

static int sap_cli_tx_fir_set(int unit, int port, char **args, int argc)
{
    int if_side, lane;
    sap_tx_fir_t tx_fir;

    memset(&tx_fir, 0, sizeof(sap_tx_fir_t));
    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("lane",    0, &lane);
    SAP_INT_PARAM_SET("pre",     0, &tx_fir.pre);
    SAP_INT_PARAM_SET("pre2",    0, &tx_fir.pre2);
    SAP_INT_PARAM_SET("pre3",    0, &tx_fir.pre3);
    SAP_INT_PARAM_SET("main",    0, &tx_fir.main);
    SAP_INT_PARAM_SET("post",    0, &tx_fir.post);
    SAP_INT_PARAM_SET("post2",   0, &tx_fir.post2);
    SAP_INT_PARAM_SET("post3",   0, &tx_fir.post3);

    PLP_CLI_DEBUG("sap_cli_tx_fir_set(unit=%d, port=%d, if_side=%d, lane=%d, tx_fir(pre3=%d, pre2=%d, pre=%d, main=%d, post=%d, post2=%d, post3=%d))", 
        unit, port, if_side, lane, tx_fir.pre3, tx_fir.pre2, tx_fir.pre, tx_fir.main, tx_fir.post, tx_fir.post2, tx_fir.post3);

    return sap_tx_fir_set(unit, port, if_side, lane, tx_fir);
}

static int sap_cli_tx_fir_get(int unit, int port, char **args, int argc)
{
    int rv;
    int if_side, lane;
    sap_tx_fir_t tx_fir;
    memset(&tx_fir, 0, sizeof(sap_tx_fir_t));

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("lane",    0, &lane);

    PLP_CLI_DEBUG("sap_cli_tx_fir_get(unit=%d, port=%d, if_side=%d, lane=%d)", unit, port, if_side, lane);
    rv = sap_tx_fir_get(unit, port, if_side, lane, &tx_fir);

    PLP_CLI("port: %d, pre3=%d, pre2=%d pre=%d main=%d post=%d post2=%d post3=%d", 
             port, tx_fir.pre3, tx_fir.pre2, tx_fir.pre, tx_fir.main, tx_fir.post, tx_fir.post2, tx_fir.post3);
    return rv;
}

static int sap_cli_setreg(int unit, int port, char **args, int argc)
{
    uint64_t data_64;
    int if_side, flag;
    uint32_t devaddr, regaddr;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("flag",    0, &flag);
    SAP_INT_PARAM_SET("devaddr", 0, &devaddr);
    SAP_INT_PARAM_SET("regaddr", 0, &regaddr);
    SAP_INT_PARAM_SET("data_64", 0, &data_64);

    PLP_CLI_DEBUG("sap_cli_setreg(unit=%d, port=%d, if_side=%d, devaddr=0x%x, regaddr=0x%x, flag=%d, data=0x%lx)", unit, port, if_side, devaddr, regaddr, flag, data_64);
    return sap_reg_set(unit, port, if_side, devaddr, regaddr, flag, data_64);
}

static int sap_cli_getreg(int unit, int port, char **args, int argc)
{
    int rv;
    uint64_t data_64 = 0;
    int if_side, flag;
    uint32_t devaddr, regaddr;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("flag",    0, &flag);
    SAP_INT_PARAM_SET("devaddr", 0, &devaddr);
    SAP_INT_PARAM_SET("regaddr", 0, &regaddr);
    
    PLP_CLI_DEBUG("sap_cli_getreg(unit=%d, port=%d, if_side=%d, devaddr=0x%x, regaddr=0x%x, flag=%d)", unit, port, if_side, devaddr, regaddr, flag);
    rv = sap_reg_get(unit, port, if_side, devaddr, regaddr, flag, &data_64);
    if (rv == SAP_STATUS_SUCCESS) {
        PLP_CLI("devaddr=0x%x, regaddr=0x%x, data=0x%lx", devaddr, regaddr, data_64);
    }
    return rv;
}

static int sap_cli_mib_dump(int unit, int port, char **args, int argc)
{
    int if_side;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);

    PLP_CLI_DEBUG("sap_cli_mib_dump(unit=%d, port=%d, if_side: %d)", unit, port, if_side);
    return sap_mib_dump(unit, port, if_side);
}

static int sap_cli_channel_reach(int unit, int port, char **args, int argc)
{
    int rv;
    int if_side, lane, nr_er;

    SAP_INT_PARAM_SET("if_side", 0, &if_side);
    SAP_INT_PARAM_SET("lane",    0, &lane);
    SAP_INT_PARAM_SET("nr_er",   0, &nr_er);

    PLP_CLI_DEBUG("sap_cli_channel_reach(unit=%d, port=%d, if_side: %d, lane: %d, nr_er: %d)", unit, port, if_side, lane, nr_er);
    return sap_channel_reach_set(unit, port, if_side, lane, nr_er);
}

static int sap_cli_techsupport(int unit, int port, char **args, int argc)
{
    sap_techsupport(unit, port);
    return SAP_STATUS_SUCCESS;
}

static int sap_cli_fw_version(int unit, int port, char **args, int argc)
{
    int rv;
    sap_version_t version_info;

    memset(&version_info, 0, sizeof(version_info));
    rv = sap_fw_version_get(unit, port, &version_info);
    if (rv == SAP_STATUS_SUCCESS) {
        PLP_CLI("phy_id:0x%x, fw_ver:0x%x fw_crc:0x%x (rev_id:0x%x), drv_ver:%s.%d.%d (built %s %s)", 
            version_info.phy_id, version_info.fw_ver, version_info.fw_crc, version_info.rev_id, version_info.chip_name, 
            version_info.drv_major_ver, version_info.drv_minor_ver, __DATE__, __TIME__);
    }
    return rv;
}

static int sap_cli_eeprom_update(int unit, int port, char **args, int argc)
{
    int rv;
    bool use_file;

    SAP_INT_PARAM_SET("use_file", 0, &use_file);

    rv = sap_eeprom_update(unit, port, use_file);
    if (rv == SAP_STATUS_NOT_IMPLEMENTED) {
        PLP_CLI("func not implemented");
    } else if (rv == SAP_STATUS_SUCCESS) {
        PLP_CLI("eeprom update success");
    } else {
        PLP_CLI("eeprom update fail");
    }
    return rv;
}

static int sap_cli_show_techsupport(int unit, int port, char **args, int argc)
{
    sap_techsupport(unit, -1);
    return SAP_STATUS_SUCCESS; 
}

static int sap_cli_show_portmap(int unit, int port, char **args, int argc)
{
    int i, j, rv;
    int total_port_num = 0;
    int port_id = 0;
    char buffer[SAP_MAX_TABLE_CELL];
    sap_port_list_t port_list[EXTPHY_APIS_NUMBER];
    sap_port_info_t *port_info;
    char *header[1][SAP_MAX_TABLE_COL] = {
        {"Index", "Port", "Phy Lane0", "Chip Type"}
    };
    memset(&port_list, 0, sizeof(port_list));
    rv = sap_port_list_get(port_list);
    if (rv != SAP_STATUS_SUCCESS) {
        PLP_CLI("sap_port_list_get failed, rv=%d", rv);
        return SAP_STATUS_FAILURE;
    }
    for (i=0; i< EXTPHY_APIS_NUMBER; i++) {
        total_port_num += port_list[i].port_count;
    }
    char *table[total_port_num][SAP_MAX_TABLE_COL];
    for (i=0; i< EXTPHY_APIS_NUMBER; i++) {
        if (port_list[i].list == NULL) {
            continue;
        }
        SAP_PLIST_ITER(&port_list[i], port_info) {
            snprintf(buffer, SAP_MAX_TABLE_CELL, "%d", port_id);
            table[port_id][0] = strdup(buffer);
            snprintf(buffer, SAP_MAX_TABLE_CELL, "%d", port_info->port);
            table[port_id][1] = strdup(buffer);
            snprintf(buffer, SAP_MAX_TABLE_CELL, "%d", port_info->phy_lane0);
            table[port_id][2] = strdup(buffer);
            snprintf(buffer, SAP_MAX_TABLE_CELL, "%s", port_list[i].type);
            table[port_id][3] = strdup(buffer);
            port_id++;
        }
    }
    print_table_header(table, total_port_num, header, 1, 4);
    int start = 0;
    for (i=0; i< EXTPHY_APIS_NUMBER; i++) {
        if (port_list[i].list == NULL) {
            continue;
        }
        start += port_list[i].port_count;
        print_table_without_header(table, start - port_list[i].port_count, port_list[i].port_count, total_port_num, header, 1, 4);
    }  
    table_memfree(table, total_port_num, header, 1, 4);
    return SAP_STATUS_SUCCESS;
}

static int sap_cli_port_status(int unit, int port, char **args, int argc)
{
    int i, j, rv;
    int total_port_num = 0;
    int port_id = 0;
    sap_fec_e sap_fec;
    char *sys_fec_str = "FAILED", *line_fec_str = "FAILED";
    char buffer[SAP_MAX_TABLE_CELL];
    sap_port_list_t port_list[EXTPHY_APIS_NUMBER];
    sap_port_info_t *port_info;
    sap_port_status_t port_status;
    char *header[2][SAP_MAX_TABLE_COL] = {
        {"Index", "Port", "Phy Lane0", "Chip Type", "Host-Link",  "Line-Link",  "Speed", "fec"},
        {"",      "",     "",          "",          "Admin,Oper", "Admin,Oper", "",      "sys,line"},
    };
    memset(&port_list, 0, sizeof(sap_port_list_t));
    rv = sap_port_list_get(port_list);
    if (rv != SAP_STATUS_SUCCESS) {
        PLP_CLI("sap_port_list_get failed, rv=%d", rv);
        return SAP_STATUS_FAILURE;
    }
    for (i=0; i< EXTPHY_APIS_NUMBER; i++) {
        total_port_num += port_list[i].port_count;
    }
    char *table[total_port_num][SAP_MAX_TABLE_COL];
    for (i=0; i< EXTPHY_APIS_NUMBER; i++) {
        if (port_list[i].list == NULL) {
            continue;
        }
        SAP_PLIST_ITER(&port_list[i], port_info) {
            snprintf(buffer, SAP_MAX_TABLE_CELL, "%d", port_id);
            table[port_id][0] = strdup(buffer);
            snprintf(buffer, SAP_MAX_TABLE_CELL, "%d", port_info->port);
            table[port_id][1] = strdup(buffer);
            snprintf(buffer, SAP_MAX_TABLE_CELL, "%d", port_info->phy_lane0);
            table[port_id][2] = strdup(buffer);
            snprintf(buffer, SAP_MAX_TABLE_CELL, "%s", port_list[i].type);
            table[port_id][3] = strdup(buffer);
            rv = sap_port_status_get(port_info->unit, port_info->port, &port_status);
            if (rv != SAP_STATUS_SUCCESS) {
                PLP_CLI_DEBUG("sap_port_status_get failed, rv=%d", rv);
                table[port_id][4] = strdup("NA,NA");
                table[port_id][5] = strdup("NA,NA");
                table[port_id][6] = strdup("NA");
                table[port_id][7] = strdup("NA,NA");
            } else {
                snprintf(buffer, SAP_MAX_TABLE_CELL, "%s,%s", LINK_STRING(port_status.host_admin), LINK_STRING(port_status.host_link_up));
                table[port_id][4] = strdup(buffer);
                snprintf(buffer, SAP_MAX_TABLE_CELL, "%s,%s", LINK_STRING(port_status.line_admin), LINK_STRING(port_status.line_link_up));
                table[port_id][5] = strdup(buffer);
                snprintf(buffer, SAP_MAX_TABLE_CELL, "%d", port_status.speed);
                table[port_id][6] = strdup(buffer);
                rv = sap_port_fec_get(unit, port_info->port, PLP_SYS_IF_SIDE, &sap_fec);
                if (rv == SAP_STATUS_SUCCESS) {
                    sys_fec_str = fec_str[sap_fec];
                }
                rv = sap_port_fec_get(unit, port_info->port, PLP_LINE_IF_SIDE, &sap_fec);
                if (rv == SAP_STATUS_SUCCESS) {
                    line_fec_str = fec_str[sap_fec];
                }
                snprintf(buffer, SAP_MAX_TABLE_CELL, "%s,%s", sys_fec_str, line_fec_str);
                table[port_id][7] = strdup(buffer);
            }
            port_id++;
        }
    }
    print_table_header(table, total_port_num, header, 2, 8);
    int start = 0;
    for (i=0; i< EXTPHY_APIS_NUMBER; i++) {
        if (port_list[i].list == NULL) {
            continue;
        }
        start += port_list[i].port_count;
        print_table_without_header(table, start - port_list[i].port_count, port_list[i].port_count, total_port_num, header, 2, 8);
    }
    table_memfree(table, total_port_num, header, 2, 7);
    return SAP_STATUS_SUCCESS;
}

#ifdef SAPHY_BINARY
#include "sap_mdio.h"
#include "credo/shell.h"

/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
static char *command_generator(const char * text, int state)
{
    static int list_index, len;
    char *name;
    /* If this is a new word to complete, initialize now.  This includes
    saving the length of TEXT for efficiency, and initializing the index
    variable to 0. */
    if (!state) {
        list_index = 0;
        len = strlen (text);
    }
    /* Return the next name which partially matches from the command list. */
    name = commands[list_index].name; 
    while (name != NULL) {
        list_index++;
        if (strncmp (name, text, len) == 0) {
            return (strdup(name));
        }
        name = commands[list_index].name; 
    }
    /* If no names matched, then return NULL. */
    return ((char *)NULL);
}

char **fileman_completion(const char *text, int start, int end)
{
    char **matches;
    matches = (char **)NULL;
    /* If this word is at the start of the line, then it is a command
    to complete.  Otherwise it is the name of a file in the current
    directory. */
    if (start == 0) {
        matches = rl_completion_matches(text, command_generator);
    }
    return (matches);
}

/* Look up NAME as the name of a command, and return a pointer to that
    command.  Return a NULL pointer if NAME isn't a command name. */
static sap_debug_command_t *find_command(char *name)
{
    register int i;
    for (i = 0; commands[i].name; i++) {
        if (strcmp (name, commands[i].name) == 0) {
            return (&commands[i]);
        }
        if (strcmp (commands[i].name, name) == 0) {
            return (&commands[i]);
        }
    }
    return ((sap_debug_command_t *)NULL);
}
   
/* Strip whitespace from the start and end of STRING.  Return a pointer
    into STRING. */
static char *stripwhite(char *string)
{
    register char *s, *t;

    for (s = string; whitespace (*s); s++);
    if (*s == 0) {
        return (s);
    }
    t = s + strlen (s) - 1;
    while (t > s && whitespace (*t)) {
        t--;
    }
    *++t = '\0';
    return s;
}

/* Function which tells you that you can't do this. */
static void too_dangerous(char *caller)
{
    fprintf (stderr,"%s: Too dangerous for me to distribute.  Write it yourself.\n", caller);
}
   
/* Return non-zero if ARG is a valid argument for CALLER, else print
    an error message and return zero. */
static int valid_argument(char *caller, char *arg)
{
    if (!arg || !*arg)
    {
        fprintf (stderr, "%s: Argument required.\n", caller);
        return (0);
    }
    return (1);
}

static void remove_brackets(const char* src, char* dst) {
    while (*src) {
        if (*src != '[' && *src != ']') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nCaught signal %d, exiting...\n", sig);
        exit(0);
    }
}

int main (int argc, char **argv)
{
    char *cmd;
    int i, rv, service_flag;
    sap_common_init();
    sap_mdio_init();
    rv = sap_platform_init(0, false);
    if (rv == SAP_STATUS_NOT_SUPPORTED) {
        return 0;
    }
    sapcmd_service_init();

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    service_flag = 0;
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "service") == 0) {
            service_flag = 1;
        }
    }
    if (service_flag == 1) {
        do {
            sleep(1);
        } while (1);
    } else {
        rl_attempted_completion_function = fileman_completion;
        do {
            char *buffer;
            buffer = readline("drivshell>");
            if (strlen(buffer) == 0) {
                continue;
            }
            cmd = stripwhite(buffer);
            add_history(cmd);
            PLP_CLI("%s", cmd);
            if (!strcmp(cmd, "exit")) {
                break;
            }
            if (!strcmp(cmd, "quit")) {
                break;
            }
            sap_debug_line_exec(cmd, strlen(cmd)+1);
            free(buffer);
        } while (1);
    }
    return 0;
}
#endif
