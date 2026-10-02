package applstate

import (
	"context"
	"log/slog"
	"sync"
	"testing"
	"time"

	"github.com/alicebob/miniredis/v2"
	"github.com/redis/go-redis/v9"
)

type result struct {
	key string
	err error
}

type harness struct {
	mr      *miniredis.Miniredis
	results chan result
	l       *Listener
	cancel  context.CancelFunc
	runErr  chan error
}

func start(t *testing.T) *harness {
	t.Helper()
	mr := miniredis.RunT(t)
	rdb := redis.NewClient(&redis.Options{Addr: mr.Addr()})
	t.Cleanup(func() { rdb.Close() })

	h := &harness{mr: mr, results: make(chan result, 16), runErr: make(chan error, 1)}
	h.l = New(rdb, func(key string, err error) { h.results <- result{key, err} }, slog.New(slog.NewTextHandler(testWriter{t}, nil)))
	ctx, cancel := context.WithCancel(context.Background())
	h.cancel = cancel
	t.Cleanup(cancel)
	go func() { h.runErr <- h.l.Run(ctx) }()
	select {
	case <-h.l.Ready():
	case <-time.After(5 * time.Second):
		t.Fatal("listener never became ready")
	}
	return h
}

func (h *harness) publish(t *testing.T, payload string) {
	t.Helper()
	h.mr.Publish(Channel, payload)
}

func (h *harness) next(t *testing.T) result {
	t.Helper()
	select {
	case r := <-h.results:
		return r
	case <-time.After(5 * time.Second):
		t.Fatal("no result within 5s")
	}
	return result{}
}

type testWriter struct{ t *testing.T }

func (w testWriter) Write(p []byte) (int, error) { w.t.Log(string(p)); return len(p), nil }

func TestSuccessPayload(t *testing.T) {
	h := start(t)
	h.publish(t, `["SWSS_RC_SUCCESS","10.20.0.0/16","err_str","","protocol","gribi"]`)
	r := h.next(t)
	if r.key != "10.20.0.0/16" || r.err != nil {
		t.Fatalf("got %+v", r)
	}
	// A DEL response carries only err_str.
	h.publish(t, `["SWSS_RC_SUCCESS","2001:db8:20::/64","err_str",""]`)
	if r := h.next(t); r.key != "2001:db8:20::/64" || r.err != nil {
		t.Fatalf("got %+v", r)
	}
}

func TestFailurePayloadCarriesStatusAndErrStr(t *testing.T) {
	h := start(t)
	h.publish(t, `["SWSS_RC_INVALID_PARAM","Vrfblue:10.20.0.0/16","err_str","[OrchAgent] VRF does not exist","protocol","gribi"]`)
	r := h.next(t)
	if r.key != "Vrfblue:10.20.0.0/16" || r.err == nil {
		t.Fatalf("got %+v", r)
	}
	if want := "SWSS_RC_INVALID_PARAM: [OrchAgent] VRF does not exist"; r.err.Error() != want {
		t.Fatalf("error = %q, want %q", r.err, want)
	}
}

func TestMalformedPayloadIgnored(t *testing.T) {
	h := start(t)
	for _, p := range []string{`not json`, `[]`, `["SWSS_RC_SUCCESS"]`, `["SWSS_RC_SUCCESS",""]`, `["SWSS_RC_SUCCESS","k","odd"]`, `{"a":1}`, `[1,2]`} {
		h.publish(t, p)
	}
	h.publish(t, `["SWSS_RC_SUCCESS","10.20.0.0/16","err_str",""]`)
	if r := h.next(t); r.key != "10.20.0.0/16" || r.err != nil {
		t.Fatalf("good payload after bad ones: %+v", r)
	}
	select {
	case r := <-h.results:
		t.Fatalf("a malformed payload produced a result: %+v", r)
	case <-time.After(100 * time.Millisecond):
	}
}

func TestReadyAfterSubscribe(t *testing.T) {
	mr := miniredis.RunT(t)
	rdb := redis.NewClient(&redis.Options{Addr: mr.Addr()})
	t.Cleanup(func() { rdb.Close() })
	l := New(rdb, func(string, error) {}, slog.Default())
	select {
	case <-l.Ready():
		t.Fatal("Ready closed before Run")
	default:
	}
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	go l.Run(ctx)
	select {
	case <-l.Ready():
	case <-time.After(5 * time.Second):
		t.Fatal("Ready never closed")
	}
	// A message published after Ready must be delivered.
	var once sync.Once
	got := make(chan string, 1)
	l2 := New(rdb, func(k string, _ error) { once.Do(func() { got <- k }) }, slog.Default())
	go l2.Run(ctx)
	<-l2.Ready()
	mr.Publish(Channel, `["SWSS_RC_SUCCESS","10.20.0.0/16","err_str",""]`)
	select {
	case k := <-got:
		if k != "10.20.0.0/16" {
			t.Fatalf("got %q", k)
		}
	case <-time.After(5 * time.Second):
		t.Fatal("message published after Ready was not delivered")
	}
}

func TestStopsOnCancel(t *testing.T) {
	h := start(t)
	h.cancel()
	select {
	case err := <-h.runErr:
		if err != nil {
			t.Fatalf("Run returned %v on cancel, want nil", err)
		}
	case <-time.After(5 * time.Second):
		t.Fatal("Run did not return after cancel")
	}
}
