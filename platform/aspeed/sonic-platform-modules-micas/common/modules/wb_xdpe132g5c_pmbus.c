#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hwmon-sysfs.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include "wb_pmbus.h"
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define BUF_SIZE                    (256)
#define XDPE132G5C_PAGE_NUM         (2)
#define XDPE132G5C_PROT_VR12_5MV    (0x01) /* VR12.0 mode, 5-mV DAC */
#define XDPE132G5C_PROT_VR12_5_10MV (0x02) /* VR12.5 mode, 10-mV DAC */
#define XDPE132G5C_PROT_IMVP9_10MV  (0x03) /* IMVP9 mode, 10-mV DAC */
#define XDPE132G5C_PROT_VR13_10MV   (0x04) /* VR13.0 mode, 10-mV DAC */
#define XDPE132G5C_PROT_IMVP8_5MV   (0x05) /* IMVP8 mode, 5-mV DAC */
#define XDPE132G5C_PROT_VR13_5MV    (0x07) /* VR13.0 mode, 5-mV DAC */
#define RETRY_TIME                  (15)
#define XDPE132_VERSION1_REG        (0xB0)
#define XDPE132_VERSION2_REG        (0xB1)

/*************** FW_UPG register define ****************/
#define XDPE1X2XX_RPTR_CE                         (0xCE)
#define XDPE1X2XX_RPTR_FD                         (0xFD)
#define XDPE1X2XX_MFR_REG_WRITE                   (0xDE)
#define XDPE1X2XX_MFR_REG_READ                    (0xDF)
#define XDPE1X2XX_MFR_FW_COMMAND                  (0xFE)
#define XDPE1X2XX_MFR_FW_COMMAND_DATA             (0xFD)

#define XDPE1X2XX_MAX_OFFSET                      (0xFFFFFFFF)

/*************** MFR_FW_COMMAND define ****************/
#define XDPE1X2XX_CMD_FW_VERSION                   (0x01)
#define XDPE1X2XX_CMD_STORE_CONFIG                 (0x04)
#define XDPE1X2XX_CMD_FW_RESET                     (0x0E)
#define XDPE1X2XX_CMD_OTP_REMAIN_SIZE              (0x10)
#define XDPE1X2XX_CMD_OTP_CONFIG_STORE             (0x11)
#define XDPE1X2XX_CMD_OTP_SECTION_INVALIDATE       (0x12)
#define XDPE1X2XX_CMD_STORE_PARTIAL_CONFIG         (0x14)
#define XDPE1X2XX_CMD_FW_UPDATE_HSIF               (0x1D)
#define XDPE1X2XX_CMD_STORE_HSIF_CODE              (0x1E)
#define XDPE1X2XX_CMD_STORE_HSIF_ALL               (0x1F)
#define XDPE1X2XX_CMD_WRITE_HSIF_CODE              (0x20)
#define XDPE1X2XX_CMD_READ_HSIF_CODE               (0x21)
#define XDPE1X2XX_CMD_GET_CRC                      (0x2D)
#define XDPE1X2XX_CMD_GET_FW_ADDRESS               (0x2E)

#define XDPE1X2XX_CMD_GET_CRC_DELAY                (20000)   /* 20ms */
#define XDPE1X2XX_CMD_GET_REMAIN_SPACE_DELAY       (1000)    /* 1ms */
#define XDPE1X2XX_CMD_OTP_SECTION_INVALIDATE_DELAY (40000)   /* 40ms */

#define XDPE1X2XX_PHASE_CURR_DATA_LOW                 (1)
#define XDPE1X2XX_PHASE_CURR_DATA_HIGH                (2)
#define XDPE1X2XX_PHASE_CURR_DATA_HIGH_BIT_OFFSET     (14)
#define XDPE1X2XX_PHASE1_REG_VALUE                    (0x70007460)
#define XDPE1X2XX_PHASE2_REG_VALUE                    (0x70007460)
#define XDPE1X2XX_PHASE3_REG_VALUE                    (0x70007464)
#define XDPE1X2XX_PHASE4_REG_VALUE                    (0x70007464)
#define XDPE1X2XX_PHASE5_REG_VALUE                    (0x70007468)
#define XDPE1X2XX_PHASE6_REG_VALUE                    (0x70007468)
#define XDPE1X2XX_PHASE7_REG_VALUE                    (0x7000746C)
#define XDPE1X2XX_PHASE8_REG_VALUE                    (0x7000746C)
#define XDPE1X2XX_PHASE9_REG_VALUE                    (0x70007470)
#define XDPE1X2XX_PHASE10_REG_VALUE                   (0x70007470)
#define XDPE1X2XX_PHASE11_REG_VALUE                   (0x70007474)
#define XDPE1X2XX_PHASE12_REG_VALUE                   (0x70007474)
#define XDPE1X2XX_PHASE13_REG_VALUE                   (0x70007478)
#define XDPE1X2XX_PHASE14_REG_VALUE                   (0x70007478)
#define XDPE1X2XX_PHASE15_REG_VALUE                   (0x7000747C)
#define XDPE1X2XX_PHASE16_REG_VALUE                   (0x7000747C)

#define XDPE1X2XX_PHASE_BLOCK_LEN                     (4)
#define XDPE1X2XX_PHASE_CURR_DATA_TO_VALUE(reg_val)   (((reg_val) & 0x3fff)* 21875 / 1000) /* mA */ 

/*****XDPE1X2XX IC_DEVICE_ID define ******/
#define XDPE15254_REV_C    (0x9002)
#define XDPE15254_REV_D    (0x9003)
#define XDPE15284_REV_A    (0x8A00)
#define XDPE15284_REV_B    (0x8A01)
#define XDPE15284_REV_C    (0x8A02)
#define XDPE15284_REV_D    (0x8A03)
#define XDPE152C4_REV_A    (0x8C00)
#define XDPE152C4_REV_B    (0x8C01)
#define XDPE152C4_REV_C    (0x8C02)
#define XDPE152C4_REV_D    (0x8C03)
#define XDPE19283_REV_A    (0x9500)
#define XDPE19283_REV_B    (0x9501)
#define XDPE19283_REV_C    (0x9502)
#define XDPE192C3_REV_A    (0x9600)
#define XDPE192C3_REV_B    (0x9601)
#define XDPE192C3_REV_C    (0x9602)
#define XDPE192A3_REV_A    (0x9700)
#define XDPE192A3_REV_B    (0x9701)
#define XDPE192A3_REV_C    (0x9702)
#define XDPE19284_REV_A    (0x9800)
#define XDPE19284_REV_B    (0x9801)
#define XDPE19284_REV_C    (0x9802)
#define XDPE192C4_REV_A    (0x9900)
#define XDPE192C4_REV_B    (0x9901)
#define XDPE192C4_REV_C    (0x9902)
#define XDPE1A2G3_REV_A    (0x9C00)
#define XDPE1A2G3_REV_B    (0x9C01)
#define XDPE1A2G4_REV_A    (0x9A00)
#define XDPE1A2G4_REV_B    (0x9A01)
#define XDPE1A2G5_REV_A    (0x9E00)
#define XDPE1A2G5_REV_B    (0x9E01)
#define XDPE1A2G7_REV_A    (0x9B00)
#define XDPE1A2G7_REV_B    (0x9B01)
#define XDPE1B250_REV_A    (0XA000)
#define XDPE1B254_REV_A    (0xA100)
#define XDPE1B258_REV_A    (0xA200)
#define XDPE1B284_REV_A    (0xA300)
#define XDPE1B2C4_REV_A    (0xA400)

typedef struct xdpe1x2xx_phase_curr_reg_value_s {
    u8 phase_index;
    u8 data_index;
    int phase_curr_reg_value;
} xdpe1x2xx_phase_curr_reg_value_t;

#define BLACKBOX_HEADER             (0x0014)
#define BLACKBOX_HEADER_END         (0x0000)
#define BLACKBOX_MAX_READ_COUNT     (10)
#define XDPE1A2G5_BLACKBOX_START    (0x1002F800)

enum chips {
    XDPE132G5C,
    XDPE1A2G5B,
    XDPE19284C,
    XDPE192C4B,
};

typedef enum {
    CHIP_ID_FAIL = 0,    /* Get ic_device_id failed */
    CHIP_ID_UNKNOWN,     /* Get ic_device_id success, but unsupport to upgrade this chip id */
    CHIP_ID_OK,          /* Get ic_device_id success and support fw upgrade */
} chip_id_status_t;

typedef struct {
    uint32_t ic_device_id;
    uint32_t scap_addr;
    uint8_t rptr_reg_addr;
    bool support_blackbox;
    uint32_t blackbox_start_addr;
    chip_id_status_t chip_id_status;
} xdpe_chip_info_t;


typedef struct {
    enum chips id;
    struct pmbus_driver_info info;
    u32 vout_multiplier[2];
    xdpe_chip_info_t xdpe_chip_info;
    struct i2c_client *client;
    struct miscdevice misc_dev;
    char misc_dev_name[DEV_NAME_LEN];
} xdpe_chip_data_t;
#define to_xdpe_chip_data(x) container_of(x, xdpe_chip_data_t, info)

static DEFINE_SPINLOCK(dev_array_lock);
static xdpe_chip_data_t* xdpe_dev_arry[MAX_DEV_NUM];

static const xdpe_chip_info_t xdpe1x2xx_device_infos[] = {
    {.ic_device_id = XDPE15254_REV_C, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE15254_REV_D, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE15284_REV_A, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_FD, .support_blackbox = 0,},
    {.ic_device_id = XDPE15284_REV_B, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_FD, .support_blackbox = 0,},
    {.ic_device_id = XDPE15284_REV_C, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE15284_REV_D, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE152C4_REV_A, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_FD, .support_blackbox = 0,},
    {.ic_device_id = XDPE152C4_REV_B, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_FD, .support_blackbox = 0,},
    {.ic_device_id = XDPE152C4_REV_C, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE152C4_REV_D, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE19283_REV_A, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE19283_REV_B, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE19283_REV_C, .scap_addr = 0x2005e400, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE192C3_REV_A, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE192C3_REV_B, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE192C3_REV_C, .scap_addr = 0x2005e400, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE192A3_REV_A, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE192A3_REV_B, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE192A3_REV_C, .scap_addr = 0x2005e400, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE19284_REV_A, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE19284_REV_B, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE19284_REV_C, .scap_addr = 0x2005e400, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE192C4_REV_A, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE192C4_REV_B, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE192C4_REV_C, .scap_addr = 0x2005e400, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE1A2G3_REV_A, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 1, .blackbox_start_addr = XDPE1A2G5_BLACKBOX_START},
    {.ic_device_id = XDPE1A2G3_REV_B, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 1, .blackbox_start_addr = XDPE1A2G5_BLACKBOX_START},
    {.ic_device_id = XDPE1A2G4_REV_A, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 1, .blackbox_start_addr = XDPE1A2G5_BLACKBOX_START},
    {.ic_device_id = XDPE1A2G4_REV_B, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 1, .blackbox_start_addr = XDPE1A2G5_BLACKBOX_START},
    {.ic_device_id = XDPE1A2G5_REV_A, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 1, .blackbox_start_addr = XDPE1A2G5_BLACKBOX_START},
    {.ic_device_id = XDPE1A2G5_REV_B, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 1, .blackbox_start_addr = XDPE1A2G5_BLACKBOX_START},
    {.ic_device_id = XDPE1A2G7_REV_A, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 1, .blackbox_start_addr = XDPE1A2G5_BLACKBOX_START},
    {.ic_device_id = XDPE1A2G7_REV_B, .scap_addr = 0x2005e000, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 1, .blackbox_start_addr = XDPE1A2G5_BLACKBOX_START},
    {.ic_device_id = XDPE1B250_REV_A, .scap_addr = 0x2005d400, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE1B254_REV_A, .scap_addr = 0x2005d400, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE1B258_REV_A, .scap_addr = 0x2005d400, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE1B284_REV_A, .scap_addr = 0x2005d400, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
    {.ic_device_id = XDPE1B2C4_REV_A, .scap_addr = 0x2005d400, .rptr_reg_addr = XDPE1X2XX_RPTR_CE, .support_blackbox = 0,},
};

static xdpe1x2xx_phase_curr_reg_value_t g_xdpe1x2xx_phase_curr_reg_value_group[] = {
    {.phase_index = 1, .phase_curr_reg_value = XDPE1X2XX_PHASE1_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_LOW},
    {.phase_index = 2, .phase_curr_reg_value = XDPE1X2XX_PHASE2_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_HIGH},
    {.phase_index = 3, .phase_curr_reg_value = XDPE1X2XX_PHASE3_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_LOW},
    {.phase_index = 4, .phase_curr_reg_value = XDPE1X2XX_PHASE4_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_HIGH},
    {.phase_index = 5, .phase_curr_reg_value = XDPE1X2XX_PHASE5_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_LOW},
    {.phase_index = 6, .phase_curr_reg_value = XDPE1X2XX_PHASE6_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_HIGH},
    {.phase_index = 7, .phase_curr_reg_value = XDPE1X2XX_PHASE7_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_LOW},
    {.phase_index = 8, .phase_curr_reg_value = XDPE1X2XX_PHASE8_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_HIGH},
    {.phase_index = 9, .phase_curr_reg_value = XDPE1X2XX_PHASE9_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_LOW},
    {.phase_index = 10, .phase_curr_reg_value = XDPE1X2XX_PHASE10_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_HIGH},
    {.phase_index = 11, .phase_curr_reg_value = XDPE1X2XX_PHASE11_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_LOW},
    {.phase_index = 12, .phase_curr_reg_value = XDPE1X2XX_PHASE12_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_HIGH},
    {.phase_index = 13, .phase_curr_reg_value = XDPE1X2XX_PHASE13_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_LOW},
    {.phase_index = 14, .phase_curr_reg_value = XDPE1X2XX_PHASE14_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_HIGH},
    {.phase_index = 15, .phase_curr_reg_value = XDPE1X2XX_PHASE15_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_LOW},
    {.phase_index = 16, .phase_curr_reg_value = XDPE1X2XX_PHASE16_REG_VALUE, .data_index = XDPE1X2XX_PHASE_CURR_DATA_HIGH},
};

static pmbus_info_t dfx_infos[] = {
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_BYTE", .pmbus_reg = PMBUS_STATUS_BYTE, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_WORD", .pmbus_reg = PMBUS_STATUS_WORD, .width = WORD_DATA, .data_type = RAWDATA_WORD},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_VOUT", .pmbus_reg = PMBUS_STATUS_VOUT, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_IOUT", .pmbus_reg = PMBUS_STATUS_IOUT, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_INPUT", .pmbus_reg = PMBUS_STATUS_INPUT, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_TEMP", .pmbus_reg = PMBUS_STATUS_TEMPERATURE, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_CML", .pmbus_reg = PMBUS_STATUS_CML, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_MFR_SPEC", .pmbus_reg = PMBUS_STATUS_MFR_SPECIFIC, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_VIN", .pmbus_reg = PMBUS_READ_VIN, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_IIN", .pmbus_reg = PMBUS_READ_IIN, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_VOUT", .pmbus_reg = PMBUS_READ_VOUT, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_IOUT", .pmbus_reg = PMBUS_READ_IOUT, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_TEMP1", .pmbus_reg = PMBUS_READ_TEMPERATURE_1, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_POUT", .pmbus_reg = PMBUS_READ_POUT, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_PIN", .pmbus_reg = PMBUS_READ_PIN, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_DUTY_CYCLE", .pmbus_reg = PMBUS_READ_DUTY_CYCLE, .width = WORD_DATA, .data_type = RAWDATA_WORD},
};

typedef struct xdpe_vout_data_s {
    u8 vout_mode;
    int vout_precision;
} xdpe_vout_data_t;

static int xdpe_read_device_id(struct i2c_client *client, uint32_t *device_id)
{
    int i, block_size;
    uint32_t tmp_id;
    u8 block_data[I2C_SMBUS_BLOCK_MAX + 2] = { 0 };

    block_size = wb_pmbus_read_block_data(client, 0, PMBUS_IC_DEVICE_ID, block_data);
    if (block_size < 0) {
        DEBUG_INFO("%d-%04x: read renesas ic_device_id failed, ret: %d\n",
            client->adapter->nr, client->addr, block_size);
        return block_size;
    }

    if ((block_size == 0) || (block_size > WIDTH_4Byte)) {
        DEBUG_INFO("%d-%04x: Invalid ic_device_id len %d\n",
            client->adapter->nr, client->addr, block_size);
        return -EINVAL;
    }

    tmp_id = 0;
    for (i = 0; i < block_size; i++) {
        tmp_id |= block_data[i] << (8 * i);
    }

    *device_id = tmp_id;
    DEBUG_VERBOSE("%d-%04x: xdpe ic_device_id: 0x%08x\n",
        client->adapter->nr, client->addr, *device_id);
    return 0;
}

static void xdpe_chip_id_init(xdpe_chip_data_t *chip_data)
{
    uint32_t device_id;
    struct i2c_client *client;
    struct pmbus_data *data;
    int ret, i;

    client = chip_data->client;
    data = i2c_get_clientdata(client);
    device_id = 0;
    chip_data->xdpe_chip_info.support_blackbox = 0;
    mutex_lock(&data->update_lock);
    ret = xdpe_read_device_id(client, &device_id);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        chip_data->xdpe_chip_info.chip_id_status = CHIP_ID_FAIL;
        return;
    }
    chip_data->xdpe_chip_info.ic_device_id = device_id;
    for (i = 0; i < ARRAY_SIZE(xdpe1x2xx_device_infos); i++) {
        if (xdpe1x2xx_device_infos[i].ic_device_id == device_id) {
            DEBUG_VERBOSE("%d-%04x: xdpe1x2xx device id: 0x%x match, scap_addr: 0x%x, rptr_reg_addr: 0x%x\n",
                client->adapter->nr, client->addr, device_id, xdpe1x2xx_device_infos[i].scap_addr,
                xdpe1x2xx_device_infos[i].rptr_reg_addr);
            chip_data->xdpe_chip_info.scap_addr = xdpe1x2xx_device_infos[i].scap_addr;
            chip_data->xdpe_chip_info.rptr_reg_addr = xdpe1x2xx_device_infos[i].rptr_reg_addr;
            chip_data->xdpe_chip_info.chip_id_status = CHIP_ID_OK;
            chip_data->xdpe_chip_info.support_blackbox = xdpe1x2xx_device_infos[i].support_blackbox;
            chip_data->xdpe_chip_info.blackbox_start_addr = xdpe1x2xx_device_infos[i].blackbox_start_addr;
            return;
        }
    }
    DEBUG_VERBOSE("%d-%04x: Unsupport xdpe1x2xx device id: 0x%x\n",
        client->adapter->nr, client->addr, device_id);
    chip_data->xdpe_chip_info.chip_id_status = CHIP_ID_UNKNOWN;
    return;
}

static int xdpe_chipid_reinit(struct i2c_client *client)
{
    chip_id_status_t chip_id_status;
    xdpe_chip_data_t *chip_data;

    chip_data = to_xdpe_chip_data(wb_pmbus_get_driver_info(client));
    chip_id_status = chip_data->xdpe_chip_info.chip_id_status;
    if (chip_id_status == CHIP_ID_FAIL) {
        DEBUG_VERBOSE("%d-%04x: chip id status is fail, try to get chip id\n",
            client->adapter->nr, client->addr);
        xdpe_chip_id_init(chip_data);
        chip_id_status = chip_data->xdpe_chip_info.chip_id_status;
        if (chip_id_status == CHIP_ID_FAIL) {
            DEBUG_VERBOSE("%d-%04x: get chip id failed\n", client->adapter->nr, client->addr);
            return -EIO;
        }
    }
    if (chip_id_status == CHIP_ID_UNKNOWN) {
        return -ENODEV;
    }
    return 0;
}

static int xdpe_reg_dword_read(struct i2c_client *client, uint8_t reg, uint32_t *value)
{
    int i, block_size;
    uint32_t rd_value;
    uint8_t block_data[I2C_SMBUS_BLOCK_MAX + 2] = { 0 };

    block_size = wb_pmbus_read_block_data(client, 0, reg, block_data);
    if (block_size < 0) {
        DEBUG_INFO("%d-%04x: xdpe_reg_dword_read failed, reg: 0x%02x, ret: %d\n",
            client->adapter->nr, client->addr, reg, block_size);
        return block_size;
    }

    if (block_size != WIDTH_4Byte) {
        DEBUG_INFO("%d-%04x: Invalid block size: %d, reg: 0x%02x\n",
            client->adapter->nr, client->addr, block_size, reg);
        return -EINVAL;
    }

    rd_value = 0;
    for (i = 0; i < block_size; i++) {
        rd_value |= block_data[i] << (8 * i);
    }
    *value = rd_value;
    DEBUG_VERBOSE("%d-%04x: xdpe_reg_dword_read success, reg: 0x%02x, read value: 0x%08x\n",
        client->adapter->nr, client->addr, reg, rd_value);
    return 0;
}

static int xdpe_reg_dword_write(struct i2c_client *client, uint8_t reg, uint32_t value)
{
    int i, ret;
    uint8_t buf[WIDTH_4Byte];

    mem_clear(buf, sizeof(buf));
    for (i = 0; i < sizeof(buf); i++) {
        buf[i] = (uint8_t)((value >> (8 * i)) & 0xff);
    }

    ret = wb_pmbus_write_block_data(client, 0, reg, sizeof(buf), buf);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: xdpe_reg_dword_write failed, reg: 0x%02x, write value: 0x%08x, ret: %d\n",
            client->adapter->nr, client->addr, reg, value, ret);
        return ret;
    }

    DEBUG_VERBOSE("%d-%04x: xdpe_reg_dword_write success, reg: 0x%02x, write value: 0x%08x\n",
        client->adapter->nr, client->addr, reg, value);
    return 0;
}

static int xdpe_send_mfr_fw_cmd(struct i2c_client *client, uint8_t fw_cmd)
{
    int ret;

    ret = wb_pmbus_write_byte_data(client, 0, XDPE1X2XX_MFR_FW_COMMAND, fw_cmd);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: send_mfr_fw_cmd failed, fw_cmd: 0x%02x, ret: %d\n",
            client->adapter->nr, client->addr, fw_cmd, ret);
        return ret;
    }

    DEBUG_VERBOSE("%d-%04x: send_mfr_fw_cmd success, fw_cmd: 0x%02x\n",
        client->adapter->nr, client->addr, fw_cmd);
    return 0;
}

static int xdpe_mfr_spec_cmd_read(struct i2c_client *client, uint8_t fw_cmd, uint32_t init_data,
               uint32_t *rd_val, int delay)
{
    int ret;

    /* write init cmd data */
    ret = xdpe_reg_dword_write(client, XDPE1X2XX_MFR_FW_COMMAND_DATA, init_data);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: mfr_spec_cmd_read write init value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, init_data, ret);
        return ret;
    }
    /* send cmd */
    ret = xdpe_send_mfr_fw_cmd(client, fw_cmd);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: mfr_spec_cmd_read send cmd: 0x%02x failed, ret: %d\n",
            client->adapter->nr, client->addr, fw_cmd, ret);
        return ret;
    }

    if (delay > 0) {
        usleep_range(delay, delay + 1);
    }

    /* read cmd data */
    ret = xdpe_reg_dword_read(client, XDPE1X2XX_MFR_FW_COMMAND_DATA, rd_val);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: mfr_spec_cmd_read read cmd data failed, ret: %d\n",
            client->adapter->nr, client->addr, ret);
        return ret;
    }
    DEBUG_VERBOSE("%d-%04x: mfr_spec_cmd_read success, fw_cmd: 0x%02x, init data: 0x%x, rd_val: 0x%x\n",
        client->adapter->nr, client->addr, fw_cmd, init_data, *rd_val);
    return 0;
}

static int xdpe_mfr_spec_cmd_write(struct i2c_client *client, uint8_t fw_cmd, uint32_t wr_val, int delay)
{
    int ret;

    /* write cmd data */
    ret = xdpe_reg_dword_write(client, XDPE1X2XX_MFR_FW_COMMAND_DATA, wr_val);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: mfr_spec_cmd_write write value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, wr_val, ret);
        return ret;
    }
    /* send cmd */
    ret = xdpe_send_mfr_fw_cmd(client, fw_cmd);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: mfr_spec_cmd_write send cmd: 0x%02x failed, ret: %d\n",
            client->adapter->nr, client->addr, fw_cmd, ret);
        return ret;
    }

    if (delay > 0) {
        usleep_range(delay, delay + 1);
    }

    DEBUG_VERBOSE("%d-%04x: mfr_spec_cmd_write success, fw_cmd: 0x%02x, wr_val: 0x%x\n",
        client->adapter->nr, client->addr, fw_cmd, wr_val);
    return 0;
}

static int xdpe_set_mfr_spec_reg_addr(struct i2c_client *client, uint32_t addr)
{
    int ret;
    uint8_t rptr_reg_addr;
    xdpe_chip_data_t *chip_data;

    chip_data = to_xdpe_chip_data(wb_pmbus_get_driver_info(client));
    rptr_reg_addr = chip_data->xdpe_chip_info.rptr_reg_addr;
    ret = xdpe_reg_dword_write(client, rptr_reg_addr, addr);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: set_mfr_spec_reg_addr failed, rptr_reg_addr: 0x%02x, mfr_spec_reg_addr: 0x%08x, ret: %d\n",
            client->adapter->nr, client->addr, rptr_reg_addr, addr, ret);
        return ret;
    }

    DEBUG_VERBOSE("%d-%04x: set_mfr_spec_reg_addr success, rptr_reg_addr: 0x%02x, mfr_spec_reg_addr: 0x%08x\n",
        client->adapter->nr, client->addr, rptr_reg_addr, addr);
    return 0;
}

static int xdpe_mfr_spec_reg_write(struct i2c_client *client, uint8_t length, uint8_t *data_buf)
{
    int i, ret;
    uint32_t wr_value;

    if (length != WIDTH_4Byte) {
        DEBUG_INFO("%d-%04x: Invalid len: %u, mfr_spec_reg_write failed\n",
            client->adapter->nr, client->addr, length);
        return -EINVAL;
    }

    wr_value = 0;
    for (i = 0; i < length; i++) {
        wr_value |= data_buf[i] << (8 * i);
    }
    ret = xdpe_reg_dword_write(client, XDPE1X2XX_MFR_REG_WRITE, wr_value);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: mfr_spec_reg_write failed, write value: 0x%08x, ret: %d\n",
            client->adapter->nr, client->addr, wr_value, ret);
        return ret;
    }

    DEBUG_VERBOSE("%d-%04x: mfr_spec_reg_write success, write value: 0x%08x\n",
        client->adapter->nr, client->addr, wr_value);
    return 0;
}

static int xdpe_mfr_spec_reg_read(struct i2c_client *client, uint8_t length, uint8_t *data_buf)
{
    int i, ret;
    uint32_t rd_value;

    if (length != WIDTH_4Byte) {
        DEBUG_INFO("%d-%04x: Invalid len: %u, mfr_spec_reg_read failed\n",
            client->adapter->nr, client->addr, length);
        return -EINVAL;
    }

    rd_value = 0;
    ret = xdpe_reg_dword_read(client, XDPE1X2XX_MFR_REG_READ, &rd_value);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: mfr_spec_reg_read failed, ret: %d\n",
            client->adapter->nr, client->addr, ret);
        return ret;
    }

    for (i = 0; i < length; i++) {
        data_buf[i] = (uint8_t)((rd_value >> (8 * i)) & 0xff);
    }

    DEBUG_VERBOSE("%d-%04x: mfr_spec_reg_read success, read value: 0x%08x\n",
        client->adapter->nr, client->addr, rd_value);
    return 0;
}

static ssize_t xdpe_chip_crc_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;
    uint32_t chip_crc;
    int ret;

    ret = xdpe_chipid_reinit(client);
    if (ret < 0) {
        return ret;
    }
    chip_crc = 0;
    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret = xdpe_mfr_spec_cmd_read(client, XDPE1X2XX_CMD_GET_CRC, 0, &chip_crc, XDPE1X2XX_CMD_GET_CRC_DELAY);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: get chip crc failed, ret: %d\n",
            client->adapter->nr, client->addr, ret);
        return ret;
    }
    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "0x%08x\n", chip_crc);
}

static ssize_t xdpe_remain_space_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;
    uint32_t reg_val, remain_space;
    int ret;

    ret = xdpe_chipid_reinit(client);
    if (ret < 0) {
        return ret;
    }
    reg_val = 0;
    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret = xdpe_mfr_spec_cmd_read(client, XDPE1X2XX_CMD_OTP_REMAIN_SIZE, 0, &reg_val,
              XDPE1X2XX_CMD_GET_REMAIN_SPACE_DELAY);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: get remain space failed, ret: %d\n",
            client->adapter->nr, client->addr, ret);
        return ret;
    }
    remain_space = reg_val & 0xFFFF;
    DEBUG_VERBOSE("%d-%04x: get remain space success, reg_value: 0x%08x, remain_space: 0x%04x\n",
        client->adapter->nr, client->addr, reg_val, remain_space);
    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "0x%04x\n", remain_space);
}

static ssize_t xdpe_otp_section_invalidate_store(struct device *dev, struct device_attribute *da,
                   const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;
    int ret, val;

    val = 0;
    ret = kstrtoint(buf, 0, &val);
    if (ret) {
        DEBUG_INFO("%d-%04x: Invalid val: %s, kstrtoint failed, ret: %d\n",
            client->adapter->nr, client->addr, buf, ret);
        return ret;
    }

    if ((val < 0) || (val > 0xFFFF)) {
        DEBUG_INFO("%d-%04x: Invalid val: %d, can't do otp_section_invalidate operation\n",
            client->adapter->nr, client->addr, val);
        return -EINVAL;
    }

    ret = xdpe_chipid_reinit(client);
    if (ret < 0) {
        return ret;
    }

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret = xdpe_mfr_spec_cmd_write(client, XDPE1X2XX_CMD_OTP_SECTION_INVALIDATE, val,
              XDPE1X2XX_CMD_OTP_SECTION_INVALIDATE_DELAY);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: set otp_section_invalidate failed, write value: 0x%04x,ret: %d\n",
            client->adapter->nr, client->addr, val, ret);
        return ret;
    }
    DEBUG_VERBOSE("%d-%04x: set otp_section_invalidate success, write value: 0x%04x\n",
        client->adapter->nr, client->addr, val);
    return count;
}

static ssize_t xdpe_upload_to_otp_store(struct device *dev, struct device_attribute *da,
                   const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;
    int ret, val;

    val = 0;
    ret = kstrtoint(buf, 0, &val);
    if (ret) {
        DEBUG_INFO("%d-%04x: Invalid val: %s, kstrtoint failed, ret: %d\n",
            client->adapter->nr, client->addr, buf, ret);
        return ret;
    }

    if ((val < 0) || (val > 0xFFFF)) {
        DEBUG_INFO("%d-%04x: Invalid val: %d, can't do upload_to_otp operation\n",
            client->adapter->nr, client->addr, val);
        return -EINVAL;
    }

    ret = xdpe_chipid_reinit(client);
    if (ret < 0) {
        return ret;
    }

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret = xdpe_mfr_spec_cmd_write(client, XDPE1X2XX_CMD_OTP_CONFIG_STORE, val, 0);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: upload_to_otp failed, write value: 0x%04x,ret: %d\n",
            client->adapter->nr, client->addr, val, ret);
        return ret;
    }
    DEBUG_VERBOSE("%d-%04x: upload_to_otp success, write value: 0x%04x\n",
        client->adapter->nr, client->addr, val);
    return count;
}

static ssize_t xdpe_clear_faults_store(struct device *dev, struct device_attribute *da,
                   const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;
    int ret, val;

    val = 0;
    ret = kstrtoint(buf, 0, &val);
    if (ret) {
        DEBUG_INFO("%d-%04x: Invalid val: %s, kstrtoint failed, ret: %d\n",
            client->adapter->nr, client->addr, buf, ret);
        return ret;
    }

    if (val != 1) {
        DEBUG_INFO("%d-%04x: Invalid val: %d, can't do clear_fault\n",
            client->adapter->nr, client->addr, val);
        return -EINVAL;
    }

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret = wb_pmbus_clear_faults_rv(client);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: clear_fault failed,ret: %d\n",
            client->adapter->nr, client->addr, ret);
        return ret;
    }
    DEBUG_VERBOSE("%d-%04x: clear_fault success\n", client->adapter->nr, client->addr);
    return count;
}

static ssize_t xdpe_status_cml_show(struct device *dev,
        struct device_attribute *devattr,
        char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret;

    mutex_lock(&data->update_lock);
    ret = wb_pmbus_read_byte_data(client, attr->index, PMBUS_STATUS_CML);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: get page%d status cml failed,ret: %d\n",
            client->adapter->nr, client->addr, attr->index, ret);
        return ret;
    }

    DEBUG_VERBOSE("%d-%04x: get page%d status cml success, value: 0x%02x\n",
        client->adapter->nr, client->addr, attr->index, ret);
    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "0x%02x\n", ret);
}

static ssize_t xdpe_scratchpad_addr_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;
    uint32_t scap_addr;
    xdpe_chip_data_t *chip_data;
    int ret;

    ret = xdpe_chipid_reinit(client);
    if (ret < 0) {
        return ret;
    }
    chip_data = to_xdpe_chip_data(wb_pmbus_get_driver_info(client));
    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    scap_addr = chip_data->xdpe_chip_info.scap_addr;
    mutex_unlock(&data->update_lock);

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "0x%08x\n", scap_addr);
}

static loff_t xdpe_dev_llseek_set_addr(struct file *file, loff_t offset)
{
    int ret;
    struct i2c_client *client;
    xdpe_chip_data_t *chip_data;
    struct pmbus_data *data;

    if (offset < 0 || offset > XDPE1X2XX_MAX_OFFSET) {
        DEBUG_INFO("SEEK_SET, offset: %lld, invalid.\n", offset);
        return -EINVAL;
    }

    chip_data = file->private_data;
    if (chip_data == NULL) {
        DEBUG_INFO("Invalid xdpe_chip_data, xdpe_chip_data is NULL\n");
        return -ENODEV;
    }
    client = chip_data->client;
    if (client == NULL) {
        DEBUG_INFO("Invalid i2c client, i2c client is NULL\n");
        return -ENXIO;
    }

    ret = xdpe_chipid_reinit(client);
    if (ret < 0) {
        return ret;
    }

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret = xdpe_set_mfr_spec_reg_addr(client, offset);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_INFO("llseek_set_addr: 0x%llx failed, ret: %d\n", offset, ret);
        return ret;
    }
    DEBUG_VERBOSE("llseek_set_addr: 0x%llx success\n", offset);
    return 0;
}

static loff_t xdpe_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret;

    switch (origin) {
    case SEEK_SET:        /* Offset relative to the beginning of the file. */
        ret = xdpe_dev_llseek_set_addr(file, offset);
        if (ret < 0) {
            return ret;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    default:
        DEBUG_INFO("unsupport llseek type:%d.\n", origin);
        ret = -EINVAL;
        break;
    }
    return ret;
}

static ssize_t xdpe_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int ret;
    struct pmbus_data *data;
    struct i2c_client *client;
    xdpe_chip_data_t *chip_data;
    uint8_t rd_buf[WIDTH_4Byte];

    if ((file == NULL) || (buf == NULL) || (offset == NULL)) {
        DEBUG_INFO("Invalid param, read failed\n");
        return -EINVAL;
    }

    if (count != WIDTH_4Byte) {
        DEBUG_INFO("Invalid read conut %zu\n", count);
        return -EINVAL;
    }

    chip_data = file->private_data;
    if (chip_data == NULL) {
        DEBUG_INFO("Invalid xdpe_chip_data, xdpe_chip_data is NULL\n");
        return -ENODEV;
    }
    client = chip_data->client;
    if (client == NULL) {
        DEBUG_INFO("Invalid i2c client, i2c client is NULL\n");
        return -ENXIO;
    }

    DEBUG_VERBOSE("%d-%04x: offset: 0x%llx read count %zu\n",
        client->adapter->nr, client->addr, *offset, count);

    mem_clear(rd_buf, sizeof(rd_buf));
    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret = xdpe_mfr_spec_reg_read(client, sizeof(rd_buf), rd_buf);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: reg offset: 0x%llx read count %zu, read failed, ret: %d\n",
            client->adapter->nr, client->addr, *offset, count, ret);
        return ret;
    }

    if (copy_to_user(buf, rd_buf, count)) {
        DEBUG_INFO("copy_to_user error\n");
        return -EFAULT;
    }

    *offset += count;
    return count;
}

static ssize_t xdpe_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    int ret, i;
    struct pmbus_data *data;
    struct i2c_client *client;
    xdpe_chip_data_t *chip_data;
    uint8_t wr_buf[WIDTH_4Byte];

    if ((file == NULL) || (buf == NULL) || (offset == NULL)) {
        DEBUG_INFO("Invalid param, write failed\n");
        return -EINVAL;
    }

    if (count != WIDTH_4Byte) {
        DEBUG_INFO("Invalid write conut %zu\n", count);
        return -EINVAL;
    }

    chip_data = file->private_data;
    if (chip_data == NULL) {
        DEBUG_INFO("Invalid xdpe_chip_data, xdpe_chip_data is NULL\n");
        return -ENODEV;
    }
    client = chip_data->client;
    if (client == NULL) {
        DEBUG_INFO("Invalid i2c client, i2c client is NULL\n");
        return -ENXIO;
    }

    mem_clear(wr_buf, sizeof(wr_buf));
    if (copy_from_user(wr_buf, buf, count)) {
        DEBUG_INFO("copy_from_user failed.\n");
        return -EFAULT;
    }

    DEBUG_VERBOSE("%d-%04x: reg offset: 0x%llx write count %zu\n",
        client->adapter->nr, client->addr, *offset, count);
    for (i = 0; i < count; i++) {
        DEBUG_VERBOSE("buf[%d] : 0x%02x\n", i, wr_buf[i]);
    }

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret = xdpe_mfr_spec_reg_write(client, sizeof(wr_buf), wr_buf);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_INFO("%d-%04x: reg offset: 0x%llx write count %zu, write failed, ret: %d\n",
            client->adapter->nr, client->addr, *offset, count, ret);
        return ret;
    }

    *offset += count;
    return count;
}

static long xdpe_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int minor_to_dev(int minor, xdpe_chip_data_t **xdpe_dev)
{
    int i;

    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (xdpe_dev_arry[i] == NULL) {
            continue;
        }
        if (xdpe_dev_arry[i]->misc_dev.minor == minor) {
            *xdpe_dev = xdpe_dev_arry[i];
            return 0;
        }
    }
    return -ENODEV;
}

static int add_dev_to_g_dev_list(xdpe_chip_data_t *xdpe_dev)
{
    int i;
    unsigned long flags;

    spin_lock_irqsave(&dev_array_lock, flags);
    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (xdpe_dev_arry[i] == NULL) {
            xdpe_dev_arry[i] = xdpe_dev;
            spin_unlock_irqrestore(&dev_array_lock, flags);
            return 0;
        }
    }
    spin_unlock_irqrestore(&dev_array_lock, flags);
    return -EBUSY;
}

static int remove_dev_from_g_dev_list(int minor)
{
    int i;
    unsigned long flags;

    spin_lock_irqsave(&dev_array_lock, flags);
    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (xdpe_dev_arry[i] == NULL) {
            continue;
        }
        if (xdpe_dev_arry[i]->misc_dev.minor == minor) {
            xdpe_dev_arry[i] = NULL;
            spin_unlock_irqrestore(&dev_array_lock, flags);
            return 0;
        }
    }
    spin_unlock_irqrestore(&dev_array_lock, flags);
    return -ENODEV ;
}

static int xdpe_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    xdpe_chip_data_t *xdpe_info;
    int ret;

    DEBUG_VERBOSE("inode: %p, file: %p, minor: %u", inode, file, minor);

    ret = minor_to_dev(minor, &xdpe_info);
    if (ret) {
        return ret;
    }
    file->private_data = xdpe_info;
    return 0;
}

static int xdpe_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;

    return 0;
}

static const struct file_operations xdpe_dev_fops = {
    .owner          = THIS_MODULE,
    .llseek         = xdpe_dev_llseek,
    .read           = xdpe_dev_read,
    .write          = xdpe_dev_write,
    .unlocked_ioctl = xdpe_dev_ioctl,
    .open           = xdpe_dev_open,
    .release        = xdpe_dev_release,
};

static xdpe_vout_data_t g_xdpe_vout_group[] = {
    {.vout_mode = 0x18, .vout_precision = 256},
    {.vout_mode = 0x17, .vout_precision = 512},
    {.vout_mode = 0x16, .vout_precision = 1024},
    {.vout_mode = 0x15, .vout_precision = 2048},
    {.vout_mode = 0x14, .vout_precision = 4096},
};

static ssize_t set_xdpe132g5c_avs(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int ret;
    unsigned long val;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;

    data = i2c_get_clientdata(client);
    val = 0;
    ret = kstrtoul(buf, 0, &val);
    if (ret){
        return ret;
    }
    mutex_lock(&data->update_lock);
    if (data->dev_available == WB_DEV_UNAVAILABLE_FLAG) {
        DEBUG_VERBOSE("%d-%04x: dev unavailable set_xdpe132g5c_avs failed\n",
            client->adapter->nr, client->addr);
        mutex_unlock(&data->update_lock);
        return -EBUSY;
    }

    /* set value */
    ret = wb_pmbus_write_word_data(client, attr->index, PMBUS_VOUT_COMMAND, (u16)val);
    if (ret < 0) {
        DEBUG_ERROR("set pmbus_vout_command fail\n");
        goto finish_set;
    }
finish_set:
    wb_pmbus_clear_faults(client);
    mutex_unlock(&data->update_lock);
    return (ret < 0) ? ret : count;

}

static ssize_t show_xdpe132g5c_avs(struct device *dev, struct device_attribute *da, char *buf)
{
    int val;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    if (data->dev_available == WB_DEV_UNAVAILABLE_FLAG) {
        DEBUG_VERBOSE("%d-%04x: dev unavailable show_xdpe132g5c_avs failed\n",
            client->adapter->nr, client->addr);
        mutex_unlock(&data->update_lock);
        return -EBUSY;
    }

    val = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (val < 0) {
        DEBUG_ERROR("fail val = %d\n", val);
        wb_pmbus_clear_faults(client);
        mutex_unlock(&data->update_lock);
        return val;
    }
    wb_pmbus_clear_faults(client);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, BUF_SIZE, "0x%04x\n", val);
}

static int xdpe_get_vout_precision(struct i2c_client *client, int page, int *vout_precision)
{
    int i, vout_mode, a_size;

    vout_mode = wb_pmbus_read_byte_data(client, page, PMBUS_VOUT_MODE);
    if (vout_mode < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe page%d vout mode reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, page, PMBUS_VOUT_MODE, vout_mode);
        return vout_mode;
    }

    a_size = ARRAY_SIZE(g_xdpe_vout_group);
    for (i = 0; i < a_size; i++) {
        if (g_xdpe_vout_group[i].vout_mode == vout_mode) {
            *vout_precision = g_xdpe_vout_group[i].vout_precision;
            DEBUG_VERBOSE("%d-%04x: match, page%d, vout mode: 0x%x, precision: %d\n",
                client->adapter->nr, client->addr, page, vout_mode, *vout_precision);
            break;
        }
    }
    if (i == a_size) {
        DEBUG_ERROR("%d-%04x: invalid, page%d, vout mode: 0x%x\n",client->adapter->nr, client->addr,
            page, vout_mode);
        return -EINVAL;
    }
    return 0;
}

static ssize_t xdpe132g5_avs_vout_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int vout_cmd, ret, vout_precision;
    s64 vout;

    mutex_lock(&data->update_lock);
    if (data->dev_available == WB_DEV_UNAVAILABLE_FLAG) {
        DEBUG_VERBOSE("%d-%04x: dev unavailable xdpe132g5_avs_vout_show failed\n",
            client->adapter->nr, client->addr);
        mutex_unlock(&data->update_lock);
        return -EBUSY;
    }

    ret = xdpe_get_vout_precision(client, attr->index, &vout_precision);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: get xdpe avs%d vout precision failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd);
        mutex_unlock(&data->update_lock);
        return vout_cmd;
    }

    mutex_unlock(&data->update_lock);
    vout = div_s64((s64)vout_cmd * 1000L * 1000L, vout_precision);
    DEBUG_VERBOSE("%d-%04x: page%d vout: %lld, vout_cmd: 0x%x, precision: %d\n", client->adapter->nr,
        client->addr, attr->index, vout, vout_cmd, vout_precision);
    return snprintf(buf, PAGE_SIZE, "%lld\n", vout);
}

static ssize_t xdpe132g5_avs_vout_store(struct device *dev, struct device_attribute *devattr,
                   const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int vout_max, vout_min;
    int ret, vout_cmd, vout_cmd_set;
    int vout_precision;
    s64 vout;

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        DEBUG_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    vout = 0;
    ret = kstrtos64(buf, 0, &vout);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    if (vout <= 0) {
        DEBUG_ERROR("%d-%04x: invalid value: %lld \n", client->adapter->nr, client->addr, vout);
        return -EINVAL;
    }

    vout_max = data->vout_max[attr->index];
    vout_min = data->vout_min[attr->index];
    if ((vout > vout_max) || (vout < vout_min)) {
        DEBUG_ERROR("%d-%04x: vout value: %lld, out of range [%d, %d] \n", client->adapter->nr,
            client->addr, vout, vout_min, vout_max);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    if (data->dev_available == WB_DEV_UNAVAILABLE_FLAG) {
        DEBUG_VERBOSE("%d-%04x: dev unavailable xdpe132g5_avs_vout_store failed\n",
            client->adapter->nr, client->addr);
        mutex_unlock(&data->update_lock);
        return -EBUSY;
    }

    ret = xdpe_get_vout_precision(client, attr->index, &vout_precision);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: get xdpe avs%d vout precision failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    vout_cmd_set = div_s64(vout * vout_precision, 1000L * 1000L);
    if (vout_cmd_set > 0xffff) {
        DEBUG_ERROR("%d-%04x: invalid value, page%d, vout: %lld, vout_precision: %d, vout_cmd_set: 0x%x\n",
            client->adapter->nr, client->addr, attr->index, vout, vout_precision, vout_cmd_set);
        mutex_unlock(&data->update_lock);
        return -EINVAL;
    }

    /* set VOUT_COMMAND */
    ret = wb_pmbus_write_word_data(client, attr->index, PMBUS_VOUT_COMMAND, (u16)vout_cmd_set);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe page%d vout cmd reg: 0x%x, value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_set, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    /* read back VOUT_COMMAND */
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd);
        mutex_unlock(&data->update_lock);
        return vout_cmd;
    }

    if (vout_cmd != vout_cmd_set) {
        DEBUG_ERROR("%d-%04x: page%d vout cmd value check error, vout cmd read: 0x%x, vout cmd set: 0x%x\n",
            client->adapter->nr, client->addr, attr->index, vout_cmd, vout_cmd_set);
        mutex_unlock(&data->update_lock);
        return -EIO;
    }
    mutex_unlock(&data->update_lock);
    DEBUG_VERBOSE("%d-%04x: set page%d vout cmd success, vout: %lld uV, vout_cmd_set: 0x%x\n",
        client->adapter->nr, client->addr, attr->index, vout, vout_cmd_set);
    return count;
}

static ssize_t xdpe132g5_blackbox_iteration(struct i2c_client *client, uint32_t addr, char *buf, int buf_len) 
{
    int ret, i;
    u16 header, size;
    u32 tmp_val;
    int offset = 0;
    int iterations = 0;

    while (iterations++ < BLACKBOX_MAX_READ_COUNT) {
        DEBUG_VERBOSE("%d-%04x: xdpe132g5_blackbox_iteration iterations: %d\n", client->adapter->nr, client->addr, iterations);

        ret = xdpe_set_mfr_spec_reg_addr(client, addr);
        if (ret != 0) {
            DEBUG_ERROR("%d-%04x: write point addr to addr reg fail, ret::%d\n", client->adapter->nr, client->addr, ret);
            return ret;
        }

        ret = xdpe_reg_dword_read(client, XDPE1X2XX_MFR_REG_READ, &tmp_val);
        if (ret != 0) {
            DEBUG_ERROR("%d-%04x: read blackbox header fail, ret::%d\n", client->adapter->nr, client->addr, ret);
            return ret;
        }
        header = (uint16_t)(tmp_val & 0xffff);
        DEBUG_VERBOSE("%d-%04x: xdpe132g5_blackbox_iteration reg: 0x%08x, header: 0x%04x\n", 
            client->adapter->nr, client->addr, tmp_val, header);

        ret = xdpe_reg_dword_read(client, XDPE1X2XX_MFR_REG_READ, &tmp_val);
        if (ret != 0) {
            DEBUG_ERROR("%d-%04x: read blackbox size fail, ret::%d\n", client->adapter->nr, client->addr, ret);
            return ret;
        }
        size = (uint16_t)(tmp_val & 0xffff);
        DEBUG_VERBOSE("%d-%04x: xdpe132g5_blackbox_iteration reg: 0x%08x, size: 0x%04x\n", 
            client->adapter->nr, client->addr, tmp_val, size);

        if (header == BLACKBOX_HEADER_END) {
            DEBUG_ERROR("%d-%04x: did not have blackbox info\n", client->adapter->nr, client->addr);
            return -ENODEV;
        } else if (header != BLACKBOX_HEADER) {
            addr += size;
            continue;
        }

        for (i = 2; i < size; i++) {
            ret = xdpe_reg_dword_read(client, XDPE1X2XX_MFR_REG_READ, &tmp_val);
            if (ret != 0) {
                DEBUG_ERROR("%d-%04x: read blackbox data fail, ret::%d\n", client->adapter->nr, client->addr, ret);
                return ret;
            }
            offset += scnprintf(buf + offset, buf_len - offset, "%08x", tmp_val);
        }
        offset += scnprintf(buf + offset, buf_len - offset, "\n");
        return offset;
    }

    DEBUG_ERROR("%d-%04x: blackbox read count exceeds maximum limit:%d\n", client->adapter->nr, client->addr, BLACKBOX_MAX_READ_COUNT);
    return -EDOM;
}

static ssize_t xdpe132g5_blackbox_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data = i2c_get_clientdata(client);
    xdpe_chip_data_t *chip_data;
    chip_id_status_t chip_id_status;
    int ret;

    mem_clear(buf, PAGE_SIZE);
    chip_data = to_xdpe_chip_data(wb_pmbus_get_driver_info(client));
    ret = xdpe_chipid_reinit(client);
    if (ret < 0) {
        chip_id_status = chip_data->xdpe_chip_info.chip_id_status;
        if (chip_id_status == CHIP_ID_FAIL) {
            COMMON_ERROR_PRINT(buf, PAGE_SIZE, "Fail to get device id.\n");
            return strlen(buf);
        } else if (chip_id_status == CHIP_ID_UNKNOWN) {
            COMMON_ERROR_PRINT(buf, PAGE_SIZE, "Unknown device id, device_id: 0x%x.\n", chip_data->xdpe_chip_info.ic_device_id);
            return strlen(buf);
        }
        DEBUG_ERROR("%d-%04x: blackbox read chipid reinit fail ret:%d\n", client->adapter->nr, client->addr, ret);
        return ret;
    }

    if (!chip_data->xdpe_chip_info.support_blackbox) {
        COMMON_ERROR_PRINT(buf, PAGE_SIZE, "Unsupport blackbox, device_id: 0x%x.\n", chip_data->xdpe_chip_info.ic_device_id);
        return PAGE_SIZE;
    }

    mutex_lock(&data->update_lock);
    ret = xdpe132g5_blackbox_iteration(client, chip_data->xdpe_chip_info.blackbox_start_addr, buf, PAGE_SIZE);
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t xdpe1x2xx_phase_curr_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    int ret, a_size, curr_data, i, phase_curr_reg_value;
    struct i2c_client *client = to_i2c_client(dev->parent);
    u8 phase_index = to_sensor_dev_attr(devattr)->index;
    struct pmbus_data *data = i2c_get_clientdata(client);
    u8 data_index;
    s64 value;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1];

    mutex_lock(&data->update_lock);

    if (data->dev_available == WB_DEV_UNAVAILABLE_FLAG) {
        DEBUG_VERBOSE("%d-%04x: dev unavailable xdpe1x2xx_phase_curr_show failed\n",
            client->adapter->nr, client->addr);
        mutex_unlock(&data->update_lock);
        return -EBUSY;
    }

    DEBUG_VERBOSE("%d-%04x: phase_index: %u\n",
                client->adapter->nr, client->addr, phase_index);

    a_size = ARRAY_SIZE(g_xdpe1x2xx_phase_curr_reg_value_group);
    for (i = 0; i < a_size; i++) {
        if (g_xdpe1x2xx_phase_curr_reg_value_group[i].phase_index == phase_index) {
            phase_curr_reg_value = g_xdpe1x2xx_phase_curr_reg_value_group[i].phase_curr_reg_value;
            data_index = g_xdpe1x2xx_phase_curr_reg_value_group[i].data_index;
            DEBUG_VERBOSE("%d-%04x: data_index: %u, phase_index: %u, phase_curr_reg_value: 0x%x\n",
                client->adapter->nr, client->addr, data_index, phase_index, phase_curr_reg_value);
            break;
        }
    }
    if (i == a_size) {
        DEBUG_ERROR("%d-%04x: invalid, phase_index%u\n",client->adapter->nr, client->addr, phase_index);
        mutex_unlock(&data->update_lock);
        return -EINVAL;
    }
    ret = xdpe_set_mfr_spec_reg_addr(client, phase_curr_reg_value);
    if (ret < 0) {
        DEBUG_VERBOSE("%d-%04x: Failed to set mfr spec reg addr, ret = %d",
            client->adapter->nr, client->addr, ret);
        mutex_unlock(&data->update_lock);
        return -EIO;
    }

    mem_clear(block_buffer, sizeof(block_buffer));
    ret = xdpe_mfr_spec_reg_read(client, XDPE1X2XX_PHASE_BLOCK_LEN, block_buffer);
    if (ret < 0) {
        DEBUG_VERBOSE("%d-%04x: Failed to read mfr spec reg, ret = %d",
            client->adapter->nr, client->addr, ret);
        mutex_unlock(&data->update_lock);
        return -EIO;
    }

    curr_data = 0;
    for (i = 0; i < XDPE1X2XX_PHASE_BLOCK_LEN; i++) {
        DEBUG_VERBOSE("%d-%04x: block_buffer[%u] = 0x%x\n",
            client->adapter->nr, client->addr, i, block_buffer[i]);
        curr_data |= block_buffer[i] << (8 * i);
    }

    /* data: bit0~bit13 for curr1, bit14~bit27 for curr2 */
    if (data_index == XDPE1X2XX_PHASE_CURR_DATA_LOW) {
        curr_data = (curr_data & 0x3fff);
    } else if (data_index == XDPE1X2XX_PHASE_CURR_DATA_HIGH) {
        curr_data = ((curr_data >> XDPE1X2XX_PHASE_CURR_DATA_HIGH_BIT_OFFSET) & 0x3fff);
    } else {
        DEBUG_ERROR("%d-%04x: invalid, data_index%u\n",client->adapter->nr, client->addr, data_index);
        mutex_unlock(&data->update_lock);
        return -EINVAL;
    }

    value = XDPE1X2XX_PHASE_CURR_DATA_TO_VALUE(curr_data);

    DEBUG_VERBOSE("%d-%04x: read phase curr word data success, reg: 0x%04x, value: %lld mA\n",
        client->adapter->nr, client->addr, curr_data, value);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%lld\n", value);
}

static SENSOR_DEVICE_ATTR_RW(avs0_vout, xdpe132g5_avs_vout, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout, xdpe132g5_avs_vout, 1);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_max, pmbus_avs_vout_max, 0);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_min, pmbus_avs_vout_min, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_max, pmbus_avs_vout_max, 1);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_min, pmbus_avs_vout_min, 1);

static SENSOR_DEVICE_ATTR_RO(phase1_curr, xdpe1x2xx_phase_curr, 1);
static SENSOR_DEVICE_ATTR_RO(phase2_curr, xdpe1x2xx_phase_curr, 2);
static SENSOR_DEVICE_ATTR_RO(phase3_curr, xdpe1x2xx_phase_curr, 3);
static SENSOR_DEVICE_ATTR_RO(phase4_curr, xdpe1x2xx_phase_curr, 4);
static SENSOR_DEVICE_ATTR_RO(phase5_curr, xdpe1x2xx_phase_curr, 5);
static SENSOR_DEVICE_ATTR_RO(phase6_curr, xdpe1x2xx_phase_curr, 6);
static SENSOR_DEVICE_ATTR_RO(phase7_curr, xdpe1x2xx_phase_curr, 7);
static SENSOR_DEVICE_ATTR_RO(phase8_curr, xdpe1x2xx_phase_curr, 8);
static SENSOR_DEVICE_ATTR_RO(phase9_curr, xdpe1x2xx_phase_curr, 9);
static SENSOR_DEVICE_ATTR_RO(phase10_curr, xdpe1x2xx_phase_curr, 10);
static SENSOR_DEVICE_ATTR_RO(phase11_curr, xdpe1x2xx_phase_curr, 11);
static SENSOR_DEVICE_ATTR_RO(phase12_curr, xdpe1x2xx_phase_curr, 12);
static SENSOR_DEVICE_ATTR_RO(phase13_curr, xdpe1x2xx_phase_curr, 13);
static SENSOR_DEVICE_ATTR_RO(phase14_curr, xdpe1x2xx_phase_curr, 14);
static SENSOR_DEVICE_ATTR_RO(phase15_curr, xdpe1x2xx_phase_curr, 15);
static SENSOR_DEVICE_ATTR_RO(phase16_curr, xdpe1x2xx_phase_curr, 16);

static struct attribute *avs_ctrl_attrs[] = {
    &sensor_dev_attr_avs0_vout.dev_attr.attr,
    &sensor_dev_attr_avs1_vout.dev_attr.attr,
    &sensor_dev_attr_avs0_vout_max.dev_attr.attr,
    &sensor_dev_attr_avs0_vout_min.dev_attr.attr,
    &sensor_dev_attr_avs1_vout_max.dev_attr.attr,
    &sensor_dev_attr_avs1_vout_min.dev_attr.attr,
    NULL,
};

static struct attribute *phase_curr_attrs[] = {
    &sensor_dev_attr_phase1_curr.dev_attr.attr,
    &sensor_dev_attr_phase2_curr.dev_attr.attr,
    &sensor_dev_attr_phase3_curr.dev_attr.attr,
    &sensor_dev_attr_phase4_curr.dev_attr.attr,
    &sensor_dev_attr_phase5_curr.dev_attr.attr,
    &sensor_dev_attr_phase6_curr.dev_attr.attr,
    &sensor_dev_attr_phase7_curr.dev_attr.attr,
    &sensor_dev_attr_phase8_curr.dev_attr.attr,
    &sensor_dev_attr_phase9_curr.dev_attr.attr,
    &sensor_dev_attr_phase10_curr.dev_attr.attr,
    &sensor_dev_attr_phase11_curr.dev_attr.attr,
    &sensor_dev_attr_phase12_curr.dev_attr.attr,
    &sensor_dev_attr_phase13_curr.dev_attr.attr,
    &sensor_dev_attr_phase14_curr.dev_attr.attr,
    &sensor_dev_attr_phase15_curr.dev_attr.attr,
    &sensor_dev_attr_phase16_curr.dev_attr.attr,
    NULL,
};

static const struct attribute_group avs_ctrl_group = {
    .attrs = avs_ctrl_attrs,
};

static const struct attribute_group phase_curr_group = {
    .attrs = phase_curr_attrs,
};

static const struct attribute_group *xdpe132g5_attribute_groups[] = {
    &avs_ctrl_group,
    &phase_curr_group,
    NULL,
};

static SENSOR_DEVICE_ATTR(avs0_vout_command, S_IRUGO | S_IWUSR, show_xdpe132g5c_avs, set_xdpe132g5c_avs, 0);
static SENSOR_DEVICE_ATTR(avs1_vout_command, S_IRUGO | S_IWUSR, show_xdpe132g5c_avs, set_xdpe132g5c_avs, 1);
static SENSOR_DEVICE_ATTR(dfx_info, S_IRUGO, show_pmbus_dfx_info, NULL, -1);
static SENSOR_DEVICE_ATTR(dfx_info0, S_IRUGO, show_pmbus_dfx_info, NULL, 0);
static SENSOR_DEVICE_ATTR(dfx_info1, S_IRUGO, show_pmbus_dfx_info, NULL, 1);
static SENSOR_DEVICE_ATTR_RO(device_name, pmbus_device_name, PMBUS_IC_DEVICE_ID);
static SENSOR_DEVICE_ATTR_RO(device_id, pmbus_block_data, PMBUS_IC_DEVICE_ID);
static SENSOR_DEVICE_ATTR_RO(ic_device_rev, pmbus_block_data, PMBUS_IC_DEVICE_REV);
static SENSOR_DEVICE_ATTR_RO(status0_word, pmbus_get_status_word, 0);
static SENSOR_DEVICE_ATTR_RO(status1_word, pmbus_get_status_word, 1);
static ssize_t xdpe132_version_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct sensor_device_attribute attr1 = { 0 };
    struct sensor_device_attribute attr2 = { 0 };
    char ver1_buf[PMBUS_DEV_NAME_SIZE] = { 0 };
    char ver2_buf[PMBUS_DEV_NAME_SIZE] = { 0 };
    u32 ver1_data = 0, ver2_data = 0;
    u32 ver_data = 0;
    int ret;
    
    attr1.index = XDPE132_VERSION1_REG;
    attr2.index = XDPE132_VERSION2_REG;
    
    ret = pmbus_block_data_reverse_show(dev, &attr1.dev_attr, ver1_buf);
    if (ret < 0) {
        return ret;
    }
    
    ret = pmbus_block_data_reverse_show(dev, &attr2.dev_attr, ver2_buf);
    if (ret < 0) {
        return ret;
    }
    
    if (sscanf(ver1_buf, "0x%x", &ver1_data) != 1) {
        return -EINVAL;
    }
    
    if (sscanf(ver2_buf, "0x%x", &ver2_data) != 1) {
        return -EINVAL;
    }
    
    ver_data = (ver1_data << 16) | ver2_data;
    
    return snprintf(buf, PAGE_SIZE, "0x%08x\n", ver_data);
}

static SENSOR_DEVICE_ATTR_RO(version1, pmbus_block_data_reverse, XDPE132_VERSION1_REG);
static SENSOR_DEVICE_ATTR_RO(version2, pmbus_block_data_reverse, XDPE132_VERSION2_REG);
static SENSOR_DEVICE_ATTR_RO(version, xdpe132_version, 0);
static SENSOR_DEVICE_ATTR_RW(dev_available, pmbus_dev_available, 0);
static SENSOR_DEVICE_ATTR(chip_crc, S_IRUGO, xdpe_chip_crc_show, NULL, 0);
static SENSOR_DEVICE_ATTR(remain_space, S_IRUGO, xdpe_remain_space_show, NULL, 0);
static SENSOR_DEVICE_ATTR(otp_section_invalidate, S_IWUSR, NULL, xdpe_otp_section_invalidate_store, 0);
static SENSOR_DEVICE_ATTR(upload_to_otp, S_IWUSR, NULL, xdpe_upload_to_otp_store, 0);
static SENSOR_DEVICE_ATTR(clear_faults, S_IWUSR, NULL, xdpe_clear_faults_store, 0);
static SENSOR_DEVICE_ATTR(status0_cml, S_IRUGO, xdpe_status_cml_show, NULL, 0);
static SENSOR_DEVICE_ATTR(status1_cml, S_IRUGO, xdpe_status_cml_show, NULL, 1);
static SENSOR_DEVICE_ATTR(scap_addr, S_IRUGO, xdpe_scratchpad_addr_show, NULL, 0);
static SENSOR_DEVICE_ATTR_RO(blackbox, xdpe132g5_blackbox, 0);
static SENSOR_DEVICE_ATTR_RO(mfr_id, pmbus_block_data, PMBUS_MFR_ID);
static SENSOR_DEVICE_ATTR_RO(bus_status, pmbus_get_bus_status, PMBUS_MFR_ID);
static SENSOR_DEVICE_ATTR_2_RO(status0_vout, pmbus_get_byte_status, 0, PMBUS_STATUS_VOUT);
static SENSOR_DEVICE_ATTR_2_RO(status0_iout, pmbus_get_byte_status, 0, PMBUS_STATUS_IOUT);
static SENSOR_DEVICE_ATTR_2_RO(status0_input, pmbus_get_byte_status, 0, PMBUS_STATUS_INPUT);
static SENSOR_DEVICE_ATTR_2_RO(status0_temp, pmbus_get_byte_status, 0, PMBUS_STATUS_TEMPERATURE);
static SENSOR_DEVICE_ATTR_2_RO(status0_mfr_spec, pmbus_get_byte_status, 0, PMBUS_STATUS_MFR_SPECIFIC);
static SENSOR_DEVICE_ATTR_2_RO(status0_byte, pmbus_get_byte_status, 0, PMBUS_STATUS_BYTE);
static SENSOR_DEVICE_ATTR_2_RO(status1_byte, pmbus_get_byte_status, 1, PMBUS_STATUS_BYTE);

static struct attribute *xdpe132g5c_sysfs_attrs[] = {
    &sensor_dev_attr_avs0_vout_command.dev_attr.attr,
    &sensor_dev_attr_avs1_vout_command.dev_attr.attr,
    &sensor_dev_attr_dfx_info.dev_attr.attr,
    &sensor_dev_attr_dfx_info0.dev_attr.attr,
    &sensor_dev_attr_dfx_info1.dev_attr.attr,
    &sensor_dev_attr_device_id.dev_attr.attr,
    &sensor_dev_attr_device_name.dev_attr.attr,
    &sensor_dev_attr_ic_device_rev.dev_attr.attr,
    &sensor_dev_attr_status0_byte.dev_attr.attr,
    &sensor_dev_attr_status1_byte.dev_attr.attr,
    &sensor_dev_attr_status0_word.dev_attr.attr,
    &sensor_dev_attr_status1_word.dev_attr.attr,
    &sensor_dev_attr_version1.dev_attr.attr,
    &sensor_dev_attr_version2.dev_attr.attr,
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_dev_available.dev_attr.attr,
    &sensor_dev_attr_chip_crc.dev_attr.attr,
    &sensor_dev_attr_remain_space.dev_attr.attr,
    &sensor_dev_attr_otp_section_invalidate.dev_attr.attr,
    &sensor_dev_attr_upload_to_otp.dev_attr.attr,
    &sensor_dev_attr_clear_faults.dev_attr.attr,
    &sensor_dev_attr_status0_cml.dev_attr.attr,
    &sensor_dev_attr_status1_cml.dev_attr.attr,
    &sensor_dev_attr_scap_addr.dev_attr.attr,
    &sensor_dev_attr_blackbox.dev_attr.attr,
    &sensor_dev_attr_mfr_id.dev_attr.attr,
    &sensor_dev_attr_bus_status.dev_attr.attr,
    &sensor_dev_attr_status0_vout.dev_attr.attr,
    &sensor_dev_attr_status0_iout.dev_attr.attr,
    &sensor_dev_attr_status0_input.dev_attr.attr,
    &sensor_dev_attr_status0_temp.dev_attr.attr,
    &sensor_dev_attr_status0_mfr_spec.dev_attr.attr,
    NULL,
};

static const struct attribute_group xdpe132g5c_sysfs_attrs_group = {
    .attrs = xdpe132g5c_sysfs_attrs,
};

static int xdpe132g5c_identify(struct i2c_client *client, struct pmbus_driver_info *info)
{
    u8 vout_params;
    int ret, i, retry;

    /* Read the register with VOUT scaling value.*/
    for (i = 0; i < XDPE132G5C_PAGE_NUM; i++) {
        for (retry = 0; retry < RETRY_TIME; retry++) {
            ret = wb_pmbus_read_byte_data(client, i, PMBUS_VOUT_MODE);
            if (ret < 0 || ret == 0xff) {
                msleep(5);
                continue;
            } else {
                break;
            }
        }
        if (ret < 0) {
            return ret;
        }

        switch (ret >> 5) {
        case 0: /* linear mode      */
            if (info->format[PSC_VOLTAGE_OUT] != linear) {
                return -ENODEV;
            }
            break;
        case 1: /* VID mode         */
            if (info->format[PSC_VOLTAGE_OUT] != vid) {
                return -ENODEV;
            }
            vout_params = ret & GENMASK(4, 0);
            switch (vout_params) {
            case XDPE132G5C_PROT_VR13_10MV:
            case XDPE132G5C_PROT_VR12_5_10MV:
                info->vrm_version[i] = vr13;
                break;
            case XDPE132G5C_PROT_VR13_5MV:
            case XDPE132G5C_PROT_VR12_5MV:
            case XDPE132G5C_PROT_IMVP8_5MV:
                info->vrm_version[i] = vr12;
                break;
            case XDPE132G5C_PROT_IMVP9_10MV:
                info->vrm_version[i] = imvp9;
                break;
            default:
                return -EINVAL;
            }
            break;
        case 2: /* direct mode      */
            if (info->format[PSC_VOLTAGE_OUT] != direct) {
                return -ENODEV;
            }
            break;
        default:
            return -ENODEV;
        }
    }

    return 0;
}

static int xdpe_chip_read_word_data(struct i2c_client *client, int page, int phase, int reg)
{
    const struct pmbus_driver_info *info = wb_pmbus_get_driver_info(client);
    const xdpe_chip_data_t *data = to_xdpe_chip_data(info);
    int ret = 0;

    /* Virtual PMBUS Command not supported */
    if (reg >= PMBUS_VIRT_BASE) {
        DEBUG_ERROR("reg:0x%x is invalid\n", reg);
        ret = -ENXIO;
        return ret;
    }
    switch (reg) {
    case PMBUS_READ_VOUT:
        ret = wb_pmbus_read_word_data(client, page, phase, reg);
        DEBUG_INFO("READ_VOUT Value:%d %d %d\n", ret, data->vout_multiplier[0],
                    data->vout_multiplier[1]);
        ret = ((ret * data->vout_multiplier[0])/data->vout_multiplier[1]);
        break;
    default:
        ret = wb_pmbus_read_word_data(client, page, phase, reg);
        break;
    }
    return ret;
}


static struct pmbus_driver_info xdpe1x2g5_info[] = {
    [XDPE132G5C] = {
        .pages = XDPE132G5C_PAGE_NUM,
        .format[PSC_VOLTAGE_IN] = linear,
        .format[PSC_VOLTAGE_OUT] = linear,
        .format[PSC_TEMPERATURE] = linear,
        .format[PSC_CURRENT_IN] = linear,
        .format[PSC_CURRENT_OUT] = linear,
        .format[PSC_POWER] = linear,
        .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_IIN | PMBUS_HAVE_PIN
        | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_TEMP
        | PMBUS_HAVE_STATUS_TEMP
        | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
        | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT,
        .func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_IIN | PMBUS_HAVE_PIN
        | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_TEMP
        | PMBUS_HAVE_STATUS_TEMP
        | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
        | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT,
        .groups = xdpe132g5_attribute_groups,
        .identify = xdpe132g5c_identify,
    },
    [XDPE1A2G5B] = {
        .pages = XDPE132G5C_PAGE_NUM,
        .read_word_data = xdpe_chip_read_word_data,
        .format[PSC_VOLTAGE_IN] = linear,
        .format[PSC_VOLTAGE_OUT] = linear,
        .format[PSC_TEMPERATURE] = linear,
        .format[PSC_CURRENT_IN] = linear,
        .format[PSC_CURRENT_OUT] = linear,
        .format[PSC_POWER] = linear,
        .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .groups = xdpe132g5_attribute_groups,
    },
    [XDPE19284C] = {
        .pages = 2,
        .read_word_data = xdpe_chip_read_word_data,
        .format[PSC_VOLTAGE_IN] = linear,
        .format[PSC_VOLTAGE_OUT] = linear,
        .format[PSC_TEMPERATURE] = linear,
        .format[PSC_CURRENT_IN] = linear,
        .format[PSC_CURRENT_OUT] = linear,
        .format[PSC_POWER] = linear,
        .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .groups = xdpe132g5_attribute_groups,
    },
    [XDPE192C4B] = {
        .pages = 2,
        .read_word_data = xdpe_chip_read_word_data,
        .format[PSC_VOLTAGE_IN] = linear,
        .format[PSC_VOLTAGE_OUT] = linear,
        .format[PSC_TEMPERATURE] = linear,
        .format[PSC_CURRENT_IN] = linear,
        .format[PSC_CURRENT_OUT] = linear,
        .format[PSC_POWER] = linear,
        .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .groups = xdpe132g5_attribute_groups,
    },
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
static int xdpe132g5c_probe(struct i2c_client *client)
#else
static int xdpe132g5c_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
    const struct i2c_device_id *id = i2c_client_get_device_id(client);
#endif
    xdpe_chip_data_t *chip_data;
    struct pmbus_data *data;
    int ret;
    struct miscdevice *misc;

    /* 1. alloc mem and do init */
    chip_data = devm_kzalloc(&client->dev, sizeof(*chip_data), GFP_KERNEL);
    if (!chip_data) {
        dev_err(&client->dev, "devm_kzalloc fail\n");
        return -ENOMEM;
    }

    chip_data->id = id->driver_data;
    memcpy(&chip_data->info, &xdpe1x2g5_info[id->driver_data], sizeof(chip_data->info));

    ret = of_property_read_u32_array(client->dev.of_node, "vout_multiplier",
                                     chip_data->vout_multiplier, ARRAY_SIZE(chip_data->vout_multiplier));
    DEBUG_INFO("vout_multipplier:%d %d, ret:%d\n",
               chip_data->vout_multiplier[0], chip_data->vout_multiplier[1], ret);
    if (ret != 0) {
        /* get fail use default val */
        chip_data->vout_multiplier[0] = 0x01;
        chip_data->vout_multiplier[1] = 0x01;
    }

    if (chip_data->vout_multiplier[0] == 0 || chip_data->vout_multiplier[1] == 0) {
        dev_err(&client->dev, "vout_multiplier have 0. its invalid\n");
        return -EINVAL;
    }

    /* 2. pmbus do probe */
    ret = wb_pmbus_do_probe(client, &chip_data->info);
    if (ret != 0) {
        dev_info(&client->dev, "wb_pmbus_do_probe failed, ret: %d.\n", ret);
        return ret;
    }

    /* 3. init clinet data */
    chip_data->client = client;
    data = i2c_get_clientdata(client);
    data->pmbus_info_array = dfx_infos;
    data->pmbus_info_array_size = ARRAY_SIZE(dfx_infos);

    /* 4. create sysfs group */
    ret = sysfs_create_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
    if (ret != 0) {
        dev_info(&client->dev, "Failed to create xdpe132g5c_sysfs_attrs_group, ret: %d.\n", ret);
        wb_pmbus_do_remove(client);
        return ret;
    }

    ret = sysfs_chmod_file(&client->dev.kobj,
        &sensor_dev_attr_clear_faults.dev_attr.attr, 0222);
    if (ret != 0) {
        dev_warn(&client->dev, "Failed to chmod clear_faults, ret: %d.\n", ret);
    }

    /* 5. create xdpe1x2xx fw_upgrade misc device */
    snprintf(chip_data->misc_dev_name, sizeof(chip_data->misc_dev_name), "xdpe1x2xx_%d_0x%02x",
        client->adapter->nr, client->addr);
    misc = &chip_data->misc_dev;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = chip_data->misc_dev_name;
    misc->fops = &xdpe_dev_fops;
    if (misc_register(misc) != 0) {
        dev_err(&client->dev, "Failed to register %s\n", misc->name);
        sysfs_remove_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
        wb_pmbus_do_remove(client);
        return -ENXIO;
    }

    ret = add_dev_to_g_dev_list(chip_data);
    if (ret) {
        dev_err(&client->dev, "Failed to add_dev_to_g_dev_list, ret: %d\n", ret);
        misc_deregister(misc);
        sysfs_remove_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
        wb_pmbus_do_remove(client);
        return -EINVAL;
    }

    xdpe_chip_id_init(chip_data);
    dev_info(&client->dev, "Register %s with minor: %d success\n", misc->name, misc->minor);
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
static void xdpe132g5c_remove(struct i2c_client *client)
#else
static int xdpe132g5c_remove(struct i2c_client *client)
#endif
{
    int minor;
    xdpe_chip_data_t *chip_data = to_xdpe_chip_data(wb_pmbus_get_driver_info(client));

    minor = chip_data->misc_dev.minor;
    DEBUG_VERBOSE("misc_deregister %s, minor: %d\n", chip_data->misc_dev.name, minor);
    misc_deregister(&chip_data->misc_dev);
    remove_dev_from_g_dev_list(minor);

    sysfs_remove_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
    (void)wb_pmbus_do_remove(client);
    dev_info(&client->dev, "Remove xdpe1x2xx pmbus device success.\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
    return 0;
#endif
}

static const struct i2c_device_id xdpe132g5c_id[] = {
    {"wb_xdpe132g5c_pmbus", XDPE132G5C},
    {"wb_xdpe1a2g5b_pmbus", XDPE1A2G5B},
    {"wb_xdpe19284c_pmbus", XDPE19284C},
    {"wb_xdpe192c4b_pmbus", XDPE192C4B},
    {}
};

MODULE_DEVICE_TABLE(i2c, xdpe132g5c_id);

static const struct of_device_id __maybe_unused xdpe132g5c_of_match[] = {
    {.compatible = "infineon,wb_xdpe132g5c_pmbus", .data = (void *)XDPE132G5C},
    {.compatible = "infineon,wb_xdpe1a2g5b_pmbus", .data = (void *)XDPE1A2G5B},
    {.compatible = "infineon,wb_xdpe19284c_pmbus", .data = (void *)XDPE19284C},
    {.compatible = "infineon,wb_xdpe192c4b_pmbus", .data = (void *)XDPE192C4B},
    {}
};
MODULE_DEVICE_TABLE(of, xdpe132g5c_of_match);

static struct i2c_driver xdpe132g5c_driver = {
    .driver = {
        .name = "wb_xdpe132g5c_pmbus",
        .of_match_table = of_match_ptr(xdpe132g5c_of_match),
    },
    .probe = xdpe132g5c_probe,
    .remove = xdpe132g5c_remove,
    .id_table = xdpe132g5c_id,
};

module_i2c_driver(xdpe132g5c_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("PMBus driver for Infineon XDPE132g5 family");
MODULE_LICENSE("GPL");
