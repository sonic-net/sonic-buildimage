#ifndef __WB_MDIO_DEV_H__
#define __WB_MDIO_DEV_H__
#include <wb_logic_dev_common.h>
#include <wb_bsp_kernel_debug.h>
#include <linux/mii.h>

#define MDIO_DEV_COMPATIBLE_NAME         "wb_mdio_dev"

#define PHY_REG_INIT_INFO_MAX       (16)
#define NAME_LEN                    (32)
#define MAX_RETRY                   (3)
#define PHY_EXTERN_ADDR_REG         (0x1e)
#define PHY_EXTERN_DATA_REG         (0x1f)

#define PHY_LINK_STS_UP             (1)
#define PHY_LINK_STS_DOWN           (0)
#define PHY_RESET_BIT               BIT(15)
#define PHY_RESET_ENABLE            (1)
#define PHY_RESET_DISABLE           (0)
#define MDIO_STATUS_OK              (0)
#define MDIO_STATUS_NOT_OK          (1)
#define UNKNOWN_PHY_TYPE            "NA"
#define MDIO_NAME_SUFFIX            "_phy"

typedef struct {
    const char  phy_type[NAME_LEN];
    uint32_t    phy_id;
} phy_type_info_t;

phy_type_info_t phy_type_infos[] = {
    {.phy_type = "BCM53134O_GPHY",   .phy_id = 0xae025350},
    {.phy_type = "B50210S",          .phy_id = 0x600d8595},
    {.phy_type = "YT8010",           .phy_id = 0x00000309},
    {.phy_type = "YT8010AS",         .phy_id = 0x4f51eb19},
    {.phy_type = "YT8510",           .phy_id = 0x00000109},
    {.phy_type = "YT8511",           .phy_id = 0x0000010a},
    {.phy_type = "YT8512",           .phy_id = 0x00000118},
    {.phy_type = "YT8512B",          .phy_id = 0x00000128},
    {.phy_type = "YT8521",           .phy_id = 0x0000011a},
    {.phy_type = "YT8531S",          .phy_id = 0x4f51e91a},
    {.phy_type = "YT8531",           .phy_id = 0x4f51e91b},
    {.phy_type = "YT8614",           .phy_id = 0x4F51E899},
    {.phy_type = "YT8618",           .phy_id = 0x0000e889},
    {.phy_type = "YT8821",           .phy_id = 0x4f51ea10},
    {.phy_type = "BCM53134O_SGMII",  .phy_id = 0x143bff0},
};

typedef struct {
    const char  reg_name[NAME_LEN];
    uint32_t    reg_addr;
} phy_reg_info_t;

phy_reg_info_t phy_reg_infos[] = {
    {.reg_name = "CONTROL",         .reg_addr = MII_BMCR},
    {.reg_name = "STATUS",          .reg_addr = MII_BMSR},
    {.reg_name = "PHYS_ID1",        .reg_addr = MII_PHYSID1},
    {.reg_name = "PHYS_ID2",        .reg_addr = MII_PHYSID2},
    {.reg_name = "ADVERTISEMENT",   .reg_addr = MII_ADVERTISE},
    {.reg_name = "LPA",             .reg_addr = MII_LPA},
    {.reg_name = "EXPANSION",       .reg_addr = MII_EXPANSION},
    {.reg_name = "CTRL1000",        .reg_addr = MII_CTRL1000},
    {.reg_name = "STAT1000",        .reg_addr = MII_STAT1000},
    {.reg_name = "MMD_CTRL",        .reg_addr = MII_MMD_CTRL},
    {.reg_name = "MMD_DATA",        .reg_addr = MII_MMD_DATA},
};

typedef struct {
    uint32_t    phy_reg;
    uint32_t    init_value;
    uint32_t    mask;
    uint32_t    delay;
} phy_reg_init_info_t;

typedef struct mdio_dev_info {
    const char *name;
    const char *alias;
    const char *mdio_bus_name;
    struct attribute_group *sysfs_group;
    uint32_t addr;
    struct mii_bus *bus;
    phy_reg_init_info_t phy_reg_init[PHY_REG_INIT_INFO_MAX];
    int phy_need_init;
    uint32_t    reg_init_num;
    struct device *dev;
    struct kobject kobj;
    struct mutex lock;
} mdio_dev_info_t;

typedef struct mdio_dev_device_s {
    const char *dev_name;
    const char *alias;
    const char *mdio_bus_name;
    uint32_t addr;
    int device_flag;
} mdio_dev_device_t;

#endif /* __WB_MDIO_DEV_H__ */
