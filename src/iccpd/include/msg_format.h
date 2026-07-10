/*
 * msg_format.h
 *
 * Copyright(c) 2016-2019 Nephos/Estinet.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 *  Maintainer: jianjun, grace Li from nephos
 */

#ifndef MSG_FORMAT_H_
#define MSG_FORMAT_H_
#include <sys/types.h>
#include <stdint.h>
#include "../include/port.h"

#define MAX_MSG_LOG_SIZE    128

#define ETHER_ADDR_LEN 6

/* Header version for message sent from ICCPd to Mclagsyncd */
#define ICCPD_TO_MCLAGSYNCD_HDR_VERSION  1

/*
 * RFC 5561
 * 4.  Capability Message
 * [Page 7]
 */
#define MSG_T_CAPABILITY 0x0202

/*
 * RFC 7275
 * 6.1.1.  ICC Header - Message Length
 * [Page 25]
 * 2-octet integer specifying the total length of this message in octets,
 * excluding the "U-bit", "Message Type", and "Length" fields.
 */
#define MSG_L_INCLUD_U_BIT_MSG_T_L_FIELDS 4

/*
 * RFC 7275
 * 12.1.  Message Type Name Space
 * [Page 79]
 */
#define MSG_T_RG_CONNECT    0x0700
#define MSG_T_RG_DISCONNECT 0x0701
#define MSG_T_NOTIFICATION  0x0702
#define MSG_T_RG_APP_DATA   0x0703

/*
 * RFC 7275
 * 12.2.  TLV Type Name Space
 * [Page 79]
 */
#define TLV_T_ICCP_CAPABILITY 0x0700
#define TLV_L_ICCP_CAPABILITY 0x4

/*
 * RFC 7275
 * 12.3.  ICC RG Parameter Type Space
 * [Page 80]
 */
#define TLV_T_ICC_SENDER_NAME           0x0001
#define MAX_L_ICC_SENDER_NAME           80
#define TLV_T_NAK                       0x0002
#define TLV_T_REQUESTED_PROTOCOL_VER    0x0003
#define TLV_T_DISCONNECT_CODE           0x0004
#define TLV_L_DISCONNECT_CODE           0x4
#define TLV_T_ICC_RG_ID                 0x0005
#define TLV_L_ICC_RG_ID                 0x4

#define TLV_T_MLACP_CONNECT             0x0030
#define TLV_T_MLACP_DISCONNECT          0x0031
#define TLV_T_MLACP_SYSTEM_CONFIG       0x0032
#define TLV_T_MLACP_PORT_CONFIG         0x0033  //no support
#define TLV_T_MLACP_PORT_PRIORITY       0x0034  //no support
#define TLV_T_MLACP_PORT_STATE          0x0035  //no support
#define TLV_T_MLACP_AGGREGATOR_CONFIG   0x0036
#define TLV_T_MLACP_AGGREGATOR_STATE    0x0037
#define TLV_T_MLACP_SYNC_REQUEST        0x0038
#define TLV_T_MLACP_SYNC_DATA           0x0039
#define TLV_T_MLACP_HEARTBEAT           0x003A
#define TLV_T_MLACP_DISCONNECT_CAUSE    0x003B //not yet

/* Self define Feature */
#define TLV_T_MLACP_ORPHAN_PORT         0x1033 //not yet
#define TLV_T_MLACP_PORT_CHANNEL_INFO   0x1034
#define TLV_T_MLACP_PEERLINK_INFO       0x1035
#define TLV_T_MLACP_ARP_INFO            0x1036
#define TLV_T_MLACP_STP_INFO            0x1037//no support
#define TLV_T_MLACP_MAC_INFO            0x1038
#define TLV_T_MLACP_WARMBOOT_FLAG       0x1039
#define TLV_T_MLACP_NDISC_INFO          0x103A
#define TLV_T_MLACP_IF_UP_ACK           0x103B
#define TLV_T_MLACP_LIST_END            0x104a //list end

/* Debug */
static char* get_tlv_type_string(int type)
{
    switch (type)
    {
        case TLV_T_ICCP_CAPABILITY:
            return "TLV_T_ICCP_CAPABILITY";

        case TLV_T_ICC_SENDER_NAME:
            return "TLV_T_ICC_SENDER_NAME";

        case TLV_T_NAK:
            return "TLV_T_NAK";

        case TLV_T_REQUESTED_PROTOCOL_VER:
            return "TLV_T_REQUESTED_PROTOCOL_VER";

        case TLV_T_DISCONNECT_CODE:
            return "TLV_T_DISCONNECT_CODE";

        case TLV_T_ICC_RG_ID:
            return "TLV_T_ICC_RG_ID";

        case TLV_T_MLACP_CONNECT:
            return "TLV_T_MLACP_CONNECT";

        case TLV_T_MLACP_DISCONNECT:
            return "TLV_T_MLACP_DISCONNECT";

        case TLV_T_MLACP_SYSTEM_CONFIG:
            return "TLV_T_MLACP_SYSTEM_CONFIG";

        case TLV_T_MLACP_PORT_CONFIG:
            return "TLV_T_MLACP_PORT_CONFIG";

        case TLV_T_MLACP_PORT_PRIORITY:
            return "TLV_T_MLACP_PORT_PRIORITY";

        case TLV_T_MLACP_PORT_STATE:
            return "TLV_T_MLACP_PORT_STATE";

        case TLV_T_MLACP_AGGREGATOR_CONFIG:
            return "TLV_T_MLACP_AGGREGATOR_CONFIG";

        case TLV_T_MLACP_AGGREGATOR_STATE:
            return "TLV_T_MLACP_AGGREGATOR_STATE";

        case TLV_T_MLACP_SYNC_REQUEST:
            return "TLV_T_MLACP_SYNC_REQUEST";

        case TLV_T_MLACP_SYNC_DATA:
            return "TLV_T_MLACP_SYNC_DATA";

        case TLV_T_MLACP_HEARTBEAT:
            return "TLV_T_MLACP_HEARTBEAT";

        case TLV_T_MLACP_DISCONNECT_CAUSE:
            return "TLV_T_MLACP_DISCONNECT_CAUSE";

        /* NOS Feature */
        case TLV_T_MLACP_ORPHAN_PORT:
            return "TLV_T_MLACP_ORPHAN_PORT";

        case TLV_T_MLACP_PORT_CHANNEL_INFO:
            return "TLV_T_MLACP_PORT_CHANNEL_INFO";

        case TLV_T_MLACP_PEERLINK_INFO:
            return "TLV_T_MLACP_PEERLINK_INFO";

        case TLV_T_MLACP_ARP_INFO:
            return "TLV_T_MLACP_ARP_INFO";

        case TLV_T_MLACP_MAC_INFO:
            return "TLV_T_MLACP_MAC_INFO";

        case TLV_T_MLACP_STP_INFO:
            return "TLV_T_MLACP_STP_INFO";

        case TLV_T_MLACP_IF_UP_ACK:
            return "TLV_T_MLACP_IF_UP_ACK";
    }

    return "UNKNOWN";
}

/*
 * RFC 7275
 * 12.4.  Status Code Name Space
 * [Page 81]
 */
#define STATUS_CODE_U_ICCP_RG                           0x00010001
#define STATUS_CODE_ICCP_CONNECTION_COUNT_EXCEEDED      0x00010002
#define STATUS_CODE_ICCP_APP_CONNECTION_COUNT_EXCEEDED  0x00010003
#define STATUS_CODE_ICCP_APP_NOT_IN_RG                  0x00010004
#define STATUS_CODE_INCOMPATIBLE_ICCP_PROTOCOL_VER      0x00010005
#define STATUS_CODE_ICCP_REJECTED_MSG                   0x00010006
#define STATUS_CODE_ICCP_ADMINISTRATIVELY_DISABLED      0x00010007
#define STATUS_CODE_ICCP_RG_REMOVED                     0x00010010
#define STATUS_CODE_ICCP_APP_REMOVED_FROM_RG            0x00010011


/* Debug */
static char* get_status_string(int status)
{
    switch (status)
    {
        case STATUS_CODE_U_ICCP_RG:
            return "Unknown ICCP RG";

        case STATUS_CODE_ICCP_CONNECTION_COUNT_EXCEEDED:
            return "ICCP Connection Count Exceeded";

        case STATUS_CODE_ICCP_APP_CONNECTION_COUNT_EXCEEDED:
            return "ICCP Application Connection Count Exceede";

        case STATUS_CODE_ICCP_APP_NOT_IN_RG:
            return "ICCP Application not in RG";

        case STATUS_CODE_INCOMPATIBLE_ICCP_PROTOCOL_VER:
            return "Incompatible ICCP Protocol Version";

        case STATUS_CODE_ICCP_REJECTED_MSG:
            return "ICCP Rejected Message";

        case STATUS_CODE_ICCP_ADMINISTRATIVELY_DISABLED:
            return "ICCP Administratively Disabled";

        case STATUS_CODE_ICCP_RG_REMOVED:
            return "ICCP RG Removed";

        case STATUS_CODE_ICCP_APP_REMOVED_FROM_RG:
            return "ICCP Application Removed from RG";
    }

    return "UNKNOWN";
}
/*
 * RFC 5036
 * 3.5.  LDP Messages
 * [Page 44]
 */
struct LDPHdr
{
#if __BYTE_ORDER == __BIG_ENDIAN
    uint16_t u_bit : 1;
    uint16_t msg_type : 15;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    uint16_t msg_type : 15;
    uint16_t u_bit : 1;
#endif
    uint16_t msg_len;
    uint32_t msg_id;
} __attribute__ ((packed));

typedef struct LDPHdr LDPHdr;

/*
 * RFC 7275
 * 6.1.1.  ICC Header
 * [Page 24]
 */
struct ICCRGIDTLV
{
    uint16_t type;
    uint16_t len;
    uint32_t icc_rg_id;
} __attribute__ ((packed));

typedef struct ICCRGIDTLV ICCRGIDTLV;

struct ICCHdr
{
    LDPHdr ldp_hdr;
    ICCRGIDTLV icc_rg_id_tlv;
} __attribute__ ((packed));

typedef struct ICCHdr ICCHdr;

/*
 * RFC 7275
 * 6.1.2.  ICC Parameter Encoding
 * [Page 26]
 */
struct ICCParameter
{
#if __BYTE_ORDER == __BIG_ENDIAN
    uint16_t u_bit : 1;
    uint16_t f_bit : 1;
    uint16_t type : 14;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    uint16_t type : 14;
    uint16_t f_bit : 1;
    uint16_t u_bit : 1;
#endif
    uint16_t len;
} __attribute__ ((packed));

typedef struct ICCParameter ICCParameter;

/*
 * RFC 7275
 * 6.2.1.  ICC Sender Name TLV
 * [Page 28]
 */
struct ICCSenderNameTLV
{
    ICCParameter icc_parameter;
    char sender_name[MAX_L_ICC_SENDER_NAME];
} __attribute__ ((packed));

typedef struct ICCSenderNameTLV ICCSenderNameTLV;

/*
 * RFC 7275
 * 6.3.  RG Disconnect Message
 * [Page 29]
 */
struct DisconnectCodeTLV
{
    ICCParameter icc_parameter;
    uint32_t iccp_status_code;
} __attribute__ ((packed));

typedef struct DisconnectCodeTLV DisconnectCodeTLV;

/*
 * RFC 7275
 * 6.4.1.  Notification Message TLVs
 * [Page 32]
 */
struct NAKTLV
{
    ICCParameter icc_parameter;
    uint32_t iccp_status_code;
    uint32_t rejected_msg_id;
} __attribute__ ((packed));

typedef struct NAKTLV NAKTLV;

/*
 * RFC 7275
 * 6.4.1.  Notification Message TLVs
 * [Page 34]
 */
struct RequestedProtocolVerTLV
{
    ICCParameter icc_parameter;
    uint16_t connection_ref;
    uint16_t requested_ver;
} __attribute__ ((packed));

typedef struct RequestedProtocolVerTLV RequestedProtocolVerTLV;

/*
 * RFC 7275
 * 8.  LDP Capability Negotiation
 * [Page 65]
 */
struct LDPICCPCapabilityTLV
{
    ICCParameter icc_parameter;
#if __BYTE_ORDER == __BIG_ENDIAN
    uint16_t s_bit : 1;
    uint16_t reserved : 15;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    uint16_t reserved : 15;
    uint16_t s_bit : 1;
#endif
    uint8_t major_ver;
    uint8_t minior_ver;
} __attribute__ ((packed));

typedef struct LDPICCPCapabilityTLV LDPICCPCapabilityTLV;

/*
 * RFC 7275
 * 7.2.1.  mLACP Connect TLV
 * [Page 47]
 */
struct AppConnectTLV
{
    ICCParameter icc_parameter;
    uint16_t protocol_version;
#if __BYTE_ORDER == __BIG_ENDIAN
    uint16_t a_bit : 1;
    uint16_t reserved : 15;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    uint16_t reserved : 15;
    uint16_t a_bit : 1;
#endif

    /* Optional Sub-TLVs */
    /* No optional sub-TLVs in this version */
} __attribute__ ((packed));

typedef struct AppConnectTLV AppConnectTLV;

/*
 * RFC 7275
 * 7.2.2.  mLACP Disconnect TLV
 * [Page 48]
 */
struct AppDisconnectTLV
{
    ICCParameter icc_parameter;

    /* Optional Sub-TLVs */
    /*   mLACP Disconnect Cause TLV */
} __attribute__ ((packed));

typedef struct AppDisconnectTLV AppDisconnectTLV;

/*
 * RFC 7275
 * 7.2.2.1.  mLACP Disconnect Cause TLV
 * [Page 49]
 */
struct AppDisconnectCauseTLV
{
    ICCParameter iccp_parameter;

    /* Disconnect Cause String */
    char cause_string[0];    /* Trick */
} __attribute__ ((packed));

/*syncd send msg type to iccpd*/
typedef enum mclag_syncd_msg_type_e_
{
    MCLAG_SYNCD_MSG_TYPE_NONE                   = 0,
    MCLAG_SYNCD_MSG_TYPE_FDB_OPERATION          = 1,
    MCLAG_SYNCD_MSG_TYPE_CFG_MCLAG_DOMAIN       = 2,
    MCLAG_SYNCD_MSG_TYPE_CFG_MCLAG_IFACE        = 3,
    MCLAG_SYNCD_MSG_TYPE_VLAN_MBR_UPDATES       = 4,
    MCLAG_SYNCD_MSG_TYPE_CFG_MCLAG_UNIQUE_IP    = 5
}mclag_syncd_msg_type_e;

typedef enum mclag_msg_type_e_
{
    MCLAG_MSG_TYPE_NONE                 = 0,
    MCLAG_MSG_TYPE_PORT_ISOLATE         = 1,
    MCLAG_MSG_TYPE_PORT_MAC_LEARN_MODE  = 2,
    MCLAG_MSG_TYPE_FLUSH_FDB            = 3,
    MCLAG_MSG_TYPE_SET_MAC              = 4,
    MCLAG_MSG_TYPE_SET_FDB              = 5,
    MCLAG_MSG_TYPE_SET_TRAFFIC_DIST_ENABLE  = 6,
    MCLAG_MSG_TYPE_SET_TRAFFIC_DIST_DISABLE = 7,
    MCLAG_MSG_TYPE_SET_ICCP_STATE           = 8,
    MCLAG_MSG_TYPE_SET_ICCP_ROLE            = 9,
    MCLAG_MSG_TYPE_SET_ICCP_SYSTEM_ID       = 10,
    MCLAG_MSG_TYPE_DEL_ICCP_INFO            = 11,
    MCLAG_MSG_TYPE_SET_REMOTE_IF_STATE      = 12,
    MCLAG_MSG_TYPE_DEL_REMOTE_IF_INFO       = 13,
    MCLAG_MSG_TYPE_SET_PEER_LINK_ISOLATION  = 14,
    MCLAG_MSG_TYPE_SET_ICCP_PEER_SYSTEM_ID  = 15
}mclag_msg_type_e;


typedef enum mclag_sub_option_type_e_
{
    MCLAG_SUB_OPTION_TYPE_NONE              = 0,
    MCLAG_SUB_OPTION_TYPE_ISOLATE_SRC       = 1,
    MCLAG_SUB_OPTION_TYPE_ISOLATE_DST       = 2,
    MCLAG_SUB_OPTION_TYPE_MAC_LEARN_ENABLE  = 3,
    MCLAG_SUB_OPTION_TYPE_MAC_LEARN_DISABLE = 4,
    MCLAG_SUB_OPTION_TYPE_SET_MAC_SRC       = 5,
    MCLAG_SUB_OPTION_TYPE_SET_MAC_DST       = 6,
    MCLAG_SUB_OPTION_TYPE_MCLAG_INTF_NAME   = 7,
    MCLAG_SUB_OPTION_TYPE_MCLAG_ID          = 8,
    MCLAG_SUB_OPTION_TYPE_ICCP_ROLE         = 9,
    MCLAG_SUB_OPTION_TYPE_SYSTEM_ID         = 10,
    MCLAG_SUB_OPTION_TYPE_OPER_STATUS       = 11,
    MCLAG_SUB_OPTION_TYPE_ISOLATION_STATE   = 12,
    MCLAG_SUB_OPTION_TYPE_PEER_SYSTEM_ID    = 13
} mclag_sub_option_type_e;

enum MCLAG_DOMAIN_CFG_OP_TYPE {
    MCLAG_CFG_OPER_NONE       = 0,  //NOP
    MCLAG_CFG_OPER_ADD        = 1,  //Add domain
    MCLAG_CFG_OPER_DEL        = 2,  //Delete domain
    MCLAG_CFG_OPER_UPDATE     = 3,   //update domain
    MCLAG_CFG_OPER_ATTR_DEL   = 4   //Attribute del
};


enum MCLAG_DOMAIN_CFG_ATTR_BMAP_FLAGS {
    MCLAG_CFG_ATTR_NONE                  = 0x0,
    MCLAG_CFG_ATTR_SRC_ADDR              = 0x1,
    MCLAG_CFG_ATTR_PEER_ADDR             = 0x2,
    MCLAG_CFG_ATTR_PEER_LINK             = 0x4,
    MCLAG_CFG_ATTR_KEEPALIVE_INTERVAL    = 0x8,
    MCLAG_CFG_ATTR_SESSION_TIMEOUT       = 0x10
};

struct IccpSyncdHDr
{
    uint8_t ver;
    uint8_t type;
    uint16_t len;
};

typedef struct mclag_sub_option_hdr_t_
{

    uint8_t op_type;

    /*
     * Length of option value, not including the header.
     */
    uint16_t op_len;
    uint8_t data[];
}mclag_sub_option_hdr_t;

struct mclag_fdb_info
{
    uint8_t     mac[ETHER_ADDR_LEN];
    unsigned int vid;
    char port_name[MAX_L_PORT_NAME];
    short type;    /*dynamic or static*/
    short op_type; /*add or del*/
};

struct mclag_domain_cfg_info
{
    int op_type;/*add/del domain; add/del mclag domain */
    int domain_id;
    int keepalive_time;
    int session_timeout;
    char local_ip[INET_ADDRSTRLEN];
    char peer_ip[INET_ADDRSTRLEN];
    char peer_ifname[MAX_L_PORT_NAME];
    uint8_t  system_mac[ETHER_ADDR_LEN];
    int attr_bmap;
};

struct mclag_iface_cfg_info
{
    int op_type; /*add/del mclag iface */
    int domain_id;
    char mclag_iface[MAX_L_PORT_NAME];
};

struct mclag_unique_ip_cfg_info
{
    int op_type;/*add/del mclag unique ip iface */
    char mclag_unique_ip_ifname[MAX_L_PORT_NAME];
};

/* For storing message log: For Notification TLV */
struct MsgTypeSet
{
    uint32_t msg_id;
    uint16_t type;
    uint16_t tlv;

};

struct mclag_vlan_mbr_info
{
    int op_type;/*add/del vlan_member */
    unsigned int vid;
    char mclag_iface[MAX_L_PORT_NAME];
};

struct MsgLog
{
    struct MsgTypeSet msg[MAX_MSG_LOG_SIZE];
    uint32_t end_index;
};

#endif /* MSG_FORMAT_H_ */
