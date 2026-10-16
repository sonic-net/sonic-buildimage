package main

import (
	"context"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"log/slog"
	"math/big"
	"os"
	"path/filepath"
	"testing"
	"time"

	"github.com/alicebob/miniredis/v2"
	"github.com/redis/go-redis/v9"
)

func configDB(t *testing.T) (*miniredis.Miniredis, *redis.Client) {
	t.Helper()
	mr := miniredis.RunT(t)
	rdb := redis.NewClient(&redis.Options{Addr: mr.Addr()})
	t.Cleanup(func() { rdb.Close() })
	return mr, rdb
}

// withFlags sets the flag globals for one test and restores them after.
func withFlags(t *testing.T, given map[string]bool, set func()) {
	t.Helper()
	saved := struct {
		listen    string
		ack       time.Duration
		debug     bool
		reflect   bool
		plaintext bool
		given     map[string]bool
	}{*listenAddr, *ackTimeout, *debug, *reflect, *plaintext, flagsGiven}
	t.Cleanup(func() {
		*listenAddr, *ackTimeout, *debug, *reflect, *plaintext, flagsGiven =
			saved.listen, saved.ack, saved.debug, saved.reflect, saved.plaintext, saved.given
	})
	flagsGiven = given
	set()
}

func TestSettingsFromConfigDB(t *testing.T) {
	mr, rdb := configDB(t)
	mr.HSet("GRIBI|config", "port", "9559", "fib_ack_timeout", "5", "log_level", "debug", "enable_reflection", "true")
	withFlags(t, map[string]bool{}, func() {})
	s, listen, err := settings(context.Background(), rdb)
	if err != nil {
		t.Fatalf("settings: %v", err)
	}
	if listen != ":9559" || s.FIBAckTimeout != 5*time.Second || s.LogLevel != slog.LevelDebug || !s.Reflection {
		t.Fatalf("got listen %q, %+v", listen, s)
	}
}

func TestFlagsOverrideConfigDB(t *testing.T) {
	mr, rdb := configDB(t)
	mr.HSet("GRIBI|config", "port", "9559", "fib_ack_timeout", "5", "enable_reflection", "true")
	mr.HSet("GRIBI|certs", "server_crt", "/a.crt", "server_key", "/a.key")
	withFlags(t, map[string]bool{"listen": true, "fib-ack-timeout": true, "reflection": true, "insecure": true}, func() {
		*listenAddr, *ackTimeout, *reflect, *plaintext = "127.0.0.1:9999", time.Second, false, true
	})
	s, listen, err := settings(context.Background(), rdb)
	if err != nil {
		t.Fatalf("settings: %v", err)
	}
	if listen != "127.0.0.1:9999" || s.FIBAckTimeout != time.Second || s.Reflection || s.TLS() {
		t.Fatalf("flags did not win: listen %q, %+v", listen, s)
	}
}

func TestServerCreds(t *testing.T) {
	dir := t.TempDir()
	crt, key := writeSelfSigned(t, dir)
	mr, rdb := configDB(t)
	withFlags(t, map[string]bool{}, func() {})

	mr.HSet("GRIBI|certs", "server_crt", crt, "server_key", key)
	s, _, err := settings(context.Background(), rdb)
	if err != nil {
		t.Fatalf("settings: %v", err)
	}
	creds, err := serverCreds(s)
	if err != nil {
		t.Fatalf("serverCreds: %v", err)
	}
	if got := creds.Info().SecurityProtocol; got != "tls" {
		t.Fatalf("security protocol %q, want tls", got)
	}

	s.CACert = crt
	if _, err := serverCreds(s); err != nil {
		t.Fatalf("serverCreds with CA: %v", err)
	}
	s.CACert = key // a file with no certificate in it
	if _, err := serverCreds(s); err == nil {
		t.Fatal("serverCreds accepted a CA file with no certificate")
	}
	s.ServerCert = filepath.Join(dir, "missing.crt")
	if _, err := serverCreds(s); err == nil {
		t.Fatal("serverCreds accepted a missing certificate")
	}
}

func writeSelfSigned(t *testing.T, dir string) (crtPath, keyPath string) {
	t.Helper()
	priv, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		t.Fatal(err)
	}
	tmpl := &x509.Certificate{
		SerialNumber:          big.NewInt(1),
		Subject:               pkix.Name{CommonName: "gribid-test"},
		NotBefore:             time.Now().Add(-time.Hour),
		NotAfter:              time.Now().Add(time.Hour),
		IsCA:                  true,
		BasicConstraintsValid: true,
		KeyUsage:              x509.KeyUsageCertSign | x509.KeyUsageDigitalSignature,
	}
	der, err := x509.CreateCertificate(rand.Reader, tmpl, tmpl, &priv.PublicKey, priv)
	if err != nil {
		t.Fatal(err)
	}
	keyDER, err := x509.MarshalECPrivateKey(priv)
	if err != nil {
		t.Fatal(err)
	}
	crtPath, keyPath = filepath.Join(dir, "gribi.crt"), filepath.Join(dir, "gribi.key")
	if err := os.WriteFile(crtPath, pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: der}), 0o600); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(keyPath, pem.EncodeToMemory(&pem.Block{Type: "EC PRIVATE KEY", Bytes: keyDER}), 0o600); err != nil {
		t.Fatal(err)
	}
	return crtPath, keyPath
}
