#ifndef __WB_SWITCH_DEV_H__
#define __WB_SWITCH_DEV_H__

#define MAX_MDIO_DEVICES 5

enum chips {
    BCM53134
};

typedef struct switch_dev_info {
    mdio_dev_device_t mdio_dev_device[MAX_MDIO_DEVICES];
    struct device *dev;
    uint32_t dev_num;
    struct platform_device platform_device[MAX_MDIO_DEVICES];
    const char *mdio_bus_name;
    int dev_type;
    int device_flag;
} switch_dev_info_t;

#define BCM53134_MAX_DEV_NUMBER          (5)

#endif /* __WB_SWITCH_DEV_H__ */
