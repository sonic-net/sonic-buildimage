// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for the dplane NHG object model (fpm_nhg.c).
 *
 * Built and run by src/sonic-frr/Makefile right after the FRR package build,
 * against the just-built libfrr; the few zebra-daemon symbols fpm_nhg.c
 * references are stubbed in stubs.c. No zebra, no dataplane, no device state.
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

static int checks;
static int failures;

#define CHECK(cond, ...)                                                       \
	do {                                                                   \
		checks++;                                                      \
		if (!(cond)) {                                                 \
			failures++;                                            \
			printf("FAIL %s:%d: ", __func__, __LINE__);            \
			printf(__VA_ARGS__);                                   \
			printf("\n");                                          \
		}                                                              \
	} while (0)

/* -------------------------------------------------------------------------
 * nexthop fixtures
 * ------------------------------------------------------------------------- */

static struct fpm_nhg_tables t;

static void tables_reset(void)
{
	fpm_nhg_tables_fini(&t);
	fpm_nhg_tables_init(&t);
}

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

static void test_build(void)
{
	struct fpm_nhg_staging newq = {};
	struct nexthop *l1 = leaf_nh4("192.168.1.1/32", 5);
	struct nexthop *r1 =
		recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1, l1);
	struct fpm_dplane_nhg *top, *lb, *leaf;

	printf("--- test_build\n");
	tables_reset();

	top = fpm_nhg_build(&t, r1, &newq);
	CHECK(top != NULL, "build returned NULL");
	if (!top)
		goto out;

	/* L-A -> one L-B -> one L-C */
	CHECK(top->level == FPM_NHG_L_A, "top level %u", top->level);
	CHECK(top->num_children == 1, "top children %u", top->num_children);
	if (top->num_children != 1)
		goto out;
	lb = top->children[0].obj;
	CHECK(lb->level == FPM_NHG_L_B, "L-B level %u", lb->level);
	CHECK(lb->num_children == 1, "L-B children %u", lb->num_children);
	if (lb->num_children != 1)
		goto out;
	leaf = lb->children[0].obj;
	CHECK(leaf->level == FPM_NHG_L_C, "leaf level %u", leaf->level);
	CHECK(leaf->num_children == 0, "leaf children %u", leaf->num_children);

	/* Creation is child-first, one id per object, in staging order. */
	CHECK(leaf->dplane_id == 1 && lb->dplane_id == 2 &&
		      top->dplane_id == 3,
	      "ids leaf %u L-B %u top %u", leaf->dplane_id, lb->dplane_id,
	      top->dplane_id);
	CHECK(newq.count == 3, "staged %u", newq.count);
	if (newq.count == 3)
		CHECK(newq.objs[0] == leaf && newq.objs[1] == lb &&
			      newq.objs[2] == top,
		      "staging order is not child-first");

	/* Reference counts: each parent holds one reference. */
	CHECK(top->refcount == 0, "top refcount %u", top->refcount);
	CHECK(lb->refcount == 1, "L-B refcount %u", lb->refcount);
	CHECK(leaf->refcount == 1, "leaf refcount %u", leaf->refcount);

	/* Resolving info recorded from the recursive parent. */
	CHECK(lb->resolved_via == 1000, "resolved_via %u", lb->resolved_via);
	CHECK(lb->resolved_prefix.family == AF_INET &&
		      lb->resolved_prefix.prefixlen == 24,
	      "resolved prefix wrong");
	/* The recursive gate carries no resolving info of its own, so the
	 * build collects nothing here: the resolution lives in the L-B
	 * identity, not as a leaf membership.
	 */
	CHECK(newq.contrib_count == 0, "contribs %u", newq.contrib_count);
	CHECK(leaf->res_member_count == 0, "members %u", leaf->res_member_count);
	CHECK(fpm_nhg_count(&t) == 3, "count %u", fpm_nhg_count(&t));

	/* Nothing committed: rollback destroys everything the build made. */
	fpm_nhg_rollback(&t, &newq);
	fpm_nhg_staging_free(&newq);
	CHECK(fpm_nhg_count(&t) == 0, "count after rollback %u",
	      fpm_nhg_count(&t));
	CHECK(t.obj_created == t.obj_deleted, "created %" PRIu64 " deleted %" PRIu64,
	      t.obj_created, t.obj_deleted);
	CHECK(t.free_id_count == 3, "free ids %u", t.free_id_count);

out:
	chain_free(r1);
}

/* -------------------------------------------------------------------------
 * dedupe
 * ------------------------------------------------------------------------- */

static void test_dedupe(void)
{
	struct fpm_nhg_staging newq = {};
	struct fpm_nhg_del_queue delq = {};
	struct fpm_nhg_route_key ka, kb;
	struct nexthop *ra, *rb, *rc;
	struct fpm_dplane_nhg *topa, *topb, *topc;
	uint64_t hits0;

	printf("--- test_dedupe\n");
	tables_reset();

	ra = recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.1/32", 5));
	rb = recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.1/32", 5));
	rc = recursive_nh4("10.0.0.2/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.2/32", 5));

	topa = fpm_nhg_build(&t, ra, &newq);
	CHECK(topa != NULL && newq.count == 3, "first build");
	if (!topa)
		return;
	route_key(&ka, "172.16.1.0/24");
	fpm_nhg_route_commit(&t, &ka, topa, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);

	/* An equal chain dedupes at every level: leaf, L-B and L-A, and the
	 * reuse stages nothing at all — no object, no contribution.
	 */
	hits0 = t.dedupe_hits;
	topb = fpm_nhg_build(&t, rb, &newq);
	CHECK(topb == topa, "equal chain did not dedupe");
	CHECK(t.dedupe_hits - hits0 == 3, "dedupe hits +%" PRIu64,
	      t.dedupe_hits - hits0);
	CHECK(t.obj_created == 3, "created %" PRIu64, t.obj_created);
	CHECK(newq.count == 0 && newq.contrib_count == 0,
	      "reuse staged %u object(s) / %u contribution(s)", newq.count,
	      newq.contrib_count);

	route_key(&kb, "172.16.2.0/24");
	fpm_nhg_route_commit(&t, &kb, topb, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);
	CHECK(topa->refcount == 2, "refcount %u", topa->refcount);

	/* A changed gate derives a whole new tree. */
	topc = fpm_nhg_build(&t, rc, &newq);
	CHECK(topc != NULL && topc != topa, "changed chain must not dedupe");
	CHECK(t.obj_created == 6, "created %" PRIu64, t.obj_created);
	fpm_nhg_rollback(&t, &newq);
	fpm_nhg_staging_free(&newq);

	/* Forgetting one of two sharers keeps the tree alive. */
	fpm_nhg_route_forget(&t, &kb, &delq);
	CHECK(topa->refcount == 1, "refcount %u", topa->refcount);
	CHECK(fpm_nhg_del_queue_count(&delq) == 0, "premature DELs");

	/* The last dereference stages parent-before-child DELs and frees the
	 * whole tree.
	 */
	fpm_nhg_route_forget(&t, &ka, &delq);
	CHECK(fpm_nhg_del_queue_count(&delq) == 3, "DELs %u",
	      fpm_nhg_del_queue_count(&delq));
	CHECK(fpm_nhg_count(&t) == 0, "count %u", fpm_nhg_count(&t));
	CHECK(t.obj_created == t.obj_deleted, "created %" PRIu64 " deleted %" PRIu64,
	      t.obj_created, t.obj_deleted);

	fpm_nhg_del_queue_free(&delq);
	chain_free(ra);
	chain_free(rb);
	chain_free(rc);
}

/* -------------------------------------------------------------------------
 * rollback of a partially-shared build
 * ------------------------------------------------------------------------- */

static void test_rollback(void)
{
	struct fpm_nhg_staging newq = {};
	struct fpm_nhg_del_queue delq = {};
	struct fpm_nhg_route_key ka;
	struct nexthop *ra, *rb;
	struct fpm_dplane_nhg *topa, *topb, *lbref;
	uint32_t lb_ref0;
	uint64_t created0;

	printf("--- test_rollback\n");
	tables_reset();

	ra = recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.1/32", 5));
	topa = fpm_nhg_build(&t, ra, &newq);
	CHECK(topa != NULL, "build A failed");
	if (!topa || topa->num_children != 1) {
		chain_free(ra);
		return;
	}
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
	CHECK(topb != NULL && topb != topa, "B top must be a new object");
	CHECK(t.obj_created - created0 == 2, "created +%" PRIu64,
	      t.obj_created - created0);
	CHECK(lbref->refcount == lb_ref0 + 1, "shared L-B refcount %u",
	      lbref->refcount);

	/* A failed context (encode failure / no room) rolls the whole build
	 * back: new objects die, shared refcounts are restored.
	 */
	fpm_nhg_rollback(&t, &newq);
	fpm_nhg_staging_free(&newq);
	CHECK(lbref->refcount == lb_ref0, "refcount not restored: %u",
	      lbref->refcount);
	CHECK(t.obj_created == created0, "created not restored");
	CHECK(fpm_nhg_count(&t) == 3, "count %u", fpm_nhg_count(&t));

	fpm_nhg_route_forget(&t, &ka, &delq);
	fpm_nhg_del_queue_free(&delq);
	chain_free(ra);
	chain_free(rb);
}

/* -------------------------------------------------------------------------
 * id reuse
 * ------------------------------------------------------------------------- */

static void test_id_reuse(void)
{
	struct fpm_nhg_staging newq = {};
	struct fpm_nhg_del_queue delq = {};
	struct fpm_frame_batch batch = {};
	struct fpm_nhg_route_key ka;
	struct nexthop *ra, *rd;
	struct fpm_dplane_nhg *topa, *topd;
	uint32_t a, b, i, j;
	bool overlap = false;

	printf("--- test_id_reuse\n");
	tables_reset();

	/* Allocator basics: ids count up from 1, freed ids come back LIFO. */
	a = fpm_nhg_id_alloc(&t);
	b = fpm_nhg_id_alloc(&t);
	CHECK(a == 1 && b == 2, "fresh ids %u %u", a, b);
	fpm_nhg_id_free(&t, a);
	CHECK(fpm_nhg_id_alloc(&t) == a, "free list must be LIFO");

	/* One route in, then forgotten: its tree's ids are freed and its DELs
	 * are staged. The next context reuses those very ids, and its NEWs must
	 * follow the staged DELs in the batch.
	 */
	ra = recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.1/32", 5));
	topa = fpm_nhg_build(&t, ra, &newq);
	CHECK(topa != NULL, "build A failed");
	if (!topa) {
		chain_free(ra);
		return;
	}
	route_key(&ka, "172.16.1.0/24");
	fpm_nhg_route_commit(&t, &ka, topa, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);

	fpm_nhg_route_forget(&t, &ka, &delq);
	CHECK(fpm_nhg_del_queue_count(&delq) == 3, "DELs %u",
	      fpm_nhg_del_queue_count(&delq));

	rd = recursive_nh4("10.0.0.2/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.2/32", 6));
	topd = fpm_nhg_build(&t, rd, &newq);
	CHECK(topd != NULL, "build D failed");
	if (!topd) {
		fpm_nhg_del_queue_free(&delq);
		chain_free(ra);
		chain_free(rd);
		return;
	}

	/* The fresh tree pops the freed ids, so DEL and NEW collide. */
	CHECK(fpm_nhg_batch_add_pending_dels(&delq, &batch), "DEL encode failed");
	CHECK(fpm_nhg_batch_add_staging(&newq, &batch), "NEW encode failed");
	CHECK(batch.count == 6, "frames %u", batch.count);
	for (i = 0; i < 3 && i < batch.count; i++)
		CHECK(msg_type(&batch.frames[i]) == RTM_DELNHGFIB,
		      "frame %u must be a DEL", i);
	for (i = 3; i < batch.count; i++)
		CHECK(msg_type(&batch.frames[i]) == RTM_NEWNHGFIB,
		      "frame %u must be a NEW", i);

	for (i = 3; i < batch.count; i++) { /* NEWs start after the 3 DELs */
		uint32_t id = msg_nha_id(&batch.frames[i]);

		for (j = 0; j < 3; j++) {
			if (msg_nha_id(&batch.frames[j]) != id)
				continue;
			overlap = true;
			CHECK(j < i, "NEW id %u (frame %u) precedes its DEL", id,
			      i);
		}
	}
	CHECK(overlap, "no id was reused; test is vacuous");

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

static void test_assembly(void)
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

	printf("--- test_assembly\n");
	tables_reset();

	r1 = recursive_nh4("10.0.0.1/32", "10.0.0.0/24", 1000, 1,
			   leaf_nh4("192.168.1.1/32", 5));
	r2 = recursive_nh4("10.0.0.2/32", "10.0.0.0/24", 1001, 1,
			   leaf_nh4("192.168.1.2/32", 6));
	chain_append(&r1, r2);

	top = fpm_nhg_build(&t, r1, &newq);
	CHECK(top != NULL && newq.count == 5, "staged %u", newq.count);
	if (!top) {
		chain_free(r1);
		return;
	}

	/* Staging is child-before-parent. */
	for (i = 0; i < newq.count; i++) {
		const struct fpm_dplane_nhg *o = newq.objs[i];

		for (j = 0; j < o->num_children; j++) {
			uint32_t idx = UINT32_MAX;

			for (k = 0; k < newq.count; k++)
				if (newq.objs[k] == o->children[j].obj)
					idx = k;
			CHECK(idx < i, "member staged after its parent");
		}
	}

	/* The frames mirror the staging order, all NHGFIB. */
	CHECK(fpm_nhg_batch_add_staging(&newq, &batch), "encode failed");
	CHECK(batch.count == newq.count, "frames %u", batch.count);
	for (i = 0; i < batch.count && i < newq.count; i++) {
		CHECK(msg_type(&batch.frames[i]) == RTM_NEWNHGFIB,
		      "frame %u type %u", i, msg_type(&batch.frames[i]));
		CHECK(msg_nha_id(&batch.frames[i]) ==
			      newq.objs[i]->dplane_id,
		      "frame %u id mismatch", i);
	}

	/* The route frame is appended after the NHGFIBs: define before
	 * reference, NEWNHGFIB before NEWROUTE.
	 */
	rframe.n.nlmsg_len = NLMSG_LENGTH(0);
	rframe.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
	rframe.n.nlmsg_type = RTM_NEWROUTE;
	fpm_nhg_frame_batch_add(&batch, (uint8_t *)&rframe,
				NLMSG_ALIGN(rframe.n.nlmsg_len));
	CHECK(msg_type(&batch.frames[batch.count - 1]) == RTM_NEWROUTE,
	      "the route frame must come last");
	for (i = 0; i + 1 < batch.count; i++)
		CHECK(msg_type(&batch.frames[i]) == RTM_NEWNHGFIB,
		      "frame %u before the route is not NHGFIB", i);

	fpm_nhg_frame_batch_free(&batch);
	fpm_nhg_rollback(&t, &newq);
	fpm_nhg_staging_free(&newq);
	chain_free(r1);
}

/* -------------------------------------------------------------------------
 * leaf membership contributions
 * ------------------------------------------------------------------------- */

static void test_contrib(void)
{
	struct fpm_nhg_staging newq = {};
	struct fpm_nhg_del_queue delq = {};
	struct fpm_nhg_route_key ka, kb;
	struct nexthop *ra, *rb, *rc, *rd = NULL, *re = NULL;
	struct fpm_dplane_nhg *topa, *topb, *topc, *topd = NULL, *tope = NULL;
	struct fpm_dplane_nhg *leaf;
	struct prefix p1, p2;
	uint64_t created0;

	printf("--- test_contrib\n");
	tables_reset();

	if (str2prefix("10.0.0.0/24", &p1) != 1 ||
	    str2prefix("172.16.0.0/24", &p2) != 1)
		abort();

	/* A plain leaf reached through P1. A single non-recursive member
	 * makes the L-A the leaf itself.
	 */
	ra = leaf_nh4_res("192.168.1.1/32", 5, "10.0.0.0/24", 1000);
	topa = fpm_nhg_build(&t, ra, &newq);
	CHECK(topa != NULL && topa->level == FPM_NHG_L_C, "build A failed");
	if (!topa) {
		fpm_nhg_rollback(&t, &newq);
		fpm_nhg_staging_free(&newq);
		chain_free(ra);
		return;
	}
	leaf = topa;
	route_key(&ka, "172.16.1.0/24");
	fpm_nhg_route_commit(&t, &ka, topa, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);
	CHECK(leaf->res_member_count == 1 && leaf->res_members[0].count == 1 &&
		      prefix_same(&leaf->res_members[0].p, &p1),
	      "initial membership wrong");

	/* Re-committing an identical build moves nothing. */
	rb = leaf_nh4_res("192.168.1.1/32", 5, "10.0.0.0/24", 1000);
	topb = fpm_nhg_build(&t, rb, &newq);
	CHECK(topb == topa, "identical chain must dedupe");
	fpm_nhg_route_commit(&t, &ka, topb, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);
	CHECK(leaf->res_member_count == 1 && leaf->res_members[0].count == 1,
	      "idempotent commit moved the count");

	/* The same gate reached through P2 releases P1 and acquires P2, on
	 * the very same shared leaf.
	 */
	rc = leaf_nh4_res("192.168.1.1/32", 5, "172.16.0.0/24", 1000);
	topc = fpm_nhg_build(&t, rc, &newq);
	CHECK(topc == leaf, "gate unchanged: the leaf must be shared");
	if (!topc)
		goto out;
	fpm_nhg_route_commit(&t, &ka, topc, 1000, &delq, &newq);
	fpm_nhg_staging_free(&newq);
	CHECK(leaf->res_member_count == 1 && leaf->res_members[0].count == 1 &&
		      prefix_same(&leaf->res_members[0].p, &p2),
	      "rebind must release P1 and keep only P2");

	/* A rolled-back build leaves no trace, not even a fresh membership on
	 * a shared leaf.
	 */
	rd = leaf_nh4_res("192.168.1.1/32", 5, "10.0.0.0/24", 1000);
	created0 = t.obj_created;
	topd = fpm_nhg_build(&t, rd, &newq);
	CHECK(topd == leaf, "build D must reuse the leaf");
	fpm_nhg_rollback(&t, &newq);
	fpm_nhg_staging_free(&newq);
	CHECK(t.obj_created == created0, "rollback left objects behind");
	CHECK(leaf->res_member_count == 1 && leaf->res_members[0].count == 1 &&
		      prefix_same(&leaf->res_members[0].p, &p2),
	      "rollback must not touch the memberships");

	/* A second route on the same gate and prefix raises the count;
	 * forgetting one drops it back, forgetting the last unindexes the
	 * leaf entirely.
	 */
	re = leaf_nh4_res("192.168.1.1/32", 5, "172.16.0.0/24", 1000);
	tope = fpm_nhg_build(&t, re, &newq);
	CHECK(tope == leaf, "equal chain must dedupe");
	if (tope) {
		route_key(&kb, "172.16.2.0/24");
		fpm_nhg_route_commit(&t, &kb, tope, 1000, &delq, &newq);
		fpm_nhg_staging_free(&newq);
		CHECK(leaf->res_member_count == 1 &&
			      leaf->res_members[0].count == 2,
		      "shared count wrong");

		fpm_nhg_route_forget(&t, &kb, &delq);
		CHECK(leaf->res_member_count == 1 &&
			      leaf->res_members[0].count == 1,
		      "forget took too much");
	}

out:
	fpm_nhg_route_forget(&t, &ka, &delq);
	CHECK(t.obj_created == t.obj_deleted,
	      "created %" PRIu64 " deleted %" PRIu64,
	      t.obj_created, t.obj_deleted);

	fpm_nhg_del_queue_free(&delq);
	chain_free(ra);
	chain_free(rb);
	chain_free(rc);
	chain_free(rd);
	chain_free(re);
}

int main(void)
{
	zlog_aux_init("NHG_TEST: ", LOG_DEBUG);
	fpm_nhg_tables_init(&t);

	test_build();
	test_dedupe();
	test_rollback();
	test_id_reuse();
	test_assembly();
	test_contrib();

	fpm_nhg_tables_fini(&t);

	printf("test_fpm_nhg: %d checks, %d failure(s)\n", checks, failures);
	return failures ? 1 : 0;
}
