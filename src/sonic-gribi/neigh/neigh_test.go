package neigh

import (
	"context"
	"testing"
	"time"

	"github.com/alicebob/miniredis/v2"
	"github.com/redis/go-redis/v9"
)

func newResolver(t *testing.T) (*miniredis.Miniredis, *Redis) {
	t.Helper()
	mr := miniredis.RunT(t)
	rdb := redis.NewClient(&redis.Options{Addr: mr.Addr()})
	t.Cleanup(func() { rdb.Close() })
	return mr, NewRedis(rdb, nil)
}

func seed(t *testing.T, mr *miniredis.Miniredis, ifname, ip string) {
	t.Helper()
	mr.HSet("NEIGH_TABLE:"+ifname+":"+ip, "neigh", "00:11:22:33:44:55", "family", "IPv4")
}

func TestInterfaceV4AndV6(t *testing.T) {
	mr, r := newResolver(t)
	seed(t, mr, "PortChannel101", "10.0.0.57")
	seed(t, mr, "PortChannel103", "fc00::7a")
	ctx := context.Background()

	if got, ok := r.Interface(ctx, "", "10.0.0.57"); !ok || got != "PortChannel101" {
		t.Fatalf("v4: got %q,%v", got, ok)
	}
	if got, ok := r.Interface(ctx, "", "fc00::7a"); !ok || got != "PortChannel103" {
		t.Fatalf("v6: got %q,%v", got, ok)
	}
	if got, ok := r.Interface(ctx, "", "FC00:0:0:0::7A"); !ok || got != "PortChannel103" {
		t.Fatalf("v6 non-canonical spelling: got %q,%v", got, ok)
	}
}

func TestRefreshOnMiss(t *testing.T) {
	mr, r := newResolver(t)
	ctx := context.Background()
	if _, ok := r.Interface(ctx, "", "10.0.0.59"); ok {
		t.Fatal("resolved a neighbor that does not exist")
	}
	seed(t, mr, "PortChannel102", "10.0.0.59")
	if got, ok := r.Interface(ctx, "", "10.0.0.59"); !ok || got != "PortChannel102" {
		t.Fatalf("after seeding: got %q,%v", got, ok)
	}
	// A cached hit must not go back to Redis: prove it by pulling the key.
	mr.Del("NEIGH_TABLE:PortChannel102:10.0.0.59")
	if got, ok := r.Interface(ctx, "", "10.0.0.59"); !ok || got != "PortChannel102" {
		t.Fatalf("cache bypassed: got %q,%v", got, ok)
	}
}

func TestUnresolved(t *testing.T) {
	mr, r := newResolver(t)
	seed(t, mr, "PortChannel101", "10.0.0.57")
	if got, ok := r.Interface(context.Background(), "", "192.0.2.1"); ok {
		t.Fatalf("resolved %q for an address with no neighbor", got)
	}
}

func TestMalformedKeysIgnored(t *testing.T) {
	mr, r := newResolver(t)
	mr.HSet("NEIGH_TABLE:", "neigh", "x")
	mr.HSet("NEIGH_TABLE:PortChannel101", "neigh", "x")
	mr.HSet("NEIGH_TABLE::10.0.0.61", "neigh", "x")
	mr.HSet("NEIGH_TABLE_JUNK:PortChannel104:10.0.0.63", "neigh", "x")
	seed(t, mr, "PortChannel101", "10.0.0.57")
	ctx := context.Background()
	if got, ok := r.Interface(ctx, "", "10.0.0.57"); !ok || got != "PortChannel101" {
		t.Fatalf("good key lost among bad ones: got %q,%v", got, ok)
	}
	for _, ip := range []string{"10.0.0.61", "10.0.0.63", ""} {
		if got, ok := r.Interface(ctx, "", ip); ok {
			t.Errorf("%q resolved to %q from a malformed key", ip, got)
		}
	}
}

func TestLookupHonoursContextTimeout(t *testing.T) {
	mr, r := newResolver(t)
	seed(t, mr, "PortChannel101", "10.0.0.57")
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	if _, ok := r.Interface(ctx, "", "10.0.0.57"); ok {
		t.Fatal("a cancelled context still produced a fresh lookup")
	}
	ctx, cancel = context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	if got, ok := r.Interface(ctx, "", "10.0.0.57"); !ok || got != "PortChannel101" {
		t.Fatalf("live context: got %q,%v", got, ok)
	}
}

func TestInterfaceIsPerVRF(t *testing.T) {
	appl := miniredis.RunT(t)
	config := miniredis.RunT(t)
	applDB := redis.NewClient(&redis.Options{Addr: appl.Addr()})
	configDB := redis.NewClient(&redis.Options{Addr: config.Addr()})
	t.Cleanup(func() { applDB.Close(); configDB.Close() })
	r := NewRedis(applDB, configDB)

	// The same address is a neighbor in the default VRF and in Vrfblue.
	seed(t, appl, "PortChannel101", "10.0.0.57")
	seed(t, appl, "Ethernet8", "10.0.0.57")
	seed(t, appl, "Vlan1000", "192.168.0.2")
	config.HSet("INTERFACE|Ethernet8", "vrf_name", "Vrfblue")
	config.HSet("INTERFACE|Ethernet8|10.0.0.56/31", "NULL", "NULL")
	config.HSet("PORTCHANNEL_INTERFACE|PortChannel101", "NULL", "NULL")
	config.HSet("VLAN_INTERFACE|Vlan1000", "vrf_name", "Vrfblue")
	ctx := context.Background()

	cases := []struct{ vrf, ip, want string }{
		{"", "10.0.0.57", "PortChannel101"},
		{"Vrfblue", "10.0.0.57", "Ethernet8"},
		{"Vrfblue", "192.168.0.2", "Vlan1000"},
	}
	for _, c := range cases {
		if got, ok := r.Interface(ctx, c.vrf, c.ip); !ok || got != c.want {
			t.Errorf("Interface(%q, %q) = %q,%v, want %q", c.vrf, c.ip, got, ok, c.want)
		}
	}
	if got, ok := r.Interface(ctx, "", "192.168.0.2"); ok {
		t.Errorf("a Vrfblue neighbor resolved in the default VRF to %q", got)
	}
	if got, ok := r.Interface(ctx, "Vrfred", "10.0.0.57"); ok {
		t.Errorf("resolved %q in a VRF with no neighbors", got)
	}
}
