// Command gribid is the SONiC gRIBI agent: a gRIBI server whose
// routes reach orchagent over SONiC's ZeroMQ route channel.
//
//	gRIBI client ──grpc──▶ gribi.Service (wraps gribigo)
//	                          │ hook
//	                          ▼
//	                    adapter.Programmer ──▶ swsszmq.Producer ──PUSH──▶ orchagent :8100
//	                          ▲                                              │
//	                    fibtrack.Tracker ◀── applstate.Listener ◀── Redis pub/sub
//
// orchagent must have been started with SYSTEM_DEFAULTS|swss_zmq status=enabled,
// otherwise it only reads ROUTE_TABLE from Redis and every frame sent here is
// dropped by its handler registry.
package main

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"errors"
	"flag"
	"fmt"
	"log/slog"
	"net"
	"os"
	"os/signal"
	"slices"
	"strings"
	"syscall"
	"time"

	"github.com/openconfig/gribigo/server"
	"github.com/redis/go-redis/v9"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"

	spb "github.com/openconfig/gribi/v1/proto/service"

	"github.com/sonic-net/sonic-gribi/adapter"
	"github.com/sonic-net/sonic-gribi/applstate"
	"github.com/sonic-net/sonic-gribi/config"
	"github.com/sonic-net/sonic-gribi/fibtrack"
	"github.com/sonic-net/sonic-gribi/gribi"
	"github.com/sonic-net/sonic-gribi/neigh"
	"github.com/sonic-net/sonic-gribi/routekey"
	"github.com/sonic-net/sonic-gribi/swsszmq"
)

// stringList is a repeatable string flag.
type stringList []string

func (s *stringList) String() string     { return strings.Join(*s, ",") }
func (s *stringList) Set(v string) error { *s = append(*s, v); return nil }

// Flags left unset take their value from CONFIG_DB's GRIBI table, then from
// config.Defaults.
var (
	listenAddr   = flag.String("listen", "", "address to serve gRIBI on (default: GRIBI|config port on all addresses)")
	zmqEndpoint  = flag.String("zmq", "tcp://127.0.0.1:8100", "orchagent's ZeroMQ route server (its -q flag)")
	redisAddr    = flag.String("redis", "localhost:6379", "SONiC Redis; read for NEIGH_TABLE and the route response channel")
	ackTimeout   = flag.Duration("fib-ack-timeout", 0, "how long a route may wait for orchagent's response before FIB_FAILED (default: GRIBI|config fib_ack_timeout)")
	startTimeout = flag.Duration("start-timeout", 10*time.Second, "how long startup may wait for Redis and the subscription")
	vrfs         stringList

	// Not -v: glog, which gribigo imports, registers that flag itself and a
	// duplicate registration panics before main runs.
	debug      = flag.Bool("debug", false, "log at debug level, overriding GRIBI|config log_level")
	reflect    = flag.Bool("reflection", false, "register gRPC server reflection, overriding GRIBI|config enable_reflection")
	plaintext  = flag.Bool("insecure", false, "serve without TLS even if GRIBI|certs is configured")
	logLevel   = new(slog.LevelVar)
	flagsGiven = map[string]bool{}
)

func main() {
	flag.Var(&vrfs, "vrf", "network instance to accept routes for (repeatable), replacing the CONFIG_DB VRF table; "+server.DefaultNetworkInstanceName+" is always present")
	// gribigo logs through glog, which defaults to files under /tmp.
	_ = flag.Set("logtostderr", "true")
	flag.Parse()
	flag.Visit(func(f *flag.Flag) { flagsGiven[f.Name] = true })

	log := slog.New(slog.NewTextHandler(os.Stderr, &slog.HandlerOptions{Level: logLevel}))

	if err := run(log); err != nil {
		log.Error("exiting", "error", err)
		os.Exit(1)
	}
}

// networkInstances returns the non-default instances to serve: the -vrf
// flags if any, else every VRF in CONFIG_DB. gribigo only accepts routes for
// instances that exist when it starts, so a VRF added later needs a restart.
func networkInstances(ctx context.Context, cfgDB *redis.Client, log *slog.Logger) ([]string, error) {
	known, err := config.VRFs(ctx, cfgDB)
	if err != nil {
		return nil, fmt.Errorf("read CONFIG_DB VRF table: %w", err)
	}
	if len(vrfs) == 0 {
		return known, nil
	}
	var out []string
	for _, v := range vrfs {
		if v == server.DefaultNetworkInstanceName {
			continue
		}
		out = append(out, v)
		// routeorch defers a route in an unknown VRF without an error, so
		// its gRIBI operation would only ever time out.
		if !slices.Contains(known, routekey.VRF(v)) {
			log.Warn("network instance has no VRF in CONFIG_DB; its routes will time out until it is created", "network_instance", v, "vrf", routekey.VRF(v))
		}
	}
	return out, nil
}

// settings merges CONFIG_DB's GRIBI table with any flags given explicitly.
func settings(ctx context.Context, cfgDB *redis.Client) (config.Settings, string, error) {
	s, err := config.Load(ctx, cfgDB)
	if err != nil {
		return s, "", err
	}
	if flagsGiven["fib-ack-timeout"] {
		s.FIBAckTimeout = *ackTimeout
	}
	if flagsGiven["debug"] && *debug {
		s.LogLevel = slog.LevelDebug
	}
	if flagsGiven["reflection"] {
		s.Reflection = *reflect
	}
	if *plaintext {
		s.ServerCert, s.ServerKey, s.CACert = "", "", ""
	}
	listen := fmt.Sprintf(":%d", s.Port)
	if flagsGiven["listen"] {
		listen = *listenAddr
	}
	return s, listen, nil
}

// serverCreds builds the gRIBI transport credentials: mutual TLS when
// GRIBI|certs names a CA, server-only TLS when it names just a key pair.
func serverCreds(s config.Settings) (credentials.TransportCredentials, error) {
	cert, err := tls.LoadX509KeyPair(s.ServerCert, s.ServerKey)
	if err != nil {
		return nil, fmt.Errorf("load server certificate: %w", err)
	}
	conf := &tls.Config{Certificates: []tls.Certificate{cert}, MinVersion: tls.VersionTLS12}
	if s.CACert != "" {
		pem, err := os.ReadFile(s.CACert)
		if err != nil {
			return nil, fmt.Errorf("read CA certificate: %w", err)
		}
		pool := x509.NewCertPool()
		if !pool.AppendCertsFromPEM(pem) {
			return nil, fmt.Errorf("no certificates in %s", s.CACert)
		}
		conf.ClientCAs = pool
		conf.ClientAuth = tls.RequireAndVerifyClientCert
	}
	return credentials.NewTLS(conf), nil
}

func run(log *slog.Logger) error {
	ctx, stop := signal.NotifyContext(context.Background(), syscall.SIGINT, syscall.SIGTERM)
	defer stop()

	rdb := redis.NewClient(&redis.Options{Addr: *redisAddr, DB: 0}) // APPL_DB
	defer rdb.Close()
	pingCtx, cancel := context.WithTimeout(ctx, *startTimeout)
	err := rdb.Ping(pingCtx).Err()
	cancel()
	if err != nil {
		return fmt.Errorf("redis at %s did not answer: %w", *redisAddr, err)
	}

	cfgDB := redis.NewClient(&redis.Options{Addr: *redisAddr, DB: config.DB})
	defer cfgDB.Close()
	cfg, listen, err := settings(ctx, cfgDB)
	if err != nil {
		return err
	}
	logLevel.Set(cfg.LogLevel)

	// The listener must be subscribed before any client can program a route,
	// or the first responses are lost and reported as timeouts.
	tracker := fibtrack.New()
	listener := applstate.New(rdb, tracker.Done, log)
	listenerErr := make(chan error, 1)
	go func() { listenerErr <- listener.Run(ctx) }()
	select {
	case <-listener.Ready():
	case err := <-listenerErr:
		return fmt.Errorf("route response listener: %w", err)
	case <-time.After(*startTimeout):
		return fmt.Errorf("subscribing to %s took longer than %s", applstate.Channel, *startTimeout)
	}
	log.Info("listening for route responses", "channel", applstate.Channel)

	producer, err := swsszmq.Dial(ctx, *zmqEndpoint)
	if err != nil {
		return fmt.Errorf("%w (is swss up, and is SYSTEM_DEFAULTS|swss_zmq enabled?)", err)
	}
	defer producer.Close()
	log.Info("connected to orchagent", "zmq", *zmqEndpoint)

	programmer := adapter.New(producer, neigh.NewRedis(rdb, cfgDB), tracker, log)

	extra, err := networkInstances(ctx, cfgDB, log)
	if err != nil {
		return err
	}
	gs, err := server.New(
		server.WithPostChangeRIBHook(programmer.OnChange),
		server.WithVRFs(extra),
	)
	if err != nil {
		return fmt.Errorf("build gRIBI server: %w", err)
	}

	var opts []grpc.ServerOption
	if cfg.TLS() {
		creds, err := serverCreds(cfg)
		if err != nil {
			return err
		}
		opts = append(opts, grpc.Creds(creds))
	}
	grpcSrv := grpc.NewServer(opts...)
	svc := gribi.New(gs, tracker, cfg.FIBAckTimeout, log)
	fibResults, err := config.FIBResultsReported(ctx, cfgDB)
	if err != nil {
		return err
	}
	if !fibResults {
		const why = "orchagent reports route results only with suppress-fib-pending enabled (sudo config suppress-fib-pending enabled)"
		svc.RefuseFIBAck(why)
		log.Warn("RIB_AND_FIB_ACK sessions will be refused", "reason", why)
	}
	spb.RegisterGRIBIServer(grpcSrv, svc)
	if cfg.Reflection {
		registerReflection(grpcSrv) // lets grpcurl list and call the service
	}

	lis, err := net.Listen("tcp", listen)
	if err != nil {
		return fmt.Errorf("listen on %s: %w", listen, err)
	}
	if !cfg.TLS() {
		log.Warn("serving gRIBI without transport security; set GRIBI|certs to enable TLS", "listen", lis.Addr().String())
	}
	log.Info("agent ready",
		"listen", lis.Addr().String(),
		"tls", cfg.TLS(),
		"mutual_tls", cfg.CACert != "",
		"network_instances", append([]string{server.DefaultNetworkInstanceName}, extra...),
		"fib_ack_timeout", cfg.FIBAckTimeout.String())

	serveErr := make(chan error, 1)
	go func() {
		if err := grpcSrv.Serve(lis); err != nil && !errors.Is(err, grpc.ErrServerStopped) {
			serveErr <- err
			return
		}
		serveErr <- nil
	}()

	select {
	case err := <-serveErr:
		return err
	case err := <-listenerErr:
		grpcSrv.Stop()
		return fmt.Errorf("route response listener stopped: %w", err)
	case <-ctx.Done():
		// Programmed routes stay in the FIB: the agent owns no cleanup
		// policy, and a restart leaves the switch forwarding on what it was
		// last told. Reversing that is the controller's decision.
		log.Info("shutting down; programmed routes are left in place")
		grpcSrv.GracefulStop()
		return nil
	}
}
