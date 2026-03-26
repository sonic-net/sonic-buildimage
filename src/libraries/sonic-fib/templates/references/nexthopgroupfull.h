#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <netinet/in.h>

namespace fib {

    /* Can't be generated defintions */
    typedef std::uint32_t vrf_id_t;
    typedef signed int ifindex_t;

    union g_addr {
        struct in_addr ipv4;
        struct in6_addr ipv6;
    };

    /* Auto generated Enums */
    enum nexthop_types_t {
        NEXTHOP_TYPE_INVALID,
        NEXTHOP_TYPE_IFINDEX,
        NEXTHOP_TYPE_IPV4,
        NEXTHOP_TYPE_IPV4_IFINDEX,
        NEXTHOP_TYPE_IPV6,
        NEXTHOP_TYPE_IPV6_IFINDEX,
        NEXTHOP_TYPE_BLACKHOLE
    };
    enum lsp_types_t {
        ZEBRA_LSP_NONE,
        ZEBRA_LSP_STATIC,
        ZEBRA_LSP_LDP,
        ZEBRA_LSP_BGP,
        ZEBRA_LSP_OSPF_SR,
        ZEBRA_LSP_ISIS_SR,
        ZEBRA_LSP_SHARP,
        ZEBRA_LSP_SRTE,
        ZEBRA_LSP_EVPN
    };
    enum blackhole_type {
        BLACKHOLE_UNSPEC,
        BLACKHOLE_NULL,
        BLACKHOLE_REJECT,
        BLACKHOLE_ADMINPROHIB
    };
    enum seg6local_action_t {
        SEG6_LOCAL_ACTION_UNSPEC,
        SEG6_LOCAL_ACTION_END,
        SEG6_LOCAL_ACTION_END_X,
        SEG6_LOCAL_ACTION_END_T,
        SEG6_LOCAL_ACTION_END_DX2,
        SEG6_LOCAL_ACTION_END_DX6,
        SEG6_LOCAL_ACTION_END_DX4,
        SEG6_LOCAL_ACTION_END_DT6,
        SEG6_LOCAL_ACTION_END_DT4,
        SEG6_LOCAL_ACTION_END_B6,
        SEG6_LOCAL_ACTION_END_B6_ENCAP,
        SEG6_LOCAL_ACTION_END_BM,
        SEG6_LOCAL_ACTION_END_S,
        SEG6_LOCAL_ACTION_END_AS,
        SEG6_LOCAL_ACTION_END_AM,
        SEG6_LOCAL_ACTION_END_BPF,
        SEG6_LOCAL_ACTION_END_DT46
    };
    enum srv6_headend_behavior {
        SRV6_HEADEND_BEHAVIOR_H_INSERT,
        SRV6_HEADEND_BEHAVIOR_H_ENCAPS,
        SRV6_HEADEND_BEHAVIOR_H_ENCAPS_RED,
        SRV6_HEADEND_BEHAVIOR_H_ENCAPS_L2,
        SRV6_HEADEND_BEHAVIOR_H_ENCAPS_L2_RED
    };

    /* Auto-generated simple structs */
    struct nh_grp_full {
        std::uint32_t id;
        std::uint8_t weight;
        std::uint32_t num_direct;
    };
    struct seg6local_flavors_info {
        std::uint32_t flv_ops;
        std::uint8_t lcblock_len;
        std::uint8_t lcnode_func_len;
    };
    struct seg6local_context {
        struct in_addr nh4;
        struct in6_addr nh6;
        std::uint32_t table;
        struct seg6local_flavors_info flv;
        std::uint8_t block_len;
        std::uint8_t node_len;
        std::uint8_t function_len;
        std::uint8_t argument_len;
    };

    /* --- Special Struct: nexthop_srv6 --- */
    struct nexthop_srv6 {
        enum seg6local_action_t seg6local_action = {};
        struct seg6local_context seg6local_ctx = {};
        struct in6_addr seg6_src = {};
        struct seg6_seg_stack *seg6_segs = nullptr;
    };

    /* --- Other C-specific structs (not in schema as objects) --- */
    struct seg6_seg_stack {
        enum srv6_headend_behavior encap_behavior;
        std::uint8_t num_segs;
        struct in6_addr seg[0];
    };

    /* --- Special Struct: NextHopGroupFull --- */
    struct NextHopGroupFull {
        std::uint32_t id  = 0;
        std::uint32_t key  = 0;
        std::uint8_t weight  = 0;
        std::uint8_t flags  = 0;
        std::uint32_t nhg_flags  = 0;

    #define NEXTHOP_FLAG_ONLINK (1 << 3)
        std::string ifname  = "";
        std::vector<struct nh_grp_full> nh_grp_full_list ;
        std::vector<std::uint32_t> depends ;
        std::vector<std::uint32_t> dependents ;

        char _hash_begin[0];
        enum nexthop_types_t type  = NEXTHOP_TYPE_INVALID;
        std::uint32_t vrf_id  = 0;
        std::uint32_t ifindex  = 0;
        enum lsp_types_t nh_label_type  = ZEBRA_LSP_NONE;
        union {
            union g_addr gate;
            enum blackhole_type bh_type;
        };
        union g_addr src ;
        union g_addr rmap_src ;
        char _hash_end[0];
        struct nexthop_srv6 *nh_srv6 = nullptr;

        // Add default constructor
        NextHopGroupFull() = default;

        // Constructors
        NextHopGroupFull(std::uint32_t id_in, std::uint32_t key_in, std::uint32_t nhg_flags_in,
                         const std::vector<nh_grp_full>& nh_grp_full_list_in,
                         const std::vector<uint32_t>& depends_in,
                         const std::vector<uint32_t>& dependents_in);

        NextHopGroupFull(std::uint32_t id_in, std::uint32_t key_in, enum nexthop_types_t type_in,
                         vrf_id_t vrf_id_in, ifindex_t ifindex_in, std::string ifname_in,
                         const std::vector<uint32_t>& depends_in,
                         const std::vector<uint32_t>& dependents_in,
                         enum lsp_types_t label_type_in, enum blackhole_type bh_type_in,
                         union g_addr gateway_in, union g_addr src_in, union g_addr rmap_src_in,
                         std::uint8_t weight_in, std::uint8_t flags_in, std::uint32_t nhg_flags_in,
                         bool has_srv6, bool has_seg6_segs,
                         const struct nexthop_srv6* nh_srv6_in,
                         const struct seg6_seg_stack* nh_seg6_segs_in,
                         const std::vector<struct in6_addr>& nh_segs_in);

        // Copy assignment operator
        NextHopGroupFull& operator = (const NextHopGroupFull &other);

        // Copy constructor
        NextHopGroupFull(const NextHopGroupFull& other);

        // operator == and !=
        bool operator==(const NextHopGroupFull& other) const;
        bool operator!=(const NextHopGroupFull& other) const;

        ~NextHopGroupFull();
    };

} // namespace fib