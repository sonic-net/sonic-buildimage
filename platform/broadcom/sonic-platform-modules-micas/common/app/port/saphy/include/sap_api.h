/********************************************************************************
 * Copyright(C) 2020 Micas Network. All rights reserved.
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期        | 作者     |  修改记录
**===============================================================================
**| 2024/08/23  | zhoutenghui  |  创建该文件
**
*********************************************************************************/
#ifndef _SAP_APIS_H_
#define _SAP_APIS_H_

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "list.h"

#define SAPHY_PROGRAM_VERSION "1.0"

#define EXTPHY_APIS_NUMBER (5)

#define PLP_SYS_IF_SIDE (1)
#define PLP_LINE_IF_SIDE (0)
#define PLP_TX_DIR (0)
#define PLP_RX_DIR (1)
#define PLP_PORT_ENABLE (1)
#define PLP_PORT_DISABLE (0)
#define SAP_MAX_PORT_NUM 512
#define SAP_LANE_ID_START 1
#define SAP_LANE_ID_ALL   0
typedef struct {
    uint8_t  unit;
    uint16_t port;
    uint16_t phy_lane0;
    void *port_info;
    struct list_head node;
} sap_port_info_t;

typedef struct
{
    char *type;
    struct list_head *list;
    int port_count;
}sap_port_list_t;

#define SAP_PLIST_ITER(port_list, port_info) \
    list_for_each_entry(port_info, ((port_list)->list), node, sap_port_info_t)

typedef enum {
    SAP_PORT_SIDE_LINE = 0,
    SAP_PORT_SIDE_SYS = 1,
    SAP_PORT_SIDE_MAX
} sap_port_side_e;

typedef enum {
    SAP_LOOPBACK_DIR_BOTH = 0,
    SAP_LOOPBACK_DIR_LOCAL,
    SAP_LOOPBACK_DIR_REMOTE,
    SAP_LOOPBACK_DIR_ANALOG,
    SAP_LOOPBACK_DIR_MAX
} sap_loopback_dir_e;

typedef enum {
    SAP_CHANNEL_TX = 0,
    SAP_CHANNEL_RX = 1,
    SAP_CHANNEL_BOTH = 2,
    SAP_CHANNEL_MAX
} sap_channel_dir_e;
typedef enum {
    SAP_CHANNEL_RANGE_NR = 0,
    SAP_CHANNEL_RANGE_ER = 1,
    SAP_CHANNEL_RANGE_MAX
} sap_channel_range_e;

typedef enum {
    SAP_PRBS_POLY_P7 = 0,
    SAP_PRBS_POLY_P15 = 1,
    SAP_PRBS_POLY_P23 = 2,
    SAP_PRBS_POLY_P31 = 3,
    SAP_PRBS_POLY_P9 = 4,
    SAP_PRBS_POLY_P11 = 5,
    SAP_PRBS_POLY_P58 = 6,
    SAP_PRBS_POLY_P49 = 7,
    SAP_PRBS_POLY_P20 = 8,
    SAP_PRBS_POLY_P13 = 9,
    SAP_PRBS_POLY_P10 = 10,
    SAP_PRBS_POLY_P19 = 11,
    SAP_PRBS_POLY_MAX
} sap_prbs_pol_e;

typedef enum {
    SAP_STATUS_SUCCESS                =  0,
    SAP_STATUS_FAILURE                = -1,
    SAP_STATUS_NOT_SUPPORTED          = -2,
    SAP_STATUS_NO_MEMORY              = -3,
    SAP_STATUS_INSUFFICIENT_RESOURCES = -4,
    SAP_STATUS_INVALID_PARAMETER      = -5,
    SAP_STATUS_ITEM_ALREADY_EXISTS    = -6,
    SAP_STATUS_ITEM_NOT_FOUND         = -7,
    SAP_STATUS_NOT_IMPLEMENTED        = -8,
    SAP_STATUS_NOT_SUPPORTED_PORT     = -9,
} sap_status_type_e;

typedef enum {
    SAP_FEC_INVALID = 0, /* invalid */
    SAP_FEC_NONE, /* no fec */
    SAP_FEC_BASER, /* CL74/Base-R. 64/66b KR FEC for fabric */
    SAP_FEC_RSFEC, /* CL91/RS-FEC */
    SAP_FEC_RS544, /* Rs544, using 1xN RS FEC architecture */
    SAP_FEC_RS272, /* Rs272, using 1xN RS FEC architecture */
    SAP_FEC_RS206, /* Rs206. 64/66b 5T RS FEC for fabric */
    SAP_FEC_RS108, /* Rs108. 64/66b 5T low latency RS FEC for fabric */
    SAP_FEC_RS545, /* Rs545. 64/66b 15T RS FEC for fabric */
    SAP_FEC_RS304, /* Rs304. 64/66b 15T low latency RS FEC for fabric */ 
    SAP_FEC_RS544_2XN, /* Rs544, using 2xN RS FEC architecture */
    SAP_FEC_RS272_2XN, /* Rs272, using 2xN RS FEC architecture */
    SAP_FEC_MAX
} sap_fec_e;

typedef enum {
    SAP_MEDIUM_INVALID = 0,
    SAP_MEDIUM_BACKPLANE,
    SAP_MEDIUM_COPPER,
    SAP_MEDIUM_FIBER,
    SAP_MEDIUM_MAX
} sap_medium_e;

typedef struct {
    uint8_t  is_get;
    uint32_t prbs_lock;
    uint32_t lock_loss;
    uint32_t lane_err_cnt;
    double ber;
} sap_prbs_status_t;

typedef struct {
    int phy_lane0;
    int speed;
    int host_lanes;
    int line_lanes;
    sap_fec_e host_fec_type;
    sap_fec_e line_fec_type;
    int link_training;
} sap_port_resource_t;

typedef struct {
    int pre3;
    int pre2;
    int pre;
    int main;
    int post;
    int post2;
    int post3;
} sap_tx_fir_t;

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

int  sap_platform_init(int unit, bool warm_boot);
int  sap_port_speed_set(int unit, int port, int speed);
int  sap_port_linktrain_set(int unit, int port, int if_side, int enable);
int  sap_squelch_set(int unit, int port, int if_side, int tx_rx, int enable, int lane);
int  sap_port_fec_get(int unit, int port, int if_side, sap_fec_e *fec);
int  sap_port_prbs_set(int unit, int port, int poly, int inv, int if_side, int lane);
int  sap_port_prbs_get(int unit, int port, int if_side, sap_prbs_status_t *prbs_info);
int  sap_port_prbs_clear(int unit, int port, int if_side);
int  sap_port_prbs_ber_get(int unit, int port, int if_side, int time_v, sap_prbs_status_t *prbs_info);
int  sap_loopback_set(int unit, int port, int if_side, int lb_dir, int enable);
int  sap_loopback_get(int unit, int port, int if_side, int lb_dir, int *enable);
int  sap_autoneg_set(int unit, int port, int if_side, int enable);
int  sap_autoneg_get(int unit, int port, int if_side, int *enable);
int  sap_polarity_set(int unit, int port, int if_side, int tx_rx, uint32_t polarity, bool override);
int  sap_polarity_get(int unit, int port, int if_side, int tx_rx, uint32_t *polarity);
int  sap_tx_fir_set(int unit, int port, int if_side, int lane, sap_tx_fir_t tx_fir);
int  sap_tx_fir_get(int unit, int port, int if_side, int lane, sap_tx_fir_t *tx_fir);
int  sap_reg_set(int unit, int port, int if_side, int devaddr, int regaddr, int flag, uint64_t data);
int  sap_reg_get(int unit, int port, int if_side, int devaddr, int regaddr, int flag, uint64_t *data);
int  sap_phy_status_dump(int unit, int port, int level);
int  sap_dsc_dump(int unit, int port, int if_side, int flag, int lane);
int  sap_eyescan_dump(int unit, int port, int if_side, int lane);
int  sap_shell_cmd_run(int unit, int port, char *cmdline);
int  sap_phy_reset(int unit, int port, int flags);
int  sap_temperature_get(int unit, int port, double *temp);
int  sap_highest_temperature_get(double *temp);
bool sap_port_support(int unit, int port);
bool sap_physical_port_support(int unit, int physical_port);
int  sap_mib_dump(int unit, int port, int if_side);
int  sap_fw_version_get(int unit, int port, sap_version_t *version_info);
int  sap_eeprom_update(int unit, int port, bool use_file);
void sap_techsupport(int unit, int port);
int  sap_channel_reach_set(int unit, int port, int if_side, int lane, int nr_er);

int sap_port_list_get(sap_port_list_t *port_list);
int sap_port_status_get(int unit, int port, sap_port_status_t *port_status);
#endif /* _SAP_APIS_H_ */
