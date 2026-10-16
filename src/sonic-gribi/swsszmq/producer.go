package swsszmq

import (
	"context"
	"fmt"
	"sync"
	"time"

	"github.com/go-zeromq/zmq4"
)

// Sender delivers table operations to orchagent. The adapter depends on this
// rather than on Producer so tests can capture what would have been sent.
type Sender interface {
	Send(ctx context.Context, table string, tuples ...Tuple) error
}

// Producer is a ZeroMQ PUSH socket connected to orchagent's PULL server.
//
// One socket, one mutex: orchagent applies whatever arrives last for a key, so
// two writes to the same key must leave here in the order the RIB produced
// them. Delivery is fire-and-forget; the FIB acknowledgement travels back on
// the Redis response channel, not on this socket.
type Producer struct {
	db   string
	sock zmq4.Socket

	mu sync.Mutex
}

// Option configures Dial.
type Option func(*config)

type config struct {
	db          string
	sendTimeout time.Duration
}

// WithDB overrides the database name stamped into pair 0. orchagent's handler
// registry keys on it, so it has to match the DBConnector orchagent used to
// create the consumer: "APPL_DB" for ROUTE_TABLE.
func WithDB(db string) Option { return func(c *config) { c.db = db } }

// WithSendTimeout bounds how long Send blocks when orchagent is not draining
// the socket, e.g. while swss restarts. The hook that calls Send runs on the
// gRIBI Modify path, so this is also the ceiling on how long a client waits
// for RIB_PROGRAMMED when orchagent is down.
func WithSendTimeout(d time.Duration) Option { return func(c *config) { c.sendTimeout = d } }

// Dial connects to endpoint (for orchagent: tcp://127.0.0.1:8100). It fails
// if nothing is listening, which at startup is the right answer: it means
// swss is down or swss_zmq is disabled.
func Dial(ctx context.Context, endpoint string, opts ...Option) (*Producer, error) {
	cfg := config{db: "APPL_DB", sendTimeout: 5 * time.Second}
	for _, o := range opts {
		o(&cfg)
	}
	sock := zmq4.NewPush(ctx,
		zmq4.WithTimeout(cfg.sendTimeout),
		zmq4.WithAutomaticReconnect(true),
	)
	if err := sock.Dial(endpoint); err != nil {
		sock.Close()
		return nil, fmt.Errorf("swsszmq: dial %s: %w", endpoint, err)
	}
	return &Producer{db: cfg.db, sock: sock}, nil
}

// Send encodes tuples into one frame and pushes it. Several tuples in one
// frame reach orchagent together and are merged into one drain batch.
func (p *Producer) Send(ctx context.Context, table string, tuples ...Tuple) error {
	if len(tuples) == 0 {
		return nil
	}
	frame, err := Encode(p.db, table, tuples)
	if err != nil {
		return err
	}
	if err := ctx.Err(); err != nil {
		return err
	}
	p.mu.Lock()
	defer p.mu.Unlock()
	if err := p.sock.Send(zmq4.NewMsg(frame)); err != nil {
		return fmt.Errorf("swsszmq: send %d tuple(s) to %s: %w", len(tuples), table, err)
	}
	return nil
}

// Close releases the socket.
func (p *Producer) Close() error {
	p.mu.Lock()
	defer p.mu.Unlock()
	return p.sock.Close()
}
