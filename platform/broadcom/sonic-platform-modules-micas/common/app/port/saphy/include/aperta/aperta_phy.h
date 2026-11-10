/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * PHY Application Layer Interface Header
 ********************************************************************************/
#ifndef _PORT_PHY_H_
#define _PORT_PHY_H_

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "common.h"
#include "mdio.h"
#include "aperta_phy_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================================================
 * Constants
 * ========================================================================== */
#define PHY_MAX_NUM              (64)
#define PHY_MAX_LANE             (8)
#define PHY_MAX_PORT_NUM         (16)
#define PHY_MAX_LINE_STRING_LEN  (128)

#define PHY_SPEED_10G            (10000)
#define PHY_SPEED_25G            (25000)
#define PHY_SPEED_40G            (40000)
#define PHY_SPEED_50G            (50000)
#define PHY_SPEED_100G           (100000)
#define PHY_SPEED_200G           (200000)
#define PHY_SPEED_400G           (400000)
#define PHY_SPEED_800G           (800000)

#define BCM_81394_CHIP_ID_ADDR   (0x18b00)
#define BCM_81394_CHIP_ID        (0x1394)

#define PORT_ADD_F_INIT_PASS1    (1)
#define PORT_ADD_F_INIT_PASS2    (2)

/* ==========================================================================
 * FEC enum & mapping
 * ========================================================================== */
typedef enum {
    FEC_INVALID = 0,
    FEC_NONE,
    FEC_BASER,
    FEC_RSFEC,
    FEC_RS544,
    FEC_RS272,
    FEC_RS206,
    FEC_RS108,
    FEC_RS545,
    FEC_RS304,
    FEC_RS544_2XN,
    FEC_RS272_2XN,
    FEC_MAX
} fec_e;

typedef struct {
    fec_e sap_fec;
    int   phy_fec;
} phy_fec_map_t;

/* ==========================================================================
 * Port resource, version, status types
 * ========================================================================== */
typedef struct {
    int phy_lane0;
    int speed;
    int host_lanes;
    int line_lanes;
    fec_e host_fec_type;
    fec_e line_fec_type;
    int link_training;
} sap_port_resource_t;

typedef struct {
    uint32_t phy_id;
    char chip_name[32];
    uint32_t fw_ver;
    uint32_t fw_crc;
    uint32_t rev_id;
    uint32_t drv_major_ver;
    uint32_t drv_minor_ver;
} sap_version_t;

typedef struct {
    bool host_admin;
    bool line_admin;
    bool host_link_up;
    bool line_link_up;
    uint8_t host_lane_num;
    uint8_t line_lane_num;
    char mode[32];
    int speed;
    int fec_sys;
    int fec_line;
} sap_port_status_t;

#define SAP_BCM_CFG_FILE  "/usr/share/sonic/hwsku/bcm_81394.cfg"

/* ==========================================================================
 * PHY info structure
 * ========================================================================== */
typedef struct phy_info_s {
    char     *chip_name;            /* "aperta" */
    uint8_t   loaded;               /* FW loaded flag */
    uint8_t   unit;                 /* unit id */
    uint32_t  phy_addr;             /* PHY address */
    uint8_t   card_id;              /* slot id */
    uint8_t   mdio_id;              /* MDIO ID */
    uint8_t   mdio_type;            /* MDIO access type */
    uint32_t  macsec_option;        /* MACsec option */
    uint32_t  pll1_vco_rate;        /* PLL1 VCO rate */
    uint32_t  ptp_option;           /* PTP option */
    uint8_t   vco_octal_0_sys;      /* Octal 0 System side VCO */
    uint8_t   vco_octal_0_line;     /* Octal 0 Line side VCO */
    uint8_t   vco_octal_1_sys;      /* Octal 1 System side VCO */
    uint8_t   vco_octal_1_line;     /* Octal 1 Line side VCO */
    int       tx_pol_sys;           /* System side TX polarity flip */
    int       rx_pol_sys;           /* System side RX polarity flip */
    int       tx_pol_line;          /* Line side TX polarity flip */
    int       rx_pol_line;          /* Line side RX polarity flip */
    int       tx_lane_map_line[2];  /* Line side TX lane map */
    int       rx_lane_map_line[2];  /* Line side RX lane map */
    int       tx_lane_map_sys[2];   /* System side TX lane map */
    int       rx_lane_map_sys[2];   /* System side RX lane map */
    int       profile_id;           /* profile ID */
    uint8_t   lane_num;             /* lane count */
    uint8_t   port_num;             /* port count */
    int       lanes[PHY_MAX_PORT_NUM];        /* MAC side physical lanes */
    int       ports[PHY_MAX_PORT_NUM];        /* MAC side logical ports */
    int       ports_lane_num[PHY_MAX_PORT_NUM]; /* lanes per port */
    void     *port_infos[PHY_MAX_PORT_NUM];   /* built port_info_t per logical port (robust, list-free) */
    mdio_read_func_t  mdio_read;    /* MDIO read function */
    mdio_write_func_t mdio_write;   /* MDIO write function */
} phy_info_t;

/* ==========================================================================
 * Port info structure
 * ========================================================================== */
typedef struct port_info_s {
    int  inited;                    /* port initialized */
    int  unit;                      /* unit id */
    int  port;                      /* port number */
    int  phy_lane0;                 /* starting lane */
    int  lanes;                     /* lane count */
    int  speed;                     /* speed */
    int  linktrain_sys;             /* System side link training */
    int  linktrain_line;            /* Line side link training */
    int  lanemap_sys;               /* System side lane bitmap */
    int  lanemap_line;              /* Line side lane bitmap */
    int  fec_sys;                   /* System side FEC */
    int  fec_line;                  /* Line side FEC */
    int  if_type_sys;               /* System side interface type */
    int  if_type_line;              /* Line side interface type */
    int  force_nr_sys;              /* System side NR force */
    int  force_nr_line;             /* Line side NR force */
    int  force_er_sys;              /* System side ER force */
    int  force_er_line;             /* Line side ER force */
    phy_info_t *phy_info;           /* parent PHY */
} port_info_t;

/* ==========================================================================
 * Platform init
 * ========================================================================== */
int  phy_platform_init(int unit);

/* ==========================================================================
 * Port operations
 * ========================================================================== */
int  phy_port_info_get(int unit, int port, port_info_t **port_info);

/* ==========================================================================
 * PHY control
 * ========================================================================== */
int  phy_fw_version_get(int unit, int port, sap_version_t *version_info);
int  phy_port_status_get(int unit, int port, sap_port_status_t *port_status);

/* ==========================================================================
 * Internal functions (exposed for debug)
 * ========================================================================== */
int  phy_chip_id_get(phy_info_t *phy_info, uint32_t phy_addr, uint32_t *chip_id);
int  phy_sdk_access_get(int unit, int port, int if_side, phy_sdk_access_t *sdk_access);

#ifdef __cplusplus
}
#endif

#endif /* _PORT_PHY_H_ */
