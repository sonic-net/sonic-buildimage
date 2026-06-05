/********************************************************************************
 * Copyright(C) 2020 Micas Network. All rights reserved.
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期        | 作者     |  修改记录
**===============================================================================
**| 2025/4/10  | zhoutenghui  |  创建该文件
**
*********************************************************************************/
#include "sap_common.h"
#include "sap_mdio.h"
#include "sap_debug.h"
#include "crs.h"
#include "credo.h"
#include "cJSON.h"
#include "list.h"

#define SAP_CR_LOG_ERR      SAP_LOG_ERR
#define SAP_CR_LOG_WARN     SAP_LOG_WARN
#define SAP_CR_LOG_INFO     SAP_LOG_INFO
#define SAP_CR_LOG_NOTICE   SAP_LOG_NOTICE
#define SAP_CR_LOG_DBG      SAP_LOG_DBG

#define SAP_CFG_ROOT_PATH     "/usr/share/sonic/hwsku"
#define SAP_CREDO_CFG_FILE    "sap_credo_88322"

#define USE_POLARITY_REVERSE   (0)

#define MAX_MDIO_NUM              (8)
#define MAX_MDIO_DEV_NUM          (16)
#define MAX_DEV_SLICE_NUM         (2)
#define MAX_TOTAL_SLICE_NUM       (MAX_MDIO_NUM * MAX_MDIO_DEV_NUM * MAX_DEV_SLICE_NUM)

#define MAX_PROFILE_STR_LEN       (32)
#define MAX_MAC_LANE_NUM          (8)

#define MAX_BREAKOUT_MODE_STR_LEN (32)

#define MAX_SLICE_PORT_NUM        (8)
#define MAX_SLICE_LANE_NUM        (16)
#define MAX_HOST_LANE_NUM   (8)
#define MAX_LINE_LANE_NUM   (8)

#define CR_FORCE_FW_LOAD            (1)
#define CR_NOT_FORCE_FW_LOAD        (0)
#define CR_FW_WRITE_DELAY_US        (2)
#define MAX_FW_RETRY_TIMES          (10)
#define MAX_FW_RETRY_DELAY_SECS     (5)

#define CR_NOT_FORCE_PORT_CONFIG        (0)
#define CR_FORCE_PORT_CONFIG            (1)

#define CR_DISABLE          (0)
#define CR_ENABLE           (1)

#define CR_LINK_UP       (1)
#define CR_LINK_DOWN     (0)

#define CR_RX_PRBS_LINK_WAIT   (3)

#define GET_NUMBER_VALUE(cJSON_Object, field) cJSON_GetNumberValue(cJSON_GetObjectItem(cJSON_Object, field))

#define SAP_CR_RV_CHECK(slice_info, string, rv) \
    do { \
        if (rv != CR_SUCCESS) { \
            SAP_CR_LOG_ERR("dev_id:%d slice_id:%d, %s failed, rv=%d", \
                slice_info->dev_info->dev_id, slice_info->slice_id, string, rv); \
            return rv; \
        } \
    } while(0)
    
#define SAP_RV_CHECK(slice_info, string, rv) \
    do { \
        if (rv != SAP_STATUS_SUCCESS) { \
            SAP_CR_LOG_ERR("dev_id:%d slice_id:%d, %s failed, rv=%d", \
                slice_info->dev_info->dev_id, slice_info->slice_id, string, rv); \
            return rv; \
        } \
    } while(0)

#define SAP_POINT_CHECK_RV(ptr, ptr_name, rv) \
    do { \
        if (ptr == NULL) { \
            SAP_CR_LOG_ERR("%s is null, rv:%d", ptr_name, rv); \
            return rv; \
        } \
    } while(0)

typedef struct sap_pol_info_s {
    uint32_t tx_pol;
    uint32_t rx_pol;
} sap_pol_info_t;

typedef struct sap_chassis_info_s {
    uint8_t slot_num;
} sap_chassis_info_t;

typedef struct sap_dev_info_s sap_dev_info_t;
typedef struct sap_slice_info_s sap_slice_info_t;
typedef struct sap_credo_port_info_s sap_credo_port_info_t;

typedef struct
{
    char *fw_path;
    char *fw_ver;
    char *fw_crc;
} sap_firmware_info_t;

typedef struct
{
    char type[MAX_MDIO_TYPE_STR_LEN];
    uint8_t mdio_id;
    mdio_read_func_t mdio_read;
    mdio_write_func_t mdio_write;
    uint32_t phy_reg_offset;
} sap_credo_mdio_info_t;

typedef struct sap_dev_info_s {
    CredoDevice_t *cr_device;
    uint8_t unit;
    uint8_t slot_id;
    uint8_t dev_id;
    sap_slice_info_t *slice_info[MAX_DEV_SLICE_NUM];
    uint8_t slice_num;
} sap_dev_info_t;

typedef struct sap_slice_info_s {
    sap_dev_info_t *dev_info;
    CredoSlice_t *cr_slice;
    SliceContext_t slice_ctx;
    uint8_t slice_id;
    uint8_t mdio_id;
    sap_credo_mdio_info_t *mdio_info;
    uint32_t phy_addr;
    bool inited;
    bool loaded;
    uint8_t host_tx_lane_map[MAX_HOST_LANE_NUM];
    uint8_t host_rx_lane_map[MAX_HOST_LANE_NUM];
    uint8_t line_tx_lane_map[MAX_LINE_LANE_NUM];
    uint8_t line_rx_lane_map[MAX_LINE_LANE_NUM];
    char profile[MAX_PROFILE_STR_LEN];
    uint16_t global_lanes[MAX_MAC_LANE_NUM];
    uint8_t global_lane_num;
    sap_credo_port_info_t *port_info[MAX_SLICE_PORT_NUM];
    uint8_t port_num;
} sap_slice_info_t;

typedef struct sap_credo_port_info_s {
    sap_slice_info_t *slice_info;
    bool Uni_Directional;
    uint8_t port_id;
    uint8_t unit;
    uint16_t port;
    uint16_t phy_lane0;
    uint8_t num_lanes;
    uint8_t enable;
    uint8_t fec;
    uint8_t link_trainning;
    int speed;
} sap_credo_port_info_t;
typedef struct sap_credo_port_option_s {
    CredoOption_t cr_option;
    int option_value;
} sap_credo_port_option_t;

static cJSON *j_config = NULL;
static struct list_head global_port_list_head;
static uint16_t global_port_num = 0;

static sap_firmware_info_t global_firmware_info = {0};
static sap_chassis_info_t global_chassis_info = {0};

static sap_dev_info_t *global_dev_info = NULL;
static uint8_t global_dev_num = 0;

static sap_slice_info_t *global_slice_info = NULL;
static uint8_t global_slice_num = 0;

static sap_credo_mdio_info_t *global_mdio_info = NULL;
static uint8_t global_mdio_num = 0;

static int global_inited = 0;
static pthread_mutex_t global_mutex;
static pthread_mutexattr_t global_mutex_attr;
static char config_filepath[SAP_FILEPATH_BUFF_SIZE];

static int sap_credo_platform_init(int unit, bool warm_boot);
static int sap_credo_config_init(int unit);
static int sap_credo_config_parse_json(int unit);
static int sap_credo_config_parse_cfg(int unit);
static int sap_credo_slot_init(int unit, int slot_id);
static void *sap_credo_slice_init_thread(void *arg);
static int sap_credo_slice_init(int bus_id);
static int sap_credo_mdio_read(void* slice_context, unsigned reg_addr, unsigned* val);
static int sap_credo_mdio_write(void* slice_context, unsigned reg_addr, unsigned val);
static int sap_credo_mdio_broadcast_write(void* slice_context, unsigned reg_addr, unsigned val);
static void sap_credo_log_func(void* slice_context, void* user_data, CredoLogLevel_t level, const char* scope, const char* message);
static int sap_credo_set_port_admin(int unit, int port, bool enable);
// static int sap_credo_get_profile_info(int profile_id, int speed, int split_num, int port_id, CredoPortConfig_t *conf);
static int sap_credo_get_port_info(int unit, int port, sap_credo_port_info_t **port_info);
static int sap_credo_get_slice_info(int unit, int port, sap_slice_info_t **slice_info);
static int sap_credo_get_slice_info_by_phy_lane0(int unit, int phy_lane0, sap_slice_info_t **slice_info);
static int sap_credo_pre_port_init(sap_slice_info_t *slice_info);
static int sap_credo_port_init(sap_slice_info_t *slice_info);
static int sap_credo_post_port_init(sap_slice_info_t *slice_info);
static int sap_credo_check_fw(sap_slice_info_t *slice_info);
static void sap_credo_global_lock(void);
static void sap_credo_global_unlock(void);
static void sap_credo_port_lock(int unit, int port);
static void sap_credo_port_unlock(int unit, int port);

static int sap_credo_get_profile_info(sap_slice_info_t *slice_info, int port_id, CredoPortSetup_t *conf);
static int sap_credo_get_profile_info_uniretimer(sap_slice_info_t *slice_info, int port_id, CredoPortSetup_t *host_setup, CredoPortSetup_t *line_setup);
static int sap_credo_query_port_setup(sap_credo_port_info_t *port_info, int if_side, int tx_rx, CredoPortSetup_t *port_setup, bool *started);
static int sap_credo_prbs_operation_set(int tx_rx, sap_slice_info_t *slice_info, int if_side, 
    const CredoPortSetup_t *port_setup, int lane, CredoPrbsPattern_t cr_poly, int admin);
static int sap_credo_prbs_operation_get(sap_slice_info_t *slice_info, int if_side, 
    const CredoPortSetup_t *port_setup, int lane, sap_prbs_status_t *prbs_info);
static int sap_credo_prbs_change(int poly, CredoPrbsPattern_t *cr_poly);
static int sap_credo_port_prbs_set_ex(int unit, int port, int poly, int inv, int if_side, int lane, int admin);
static int sap_credo_port_option_save(sap_slice_info_t *slice_info, int port_id, sap_credo_port_option_t *port_option, int option_cnt);
static int sap_credo_port_option_restore(sap_slice_info_t *slice_info, int port_id, sap_credo_port_option_t *port_option, int option_cnt);

static int sap_credo_loopback_set(int unit, int port, int if_side, int lb_dir, int enable);
static int sap_credo_loopback_get(int unit, int port, int if_side, int lb_dir, int *enable);
static bool sap_credo_port_support(int unit, int port, int physical_port);
static int sap_credo_polarity_set(int unit, int port, int if_side, int tx_rx, uint32_t polarity, bool override);
static int sap_credo_polarity_get(int unit, int port, int if_side, int tx_rx, uint32_t *polarity);
static int sap_credo_port_list_get(sap_port_list_t *port_list);
static int sap_credo_port_prbs_set(int unit, int port, int poly, int inv, int if_side, int lane);
static int sap_credo_port_prbs_get(int unit, int port, int if_side, sap_prbs_status_t *prbs_info);
static int sap_credo_port_prbs_clear(int unit, int port, int if_side);
static int sap_credo_port_prbs_ber_get(int unit, int port, int if_side, int time_v, sap_prbs_status_t *prbs_info);
static int sap_credo_linktraining_set(int unit, int port, int if_side, int enable);
static int sap_credo_port_status_get(int unit, int port, sap_port_status_t *port_status);
static int sap_credo_tx_taps_set(int unit, int port, int if_side, int lane, sap_tx_fir_t tx_fir);
static int sap_credo_tx_taps_get(int unit, int port, int if_side, int lane, sap_tx_fir_t *tx_fir);
static void sap_credo_techsupport(int unit, int port);
static int sap_credo_dsc_dump(int unit, int port, int if_side, int flag, int lane);

sap_apis_t sap_extphy_credo_88322_apis = {
    .type                           = "CREDO_88322",
    .sap_platform_init              = sap_credo_platform_init,
    .sap_port_support               = sap_credo_port_support,
    .sap_loopback_set               = sap_credo_loopback_set,
    .sap_loopback_get               = sap_credo_loopback_get,
    .sap_polarity_set               = sap_credo_polarity_set,
    .sap_polarity_get               = sap_credo_polarity_get,
    .sap_port_list_get              = sap_credo_port_list_get,
    .sap_port_prbs_set              = sap_credo_port_prbs_set,
    .sap_port_prbs_get              = sap_credo_port_prbs_get,
    .sap_port_prbs_clear            = sap_credo_port_prbs_clear,
    .sap_port_prbs_ber_get          = sap_credo_port_prbs_ber_get,
    .sap_port_linktrain_set         = sap_credo_linktraining_set,
    .sap_port_status_get            = sap_credo_port_status_get,
    .sap_tx_fir_set                 = sap_credo_tx_taps_set,
    .sap_tx_fir_get                 = sap_credo_tx_taps_get,
    .sap_techsupport                = sap_credo_techsupport,
    .sap_dsc_dump                   = sap_credo_dsc_dump
    /* credo在retimer、bitmux模式下不支持设置fec，直接透传fec */
};

static void* nonblocking_shell_server(void *vargp) {
    
    cr_shell_spawn_server2(NULL);
    pthread_exit(NULL);
}

static int sap_credo_platform_init(int unit, bool warm_boot)
{
    int i, rv;
    int slot_id;
    pthread_t thread_id;
    CredoSlice_t* slices[32] = {0};

    if (global_inited == 0) {
        INIT_LIST_HEAD(&global_port_list_head);
        /* 初始化全局锁属性为递归锁 */
        pthread_mutexattr_init(&global_mutex_attr);
        pthread_mutexattr_settype(&global_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
        /* 初始化全局锁 */
        pthread_mutex_init(&global_mutex, &global_mutex_attr);
        global_inited = 1;
        rv = sap_credo_config_init(unit);
        if (rv != SAP_STATUS_SUCCESS) {
            return rv;
        }
    }
    sap_credo_global_lock();
    for (slot_id = 0; slot_id < global_chassis_info.slot_num; slot_id++) {
        SAP_CR_LOG_INFO("sap_credo_slot_init start, slot_id:%d", slot_id);
        if (sap_credo_slot_init(unit, slot_id) != SAP_STATUS_SUCCESS) {
            SAP_CR_LOG_ERR("sap_credo_slot_init failed, slot_id:%d", slot_id);
            sap_credo_global_unlock();
            return SAP_STATUS_FAILURE;
        }
        SAP_CR_LOG_INFO("sap_credo_slot_init end, slot_id:%d", slot_id);
    }
    sap_credo_global_unlock();

    SAP_CR_LOG_INFO("sap_credo_shell_server_init start");
    for (i=0; i<global_slice_num; i++) {
        slices[i] = global_slice_info[i].cr_slice;
    }
    cr_shell_set_slices(slices, global_slice_num);
    pthread_create(&thread_id, NULL, nonblocking_shell_server, NULL);
    pthread_detach(thread_id);
    SAP_CR_LOG_INFO("sap_credo_shell_server_init end");
    return SAP_STATUS_SUCCESS;
}

static void sap_credo_global_lock(void)
{
    if (global_inited) {
        pthread_mutex_lock(&global_mutex);
    }
}

static void sap_credo_global_unlock(void)
{
    if (global_inited) {
        pthread_mutex_unlock(&global_mutex);
    }
}

static void sap_credo_port_lock(int unit, int port)
{
    return sap_credo_global_lock();
}

static void sap_credo_port_unlock(int unit, int port)
{
    return sap_credo_global_unlock();
}

static bool sap_credo_port_support(int unit, int port, int physical_port)
{
    int rv;
    sap_credo_port_info_t *port_info;
    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        return false;
    }
    return true;
}

static int sap_credo_config_parse_json(int unit)
{
    if (j_config != NULL) {
        SAP_CR_LOG_WARN("json config already parse");
        return SAP_STATUS_SUCCESS;
    }
    /* 打开文件 */
    FILE *file = fopen(config_filepath, "r");
    if (file == NULL) {
        SAP_CR_LOG_ERR("Unable to open file");
        return SAP_STATUS_FAILURE;
    }
    /* 读取文件内容到缓冲区 */
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = (char *)malloc(length + 1);
    if (buffer == NULL) {
        SAP_CR_LOG_ERR("Unable to allocate memory");
        fclose(file);
        return SAP_STATUS_FAILURE;
    }
    fread(buffer, 1, length, file);
    buffer[length] = '\0'; /* 确保字符串以 NULL 结尾 */
    fclose(file);
    /* 使用cJSON解析 */
    // cJSON *j_config = cJSON_Parse(buffer);
    j_config = cJSON_Parse(buffer);
    free(buffer); /* 释放内存 */
    if (j_config == NULL) {
        SAP_CR_LOG_ERR("Error parsing JSON.\n");
        return SAP_STATUS_FAILURE;
    }
    int rv;
    int i,j,k;
    int dev_id, slice_id;
    sap_dev_info_t *dev_info;
    sap_slice_info_t *slice_info;

    /* 全局配置解析 */
    cJSON *chassis_info = cJSON_GetObjectItem(j_config, "chassis_info");
    cJSON *subcard_num = cJSON_GetObjectItem(chassis_info, "subcard_num");
    global_chassis_info.slot_num = cJSON_GetNumberValue(subcard_num);
    cJSON *sap_version = cJSON_GetObjectItem(j_config, "sap_version");
    cJSON *sdk_version = cJSON_GetObjectItem(j_config, "sdk_version");
    SAP_CR_LOG_INFO("sap_version: %s", cJSON_GetStringValue(sap_version));
    SAP_CR_LOG_INFO("sdk_version: %s", cJSON_GetStringValue(sdk_version));
    cJSON *firmware_info = cJSON_GetObjectItem(j_config, "firmware_info");
    cJSON *fw_path = cJSON_GetObjectItem(firmware_info, "fw_path");
    cJSON *fw_ver = cJSON_GetObjectItem(firmware_info, "fw_ver");
    cJSON *fw_crc = cJSON_GetObjectItem(firmware_info, "fw_crc");
    global_firmware_info.fw_path = cJSON_GetStringValue(fw_path);
    global_firmware_info.fw_ver = cJSON_GetStringValue(fw_ver);
    global_firmware_info.fw_crc = cJSON_GetStringValue(fw_crc);
    SAP_CR_LOG_INFO("fw_path: %s", cJSON_GetStringValue(fw_path));
    SAP_CR_LOG_INFO("fw_ver: %s", cJSON_GetStringValue(fw_ver));
    SAP_CR_LOG_INFO("fw_crc: %s", cJSON_GetStringValue(fw_crc));
    /* mdio_list配置解析 */
    cJSON *mdio_list = cJSON_GetObjectItem(j_config, "mdio_list");
    global_mdio_num = cJSON_GetArraySize(mdio_list);
    global_mdio_info = (sap_credo_mdio_info_t *)malloc(sizeof(sap_credo_mdio_info_t) * global_mdio_num);
    memset(global_mdio_info, 0, sizeof(sap_credo_mdio_info_t) * global_mdio_num);
    for (i = 0; i < global_mdio_num; i++) {
        cJSON *mdio_item = cJSON_GetArrayItem(mdio_list, i);
        cJSON *mdio_id = cJSON_GetObjectItem(mdio_item, "index");
        cJSON *mdio_tpye = cJSON_GetObjectItem(mdio_item, "type");
        cJSON *mdio_phy_reg_offset = cJSON_GetObjectItem(mdio_item, "phy_reg_offset");
        sap_credo_mdio_info_t *mdio_info = &global_mdio_info[i];
        mdio_info->mdio_id = cJSON_GetNumberValue(mdio_id);
        snprintf(mdio_info->type, sizeof(mdio_info->type), "%s", cJSON_GetStringValue(mdio_tpye));
        sap_str_to_uint32(cJSON_GetStringValue(mdio_phy_reg_offset), &mdio_info->phy_reg_offset);
        rv = sap_mdio_read_func_get(mdio_info->type, &mdio_info->mdio_read);
        if (rv) {
            SAP_CR_LOG_ERR("sap_mdio_read_func_get failed, mdio type: %s", mdio_info->type);
        }
        rv = sap_mdio_write_func_get(mdio_info->type, &mdio_info->mdio_write);
        if (rv) {
            SAP_CR_LOG_ERR("sap_mdio_write_func_get failed, mdio type: %s", mdio_info->type);
        }
        // mdio_info->phy_reg_offset = cJSON_GetStringValue(mdio_phy_reg_offset);
    }
    cJSON *chip_list = cJSON_GetObjectItem(j_config, "chip_list");
    cJSON *slice_list = cJSON_GetObjectItem(j_config, "slice_list");
    global_dev_num = cJSON_GetArraySize(chip_list);
    global_dev_info = (sap_dev_info_t *)malloc(sizeof(sap_dev_info_t) * global_dev_num);
    memset(global_dev_info, 0, sizeof(sap_dev_info_t) * global_dev_num);
    global_slice_num = cJSON_GetArraySize(slice_list);
    global_slice_info = (sap_slice_info_t *)malloc(sizeof(sap_slice_info_t) * global_slice_num);
    memset(global_slice_info, 0, sizeof(sap_slice_info_t) * global_slice_num);
    /* chip_list配置解析 */
    for (dev_id = 0; dev_id < global_dev_num; dev_id++) {
        cJSON *chip_info = cJSON_GetArrayItem(chip_list, dev_id);
        cJSON *unit = cJSON_GetObjectItem(chip_info, "unit");
        cJSON *slot = cJSON_GetObjectItem(chip_info, "slot");
        // cJSON *mdio_index = cJSON_GetObjectItem(chip_info, "mdio_index");
        cJSON *slices = cJSON_GetObjectItem(chip_info, "slices");
        dev_info = &global_dev_info[dev_id];
        dev_info->cr_device= NULL;
        dev_info->dev_id= dev_id;
        dev_info->unit = cJSON_GetNumberValue(unit);
        dev_info->slot_id = cJSON_GetNumberValue(slot);
        // dev_info->mdio_id = cJSON_GetNumberValue(mdio_index);
        dev_info->slice_num = cJSON_GetArraySize(slices);
        for (slice_id=0; slice_id<dev_info->slice_num; slice_id++) {
            cJSON *slice_item = cJSON_GetArrayItem(slices, slice_id);
            int slice_index = cJSON_GetNumberValue(slice_item);
            if (slice_index >= global_slice_num) {
                SAP_CR_LOG_ERR("slice index %d is out of range", slice_index);
                continue;
            }
            slice_info = &global_slice_info[slice_index];
            dev_info->slice_info[slice_id] = slice_info;
            slice_info->dev_info = dev_info;
            slice_info->slice_id = slice_id;
            slice_info->slice_ctx.m_nChipId = dev_id;
            slice_info->slice_ctx.m_nSliceId = slice_id;
        }
    }
    /* slice_list配置解析 */
    for (i = 0; i < global_slice_num; i++) {
        slice_info = &global_slice_info[i];
        slice_info->inited = 0;
        slice_info->loaded = 0;
        cJSON *slice_info_json = cJSON_GetArrayItem(slice_list, i);
        cJSON *mdio_id = cJSON_GetObjectItem(slice_info_json, "mdio_index");
        slice_info->mdio_id = cJSON_GetNumberValue(mdio_id);
        cJSON *phy_addr = cJSON_GetObjectItem(slice_info_json, "phy_addr");
        slice_info->phy_addr = cJSON_GetNumberValue(phy_addr);
        for(j = 0; j < global_mdio_num; j++) {
            if (global_mdio_info[j].mdio_id == slice_info->mdio_id) {
                slice_info->mdio_info = &global_mdio_info[j];
                break;
            }
        }
        cJSON *slice_profile = cJSON_GetObjectItem(slice_info_json, "slice_profile");
        snprintf(slice_info->profile, MAX_PROFILE_STR_LEN, "%s", cJSON_GetStringValue(slice_profile));
        cJSON *global_lanes = cJSON_GetObjectItem(slice_info_json, "global_lanes");
        slice_info->global_lane_num = cJSON_GetArraySize(global_lanes);
        for (j = 0; j < slice_info->global_lane_num; j++) {
            cJSON *global_lane_item = cJSON_GetArrayItem(global_lanes, j);
            slice_info->global_lanes[j] = cJSON_GetNumberValue(global_lane_item);
        }
        cJSON *host_tx_lane_map = cJSON_GetObjectItem(slice_info_json, "host_tx_lane_map");
        if (host_tx_lane_map && (cJSON_GetArraySize(host_tx_lane_map) == MAX_HOST_LANE_NUM)) {
            for (j = 0; j < MAX_HOST_LANE_NUM; j++) {
                cJSON *lane_map_item = cJSON_GetArrayItem(host_tx_lane_map, j);
                slice_info->host_tx_lane_map[j] = cJSON_GetNumberValue(lane_map_item);
                // printf("%d: %d\n", j, slice_info->host_tx_lane_map[j]);
            }
        } else {
            SAP_CR_LOG_INFO("global slice_id:%d, host_tx_lane_map use default setting", i);
            for (j = 0; j < MAX_HOST_LANE_NUM; j++) {
                slice_info->host_tx_lane_map[j] = j;
            }
        }
        cJSON *host_rx_lane_map = cJSON_GetObjectItem(slice_info_json, "host_rx_lane_map");
        if (host_rx_lane_map && (cJSON_GetArraySize(host_rx_lane_map) == MAX_HOST_LANE_NUM)) {
            for (j = 0; j < MAX_HOST_LANE_NUM; j++) {
                cJSON *lane_map_item = cJSON_GetArrayItem(host_rx_lane_map, j);
                slice_info->host_rx_lane_map[j] = cJSON_GetNumberValue(lane_map_item);
            }
        } else {
            SAP_CR_LOG_INFO("global slice_id:%d, host_rx_lane_map use default setting", i);
            for (j = 0; j < MAX_HOST_LANE_NUM; j++) {
                slice_info->host_rx_lane_map[j] = j;
            }
        }
        cJSON *line_tx_lane_map = cJSON_GetObjectItem(slice_info_json, "line_tx_lane_map");
        if (line_tx_lane_map && (cJSON_GetArraySize(line_tx_lane_map) == MAX_LINE_LANE_NUM)) {
            for (j = 0; j < MAX_LINE_LANE_NUM; j++) {
                cJSON *lane_map_item = cJSON_GetArrayItem(line_tx_lane_map, j);
                slice_info->line_tx_lane_map[j] = cJSON_GetNumberValue(lane_map_item);
                // printf("%d: %d\n", j, slice_info->line_tx_lane_map[j]);
            }
        } else {
            SAP_CR_LOG_INFO("global slice_id:%d, line_tx_lane_map use default setting", i);
            for (j = 0; j < MAX_LINE_LANE_NUM; j++) {
                slice_info->line_tx_lane_map[j] = j;
            }
        }
        cJSON *line_rx_lane_map = cJSON_GetObjectItem(slice_info_json, "line_rx_lane_map");
        if (line_rx_lane_map && (cJSON_GetArraySize(line_rx_lane_map) == MAX_LINE_LANE_NUM)) {
            for (j = 0; j < MAX_LINE_LANE_NUM; j++) {
                cJSON *lane_map_item = cJSON_GetArrayItem(line_rx_lane_map, j);
                slice_info->line_rx_lane_map[j] = cJSON_GetNumberValue(lane_map_item);
            }
        } else {
            SAP_CR_LOG_INFO("global slice_id:%d, line_rx_lane_map use default setting", i);
            for (j = 0; j < MAX_LINE_LANE_NUM; j++) {
                slice_info->line_rx_lane_map[j] = j;
            }
        }
    }
    /* port_list配置解析 */
    cJSON *port_list = cJSON_GetObjectItem(j_config, "port_list");
    for (i = 0; i < cJSON_GetArraySize(port_list); i++) {
        cJSON *port_item = cJSON_GetArrayItem(port_list, i);
        // sap_port_info_t *port_info
        sap_credo_port_info_t *port_info = (sap_credo_port_info_t *)malloc(sizeof(sap_credo_port_info_t));
        if (port_info == NULL) {
            SAP_CR_LOG_ERR("malloc failed!");
            continue;
        }
        port_info->unit = GET_NUMBER_VALUE(port_item, "unit");
        port_info->port = GET_NUMBER_VALUE(port_item, "port");
        port_info->phy_lane0 = GET_NUMBER_VALUE(port_item, "phy_lane0");
        port_info->num_lanes = GET_NUMBER_VALUE(port_item, "num_lanes");
        port_info->speed = GET_NUMBER_VALUE(port_item, "speed");
        port_info->enable = GET_NUMBER_VALUE(port_item, "enable");
        port_info->fec = GET_NUMBER_VALUE(port_item, "fec");
        port_info->link_trainning = GET_NUMBER_VALUE(port_item, "link_trainning");
        // memset(&port_info->cr_port_config, 0, sizeof(CredoPortConfig_t));
        rv = sap_credo_get_slice_info_by_phy_lane0(port_info->unit, port_info->phy_lane0, &port_info->slice_info);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_CR_LOG_ERR("get slice info failed, unit:%d, phy_lane0:%d", port_info->unit, port_info->phy_lane0);
            free(port_info);
            continue;
        }
        for (j = 0; j<port_info->slice_info->global_lane_num; j++) {
            if (port_info->slice_info->global_lanes[j] == port_info->phy_lane0) {
                port_info->port_id = j;
                break;
            }
        }
        port_info->slice_info->port_info[port_info->port_id] = port_info;
        port_info->slice_info->port_num++;

        sap_port_info_t *sap_port_info = (sap_port_info_t *)malloc(sizeof(sap_port_info_t));
        if (sap_port_info == NULL) {
            SAP_CR_LOG_ERR("malloc failed!");
            free(port_info);
            continue;
        }
        sap_port_info->unit = port_info->unit;
        sap_port_info->port = port_info->port;
        sap_port_info->phy_lane0 = port_info->phy_lane0;
        sap_port_info->port_info = port_info;

        list_add_tail(&sap_port_info->node, &global_port_list_head);
        global_port_num++;
    }

    return SAP_STATUS_SUCCESS;
}

static int sap_credo_config_parse_cfg(int unit)
{
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_config_init(int unit)
{
    int rv;
    memset(&global_firmware_info, 0, sizeof(sap_firmware_info_t));
    memset(&global_chassis_info, 0, sizeof(sap_chassis_info_t));

    snprintf(config_filepath, SAP_FILEPATH_BUFF_SIZE, "%s/%s.json", SAP_CFG_ROOT_PATH, SAP_CREDO_CFG_FILE);
    if (fileExists(config_filepath) == 1) {
        rv = sap_credo_config_parse_json(unit);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_CR_LOG_ERR("sap_credo_config_parse_json failed");
            return SAP_STATUS_FAILURE;
        }
        return SAP_STATUS_SUCCESS;
    } else {
        SAP_CR_LOG_DBG("config file %s not exist", config_filepath);
    }
    snprintf(config_filepath, SAP_FILEPATH_BUFF_SIZE, "%s/%s.cfg", SAP_CFG_ROOT_PATH, SAP_CREDO_CFG_FILE);
    if (fileExists(config_filepath) == 1) {
        rv = sap_credo_config_parse_cfg(unit);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_CR_LOG_ERR("sap_credo_config_parse_cfg failed");
            return SAP_STATUS_FAILURE;
        }
        return SAP_STATUS_SUCCESS;
    } else {
        SAP_CR_LOG_DBG("config file %s not exist", config_filepath);
    }
    return SAP_STATUS_NOT_SUPPORTED;
}

static int sap_credo_slot_init(int unit, int slot_id)
{
    int i,j,rv;
    int mdio_list[MAX_MDIO_NUM];
    int mdio_num = 0;
    bool exist = false;
    int slice_id;
    int dev_id;
    int *retval;
    CredoSdk_t *sdk;
    CredoSdkConfig_t sdk_config;
    sap_dev_info_t *dev_info;

    memset(&sdk_config, 0, sizeof(CredoSdkConfig_t));
    sdk_config.log = sap_credo_log_func;
    sdk_config.max_log_level = CR_LOG_TRACE;
    sdk_config.read_register = sap_credo_mdio_read;
    sdk_config.write_register = sap_credo_mdio_write;

    /* sdk_config的配置无效, 这里需要调API设置日志接口 */
    cr_logger_set(sap_credo_log_func);
    cr_logger_set_level(CR_LOG_TRACE);
    cr_logger_set_feature(CR_LOG_FEAT_API, 1);

    rv = cr_sdk_create(&sdk_config, &sdk);
    if (rv) {
        SAP_CR_LOG_ERR("credo sdk create failed, slot_id:%d, rv:%d", slot_id, rv);
        return SAP_STATUS_FAILURE;
    }
    (void)cr_sdk_set_broadcast_write(sdk, sap_credo_mdio_broadcast_write); /* TODO: ? */
    for (dev_id = 0; dev_id < global_dev_num; dev_id++) {
        dev_info = &global_dev_info[dev_id];
        if (dev_info->slot_id != slot_id) {
            continue;
        }
        if (dev_info->cr_device == NULL) {
            rv = cr_device_create(sdk, CR_DEV_SCREAMING_EAGLE, &dev_info->cr_device);
            if (rv) {
                SAP_CR_LOG_ERR("credo device create failed, dev_id:%d, rv:%d", dev_id, rv);
                return SAP_STATUS_FAILURE;
            }
        }
        for (slice_id = 0; slice_id < dev_info->slice_num; slice_id++) {
            dev_info->slice_info[slice_id]->slice_id = slice_id;
            rv = cr_device_get_slice(dev_info->cr_device, slice_id, &dev_info->slice_info[slice_id]->cr_slice);
            if (rv) {
                SAP_CR_LOG_ERR("credo device get slice failed, dev_id:%d, slice_id:%d, rv:%d", dev_id, slice_id, rv);
                return SAP_STATUS_FAILURE;
            }
        }
    }

    for (i = 0; i < global_slice_num; i++) {
        if (global_slice_info[i].dev_info->slot_id != slot_id) {
            continue;
        }
        exist = false;
        for (j = 0; j < mdio_num; j++) {
            if (mdio_list[j] == global_slice_info[i].mdio_id) {
                exist = true;
                break;
            }
        }
        if (!exist) {
            mdio_list[mdio_num] = global_slice_info[i].mdio_id;
            mdio_num++;
        }
    }

    pthread_t threads[MAX_MDIO_NUM];
    for (i = 0; i < mdio_num; i++) {
        SAP_CR_LOG_INFO("credo mdio_id:%d init pthread create start", mdio_list[i]);
        rv = pthread_create(&threads[i], NULL, sap_credo_slice_init_thread, (void *)&mdio_list[i]);
        if (rv) {
            SAP_CR_LOG_ERR("credo pthread create failed, rv:%d", rv);
            return SAP_STATUS_FAILURE;
        }
    }
    for (i = 0; i < mdio_num; i++) {
        SAP_CR_LOG_INFO("credo mdio_id:%d init pthread join start", mdio_list[i]);
        if (!threads[i]) {
            SAP_CR_LOG_ERR("pthread_join join failed, thread_id null");
        }
        rv = pthread_join(threads[i], (void **)&retval);
        if (rv)  {
            SAP_CR_LOG_ERR("pthread_join join failed, rv:%d, mdio_id:%d", rv, mdio_list[i]);
            continue;
        }
        if (retval == NULL) {
            SAP_CR_LOG_ERR("credo mdio_id:%d init failed, rv null", mdio_list[i]);
            return SAP_STATUS_FAILURE;
        }
        if (*retval != SAP_STATUS_SUCCESS) {
            SAP_CR_LOG_ERR("credo mdio_id:%d init failed, rv: %d", mdio_list[i], *retval);
            free(retval);
            return SAP_STATUS_FAILURE;
        }
        SAP_CR_LOG_INFO("credo mdio_id:%d init success, rv: %d", mdio_list[i], *retval);
        free(retval);
    }
    return SAP_STATUS_SUCCESS;
}

static void *sap_credo_slice_init_thread(void *arg)
{
    int *rv, mdio_id;
    mdio_id = *(int *)arg;
    rv = (int *)malloc(sizeof(int));
    if (rv == NULL) {
        pthread_exit(NULL);
    }
    *rv = sap_credo_slice_init(mdio_id);
    pthread_exit((void *)rv);
}

static void sap_credo_log_func(void* slice_context, void* user_data, CredoLogLevel_t level, const char* scope, const char* message)
{
    switch(level) {
    case CR_LOG_ERROR:
        SAP_CR_LOG_ERR("%s, %s", scope, message);
        break;
    case CR_LOG_WARN:
        SAP_CR_LOG_WARN("%s, %s", scope, message);
        break;
    case CR_LOG_INFO:
        SAP_CR_LOG_INFO("%s, %s", scope, message);
        break;
    case CR_LOG_DEBUG:
        SAP_CR_LOG_DBG("%s, %s", scope, message);
        break;
    case CR_LOG_TRACE:
        SAP_CR_LOG_DBG("%s, %s", scope, message);
        break;
    }
}

static int sap_credo_mdio_read(void* slice_context, unsigned reg_addr, unsigned* val)
{
    int rv;
    unsigned int phy_addr;
    SliceContext_t *context;
    mdio_read_func credo_phy_read = NULL;
    sap_slice_info_t *slice_info;
    sap_credo_mdio_info_t *mdio_info;
    context = (SliceContext_t *)slice_context;

    slice_info = global_dev_info[context->m_nChipId].slice_info[context->m_nSliceId];
    // mdio_info = &global_mdio_info[slice_info->mdio_id];
    mdio_info = slice_info->mdio_info;
    /* no verification for read/write faster */
    phy_addr = slice_info->phy_addr;
    reg_addr = mdio_info->phy_reg_offset | reg_addr;
    // printf("phy: 0x%x, read reg_addr: 0x%x, data: 0x%x, rv: %d\n", phy_addr, reg_addr, *data, rv);
    rv = mdio_info->mdio_read(NULL, slice_info->mdio_id, phy_addr, reg_addr, val);
    if (rv) {
        SAP_CR_LOG_ERR("read phy_addr:0x%x reg:0x%x failed, rv:%d", phy_addr, reg_addr, rv);
        return SAP_STATUS_FAILURE;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_mdio_write(void* slice_context, unsigned reg_addr, unsigned val)
{
    int rv;
    unsigned int phy_addr;
    SliceContext_t *context;
    mdio_write_func credo_phy_write = NULL;
    sap_slice_info_t *slice_info;
    sap_credo_mdio_info_t *mdio_info;
    context = (SliceContext_t *)slice_context;
    slice_info = global_dev_info[context->m_nChipId].slice_info[context->m_nSliceId];
    // mdio_info = &global_mdio_info[slice_info->mdio_id];
    mdio_info = slice_info->mdio_info;
    /* no verification for read/write faster */
    phy_addr = slice_info->phy_addr;
    // printf("m_nChipId:%d, m_nSliceId:%d, phy_addr:0x%x\n", context->m_nChipId, context->m_nSliceId, phy_addr);
    reg_addr = mdio_info->phy_reg_offset | reg_addr;
    rv = mdio_info->mdio_write(NULL, slice_info->mdio_id, phy_addr, reg_addr, val);
    if (rv) {
        SAP_CR_LOG_ERR("write phy_addr:0x%x reg:0x%x val:0x%x failed, rv:%d", phy_addr, reg_addr, val, rv);
        return SAP_STATUS_FAILURE;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_mdio_broadcast_write(void* slice_context, unsigned reg_addr, unsigned val)
{
    int rv;
    int slice_id;
    sap_slice_info_t *slice_info;
    SliceContext_t *context = (SliceContext_t *)slice_context;
    sap_dev_info_t *dev_info;
    dev_info = &global_dev_info[context->m_nChipId];
    for(slice_id = 0; slice_id < dev_info->slice_num; slice_id++) {
        slice_info = dev_info->slice_info[slice_id];
        rv = sap_credo_mdio_write(&slice_info->slice_ctx, reg_addr, val);
        if (rv) {
            SAP_CR_LOG_ERR("write phy_addr:0x%x reg:0x%x val:0x%x failed, rv:%d", slice_info->phy_addr, reg_addr, val, rv);
            return SAP_STATUS_FAILURE;
        }
    }
    return SAP_STATUS_SUCCESS;
}

// static int sap_credo_create_port(int unit, int port, sap_plp_port_resource_t *port_resource)
// {
//     int err;
//     int split_num;
//     int port_id;
//     int speed, phy_lane0, lanes;
//     CredoPortConfig_t port_config;
//     sap_slice_info_t *slice_info;

//     speed = port_resource->speed;
//     phy_lane0 = port_resource->phy_lane0;
//     lanes = port_resource->host_lanes;

//     err = sap_cr_get_slice_info_by_lane(unit, phy_lane0, &slice_info);
//     if (err) {
//         SAP_CR_LOG_ERR("sap_cr_get_slice_info_by_lane fail");
//         return SAP_STATUS_FAILURE;
//     }

//     split_num = slice_info->mac_lane_num / lanes;

//     for (port_id=0; port_id<slice_info->mac_lane_num; port_id++) {
//         if (slice_info->lanes[port_id] == phy_lane0) {
//             break;
//         }
//     }

//     err = sap_cr_get_profile_info(slice_info->profile_id, speed, split_num, port_id, &port_config);
//     if (err) {
//         SAP_CR_LOG_ERR("sap_cr_get_profile_info fail");
//         return SAP_STATUS_FAILURE;
//     }

//     port_config.port_id = port_id;
//     err = cr_port_configure(slice_info->slice, &port_config, CR_FORCE_PORT_CONFIG);
//     if (err) {
//         SAP_CR_LOG_ERR("cr_port_configure fail, err:%d", err);
//         return SAP_STATUS_FAILURE;
//     }

//     slice_info->port[port_id] = port;
//     slice_info->port_speed[port_id] = speed;
//     slice_info->port_num_lanes[port_id] = lanes;
//     slice_info->port_num++;

//     return SAP_STATUS_SUCCESS;
// }

// static int sap_credo_remove_port(int unit, int port)
// {
//     int rv, port_id;
//     sap_port_hdl_info_t cr_port_hdl;
//     sap_slice_info_t *slice_info;

//     rv = sap_cr_get_port_conf_hdl(unit, port, &cr_port_hdl);
//     if (rv) {
//         SAP_CR_LOG_ERR("sap_credo_remove_port destroy port fail, rv:%d", rv);
//         return SAP_STATUS_FAILURE;
//     }

//     rv = cr_port_destroy(cr_port_hdl.slice, cr_port_hdl.port_id);
//     if (rv) {
//         SAP_CR_LOG_ERR("sap_credo_remove_port destroy port fail, rv:%d", rv);
//         return SAP_STATUS_FAILURE;
//     }

//     slice_info = cr_port_hdl.slice_info;

//     for (port_id = 0; port_id < CR_SLICE_MAX_PORT_NUM; port_id++) {
//         if (slice_info->port[port_id] == port) {
//             slice_info->port[port_id] = -1;
//             slice_info->port_speed[port_id] = -1;
//             slice_info->port_num_lanes[port_id] = -1;
//             slice_info->port_num--;
//         }
//     }
    
//     return SAP_STATUS_SUCCESS;
// }

static int sap_credo_poly_lane_swap(sap_slice_info_t *slice_info, uint8_t lane_index, sap_port_side_e side, sap_channel_dir_e tx_rx)
{
    int lane_id = 0;
    if (side == SAP_PORT_SIDE_SYS) {
        if (tx_rx == SAP_CHANNEL_TX) {
            lane_id = slice_info->host_tx_lane_map[lane_index];
        }
        if (tx_rx == SAP_CHANNEL_RX) {
            lane_id = slice_info->host_rx_lane_map[lane_index];
        }
    }
    if (side == SAP_PORT_SIDE_LINE) {
        if (tx_rx == SAP_CHANNEL_TX) {
            lane_id = slice_info->line_tx_lane_map[lane_index];
        }
        if (tx_rx == SAP_CHANNEL_RX) {
            lane_id = slice_info->line_rx_lane_map[lane_index];
        }
    }
    return lane_id;
}

static int sap_credo_pre_port_init(sap_slice_info_t *slice_info)
{
    int i, rv;
    int j, pol;
    int dev_id, slice_id, lane_id, swap_lane_id;
    dev_id = slice_info->slice_ctx.m_nChipId;
    slice_id = slice_info->slice_ctx.m_nSliceId;
    int tmp_polarity = 0;
    if (!slice_info->loaded) {
        SAP_CR_LOG_ERR("firmware not loaded, fail to configure port, dev_id:%d slice_id:%d", dev_id, slice_id);
        return SAP_STATUS_FAILURE;
    }
    /* disable all lanes before confige lane/port */
    for (lane_id = 0; lane_id < MAX_SLICE_LANE_NUM; lane_id++) {
        rv = cr_lane_rx_disable(slice_info->cr_slice, lane_id);
        SAP_CR_RV_CHECK(slice_info, "cr_lane_rx_disable", rv);
        rv = cr_lane_tx_disable(slice_info->cr_slice, lane_id);
        SAP_CR_RV_CHECK(slice_info, "cr_lane_tx_disable", rv);
    }
    for (lane_id = 0; lane_id < MAX_SLICE_LANE_NUM; lane_id++) {
        rv= cr_serdes_set_rx_coupling(slice_info->cr_slice, lane_id, CR_COUPLING_AC);
        SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_rx_coupling", rv);
        rv = cr_serdes_set_tx_gray_code(slice_info->cr_slice, lane_id, CR_ENABLE);
        SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_tx_gray_code", rv);
        rv = cr_serdes_set_rx_gray_code(slice_info->cr_slice, lane_id, CR_ENABLE);
        SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_rx_gray_code", rv);
    }
    cJSON *slice_list = cJSON_GetObjectItem(j_config, "slice_list");
    for (i = 0; i < global_slice_num; i++) {
        if (&global_slice_info[i] != slice_info) {
            continue;
        }
        cJSON *slice_list_item = cJSON_GetArrayItem(slice_list, i);
        cJSON *tx_polarity_flip_host = cJSON_GetObjectItem(slice_list_item, "host_tx_polarity_flip");
        cJSON *rx_polarity_flip_host = cJSON_GetObjectItem(slice_list_item, "host_rx_polarity_flip");
        cJSON *tx_polarity_flip_line = cJSON_GetObjectItem(slice_list_item, "line_tx_polarity_flip");
        cJSON *rx_polarity_flip_line = cJSON_GetObjectItem(slice_list_item, "line_rx_polarity_flip");
        if (tx_polarity_flip_host) {
            for (lane_id = 0; lane_id < cJSON_GetArraySize(tx_polarity_flip_host); lane_id++) {
                cJSON *poly_item = cJSON_GetArrayItem(tx_polarity_flip_host, lane_id);
                pol = cJSON_GetNumberValue(poly_item);
                swap_lane_id = sap_credo_poly_lane_swap(slice_info, lane_id, SAP_PORT_SIDE_SYS, SAP_CHANNEL_TX);
                if (USE_POLARITY_REVERSE) {
                    SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_tx_polarity", cr_serdes_get_tx_polarity(slice_info->cr_slice, swap_lane_id, &tmp_polarity));
                }
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_tx_polarity", cr_serdes_set_tx_polarity(slice_info->cr_slice, swap_lane_id, tmp_polarity^pol));
            }
        }
        if (rx_polarity_flip_host) {
            for (lane_id = 0; lane_id < cJSON_GetArraySize(rx_polarity_flip_host); lane_id++) {
                cJSON *poly_item = cJSON_GetArrayItem(rx_polarity_flip_host, lane_id);
                pol = cJSON_GetNumberValue(poly_item);
                swap_lane_id = sap_credo_poly_lane_swap(slice_info, lane_id, SAP_PORT_SIDE_SYS, SAP_CHANNEL_RX);
                if (USE_POLARITY_REVERSE) {
                    SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_rx_polarity", cr_serdes_get_rx_polarity(slice_info->cr_slice, swap_lane_id, &tmp_polarity));
                }
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_tx_polarity", cr_serdes_set_rx_polarity(slice_info->cr_slice, swap_lane_id, tmp_polarity^pol));
            }
        }
        if (tx_polarity_flip_line) {
            for (lane_id = 0; lane_id < cJSON_GetArraySize(tx_polarity_flip_line); lane_id++) {
                cJSON *poly_item = cJSON_GetArrayItem(tx_polarity_flip_line, lane_id);
                pol = cJSON_GetNumberValue(poly_item);
                swap_lane_id = sap_credo_poly_lane_swap(slice_info, lane_id, SAP_PORT_SIDE_LINE, SAP_CHANNEL_TX) + MAX_HOST_LANE_NUM;
                if (USE_POLARITY_REVERSE) {
                    SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_tx_polarity", cr_serdes_get_tx_polarity(slice_info->cr_slice, swap_lane_id, &tmp_polarity));
                }
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_tx_polarity", cr_serdes_set_tx_polarity(slice_info->cr_slice, swap_lane_id, tmp_polarity^pol));
            }
        }
        if (rx_polarity_flip_line) {
            for (lane_id = 0; lane_id < cJSON_GetArraySize(rx_polarity_flip_line); lane_id++) {
                cJSON *poly_item = cJSON_GetArrayItem(rx_polarity_flip_line, lane_id);
                pol = cJSON_GetNumberValue(poly_item);
                swap_lane_id = sap_credo_poly_lane_swap(slice_info, lane_id, SAP_PORT_SIDE_LINE, SAP_CHANNEL_RX) + MAX_HOST_LANE_NUM;
                if (USE_POLARITY_REVERSE) {
                    SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_rx_polarity", cr_serdes_get_rx_polarity(slice_info->cr_slice, swap_lane_id, &tmp_polarity));
                }
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_tx_polarity", cr_serdes_set_rx_polarity(slice_info->cr_slice, swap_lane_id, tmp_polarity^pol));
            }
        }
        break;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_uniretimer_check(sap_credo_port_info_t *port_info, CredoPortSetup_t *port_config, bool *Uni_Directional)
{
    int lane_id;
    int line_lane_id;
    int host_lane_id;
    int tx_lane_id, rx_lane_id;
    sap_slice_info_t *slice_info = port_info->slice_info;
    for (lane_id = 0; lane_id < port_config->line_count; lane_id++) {
        line_lane_id = port_config->line_lanes[lane_id] - MAX_HOST_LANE_NUM;
        tx_lane_id = slice_info->line_tx_lane_map[line_lane_id];
        rx_lane_id = slice_info->line_rx_lane_map[line_lane_id];
        if (tx_lane_id != rx_lane_id) {
            *Uni_Directional = true;
            return SAP_STATUS_SUCCESS;
        }
    }
    for (lane_id = 0; lane_id < port_config->host_count; lane_id++) {
        host_lane_id = port_config->host_lanes[lane_id];
        tx_lane_id = slice_info->host_tx_lane_map[host_lane_id];
        rx_lane_id = slice_info->host_rx_lane_map[host_lane_id];
        if (tx_lane_id != rx_lane_id) {
            *Uni_Directional = true;
            return SAP_STATUS_SUCCESS;
        }
    }
    *Uni_Directional = false;
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_port_init(sap_slice_info_t *slice_info)
{
    int lane_id, rv;
    int line_lane_id, host_lane_id;
    int cage_lane_id, back_lane_id;
    bool A1B2 = false;
    bool A2B1 = false;
    sap_credo_port_info_t *port_info;
    sap_port_info_t *sap_port_info;
    CredoPortSetup_t port_config;
    CredoPortSetup_t h2l_port_config;
    CredoPortSetup_t l2h_port_config;
    bool Uni_Directional;

    list_for_each_entry(sap_port_info, &global_port_list_head, node, sap_port_info_t) {
        port_info = sap_port_info->port_info;
        if (port_info->slice_info != slice_info) {
            continue;
        }
        SAP_CR_LOG_DBG("dev_id:%d, slice_id:%d, port_id:%d, port_init", slice_info->dev_info->dev_id, slice_info->slice_id, port_info->port_id);
        rv = sap_credo_get_profile_info(slice_info, port_info->port_id, &port_config);
        SAP_RV_CHECK(slice_info, "sap_credo_get_profile_info", rv);
        rv = sap_credo_uniretimer_check(port_info, &port_config, &Uni_Directional);
        SAP_RV_CHECK(slice_info, "sap_credo_uniretimer_check", rv);
        if (port_config.host_count > port_config.line_count) {
            A2B1 = true;
        }
        if (port_config.host_count < port_config.line_count) {
            A1B2 = true;
        }
        if (Uni_Directional == false) {
            SAP_CR_LOG_DBG("dev_id:%d, slice_id:%d, port_id:%d, Uni_Directional=false", slice_info->dev_info->dev_id, slice_info->slice_id, port_info->port_id);
            port_info->Uni_Directional = Uni_Directional;
            /* 处理bitmux/retimer/gearbox模式下crossbar线序交换问题 */
            for (host_lane_id = 0; host_lane_id < port_config.host_count; host_lane_id++) {
                cage_lane_id = port_config.host_lanes[host_lane_id];
                h2l_port_config.host_lanes[host_lane_id] = slice_info->host_rx_lane_map[cage_lane_id];
                // SAP_CR_LOG_DBG("port_config.host_lanes[%d]: %d", host_lane_id, port_config.host_lanes[host_lane_id]);
                if (A1B2) {
                    line_lane_id = host_lane_id * 2;
                    back_lane_id = port_config.line_lanes[line_lane_id] - MAX_HOST_LANE_NUM;
                    port_config.line_lanes[line_lane_id] = slice_info->line_rx_lane_map[back_lane_id] + MAX_HOST_LANE_NUM;
                    // SAP_CR_LOG_DBG("port_config.line_lanes[%d]: %d", line_lane_id, port_config.line_lanes[line_lane_id]);
                    line_lane_id = host_lane_id * 2 + 1;
                    back_lane_id = port_config.line_lanes[line_lane_id] - MAX_HOST_LANE_NUM;
                    port_config.line_lanes[line_lane_id] = slice_info->line_rx_lane_map[back_lane_id] + MAX_HOST_LANE_NUM;
                    // SAP_CR_LOG_DBG("port_config.line_lanes[%d]: %d", line_lane_id, port_config.line_lanes[line_lane_id]);
                } else if (A2B1) {
                    line_lane_id = (host_lane_id - (host_lane_id % 2)) / 2;
                    back_lane_id = port_config.line_lanes[line_lane_id] - MAX_HOST_LANE_NUM;
                    port_config.line_lanes[line_lane_id] = slice_info->line_rx_lane_map[back_lane_id] + MAX_HOST_LANE_NUM;
                    // SAP_CR_LOG_DBG("port_config.line_lanes[%d]: %d", line_lane_id, port_config.line_lanes[line_lane_id]);
                } else {
                    line_lane_id = host_lane_id;
                    back_lane_id = port_config.line_lanes[line_lane_id] - MAX_HOST_LANE_NUM;
                    port_config.line_lanes[line_lane_id] = slice_info->line_rx_lane_map[back_lane_id] + MAX_HOST_LANE_NUM;
                    // SAP_CR_LOG_DBG("port_config.line_lanes[%d]: %d", line_lane_id, port_config.line_lanes[line_lane_id]);
                }
            }
            rv = cr_port_build(slice_info->cr_slice, port_info->port_id, &port_config);
            SAP_CR_RV_CHECK(slice_info, "cr_port_build", rv);
            rv = cr_port_start(slice_info->cr_slice, port_info->port_id, CR_NOT_FORCE_PORT_CONFIG);
            SAP_CR_RV_CHECK(slice_info, "cr_port_start", rv);
        } else {
            SAP_CR_LOG_DBG("dev_id:%d, slice_id:%d, port_id:%d, Uni_Directional=true", slice_info->dev_info->dev_id, slice_info->slice_id, port_info->port_id);
            port_info->Uni_Directional = Uni_Directional;
            /* 处理uniretimer模式下crossbar线序交换问题 */
            /* CR_PORT_DIR_HOST_TO_LINE */
            memcpy(&h2l_port_config, &port_config, sizeof(CredoPortSetup_t));
            for (host_lane_id = 0; host_lane_id < port_config.host_count; host_lane_id++) {
                cage_lane_id = port_config.host_lanes[host_lane_id];
                h2l_port_config.host_lanes[host_lane_id] = slice_info->host_rx_lane_map[cage_lane_id];
                // SAP_CR_LOG_DBG("h2l_port_config.host_lanes[%d]: %d", host_lane_id, h2l_port_config.host_lanes[host_lane_id]);
                if (A1B2) {
                    line_lane_id = host_lane_id * 2;
                    back_lane_id = port_config.line_lanes[line_lane_id] - MAX_HOST_LANE_NUM;
                    h2l_port_config.line_lanes[line_lane_id] = slice_info->line_rx_lane_map[back_lane_id] + MAX_HOST_LANE_NUM;
                    // SAP_CR_LOG_DBG("h2l_port_config.line_lanes[%d]: %d", line_lane_id, h2l_port_config.line_lanes[line_lane_id]);
                    line_lane_id = host_lane_id * 2 + 1;
                    back_lane_id = port_config.line_lanes[line_lane_id] - MAX_HOST_LANE_NUM;
                    h2l_port_config.line_lanes[line_lane_id] = slice_info->line_rx_lane_map[back_lane_id] + MAX_HOST_LANE_NUM;
                    // SAP_CR_LOG_DBG("h2l_port_config.line_lanes[%d]: %d", line_lane_id, h2l_port_config.line_lanes[line_lane_id]);
                } else if (A2B1) {
                    line_lane_id = (host_lane_id - (host_lane_id % 2)) / 2;
                    back_lane_id = port_config.line_lanes[line_lane_id] - MAX_HOST_LANE_NUM;
                    h2l_port_config.line_lanes[line_lane_id] = slice_info->line_rx_lane_map[back_lane_id] + MAX_HOST_LANE_NUM;
                    // SAP_CR_LOG_DBG("h2l_port_config.line_lanes[%d]: %d", line_lane_id, h2l_port_config.line_lanes[line_lane_id]);
                } else {
                    line_lane_id = host_lane_id;
                    back_lane_id = port_config.line_lanes[line_lane_id] - MAX_HOST_LANE_NUM;
                    h2l_port_config.line_lanes[line_lane_id] = slice_info->line_rx_lane_map[back_lane_id] + MAX_HOST_LANE_NUM;
                    // SAP_CR_LOG_DBG("h2l_port_config.line_lanes[%d]: %d", line_lane_id, h2l_port_config.line_lanes[line_lane_id]);
                }
            }
            rv = cr_port_build(slice_info->cr_slice, port_info->port_id, &h2l_port_config);
            SAP_CR_RV_CHECK(slice_info, "cr_port_build", rv);
            rv = cr_port_set_option(slice_info->cr_slice, port_info->port_id, "direction", CR_PORT_DIR_HOST_TO_LINE);
            SAP_CR_RV_CHECK(slice_info, "cr_port_set_option", rv);
            rv = cr_port_start(slice_info->cr_slice, port_info->port_id, CR_NOT_FORCE_PORT_CONFIG);
            SAP_CR_RV_CHECK(slice_info, "cr_port_start", rv);
            /* CR_PORT_DIR_LINE_TO_HOST */
            memcpy(&l2h_port_config, &port_config, sizeof(CredoPortSetup_t));
            for (line_lane_id = 0; line_lane_id < port_config.line_count; line_lane_id++) {
                back_lane_id = port_config.line_lanes[line_lane_id] - MAX_HOST_LANE_NUM;
                l2h_port_config.line_lanes[line_lane_id] = slice_info->line_tx_lane_map[back_lane_id] + MAX_HOST_LANE_NUM;
                // SAP_CR_LOG_DBG("l2h_port_config.line_lanes[%d]: %d", line_lane_id, l2h_port_config.line_lanes[line_lane_id]);
                if (A2B1) {
                    host_lane_id = line_lane_id * 2;
                    cage_lane_id = port_config.host_lanes[host_lane_id];
                    l2h_port_config.host_lanes[host_lane_id] = slice_info->host_tx_lane_map[cage_lane_id];
                    // SAP_CR_LOG_DBG("l2h_port_config.host_lanes[%d]: %d", host_lane_id, l2h_port_config.host_lanes[host_lane_id]);
                    host_lane_id = line_lane_id * 2 + 1;
                    cage_lane_id = port_config.host_lanes[host_lane_id];
                    l2h_port_config.host_lanes[host_lane_id] = slice_info->host_tx_lane_map[cage_lane_id];
                    // SAP_CR_LOG_DBG("l2h_port_config.host_lanes[%d]: %d", host_lane_id, l2h_port_config.host_lanes[host_lane_id]);
                } else if (A1B2) {
                    host_lane_id = (line_lane_id - (line_lane_id % 2)) / 2;
                    cage_lane_id = port_config.host_lanes[host_lane_id];
                    l2h_port_config.host_lanes[host_lane_id] = slice_info->host_tx_lane_map[cage_lane_id];
                    // SAP_CR_LOG_DBG("l2h_port_config.host_lanes[%d]: %d", host_lane_id, l2h_port_config.host_lanes[host_lane_id]);
                } else {
                    host_lane_id = line_lane_id;
                    cage_lane_id = port_config.host_lanes[host_lane_id];
                    l2h_port_config.host_lanes[host_lane_id] = slice_info->host_tx_lane_map[cage_lane_id];
                    // SAP_CR_LOG_DBG("l2h_port_config.host_lanes[%d]: %d", host_lane_id, l2h_port_config.host_lanes[host_lane_id]);
                }
            }
            rv = cr_port_build(slice_info->cr_slice, port_info->port_id + MAX_SLICE_PORT_NUM, &l2h_port_config);
            SAP_CR_RV_CHECK(slice_info, "cr_port_build", rv);
            rv = cr_port_set_option(slice_info->cr_slice, port_info->port_id + MAX_SLICE_PORT_NUM, "direction", CR_PORT_DIR_LINE_TO_HOST);
            SAP_CR_RV_CHECK(slice_info, "cr_port_set_option", rv);
            rv = cr_port_start(slice_info->cr_slice, port_info->port_id + MAX_SLICE_PORT_NUM, CR_NOT_FORCE_PORT_CONFIG);
            SAP_CR_RV_CHECK(slice_info, "cr_port_start", rv);
        }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_post_port_init(sap_slice_info_t *slice_info)
{
    int i,j,rv;
    sap_credo_port_info_t *port_info;

    for (i = 0; i < MAX_SLICE_PORT_NUM; i++) {
        if (slice_info->port_info[i] == NULL) {
            continue;
        }
        port_info = slice_info->port_info[i];
        rv = sap_credo_set_port_admin(port_info->unit, port_info->port, CR_ENABLE);
        SAP_RV_CHECK(slice_info, "sap_credo_set_port_admin", rv);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_get_profile_info(sap_slice_info_t *slice_info, int port_id, CredoPortSetup_t *conf)
{
    int i,j;
    int speed;
    int split_num;
    char breakout_mode_buff[MAX_BREAKOUT_MODE_STR_LEN];
    sap_credo_port_info_t *port_info;
    port_info = slice_info->port_info[port_id];
    speed = port_info->speed;
    split_num = slice_info->global_lane_num / port_info->num_lanes;
    SAP_CR_LOG_DBG("global_lane_num:%d, port_lane_num:%d", slice_info->global_lane_num, port_info->num_lanes);
    snprintf(breakout_mode_buff, MAX_BREAKOUT_MODE_STR_LEN, "%dx%d", split_num, speed);
    SAP_CR_LOG_DBG("port breakout_mode: %s", breakout_mode_buff);
    cJSON *slice_profile_config = cJSON_GetObjectItem(j_config, "slice_profile_config");
    cJSON *slice_profile = cJSON_GetObjectItem(slice_profile_config, slice_info->profile);
    cJSON *breakout_mode_config = cJSON_GetObjectItem(slice_profile, "breakout_mode");
    cJSON *breakout_mode = cJSON_GetObjectItem(breakout_mode_config, breakout_mode_buff);
    cJSON *port_profile_type = NULL;
    for (i = 0; i < cJSON_GetArraySize(breakout_mode); i++) {
        cJSON *subport_item = cJSON_GetArrayItem(breakout_mode, i);
        if (GET_NUMBER_VALUE(subport_item, "port_id") != port_id) {
            continue;
        }
        port_profile_type = cJSON_GetObjectItem(subport_item, "port_profile");
        break;
    }
    if (port_profile_type == NULL) {
        SAP_CR_LOG_ERR("get port profile type failed!");
        return SAP_STATUS_FAILURE;
    }
    cJSON *port_profile_config = cJSON_GetObjectItem(j_config, "port_profile_config");
    cJSON *port_profile = cJSON_GetObjectItem(port_profile_config, cJSON_GetStringValue(port_profile_type));
    conf->mode = GET_NUMBER_VALUE(port_profile, "connection_mode");
    conf->line_count = GET_NUMBER_VALUE(port_profile, "line_no_of_lanes");
    conf->host_count = GET_NUMBER_VALUE(port_profile, "host_no_of_lanes");
    /* TODO: 不能用start_lane */
    for (i = 0; i < conf->line_count; i++) {
        conf->line_lanes[i] = GET_NUMBER_VALUE(port_profile, "line_start_lane") + i;
    }
    for (i = 0; i < conf->host_count; i++) {
        conf->host_lanes[i] = GET_NUMBER_VALUE(port_profile, "host_start_lane") + i;
    }
    conf->speed = speed;
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_get_slice_info(int unit, int port, sap_slice_info_t **slice_info)
{
    int rv;
    sap_credo_port_info_t *port_info;
    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv == SAP_STATUS_ITEM_NOT_FOUND) {
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    *slice_info = port_info->slice_info;
    return SAP_STATUS_SUCCESS; 
}

static int sap_credo_get_slice_info_by_phy_lane0(int unit, int phy_lane0, sap_slice_info_t **slice_info)
{
    int i,j,rv;
    for (i = 0; i < global_slice_num; i++) {
        for (j = 0; j < global_slice_info[i].global_lane_num; j++) {
            if (global_slice_info[i].global_lanes[j] != phy_lane0) {
                continue;
            }
            *slice_info = &global_slice_info[i];
            return SAP_STATUS_SUCCESS;
        }
    }
    return SAP_STATUS_ITEM_NOT_FOUND;
}

static int sap_credo_get_port_info(int unit, int port, sap_credo_port_info_t **port_info)
{
    sap_port_info_t *tmp;
    // sap_credo_port_info_t *port_info;
    list_for_each_entry(tmp, &global_port_list_head, node, sap_port_info_t) {
        if (tmp->unit != unit || tmp->port != port) {
            continue;
        }
        *port_info = tmp->port_info;
        return SAP_STATUS_SUCCESS;
    }
    return SAP_STATUS_ITEM_NOT_FOUND;
}

static int sap_credo_set_port_admin(int unit, int port, bool enable)
{
    int rv, lane, lane_id;
    sap_credo_port_info_t *port_info;
    CredoPortSetup_t port_setup;
    sap_slice_info_t *slice_info;
    bool started;

    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv) {
        SAP_CR_LOG_ERR("sap_credo_set_port_admin fail, unit:%d port:%d enable:%d", unit, port, enable);
        return SAP_STATUS_FAILURE;
    }
    rv = cr_port_get_setup(port_info->slice_info->cr_slice, port_info->port_id, &started, &port_setup);
    SAP_CR_RV_CHECK(port_info->slice_info, "cr_port_get_setup", rv);
    if (started == false) {
        SAP_CR_LOG_WARN("unit:%d, port:%d, port not started.", unit, port);
        return SAP_STATUS_FAILURE; 
    }
    for (lane_id = 0; lane_id < port_setup.host_count; lane_id++) {
        lane = port_setup.host_lanes[lane_id];
        if (enable) {
            rv = cr_lane_rx_no_disable(port_info->slice_info->cr_slice, lane);
            SAP_CR_RV_CHECK(port_info->slice_info, "cr_lane_rx_no_disable", rv);
            rv = cr_lane_tx_no_disable(port_info->slice_info->cr_slice, lane);
            SAP_CR_RV_CHECK(port_info->slice_info, "cr_lane_tx_no_disable", rv);
        } else {
            rv = cr_lane_rx_disable(port_info->slice_info->cr_slice, lane);
            SAP_CR_RV_CHECK(port_info->slice_info, "cr_lane_rx_disable", rv);
            rv = cr_lane_tx_disable(port_info->slice_info->cr_slice, lane);
            SAP_CR_RV_CHECK(port_info->slice_info, "cr_lane_tx_disable", rv);
        }
    }
    for (lane_id = 0; lane_id < port_setup.line_count; lane_id++) {
        lane = port_setup.line_lanes[lane_id];
        if (enable) {
            rv = cr_lane_rx_no_disable(port_info->slice_info->cr_slice, lane);
            SAP_CR_RV_CHECK(port_info->slice_info, "cr_lane_rx_no_disable", rv);
            rv = cr_lane_tx_no_disable(port_info->slice_info->cr_slice, lane);
            SAP_CR_RV_CHECK(port_info->slice_info, "cr_lane_tx_no_disable", rv);
        } else {
            rv = cr_lane_rx_disable(port_info->slice_info->cr_slice, lane);
            SAP_CR_RV_CHECK(port_info->slice_info, "cr_lane_rx_disable", rv);
            rv = cr_lane_tx_disable(port_info->slice_info->cr_slice, lane);
            SAP_CR_RV_CHECK(port_info->slice_info, "cr_lane_tx_disable", rv);
        }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_check_fw(sap_slice_info_t *slice_info)
{
    int rv, retry_cnt;
    unsigned int fw_ver, fw_crc;
    char buffer[128];
    char buffer2[128];
    sap_dev_info_t *dev_info = slice_info->dev_info;

    retry_cnt = 0;
    do {
        retry_cnt++;
        rv = cr_firmware_hash(slice_info->cr_slice, &fw_ver);
        SAP_CR_RV_CHECK(slice_info, "cr_firmware_hash", rv);
        rv = cr_firmware_crc(slice_info->cr_slice, &fw_crc);
        SAP_CR_RV_CHECK(slice_info, "cr_firmware_crc", rv);
        snprintf(buffer, sizeof(buffer), "fw_ver:0x%x, fw_crc:0x%x", fw_ver, fw_crc);
        snprintf(buffer2, sizeof(buffer2), "fw_ver:%s, fw_crc:%s", global_firmware_info.fw_ver, global_firmware_info.fw_crc);
        if (strncmp(buffer, buffer2, strlen(buffer)) == 0) {
            SAP_CR_LOG_INFO("credo dev_id:%d slice_id:%d, fw version match, %s", dev_info->dev_id, slice_info->slice_id, buffer);
            return SAP_STATUS_SUCCESS;
        }
        SAP_CR_LOG_ERR("credo dev_id:%d slice_id:%d, fw version not match, %s", dev_info->dev_id, slice_info->slice_id, buffer);
        if (retry_cnt < MAX_FW_RETRY_TIMES) {
            sleep(MAX_FW_RETRY_DELAY_SECS);
            SAP_CR_LOG_WARN("credo dev_id:%d slice_id:%d, fw load retry_cnt:%d", dev_info->dev_id, slice_info->slice_id, retry_cnt);
            rv = cr_firmware_load(slice_info->cr_slice, global_firmware_info.fw_path, CR_FORCE_FW_LOAD);
            SAP_CR_RV_CHECK(slice_info, "cr_firmware_load", rv);
        }
    } while (retry_cnt < MAX_FW_RETRY_TIMES);

    return SAP_STATUS_FAILURE;
}

static int sap_credo_firmware_broadcast_load(sap_slice_info_t *slice_info[], int slice_num)
{
    int rv;
    int slice_id;
    CredoSlice_t *slices_broadcast[MAX_MDIO_DEV_NUM*MAX_DEV_SLICE_NUM];

    for (slice_id = 0; slice_id < slice_num; slice_id++) {
        slices_broadcast[slice_id] = slice_info[slice_id]->cr_slice;
    }
    rv = cr_firmware_load_broadcast(slices_broadcast, slice_num, global_firmware_info.fw_path, CR_FW_WRITE_DELAY_US, CR_FORCE_FW_LOAD);
    if (rv) {
        return SAP_STATUS_FAILURE;
    } 
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_slice_init(int mdio_id)
{
    int slice_id, dev_id, rv;
    CredoSliceConfig_t slice_cfg;
    CredoSliceInitType_t init_type;
    // sap_dev_info_t *dev_info;
    sap_slice_info_t *broadcast_slices[MAX_MDIO_DEV_NUM*MAX_DEV_SLICE_NUM];
    sap_slice_info_t *slice_info;
    int broadcast_slices_num = 0;
    init_type = CR_INIT_FULL; /* TODO: 需要从配置文件里面获取 */

    for (slice_id = 0; slice_id < global_slice_num; slice_id++) {
        slice_info = &global_slice_info[slice_id];
        if (slice_info->mdio_id != mdio_id) {
            continue;
        }
        slice_cfg.init_type = init_type;
        slice_cfg.firmware_filename = global_firmware_info.fw_path,
        slice_cfg.slice_id = slice_id;
        // slice_cfg.slice_id = slice_info->slice_id;
        slice_cfg.slice_context = &slice_info->slice_ctx;
        rv = cr_slice_init(slice_info->cr_slice, &slice_cfg);
        if (rv) {
            SAP_CR_LOG_ERR("credo slice init failed, slice_id:%d, rv:%d", slice_id, rv);
            return SAP_STATUS_FAILURE;
        }
        /* 细化固件加载方式 */
        slice_info->inited = true;
        if (init_type == CR_INIT_NO_FIRMWARE) {
            broadcast_slices[broadcast_slices_num++] = slice_info;
        }
    }
    /* 加载固件 */
    if (broadcast_slices_num > 0) {
        SAP_CR_LOG_INFO("credo mdio_id:%d fw load start...", mdio_id);
        rv = sap_credo_firmware_broadcast_load(broadcast_slices, broadcast_slices_num);
        if (rv) {
            SAP_CR_LOG_ERR("sap_credo_firmware_broadcast_load failed, mdio_id:%d, rv:%d", mdio_id, rv);
            return SAP_STATUS_FAILURE;
        }
        SAP_CR_LOG_INFO("credo mdio_id:%d fw load end...", mdio_id);
    }
    /* 检查固件是否加载成功, 对加载失败的进行重试 */
    SAP_CR_LOG_INFO("credo mdio:%d fw check start...", mdio_id);
    for (slice_id = 0; slice_id < global_slice_num; slice_id++) {
        if (global_slice_info[slice_id].mdio_id != mdio_id) {
            continue;
        }
        if (sap_credo_check_fw(&global_slice_info[slice_id]) == SAP_STATUS_SUCCESS) {
            global_slice_info[slice_id].loaded = true;
        } else {
            SAP_CR_LOG_ERR("credo firmware check failed, slice_id:%d", slice_id);
        }
    }
    SAP_CR_LOG_INFO("credo mdio:%d fw check end...", mdio_id);
    /* 初始化端口 */
    for (slice_id = 0; slice_id < global_slice_num; slice_id++) {
        if (global_slice_info[slice_id].mdio_id != mdio_id) {
            continue;
        }
        SAP_CR_LOG_INFO("sap credo port init start, mdio_id:%d, slice_id:%d", mdio_id, slice_id);
        rv = sap_credo_pre_port_init(&global_slice_info[slice_id]);
        if (rv) {
            SAP_CR_LOG_ERR("sap_credo_pre_port_init failed, mdio_id:%d, rv:%d", mdio_id, rv);
            return SAP_STATUS_FAILURE;
        }
        rv = sap_credo_port_init(&global_slice_info[slice_id]);
        if (rv) {
            SAP_CR_LOG_ERR("sap_credo_port_init failed, mdio_id:%d, rv:%d", mdio_id, rv);
            return SAP_STATUS_FAILURE;
        }
        rv = sap_credo_post_port_init(&global_slice_info[slice_id]);
        if (rv) {
            SAP_CR_LOG_ERR("sap_credo_post_port_init failed, mdio_id:%d, rv:%d", mdio_id, rv);
            return SAP_STATUS_FAILURE;
        }
        SAP_CR_LOG_INFO("sap credo port init end, mdio_id:%d, slice_id:%d", mdio_id, slice_id);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_loopback_set(int unit, int port, int if_side, int lb_dir, int enable)
{
    int i,rv;
    int lane_id;
    sap_credo_port_info_t *port_info;
    sap_slice_info_t *slice_info;
    CredoLaneLoopbackMode_t mode;
    CredoPortSetup_t port_setup;
    bool started;

    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_CR_LOG_ERR("sap_credo_get_port_info failed, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE;
    }
    if (port_info->Uni_Directional == true) {
        SAP_CR_LOG_WARN("port %d under uniretimer mode", port);
    }
    slice_info = port_info->slice_info;
    if (enable) {
        switch (lb_dir) {
            case SAP_LOOPBACK_DIR_LOCAL:
                mode = CR_LB_TX_TO_RX;
                break;
            case SAP_LOOPBACK_DIR_REMOTE:
                mode = CR_LB_RX_TO_TX;
                break;
            default:
                SAP_CR_LOG_WARN("invalid loopback direction, unit:%d, port:%d, dir:%d", unit, port, lb_dir);
                return SAP_STATUS_FAILURE;
        }
    } else {
        mode = CR_LB_DISABLED;
    }
    rv = cr_port_get_setup(slice_info->cr_slice, port_info->port_id, &started, &port_setup);
    SAP_CR_RV_CHECK(slice_info, "cr_port_get_setup", rv);
    if (started == false) {
        SAP_CR_LOG_WARN("port not started, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE; 
    }
    if (if_side == SAP_PORT_SIDE_LINE) {
        for (i = 0; i < port_setup.line_count; i ++) {
            lane_id = port_setup.line_lanes[i];
            rv = cr_lane_set_loopback_mode(slice_info->cr_slice, lane_id, mode);
            SAP_CR_RV_CHECK(slice_info, "cr_lane_set_loopback_mode", rv);
        }
    } else if (if_side == SAP_PORT_SIDE_SYS) {
        for (i = 0; i < port_setup.host_count; i ++) {
            lane_id = port_setup.host_lanes[i];
            rv = cr_lane_set_loopback_mode(slice_info->cr_slice, lane_id, mode);
            SAP_CR_RV_CHECK(slice_info, "cr_lane_set_loopback_mode", rv);
        }
    }

    return SAP_STATUS_SUCCESS;
}

static int sap_credo_loopback_get(int unit, int port, int if_side, int lb_dir, int *enable)
{
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_polarity_set(int unit, int port, int if_side, int tx_rx, uint32_t polarity, bool override)
{
    int i,rv;
    int lane_id;
    int port_id;
    sap_credo_port_info_t *port_info;
    sap_slice_info_t *slice_info;
    CredoPortSetup_t port_setup;
    bool started;
    int tmp_polarity;

    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_CR_LOG_ERR("sap_credo_get_port_info failed, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE;
    }
    slice_info = port_info->slice_info;
    rv = sap_credo_query_port_setup(port_info, if_side, tx_rx, &port_setup, &started);
    SAP_RV_CHECK(slice_info, "sap_credo_query_port_setup", rv);
    if (started == false) {
        SAP_CR_LOG_WARN("port not started, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE; 
    }

    if (if_side == SAP_PORT_SIDE_LINE) {
        for (i = 0; i < port_setup.line_count; i ++) {
            lane_id = port_setup.line_lanes[i];
            if (tx_rx == SAP_CHANNEL_TX) {
                rv = cr_serdes_get_tx_polarity(slice_info->cr_slice, lane_id, &tmp_polarity);
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_tx_polarity", rv);
            } else if (tx_rx == SAP_CHANNEL_RX) {
                rv = cr_serdes_get_rx_polarity(slice_info->cr_slice, lane_id, &tmp_polarity);
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_rx_polarity", rv);
            }
            if (override == true) {
                tmp_polarity = (polarity & (0x1 << i)) >> i;
            } else {
                tmp_polarity = tmp_polarity ^ (polarity & (0x1 << i)) >> i;
            }

            if (tx_rx == SAP_CHANNEL_TX) {
                rv = cr_serdes_set_tx_polarity(slice_info->cr_slice, lane_id, tmp_polarity);
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_tx_polarity", rv);
            } else if (tx_rx == SAP_CHANNEL_RX) {
                rv = cr_serdes_set_rx_polarity(slice_info->cr_slice, lane_id, tmp_polarity);
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_rx_polarity", rv);
            }
        }
    } else if (if_side == SAP_PORT_SIDE_SYS) {
        for (i = 0; i < port_setup.host_count; i ++) {
            lane_id = port_setup.host_lanes[i];

            if (tx_rx == SAP_CHANNEL_TX) {
                rv = cr_serdes_get_tx_polarity(slice_info->cr_slice, lane_id, &tmp_polarity);
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_tx_polarity", rv);
            } else if (tx_rx == SAP_CHANNEL_RX) {
                rv = cr_serdes_get_rx_polarity(slice_info->cr_slice, lane_id, &tmp_polarity);
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_rx_polarity", rv);
            }

            if (override == true) {
                tmp_polarity = (polarity & (0x1 << i)) >> i;
            } else {
                tmp_polarity = tmp_polarity ^ (polarity & (0x1 << i)) >> i;
            }
            
            if (tx_rx == SAP_CHANNEL_TX) {
                rv = cr_serdes_set_tx_polarity(slice_info->cr_slice, lane_id, tmp_polarity);
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_tx_polarity", rv);
            } else if (tx_rx == SAP_CHANNEL_RX) {
                rv = cr_serdes_set_rx_polarity(slice_info->cr_slice, lane_id, tmp_polarity);
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_rx_polarity", rv);
            }
        }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_query_port_setup(sap_credo_port_info_t *port_info, int if_side, int tx_rx, CredoPortSetup_t *port_setup, bool *started)
{
    int rv, port_id;

    port_id = port_info->port_id;
    if (port_info->Uni_Directional == false) {
        port_id = port_info->port_id;
    } else {
        /* 针对uniretimer模式,HOST->LINE放在前8，LINE->HOST放在后8 */
        if ((if_side == SAP_PORT_SIDE_LINE) && (tx_rx == SAP_CHANNEL_TX)) {
            port_id = port_info->port_id;
        } else if ((if_side == SAP_PORT_SIDE_SYS) && (tx_rx == SAP_CHANNEL_RX)) {
            port_id = port_info->port_id;
        } else if (if_side == SAP_CHANNEL_BOTH){
            port_id = port_info->port_id;
        } else {
            port_id = port_info->port_id + MAX_SLICE_PORT_NUM;
        }
    }
    rv = cr_port_get_setup(port_info->slice_info->cr_slice, port_id, started, port_setup);
    SAP_CR_RV_CHECK(port_info->slice_info, "cr_port_get_setup", rv);
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_polarity_get(int unit, int port, int if_side, int tx_rx, uint32_t *polarity)
{
    int i,rv;
    int lane_id;
    int port_id;
    sap_credo_port_info_t *port_info;
    sap_slice_info_t *slice_info;
    CredoPortSetup_t port_setup;
    bool started;
    int tmp_polarity;

    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_CR_LOG_ERR("sap_credo_get_port_info failed, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE;
    }
    slice_info = port_info->slice_info;
    SAP_RV_CHECK(slice_info, "sap_credo_query_port_setup", sap_credo_query_port_setup(port_info, if_side, tx_rx, &port_setup, &started));
    if (started == false) {
        SAP_CR_LOG_WARN("port not started, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE; 
    }
    *polarity = 0;
    if (if_side == SAP_PORT_SIDE_LINE) {
        for (i = 0; i < port_setup.line_count; i ++) {
            lane_id = port_setup.line_lanes[i];
            if (tx_rx == SAP_CHANNEL_TX) {
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_tx_polarity", cr_serdes_get_tx_polarity(slice_info->cr_slice, lane_id, &tmp_polarity));
            } else if (tx_rx == SAP_CHANNEL_RX) {
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_rx_polarity", cr_serdes_get_rx_polarity(slice_info->cr_slice, lane_id, &tmp_polarity));
            }
            *polarity = (*polarity) | (tmp_polarity << i);
        }
    } else if (if_side == SAP_PORT_SIDE_SYS) {
        for (i = 0; i < port_setup.host_count; i ++) {
            lane_id = port_setup.host_lanes[i];
            if (tx_rx == SAP_CHANNEL_TX) {
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_tx_polarity", cr_serdes_get_tx_polarity(slice_info->cr_slice, lane_id, &tmp_polarity));
            } else if (tx_rx == SAP_CHANNEL_RX) {
                SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_rx_polarity", cr_serdes_get_rx_polarity(slice_info->cr_slice, lane_id, &tmp_polarity));
            }
            *polarity = (*polarity) | (tmp_polarity << i);
        }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_port_list_get(sap_port_list_t *port_list)
{
    port_list->list = &global_port_list_head;
    port_list->port_count = global_port_num;
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_prbs_change(int poly, CredoPrbsPattern_t *cr_poly)
{
    SAP_POINT_CHECK_RV(cr_poly, "cr_poly", SAP_STATUS_INVALID_PARAMETER);
    switch(poly) {
        case SAP_PRBS_POLY_P7:
            *cr_poly = CR_PRBS7;
            break;
        case SAP_PRBS_POLY_P9:
            *cr_poly = CR_PRBS9;
            break;
        case SAP_PRBS_POLY_P11:
            *cr_poly = CR_PRBS11;
            break;
        case SAP_PRBS_POLY_P13:
            *cr_poly = CR_PRBS13;
            break;
        case SAP_PRBS_POLY_P15:
            *cr_poly = CR_PRBS15;
            break;
        case SAP_PRBS_POLY_P23:
            *cr_poly = CR_PRBS23;
            break;
        case SAP_PRBS_POLY_P31:
            *cr_poly = CR_PRBS31;
            break;
        case SAP_PRBS_POLY_P19:
            *cr_poly = CR_PRBS19;
            break;
        default:
            return SAP_STATUS_INVALID_PARAMETER;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_prbs_operation_set(int tx_rx, sap_slice_info_t *slice_info, int if_side, 
                       const CredoPortSetup_t *port_setup, int lane, CredoPrbsPattern_t cr_poly, int admin) {
    int lane_id, i, count;
    const int *lanes;

    if (if_side == SAP_PORT_SIDE_LINE) {
        count = port_setup->line_count;
        lanes = port_setup->line_lanes;
    } else if (if_side == SAP_PORT_SIDE_SYS) {
        count = port_setup->host_count;
        lanes = port_setup->host_lanes;
    } else {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if (lane == SAP_LANE_ID_ALL) {
        for (i = 0; i < count; i++) {
            lane_id = lanes[i];
            if (tx_rx == SAP_CHANNEL_TX) {
                SAP_CR_RV_CHECK(slice_info, "cr_prbs_set_tx_generator", cr_prbs_set_tx_generator(slice_info->cr_slice, lane_id, admin, cr_poly));
            } else {
                SAP_CR_RV_CHECK(slice_info, "cr_prbs_set_rx_checker", cr_prbs_set_rx_checker(slice_info->cr_slice, lane_id, admin, cr_poly));
                SAP_CR_RV_CHECK(slice_info, "cr_prbs_reset_rx_count", cr_prbs_reset_rx_count(slice_info->cr_slice, lane_id));
            }
        }
    } else {
        if ((lane - SAP_LANE_ID_START) >= count) {
            return SAP_STATUS_INVALID_PARAMETER;
        }
        lane_id = lanes[lane - SAP_LANE_ID_START];
        if (tx_rx == SAP_CHANNEL_TX) {
            SAP_CR_RV_CHECK(slice_info, "cr_prbs_set_tx_generator", cr_prbs_set_tx_generator(slice_info->cr_slice, lane_id, admin, cr_poly));
        } else {
            SAP_CR_RV_CHECK(slice_info, "cr_prbs_set_rx_checker", cr_prbs_set_rx_checker(slice_info->cr_slice, lane_id, admin, cr_poly));
            SAP_CR_RV_CHECK(slice_info, "cr_prbs_reset_rx_count", cr_prbs_reset_rx_count(slice_info->cr_slice, lane_id));
        }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_port_prbs_set_ex(int unit, int port, int poly, int inv, int if_side, int lane, int admin)
{
    int i,rv;
    int lane_id;
    sap_credo_port_info_t *port_info;
    sap_slice_info_t *slice_info;
    CredoPortSetup_t port_setup;
    CredoPrbsPattern_t cr_poly;
    bool started;

    if (lane != SAP_LANE_ID_ALL) {
        if (lane < SAP_LANE_ID_START || lane > (MAX_MAC_LANE_NUM + SAP_LANE_ID_START)) {
            SAP_CR_LOG_ERR("Invalid lane value: %d, unit:%d, port:%d", lane, unit, port);
            return SAP_STATUS_INVALID_PARAMETER;
        }
    }
    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_CR_LOG_ERR("sap_credo_get_port_info failed, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE;
    }
    slice_info = port_info->slice_info;
    SAP_RV_CHECK(slice_info, "sap_credo_get_port_info", sap_credo_prbs_change(poly, &cr_poly));
    rv = sap_credo_query_port_setup(port_info, if_side, SAP_CHANNEL_TX, &port_setup, &started);
    SAP_RV_CHECK(slice_info, "sap_credo_query_port_setup", rv);
    if (started == false) {
        SAP_CR_LOG_WARN("port not started, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE; 
    }
    rv = sap_credo_prbs_operation_set(SAP_CHANNEL_TX, slice_info, if_side, &port_setup, lane, cr_poly, admin);
    SAP_RV_CHECK(slice_info, "sap_credo_prbs_operation_set", rv);
    /* Configure RX Checker to the correct PRBS pattern */
    rv = sap_credo_query_port_setup(port_info, if_side, SAP_CHANNEL_RX, &port_setup, &started);
    SAP_RV_CHECK(slice_info, "sap_credo_query_port_setup", rv);
    if (started == false) {
        SAP_CR_LOG_WARN("port not started, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE; 
    }
    rv = sap_credo_prbs_operation_set(SAP_CHANNEL_RX, slice_info, if_side, &port_setup, lane, cr_poly, admin);
    SAP_RV_CHECK(slice_info, "sap_credo_prbs_operation_set", rv);
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_port_prbs_set(int unit, int port, int poly, int inv, int if_side, int lane)
{
    return sap_credo_port_prbs_set_ex(unit, port, poly, inv, if_side, lane, CR_ENABLE);
}

/* 读完清除 */
static int sap_credo_prbs_operation_get(sap_slice_info_t *slice_info, int if_side, const CredoPortSetup_t *port_setup, int lane, sap_prbs_status_t *prbs_info) {
    int lane_id, rv, i, count;
    const int *lanes;
    int link_up = 1;
    unsigned rdy;
    int all_rdy = 0;
    int passed_timesec = 0;

    if (if_side == SAP_PORT_SIDE_LINE) {
        count = port_setup->line_count;
        lanes = port_setup->line_lanes;
    } else if (if_side == SAP_PORT_SIDE_SYS) {
        count = port_setup->host_count;
        lanes = port_setup->host_lanes;
    } else {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if ((lane - SAP_LANE_ID_START) >= count) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if (lane == SAP_LANE_ID_ALL) {
        for (i = 0; i < count; i++) {
            lane_id = lanes[i];
            prbs_info[i].is_get = 1;
            prbs_info[i].lock_loss = 0;
            /* The lock check only works in PAM4 mode. */
            SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_phy_ready", cr_serdes_get_phy_ready(slice_info->cr_slice, lane_id, &rdy));
            if (rdy) {
                /* ready之后才能统计，否则会有很多错误 */
                SAP_CR_RV_CHECK(slice_info, "cr_prbs_get_rx_lock", cr_prbs_get_rx_lock(slice_info->cr_slice, lane_id, &prbs_info[i].prbs_lock));
            } else {
                prbs_info[i].prbs_lock = 0;
            }
            SAP_CR_RV_CHECK(slice_info, "cr_prbs_get_rx_count", cr_prbs_get_rx_count(slice_info->cr_slice, lane_id, &prbs_info[i].lane_err_cnt));
            SAP_CR_RV_CHECK(slice_info, "cr_prbs_get_rx_ber", cr_prbs_get_rx_ber(slice_info->cr_slice, lane_id, 0, &prbs_info[i].ber));
            SAP_CR_RV_CHECK(slice_info, "cr_prbs_reset_rx_count", cr_prbs_reset_rx_count(slice_info->cr_slice, lane_id));
        }
    } else {
        if ((lane - SAP_LANE_ID_START) >= count) {
            return SAP_STATUS_INVALID_PARAMETER;
        }
        lane_id = lanes[lane - SAP_LANE_ID_START];
        prbs_info->is_get = 1;
        prbs_info->lock_loss = 0;
        SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_phy_ready", cr_serdes_get_phy_ready(slice_info->cr_slice, lane_id, &rdy));
        if (rdy) {
            SAP_CR_RV_CHECK(slice_info, "cr_prbs_get_rx_lock", cr_prbs_get_rx_lock(slice_info->cr_slice, lane_id, &prbs_info->prbs_lock));
        } else {
            prbs_info->prbs_lock = 0;
        }
        /* The lock check only works in PAM4 mode. */
        SAP_CR_RV_CHECK(slice_info, "cr_prbs_get_rx_count", cr_prbs_get_rx_count(slice_info->cr_slice, lane_id, &prbs_info->lane_err_cnt));
        SAP_CR_RV_CHECK(slice_info, "cr_prbs_get_rx_ber", cr_prbs_get_rx_ber(slice_info->cr_slice, lane_id, 0, &prbs_info->ber));
        SAP_CR_RV_CHECK(slice_info, "cr_prbs_reset_rx_count", cr_prbs_reset_rx_count(slice_info->cr_slice, lane_id));
    }

    return SAP_STATUS_SUCCESS;
}

static int sap_credo_port_prbs_get(int unit, int port, int if_side, sap_prbs_status_t *prbs_info)
{
    int i,rv;
    int lane_id;
    sap_credo_port_info_t *port_info;
    sap_slice_info_t *slice_info;
    CredoPortSetup_t port_setup;
    CredoPrbsPattern_t cr_poly;
    int lane = SAP_LANE_ID_ALL;
    bool started;

    SAP_POINT_CHECK_RV(prbs_info, "prbs_info", SAP_STATUS_INVALID_PARAMETER);
    if (lane != SAP_LANE_ID_ALL) {
        if (lane < SAP_LANE_ID_START || lane > (MAX_MAC_LANE_NUM + SAP_LANE_ID_START)) {
            SAP_CR_LOG_ERR("Invalid lane value: %d, unit:%d, port:%d", lane, unit, port);
            return SAP_STATUS_INVALID_PARAMETER;
        }
    }
    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_CR_LOG_ERR("sap_credo_get_port_info failed, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE;
    }
    slice_info = port_info->slice_info;
    rv = sap_credo_query_port_setup(port_info, if_side, SAP_CHANNEL_RX, &port_setup, &started);
    SAP_RV_CHECK(slice_info, "sap_credo_query_port_setup", rv);
    if (started == false) {
        SAP_CR_LOG_WARN("port not started, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE; 
    }
    rv = sap_credo_prbs_operation_get(slice_info, if_side, &port_setup, lane, prbs_info);
    SAP_RV_CHECK(slice_info, "sap_credo_prbs_operation_get", rv);
    return SAP_STATUS_SUCCESS;  
}

static int sap_credo_port_prbs_clear(int unit, int port, int if_side)
{
    return sap_credo_port_prbs_set_ex(unit, port, 0, 0, if_side, SAP_LANE_ID_ALL, CR_DISABLE);
}

static int sap_credo_port_prbs_ber_get(int unit, int port, int if_side, int time_v, sap_prbs_status_t *prbs_info)
{
    return sap_credo_port_prbs_get(unit, port, if_side, prbs_info);
}

static int sap_credo_port_option_save(sap_slice_info_t *slice_info, int port_id, sap_credo_port_option_t *port_option, int option_cnt)
{
    int i, rv;
    for (i = 0; i < option_cnt; i++) {
        rv = cr_port_get_option(slice_info->cr_slice, port_id, port_option[i].cr_option.name, &port_option[i].option_value);
        SAP_CR_RV_CHECK(slice_info, "cr_port_get_option", rv);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_port_option_restore(sap_slice_info_t *slice_info, int port_id, sap_credo_port_option_t *port_option, int option_cnt)
{
    int i, rv;
    for (i = 0; i < option_cnt; i++) {
        rv = cr_port_set_option(slice_info->cr_slice, port_id, port_option[i].cr_option.name, port_option[i].option_value);
        SAP_CR_RV_CHECK(slice_info, "cr_port_set_option", rv);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_linktraining_set(int unit, int port, int if_side, int enable)
{
    int i, rv;
    sap_credo_port_info_t *port_info;
    sap_slice_info_t *slice_info;
    CredoPortSetup_t port_setup;
    int option_cnt;
    bool started;
    sap_credo_port_option_t port_option[] = {
        {
            .cr_option = {
                .name = "line_lt",
            },
            .option_value = -1
        },
        {
            .cr_option = {
                .name = "host_lt",
            },
            .option_value = -1
        }
    };

    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_CR_LOG_ERR("sap_credo_get_port_info failed, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE;
    }
    slice_info = port_info->slice_info;
    rv = sap_credo_query_port_setup(port_info, if_side, SAP_CHANNEL_BOTH, &port_setup, &started);
    SAP_RV_CHECK(slice_info, "sap_credo_query_port_setup", rv);
    if (started == false) {
        SAP_CR_LOG_WARN("port not started, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE; 
    }
    if (port_info->Uni_Directional == true) {
        SAP_CR_LOG_WARN("Uni_Directional port not support an/lt, unit:%d, port:%d", unit, port);
        return SAP_STATUS_NOT_SUPPORTED;
    }
    option_cnt = sizeof(port_option) / sizeof(sap_credo_port_option_t);
    sap_credo_port_option_save(slice_info, port_info->port_id, port_option, option_cnt);
    SAP_CR_RV_CHECK(slice_info, "cr_port_destroy", cr_port_destroy(slice_info->cr_slice, port_info->port_id));
    SAP_CR_RV_CHECK(slice_info, "cr_port_build", cr_port_build(slice_info->cr_slice, port_info->port_id, &port_setup));
    sap_credo_port_option_restore(slice_info, port_info->port_id, port_option, option_cnt);

    if (if_side == SAP_PORT_SIDE_LINE) {
        SAP_CR_RV_CHECK(slice_info, "cr_port_set_option", cr_port_set_option(slice_info->cr_slice, port_info->port_id, "line_lt", enable));
    } else {
        SAP_CR_RV_CHECK(slice_info, "cr_port_set_option", cr_port_set_option(slice_info->cr_slice, port_info->port_id, "host_lt", enable));
    }
    rv = cr_port_start(slice_info->cr_slice, port_info->port_id, CR_NOT_FORCE_PORT_CONFIG);
    SAP_CR_RV_CHECK(slice_info, "cr_port_start", rv);
    return SAP_STATUS_SUCCESS;  
}

static int sap_credo_port_status_get(int unit, int port, sap_port_status_t *port_status)
{
    int i,rv;
    sap_credo_port_info_t *port_info;
    sap_slice_info_t *slice_info;
    CredoPortSetup_t port_setup;
    bool started;

    SAP_POINT_CHECK_RV(port_status, "port_status", SAP_STATUS_INVALID_PARAMETER);

    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_CR_LOG_ERR("sap_credo_get_port_info failed, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE;
    }
    slice_info = port_info->slice_info;

    rv = sap_credo_query_port_setup(port_info, SAP_PORT_SIDE_LINE, SAP_CHANNEL_RX, &port_setup, &started);
    SAP_RV_CHECK(slice_info, "sap_credo_query_port_setup", rv);
    if (started == false) {
        port_status->line_admin = false;
        port_status->line_link_up = false;
    } else {
        port_status->line_admin = true;
        SAP_CR_RV_CHECK(slice_info, "cr_port_is_link_up", cr_port_is_link_up(slice_info->cr_slice, port_info->port_id, CR_SIDE_LINE, &port_status->line_link_up));
        port_status->speed = port_setup.speed;
        port_status->line_lane_num = port_setup.line_count;
    }
    
    rv = sap_credo_query_port_setup(port_info, SAP_PORT_SIDE_SYS, SAP_CHANNEL_RX, &port_setup, &started);
    SAP_RV_CHECK(slice_info, "sap_credo_query_port_setup", rv);
    if (started == false) {
        port_status->host_admin = false;
        port_status->host_link_up = false;
    } else {
        port_status->host_admin = true;
        SAP_CR_RV_CHECK(slice_info, "cr_port_is_link_up", cr_port_is_link_up(slice_info->cr_slice, port_info->port_id, CR_SIDE_HOST, &port_status->host_link_up));
        port_status->speed = port_setup.speed;
        port_status->host_lane_num = port_setup.host_count;
    }

    return SAP_STATUS_SUCCESS;  
}

static int sap_credo_tx_taps_set(int unit, int port, int if_side, int lane, sap_tx_fir_t tx_fir)
{
    int i,rv;
    int lane_id;
    sap_credo_port_info_t *port_info;
    sap_slice_info_t *slice_info;
    CredoPortSetup_t port_setup;
    int count;
    int lane_num;
    const int *lanes;
    bool started;
    int tx_taps[7] = {
        0,
        tx_fir.pre2,
        tx_fir.pre,
        tx_fir.main,
        tx_fir.post,
        tx_fir.post2,
        tx_fir.post3
    };
    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_CR_LOG_ERR("sap_credo_get_port_info failed, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE;
    }
    slice_info = port_info->slice_info;
    rv = sap_credo_query_port_setup(port_info, if_side, SAP_CHANNEL_TX, &port_setup, &started);
    SAP_RV_CHECK(slice_info, "sap_credo_query_port_setup", rv);
    if (started == false) {
        SAP_CR_LOG_WARN("port not started, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE; 
    }
    if (if_side == SAP_PORT_SIDE_LINE) {
        count = port_setup.line_count;
        lanes = port_setup.line_lanes;
    } else if (if_side == SAP_PORT_SIDE_SYS) {
        count = port_setup.host_count;
        lanes = port_setup.host_lanes;
    } else {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if (lane == SAP_LANE_ID_ALL) {
        lane_num = count;
    } else {
        if (((lane - SAP_LANE_ID_START) >= count) || (lane < SAP_LANE_ID_START)) {
            SAP_CR_LOG_ERR("lane out of range, unit:%d, port:%d, lane:%d", unit, port, lane);
            return SAP_STATUS_INVALID_PARAMETER;
        }
        lane_num = 1;
    }
    for (i = 0; i < lane_num; i++) {
        if (lane == SAP_LANE_ID_ALL) {
            lane_id = lanes[i];
        } else {
            lane_id = lanes[lane - SAP_LANE_ID_START];
        }
        rv = cr_serdes_set_tx_taps(slice_info->cr_slice, lane_id, tx_taps);
        SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_tx_taps", rv);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_credo_tx_taps_get(int unit, int port, int if_side, int lane, sap_tx_fir_t *tx_fir)
{
    int i,rv;
    int lane_id;
    sap_credo_port_info_t *port_info;
    sap_slice_info_t *slice_info;
    CredoPortSetup_t port_setup;
    int count;
    const int *lanes;
    bool started;
    int tx_taps[7] = {0};
    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_CR_LOG_ERR("sap_credo_get_port_info failed, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE;
    }
    slice_info = port_info->slice_info;
    rv = sap_credo_query_port_setup(port_info, if_side, SAP_CHANNEL_TX, &port_setup, &started);
    SAP_RV_CHECK(slice_info, "sap_credo_query_port_setup", rv);
    if (started == false) {
        SAP_CR_LOG_WARN("port not started, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE; 
    }
    if (if_side == SAP_PORT_SIDE_LINE) {
        count = port_setup.line_count;
        lanes = port_setup.line_lanes;
    } else if (if_side == SAP_PORT_SIDE_SYS) {
        count = port_setup.host_count;
        lanes = port_setup.host_lanes;
    } else {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if (((lane - SAP_LANE_ID_START) >= count) || (lane < SAP_LANE_ID_START)) {
        SAP_CR_LOG_ERR("lane out of range, unit:%d, port:%d, lane:%d", unit, port, lane);
        return SAP_STATUS_INVALID_PARAMETER;
    }
    lane_id = lanes[lane - SAP_LANE_ID_START];
    rv = cr_serdes_get_tx_taps(slice_info->cr_slice, lane_id, tx_taps);
    SAP_CR_RV_CHECK(slice_info, "cr_serdes_set_tx_taps", rv);
    tx_fir->pre2 = tx_taps[1];
    tx_fir->pre = tx_taps[2];
    tx_fir->main = tx_taps[3];
    tx_fir->post = tx_taps[4];
    tx_fir->post2 = tx_taps[5];
    tx_fir->post3 = tx_taps[6];
    return SAP_STATUS_SUCCESS;
}

static void sap_credo_techsupport(int unit, int port)
{

}

static void processDsc(const char* rx_param, int count, int type, CredoParamDataBuf_t values) {
    int k;
    char result[4096] = {0}; /* 用来存储拼接后的字符串 */
    char temp[128]; /* 临时存储每个数值的字符串表示 */
    strcat(result, "(");
    for (k = 0; k < count; k++) {
        switch (type) {
            case CR_PARAM_VAL_INT:
                snprintf(temp, sizeof(temp), "%d", values.i[k]);
                break;
            case CR_PARAM_VAL_UINT:
                snprintf(temp, sizeof(temp), "%u", values.u[k]);
                break;
            case CR_PARAM_VAL_FLOAT:
                snprintf(temp, sizeof(temp), "%f", values.d[k]);
                break;
            default:
                continue;
        }
        if (k > 0) {
            strcat(result, ", ");
        }
        strcat(result, temp);
    }
    strcat(result, ")");
    SAP_CLI_LOG_NO_LF("%-32s", result);
}

static int sap_credo_dsc_dump(int unit, int port, int if_side, int flag, int lane)
{
    int i,j,k,rv;
    int lane_id;
    sap_credo_port_info_t *port_info;
    sap_slice_info_t *slice_info;
    CredoPortSetup_t port_setup;
    int lane_num;
    int count;
    const int *lanes;
    bool started;
    CredoParamData_t data;
    rv = sap_credo_get_port_info(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_CR_LOG_ERR("sap_credo_get_port_info failed, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE;
    }
    slice_info = port_info->slice_info;
    rv = sap_credo_query_port_setup(port_info, if_side, SAP_CHANNEL_RX, &port_setup, &started);
    SAP_RV_CHECK(slice_info, "sap_credo_query_port_setup", rv);
    if (started == false) {
        SAP_CR_LOG_WARN("port not started, unit:%d, port:%d", unit, port);
        return SAP_STATUS_FAILURE; 
    }
    if (if_side == SAP_PORT_SIDE_LINE) {
        count = port_setup.line_count;
        lanes = port_setup.line_lanes;
    } else if (if_side == SAP_PORT_SIDE_SYS) {
        count = port_setup.host_count;
        lanes = port_setup.host_lanes;
    } else {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if (lane == SAP_LANE_ID_ALL) {
        lane_num = count;
    } else {
        if (((lane - SAP_LANE_ID_START) >= count) || (lane < SAP_LANE_ID_START)) {
            SAP_CR_LOG_ERR("lane out of range, unit:%d, port:%d, lane:%d", unit, port, lane);
            return SAP_STATUS_INVALID_PARAMETER;
        }
        lane_num = 1;
    }
    const char *rx_params[] = {
        "rx_snr",
        "rx_channel_est",
        "tx_taps",
        "rx_eye_height"
    };
    SAP_CLI_LOG("unit:%d, port:%d", unit, port);
    SAP_CLI_LOG_NO_LF("%-8s", "lane");
    for (j = 0; j<sizeof(rx_params)/sizeof(rx_params[0]); j++) {
        SAP_CLI_LOG_NO_LF("%-32s", rx_params[j]);
    }
    SAP_CLI_LOG("");
    for (i = 0; i < lane_num; i++) {
        if (lane == SAP_LANE_ID_ALL) {
            lane_id = lanes[i];
        } else {
            lane_id = lanes[lane - SAP_LANE_ID_START];
        }
        SAP_CLI_LOG_NO_LF("%-8d", i);
        for (j = 0; j<sizeof(rx_params)/sizeof(rx_params[0]); j++) {
            rv = cr_serdes_get_paramh(slice_info->cr_slice, rx_params[j], lane_id, &data);
            SAP_CR_RV_CHECK(slice_info, "cr_serdes_get_param", rv);
            processDsc(rx_params[j], data.count, data.type, data.value);
            CR_PDATA_FREE(data);
        }
        SAP_CLI_LOG("");
    }
    return SAP_STATUS_SUCCESS;
}
