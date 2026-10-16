package swsszmq

import (
	"context"
	"testing"
	"time"

	"github.com/go-zeromq/zmq4"
	"github.com/google/go-cmp/cmp"
)

// sink is the PULL side of the protocol, standing in for orchagent's
// ZmqServer on a loopback port the kernel picks.
type sink struct {
	sock zmq4.Socket
}

func newSink(t *testing.T) *sink {
	t.Helper()
	ctx, cancel := context.WithCancel(context.Background())
	t.Cleanup(cancel)
	sock := zmq4.NewPull(ctx)
	if err := sock.Listen("tcp://127.0.0.1:0"); err != nil {
		t.Fatalf("listen: %v", err)
	}
	t.Cleanup(func() { sock.Close() })
	return &sink{sock: sock}
}

func (s *sink) endpoint() string { return "tcp://" + s.sock.Addr().String() }

func (s *sink) recv(t *testing.T) (string, string, []Tuple) {
	t.Helper()
	type result struct {
		msg zmq4.Msg
		err error
	}
	ch := make(chan result, 1)
	go func() {
		msg, err := s.sock.Recv()
		ch <- result{msg, err}
	}()
	select {
	case r := <-ch:
		if r.err != nil {
			t.Fatalf("recv: %v", r.err)
		}
		if len(r.msg.Frames) != 1 {
			t.Fatalf("got %d frames, want 1", len(r.msg.Frames))
		}
		db, table, tuples, err := Decode(r.msg.Frames[0])
		if err != nil {
			t.Fatalf("decode: %v", err)
		}
		return db, table, tuples
	case <-time.After(10 * time.Second):
		t.Fatal("no frame arrived within 10s")
	}
	return "", "", nil
}

func TestProducerDeliversInOrder(t *testing.T) {
	s := newSink(t)
	ctx := context.Background()

	p, err := Dial(ctx, s.endpoint())
	if err != nil {
		t.Fatalf("Dial: %v", err)
	}
	defer p.Close()

	set := Tuple{Key: "10.20.0.0/16", Fields: []FieldValue{{Field: "nexthop", Value: "10.0.0.57"}, {Field: "protocol", Value: "gribi"}}}
	del := Tuple{Key: "10.20.0.0/16"}
	batch := []Tuple{{Key: "10.21.0.0/16", Fields: []FieldValue{{Field: "nexthop", Value: "10.0.0.59"}}}, {Key: "10.22.0.0/16"}}

	if err := p.Send(ctx, "ROUTE_TABLE", set); err != nil {
		t.Fatalf("send set: %v", err)
	}
	if err := p.Send(ctx, "ROUTE_TABLE", del); err != nil {
		t.Fatalf("send del: %v", err)
	}
	if err := p.Send(ctx, "ROUTE_TABLE", batch...); err != nil {
		t.Fatalf("send batch: %v", err)
	}
	if err := p.Send(ctx, "ROUTE_TABLE"); err != nil {
		t.Fatalf("send nothing: %v", err)
	}

	for i, want := range [][]Tuple{{set}, {del}, batch} {
		db, table, got := s.recv(t)
		if db != "APPL_DB" || table != "ROUTE_TABLE" {
			t.Fatalf("frame %d: db/table %q/%q", i, db, table)
		}
		if diff := cmp.Diff(want, got); diff != "" {
			t.Fatalf("frame %d differs (-want +got):\n%s", i, diff)
		}
	}
}

func TestDialFailsWithoutListener(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	// Port 9 is discard; nothing listens on loopback there on a dev box.
	if _, err := Dial(ctx, "tcp://127.0.0.1:9"); err == nil {
		t.Fatal("Dial succeeded with nothing listening")
	}
}
