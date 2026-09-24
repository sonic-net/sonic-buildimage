#ifndef _WB_NVME_DRIVER_H_
#define _WB_NVME_DRIVER_H_

#define NVME_MAX_NUMBER                             (4)

#define DFD_GET_NVME_KEY2(nvme_id, func) \
        (((nvme_id & 0xff) << 8) | (func & 0xff))

#define DFD_GET_NVME_SENSOR_KEY1(gcu_type, gcu_id) \
    (((gcu_type & 0xffff) << 16) | (gcu_id & 0xff))

#define DFD_GET_NVME_SENSOR_KEY2(func, sensor_index) \
    (((func & 0xff) << 8) | (sensor_index & 0xff))

#define DFD_COM_NVME_SENSOR_MAIN_DEV_ID(nvme_index, sensor_index)  \
        (nvme_index << 16 | sensor_index)

#define IS_TARGET_SENSOR_STATIC(x) \
    ((x) == DFD_BMC_MANAGED_SENSOR_ALIAS_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_MAX_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_MIN_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_HIGH_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_LOW_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E)

typedef enum {
    STORAGE_INTERFACE_SATA,
    STORAGE_INTERFACE_NVME,
    STORAGE_INTERFACE_SAS,
    STORAGE_INTERFACE_IDE,
    STORAGE_INTERFACE_USB,
    STORAGE_INTERFACE_ESATA,
    STORAGE_INTERFACE_FC,    /* fiber channel */
    STORAGE_INTERFACE_NOF,   /* NVME over fiber */
    STORAGE_INTERFACE_END
} storage_interface_type_t;

typedef enum {
    STORAGE_MEDIA_HDD,
    STORAGE_MEDIA_FDD,
    STORAGE_MEDIA_SSD,
    STORAGE_MEDIA_HYBRID,
    STORAGE_MEDIA_SCM,
    STORAGE_MEDIA_END
} storage_media_type_t;

typedef enum {
    STORAGE_OUTPUT_ASCII,
    STORAGE_OUTPUT_INT,
    STORAGE_OUTPUT_HEXADECIMAL ,
    STORAGE_OUTPUT_END
} storage_output_type_t;

typedef enum {
    STORAGE_ENDIAN_BE,
    STORAGE_ENDIAN_LE,
    STORAGE_ENDIAN_END
} storage_endian_type_t;

typedef struct {
    const char *name;
    uint8_t command_code;
    uint8_t block_length;
    uint8_t offset;
    uint8_t size;
    uint8_t output_type;
    uint8_t endian;
    int     conv_factor;
} nvme_func_attr_t;

typedef struct {
    const char *name;
    uint16_t nvme_main_id;
    nvme_func_attr_t vendor_attr;
    nvme_func_attr_t dev_attr;
    uint16_t vendor_id;
    uint16_t device_id;
    nvme_func_attr_t *basic_attr_table;
    nvme_func_attr_t *temp_attr_table;
    unsigned int temp_number;
    nvme_func_attr_t *power_attr_table;
    unsigned int power_number;
    storage_interface_type_t interface_type;
    storage_media_type_t storage_type;
} nvme_list_t;

typedef struct {
    int nvme_type;
    int monitor_last_flag;
    int monitor_curr_flag;
    int i2c_bus_num;
    uint32_t i2c_oob_addr;
    uint32_t i2c_vpd_addr;
} nvme_device_type_t;

extern dfd_sysfs_func_map_t nvme_basic_func_table[DFD_NVME_MAX_E];
extern dfd_sysfs_func_map_t nvme_temp_func_table[DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E] ;
extern dfd_sysfs_func_map_t nvme_power_func_table[DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E];
extern nvme_list_t nvme_list[WB_MIAN_NVME_TYPE_MAX];
#endif /* _WB_NVME_DRIVER_H_ */