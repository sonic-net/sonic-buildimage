#include <gtest/gtest.h>
#include <arpa/inet.h>

#include <iostream>
#include <thread>

#include <unistd.h>

#include "src/nexthopgroupfull.h"
#include "src/nexthopgroupfull_json.h"

using namespace std;
using namespace fib;

static nh_grp_full make_nh_grp_full(uint32_t id, uint8_t weight, uint32_t num_direct) {
    return {id, weight, num_direct};
}

TEST(EnumToJson, nexthop_type)
{
    cout << "TEST_EnumToJson::nexthop_type started: " << endl;
    cout << "[DEBUG] Preparing values ..." << endl;
    /* Prepare the value */
    fib::nexthop_types_t test_val = NEXTHOP_TYPE_IPV4_IFINDEX;

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json function for enum nexthop_types_t ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << std::endl;
    /* Check the value of constructed JSON */
    cout << "[DEBUG] Checking JSON string values ..." << endl;
    EXPECT_EQ(j, "NEXTHOP_TYPE_IPV4_IFINDEX");

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json function for enum nexthop_types_t ..." << endl;
    fib::nexthop_types_t parsed_val;
    fib::from_json(j, parsed_val);

    /* Check the value of struct parsed from JSON */
    cout << "[DEBUG] Checking STRUCT value parsed from JSON string ..." << endl;
    EXPECT_EQ(parsed_val, test_val);

    cout << "TEST_EnumToJson::nexthop_type finished." << endl;
}

TEST(EnumToJson, blackhole_type)
{
    cout << "TEST_EnumToJson::blackhole_type started: " << endl;
    /* Prepare the value */
    cout << "[DEBUG] Preparing values ..." << endl;
    fib::blackhole_type test_val = BLACKHOLE_REJECT;

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json function for enum blackhole_type ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << endl;
    /* Check the value of constructed JSON */
    EXPECT_EQ(j, "BLACKHOLE_REJECT");

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json function for enum blackhole_type ..." << endl;
    fib::blackhole_type parsed_val;
    fib::from_json(j, parsed_val);

    /* Check the value of struct parsed from JSON */
    cout << "[DEBUG] Checking STRUCT value parsed from JSON string ..." << endl;
    EXPECT_EQ(parsed_val, test_val);

    cout << "TEST_EnumToJson::blackhole_type finished." << endl;
}

TEST(EnumToJson, lsp_type)
{
    cout << "Test_EnumToJson::lsp_type started: " << endl;
    /* Prepare the value */
    cout << "[DEBUG] Preparing values ..." << endl;
    fib::lsp_types_t test_val = ZEBRA_LSP_BGP;

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json function for enum lsp_types_t ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << endl;
    /* Check the value of constructed JSON */
    EXPECT_EQ(j, "ZEBRA_LSP_BGP");

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json function for enum lsp_types_t ..." << endl;
    fib::lsp_types_t parsed_val;
    fib::from_json(j, parsed_val);

    /* Check the value of struct parsed from JSON */
    cout << "[DEBUG] Checking STRUCT value parsed from JSON string ..." << endl;
    EXPECT_EQ(parsed_val, test_val);

    cout << "TEST_EnumToJson::lsp_types_t finished." << endl;
}

TEST(EnumToJson, seg6local_action)
{
    cout << "Test_EnumToJson::seg6local_action started: " << endl;
    /* Prepare the value */
    cout << "[DEBUG] Preparing values ..." << endl;
    fib::seg6local_action_t test_val = SEG6_LOCAL_ACTION_END_DT46;

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json function for enum seg6local_action_t ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << endl;
    /* Check the value of constructed JSON */
    EXPECT_EQ(j, "SEG6_LOCAL_ACTION_END_DT46");

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json function for enum seg6local_action_t ..." << endl;
    fib::seg6local_action_t parsed_val;
    fib::from_json(j, parsed_val);

    /* Check the value of struct parsed from JSON */
    cout << "[DEBUG] Checking STRUCT value parsed from JSON string ..." << endl;
    EXPECT_EQ(parsed_val, test_val);

    cout << "TEST_EnumToJson::seg6local_action_t finished." << endl;
}

TEST(EnumToJson, srv6_headend_behavior)
{
    cout << "Test_EnumToJson::srv6_headend_behavior started: " << endl;
    /* Prepare the value */
    cout << "[DEBUG] Preparing value ..." << endl;
    fib::srv6_headend_behavior test_val = SRV6_HEADEND_BEHAVIOR_H_INSERT;

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json function for enum srv6_headend_behavior ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << endl;
    /* Check the value of constructed JSON */
    EXPECT_EQ(j, "SRV6_HEADEND_BEHAVIOR_H_INSERT");

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json function for enum sev6_headend_behavior ..." << endl;
    fib::srv6_headend_behavior parsed_val;
    fib::from_json(j, parsed_val);

    /* Check the value of struct parsed from JSON */
    cout << "[DEBUG] Checking STRUCT value parsed from JSON string ..." << endl;
    EXPECT_EQ(parsed_val, test_val);

    cout << "TEST_EnumToJson::srv6_headend_behavior finishd." << endl;
}


// --- Test: nh_grp_full ---
TEST(StructToFromJson, nh_grp_full)
{
    cout << "TEST_StructToFromJson::nh_grp_full started:" << endl;
    /* Prepare the value */
    cout << "[DEBUG] Preparing values ..." << endl;
    fib::nh_grp_full test_val{1001, 5, 3};

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json for nh_grp_full ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << endl;
    /* Check the values of constructed JSON */
    EXPECT_EQ(j["id"], 1001);
    EXPECT_EQ(j["weight"], 5);
    EXPECT_EQ(j["num_direct"], 3);

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json for nh_grp_full ..." << endl;
    fib::nh_grp_full parsed_val;
    fib::from_json(j, parsed_val);

    /* Check the value of nh_grp_full struct parsed from JSON */
    cout << "[DEBUG] Checking STRUCT value parsed from JSON string ..." << endl;
    EXPECT_EQ(parsed_val.id, test_val.id);
    EXPECT_EQ(parsed_val.weight, test_val.weight);
    EXPECT_EQ(parsed_val.num_direct, test_val.num_direct);

    cout << "TEST_StructToFromJson::nh_grp_full finished." << endl;
}

// --- Test: seg6local_flavors_info ---
TEST(StructToFromJson, seg6local_flavors_info)
{
    cout << "TEST_StructToFromJson::seg6local_flavors_info started:" << endl;
    /* Prepare the value */
    cout << "[DEBUG] Preparing values ..." << endl;
    fib::seg6local_flavors_info test_val{1000, 32, 16};

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json for seg6local_flavors_info ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << endl;
    /* Check the values of constructed JSON */
    EXPECT_EQ(j["flv_ops"], test_val.flv_ops);
    EXPECT_EQ(j["lcblock_len"], test_val.lcblock_len);
    EXPECT_EQ(j["lcnode_func_len"], test_val.lcnode_func_len);

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json for seg6local_flavors_info ..." << endl;
    fib::seg6local_flavors_info parsed_val;
    fib::from_json(j, parsed_val);

    /* Check the value of seg6local_flavors_info struct parsed from JSON */
    cout << "[DEBUG] Checking STRUCT value parsed from JSON string ..." << endl;
    EXPECT_EQ(parsed_val.flv_ops, test_val.flv_ops);
    EXPECT_EQ(parsed_val.lcblock_len, test_val.lcblock_len);
    EXPECT_EQ(parsed_val.lcnode_func_len, test_val.lcnode_func_len);

    cout << "TEST_StructToFromJson::seg6local_flavors_info finished." << endl;
}

// --- Helper for IP conversion in tests ---
static void set_ipv4(struct in_addr& addr, const char* ip_str) {
    ASSERT_EQ(inet_pton(AF_INET, ip_str, &addr), 1);
}
static void set_ipv6(struct in6_addr& addr, const char* ip_str) {
    ASSERT_EQ(inet_pton(AF_INET6, ip_str, &addr), 1);
}

// --- Test: seg6local_context ---
TEST(StructToFromJson, seg6local_context)
{
    cout << "TEST_StructToFromJson::seg6local_context started:" << endl;
    /* Prepare the value */
    cout << "[DEBUG] Preparing values ..." << endl;
    fib::seg6local_context test_val{};
    char* test_nh4 = "192.168.10.1";
    char* test_nh6 = "2001:db8::a";
    set_ipv4(test_val.nh4, test_nh4);
    set_ipv6(test_val.nh6, test_nh6);
    test_val.table = 100;
    test_val.flv = {1000, 32,16};
    test_val.block_len = 32;
    test_val.node_len = 16;
    test_val.function_len = 16;
    test_val.argument_len = 0;

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json for seg6local_context ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << endl;
    /* Check the values of constructed JSON */
    EXPECT_EQ(j["nh4"], test_nh4);
    EXPECT_EQ(j["nh6"], test_nh6);
    EXPECT_EQ(j["table"], test_val.table);
    EXPECT_EQ(j["flv"]["flv_ops"], test_val.flv.flv_ops);
    EXPECT_EQ(j["flv"]["lcblock_len"], test_val.flv.lcblock_len);
    EXPECT_EQ(j["flv"]["lcnode_func_len"], test_val.flv.lcnode_func_len);
    EXPECT_EQ(j["block_len"], test_val.block_len);
    EXPECT_EQ(j["node_len"], test_val.node_len);
    EXPECT_EQ(j["function_len"], test_val.function_len);
    EXPECT_EQ(j["argument_len"], test_val.argument_len);

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json for seg6local_context ..." << endl;
    fib::seg6local_context parsed_val;
    fib::from_json(j, parsed_val);

    /* Check the value of seg6local_context struct parsed from JSON */
    cout << "[DEBUG] Checking STRUCT value parsed from JSON string ..." << endl;
    char buf4[INET_ADDRSTRLEN], buf6[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET, &parsed_val.nh4, buf4, sizeof(buf4));
    inet_ntop(AF_INET6, &parsed_val.nh6, buf6, sizeof(buf6));
    EXPECT_STREQ(buf4, test_nh4);
    EXPECT_STREQ(buf6, test_nh6);
    EXPECT_EQ(parsed_val.table, test_val.table);
    EXPECT_EQ(parsed_val.flv.flv_ops, test_val.flv.flv_ops);
    EXPECT_EQ(parsed_val.flv.lcblock_len, test_val.flv.lcblock_len);
    EXPECT_EQ(parsed_val.flv.lcnode_func_len, test_val.flv.lcnode_func_len);
    EXPECT_EQ(parsed_val.block_len, test_val.block_len);
    EXPECT_EQ(parsed_val.node_len, test_val.node_len);
    EXPECT_EQ(parsed_val.function_len, test_val.function_len);
    EXPECT_EQ(parsed_val.argument_len, test_val.argument_len);

    cout << "TEST_StructToFromJson::seg6local_context finished." << endl;
}

// --- Test: seg6_seg_stack* (pointer) ---
TEST(StructToFromJson, seg6_seg_stack_ptr)
{
    cout << "TEST_StructToFromJson::seg6_seg_stack_ptr started:" << endl;
    /* Prepare the value */
    cout << "[DEBUG] Preparing values ..." << endl;
    std::vector<std::string> test_segs = {"2001:db8::1", "2001:db8::2"};
    size_t total = sizeof(fib::seg6_seg_stack) + test_segs.size() * sizeof(in6_addr);
    fib::seg6_seg_stack* test_val = static_cast<fib::seg6_seg_stack*>(malloc(total));
    ASSERT_NE(test_val, nullptr);
    test_val->encap_behavior = fib::SRV6_HEADEND_BEHAVIOR_H_ENCAPS;
    test_val->num_segs = 2;
    for (size_t i = 0; i < test_segs.size(); ++i) {
        set_ipv6(test_val->seg[i], test_segs[i].c_str());
    }

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json for seg6_seg_stack_ptr ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << endl;
    /* Check the values of constructed JSON */
    EXPECT_EQ(j["encap_behavior"], "SRV6_HEADEND_BEHAVIOR_H_ENCAPS");
    auto j_segs_out = j["segs"].get<std::vector<std::string>>();
    EXPECT_EQ(j["num_segs"], test_val->num_segs);
    EXPECT_EQ(j_segs_out.size(), 2);
    EXPECT_EQ(j_segs_out[0], "2001:db8::1");
    EXPECT_EQ(j_segs_out[1], "2001:db8::2");

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json for seg6_seg_stack_ptr ..." << endl;
    fib::seg6_seg_stack* parsed_val = nullptr;
    fib::from_json(j, parsed_val);
    ASSERT_NE(parsed_val, nullptr);

    cout << "[DEBUG] Checking parsed values ..." << endl;
    EXPECT_EQ(parsed_val->encap_behavior, test_val->encap_behavior);
    EXPECT_EQ(parsed_val->num_segs, test_val->num_segs);
    for (int i = 0; i < parsed_val->num_segs; ++i) {
        char test_buf[INET6_ADDRSTRLEN], parsed_buf[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &test_val->seg[i], test_buf, sizeof(test_buf));
        inet_ntop(AF_INET6, &parsed_val->seg[i], parsed_buf, sizeof(parsed_buf));
        EXPECT_STREQ(test_buf, parsed_buf);
    }

    // Cleanup
    free(test_val);
    test_val = nullptr;
    if (parsed_val) {
        free(parsed_val);
        parsed_val = nullptr;
    }

    cout << "TEST_StructToJson::seg6_seg_stack_ptr finished." << endl;
}

// --- Test: nexthop_srv6* (pointer) ---
TEST(StructToFromJson, nexthop_srv6_ptr)
{
    cout << "TEST_StructToFromJson::nexthop_srv6_ptr started:" << endl;
    /* Prepare the value */
    cout << "[DEBUG] Preparing values ..." << endl;
    fib::nexthop_srv6* test_val = (fib::nexthop_srv6*)malloc(sizeof(fib::nexthop_srv6));
    if (!test_val) {
        FAIL() << "Failed to allocate test_val";
        return;
    }
    test_val->seg6local_action = fib::SEG6_LOCAL_ACTION_END_DT6;
    char* test_nh4 = "192.168.10.1";
    char* test_nh6 = "2001:db8::a";
    set_ipv4(test_val->seg6local_ctx.nh4, test_nh4);
    set_ipv6(test_val->seg6local_ctx.nh6, test_nh6);
    test_val->seg6local_ctx.table = 200;
    test_val->seg6local_ctx.flv = {1000, 32, 16};
    test_val->seg6local_ctx.block_len = 48;
    test_val->seg6local_ctx.node_len = 16;
    test_val->seg6local_ctx.function_len = 16;
    test_val->seg6local_ctx.argument_len = 0;
    // Add seg6_segs
    std::vector<std::string> test_segs = {"2001:db8::1", "2001:db8::2"};
    size_t total = sizeof(fib::seg6_seg_stack) + test_segs.size() * sizeof(in6_addr);
    test_val->seg6_segs = static_cast<fib::seg6_seg_stack*>(malloc(total));
    test_val->seg6_segs->encap_behavior = fib::SRV6_HEADEND_BEHAVIOR_H_INSERT;
    test_val->seg6_segs->num_segs = 2;
    for (size_t i = 0; i < test_segs.size(); ++i) {
        set_ipv6(test_val->seg6_segs->seg[i], test_segs[i].c_str());
    }

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json for nexthop_srv6 ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << endl;
    /* Check the values of constructed JSON */
    EXPECT_EQ(j["seg6local_action"], "SEG6_LOCAL_ACTION_END_DT6");
    // seg6local_context
    EXPECT_EQ(j["seg6local_ctx"]["nh4"], "192.168.10.1");
    EXPECT_EQ(j["seg6local_ctx"]["nh6"], "2001:db8::a");
    EXPECT_EQ(j["seg6local_ctx"]["table"], test_val->seg6local_ctx.table);
        // seg6local_context.seg6local_flavors_info
        EXPECT_EQ(j["seg6local_ctx"]["flv"]["flv_ops"], test_val->seg6local_ctx.flv.flv_ops);
        EXPECT_EQ(j["seg6local_ctx"]["flv"]["lcblock_len"], test_val->seg6local_ctx.flv.lcblock_len);
        EXPECT_EQ(j["seg6local_ctx"]["flv"]["lcnode_func_len"],test_val->seg6local_ctx.flv.lcnode_func_len);
    EXPECT_EQ(j["seg6local_ctx"]["block_len"], test_val->seg6local_ctx.block_len);
    EXPECT_EQ(j["seg6local_ctx"]["node_len"], test_val->seg6local_ctx.node_len);
    EXPECT_EQ(j["seg6local_ctx"]["function_len"], test_val->seg6local_ctx.function_len);
    EXPECT_EQ(j["seg6local_ctx"]["argument_len"], test_val->seg6local_ctx.argument_len);
    // seg6_seg_stack *
    EXPECT_EQ(j["seg6_segs"]["encap_behavior"], "SRV6_HEADEND_BEHAVIOR_H_INSERT");
    EXPECT_EQ(j["seg6_segs"]["num_segs"], 2);
    auto j_segs_out = j["seg6_segs"]["segs"].get<std::vector<std::string>>();
    EXPECT_EQ(j_segs_out.size(), 2);
    EXPECT_EQ(j_segs_out[0], "2001:db8::1");
    EXPECT_EQ(j_segs_out[1], "2001:db8::2");

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json for nexthop_srv6 * ..." << endl;
    fib::nexthop_srv6 *parsed_val = nullptr;
    fib::from_json(j, parsed_val);
    ASSERT_NE(parsed_val, nullptr);

    /* Check values of STRUCT parsed from JSON */
    cout << "[DEBUG] Checking parsed values from constructed JSON ..." << endl;
    EXPECT_EQ(parsed_val->seg6local_action, test_val->seg6local_action);
    // seg6local_context
    char buf4[INET_ADDRSTRLEN], buf6[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET, &parsed_val->seg6local_ctx.nh4, buf4, sizeof(buf4));
    inet_ntop(AF_INET6, &parsed_val->seg6local_ctx.nh6, buf6, sizeof(buf6));
    EXPECT_STREQ(buf4, test_nh4);
    EXPECT_STREQ(buf6, test_nh6);
    EXPECT_EQ(parsed_val->seg6local_ctx.table, test_val->seg6local_ctx.table);
        // seg6local_context.seg6local_flavors_info
        EXPECT_EQ(parsed_val->seg6local_ctx.flv.flv_ops, test_val->seg6local_ctx.flv.flv_ops);
        EXPECT_EQ(parsed_val->seg6local_ctx.flv.lcblock_len, test_val->seg6local_ctx.flv.lcblock_len);
        EXPECT_EQ(parsed_val->seg6local_ctx.flv.lcnode_func_len, test_val->seg6local_ctx.flv.lcnode_func_len);
    EXPECT_EQ(parsed_val->seg6local_ctx.block_len, test_val->seg6local_ctx.block_len);
    EXPECT_EQ(parsed_val->seg6local_ctx.node_len, test_val->seg6local_ctx.node_len);
    EXPECT_EQ(parsed_val->seg6local_ctx.function_len, test_val->seg6local_ctx.function_len);
    EXPECT_EQ(parsed_val->seg6local_ctx.argument_len, test_val->seg6local_ctx.argument_len);
    // seg6_seg_stack *
    EXPECT_EQ(parsed_val->seg6_segs->encap_behavior, test_val->seg6_segs->encap_behavior);
    EXPECT_EQ(parsed_val->seg6_segs->num_segs, test_val->seg6_segs->num_segs);
    for (int i = 0; i < parsed_val->seg6_segs->num_segs; ++i) {
        char test_buf[INET6_ADDRSTRLEN], parsed_buf[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &test_val->seg6_segs->seg[i], test_buf, sizeof(test_buf));
        inet_ntop(AF_INET6, &parsed_val->seg6_segs->seg[i], parsed_buf, sizeof(parsed_buf));
        EXPECT_STREQ(test_buf, parsed_buf);
    }

    /* Clean up */
    if (test_val) {
        if (test_val->seg6_segs) {
            free(test_val->seg6_segs);
            test_val->seg6_segs = nullptr;
        }
        free(test_val);
        test_val = nullptr;
    }
    if (parsed_val) {
        if (parsed_val->seg6_segs) {
            free(parsed_val->seg6_segs);
            parsed_val->seg6_segs = nullptr;
        }
        free(parsed_val);
        parsed_val = nullptr;
    }

    cout << "TEST_StructToFromJson::nexthop_srv6_ptr finished." << endl;
}

TEST(StructToFromJson, NextHopGroupFull_multi_nexthop)
{
    cout << "TEST_StructToFromJson::NextHopGroupFull started:" << endl;
    /* Prepare the value */
    cout << "[DEBUG] Preparing values ..." << endl;
    uint32_t test_id = 100;
    uint32_t test_key = 1234567;
    uint32_t test_nhg_flags = 1024;
    vector<nh_grp_full> test_nh_grp_full_list = {
        make_nh_grp_full(200, 1, 0),
        make_nh_grp_full(300, 1, 2),
        make_nh_grp_full(310, 2, 0),
        make_nh_grp_full(320, 2, 0),
        make_nh_grp_full(400, 1, 0)
    };
    vector<uint32_t> test_depends = {200, 300, 400};
    vector<uint32_t> test_dependents = {500, 600};

    /* Call constructor function */
    cout << "[DEBUG] Calling NextHopGroupFull Constructor ..." << endl;
    NextHopGroupFull test_val(test_id, test_key, test_nhg_flags,
                           test_nh_grp_full_list, test_depends, test_dependents);

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json for NextHopGroupFull in multi-nexthop case ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << endl;
    /* Check the values of constructed JSON */
    EXPECT_EQ(j["id"], 100);
    EXPECT_EQ(j["key"], 1234567);
    EXPECT_EQ(j["nhg_flags"], 1024);
    for (size_t i = 0; i < test_val.nh_grp_full_list.size(); i++) {
        EXPECT_EQ(j["nh_grp_full_list"][i]["id"], test_val.nh_grp_full_list[i].id);
        EXPECT_EQ(j["nh_grp_full_list"][i]["weight"], test_val.nh_grp_full_list[i].weight);
        EXPECT_EQ(j["nh_grp_full_list"][i]["num_direct"], test_val.nh_grp_full_list[i].num_direct);
    }
    for (size_t i = 0; i < test_val.depends.size(); i++) {
        EXPECT_EQ(j["depends"][i], test_val.depends[i]);
    }
    for (size_t i = 0; i < test_val.dependents.size(); i++) {
        EXPECT_EQ(j["dependents"][i], test_val.dependents[i]);
    }

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json for NextHopGroupFull in multi-nexthop case ..." << endl;
    fib::NextHopGroupFull parsed_val;
    fib::from_json(j, parsed_val);

    /* Check values of STRUCT parsed from JSON */
    cout << "[DEBUG] Checking parsed values from constructed JSON ..." << endl;
    EXPECT_TRUE(parsed_val == test_val);

    cout << "TEST_StructToFromJson::NextHopGroupFull_multi_nexthop finished." << endl;
}

TEST(StructToFromJson, NextHopGroupFull_singleton)
{
    cout << "TEST_StructToFromJson::NextHopGroupFull_singleton started:" << endl;
    /* Prepare the values */
    uint32_t test_id = 100;
    uint32_t test_key = 1234567;
    enum nexthop_types_t test_type = NEXTHOP_TYPE_IPV6;
    vrf_id_t test_vrf_id = 101;
    ifindex_t test_ifindex = 101;
    string test_ifname = "eth101";
    vector<uint32_t> test_depends = {200, 300, 400};
    vector<uint32_t> test_dependents = {500, 600};
    enum lsp_types_t test_nh_label_type = ZEBRA_LSP_NONE;
    enum blackhole_type test_bh_type = BLACKHOLE_NULL;

    union g_addr test_gateway = {};
    union g_addr test_src = {};
    union g_addr test_rmap_src = {};
    inet_pton(AF_INET6, "2001:db8::1", &test_gateway.ipv6.s6_addr);
    inet_pton(AF_INET6, "2001:db8::2", &test_src.ipv6.s6_addr);
    inet_pton(AF_INET6, "2001:db8::3", &test_rmap_src.ipv6.s6_addr);

    uint8_t test_weight = 8;
    uint8_t test_flags = 8;
    uint32_t test_nhg_flags = 1024;
    bool test_has_srv6 = true;
    bool test_has_seg6_segs = true;

    // Prepare the segment list
    vector<struct in6_addr> test_nh_segs {
        {}, {}, {}
    };
    inet_pton(AF_INET6, "2001:db8:1::1", test_nh_segs[0].s6_addr);
    inet_pton(AF_INET6, "2001:db8:1::2", test_nh_segs[1].s6_addr);
    inet_pton(AF_INET6, "2001:db8:1::3", test_nh_segs[2].s6_addr);

    // Prepare seg6_segs
    size_t seg6_segs_size = sizeof(struct seg6_seg_stack) +
                                test_nh_segs.size() * sizeof(struct in6_addr);
    struct seg6_seg_stack* test_nh_seg6_segs = 
        (struct seg6_seg_stack*)malloc(seg6_segs_size);
    test_nh_seg6_segs->encap_behavior = SRV6_HEADEND_BEHAVIOR_H_ENCAPS;
    test_nh_seg6_segs->num_segs = 3;
    memcpy(test_nh_seg6_segs->seg, test_nh_segs.data(), test_nh_segs.size() * sizeof(in6_addr));

    //Prepare seg6local_flavors_info
    struct seg6local_flavors_info test_flv = {
        .flv_ops = 100,
        .lcblock_len = 20,
        .lcnode_func_len = 16
    };

    // Prepare seg6local_ctx
    struct seg6local_context test_seg6local_ctx = {};
    char* test_nh4 = "192.168.10.1";
    char* test_nh6 = "2001:db8::a";
    set_ipv4(test_seg6local_ctx.nh4, test_nh4);
    set_ipv6(test_seg6local_ctx.nh6, test_nh6);
    test_seg6local_ctx.table = 100;
    test_seg6local_ctx.block_len = 36;
    test_seg6local_ctx.node_len = 12;
    test_seg6local_ctx.function_len = 20;
    test_seg6local_ctx.argument_len = 16;
    memcpy(&test_seg6local_ctx.flv, &test_flv, sizeof(struct seg6local_flavors_info));

    // Prepare nh_srv6
    struct nexthop_srv6 test_nh_srv6 = {};
    test_nh_srv6.seg6local_action = SEG6_LOCAL_ACTION_END_DT6;
    memcpy(&test_nh_srv6.seg6local_ctx, &test_seg6local_ctx, sizeof(struct seg6local_context));

    /* Call constructor function */
    cout << "[DEBUG] Calling NextHopGroupFull Constructor ..." << endl;
    NextHopGroupFull test_val(test_id, test_key, test_type, test_vrf_id, test_ifindex,
                test_ifname, test_depends, test_dependents, test_nh_label_type,
                test_bh_type, test_gateway, test_src, test_rmap_src,
                test_weight, test_flags, test_nhg_flags, test_has_srv6, test_has_seg6_segs,
                &test_nh_srv6, test_nh_seg6_segs, test_nh_segs);

    // Free the memory allocated dynamically
    free(test_nh_seg6_segs);
    test_nh_seg6_segs = nullptr;

    /* Call to_json function */
    cout << "[DEBUG] Calling to_json for NextHopGroupFull in singleton case ..." << endl;
    nlohmann::ordered_json j;
    fib::to_json(j, test_val);

    /* Output the constructed JSON string */
    cout << "    [DEBUG] The constructed JSON string is:" << endl;
    cout << "    " << j.dump(4) << endl;
    /* Check the values of constructed JSON */
    cout << "[DEBUG] Checking JSON values ..." << endl;
    EXPECT_EQ(j["id"], test_val.id);
    EXPECT_EQ(j["key"], test_val.key);
    EXPECT_EQ(j["type"], test_val.type);
    EXPECT_EQ(j["vrf_id"], test_val.vrf_id);
    EXPECT_EQ(j["ifindex"], test_val.ifindex);
    for (size_t i = 0; i < test_val.depends.size(); i++) {
        EXPECT_EQ(j["depends"][i], test_val.depends[i]);
    }
    for (size_t i = 0; i < test_val.dependents.size(); i++) {
        EXPECT_EQ(j["dependents"][i], test_val.dependents[i]);
    }
    EXPECT_EQ(j["nh_label_type"], "ZEBRA_LSP_NONE");
    EXPECT_EQ(j["weight"], test_val.weight);
    EXPECT_EQ(j["flags"], test_val.flags);
    EXPECT_EQ(j["nhg_flags"], test_val.nhg_flags);
    EXPECT_EQ(j["gate"], "2001:db8::1");
    EXPECT_EQ(j["src"], "2001:db8::2");
    EXPECT_EQ(j["rmap_src"], "2001:db8::3");
    // Check SRv6 info
    // Check nh_srv6's seg6local_action
    EXPECT_EQ(j["nh_srv6"]["seg6local_action"], "SEG6_LOCAL_ACTION_END_DT6");
    // Check nh_srv6->seg6local_ctx
    EXPECT_EQ(j["nh_srv6"]["seg6local_ctx"]["table"], test_val.nh_srv6->seg6local_ctx.table);
    EXPECT_EQ(j["nh_srv6"]["seg6local_ctx"]["nh4"], test_nh4);
    EXPECT_EQ(j["nh_srv6"]["seg6local_ctx"]["nh6"], test_nh6);
        // Check nh_srv6->seg6local_ctx.flv
        EXPECT_EQ(j["nh_srv6"]["seg6local_ctx"]["flv"]["flv_ops"], test_val.nh_srv6->seg6local_ctx.flv.flv_ops);
        EXPECT_EQ(j["nh_srv6"]["seg6local_ctx"]["flv"]["lcblock_len"], test_val.nh_srv6->seg6local_ctx.flv.lcblock_len);
        EXPECT_EQ(j["nh_srv6"]["seg6local_ctx"]["flv"]["lcnode_func_len"], test_val.nh_srv6->seg6local_ctx.flv.lcnode_func_len);
    EXPECT_EQ(j["nh_srv6"]["seg6local_ctx"]["block_len"], test_val.nh_srv6->seg6local_ctx.block_len);
    EXPECT_EQ(j["nh_srv6"]["seg6local_ctx"]["node_len"], test_val.nh_srv6->seg6local_ctx.node_len);
    EXPECT_EQ(j["nh_srv6"]["seg6local_ctx"]["function_len"], test_val.nh_srv6->seg6local_ctx.function_len);
    EXPECT_EQ(j["nh_srv6"]["seg6local_ctx"]["argument_len"], test_val.nh_srv6->seg6local_ctx.argument_len);
    // Check nh_srv6->seg6_segs
    EXPECT_EQ(j["nh_srv6"]["seg6_segs"]["encap_behavior"], "SRV6_HEADEND_BEHAVIOR_H_ENCAPS");
    auto j_segs_out = j["nh_srv6"]["seg6_segs"]["segs"].get<std::vector<std::string>>();
    EXPECT_EQ(j["nh_srv6"]["seg6_segs"]["num_segs"], test_val.nh_srv6->seg6_segs->num_segs);
    EXPECT_EQ(j_segs_out.size(), 3);
    EXPECT_EQ(j_segs_out[0], "2001:db8:1::1");
    EXPECT_EQ(j_segs_out[1], "2001:db8:1::2");
    EXPECT_EQ(j_segs_out[2], "2001:db8:1::3");

    /* Call from_json function */
    cout << "[DEBUG] Calling from_json for NextHopGroupFull in singleton case ..." << endl;
    fib::NextHopGroupFull parsed_val;
    fib::from_json(j, parsed_val);

    /* Cehck values of STRUCT parsed from JSON */
    cout << "[DEBUG] Checking parsed values from constructed JSON ..." << endl;
    EXPECT_TRUE(parsed_val == test_val);

    cout << "TEST_StructToFromJson::NextHopGroupFull_singleton finished." << endl;
}