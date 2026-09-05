#ifndef _LINUX_DRAM_DRIVER_H
#define _LINUX_DRAM_DRIVER_H

#include <linux/types.h>
#include <linux/compiler.h>

struct phydev_user_info {
    int phy_index;    /* Indicates which phydev to operate on */
    u32 regnum;       /* Register address */
    u32 regval;       /* Register value */
};

#define CMD_PHY_LIST                        _IOR('P', 0, struct phydev_user_info)
#define CMD_PHY_READ                        _IOR('P', 1, struct phydev_user_info)
#define CMD_PHY_WRITE                       _IOR('P', 2, struct phydev_user_info)

struct mdio_dev_user_info {
    int mdio_index;    /* Indicates which mdio dev to operate on */
    int phyaddr;       /* phy device address */
    u32 regnum;        /* Register address */
    u32 regval;        /* Register value */
};

#define CMD_MDIO_LIST                        _IOR('M', 0, struct mdio_dev_user_info)
#define CMD_MDIO_READ                        _IOR('M', 1, struct mdio_dev_user_info)
#define CMD_MDIO_WRITE                       _IOR('M', 2, struct mdio_dev_user_info)

#endif
