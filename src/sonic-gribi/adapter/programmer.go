// Package adapter turns gribigo's RIB changes into ROUTE_TABLE writes.
//
// gribigo terminates gRIBI, validates operations and holds the AFT objects;
// it has no programming backend of its own. Its seam for one is the
// post-change RIB hook, which fires synchronously for every next hop,
// next-hop group and route installed or removed, before the client is
// answered. This package is that hook's SONiC implementation.
//
// SONiC's ZeroMQ route path carries ROUTE_TABLE only; NEXTHOP_GROUP_TABLE
// stays on Redis, which orchagent no longer reads for routes. So a gRIBI
// next-hop group is flattened into each route that uses it, as fpmsyncd does
// for BGP ECMP: `nexthop`, `ifname` and `weight` are index-aligned comma
// lists. The Programmer keeps a mirror of the RIB's next hops and groups
// purely so that when a group or next hop changes it can re-render every
// route that depends on it.
//
// Route writes are batched. orchagent drains and commits one ZeroMQ frame at
// a time, so a frame per route costs it a fixed per-commit overhead per
// route. The hook therefore only queues its writes; the queue leaves as one
// frame when the caller calls Flush (at the end of each ModifyRequest and
// Flush RPC), when it reaches Batch.Max tuples, or Batch.Linger after its
// first write, whichever comes first.
package adapter

import (
	"context"
	"fmt"
	"log/slog"
	"sort"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/openconfig/gribigo/aft"
	"github.com/openconfig/gribigo/constants"
	"github.com/openconfig/ygot/ygot"

	"github.com/sonic-net/sonic-gribi/fibtrack"
	"github.com/sonic-net/sonic-gribi/neigh"
	"github.com/sonic-net/sonic-gribi/routekey"
	"github.com/sonic-net/sonic-gribi/swsszmq"
)

// RouteTable is the only table this agent writes.
const RouteTable = "ROUTE_TABLE"

// Protocol is stamped on every route. routeorch copies it into APPL_STATE_DB
// and the response channel, so it attributes the route in `show` output and
// tells these routes from fpmsyncd's on the same table.
const Protocol = "gribi"

// Batch defaults: DefaultBatchMax matches orchagent's -b 1024, and
// DefaultBatchLinger bounds how long a slow ModifyRequest can hold a route.
const (
	DefaultBatchMax    = 1024
	DefaultBatchLinger = 50 * time.Millisecond
)

// maxBatchBytes keeps a frame well under orchagent's receive buffer even for
// wide ECMP groups.
const maxBatchBytes = swsszmq.MaxFrameSize / 2

// Batch bounds how route writes are coalesced into frames.
type Batch struct {
	Max    int           // tuples per frame; 0 => DefaultBatchMax
	Linger time.Duration // longest a queued write waits for Flush; 0 => no timer
}

// resolveTimeout bounds a NEIGH_TABLE lookup. The hook runs on the gRIBI
// Modify path, so a slow Redis must not stall the client indefinitely.
const resolveTimeout = 2 * time.Second

// Programmer implements rib.RIBHookFn for SONiC.
type Programmer struct {
	sender  swsszmq.Sender
	neigh   neigh.Resolver
	tracker *fibtrack.Tracker
	log     *slog.Logger
	batch   Batch

	// One mutex for every network instance and the queue: gribigo runs one
	// Modify goroutine per client, and hooks from two clients must not
	// interleave their writes on the single ordered socket.
	mu      sync.Mutex
	nis     map[string]*mirror
	pending []swsszmq.Tuple // queued writes, in RIB order
	bytes   int             // encoded size of pending
	linger  *time.Timer     // flushes pending; nil until first armed
}

// New wires a programmer. Every ROUTE_TABLE write is announced to tracker
// when it is queued, before it is sent, so a response can never arrive
// unannounced.
func New(sender swsszmq.Sender, resolver neigh.Resolver, tracker *fibtrack.Tracker, batch Batch, log *slog.Logger) *Programmer {
	if batch.Max <= 0 {
		batch.Max = DefaultBatchMax
	}
	return &Programmer{sender: sender, neigh: resolver, tracker: tracker, batch: batch, log: log, nis: map[string]*mirror{}}
}

// Flush sends every queued write now. It is safe to call at any time and
// from any goroutine; with nothing queued it does nothing.
func (p *Programmer) Flush() {
	p.mu.Lock()
	defer p.mu.Unlock()
	p.flushLocked()
}

// queue appends one write, flushing first if it would push the frame past
// maxBatchBytes and after if the frame is full. p.mu must be held.
func (p *Programmer) queue(t swsszmq.Tuple) {
	size := t.EncodedSize()
	if len(p.pending) > 0 && p.bytes+size > maxBatchBytes {
		p.flushLocked()
	}
	if len(p.pending) == 0 && p.batch.Linger > 0 {
		if p.linger == nil {
			p.linger = time.AfterFunc(p.batch.Linger, p.Flush)
		} else {
			p.linger.Reset(p.batch.Linger)
		}
	}
	p.pending = append(p.pending, t)
	p.bytes += size
	if len(p.pending) >= p.batch.Max {
		p.flushLocked()
	}
}

// flushLocked sends pending as one frame. A failed send fails every key in
// it. p.mu must be held.
func (p *Programmer) flushLocked() {
	if len(p.pending) == 0 {
		return
	}
	if p.linger != nil {
		p.linger.Stop()
	}
	tuples := p.pending
	p.pending, p.bytes = nil, 0
	if err := p.sender.Send(context.Background(), RouteTable, tuples...); err != nil {
		p.log.Error("send routes", "count", len(tuples), "error", err)
		for _, t := range tuples {
			p.tracker.Done(t.Key, err)
		}
		return
	}
	p.log.Debug("sent routes", "count", len(tuples))
}

// OnChange is the rib.RIBHookFn. The int64 is gribigo's timestamp, not an
// operation id; correlation with the client's operation happens by route key
// in the gribi package.
func (p *Programmer) OnChange(op constants.OpType, _ int64, ni string, e ygot.ValidatedGoStruct) {
	p.mu.Lock()
	defer p.mu.Unlock()

	m := p.nis[ni]
	if m == nil {
		m = newMirror(routekey.VRF(ni))
		p.nis[ni] = m
	}

	switch v := e.(type) {
	case *aft.Afts_Ipv4Entry:
		p.route(op, ni, m, v.GetPrefix(), v.GetNextHopGroup(), v.GetNextHopGroupNetworkInstance())
	case *aft.Afts_Ipv6Entry:
		p.route(op, ni, m, v.GetPrefix(), v.GetNextHopGroup(), v.GetNextHopGroupNetworkInstance())
	case *aft.Afts_NextHopGroup:
		p.group(op, ni, m, v)
	case *aft.Afts_NextHop:
		p.nextHop(op, ni, m, v)
	case *aft.Afts_LabelEntry:
		p.log.Warn("ignoring MPLS label entry; LABEL_ROUTE_TABLE is not programmed", "ni", ni, "op", op.String())
	default:
		p.log.Warn("ignoring unexpected RIB entry type", "ni", ni, "op", op.String(), "type", fmt.Sprintf("%T", e))
	}
}

func (p *Programmer) route(op constants.OpType, ni string, m *mirror, prefix string, nhg uint64, nhgNI string) {
	key := routekey.For(ni, prefix)

	switch op {
	case constants.Delete:
		m.forgetRoute(prefix)
		p.tracker.SentDelete(key)
		p.queue(swsszmq.Tuple{Key: key})
		p.log.Debug("deleted route", "key", key)

	case constants.Add, constants.Replace:
		if nhgNI != "" && nhgNI != ni {
			m.forgetRoute(prefix)
			p.fail(key, fmt.Errorf("next-hop group %d lives in network instance %q, route in %q; cross-instance groups are not supported", nhg, nhgNI, ni))
			return
		}
		m.setRoute(prefix, nhg)
		tuple, err := p.render(m, key, prefix)
		if err != nil {
			p.fail(key, err)
			return
		}
		p.tracker.Sent(key)
		p.queue(tuple)
		p.log.Debug("programmed route", "key", key, "fields", fieldSummary(tuple))

	default:
		p.log.Warn("ignoring unknown operation on route", "key", key, "op", op.String())
	}
}

func (p *Programmer) group(op constants.OpType, ni string, m *mirror, g *aft.Afts_NextHopGroup) {
	id := g.GetId()
	switch op {
	case constants.Delete:
		m.dropGroup(id)
		p.log.Debug("forgot next-hop group", "ni", ni, "id", id)
	case constants.Add, constants.Replace:
		members := make([]member, 0, len(g.NextHop))
		for idx, nh := range g.NextHop {
			members = append(members, member{index: idx, weight: nh.GetWeight()})
		}
		sort.Slice(members, func(i, j int) bool { return members[i].index < members[j].index })
		m.setGroup(id, members)
		p.log.Debug("mirrored next-hop group", "ni", ni, "id", id, "members", len(members))
		p.reprogram(ni, m, m.routesOfGroup(id))
	default:
		p.log.Warn("ignoring unknown operation on next-hop group", "ni", ni, "id", id, "op", op.String())
	}
}

func (p *Programmer) nextHop(op constants.OpType, ni string, m *mirror, nh *aft.Afts_NextHop) {
	idx := nh.GetIndex()
	switch op {
	case constants.Delete:
		m.dropNextHop(idx)
		p.log.Debug("forgot next hop", "ni", ni, "index", idx)
	case constants.Add, constants.Replace:
		m.setNextHop(idx, nextHop{addr: nh.GetIpAddress(), iface: nh.GetInterfaceRef().GetInterface()})
		p.log.Debug("mirrored next hop", "ni", ni, "index", idx, "address", nh.GetIpAddress())
		p.reprogram(ni, m, m.routesOfNextHop(idx))
	default:
		p.log.Warn("ignoring unknown operation on next hop", "ni", ni, "index", idx, "op", op.String())
	}
}

// reprogram re-renders every route in prefixes and queues the survivors, so
// orchagent applies a group change to all its routes in one batch.
func (p *Programmer) reprogram(ni string, m *mirror, prefixes []string) {
	if len(prefixes) == 0 {
		return
	}
	var keys []string
	for _, prefix := range prefixes {
		key := routekey.For(ni, prefix)
		tuple, err := p.render(m, key, prefix)
		if err != nil {
			p.fail(key, err)
			continue
		}
		p.tracker.Sent(key)
		p.queue(tuple)
		keys = append(keys, key)
	}
	if len(keys) > 0 {
		p.log.Info("reprogrammed routes after next-hop change", "ni", ni, "count", len(keys), "keys", keys)
	}
}

// fail records a route the adapter refused to send. The client gets a real
// answer either way, so a route that cannot be fully rendered is failed
// rather than programmed with the legs that happened to resolve: partial
// programming would hide a controller-visible fault.
func (p *Programmer) fail(key string, err error) {
	p.log.Error("route not programmed", "key", key, "error", err)
	p.tracker.Sent(key)
	p.tracker.Done(key, err)
}

func inVRF(vrf string) string {
	if vrf == "" {
		return ""
	}
	return " in " + vrf
}

type leg struct {
	addr   string
	iface  string
	weight uint64
}

// render flattens prefix's group into the four ROUTE_TABLE fields.
func (p *Programmer) render(m *mirror, key, prefix string) (swsszmq.Tuple, error) {
	gid, ok := m.routes[prefix]
	if !ok {
		return swsszmq.Tuple{}, fmt.Errorf("route %s is not in the mirror", prefix)
	}
	g, ok := m.groups[gid]
	if !ok {
		return swsszmq.Tuple{}, fmt.Errorf("next-hop group %d is not in the mirror", gid)
	}
	if len(g.members) == 0 {
		return swsszmq.Tuple{}, fmt.Errorf("next-hop group %d has no members", gid)
	}

	legs := make([]leg, 0, len(g.members))
	for _, mem := range g.members {
		nh, ok := m.nexthops[mem.index]
		if !ok {
			return swsszmq.Tuple{}, fmt.Errorf("next hop %d is not in the mirror", mem.index)
		}
		if nh.addr == "" {
			return swsszmq.Tuple{}, fmt.Errorf("next hop %d has no IP address; only IP next hops are supported", mem.index)
		}
		iface := nh.iface
		if iface == "" {
			ctx, cancel := context.WithTimeout(context.Background(), resolveTimeout)
			iface, ok = p.neigh.Interface(ctx, m.vrf, nh.addr)
			cancel()
			if !ok {
				return swsszmq.Tuple{}, fmt.Errorf("next hop %d (%s) has no resolved neighbor in NEIGH_TABLE%s and no interface-ref", mem.index, nh.addr, inVRF(m.vrf))
			}
		}
		w := mem.weight
		if w == 0 {
			w = 1
		}
		legs = append(legs, leg{addr: nh.addr, iface: iface, weight: w})
	}
	// gribigo hands over a map, so sort for a stable rendering: orchagent
	// treats a reordered list as a change and reprograms the group.
	sort.Slice(legs, func(i, j int) bool { return legs[i].addr < legs[j].addr })

	addrs := make([]string, len(legs))
	ifaces := make([]string, len(legs))
	weights := make([]string, len(legs))
	for i, l := range legs {
		addrs[i] = l.addr
		ifaces[i] = l.iface
		weights[i] = strconv.FormatUint(l.weight, 10)
	}
	return swsszmq.Tuple{Key: key, Fields: []swsszmq.FieldValue{
		{Field: "nexthop", Value: strings.Join(addrs, ",")},
		{Field: "ifname", Value: strings.Join(ifaces, ",")},
		{Field: "weight", Value: strings.Join(weights, ",")},
		{Field: "protocol", Value: Protocol},
	}}, nil
}

func fieldSummary(t swsszmq.Tuple) string {
	parts := make([]string, 0, len(t.Fields))
	for _, fv := range t.Fields {
		parts = append(parts, fv.Field+"="+fv.Value)
	}
	return strings.Join(parts, " ")
}

// mirror is the per-network-instance copy of what the RIB has installed,
// reduced to the fields a ROUTE_TABLE row needs, plus the reverse indexes
// that answer "which routes must be re-rendered when this changes".
type mirror struct {
	vrf         string // SONiC VRF name, "" for the default VRF
	nexthops    map[uint64]nextHop
	groups      map[uint64]group
	routes      map[string]uint64
	groupRoutes map[uint64]map[string]struct{}
	nhGroups    map[uint64]map[uint64]struct{}
}

type nextHop struct {
	addr  string
	iface string
}

type group struct {
	members []member
}

type member struct {
	index  uint64
	weight uint64
}

func newMirror(vrf string) *mirror {
	return &mirror{
		vrf:         vrf,
		nexthops:    map[uint64]nextHop{},
		groups:      map[uint64]group{},
		routes:      map[string]uint64{},
		groupRoutes: map[uint64]map[string]struct{}{},
		nhGroups:    map[uint64]map[uint64]struct{}{},
	}
}

func (m *mirror) setRoute(prefix string, gid uint64) {
	m.forgetRoute(prefix)
	m.routes[prefix] = gid
	if m.groupRoutes[gid] == nil {
		m.groupRoutes[gid] = map[string]struct{}{}
	}
	m.groupRoutes[gid][prefix] = struct{}{}
}

func (m *mirror) forgetRoute(prefix string) {
	gid, ok := m.routes[prefix]
	if !ok {
		return
	}
	delete(m.routes, prefix)
	delete(m.groupRoutes[gid], prefix)
	if len(m.groupRoutes[gid]) == 0 {
		delete(m.groupRoutes, gid)
	}
}

func (m *mirror) setGroup(id uint64, members []member) {
	m.dropGroup(id)
	m.groups[id] = group{members: members}
	for _, mem := range members {
		if m.nhGroups[mem.index] == nil {
			m.nhGroups[mem.index] = map[uint64]struct{}{}
		}
		m.nhGroups[mem.index][id] = struct{}{}
	}
}

// dropGroup removes the group's definition but keeps its routes' reference to
// it, since gribigo refuses to delete a group that routes still name.
func (m *mirror) dropGroup(id uint64) {
	g, ok := m.groups[id]
	if !ok {
		return
	}
	delete(m.groups, id)
	for _, mem := range g.members {
		delete(m.nhGroups[mem.index], id)
		if len(m.nhGroups[mem.index]) == 0 {
			delete(m.nhGroups, mem.index)
		}
	}
}

func (m *mirror) setNextHop(idx uint64, nh nextHop) { m.nexthops[idx] = nh }

func (m *mirror) dropNextHop(idx uint64) { delete(m.nexthops, idx) }

func (m *mirror) routesOfGroup(id uint64) []string {
	out := make([]string, 0, len(m.groupRoutes[id]))
	for prefix := range m.groupRoutes[id] {
		out = append(out, prefix)
	}
	sort.Strings(out)
	return out
}

func (m *mirror) routesOfNextHop(idx uint64) []string {
	seen := map[string]struct{}{}
	for gid := range m.nhGroups[idx] {
		for prefix := range m.groupRoutes[gid] {
			seen[prefix] = struct{}{}
		}
	}
	out := make([]string, 0, len(seen))
	for prefix := range seen {
		out = append(out, prefix)
	}
	sort.Strings(out)
	return out
}
