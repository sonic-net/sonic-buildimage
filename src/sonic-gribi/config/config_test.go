package config

import (
	"context"
	"log/slog"
	"testing"
	"time"

	"github.com/alicebob/miniredis/v2"
	"github.com/google/go-cmp/cmp"
	"github.com/redis/go-redis/v9"
)

func newDB(t *testing.T) (*miniredis.Miniredis, *redis.Client) {
	t.Helper()
	mr := miniredis.RunT(t)
	rdb := redis.NewClient(&redis.Options{Addr: mr.Addr()})
	t.Cleanup(func() { rdb.Close() })
	return mr, rdb
}

func TestVRFs(t *testing.T) {
	mr, rdb := newDB(t)
	mr.HSet("VRF|Vrfred", "NULL", "NULL")
	mr.HSet("VRF|Vrfblue", "vni", "1000")
	mr.HSet("VRF_ROUTE_LEAK|Vrfblue", "x", "y")
	mr.HSet("VRF|", "NULL", "NULL")
	got, err := VRFs(context.Background(), rdb)
	if err != nil {
		t.Fatalf("VRFs: %v", err)
	}
	if diff := cmp.Diff(got, []string{"Vrfblue", "Vrfred"}); diff != "" {
		t.Fatalf("VRFs diff(-got,+want):\n%s", diff)
	}
}

func TestVRFsEmpty(t *testing.T) {
	_, rdb := newDB(t)
	got, err := VRFs(context.Background(), rdb)
	if err != nil || len(got) != 0 {
		t.Fatalf("VRFs = %v, %v; want none", got, err)
	}
}

func TestLoadDefaults(t *testing.T) {
	_, rdb := newDB(t)
	got, err := Load(context.Background(), rdb)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if diff := cmp.Diff(got, Defaults); diff != "" {
		t.Fatalf("Load diff(-got,+want):\n%s", diff)
	}
	if got.TLS() {
		t.Fatal("TLS with no certs configured")
	}
}

func TestLoad(t *testing.T) {
	mr, rdb := newDB(t)
	mr.HSet("GRIBI|config", "port", "9559", "fib_ack_timeout", "5", "log_level", "debug", "enable_reflection", "true")
	mr.HSet("GRIBI|certs", "server_crt", "/etc/sonic/tls/gribi.crt", "server_key", "/etc/sonic/tls/gribi.key", "ca_crt", "/etc/sonic/tls/ca.crt")
	got, err := Load(context.Background(), rdb)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	want := Settings{
		Port:          9559,
		FIBAckTimeout: 5 * time.Second,
		LogLevel:      slog.LevelDebug,
		Reflection:    true,
		ServerCert:    "/etc/sonic/tls/gribi.crt",
		ServerKey:     "/etc/sonic/tls/gribi.key",
		CACert:        "/etc/sonic/tls/ca.crt",
	}
	if diff := cmp.Diff(got, want); diff != "" {
		t.Fatalf("Load diff(-got,+want):\n%s", diff)
	}
	if !got.TLS() {
		t.Fatal("certs configured but TLS() is false")
	}
}

func TestLoadRejectsBadValues(t *testing.T) {
	cases := []struct {
		table  string
		fields []string
	}{
		{"GRIBI|config", []string{"port", "0"}},
		{"GRIBI|config", []string{"port", "65536"}},
		{"GRIBI|config", []string{"port", "grpc"}},
		{"GRIBI|config", []string{"fib_ack_timeout", "0"}},
		{"GRIBI|config", []string{"fib_ack_timeout", "30s"}},
		{"GRIBI|config", []string{"log_level", "loud"}},
		{"GRIBI|config", []string{"enable_reflection", "yes please"}},
		{"GRIBI|certs", []string{"server_crt", "/a.crt"}},
		{"GRIBI|certs", []string{"server_key", "/a.key"}},
		{"GRIBI|certs", []string{"ca_crt", "/ca.crt"}},
	}
	for _, c := range cases {
		mr, rdb := newDB(t)
		mr.HSet(c.table, c.fields...)
		if _, err := Load(context.Background(), rdb); err == nil {
			t.Errorf("%s %v: Load accepted it", c.table, c.fields)
		}
	}
}

func TestFIBResultsReported(t *testing.T) {
	for _, c := range []struct {
		value string
		want  bool
	}{{"", false}, {"disabled", false}, {"enabled", true}} {
		mr, rdb := newDB(t)
		if c.value != "" {
			mr.HSet("DEVICE_METADATA|localhost", "hostname", "sw1", "suppress-fib-pending", c.value)
		}
		got, err := FIBResultsReported(context.Background(), rdb)
		if err != nil || got != c.want {
			t.Errorf("suppress-fib-pending %q: got %v, %v; want %v", c.value, got, err, c.want)
		}
	}
}
