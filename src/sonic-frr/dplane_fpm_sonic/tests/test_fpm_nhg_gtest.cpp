// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for the dplane NHG object model (fpm_nhg.c), written against
 * googletest following the sonic-fib tests/ pattern.
 *
 * Built and run by src/sonic-frr/Makefile right after the FRR package build,
 * against the just-built libfrr; the few zebra-daemon symbols fpm_nhg.c
 * references are stubbed in stubs.c. The module sources compile as C, this
 * file as C++.
 *
 * Covered contracts:
 *   build     - a route's nexthop chain derives the L-A/L-B/L-C object tree
 *   dedupe    - equal subtrees reuse objects; changed input derives new ones
 *   rollback  - a failed context undoes everything it created, including the
 *               refcounts it put on pre-existing shared objects
 *   id-reuse  - a freed dplane id is never handed out ahead of its
 *               DELNHGFIB: in one batch every DEL precedes the NEWs
 *   assembly  - carried-over DELs first, this context's NEWs child-first,
 *               and the route frame last (NEWNHGFIB before NEWROUTE)
 *   contrib   - commit-time membership counting: a route's build collects
 *               (leaf, vrf, prefix) resolutions, the commit diff moves the
 *               counts, zero unindexes the membership, and rollback/retry
 *               move nothing
 */
#include "config.h"

#include "gtest/gtest.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "lib/zebra.h"
#include "lib/memory.h"
#include "lib/prefix.h"
#include "lib/nexthop.h"
#include "lib/zlog.h"

#include "zebra/fpm_nhg.h"

#include <linux/nexthop.h>
#include <linux/rtnetlink.h>

/* -------------------------------------------------------------------------
 * table lifecycle: every test gets a fresh set of tables
 * ------------------------------------------------------------------------- */

static struct fpm_nhg_tables t;

class NhgTables : public testing::Test {
protected:
	void SetUp() override { fpm_nhg_tables_init(&t); }
	void TearDown() override { fpm_nhg_tables_fini(&t); }
};

/* -------------------------------------------------------------------------
 * nexthop fixtures
 * ------------------------------------------------------------------------- */

static struct nexthop *leaf_nh4(const char *gate, ifindex_t ifindex)
{
	struct nexthop *nh = nexthop_new();
	struct prefix p;

	if (str2prefix(gate, &p) != 1)
		abort();
	nh->type = NEXTHOP_TYPE_IPV4_IFINDEX;
	nh->gate.ipv4 = p.u.prefix4;
	nh->ifindex = ifindex;
	nh->weight = 1;
	nh->vrf_id = 0;
	return nh;
}

/* A plain leaf zebra reached through a covering prefix (the "res via"
 * connected match): the resolving info sits on the leaf itself.
 */
static struct nexthop *leaf_nh4_res(const char *gate, ifindex_t ifindex,
				    const char *res_prefix, uint32_t res_id)
{
	struct nexthop *nh = leaf_nh4(gate, ifindex);
	struct prefix rp;

	if (str2prefix(res_prefix, &rp) != 1)
		abort();
	nexthop_set_res_info(nh, res_id, &rp);
	return nh;
}

/* A recursive nexthop: gate plus the chain it resolved to, stamped with the
 * resolving prefix and NHG id like zebra's set_resolving_info() does.
 */
static struct nexthop *recursive_nh4(const char *gate, const char *res_prefix,
				     uint32_t res_id, uint16_t weight,
				     struct nexthop *resolved)
{
	struct nexthop *nh = nexthop_new();
	struct prefix gp, rp;

	if (str2prefix(gate, &gp) != 1 || str2prefix(res_prefix, &rp) != 1)
		abort();
	nh->type = NEXTHOP_TYPE_IPV4;
	nh->gate.ipv4 = gp.u.prefix4;
	nh->weight = weight;
	nh->vrf_id = 0;
	SET_FLAG(nh->flags, NEXTHOP_FLAG_RECURSIVE);
	nh->resolved = resolved;
	nexthop_set_res_info(nh, res_id, &rp);
	return nh;
}

static void chain_append(struct nexthop **head, struct nexthop *nh)
{
	while (*head)
		head = &(*head)->next;
	*head = nh;
}

/* nexthop_free() releases ->resolved and res_info as well. */
static void chain_free(struct nexthop *head)
{
	while (head) {
		struct nexthop *next = head->next;

		head->next = NULL;
		nexthop_free(head);
		head = next;
	}
}

static void route_key(struct fpm_nhg_route_key *k, const char *dest)
{
	struct prefix p;

	if (str2prefix(dest, &p) != 1)
		abort();
	fpm_nhg_route_key_init(k, /* table */ 254, &p, NULL);
}

/* -------------------------------------------------------------------------
 * frame decoding (netlink payloads as the batch holds them)
 * ------------------------------------------------------------------------- */

static uint16_t msg_type(const struct fpm_frame *f)
{
	return ((const struct nlmsghdr *)f->buf)->nlmsg_type;
}

static uint32_t msg_nha_id(const struct fpm_frame *f)
{
	const struct nlmsghdr *n = (const struct nlmsghdr *)f->buf;
	const struct rtattr *a = (const struct rtattr *)(
		(const char *)n + NLMSG_ALIGN(sizeof(struct nhmsg)));

	return *(const uint32_t *)RTA_DATA(a);
}

/* -------------------------------------------------------------------------
 * build
 * ------------------------------------------------------------------------- */

TEST_F(NhgTables, build)
{
	struct fpm_nhg_staging newq = {};
	struct nexthop *l1 = leaf_nh4("192.168.1.1/32", 5);
	struct nexthop *r1 =
		recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1, l1);
	struct fpm_dplane_nhg *top, *lb, *leaf;

	top = fpm_nhg_build(&t, r1, &newq);
	ASSERT_NE(top, nullptr);

	/* L-A -> one L-B -> one L-C */
	EXPECT_EQ(FPM_NHG_L_A, top->level);
	ASSERT_EQ(1, top->num_children);
	lb = top->children[0].obj;
	EXPECT_EQ(FPM_NHG_L_B, lb->level);
	ASSERT_EQ(1, lb->num_children);
	leaf = lb->children[0].obj;
	EXPECT_EQ(FPM_NHG_L_C, leaf->level);
	EXPECT_EQ(0, leaf->num_children);

	/* Creation is child-first, one id per object, in staging order. */
	EXPECT_EQ(1u, leaf->dplane_id);
	EXPECT_EQ(2u, lb->dplane_id);
	EXPECT_EQ(3u, top->dplane_id);
	ASSERT_EQ(3u, newq.count);
	EXPECT_EQ(leaf, newq.objs[0]);
	EXPECT_EQ(lb, newq.objs[1]);
	EXPECT_EQ(top, newq.objs[2]);

	/* Reference counts: each parent holds one reference. */
	EXPECT_EQ(0u, top->refcount);
	EXPECT_EQ(1u, lb->refcount);
	EXPECT_EQ(1u, leaf->refcount);

	/* Resolving info recorded from the recursive parent. */
	EXPECT_EQ(1000u, lb->resolved_via);
	EXPECT_EQ(AF_INET, lb->resolved_prefix.family);
	EXPECT_EQ(24, lb->resolved_prefix.prefixlen);

	/* The recursive gate carries no resolving info of its own, so the
	 * build collects nothing here: the resolution lives in the L-B
	 * identity, not as a leaf membership.
	 */
	EXPECT_EQ(0u, newq.contrib_count);
	EXPECT_EQ(0, leaf->res_member_count);

	EXPECT_EQ(3u, fpm_nhg_count(&t));

	/* Nothing committed: rollback destroys everything the build made. */
	fpm_nhg_rollback(&t, &newq);
	fpm_nhg_staging_free(&newq);
	EXPECT_EQ(0u, fpm_nhg_count(&t));
	EXPECT_EQ(t.obj_created, t.obj_deleted);
	EXPECT_EQ(3u, t.free_id_count);

	chain_free(r1);
}

/* -------------------------------------------------------------------------
 * dedupe
 * ------------------------------------------------------------------------- */

TEST_F(NhgTables, dedupe)
{
	struct fpm_nhg_staging newq = {};
	struct fpm_nhg_del_queue delq = {};
	struct fpm_nhg_route_key ka, kb;
	struct nexthop *ra, *rb, *rc;
	struct fpm_dplane_nhg *topa, *topb, *topc;
	uint64_t hits0;

	ra = recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.1/32", 5));
	rb = recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.1/32", 5));
	rc = recursive_nh4("10.0.0.2/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.2/32", 5));

	topa = fpm_nhg_build(&t, ra, &newq);
	ASSERT_NE(topa, nullptr);
	EXPECT_EQ(3u, newq.count);
	route_key(&ka, "172.16.1.0/24");
	fpm_nhg_route_commit(&t, &ka, topa, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);

	/* An equal chain dedupes at every level: leaf, L-B and L-A, and the
	 * reuse stages nothing at all — no object, no contribution.
	 */
	hits0 = t.dedupe_hits;
	topb = fpm_nhg_build(&t, rb, &newq);
	EXPECT_EQ(topa, topb) << "equal chain did not dedupe";
	EXPECT_EQ(3u, t.dedupe_hits - hits0);
	EXPECT_EQ(3u, t.obj_created);
	EXPECT_EQ(0u, newq.count);
	EXPECT_EQ(0u, newq.contrib_count);

	route_key(&kb, "172.16.2.0/24");
	fpm_nhg_route_commit(&t, &kb, topb, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);
	EXPECT_EQ(2u, topa->refcount);

	/* A changed gate derives a whole new tree. */
	topc = fpm_nhg_build(&t, rc, &newq);
	ASSERT_NE(topc, nullptr);
	EXPECT_NE(topa, topc) << "changed chain must not dedupe";
	EXPECT_EQ(6u, t.obj_created);
	fpm_nhg_rollback(&t, &newq);
	fpm_nhg_staging_free(&newq);

	/* Forgetting one of two sharers keeps the tree alive. */
	fpm_nhg_route_forget(&t, &kb, &delq);
	EXPECT_EQ(1u, topa->refcount);
	EXPECT_EQ(0u, fpm_nhg_del_queue_count(&delq));

	/* The last dereference stages parent-before-child DELs and frees the
	 * whole tree.
	 */
	fpm_nhg_route_forget(&t, &ka, &delq);
	EXPECT_EQ(3u, fpm_nhg_del_queue_count(&delq));
	EXPECT_EQ(0u, fpm_nhg_count(&t));
	EXPECT_EQ(t.obj_created, t.obj_deleted);

	fpm_nhg_del_queue_free(&delq);
	chain_free(ra);
	chain_free(rb);
	chain_free(rc);
}

/* -------------------------------------------------------------------------
 * rollback of a partially-shared build
 * ------------------------------------------------------------------------- */

TEST_F(NhgTables, rollback_partial_share)
{
	struct fpm_nhg_staging newq = {};
	struct fpm_nhg_del_queue delq = {};
	struct fpm_nhg_route_key ka;
	struct nexthop *ra, *rb;
	struct fpm_dplane_nhg *topa, *topb, *lbref;
	uint32_t lb_ref0;
	uint64_t created0;

	ra = recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.1/32", 5));
	topa = fpm_nhg_build(&t, ra, &newq);
	ASSERT_NE(topa, nullptr);
	ASSERT_EQ(1, topa->num_children);
	route_key(&ka, "172.16.1.0/24");
	fpm_nhg_route_commit(&t, &ka, topa, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);

	/* Route B reuses A's recursive subtree and adds a direct leaf: the
	 * L-A object is new, the L-B and the first leaf are shared.
	 */
	rb = recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.1/32", 5));
	chain_append(&rb, leaf_nh4("192.168.1.9/32", 9));
	lbref = topa->children[0].obj;
	lb_ref0 = lbref->refcount;
	created0 = t.obj_created;

	topb = fpm_nhg_build(&t, rb, &newq);
	ASSERT_NE(topb, nullptr);
	EXPECT_NE(topa, topb) << "B top must be a new object";
	EXPECT_EQ(2u, t.obj_created - created0);
	EXPECT_EQ(lb_ref0 + 1, lbref->refcount);

	/* A failed context (encode failure / no room) rolls the whole build
	 * back: new objects die, shared refcounts are restored.
	 */
	fpm_nhg_rollback(&t, &newq);
	fpm_nhg_staging_free(&newq);
	EXPECT_EQ(lb_ref0, lbref->refcount);
	EXPECT_EQ(created0, t.obj_created);
	EXPECT_EQ(3u, fpm_nhg_count(&t));

	fpm_nhg_route_forget(&t, &ka, &delq);
	fpm_nhg_del_queue_free(&delq);
	chain_free(ra);
	chain_free(rb);
}

/* -------------------------------------------------------------------------
 * id reuse
 * ------------------------------------------------------------------------- */

TEST_F(NhgTables, id_reuse)
{
	struct fpm_nhg_staging newq = {};
	struct fpm_nhg_del_queue delq = {};
	struct fpm_frame_batch batch = {};
	struct fpm_nhg_route_key ka;
	struct nexthop *ra, *rd;
	struct fpm_dplane_nhg *topa, *topd;
	uint32_t a, b, i, j;
	bool overlap = false;

	/* Allocator basics: ids count up from 1, freed ids come back LIFO. */
	a = fpm_nhg_id_alloc(&t);
	b = fpm_nhg_id_alloc(&t);
	EXPECT_EQ(1u, a);
	EXPECT_EQ(2u, b);
	fpm_nhg_id_free(&t, a);
	EXPECT_EQ(a, fpm_nhg_id_alloc(&t)) << "free list must be LIFO";

	/* One route in, then forgotten: its tree's ids are freed and its DELs
	 * are staged. The next context reuses those very ids, and its NEWs must
	 * follow the staged DELs in the batch.
	 */
	ra = recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.1/32", 5));
	topa = fpm_nhg_build(&t, ra, &newq);
	ASSERT_NE(topa, nullptr);
	route_key(&ka, "172.16.1.0/24");
	fpm_nhg_route_commit(&t, &ka, topa, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);

	fpm_nhg_route_forget(&t, &ka, &delq);
	EXPECT_EQ(3u, fpm_nhg_del_queue_count(&delq));

	rd = recursive_nh4("10.0.0.2/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.2/32", 6));
	topd = fpm_nhg_build(&t, rd, &newq);
	if (!topd) {
		ADD_FAILURE() << "build D failed";
		goto out;
	}

	/* The fresh tree pops the freed ids, so DEL and NEW collide. */
	ASSERT_TRUE(fpm_nhg_batch_add_pending_dels(&delq, &batch));
	ASSERT_TRUE(fpm_nhg_batch_add_staging(&newq, &batch));
	ASSERT_EQ(6u, batch.count);
	for (i = 0; i < 3 && i < batch.count; i++)
		EXPECT_EQ(RTM_DELNHGFIB, msg_type(&batch.frames[i]))
			<< "frame " << i << " must be a DEL";
	for (i = 3; i < batch.count; i++)
		EXPECT_EQ(RTM_NEWNHGFIB, msg_type(&batch.frames[i]))
			<< "frame " << i << " must be a NEW";

	for (i = 3; i < batch.count; i++) { /* NEWs start after the 3 DELs */
		uint32_t id = msg_nha_id(&batch.frames[i]);

		for (j = 0; j < 3; j++) {
			if (msg_nha_id(&batch.frames[j]) != id)
				continue;
			overlap = true;
			EXPECT_LT(j, i)
				<< "NEW id " << id << " (frame " << i
				<< ") precedes its DEL";
		}
	}
	EXPECT_TRUE(overlap) << "no id was reused; test is vacuous";

out:
	/* The caller resets the deferred queue once the batch is on the wire. */
	fpm_nhg_del_queue_reset(&delq);

	fpm_nhg_frame_batch_free(&batch);
	fpm_nhg_rollback(&t, &newq);
	fpm_nhg_staging_free(&newq);
	fpm_nhg_del_queue_free(&delq);
	chain_free(ra);
	chain_free(rd);
}

/* -------------------------------------------------------------------------
 * batch assembly order
 * ------------------------------------------------------------------------- */

TEST_F(NhgTables, assembly)
{
	struct fpm_nhg_staging newq = {};
	struct fpm_frame_batch batch = {};
	struct nexthop *r1, *r2;
	struct fpm_dplane_nhg *top;
	struct {
		struct nlmsghdr n;
		char buf[16];
	} rframe = {};
	uint32_t i, j, k;

	r1 = recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.1/32", 5));
	r2 = recursive_nh4("10.0.0.2/32", "10.0.0.0/24", 1001, 1,
			   leaf_nh4("192.168.1.2/32", 6));
	chain_append(&r1, r2);

	top = fpm_nhg_build(&t, r1, &newq);
	ASSERT_NE(top, nullptr);
	EXPECT_EQ(5u, newq.count);

	/* Staging is child-before-parent. */
	for (i = 0; i < newq.count; i++) {
		const struct fpm_dplane_nhg *o = newq.objs[i];

		for (j = 0; j < o->num_children; j++) {
			uint32_t idx = UINT32_MAX;

			for (k = 0; k < newq.count; k++)
				if (newq.objs[k] == o->children[j].obj)
					idx = k;
			EXPECT_LT(idx, i) << "member staged after its parent";
		}
	}

	/* The frames mirror the staging order, all NHGFIB. */
	ASSERT_TRUE(fpm_nhg_batch_add_staging(&newq, &batch));
	ASSERT_EQ(newq.count, batch.count);
	for (i = 0; i < batch.count && i < newq.count; i++) {
		EXPECT_EQ(RTM_NEWNHGFIB, msg_type(&batch.frames[i]))
			<< "frame " << i;
		EXPECT_EQ(newq.objs[i]->dplane_id, msg_nha_id(&batch.frames[i]))
			<< "frame " << i << " id mismatch";
	}

	/* The route frame is appended after the NHGFIBs: define before
	 * reference, NEWNHGFIB before NEWROUTE.
	 */
	rframe.n.nlmsg_len = NLMSG_LENGTH(0);
	rframe.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
	rframe.n.nlmsg_type = RTM_NEWROUTE;
	fpm_nhg_frame_batch_add(&batch, (uint8_t *)&rframe,
				NLMSG_ALIGN(rframe.n.nlmsg_len));
	EXPECT_EQ(RTM_NEWROUTE, msg_type(&batch.frames[batch.count - 1]))
		<< "the route frame must come last";
	for (i = 0; i + 1 < batch.count; i++)
		EXPECT_EQ(RTM_NEWNHGFIB, msg_type(&batch.frames[i]))
			<< "frame " << i << " before the route is not NHGFIB";

	fpm_nhg_frame_batch_free(&batch);
	fpm_nhg_rollback(&t, &newq);
	fpm_nhg_staging_free(&newq);
	chain_free(r1);
}

/* -------------------------------------------------------------------------
 * leaf membership contributions
 * ------------------------------------------------------------------------- */

TEST_F(NhgTables, contrib)
{
	struct fpm_nhg_staging newq = {};
	struct fpm_nhg_del_queue delq = {};
	struct fpm_nhg_route_key ka, kb;
	struct nexthop *ra, *rb, *rc, *rd = NULL, *re = NULL;
	struct fpm_dplane_nhg *topa, *topb, *topc, *topd = NULL, *tope = NULL;
	struct fpm_dplane_nhg *leaf;
	struct prefix p1, p2;
	uint64_t created0;

	if (str2prefix("10.0.0.0/24", &p1) != 1 ||
	    str2prefix("172.16.0.0/24", &p2) != 1)
		abort();

	/* A plain leaf reached through P1. A single non-recursive member
	 * makes the L-A the leaf itself.
	 */
	ra = leaf_nh4_res("192.168.1.1/32", 5, "10.0.0.0/24", 1000);
	topa = fpm_nhg_build(&t, ra, &newq);
	ASSERT_NE(topa, nullptr);
	EXPECT_EQ(FPM_NHG_L_C, topa->level);
	leaf = topa;
	route_key(&ka, "172.16.1.0/24");
	fpm_nhg_route_commit(&t, &ka, topa, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);
	ASSERT_EQ(1, leaf->res_member_count);
	EXPECT_EQ(1u, leaf->res_members[0].count);
	EXPECT_TRUE(prefix_same(&leaf->res_members[0].p, &p1));

	/* Re-committing an identical build moves nothing. */
	rb = leaf_nh4_res("192.168.1.1/32", 5, "10.0.0.0/24", 1000);
	topb = fpm_nhg_build(&t, rb, &newq);
	EXPECT_EQ(topa, topb) << "identical chain must dedupe";
	fpm_nhg_route_commit(&t, &ka, topb, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);
	ASSERT_EQ(1, leaf->res_member_count);
	EXPECT_EQ(1u, leaf->res_members[0].count);

	/* The same gate reached through P2 releases P1 and acquires P2, on
	 * the very same shared leaf.
	 */
	rc = leaf_nh4_res("192.168.1.1/32", 5, "172.16.0.0/24", 1000);
	topc = fpm_nhg_build(&t, rc, &newq);
	EXPECT_EQ(leaf, topc) << "gate unchanged: the leaf must be shared";
	if (!topc)
		goto out;
	fpm_nhg_route_commit(&t, &ka, topc, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);
	ASSERT_EQ(1, leaf->res_member_count);
	EXPECT_EQ(1u, leaf->res_members[0].count);
	EXPECT_TRUE(prefix_same(&leaf->res_members[0].p, &p2))
		<< "rebind must release P1 and keep only P2";

	/* A rolled-back build leaves no trace, not even a fresh membership on
	 * a shared leaf.
	 */
	rd = leaf_nh4_res("192.168.1.1/32", 5, "10.0.0.0/24", 1000);
	created0 = t.obj_created;
	topd = fpm_nhg_build(&t, rd, &newq);
	EXPECT_EQ(leaf, topd) << "build D must reuse the leaf";
	fpm_nhg_rollback(&t, &newq);
	fpm_nhg_staging_free(&newq);
	EXPECT_EQ(created0, t.obj_created);
	ASSERT_EQ(1, leaf->res_member_count);
	EXPECT_EQ(1u, leaf->res_members[0].count);
	EXPECT_TRUE(prefix_same(&leaf->res_members[0].p, &p2))
		<< "rollback must not touch the memberships";

	/* A second route on the same gate and prefix raises the count;
	 * forgetting one drops it back, forgetting the last unindexes the
	 * leaf entirely.
	 */
	re = leaf_nh4_res("192.168.1.1/32", 5, "172.16.0.0/24", 1000);
	tope = fpm_nhg_build(&t, re, &newq);
	EXPECT_EQ(leaf, tope) << "equal chain must dedupe";
	if (tope) {
		route_key(&kb, "172.16.2.0/24");
		fpm_nhg_route_commit(&t, &kb, tope, 1000, &delq, &newq);
		fpm_nhg_staging_free(&newq);
		ASSERT_EQ(1, leaf->res_member_count);
		EXPECT_EQ(2u, leaf->res_members[0].count);

		fpm_nhg_route_forget(&t, &kb, &delq);
		ASSERT_EQ(1, leaf->res_member_count);
		EXPECT_EQ(1u, leaf->res_members[0].count);
	}

out:
	fpm_nhg_route_forget(&t, &ka, &delq);
	EXPECT_EQ(t.obj_created, t.obj_deleted);

	fpm_nhg_del_queue_free(&delq);
	chain_free(ra);
	chain_free(rb);
	chain_free(rc);
	chain_free(rd);
	chain_free(re);
}
