#include "nexthopgroupfull.h"

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <cstdint>
#include "nexthopgroup_debug.h"

using namespace std;
using namespace fib;

/* Constructor for multi-path NextHopGroupFull */
NextHopGroupFull::NextHopGroupFull(std::uint32_t id_in, std::uint32_t key_in, std::uint32_t nhg_flags_in,
                const std::vector<nh_grp_full>& nh_grp_full_list_in,
                const std::vector<uint32_t>& depends_in,
                const std::vector<uint32_t>& dependents_in)
    : id(id_in), key(key_in), nhg_flags(nhg_flags_in), nh_grp_full_list(nh_grp_full_list_in), depends(depends_in), dependents(dependents_in)
{

    FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NextHopGroupFull construction started (multi-nexthop) with id: %d, key: %d, nhg_flags: %d", id_in, key_in, nhg_flags_in);
    for (size_t i = 0; i < nh_grp_full_list_in.size(); i++) {
        FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] nh_grp_full_list item %d: id= %d weight %d num_direct %d", i, nh_grp_full_list_in[i].id, static_cast<unsigned int>(nh_grp_full_list_in[i].weight), nh_grp_full_list_in[i].num_direct);
    }
    for (size_t i = 0; i < depends_in.size(); i++) {
        FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] depends item %d : %d ", i, depends_in[i]);
    }
    for (size_t i = 0; i < dependents_in.size(); i++) {
        FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] dependents item %d : %d", i, dependents_in[i]);
    }

    memset(&gate, 0, sizeof(gate));
    bh_type = BLACKHOLE_UNSPEC;
    memset(&src, 0, sizeof(src));
    memset(&rmap_src, 0, sizeof(rmap_src));

    FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NextHopGroupFull construction finished (multi-nexthop) for id: %d", id_in);
}

/* Constructor for singleton NextHopGroupFull */
NextHopGroupFull::NextHopGroupFull(std::uint32_t id_in, std::uint32_t key_in,
                enum nexthop_types_t type_in, vrf_id_t vrf_id_in, ifindex_t ifindex_in,
                std::string ifname_in, const std::vector<uint32_t>& depends_in,
                const std::vector<uint32_t>& dependents_in,
                enum lsp_types_t label_type_in, enum blackhole_type bh_type_in,
                union g_addr gateway_in, union g_addr src_in, union g_addr rmap_src_in,
                std::uint8_t weight_in, std::uint8_t flags_in, std::uint32_t nhg_flags_in,
                bool has_srv6, bool has_seg6_segs,
                const struct nexthop_srv6* nh_srv6_in,
                const struct seg6_seg_stack* nh_seg6_segs_in,
                const std::vector<struct in6_addr>& nh_segs_in)
    : id(id_in), key(key_in), weight(weight_in), flags(flags_in), nhg_flags(nhg_flags_in), ifname(ifname_in),
      depends(depends_in), dependents(dependents_in), type(type_in), vrf_id(vrf_id_in),
      ifindex(ifindex_in), nh_label_type(label_type_in), src(src_in), rmap_src(rmap_src_in),
      nh_srv6(nullptr)
{
    FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NextHopGroupFull construction started (singleton) for id: %d, key: %d, nhg_flags: %d", id_in, key_in, nhg_flags_in);

    /* Initialize union member */
    this->bh_type = bh_type_in;
    this->gate = gateway_in;

    /* Check if need to allocate the nexthop_srv6 structure */
    if (has_srv6 && nh_srv6_in != nullptr)
    {
        nh_srv6 = (struct nexthop_srv6 *)malloc(sizeof(struct nexthop_srv6));
        if (!nh_srv6) {
            FIB_LOG(fib::LogLevel::ERROR, "nh_srv6 allocation failed in NextHopGroupFull constructor, abort");
            return;
        }

        memcpy(nh_srv6, nh_srv6_in, sizeof(struct nexthop_srv6));
        FIB_LOG(fib::LogLevel::DEBUG, "NextHopGroupFull constructor finished nh_srv6 initialization");
    }
    else
        FIB_LOG(fib::LogLevel::DEBUG, "NextHopGroupFull does not have srv6 info");

    /* Check if need to allocate the seg6_seg_stack structure */
    if (has_seg6_segs && nh_seg6_segs_in != nullptr)
    {
        FIB_LOG(fib::LogLevel::DEBUG, "NextHopGroupFull has seg6_segs, allocating...");
        size_t total_size = sizeof(struct seg6_seg_stack) +
                                    nh_seg6_segs_in->num_segs * sizeof(struct in6_addr);
        nh_srv6->seg6_segs = (struct seg6_seg_stack *)malloc(total_size);
        if (!nh_srv6->seg6_segs) {
            FIB_LOG(fib::LogLevel::ERROR, "seg6_segs allocation failed in NextHopGroupFull constructor, abort");
            free(nh_srv6);
            nh_srv6 = nullptr;
            return;
        }

        memcpy(nh_srv6->seg6_segs, nh_seg6_segs_in, sizeof(struct seg6_seg_stack));

        if (nh_srv6->seg6_segs->num_segs > 0)
        {
            for (size_t i = 0; i < nh_srv6->seg6_segs->num_segs; i++)
            {
                if (i < nh_segs_in.size())
                    memcpy(&nh_srv6->seg6_segs->seg[i], &nh_segs_in[i], sizeof(struct in6_addr));
                else
                {
                    memset(&nh_srv6->seg6_segs->seg[i], 0, sizeof(in6_addr));
                    FIB_LOG(fib::LogLevel::DEBUG, "Size between num_segs and vector segs is not matching, num_segs %d, vector size %zu",
                                nh_srv6->seg6_segs->num_segs, nh_segs_in.size());
                }
            }
        }
    }
    FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NextHopGroupFull construction finished (singleton) for id: %d",  id_in);
}

/* Copy Constructor */
NextHopGroupFull::NextHopGroupFull(const NextHopGroupFull& other)
      : id(other.id), key(other.key), weight(other.weight), flags(other.flags), nhg_flags(other.nhg_flags),
        ifname(other.ifname), nh_grp_full_list(other.nh_grp_full_list),
        depends(other.depends), dependents(other.dependents),
        type(other.type), vrf_id(other.vrf_id), ifindex(other.ifindex),
        nh_label_type(other.nh_label_type), bh_type(other.bh_type)
{
    FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NextHopGroupFull copy constructor started for id: %d, key: %d, nhg_flags: %d",  other.id, other.key, other.nhg_flags);

    memcpy(&gate, &other.gate, sizeof(g_addr));
    memcpy(&src, &other.src, sizeof(g_addr));
    memcpy(&rmap_src, &other.rmap_src, sizeof(g_addr));

    /* Deep copy for nh_srv6 information */
    if (other.nh_srv6 != nullptr) {
        // allocate "nh_srv6" structure
        nh_srv6 = static_cast<struct nexthop_srv6*>(malloc(sizeof(struct nexthop_srv6)));
        if (!nh_srv6) {
            FIB_LOG(fib::LogLevel::ERROR, "Failed to allocate nh_srv6 in copy constructor");
            return;
        }

        // copy plain attributes
        nh_srv6->seg6local_action = other.nh_srv6->seg6local_action;
        nh_srv6->seg6local_ctx = other.nh_srv6->seg6local_ctx;
        nh_srv6->seg6_src = other.nh_srv6->seg6_src;

        // Deep copy "seg6_segs" structure
        if (other.nh_srv6->seg6_segs != nullptr) {
            size_t size = sizeof(seg6_seg_stack)
                            + other.nh_srv6->seg6_segs->num_segs * sizeof(in6_addr);
            nh_srv6->seg6_segs = static_cast<seg6_seg_stack*>(malloc(size));
            if (!nh_srv6->seg6_segs) {
                FIB_LOG(fib::LogLevel::ERROR, "Failed to allocate seg6_segs in copy constructor");
                free(nh_srv6);  // free the allocated nh_srv6 earlier
                nh_srv6 = nullptr;
                return;
            }
            // Copy the whole "seg6_segs" structure (including flexible array)
            memcpy(nh_srv6->seg6_segs, other.nh_srv6->seg6_segs, size);
        } else {
            nh_srv6->seg6_segs = nullptr;
        }

    FIB_LOG(fib::LogLevel::DEBUG, "NextHopGroupFull copy constructor finished");
    }
}

/* Copy Assignment Operator */
NextHopGroupFull& NextHopGroupFull::operator = (const NextHopGroupFull &other)
{
    FIB_LOG(fib::LogLevel::DEBUG,  "[CPP DEBUG] NextHopGroupFull copy assignment operator started for id: %d ", other.id,
            ", key: %x", other.key, ", nhg_flags: %x ", other.nhg_flags);

    if (this == &other)     return *this;

    id = other.id;
    key = other.key;
    weight = other.weight;
    flags = other.flags;
    nhg_flags = other.nhg_flags;

    ifname = other.ifname;

    nh_grp_full_list = other.nh_grp_full_list;
    depends = other.depends;
    dependents = other.dependents;

    type = other.type;
    vrf_id = other.vrf_id;
    ifindex = other.ifindex;
    nh_label_type = other.nh_label_type;
    bh_type = other.bh_type;

    memcpy(&gate, &other.gate, sizeof(g_addr));
    memcpy(&src, &other.src, sizeof(g_addr));
    memcpy(&rmap_src, &other.rmap_src, sizeof(g_addr));

    /* Deep copy for nh_srv6 information */
    if (nh_srv6 != nullptr) {
        delete nh_srv6;
    }
    if (other.nh_srv6 != nullptr) {
        nh_srv6 = new nexthop_srv6();

        nh_srv6->seg6local_action = other.nh_srv6->seg6local_action;
        nh_srv6->seg6local_ctx = other.nh_srv6->seg6local_ctx;
        nh_srv6->seg6_src = other.nh_srv6->seg6_src;

        /* Deep copy the flexible array in seg6_segs */
        if (other.nh_srv6->seg6_segs != nullptr) {
            size_t size = sizeof(seg6_seg_stack)
                            + other.nh_srv6->seg6_segs->num_segs * sizeof(in6_addr);
            nh_srv6->seg6_segs = static_cast<seg6_seg_stack*>(malloc(size));
            memcpy(nh_srv6->seg6_segs, other.nh_srv6->seg6_segs, size);
        } else {
            nh_srv6->seg6_segs = nullptr;
        }
    } else {
        nh_srv6 = nullptr;
    }

    FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NextHopGroupFull copy assignment operator finished for id: %d", other.id);

    return *this;
}

/* operator == */
bool NextHopGroupFull::operator==(const NextHopGroupFull& other) const
{
    FIB_LOG(fib::LogLevel::DEBUG,  "[CPP DEBUG] NextHopGroupFull operator == started for [id: %d", id, "] vs [id: %d", other.id, "]");

    /* Compare plain values */
    if (id != other.id ||
        key != other.key ||
        weight != other.weight ||
        flags != other.flags ||
        nhg_flags != other.nhg_flags ||
        ifname != other.ifname ||
        depends != other.depends ||
        dependents != other.dependents ||
        type != other.type ||
        vrf_id != other.vrf_id ||
        ifindex != other.ifindex ||
        nh_label_type != other.nh_label_type) {
            FIB_LOG(fib::LogLevel::DEBUG,  "[CPP DEBUG] NOT SAME plain values!");
            return false;
    }
    /* Compare nh_grp_full_list */
    if (nh_grp_full_list.size() != other.nh_grp_full_list.size()) {
        FIB_LOG(fib::LogLevel::DEBUG,  "[CPP DEBUG] NOT SAME size of nh_grp_full_list!");
        return false;
    }
    for (size_t i = 0; i < nh_grp_full_list.size(); ++i) {
        if (nh_grp_full_list[i].id != other.nh_grp_full_list[i].id ||
            nh_grp_full_list[i].weight != other.nh_grp_full_list[i].weight ||
            nh_grp_full_list[i].num_direct != other.nh_grp_full_list[i].num_direct) {
            FIB_LOG(fib::LogLevel::DEBUG,  "[CPP DEBUG] NOT SAME values of nh_grp_full_list!");
            return false;
        }
    }
    /* Compare gate/bh_type, depending on the nexthop type */
    if (type == NEXTHOP_TYPE_BLACKHOLE) {
        if (bh_type != other.bh_type) {
            FIB_LOG(fib::LogLevel::DEBUG,  "[CPP DEBUG] NOT SAME bh_type!");
            return false;
        }
    } else {
        if (memcmp(&gate, &other.gate, sizeof(union g_addr)) != 0) {
            FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NOT SAME gate address!");
            return false;
        }
    }
    /* Compare src and rmap_src */
    if (memcmp(&src, &other.src, sizeof(union g_addr)) != 0 ||
        memcmp(&rmap_src, &other.rmap_src, sizeof(union g_addr)) != 0) {
        FIB_LOG(fib::LogLevel::DEBUG,  "[CPP DEBUG] NOT SAME src/rmap_src!");
            return false;
    }
    /* Compare nh_srv6 */
    if ((nh_srv6 == nullptr) != (other.nh_srv6 == nullptr)) {
        FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NOT SAME nh_srv6 pointer state!");
        return false;
    }
    if (nh_srv6 != nullptr) {
        if (nh_srv6->seg6local_action != other.nh_srv6->seg6local_action) {
            FIB_LOG(fib::LogLevel::DEBUG,  "[CPP DEBUG] NOT SAME nh_srv6->seg6local_action!");
            return false;
        }
        // Compare seg6local_ctx
        if (memcmp(&nh_srv6->seg6local_ctx, &other.nh_srv6->seg6local_ctx,
                    sizeof(struct seg6local_context)) != 0) {
            FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NOT SAME nh_srv6->seg6local_ctx!");
            return false;
        }

        if (memcmp(&nh_srv6->seg6_src, &other.nh_srv6->seg6_src,
                    sizeof(struct in6_addr)) != 0) {
            FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NOT SAME nh_srv6->seg6_src!");
            return false;
        }

        // Compare seg6_segs
        if ((nh_srv6->seg6_segs == nullptr) != (other.nh_srv6->seg6_segs == nullptr)) {
            FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NOT SAME nh_srv6->seg6_segs pointer state!");
            return false;
        }
        if (nh_srv6->seg6_segs != nullptr) {
            if (nh_srv6->seg6_segs->encap_behavior != other.nh_srv6->seg6_segs->encap_behavior ||
                nh_srv6->seg6_segs->num_segs != other.nh_srv6->seg6_segs->num_segs) {
                FIB_LOG(fib::LogLevel::DEBUG,  "[CPP DEBUG] NOT SAME nh_srv6->seg6_segs plain values!");
                return false;
            }
            // Compare seg6_segs list
            for (int i = 0; i < nh_srv6->seg6_segs->num_segs; ++i) {
                if (memcmp(&nh_srv6->seg6_segs->seg[i], &other.nh_srv6->seg6_segs->seg[i],
                            sizeof(struct in6_addr)) != 0) {
                    FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NOT SAME nh_srv6->seg6_segs list!");
                    return false;
                }
            }
        }
    }

    FIB_LOG(fib::LogLevel::DEBUG,  "[CPP DEBUG] The two NextHopGroupFull are the same!");

    return true;
}

/* operator != */
bool NextHopGroupFull::operator!=(const NextHopGroupFull& other) const {
    FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NextHopGroupFull operator != started for [id: %d",
        id, "] vs [id: %d", other.id , "], calling operator ==");
    return !(*this == other);
}

/* Destructor of NextHopGroupFull */
NextHopGroupFull::~NextHopGroupFull()
{
    FIB_LOG(fib::LogLevel::DEBUG, "[CPP DEBUG] NextHopGroupFull destructor started for id: %d", id);
    if (nh_srv6 != nullptr)
    {
        if (nh_srv6->seg6_segs != nullptr)
        {
            FIB_LOG(fib::LogLevel::DEBUG, "Free seg6_segs in NextHopGroupFull Destructor");
            free(nh_srv6->seg6_segs);
        }
        FIB_LOG(fib::LogLevel::DEBUG, "Free nh_srv6 in NextHopGroupFull Destructor");
        free(nh_srv6);
    }

    FIB_LOG(fib::LogLevel::DEBUG, "NextHopGroupFull has been destroyed successfully.");
}