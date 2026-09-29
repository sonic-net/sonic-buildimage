// Package applstate listens for orchagent's per-route programming results.
//
// After routeorch applies a ROUTE_TABLE write it publishes on the Redis
// channel APPL_DB_ROUTE_TABLE_RESPONSE_CHANNEL (response_publisher.cpp) a JSON
// array built by JSon::buildJson:
//
//	["SWSS_RC_SUCCESS", "<key>", "err_str", "", "protocol", "gribi"]
//
// The first element is the status, the second the route key; the rest are
// field/value pairs. Redis pub/sub is at-most-once, so a lost message is
// surfaced by the caller's timeout, never as a false success.
package applstate

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"log/slog"

	"github.com/redis/go-redis/v9"
)

// Channel is where routeorch publishes ROUTE_TABLE results.
const Channel = "APPL_DB_ROUTE_TABLE_RESPONSE_CHANNEL"

const success = "SWSS_RC_SUCCESS"

// Listener subscribes to Channel and reports each result.
type Listener struct {
	rdb   *redis.Client
	on    func(key string, err error)
	log   *slog.Logger
	ready chan struct{}
}

// New builds a listener; on is called for every well-formed result with a nil
// error for SWSS_RC_SUCCESS and "<status>: <err_str>" otherwise.
func New(rdb *redis.Client, on func(key string, err error), log *slog.Logger) *Listener {
	return &Listener{rdb: rdb, on: on, log: log, ready: make(chan struct{})}
}

// Ready is closed once Redis has confirmed the subscription. Nothing published
// before that point is seen, so the caller must wait on it before it lets a
// gRIBI client program anything.
func (l *Listener) Ready() <-chan struct{} { return l.ready }

// Run blocks until ctx is cancelled or the subscription cannot be established.
func (l *Listener) Run(ctx context.Context) error {
	ps := l.rdb.Subscribe(ctx, Channel)
	defer ps.Close()

	// Receive returns the *redis.Subscription confirmation first.
	if _, err := ps.Receive(ctx); err != nil {
		return fmt.Errorf("applstate: subscribe %s: %w", Channel, err)
	}
	close(l.ready)

	// Channel() reconnects and resubscribes on connection loss, where the
	// blocking Receive* calls would return an error and ignore ctx. Messages
	// published during an outage are lost either way; the caller's timeout
	// turns those into FIB_FAILED.
	msgs := ps.Channel()
	for {
		select {
		case <-ctx.Done():
			return nil
		case msg, ok := <-msgs:
			if !ok {
				return errors.New("applstate: subscription channel closed")
			}
			key, res, err := parse(msg.Payload)
			if err != nil {
				l.log.Warn("ignoring malformed route response", "payload", msg.Payload, "error", err)
				continue
			}
			l.on(key, res)
		}
	}
}

// parse splits a payload into the route key and its outcome.
func parse(payload string) (key string, result error, err error) {
	var arr []string
	if err := json.Unmarshal([]byte(payload), &arr); err != nil {
		return "", nil, err
	}
	if len(arr) < 2 || len(arr)%2 != 0 {
		return "", nil, fmt.Errorf("want an even list of at least 2 strings, got %d", len(arr))
	}
	status, key := arr[0], arr[1]
	if key == "" {
		return "", nil, errors.New("empty route key")
	}
	if status == success {
		return key, nil, nil
	}
	var errStr string
	for i := 2; i+1 < len(arr); i += 2 {
		if arr[i] == "err_str" {
			errStr = arr[i+1]
		}
	}
	return key, fmt.Errorf("%s: %s", status, errStr), nil
}
