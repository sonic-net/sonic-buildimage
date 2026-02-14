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

#define DRIVER_VERSION  "1.0.1"

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
ssize_t themal_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t themal_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t themal_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t sys_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sys_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sys_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t themal_temp_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t themal_temp_max_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t themal_temp_min_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t themal_temp_crit_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t themal_temp_lcrit_get(struct device *dev, struct device_attribute *da, char *buf);
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
    TEMP_R_B_INT,
    TEMP_L_B_INT,
    TEMP_R_T_INT,
    TEMP_L_T_INT,
    TEMP_R_B_INT_MASK,
    TEMP_L_B_INT_MASK,    
    TEMP_R_T_INT_MASK,    
    TEMP_L_T_INT_MASK,
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
    TEMP_R_B_F,
    TEMP_R_B_B,
    TEMP_L_B_F,
    TEMP_L_B_B,
    TEMP_R_T_F,
    TEMP_R_T_B,
    TEMP_L_T_F,
    TEMP_L_T_B,
    TEMP_R_B_F_MAX,
    TEMP_L_B_F_MAX,
    TEMP_R_T_F_MAX,
    TEMP_L_T_F_MAX,
    TEMP_R_B_B_MAX,
    TEMP_L_B_B_MAX,
    TEMP_R_T_B_MAX,
    TEMP_L_T_B_MAX,
    TEMP_R_B_F_MIN,
    TEMP_L_B_F_MIN,
    TEMP_R_T_F_MIN,
    TEMP_L_T_F_MIN,
    TEMP_R_B_B_MIN,
    TEMP_L_B_B_MIN,
    TEMP_R_T_B_MIN,
    TEMP_L_T_B_MIN,
    TEMP_R_B_F_CRIT,
    TEMP_L_B_F_CRIT,
    TEMP_R_T_F_CRIT,
    TEMP_L_T_F_CRIT,
    TEMP_R_B_B_CRIT,
    TEMP_L_B_B_CRIT,
    TEMP_R_T_B_CRIT,
    TEMP_L_T_B_CRIT,
    TEMP_R_B_F_LCRIT,
    TEMP_L_B_F_LCRIT,
    TEMP_R_T_F_LCRIT,
    TEMP_L_T_F_LCRIT,
    TEMP_R_B_B_LCRIT,
    TEMP_L_B_B_LCRIT,
    TEMP_R_T_B_LCRIT,
    TEMP_L_T_B_LCRIT,
    FANCTRL_RPM,
    FANCTRL_MODE,
    FAN1_STAT,
    FAN2_STAT,
    FAN3_STAT,
    FAN4_STAT,
    FAN5_STAT,
    FAN1_PRESENT,
    FAN2_PRESENT,
    FAN3_PRESENT,
    FAN4_PRESENT,
    FAN5_PRESENT,
    FAN1_POWER,
    FAN2_POWER,
    FAN3_POWER,
    FAN4_POWER,
    FAN5_POWER,
    FAN1_FRONT_RPM,
    FAN2_FRONT_RPM,
    FAN3_FRONT_RPM,
    FAN4_FRONT_RPM,
    FAN5_FRONT_RPM,
    FAN1_REAR_RPM,
    FAN2_REAR_RPM,
    FAN3_REAR_RPM,
    FAN4_REAR_RPM,
    FAN5_REAR_RPM,
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
    QSFP_LOW_POWER_ALL,
    QSFP1_LOW_POWER,
    QSFP2_LOW_POWER,
    QSFP3_LOW_POWER,
    QSFP4_LOW_POWER,
    QSFP5_LOW_POWER,
    QSFP6_LOW_POWER,
    QSFP7_LOW_POWER,
    QSFP8_LOW_POWER,
    QSFP9_LOW_POWER,
    QSFP10_LOW_POWER,
    QSFP11_LOW_POWER,
    QSFP12_LOW_POWER,
    QSFP13_LOW_POWER,
    QSFP14_LOW_POWER,
    QSFP15_LOW_POWER,
    QSFP16_LOW_POWER,
    QSFP17_LOW_POWER,
    QSFP18_LOW_POWER,
    QSFP19_LOW_POWER,
    QSFP20_LOW_POWER,
    QSFP21_LOW_POWER,
    QSFP22_LOW_POWER,
    QSFP23_LOW_POWER,
    QSFP24_LOW_POWER,
    QSFP25_LOW_POWER,
    QSFP26_LOW_POWER,
    QSFP27_LOW_POWER,
    QSFP28_LOW_POWER,
    QSFP29_LOW_POWER,
    QSFP30_LOW_POWER,
    QSFP31_LOW_POWER,
    QSFP32_LOW_POWER,
    QSFP_RESET_ALL,
    QSFP1_RESET,
    QSFP2_RESET,
    QSFP3_RESET,
    QSFP4_RESET,
    QSFP5_RESET,
    QSFP6_RESET,
    QSFP7_RESET,
    QSFP8_RESET,
    QSFP9_RESET,
    QSFP10_RESET,
    QSFP11_RESET,
    QSFP12_RESET,
    QSFP13_RESET,
    QSFP14_RESET,
    QSFP15_RESET,
    QSFP16_RESET,
    QSFP17_RESET,
    QSFP18_RESET,
    QSFP19_RESET,
    QSFP20_RESET,
    QSFP21_RESET,
    QSFP22_RESET,
    QSFP23_RESET,
    QSFP24_RESET,
    QSFP25_RESET,
    QSFP26_RESET,
    QSFP27_RESET,
    QSFP28_RESET,
    QSFP29_RESET,
    QSFP30_RESET,
    QSFP31_RESET,
    QSFP32_RESET,
    QSFP_PRESENT_ALL,
    QSFP1_PRESENT,
    QSFP2_PRESENT,
    QSFP3_PRESENT,
    QSFP4_PRESENT,
    QSFP5_PRESENT,
    QSFP6_PRESENT,
    QSFP7_PRESENT,
    QSFP8_PRESENT,
    QSFP9_PRESENT,
    QSFP10_PRESENT,
    QSFP11_PRESENT,
    QSFP12_PRESENT,
    QSFP13_PRESENT,
    QSFP14_PRESENT,
    QSFP15_PRESENT,
    QSFP16_PRESENT,
    QSFP17_PRESENT,
    QSFP18_PRESENT,
    QSFP19_PRESENT,
    QSFP20_PRESENT,
    QSFP21_PRESENT,
    QSFP22_PRESENT,
    QSFP23_PRESENT,
    QSFP24_PRESENT,
    QSFP25_PRESENT,
    QSFP26_PRESENT,
    QSFP27_PRESENT,
    QSFP28_PRESENT,
    QSFP29_PRESENT,
    QSFP30_PRESENT,
    QSFP31_PRESENT,
    QSFP32_PRESENT,
    QSFP_INT_ALL,
    QSFP1_INT,
    QSFP2_INT,
    QSFP3_INT,
    QSFP4_INT,
    QSFP5_INT,
    QSFP6_INT,
    QSFP7_INT,
    QSFP8_INT,
    QSFP9_INT,
    QSFP10_INT,
    QSFP11_INT,
    QSFP12_INT,
    QSFP13_INT,
    QSFP14_INT,
    QSFP15_INT,
    QSFP16_INT,
    QSFP17_INT,
    QSFP18_INT,
    QSFP19_INT,
    QSFP20_INT,
    QSFP21_INT,
    QSFP22_INT,
    QSFP23_INT,
    QSFP24_INT,
    QSFP25_INT,
    QSFP26_INT,
    QSFP27_INT,
    QSFP28_INT,
    QSFP29_INT,
    QSFP30_INT,
    QSFP31_INT,
    QSFP32_INT,
    QSFP1_4_INT,
    QSFP2_4_INT,
    QSFP3_4_INT,
    QSFP4_4_INT,
    QSFP1_4_MODPRS,
    QSFP2_4_MODPRS,
    QSFP3_4_MODPRS,
    QSFP4_4_MODPRS,
    QSFP1_4_INT_MASK,
    QSFP2_4_INT_MASK,
    QSFP3_4_INT_MASK,
    QSFP4_4_INT_MASK,
    QSFP1_4_MODPRS_MASK,
    QSFP2_4_MODPRS_MASK,
    QSFP3_4_MODPRS_MASK,
    QSFP4_4_MODPRS_MASK
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
static SENSOR_DEVICE_ATTR(temp_r_b_int          , S_IRUGO           , themal_int_get           , NULL                     , TEMP_R_B_INT);
static SENSOR_DEVICE_ATTR(temp_l_b_int          , S_IRUGO           , themal_int_get           , NULL                     , TEMP_L_B_INT);
static SENSOR_DEVICE_ATTR(temp_r_t_int          , S_IRUGO           , themal_int_get           , NULL                     , TEMP_R_T_INT);
static SENSOR_DEVICE_ATTR(temp_l_t_int          , S_IRUGO           , themal_int_get           , NULL                     , TEMP_L_T_INT);
static SENSOR_DEVICE_ATTR(temp_r_b_int_mask     , S_IRUGO | S_IWUSR , themal_int_mask_get      , themal_int_mask_set      , TEMP_R_B_INT_MASK);
static SENSOR_DEVICE_ATTR(temp_l_b_int_mask     , S_IRUGO | S_IWUSR , themal_int_mask_get      , themal_int_mask_set      , TEMP_L_B_INT_MASK);
static SENSOR_DEVICE_ATTR(temp_r_t_int_mask     , S_IRUGO | S_IWUSR , themal_int_mask_get      , themal_int_mask_set      , TEMP_R_T_INT_MASK);
static SENSOR_DEVICE_ATTR(temp_l_t_int_mask     , S_IRUGO | S_IWUSR , themal_int_mask_get      , themal_int_mask_set      , TEMP_L_T_INT_MASK);
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
static SENSOR_DEVICE_ATTR(temp_r_b_f            , S_IRUGO           , themal_temp_get          , NULL                     , TEMP_R_B_F);
static SENSOR_DEVICE_ATTR(temp_r_b_b            , S_IRUGO           , themal_temp_get          , NULL                     , TEMP_R_B_B);
static SENSOR_DEVICE_ATTR(temp_l_b_f            , S_IRUGO           , themal_temp_get          , NULL                     , TEMP_L_B_F);
static SENSOR_DEVICE_ATTR(temp_l_b_b            , S_IRUGO           , themal_temp_get          , NULL                     , TEMP_L_B_B);
static SENSOR_DEVICE_ATTR(temp_r_t_f            , S_IRUGO           , themal_temp_get          , NULL                     , TEMP_R_T_F);
static SENSOR_DEVICE_ATTR(temp_r_t_b            , S_IRUGO           , themal_temp_get          , NULL                     , TEMP_R_T_B);
static SENSOR_DEVICE_ATTR(temp_l_t_f            , S_IRUGO           , themal_temp_get          , NULL                     , TEMP_L_T_F);
static SENSOR_DEVICE_ATTR(temp_l_t_b            , S_IRUGO           , themal_temp_get          , NULL                     , TEMP_L_T_B);
static SENSOR_DEVICE_ATTR(temp_r_b_f_max        , S_IRUGO           , themal_temp_max_get      , NULL                     , TEMP_R_B_F_MAX);
static SENSOR_DEVICE_ATTR(temp_l_b_f_max        , S_IRUGO           , themal_temp_max_get      , NULL                     , TEMP_L_B_F_MAX);
static SENSOR_DEVICE_ATTR(temp_r_t_f_max        , S_IRUGO           , themal_temp_max_get      , NULL                     , TEMP_R_T_F_MAX);
static SENSOR_DEVICE_ATTR(temp_l_t_f_max        , S_IRUGO           , themal_temp_max_get      , NULL                     , TEMP_L_T_F_MAX);
static SENSOR_DEVICE_ATTR(temp_r_b_b_max        , S_IRUGO           , themal_temp_max_get      , NULL                     , TEMP_R_B_B_MAX);
static SENSOR_DEVICE_ATTR(temp_l_b_b_max        , S_IRUGO           , themal_temp_max_get      , NULL                     , TEMP_L_B_B_MAX);
static SENSOR_DEVICE_ATTR(temp_r_t_b_max        , S_IRUGO           , themal_temp_max_get      , NULL                     , TEMP_R_T_B_MAX);
static SENSOR_DEVICE_ATTR(temp_l_t_b_max        , S_IRUGO           , themal_temp_max_get      , NULL                     , TEMP_L_T_B_MAX);
static SENSOR_DEVICE_ATTR(temp_r_b_f_min        , S_IRUGO           , themal_temp_min_get      , NULL                     , TEMP_R_B_F_MIN);
static SENSOR_DEVICE_ATTR(temp_l_b_f_min        , S_IRUGO           , themal_temp_min_get      , NULL                     , TEMP_L_B_F_MIN);
static SENSOR_DEVICE_ATTR(temp_r_t_f_min        , S_IRUGO           , themal_temp_min_get      , NULL                     , TEMP_R_T_F_MIN);
static SENSOR_DEVICE_ATTR(temp_l_t_f_min        , S_IRUGO           , themal_temp_min_get      , NULL                     , TEMP_L_T_F_MIN);
static SENSOR_DEVICE_ATTR(temp_r_b_b_min        , S_IRUGO           , themal_temp_min_get      , NULL                     , TEMP_R_B_B_MIN);
static SENSOR_DEVICE_ATTR(temp_l_b_b_min        , S_IRUGO           , themal_temp_min_get      , NULL                     , TEMP_L_B_B_MIN);
static SENSOR_DEVICE_ATTR(temp_r_t_b_min        , S_IRUGO           , themal_temp_min_get      , NULL                     , TEMP_R_T_B_MIN);
static SENSOR_DEVICE_ATTR(temp_l_t_b_min        , S_IRUGO           , themal_temp_min_get      , NULL                     , TEMP_L_T_B_MIN);
static SENSOR_DEVICE_ATTR(temp_r_b_f_crit       , S_IRUGO           , themal_temp_crit_get     , NULL                     , TEMP_R_B_F_CRIT);
static SENSOR_DEVICE_ATTR(temp_l_b_f_crit       , S_IRUGO           , themal_temp_crit_get     , NULL                     , TEMP_L_B_F_CRIT);
static SENSOR_DEVICE_ATTR(temp_r_t_f_crit       , S_IRUGO           , themal_temp_crit_get     , NULL                     , TEMP_R_T_F_CRIT);
static SENSOR_DEVICE_ATTR(temp_l_t_f_crit       , S_IRUGO           , themal_temp_crit_get     , NULL                     , TEMP_L_T_F_CRIT);
static SENSOR_DEVICE_ATTR(temp_r_b_b_crit       , S_IRUGO           , themal_temp_crit_get     , NULL                     , TEMP_R_B_B_CRIT);
static SENSOR_DEVICE_ATTR(temp_l_b_b_crit       , S_IRUGO           , themal_temp_crit_get     , NULL                     , TEMP_L_B_B_CRIT);
static SENSOR_DEVICE_ATTR(temp_r_t_b_crit       , S_IRUGO           , themal_temp_crit_get     , NULL                     , TEMP_R_T_B_CRIT);
static SENSOR_DEVICE_ATTR(temp_l_t_b_crit       , S_IRUGO           , themal_temp_crit_get     , NULL                     , TEMP_L_T_B_CRIT);
static SENSOR_DEVICE_ATTR(temp_r_b_f_lcrit      , S_IRUGO           , themal_temp_lcrit_get    , NULL                     , TEMP_R_B_F_LCRIT);
static SENSOR_DEVICE_ATTR(temp_l_b_f_lcrit      , S_IRUGO           , themal_temp_lcrit_get    , NULL                     , TEMP_L_B_F_LCRIT);
static SENSOR_DEVICE_ATTR(temp_r_t_f_lcrit      , S_IRUGO           , themal_temp_lcrit_get    , NULL                     , TEMP_R_T_F_LCRIT);
static SENSOR_DEVICE_ATTR(temp_l_t_f_lcrit      , S_IRUGO           , themal_temp_lcrit_get    , NULL                     , TEMP_L_T_F_LCRIT);
static SENSOR_DEVICE_ATTR(temp_r_b_b_lcrit      , S_IRUGO           , themal_temp_lcrit_get    , NULL                     , TEMP_R_B_B_LCRIT);
static SENSOR_DEVICE_ATTR(temp_l_b_b_lcrit      , S_IRUGO           , themal_temp_lcrit_get    , NULL                     , TEMP_L_B_B_LCRIT);
static SENSOR_DEVICE_ATTR(temp_r_t_b_lcrit      , S_IRUGO           , themal_temp_lcrit_get    , NULL                     , TEMP_R_T_B_LCRIT);
static SENSOR_DEVICE_ATTR(temp_l_t_b_lcrit      , S_IRUGO           , themal_temp_lcrit_get    , NULL                     , TEMP_L_T_B_LCRIT);
static SENSOR_DEVICE_ATTR(fanctrl_rpm           , S_IRUGO           , fan_ctrl_rpm_get         , NULL                     , FANCTRL_RPM);
static SENSOR_DEVICE_ATTR(fanctrl_mode          , S_IRUGO           , fan_ctrl_mode_get        , NULL                     , FANCTRL_MODE);
static SENSOR_DEVICE_ATTR(fan1_stat             , S_IRUGO           , fan_status_get           , NULL                     , 1);
static SENSOR_DEVICE_ATTR(fan2_stat             , S_IRUGO           , fan_status_get           , NULL                     , 2);
static SENSOR_DEVICE_ATTR(fan3_stat             , S_IRUGO           , fan_status_get           , NULL                     , 3);
static SENSOR_DEVICE_ATTR(fan4_stat             , S_IRUGO           , fan_status_get           , NULL                     , 4);
static SENSOR_DEVICE_ATTR(fan5_stat             , S_IRUGO           , fan_status_get           , NULL                     , 5);
static SENSOR_DEVICE_ATTR(fan1_present          , S_IRUGO           , fan_present_get          , NULL                     , 1);
static SENSOR_DEVICE_ATTR(fan2_present          , S_IRUGO           , fan_present_get          , NULL                     , 2);
static SENSOR_DEVICE_ATTR(fan3_present          , S_IRUGO           , fan_present_get          , NULL                     , 3);
static SENSOR_DEVICE_ATTR(fan4_present          , S_IRUGO           , fan_present_get          , NULL                     , 4);
static SENSOR_DEVICE_ATTR(fan5_present          , S_IRUGO           , fan_present_get          , NULL                     , 5);
static SENSOR_DEVICE_ATTR(fan1_power            , S_IRUGO           , fan_power_get            , NULL                     , 1);
static SENSOR_DEVICE_ATTR(fan2_power            , S_IRUGO           , fan_power_get            , NULL                     , 2);
static SENSOR_DEVICE_ATTR(fan3_power            , S_IRUGO           , fan_power_get            , NULL                     , 3);
static SENSOR_DEVICE_ATTR(fan4_power            , S_IRUGO           , fan_power_get            , NULL                     , 4);
static SENSOR_DEVICE_ATTR(fan5_power            , S_IRUGO           , fan_power_get            , NULL                     , 5);
static SENSOR_DEVICE_ATTR(fan1_front_rpm        , S_IRUGO           , fan_rpm_get              , NULL                     , FAN1_FRONT_RPM);
static SENSOR_DEVICE_ATTR(fan2_front_rpm        , S_IRUGO           , fan_rpm_get              , NULL                     , FAN2_FRONT_RPM);
static SENSOR_DEVICE_ATTR(fan3_front_rpm        , S_IRUGO           , fan_rpm_get              , NULL                     , FAN3_FRONT_RPM);
static SENSOR_DEVICE_ATTR(fan4_front_rpm        , S_IRUGO           , fan_rpm_get              , NULL                     , FAN4_FRONT_RPM);
static SENSOR_DEVICE_ATTR(fan5_front_rpm        , S_IRUGO           , fan_rpm_get              , NULL                     , FAN5_FRONT_RPM);
static SENSOR_DEVICE_ATTR(fan1_rear_rpm         , S_IRUGO           , fan_rpm_get              , NULL                     , FAN1_REAR_RPM);
static SENSOR_DEVICE_ATTR(fan2_rear_rpm         , S_IRUGO           , fan_rpm_get              , NULL                     , FAN2_REAR_RPM);
static SENSOR_DEVICE_ATTR(fan3_rear_rpm         , S_IRUGO           , fan_rpm_get              , NULL                     , FAN3_REAR_RPM);
static SENSOR_DEVICE_ATTR(fan4_rear_rpm         , S_IRUGO           , fan_rpm_get              , NULL                     , FAN4_REAR_RPM);
static SENSOR_DEVICE_ATTR(fan5_rear_rpm         , S_IRUGO           , fan_rpm_get              , NULL                     , FAN5_REAR_RPM);
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
static SENSOR_DEVICE_ATTR(qsfp_low_power_all    , S_IRUGO | S_IWUSR , qsfp_low_power_all_get   , qsfp_low_power_all_set   , QSFP_LOW_POWER_ALL);
static SENSOR_DEVICE_ATTR(qsfp1_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 1);
static SENSOR_DEVICE_ATTR(qsfp2_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 2);
static SENSOR_DEVICE_ATTR(qsfp3_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 3);
static SENSOR_DEVICE_ATTR(qsfp4_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 4);
static SENSOR_DEVICE_ATTR(qsfp5_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 5);
static SENSOR_DEVICE_ATTR(qsfp6_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 6);
static SENSOR_DEVICE_ATTR(qsfp7_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 7);
static SENSOR_DEVICE_ATTR(qsfp8_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 8);
static SENSOR_DEVICE_ATTR(qsfp9_low_power       , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 9);
static SENSOR_DEVICE_ATTR(qsfp10_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 10);
static SENSOR_DEVICE_ATTR(qsfp11_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 11);
static SENSOR_DEVICE_ATTR(qsfp12_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 12);
static SENSOR_DEVICE_ATTR(qsfp13_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 13);
static SENSOR_DEVICE_ATTR(qsfp14_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 14);
static SENSOR_DEVICE_ATTR(qsfp15_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 15);
static SENSOR_DEVICE_ATTR(qsfp16_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 16);
static SENSOR_DEVICE_ATTR(qsfp17_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 17);
static SENSOR_DEVICE_ATTR(qsfp18_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 18);
static SENSOR_DEVICE_ATTR(qsfp19_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 19);
static SENSOR_DEVICE_ATTR(qsfp20_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 20);
static SENSOR_DEVICE_ATTR(qsfp21_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 21);
static SENSOR_DEVICE_ATTR(qsfp22_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 22);
static SENSOR_DEVICE_ATTR(qsfp23_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 23);
static SENSOR_DEVICE_ATTR(qsfp24_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 24);
static SENSOR_DEVICE_ATTR(qsfp25_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 25);
static SENSOR_DEVICE_ATTR(qsfp26_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 26);
static SENSOR_DEVICE_ATTR(qsfp27_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 27);
static SENSOR_DEVICE_ATTR(qsfp28_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 28);
static SENSOR_DEVICE_ATTR(qsfp29_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 29);
static SENSOR_DEVICE_ATTR(qsfp30_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 30);
static SENSOR_DEVICE_ATTR(qsfp31_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 31);
static SENSOR_DEVICE_ATTR(qsfp32_low_power      , S_IRUGO | S_IWUSR , qsfp_low_power_get       , qsfp_low_power_set       , 32);
static SENSOR_DEVICE_ATTR(qsfp_reset_all        , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_all_set       , QSFP_RESET_ALL);
static SENSOR_DEVICE_ATTR(qsfp1_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 1);
static SENSOR_DEVICE_ATTR(qsfp2_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 2);
static SENSOR_DEVICE_ATTR(qsfp3_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 3);
static SENSOR_DEVICE_ATTR(qsfp4_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 4);
static SENSOR_DEVICE_ATTR(qsfp5_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 5);
static SENSOR_DEVICE_ATTR(qsfp6_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 6);
static SENSOR_DEVICE_ATTR(qsfp7_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 7);
static SENSOR_DEVICE_ATTR(qsfp8_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 8);
static SENSOR_DEVICE_ATTR(qsfp9_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 9);
static SENSOR_DEVICE_ATTR(qsfp10_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 10);
static SENSOR_DEVICE_ATTR(qsfp11_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 11);
static SENSOR_DEVICE_ATTR(qsfp12_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 12);
static SENSOR_DEVICE_ATTR(qsfp13_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 13);
static SENSOR_DEVICE_ATTR(qsfp14_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 14);
static SENSOR_DEVICE_ATTR(qsfp15_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 15);
static SENSOR_DEVICE_ATTR(qsfp16_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 16);
static SENSOR_DEVICE_ATTR(qsfp17_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 17);
static SENSOR_DEVICE_ATTR(qsfp18_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 18);
static SENSOR_DEVICE_ATTR(qsfp19_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 19);
static SENSOR_DEVICE_ATTR(qsfp20_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 20);
static SENSOR_DEVICE_ATTR(qsfp21_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 21);
static SENSOR_DEVICE_ATTR(qsfp22_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 22);
static SENSOR_DEVICE_ATTR(qsfp23_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 23);
static SENSOR_DEVICE_ATTR(qsfp24_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 24);
static SENSOR_DEVICE_ATTR(qsfp25_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 25);
static SENSOR_DEVICE_ATTR(qsfp26_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 26);
static SENSOR_DEVICE_ATTR(qsfp27_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 27);
static SENSOR_DEVICE_ATTR(qsfp28_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 28);
static SENSOR_DEVICE_ATTR(qsfp29_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 29);
static SENSOR_DEVICE_ATTR(qsfp30_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 30);
static SENSOR_DEVICE_ATTR(qsfp31_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 31);
static SENSOR_DEVICE_ATTR(qsfp32_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 32);
static SENSOR_DEVICE_ATTR(qsfp_present_all      , S_IRUGO           , qsfp_present_all_get     , NULL                     , QSFP_PRESENT_ALL);
static SENSOR_DEVICE_ATTR(qsfp1_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 1);
static SENSOR_DEVICE_ATTR(qsfp2_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 2);
static SENSOR_DEVICE_ATTR(qsfp3_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 3);
static SENSOR_DEVICE_ATTR(qsfp4_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 4);
static SENSOR_DEVICE_ATTR(qsfp5_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 5);
static SENSOR_DEVICE_ATTR(qsfp6_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 6);
static SENSOR_DEVICE_ATTR(qsfp7_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 7);
static SENSOR_DEVICE_ATTR(qsfp8_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 8);
static SENSOR_DEVICE_ATTR(qsfp9_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 9);
static SENSOR_DEVICE_ATTR(qsfp10_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 10);
static SENSOR_DEVICE_ATTR(qsfp11_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 11);
static SENSOR_DEVICE_ATTR(qsfp12_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 12);
static SENSOR_DEVICE_ATTR(qsfp13_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 13);
static SENSOR_DEVICE_ATTR(qsfp14_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 14);
static SENSOR_DEVICE_ATTR(qsfp15_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 15);
static SENSOR_DEVICE_ATTR(qsfp16_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 16);
static SENSOR_DEVICE_ATTR(qsfp17_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 17);
static SENSOR_DEVICE_ATTR(qsfp18_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 18);
static SENSOR_DEVICE_ATTR(qsfp19_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 19);
static SENSOR_DEVICE_ATTR(qsfp20_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 20);
static SENSOR_DEVICE_ATTR(qsfp21_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 21);
static SENSOR_DEVICE_ATTR(qsfp22_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 22);
static SENSOR_DEVICE_ATTR(qsfp23_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 23);
static SENSOR_DEVICE_ATTR(qsfp24_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 24);
static SENSOR_DEVICE_ATTR(qsfp25_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 25);
static SENSOR_DEVICE_ATTR(qsfp26_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 26);
static SENSOR_DEVICE_ATTR(qsfp27_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 27);
static SENSOR_DEVICE_ATTR(qsfp28_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 28);
static SENSOR_DEVICE_ATTR(qsfp29_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 29);
static SENSOR_DEVICE_ATTR(qsfp30_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 30);
static SENSOR_DEVICE_ATTR(qsfp31_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 31);
static SENSOR_DEVICE_ATTR(qsfp32_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 32);
static SENSOR_DEVICE_ATTR(qsfp_int_all          , S_IRUGO           , qsfp_int_all_get         , NULL                     , QSFP_INT_ALL);
static SENSOR_DEVICE_ATTR(qsfp1_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 1);
static SENSOR_DEVICE_ATTR(qsfp2_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 2);
static SENSOR_DEVICE_ATTR(qsfp3_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 3);
static SENSOR_DEVICE_ATTR(qsfp4_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 4);
static SENSOR_DEVICE_ATTR(qsfp5_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 5);
static SENSOR_DEVICE_ATTR(qsfp6_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 6);
static SENSOR_DEVICE_ATTR(qsfp7_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 7);
static SENSOR_DEVICE_ATTR(qsfp8_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 8);
static SENSOR_DEVICE_ATTR(qsfp9_int             , S_IRUGO           , qsfp_int_get             , NULL                     , 9);
static SENSOR_DEVICE_ATTR(qsfp10_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 10);
static SENSOR_DEVICE_ATTR(qsfp11_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 11);
static SENSOR_DEVICE_ATTR(qsfp12_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 12);
static SENSOR_DEVICE_ATTR(qsfp13_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 13);
static SENSOR_DEVICE_ATTR(qsfp14_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 14);
static SENSOR_DEVICE_ATTR(qsfp15_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 15);
static SENSOR_DEVICE_ATTR(qsfp16_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 16);
static SENSOR_DEVICE_ATTR(qsfp17_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 17);
static SENSOR_DEVICE_ATTR(qsfp18_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 18);
static SENSOR_DEVICE_ATTR(qsfp19_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 19);
static SENSOR_DEVICE_ATTR(qsfp20_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 20);
static SENSOR_DEVICE_ATTR(qsfp21_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 21);
static SENSOR_DEVICE_ATTR(qsfp22_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 22);
static SENSOR_DEVICE_ATTR(qsfp23_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 23);
static SENSOR_DEVICE_ATTR(qsfp24_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 24);
static SENSOR_DEVICE_ATTR(qsfp25_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 25);
static SENSOR_DEVICE_ATTR(qsfp26_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 26);
static SENSOR_DEVICE_ATTR(qsfp27_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 27);
static SENSOR_DEVICE_ATTR(qsfp28_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 28);
static SENSOR_DEVICE_ATTR(qsfp29_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 29);
static SENSOR_DEVICE_ATTR(qsfp30_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 30);
static SENSOR_DEVICE_ATTR(qsfp31_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 31);
static SENSOR_DEVICE_ATTR(qsfp32_int            , S_IRUGO           , qsfp_int_get             , NULL                     , 32);
static SENSOR_DEVICE_ATTR(qsfp1_4_int           , S_IRUGO           , qsfp_quter_int_get       , NULL                     , 1);
static SENSOR_DEVICE_ATTR(qsfp2_4_int           , S_IRUGO           , qsfp_quter_int_get       , NULL                     , 2);
static SENSOR_DEVICE_ATTR(qsfp3_4_int           , S_IRUGO           , qsfp_quter_int_get       , NULL                     , 3);
static SENSOR_DEVICE_ATTR(qsfp4_4_int           , S_IRUGO           , qsfp_quter_int_get       , NULL                     , 4);
static SENSOR_DEVICE_ATTR(qsfp1_4_modprs        , S_IRUGO           , qsfp_modprs_int_get      , NULL                     , 1);
static SENSOR_DEVICE_ATTR(qsfp2_4_modprs        , S_IRUGO           , qsfp_modprs_int_get      , NULL                     , 2);
static SENSOR_DEVICE_ATTR(qsfp3_4_modprs        , S_IRUGO           , qsfp_modprs_int_get      , NULL                     , 3);
static SENSOR_DEVICE_ATTR(qsfp4_4_modprs        , S_IRUGO           , qsfp_modprs_int_get      , NULL                     , 4);
static SENSOR_DEVICE_ATTR(qsfp1_4_int_mask      , S_IRUGO | S_IWUSR , qsfp_quter_int_mask_get  , qsfp_quter_int_mask_set  , 1);
static SENSOR_DEVICE_ATTR(qsfp2_4_int_mask      , S_IRUGO | S_IWUSR , qsfp_quter_int_mask_get  , qsfp_quter_int_mask_set  , 2);
static SENSOR_DEVICE_ATTR(qsfp3_4_int_mask      , S_IRUGO | S_IWUSR , qsfp_quter_int_mask_get  , qsfp_quter_int_mask_set  , 3);
static SENSOR_DEVICE_ATTR(qsfp4_4_int_mask      , S_IRUGO | S_IWUSR , qsfp_quter_int_mask_get  , qsfp_quter_int_mask_set  , 4);
static SENSOR_DEVICE_ATTR(qsfp1_4_modprs_mask   , S_IRUGO | S_IWUSR , qsfp_modprs_int_mask_get , qsfp_modprs_int_mask_set , 1);
static SENSOR_DEVICE_ATTR(qsfp2_4_modprs_mask   , S_IRUGO | S_IWUSR , qsfp_modprs_int_mask_get , qsfp_modprs_int_mask_set , 2);
static SENSOR_DEVICE_ATTR(qsfp3_4_modprs_mask   , S_IRUGO | S_IWUSR , qsfp_modprs_int_mask_get , qsfp_modprs_int_mask_set , 3);
static SENSOR_DEVICE_ATTR(qsfp4_4_modprs_mask   , S_IRUGO | S_IWUSR , qsfp_modprs_int_mask_get , qsfp_modprs_int_mask_set , 4);
/* end of sysfs attributes for SENSOR_DEVICE_ATTR */

/* sysfs attributes for hwmon */
/* i2c-0 */
static struct attribute *NBA715_SYS_attributes[] =
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
static struct attribute *NBA715_LED_attributes[] =
{
    &sensor_dev_attr_led_1.dev_attr.attr,
    &sensor_dev_attr_led_2.dev_attr.attr,
    &sensor_dev_attr_led_flow.dev_attr.attr,
    &sensor_dev_attr_led_sys.dev_attr.attr,
    &sensor_dev_attr_led_fiber.dev_attr.attr,
    NULL
};
static struct attribute *NBA715_THERMAL_attributes[] =
{
    &sensor_dev_attr_temp_r_b_f.dev_attr.attr,
    &sensor_dev_attr_temp_r_b_b.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_f.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_b.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_f.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_b.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_f.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_b.dev_attr.attr,
    &sensor_dev_attr_temp_r_b_int.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_int.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_int.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_int.dev_attr.attr,
    &sensor_dev_attr_temp_r_b_int_mask.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_int_mask.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_int_mask.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_int_mask.dev_attr.attr,
    &sensor_dev_attr_temp_r_b_f_max.dev_attr.attr,
    &sensor_dev_attr_temp_r_b_f_min.dev_attr.attr,
    &sensor_dev_attr_temp_r_b_f_crit.dev_attr.attr,
    &sensor_dev_attr_temp_r_b_f_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_r_b_b_max.dev_attr.attr,
    &sensor_dev_attr_temp_r_b_b_min.dev_attr.attr,
    &sensor_dev_attr_temp_r_b_b_crit.dev_attr.attr,
    &sensor_dev_attr_temp_r_b_b_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_f_max.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_f_min.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_f_crit.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_f_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_b_max.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_b_min.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_b_crit.dev_attr.attr,
    &sensor_dev_attr_temp_l_b_b_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_f_max.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_f_min.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_f_crit.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_f_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_b_max.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_b_min.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_b_crit.dev_attr.attr,
    &sensor_dev_attr_temp_r_t_b_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_f_max.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_f_min.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_f_crit.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_f_lcrit.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_b_max.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_b_min.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_b_crit.dev_attr.attr,
    &sensor_dev_attr_temp_l_t_b_lcrit.dev_attr.attr,
    NULL
};
static struct attribute *NBA715_FAN_attributes[] =
{
    &sensor_dev_attr_fanctrl_rpm.dev_attr.attr,
    &sensor_dev_attr_fanctrl_mode.dev_attr.attr,
    &sensor_dev_attr_fan1_stat.dev_attr.attr,
    &sensor_dev_attr_fan2_stat.dev_attr.attr,
    &sensor_dev_attr_fan3_stat.dev_attr.attr,
    &sensor_dev_attr_fan4_stat.dev_attr.attr,
    &sensor_dev_attr_fan5_stat.dev_attr.attr,
    &sensor_dev_attr_fan1_present.dev_attr.attr,
    &sensor_dev_attr_fan2_present.dev_attr.attr,
    &sensor_dev_attr_fan3_present.dev_attr.attr,
    &sensor_dev_attr_fan4_present.dev_attr.attr,
    &sensor_dev_attr_fan5_present.dev_attr.attr,
    &sensor_dev_attr_fan1_power.dev_attr.attr,
    &sensor_dev_attr_fan2_power.dev_attr.attr,
    &sensor_dev_attr_fan3_power.dev_attr.attr,
    &sensor_dev_attr_fan4_power.dev_attr.attr,
    &sensor_dev_attr_fan5_power.dev_attr.attr,
    &sensor_dev_attr_fan1_front_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_front_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_front_rpm.dev_attr.attr,
    &sensor_dev_attr_fan4_front_rpm.dev_attr.attr,
    &sensor_dev_attr_fan5_front_rpm.dev_attr.attr,
    &sensor_dev_attr_fan1_rear_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_rear_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_rear_rpm.dev_attr.attr,
    &sensor_dev_attr_fan4_rear_rpm.dev_attr.attr,
    &sensor_dev_attr_fan5_rear_rpm.dev_attr.attr,
    NULL
};
static struct attribute *NBA715_POWER_attributes[] =
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
static struct attribute *NBA715_QSFP_attributes[] =
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
    &sensor_dev_attr_qsfp9_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp10_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp11_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp12_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp13_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp14_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp15_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp16_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp17_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp18_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp19_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp20_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp21_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp22_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp23_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp24_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp25_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp26_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp27_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp28_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp29_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp30_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp31_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp32_low_power.dev_attr.attr,
    &sensor_dev_attr_qsfp_reset_all.dev_attr.attr,
    &sensor_dev_attr_qsfp1_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp2_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp3_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp4_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp5_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp6_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp7_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp8_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp9_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp10_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp11_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp12_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp13_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp14_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp15_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp16_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp17_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp18_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp19_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp20_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp21_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp22_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp23_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp24_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp25_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp26_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp27_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp28_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp29_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp30_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp31_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp32_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp_present_all.dev_attr.attr,
    &sensor_dev_attr_qsfp1_present.dev_attr.attr,
    &sensor_dev_attr_qsfp2_present.dev_attr.attr,
    &sensor_dev_attr_qsfp3_present.dev_attr.attr,
    &sensor_dev_attr_qsfp4_present.dev_attr.attr,
    &sensor_dev_attr_qsfp5_present.dev_attr.attr,
    &sensor_dev_attr_qsfp6_present.dev_attr.attr,
    &sensor_dev_attr_qsfp7_present.dev_attr.attr,
    &sensor_dev_attr_qsfp8_present.dev_attr.attr,
    &sensor_dev_attr_qsfp9_present.dev_attr.attr,
    &sensor_dev_attr_qsfp10_present.dev_attr.attr,
    &sensor_dev_attr_qsfp11_present.dev_attr.attr,
    &sensor_dev_attr_qsfp12_present.dev_attr.attr,
    &sensor_dev_attr_qsfp13_present.dev_attr.attr,
    &sensor_dev_attr_qsfp14_present.dev_attr.attr,
    &sensor_dev_attr_qsfp15_present.dev_attr.attr,
    &sensor_dev_attr_qsfp16_present.dev_attr.attr,
    &sensor_dev_attr_qsfp17_present.dev_attr.attr,
    &sensor_dev_attr_qsfp18_present.dev_attr.attr,
    &sensor_dev_attr_qsfp19_present.dev_attr.attr,
    &sensor_dev_attr_qsfp20_present.dev_attr.attr,
    &sensor_dev_attr_qsfp21_present.dev_attr.attr,
    &sensor_dev_attr_qsfp22_present.dev_attr.attr,
    &sensor_dev_attr_qsfp23_present.dev_attr.attr,
    &sensor_dev_attr_qsfp24_present.dev_attr.attr,
    &sensor_dev_attr_qsfp25_present.dev_attr.attr,
    &sensor_dev_attr_qsfp26_present.dev_attr.attr,
    &sensor_dev_attr_qsfp27_present.dev_attr.attr,
    &sensor_dev_attr_qsfp28_present.dev_attr.attr,
    &sensor_dev_attr_qsfp29_present.dev_attr.attr,
    &sensor_dev_attr_qsfp30_present.dev_attr.attr,
    &sensor_dev_attr_qsfp31_present.dev_attr.attr,
    &sensor_dev_attr_qsfp32_present.dev_attr.attr,
    &sensor_dev_attr_qsfp_int_all.dev_attr.attr,
    &sensor_dev_attr_qsfp1_int.dev_attr.attr,
    &sensor_dev_attr_qsfp2_int.dev_attr.attr,
    &sensor_dev_attr_qsfp3_int.dev_attr.attr,
    &sensor_dev_attr_qsfp4_int.dev_attr.attr,
    &sensor_dev_attr_qsfp5_int.dev_attr.attr,
    &sensor_dev_attr_qsfp6_int.dev_attr.attr,
    &sensor_dev_attr_qsfp7_int.dev_attr.attr,
    &sensor_dev_attr_qsfp8_int.dev_attr.attr,
    &sensor_dev_attr_qsfp9_int.dev_attr.attr,
    &sensor_dev_attr_qsfp10_int.dev_attr.attr,
    &sensor_dev_attr_qsfp11_int.dev_attr.attr,
    &sensor_dev_attr_qsfp12_int.dev_attr.attr,
    &sensor_dev_attr_qsfp13_int.dev_attr.attr,
    &sensor_dev_attr_qsfp14_int.dev_attr.attr,
    &sensor_dev_attr_qsfp15_int.dev_attr.attr,
    &sensor_dev_attr_qsfp16_int.dev_attr.attr,
    &sensor_dev_attr_qsfp17_int.dev_attr.attr,
    &sensor_dev_attr_qsfp18_int.dev_attr.attr,
    &sensor_dev_attr_qsfp19_int.dev_attr.attr,
    &sensor_dev_attr_qsfp20_int.dev_attr.attr,
    &sensor_dev_attr_qsfp21_int.dev_attr.attr,
    &sensor_dev_attr_qsfp22_int.dev_attr.attr,
    &sensor_dev_attr_qsfp23_int.dev_attr.attr,
    &sensor_dev_attr_qsfp24_int.dev_attr.attr,
    &sensor_dev_attr_qsfp25_int.dev_attr.attr,
    &sensor_dev_attr_qsfp26_int.dev_attr.attr,
    &sensor_dev_attr_qsfp27_int.dev_attr.attr,
    &sensor_dev_attr_qsfp28_int.dev_attr.attr,
    &sensor_dev_attr_qsfp29_int.dev_attr.attr,
    &sensor_dev_attr_qsfp30_int.dev_attr.attr,
    &sensor_dev_attr_qsfp31_int.dev_attr.attr,
    &sensor_dev_attr_qsfp32_int.dev_attr.attr,
    &sensor_dev_attr_qsfp1_4_int.dev_attr.attr,
    &sensor_dev_attr_qsfp2_4_int.dev_attr.attr,
    &sensor_dev_attr_qsfp3_4_int.dev_attr.attr,
    &sensor_dev_attr_qsfp4_4_int.dev_attr.attr,
    &sensor_dev_attr_qsfp1_4_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp2_4_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp3_4_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp4_4_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp1_4_int_mask.dev_attr.attr,
    &sensor_dev_attr_qsfp2_4_int_mask.dev_attr.attr,
    &sensor_dev_attr_qsfp3_4_int_mask.dev_attr.attr,
    &sensor_dev_attr_qsfp4_4_int_mask.dev_attr.attr,
    &sensor_dev_attr_qsfp1_4_modprs_mask.dev_attr.attr,
    &sensor_dev_attr_qsfp2_4_modprs_mask.dev_attr.attr,
    &sensor_dev_attr_qsfp3_4_modprs_mask.dev_attr.attr,
    &sensor_dev_attr_qsfp4_4_modprs_mask.dev_attr.attr,
    NULL
};
/* end of sysfs attributes for hwmon */

/* struct attribute_group */
static const struct attribute_group NBA715_SYS_group =
{
    .name  = "NBA715_SYS",
    .attrs = NBA715_SYS_attributes,
};

static const struct attribute_group NBA715_LED_group =
{
    .name  = "NBA715_LED",
    .attrs = NBA715_LED_attributes,
};

static const struct attribute_group NBA715_THERMAL_group =
{
    .name  = "NBA715_THERMAL",
    .attrs = NBA715_THERMAL_attributes,
};

static const struct attribute_group NBA715_FAN_group =
{
    .name  = "NBA715_FAN",
    .attrs = NBA715_FAN_attributes,
};

static const struct attribute_group NBA715_POWER_group =
{
    .name  = "NBA715_POWER",
    .attrs = NBA715_POWER_attributes,
};

static const struct attribute_group NBA715_QSFP_group =
{
    .name  = "NBA715_QSFP",
    .attrs = NBA715_QSFP_attributes,
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
