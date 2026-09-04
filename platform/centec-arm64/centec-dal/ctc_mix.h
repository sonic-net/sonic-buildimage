/**
 @file ctc_mix.h

 @date 2009-10-27

 @version v2.0

The file define  chipset  independent common Macros and constant.
*/

#ifndef _CTC_MIX_H
#define _CTC_MIX_H
#ifdef __cplusplus
extern "C" {
#endif

#define CTC_MAX_LOCAL_CHIP_NUM          8             /**< Max local device number (physical chip)*/
#define CTC_MAX_LOCAL_DEV_NUM           CTC_MAX_LOCAL_CHIP_NUM             /**< Max local device number (physical chip)*/
#define CTC_MAX_LOCAL_CHIP_NUM_PP       32            /**< Max local chip number (include pp number) */
#define CTC_MAX_PHY_PORT                512           
#define CTC_MAX_VRFID_ID                255           /**< Max vrfid id */
#define CTC_MAX_LINKAGG_GROUP_NUM       64            /**< Max Linkagg group num */
#define MAX_PORT_NUM_PER_CHIP           512           /**< Max port num per chip */
#define CTC_MAX_ECPN                    256           /**< Maximum equal cost path number */
#define CTC_IP_MAX_RPF_IF               16            /**< Define max RPF interfaces that ipuc and ipmc supports*/
#define MIN_CTC_L3IF_ID                 1             /**< Min value of layer 3 interface ID */
#define MAX_CTC_L3IF_ID                 1022          /**< Max value of layer 3 interface ID */
#define CTC_MAX_APS_GROUP_NUM           2048          /**< Max aps group num */
#define CTC_DEFAULT_MAX_ECMP_NUM        16            /**< By default,Maximum equal cost path number */
#define CTC_ACL_PORT_BITMAP_MAX_NUM     128

#define CTC_ACL_PORT_CLASSID_MAX        1023          /**< Max port classid*/
#define CTC_ACL_PORT_CLASSID_MIN        1             /**< Min port classid*/

#define CTC_SCL_PORT_CLASSID_MAX        255            /**< Max port classid*/
#define CTC_SCL_PORT_CLASSID_MIN        1             /**< Min port classid*/

#define CTC_ACL_VLAN_CLASSID_MAX        255          /**< Max vlan classid*/
#define CTC_ACL_VLAN_CLASSID_MIN        1             /**< Min vlan classid*/

#define CTC_PKT_CPUMAC_LEN              18            /**<  CPUMAC Header Length: MACDA(6) + MACSA(6) + VLAN(4) + Type(2) + Reserved(2) */
#define CTC_PKT_HEADER_LEN              40            /**<  Packet Header Length */
#define CTC_LPORT_CPU                   0x10000000    /**<  cpu local phy port, refer to DRV_DATA_PATH_CPU_LPORT */
#define CTC_LPORT_OAM                   64            /**<  oam local phy port, refer to DRV_DATA_PATH_OAM_LPORT */
#define CTC_GPORT_RCPU(gchip)           (0x10000000 | (((gchip) & CTC_GCHIP_MASK) << CTC_LOCAL_PORT_LENGTH))
#define MAX_CTC_CPU_MACDA_NUM           4             /**< define cpu macda number */
#define CTC_MAX_LINKAGG_ID              0xFF          /**< define max linkagg id identified by GPORT low 8bit.*/
#define CTC_MAX_CPU_REASON_GROUP_NUM    16            /**< MAX CPU Reason number.*/
#define CTC_MAX_QNUM_PER_GROUP          8             /**< MAX Queue number in one Queue Group.*/
#define CTC_INTR_MAX_GROUP              8             /**< max number of interrupt groups/PINs/IRQs */
#define CTC_LINKAGGID_MASK              0xFF          /**< Linkagg ID mask */
#define CTC_IPUC_IP_TUNNEL_MAX_IF_NUM   8             /**< Max rpf check interface id number */
#define CTC_DATAPATH_SERDES_NUM         516
#define CTC_IPMC_RP_INTF                16             /**< Define rp interface that an ipmc group add one time*/
#define CTC_IPMC_MAX_RPF_IF             16           /**< Define max RPF interface that an ipmc group supports*/
#define CTC_CHIP_FFE_PARAM_NUM          6              /** serses ffe coefficient reg num */
#define CTC_MAX_GCHIP_CHIP_ID          0x7F        /**< Max global chip id */
#define CTC_DATAPATH_MAX_DP_NUM         2             /**< Max DP number*/

#define CTC_MLAG_ISOLATION_GROUP        31            /**<  MLAG isolation group id */

#define MAX_RESERVED_PORT               32            /**<  reserved for internal port use */
#define MAX_NORMAL_PANNEL_PORT          96            /**<  when disable extent port, support 96 normal pannel port */
#define MAX_EXTEND_PANNEL_PORT          384           /**<  when enable extent port, support 384 extend pannel port */

#define CTC_NORMAL_PANNEL_PORT_BASE0    0             /**<  Normal pannel port base*/
#define CTC_NORMAL_PANNEL_PORT_BASE1    64            /**<  Normal pannel port base*/

#define CTC_EXTEND_PANNEL_PORT_BASE0    128           /**<  Extend pannel port base*/
#define CTC_EXTEND_PANNEL_PORT_BASE1    320           /**<  Extend pannel port base*/

#define CTC_DLB_QUANT_THRD_MAX          16            /**<  max threshold number used in dlb path quality quantization */

#ifdef __cplusplus
}
#endif

#endif /* _CTC_MIX_H*/

