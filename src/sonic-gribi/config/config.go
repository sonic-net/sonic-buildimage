// Package config reads gribid's settings and the switch's VRFs from
// CONFIG_DB.
package config

import (
	"context"
	"fmt"
	"log/slog"
	"sort"
	"strconv"
	"strings"
	"time"

	"github.com/redis/go-redis/v9"
)

// DB is CONFIG_DB's Redis database number.
const DB = 4

// Settings is the GRIBI table, as modelled by sonic-gribi.yang. The zero
// value of each field means "not configured"; Load fills in the defaults.
type Settings struct {
	Port          int           // GRIBI|config port
	FIBAckTimeout time.Duration // GRIBI|config fib_ack_timeout, in seconds
	LogLevel      slog.Level    // GRIBI|config log_level
	Reflection    bool          // GRIBI|config enable_reflection

	// GRIBI|certs. With ServerCert and ServerKey set, gRIBI is served over
	// TLS; with CACert as well, clients must present a certificate it signed.
	ServerCert, ServerKey, CACert string
}

// Defaults are the values used when the GRIBI table leaves a field unset.
var Defaults = Settings{
	Port:          9340,
	FIBAckTimeout: 30 * time.Second,
	LogLevel:      slog.LevelInfo,
}

// Load reads the GRIBI table. A missing table yields Defaults; a malformed
// value is an error rather than a silent default.
func Load(ctx context.Context, rdb *redis.Client) (Settings, error) {
	s := Defaults
	cfg, err := rdb.HGetAll(ctx, "GRIBI|config").Result()
	if err != nil {
		return s, fmt.Errorf("read GRIBI|config: %w", err)
	}
	if v, ok := cfg["port"]; ok {
		p, err := strconv.Atoi(v)
		if err != nil || p < 1 || p > 65535 {
			return s, fmt.Errorf("GRIBI|config port %q is not a port number", v)
		}
		s.Port = p
	}
	if v, ok := cfg["fib_ack_timeout"]; ok {
		n, err := strconv.Atoi(v)
		if err != nil || n < 1 || n > 3600 {
			return s, fmt.Errorf("GRIBI|config fib_ack_timeout %q is not 1-3600 seconds", v)
		}
		s.FIBAckTimeout = time.Duration(n) * time.Second
	}
	if v, ok := cfg["log_level"]; ok {
		if err := s.LogLevel.UnmarshalText([]byte(v)); err != nil {
			return s, fmt.Errorf("GRIBI|config log_level %q: %w", v, err)
		}
	}
	if v, ok := cfg["enable_reflection"]; ok {
		b, err := strconv.ParseBool(v)
		if err != nil {
			return s, fmt.Errorf("GRIBI|config enable_reflection %q is not a boolean", v)
		}
		s.Reflection = b
	}

	certs, err := rdb.HGetAll(ctx, "GRIBI|certs").Result()
	if err != nil {
		return s, fmt.Errorf("read GRIBI|certs: %w", err)
	}
	s.ServerCert, s.ServerKey, s.CACert = certs["server_crt"], certs["server_key"], certs["ca_crt"]
	if (s.ServerCert == "") != (s.ServerKey == "") {
		return s, fmt.Errorf("GRIBI|certs needs both server_crt and server_key")
	}
	if s.CACert != "" && s.ServerCert == "" {
		return s, fmt.Errorf("GRIBI|certs ca_crt needs server_crt and server_key")
	}
	return s, nil
}

// TLS reports whether gRIBI is to be served over TLS.
func (s Settings) TLS() bool { return s.ServerCert != "" }

// VRFs returns the names of the VRFs in CONFIG_DB's VRF table, sorted.
func VRFs(ctx context.Context, rdb *redis.Client) ([]string, error) {
	var out []string
	iter := rdb.Scan(ctx, 0, "VRF|*", 1000).Iterator()
	for iter.Next(ctx) {
		if name, ok := strings.CutPrefix(iter.Val(), "VRF|"); ok && name != "" && !strings.Contains(name, "|") {
			out = append(out, name)
		}
	}
	if err := iter.Err(); err != nil {
		return nil, err
	}
	sort.Strings(out)
	return out, nil
}

// FIBResultsReported reports whether orchagent publishes route results, the
// only source of gRIBI FIB acknowledgements. routeorch does so only when
// orchagent runs with -F, which orchagent.sh sets from DEVICE_METADATA's
// suppress-fib-pending.
func FIBResultsReported(ctx context.Context, rdb *redis.Client) (bool, error) {
	v, err := rdb.HGet(ctx, "DEVICE_METADATA|localhost", "suppress-fib-pending").Result()
	if err == redis.Nil {
		return false, nil
	}
	if err != nil {
		return false, fmt.Errorf("read DEVICE_METADATA|localhost: %w", err)
	}
	return v == "enabled", nil
}
