#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <esmi_oob/esmi_mailbox.h>


#define UMC_ID_MAX                      12
#define ONE_UMC_DIMM_NUM_MAX            2
#define ONE_DIMM_TEMP_SENSOR_NUM_MAX    2
#define SCALING_FACTOR                  0.25
#define CPU_BOARD_STATUS_PATH           "/sys/s3ip/system/cpu_board_status"
#define CPU_POWER_ON_STATUS             2
#define INVALID_CPU_STATUS              0x5a5aa5a5
#define INVALID_MAX_TEMP                -1000.0f
#define ALL_DIMMS_TEMP_FILE             "/tmp/.all_dimm_temp"
#define DIMMS_MAX_TEMP_FILE             "/tmp/.dimms_max_temp"
#define DIMMS_MAX_TEMP_NAME_FILE        "/tmp/.dimms_max_temp_name"
#define INFO_NAME_LEN                   16

#define READ_DIMM_TEMP_PERIOD           5


typedef union dimm_addr_un {
    uint8_t addr;
    struct {
        uint8_t umc_id: 4;
        uint8_t dimm_id: 1;
        uint8_t reserve: 1;
        uint8_t sensor_id: 1;
        uint8_t model: 1;
    } addr_st;
} dimm_addr_t;

/******* begin: 日志打印 **********/
#define LOG_LEVEL_FILE     "/tmp/.apml_dimm_temp_log_level"

#define LOG_LEVEL_NONE    0
#define LOG_LEVEL_VERBOSE 1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_WARN    4
#define LOG_LEVEL_ERR     8

static int g_log_level = LOG_LEVEL_NONE;

#define DEBUG_VERBOSE(fmt, args...) do {              \
    if (g_log_level & LOG_LEVEL_VERBOSE) {               \
        printf("[%s]:<File:%s, Func:%s, Line:%d>\n" fmt, "ver", \
            __FILE__, __FUNCTION__, __LINE__, ##args);  \
    }                                                   \
} while(0)

#define DEBUG_INFO(fmt, args...) do {              \
    if (g_log_level & LOG_LEVEL_INFO) {               \
        printf("[%s]:<File:%s, Func:%s, Line:%d>\n" fmt, "info", \
            __FILE__, __FUNCTION__, __LINE__, ##args);  \
    }                                                   \
} while(0)

#define DEBUG_WARN(fmt, args...) do {              \
    if (g_log_level & LOG_LEVEL_WARN) {               \
        printf("[%s]:<File:%s, Func:%s, Line:%d>\n" fmt, "warn", \
            __FILE__, __FUNCTION__, __LINE__, ##args);  \
    }                                                   \
} while(0)

#define DEBUG_ERROR(fmt, args...) do {              \
    if (g_log_level & LOG_LEVEL_ERR) {               \
        printf("[%s]:<File:%s, Func:%s, Line:%d>\n" fmt, "err", \
            __FILE__, __FUNCTION__, __LINE__, ##args);  \
    }                                                   \
} while(0)

/* ss3.0 硬件内存条的槽位号信息, 可通过板卡id区分 */
const char *slot_info_tab[UMC_ID_MAX] = {"DIMMC", "DIMME", "DIMMF", "DIMMA", "DIMMB", "DIMMD",
                                         "DIMMI", "DIMMK", "DIMML", "DIMMG", "DIMMH", "DIMMJ"};


static int get_log_level(void)
{
    int level = 0;

    FILE *fp = fopen(LOG_LEVEL_FILE, "r");
    if (!fp) {
        return LOG_LEVEL_NONE;
    }

    if (fscanf(fp, "%d", &level) != 1) {
        level = LOG_LEVEL_NONE;
    }
    fclose(fp);
    return level;
}

/******* end: 日志打印 **********/

static int read_cpu_board_status(const char *path)
{
    char buf[16];
    char *endptr, *newline;
    long value;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        DEBUG_ERROR("Failed to open cpu_board_status\n");
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    if (!fgets(buf, sizeof(buf), fp)) {
        DEBUG_ERROR("Failed to read cpu_board_status\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);

    newline = strchr(buf, '\n');
    if (newline) {
        *newline = '\0';
    }

    errno = 0;
    value = strtol(buf, &endptr, 10);
    if (errno == ERANGE) {
        DEBUG_ERROR("digval out of range\n");
        return INVALID_CPU_STATUS;
    } else if (*endptr != '\0') {
        DEBUG_ERROR("include non digatal char\n");
        return INVALID_CPU_STATUS;
    }

    return (int)value;
}

static void decode_dimm_temp(uint16_t raw, float *temp)
{
    if (raw <= 0x3FF) {
        *temp = raw * SCALING_FACTOR;
    } else {
        *temp = (raw - 0x800) * SCALING_FACTOR;
    }

    return;
}

static oob_status_t apml_get_dimm_temp(uint8_t soc_num, dimm_addr_t dimm_addr, float *p_temp)
{
    oob_status_t ret;
    struct dimm_thermal d_sensor;

    if (p_temp == NULL) {
        DEBUG_ERROR("p_temp null\n");
        return OOB_INVALID_INPUT;
    }

    if (dimm_addr.addr_st.umc_id >= UMC_ID_MAX) {
        DEBUG_ERROR("invaild umc_id:%d\n", dimm_addr.addr_st.umc_id);
        return OOB_INVALID_INPUT;
    }
    dimm_addr.addr_st.model = 1;

    memset(&d_sensor, 0, sizeof(d_sensor));
    ret = read_dimm_thermal_sensor(soc_num, dimm_addr.addr, &d_sensor);
    DEBUG_INFO("ret:%d, dimm_addr:0x%x, d_sensor---dimm_addr:0x%x, update_rate:0x%x, sensor:0x%x\n",
              ret, dimm_addr.addr, d_sensor.dimm_addr, d_sensor.update_rate, d_sensor.sensor);
    if (ret != OOB_SUCCESS) {
        DEBUG_ERROR("Failed to get dimm temp, Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
        return ret;
    }
    decode_dimm_temp(d_sensor.sensor, p_temp);

    return OOB_SUCCESS;
}

static int write_temps_to_file(uint8_t soc_num, const char *all_temp_path,
                                        const char *max_temp_path, const char *max_temp_name_path)
{
    char tmp_all[256];
    char tmp_max[256];
    char tmp_max_name[256];
    FILE *fp_all;
    FILE *fp_max;
    FILE *fp_max_name;
    float temp;
    float max_temp;
    oob_status_t ret;
    dimm_addr_t dimm_addr;
    uint8_t umc, dimm, sensor;
    char slot_info[INFO_NAME_LEN], max_temp_name[INFO_NAME_LEN];

    if (all_temp_path == NULL || max_temp_path == NULL || max_temp_name_path == NULL) {
        DEBUG_ERROR("null point: %p,%p,%p\n", all_temp_path, max_temp_path, max_temp_name_path);
        return -1;
    }

    /* 1. 临时文件路径初始化 */
    memset(tmp_all, 0, sizeof(tmp_all));
    memset(tmp_max, 0, sizeof(tmp_max));
    memset(tmp_max_name, 0, sizeof(tmp_max_name));
    snprintf(tmp_all, sizeof(tmp_all), "%s.tmp", all_temp_path);
    snprintf(tmp_max, sizeof(tmp_max), "%s.tmp", max_temp_path);
    snprintf(tmp_max_name, sizeof(tmp_max_name), "%s.tmp", max_temp_name_path);

    /* 2. 只写的方式打开临时文件，清除文件的内容 */
    fp_all = fopen(tmp_all, "w");
    if (!fp_all) {
        DEBUG_ERROR("Failed to open temporary all temp file\n");
        return -1;
    }
    fp_max = fopen(tmp_max, "w");
    if (!fp_max) {
        DEBUG_ERROR("Failed to open temporary max temp file\n");
        fclose(fp_all);
        return -1;
    }
    fp_max_name = fopen(tmp_max_name, "w");
    if (!fp_max_name) {
        DEBUG_ERROR("Failed to open temporary max temp name file\n");
        fclose(fp_max);
        fclose(fp_all);
        return -1;
    }

    /* 3. 写入表头 */
    fprintf(fp_all, "%-10s %-7s %-8s %-10s %-6s\n", "slot_info", "umcid", "dimm_id", "sensor_id", "temp");

    /* 4. 读取温度并写入文件 */
    max_temp = INVALID_MAX_TEMP;
    memset(max_temp_name, 0, sizeof(max_temp_name));
    for (umc = 0; umc < UMC_ID_MAX; umc++) {
        for (dimm = 0; dimm < ONE_UMC_DIMM_NUM_MAX; dimm++) {
            for (sensor = 0; sensor < ONE_DIMM_TEMP_SENSOR_NUM_MAX; sensor++) {
                memset(&dimm_addr, 0, sizeof(dimm_addr));
                dimm_addr.addr_st.umc_id = umc;
                dimm_addr.addr_st.dimm_id = dimm;
                dimm_addr.addr_st.sensor_id = sensor;

                temp = 0.0f;
                ret = apml_get_dimm_temp(soc_num, dimm_addr, &temp);
                DEBUG_INFO("ret:%d, temp:%-6.3f\n", ret, temp);
                if (ret == OOB_SUCCESS) {
                    memset(slot_info, 0, sizeof(slot_info));
                    snprintf(slot_info, sizeof(slot_info), "%s%u", slot_info_tab[umc], dimm);

                    fprintf(fp_all, "%-10s %-7u %-8u %-10u %-6.3f\n", slot_info, umc, dimm, sensor, temp);
                    if (temp > max_temp) {
                        max_temp = temp;
                        memcpy(max_temp_name, slot_info, sizeof(max_temp_name));
                    }
                }
            }
        }
    }

    if (max_temp > INVALID_MAX_TEMP) {
        fprintf(fp_max, "%d\n", (int)(max_temp * 1000));
        fprintf(fp_max_name, "%s", max_temp_name);
    }

    /* 5. 关文件 */
    fclose(fp_all);
    fclose(fp_max);
    fclose(fp_max_name);

    /* 6. 原子重命名临时文件到目标文件，保证写文件期间外部读进程不会读到半写数据 */
    if (rename(tmp_all, all_temp_path) != 0) {
        DEBUG_ERROR("Failed to rename all temp file\n");
        return -1;
    }
    if (rename(tmp_max, max_temp_path) != 0) {
        DEBUG_ERROR("Failed to rename max temp file\n");
        return -1;
    }
    if (rename(tmp_max_name, max_temp_name_path) != 0) {
        DEBUG_ERROR("Failed to rename max temp name file\n");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int cpu_status;

    while (1) {
        g_log_level = get_log_level();
        cpu_status = read_cpu_board_status(CPU_BOARD_STATUS_PATH);
        DEBUG_INFO("g_log_level:0x%x, cpu_status:%d\n", g_log_level, cpu_status);

        if (cpu_status == CPU_POWER_ON_STATUS) {
            if (write_temps_to_file(0, ALL_DIMMS_TEMP_FILE, 
                DIMMS_MAX_TEMP_FILE, DIMMS_MAX_TEMP_NAME_FILE) != 0) {
                DEBUG_ERROR("Error writing temperature files\n");
            }
        }
        sleep(READ_DIMM_TEMP_PERIOD);
    }

    return 0;
}
