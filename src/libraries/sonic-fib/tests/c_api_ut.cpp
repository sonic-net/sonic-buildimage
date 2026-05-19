#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <gtest/gtest.h>
#include <arpa/inet.h>

#include <iostream>
#include <thread>

#include <unistd.h>

#include "src/c-api/nexthopgroup_capi.h"
#include "src/nexthopgroupfull.h"
#include "src/nexthopgroupfull_json.h"
#include "src/c_nexthopgroupfull.h"

using namespace std;
using fib::from_json;
using fib::to_json;


TEST(NextHopGroupFull_CAPI, multi_nexthop) {
    cout << "TEST_NextHopGroupFull_CAPI::multi_nexthop started: "  << endl;
    cout << "[DEBUG] Constructing values ..." << endl;

    /* Prepare the parameters */
    C_NextHopGroupFull c_nhg = {};

    c_nhg.id = 100;
    c_nhg.key = 1234567;
    c_nhg.nhg_flags = 1024;

    // set up nh_grp_full_list
    struct C_nh_grp_full nh1 = {};
    nh1.id = 200;
    nh1.weight = 1;
    nh1.num_direct = 0;
    c_nhg.nh_grp_full_list[0] = nh1;

    struct C_nh_grp_full nh2 = {};
    nh2.id = 300;
    nh2.weight = 1;
    nh2.num_direct = 2;
    c_nhg.nh_grp_full_list[1] = nh2;

    struct C_nh_grp_full nh3 = {};
    nh3.id = 310;
    nh3.weight = 2;
    nh3.num_direct = 0;
    c_nhg.nh_grp_full_list[2] = nh3;

    struct C_nh_grp_full nh4 = {};
    nh4.id = 320;
    nh4.weight = 2;
    nh4.num_direct = 0;
    c_nhg.nh_grp_full_list[3] = nh4;

    struct C_nh_grp_full nh5 = {};
    nh5.id = 400;
    nh5.weight = 1;
    nh5.num_direct = 0;
    c_nhg.nh_grp_full_list[4] = nh5;

    // terminate the list with id = 0
    c_nhg.nh_grp_full_list[5].id = 0;

    // set up depends
    c_nhg.depends[0] = 200;
    c_nhg.depends[1] = 300;
    c_nhg.depends[2] = 400;
    c_nhg.depends[3] = 0;

    // set up dependents
    c_nhg.dependents[0] = 500;
    c_nhg.dependents[1] = 600;
    c_nhg.dependents[2] = 0;

    /* Call c-api to convert C_NextHopGroupFull to C++ NextHopGroupFull and return JSON string */
    cout << "[DEBUG] Calling nexthopgroupfull_json_from_c_nhg_multi ..." << endl;
    char* json_str = nexthopgroupfull_json_from_c_nhg_multi(&c_nhg, 5, 3, 2, false);

    ASSERT_NE(json_str, nullptr) << "[ERROR] C-API returned nullptr";

    /* Output the generated JSON string */
    cout << "    [DEBUG] The generated JSON string is:" << endl;
    cout << "    " << json_str << endl;

    /* Parse JSON string and deserialize to C++ object */
    cout << "[DEBUG] Parsing JSON string and deserializing to C++ object ..." << endl;
    nlohmann::ordered_json j = nlohmann::json::parse(json_str);
    fib::NextHopGroupFull cpp_nhg;
    from_json(j, cpp_nhg);

    /* Verify round-trip conversion by comparing C structure and C++ object */
    cout << "[DEBUG] Verifying C_NextHopGroupFull vs C++ NextHopGroupFull ..." << endl;
    // verify basic fields
    EXPECT_EQ(cpp_nhg.id, c_nhg.id);
    EXPECT_EQ(cpp_nhg.key, c_nhg.key);
    EXPECT_EQ(cpp_nhg.nhg_flags, c_nhg.nhg_flags);
    // verify nh_grp_full_list
    EXPECT_EQ(cpp_nhg.nh_grp_full_list.size(), 5);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(cpp_nhg.nh_grp_full_list[i].id, c_nhg.nh_grp_full_list[i].id);
        EXPECT_EQ(cpp_nhg.nh_grp_full_list[i].weight, c_nhg.nh_grp_full_list[i].weight);
        EXPECT_EQ(cpp_nhg.nh_grp_full_list[i].num_direct, c_nhg.nh_grp_full_list[i].num_direct);
    }
    // verify depends
    EXPECT_EQ(cpp_nhg.depends.size(), 3);
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(cpp_nhg.depends[i], c_nhg.depends[i]);
    }
    // verify dependents
    EXPECT_EQ(cpp_nhg.dependents.size(), 2);
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(cpp_nhg.dependents[i], c_nhg.dependents[i]);
    }

    /* Clean up */
    free(json_str);

    cout << "TEST_NextHopGroupFull_CAPI::multi_nexthop finished." << endl;
}

TEST(NextHopGroupFull_CAPI, singleton) {
    cout << "TEST_NextHopGroupFull_CAPI::singleton started:" << endl;
    cout << "[DEBUG] Constructing C_NextHopGroupFull type singleton values ..." << endl;

    /* Prepare the parameters */
    C_NextHopGroupFull c_nhg = {};

    c_nhg.id = 100;
    c_nhg.key = 1234567;
    c_nhg.type = C_NEXTHOP_TYPE_IPV6;
    c_nhg.vrf_id = 101;
    c_nhg.ifindex = 101;
    c_nhg.nh_label_type = C_ZEBRA_LSP_NONE;
    c_nhg.bh_type = C_BLACKHOLE_NULL;
    c_nhg.weight = 8;
    c_nhg.flags = 8;
    c_nhg.nhg_flags = 1024;

    // set up addresses
    inet_pton(AF_INET6, "2001:db8::1", &c_nhg.gate.ipv6.s6_addr);
    inet_pton(AF_INET6, "2001:db8::2", &c_nhg.src.ipv6.s6_addr);
    inet_pton(AF_INET6, "2001:db8::3", &c_nhg.rmap_src.ipv6.s6_addr);

    // set up depends and dependents
    c_nhg.depends[0] = 200;
    c_nhg.depends[1] = 300;
    c_nhg.depends[2] = 400;
    c_nhg.depends[3] = 0;

    c_nhg.dependents[0] = 500;
    c_nhg.dependents[1] = 600;
    c_nhg.dependents[2] = 0;

    // prepare segment list
    vector<struct in6_addr> test_nh_segs(3);
    inet_pton(AF_INET6, "2001:db8:1::1", test_nh_segs[0].s6_addr);
    inet_pton(AF_INET6, "2001:db8:1::2", test_nh_segs[1].s6_addr);
    inet_pton(AF_INET6, "2001:db8:1::3", test_nh_segs[2].s6_addr);

    // prepare seg6_segs
    size_t total = sizeof(struct C_seg6_seg_stack) +
                            test_nh_segs.size() * sizeof(struct in6_addr);
    struct C_seg6_seg_stack* test_nh_seg6_segs =
        (struct C_seg6_seg_stack*)malloc(total);
    test_nh_seg6_segs->encap_behavior = C_SRV6_HEADEND_BEHAVIOR_H_ENCAPS;
    test_nh_seg6_segs->num_segs = 3;
    memcpy(test_nh_seg6_segs->seg, test_nh_segs.data(),
           test_nh_segs.size() * sizeof(in6_addr));

    // Prepare seg6local_flavors_info
    struct C_seg6local_flavors_info test_flv = {
        .flv_ops = 100,
        .lcblock_len = 20,
        .lcnode_func_len = 16
    };

    // Prepare seg6local_ctx
    struct C_seg6local_context test_seg6local_ctx = {};
    inet_pton(AF_INET, "192.168.10.1", &test_seg6local_ctx.nh4.s_addr);
    inet_pton(AF_INET6, "2001:db8::a", &test_seg6local_ctx.nh6.s6_addr);
    test_seg6local_ctx.table = 100;
    test_seg6local_ctx.block_len = 36;
    test_seg6local_ctx.node_len = 12;
    test_seg6local_ctx.function_len = 20;
    test_seg6local_ctx.argument_len = 16;
    memcpy(&test_seg6local_ctx.flv, &test_flv, sizeof(struct C_seg6local_flavors_info));

    // Prepare nh_srv6
    struct C_nexthop_srv6* test_nh_srv6 =
        (struct C_nexthop_srv6*)malloc(sizeof(struct C_nexthop_srv6));
    memset(test_nh_srv6, 0, sizeof(struct C_nexthop_srv6));
    test_nh_srv6->seg6local_action = C_SEG6_LOCAL_ACTION_END_DT6;
    memcpy(&test_nh_srv6->seg6local_ctx, &test_seg6local_ctx,
           sizeof(struct C_seg6local_context));
    test_nh_srv6->seg6_segs = test_nh_seg6_segs;

    c_nhg.nh_srv6 = test_nh_srv6;

    /* Call c-api to convert C_NextHopGroupFull to C++ NextHopGroupFull and return JSON string */
    cout << "[DEBUG] Calling nexthopgroupfull_json_from_c_nhg_singleton ..." << endl;
    char* json_str = nexthopgroupfull_json_from_c_nhg_singleton(&c_nhg, 3, 2);

    ASSERT_NE(json_str, nullptr) << "[ERROR] C-API returned nullptr";

    /* Output the generated JSON string */
    cout << "    [DEBUG] The generated JSON string is:" << endl;
    cout << "    " << json_str << endl;

    /* Parse JSON string and deserialize to C++ object */
    cout << "[DEBUG] Parsing JSON string and deserializing to C++ object ..." << endl;
    nlohmann::ordered_json j = nlohmann::json::parse(json_str);
    fib::NextHopGroupFull cpp_nhg;
    from_json(j, cpp_nhg);

    /* Verify round-trip conversion by comparing C structure and C++ object */
    cout << "[DEBUG] Verifying C_NextHopGroupFull vs C++ NextHopGroupFull ..." << endl;
    // verify basic fields
    EXPECT_EQ(cpp_nhg.id, c_nhg.id);
    EXPECT_EQ(cpp_nhg.key, c_nhg.key);
    EXPECT_EQ(cpp_nhg.type, c_nhg.type);
    EXPECT_EQ(cpp_nhg.vrf_id, c_nhg.vrf_id);
    EXPECT_EQ(cpp_nhg.ifindex, c_nhg.ifindex);
    EXPECT_EQ(cpp_nhg.weight, c_nhg.weight);
    EXPECT_EQ(cpp_nhg.flags, c_nhg.flags);
    EXPECT_EQ(cpp_nhg.nhg_flags, c_nhg.nhg_flags);
    EXPECT_EQ(cpp_nhg.nh_label_type, c_nhg.nh_label_type);
    EXPECT_EQ(cpp_nhg.bh_type, c_nhg.bh_type);

    // verify addresses
    EXPECT_EQ(memcmp(&cpp_nhg.gate.ipv6, &c_nhg.gate.ipv6, sizeof(struct in6_addr)), 0);
    EXPECT_EQ(memcmp(&cpp_nhg.src.ipv6, &c_nhg.src.ipv6, sizeof(struct in6_addr)), 0);
    EXPECT_EQ(memcmp(&cpp_nhg.rmap_src.ipv6, &c_nhg.rmap_src.ipv6, sizeof(struct in6_addr)), 0);

    // verify depends
    EXPECT_EQ(cpp_nhg.depends.size(), 3);
    EXPECT_EQ(cpp_nhg.depends[0], c_nhg.depends[0]);
    EXPECT_EQ(cpp_nhg.depends[1], c_nhg.depends[1]);
    EXPECT_EQ(cpp_nhg.depends[2], c_nhg.depends[2]);

    // verify dependents
    EXPECT_EQ(cpp_nhg.dependents.size(), 2);
    EXPECT_EQ(cpp_nhg.dependents[0], c_nhg.dependents[0]);
    EXPECT_EQ(cpp_nhg.dependents[1], c_nhg.dependents[1]);

    // verify SRv6 basic information
    ASSERT_NE(cpp_nhg.nh_srv6, nullptr) << "nh_srv6 should not be null";
    EXPECT_EQ(cpp_nhg.nh_srv6->seg6local_action, c_nhg.nh_srv6->seg6local_action);
    // verify seg6local_ctx
    EXPECT_EQ(cpp_nhg.nh_srv6->seg6local_ctx.table, c_nhg.nh_srv6->seg6local_ctx.table);
    EXPECT_EQ(cpp_nhg.nh_srv6->seg6local_ctx.block_len, c_nhg.nh_srv6->seg6local_ctx.block_len);
    EXPECT_EQ(cpp_nhg.nh_srv6->seg6local_ctx.node_len, c_nhg.nh_srv6->seg6local_ctx.node_len);
    EXPECT_EQ(cpp_nhg.nh_srv6->seg6local_ctx.function_len, c_nhg.nh_srv6->seg6local_ctx.function_len);
    EXPECT_EQ(cpp_nhg.nh_srv6->seg6local_ctx.argument_len, c_nhg.nh_srv6->seg6local_ctx.argument_len);
    // verify seg6local_ctx.flv
    EXPECT_EQ(cpp_nhg.nh_srv6->seg6local_ctx.flv.flv_ops, c_nhg.nh_srv6->seg6local_ctx.flv.flv_ops);
    EXPECT_EQ(cpp_nhg.nh_srv6->seg6local_ctx.flv.lcblock_len, c_nhg.nh_srv6->seg6local_ctx.flv.lcblock_len);
    EXPECT_EQ(cpp_nhg.nh_srv6->seg6local_ctx.flv.lcnode_func_len, c_nhg.nh_srv6->seg6local_ctx.flv.lcnode_func_len);
    // verify seg6local_ctx nh4 and nh6
    EXPECT_EQ(memcmp(&cpp_nhg.nh_srv6->seg6local_ctx.nh4, &c_nhg.nh_srv6->seg6local_ctx.nh4,
                     sizeof(struct in_addr)), 0);
    EXPECT_EQ(memcmp(&cpp_nhg.nh_srv6->seg6local_ctx.nh6, &c_nhg.nh_srv6->seg6local_ctx.nh6,
                     sizeof(struct in6_addr)), 0);
    // verify seg6_segs
    ASSERT_NE(cpp_nhg.nh_srv6->seg6_segs, nullptr) << "seg6_segs should not be null";
    EXPECT_EQ(cpp_nhg.nh_srv6->seg6_segs->encap_behavior, c_nhg.nh_srv6->seg6_segs->encap_behavior);
    EXPECT_EQ(cpp_nhg.nh_srv6->seg6_segs->num_segs, c_nhg.nh_srv6->seg6_segs->num_segs);
    // verify segment list
    for (size_t i = 0; i < cpp_nhg.nh_srv6->seg6_segs->num_segs; i++) {
        EXPECT_EQ(memcmp(&cpp_nhg.nh_srv6->seg6_segs->seg[i], &c_nhg.nh_srv6->seg6_segs->seg[i],
                         sizeof(struct in6_addr)), 0);
    }

    /* Clean up */
    free(json_str);
    free(test_nh_srv6->seg6_segs);
    free(test_nh_srv6);

    cout << "TEST_NextHopGroupFull_CAPI::singleton finished." << endl;
}