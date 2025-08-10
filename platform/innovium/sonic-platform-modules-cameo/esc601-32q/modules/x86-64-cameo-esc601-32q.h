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

#define DRIVER_VERSION  "3.1"

#define TURN_OFF        0
#define TURN_ON         1
#define GET_USB         2
#define GET_LOC         3
#define LOC_OFF         0
#define LOC_BLINK       1
#define ALARM_OFF       0
#define ALARM_AMBER     1
#define ALARM_GREEN     2
#define PSU_1_GOOD      3
#define PSU_2_GOOD      4
#define PCIE_INT        1
#define QSFP_1_INT      2
#define QSFP_2_INT      3
#define FAN_INT         4
#define PSU_INT         5
#define SENSOR_INT      6
#define USB_INT         7
#define USB_ON          0x2
#define USB_OFF         0xfd
#define DIAG_G_ON       0x2
#define DIAG_G_OFF      0xfd
#define LED_ON          0x1
#define LED_OFF         0xfe
#define DIAG_A_ON       0x1
#define DIAG_A_OFF      0xfe
#define LOC_LED_OFF     0x4
#define LOC_LED_BLINK   0xfb

#define SYSFAN_MAX_NUM  5

struct i2c_adap {
	int nr;
	char *name;
	const char *funcs;
	const char *algo;
};

struct i2c_adap *gather_i2c_busses(void);
void free_adapters(struct i2c_adap *adapters);

/* compiler conditional */
#define LED_CTRL_WANTED
#define USB_CTRL_WANTED
#define ASPEED_BMC_WANTED
#define PSU_STAT_WANTED
//#define WDT_CTRL_WANTED
//#define EEPROM_WP_WANTED
//#define I2C_SWITCH_WANTED
//#define EEPROM_WANTED
//#define THEMAL_WANTED
//#define QSFP_WANTED
//#define FAN_CTRL_WANTED
//#define FAN_DUTY_CTRL_WANTED

//#define DEBUG_MSG
#ifdef DEBUG_MSG
    #define debug_print(s) printk s
#else
    #define debug_print(s)
#endif

/* end of compiler conditional */

/* i2c_client Declaration */
static struct i2c_client *ESC_601_i2c_client; //0x30 for other device
static struct i2c_client *Cameo_CPLD_2_client; //0x31 for Port 01-16
static struct i2c_client *Cameo_CPLD_3_client; //0x32 for Port 17-32
static struct i2c_client *Cameo_CPLD_4_client; //0x23 for Fan Status
#ifdef I2C_SWITCH_WANTED
static struct i2c_client *Cameo_Switch_1_client; //0x73
static struct i2c_client *Cameo_Switch_2_client; //0x77
#endif
#ifdef QSFP_WANTED
static struct i2c_client *Cameo_QSFP_Switch_client; //0x74
static struct i2c_client *Cameo_QSFP_client; //0x50
#endif
#ifdef EEPROM_WANTED
static struct i2c_client *Cameo_EEPROM_client; //0x56
#endif
#ifdef THEMAL_WANTED
static struct i2c_client *Cameo_Sensor_client; //0x4c themal sensor
static struct i2c_client *Cameo_MAC_Sensor_client; //0x68 MCP3425
static struct i2c_client *Cameo_Sensor_fan_client; //0x2e themal sensor
#endif
#ifdef ASPEED_BMC_WANTED
static struct i2c_client *Cameo_BMC_client; //0x14 ASPEED BMC
#endif
/* end of i2c_client Declaration */

/* Function Declaration */
/* i2c-0 */
#ifdef EEPROM_WANTED
static ssize_t tlv_status_get(struct device *dev, struct device_attribute *da, char *buf);
#endif
static ssize_t psu_status_get(struct device *dev, struct device_attribute *da, char *buf);
#ifdef PSU_STAT_WANTED
static ssize_t psu_module_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t dc_chip_switch_get(struct device *dev, struct device_attribute *da, char *buf);
#endif
#ifdef USB_CTRL_WANTED
static ssize_t usb_power_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t usb_power_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
#endif
#ifdef LED_CTRL_WANTED
static ssize_t led_ctrl_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t led_ctrl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
#endif
static ssize_t led_loc_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t led_loc_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t led_alarm_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t led_alarm_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t reset_mac_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t themal_status_get(struct device *dev, struct device_attribute *da, char *buf);
#ifdef THEMAL_WANTED
static ssize_t themal_temp_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t mac_temp_get(struct device *dev, struct device_attribute *da, char *buf);
#endif
static ssize_t themal_mask_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t themal_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t int_status_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t low_power_all_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t low_power_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t low_power_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t low_power_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t qsfp_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t qsfp_status_get(struct device *dev, struct device_attribute *da, char *buf);
#ifdef QSFP_WANTED
static ssize_t qsfp_temp_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_date_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_sn_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_pn_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_name_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_oui_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_rev_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_connector_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_encoding_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_nominal_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_ext_rate_com_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_eth_com_code_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_identifier_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_fc_media_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_fc_speed_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_cab_tech_get(struct device *dev, struct device_attribute *da, char *buf);
#endif
static ssize_t fan_status_get(struct device *dev, struct device_attribute *da, char *buf);
#ifdef FAN_CTRL_WANTED
static ssize_t fan_mode_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t fan_mode_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t fan_rpm_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t fan_rpm_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
#endif
#ifdef FAN_DUTY_CTRL_WANTED
static ssize_t fan_duty_control(void);
#endif /*FAN_DUTY_CTRL_WANTED*/
#ifdef ASPEED_BMC_WANTED
static ssize_t bmc_register_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t bmc_module_detect(struct device *dev, struct device_attribute *da, char *buf);
#endif
#ifdef WDT_CTRL_WANTED
static ssize_t wdt_status_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t wdt_status_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
#endif
#ifdef EEPROM_WP_WANTED
static ssize_t eeprom_wp_status_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t eeprom_wp_status_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
#endif
static ssize_t hw_version_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t cpld_version_get(struct device *dev, struct device_attribute *da, char *buf);
/* end of Function Declaration */

/* struct i2c_data */
struct Cameo_i2c_data
{
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;          /* !=0 if registers are valid */
    unsigned long       last_updated;   /* In jiffies */
    u8  status;                         /* Status register read from CPLD */
};

#ifdef EEPROM_WANTED
struct qsfp_table {
	int v;
	const char *n;
};

const char * find_value(struct qsfp_table *x, int value);
const char * find_zero_bit(struct qsfp_table *x, int value, int sz);

/**
 *  The TLV Types.
 *
 *  Keep these in sync with tlv_code_list in cmd_sys_eeprom.c
 */
#define TLV_CODE_PRODUCT_NAME   0x21
#define TLV_CODE_PART_NUMBER    0x22
#define TLV_CODE_SERIAL_NUMBER  0x23
#define TLV_CODE_MAC_BASE       0x24
#define TLV_CODE_MANUF_DATE     0x25
#define TLV_CODE_DEVICE_VERSION 0x26
#define TLV_CODE_LABEL_REVISION 0x27
#define TLV_CODE_PLATFORM_NAME  0x28
#define TLV_CODE_ONIE_VERSION   0x29
#define TLV_CODE_MAC_SIZE       0x2A
#define TLV_CODE_MANUF_NAME     0x2B
#define TLV_CODE_MANUF_COUNTRY  0x2C
#define TLV_CODE_VENDOR_NAME    0x2D
#define TLV_CODE_DIAG_VERSION   0x2E
#define TLV_CODE_SERVICE_TAG    0x2F
#define TLV_CODE_VENDOR_EXT     0xFD
#define TLV_CODE_CRC_32         0xFE

/*
 *  Struct for displaying the TLV codes and names.
 */
struct tlv_code_desc {
    u_int8_t m_code;
    char* m_name;
};

/*
 *  List of TLV codes and names.
 */
static const struct tlv_code_desc tlv_code_list[] = {
    { TLV_CODE_PRODUCT_NAME	 , "Product Name"},
    { TLV_CODE_PART_NUMBER	 , "Part Number"},
    { TLV_CODE_SERIAL_NUMBER     , "Serial Number"},
    { TLV_CODE_MAC_BASE	         , "Base MAC Address"},
    { TLV_CODE_MANUF_DATE	 , "Manufacture Date"},
    { TLV_CODE_DEVICE_VERSION    , "Device Version"},
    { TLV_CODE_LABEL_REVISION    , "Label Revision"},
    { TLV_CODE_PLATFORM_NAME     , "Platform Name"},
    { TLV_CODE_ONIE_VERSION	 , "ONIE Version"},
    { TLV_CODE_MAC_SIZE	         , "MAC Addresses"},
    { TLV_CODE_MANUF_NAME	 , "Manufacturer"},
    { TLV_CODE_MANUF_COUNTRY     , "Country Code"},
    { TLV_CODE_VENDOR_NAME	 , "Vendor Name"},
    { TLV_CODE_DIAG_VERSION	 , "Diag Version"},
    { TLV_CODE_SERVICE_TAG       , "Service Tag"},
    { TLV_CODE_VENDOR_EXT	 , "Vendor Extension"},
    { TLV_CODE_CRC_32	         , "CRC-32"},
};

struct __attribute__ ((__packed__)) tlvinfo_header_s {
    char    signature[8];   /* 0x00 - 0x07 EEPROM Tag "TlvInfo" */
    u_int8_t      version;  /* 0x08        Structure version */
    u_int16_t     totallen; /* 0x09 - 0x0A Length of all data which follows */
};
typedef struct tlvinfo_header_s tlvinfo_header_t;

// Header Field Constants
#define TLV_INFO_ID_STRING      "TlvInfo"
#define TLV_INFO_VERSION        0x01

struct __attribute__ ((__packed__)) tlvinfo_tlv_s {
    u_int8_t  type;
    u_int8_t  length;
    u_int8_t  value[0];
};
typedef struct tlvinfo_tlv_s tlvinfo_tlv_t;

/* Maximum length of a TLV value in bytes */
#define TLV_VALUE_MAX_LEN        255
#define TLV_DECODE_VALUE_MAX_LEN    ((5 * TLV_VALUE_MAX_LEN) + 1)

static void decode_tlv_value(tlvinfo_tlv_t * tlv, char* value);
static inline const char* tlv_type2name(u_int8_t type);
static void decode_tlv(tlvinfo_tlv_t * tlv, char *buf);
void show_eeprom(u_int8_t *eeprom, char *buf,int tlv_len);
#endif

#ifdef QSFP_WANTED
/* SFF-8472 Rev. 11.4 table 3.4: Connector values */
static struct qsfp_table conn[] = {
	{ 0x00, "Unknown" },
	{ 0x01, "SC" },
	{ 0x02, "Fibre Channel Style 1 copper" },
	{ 0x03, "Fibre Channel Style 2 copper" },
	{ 0x04, "BNC/TNC" },
	{ 0x05, "Fibre Channel coaxial" },
	{ 0x06, "FiberJack" },
	{ 0x07, "LC" },
	{ 0x08, "MT-RJ" },
	{ 0x09, "MU" },
	{ 0x0A, "SG" },
	{ 0x0B, "Optical pigtail" },
	{ 0x0C, "MPO Parallel Optic" },
	{ 0x20, "HSSDC II" },
	{ 0x21, "Copper pigtail" },
	{ 0x22, "RJ45" },
	{ 0x23, "No separate connector" }, /* SFF-8436 */
	{ 0, NULL }
};

/* Channel/Cable technology, byte 7-8 */
static struct qsfp_table cab_tech[] = {
	{ 0x0400, "Shortwave laser (SA)" },
	{ 0x0200, "Longwave laser (LC)" },
	{ 0x0100, "Electrical inter-enclosure (EL)" },
	{ 0x80, "Electrical intra-enclosure (EL)" },
	{ 0x40, "Shortwave laser (SN)" },
	{ 0x20, "Shortwave laser (SL)" },
	{ 0x10, "Longwave laser (LL)" },
	{ 0x08, "Active Cable" },
	{ 0x04, "Passive Cable" },
	{ 0, NULL }
};

/* FC Transmission media, byte 9 */
static struct qsfp_table fc_media[] = {
	{ 0x80, "Twin Axial Pair" },
	{ 0x40, "Twisted Pair" },
	{ 0x20, "Miniature Coax" },
	{ 0x10, "Viao Coax" },
	{ 0x08, "Miltimode, 62.5um" },
	{ 0x04, "Multimode, 50um" },
	{ 0x02, "" },
	{ 0x01, "Single Mode" },
	{ 0, NULL }
};

/* FC Speed, byte 10 */
static struct qsfp_table fc_speed[] = {
	{ 0x80, "1200 MBytes/sec" },
	{ 0x40, "800 MBytes/sec" },
	{ 0x20, "1600 MBytes/sec" },
	{ 0x10, "400 MBytes/sec" },
	{ 0x08, "3200 MBytes/sec" },
	{ 0x04, "200 MBytes/sec" },
	{ 0x01, "100 MBytes/sec" },
	{ 0, NULL }
};

/* SFF-8436 Rev. 4.8 table 33: Specification compliance  */

/* 10/40G Ethernet compliance codes, byte 128 + 3 */
static struct qsfp_table eth_1040g[] = {
	{ 0x80, "Extended" },
	{ 0x40, "10GBASE-LRM" },
	{ 0x20, "10GBASE-LR" },
	{ 0x10, "10GBASE-SR" },
	{ 0x08, "40GBASE-CR4" },
	{ 0x04, "40GBASE-SR4" },
	{ 0x02, "40GBASE-LR4" },
	{ 0x01, "40G Active Cable" },
	{ 0, NULL }
};
#define	SFF_8636_EXT_COMPLIANCE	0x80

/* SFF-8636 Encoding */
static struct qsfp_table encoding[] = {
	{ 0x1, "8B10B" },
	{ 0x2, "4B5B" },
	{ 0x3, "NRZ" },
	{ 0x4, "SONET Scrambled" },
	{ 0x5, "64B66B" },
	{ 0x6, "Manchester" },
	{ 0x0, "Unspecified" }
};
#endif

/* struct i2c_sysfs_attributes */
enum Cameo_i2c_sysfs_attributes
{
#ifdef EEPROM_WANTED
    TLV_STATUS,
#endif
    /* i2c-0 */
    /*CPLD 0X30*/
    PSU_PRESENT,
    PSU_STATUS,
#ifdef PSU_STAT_WANTED
    PSU_MODULE_1,
    PSU_MODULE_2,
    DC_CHIP_SWITCH,
#endif
#ifdef USB_CTRL_WANTED
    USB_POWER,
#endif
#ifdef LED_CTRL_WANTED
    LED_CTRL,
#endif
    LED_LOC,
    LED_ALARM,
    RESET_MAC,
    SENSOR_STATUS,
#ifdef THEMAL_WANTED
    SENSOR_TEMP,
    MAC_TEMP,
#endif
    SENSOR_INT_MASK,
    INT_STATUS,
    /*CPLD 0X31 & 0X32*/
    QSFP_LOW_POWER_ALL,
    QSFP_RESET,
    QSFP_PRESENT,
    QSFP_INT,
#ifdef QSFP_WANTED
    QSFP_TEMP,
    QSFP_DATE,
    QSFP_SN,
    QSFP_PN,
    QSFP_NAME,
    QSFP_OUI,
    QSFP_REV,
    QSFP_CONNECTOR,
    QSFP_ENCODING,
    QSFP_NOMINAL,
    QSFP_EXT_RATE_COM,
    QSFP_ETH_COM_CODE,
    QSFP_IDENTIFIER,
    QSFP_FCMEDIA,
    QSFP_FCSPEED,
    QSFP_FCTECH,
#endif
    FAN_STATUS,
    FAN_PRESENT,
    FAN_POWER,
    FAN_SPEED_RPM,
#ifdef FAN_CTRL_WANTED
    FAN_MODE,
    FAN_RPM,
#endif
#ifdef ASPEED_BMC_WANTED
    BMC_SERSOR_1,
    BMC_SERSOR_2,
    BMC_SERSOR_3,
    BMC_SERSOR_4,
    BMC_MAC_SENSOR,
    BMC_DETECT,
#endif
#ifdef WDT_CTRL_WANTED
    WDT_CTRL,
#endif
#ifdef EEPROM_WP_WANTED
    EEPROM_WP_CTRL,
#endif
    HW_VER,
    SWITCH_BORAD_CPLD1,
    SWITCH_BORAD_CPLD2,
    SWITCH_BORAD_CPLD3,
    FAN_BORAD_CPLD
};
/* end of struct i2c_sysfs_attributes */

/* sysfs attributes for SENSOR_DEVICE_ATTR */
/* i2c-0 */
#ifdef EEPROM_WANTED
static SENSOR_DEVICE_ATTR(tlv_status        , S_IRUGO           , tlv_status_get    , NULL              , TLV_STATUS);
#endif
/*CPLD 0X30*/
static SENSOR_DEVICE_ATTR(psu_present       , S_IRUGO           , psu_status_get    , NULL              , PSU_PRESENT);
static SENSOR_DEVICE_ATTR(psu_status        , S_IRUGO           , psu_status_get    , NULL              , PSU_STATUS);
#ifdef PSU_STAT_WANTED
static SENSOR_DEVICE_ATTR(psu_module_1      , S_IRUGO           , psu_module_get    , NULL              , PSU_MODULE_1);
static SENSOR_DEVICE_ATTR(psu_module_2      , S_IRUGO           , psu_module_get    , NULL              , PSU_MODULE_2);
static SENSOR_DEVICE_ATTR(dc_chip_switch    , S_IRUGO           , dc_chip_switch_get , NULL             , DC_CHIP_SWITCH);
#endif
#ifdef USB_CTRL_WANTED
static SENSOR_DEVICE_ATTR(usb_power         , S_IRUGO | S_IWUSR , usb_power_get     , usb_power_set     , USB_POWER);
#endif
#ifdef LED_CTRL_WANTED
static SENSOR_DEVICE_ATTR(led_ctrl          , S_IRUGO | S_IWUSR , led_ctrl_get      , led_ctrl_set      , LED_CTRL);
#endif
static SENSOR_DEVICE_ATTR(led_loc           , S_IRUGO | S_IWUSR , led_loc_get       , led_loc_set       , LED_LOC);
static SENSOR_DEVICE_ATTR(led_alarm         , S_IRUGO | S_IWUSR , led_alarm_get     , led_alarm_set     , LED_ALARM);
static SENSOR_DEVICE_ATTR(reset_mac         , S_IRUGO | S_IWUSR , NULL              , reset_mac_set     , RESET_MAC);
static SENSOR_DEVICE_ATTR(sensor_status     , S_IRUGO           , themal_status_get , NULL              , SENSOR_STATUS);
#ifdef THEMAL_WANTED
static SENSOR_DEVICE_ATTR(sensor_temp       , S_IRUGO           , themal_temp_get   , NULL              , SENSOR_TEMP);
static SENSOR_DEVICE_ATTR(mac_temp          , S_IRUGO           , mac_temp_get      , NULL              , MAC_TEMP);
#endif
static SENSOR_DEVICE_ATTR(sensor_int_mask   , S_IRUGO           , themal_mask_get   , NULL              , SENSOR_INT_MASK);
static SENSOR_DEVICE_ATTR(sensor_int_mask_1   , S_IRUGO | S_IWUSR , NULL            , themal_mask_set   , 1);
static SENSOR_DEVICE_ATTR(sensor_int_mask_2   , S_IRUGO | S_IWUSR , NULL            , themal_mask_set   , 2);
static SENSOR_DEVICE_ATTR(sensor_int_mask_3   , S_IRUGO | S_IWUSR , NULL            , themal_mask_set   , 3);
static SENSOR_DEVICE_ATTR(sensor_int_mask_4   , S_IRUGO | S_IWUSR , NULL            , themal_mask_set   , 4);
static SENSOR_DEVICE_ATTR(int_status        , S_IRUGO           , int_status_get    , NULL              , INT_STATUS);
/*CPLD 0X31 & 0X32*/
static SENSOR_DEVICE_ATTR(QSFP_low_power_all , S_IRUGO | S_IWUSR , low_power_all_get , low_power_all_set , QSFP_LOW_POWER_ALL);
static SENSOR_DEVICE_ATTR(QSFP_low_power_1   , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 1);
static SENSOR_DEVICE_ATTR(QSFP_low_power_2   , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 2);
static SENSOR_DEVICE_ATTR(QSFP_low_power_3   , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 3);
static SENSOR_DEVICE_ATTR(QSFP_low_power_4   , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 4);
static SENSOR_DEVICE_ATTR(QSFP_low_power_5   , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 5);
static SENSOR_DEVICE_ATTR(QSFP_low_power_6   , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 6);
static SENSOR_DEVICE_ATTR(QSFP_low_power_7   , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 7);
static SENSOR_DEVICE_ATTR(QSFP_low_power_8   , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 8);
static SENSOR_DEVICE_ATTR(QSFP_low_power_9   , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 9);
static SENSOR_DEVICE_ATTR(QSFP_low_power_10  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 10);
static SENSOR_DEVICE_ATTR(QSFP_low_power_11  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 11);
static SENSOR_DEVICE_ATTR(QSFP_low_power_12  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 12);
static SENSOR_DEVICE_ATTR(QSFP_low_power_13  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 13);
static SENSOR_DEVICE_ATTR(QSFP_low_power_14  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 14);
static SENSOR_DEVICE_ATTR(QSFP_low_power_15  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 15);
static SENSOR_DEVICE_ATTR(QSFP_low_power_16  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 16);
static SENSOR_DEVICE_ATTR(QSFP_low_power_17  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 17);
static SENSOR_DEVICE_ATTR(QSFP_low_power_18  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 18);
static SENSOR_DEVICE_ATTR(QSFP_low_power_19  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 19);
static SENSOR_DEVICE_ATTR(QSFP_low_power_20  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 20);
static SENSOR_DEVICE_ATTR(QSFP_low_power_21  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 21);
static SENSOR_DEVICE_ATTR(QSFP_low_power_22  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 22);
static SENSOR_DEVICE_ATTR(QSFP_low_power_23  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 23);
static SENSOR_DEVICE_ATTR(QSFP_low_power_24  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 24);
static SENSOR_DEVICE_ATTR(QSFP_low_power_25  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 25);
static SENSOR_DEVICE_ATTR(QSFP_low_power_26  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 26);
static SENSOR_DEVICE_ATTR(QSFP_low_power_27  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 27);
static SENSOR_DEVICE_ATTR(QSFP_low_power_28  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 28);
static SENSOR_DEVICE_ATTR(QSFP_low_power_29  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 29);
static SENSOR_DEVICE_ATTR(QSFP_low_power_30  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 30);
static SENSOR_DEVICE_ATTR(QSFP_low_power_31  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 31);
static SENSOR_DEVICE_ATTR(QSFP_low_power_32  , S_IRUGO | S_IWUSR , low_power_get     , low_power_set     , 32);
static SENSOR_DEVICE_ATTR(QSFP_reset        , S_IRUGO | S_IWUSR , NULL              , qsfp_reset_set    , QSFP_RESET);
static SENSOR_DEVICE_ATTR(QSFP_present      , S_IRUGO           , qsfp_status_get   , NULL              , QSFP_PRESENT);
static SENSOR_DEVICE_ATTR(QSFP_int          , S_IRUGO           , qsfp_status_get   , NULL              , QSFP_INT);
#ifdef QSFP_WANTED
static SENSOR_DEVICE_ATTR(QSFP_temp         , S_IRUGO           , qsfp_temp_get     , NULL              , QSFP_TEMP);
static SENSOR_DEVICE_ATTR(QSFP_date         , S_IRUGO           , qsfp_date_get     , NULL              , QSFP_DATE);
static SENSOR_DEVICE_ATTR(QSFP_sn           , S_IRUGO           , qsfp_sn_get       , NULL              , QSFP_SN);
static SENSOR_DEVICE_ATTR(QSFP_pn           , S_IRUGO           , qsfp_pn_get       , NULL              , QSFP_PN);
static SENSOR_DEVICE_ATTR(QSFP_name         , S_IRUGO           , qsfp_name_get     , NULL              , QSFP_NAME);
static SENSOR_DEVICE_ATTR(QSFP_oui          , S_IRUGO           , qsfp_oui_get      , NULL              , QSFP_OUI);
static SENSOR_DEVICE_ATTR(QSFP_rev          , S_IRUGO           , qsfp_rev_get      , NULL              , QSFP_REV);
static SENSOR_DEVICE_ATTR(QSFP_connector    , S_IRUGO           , qsfp_connector_get, NULL              , QSFP_CONNECTOR);
static SENSOR_DEVICE_ATTR(QSFP_encoding     , S_IRUGO           , qsfp_encoding_get , NULL              , QSFP_ENCODING);
static SENSOR_DEVICE_ATTR(QSFP_nominal      , S_IRUGO           , qsfp_nominal_get  , NULL              , QSFP_NOMINAL);
static SENSOR_DEVICE_ATTR(QSFP_ext_rate_com , S_IRUGO           , qsfp_ext_rate_com_get  , NULL         , QSFP_EXT_RATE_COM);
static SENSOR_DEVICE_ATTR(QSFP_eth_com_code , S_IRUGO           , qsfp_eth_com_code_get  , NULL         , QSFP_ETH_COM_CODE);
static SENSOR_DEVICE_ATTR(QSFP_identifier   , S_IRUGO           , qsfp_identifier_get    , NULL         , QSFP_IDENTIFIER);
static SENSOR_DEVICE_ATTR(QSFP_fcmedia      , S_IRUGO           , qsfp_fc_media_get      , NULL         , QSFP_FCMEDIA);
static SENSOR_DEVICE_ATTR(QSFP_fcspeed      , S_IRUGO           , qsfp_fc_speed_get      , NULL         , QSFP_FCSPEED);
static SENSOR_DEVICE_ATTR(QSFP_fctech       , S_IRUGO           , qsfp_cab_tech_get      , NULL         , QSFP_FCTECH);
#endif
static SENSOR_DEVICE_ATTR(fan_status        , S_IRUGO           , fan_status_get         , NULL         , FAN_STATUS);
static SENSOR_DEVICE_ATTR(fan_present       , S_IRUGO           , fan_status_get         , NULL         , FAN_PRESENT);
static SENSOR_DEVICE_ATTR(fan_power         , S_IRUGO           , fan_status_get         , NULL         , FAN_POWER);
static SENSOR_DEVICE_ATTR(fan_speed_rpm     , S_IRUGO           , fan_status_get         , NULL         , FAN_SPEED_RPM);
#ifdef FAN_CTRL_WANTED
static SENSOR_DEVICE_ATTR(fan_mode          , S_IRUGO | S_IWUSR , fan_mode_get           , fan_mode_set , FAN_MODE);
static SENSOR_DEVICE_ATTR(fan_rpm           , S_IRUGO | S_IWUSR , fan_rpm_get            , fan_rpm_set  , FAN_RPM);
#endif
#ifdef ASPEED_BMC_WANTED
static SENSOR_DEVICE_ATTR(bmc_sersor_1      , S_IRUGO           , bmc_register_get       , NULL         , BMC_SERSOR_1);
static SENSOR_DEVICE_ATTR(bmc_sersor_2      , S_IRUGO           , bmc_register_get       , NULL         , BMC_SERSOR_2);
static SENSOR_DEVICE_ATTR(bmc_sersor_3      , S_IRUGO           , bmc_register_get       , NULL         , BMC_SERSOR_3);
static SENSOR_DEVICE_ATTR(bmc_sersor_4      , S_IRUGO           , bmc_register_get       , NULL         , BMC_SERSOR_4);
static SENSOR_DEVICE_ATTR(bmc_mac_sensor    , S_IRUGO           , bmc_register_get       , NULL         , BMC_MAC_SENSOR);
static SENSOR_DEVICE_ATTR(bmc_present       , S_IRUGO           , bmc_module_detect      , NULL         , BMC_DETECT);
#endif
#ifdef WDT_CTRL_WANTED
static SENSOR_DEVICE_ATTR(wdt_ctrl          , S_IRUGO | S_IWUSR , wdt_status_get         , wdt_status_set , WDT_CTRL);
#endif
#ifdef EEPROM_WP_WANTED
static SENSOR_DEVICE_ATTR(eeprom_wp_ctrl    , S_IRUGO | S_IWUSR , eeprom_wp_status_get   , eeprom_wp_status_set   , EEPROM_WP_CTRL);
#endif
static SENSOR_DEVICE_ATTR(hw_version        , S_IRUGO           , hw_version_get         , NULL         , HW_VER);
static SENSOR_DEVICE_ATTR(cpld1_version     , S_IRUGO           , cpld_version_get       , NULL         , SWITCH_BORAD_CPLD1);
static SENSOR_DEVICE_ATTR(cpld2_version     , S_IRUGO           , cpld_version_get       , NULL         , SWITCH_BORAD_CPLD2);
static SENSOR_DEVICE_ATTR(cpld3_version     , S_IRUGO           , cpld_version_get       , NULL         , SWITCH_BORAD_CPLD3);
static SENSOR_DEVICE_ATTR(cpld4_version     , S_IRUGO           , cpld_version_get       , NULL         , FAN_BORAD_CPLD);
/* end of sysfs attributes for SENSOR_DEVICE_ATTR */

/* sysfs attributes for hwmon */
/* i2c-0 */
#ifdef EEPROM_WANTED
static struct attribute *ESC601_EEPROM_attributes[] =
{
    &sensor_dev_attr_tlv_status.dev_attr.attr,
    NULL
};
#endif

static struct attribute *ESC601_SYS_attributes[] =
{
    &sensor_dev_attr_hw_version.dev_attr.attr,
    &sensor_dev_attr_cpld1_version.dev_attr.attr,
    &sensor_dev_attr_cpld2_version.dev_attr.attr,
    &sensor_dev_attr_cpld3_version.dev_attr.attr,
    &sensor_dev_attr_cpld4_version.dev_attr.attr,
#ifdef WDT_CTRL_WANTED
    &sensor_dev_attr_wdt_ctrl.dev_attr.attr,
#endif
#ifdef EEPROM_WP_WANTED
    &sensor_dev_attr_eeprom_wp_ctrl.dev_attr.attr,
#endif
    NULL
};

static struct attribute *ESC601_PSU_attributes[] =
{
    &sensor_dev_attr_psu_present.dev_attr.attr,
    &sensor_dev_attr_psu_status.dev_attr.attr,
#ifdef PSU_STAT_WANTED
    &sensor_dev_attr_psu_module_1.dev_attr.attr,
    &sensor_dev_attr_psu_module_2.dev_attr.attr,
    &sensor_dev_attr_dc_chip_switch.dev_attr.attr,
#endif
    NULL
};
#ifdef USB_CTRL_WANTED
static struct attribute *ESC601_USB_attributes[] =
{
    &sensor_dev_attr_usb_power.dev_attr.attr,
    NULL
};
#endif
static struct attribute *ESC601_LED_attributes[] =
{
#ifdef LED_CTRL_WANTED
    &sensor_dev_attr_led_ctrl.dev_attr.attr,
#endif
    &sensor_dev_attr_led_loc.dev_attr.attr,
    &sensor_dev_attr_led_alarm.dev_attr.attr,
    NULL
};

static struct attribute *ESC601_Reset_attributes[] =
{
    &sensor_dev_attr_reset_mac.dev_attr.attr,
    NULL
};

static struct attribute *ESC601_Sensor_attributes[] =
{
    &sensor_dev_attr_sensor_status.dev_attr.attr,
#ifdef THEMAL_WANTED
    &sensor_dev_attr_sensor_temp.dev_attr.attr,
    &sensor_dev_attr_mac_temp.dev_attr.attr,
#endif
    NULL
};

static struct attribute *ESC601_INT_attributes[] =
{
    &sensor_dev_attr_int_status.dev_attr.attr,
    &sensor_dev_attr_QSFP_int.dev_attr.attr,
    &sensor_dev_attr_sensor_int_mask.dev_attr.attr,
    &sensor_dev_attr_sensor_int_mask_1.dev_attr.attr,
    &sensor_dev_attr_sensor_int_mask_2.dev_attr.attr,
    &sensor_dev_attr_sensor_int_mask_3.dev_attr.attr,
    &sensor_dev_attr_sensor_int_mask_4.dev_attr.attr,
    NULL
};

static struct attribute *ESC601_QSFP_attributes[] =
{
    &sensor_dev_attr_QSFP_present.dev_attr.attr,
#ifdef QSFP_WANTED
    &sensor_dev_attr_QSFP_temp.dev_attr.attr,
    &sensor_dev_attr_QSFP_date.dev_attr.attr,
    &sensor_dev_attr_QSFP_sn.dev_attr.attr,
    &sensor_dev_attr_QSFP_pn.dev_attr.attr,
    &sensor_dev_attr_QSFP_name.dev_attr.attr,
    &sensor_dev_attr_QSFP_oui.dev_attr.attr,
    &sensor_dev_attr_QSFP_rev.dev_attr.attr,
    &sensor_dev_attr_QSFP_connector.dev_attr.attr,
    &sensor_dev_attr_QSFP_encoding.dev_attr.attr,
    &sensor_dev_attr_QSFP_nominal.dev_attr.attr,
    &sensor_dev_attr_QSFP_ext_rate_com.dev_attr.attr,
    &sensor_dev_attr_QSFP_eth_com_code.dev_attr.attr,
    &sensor_dev_attr_QSFP_identifier.dev_attr.attr,
    &sensor_dev_attr_QSFP_fcmedia.dev_attr.attr,
    &sensor_dev_attr_QSFP_fcspeed.dev_attr.attr,
    &sensor_dev_attr_QSFP_fctech.dev_attr.attr,
#endif
    &sensor_dev_attr_QSFP_reset.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_all.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_1.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_2.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_3.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_4.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_5.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_6.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_7.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_8.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_9.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_10.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_11.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_12.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_13.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_14.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_15.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_16.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_17.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_18.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_19.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_20.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_21.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_22.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_23.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_24.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_25.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_26.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_27.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_28.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_29.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_30.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_31.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_32.dev_attr.attr,
    NULL
};

static struct attribute *ESC601_FAN_attributes[] = {
    &sensor_dev_attr_fan_status.dev_attr.attr,
    &sensor_dev_attr_fan_present.dev_attr.attr,
    &sensor_dev_attr_fan_power.dev_attr.attr,
    &sensor_dev_attr_fan_speed_rpm.dev_attr.attr,
#ifdef FAN_CTRL_WANTED
    &sensor_dev_attr_fan_mode.dev_attr.attr,
    &sensor_dev_attr_fan_rpm.dev_attr.attr,
#endif
    NULL
};

#ifdef ASPEED_BMC_WANTED
static struct attribute *ESC601_BMC_attributes[] = {
    &sensor_dev_attr_bmc_sersor_1.dev_attr.attr,
    &sensor_dev_attr_bmc_sersor_2.dev_attr.attr,
    &sensor_dev_attr_bmc_sersor_3.dev_attr.attr,
    &sensor_dev_attr_bmc_sersor_4.dev_attr.attr,
    &sensor_dev_attr_bmc_mac_sensor.dev_attr.attr,
    &sensor_dev_attr_bmc_present.dev_attr.attr,
    NULL
};
#endif
/* end of sysfs attributes for hwmon */

/* struct attribute_group */
#ifdef EEPROM_WANTED
static const struct attribute_group ESC601_EEPROM_group =
{
    .name  = "ESC601_EEPROM",
    .attrs = ESC601_EEPROM_attributes,
};
#endif

static const struct attribute_group ESC601_SYS_group =
{
    .name  = "ESC601_SYS",
    .attrs = ESC601_SYS_attributes,
};

static const struct attribute_group ESC601_PSU_group =
{
    .name  = "ESC601_PSU",
    .attrs = ESC601_PSU_attributes,
};

#ifdef USB_CTRL_WANTED
static const struct attribute_group ESC601_USB_group =
{
    .name  = "ESC601_USB",
    .attrs = ESC601_USB_attributes,
};
#endif

static const struct attribute_group ESC601_LED_group =
{
    .name  = "ESC601_LED",
    .attrs = ESC601_LED_attributes,
};

static const struct attribute_group ESC601_Reset_group =
{
    .name  = "ESC601_Reset",
    .attrs = ESC601_Reset_attributes,
};

static const struct attribute_group ESC601_Sensor_group =
{
    .name  = "ESC601_Sensor",
    .attrs = ESC601_Sensor_attributes,
};

static const struct attribute_group ESC601_INT_group =
{
    .name  = "ESC601_INT",
    .attrs = ESC601_INT_attributes,
};

static const struct attribute_group ESC601_QSFP_group =
{
    .name  = "ESC601_QSFP",
    .attrs = ESC601_QSFP_attributes,
};

static const struct attribute_group ESC601_FAN_group =
{
    .name  = "ESC601_FAN",
    .attrs = ESC601_FAN_attributes,
};

#ifdef ASPEED_BMC_WANTED
static const struct attribute_group ESC601_BMC_group =
{
    .name  = "ESC601_BMC",
    .attrs = ESC601_BMC_attributes,
};
#endif
/* end of struct attribute_group */
