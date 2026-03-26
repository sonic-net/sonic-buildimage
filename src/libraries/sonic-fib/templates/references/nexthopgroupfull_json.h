// Auto-generated from JSON Schema. DO NOT EDIT.
#pragma once

#include "nexthopgroupfull.h"
#include <nlohmann/json.hpp>

#include <string>
#include <arpa/inet.h> // for inet_ntop / inet_pton

namespace fib {

// --- IP Conversion Helpers ---
inline std::string ipv4_to_string(const struct in_addr& addr) {
    char buf[INET_ADDRSTRLEN];
    return inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN) ? std::string(buf) : "0.0.0.0";
}
inline std::string ipv6_to_string(const struct in6_addr& addr) {
    char buf[INET6_ADDRSTRLEN];
    return inet_ntop(AF_INET6, &addr, buf, INET6_ADDRSTRLEN) ? std::string(buf) : "::";
}
inline bool string_to_ipv4(const std::string& ip, struct in_addr& out) {
    return inet_pton(AF_INET, ip.c_str(), &out) == 1;
}
inline bool string_to_ipv6(const std::string& ip, struct in6_addr& out) {
    return inet_pton(AF_INET6, ip.c_str(), &out) == 1;
}
inline std::string gaddr_to_string(const union g_addr& g, nexthop_types_t type) {
    if (type == NEXTHOP_TYPE_IPV4 || type == NEXTHOP_TYPE_IPV4_IFINDEX) {
        return ipv4_to_string(g.ipv4);
    }
    return ipv6_to_string(g.ipv6);
}
inline bool string_to_gaddr(const std::string& ip, union g_addr& g, nexthop_types_t type) {
    if (type == NEXTHOP_TYPE_IPV4 || type == NEXTHOP_TYPE_IPV4_IFINDEX) {
        return string_to_ipv4(ip, g.ipv4);
    }
    return string_to_ipv6(ip, g.ipv6);
}

// --- Enum to_json / from_json ---
inline void to_json(nlohmann::ordered_json& j, const nexthop_types_t& e) {
    static const char* names[] = {
        "NEXTHOP_TYPE_INVALID",
        "NEXTHOP_TYPE_IFINDEX",
        "NEXTHOP_TYPE_IPV4",
        "NEXTHOP_TYPE_IPV4_IFINDEX",
        "NEXTHOP_TYPE_IPV6",
        "NEXTHOP_TYPE_IPV6_IFINDEX",
        "NEXTHOP_TYPE_BLACKHOLE",
    };
    if (static_cast<int>(e) >= 0 && static_cast<int>(e) < static_cast<int>(sizeof(names)/sizeof(names[0]))) {
        j = names[static_cast<int>(e)];
    } else {
        j = "UNKNOWN";
    }
}
inline void from_json(const nlohmann::ordered_json& j, nexthop_types_t& e) {
    const std::string s = j;
    if (s == "NEXTHOP_TYPE_INVALID") { e = static_cast<nexthop_types_t>(0); return; }
    if (s == "NEXTHOP_TYPE_IFINDEX") { e = static_cast<nexthop_types_t>(1); return; }
    if (s == "NEXTHOP_TYPE_IPV4") { e = static_cast<nexthop_types_t>(2); return; }
    if (s == "NEXTHOP_TYPE_IPV4_IFINDEX") { e = static_cast<nexthop_types_t>(3); return; }
    if (s == "NEXTHOP_TYPE_IPV6") { e = static_cast<nexthop_types_t>(4); return; }
    if (s == "NEXTHOP_TYPE_IPV6_IFINDEX") { e = static_cast<nexthop_types_t>(5); return; }
    if (s == "NEXTHOP_TYPE_BLACKHOLE") { e = static_cast<nexthop_types_t>(6); return; }
    e = static_cast<nexthop_types_t>(0); // default to first (e.g., *_INVALID or *_UNSPEC)
}
inline void to_json(nlohmann::ordered_json& j, const lsp_types_t& e) {
    static const char* names[] = {
        "ZEBRA_LSP_NONE",
        "ZEBRA_LSP_STATIC",
        "ZEBRA_LSP_LDP",
        "ZEBRA_LSP_BGP",
        "ZEBRA_LSP_OSPF_SR",
        "ZEBRA_LSP_ISIS_SR",
        "ZEBRA_LSP_SHARP",
        "ZEBRA_LSP_SRTE",
        "ZEBRA_LSP_EVPN",
    };
    if (static_cast<int>(e) >= 0 && static_cast<int>(e) < static_cast<int>(sizeof(names)/sizeof(names[0]))) {
        j = names[static_cast<int>(e)];
    } else {
        j = "UNKNOWN";
    }
}
inline void from_json(const nlohmann::ordered_json& j, lsp_types_t& e) {
    const std::string s = j;
    if (s == "ZEBRA_LSP_NONE") { e = static_cast<lsp_types_t>(0); return; }
    if (s == "ZEBRA_LSP_STATIC") { e = static_cast<lsp_types_t>(1); return; }
    if (s == "ZEBRA_LSP_LDP") { e = static_cast<lsp_types_t>(2); return; }
    if (s == "ZEBRA_LSP_BGP") { e = static_cast<lsp_types_t>(3); return; }
    if (s == "ZEBRA_LSP_OSPF_SR") { e = static_cast<lsp_types_t>(4); return; }
    if (s == "ZEBRA_LSP_ISIS_SR") { e = static_cast<lsp_types_t>(5); return; }
    if (s == "ZEBRA_LSP_SHARP") { e = static_cast<lsp_types_t>(6); return; }
    if (s == "ZEBRA_LSP_SRTE") { e = static_cast<lsp_types_t>(7); return; }
    if (s == "ZEBRA_LSP_EVPN") { e = static_cast<lsp_types_t>(8); return; }
    e = static_cast<lsp_types_t>(0); // default to first (e.g., *_INVALID or *_UNSPEC)
}
inline void to_json(nlohmann::ordered_json& j, const blackhole_type& e) {
    static const char* names[] = {
        "BLACKHOLE_UNSPEC",
        "BLACKHOLE_NULL",
        "BLACKHOLE_REJECT",
        "BLACKHOLE_ADMINPROHIB",
    };
    if (static_cast<int>(e) >= 0 && static_cast<int>(e) < static_cast<int>(sizeof(names)/sizeof(names[0]))) {
        j = names[static_cast<int>(e)];
    } else {
        j = "UNKNOWN";
    }
}
inline void from_json(const nlohmann::ordered_json& j, blackhole_type& e) {
    const std::string s = j;
    if (s == "BLACKHOLE_UNSPEC") { e = static_cast<blackhole_type>(0); return; }
    if (s == "BLACKHOLE_NULL") { e = static_cast<blackhole_type>(1); return; }
    if (s == "BLACKHOLE_REJECT") { e = static_cast<blackhole_type>(2); return; }
    if (s == "BLACKHOLE_ADMINPROHIB") { e = static_cast<blackhole_type>(3); return; }
    e = static_cast<blackhole_type>(0); // default to first (e.g., *_INVALID or *_UNSPEC)
}
inline void to_json(nlohmann::ordered_json& j, const seg6local_action_t& e) {
    static const char* names[] = {
        "SEG6_LOCAL_ACTION_UNSPEC",
        "SEG6_LOCAL_ACTION_END",
        "SEG6_LOCAL_ACTION_END_X",
        "SEG6_LOCAL_ACTION_END_T",
        "SEG6_LOCAL_ACTION_END_DX2",
        "SEG6_LOCAL_ACTION_END_DX6",
        "SEG6_LOCAL_ACTION_END_DX4",
        "SEG6_LOCAL_ACTION_END_DT6",
        "SEG6_LOCAL_ACTION_END_DT4",
        "SEG6_LOCAL_ACTION_END_B6",
        "SEG6_LOCAL_ACTION_END_B6_ENCAP",
        "SEG6_LOCAL_ACTION_END_BM",
        "SEG6_LOCAL_ACTION_END_S",
        "SEG6_LOCAL_ACTION_END_AS",
        "SEG6_LOCAL_ACTION_END_AM",
        "SEG6_LOCAL_ACTION_END_BPF",
        "SEG6_LOCAL_ACTION_END_DT46",
    };
    if (static_cast<int>(e) >= 0 && static_cast<int>(e) < static_cast<int>(sizeof(names)/sizeof(names[0]))) {
        j = names[static_cast<int>(e)];
    } else {
        j = "UNKNOWN";
    }
}
inline void from_json(const nlohmann::ordered_json& j, seg6local_action_t& e) {
    const std::string s = j;
    if (s == "SEG6_LOCAL_ACTION_UNSPEC") { e = static_cast<seg6local_action_t>(0); return; }
    if (s == "SEG6_LOCAL_ACTION_END") { e = static_cast<seg6local_action_t>(1); return; }
    if (s == "SEG6_LOCAL_ACTION_END_X") { e = static_cast<seg6local_action_t>(2); return; }
    if (s == "SEG6_LOCAL_ACTION_END_T") { e = static_cast<seg6local_action_t>(3); return; }
    if (s == "SEG6_LOCAL_ACTION_END_DX2") { e = static_cast<seg6local_action_t>(4); return; }
    if (s == "SEG6_LOCAL_ACTION_END_DX6") { e = static_cast<seg6local_action_t>(5); return; }
    if (s == "SEG6_LOCAL_ACTION_END_DX4") { e = static_cast<seg6local_action_t>(6); return; }
    if (s == "SEG6_LOCAL_ACTION_END_DT6") { e = static_cast<seg6local_action_t>(7); return; }
    if (s == "SEG6_LOCAL_ACTION_END_DT4") { e = static_cast<seg6local_action_t>(8); return; }
    if (s == "SEG6_LOCAL_ACTION_END_B6") { e = static_cast<seg6local_action_t>(9); return; }
    if (s == "SEG6_LOCAL_ACTION_END_B6_ENCAP") { e = static_cast<seg6local_action_t>(10); return; }
    if (s == "SEG6_LOCAL_ACTION_END_BM") { e = static_cast<seg6local_action_t>(11); return; }
    if (s == "SEG6_LOCAL_ACTION_END_S") { e = static_cast<seg6local_action_t>(12); return; }
    if (s == "SEG6_LOCAL_ACTION_END_AS") { e = static_cast<seg6local_action_t>(13); return; }
    if (s == "SEG6_LOCAL_ACTION_END_AM") { e = static_cast<seg6local_action_t>(14); return; }
    if (s == "SEG6_LOCAL_ACTION_END_BPF") { e = static_cast<seg6local_action_t>(15); return; }
    if (s == "SEG6_LOCAL_ACTION_END_DT46") { e = static_cast<seg6local_action_t>(16); return; }
    e = static_cast<seg6local_action_t>(0); // default to first (e.g., *_INVALID or *_UNSPEC)
}
inline void to_json(nlohmann::ordered_json& j, const srv6_headend_behavior& e) {
    static const char* names[] = {
        "SRV6_HEADEND_BEHAVIOR_H_INSERT",
        "SRV6_HEADEND_BEHAVIOR_H_ENCAPS",
        "SRV6_HEADEND_BEHAVIOR_H_ENCAPS_RED",
        "SRV6_HEADEND_BEHAVIOR_H_ENCAPS_L2",
        "SRV6_HEADEND_BEHAVIOR_H_ENCAPS_L2_RED",
    };
    if (static_cast<int>(e) >= 0 && static_cast<int>(e) < static_cast<int>(sizeof(names)/sizeof(names[0]))) {
        j = names[static_cast<int>(e)];
    } else {
        j = "UNKNOWN";
    }
}
inline void from_json(const nlohmann::ordered_json& j, srv6_headend_behavior& e) {
    const std::string s = j;
    if (s == "SRV6_HEADEND_BEHAVIOR_H_INSERT") { e = static_cast<srv6_headend_behavior>(0); return; }
    if (s == "SRV6_HEADEND_BEHAVIOR_H_ENCAPS") { e = static_cast<srv6_headend_behavior>(1); return; }
    if (s == "SRV6_HEADEND_BEHAVIOR_H_ENCAPS_RED") { e = static_cast<srv6_headend_behavior>(2); return; }
    if (s == "SRV6_HEADEND_BEHAVIOR_H_ENCAPS_L2") { e = static_cast<srv6_headend_behavior>(3); return; }
    if (s == "SRV6_HEADEND_BEHAVIOR_H_ENCAPS_L2_RED") { e = static_cast<srv6_headend_behavior>(4); return; }
    e = static_cast<srv6_headend_behavior>(0); // default to first (e.g., *_INVALID or *_UNSPEC)
}

/* ======== AUTO-GEN each sub structs ======== */
// Add declaration for each generating func

// --- nh_grp_full ---
inline void to_json(nlohmann::ordered_json& j, const nh_grp_full& obj) {
    j = nlohmann::ordered_json{
        {"id", obj.id},
        {"weight", obj.weight},
        {"num_direct", obj.num_direct},
    };
}
inline void from_json(const nlohmann::ordered_json& j, nh_grp_full& obj) {
    j.at("id").get_to(obj.id);
    j.at("weight").get_to(obj.weight);
    j.at("num_direct").get_to(obj.num_direct);
}

// --- seg6local_flavors_info ---
inline void to_json(nlohmann::ordered_json& j, const seg6local_flavors_info& obj) {
    j = nlohmann::ordered_json{
        {"flv_ops", obj.flv_ops},
        {"lcblock_len", obj.lcblock_len},
        {"lcnode_func_len", obj.lcnode_func_len},
    };
}
inline void from_json(const nlohmann::ordered_json& j, seg6local_flavors_info& obj) {
    j.at("flv_ops").get_to(obj.flv_ops);
    j.at("lcblock_len").get_to(obj.lcblock_len);
    j.at("lcnode_func_len").get_to(obj.lcnode_func_len);
}

// --- seg6local_context ---
inline void to_json(nlohmann::ordered_json& j, const seg6local_context& obj) {
    j = nlohmann::ordered_json{
        {"nh4", ipv4_to_string(obj.nh4)},
        {"nh6", ipv6_to_string(obj.nh6)},
        {"table", obj.table},
        {"flv", obj.flv},
        {"block_len", obj.block_len},
        {"node_len", obj.node_len},
        {"function_len", obj.function_len},
        {"argument_len", obj.argument_len},
    };
}
inline void from_json(const nlohmann::ordered_json& j, seg6local_context& obj) {
    std::string nh4_str = j.at("nh4");
    string_to_ipv4(nh4_str, obj.nh4);
    std::string nh6_str = j.at("nh6");
    string_to_ipv6(nh6_str, obj.nh6);
    j.at("table").get_to(obj.table);
    j.at("flv").get_to(obj.flv);
    j.at("block_len").get_to(obj.block_len);
    j.at("node_len").get_to(obj.node_len);
    j.at("function_len").get_to(obj.function_len);
    j.at("argument_len").get_to(obj.argument_len);
}


// --- seg6_seg_stack ---
inline void to_json(nlohmann::ordered_json& j, const seg6_seg_stack* stack) {
    if (!stack) {
        j = nullptr;
        return;
    }
    nlohmann::ordered_json segs = nlohmann::ordered_json::array();
    for (int i = 0; i < stack->num_segs; ++i) {
        segs.push_back(ipv6_to_string(stack->seg[i]));
    }
    j = nlohmann::ordered_json{
        {"encap_behavior", stack->encap_behavior},
        {"num_segs", stack->num_segs},
        {"segs", segs}
    };
}
inline void from_json(const nlohmann::ordered_json& j, seg6_seg_stack*& stack) {
    if (j.is_null()) {
        stack = nullptr;
        return;
    }
    auto behavior = j.at("encap_behavior").get<srv6_headend_behavior>();
    auto segs = j.at("segs").get<std::vector<std::string>>();
    size_t total = sizeof(seg6_seg_stack) + segs.size() * sizeof(in6_addr);
    stack = static_cast<seg6_seg_stack*>(malloc(total));
    if (!stack) return;
    stack->encap_behavior = behavior;
    stack->num_segs = static_cast<uint8_t>(segs.size());
    for (size_t i = 0; i < segs.size(); ++i) {
        string_to_ipv6(segs[i], stack->seg[i]);
    }
}

// --- nexthop_srv6 ---
inline void to_json(nlohmann::ordered_json& j, const nexthop_srv6* srv6) {
    if (!srv6) {
        j = nullptr;
        return;
    }
    j = nlohmann::ordered_json{
        {"seg6local_action", srv6->seg6local_action},
        {"seg6local_ctx", srv6->seg6local_ctx},
        {"seg6_src", ipv6_to_string(srv6->seg6_src)},
        {"seg6_segs", srv6->seg6_segs}
    };
}
inline void from_json(const nlohmann::ordered_json& j, nexthop_srv6*& srv6) {
    if (j.is_null()) {
        srv6 = nullptr;
        return;
    }
    srv6 = new nexthop_srv6{};
    j.at("seg6local_action").get_to(srv6->seg6local_action);
    j.at("seg6local_ctx").get_to(srv6->seg6local_ctx);
    j.at("seg6_segs").get_to(srv6->seg6_segs);
    std::string nh6_str = j.at("seg6_src");
    string_to_ipv6(nh6_str, srv6->seg6_src);
}

// --- NextHopGroupFull ---
inline void to_json(nlohmann::ordered_json& j, const NextHopGroupFull& nh) {
    j = nlohmann::ordered_json{
        {"id", nh.id},
        {"key", nh.key},
        {"weight", nh.weight},
        {"flags", nh.flags},
        {"nhg_flags", nh.nhg_flags},
        {"ifname", nh.ifname},
        {"nh_grp_full_list", nh.nh_grp_full_list},
        {"depends", nh.depends},
        {"dependents", nh.dependents},
        {"type", nh.type},
        {"vrf_id", nh.vrf_id},
        {"ifindex", nh.ifindex},
        {"nh_label_type", nh.nh_label_type},
        {"gate", gaddr_to_string(nh.gate, nh.type)},
        {"src", gaddr_to_string(nh.src, nh.type)},
        {"rmap_src", gaddr_to_string(nh.rmap_src, nh.type)},
    };

    // deal with the bh_type
    if (nh.type == NEXTHOP_TYPE_BLACKHOLE) {
        j["bh_type"] = nh.bh_type;
    }

    // deal with the nh_srv6
    if (nh.nh_srv6) {
        j["nh_srv6"] = nh.nh_srv6;
    } else {
        j["nh_srv6"] = nullptr;
    }
}
inline void from_json(const nlohmann::ordered_json& j, NextHopGroupFull& nh) {
    j.at("id").get_to(nh.id);
    j.at("key").get_to(nh.key);
    j.at("weight").get_to(nh.weight);
    j.at("flags").get_to(nh.flags);
    j.at("nhg_flags").get_to(nh.nhg_flags);
    j.at("ifname").get_to(nh.ifname);
    j.at("nh_grp_full_list").get_to(nh.nh_grp_full_list);
    j.at("depends").get_to(nh.depends);
    j.at("dependents").get_to(nh.dependents);
    j.at("type").get_to(nh.type);
    j.at("vrf_id").get_to(nh.vrf_id);
    j.at("ifindex").get_to(nh.ifindex);
    j.at("nh_label_type").get_to(nh.nh_label_type);
    std::string gate_str = j.at("gate");
    string_to_gaddr(gate_str, nh.gate, nh.type);
    if (j.contains("bh_type")) {
        j.at("bh_type").get_to(nh.bh_type);
    }
    std::string src_str = j.at("src");
    string_to_gaddr(src_str, nh.src, nh.type);
    std::string rmap_src_str = j.at("rmap_src");
    string_to_gaddr(rmap_src_str, nh.rmap_src, nh.type);
    if (j.contains("nh_srv6")) {
        j.at("nh_srv6").get_to(nh.nh_srv6);
    } else {
        nh.nh_srv6 = nullptr;
    }
}

// --- Top-level string helpers ---
inline std::string to_json_string(NextHopGroupFull& obj) {
    return nlohmann::ordered_json(obj).dump();
}

inline bool from_json_string(std::string& json_str, NextHopGroupFull& out_obj) {
    try {
        auto j = nlohmann::ordered_json::parse(json_str);
        j.get_to(out_obj);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace fib