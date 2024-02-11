#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define DRIVER_VERSION  "1.0.2"

#define EEPROM_SIZE             0x100
#define SLICE_BITS              (6)
#define SLICE_SIZE              (1 << SLICE_BITS)
#define SLICE_NUM               (EEPROM_SIZE/SLICE_SIZE)

struct i2c_adap {
	int nr;
	char *name;
	const char *funcs;
	const char *algo;
};

struct eeprom_data {
        struct mutex update_lock;
        u8 valid;                       /* bitfield, bit!=0 if slice is valid */
        unsigned long last_updated[SLICE_NUM];  /* In jiffies, 8 slices */
        u8 data[EEPROM_SIZE];           /* Register values */
};

struct i2c_adap *gather_i2c_busses(void);
void free_adapters(struct i2c_adap *adapters);

/* compiler conditional */
/* end of compiler conditional */

/* Function Declaration */
/* i2c-0 */
ssize_t cpld_hw_ver_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t wdt_enable_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t wdt_enable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t eeprom_wp_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t eeprom_wp_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

ssize_t net_eeprom_read(struct file *filp, struct kobject *kobj, struct bin_attribute *bin_attr, char *buf, loff_t off, size_t count);

ssize_t usb_enable_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t usb_enable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t reset_mac_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t shutdown_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t bmc_enable_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t led_ctrl_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t led_ctrl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t led_fiber_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t led_fiber_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t thermal_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t thermal_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t thermal_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t sys_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sys_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sys_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t thermal_temp_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t thermal_temp_max_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t thermal_temp_min_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t thermal_temp_crit_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t thermal_temp_lcrit_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_ctrl_mode_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_ctrl_rpm_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_status_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_present_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_power_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_rpm_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_status_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_present_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_vin_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_iin_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_vout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_iout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_temp_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_fan_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_pout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_pin_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_mfr_model_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_iout_max_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_vmode_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_vout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_iout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_pout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_tx_enable_all_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_tx_enable_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t sfp_tx_enable_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_tx_enable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t sfp_rx_loss_all_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_rx_loss_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_rx_loss_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_rx_loss_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_rx_loss_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t sfp_present_all_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_present_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_present_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_present_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_present_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t qsfp_low_power_all_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t qsfp_low_power_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t qsfp_low_power_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t qsfp_low_power_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t qsfp_reset_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t qsfp_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t qsfp_present_all_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t qsfp_present_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t qsfp_int_all_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t qsfp_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t qsfp_quter_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t qsfp_quter_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t qsfp_quter_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t qsfp_modprs_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t qsfp_modprs_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t qsfp_modprs_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
/* end of Function Declaration */

/* struct i2c_data */
struct Netberg_i2c_data
{
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;
    unsigned long       last_updated;
    u8  status;
};

/* struct i2c_sysfs_attributes */
enum Netberg_i2c_sysfs_attributes
{
    CPLD_23_VER,
    CPLD_30_VER,
    CPLD_31_VER,
    CPLD_32_VER,
    WDT_EN,
    EEPROM_WP,
    USB_EN,
    SHUTDOWN_SET,
    RESET,
    BMC_PRESENT,
    LED_1,
    LED_2,
    LED_FLOW,
    LED_SYS,
    LED_FIBER,
    TEMP_TH0_INT,
    TEMP_TH1_INT,
    TEMP_TH3_INT,
    TEMP_TH2_INT,
    TEMP_TH0_INT_MASK,
    TEMP_TH1_INT_MASK,    
    TEMP_TH3_INT_MASK,    
    TEMP_TH2_INT_MASK,
    CPLD_FP_INT,
    CPLD_RP_INT,
    CPLD_FAN_INT,
    CPLD_PSU_INT,
    THERMAL_INT,
    USB_INT,
    CPLD_FP_INT_MASK,
    CPLD_RP_INT_MASK,
    CPLD_FAN_INT_MASK,
    CPLD_PSU_INT_MASK,
    THERMAL_INT_MASK,
    USB_INT_MASK,
    TEMP_TH0_T,
    TEMP_TH0_B,
    TEMP_TH0_R,
    TEMP_TH1_T,
    TEMP_TH1_B,
    TEMP_TH3_T,
    TEMP_TH3_B,
    TEMP_TH2_T,
    TEMP_TH2_B,
    TEMP_TH0_T_MAX,
    TEMP_TH1_T_MAX,
    TEMP_TH3_T_MAX,
    TEMP_TH2_T_MAX,
    TEMP_TH0_R_MAX,
    TEMP_TH0_B_MAX,
    TEMP_TH1_B_MAX,
    TEMP_TH3_B_MAX,
    TEMP_TH2_B_MAX,
    TEMP_TH0_T_MIN,
    TEMP_TH1_T_MIN,
    TEMP_TH3_T_MIN,
    TEMP_TH2_T_MIN,
    TEMP_TH0_R_MIN,
    TEMP_TH0_B_MIN,
    TEMP_TH1_B_MIN,
    TEMP_TH3_B_MIN,
    TEMP_TH2_B_MIN,
    TEMP_TH0_T_CRIT,
    TEMP_TH1_T_CRIT,
    TEMP_TH3_T_CRIT,
    TEMP_TH2_T_CRIT,
    TEMP_TH0_R_CRIT,
    TEMP_TH0_B_CRIT,
    TEMP_TH1_B_CRIT,
    TEMP_TH3_B_CRIT,
    TEMP_TH2_B_CRIT,
    TEMP_TH0_T_LCRIT,
    TEMP_TH1_T_LCRIT,
    TEMP_TH3_T_LCRIT,
    TEMP_TH2_T_LCRIT,
    TEMP_TH0_R_LCRIT,
    TEMP_TH0_B_LCRIT,
    TEMP_TH1_B_LCRIT,
    TEMP_TH3_B_LCRIT,
    TEMP_TH2_B_LCRIT,
    FANCTRL_RPM,
    FANCTRL_MODE,
    FAN1_STAT,
    FAN2_STAT,
    FAN3_STAT,
    FAN4_STAT,
    FAN1_PRESENT,
    FAN2_PRESENT,
    FAN3_PRESENT,
    FAN4_PRESENT,
    FAN1_POWER,
    FAN2_POWER,
    FAN3_POWER,
    FAN4_POWER,
    FAN1_FRONT_RPM,
    FAN2_FRONT_RPM,
    FAN3_FRONT_RPM,
    FAN4_FRONT_RPM,
    FAN1_REAR_RPM,
    FAN2_REAR_RPM,
    FAN3_REAR_RPM,
    FAN4_REAR_RPM,
    PSU1_GOOD,
    PSU2_GOOD,
    PSU1_PRNT,
    PSU2_PRNT,
    PSU1_VIN,
    PSU1_IIN,
    PSU1_VOUT,
    PSU1_IOUT,
    PSU1_TEMP,
    PSU1_FAN_SPEED,
    PSU1_POUT,
    PSU1_PIN,
    PSU1_MFR_MODEL,
    PSU1_MFR_IOUT_MAX,
    PSU1_VOMDE,
    PSU2_VIN,
    PSU2_IIN,
    PSU2_VOUT,
    PSU2_IOUT,
    PSU2_TEMP,
    PSU2_FAN_SPEED,
    PSU2_POUT,
    PSU2_PIN,
    PSU2_MFR_MODEL,
    PSU2_MFR_IOUT_MAX,
    PSU2_VOMDE,
    DC6E_P0_VOUT,
    DC6E_P0_IOUT,
    DC6E_P0_POUT,
    DC6E_P1_VOUT,
    DC6E_P1_IOUT,
    DC6E_P1_POUT,
    DC70_P0_VOUT,
    DC70_P0_IOUT,
    DC70_P0_POUT,
    DC70_P1_VOUT,
    DC70_P1_IOUT,
    DC70_P1_POUT,
    SFP_TX_ENABLE_ALL,
    SFP1_TX_ENABLE,
    SFP2_TX_ENABLE,
    SFP3_TX_ENABLE,
    SFP4_TX_ENABLE,
    SFP5_TX_ENABLE,
    SFP6_TX_ENABLE,
    SFP7_TX_ENABLE,
    SFP8_TX_ENABLE,
    SFP9_TX_ENABLE,
    SFP10_TX_ENABLE,
    SFP11_TX_ENABLE,
    SFP12_TX_ENABLE,
    SFP13_TX_ENABLE,
    SFP14_TX_ENABLE,
    SFP15_TX_ENABLE,
    SFP16_TX_ENABLE,
    SFP17_TX_ENABLE,
    SFP18_TX_ENABLE,
    SFP19_TX_ENABLE,
    SFP20_TX_ENABLE,
    SFP21_TX_ENABLE,
    SFP22_TX_ENABLE,
    SFP23_TX_ENABLE,
    SFP24_TX_ENABLE,
    SFP25_TX_ENABLE,
    SFP26_TX_ENABLE,
    SFP27_TX_ENABLE,
    SFP28_TX_ENABLE,
    SFP29_TX_ENABLE,
    SFP30_TX_ENABLE,
    SFP31_TX_ENABLE,
    SFP32_TX_ENABLE,
    SFP33_TX_ENABLE,
    SFP34_TX_ENABLE,
    SFP35_TX_ENABLE,
    SFP36_TX_ENABLE,
    SFP37_TX_ENABLE,
    SFP38_TX_ENABLE,
    SFP39_TX_ENABLE,
    SFP40_TX_ENABLE,
    SFP41_TX_ENABLE,
    SFP42_TX_ENABLE,
    SFP43_TX_ENABLE,
    SFP44_TX_ENABLE,
    SFP45_TX_ENABLE,
    SFP46_TX_ENABLE,
    SFP47_TX_ENABLE,
    SFP48_TX_ENABLE,
    SFP_RX_LOSS_ALL,
    SFP1_RX_LOSS,
    SFP2_RX_LOSS,
    SFP3_RX_LOSS,
    SFP4_RX_LOSS,
    SFP5_RX_LOSS,
    SFP6_RX_LOSS,
    SFP7_RX_LOSS,
    SFP8_RX_LOSS,
    SFP9_RX_LOSS,
    SFP10_RX_LOSS,
    SFP11_RX_LOSS,
    SFP12_RX_LOSS,
    SFP13_RX_LOSS,
    SFP14_RX_LOSS,
    SFP15_RX_LOSS,
    SFP16_RX_LOSS,
    SFP17_RX_LOSS,
    SFP18_RX_LOSS,
    SFP19_RX_LOSS,
    SFP20_RX_LOSS,
    SFP21_RX_LOSS,
    SFP22_RX_LOSS,
    SFP23_RX_LOSS,
    SFP24_RX_LOSS,
    SFP25_RX_LOSS,
    SFP26_RX_LOSS,
    SFP27_RX_LOSS,
    SFP28_RX_LOSS,
    SFP29_RX_LOSS,
    SFP30_RX_LOSS,
    SFP31_RX_LOSS,
    SFP32_RX_LOSS,
    SFP33_RX_LOSS,
    SFP34_RX_LOSS,
    SFP35_RX_LOSS,
    SFP36_RX_LOSS,
    SFP37_RX_LOSS,
    SFP38_RX_LOSS,
    SFP39_RX_LOSS,
    SFP40_RX_LOSS,
    SFP41_RX_LOSS,
    SFP42_RX_LOSS,
    SFP43_RX_LOSS,
    SFP44_RX_LOSS,
    SFP45_RX_LOSS,
    SFP46_RX_LOSS,
    SFP47_RX_LOSS,
    SFP48_RX_LOSS,
    SFP_1_6_RX_LOSS_INT,
    SFP_2_6_RX_LOSS_INT,
    SFP_3_6_RX_LOSS_INT,
    SFP_4_6_RX_LOSS_INT,
    SFP_5_6_RX_LOSS_INT,
    SFP_6_6_RX_LOSS_INT,
    SFP_1_6_RX_LOSS_INT_MASK,
    SFP_2_6_RX_LOSS_INT_MASK,
    SFP_3_6_RX_LOSS_INT_MASK,
    SFP_4_6_RX_LOSS_INT_MASK,
    SFP_5_6_RX_LOSS_INT_MASK,
    SFP_6_6_RX_LOSS_INT_MASK,
    SFP_PRESENT_ALL,
    SFP1_PRESENT,
    SFP2_PRESENT,
    SFP3_PRESENT,
    SFP4_PRESENT,
    SFP5_PRESENT,
    SFP6_PRESENT,
    SFP7_PRESENT,
    SFP8_PRESENT,
    SFP9_PRESENT,
    SFP10_PRESENT,
    SFP11_PRESENT,
    SFP12_PRESENT,
    SFP13_PRESENT,
    SFP14_PRESENT,
    SFP15_PRESENT,
    SFP16_PRESENT,
    SFP17_PRESENT,
    SFP18_PRESENT,
    SFP19_PRESENT,
    SFP20_PRESENT,
    SFP21_PRESENT,
    SFP22_PRESENT,
    SFP23_PRESENT,
    SFP24_PRESENT,
    SFP25_PRESENT,
    SFP26_PRESENT,
    SFP27_PRESENT,
    SFP28_PRESENT,
    SFP29_PRESENT,
    SFP30_PRESENT,
    SFP31_PRESENT,
    SFP32_PRESENT,
    SFP33_PRESENT,
    SFP34_PRESENT,
    SFP35_PRESENT,
    SFP36_PRESENT,
    SFP37_PRESENT,
    SFP38_PRESENT,
    SFP39_PRESENT,
    SFP40_PRESENT,
    SFP41_PRESENT,
    SFP42_PRESENT,
    SFP43_PRESENT,
    SFP44_PRESENT,
    SFP45_PRESENT,
    SFP46_PRESENT,
    SFP47_PRESENT,
    SFP48_PRESENT,
    SFP_1_6_PRESENT_INT,
    SFP_2_6_PRESENT_INT,
    SFP_3_6_PRESENT_INT,
    SFP_4_6_PRESENT_INT,
    SFP_5_6_PRESENT_INT,
    SFP_6_6_PRESENT_INT,
    SFP_1_6_PRESENT_INT_MASK,
    SFP_2_6_PRESENT_INT_MASK,
    SFP_3_6_PRESENT_INT_MASK,
    SFP_4_6_PRESENT_INT_MASK,
    SFP_5_6_PRESENT_INT_MASK,
    SFP_6_6_PRESENT_INT_MASK,
    QSFP_LOW_POWER_ALL,
    QSFP1_LOW_POWER,
    QSFP2_LOW_POWER,
    QSFP3_LOW_POWER,
    QSFP4_LOW_POWER,
    QSFP5_LOW_POWER,
    QSFP6_LOW_POWER,
    QSFP7_LOW_POWER,
    QSFP8_LOW_POWER,
    QSFP_RESET_ALL,
    QSFP1_RESET,
    QSFP2_RESET,
    QSFP3_RESET,
    QSFP4_RESET,
    QSFP5_RESET,
    QSFP6_RESET,
    QSFP7_RESET,
    QSFP8_RESET,
    QSFP_PRESENT_ALL,
    QSFP1_PRESENT,
    QSFP2_PRESENT,
    QSFP3_PRESENT,
    QSFP4_PRESENT,
    QSFP5_PRESENT,
    QSFP6_PRESENT,
    QSFP7_PRESENT,
    QSFP8_PRESENT,
    QSFP_INT_ALL,
    QSFP1_INT,
    QSFP2_INT,
    QSFP3_INT,
    QSFP4_INT,
    QSFP5_INT,
    QSFP6_INT,
    QSFP7_INT,
    QSFP8_INT,
    QSFP_INT,
    QSFP_MODPRS_INT,
    QSFP_INT_MASK,
    QSFP_MODPRS_MASK
};
/* end of struct i2c_sysfs_attributes */

/* sysfs attributes for SENSOR_DEVICE_ATTR */
static SENSOR_DEVICE_ATTR(cpld_23_ver           , S_IRUGO           , cpld_hw_ver_get          , NULL                     , 23);
static SENSOR_DEVICE_ATTR(cpld_30_ver           , S_IRUGO           , cpld_hw_ver_get          , NULL                     , 30);
static SENSOR_DEVICE_ATTR(cpld_31_ver           , S_IRUGO           , cpld_hw_ver_get          , NULL                     , 31);
static SENSOR_DEVICE_ATTR(cpld_32_ver           , S_IRUGO           , cpld_hw_ver_get          , NULL                     , 32);
static SENSOR_DEVICE_ATTR(wdt_en                , S_IRUGO | S_IWUSR , wdt_enable_get           , wdt_enable_set           , WDT_EN);
static SENSOR_DEVICE_ATTR(eeprom_wp             , S_IRUGO | S_IWUSR , eeprom_wp_get            , eeprom_wp_set            , EEPROM_WP);
static SENSOR_DEVICE_ATTR(usb_en                , S_IRUGO | S_IWUSR , usb_enable_get           , usb_enable_set           , USB_EN);
static SENSOR_DEVICE_ATTR(shutdown_set          , S_IRUGO | S_IWUSR , NULL                     , shutdown_set             , SHUTDOWN_SET);
static SENSOR_DEVICE_ATTR(reset                 , S_IRUGO | S_IWUSR , NULL                     , reset_mac_set            , RESET);
static SENSOR_DEVICE_ATTR(bmc_present           , S_IRUGO           , bmc_enable_get           , NULL                     , BMC_PRESENT);
static SENSOR_DEVICE_ATTR(led_1                 , S_IRUGO | S_IWUSR , led_ctrl_get             , led_ctrl_set             , 4);
static SENSOR_DEVICE_ATTR(led_2                 , S_IRUGO | S_IWUSR , led_ctrl_get             , led_ctrl_set             , 3);
static SENSOR_DEVICE_ATTR(led_flow              , S_IRUGO | S_IWUSR , led_ctrl_get             , led_ctrl_set             , 2);
static SENSOR_DEVICE_ATTR(led_sys               , S_IRUGO | S_IWUSR , led_ctrl_get             , led_ctrl_set             , 1);
static SENSOR_DEVICE_ATTR(led_fiber             , S_IRUGO | S_IWUSR , led_fiber_get            , led_fiber_set            , LED_FIBER);
static SENSOR_DEVICE_ATTR(temp_th0_int          , S_IRUGO           , thermal_int_get           , NULL                     , TEMP_TH0_INT);
static SENSOR_DEVICE_ATTR(temp_th1_int          , S_IRUGO           , thermal_int_get           , NULL                     , TEMP_TH1_INT);
static SENSOR_DEVICE_ATTR(temp_th3_int          , S_IRUGO           , thermal_int_get           , NULL                     , TEMP_TH3_INT);
static SENSOR_DEVICE_ATTR(temp_th2_int          , S_IRUGO           , thermal_int_get           , NULL                     , TEMP_TH2_INT);
static SENSOR_DEVICE_ATTR(temp_th0_int_mask     , S_IRUGO | S_IWUSR , thermal_int_mask_get      , thermal_int_mask_set      , TEMP_TH0_INT_MASK);
static SENSOR_DEVICE_ATTR(temp_th1_int_mask     , S_IRUGO | S_IWUSR , thermal_int_mask_get      , thermal_int_mask_set      , TEMP_TH1_INT_MASK);
static SENSOR_DEVICE_ATTR(temp_th3_int_mask     , S_IRUGO | S_IWUSR , thermal_int_mask_get      , thermal_int_mask_set      , TEMP_TH3_INT_MASK);
static SENSOR_DEVICE_ATTR(temp_th2_int_mask     , S_IRUGO | S_IWUSR , thermal_int_mask_get      , thermal_int_mask_set      , TEMP_TH2_INT_MASK);
static SENSOR_DEVICE_ATTR(cpld_fp_int           , S_IRUGO           , sys_int_get              , NULL                     , CPLD_FP_INT);
static SENSOR_DEVICE_ATTR(cpld_rp_int           , S_IRUGO           , sys_int_get              , NULL                     , CPLD_RP_INT);
static SENSOR_DEVICE_ATTR(cpld_fan_int          , S_IRUGO           , sys_int_get              , NULL                     , CPLD_FAN_INT);
static SENSOR_DEVICE_ATTR(cpld_psu_int          , S_IRUGO           , sys_int_get              , NULL                     , CPLD_PSU_INT);
static SENSOR_DEVICE_ATTR(thermal_int           , S_IRUGO           , sys_int_get              , NULL                     , THERMAL_INT);
static SENSOR_DEVICE_ATTR(usb_int               , S_IRUGO           , sys_int_get              , NULL                     , USB_INT);
static SENSOR_DEVICE_ATTR(cpld_fp_int_mask      , S_IRUGO | S_IWUSR , sys_int_mask_get         , sys_int_mask_set         , CPLD_FP_INT_MASK);
static SENSOR_DEVICE_ATTR(cpld_rp_int_mask      , S_IRUGO | S_IWUSR , sys_int_mask_get         , sys_int_mask_set         , CPLD_RP_INT_MASK);
static SENSOR_DEVICE_ATTR(cpld_fan_int_mask     , S_IRUGO | S_IWUSR , sys_int_mask_get         , sys_int_mask_set         , CPLD_FAN_INT_MASK);
static SENSOR_DEVICE_ATTR(cpld_psu_int_mask     , S_IRUGO | S_IWUSR , sys_int_mask_get         , sys_int_mask_set         , CPLD_PSU_INT_MASK);
static SENSOR_DEVICE_ATTR(thermal_int_mask      , S_IRUGO | S_IWUSR , sys_int_mask_get         , sys_int_mask_set         , THERMAL_INT_MASK);
static SENSOR_DEVICE_ATTR(usb_int_mask          , S_IRUGO | S_IWUSR , sys_int_mask_get         , sys_int_mask_set         , USB_INT_MASK);
static SENSOR_DEVICE_ATTR(temp_th0_t            , S_IRUGO           , thermal_temp_get          , NULL                     , TEMP_TH0_T);
static SENSOR_DEVICE_ATTR(temp_th0_b            , S_IRUGO           , thermal_temp_get          , NULL                     , TEMP_TH0_B);
static SENSOR_DEVICE_ATTR(temp_th0_r            , S_IRUGO           , thermal_temp_get          , NULL                     , TEMP_TH0_R);
static SENSOR_DEVICE_ATTR(temp_th1_t            , S_IRUGO           , thermal_temp_get          , NULL                     , TEMP_TH1_T);
static SENSOR_DEVICE_ATTR(temp_th1_b            , S_IRUGO           , thermal_temp_get          , NULL                     , TEMP_TH1_B);
static SENSOR_DEVICE_ATTR(temp_th3_t            , S_IRUGO           , thermal_temp_get          , NULL                     , TEMP_TH3_T);
static SENSOR_DEVICE_ATTR(temp_th3_b            , S_IRUGO           , thermal_temp_get          , NULL                     , TEMP_TH3_B);
static SENSOR_DEVICE_ATTR(temp_th2_t            , S_IRUGO           , thermal_temp_get          , NULL                     , TEMP_TH2_T);
static SENSOR_DEVICE_ATTR(temp_th2_b            , S_IRUGO           , thermal_temp_get          , NULL                     , TEMP_TH2_B);
static SENSOR_DEVICE_ATTR(temp_th0_t_max        , S_IRUGO           , thermal_temp_max_get      , NULL                     , TEMP_TH0_T_MAX);
static SENSOR_DEVICE_ATTR(temp_th1_t_max        , S_IRUGO           , thermal_temp_max_get      , NULL                     , TEMP_TH1_T_MAX);
static SENSOR_DEVICE_ATTR(temp_th3_t_max        , S_IRUGO           , thermal_temp_max_get      , NULL                     , TEMP_TH3_T_MAX);
static SENSOR_DEVICE_ATTR(temp_th2_t_max        , S_IRUGO           , thermal_temp_max_get      , NULL                     , TEMP_TH2_T_MAX);
static SENSOR_DEVICE_ATTR(temp_th0_r_max        , S_IRUGO           , thermal_temp_max_get      , NULL                     , TEMP_TH0_R_MAX);
static SENSOR_DEVICE_ATTR(temp_th0_b_max        , S_IRUGO           , thermal_temp_max_get      , NULL                     , TEMP_TH0_B_MAX);
static SENSOR_DEVICE_ATTR(temp_th1_b_max        , S_IRUGO           , thermal_temp_max_get      , NULL                     , TEMP_TH1_B_MAX);
static SENSOR_DEVICE_ATTR(temp_th3_b_max        , S_IRUGO           , thermal_temp_max_get      , NULL                     , TEMP_TH3_B_MAX);
static SENSOR_DEVICE_ATTR(temp_th2_b_max        , S_IRUGO           , thermal_temp_max_get      , NULL                     , TEMP_TH2_B_MAX);
static SENSOR_DEVICE_ATTR(temp_th0_t_min        , S_IRUGO           , thermal_temp_min_get      , NULL                     , TEMP_TH0_T_MIN);
static SENSOR_DEVICE_ATTR(temp_th1_t_min        , S_IRUGO           , thermal_temp_min_get      , NULL                     , TEMP_TH1_T_MIN);
static SENSOR_DEVICE_ATTR(temp_th3_t_min        , S_IRUGO           , thermal_temp_min_get      , NULL                     , TEMP_TH3_T_MIN);
static SENSOR_DEVICE_ATTR(temp_th2_t_min        , S_IRUGO           , thermal_temp_min_get      , NULL                     , TEMP_TH2_T_MIN);
static SENSOR_DEVICE_ATTR(temp_th0_r_min        , S_IRUGO           , thermal_temp_min_get      , NULL                     , TEMP_TH0_R_MIN);
static SENSOR_DEVICE_ATTR(temp_th0_b_min        , S_IRUGO           , thermal_temp_min_get      , NULL                     , TEMP_TH0_B_MIN);
static SENSOR_DEVICE_ATTR(temp_th1_b_min        , S_IRUGO           , thermal_temp_min_get      , NULL                     , TEMP_TH1_B_MIN);
static SENSOR_DEVICE_ATTR(temp_th3_b_min        , S_IRUGO           , thermal_temp_min_get      , NULL                     , TEMP_TH3_B_MIN);
static SENSOR_DEVICE_ATTR(temp_th2_b_min        , S_IRUGO           , thermal_temp_min_get      , NULL                     , TEMP_TH2_B_MIN);
static SENSOR_DEVICE_ATTR(temp_th0_t_crit       , S_IRUGO           , thermal_temp_crit_get     , NULL                     , TEMP_TH0_T_CRIT);
static SENSOR_DEVICE_ATTR(temp_th1_t_crit       , S_IRUGO           , thermal_temp_crit_get     , NULL                     , TEMP_TH1_T_CRIT);
static SENSOR_DEVICE_ATTR(temp_th3_t_crit       , S_IRUGO           , thermal_temp_crit_get     , NULL                     , TEMP_TH3_T_CRIT);
static SENSOR_DEVICE_ATTR(temp_th2_t_crit       , S_IRUGO           , thermal_temp_crit_get     , NULL                     , TEMP_TH2_T_CRIT);
static SENSOR_DEVICE_ATTR(temp_th0_r_crit       , S_IRUGO           , thermal_temp_crit_get     , NULL                     , TEMP_TH0_R_CRIT);
static SENSOR_DEVICE_ATTR(temp_th0_b_crit       , S_IRUGO           , thermal_temp_crit_get     , NULL                     , TEMP_TH0_B_CRIT);
static SENSOR_DEVICE_ATTR(temp_th1_b_crit       , S_IRUGO           , thermal_temp_crit_get     , NULL                     , TEMP_TH1_B_CRIT);
static SENSOR_DEVICE_ATTR(temp_th3_b_crit       , S_IRUGO           , thermal_temp_crit_get     , NULL                     , TEMP_TH3_B_CRIT);
static SENSOR_DEVICE_ATTR(temp_th2_b_crit       , S_IRUGO           , thermal_temp_crit_get     , NULL                     , TEMP_TH2_B_CRIT);
static SENSOR_DEVICE_ATTR(temp_th0_t_lcrit      , S_IRUGO           , thermal_temp_lcrit_get    , NULL                     , TEMP_TH0_T_LCRIT);
static SENSOR_DEVICE_ATTR(temp_th1_t_lcrit      , S_IRUGO           , thermal_temp_lcrit_get    , NULL                     , TEMP_TH1_T_LCRIT);
static SENSOR_DEVICE_ATTR(temp_th3_t_lcrit      , S_IRUGO           , thermal_temp_lcrit_get    , NULL                     , TEMP_TH3_T_LCRIT);
static SENSOR_DEVICE_ATTR(temp_th2_t_lcrit      , S_IRUGO           , thermal_temp_lcrit_get    , NULL                     , TEMP_TH2_T_LCRIT);
static SENSOR_DEVICE_ATTR(temp_th0_r_lcrit      , S_IRUGO           , thermal_temp_lcrit_get    , NULL                     , TEMP_TH0_R_LCRIT);
static SENSOR_DEVICE_ATTR(temp_th0_b_lcrit      , S_IRUGO           , thermal_temp_lcrit_get    , NULL                     , TEMP_TH0_B_LCRIT);
static SENSOR_DEVICE_ATTR(temp_th1_b_lcrit      , S_IRUGO           , thermal_temp_lcrit_get    , NULL                     , TEMP_TH1_B_LCRIT);
static SENSOR_DEVICE_ATTR(temp_th3_b_lcrit      , S_IRUGO           , thermal_temp_lcrit_get    , NULL                     , TEMP_TH3_B_LCRIT);
static SENSOR_DEVICE_ATTR(temp_th2_b_lcrit      , S_IRUGO           , thermal_temp_lcrit_get    , NULL                     , TEMP_TH2_B_LCRIT);
static SENSOR_DEVICE_ATTR(fanctrl_rpm           , S_IRUGO           , fan_ctrl_rpm_get         , NULL                     , FANCTRL_RPM);
static SENSOR_DEVICE_ATTR(fanctrl_mode          , S_IRUGO           , fan_ctrl_mode_get        , NULL                     , FANCTRL_MODE);
static SENSOR_DEVICE_ATTR(fan1_stat             , S_IRUGO           , fan_status_get           , NULL                     , 1);
static SENSOR_DEVICE_ATTR(fan2_stat             , S_IRUGO           , fan_status_get           , NULL                     , 2);
static SENSOR_DEVICE_ATTR(fan3_stat             , S_IRUGO           , fan_status_get           , NULL                     , 3);
static SENSOR_DEVICE_ATTR(fan4_stat             , S_IRUGO           , fan_status_get           , NULL                     , 4);
static SENSOR_DEVICE_ATTR(fan1_present          , S_IRUGO           , fan_present_get          , NULL                     , 1);
static SENSOR_DEVICE_ATTR(fan2_present          , S_IRUGO           , fan_present_get          , NULL                     , 2);
static SENSOR_DEVICE_ATTR(fan3_present          , S_IRUGO           , fan_present_get          , NULL                     , 3);
static SENSOR_DEVICE_ATTR(fan4_present          , S_IRUGO           , fan_present_get          , NULL                     , 4);
static SENSOR_DEVICE_ATTR(fan1_power            , S_IRUGO           , fan_power_get            , NULL                     , 1);
static SENSOR_DEVICE_ATTR(fan2_power            , S_IRUGO           , fan_power_get            , NULL                     , 2);
static SENSOR_DEVICE_ATTR(fan3_power            , S_IRUGO           , fan_power_get            , NULL                     , 3);
static SENSOR_DEVICE_ATTR(fan4_power            , S_IRUGO           , fan_power_get            , NULL                     , 4);
static SENSOR_DEVICE_ATTR(fan1_front_rpm        , S_IRUGO           , fan_rpm_get              , NULL                     , FAN1_FRONT_RPM);
static SENSOR_DEVICE_ATTR(fan2_front_rpm        , S_IRUGO           , fan_rpm_get              , NULL                     , FAN2_FRONT_RPM);
static SENSOR_DEVICE_ATTR(fan3_front_rpm        , S_IRUGO           , fan_rpm_get              , NULL                     , FAN3_FRONT_RPM);
static SENSOR_DEVICE_ATTR(fan4_front_rpm        , S_IRUGO           , fan_rpm_get              , NULL                     , FAN4_FRONT_RPM);
static SENSOR_DEVICE_ATTR(fan1_rear_rpm         , S_IRUGO           , fan_rpm_get              , NULL                     , FAN1_REAR_RPM);
static SENSOR_DEVICE_ATTR(fan2_rear_rpm         , S_IRUGO           , fan_rpm_get              , NULL                     , FAN2_REAR_RPM);
static SENSOR_DEVICE_ATTR(fan3_rear_rpm         , S_IRUGO           , fan_rpm_get              , NULL                     , FAN3_REAR_RPM);
static SENSOR_DEVICE_ATTR(fan4_rear_rpm         , S_IRUGO           , fan_rpm_get              , NULL                     , FAN4_REAR_RPM);
static SENSOR_DEVICE_ATTR(psu1_good             , S_IRUGO           , psu_status_get           , NULL                     , 1);
static SENSOR_DEVICE_ATTR(psu2_good             , S_IRUGO           , psu_status_get           , NULL                     , 2);
static SENSOR_DEVICE_ATTR(psu1_prnt             , S_IRUGO           , psu_present_get          , NULL                     , 1);
static SENSOR_DEVICE_ATTR(psu2_prnt             , S_IRUGO           , psu_present_get          , NULL                     , 2);
static SENSOR_DEVICE_ATTR(psu1_vin              , S_IRUGO           , psu_vin_get              , NULL                     , PSU1_VIN);
static SENSOR_DEVICE_ATTR(psu1_iin              , S_IRUGO           , psu_iin_get              , NULL                     , PSU1_IIN);
static SENSOR_DEVICE_ATTR(psu1_vout             , S_IRUGO           , psu_vout_get             , NULL                     , PSU1_VOUT);
static SENSOR_DEVICE_ATTR(psu1_iout             , S_IRUGO           , psu_iout_get             , NULL                     , PSU1_IOUT);
static SENSOR_DEVICE_ATTR(psu1_temp             , S_IRUGO           , psu_temp_get             , NULL                     , PSU1_TEMP);
static SENSOR_DEVICE_ATTR(psu1_fan_speed        , S_IRUGO           , psu_fan_get              , NULL                     , PSU1_FAN_SPEED);
static SENSOR_DEVICE_ATTR(psu1_pout             , S_IRUGO           , psu_pout_get             , NULL                     , PSU1_POUT);
static SENSOR_DEVICE_ATTR(psu1_pin              , S_IRUGO           , psu_pin_get              , NULL                     , PSU1_PIN);
static SENSOR_DEVICE_ATTR(psu1_mfr_model        , S_IRUGO           , psu_mfr_model_get        , NULL                     , PSU1_MFR_MODEL);
static SENSOR_DEVICE_ATTR(psu1_mfr_iout_max     , S_IRUGO           , psu_iout_max_get         , NULL                     , PSU1_MFR_IOUT_MAX);
static SENSOR_DEVICE_ATTR(psu1_vomde            , S_IRUGO           , psu_vmode_get            , NULL                     , PSU1_VOMDE);
static SENSOR_DEVICE_ATTR(psu2_vin              , S_IRUGO           , psu_vin_get              , NULL                     , PSU2_VIN);
static SENSOR_DEVICE_ATTR(psu2_iin              , S_IRUGO           , psu_iin_get              , NULL                     , PSU2_IIN);
static SENSOR_DEVICE_ATTR(psu2_vout             , S_IRUGO           , psu_vout_get             , NULL                     , PSU2_VOUT);
static SENSOR_DEVICE_ATTR(psu2_iout             , S_IRUGO           , psu_iout_get             , NULL                     , PSU2_IOUT);
static SENSOR_DEVICE_ATTR(psu2_temp             , S_IRUGO           , psu_temp_get             , NULL                     , PSU2_TEMP);
static SENSOR_DEVICE_ATTR(psu2_fan_speed        , S_IRUGO           , psu_fan_get              , NULL                     , PSU2_FAN_SPEED);
static SENSOR_DEVICE_ATTR(psu2_pout             , S_IRUGO           , psu_pout_get             , NULL                     , PSU2_POUT);
static SENSOR_DEVICE_ATTR(psu2_pin              , S_IRUGO           , psu_pin_get              , NULL                     , PSU2_PIN);
static SENSOR_DEVICE_ATTR(psu2_mfr_model        , S_IRUGO           , psu_mfr_model_get        , NULL                     , PSU2_MFR_MODEL);
static SENSOR_DEVICE_ATTR(psu2_mfr_iout_max     , S_IRUGO           , psu_iout_max_get         , NULL                     , PSU2_MFR_IOUT_MAX);
static SENSOR_DEVICE_ATTR(psu2_vomde            , S_IRUGO           , psu_vmode_get            , NULL                     , PSU2_VOMDE);
static SENSOR_DEVICE_ATTR(dc6e_p0_vout          , S_IRUGO           , dc_vout_get              , NULL                     , DC6E_P0_VOUT);
static SENSOR_DEVICE_ATTR(dc6e_p0_iout          , S_IRUGO           , dc_iout_get              , NULL                     , DC6E_P0_IOUT);
static SENSOR_DEVICE_ATTR(dc6e_p0_pout          , S_IRUGO           , dc_pout_get              , NULL                     , DC6E_P0_POUT);
static SENSOR_DEVICE_ATTR(dc6e_p1_vout          , S_IRUGO           , dc_vout_get              , NULL                     , DC6E_P1_VOUT);
static SENSOR_DEVICE_ATTR(dc6e_p1_iout          , S_IRUGO           , dc_iout_get              , NULL                     , DC6E_P1_IOUT);
static SENSOR_DEVICE_ATTR(dc6e_p1_pout          , S_IRUGO           , dc_pout_get              , NULL                     , DC6E_P1_POUT);
static SENSOR_DEVICE_ATTR(dc70_p0_vout          , S_IRUGO           , dc_vout_get              , NULL                     , DC70_P0_VOUT);
static SENSOR_DEVICE_ATTR(dc70_p0_iout          , S_IRUGO           , dc_iout_get              , NULL                     , DC70_P0_IOUT);
static SENSOR_DEVICE_ATTR(dc70_p0_pout          , S_IRUGO           , dc_pout_get              , NULL                     , DC70_P0_POUT);
static SENSOR_DEVICE_ATTR(dc70_p1_vout          , S_IRUGO           , dc_vout_get              , NULL                     , DC70_P1_VOUT);
static SENSOR_DEVICE_ATTR(dc70_p1_iout          , S_IRUGO           , dc_iout_get              , NULL                     , DC70_P1_IOUT);
static SENSOR_DEVICE_ATTR(dc70_p1_pout          , S_IRUGO           , dc_pout_get              , NULL                     , DC70_P1_POUT);
static SENSOR_DEVICE_ATTR(sfp_tx_enable_all     , S_IRUGO | S_IWUSR , sfp_tx_enable_all_get    , sfp_tx_enable_all_set    , SFP_TX_ENABLE_ALL);
static SENSOR_DEVICE_ATTR(sfp1_tx_enable        , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 1);
static SENSOR_DEVICE_ATTR(sfp2_tx_enable        , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 2);
static SENSOR_DEVICE_ATTR(sfp3_tx_enable        , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 3);
static SENSOR_DEVICE_ATTR(sfp4_tx_enable        , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 4);
static SENSOR_DEVICE_ATTR(sfp5_tx_enable        , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 5);
static SENSOR_DEVICE_ATTR(sfp6_tx_enable        , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 6);
static SENSOR_DEVICE_ATTR(sfp7_tx_enable        , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 7);
static SENSOR_DEVICE_ATTR(sfp8_tx_enable        , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 8);
static SENSOR_DEVICE_ATTR(sfp9_tx_enable        , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 9);
static SENSOR_DEVICE_ATTR(sfp10_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 10);
static SENSOR_DEVICE_ATTR(sfp11_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 11);
static SENSOR_DEVICE_ATTR(sfp12_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 12);
static SENSOR_DEVICE_ATTR(sfp13_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 13);
static SENSOR_DEVICE_ATTR(sfp14_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 14);
static SENSOR_DEVICE_ATTR(sfp15_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 15);
static SENSOR_DEVICE_ATTR(sfp16_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 16);
static SENSOR_DEVICE_ATTR(sfp17_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 17);
static SENSOR_DEVICE_ATTR(sfp18_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 18);
static SENSOR_DEVICE_ATTR(sfp19_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 19);
static SENSOR_DEVICE_ATTR(sfp20_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 20);
static SENSOR_DEVICE_ATTR(sfp21_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 21);
static SENSOR_DEVICE_ATTR(sfp22_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 22);
static SENSOR_DEVICE_ATTR(sfp23_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 23);
static SENSOR_DEVICE_ATTR(sfp24_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 24);
static SENSOR_DEVICE_ATTR(sfp25_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 25);
static SENSOR_DEVICE_ATTR(sfp26_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 26);
static SENSOR_DEVICE_ATTR(sfp27_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 27);
static SENSOR_DEVICE_ATTR(sfp28_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 28);
static SENSOR_DEVICE_ATTR(sfp29_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 29);
static SENSOR_DEVICE_ATTR(sfp30_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 30);
static SENSOR_DEVICE_ATTR(sfp31_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 31);
static SENSOR_DEVICE_ATTR(sfp32_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 32);
static SENSOR_DEVICE_ATTR(sfp33_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 33);
static SENSOR_DEVICE_ATTR(sfp34_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 34);
static SENSOR_DEVICE_ATTR(sfp35_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 35);
static SENSOR_DEVICE_ATTR(sfp36_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 36);
static SENSOR_DEVICE_ATTR(sfp37_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 37);
static SENSOR_DEVICE_ATTR(sfp38_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 38);
static SENSOR_DEVICE_ATTR(sfp39_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 39);
static SENSOR_DEVICE_ATTR(sfp40_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 40);
static SENSOR_DEVICE_ATTR(sfp41_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 41);
static SENSOR_DEVICE_ATTR(sfp42_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 42);
static SENSOR_DEVICE_ATTR(sfp43_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 43);
static SENSOR_DEVICE_ATTR(sfp44_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 44);
static SENSOR_DEVICE_ATTR(sfp45_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 45);
static SENSOR_DEVICE_ATTR(sfp46_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 46);
static SENSOR_DEVICE_ATTR(sfp47_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 47);
static SENSOR_DEVICE_ATTR(sfp48_tx_enable       , S_IRUGO | S_IWUSR , sfp_tx_enable_get        , sfp_tx_enable_set        , 48);
static SENSOR_DEVICE_ATTR(sfp_rx_loss_all       , S_IRUGO           , sfp_rx_loss_all_get      , NULL                     , SFP_RX_LOSS_ALL);
static SENSOR_DEVICE_ATTR(sfp1_rx_loss          , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 1);
static SENSOR_DEVICE_ATTR(sfp2_rx_loss          , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 2);
static SENSOR_DEVICE_ATTR(sfp3_rx_loss          , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 3);
static SENSOR_DEVICE_ATTR(sfp4_rx_loss          , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 4);
static SENSOR_DEVICE_ATTR(sfp5_rx_loss          , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 5);
static SENSOR_DEVICE_ATTR(sfp6_rx_loss          , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 6);
static SENSOR_DEVICE_ATTR(sfp7_rx_loss          , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 7);
static SENSOR_DEVICE_ATTR(sfp8_rx_loss          , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 8);
static SENSOR_DEVICE_ATTR(sfp9_rx_loss          , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 9);
static SENSOR_DEVICE_ATTR(sfp10_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 10);
static SENSOR_DEVICE_ATTR(sfp11_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 11);
static SENSOR_DEVICE_ATTR(sfp12_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 12);
static SENSOR_DEVICE_ATTR(sfp13_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 13);
static SENSOR_DEVICE_ATTR(sfp14_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 14);
static SENSOR_DEVICE_ATTR(sfp15_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 15);
static SENSOR_DEVICE_ATTR(sfp16_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 16);
static SENSOR_DEVICE_ATTR(sfp17_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 17);
static SENSOR_DEVICE_ATTR(sfp18_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 18);
static SENSOR_DEVICE_ATTR(sfp19_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 19);
static SENSOR_DEVICE_ATTR(sfp20_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 20);
static SENSOR_DEVICE_ATTR(sfp21_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 21);
static SENSOR_DEVICE_ATTR(sfp22_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 22);
static SENSOR_DEVICE_ATTR(sfp23_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 23);
static SENSOR_DEVICE_ATTR(sfp24_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 24);
static SENSOR_DEVICE_ATTR(sfp25_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 25);
static SENSOR_DEVICE_ATTR(sfp26_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 26);
static SENSOR_DEVICE_ATTR(sfp27_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 27);
static SENSOR_DEVICE_ATTR(sfp28_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 28);
static SENSOR_DEVICE_ATTR(sfp29_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 29);
static SENSOR_DEVICE_ATTR(sfp30_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 30);
static SENSOR_DEVICE_ATTR(sfp31_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 31);
static SENSOR_DEVICE_ATTR(sfp32_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 32);
static SENSOR_DEVICE_ATTR(sfp33_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 33);
static SENSOR_DEVICE_ATTR(sfp34_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 34);
static SENSOR_DEVICE_ATTR(sfp35_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 35);
static SENSOR_DEVICE_ATTR(sfp36_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 36);
static SENSOR_DEVICE_ATTR(sfp37_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 37);
static SENSOR_DEVICE_ATTR(sfp38_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 38);
static SENSOR_DEVICE_ATTR(sfp39_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 39);
static SENSOR_DEVICE_ATTR(sfp40_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 40);
static SENSOR_DEVICE_ATTR(sfp41_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 41);
static SENSOR_DEVICE_ATTR(sfp42_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 42);
static SENSOR_DEVICE_ATTR(sfp43_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 43);
static SENSOR_DEVICE_ATTR(sfp44_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 44);
static SENSOR_DEVICE_ATTR(sfp45_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 45);
static SENSOR_DEVICE_ATTR(sfp46_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 46);
static SENSOR_DEVICE_ATTR(sfp47_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 47);
static SENSOR_DEVICE_ATTR(sfp48_rx_loss         , S_IRUGO           , sfp_rx_loss_get          , NULL                     , 48);
static SENSOR_DEVICE_ATTR(sfp_1_6_rx_loss_int     , S_IRUGO           , sfp_rx_loss_int_get      , NULL                     , 1);
static SENSOR_DEVICE_ATTR(sfp_2_6_rx_loss_int     , S_IRUGO           , sfp_rx_loss_int_get      , NULL                     , 2);
static SENSOR_DEVICE_ATTR(sfp_3_6_rx_loss_int     , S_IRUGO           , sfp_rx_loss_int_get      , NULL                     , 3);
static SENSOR_DEVICE_ATTR(sfp_4_6_rx_loss_int     , S_IRUGO           , sfp_rx_loss_int_get      , NULL                     , 4);
static SENSOR_DEVICE_ATTR(sfp_5_6_rx_loss_int     , S_IRUGO           , sfp_rx_loss_int_get      , NULL                     , 5);
static SENSOR_DEVICE_ATTR(sfp_6_6_rx_loss_int     , S_IRUGO           , sfp_rx_loss_int_get      , NULL                     , 6);
static SENSOR_DEVICE_ATTR(sfp_1_6_rx_loss_int_mask, S_IRUGO | S_IWUSR , sfp_rx_loss_int_mask_get , sfp_rx_loss_int_mask_set , 1);
static SENSOR_DEVICE_ATTR(sfp_2_6_rx_loss_int_mask, S_IRUGO | S_IWUSR , sfp_rx_loss_int_mask_get , sfp_rx_loss_int_mask_set , 2);
static SENSOR_DEVICE_ATTR(sfp_3_6_rx_loss_int_mask, S_IRUGO | S_IWUSR , sfp_rx_loss_int_mask_get , sfp_rx_loss_int_mask_set , 3);
static SENSOR_DEVICE_ATTR(sfp_4_6_rx_loss_int_mask, S_IRUGO | S_IWUSR , sfp_rx_loss_int_mask_get , sfp_rx_loss_int_mask_set , 4);
static SENSOR_DEVICE_ATTR(sfp_5_6_rx_loss_int_mask, S_IRUGO | S_IWUSR , sfp_rx_loss_int_mask_get , sfp_rx_loss_int_mask_set , 5);
static SENSOR_DEVICE_ATTR(sfp_6_6_rx_loss_int_mask, S_IRUGO | S_IWUSR , sfp_rx_loss_int_mask_get , sfp_rx_loss_int_mask_set , 6);
static SENSOR_DEVICE_ATTR(sfp_present_all       , S_IRUGO           , sfp_present_all_get      , NULL                     , SFP_PRESENT_ALL);
static SENSOR_DEVICE_ATTR(sfp1_present          , S_IRUGO           , sfp_present_get          , NULL                     , 1);
static SENSOR_DEVICE_ATTR(sfp2_present          , S_IRUGO           , sfp_present_get          , NULL                     , 2);
static SENSOR_DEVICE_ATTR(sfp3_present          , S_IRUGO           , sfp_present_get          , NULL                     , 3);
static SENSOR_DEVICE_ATTR(sfp4_present          , S_IRUGO           , sfp_present_get          , NULL                     , 4);
static SENSOR_DEVICE_ATTR(sfp5_present          , S_IRUGO           , sfp_present_get          , NULL                     , 5);
static SENSOR_DEVICE_ATTR(sfp6_present          , S_IRUGO           , sfp_present_get          , NULL                     , 6);
static SENSOR_DEVICE_ATTR(sfp7_present          , S_IRUGO           , sfp_present_get          , NULL                     , 7);
static SENSOR_DEVICE_ATTR(sfp8_present          , S_IRUGO           , sfp_present_get          , NULL                     , 8);
static SENSOR_DEVICE_ATTR(sfp9_present          , S_IRUGO           , sfp_present_get          , NULL                     , 9);
static SENSOR_DEVICE_ATTR(sfp10_present         , S_IRUGO           , sfp_present_get          , NULL                     , 10);
static SENSOR_DEVICE_ATTR(sfp11_present         , S_IRUGO           , sfp_present_get          , NULL                     , 11);
static SENSOR_DEVICE_ATTR(sfp12_present         , S_IRUGO           , sfp_present_get          , NULL                     , 12);
static SENSOR_DEVICE_ATTR(sfp13_present         , S_IRUGO           , sfp_present_get          , NULL                     , 13);
static SENSOR_DEVICE_ATTR(sfp14_present         , S_IRUGO           , sfp_present_get          , NULL                     , 14);
static SENSOR_DEVICE_ATTR(sfp15_present         , S_IRUGO           , sfp_present_get          , NULL                     , 15);
static SENSOR_DEVICE_ATTR(sfp16_present         , S_IRUGO           , sfp_present_get          , NULL                     , 16);
static SENSOR_DEVICE_ATTR(sfp17_present         , S_IRUGO           , sfp_present_get          , NULL                     , 17);
static SENSOR_DEVICE_ATTR(sfp18_present         , S_IRUGO           , sfp_present_get          , NULL                     , 18);
static SENSOR_DEVICE_ATTR(sfp19_present         , S_IRUGO           , sfp_present_get          , NULL                     , 19);
static SENSOR_DEVICE_ATTR(sfp20_present         , S_IRUGO           , sfp_present_get          , NULL                     , 20);
static SENSOR_DEVICE_ATTR(sfp21_present         , S_IRUGO           , sfp_present_get          , NULL                     , 21);
static SENSOR_DEVICE_ATTR(sfp22_present         , S_IRUGO           , sfp_present_get          , NULL                     , 22);
static SENSOR_DEVICE_ATTR(sfp23_present         , S_IRUGO           , sfp_present_get          , NULL                     , 23);
static SENSOR_DEVICE_ATTR(sfp24_present         , S_IRUGO           , sfp_present_get          , NULL                     , 24);
static SENSOR_DEVICE_ATTR(sfp25_present         , S_IRUGO           , sfp_present_get          , NULL                     , 25);
static SENSOR_DEVICE_ATTR(sfp26_present         , S_IRUGO           , sfp_present_get          , NULL                     , 26);
static SENSOR_DEVICE_ATTR(sfp27_present         , S_IRUGO           , sfp_present_get          , NULL                     , 27);
static SENSOR_DEVICE_ATTR(sfp28_present         , S_IRUGO           , sfp_present_get          , NULL                     , 28);
static SENSOR_DEVICE_ATTR(sfp29_present         , S_IRUGO           , sfp_present_get          , NULL                     , 29);
static SENSOR_DEVICE_ATTR(sfp30_present         , S_IRUGO           , sfp_present_get          , NULL                     , 30);
static SENSOR_DEVICE_ATTR(sfp31_present         , S_IRUGO           , sfp_present_get          , NULL                     , 31);
static SENSOR_DEVICE_ATTR(sfp32_present         , S_IRUGO           , sfp_present_get          , NULL                     , 32);
static SENSOR_DEVICE_ATTR(sfp33_present         , S_IRUGO           , sfp_present_get          , NULL                     , 33);
static SENSOR_DEVICE_ATTR(sfp34_present         , S_IRUGO           , sfp_present_get          , NULL                     , 34);
static SENSOR_DEVICE_ATTR(sfp35_present         , S_IRUGO           , sfp_present_get          , NULL                     , 35);
static SENSOR_DEVICE_ATTR(sfp36_present         , S_IRUGO           , sfp_present_get          , NULL                     , 36);
static SENSOR_DEVICE_ATTR(sfp37_present         , S_IRUGO           , sfp_present_get          , NULL                     , 37);
static SENSOR_DEVICE_ATTR(sfp38_present         , S_IRUGO           , sfp_present_get          , NULL                     , 38);
static SENSOR_DEVICE_ATTR(sfp39_present         , S_IRUGO           , sfp_present_get          , NULL                     , 39);
static SENSOR_DEVICE_ATTR(sfp40_present         , S_IRUGO           , sfp_present_get          , NULL                     , 40);
static SENSOR_DEVICE_ATTR(sfp41_present         , S_IRUGO           , sfp_present_get          , NULL                     , 41);
static SENSOR_DEVICE_ATTR(sfp42_present         , S_IRUGO           , sfp_present_get          , NULL                     , 42);
static SENSOR_DEVICE_ATTR(sfp43_present         , S_IRUGO           , sfp_present_get          , NULL                     , 43);
static SENSOR_DEVICE_ATTR(sfp44_present         , S_IRUGO           , sfp_present_get          , NULL                     , 44);
static SENSOR_DEVICE_ATTR(sfp45_present         , S_IRUGO           , sfp_present_get          , NULL                     , 45);
static SENSOR_DEVICE_ATTR(sfp46_present         , S_IRUGO           , sfp_present_get          , NULL                     , 46);
static SENSOR_DEVICE_ATTR(sfp47_present         , S_IRUGO           , sfp_present_get          , NULL                     , 47);
static SENSOR_DEVICE_ATTR(sfp48_present         , S_IRUGO           , sfp_present_get          , NULL                     , 48);
static SENSOR_DEVICE_ATTR(sfp_1_6_present_int     , S_IRUGO           , sfp_present_int_get      , NULL                     , 1);
static SENSOR_DEVICE_ATTR(sfp_2_6_present_int     , S_IRUGO           , sfp_present_int_get      , NULL                     , 2);
static SENSOR_DEVICE_ATTR(sfp_3_6_present_int     , S_IRUGO           , sfp_present_int_get      , NULL                     , 3);
static SENSOR_DEVICE_ATTR(sfp_4_6_present_int     , S_IRUGO           , sfp_present_int_get      , NULL                     , 4);
static SENSOR_DEVICE_ATTR(sfp_5_6_present_int     , S_IRUGO           , sfp_present_int_get      , NULL                     , 5);
static SENSOR_DEVICE_ATTR(sfp_6_6_present_int     , S_IRUGO           , sfp_present_int_get      , NULL                     , 6);
static SENSOR_DEVICE_ATTR(sfp_1_6_present_int_mask, S_IRUGO | S_IWUSR , sfp_present_int_mask_get , sfp_present_int_mask_set , 1);
static SENSOR_DEVICE_ATTR(sfp_2_6_present_int_mask, S_IRUGO | S_IWUSR , sfp_present_int_mask_get , sfp_present_int_mask_set , 2);
static SENSOR_DEVICE_ATTR(sfp_3_6_present_int_mask, S_IRUGO | S_IWUSR , sfp_present_int_mask_get , sfp_present_int_mask_set , 3);
static SENSOR_DEVICE_ATTR(sfp_4_6_present_int_mask, S_IRUGO | S_IWUSR , sfp_present_int_mask_get , sfp_present_int_mask_set , 4);
static SENSOR_DEVICE_ATTR(sfp_5_6_present_int_mask, S_IRUGO | S_IWUSR , sfp_present_int_mask_get , sfp_present_int_mask_set , 5);
static SENSOR_DEVICE_ATTR(sfp_6_6_present_int_mask, S_IRUGO | S_IWUSR , sfp_present_int_mask_get , sfp_present_int_mask_set , 6);
static SENSOR_DEVICE_ATTR(qsfp_low_power_all    , S_IRUGO | S_IWUSR , qsfp_low_power_all_get   , qsfp_low_power_all_set   , QSFP_LOW_POWER_ALL);
static SENSOR_DEVICE_ATTR(qsfp1_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 1);
static SENSOR_DEVICE_ATTR(qsfp2_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 2);
static SENSOR_DEVICE_ATTR(qsfp3_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 3);
static SENSOR_DEVICE_ATTR(qsfp4_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 4);
static SENSOR_DEVICE_ATTR(qsfp5_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 5);
static SENSOR_DEVICE_ATTR(qsfp6_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 6);
static SENSOR_DEVICE_ATTR(qsfp7_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 7);
static SENSOR_DEVICE_ATTR(qsfp8_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 8);
static SENSOR_DEVICE_ATTR(qsfp_reset_all        , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_all_set       , QSFP_RESET_ALL);
static SENSOR_DEVICE_ATTR(qsfp1_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 1);
static SENSOR_DEVICE_ATTR(qsfp2_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 2);
static SENSOR_DEVICE_ATTR(qsfp3_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 3);
static SENSOR_DEVICE_ATTR(qsfp4_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 4);
static SENSOR_DEVICE_ATTR(qsfp5_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 5);
static SENSOR_DEVICE_ATTR(qsfp6_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 6);
static SENSOR_DEVICE_ATTR(qsfp7_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 7);
static SENSOR_DEVICE_ATTR(qsfp8_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 8);
static SENSOR_DEVICE_ATTR(qsfp_present_all      , S_IRUGO           , qsfp_present_all_get     , NULL                     , QSFP_PRESENT_ALL);
static SENSOR_DEVICE_ATTR(qsfp1_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 1);
static SENSOR_DEVICE_ATTR(qsfp2_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 2);
static SENSOR_DEVICE_ATTR(qsfp3_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 3);
static SENSOR_DEVICE_ATTR(qsfp4_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 4);
static SENSOR_DEVICE_ATTR(qsfp5_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 5);
static SENSOR_DEVICE_ATTR(qsfp6_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 6);
static SENSOR_DEVICE_ATTR(qsfp7_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 7);
static SENSOR_DEVICE_ATTR(qsfp8_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 8);
static SENSOR_DEVICE_ATTR(qsfp_int_all          , S_IRUGO           , qsfp_int_all_get         , NULL                     , QSFP_INT_ALL);
static SENSOR_DEVICE_ATTR(qsfp1_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 1);
static SENSOR_DEVICE_ATTR(qsfp2_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 2);
static SENSOR_DEVICE_ATTR(qsfp3_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 3);
static SENSOR_DEVICE_ATTR(qsfp4_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 4);
static SENSOR_DEVICE_ATTR(qsfp5_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 5);
static SENSOR_DEVICE_ATTR(qsfp6_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 6);
static SENSOR_DEVICE_ATTR(qsfp7_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 7);
static SENSOR_DEVICE_ATTR(qsfp8_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 8);
static SENSOR_DEVICE_ATTR(qsfp_int              , S_IRUGO           , qsfp_quter_int_get       , NULL                     , 1);
static SENSOR_DEVICE_ATTR(qsfp_modprs_int       , S_IRUGO           , qsfp_modprs_int_get      , NULL                     , 1);
static SENSOR_DEVICE_ATTR(qsfp_int_mask         , S_IRUGO | S_IWUSR , qsfp_quter_int_mask_get  , qsfp_quter_int_mask_set  , 1);
static SENSOR_DEVICE_ATTR(qsfp_modprs_mask      , S_IRUGO | S_IWUSR , qsfp_modprs_int_mask_get , qsfp_modprs_int_mask_set , 1);
/* end of sysfs attributes for SENSOR_DEVICE_ATTR */

/* sysfs attributes for hwmon */
/* i2c-0 */
static struct attribute *NBA615_SYS_attributes[] =
{
    &sensor_dev_attr_cpld_23_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_30_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_31_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_32_ver.dev_attr.attr,
    &sensor_dev_attr_wdt_en.dev_attr.attr,
    &sensor_dev_attr_eeprom_wp.dev_attr.attr,
    &sensor_dev_attr_usb_en.dev_attr.attr,
    &sensor_dev_attr_shutdown_set.dev_attr.attr,
    &sensor_dev_attr_reset.dev_attr.attr,
    &sensor_dev_attr_bmc_present.dev_attr.attr,
    &sensor_dev_attr_cpld_fp_int.dev_attr.attr,
    &sensor_dev_attr_cpld_rp_int.dev_attr.attr,
    &sensor_dev_attr_cpld_fan_int.dev_attr.attr,
    &sensor_dev_attr_cpld_psu_int.dev_attr.attr,
    &sensor_dev_attr_thermal_int.dev_attr.attr,
    &sensor_dev_attr_usb_int.dev_attr.attr,
    &sensor_dev_attr_cpld_fp_int_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_rp_int_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_fan_int_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_psu_int_mask.dev_attr.attr,
    &sensor_dev_attr_thermal_int_mask.dev_attr.attr,
    &sensor_dev_attr_usb_int_mask.dev_attr.attr,
    NULL
};
static struct attribute *NBA615_LED_attributes[] =
{
    &sensor_dev_attr_led_1.dev_attr.attr,
    &sensor_dev_attr_led_2.dev_attr.attr,
    &sensor_dev_attr_led_flow.dev_attr.attr,
    &sensor_dev_attr_led_sys.dev_attr.attr,
    &sensor_dev_attr_led_fiber.dev_attr.attr,
    NULL
};
static struct attribute *NBA615_THERMAL_attributes[] =
{
    &sensor_dev_attr_temp_th0_t.dev_attr.attr,
    &sensor_dev_attr_temp_th0_b.dev_attr.attr,
    &sensor_dev_attr_temp_th0_r.dev_attr.attr,
    &sensor_dev_attr_temp_th1_t.dev_attr.attr,
    &sensor_dev_attr_temp_th1_b.dev_attr.attr,
    &sensor_dev_attr_temp_th3_t.dev_attr.attr,
    &sensor_dev_attr_temp_th3_b.dev_attr.attr,
    &sensor_dev_attr_temp_th2_t.dev_attr.attr,
    &sensor_dev_attr_temp_th2_b.dev_attr.attr,
    &sensor_dev_attr_temp_th0_int.dev_attr.attr,
    &sensor_dev_attr_temp_th1_int.dev_attr.attr,
    &sensor_dev_attr_temp_th3_int.dev_attr.attr,
    &sensor_dev_attr_temp_th2_int.dev_attr.attr,
    &sensor_dev_attr_temp_th0_int_mask.dev_attr.attr,
    &sensor_dev_attr_temp_th1_int_mask.dev_attr.attr,
    &sensor_dev_attr_temp_th3_int_mask.dev_attr.attr,
    &sensor_dev_attr_temp_th2_int_mask.dev_attr.attr,
    &sensor_dev_attr_temp_th0_t_max.dev_attr.attr,
    &sensor_dev_attr_temp_th0_t_min.dev_attr.attr,
    &sensor_dev_attr_temp_th0_t_crit.dev_attr.attr,
    &sensor_dev_attr_temp_th0_t_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_th0_b_max.dev_attr.attr,
    &sensor_dev_attr_temp_th0_b_min.dev_attr.attr,
    &sensor_dev_attr_temp_th0_b_crit.dev_attr.attr,
    &sensor_dev_attr_temp_th0_b_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_th0_r_max.dev_attr.attr,
    &sensor_dev_attr_temp_th0_r_min.dev_attr.attr,
    &sensor_dev_attr_temp_th0_r_crit.dev_attr.attr,
    &sensor_dev_attr_temp_th0_r_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_th1_t_max.dev_attr.attr,
    &sensor_dev_attr_temp_th1_t_min.dev_attr.attr,
    &sensor_dev_attr_temp_th1_t_crit.dev_attr.attr,
    &sensor_dev_attr_temp_th1_t_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_th1_b_max.dev_attr.attr,
    &sensor_dev_attr_temp_th1_b_min.dev_attr.attr,
    &sensor_dev_attr_temp_th1_b_crit.dev_attr.attr,
    &sensor_dev_attr_temp_th1_b_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_th3_t_max.dev_attr.attr,
    &sensor_dev_attr_temp_th3_t_min.dev_attr.attr,
    &sensor_dev_attr_temp_th3_t_crit.dev_attr.attr,
    &sensor_dev_attr_temp_th3_t_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_th3_b_max.dev_attr.attr,
    &sensor_dev_attr_temp_th3_b_min.dev_attr.attr,
    &sensor_dev_attr_temp_th3_b_crit.dev_attr.attr,
    &sensor_dev_attr_temp_th3_b_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_th2_t_max.dev_attr.attr,
    &sensor_dev_attr_temp_th2_t_min.dev_attr.attr,
    &sensor_dev_attr_temp_th2_t_crit.dev_attr.attr,
    &sensor_dev_attr_temp_th2_t_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_th2_b_max.dev_attr.attr,
    &sensor_dev_attr_temp_th2_b_min.dev_attr.attr,
    &sensor_dev_attr_temp_th2_b_crit.dev_attr.attr,
    &sensor_dev_attr_temp_th2_b_lcrit.dev_attr.attr,
    NULL
};
static struct attribute *NBA615_FAN_attributes[] =
{
    &sensor_dev_attr_fanctrl_rpm.dev_attr.attr,
    &sensor_dev_attr_fanctrl_mode.dev_attr.attr,
    &sensor_dev_attr_fan1_stat.dev_attr.attr,
    &sensor_dev_attr_fan2_stat.dev_attr.attr,
    &sensor_dev_attr_fan3_stat.dev_attr.attr,
    &sensor_dev_attr_fan4_stat.dev_attr.attr,
    &sensor_dev_attr_fan1_present.dev_attr.attr,
    &sensor_dev_attr_fan2_present.dev_attr.attr,
    &sensor_dev_attr_fan3_present.dev_attr.attr,
    &sensor_dev_attr_fan4_present.dev_attr.attr,
    &sensor_dev_attr_fan1_power.dev_attr.attr,
    &sensor_dev_attr_fan2_power.dev_attr.attr,
    &sensor_dev_attr_fan3_power.dev_attr.attr,
    &sensor_dev_attr_fan4_power.dev_attr.attr,
    &sensor_dev_attr_fan1_front_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_front_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_front_rpm.dev_attr.attr,
    &sensor_dev_attr_fan4_front_rpm.dev_attr.attr,
    &sensor_dev_attr_fan1_rear_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_rear_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_rear_rpm.dev_attr.attr,
    &sensor_dev_attr_fan4_rear_rpm.dev_attr.attr,
    NULL
};
static struct attribute *NBA615_POWER_attributes[] =
{
    &sensor_dev_attr_psu1_good.dev_attr.attr,
    &sensor_dev_attr_psu2_good.dev_attr.attr,
    &sensor_dev_attr_psu1_prnt.dev_attr.attr,
    &sensor_dev_attr_psu2_prnt.dev_attr.attr,
    &sensor_dev_attr_psu1_vin.dev_attr.attr,
    &sensor_dev_attr_psu1_iin.dev_attr.attr,
    &sensor_dev_attr_psu1_vout.dev_attr.attr,
    &sensor_dev_attr_psu1_iout.dev_attr.attr,
    &sensor_dev_attr_psu1_temp.dev_attr.attr,
    &sensor_dev_attr_psu1_fan_speed.dev_attr.attr,
    &sensor_dev_attr_psu1_pout.dev_attr.attr,
    &sensor_dev_attr_psu1_pin.dev_attr.attr,
    &sensor_dev_attr_psu1_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu1_mfr_iout_max.dev_attr.attr,
    &sensor_dev_attr_psu1_vomde.dev_attr.attr,
    &sensor_dev_attr_psu2_vin.dev_attr.attr,
    &sensor_dev_attr_psu2_iin.dev_attr.attr,
    &sensor_dev_attr_psu2_vout.dev_attr.attr,
    &sensor_dev_attr_psu2_iout.dev_attr.attr,
    &sensor_dev_attr_psu2_temp.dev_attr.attr,
    &sensor_dev_attr_psu2_fan_speed.dev_attr.attr,
    &sensor_dev_attr_psu2_pout.dev_attr.attr,
    &sensor_dev_attr_psu2_pin.dev_attr.attr,
    &sensor_dev_attr_psu2_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu2_mfr_iout_max.dev_attr.attr,
    &sensor_dev_attr_psu2_vomde.dev_attr.attr,
    &sensor_dev_attr_dc6e_p0_vout.dev_attr.attr,
    &sensor_dev_attr_dc6e_p0_iout.dev_attr.attr,
    &sensor_dev_attr_dc6e_p0_pout.dev_attr.attr,
    &sensor_dev_attr_dc6e_p1_vout.dev_attr.attr,
    &sensor_dev_attr_dc6e_p1_iout.dev_attr.attr,
    &sensor_dev_attr_dc6e_p1_pout.dev_attr.attr,
    &sensor_dev_attr_dc70_p0_vout.dev_attr.attr,
    &sensor_dev_attr_dc70_p0_iout.dev_attr.attr,
    &sensor_dev_attr_dc70_p0_pout.dev_attr.attr,
    &sensor_dev_attr_dc70_p1_vout.dev_attr.attr,
    &sensor_dev_attr_dc70_p1_iout.dev_attr.attr,
    &sensor_dev_attr_dc70_p1_pout.dev_attr.attr,
    NULL
};
static struct attribute *NBA615_SFP_attributes[] =
{
    &sensor_dev_attr_sfp_tx_enable_all.dev_attr.attr,
    &sensor_dev_attr_sfp1_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp2_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp3_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp4_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp5_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp6_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp7_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp8_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp9_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp10_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp11_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp12_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp13_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp14_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp15_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp16_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp17_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp18_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp19_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp20_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp21_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp22_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp23_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp24_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp25_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp26_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp27_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp28_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp29_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp30_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp31_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp32_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp33_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp34_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp35_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp36_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp37_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp38_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp39_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp40_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp41_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp42_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp43_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp44_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp45_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp46_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp47_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp48_tx_enable.dev_attr.attr,
    &sensor_dev_attr_sfp_rx_loss_all.dev_attr.attr,
    &sensor_dev_attr_sfp1_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp2_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp3_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp4_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp5_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp6_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp7_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp8_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp9_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp10_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp11_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp12_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp13_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp14_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp15_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp16_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp17_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp18_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp19_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp20_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp21_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp22_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp23_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp24_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp25_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp26_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp27_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp28_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp29_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp30_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp31_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp32_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp33_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp34_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp35_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp36_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp37_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp38_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp39_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp40_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp41_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp42_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp43_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp44_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp45_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp46_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp47_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp48_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp_present_all.dev_attr.attr,
    &sensor_dev_attr_sfp1_present.dev_attr.attr,
    &sensor_dev_attr_sfp2_present.dev_attr.attr,
    &sensor_dev_attr_sfp3_present.dev_attr.attr,
    &sensor_dev_attr_sfp4_present.dev_attr.attr,
    &sensor_dev_attr_sfp5_present.dev_attr.attr,
    &sensor_dev_attr_sfp6_present.dev_attr.attr,
    &sensor_dev_attr_sfp7_present.dev_attr.attr,
    &sensor_dev_attr_sfp8_present.dev_attr.attr,
    &sensor_dev_attr_sfp9_present.dev_attr.attr,
    &sensor_dev_attr_sfp10_present.dev_attr.attr,
    &sensor_dev_attr_sfp11_present.dev_attr.attr,
    &sensor_dev_attr_sfp12_present.dev_attr.attr,
    &sensor_dev_attr_sfp13_present.dev_attr.attr,
    &sensor_dev_attr_sfp14_present.dev_attr.attr,
    &sensor_dev_attr_sfp15_present.dev_attr.attr,
    &sensor_dev_attr_sfp16_present.dev_attr.attr,
    &sensor_dev_attr_sfp17_present.dev_attr.attr,
    &sensor_dev_attr_sfp18_present.dev_attr.attr,
    &sensor_dev_attr_sfp19_present.dev_attr.attr,
    &sensor_dev_attr_sfp20_present.dev_attr.attr,
    &sensor_dev_attr_sfp21_present.dev_attr.attr,
    &sensor_dev_attr_sfp22_present.dev_attr.attr,
    &sensor_dev_attr_sfp23_present.dev_attr.attr,
    &sensor_dev_attr_sfp24_present.dev_attr.attr,
    &sensor_dev_attr_sfp25_present.dev_attr.attr,
    &sensor_dev_attr_sfp26_present.dev_attr.attr,
    &sensor_dev_attr_sfp27_present.dev_attr.attr,
    &sensor_dev_attr_sfp28_present.dev_attr.attr,
    &sensor_dev_attr_sfp29_present.dev_attr.attr,
    &sensor_dev_attr_sfp30_present.dev_attr.attr,
    &sensor_dev_attr_sfp31_present.dev_attr.attr,
    &sensor_dev_attr_sfp32_present.dev_attr.attr,
    &sensor_dev_attr_sfp33_present.dev_attr.attr,
    &sensor_dev_attr_sfp34_present.dev_attr.attr,
    &sensor_dev_attr_sfp35_present.dev_attr.attr,
    &sensor_dev_attr_sfp36_present.dev_attr.attr,
    &sensor_dev_attr_sfp37_present.dev_attr.attr,
    &sensor_dev_attr_sfp38_present.dev_attr.attr,
    &sensor_dev_attr_sfp39_present.dev_attr.attr,
    &sensor_dev_attr_sfp40_present.dev_attr.attr,
    &sensor_dev_attr_sfp41_present.dev_attr.attr,
    &sensor_dev_attr_sfp42_present.dev_attr.attr,
    &sensor_dev_attr_sfp43_present.dev_attr.attr,
    &sensor_dev_attr_sfp44_present.dev_attr.attr,
    &sensor_dev_attr_sfp45_present.dev_attr.attr,
    &sensor_dev_attr_sfp46_present.dev_attr.attr,
    &sensor_dev_attr_sfp47_present.dev_attr.attr,
    &sensor_dev_attr_sfp48_present.dev_attr.attr,
    &sensor_dev_attr_sfp_1_6_rx_loss_int.dev_attr.attr,
    &sensor_dev_attr_sfp_2_6_rx_loss_int.dev_attr.attr,
    &sensor_dev_attr_sfp_3_6_rx_loss_int.dev_attr.attr,
    &sensor_dev_attr_sfp_4_6_rx_loss_int.dev_attr.attr,
    &sensor_dev_attr_sfp_5_6_rx_loss_int.dev_attr.attr,
    &sensor_dev_attr_sfp_6_6_rx_loss_int.dev_attr.attr,
    &sensor_dev_attr_sfp_1_6_rx_loss_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_2_6_rx_loss_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_3_6_rx_loss_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_4_6_rx_loss_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_5_6_rx_loss_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_6_6_rx_loss_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_1_6_present_int.dev_attr.attr,
    &sensor_dev_attr_sfp_2_6_present_int.dev_attr.attr,
    &sensor_dev_attr_sfp_3_6_present_int.dev_attr.attr,
    &sensor_dev_attr_sfp_4_6_present_int.dev_attr.attr,
    &sensor_dev_attr_sfp_5_6_present_int.dev_attr.attr,
    &sensor_dev_attr_sfp_6_6_present_int.dev_attr.attr,
    &sensor_dev_attr_sfp_1_6_present_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_2_6_present_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_3_6_present_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_4_6_present_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_5_6_present_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_6_6_present_int_mask.dev_attr.attr,
    NULL
};
static struct attribute *NBA615_QSFP_attributes[] =
{
    &sensor_dev_attr_qsfp_low_power_all.dev_attr.attr,
    &sensor_dev_attr_qsfp1_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp2_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp3_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp4_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp5_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp6_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp7_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp8_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp_reset_all.dev_attr.attr,
    &sensor_dev_attr_qsfp1_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp2_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp3_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp4_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp5_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp6_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp7_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp8_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp_present_all.dev_attr.attr,
    &sensor_dev_attr_qsfp1_present.dev_attr.attr,
    &sensor_dev_attr_qsfp2_present.dev_attr.attr,
    &sensor_dev_attr_qsfp3_present.dev_attr.attr,
    &sensor_dev_attr_qsfp4_present.dev_attr.attr,
    &sensor_dev_attr_qsfp5_present.dev_attr.attr,
    &sensor_dev_attr_qsfp6_present.dev_attr.attr,
    &sensor_dev_attr_qsfp7_present.dev_attr.attr,
    &sensor_dev_attr_qsfp8_present.dev_attr.attr,
    &sensor_dev_attr_qsfp_int_all.dev_attr.attr,
    &sensor_dev_attr_qsfp1_int.dev_attr.attr,
    &sensor_dev_attr_qsfp2_int.dev_attr.attr,
    &sensor_dev_attr_qsfp3_int.dev_attr.attr,
    &sensor_dev_attr_qsfp4_int.dev_attr.attr,
    &sensor_dev_attr_qsfp5_int.dev_attr.attr,
    &sensor_dev_attr_qsfp6_int.dev_attr.attr,
    &sensor_dev_attr_qsfp7_int.dev_attr.attr,
    &sensor_dev_attr_qsfp8_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_modprs_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_int_mask.dev_attr.attr,
    &sensor_dev_attr_qsfp_modprs_mask.dev_attr.attr,
    NULL
};
/* end of sysfs attributes for hwmon */

/* struct attribute_group */
static const struct attribute_group NBA615_SYS_group =
{
    .name  = "NBA615_SYS",
    .attrs = NBA615_SYS_attributes,
};

static const struct attribute_group NBA615_LED_group =
{
    .name  = "NBA615_LED",
    .attrs = NBA615_LED_attributes,
};

static const struct attribute_group NBA615_THERMAL_group =
{
    .name  = "NBA615_THERMAL",
    .attrs = NBA615_THERMAL_attributes,
};

static const struct attribute_group NBA615_FAN_group =
{
    .name  = "NBA615_FAN",
    .attrs = NBA615_FAN_attributes,
};

static const struct attribute_group NBA615_POWER_group =
{
    .name  = "NBA615_POWER",
    .attrs = NBA615_POWER_attributes,
};

static const struct attribute_group NBA615_SFP_group =
{
    .name  = "NBA615_SFP",
    .attrs = NBA615_SFP_attributes,
};

static const struct attribute_group NBA615_QSFP_group =
{
    .name  = "NBA615_QSFP",
    .attrs = NBA615_QSFP_attributes,
};

static const struct bin_attribute net_eeprom_attr = {
        .attr = {
                .name = "eeprom",
                .mode = S_IRUGO,
        },
        .size = EEPROM_SIZE,
        .read = net_eeprom_read,
};

/* end of struct attribute_group */
