//go:build e2e

// Package e2e drives a running gribid on a SONiC box and checks the
// result in APPL_STATE_DB and ASIC_DB. Build with
//
//	CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go test -c -tags e2e -o e2e.test ./e2e
//
// and run on the switch:
//
//	./e2e.test -test.v -target localhost:9340 -redis localhost:6379 \
//	  -nexthops 10.0.0.57,10.0.0.59 -prefix 10.20.0.0/16 \
//	  -nexthops6 fc00::72,fc00::7a -prefix6 2001:db8:20::/64
//
// Add -vrf Vrfblue -vrf-nexthop <neighbor in Vrfblue> to cover a non-default
// VRF; gribid must have been started with that VRF in CONFIG_DB.
package e2e

import (
	"context"
	"flag"
	"sort"
	"strings"
	"testing"
	"time"

	"github.com/openconfig/gribigo/chk"
	"github.com/openconfig/gribigo/constants"
	"github.com/openconfig/gribigo/fluent"
	"github.com/openconfig/gribigo/server"
	"github.com/redis/go-redis/v9"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"

	spb "github.com/openconfig/gribi/v1/proto/service"
)

var (
	target      = flag.String("target", "localhost:9340", "gribid's gRIBI listener")
	redisAddr   = flag.String("redis", "localhost:6379", "SONiC Redis")
	nextHops    = flag.String("nexthops", "10.0.0.57,10.0.0.59", "resolved IPv4 next hops, comma separated (at least two)")
	prefix      = flag.String("prefix", "10.20.0.0/16", "IPv4 prefix nothing else advertises")
	nextHops6   = flag.String("nexthops6", "", "resolved IPv6 next hops; empty skips IPv6")
	prefix6     = flag.String("prefix6", "2001:db8:20::/64", "IPv6 prefix; used when -nexthops6 is set")
	unresolved  = flag.String("unresolved-nexthop", "192.0.2.1", "an address with no neighbor")
	badPrefix   = flag.String("unresolved-prefix", "10.21.0.0/16", "prefix used for the negative test")
	electionLow = flag.Uint64("election", 0, "initial election id; 0 uses the current Unix time so reruns against one agent never go backwards")
	timeout     = flag.Duration("timeout", 45*time.Second, "how long to wait for a batch to converge")
	hold        = flag.Duration("hold", 0, "pause after the initial add so the programmed state can be inspected by hand")
	vrf         = flag.String("vrf", "", "a VRF in CONFIG_DB to program a route in; empty skips the VRF test")
	vrfNextHop  = flag.String("vrf-nexthop", "", "a resolved neighbor in -vrf")
	vrfPrefix   = flag.String("vrf-prefix", "10.22.0.0/16", "IPv4 prefix for the VRF test")
)

const (
	defaultNI = server.DefaultNetworkInstanceName
	asicDB    = 1
	stateDB   = 14
)

var election uint64

func nextElection() uint64 {
	if election == 0 {
		election = *electionLow
		if election == 0 {
			election = uint64(time.Now().Unix())
		}
	}
	election++
	return election
}

func split(s string) []string {
	var out []string
	for _, p := range strings.Split(s, ",") {
		if p = strings.TrimSpace(p); p != "" {
			out = append(out, p)
		}
	}
	return out
}

func connect(t *testing.T) *fluent.GRIBIClient {
	t.Helper()
	ctx := context.Background()
	conn, err := grpc.NewClient(*target, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		t.Fatalf("dial %s: %v", *target, err)
	}
	t.Cleanup(func() { conn.Close() })
	c := fluent.NewClient()
	c.Connection().WithStub(spb.NewGRIBIClient(conn)).
		WithRedundancyMode(fluent.ElectedPrimaryClient).
		WithPersistence().
		WithFIBACK().
		WithInitialElectionID(nextElection(), 0)
	c.Start(ctx, t)
	t.Cleanup(func() { c.Stop(t) })
	c.StartSending(ctx, t)
	converge(t, c)
	return c
}

func converge(t *testing.T, c *fluent.GRIBIClient) {
	t.Helper()
	ctx, cancel := context.WithTimeout(context.Background(), *timeout)
	defer cancel()
	if err := c.Await(ctx, t); err != nil {
		for _, r := range c.Results(t) {
			t.Logf("result: %+v", r)
		}
		t.Fatalf("batch did not converge: %v", err)
	}
	for _, r := range c.Results(t) {
		t.Logf("result: %+v", r)
	}
}

func db(t *testing.T, n int) *redis.Client {
	t.Helper()
	rdb := redis.NewClient(&redis.Options{Addr: *redisAddr, DB: n})
	t.Cleanup(func() { rdb.Close() })
	return rdb
}

type route struct {
	ni     string // network instance; empty is the default
	v6     bool
	prefix string
	nhg    uint64
	nhs    []string
	idx    []uint64
}

func (r route) netInst() string {
	if r.ni == "" {
		return defaultNI
	}
	return r.ni
}

// key is the route's ROUTE_TABLE key: `<vrf>:<prefix>` outside the default VRF.
func (r route) key() string {
	if r.ni == "" {
		return r.prefix
	}
	return r.ni + ":" + r.prefix
}

func (r route) entry() fluent.GRIBIEntry {
	ni := r.netInst()
	if r.v6 {
		return fluent.IPv6Entry().WithNetworkInstance(ni).WithPrefix(r.prefix).WithNextHopGroup(r.nhg)
	}
	return fluent.IPv4Entry().WithNetworkInstance(ni).WithPrefix(r.prefix).WithNextHopGroup(r.nhg)
}

func (r route) group(idx ...uint64) fluent.GRIBIEntry {
	g := fluent.NextHopGroupEntry().WithNetworkInstance(r.netInst()).WithID(r.nhg)
	for _, i := range idx {
		g = g.AddNextHop(i, 1)
	}
	return g
}

func (r route) nextHops() []fluent.GRIBIEntry {
	var out []fluent.GRIBIEntry
	for i, ip := range r.nhs {
		out = append(out, fluent.NextHopEntry().WithNetworkInstance(r.netInst()).WithIndex(r.idx[i]).WithIPAddress(ip))
	}
	return out
}

func wantRouteResult(t *testing.T, c *fluent.GRIBIClient, r route, op constants.OpType, pr fluent.ProgrammingResult) {
	t.Helper()
	res := fluent.OperationResult().WithOperationType(op).WithProgrammingResult(pr)
	if r.v6 {
		res = res.WithIPv6Operation(r.prefix)
	} else {
		res = res.WithIPv4Operation(r.prefix)
	}
	chk.HasResult(t, c.Results(t), res.AsResult(), chk.IgnoreOperationID())
}

func routes() []route {
	nhs := split(*nextHops)
	if len(nhs) < 2 {
		panic("-nexthops needs at least two addresses")
	}
	out := []route{{prefix: *prefix, nhg: 10, nhs: nhs, idx: indexes(100, len(nhs))}}
	if nhs6 := split(*nextHops6); len(nhs6) > 0 {
		out = append(out, route{v6: true, prefix: *prefix6, nhg: 20, nhs: nhs6, idx: indexes(200, len(nhs6))})
	}
	return out
}

func indexes(base uint64, n int) []uint64 {
	out := make([]uint64, n)
	for i := range out {
		out[i] = base + uint64(i)
	}
	return out
}

// stateProtocol returns APPL_STATE_DB ROUTE_TABLE:<prefix>'s protocol field.
func stateProtocol(t *testing.T, prefix string) (string, bool) {
	t.Helper()
	res, err := db(t, stateDB).HGetAll(context.Background(), "ROUTE_TABLE:"+prefix).Result()
	if err != nil {
		t.Fatalf("APPL_STATE_DB: %v", err)
	}
	if len(res) == 0 {
		return "", false
	}
	return res["protocol"], true
}

// asicNextHops returns the next-hop addresses the ASIC route for prefix
// points at, resolved through SAI_OBJECT_TYPE_NEXT_HOP(_GROUP[_MEMBER]).
func asicNextHops(t *testing.T, prefix string) ([]string, bool) {
	t.Helper()
	ctx := context.Background()
	asic := db(t, asicDB)
	var routeKey string
	iter := asic.Scan(ctx, 0, `ASIC_STATE:SAI_OBJECT_TYPE_ROUTE_ENTRY:*"dest":"`+prefix+`"*`, 1000).Iterator()
	for iter.Next(ctx) {
		routeKey = iter.Val()
	}
	if err := iter.Err(); err != nil {
		t.Fatalf("ASIC_DB scan: %v", err)
	}
	if routeKey == "" {
		return nil, false
	}
	nhOID, err := asic.HGet(ctx, routeKey, "SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID").Result()
	if err != nil {
		t.Fatalf("ASIC_DB %s: %v", routeKey, err)
	}
	if ip, err := asic.HGet(ctx, "ASIC_STATE:SAI_OBJECT_TYPE_NEXT_HOP:"+nhOID, "SAI_NEXT_HOP_ATTR_IP").Result(); err == nil {
		return []string{ip}, true
	}
	var ips []string
	iter = asic.Scan(ctx, 0, "ASIC_STATE:SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER:*", 1000).Iterator()
	for iter.Next(ctx) {
		m, err := asic.HGetAll(ctx, iter.Val()).Result()
		if err != nil {
			t.Fatalf("ASIC_DB %s: %v", iter.Val(), err)
		}
		if m["SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_GROUP_ID"] != nhOID {
			continue
		}
		ip, err := asic.HGet(ctx, "ASIC_STATE:SAI_OBJECT_TYPE_NEXT_HOP:"+m["SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_ID"], "SAI_NEXT_HOP_ATTR_IP").Result()
		if err != nil {
			t.Fatalf("ASIC_DB next hop of member %s: %v", iter.Val(), err)
		}
		ips = append(ips, ip)
	}
	sort.Strings(ips)
	return ips, true
}

func sorted(s []string) []string {
	out := append([]string(nil), s...)
	sort.Strings(out)
	return out
}

func TestAddReplaceDelete(t *testing.T) {
	c := connect(t)
	rs := routes()

	var add []fluent.GRIBIEntry
	for _, r := range rs {
		add = append(add, r.nextHops()...)
		add = append(add, r.group(r.idx...), r.entry())
	}
	c.Modify().AddEntry(t, add...)
	converge(t, c)
	for _, r := range rs {
		wantRouteResult(t, c, r, constants.Add, fluent.InstalledInFIB)
		if proto, ok := stateProtocol(t, r.prefix); !ok || proto != "gribi" {
			t.Errorf("%s: APPL_STATE_DB protocol = %q,%v; want gribi", r.prefix, proto, ok)
		}
		if ips, ok := asicNextHops(t, r.prefix); !ok || strings.Join(ips, ",") != strings.Join(sorted(r.nhs), ",") {
			t.Errorf("%s: ASIC next hops = %v,%v; want %v", r.prefix, ips, ok, sorted(r.nhs))
		}
		if n, err := db(t, 0).Exists(context.Background(), "ROUTE_TABLE:"+r.prefix).Result(); err != nil || n != 0 {
			t.Errorf("%s: APPL_DB ROUTE_TABLE mirror exists=%d err=%v; ZMQ routes must not appear in APPL_DB", r.prefix, n, err)
		}
	}

	if *hold > 0 {
		t.Logf("holding %s with routes programmed; inspect APPL_STATE_DB and ASIC_DB now", *hold)
		time.Sleep(*hold)
	}

	// Shrink every group to its first next hop: the routes must follow.
	var replace []fluent.GRIBIEntry
	for _, r := range rs {
		replace = append(replace, r.group(r.idx[0]))
	}
	c.Modify().ReplaceEntry(t, replace...)
	converge(t, c)
	for _, r := range rs {
		chk.HasResult(t, c.Results(t), fluent.OperationResult().WithNextHopGroupOperation(r.nhg).WithOperationType(constants.Replace).WithProgrammingResult(fluent.InstalledInFIB).AsResult(), chk.IgnoreOperationID())
		deadline := time.Now().Add(10 * time.Second)
		for {
			ips, ok := asicNextHops(t, r.prefix)
			if ok && len(ips) == 1 && ips[0] == r.nhs[0] {
				break
			}
			if time.Now().After(deadline) {
				t.Fatalf("%s: ASIC next hops after group replace = %v,%v; want [%s]", r.prefix, ips, ok, r.nhs[0])
			}
			time.Sleep(200 * time.Millisecond)
		}
	}

	var del []fluent.GRIBIEntry
	for _, r := range rs {
		del = append(del, r.entry())
	}
	for _, r := range rs {
		del = append(del, r.group(r.idx[0]))
		del = append(del, r.nextHops()...)
	}
	c.Modify().DeleteEntry(t, del...)
	converge(t, c)
	for _, r := range rs {
		wantRouteResult(t, c, r, constants.Delete, fluent.InstalledInFIB)
		if _, ok := stateProtocol(t, r.prefix); ok {
			t.Errorf("%s: APPL_STATE_DB entry survived the delete", r.prefix)
		}
		if ips, ok := asicNextHops(t, r.prefix); ok {
			t.Errorf("%s: ASIC route survived the delete: %v", r.prefix, ips)
		}
	}
}

func TestUnresolvedNextHopIsFIBFailed(t *testing.T) {
	c := connect(t)
	r := route{prefix: *badPrefix, nhg: 30, nhs: []string{*unresolved}, idx: []uint64{300}}
	c.Modify().AddEntry(t, append(r.nextHops(), r.group(300), r.entry())...)
	converge(t, c)

	var failed bool
	for _, res := range c.Results(t) {
		if res.Details != nil && res.Details.IPv4Prefix == r.prefix && res.ProgrammingResult == spb.AFTResult_FIB_FAILED {
			failed = true
			t.Logf("FIB_FAILED as expected: %s", res.ServerError)
		}
	}
	if !failed {
		t.Fatal("route over an unresolved next hop was not reported FIB_FAILED")
	}
	if _, ok := stateProtocol(t, r.prefix); ok {
		t.Error("APPL_STATE_DB has an entry for a route that failed")
	}
	if ips, ok := asicNextHops(t, r.prefix); ok {
		t.Errorf("ASIC has a route that failed: %v", ips)
	}

	// Cleanup must not hang: orchagent never had the route, so no response is
	// coming, and the agent knows that.
	c.Modify().DeleteEntry(t, r.entry(), r.group(300))
	c.Modify().DeleteEntry(t, r.nextHops()...)
	start := time.Now()
	converge(t, c)
	wantRouteResult(t, c, r, constants.Delete, fluent.InstalledInFIB)
	if d := time.Since(start); d > 10*time.Second {
		t.Errorf("deleting a never-installed route took %s", d)
	}
}

func TestVRFAddDelete(t *testing.T) {
	if *vrf == "" {
		t.Skip("-vrf not set")
	}
	if *vrfNextHop == "" {
		t.Fatal("-vrf needs -vrf-nexthop")
	}
	c := connect(t)
	r := route{ni: *vrf, prefix: *vrfPrefix, nhg: 40, nhs: []string{*vrfNextHop}, idx: []uint64{400}}

	c.Modify().AddEntry(t, append(r.nextHops(), r.group(r.idx...), r.entry())...)
	converge(t, c)
	wantRouteResult(t, c, r, constants.Add, fluent.InstalledInFIB)
	if proto, ok := stateProtocol(t, r.key()); !ok || proto != "gribi" {
		t.Errorf("%s: APPL_STATE_DB protocol = %q,%v; want gribi", r.key(), proto, ok)
	}
	if ips, ok := asicNextHops(t, r.prefix); !ok || strings.Join(ips, ",") != *vrfNextHop {
		t.Errorf("%s: ASIC next hops = %v,%v; want %s", r.key(), ips, ok, *vrfNextHop)
	}

	c.Modify().DeleteEntry(t, r.entry(), r.group(r.idx...))
	c.Modify().DeleteEntry(t, r.nextHops()...)
	converge(t, c)
	wantRouteResult(t, c, r, constants.Delete, fluent.InstalledInFIB)
	if _, ok := stateProtocol(t, r.key()); ok {
		t.Errorf("%s: still in APPL_STATE_DB after delete", r.key())
	}
}
