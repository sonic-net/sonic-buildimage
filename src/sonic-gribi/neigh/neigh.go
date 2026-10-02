// Package neigh supplies the one thing a gRIBI next hop lacks and a SONiC
// route needs: the egress interface. A gRIBI next hop is usually just an
// address; ROUTE_TABLE wants `ifname` alongside `nexthop`. SONiC's own
// neighbor table already knows which interface each resolved neighbor sits
// behind, so that is where the answer comes from.
package neigh

import (
	"context"
	"net/netip"
	"strings"
	"sync"

	"github.com/redis/go-redis/v9"
)

// Resolver maps a next-hop address to its egress interface.
type Resolver interface {
	// Interface returns the interface behind ip in VRF vrf ("" for the
	// default VRF). The bool is false when no resolved neighbor exists there,
	// a normal transient condition rather than an error.
	Interface(ctx context.Context, vrf, ip string) (string, bool)
}

const tablePrefix = "NEIGH_TABLE:"

// interfaceTables are the CONFIG_DB tables that bind an L3 interface to a VRF
// through `<TABLE>|<ifname>` `vrf_name`. An interface with no binding is in
// the default VRF.
var interfaceTables = []string{"INTERFACE", "VLAN_INTERFACE", "PORTCHANNEL_INTERFACE", "VLAN_SUB_INTERFACE"}

// Redis reads APPL_DB's NEIGH_TABLE.
//
// Keys are `NEIGH_TABLE:<ifname>:<ip>`. neighorch splits on the first colon
// after the prefix (neighorch.cpp), which is what keeps IPv6 addresses, full
// of colons, safe on the right of the split.
// The same address can be a neighbor in two VRFs, so entries are keyed by
// (VRF, address), with each interface's VRF read from CONFIG_DB.
type Redis struct {
	appl   *redis.Client
	config *redis.Client

	mu     sync.RWMutex
	byIP   map[vrfIP]string
	loaded bool
}

type vrfIP struct{ vrf, ip string }

// NewRedis returns a resolver over APPL_DB (db 0) and CONFIG_DB (db 4)
// connections. A nil config puts every interface in the default VRF.
func NewRedis(appl, config *redis.Client) *Redis {
	return &Redis{appl: appl, config: config, byIP: map[vrfIP]string{}}
}

// Interface answers from cache and rescans the table once on a miss, so a
// neighbor that resolved after the last scan is found on the next lookup.
func (r *Redis) Interface(ctx context.Context, vrf, ip string) (string, bool) {
	k := vrfIP{vrf, canonical(ip)}
	if name, ok := r.lookup(k); ok {
		return name, true
	}
	if err := r.refresh(ctx); err != nil {
		return "", false
	}
	return r.lookup(k)
}

func (r *Redis) lookup(k vrfIP) (string, bool) {
	r.mu.RLock()
	defer r.mu.RUnlock()
	if !r.loaded {
		return "", false
	}
	name, ok := r.byIP[k]
	return name, ok
}

func (r *Redis) refresh(ctx context.Context) error {
	vrfOf, err := r.interfaceVRFs(ctx)
	if err != nil {
		return err
	}
	byIP := map[vrfIP]string{}
	iter := r.appl.Scan(ctx, 0, tablePrefix+"*", 1000).Iterator()
	for iter.Next(ctx) {
		if name, ip, ok := parseKey(iter.Val()); ok {
			byIP[vrfIP{vrfOf[name], ip}] = name
		}
	}
	if err := iter.Err(); err != nil {
		return err
	}
	r.mu.Lock()
	r.byIP, r.loaded = byIP, true
	r.mu.Unlock()
	return nil
}

// interfaceVRFs returns the VRF of every interface that CONFIG_DB binds to
// one. Only `<TABLE>|<ifname>` keys carry vrf_name; `<TABLE>|<ifname>|<ip>`
// keys are the addresses and are skipped.
func (r *Redis) interfaceVRFs(ctx context.Context) (map[string]string, error) {
	vrfOf := map[string]string{}
	if r.config == nil {
		return vrfOf, nil
	}
	for _, table := range interfaceTables {
		iter := r.config.Scan(ctx, 0, table+"|*", 1000).Iterator()
		for iter.Next(ctx) {
			ifname, ok := strings.CutPrefix(iter.Val(), table+"|")
			if !ok || ifname == "" || strings.Contains(ifname, "|") {
				continue
			}
			vrf, err := r.config.HGet(ctx, iter.Val(), "vrf_name").Result()
			if err == redis.Nil {
				continue
			}
			if err != nil {
				return nil, err
			}
			vrfOf[ifname] = vrf
		}
		if err := iter.Err(); err != nil {
			return nil, err
		}
	}
	return vrfOf, nil
}

// parseKey splits `NEIGH_TABLE:<ifname>:<ip>` at the first colon after the
// prefix and returns the address in canonical form.
func parseKey(key string) (ifname, ip string, ok bool) {
	rest, found := strings.CutPrefix(key, tablePrefix)
	if !found {
		return "", "", false
	}
	ifname, ip, ok = strings.Cut(rest, ":")
	if !ok || ifname == "" || ip == "" {
		return "", "", false
	}
	return ifname, canonical(ip), true
}

// canonical normalises an address so `FC00::72`, `fc00:0:0::72` and `fc00::72`
// meet in the map. Unparseable input is returned as-is.
func canonical(ip string) string {
	if a, err := netip.ParseAddr(ip); err == nil {
		return a.String()
	}
	return ip
}
