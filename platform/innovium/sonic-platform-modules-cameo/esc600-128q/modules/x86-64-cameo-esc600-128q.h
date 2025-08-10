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

#define DRIVER_VERSION  "1.0.6"

struct i2c_adap {
	int nr;
	char *name;
	const char *funcs;
	const char *algo;
};

struct i2c_adap *gather_i2c_busses(void);
void free_adapters(struct i2c_adap *adapters);

/* compiler conditional */
/* end of compiler conditional */

/* Function Declaration */
/* x86-64-cameo-esc600-128q-sys.c */
ssize_t cpld_hw_ver_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t wdt_enable_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t wdt_enable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t eeprom_wp_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t eeprom_wp_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t usb_enable_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t usb_enable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t reset_mac_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t shutdown_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t bmc_enable_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t switch_alarm_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t switch_alarm_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t switch_alarm_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t sensor_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sersor_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sersor_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t module_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t module_insert_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t module_power_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t module_enable_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t module_enable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t switch_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t switch_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t switch_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t sfp_select_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_select_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t sfp_tx_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_tx_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t sfp_insert_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_rx_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sys_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sys_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sys_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t thermal_int_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t thermal_int_mask_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t thermal_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
/* x86-64-cameo-esc600-128q-led.c */
ssize_t led_ctrl_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t led_ctrl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t switch_led_4_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t switch_led_4_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t switch_led_5_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t switch_led_5_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t led_fiber_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t led_fiber_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
/* x86-64-cameo-esc600-128q-thermal.c */
ssize_t line_card_temp_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t themal_temp_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t themal_temp_max_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t themal_temp_min_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t themal_temp_crit_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t themal_temp_lcrit_get(struct device *dev, struct device_attribute *da, char *buf);
/* x86-64-cameo-esc600-128q-fan.c */
ssize_t fan_ctrl_mode_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_ctrl_rpm_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_status_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_present_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_power_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_direct_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t fan_rpm_get(struct device *dev, struct device_attribute *da, char *buf);
/* x86-64-cameo-esc600-128q-power.c */
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
ssize_t dc_11_p0_vout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_11_p1_vout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_12_p0_vout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_12_p1_vout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_13_p0_vout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_13_p1_vout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_11_p0_iout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_11_p1_iout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_12_p0_iout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_12_p1_iout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_13_p0_iout_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t dc_13_p1_iout_get(struct device *dev, struct device_attribute *da, char *buf);
/* end of Function Declaration */

/* struct i2c_data */
struct Cameo_i2c_data
{
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;
    unsigned long       last_updated;
    u8  status;
};

/* struct i2c_sysfs_attributes */
enum Cameo_i2c_sysfs_attributes
{
    /* x86-64-cameo-esc600-128q-sys.c */
    CPLD_30_VER,
    CPLD_31_VER,
    CPLD_33_VER,
    WDT_EN,
    EEPROM_WP,
    USB_EN,
    SHUTDOWN_SET,
    RESET,
    BMC_PRESENT,
    SW_ALERT_TH0,
    SW_ALERT_TH1,
    SW_ALERT_TH2,
    SW_ALERT_TH3,
    SW_ALERT_TH0_MASK,
    SW_ALERT_TH1_MASK,
    SW_ALERT_TH2_MASK,
    SW_ALERT_TH3_MASK,
    CB_INT,
    SB_INT,
    CB_INT_MASK,
    SB_INT_MASK,
    MODULE_RESET,
    MODULE_1_PRESENT,
    MODULE_2_PRESENT,
    MODULE_3_PRESENT,
    MODULE_4_PRESENT,
    MODULE_5_PRESENT,
    MODULE_6_PRESENT,
    MODULE_7_PRESENT,
    MODULE_8_PRESENT,
    MODULE_1_POWER,
    MODULE_2_POWER,
    MODULE_3_POWER,
    MODULE_4_POWER,
    MODULE_5_POWER,
    MODULE_6_POWER,
    MODULE_7_POWER,
    MODULE_8_POWER,
    MODULE_1_ENABLE,
    MODULE_2_ENABLE,
    MODULE_3_ENABLE,
    MODULE_4_ENABLE,
    MODULE_5_ENABLE,
    MODULE_6_ENABLE,
    MODULE_7_ENABLE,
    MODULE_8_ENABLE,
    MODULE_INS_INT,
    MODULE_INT,
    MODULE_POWER_INT,
    THER_SENSOR_INT,
    IO_BOARD_INT,
    FAN_ERROR_INT,
    PHY_POWER_INT,
    SW_POWER_INT,
    MODULE_INS_INT_MASK,
    MODULE_INT_MASK,
    MODULE_POW_INT_MASK,
    THER_SEN_INT_MASK,
    IO_BOARD_INT_MASK,
    FAN_ERROR_INT_MASK,
    PHY_POWER_INT_MASK,
    SW_POWER_INT_MASK,
    SFP_SELECT,
    SFP_PORT_TX_1,
    SFP_PORT_TX_2,
    SFP_PORT_TX_MGM,
    SFP_PORT_1,
    SFP_PORT_2,
    SFP_PORT_MGM,
    SFP_PORT_RX_1,
    SFP_PORT_RX_2,
    SFP_PORT_RX_MGM,
    SFP_LOSS_INT,
    SFP_ABS_INT,
    SFP_LOSS_MASK,
    SFP_ABS_MASK,
    ALERT_TH0_INT,
    ALERT_TH1_INT,
    ALERT_TH2_INT,
    ALERT_TH3_INT,
    ALERT_TH4_INT,
    ALERT_TH5_INT,
    ALERT_TH0_INT_MASK,
    ALERT_TH1_INT_MASK,
    ALERT_TH2_INT_MASK,
    ALERT_TH3_INT_MASK,
    ALERT_TH4_INT_MASK,
    ALERT_TH5_INT_MASK,
    /* x86-64-cameo-esc600-128q-led.c */
    LED_SYS,
    LED_LOC,
    LED_FLOW,
    LED_4_1,
    LED_4_2,
    LED_4_3,
    LED_4_4,
    LED_5_1,
    LED_5_2,
    LED_5_3,
    LED_5_4,
    LED_FIBER,
    /* x86-64-cameo-esc600-128q-thermal.c */
    LINE_CARD_1_UP_TEMP,
    LINE_CARD_2_UP_TEMP,
    LINE_CARD_3_UP_TEMP,
    LINE_CARD_4_UP_TEMP,
    LINE_CARD_5_UP_TEMP,
    LINE_CARD_6_UP_TEMP,
    LINE_CARD_7_UP_TEMP,
    LINE_CARD_8_UP_TEMP,
    LINE_CARD_1_DN_TEMP,
    LINE_CARD_2_DN_TEMP,
    LINE_CARD_3_DN_TEMP,
    LINE_CARD_4_DN_TEMP,
    LINE_CARD_5_DN_TEMP,
    LINE_CARD_6_DN_TEMP,
    LINE_CARD_7_DN_TEMP,
    LINE_CARD_8_DN_TEMP,
    NCT7511_TEMP,
    LEFT_BOT_SB_TEMP,
    CTR_TOP_SB_TEMP,
    CTR_SB_TEMP,
    LEFT_TOP_CB_TEMP,
    CTR_CB_TEMP,
    RIGHT_BOT_CB_TEMP,
    LEFT_BOT_CB_TEMP,
    IO_BOARD_TEMP,
    /* x86-64-cameo-esc600-128q-fan.c */
    FANCTRL_MODE,
    FANCTRL_RPM,
    FAN_1_STAT,
    FAN_2_STAT,
    FAN_3_STAT,
    FAN_4_STAT,
    FAN_1_PRESENT,
    FAN_2_PRESENT,
    FAN_3_PRESENT,
    FAN_4_PRESENT,
    FAN_1_POWER,
    FAN_2_POWER,
    FAN_3_POWER,
    FAN_4_POWER,
    FAN_1_DIRECT,
    FAN_2_DIRECT,
    FAN_3_DIRECT,
    FAN_4_DIRECT,
    FAN_1_RPM,
    FAN_2_RPM,
    FAN_3_RPM,
    FAN_4_RPM,
    /* x86-64-cameo-esc600-128q-power.c */
    PSU_1_STST,
    PSU_2_STST,
    PSU_3_STST,
    PSU_4_STST,
    PSU_1_PRESENT,
    PSU_2_PRESENT,
    PSU_3_PRESENT,
    PSU_4_PRESENT,
    PSU1_VIN,
    PSU2_VIN,
    PSU3_VIN,
    PSU4_VIN,
    PSU1_IIN,
    PSU2_IIN,
    PSU3_IIN,
    PSU4_IIN,
    PSU1_VOUT,
    PSU2_VOUT,
    PSU3_VOUT,
    PSU4_VOUT,
    PSU1_IOUT,
    PSU2_IOUT,
    PSU3_IOUT,
    PSU4_IOUT,
    PSU1_TEMP,
    PSU2_TEMP,
    PSU3_TEMP,
    PSU4_TEMP,
    PSU1_FAN_SPEED,
    PSU2_FAN_SPEED,
    PSU3_FAN_SPEED,
    PSU4_FAN_SPEED,
    PSU1_POUT,
    PSU2_POUT,
    PSU3_POUT,
    PSU4_POUT, 
    PSU1_PIN,
    PSU2_PIN,
    PSU3_PIN,
    PSU4_PIN,
    PSU1_MFR_MODEL,
    PSU2_MFR_MODEL,
    PSU3_MFR_MODEL,
    PSU4_MFR_MODEL,
    PSU1_MFR_IOUT_MAX,
    PSU2_MFR_IOUT_MAX,
    PSU3_MFR_IOUT_MAX,
    PSU4_MFR_IOUT_MAX,
    PSU1_VMODE,
    PSU2_VMODE,
    PSU3_VMODE,
    PSU4_VMODE,
    DC_6E_P0_VOUT,
    DC_70_P0_VOUT,
    DC_70_P1_VOUT,
    DC_6E_P0_IOUT,
    DC_70_P0_IOUT,
    DC_70_P1_IOUT,
    DC_6E_P0_POUT,
    DC_70_P0_POUT,
    DC_70_P1_POUT,
    CARD_1_DC_11_P0_VOUT,
    CARD_2_DC_11_P0_VOUT,
    CARD_3_DC_11_P0_VOUT,
    CARD_4_DC_11_P0_VOUT,
    CARD_5_DC_11_P0_VOUT,
    CARD_6_DC_11_P0_VOUT,
    CARD_7_DC_11_P0_VOUT,
    CARD_8_DC_11_P0_VOUT,
    CARD_1_DC_11_P1_VOUT,
    CARD_2_DC_11_P1_VOUT,
    CARD_3_DC_11_P1_VOUT,
    CARD_4_DC_11_P1_VOUT,
    CARD_5_DC_11_P1_VOUT,
    CARD_6_DC_11_P1_VOUT,
    CARD_7_DC_11_P1_VOUT,
    CARD_8_DC_11_P1_VOUT,
    CARD_1_DC_12_P0_VOUT,
    CARD_2_DC_12_P0_VOUT,
    CARD_3_DC_12_P0_VOUT,
    CARD_4_DC_12_P0_VOUT,
    CARD_5_DC_12_P0_VOUT,
    CARD_6_DC_12_P0_VOUT,
    CARD_7_DC_12_P0_VOUT,
    CARD_8_DC_12_P0_VOUT,
    CARD_1_DC_12_P1_VOUT,
    CARD_2_DC_12_P1_VOUT,
    CARD_3_DC_12_P1_VOUT,
    CARD_4_DC_12_P1_VOUT,
    CARD_5_DC_12_P1_VOUT,
    CARD_6_DC_12_P1_VOUT,
    CARD_7_DC_12_P1_VOUT,
    CARD_8_DC_12_P1_VOUT,
    CARD_1_DC_13_P0_VOUT,
    CARD_2_DC_13_P0_VOUT,
    CARD_3_DC_13_P0_VOUT,
    CARD_4_DC_13_P0_VOUT,
    CARD_5_DC_13_P0_VOUT,
    CARD_6_DC_13_P0_VOUT,
    CARD_7_DC_13_P0_VOUT,
    CARD_8_DC_13_P0_VOUT,
    CARD_1_DC_13_P1_VOUT,
    CARD_2_DC_13_P1_VOUT,
    CARD_3_DC_13_P1_VOUT,
    CARD_4_DC_13_P1_VOUT,
    CARD_5_DC_13_P1_VOUT,
    CARD_6_DC_13_P1_VOUT,
    CARD_7_DC_13_P1_VOUT,
    CARD_8_DC_13_P1_VOUT,
    CARD_1_DC_11_P0_IOUT,
    CARD_2_DC_11_P0_IOUT,
    CARD_3_DC_11_P0_IOUT,
    CARD_4_DC_11_P0_IOUT,
    CARD_5_DC_11_P0_IOUT,
    CARD_6_DC_11_P0_IOUT,
    CARD_7_DC_11_P0_IOUT,
    CARD_8_DC_11_P0_IOUT,
    CARD_1_DC_11_P1_IOUT,
    CARD_2_DC_11_P1_IOUT,
    CARD_3_DC_11_P1_IOUT,
    CARD_4_DC_11_P1_IOUT,
    CARD_5_DC_11_P1_IOUT,
    CARD_6_DC_11_P1_IOUT,
    CARD_7_DC_11_P1_IOUT,
    CARD_8_DC_11_P1_IOUT,
    CARD_1_DC_12_P0_IOUT,
    CARD_2_DC_12_P0_IOUT,
    CARD_3_DC_12_P0_IOUT,
    CARD_4_DC_12_P0_IOUT,
    CARD_5_DC_12_P0_IOUT,
    CARD_6_DC_12_P0_IOUT,
    CARD_7_DC_12_P0_IOUT,
    CARD_8_DC_12_P0_IOUT,
    CARD_1_DC_12_P1_IOUT,
    CARD_2_DC_12_P1_IOUT,
    CARD_3_DC_12_P1_IOUT,
    CARD_4_DC_12_P1_IOUT,
    CARD_5_DC_12_P1_IOUT,
    CARD_6_DC_12_P1_IOUT,
    CARD_7_DC_12_P1_IOUT,
    CARD_8_DC_12_P1_IOUT,
    CARD_1_DC_13_P0_IOUT,
    CARD_2_DC_13_P0_IOUT,
    CARD_3_DC_13_P0_IOUT,
    CARD_4_DC_13_P0_IOUT,
    CARD_5_DC_13_P0_IOUT,
    CARD_6_DC_13_P0_IOUT,
    CARD_7_DC_13_P0_IOUT,
    CARD_8_DC_13_P0_IOUT,
    CARD_1_DC_13_P1_IOUT,
    CARD_2_DC_13_P1_IOUT,
    CARD_3_DC_13_P1_IOUT,
    CARD_4_DC_13_P1_IOUT,
    CARD_5_DC_13_P1_IOUT,
    CARD_6_DC_13_P1_IOUT,
    CARD_7_DC_13_P1_IOUT,
    CARD_8_DC_13_P1_IOUT,
};
/* end of struct i2c_sysfs_attributes */

/* sysfs attributes for SENSOR_DEVICE_ATTR */
 /* x86-64-cameo-esc600-128q-sys.c */
 static SENSOR_DEVICE_ATTR(cpld_30_ver              , S_IRUGO           , cpld_hw_ver_get           , NULL                      , 30);
 static SENSOR_DEVICE_ATTR(cpld_31_ver              , S_IRUGO           , cpld_hw_ver_get           , NULL                      , 31);
 static SENSOR_DEVICE_ATTR(cpld_33_ver              , S_IRUGO           , cpld_hw_ver_get           , NULL                      , 33);
 static SENSOR_DEVICE_ATTR(wdt_en                   , S_IRUGO | S_IWUSR , wdt_enable_get            , wdt_enable_set            , WDT_EN);
 static SENSOR_DEVICE_ATTR(eeprom_wp                , S_IRUGO | S_IWUSR , eeprom_wp_get             , eeprom_wp_set             , EEPROM_WP);
 static SENSOR_DEVICE_ATTR(usb_en                   , S_IRUGO | S_IWUSR , usb_enable_get            , usb_enable_set            , USB_EN);
 static SENSOR_DEVICE_ATTR(shutdown_set             , S_IRUGO | S_IWUSR , NULL                      , shutdown_set              , SHUTDOWN_SET);
 static SENSOR_DEVICE_ATTR(reset                    , S_IRUGO | S_IWUSR , NULL                      , reset_mac_set             , RESET);
 static SENSOR_DEVICE_ATTR(bmc_present              , S_IRUGO           , bmc_enable_get            , NULL                      , BMC_PRESENT);
 static SENSOR_DEVICE_ATTR(sw_alert_th0             , S_IRUGO           , switch_alarm_get          , NULL                      , SW_ALERT_TH0);
 static SENSOR_DEVICE_ATTR(sw_alert_th1             , S_IRUGO           , switch_alarm_get          , NULL                      , SW_ALERT_TH1);
 static SENSOR_DEVICE_ATTR(sw_alert_th2             , S_IRUGO           , switch_alarm_get          , NULL                      , SW_ALERT_TH2);
 static SENSOR_DEVICE_ATTR(sw_alert_th3             , S_IRUGO           , switch_alarm_get          , NULL                      , SW_ALERT_TH3);
 static SENSOR_DEVICE_ATTR(sw_alert_th0_mask        , S_IRUGO | S_IWUSR , switch_alarm_mask_get     , switch_alarm_mask_set     , SW_ALERT_TH0_MASK);
 static SENSOR_DEVICE_ATTR(sw_alert_th1_mask        , S_IRUGO | S_IWUSR , switch_alarm_mask_get     , switch_alarm_mask_set     , SW_ALERT_TH1_MASK);
 static SENSOR_DEVICE_ATTR(sw_alert_th2_mask        , S_IRUGO | S_IWUSR , switch_alarm_mask_get     , switch_alarm_mask_set     , SW_ALERT_TH2_MASK);
 static SENSOR_DEVICE_ATTR(sw_alert_th3_mask        , S_IRUGO | S_IWUSR , switch_alarm_mask_get     , switch_alarm_mask_set     , SW_ALERT_TH3_MASK);
 static SENSOR_DEVICE_ATTR(cb_int                   , S_IRUGO           , sensor_int_get            , NULL                      , CB_INT);
 static SENSOR_DEVICE_ATTR(sb_int                   , S_IRUGO           , sensor_int_get            , NULL                      , SB_INT);
 static SENSOR_DEVICE_ATTR(cb_int_mask              , S_IRUGO | S_IWUSR , sersor_int_mask_get       , sersor_int_mask_set       , CB_INT_MASK);
 static SENSOR_DEVICE_ATTR(sb_int_mask              , S_IRUGO | S_IWUSR , sersor_int_mask_get       , sersor_int_mask_set       , SB_INT_MASK);
 static SENSOR_DEVICE_ATTR(module_reset             , S_IRUGO | S_IWUSR , NULL                      , module_reset_set          , MODULE_RESET);
 static SENSOR_DEVICE_ATTR(module_1_present         , S_IRUGO           , module_insert_get         , NULL                      , MODULE_1_PRESENT);
 static SENSOR_DEVICE_ATTR(module_2_present         , S_IRUGO           , module_insert_get         , NULL                      , MODULE_2_PRESENT);
 static SENSOR_DEVICE_ATTR(module_3_present         , S_IRUGO           , module_insert_get         , NULL                      , MODULE_3_PRESENT);
 static SENSOR_DEVICE_ATTR(module_4_present         , S_IRUGO           , module_insert_get         , NULL                      , MODULE_4_PRESENT);
 static SENSOR_DEVICE_ATTR(module_5_present         , S_IRUGO           , module_insert_get         , NULL                      , MODULE_5_PRESENT);
 static SENSOR_DEVICE_ATTR(module_6_present         , S_IRUGO           , module_insert_get         , NULL                      , MODULE_6_PRESENT);
 static SENSOR_DEVICE_ATTR(module_7_present         , S_IRUGO           , module_insert_get         , NULL                      , MODULE_7_PRESENT);
 static SENSOR_DEVICE_ATTR(module_8_present         , S_IRUGO           , module_insert_get         , NULL                      , MODULE_8_PRESENT);
 static SENSOR_DEVICE_ATTR(module_1_power           , S_IRUGO           , module_power_get          , NULL                      , MODULE_1_POWER);
 static SENSOR_DEVICE_ATTR(module_2_power           , S_IRUGO           , module_power_get          , NULL                      , MODULE_2_POWER);
 static SENSOR_DEVICE_ATTR(module_3_power           , S_IRUGO           , module_power_get          , NULL                      , MODULE_3_POWER);
 static SENSOR_DEVICE_ATTR(module_4_power           , S_IRUGO           , module_power_get          , NULL                      , MODULE_4_POWER);
 static SENSOR_DEVICE_ATTR(module_5_power           , S_IRUGO           , module_power_get          , NULL                      , MODULE_5_POWER);
 static SENSOR_DEVICE_ATTR(module_6_power           , S_IRUGO           , module_power_get          , NULL                      , MODULE_6_POWER);
 static SENSOR_DEVICE_ATTR(module_7_power           , S_IRUGO           , module_power_get          , NULL                      , MODULE_7_POWER);
 static SENSOR_DEVICE_ATTR(module_8_power           , S_IRUGO           , module_power_get          , NULL                      , MODULE_8_POWER);
 static SENSOR_DEVICE_ATTR(module_1_enable          , S_IRUGO | S_IWUSR , module_enable_get         , module_enable_set         , MODULE_1_ENABLE);
 static SENSOR_DEVICE_ATTR(module_2_enable          , S_IRUGO | S_IWUSR , module_enable_get         , module_enable_set         , MODULE_2_ENABLE);
 static SENSOR_DEVICE_ATTR(module_3_enable          , S_IRUGO | S_IWUSR , module_enable_get         , module_enable_set         , MODULE_3_ENABLE);
 static SENSOR_DEVICE_ATTR(module_4_enable          , S_IRUGO | S_IWUSR , module_enable_get         , module_enable_set         , MODULE_4_ENABLE);
 static SENSOR_DEVICE_ATTR(module_5_enable          , S_IRUGO | S_IWUSR , module_enable_get         , module_enable_set         , MODULE_5_ENABLE);
 static SENSOR_DEVICE_ATTR(module_6_enable          , S_IRUGO | S_IWUSR , module_enable_get         , module_enable_set         , MODULE_6_ENABLE);
 static SENSOR_DEVICE_ATTR(module_7_enable          , S_IRUGO | S_IWUSR , module_enable_get         , module_enable_set         , MODULE_7_ENABLE);
 static SENSOR_DEVICE_ATTR(module_8_enable          , S_IRUGO | S_IWUSR , module_enable_get         , module_enable_set         , MODULE_8_ENABLE);
 static SENSOR_DEVICE_ATTR(module_ins_int           , S_IRUGO           , switch_int_get            , NULL                      , MODULE_INS_INT);
 static SENSOR_DEVICE_ATTR(module_int               , S_IRUGO           , switch_int_get            , NULL                      , MODULE_INT);
 static SENSOR_DEVICE_ATTR(module_power_int         , S_IRUGO           , switch_int_get            , NULL                      , MODULE_POWER_INT);
 static SENSOR_DEVICE_ATTR(ther_sensor_int          , S_IRUGO           , switch_int_get            , NULL                      , THER_SENSOR_INT);
 static SENSOR_DEVICE_ATTR(io_board_int             , S_IRUGO           , switch_int_get            , NULL                      , IO_BOARD_INT);
 static SENSOR_DEVICE_ATTR(fan_error_int            , S_IRUGO           , switch_int_get            , NULL                      , FAN_ERROR_INT);
 static SENSOR_DEVICE_ATTR(phy_power_int            , S_IRUGO           , switch_int_get            , NULL                      , PHY_POWER_INT);
 static SENSOR_DEVICE_ATTR(sw_power_int             , S_IRUGO           , switch_int_get            , NULL                      , SW_POWER_INT);
 static SENSOR_DEVICE_ATTR(module_ins_int_mask      , S_IRUGO | S_IWUSR , switch_int_mask_get       , switch_int_mask_set       , MODULE_INS_INT_MASK);
 static SENSOR_DEVICE_ATTR(module_int_mask          , S_IRUGO | S_IWUSR , switch_int_mask_get       , switch_int_mask_set       , MODULE_INT_MASK);
 static SENSOR_DEVICE_ATTR(module_pow_int_mask      , S_IRUGO | S_IWUSR , switch_int_mask_get       , switch_int_mask_set       , MODULE_POW_INT_MASK);
 static SENSOR_DEVICE_ATTR(ther_sen_int_mask        , S_IRUGO | S_IWUSR , switch_int_mask_get       , switch_int_mask_set       , THER_SEN_INT_MASK);
 static SENSOR_DEVICE_ATTR(io_board_int_mask        , S_IRUGO | S_IWUSR , switch_int_mask_get       , switch_int_mask_set       , IO_BOARD_INT_MASK);
 static SENSOR_DEVICE_ATTR(fan_error_int_mask       , S_IRUGO | S_IWUSR , switch_int_mask_get       , switch_int_mask_set       , FAN_ERROR_INT_MASK);
 static SENSOR_DEVICE_ATTR(phy_power_int_mask       , S_IRUGO | S_IWUSR , switch_int_mask_get       , switch_int_mask_set       , PHY_POWER_INT_MASK);
 static SENSOR_DEVICE_ATTR(sw_power_int_mask        , S_IRUGO | S_IWUSR , switch_int_mask_get       , switch_int_mask_set       , SW_POWER_INT_MASK);
 static SENSOR_DEVICE_ATTR(sfp_select               , S_IRUGO | S_IWUSR , sfp_select_get            , sfp_select_set            , SFP_SELECT);
 static SENSOR_DEVICE_ATTR(sfp_port_tx_1            , S_IRUGO | S_IWUSR , sfp_tx_get                , sfp_tx_set                , 1);
 static SENSOR_DEVICE_ATTR(sfp_port_tx_2            , S_IRUGO | S_IWUSR , sfp_tx_get                , sfp_tx_set                , 2);
 static SENSOR_DEVICE_ATTR(sfp_port_tx_mgm          , S_IRUGO | S_IWUSR , sfp_tx_get                , sfp_tx_set                , 3);
 static SENSOR_DEVICE_ATTR(sfp_port_1               , S_IRUGO           , sfp_insert_get            , NULL                      , SFP_PORT_1);
 static SENSOR_DEVICE_ATTR(sfp_port_2               , S_IRUGO           , sfp_insert_get            , NULL                      , SFP_PORT_2);
 static SENSOR_DEVICE_ATTR(sfp_port_mgm             , S_IRUGO           , sfp_insert_get            , NULL                      , SFP_PORT_MGM);
 static SENSOR_DEVICE_ATTR(sfp_port_rx_1            , S_IRUGO           , sfp_rx_get                , NULL                      , SFP_PORT_RX_1);
 static SENSOR_DEVICE_ATTR(sfp_port_rx_2            , S_IRUGO           , sfp_rx_get                , NULL                      , SFP_PORT_RX_2);
 static SENSOR_DEVICE_ATTR(sfp_port_rx_mgm          , S_IRUGO           , sfp_rx_get                , NULL                      , SFP_PORT_RX_MGM);
 static SENSOR_DEVICE_ATTR(sfp_loss_int             , S_IRUGO           , sys_int_get               , NULL                      , SFP_LOSS_INT);
 static SENSOR_DEVICE_ATTR(sfp_abs_int              , S_IRUGO           , sys_int_get               , NULL                      , SFP_ABS_INT);
 static SENSOR_DEVICE_ATTR(sfp_loss_mask            , S_IRUGO | S_IWUSR , sys_int_mask_get          , sys_int_mask_set          , SFP_LOSS_MASK);
 static SENSOR_DEVICE_ATTR(sfp_abs_mask             , S_IRUGO | S_IWUSR , sys_int_mask_get          , sys_int_mask_set          , SFP_ABS_MASK);
 static SENSOR_DEVICE_ATTR(alert_th0_int            , S_IRUGO           , thermal_int_get           , NULL                      , ALERT_TH0_INT);
 static SENSOR_DEVICE_ATTR(alert_th1_int            , S_IRUGO           , thermal_int_get           , NULL                      , ALERT_TH1_INT);
 static SENSOR_DEVICE_ATTR(alert_th2_int            , S_IRUGO           , thermal_int_get           , NULL                      , ALERT_TH2_INT);
 static SENSOR_DEVICE_ATTR(alert_th3_int            , S_IRUGO           , thermal_int_get           , NULL                      , ALERT_TH3_INT);
 static SENSOR_DEVICE_ATTR(alert_th4_int            , S_IRUGO           , thermal_int_get           , NULL                      , ALERT_TH4_INT);
 static SENSOR_DEVICE_ATTR(alert_th5_int            , S_IRUGO           , thermal_int_get           , NULL                      , ALERT_TH5_INT);
 static SENSOR_DEVICE_ATTR(alert_th0_int_mask       , S_IRUGO | S_IWUSR , thermal_int_mask_get      , thermal_int_mask_set      , ALERT_TH0_INT_MASK);
 static SENSOR_DEVICE_ATTR(alert_th1_int_mask       , S_IRUGO | S_IWUSR , thermal_int_mask_get      , thermal_int_mask_set      , ALERT_TH1_INT_MASK);
 static SENSOR_DEVICE_ATTR(alert_th2_int_mask       , S_IRUGO | S_IWUSR , thermal_int_mask_get      , thermal_int_mask_set      , ALERT_TH2_INT_MASK);
 static SENSOR_DEVICE_ATTR(alert_th3_int_mask       , S_IRUGO | S_IWUSR , thermal_int_mask_get      , thermal_int_mask_set      , ALERT_TH3_INT_MASK);
 static SENSOR_DEVICE_ATTR(alert_th4_int_mask       , S_IRUGO | S_IWUSR , thermal_int_mask_get      , thermal_int_mask_set      , ALERT_TH4_INT_MASK);
 static SENSOR_DEVICE_ATTR(alert_th5_int_mask       , S_IRUGO | S_IWUSR , thermal_int_mask_get      , thermal_int_mask_set      , ALERT_TH5_INT_MASK);
 /* x86-64-cameo-esc600-128q-led.c */
 static SENSOR_DEVICE_ATTR(led_sys                  , S_IRUGO | S_IWUSR , led_ctrl_get              , led_ctrl_set              , 1 );
 static SENSOR_DEVICE_ATTR(led_loc                  , S_IRUGO | S_IWUSR , led_ctrl_get              , led_ctrl_set              , 2 );
 static SENSOR_DEVICE_ATTR(led_flow                 , S_IRUGO | S_IWUSR , led_ctrl_get              , led_ctrl_set              , 3 );
 static SENSOR_DEVICE_ATTR(led_4_1                  , S_IRUGO | S_IWUSR , switch_led_4_get          , switch_led_4_set          , 1 );
 static SENSOR_DEVICE_ATTR(led_4_2                  , S_IRUGO | S_IWUSR , switch_led_4_get          , switch_led_4_set          , 2 );
 static SENSOR_DEVICE_ATTR(led_4_3                  , S_IRUGO | S_IWUSR , switch_led_4_get          , switch_led_4_set          , 3 );
 static SENSOR_DEVICE_ATTR(led_4_4                  , S_IRUGO | S_IWUSR , switch_led_4_get          , switch_led_4_set          , 4 );
 static SENSOR_DEVICE_ATTR(led_5_1                  , S_IRUGO | S_IWUSR , switch_led_5_get          , switch_led_5_set          , 1 );
 static SENSOR_DEVICE_ATTR(led_5_2                  , S_IRUGO | S_IWUSR , switch_led_5_get          , switch_led_5_set          , 2 );
 static SENSOR_DEVICE_ATTR(led_5_3                  , S_IRUGO | S_IWUSR , switch_led_5_get          , switch_led_5_set          , 3 );
 static SENSOR_DEVICE_ATTR(led_5_4                  , S_IRUGO | S_IWUSR , switch_led_5_get          , switch_led_5_set          , 4 );
 static SENSOR_DEVICE_ATTR(led_fiber                , S_IRUGO | S_IWUSR , led_fiber_get             , led_fiber_set             , LED_FIBER);
 /* x86-64-cameo-esc600-128q-thermal.c */
 static SENSOR_DEVICE_ATTR(line_card_1_up_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_1_UP_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_2_up_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_2_UP_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_3_up_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_3_UP_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_4_up_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_4_UP_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_5_up_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_5_UP_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_6_up_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_6_UP_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_7_up_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_7_UP_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_8_up_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_8_UP_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_1_dn_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_1_DN_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_2_dn_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_2_DN_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_3_dn_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_3_DN_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_4_dn_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_4_DN_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_5_dn_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_5_DN_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_6_dn_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_6_DN_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_7_dn_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_7_DN_TEMP);
 static SENSOR_DEVICE_ATTR(line_card_8_dn_temp      , S_IRUGO           , line_card_temp_get        , NULL                      , LINE_CARD_8_DN_TEMP);
 static SENSOR_DEVICE_ATTR(nct7511_temp             , S_IRUGO           , themal_temp_get           , NULL                      , NCT7511_TEMP);
 static SENSOR_DEVICE_ATTR(left_bot_sb_temp         , S_IRUGO           , themal_temp_get           , NULL                      , LEFT_BOT_SB_TEMP);
 static SENSOR_DEVICE_ATTR(ctr_top_sb_temp          , S_IRUGO           , themal_temp_get           , NULL                      , CTR_TOP_SB_TEMP);
 static SENSOR_DEVICE_ATTR(ctr_sb_temp              , S_IRUGO           , themal_temp_get           , NULL                      , CTR_SB_TEMP);
 static SENSOR_DEVICE_ATTR(left_top_cb_temp         , S_IRUGO           , themal_temp_get           , NULL                      , LEFT_TOP_CB_TEMP);
 static SENSOR_DEVICE_ATTR(ctr_cb_temp              , S_IRUGO           , themal_temp_get           , NULL                      , CTR_CB_TEMP);
 static SENSOR_DEVICE_ATTR(right_bot_cb_temp        , S_IRUGO           , themal_temp_get           , NULL                      , RIGHT_BOT_CB_TEMP);
 static SENSOR_DEVICE_ATTR(left_bot_cb_temp         , S_IRUGO           , themal_temp_get           , NULL                      , LEFT_BOT_CB_TEMP);
 static SENSOR_DEVICE_ATTR(io_board_temp            , S_IRUGO           , themal_temp_get           , NULL                      , IO_BOARD_TEMP);
 /* x86-64-cameo-esc600-128q-fan.c */
 static SENSOR_DEVICE_ATTR(fanctrl_mode             , S_IRUGO           , fan_ctrl_mode_get         , NULL                      , FANCTRL_MODE);
 static SENSOR_DEVICE_ATTR(fanctrl_rpm              , S_IRUGO           , fan_ctrl_rpm_get          , NULL                      , FANCTRL_RPM);
 static SENSOR_DEVICE_ATTR(fan1_stat               , S_IRUGO           , fan_status_get            , NULL                      , 1);
 static SENSOR_DEVICE_ATTR(fan2_stat               , S_IRUGO           , fan_status_get            , NULL                      , 2);
 static SENSOR_DEVICE_ATTR(fan3_stat               , S_IRUGO           , fan_status_get            , NULL                      , 3);
 static SENSOR_DEVICE_ATTR(fan4_stat               , S_IRUGO           , fan_status_get            , NULL                      , 4);
 static SENSOR_DEVICE_ATTR(fan1_present            , S_IRUGO           , fan_present_get           , NULL                      , 1);
 static SENSOR_DEVICE_ATTR(fan2_present            , S_IRUGO           , fan_present_get           , NULL                      , 2);
 static SENSOR_DEVICE_ATTR(fan3_present            , S_IRUGO           , fan_present_get           , NULL                      , 3);
 static SENSOR_DEVICE_ATTR(fan4_present            , S_IRUGO           , fan_present_get           , NULL                      , 4);
 static SENSOR_DEVICE_ATTR(fan1_power              , S_IRUGO           , fan_power_get             , NULL                      , 1);
 static SENSOR_DEVICE_ATTR(fan2_power              , S_IRUGO           , fan_power_get             , NULL                      , 2);
 static SENSOR_DEVICE_ATTR(fan3_power              , S_IRUGO           , fan_power_get             , NULL                      , 3);
 static SENSOR_DEVICE_ATTR(fan4_power              , S_IRUGO           , fan_power_get             , NULL                      , 4);
 static SENSOR_DEVICE_ATTR(fan1_direct             , S_IRUGO           , fan_direct_get            , NULL                      , 1);
 static SENSOR_DEVICE_ATTR(fan2_direct             , S_IRUGO           , fan_direct_get            , NULL                      , 2);
 static SENSOR_DEVICE_ATTR(fan3_direct             , S_IRUGO           , fan_direct_get            , NULL                      , 3);
 static SENSOR_DEVICE_ATTR(fan4_direct             , S_IRUGO           , fan_direct_get            , NULL                      , 4);
 static SENSOR_DEVICE_ATTR(fan1_rpm                , S_IRUGO           , fan_rpm_get               , NULL                      , FAN_1_RPM);
 static SENSOR_DEVICE_ATTR(fan2_rpm                , S_IRUGO           , fan_rpm_get               , NULL                      , FAN_2_RPM);
 static SENSOR_DEVICE_ATTR(fan3_rpm                , S_IRUGO           , fan_rpm_get               , NULL                      , FAN_3_RPM);
 static SENSOR_DEVICE_ATTR(fan4_rpm                , S_IRUGO           , fan_rpm_get               , NULL                      , FAN_4_RPM);
 /* x86-64-cameo-esc600-128q-power.c */
 static SENSOR_DEVICE_ATTR(psu1_good                , S_IRUGO           , psu_status_get            , NULL                      , 1);
 static SENSOR_DEVICE_ATTR(psu2_good                , S_IRUGO           , psu_status_get            , NULL                      , 2);
 static SENSOR_DEVICE_ATTR(psu3_good                , S_IRUGO           , psu_status_get            , NULL                      , 3);
 static SENSOR_DEVICE_ATTR(psu4_good                , S_IRUGO           , psu_status_get            , NULL                      , 4);
 static SENSOR_DEVICE_ATTR(psu1_prnt                , S_IRUGO           , psu_present_get           , NULL                      , 1);
 static SENSOR_DEVICE_ATTR(psu2_prnt                , S_IRUGO           , psu_present_get           , NULL                      , 2);
 static SENSOR_DEVICE_ATTR(psu3_prnt                , S_IRUGO           , psu_present_get           , NULL                      , 3);
 static SENSOR_DEVICE_ATTR(psu4_prnt                , S_IRUGO           , psu_present_get           , NULL                      , 4);
 static SENSOR_DEVICE_ATTR(psu1_vin                 , S_IRUGO           , psu_vin_get               , NULL                      , PSU1_VIN);
 static SENSOR_DEVICE_ATTR(psu2_vin                 , S_IRUGO           , psu_vin_get               , NULL                      , PSU2_VIN);
 static SENSOR_DEVICE_ATTR(psu3_vin                 , S_IRUGO           , psu_vin_get               , NULL                      , PSU3_VIN);
 static SENSOR_DEVICE_ATTR(psu4_vin                 , S_IRUGO           , psu_vin_get               , NULL                      , PSU4_VIN);
 static SENSOR_DEVICE_ATTR(psu1_iin                 , S_IRUGO           , psu_iin_get               , NULL                      , PSU1_IIN);
 static SENSOR_DEVICE_ATTR(psu2_iin                 , S_IRUGO           , psu_iin_get               , NULL                      , PSU2_IIN);
 static SENSOR_DEVICE_ATTR(psu3_iin                 , S_IRUGO           , psu_iin_get               , NULL                      , PSU3_IIN);
 static SENSOR_DEVICE_ATTR(psu4_iin                 , S_IRUGO           , psu_iin_get               , NULL                      , PSU4_IIN);
 static SENSOR_DEVICE_ATTR(psu1_vout                , S_IRUGO           , psu_vout_get              , NULL                      , PSU1_VOUT);
 static SENSOR_DEVICE_ATTR(psu2_vout                , S_IRUGO           , psu_vout_get              , NULL                      , PSU2_VOUT);
 static SENSOR_DEVICE_ATTR(psu3_vout                , S_IRUGO           , psu_vout_get              , NULL                      , PSU3_VOUT);
 static SENSOR_DEVICE_ATTR(psu4_vout                , S_IRUGO           , psu_vout_get              , NULL                      , PSU4_VOUT);
 static SENSOR_DEVICE_ATTR(psu1_iout                , S_IRUGO           , psu_iout_get              , NULL                      , PSU1_IOUT);
 static SENSOR_DEVICE_ATTR(psu2_iout                , S_IRUGO           , psu_iout_get              , NULL                      , PSU2_IOUT);
 static SENSOR_DEVICE_ATTR(psu3_iout                , S_IRUGO           , psu_iout_get              , NULL                      , PSU3_IOUT);
 static SENSOR_DEVICE_ATTR(psu4_iout                , S_IRUGO           , psu_iout_get              , NULL                      , PSU4_IOUT);
 static SENSOR_DEVICE_ATTR(psu1_temp                , S_IRUGO           , psu_temp_get              , NULL                      , PSU1_TEMP);
 static SENSOR_DEVICE_ATTR(psu2_temp                , S_IRUGO           , psu_temp_get              , NULL                      , PSU2_TEMP);
 static SENSOR_DEVICE_ATTR(psu3_temp                , S_IRUGO           , psu_temp_get              , NULL                      , PSU3_TEMP);
 static SENSOR_DEVICE_ATTR(psu4_temp                , S_IRUGO           , psu_temp_get              , NULL                      , PSU4_TEMP);
 static SENSOR_DEVICE_ATTR(psu1_fan_speed           , S_IRUGO           , psu_fan_get               , NULL                      , PSU1_FAN_SPEED);
 static SENSOR_DEVICE_ATTR(psu2_fan_speed           , S_IRUGO           , psu_fan_get               , NULL                      , PSU2_FAN_SPEED);
 static SENSOR_DEVICE_ATTR(psu3_fan_speed           , S_IRUGO           , psu_fan_get               , NULL                      , PSU3_FAN_SPEED);
 static SENSOR_DEVICE_ATTR(psu4_fan_speed           , S_IRUGO           , psu_fan_get               , NULL                      , PSU4_FAN_SPEED);
 static SENSOR_DEVICE_ATTR(psu1_pout                , S_IRUGO           , psu_pout_get              , NULL                      , PSU1_POUT);
 static SENSOR_DEVICE_ATTR(psu2_pout                , S_IRUGO           , psu_pout_get              , NULL                      , PSU2_POUT);
 static SENSOR_DEVICE_ATTR(psu3_pout                , S_IRUGO           , psu_pout_get              , NULL                      , PSU3_POUT);
 static SENSOR_DEVICE_ATTR(psu4_pout                , S_IRUGO           , psu_pout_get              , NULL                      , PSU4_POUT);
 static SENSOR_DEVICE_ATTR(psu1_pin                 , S_IRUGO           , psu_pin_get               , NULL                      , PSU1_PIN);
 static SENSOR_DEVICE_ATTR(psu2_pin                 , S_IRUGO           , psu_pin_get               , NULL                      , PSU2_PIN);
 static SENSOR_DEVICE_ATTR(psu3_pin                 , S_IRUGO           , psu_pin_get               , NULL                      , PSU3_PIN);
 static SENSOR_DEVICE_ATTR(psu4_pin                 , S_IRUGO           , psu_pin_get               , NULL                      , PSU4_PIN);
 static SENSOR_DEVICE_ATTR(psu1_mfr_model           , S_IRUGO           , psu_mfr_model_get         , NULL                      , PSU1_MFR_MODEL);
 static SENSOR_DEVICE_ATTR(psu2_mfr_model           , S_IRUGO           , psu_mfr_model_get         , NULL                      , PSU2_MFR_MODEL);
 static SENSOR_DEVICE_ATTR(psu3_mfr_model           , S_IRUGO           , psu_mfr_model_get         , NULL                      , PSU3_MFR_MODEL);
 static SENSOR_DEVICE_ATTR(psu4_mfr_model           , S_IRUGO           , psu_mfr_model_get         , NULL                      , PSU4_MFR_MODEL);
 static SENSOR_DEVICE_ATTR(psu1_mfr_iout_max        , S_IRUGO           , psu_iout_max_get          , NULL                      , PSU1_MFR_IOUT_MAX);
 static SENSOR_DEVICE_ATTR(psu2_mfr_iout_max        , S_IRUGO           , psu_iout_max_get          , NULL                      , PSU2_MFR_IOUT_MAX);
 static SENSOR_DEVICE_ATTR(psu3_mfr_iout_max        , S_IRUGO           , psu_iout_max_get          , NULL                      , PSU3_MFR_IOUT_MAX);
 static SENSOR_DEVICE_ATTR(psu4_mfr_iout_max        , S_IRUGO           , psu_iout_max_get          , NULL                      , PSU4_MFR_IOUT_MAX);
 static SENSOR_DEVICE_ATTR(psu1_vmode               , S_IRUGO           , psu_vmode_get             , NULL                      , PSU1_VMODE);
 static SENSOR_DEVICE_ATTR(psu2_vmode               , S_IRUGO           , psu_vmode_get             , NULL                      , PSU2_VMODE);
 static SENSOR_DEVICE_ATTR(psu3_vmode               , S_IRUGO           , psu_vmode_get             , NULL                      , PSU3_VMODE);
 static SENSOR_DEVICE_ATTR(psu4_vmode               , S_IRUGO           , psu_vmode_get             , NULL                      , PSU4_VMODE);
 static SENSOR_DEVICE_ATTR(dc_6e_p0_vout            , S_IRUGO           , dc_vout_get               , NULL                      , DC_6E_P0_VOUT);        
 static SENSOR_DEVICE_ATTR(dc_70_p0_vout            , S_IRUGO           , dc_vout_get               , NULL                      , DC_70_P0_VOUT); 
 static SENSOR_DEVICE_ATTR(dc_70_p1_vout            , S_IRUGO           , dc_vout_get               , NULL                      , DC_70_P1_VOUT); 
 static SENSOR_DEVICE_ATTR(dc_6e_p0_iout            , S_IRUGO           , dc_iout_get               , NULL                      , DC_6E_P0_IOUT);
 static SENSOR_DEVICE_ATTR(dc_70_p0_iout            , S_IRUGO           , dc_iout_get               , NULL                      , DC_70_P0_IOUT);
 static SENSOR_DEVICE_ATTR(dc_70_p1_iout            , S_IRUGO           , dc_iout_get               , NULL                      , DC_70_P1_IOUT);
 static SENSOR_DEVICE_ATTR(dc_6e_p0_pout            , S_IRUGO           , dc_pout_get               , NULL                      , DC_6E_P0_POUT);
 static SENSOR_DEVICE_ATTR(dc_70_p0_pout            , S_IRUGO           , dc_pout_get               , NULL                      , DC_70_P0_POUT);
 static SENSOR_DEVICE_ATTR(dc_70_p1_pout            , S_IRUGO           , dc_pout_get               , NULL                      , DC_70_P1_POUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_11_p0_vout     , S_IRUGO           , dc_11_p0_vout_get         , NULL                      , CARD_1_DC_11_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_11_p0_vout     , S_IRUGO           , dc_11_p0_vout_get         , NULL                      , CARD_2_DC_11_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_11_p0_vout     , S_IRUGO           , dc_11_p0_vout_get         , NULL                      , CARD_3_DC_11_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_11_p0_vout     , S_IRUGO           , dc_11_p0_vout_get         , NULL                      , CARD_4_DC_11_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_11_p0_vout     , S_IRUGO           , dc_11_p0_vout_get         , NULL                      , CARD_5_DC_11_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_11_p0_vout     , S_IRUGO           , dc_11_p0_vout_get         , NULL                      , CARD_6_DC_11_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_11_p0_vout     , S_IRUGO           , dc_11_p0_vout_get         , NULL                      , CARD_7_DC_11_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_11_p0_vout     , S_IRUGO           , dc_11_p0_vout_get         , NULL                      , CARD_8_DC_11_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_11_p1_vout     , S_IRUGO           , dc_11_p1_vout_get         , NULL                      , CARD_1_DC_11_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_11_p1_vout     , S_IRUGO           , dc_11_p1_vout_get         , NULL                      , CARD_2_DC_11_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_11_p1_vout     , S_IRUGO           , dc_11_p1_vout_get         , NULL                      , CARD_3_DC_11_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_11_p1_vout     , S_IRUGO           , dc_11_p1_vout_get         , NULL                      , CARD_4_DC_11_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_11_p1_vout     , S_IRUGO           , dc_11_p1_vout_get         , NULL                      , CARD_5_DC_11_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_11_p1_vout     , S_IRUGO           , dc_11_p1_vout_get         , NULL                      , CARD_6_DC_11_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_11_p1_vout     , S_IRUGO           , dc_11_p1_vout_get         , NULL                      , CARD_7_DC_11_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_11_p1_vout     , S_IRUGO           , dc_11_p1_vout_get         , NULL                      , CARD_8_DC_11_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_12_p0_vout     , S_IRUGO           , dc_12_p0_vout_get         , NULL                      , CARD_1_DC_12_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_12_p0_vout     , S_IRUGO           , dc_12_p0_vout_get         , NULL                      , CARD_2_DC_12_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_12_p0_vout     , S_IRUGO           , dc_12_p0_vout_get         , NULL                      , CARD_3_DC_12_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_12_p0_vout     , S_IRUGO           , dc_12_p0_vout_get         , NULL                      , CARD_4_DC_12_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_12_p0_vout     , S_IRUGO           , dc_12_p0_vout_get         , NULL                      , CARD_5_DC_12_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_12_p0_vout     , S_IRUGO           , dc_12_p0_vout_get         , NULL                      , CARD_6_DC_12_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_12_p0_vout     , S_IRUGO           , dc_12_p0_vout_get         , NULL                      , CARD_7_DC_12_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_12_p0_vout     , S_IRUGO           , dc_12_p0_vout_get         , NULL                      , CARD_8_DC_12_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_12_p1_vout     , S_IRUGO           , dc_12_p1_vout_get         , NULL                      , CARD_1_DC_12_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_12_p1_vout     , S_IRUGO           , dc_12_p1_vout_get         , NULL                      , CARD_2_DC_12_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_12_p1_vout     , S_IRUGO           , dc_12_p1_vout_get         , NULL                      , CARD_3_DC_12_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_12_p1_vout     , S_IRUGO           , dc_12_p1_vout_get         , NULL                      , CARD_4_DC_12_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_12_p1_vout     , S_IRUGO           , dc_12_p1_vout_get         , NULL                      , CARD_5_DC_12_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_12_p1_vout     , S_IRUGO           , dc_12_p1_vout_get         , NULL                      , CARD_6_DC_12_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_12_p1_vout     , S_IRUGO           , dc_12_p1_vout_get         , NULL                      , CARD_7_DC_12_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_12_p1_vout     , S_IRUGO           , dc_12_p1_vout_get         , NULL                      , CARD_8_DC_12_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_13_p0_vout     , S_IRUGO           , dc_13_p0_vout_get         , NULL                      , CARD_1_DC_13_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_13_p0_vout     , S_IRUGO           , dc_13_p0_vout_get         , NULL                      , CARD_2_DC_13_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_13_p0_vout     , S_IRUGO           , dc_13_p0_vout_get         , NULL                      , CARD_3_DC_13_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_13_p0_vout     , S_IRUGO           , dc_13_p0_vout_get         , NULL                      , CARD_4_DC_13_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_13_p0_vout     , S_IRUGO           , dc_13_p0_vout_get         , NULL                      , CARD_5_DC_13_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_13_p0_vout     , S_IRUGO           , dc_13_p0_vout_get         , NULL                      , CARD_6_DC_13_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_13_p0_vout     , S_IRUGO           , dc_13_p0_vout_get         , NULL                      , CARD_7_DC_13_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_13_p0_vout     , S_IRUGO           , dc_13_p0_vout_get         , NULL                      , CARD_8_DC_13_P0_VOUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_13_p1_vout     , S_IRUGO           , dc_13_p1_vout_get         , NULL                      , CARD_1_DC_13_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_13_p1_vout     , S_IRUGO           , dc_13_p1_vout_get         , NULL                      , CARD_2_DC_13_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_13_p1_vout     , S_IRUGO           , dc_13_p1_vout_get         , NULL                      , CARD_3_DC_13_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_13_p1_vout     , S_IRUGO           , dc_13_p1_vout_get         , NULL                      , CARD_4_DC_13_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_13_p1_vout     , S_IRUGO           , dc_13_p1_vout_get         , NULL                      , CARD_5_DC_13_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_13_p1_vout     , S_IRUGO           , dc_13_p1_vout_get         , NULL                      , CARD_6_DC_13_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_13_p1_vout     , S_IRUGO           , dc_13_p1_vout_get         , NULL                      , CARD_7_DC_13_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_13_p1_vout     , S_IRUGO           , dc_13_p1_vout_get         , NULL                      , CARD_8_DC_13_P1_VOUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_11_p0_iout     , S_IRUGO           , dc_11_p0_iout_get         , NULL                      , CARD_1_DC_11_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_11_p0_iout     , S_IRUGO           , dc_11_p0_iout_get         , NULL                      , CARD_2_DC_11_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_11_p0_iout     , S_IRUGO           , dc_11_p0_iout_get         , NULL                      , CARD_3_DC_11_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_11_p0_iout     , S_IRUGO           , dc_11_p0_iout_get         , NULL                      , CARD_4_DC_11_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_11_p0_iout     , S_IRUGO           , dc_11_p0_iout_get         , NULL                      , CARD_5_DC_11_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_11_p0_iout     , S_IRUGO           , dc_11_p0_iout_get         , NULL                      , CARD_6_DC_11_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_11_p0_iout     , S_IRUGO           , dc_11_p0_iout_get         , NULL                      , CARD_7_DC_11_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_11_p0_iout     , S_IRUGO           , dc_11_p0_iout_get         , NULL                      , CARD_8_DC_11_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_11_p1_iout     , S_IRUGO           , dc_11_p1_iout_get         , NULL                      , CARD_1_DC_11_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_11_p1_iout     , S_IRUGO           , dc_11_p1_iout_get         , NULL                      , CARD_2_DC_11_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_11_p1_iout     , S_IRUGO           , dc_11_p1_iout_get         , NULL                      , CARD_3_DC_11_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_11_p1_iout     , S_IRUGO           , dc_11_p1_iout_get         , NULL                      , CARD_4_DC_11_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_11_p1_iout     , S_IRUGO           , dc_11_p1_iout_get         , NULL                      , CARD_5_DC_11_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_11_p1_iout     , S_IRUGO           , dc_11_p1_iout_get         , NULL                      , CARD_6_DC_11_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_11_p1_iout     , S_IRUGO           , dc_11_p1_iout_get         , NULL                      , CARD_7_DC_11_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_11_p1_iout     , S_IRUGO           , dc_11_p1_iout_get         , NULL                      , CARD_8_DC_11_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_12_p0_iout     , S_IRUGO           , dc_12_p0_iout_get         , NULL                      , CARD_1_DC_12_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_12_p0_iout     , S_IRUGO           , dc_12_p0_iout_get         , NULL                      , CARD_2_DC_12_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_12_p0_iout     , S_IRUGO           , dc_12_p0_iout_get         , NULL                      , CARD_3_DC_12_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_12_p0_iout     , S_IRUGO           , dc_12_p0_iout_get         , NULL                      , CARD_4_DC_12_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_12_p0_iout     , S_IRUGO           , dc_12_p0_iout_get         , NULL                      , CARD_5_DC_12_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_12_p0_iout     , S_IRUGO           , dc_12_p0_iout_get         , NULL                      , CARD_6_DC_12_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_12_p0_iout     , S_IRUGO           , dc_12_p0_iout_get         , NULL                      , CARD_7_DC_12_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_12_p0_iout     , S_IRUGO           , dc_12_p0_iout_get         , NULL                      , CARD_8_DC_12_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_12_p1_iout     , S_IRUGO           , dc_12_p1_iout_get         , NULL                      , CARD_1_DC_12_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_12_p1_iout     , S_IRUGO           , dc_12_p1_iout_get         , NULL                      , CARD_2_DC_12_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_12_p1_iout     , S_IRUGO           , dc_12_p1_iout_get         , NULL                      , CARD_3_DC_12_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_12_p1_iout     , S_IRUGO           , dc_12_p1_iout_get         , NULL                      , CARD_4_DC_12_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_12_p1_iout     , S_IRUGO           , dc_12_p1_iout_get         , NULL                      , CARD_5_DC_12_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_12_p1_iout     , S_IRUGO           , dc_12_p1_iout_get         , NULL                      , CARD_6_DC_12_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_12_p1_iout     , S_IRUGO           , dc_12_p1_iout_get         , NULL                      , CARD_7_DC_12_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_12_p1_iout     , S_IRUGO           , dc_12_p1_iout_get         , NULL                      , CARD_8_DC_12_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_13_p0_iout     , S_IRUGO           , dc_13_p0_iout_get         , NULL                      , CARD_1_DC_13_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_13_p0_iout     , S_IRUGO           , dc_13_p0_iout_get         , NULL                      , CARD_2_DC_13_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_13_p0_iout     , S_IRUGO           , dc_13_p0_iout_get         , NULL                      , CARD_3_DC_13_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_13_p0_iout     , S_IRUGO           , dc_13_p0_iout_get         , NULL                      , CARD_4_DC_13_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_13_p0_iout     , S_IRUGO           , dc_13_p0_iout_get         , NULL                      , CARD_5_DC_13_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_13_p0_iout     , S_IRUGO           , dc_13_p0_iout_get         , NULL                      , CARD_6_DC_13_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_13_p0_iout     , S_IRUGO           , dc_13_p0_iout_get         , NULL                      , CARD_7_DC_13_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_13_p0_iout     , S_IRUGO           , dc_13_p0_iout_get         , NULL                      , CARD_8_DC_13_P0_IOUT);
 static SENSOR_DEVICE_ATTR(card_1_dc_13_p1_iout     , S_IRUGO           , dc_13_p1_iout_get         , NULL                      , CARD_1_DC_13_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_2_dc_13_p1_iout     , S_IRUGO           , dc_13_p1_iout_get         , NULL                      , CARD_2_DC_13_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_3_dc_13_p1_iout     , S_IRUGO           , dc_13_p1_iout_get         , NULL                      , CARD_3_DC_13_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_4_dc_13_p1_iout     , S_IRUGO           , dc_13_p1_iout_get         , NULL                      , CARD_4_DC_13_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_5_dc_13_p1_iout     , S_IRUGO           , dc_13_p1_iout_get         , NULL                      , CARD_5_DC_13_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_6_dc_13_p1_iout     , S_IRUGO           , dc_13_p1_iout_get         , NULL                      , CARD_6_DC_13_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_7_dc_13_p1_iout     , S_IRUGO           , dc_13_p1_iout_get         , NULL                      , CARD_7_DC_13_P1_IOUT);
 static SENSOR_DEVICE_ATTR(card_8_dc_13_p1_iout     , S_IRUGO           , dc_13_p1_iout_get         , NULL                      , CARD_8_DC_13_P1_IOUT);
/* end of sysfs attributes for SENSOR_DEVICE_ATTR */

/* sysfs attributes for hwmon */
/* i2c-0 */
static struct attribute *ESC600_SYS_attributes[] =
{
    &sensor_dev_attr_cpld_30_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_31_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_33_ver.dev_attr.attr,
    &sensor_dev_attr_wdt_en.dev_attr.attr,
    &sensor_dev_attr_eeprom_wp.dev_attr.attr,
    &sensor_dev_attr_usb_en.dev_attr.attr,
    &sensor_dev_attr_shutdown_set.dev_attr.attr,
    &sensor_dev_attr_reset.dev_attr.attr,
    &sensor_dev_attr_bmc_present.dev_attr.attr,
    &sensor_dev_attr_sw_alert_th0.dev_attr.attr,
    &sensor_dev_attr_sw_alert_th1.dev_attr.attr,
    &sensor_dev_attr_sw_alert_th2.dev_attr.attr,
    &sensor_dev_attr_sw_alert_th3.dev_attr.attr,
    &sensor_dev_attr_sw_alert_th0_mask.dev_attr.attr,
    &sensor_dev_attr_sw_alert_th1_mask.dev_attr.attr,
    &sensor_dev_attr_sw_alert_th2_mask.dev_attr.attr,
    &sensor_dev_attr_sw_alert_th3_mask.dev_attr.attr,
    &sensor_dev_attr_cb_int.dev_attr.attr,
    &sensor_dev_attr_sb_int.dev_attr.attr,
    &sensor_dev_attr_cb_int_mask.dev_attr.attr,
    &sensor_dev_attr_sb_int_mask.dev_attr.attr,
    &sensor_dev_attr_module_reset.dev_attr.attr,
    &sensor_dev_attr_module_1_present.dev_attr.attr,  
    &sensor_dev_attr_module_2_present.dev_attr.attr,  
    &sensor_dev_attr_module_3_present.dev_attr.attr,  
    &sensor_dev_attr_module_4_present.dev_attr.attr,  
    &sensor_dev_attr_module_5_present.dev_attr.attr,  
    &sensor_dev_attr_module_6_present.dev_attr.attr,  
    &sensor_dev_attr_module_7_present.dev_attr.attr,  
    &sensor_dev_attr_module_8_present.dev_attr.attr,  
    &sensor_dev_attr_module_1_power.dev_attr.attr,    
    &sensor_dev_attr_module_2_power.dev_attr.attr,    
    &sensor_dev_attr_module_3_power.dev_attr.attr,    
    &sensor_dev_attr_module_4_power.dev_attr.attr,    
    &sensor_dev_attr_module_5_power.dev_attr.attr,    
    &sensor_dev_attr_module_6_power.dev_attr.attr,    
    &sensor_dev_attr_module_7_power.dev_attr.attr,    
    &sensor_dev_attr_module_8_power.dev_attr.attr,    
    &sensor_dev_attr_module_1_enable.dev_attr.attr,   
    &sensor_dev_attr_module_2_enable.dev_attr.attr,   
    &sensor_dev_attr_module_3_enable.dev_attr.attr,   
    &sensor_dev_attr_module_4_enable.dev_attr.attr,   
    &sensor_dev_attr_module_5_enable.dev_attr.attr,   
    &sensor_dev_attr_module_6_enable.dev_attr.attr,   
    &sensor_dev_attr_module_7_enable.dev_attr.attr,   
    &sensor_dev_attr_module_8_enable.dev_attr.attr,   
    &sensor_dev_attr_module_ins_int.dev_attr.attr,
    &sensor_dev_attr_module_int.dev_attr.attr,
    &sensor_dev_attr_module_power_int.dev_attr.attr,
    &sensor_dev_attr_ther_sensor_int.dev_attr.attr,
    &sensor_dev_attr_io_board_int.dev_attr.attr,
    &sensor_dev_attr_fan_error_int.dev_attr.attr,
    &sensor_dev_attr_phy_power_int.dev_attr.attr,
    &sensor_dev_attr_sw_power_int.dev_attr.attr,
    &sensor_dev_attr_module_ins_int_mask.dev_attr.attr,
    &sensor_dev_attr_module_int_mask.dev_attr.attr,
    &sensor_dev_attr_module_pow_int_mask.dev_attr.attr,
    &sensor_dev_attr_ther_sen_int_mask.dev_attr.attr,
    &sensor_dev_attr_io_board_int_mask.dev_attr.attr,
    &sensor_dev_attr_fan_error_int_mask.dev_attr.attr,
    &sensor_dev_attr_phy_power_int_mask.dev_attr.attr,
    &sensor_dev_attr_sw_power_int_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_select.dev_attr.attr,
    &sensor_dev_attr_sfp_port_tx_1.dev_attr.attr,
    &sensor_dev_attr_sfp_port_tx_2.dev_attr.attr,
    &sensor_dev_attr_sfp_port_tx_mgm.dev_attr.attr,
    &sensor_dev_attr_sfp_port_1.dev_attr.attr,
    &sensor_dev_attr_sfp_port_2.dev_attr.attr,
    &sensor_dev_attr_sfp_port_mgm.dev_attr.attr,
    &sensor_dev_attr_sfp_port_rx_1.dev_attr.attr,
    &sensor_dev_attr_sfp_port_rx_2.dev_attr.attr,
    &sensor_dev_attr_sfp_port_rx_mgm.dev_attr.attr,
    &sensor_dev_attr_sfp_loss_int.dev_attr.attr,
    &sensor_dev_attr_sfp_abs_int.dev_attr.attr,
    &sensor_dev_attr_sfp_loss_mask.dev_attr.attr,
    &sensor_dev_attr_sfp_abs_mask.dev_attr.attr,
    &sensor_dev_attr_alert_th0_int.dev_attr.attr,       
    &sensor_dev_attr_alert_th1_int.dev_attr.attr,       
    &sensor_dev_attr_alert_th2_int.dev_attr.attr,       
    &sensor_dev_attr_alert_th3_int.dev_attr.attr,       
    &sensor_dev_attr_alert_th4_int.dev_attr.attr,       
    &sensor_dev_attr_alert_th5_int.dev_attr.attr,       
    &sensor_dev_attr_alert_th0_int_mask.dev_attr.attr,
    &sensor_dev_attr_alert_th1_int_mask.dev_attr.attr,
    &sensor_dev_attr_alert_th2_int_mask.dev_attr.attr,
    &sensor_dev_attr_alert_th3_int_mask.dev_attr.attr,
    &sensor_dev_attr_alert_th4_int_mask.dev_attr.attr,
    &sensor_dev_attr_alert_th5_int_mask.dev_attr.attr,
    NULL
};
static struct attribute *ESC600_LED_attributes[] =
{
    &sensor_dev_attr_led_sys.dev_attr.attr,
    &sensor_dev_attr_led_loc.dev_attr.attr,
    &sensor_dev_attr_led_flow.dev_attr.attr,        
    &sensor_dev_attr_led_4_1.dev_attr.attr,
    &sensor_dev_attr_led_4_2.dev_attr.attr,
    &sensor_dev_attr_led_4_3.dev_attr.attr,
    &sensor_dev_attr_led_4_4.dev_attr.attr,
    &sensor_dev_attr_led_5_1.dev_attr.attr,
    &sensor_dev_attr_led_5_2.dev_attr.attr,
    &sensor_dev_attr_led_5_3.dev_attr.attr,
    &sensor_dev_attr_led_5_4.dev_attr.attr,
    &sensor_dev_attr_led_fiber.dev_attr.attr,
    NULL
};
static struct attribute *ESC600_THERMAL_attributes[] =
{
    &sensor_dev_attr_line_card_1_up_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_2_up_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_3_up_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_4_up_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_5_up_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_6_up_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_7_up_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_8_up_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_1_dn_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_2_dn_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_3_dn_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_4_dn_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_5_dn_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_6_dn_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_7_dn_temp.dev_attr.attr,
    &sensor_dev_attr_line_card_8_dn_temp.dev_attr.attr,
    &sensor_dev_attr_nct7511_temp.dev_attr.attr,
    &sensor_dev_attr_left_bot_sb_temp.dev_attr.attr,
    &sensor_dev_attr_ctr_top_sb_temp.dev_attr.attr,
    &sensor_dev_attr_ctr_sb_temp.dev_attr.attr,
    &sensor_dev_attr_left_top_cb_temp.dev_attr.attr,
    &sensor_dev_attr_ctr_cb_temp.dev_attr.attr,
    &sensor_dev_attr_right_bot_cb_temp.dev_attr.attr,
    &sensor_dev_attr_left_bot_cb_temp.dev_attr.attr,
    &sensor_dev_attr_io_board_temp.dev_attr.attr,
    NULL
};
static struct attribute *ESC600_FAN_attributes[] =
{
    &sensor_dev_attr_fanctrl_mode.dev_attr.attr,
    &sensor_dev_attr_fanctrl_rpm.dev_attr.attr,
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
    &sensor_dev_attr_fan1_direct.dev_attr.attr,       
    &sensor_dev_attr_fan2_direct.dev_attr.attr,       
    &sensor_dev_attr_fan3_direct.dev_attr.attr,       
    &sensor_dev_attr_fan4_direct.dev_attr.attr,       
    &sensor_dev_attr_fan1_rpm.dev_attr.attr,         
    &sensor_dev_attr_fan2_rpm.dev_attr.attr,         
    &sensor_dev_attr_fan3_rpm.dev_attr.attr,         
    &sensor_dev_attr_fan4_rpm.dev_attr.attr,   
    NULL
};
static struct attribute *ESC600_POWER_attributes[] =
{
    &sensor_dev_attr_psu1_good.dev_attr.attr,          
    &sensor_dev_attr_psu2_good.dev_attr.attr,          
    &sensor_dev_attr_psu3_good.dev_attr.attr,          
    &sensor_dev_attr_psu4_good.dev_attr.attr,          
    &sensor_dev_attr_psu1_prnt.dev_attr.attr,       
    &sensor_dev_attr_psu2_prnt.dev_attr.attr,       
    &sensor_dev_attr_psu3_prnt.dev_attr.attr,       
    &sensor_dev_attr_psu4_prnt.dev_attr.attr,       
    &sensor_dev_attr_psu1_vin.dev_attr.attr,           
    &sensor_dev_attr_psu2_vin.dev_attr.attr,           
    &sensor_dev_attr_psu3_vin.dev_attr.attr,           
    &sensor_dev_attr_psu4_vin.dev_attr.attr,           
    &sensor_dev_attr_psu1_iin.dev_attr.attr,           
    &sensor_dev_attr_psu2_iin.dev_attr.attr,           
    &sensor_dev_attr_psu3_iin.dev_attr.attr,           
    &sensor_dev_attr_psu4_iin.dev_attr.attr,           
    &sensor_dev_attr_psu1_vout.dev_attr.attr,          
    &sensor_dev_attr_psu2_vout.dev_attr.attr,          
    &sensor_dev_attr_psu3_vout.dev_attr.attr,          
    &sensor_dev_attr_psu4_vout.dev_attr.attr,          
    &sensor_dev_attr_psu1_iout.dev_attr.attr,          
    &sensor_dev_attr_psu2_iout.dev_attr.attr,          
    &sensor_dev_attr_psu3_iout.dev_attr.attr,          
    &sensor_dev_attr_psu4_iout.dev_attr.attr,          
    &sensor_dev_attr_psu1_temp.dev_attr.attr,          
    &sensor_dev_attr_psu2_temp.dev_attr.attr,          
    &sensor_dev_attr_psu3_temp.dev_attr.attr,          
    &sensor_dev_attr_psu4_temp.dev_attr.attr,          
    &sensor_dev_attr_psu1_fan_speed.dev_attr.attr,          
    &sensor_dev_attr_psu2_fan_speed.dev_attr.attr,          
    &sensor_dev_attr_psu3_fan_speed.dev_attr.attr,          
    &sensor_dev_attr_psu4_fan_speed.dev_attr.attr,          
    &sensor_dev_attr_psu1_pout.dev_attr.attr,          
    &sensor_dev_attr_psu2_pout.dev_attr.attr,          
    &sensor_dev_attr_psu3_pout.dev_attr.attr,          
    &sensor_dev_attr_psu4_pout.dev_attr.attr,          
    &sensor_dev_attr_psu1_pin.dev_attr.attr,           
    &sensor_dev_attr_psu2_pin.dev_attr.attr,           
    &sensor_dev_attr_psu3_pin.dev_attr.attr,           
    &sensor_dev_attr_psu4_pin.dev_attr.attr,           
    &sensor_dev_attr_psu1_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu2_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu3_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu4_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu1_mfr_iout_max.dev_attr.attr,
    &sensor_dev_attr_psu2_mfr_iout_max.dev_attr.attr,
    &sensor_dev_attr_psu3_mfr_iout_max.dev_attr.attr,
    &sensor_dev_attr_psu4_mfr_iout_max.dev_attr.attr,
    &sensor_dev_attr_psu1_vmode.dev_attr.attr,       
    &sensor_dev_attr_psu2_vmode.dev_attr.attr,       
    &sensor_dev_attr_psu3_vmode.dev_attr.attr,       
    &sensor_dev_attr_psu4_vmode.dev_attr.attr,       
    &sensor_dev_attr_dc_6e_p0_vout.dev_attr.attr,      
    &sensor_dev_attr_dc_70_p0_vout.dev_attr.attr,      
    &sensor_dev_attr_dc_70_p1_vout.dev_attr.attr,      
    &sensor_dev_attr_dc_6e_p0_iout.dev_attr.attr,      
    &sensor_dev_attr_dc_70_p0_iout.dev_attr.attr,      
    &sensor_dev_attr_dc_70_p1_iout.dev_attr.attr,      
    &sensor_dev_attr_dc_6e_p0_pout.dev_attr.attr,      
    &sensor_dev_attr_dc_70_p0_pout.dev_attr.attr,      
    &sensor_dev_attr_dc_70_p1_pout.dev_attr.attr,      
    &sensor_dev_attr_card_1_dc_11_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_11_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_11_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_11_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_11_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_11_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_11_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_11_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_1_dc_11_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_11_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_11_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_11_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_11_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_11_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_11_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_11_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_1_dc_12_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_12_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_12_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_12_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_12_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_12_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_12_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_12_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_1_dc_12_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_12_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_12_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_12_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_12_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_12_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_12_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_12_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_1_dc_13_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_13_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_13_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_13_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_13_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_13_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_13_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_13_p0_vout.dev_attr.attr,
    &sensor_dev_attr_card_1_dc_13_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_13_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_13_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_13_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_13_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_13_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_13_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_13_p1_vout.dev_attr.attr,
    &sensor_dev_attr_card_1_dc_11_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_11_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_11_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_11_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_11_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_11_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_11_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_11_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_1_dc_11_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_11_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_11_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_11_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_11_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_11_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_11_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_11_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_1_dc_12_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_12_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_12_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_12_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_12_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_12_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_12_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_12_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_1_dc_12_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_12_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_12_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_12_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_12_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_12_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_12_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_12_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_1_dc_13_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_13_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_13_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_13_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_13_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_13_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_13_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_13_p0_iout.dev_attr.attr,
    &sensor_dev_attr_card_1_dc_13_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_2_dc_13_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_3_dc_13_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_4_dc_13_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_5_dc_13_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_6_dc_13_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_7_dc_13_p1_iout.dev_attr.attr,
    &sensor_dev_attr_card_8_dc_13_p1_iout.dev_attr.attr,
    NULL
};
/* end of sysfs attributes for hwmon */

/* struct attribute_group */
static const struct attribute_group ESC600_SYS_group =
{
    .name  = "ESC600_SYS",
    .attrs = ESC600_SYS_attributes,
};

static const struct attribute_group ESC600_LED_group =
{
    .name  = "ESC600_LED",
    .attrs = ESC600_LED_attributes,
};

static const struct attribute_group ESC600_THERMAL_group =
{
    .name  = "ESC600_THERMAL",
    .attrs = ESC600_THERMAL_attributes,
};

static const struct attribute_group ESC600_FAN_group =
{
    .name  = "ESC600_FAN",
    .attrs = ESC600_FAN_attributes,
};

static const struct attribute_group ESC600_POWER_group =
{
    .name  = "ESC600_POWER",
    .attrs = ESC600_POWER_attributes,
};
/* end of struct attribute_group */
