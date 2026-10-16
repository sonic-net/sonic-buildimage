package adapter

import (
	"context"
	"errors"
	"fmt"
	"log/slog"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/google/go-cmp/cmp"
	"github.com/openconfig/gribigo/aft"
	"github.com/openconfig/gribigo/constants"
	"github.com/openconfig/gribigo/server"
	"github.com/openconfig/ygot/ygot"

	"github.com/sonic-net/sonic-gribi/fibtrack"
	"github.com/sonic-net/sonic-gribi/swsszmq"
)

const ni = server.DefaultNetworkInstanceName

type frame struct {
	table  string
	tuples []swsszmq.Tuple
}

type fakeSender struct {
	mu     sync.Mutex
	frames []frame
	err    error
}

func (f *fakeSender) Send(_ context.Context, table string, tuples ...swsszmq.Tuple) error {
	f.mu.Lock()
	defer f.mu.Unlock()
	if f.err != nil {
		return f.err
	}
	f.frames = append(f.frames, frame{table: table, tuples: tuples})
	return nil
}

func (f *fakeSender) all() []frame {
	f.mu.Lock()
	defer f.mu.Unlock()
	return append([]frame(nil), f.frames...)
}

// last fails the test unless exactly want frames were sent, then returns the
// newest one.
func (f *fakeSender) last(t *testing.T, want int) frame {
	t.Helper()
	fr := f.all()
	if len(fr) != want {
		t.Fatalf("got %d frames, want %d: %+v", len(fr), want, fr)
	}
	if fr[want-1].table != RouteTable {
		t.Fatalf("frame went to %q, want %q", fr[want-1].table, RouteTable)
	}
	return fr[want-1]
}

type fakeResolver map[string]string

// fakeResolver is keyed by address, or by "<vrf>|<address>" outside the
// default VRF.
func (r fakeResolver) Interface(_ context.Context, vrf, ip string) (string, bool) {
	if vrf != "" {
		ip = vrf + "|" + ip
	}
	name, ok := r[ip]
	return name, ok
}

type rig struct {
	p       *Programmer
	sender  *fakeSender
	tracker *fibtrack.Tracker
}

func newRig(t *testing.T) *rig { t.Helper(); return newBatchRig(t, Batch{}) }

// newBatchRig is newRig with explicit batching. Its hooks are called through
// r.p.OnChange, not the flushing helpers below.
func newBatchRig(t *testing.T, batch Batch) *rig {
	t.Helper()
	sender := &fakeSender{}
	tracker := fibtrack.New()
	resolver := fakeResolver{
		"10.0.0.57": "PortChannel101",
		"10.0.0.59": "PortChannel102",
		"10.0.0.61": "PortChannel103",
		"fc00::72":  "PortChannel101",
		"fc00::7a":  "PortChannel103",

		"Vrfblue|10.0.0.57": "Ethernet8",
	}
	log := slog.New(slog.NewTextHandler(testWriter{t}, &slog.HandlerOptions{Level: slog.LevelDebug}))
	return &rig{p: New(sender, resolver, tracker, batch, log), sender: sender, tracker: tracker}
}

type testWriter struct{ t *testing.T }

func (w testWriter) Write(p []byte) (int, error) {
	w.t.Log(strings.TrimSpace(string(p)))
	return len(p), nil
}

// on runs one hook and flushes, as the end of a one-operation request would,
// so each hook's writes arrive as one frame.
func (r *rig) on(op constants.OpType, ts int64, n string, e ygot.ValidatedGoStruct) {
	r.p.OnChange(op, ts, n, e)
	r.p.Flush()
}

func (r *rig) add(e ygot.ValidatedGoStruct)             { r.on(constants.Add, 0, ni, e) }
func (r *rig) del(e ygot.ValidatedGoStruct)             { r.on(constants.Delete, 0, ni, e) }
func (r *rig) addIn(n string, e ygot.ValidatedGoStruct) { r.on(constants.Add, 0, n, e) }

// outcome returns the tracker's answer for key, or the error string when the
// slot is still open (so a test can tell "failed at once" from "sent").
func (r *rig) outcome(key string) error {
	ctx, cancel := context.WithTimeout(context.Background(), 50*time.Millisecond)
	defer cancel()
	return r.tracker.Await(ctx, key)
}

func nh(idx uint64, ip string) *aft.Afts_NextHop {
	return &aft.Afts_NextHop{Index: ygot.Uint64(idx), IpAddress: ygot.String(ip)}
}

func nhIface(idx uint64, ip, iface string) *aft.Afts_NextHop {
	n := nh(idx, ip)
	n.InterfaceRef = &aft.Afts_NextHop_InterfaceRef{Interface: ygot.String(iface)}
	return n
}

type mem struct{ idx, w uint64 }

func nhg(id uint64, members ...mem) *aft.Afts_NextHopGroup {
	g := &aft.Afts_NextHopGroup{Id: ygot.Uint64(id), NextHop: map[uint64]*aft.Afts_NextHopGroup_NextHop{}}
	for _, m := range members {
		g.NextHop[m.idx] = &aft.Afts_NextHopGroup_NextHop{Index: ygot.Uint64(m.idx), Weight: ygot.Uint64(m.w)}
	}
	return g
}

func v4(prefix string, gid uint64) *aft.Afts_Ipv4Entry {
	return &aft.Afts_Ipv4Entry{Prefix: ygot.String(prefix), NextHopGroup: ygot.Uint64(gid)}
}

func v6(prefix string, gid uint64) *aft.Afts_Ipv6Entry {
	return &aft.Afts_Ipv6Entry{Prefix: ygot.String(prefix), NextHopGroup: ygot.Uint64(gid)}
}

func set(key, nexthop, ifname, weight string) swsszmq.Tuple {
	return swsszmq.Tuple{Key: key, Fields: []swsszmq.FieldValue{
		{Field: "nexthop", Value: nexthop},
		{Field: "ifname", Value: ifname},
		{Field: "weight", Value: weight},
		{Field: "protocol", Value: Protocol},
	}}
}

func wantTuples(t *testing.T, got frame, want ...swsszmq.Tuple) {
	t.Helper()
	if diff := cmp.Diff(want, got.tuples); diff != "" {
		t.Fatalf("tuples differ (-want +got):\n%s", diff)
	}
}

func TestSingleNextHop(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.57"))
	r.add(nhg(10, mem{1, 1}))
	if got := r.sender.all(); len(got) != 0 {
		t.Fatalf("next hop and group with no routes sent %d frames", len(got))
	}
	r.add(v4("10.20.0.0/16", 10))
	wantTuples(t, r.sender.last(t, 1), set("10.20.0.0/16", "10.0.0.57", "PortChannel101", "1"))

	if err := r.outcome("10.20.0.0/16"); !errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("slot should be open until orchagent answers, got %v", err)
	}
	r.tracker.Done("10.20.0.0/16", nil)
	if err := r.outcome("10.20.0.0/16"); err != nil {
		t.Fatalf("after Done: %v", err)
	}
}

func TestECMPSortedWeighted(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.59"))
	r.add(nh(2, "10.0.0.57"))
	r.add(nhg(10, mem{1, 2}, mem{2, 3}))
	r.add(v4("10.20.0.0/16", 10))
	wantTuples(t, r.sender.last(t, 1), set("10.20.0.0/16", "10.0.0.57,10.0.0.59", "PortChannel101,PortChannel102", "3,2"))
}

func TestUnsetWeightIsOne(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.57"))
	g := nhg(10)
	g.NextHop[1] = &aft.Afts_NextHopGroup_NextHop{Index: ygot.Uint64(1)}
	r.add(g)
	r.add(v4("10.20.0.0/16", 10))
	wantTuples(t, r.sender.last(t, 1), set("10.20.0.0/16", "10.0.0.57", "PortChannel101", "1"))
}

func TestInterfaceRefPreferred(t *testing.T) {
	r := newRig(t)
	r.add(nhIface(1, "10.0.0.57", "Ethernet8"))     // resolver would say PortChannel101
	r.add(nhIface(2, "198.51.100.1", "Ethernet12")) // resolver knows nothing about it
	r.add(nhg(10, mem{1, 1}, mem{2, 1}))
	r.add(v4("10.20.0.0/16", 10))
	wantTuples(t, r.sender.last(t, 1), set("10.20.0.0/16", "10.0.0.57,198.51.100.1", "Ethernet8,Ethernet12", "1,1"))
}

func TestUnresolvedLegFailsRoute(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.57"))
	r.add(nh(2, "192.0.2.1"))
	r.add(nhg(10, mem{1, 1}, mem{2, 1}))
	r.add(v4("10.20.0.0/16", 10))
	if got := r.sender.all(); len(got) != 0 {
		t.Fatalf("a route with an unresolved leg was sent: %+v", got)
	}
	err := r.outcome("10.20.0.0/16")
	if err == nil || !strings.Contains(err.Error(), "192.0.2.1") {
		t.Fatalf("outcome = %v, want an error naming the unresolved next hop", err)
	}
}

func TestEmptyGroupFailsRoute(t *testing.T) {
	r := newRig(t)
	r.add(nhg(10))
	r.add(v4("10.20.0.0/16", 10))
	if got := r.sender.all(); len(got) != 0 {
		t.Fatalf("a route over an empty group was sent: %+v", got)
	}
	if err := r.outcome("10.20.0.0/16"); err == nil || errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("outcome = %v, want an immediate error", err)
	}
}

func TestNextHopWithoutAddressFailsRoute(t *testing.T) {
	r := newRig(t)
	r.add(&aft.Afts_NextHop{Index: ygot.Uint64(1), InterfaceRef: &aft.Afts_NextHop_InterfaceRef{Interface: ygot.String("Ethernet8")}})
	r.add(nhg(10, mem{1, 1}))
	r.add(v4("10.20.0.0/16", 10))
	if got := r.sender.all(); len(got) != 0 {
		t.Fatalf("an interface-only next hop was sent: %+v", got)
	}
	if err := r.outcome("10.20.0.0/16"); err == nil || errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("outcome = %v, want an immediate error", err)
	}
}

func TestReplaceRouteToOtherGroup(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.57"))
	r.add(nh(2, "10.0.0.59"))
	r.add(nhg(10, mem{1, 1}))
	r.add(nhg(11, mem{2, 1}))
	r.add(v4("10.20.0.0/16", 10))
	wantTuples(t, r.sender.last(t, 1), set("10.20.0.0/16", "10.0.0.57", "PortChannel101", "1"))
	r.on(constants.Replace, 0, ni, v4("10.20.0.0/16", 11))
	wantTuples(t, r.sender.last(t, 2), set("10.20.0.0/16", "10.0.0.59", "PortChannel102", "1"))

	// The route no longer depends on group 10 ...
	r.add(nhg(10, mem{1, 5}))
	r.sender.last(t, 2)
	// ... but does on group 11.
	r.add(nhg(11, mem{2, 7}))
	wantTuples(t, r.sender.last(t, 3), set("10.20.0.0/16", "10.0.0.59", "PortChannel102", "7"))
}

func TestNHGReplaceReprogramsDependents(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.57"))
	r.add(nh(2, "10.0.0.59"))
	r.add(nhg(10, mem{1, 1}))
	r.add(nhg(11, mem{1, 1}))
	r.add(v4("10.20.0.0/16", 10))
	r.add(v4("10.21.0.0/16", 10))
	r.add(v4("10.22.0.0/16", 11))
	r.sender.last(t, 3)

	r.on(constants.Replace, 0, ni, nhg(10, mem{1, 1}, mem{2, 1}))
	// One frame carrying both dependents, sorted, and nothing for 10.22.
	wantTuples(t, r.sender.last(t, 4),
		set("10.20.0.0/16", "10.0.0.57,10.0.0.59", "PortChannel101,PortChannel102", "1,1"),
		set("10.21.0.0/16", "10.0.0.57,10.0.0.59", "PortChannel101,PortChannel102", "1,1"),
	)
	for _, k := range []string{"10.20.0.0/16", "10.21.0.0/16"} {
		r.tracker.Done(k, nil)
		if err := r.outcome(k); err != nil {
			t.Fatalf("%s: %v", k, err)
		}
	}
}

func TestNextHopReplaceReprogramsDependents(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.57"))
	r.add(nh(2, "10.0.0.61"))
	r.add(nhg(10, mem{1, 1}))
	r.add(nhg(11, mem{1, 1}))
	r.add(nhg(12, mem{2, 1}))
	r.add(v4("10.20.0.0/16", 10))
	r.add(v4("10.21.0.0/16", 11))
	r.add(v4("10.22.0.0/16", 12))
	r.sender.last(t, 3)

	r.on(constants.Replace, 0, ni, nh(1, "10.0.0.59"))
	wantTuples(t, r.sender.last(t, 4),
		set("10.20.0.0/16", "10.0.0.59", "PortChannel102", "1"),
		set("10.21.0.0/16", "10.0.0.59", "PortChannel102", "1"),
	)
}

func TestNextHopChangeThatBreaksOneRouteStillSendsTheRest(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.57"))
	r.add(nh(2, "10.0.0.59"))
	r.add(nhg(10, mem{1, 1}))
	r.add(nhg(11, mem{1, 1}, mem{2, 1}))
	r.add(v4("10.20.0.0/16", 10))
	r.add(v4("10.21.0.0/16", 11))
	r.sender.last(t, 2)

	// Group 11 now also names a next hop with no neighbor: 10.21 fails,
	// 10.20 must still go out.
	r.add(nh(3, "192.0.2.1"))
	r.on(constants.Replace, 0, ni, nhg(11, mem{1, 1}, mem{3, 1}))
	if got := r.sender.all(); len(got) != 2 {
		t.Fatalf("got %d frames, want 2: %+v", len(got), got)
	}
	if err := r.outcome("10.21.0.0/16"); err == nil || errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("10.21 outcome = %v, want an immediate error", err)
	}
	// And a next-hop change on nh 1 reprograms only the renderable route.
	r.on(constants.Replace, 0, ni, nh(1, "10.0.0.61"))
	wantTuples(t, r.sender.last(t, 3), set("10.20.0.0/16", "10.0.0.61", "PortChannel103", "1"))
}

func TestDeleteSendsDel(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.57"))
	r.add(nhg(10, mem{1, 1}))
	r.add(v4("10.20.0.0/16", 10))
	r.sender.last(t, 1)
	r.tracker.Done("10.20.0.0/16", nil)

	r.del(v4("10.20.0.0/16", 10))
	wantTuples(t, r.sender.last(t, 2), swsszmq.Tuple{Key: "10.20.0.0/16"})
	// orchagent held the route, so the DEL is answered and must be waited for.
	if err := r.outcome("10.20.0.0/16"); !errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("DEL of an installed route resolved early: %v", err)
	}
	r.tracker.Done("10.20.0.0/16", nil)
	if err := r.outcome("10.20.0.0/16"); err != nil {
		t.Fatal(err)
	}

	// The route is no longer a dependent of group 10.
	r.add(nhg(10, mem{1, 4}))
	r.sender.last(t, 2)
}

func TestDeleteOfNeverProgrammedRouteResolvesAtOnce(t *testing.T) {
	r := newRig(t)
	r.add(nhg(10))
	r.add(v4("10.20.0.0/16", 10)) // fails: empty group
	r.del(v4("10.20.0.0/16", 10))
	wantTuples(t, r.sender.last(t, 1), swsszmq.Tuple{Key: "10.20.0.0/16"})
	if err := r.outcome("10.20.0.0/16"); err != nil {
		t.Fatalf("DEL of a route orchagent never had should succeed at once, got %v", err)
	}
}

func TestVRFKey(t *testing.T) {
	r := newRig(t)
	r.addIn("blue", nh(1, "10.0.0.57"))
	r.addIn("blue", nhg(10, mem{1, 1}))
	r.addIn("blue", v4("10.20.0.0/16", 10))
	wantTuples(t, r.sender.last(t, 1), set("Vrfblue:10.20.0.0/16", "10.0.0.57", "Ethernet8", "1"))
	r.on(constants.Delete, 0, "blue", v4("10.20.0.0/16", 10))
	wantTuples(t, r.sender.last(t, 2), swsszmq.Tuple{Key: "Vrfblue:10.20.0.0/16"})
}

func TestVRFNeighborNotBorrowedFromDefault(t *testing.T) {
	r := newRig(t)
	// 10.0.0.59 is a neighbor only in the default VRF.
	r.addIn("blue", nh(1, "10.0.0.59"))
	r.addIn("blue", nhg(10, mem{1, 1}))
	r.addIn("blue", v4("10.20.0.0/16", 10))
	if got := r.sender.all(); len(got) != 0 {
		t.Fatalf("route sent with a next hop from another VRF: %+v", got)
	}
	if err := r.outcome("Vrfblue:10.20.0.0/16"); err == nil || !strings.Contains(err.Error(), "in Vrfblue") {
		t.Fatalf("outcome = %v, want an unresolved-neighbor error naming Vrfblue", err)
	}
}

func TestCrossNIGroupRejected(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.57"))
	r.add(nhg(10, mem{1, 1}))
	e := v4("10.20.0.0/16", 10)
	e.NextHopGroupNetworkInstance = ygot.String("blue")
	r.add(e)
	if got := r.sender.all(); len(got) != 0 {
		t.Fatalf("cross-instance route was sent: %+v", got)
	}
	if err := r.outcome("10.20.0.0/16"); err == nil || errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("outcome = %v, want an immediate error", err)
	}
	// Naming the route's own instance explicitly is fine.
	e.NextHopGroupNetworkInstance = ygot.String(ni)
	r.add(e)
	r.sender.last(t, 1)
}

func TestIPv6(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "fc00::7a"))
	r.add(nh(2, "fc00::72"))
	r.add(nhg(10, mem{1, 1}, mem{2, 1}))
	r.add(v6("2001:db8:20::/64", 10))
	wantTuples(t, r.sender.last(t, 1), set("2001:db8:20::/64", "fc00::72,fc00::7a", "PortChannel101,PortChannel103", "1,1"))
	r.del(v6("2001:db8:20::/64", 10))
	wantTuples(t, r.sender.last(t, 2), swsszmq.Tuple{Key: "2001:db8:20::/64"})
}

func TestLabelEntryIgnored(t *testing.T) {
	r := newRig(t)
	r.add(&aft.Afts_LabelEntry{Label: aft.UnionUint32(100), NextHopGroup: ygot.Uint64(10)})
	r.del(&aft.Afts_LabelEntry{Label: aft.UnionUint32(100)})
	if got := r.sender.all(); len(got) != 0 {
		t.Fatalf("label entry produced frames: %+v", got)
	}
}

func TestFlushDeletesAll(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.57"))
	r.add(nhg(10, mem{1, 1}))
	r.add(v4("10.20.0.0/16", 10))
	r.add(v6("2001:db8:20::/64", 10))
	r.sender.last(t, 2)

	// gribigo's Flush fires Delete for routes, then groups, then next hops.
	r.del(v4("10.20.0.0/16", 10))
	r.del(v6("2001:db8:20::/64", 10))
	r.del(nhg(10, mem{1, 1}))
	r.del(nh(1, "10.0.0.57"))
	frames := r.sender.all()
	if len(frames) != 4 {
		t.Fatalf("got %d frames, want 4: %+v", len(frames), frames)
	}
	wantTuples(t, frames[2], swsszmq.Tuple{Key: "10.20.0.0/16"})
	wantTuples(t, frames[3], swsszmq.Tuple{Key: "2001:db8:20::/64"})

	// The mirror is clean: a fresh install works and nothing stale is re-sent.
	r.add(nh(1, "10.0.0.59"))
	r.add(nhg(10, mem{1, 1}))
	r.sender.last(t, 4)
	r.add(v4("10.20.0.0/16", 10))
	wantTuples(t, r.sender.last(t, 5), set("10.20.0.0/16", "10.0.0.59", "PortChannel102", "1"))
}

func TestSendErrorFailsRoute(t *testing.T) {
	r := newRig(t)
	r.add(nh(1, "10.0.0.57"))
	r.add(nhg(10, mem{1, 1}))
	r.sender.err = errors.New("zmq: send timeout")
	r.add(v4("10.20.0.0/16", 10))
	err := r.outcome("10.20.0.0/16")
	if err == nil || !strings.Contains(err.Error(), "send timeout") {
		t.Fatalf("outcome = %v, want the send error", err)
	}
}

func TestConcurrentHooks(t *testing.T) {
	r := newRig(t)
	const clients = 8
	const routes = 25
	var wg sync.WaitGroup
	for c := 0; c < clients; c++ {
		wg.Add(1)
		go func(c int) {
			defer wg.Done()
			n := fmt.Sprintf("vrf%d", c)
			r.addIn(n, nhIface(1, "10.0.0.57", "Ethernet0"))
			r.addIn(n, nhIface(2, "10.0.0.59", "Ethernet4"))
			r.addIn(n, nhg(10, mem{1, 1}, mem{2, 1}))
			for i := 0; i < routes; i++ {
				r.addIn(n, v4(fmt.Sprintf("10.%d.%d.0/24", c, i), 10))
			}
			r.on(constants.Replace, 0, n, nhg(10, mem{1, 1}))
			for i := 0; i < routes; i++ {
				r.on(constants.Delete, 0, n, v4(fmt.Sprintf("10.%d.%d.0/24", c, i), 10))
			}
		}(c)
	}
	wg.Wait()
	// Per client: routes SETs, a reprogram of every route, routes DELs. A
	// flush from one client may carry another's writes, so count tuples.
	got := 0
	for _, f := range r.sender.all() {
		got += len(f.tuples)
	}
	if got != clients*3*routes {
		t.Fatalf("sent %d tuples, want %d", got, clients*3*routes)
	}
}

// install adds nh 1 and group 10 without sending anything.
func (r *rig) install() {
	r.p.OnChange(constants.Add, 0, ni, nh(1, "10.0.0.57"))
	r.p.OnChange(constants.Add, 0, ni, nhg(10, mem{1, 1}))
}

func route(i int) string { return fmt.Sprintf("10.%d.%d.0/24", 20+i/256, i%256) }

func TestWritesWaitForFlush(t *testing.T) {
	r := newBatchRig(t, Batch{})
	r.install()
	const n = 300
	var want []swsszmq.Tuple
	for i := 0; i < n; i++ {
		r.p.OnChange(constants.Add, 0, ni, v4(route(i), 10))
		want = append(want, set(route(i), "10.0.0.57", "PortChannel101", "1"))
	}
	if got := r.sender.all(); len(got) != 0 {
		t.Fatalf("%d frames sent before Flush", len(got))
	}
	r.p.Flush()
	wantTuples(t, r.sender.last(t, 1), want...)
	r.p.Flush() // nothing queued: no empty frame
	r.sender.last(t, 1)
}

func TestSameKeyKeepsOrderInOneFrame(t *testing.T) {
	r := newBatchRig(t, Batch{})
	r.install()
	r.p.OnChange(constants.Add, 0, ni, v4("10.20.0.0/16", 10))
	r.p.OnChange(constants.Delete, 0, ni, v4("10.20.0.0/16", 10))
	r.p.OnChange(constants.Add, 0, ni, v4("10.20.0.0/16", 10))
	r.p.Flush()
	s := set("10.20.0.0/16", "10.0.0.57", "PortChannel101", "1")
	wantTuples(t, r.sender.last(t, 1), s, swsszmq.Tuple{Key: "10.20.0.0/16"}, s)
}

func TestReprogramJoinsTheQueue(t *testing.T) {
	r := newBatchRig(t, Batch{})
	r.install()
	r.p.OnChange(constants.Add, 0, ni, nh(2, "10.0.0.59"))
	r.p.OnChange(constants.Add, 0, ni, v4("10.20.0.0/16", 10))
	r.p.OnChange(constants.Replace, 0, ni, nhg(10, mem{1, 1}, mem{2, 1}))
	r.p.OnChange(constants.Add, 0, ni, v4("10.21.0.0/16", 10))
	r.p.Flush()
	ecmp := func(k string) swsszmq.Tuple {
		return set(k, "10.0.0.57,10.0.0.59", "PortChannel101,PortChannel102", "1,1")
	}
	wantTuples(t, r.sender.last(t, 1),
		set("10.20.0.0/16", "10.0.0.57", "PortChannel101", "1"), ecmp("10.20.0.0/16"), ecmp("10.21.0.0/16"))
}

func TestFullBatchIsSentAtOnce(t *testing.T) {
	r := newBatchRig(t, Batch{Max: 4})
	r.install()
	for i := 0; i < 10; i++ {
		r.p.OnChange(constants.Add, 0, ni, v4(route(i), 10))
	}
	frames := r.sender.all()
	if len(frames) != 2 || len(frames[0].tuples) != 4 || len(frames[1].tuples) != 4 {
		t.Fatalf("before Flush got %+v, want two frames of 4", frames)
	}
	r.p.Flush()
	if f := r.sender.last(t, 3); len(f.tuples) != 2 {
		t.Fatalf("last frame has %d tuples, want 2", len(f.tuples))
	}
}

func TestFrameStaysUnderByteLimit(t *testing.T) {
	r := newBatchRig(t, Batch{Max: 1 << 20})
	r.install()
	one := set(route(0), "10.0.0.57", "PortChannel101", "1").EncodedSize()
	n := maxBatchBytes/one + 10
	for i := 0; i < n; i++ {
		r.p.OnChange(constants.Add, 0, ni, v4(fmt.Sprintf("10.%d.%d.%d/32", i>>16&255, i>>8&255, i&255), 10))
	}
	r.p.Flush()
	frames := r.sender.all()
	total := 0
	for _, f := range frames {
		if _, err := swsszmq.Encode("APPL_DB", RouteTable, f.tuples); err != nil {
			t.Fatal(err)
		}
		size := 0
		for _, tu := range f.tuples {
			size += tu.EncodedSize()
		}
		if size > maxBatchBytes {
			t.Fatalf("frame of %d bytes, limit %d", size, maxBatchBytes)
		}
		total += len(f.tuples)
	}
	if len(frames) < 2 || total != n {
		t.Fatalf("got %d frames carrying %d tuples, want >= 2 frames carrying %d", len(frames), total, n)
	}
}

func TestLingerFlushesWithoutFlush(t *testing.T) {
	r := newBatchRig(t, Batch{Linger: 20 * time.Millisecond})
	r.install()
	start := time.Now()
	r.p.OnChange(constants.Add, 0, ni, v4("10.20.0.0/16", 10))
	r.p.OnChange(constants.Add, 0, ni, v4("10.21.0.0/16", 10))
	for len(r.sender.all()) == 0 {
		if time.Since(start) > 2*time.Second {
			t.Fatal("linger never flushed")
		}
		time.Sleep(time.Millisecond)
	}
	if waited := time.Since(start); waited < 20*time.Millisecond {
		t.Fatalf("flushed after %v, before the 20ms linger", waited)
	}
	if f := r.sender.last(t, 1); len(f.tuples) != 2 {
		t.Fatalf("linger frame has %d tuples, want 2", len(f.tuples))
	}
	// The timer re-arms for the next batch.
	r.p.OnChange(constants.Add, 0, ni, v4("10.22.0.0/16", 10))
	deadline := time.Now().Add(2 * time.Second)
	for len(r.sender.all()) < 2 {
		if time.Now().After(deadline) {
			t.Fatal("linger did not re-arm")
		}
		time.Sleep(time.Millisecond)
	}
	// Wait out the timer's flush (it holds the lock while it logs), so it
	// cannot log after the test returns.
	r.p.Flush()
}

func TestSendErrorFailsEveryKeyInTheFrame(t *testing.T) {
	r := newBatchRig(t, Batch{})
	r.install()
	r.sender.err = errors.New("zmq: send timeout")
	for i := 0; i < 3; i++ {
		r.p.OnChange(constants.Add, 0, ni, v4(route(i), 10))
	}
	r.p.Flush()
	for i := 0; i < 3; i++ {
		if err := r.outcome(route(i)); err == nil || !strings.Contains(err.Error(), "send timeout") {
			t.Fatalf("%s outcome = %v, want the send error", route(i), err)
		}
	}
}
