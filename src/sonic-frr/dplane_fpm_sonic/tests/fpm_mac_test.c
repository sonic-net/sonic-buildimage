/*
 *  Copyright 2026 (c) Microsoft Corporation.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @file fpm_mac_test.c
 * @brief Unit tests for FPM MAC message decoding.
 *
 * fpm_mac_decode() is the only part of the FPM MAC path that can be exercised
 * without a running zebra: everything after it mutates the EVPN tables and must
 * run on zebra's main thread. These tests build the netlink messages fpmsyncd
 * emits and check the decoded fields, including the malformed cases that would
 * otherwise install a zero MAC or a wrong VLAN.
 */

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <linux/neighbour.h>
#include <linux/nexthop.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "fpm_mac.h"

#define TEST_BUF_LEN 512

struct mac_msg {
	struct nlmsghdr n;
	struct ndmsg ndm;
	char buf[TEST_BUF_LEN];
};

static void msg_init(struct mac_msg *m, uint16_t type, uint32_t seq)
{
	memset(m, 0, sizeof(*m));
	m->n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
	m->n.nlmsg_type = type;
	m->n.nlmsg_seq = seq;
	m->ndm.ndm_family = AF_BRIDGE;
}

static void msg_add_attr(struct mac_msg *m, int type, const void *data,
			 size_t len)
{
	struct rtattr *rta;

	rta = (struct rtattr *)((char *)&m->n + NLMSG_ALIGN(m->n.nlmsg_len));
	rta->rta_type = (unsigned short)type;
	rta->rta_len = (unsigned short)RTA_LENGTH(len);
	memcpy(RTA_DATA(rta), data, len);
	m->n.nlmsg_len = NLMSG_ALIGN(m->n.nlmsg_len) + RTA_ALIGN(rta->rta_len);
}

static const uint8_t kMac[ETH_ALEN] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

static void test_decode_add(void)
{
	struct fpm_local_mac out;
	struct mac_msg m;
	uint16_t vid = 100;

	msg_init(&m, RTM_NEWNEIGH, 7);
	m.ndm.ndm_ifindex = 42;
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	msg_add_attr(&m, NDA_VLAN, &vid, sizeof(vid));

	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_EQUAL(out.ifindex, 42);
	CU_ASSERT_EQUAL(out.vid, 100);
	CU_ASSERT_EQUAL(out.generation, 7);
	CU_ASSERT_FALSE(out.del);
	CU_ASSERT_FALSE(out.sticky);
	CU_ASSERT_EQUAL(memcmp(out.mac, kMac, ETH_ALEN), 0);
}

static void test_decode_del_and_sticky(void)
{
	struct fpm_local_mac out;
	struct mac_msg m;

	msg_init(&m, RTM_DELNEIGH, 9);
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_TRUE(out.del);
	CU_ASSERT_EQUAL(out.generation, 9);

	/* Stickiness comes from NTF_STICKY in ndm_flags. */
	msg_init(&m, RTM_NEWNEIGH, 1);
	m.ndm.ndm_flags = NTF_STICKY;
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_TRUE(out.sticky);
	CU_ASSERT_FALSE(out.del);

	/* NUD_NOARP means "does not age", not "sticky". Treating it as sticky
	 * would set the EVPN sticky bit on every hardware-learnt MAC and stop
	 * those MACs from moving. */
	msg_init(&m, RTM_NEWNEIGH, 1);
	m.ndm.ndm_state = NUD_NOARP;
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_FALSE(out.sticky);

	/* The encoding actually on the wire: zebra and fpmsyncd both set
	 * NTF_STICKY and NUD_NOARP together for a sticky entry. */
	msg_init(&m, RTM_NEWNEIGH, 1);
	m.ndm.ndm_state = NUD_REACHABLE | NUD_NOARP;
	m.ndm.ndm_flags = NTF_MASTER | NTF_EXT_LEARNED | NTF_STICKY;
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_TRUE(out.sticky);
}

/* fpmsyncd marks hardware-learnt MACs with NDA_PROTOCOL = RTPROT_HW, the FPM
 * equivalent of the kernel path's "proto hw". */
static void test_decode_protocol(void)
{
	struct fpm_local_mac out;
	struct mac_msg m;
	uint8_t proto = RTPROT_HW;

	msg_init(&m, RTM_NEWNEIGH, 3);
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	msg_add_attr(&m, NDA_PROTOCOL, &proto, sizeof(proto));
	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_EQUAL(out.protocol, RTPROT_HW);

	/* Absent NDA_PROTOCOL leaves the entry unattributed. */
	msg_init(&m, RTM_NEWNEIGH, 3);
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_EQUAL(out.protocol, RTPROT_UNSPEC);
}

/* A MAC zebra originated must not be fed back to it as a local MAC. */
static void test_decode_rejects_zebra_protocol(void)
{
	struct fpm_local_mac out;
	struct mac_msg m;
	uint8_t proto = RTPROT_ZEBRA;

	msg_init(&m, RTM_NEWNEIGH, 4);
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	msg_add_attr(&m, NDA_PROTOCOL, &proto, sizeof(proto));
	CU_ASSERT_FALSE(fpm_mac_decode(&m.n, &out));
}

/* Without NDA_LLADDR there is no MAC to install; accepting the message would
 * install the all-zero MAC. */
static void test_decode_requires_lladdr(void)
{
	struct fpm_local_mac out;
	struct mac_msg m;
	uint16_t vid = 100;

	msg_init(&m, RTM_NEWNEIGH, 1);
	msg_add_attr(&m, NDA_VLAN, &vid, sizeof(vid));

	CU_ASSERT_FALSE(fpm_mac_decode(&m.n, &out));
}

/* zebra also receives IP neighbour messages on this socket. */
static void test_decode_rejects_non_bridge(void)
{
	struct fpm_local_mac out;
	struct mac_msg m;

	msg_init(&m, RTM_NEWNEIGH, 1);
	m.ndm.ndm_family = AF_INET;
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);

	CU_ASSERT_FALSE(fpm_mac_decode(&m.n, &out));
}

static void test_decode_rejects_truncated_and_other_types(void)
{
	struct fpm_local_mac out;
	struct mac_msg m;

	msg_init(&m, RTM_NEWNEIGH, 1);
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	m.n.nlmsg_len = NLMSG_LENGTH(0);
	CU_ASSERT_FALSE(fpm_mac_decode(&m.n, &out));

	msg_init(&m, RTM_NEWROUTE, 1);
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_FALSE(fpm_mac_decode(&m.n, &out));
}

/* A short NDA_VLAN must not be read as a uint16, and must not leave a stale
 * VLAN behind either. */
static void test_decode_ignores_malformed_vlan(void)
{
	struct fpm_local_mac out;
	struct mac_msg m;
	uint8_t short_vid = 5;

	msg_init(&m, RTM_NEWNEIGH, 1);
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	msg_add_attr(&m, NDA_VLAN, &short_vid, sizeof(short_vid));

	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_EQUAL(out.vid, 0);
}

/* A rejected message must not scribble on the caller's struct. */
static void test_decode_leaves_output_untouched_on_failure(void)
{
	struct fpm_local_mac out;
	struct mac_msg m;

	memset(&out, 0xAB, sizeof(out));
	msg_init(&m, RTM_NEWNEIGH, 1);
	m.ndm.ndm_family = AF_INET;

	CU_ASSERT_FALSE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_EQUAL(out.ifindex, (int)0xABABABAB);
}

/*
 * The sweep decides purely on the generation stamp, so this predicate is the
 * whole retirement rule. Getting it wrong either deletes MACs the replay just
 * refreshed or never retires the ones it did not.
 */
static void test_stale_predicate(void)
{
	/* Refreshed by the replay that just ended. */
	CU_ASSERT_FALSE(fpm_mac_is_stale(true, 7, 7));

	/* Not refreshed: an older generation, or never stamped at all. */
	CU_ASSERT_TRUE(fpm_mac_is_stale(true, 6, 7));
	CU_ASSERT_TRUE(fpm_mac_is_stale(true, 0, 1));

	/* Learnt some other way, so not ours to remove however it is stamped. */
	CU_ASSERT_FALSE(fpm_mac_is_stale(false, 6, 7));
	CU_ASSERT_FALSE(fpm_mac_is_stale(false, 0, 1));

	/* Generations are compared for equality, so a wrap is not special. */
	CU_ASSERT_TRUE(fpm_mac_is_stale(true, 0xFFFFFFFFu, 1));
	CU_ASSERT_FALSE(fpm_mac_is_stale(true, 0xFFFFFFFFu, 0xFFFFFFFFu));
}


/*
 * The encoder below is the wire contract fpmsyncd parses. NHA_FDB is the part
 * that matters most: without it the receiver hands the message to the L3 route
 * path and publishes an Ethernet Segment as a real nexthop group.
 */
static const struct rtattr *nhg_attr(const struct nlmsghdr *n, int type)
{
	const struct nhmsg *nhm = (const struct nhmsg *)NLMSG_DATA(n);
	int len = (int)(n->nlmsg_len - NLMSG_LENGTH(sizeof(*nhm)));
	struct rtattr *rta =
		(struct rtattr *)(void *)((char *)nhm + NLMSG_ALIGN(sizeof(*nhm)));

	for (; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
		if (rta->rta_type == type)
			return rta;
	}

	return NULL;
}

static void test_fdb_nh_encode_vtep(void)
{
	struct fpm_fdb_nh nh;
	const struct nlmsghdr *n;
	const struct rtattr *rta;
	uint8_t buf[512];
	size_t len;

	memset(&nh, 0, sizeof(nh));
	nh.id = 4001;
	nh.family = AF_INET;
	nh.addr[0] = 10;
	nh.addr[1] = 1;
	nh.addr[2] = 1;
	nh.addr[3] = 1;

	len = fpm_fdb_nh_encode(&nh, buf, sizeof(buf));
	CU_ASSERT_NOT_EQUAL(len, 0);

	n = (const struct nlmsghdr *)buf;
	CU_ASSERT_EQUAL(n->nlmsg_type, RTM_NEWNEXTHOP);
	CU_ASSERT_TRUE((n->nlmsg_flags & NLM_F_CREATE) != 0);
	CU_ASSERT_EQUAL(((const struct nhmsg *)NLMSG_DATA(n))->nh_family, AF_INET);

	CU_ASSERT_PTR_NOT_NULL(nhg_attr(n, NHA_FDB));
	CU_ASSERT_PTR_NULL(nhg_attr(n, NHA_GROUP));

	rta = nhg_attr(n, NHA_ID);
	CU_ASSERT_PTR_NOT_NULL_FATAL(rta);
	CU_ASSERT_EQUAL(*(const uint32_t *)RTA_DATA(rta), 4001u);

	rta = nhg_attr(n, NHA_GATEWAY);
	CU_ASSERT_PTR_NOT_NULL_FATAL(rta);
	CU_ASSERT_EQUAL(RTA_PAYLOAD(rta), 4);
	CU_ASSERT_EQUAL(((const uint8_t *)RTA_DATA(rta))[0], 10);
}

static void test_fdb_nh_encode_vtep_v6(void)
{
	struct fpm_fdb_nh nh;
	const struct nlmsghdr *n;
	const struct rtattr *rta;
	uint8_t buf[512];

	memset(&nh, 0, sizeof(nh));
	nh.id = 4002;
	nh.family = AF_INET6;
	nh.addr[0] = 0x20;
	nh.addr[1] = 0x01;

	CU_ASSERT_NOT_EQUAL(fpm_fdb_nh_encode(&nh, buf, sizeof(buf)), 0);

	n = (const struct nlmsghdr *)buf;
	CU_ASSERT_EQUAL(((const struct nhmsg *)NLMSG_DATA(n))->nh_family, AF_INET6);

	rta = nhg_attr(n, NHA_GATEWAY);
	CU_ASSERT_PTR_NOT_NULL_FATAL(rta);
	CU_ASSERT_EQUAL(RTA_PAYLOAD(rta), 16);
}

static void test_fdb_nh_encode_group(void)
{
	const uint32_t members[] = {4001, 4002, 4003};
	struct fpm_fdb_nh nh;
	const struct nlmsghdr *n;
	const struct rtattr *rta;
	const struct nexthop_grp *grp;
	uint8_t buf[512];

	memset(&nh, 0, sizeof(nh));
	nh.id = 5000;
	nh.member_cnt = 3;
	nh.members = members;

	CU_ASSERT_NOT_EQUAL(fpm_fdb_nh_encode(&nh, buf, sizeof(buf)), 0);

	n = (const struct nlmsghdr *)buf;
	CU_ASSERT_EQUAL(((const struct nhmsg *)NLMSG_DATA(n))->nh_family, AF_UNSPEC);
	CU_ASSERT_PTR_NOT_NULL(nhg_attr(n, NHA_FDB));
	CU_ASSERT_PTR_NULL(nhg_attr(n, NHA_GATEWAY));

	rta = nhg_attr(n, NHA_GROUP);
	CU_ASSERT_PTR_NOT_NULL_FATAL(rta);
	CU_ASSERT_EQUAL(RTA_PAYLOAD(rta) / sizeof(struct nexthop_grp), 3);

	grp = (const struct nexthop_grp *)RTA_DATA(rta);
	CU_ASSERT_EQUAL(grp[0].id, 4001u);
	CU_ASSERT_EQUAL(grp[1].id, 4002u);
	CU_ASSERT_EQUAL(grp[2].id, 4003u);
}

/* A delete still has to be recognisable as an FDB nexthop, or the receiver
 * sends it to the route path and deletes an unrelated L3 group. */
static void test_fdb_nh_encode_delete(void)
{
	struct fpm_fdb_nh nh;
	const struct nlmsghdr *n;
	uint8_t buf[512];

	memset(&nh, 0, sizeof(nh));
	nh.id = 4001;
	nh.del = true;

	CU_ASSERT_NOT_EQUAL(fpm_fdb_nh_encode(&nh, buf, sizeof(buf)), 0);

	n = (const struct nlmsghdr *)buf;
	CU_ASSERT_EQUAL(n->nlmsg_type, RTM_DELNEXTHOP);
	CU_ASSERT_TRUE((n->nlmsg_flags & NLM_F_CREATE) == 0);
	CU_ASSERT_PTR_NOT_NULL(nhg_attr(n, NHA_ID));
	CU_ASSERT_PTR_NOT_NULL(nhg_attr(n, NHA_FDB));
	CU_ASSERT_PTR_NULL(nhg_attr(n, NHA_GATEWAY));
	CU_ASSERT_PTR_NULL(nhg_attr(n, NHA_GROUP));
}

static void test_fdb_nh_encode_rejects_unusable(void)
{
	const uint32_t members[FPM_FDB_NH_MAX_MEMBERS + 1] = {0};
	struct fpm_fdb_nh nh;
	uint8_t buf[512];

	/* Neither a VTEP nor a group: the receiver could not resolve it. */
	memset(&nh, 0, sizeof(nh));
	nh.id = 4001;
	CU_ASSERT_EQUAL(fpm_fdb_nh_encode(&nh, buf, sizeof(buf)), 0);

	/* More members than an Ethernet Segment can hold. */
	memset(&nh, 0, sizeof(nh));
	nh.id = 5000;
	nh.member_cnt = FPM_FDB_NH_MAX_MEMBERS + 1;
	nh.members = members;
	CU_ASSERT_EQUAL(fpm_fdb_nh_encode(&nh, buf, sizeof(buf)), 0);

	/* Truncated buffer must fail rather than emit a short message. */
	memset(&nh, 0, sizeof(nh));
	nh.id = 4001;
	nh.family = AF_INET;
	CU_ASSERT_EQUAL(fpm_fdb_nh_encode(&nh, buf, 4), 0);

	CU_ASSERT_EQUAL(fpm_fdb_nh_encode(NULL, buf, sizeof(buf)), 0);
	CU_ASSERT_EQUAL(fpm_fdb_nh_encode(&nh, NULL, sizeof(buf)), 0);
}


int main(void)
{
	CU_pSuite suite;
	if (CU_initialize_registry() != CUE_SUCCESS)
		return CU_get_error();

	suite = CU_add_suite("FPM MAC decode", NULL, NULL);
	if (!suite)
		goto fail;

	if (!CU_add_test(suite, "add decodes all fields", test_decode_add) ||
	    !CU_add_test(suite, "delete and sticky", test_decode_del_and_sticky) ||
	    !CU_add_test(suite, "NDA_PROTOCOL decoded", test_decode_protocol) ||
	    !CU_add_test(suite, "zebra-originated rejected",
			 test_decode_rejects_zebra_protocol) ||
	    !CU_add_test(suite, "NDA_LLADDR required",
			 test_decode_requires_lladdr) ||
	    !CU_add_test(suite, "non-bridge rejected",
			 test_decode_rejects_non_bridge) ||
	    !CU_add_test(suite, "truncated and wrong type rejected",
			 test_decode_rejects_truncated_and_other_types) ||
	    !CU_add_test(suite, "malformed NDA_VLAN ignored",
			 test_decode_ignores_malformed_vlan) ||
	    !CU_add_test(suite, "output untouched on failure",
			 test_decode_leaves_output_untouched_on_failure) ||
	    !CU_add_test(suite, "fdb nexthop vtep encoded",
			 test_fdb_nh_encode_vtep) ||
	    !CU_add_test(suite, "fdb nexthop ipv6 vtep encoded",
			 test_fdb_nh_encode_vtep_v6) ||
	    !CU_add_test(suite, "fdb nexthop group encoded",
			 test_fdb_nh_encode_group) ||
	    !CU_add_test(suite, "fdb nexthop delete encoded",
			 test_fdb_nh_encode_delete) ||
	    !CU_add_test(suite, "fdb nexthop unusable rejected",
			 test_fdb_nh_encode_rejects_unusable) ||
	    !CU_add_test(suite, "stale predicate", test_stale_predicate))
		goto fail;

	CU_basic_set_mode(CU_BRM_SILENT);
	CU_basic_run_tests();

	if (CU_get_number_of_failures() != 0)
		goto fail;

	puts("fpm_mac_test: PASS");
	CU_cleanup_registry();
	return 0;

fail:
	CU_cleanup_registry();
	return 1;
}
