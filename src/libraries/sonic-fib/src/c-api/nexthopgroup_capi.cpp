// nexthopgroup_capi.cpp

#include "src/nexthopgroupfull.h"
#include "src/nexthopgroupfull_json.h"
#include "src/c_nexthopgroupfull.h"
#include "src/nexthopgroup_debug.h"
#include "nexthopgroup_capi.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <stdexcept>
#include <iostream>

using namespace std;

extern "C" {

const char* nexthopgroup_version(void) {
    return LIBNEXTHOPGROUP_VERSION;
}

char* nexthopgroupfull_json_from_c_nhg_multi(const struct C_NextHopGroupFull* c_nhg, uint32_t nh_grp_full_count,
                                           uint32_t depends_count, uint32_t dependents_count, bool is_recurisve)
{
    if (!c_nhg) {
        FIB_LOG(fib::LogLevel::ERROR, "Do NOT pass in an empty C_NextHopGroupFull *");
        return nullptr;
    }

    try {
        FIB_LOG(fib::LogLevel::DEBUG, "nh_grp_full_count %d, depends_count %d, dependents_count %d, is_recurisve 0x%x",
            nh_grp_full_count, depends_count, dependents_count, (uint8_t)is_recurisve);
        /* Convert C array to C++ vector */
        vector<fib::nh_grp_full> cpp_nh_grp_full_list;
        for (uint32_t i = 0; i < nh_grp_full_count; i++) {
            /* convert C nh_grp_full to C++ fib::nh_grp_full explicitly */
            fib::nh_grp_full cpp_nh = {
                c_nhg->nh_grp_full_list[i].id,
                c_nhg->nh_grp_full_list[i].weight,
                c_nhg->nh_grp_full_list[i].num_direct
            };
            cpp_nh_grp_full_list.push_back(cpp_nh);
        }
        vector<uint32_t> cpp_depends;
        for (uint32_t i = 0; i < depends_count; i++) {
            cpp_depends.push_back(c_nhg->depends[i]);
        }
        vector<uint32_t> cpp_dependents;
        for (uint32_t i = 0; i < dependents_count; i++) {
            cpp_dependents.push_back(c_nhg->dependents[i]);
        }

        fib::NextHopGroupFull* cpp_nhg = nullptr;

        if (is_recurisve) {
            // Recursive case: includes cpp_gate and type
            fib::g_addr cpp_gate = reinterpret_cast<const fib::g_addr&>(c_nhg->gate);
            cpp_nhg = new fib::NextHopGroupFull(c_nhg->id, c_nhg->key, c_nhg->nhg_flags, cpp_gate,
                                        static_cast<fib::nexthop_types_t>(c_nhg->type),
                                        cpp_nh_grp_full_list, cpp_depends, cpp_dependents);
        } else {
            // Non-recursive, multipath case
            cpp_nhg = new fib::NextHopGroupFull(c_nhg->id, c_nhg->key, c_nhg->nhg_flags,
                                        cpp_nh_grp_full_list, cpp_depends, cpp_dependents);
        }

        /* Convert C++ Obj to JSON stirng */
        char* json_str = nexthopgroup_to_json(cpp_nhg);
        FIB_LOG(fib::LogLevel::DEBUG, "json_str length %zu, str: %s", 
            json_str ? strlen(json_str) : 0, json_str ? json_str : "null");

        nexthopgroup_free(cpp_nhg);
        return json_str;

    } catch (const std::exception& e) {
        FIB_LOG(fib::LogLevel::ERROR, "nexthopgroupfull_json_from_c_nhg_multi::Converting failed: %s", e.what());
        return nullptr;
    } catch (...) {
        FIB_LOG(fib::LogLevel::ERROR, "nexthopgroupfull_json_from_c_nhg_multi::Converting failed with unknown exception");
        return nullptr;
    }
}

char* nexthopgroupfull_json_from_c_nhg_singleton(const struct C_NextHopGroupFull* c_nhg, uint32_t depends_count, uint32_t dependents_count)
{
    if (!c_nhg) {
        FIB_LOG(fib::LogLevel::ERROR, "Do NOT pass in an empty C_NextHopGroupFull *");
        return nullptr;
    }

    try {
        /* Convert C array to C++ vector */
        vector<uint32_t> cpp_depends;
        for (uint32_t i = 0; i < depends_count; i++) {
            cpp_depends.push_back(c_nhg->depends[i]);
        }
        vector<uint32_t> cpp_dependents;
        for (uint32_t i = 0; i < dependents_count; i++) {
            cpp_dependents.push_back(c_nhg->dependents[i]);
        }

        /* Almostly we do NOT have ifname in zebra, so set it as empty string */
        std::string cpp_ifname = "";

        /* Convert seg6_segs flexible array to C++ vector */
        vector<struct in6_addr> cpp_nh_segs;
        if (c_nhg->nh_srv6 && c_nhg->nh_srv6->seg6_segs) {
            for (uint8_t i = 0; i < c_nhg->nh_srv6->seg6_segs->num_segs; i++) {
                cpp_nh_segs.push_back(c_nhg->nh_srv6->seg6_segs->seg[i]);
            }
        }

        /* Convert g_addr from C type to C++ type */
        fib::g_addr cpp_gate = reinterpret_cast<const fib::g_addr&>(c_nhg->gate);
        fib::g_addr cpp_src = reinterpret_cast<const fib::g_addr&>(c_nhg->src);
        fib::g_addr cpp_rmap_src = reinterpret_cast<const fib::g_addr&>(c_nhg->rmap_src);

        /* Call NextHopGroupFull constructor(singleton) to create NextHopGroupFull object */
        /* Convert C types to C++ fib types by force */
        fib::NextHopGroupFull* cpp_nhg = new fib::NextHopGroupFull(c_nhg->id, c_nhg->key,
                                                         static_cast<fib::nexthop_types_t>(c_nhg->type),
                                                         static_cast<fib::vrf_id_t>(c_nhg->vrf_id),
                                                         static_cast<fib::ifindex_t>(c_nhg->ifindex),
                                                         cpp_ifname, cpp_depends, cpp_dependents,
                                                         static_cast<fib::lsp_types_t>(c_nhg->nh_label_type),
                                                         static_cast<fib::blackhole_type>(c_nhg->bh_type),
                                                         cpp_gate, cpp_src, cpp_rmap_src,c_nhg->weight,
                                                         c_nhg->flags, c_nhg->nhg_flags,
                                                         c_nhg->nh_srv6 != nullptr,
                                                         c_nhg->nh_srv6 && c_nhg->nh_srv6->seg6_segs != nullptr,
                                                         reinterpret_cast<const fib::nexthop_srv6*>(c_nhg->nh_srv6),
                                                         reinterpret_cast<const fib::seg6_seg_stack*>(c_nhg->nh_srv6 ? c_nhg->nh_srv6->seg6_segs : nullptr),
                                                         cpp_nh_segs);

        /* Convert C++ Obj to JSON string */
        char* json_str = nexthopgroup_to_json(cpp_nhg);
        FIB_LOG(fib::LogLevel::DEBUG, "json_str length %zu, str: %s",
            json_str ? strlen(json_str) : 0, json_str ? json_str : "null");

        nexthopgroup_free(cpp_nhg);

        return json_str;

    } catch (const std::exception& e) {
        FIB_LOG(fib::LogLevel::ERROR, "nexthopgroupfull_json_from_c_nhg_singleton::Converting failed: %s", e.what());
        return nullptr;
    } catch (...) {
        FIB_LOG(fib::LogLevel::ERROR, "nexthopgroupfull_json_from_c_nhg_singleton::Converting failed with unknown exception");
        return nullptr;
    }
}

void nexthopgroup_free(fib::NextHopGroupFull* obj)
{
    delete obj;
}

char* nexthopgroup_to_json(fib::NextHopGroupFull* obj)
{
    if (!obj) {
        return nullptr;
    }

    try {
        std::string json_str = fib::to_json_string(*obj);
        char* c_str = static_cast<char*>(std::malloc(json_str.size() + 1));
        if (c_str) {
            std::memcpy(c_str, json_str.c_str(), json_str.size() + 1);
        }
        return c_str;
    } catch (const std::exception& e) {
        FIB_LOG(fib::LogLevel::ERROR, "nexthopgroup_to_json failed: %s", e.what());
        return nullptr;
    } catch (...) {
        FIB_LOG(fib::LogLevel::ERROR, "nexthopgroup_to_json failed with unknown exception");
        return nullptr;
    }
}

// Global C callback pointer (set by FRR)
/* C callback signature matching FRR's needs */
typedef void (*fib_frr_log_fn)(int level,
                                const char *file,
                                int line,
                                const char *func,
                                const char *fmt,
                                va_list args);

static fib_frr_log_fn g_frr_cb = nullptr;

// C++ wrapper that forwards to C callback
static void frr_cpp_callback(fib::LogLevel level,
                             const char* file,
                             int line,
                             const char* func,
                             const char* format,
                             va_list args) {
    if (!g_frr_cb) return;

    // Forward directly to C callback (no copying needed – va_list is consumed once)
    g_frr_cb(static_cast<int>(level), file, line, func, format, args);
}
void fib_frr_register_callback(fib_frr_log_fn cb) {
    g_frr_cb = cb;
    if (cb) {
        // Bridge C callback → C++ API
        fib::registerLogCallback(frr_cpp_callback);
    } else {
        fib::registerLogCallback(nullptr);
    }
}

void fib_frr_set_log_level(int level) {
    // Map (0-3) to fib::LogLevel
    if (level >= 0 && level <= 3) {
        fib::setLogLevel(static_cast<fib::LogLevel>(level));
    }
}

int fib_frr_get_log_level() {
    return static_cast<int>(fib::getLogLevel());
}
} // extern "C"