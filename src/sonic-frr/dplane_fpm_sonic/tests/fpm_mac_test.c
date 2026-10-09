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
 * @brief Unit tests for the FPM MAC messages exchanged with fpmsyncd.
 *
 * fpm_mac_decode() and fpm_mac_encode() are the parts of the FPM MAC path that
 * run without zebra: everything else mutates the EVPN tables on zebra's main
 * thread. The decode tests build the messages fpmsyncd sends and check the
 * decoded fields, including the malformed cases that would otherwise install a
 * zero MAC or a wrong VLAN. The encode tests check every field fpmsyncd reads
 * in the messages it is sent.
 */

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <linux/neighbour.h>
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

static void msg_init(struct mac_msg *m, uint16_t type)
{
	memset(m, 0, sizeof(*m));
	m->n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
	m->n.nlmsg_type = type;
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

static const uint8_t kMacs[3][ETH_ALEN] = {
	{0x00, 0x11, 0x22, 0x33, 0x44, 0x01},
	{0x00, 0x11, 0x22, 0x33, 0x44, 0x02},
	{0x00, 0x11, 0x22, 0x33, 0x44, 0x03},
};

static struct rtattr *msg_attr(struct mac_msg *m, int type)
{
	struct rtattr *rta;
	int len;

	rta = (struct rtattr *)((char *)&m->n +
				NLMSG_ALIGN(NLMSG_LENGTH(sizeof(struct ndmsg))));
	len = (int)(m->n.nlmsg_len - NLMSG_LENGTH(sizeof(struct ndmsg)));

	for (; RTA_OK(rta, len); rta = RTA_NEXT(rta, len))
		if (rta->rta_type == type)
			return rta;

	return NULL;
}

static void check_attr(struct mac_msg *m, int type, const void *data,
		       size_t len)
{
	struct rtattr *rta = msg_attr(m, type);

	CU_ASSERT_PTR_NOT_NULL_FATAL(rta);
	CU_ASSERT_EQUAL((size_t)RTA_PAYLOAD(rta), len);
	CU_ASSERT_EQUAL(memcmp(RTA_DATA(rta), data, len), 0);
}

static void test_decode_add(void)
{
	struct fpm_hw_mac out;
	struct mac_msg m;
	uint16_t vid = 100;

	msg_init(&m, RTM_NEWNEIGH);
	m.ndm.ndm_ifindex = 42;
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	msg_add_attr(&m, NDA_VLAN, &vid, sizeof(vid));

	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_EQUAL(out.ifindex, 42);
	CU_ASSERT_EQUAL(out.vid, 100);
	CU_ASSERT_FALSE(out.del);
	CU_ASSERT_FALSE(out.sticky);
	CU_ASSERT_EQUAL(memcmp(out.mac, kMac, ETH_ALEN), 0);
}

static void test_decode_del_and_sticky(void)
{
	struct fpm_hw_mac out;
	struct mac_msg m;

	msg_init(&m, RTM_DELNEIGH);
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_TRUE(out.del);

	/* Stickiness comes from NTF_STICKY in ndm_flags. */
	msg_init(&m, RTM_NEWNEIGH);
	m.ndm.ndm_flags = NTF_STICKY;
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_TRUE(out.sticky);
	CU_ASSERT_FALSE(out.del);

	/* NUD_NOARP means "does not age", not "sticky". Treating it as sticky
	 * would set the EVPN sticky bit on every hardware-learnt MAC and stop
	 * those MACs from moving. */
	msg_init(&m, RTM_NEWNEIGH);
	m.ndm.ndm_state = NUD_NOARP;
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_FALSE(out.sticky);

	/* The encoding actually on the wire: zebra and fpmsyncd both set
	 * NTF_STICKY and NUD_NOARP together for a sticky entry. */
	msg_init(&m, RTM_NEWNEIGH);
	m.ndm.ndm_state = NUD_REACHABLE | NUD_NOARP;
	m.ndm.ndm_flags = NTF_MASTER | NTF_EXT_LEARNED | NTF_STICKY;
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_TRUE(out.sticky);
}

/* Without NDA_LLADDR there is no MAC to install; accepting the message would
 * install the all-zero MAC. */
static void test_decode_requires_lladdr(void)
{
	struct fpm_hw_mac out;
	struct mac_msg m;
	uint16_t vid = 100;

	msg_init(&m, RTM_NEWNEIGH);
	msg_add_attr(&m, NDA_VLAN, &vid, sizeof(vid));

	CU_ASSERT_FALSE(fpm_mac_decode(&m.n, &out));
}

/* Three MACs on three ports and VLANs, a dynamic add, a sticky add and a
 * delete, each decoded on its own. */
static void test_decode_three_macs(void)
{
	static const struct {
		uint16_t type;
		int ifindex;
		uint16_t vid;
		uint8_t flags;
		uint16_t state;
		bool del;
		bool sticky;
	} c[3] = {
		{RTM_NEWNEIGH, 41, 100, NTF_MASTER | NTF_EXT_LEARNED,
		 NUD_REACHABLE, false, false},
		{RTM_NEWNEIGH, 42, 200, NTF_MASTER | NTF_EXT_LEARNED | NTF_STICKY,
		 NUD_REACHABLE | NUD_NOARP, false, true},
		{RTM_DELNEIGH, 43, 300, NTF_MASTER | NTF_EXT_LEARNED,
		 NUD_REACHABLE, true, false},
	};
	struct fpm_hw_mac out;
	struct mac_msg m;
	int i;

	for (i = 0; i < 3; i++) {
		msg_init(&m, c[i].type);
		m.ndm.ndm_ifindex = c[i].ifindex;
		m.ndm.ndm_flags = c[i].flags;
		m.ndm.ndm_state = c[i].state;
		msg_add_attr(&m, NDA_LLADDR, kMacs[i], ETH_ALEN);
		msg_add_attr(&m, NDA_VLAN, &c[i].vid, sizeof(c[i].vid));

		CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
		CU_ASSERT_EQUAL(out.ifindex, c[i].ifindex);
		CU_ASSERT_EQUAL(out.vid, c[i].vid);
		CU_ASSERT_EQUAL(out.del, c[i].del);
		CU_ASSERT_EQUAL(out.sticky, c[i].sticky);
		CU_ASSERT_EQUAL(memcmp(out.mac, kMacs[i], ETH_ALEN), 0);
	}
}

/* Netlink uses RTM_NEWNEIGH for IP neighbours too. fpmsyncd sends only bridge
 * FDB entries, so anything else is rejected. */
static void test_decode_rejects_non_bridge(void)
{
	struct fpm_hw_mac out;
	struct mac_msg m;

	msg_init(&m, RTM_NEWNEIGH);
	m.ndm.ndm_family = AF_INET;
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);

	CU_ASSERT_FALSE(fpm_mac_decode(&m.n, &out));
}

static void test_decode_rejects_truncated_and_other_types(void)
{
	struct fpm_hw_mac out;
	struct mac_msg m;

	msg_init(&m, RTM_NEWNEIGH);
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	m.n.nlmsg_len = NLMSG_LENGTH(0);
	CU_ASSERT_FALSE(fpm_mac_decode(&m.n, &out));

	msg_init(&m, RTM_NEWROUTE);
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_FALSE(fpm_mac_decode(&m.n, &out));
}

/* A short NDA_VLAN must not be read as a uint16, and must not leave a stale
 * VLAN behind either. */
static void test_decode_ignores_malformed_vlan(void)
{
	struct fpm_hw_mac out;
	struct mac_msg m;
	uint8_t short_vid = 5;

	msg_init(&m, RTM_NEWNEIGH);
	msg_add_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	msg_add_attr(&m, NDA_VLAN, &short_vid, sizeof(short_vid));

	CU_ASSERT_TRUE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_EQUAL(out.vid, 0);
}

/* A rejected message must not scribble on the caller's struct. */
static void test_decode_leaves_output_untouched_on_failure(void)
{
	struct fpm_hw_mac out;
	struct mac_msg m;

	memset(&out, 0xAB, sizeof(out));
	msg_init(&m, RTM_NEWNEIGH);
	m.ndm.ndm_family = AF_INET;

	CU_ASSERT_FALSE(fpm_mac_decode(&m.n, &out));
	CU_ASSERT_EQUAL(out.ifindex, (int)0xABABABAB);
}

/* Three remote MACs: a dynamic one behind an IPv4 VTEP, a sticky one behind an
 * IPv6 VTEP, and one behind a remote Ethernet Segment's nexthop group, which
 * replaces the VTEP. */
static void test_encode_three_remote_macs(void)
{
	static const uint8_t vtep4[4] = {10, 0, 0, 1};
	static const uint8_t vtep6[16] = {0xfd, 0, 0, 0, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 2};
	struct fpm_remote_mac in[3];
	struct mac_msg m;
	uint32_t nhg;
	size_t len;
	int i;

	memset(in, 0, sizeof(in));
	for (i = 0; i < 3; i++) {
		in[i].ifindex = 10 + i;
		memcpy(in[i].mac, kMacs[i], ETH_ALEN);
		in[i].vid = (uint16_t)(100 * (i + 1));
		in[i].vni = (uint32_t)(10100 + 100 * i);
	}
	in[0].vtep_family = AF_INET;
	memcpy(in[0].vtep, vtep4, sizeof(vtep4));
	in[1].vtep_family = AF_INET6;
	memcpy(in[1].vtep, vtep6, sizeof(vtep6));
	in[1].sticky = true;
	in[2].vtep_family = AF_INET;
	memcpy(in[2].vtep, vtep4, sizeof(vtep4));
	in[2].nhg_id = 536870913;

	for (i = 0; i < 3; i++) {
		memset(&m, 0xAB, sizeof(m));
		len = fpm_mac_encode(&in[i], &m, sizeof(m));
		CU_ASSERT_EQUAL_FATAL(len, NLMSG_ALIGN(m.n.nlmsg_len));
		CU_ASSERT_EQUAL(m.n.nlmsg_type, RTM_NEWNEIGH);
		CU_ASSERT_EQUAL(m.n.nlmsg_flags,
				NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE);
		CU_ASSERT_EQUAL(m.ndm.ndm_family, AF_BRIDGE);
		CU_ASSERT_EQUAL(m.ndm.ndm_ifindex, in[i].ifindex);
		check_attr(&m, NDA_LLADDR, kMacs[i], ETH_ALEN);
		check_attr(&m, NDA_VLAN, &in[i].vid, sizeof(in[i].vid));
		check_attr(&m, NDA_SRC_VNI, &in[i].vni, sizeof(in[i].vni));

		if (in[i].sticky) {
			CU_ASSERT_EQUAL(m.ndm.ndm_flags,
					NTF_MASTER | NTF_SELF | NTF_EXT_LEARNED |
						NTF_STICKY);
			CU_ASSERT_EQUAL(m.ndm.ndm_state,
					NUD_REACHABLE | NUD_NOARP);
		} else {
			CU_ASSERT_EQUAL(m.ndm.ndm_flags,
					NTF_MASTER | NTF_SELF | NTF_EXT_LEARNED);
			CU_ASSERT_EQUAL(m.ndm.ndm_state, NUD_REACHABLE);
		}
	}

	fpm_mac_encode(&in[0], &m, sizeof(m));
	check_attr(&m, NDA_DST, vtep4, sizeof(vtep4));
	CU_ASSERT_PTR_NULL(msg_attr(&m, NDA_NH_ID));

	fpm_mac_encode(&in[1], &m, sizeof(m));
	check_attr(&m, NDA_DST, vtep6, sizeof(vtep6));

	fpm_mac_encode(&in[2], &m, sizeof(m));
	nhg = 536870913;
	check_attr(&m, NDA_NH_ID, &nhg, sizeof(nhg));
	CU_ASSERT_PTR_NULL(msg_attr(&m, NDA_DST));
}

/* A delete carries the same fields as the add, without NLM_F_CREATE. */
static void test_encode_delete(void)
{
	struct fpm_remote_mac in;
	struct mac_msg m;

	memset(&in, 0, sizeof(in));
	in.ifindex = 10;
	memcpy(in.mac, kMac, ETH_ALEN);
	in.vtep_family = AF_INET;
	in.vtep[0] = 10;
	in.vtep[3] = 1;
	in.del = true;

	CU_ASSERT_TRUE(fpm_mac_encode(&in, &m, sizeof(m)) > 0);
	CU_ASSERT_EQUAL(m.n.nlmsg_type, RTM_DELNEIGH);
	CU_ASSERT_EQUAL(m.n.nlmsg_flags, NLM_F_REQUEST);
	check_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_PTR_NOT_NULL(msg_attr(&m, NDA_DST));
}

/* A MAC on an Ethernet Segment this switch also has goes against the local
 * access port, with neither a VTEP nor a group, and an unset VLAN or VNI is not
 * sent at all. */
static void test_encode_omits_unset_fields(void)
{
	struct fpm_remote_mac in;
	struct mac_msg m;

	memset(&in, 0, sizeof(in));
	in.ifindex = 20;
	memcpy(in.mac, kMac, ETH_ALEN);

	CU_ASSERT_TRUE(fpm_mac_encode(&in, &m, sizeof(m)) > 0);
	check_attr(&m, NDA_LLADDR, kMac, ETH_ALEN);
	CU_ASSERT_PTR_NULL(msg_attr(&m, NDA_DST));
	CU_ASSERT_PTR_NULL(msg_attr(&m, NDA_NH_ID));
	CU_ASSERT_PTR_NULL(msg_attr(&m, NDA_VLAN));
	CU_ASSERT_PTR_NULL(msg_attr(&m, NDA_SRC_VNI));
}

/* A message that does not fit is not sent in part. */
static void test_encode_rejects_short_buffer(void)
{
	struct fpm_remote_mac in;
	struct mac_msg m;

	memset(&in, 0, sizeof(in));
	memcpy(in.mac, kMac, ETH_ALEN);
	in.vtep_family = AF_INET6;

	CU_ASSERT_EQUAL(fpm_mac_encode(&in, &m, NLMSG_LENGTH(sizeof(struct ndmsg))
						    + RTA_SPACE(ETH_ALEN)),
			0);
	CU_ASSERT_EQUAL(fpm_mac_encode(&in, &m, sizeof(struct nlmsghdr)), 0);
	CU_ASSERT_EQUAL(fpm_mac_encode(NULL, &m, sizeof(m)), 0);
}

int main(void)
{
	CU_pSuite suite;
	if (CU_initialize_registry() != CUE_SUCCESS)
		return CU_get_error();

	suite = CU_add_suite("FPM MAC encode and decode", NULL, NULL);
	if (!suite)
		goto fail;

	if (!CU_add_test(suite, "add decodes all fields", test_decode_add) ||
	    !CU_add_test(suite, "delete and sticky", test_decode_del_and_sticky) ||
	    !CU_add_test(suite, "three MACs decoded", test_decode_three_macs) ||
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
	    !CU_add_test(suite, "three remote MACs encoded",
			 test_encode_three_remote_macs) ||
	    !CU_add_test(suite, "remote MAC delete", test_encode_delete) ||
	    !CU_add_test(suite, "unset fields not sent",
			 test_encode_omits_unset_fields) ||
	    !CU_add_test(suite, "short buffer rejected",
			 test_encode_rejects_short_buffer))
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
