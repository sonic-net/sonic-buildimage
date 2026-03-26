#ifndef C_NEXTHOPGROUPFULL_H
#define C_NEXTHOPGROUPFULL_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Manual definitions */
typedef uint32_t vrf_id_t;
typedef signed int ifindex_t;

#define MULTIPATH_NUM 256
#define MAX_NHG_RECURSION 8

union C_g_addr {
    struct in_addr ipv4;
    struct in6_addr ipv6;
};

/* Auto-generated C Enums */
enum C_nexthop_types_t {
    C_NEXTHOP_TYPE_INVALID,
    C_NEXTHOP_TYPE_IFINDEX,
    C_NEXTHOP_TYPE_IPV4,
    C_NEXTHOP_TYPE_IPV4_IFINDEX,
    C_NEXTHOP_TYPE_IPV6,
    C_NEXTHOP_TYPE_IPV6_IFINDEX,
    C_NEXTHOP_TYPE_BLACKHOLE,
};
enum C_lsp_types_t {
    C_ZEBRA_LSP_NONE,
    C_ZEBRA_LSP_STATIC,
    C_ZEBRA_LSP_LDP,
    C_ZEBRA_LSP_BGP,
    C_ZEBRA_LSP_OSPF_SR,
    C_ZEBRA_LSP_ISIS_SR,
    C_ZEBRA_LSP_SHARP,
    C_ZEBRA_LSP_SRTE,
    C_ZEBRA_LSP_EVPN,
};
enum C_blackhole_type {
    C_BLACKHOLE_UNSPEC,
    C_BLACKHOLE_NULL,
    C_BLACKHOLE_REJECT,
    C_BLACKHOLE_ADMINPROHIB,
};
enum C_seg6local_action_t {
    C_SEG6_LOCAL_ACTION_UNSPEC,
    C_SEG6_LOCAL_ACTION_END,
    C_SEG6_LOCAL_ACTION_END_X,
    C_SEG6_LOCAL_ACTION_END_T,
    C_SEG6_LOCAL_ACTION_END_DX2,
    C_SEG6_LOCAL_ACTION_END_DX6,
    C_SEG6_LOCAL_ACTION_END_DX4,
    C_SEG6_LOCAL_ACTION_END_DT6,
    C_SEG6_LOCAL_ACTION_END_DT4,
    C_SEG6_LOCAL_ACTION_END_B6,
    C_SEG6_LOCAL_ACTION_END_B6_ENCAP,
    C_SEG6_LOCAL_ACTION_END_BM,
    C_SEG6_LOCAL_ACTION_END_S,
    C_SEG6_LOCAL_ACTION_END_AS,
    C_SEG6_LOCAL_ACTION_END_AM,
    C_SEG6_LOCAL_ACTION_END_BPF,
    C_SEG6_LOCAL_ACTION_END_DT46,
};
enum C_srv6_headend_behavior {
    C_SRV6_HEADEND_BEHAVIOR_H_INSERT,
    C_SRV6_HEADEND_BEHAVIOR_H_ENCAPS,
    C_SRV6_HEADEND_BEHAVIOR_H_ENCAPS_RED,
    C_SRV6_HEADEND_BEHAVIOR_H_ENCAPS_L2,
    C_SRV6_HEADEND_BEHAVIOR_H_ENCAPS_L2_RED,
};

/* Auto-generated Structs */
struct C_nh_grp_full {
    uint32_t id;
    uint8_t weight;
    uint32_t num_direct;
};
struct C_seg6local_flavors_info {
    uint32_t flv_ops;
    uint8_t lcblock_len;
    uint8_t lcnode_func_len;
};
struct C_seg6local_context {
    struct in_addr nh4;
    struct in6_addr nh6;
    uint32_t table;
    struct C_seg6local_flavors_info flv;
    uint8_t block_len;
    uint8_t node_len;
    uint8_t function_len;
    uint8_t argument_len;
};

/* --- Special Struct: nexthop_srv6 --- */
struct C_nexthop_srv6 {
    enum C_seg6local_action_t seg6local_action;
    struct C_seg6local_context seg6local_ctx;
    struct in6_addr seg6_src;
    struct C_seg6_seg_stack *seg6_segs;
};

/* --- Other C-specific structs (not in schema as objects) --- */
struct C_seg6_seg_stack {
    enum C_srv6_headend_behavior encap_behavior;
    uint8_t num_segs;
    struct in6_addr seg[0];
};

/* --- Root Struct: C_NextHopGroupFull --- */
struct C_NextHopGroupFull {
    uint32_t id;
    uint32_t key;
    uint8_t weight;
    uint8_t flags;
    uint32_t nhg_flags;

#define NEXTHOP_FLAG_ONLINK (1 << 3)
    struct C_nh_grp_full nh_grp_full_list[(MULTIPATH_NUM * MAX_NHG_RECURSION) + 1];
    uint32_t depends[MULTIPATH_NUM + 1];
    uint32_t dependents[MULTIPATH_NUM + 1];

    char _hash_begin[0];
    enum C_nexthop_types_t type;
    uint32_t vrf_id;
    uint32_t ifindex;
    enum C_lsp_types_t nh_label_type;
    union {
        union C_g_addr gate;
        enum C_blackhole_type bh_type;
    };
    union C_g_addr src;
    union C_g_addr rmap_src;
    char _hash_end[0];
    struct C_nexthop_srv6 *nh_srv6;
};

#ifdef __cplusplus
}
#endif

#endif /* C_NEXTHOPGROUPFULL_H */