#ifndef __SSD_POWER_H__
#define __SSD_POWER_H__

typedef int (*ata_power_reset_func_t)(unsigned int);

#define SSD_POWER_NAME_MAX_LEN (64)

typedef struct {
    uint32_t addr;
    uint32_t en_val;
    uint32_t dis_val;
    uint32_t mask;
    uint32_t delay;
} ssd_power_wr_pro_device_t;

typedef struct {
    uint32_t port_id;
    char file_name[SSD_POWER_NAME_MAX_LEN];
    uint32_t addr;
    uint32_t power_on;
    uint32_t power_off;
    uint32_t power_mask;
    uint32_t power_on_delay;
    uint32_t power_off_delay;
    uint32_t reg_access_mode;
    bool need_wr_pro;
    ssd_power_wr_pro_device_t wr_pro_info;
    int device_flag;            /* The device generates a marker, 0: success, -1: failure */
} ssd_power_device_t;

#endif /* __SSD_POWER_H__ */
