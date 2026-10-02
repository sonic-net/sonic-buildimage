// Package fibtrack correlates ROUTE_TABLE writes with orchagent's per-key
// responses so a gRIBI client can be told when its route really reached the
// ASIC.
//
// orchagent coalesces every pending write to one key inside a drain batch
// into a single apply and a single response (RouteSyncMap in orch.h), so the
// tracker keeps one slot per key rather than one per operation. Two writes to
// a key that orchagent happens to merge therefore share one outcome, which is
// exactly what happened on the switch. The cost is a documented imprecision:
// two writes it did not merge, with different outcomes, are both reported
// with the first.
package fibtrack

import (
	"context"
	"fmt"
	"sync"
)

// Tracker is safe for concurrent use.
type Tracker struct {
	mu    sync.Mutex
	slots map[string]*slot
	// installed records whether orchagent last confirmed holding the key.
	// routeorch stays silent on a DEL for a route it never installed
	// (removeRoute returns early without publishing), so a delete of an
	// uninstalled key would otherwise wait out the timeout and be reported
	// as FIB_FAILED for a route that is, in fact, absent.
	installed map[string]bool
}

type slot struct {
	del      bool
	resolved bool
	err      error
	done     chan struct{}
}

// New returns an empty tracker.
func New() *Tracker {
	return &Tracker{slots: map[string]*slot{}, installed: map[string]bool{}}
}

// Sent records that a SET for key was pushed to orchagent. If an earlier
// write to the key is still unanswered the two share a slot, because
// orchagent will answer them once.
func (t *Tracker) Sent(key string) {
	t.mu.Lock()
	defer t.mu.Unlock()
	if s, ok := t.slots[key]; ok && !s.resolved {
		s.del = false
		return
	}
	t.slots[key] = &slot{done: make(chan struct{})}
}

// SentDelete records that a DEL for key was pushed. When orchagent never
// confirmed holding the key the slot resolves at once: no response is coming,
// and the key is already absent from the FIB.
func (t *Tracker) SentDelete(key string) {
	t.mu.Lock()
	defer t.mu.Unlock()
	if s, ok := t.slots[key]; ok && !s.resolved {
		s.del = true
		return
	}
	s := &slot{del: true, done: make(chan struct{})}
	if !t.installed[key] {
		s.resolved = true
		close(s.done)
	}
	t.slots[key] = s
}

// Done records orchagent's response for key. A nil err is SWSS_RC_SUCCESS.
// Responses for keys with no slot are ignored: fpmsyncd shares ROUTE_TABLE,
// and its routes are answered on the same channel.
func (t *Tracker) Done(key string, err error) {
	t.mu.Lock()
	defer t.mu.Unlock()
	s, ok := t.slots[key]
	if !ok {
		return
	}
	if err == nil {
		t.installed[key] = !s.del
	}
	s.err = err
	if !s.resolved {
		s.resolved = true
		close(s.done)
	}
}

// Await blocks until the slot for key resolves and returns its outcome. The
// slot stays until the next Sent replaces it, so a second operation that
// orchagent merged into the same write sees the same answer. Await fails when
// ctx expires first, or when nothing was ever sent for key.
func (t *Tracker) Await(ctx context.Context, key string) error {
	t.mu.Lock()
	s, ok := t.slots[key]
	t.mu.Unlock()
	if !ok {
		return fmt.Errorf("fibtrack: no write recorded for %q", key)
	}
	select {
	case <-s.done:
	case <-ctx.Done():
		return ctx.Err()
	}
	t.mu.Lock()
	defer t.mu.Unlock()
	return s.err
}
