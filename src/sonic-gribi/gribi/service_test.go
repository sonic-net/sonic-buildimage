package gribi

import (
	"context"
	"errors"
	"fmt"
	"io"
	"log/slog"
	"net"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/openconfig/gribigo/aft"
	"github.com/openconfig/gribigo/chk"
	"github.com/openconfig/gribigo/constants"
	"github.com/openconfig/gribigo/fluent"
	"github.com/openconfig/gribigo/server"
	"github.com/openconfig/ygot/ygot"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/status"

	spb "github.com/openconfig/gribi/v1/proto/service"

	"github.com/sonic-net/sonic-gribi/fibtrack"
	"github.com/sonic-net/sonic-gribi/routekey"
)

const ni = server.DefaultNetworkInstanceName

type testWriter struct{ t *testing.T }

func (w testWriter) Write(p []byte) (int, error) {
	w.t.Log(strings.TrimSpace(string(p)))
	return len(p), nil
}

// rig is a real gribigo server behind the wrapper, with a hook that only
// announces route writes to the tracker; the test plays orchagent.
type rig struct {
	tracker *fibtrack.Tracker
	svc     *Service
	addr    string

	mu      sync.Mutex
	sent    []string // route keys the hook announced, in order
	deleted []string
	sentCh  chan string
}

func newRig(t *testing.T, ackTimeout time.Duration, vrfs ...string) *rig {
	t.Helper()
	r := &rig{tracker: fibtrack.New(), sentCh: make(chan string, 1024)}
	gs, err := server.New(server.WithPostChangeRIBHook(r.hook), server.WithVRFs(vrfs))
	if err != nil {
		t.Fatalf("server.New: %v", err)
	}
	log := slog.New(slog.NewTextHandler(testWriter{t}, &slog.HandlerOptions{Level: slog.LevelDebug}))
	svc := New(gs, r.tracker, ackTimeout, log)
	r.svc = svc

	lis, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatalf("listen: %v", err)
	}
	grpcSrv := grpc.NewServer()
	spb.RegisterGRIBIServer(grpcSrv, svc)
	go grpcSrv.Serve(lis)
	t.Cleanup(grpcSrv.Stop)
	r.addr = lis.Addr().String()
	return r
}

func (r *rig) hook(op constants.OpType, _ int64, n string, e ygot.ValidatedGoStruct) {
	var prefix string
	switch v := e.(type) {
	case *aft.Afts_Ipv4Entry:
		prefix = v.GetPrefix()
	case *aft.Afts_Ipv6Entry:
		prefix = v.GetPrefix()
	default:
		return
	}
	key := routekey.For(n, prefix)
	r.mu.Lock()
	defer r.mu.Unlock()
	switch op {
	case constants.Delete:
		r.tracker.SentDelete(key)
		r.deleted = append(r.deleted, key)
	default:
		r.tracker.Sent(key)
		r.sent = append(r.sent, key)
	}
	r.sentCh <- key
}

// waitSent blocks until the hook has announced key.
func (r *rig) waitSent(t *testing.T, key string) {
	t.Helper()
	deadline := time.After(5 * time.Second)
	for {
		select {
		case k := <-r.sentCh:
			if k == key {
				return
			}
		case <-deadline:
			t.Fatalf("hook never saw %s", key)
		}
	}
}

func (r *rig) client(t *testing.T, ctx context.Context, fibAck bool, election uint64) *fluent.GRIBIClient {
	t.Helper()
	conn, err := grpc.NewClient(r.addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		t.Fatalf("dial: %v", err)
	}
	t.Cleanup(func() { conn.Close() })
	c := fluent.NewClient()
	conf := c.Connection().WithStub(spb.NewGRIBIClient(conn)).
		WithRedundancyMode(fluent.ElectedPrimaryClient).
		WithPersistence().
		WithInitialElectionID(election, 0)
	if fibAck {
		conf.WithFIBACK()
	}
	c.Start(ctx, t)
	t.Cleanup(func() { c.Stop(t) })
	c.StartSending(ctx, t)
	if err := c.Await(ctx, t); err != nil {
		t.Fatalf("session setup: %v", err)
	}
	return c
}

func await(t *testing.T, c *fluent.GRIBIClient, d time.Duration) error {
	t.Helper()
	ctx, cancel := context.WithTimeout(context.Background(), d)
	defer cancel()
	return c.Await(ctx, t)
}

func topology(prefix string) []fluent.GRIBIEntry {
	return []fluent.GRIBIEntry{
		fluent.NextHopEntry().WithNetworkInstance(ni).WithIndex(1).WithIPAddress("10.0.0.57"),
		fluent.NextHopGroupEntry().WithNetworkInstance(ni).WithID(10).AddNextHop(1, 1),
		fluent.IPv4Entry().WithNetworkInstance(ni).WithPrefix(prefix).WithNextHopGroup(10),
	}
}

func wantRoute(t *testing.T, c *fluent.GRIBIClient, prefix string, op constants.OpType, pr fluent.ProgrammingResult) {
	t.Helper()
	chk.HasResult(t, c.Results(t),
		fluent.OperationResult().WithIPv4Operation(prefix).WithOperationType(op).WithProgrammingResult(pr).AsResult(),
		chk.IgnoreOperationID())
}

func hasRouteFIB(c *fluent.GRIBIClient, t *testing.T, prefix string) bool {
	for _, r := range c.Results(t) {
		if r.Details != nil && r.Details.IPv4Prefix == prefix && r.ProgrammingResult == spb.AFTResult_FIB_PROGRAMMED {
			return true
		}
	}
	return false
}

func TestFIBProgrammedWaitsForDone(t *testing.T) {
	r := newRig(t, 30*time.Second)
	ctx := context.Background()
	c := r.client(t, ctx, true, 1)
	c.Modify().AddEntry(t, topology("10.20.0.0/16")...)

	r.waitSent(t, "10.20.0.0/16")
	// gribigo has answered RIB_PROGRAMMED by now; FIB_PROGRAMMED must not be there.
	time.Sleep(200 * time.Millisecond)
	if hasRouteFIB(c, t, "10.20.0.0/16") {
		t.Fatal("FIB_PROGRAMMED arrived before orchagent answered")
	}
	wantRoute(t, c, "10.20.0.0/16", constants.Add, fluent.InstalledInRIB)

	r.tracker.Done("10.20.0.0/16", nil)
	if err := await(t, c, 5*time.Second); err != nil {
		t.Fatalf("Await: %v", err)
	}
	wantRoute(t, c, "10.20.0.0/16", constants.Add, fluent.InstalledInFIB)

	// Deleting: the route is installed, so the DEL waits for its answer too.
	c.Modify().DeleteEntry(t, fluent.IPv4Entry().WithNetworkInstance(ni).WithPrefix("10.20.0.0/16").WithNextHopGroup(10))
	r.waitSent(t, "10.20.0.0/16")
	if err := await(t, c, 300*time.Millisecond); err == nil {
		t.Fatal("delete converged without orchagent's answer")
	}
	r.tracker.Done("10.20.0.0/16", nil)
	if err := await(t, c, 5*time.Second); err != nil {
		t.Fatalf("Await after DEL ack: %v", err)
	}
	wantRoute(t, c, "10.20.0.0/16", constants.Delete, fluent.InstalledInFIB)
}

// Through a real gribigo server: its VRF instances are created after the
// hook is set, and must still call it.
func TestVRFRouteReachesHook(t *testing.T) {
	r := newRig(t, 30*time.Second, "blue")
	ctx := context.Background()
	c := r.client(t, ctx, true, 1)
	c.Modify().AddEntry(t,
		fluent.NextHopEntry().WithNetworkInstance("blue").WithIndex(1).WithIPAddress("10.0.0.57"),
		fluent.NextHopGroupEntry().WithNetworkInstance("blue").WithID(10).AddNextHop(1, 1),
		fluent.IPv4Entry().WithNetworkInstance("blue").WithPrefix("10.30.0.0/16").WithNextHopGroup(10),
	)
	r.waitSent(t, "Vrfblue:10.30.0.0/16")
	r.tracker.Done("Vrfblue:10.30.0.0/16", nil)
	if err := await(t, c, 5*time.Second); err != nil {
		t.Fatalf("Await: %v", err)
	}
	wantRoute(t, c, "10.30.0.0/16", constants.Add, fluent.InstalledInFIB)
}

// A client that sends its batch and half-closes, as grpcurl does, must still
// get the FIB result that is held back until orchagent answers.
func TestFIBResultSurvivesClientHalfClose(t *testing.T) {
	r := newRig(t, 30*time.Second)
	conn, err := grpc.NewClient(r.addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		t.Fatalf("dial: %v", err)
	}
	t.Cleanup(func() { conn.Close() })
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	stream, err := spb.NewGRIBIClient(conn).Modify(ctx)
	if err != nil {
		t.Fatalf("Modify: %v", err)
	}

	reqs := []*spb.ModifyRequest{
		{Params: &spb.SessionParameters{
			Redundancy:  spb.SessionParameters_SINGLE_PRIMARY,
			Persistence: spb.SessionParameters_PRESERVE,
			AckType:     spb.SessionParameters_RIB_AND_FIB_ACK,
		}},
		{ElectionId: &spb.Uint128{Low: 1}},
	}
	for i, e := range topology("10.20.0.0/16") {
		op, err := e.OpProto()
		if err != nil {
			t.Fatalf("OpProto: %v", err)
		}
		op.Id = uint64(i + 1)
		op.Op = spb.AFTOperation_ADD
		op.ElectionId = &spb.Uint128{Low: 1}
		reqs = append(reqs, &spb.ModifyRequest{Operation: []*spb.AFTOperation{op}})
	}
	for _, req := range reqs {
		if err := stream.Send(req); err != nil {
			t.Fatalf("Send: %v", err)
		}
	}
	if err := stream.CloseSend(); err != nil {
		t.Fatalf("CloseSend: %v", err)
	}

	r.waitSent(t, "10.20.0.0/16")
	r.tracker.Done("10.20.0.0/16", nil)

	var routeStatus []spb.AFTResult_Status
	for {
		res, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			t.Fatalf("Recv: %v", err)
		}
		for _, ar := range res.GetResult() {
			if ar.GetId() == 3 {
				routeStatus = append(routeStatus, ar.GetStatus())
			}
		}
	}
	if len(routeStatus) == 0 || routeStatus[len(routeStatus)-1] != spb.AFTResult_FIB_PROGRAMMED {
		t.Fatalf("route results %v, want FIB_PROGRAMMED last", routeStatus)
	}
}

// A client that sends only its session parameters (and election ID) and
// half-closes must still get the replies to them.
func TestSessionRepliesSurviveClientHalfClose(t *testing.T) {
	r := newRig(t, 30*time.Second)
	conn, err := grpc.NewClient(r.addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		t.Fatalf("dial: %v", err)
	}
	t.Cleanup(func() { conn.Close() })
	for i := 0; i < 200; i++ {
		ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		stream, err := spb.NewGRIBIClient(conn).Modify(ctx)
		if err != nil {
			t.Fatalf("Modify: %v", err)
		}
		reqs := []*spb.ModifyRequest{{Params: &spb.SessionParameters{Redundancy: spb.SessionParameters_SINGLE_PRIMARY,
			Persistence: spb.SessionParameters_PRESERVE, AckType: spb.SessionParameters_RIB_ACK}}}
		withElection := i%2 == 1
		if withElection {
			reqs = append(reqs, &spb.ModifyRequest{ElectionId: &spb.Uint128{Low: uint64(i + 1)}})
		}
		for _, req := range reqs {
			if err := stream.Send(req); err != nil {
				t.Fatalf("Send: %v", err)
			}
		}
		if err := stream.CloseSend(); err != nil {
			t.Fatalf("CloseSend: %v", err)
		}
		var params, election bool
		for {
			res, err := stream.Recv()
			if err == io.EOF {
				break
			}
			if err != nil {
				t.Fatalf("Recv: %v", err)
			}
			params = params || res.GetSessionParamsResult() != nil
			election = election || res.GetElectionId() != nil
		}
		cancel()
		if !params || election != withElection {
			t.Fatalf("session %d: session_params_result %v, election_id %v (sent: %v)", i, params, election, withElection)
		}
	}
}

func TestFIBAckSessionRefusedWhenUnavailable(t *testing.T) {
	r := newRig(t, 30*time.Second)
	r.svc.RefuseFIBAck("suppress-fib-pending is disabled")
	conn, err := grpc.NewClient(r.addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		t.Fatalf("dial: %v", err)
	}
	t.Cleanup(func() { conn.Close() })
	for _, ack := range []spb.SessionParameters_AFTResultStatusType{spb.SessionParameters_RIB_AND_FIB_ACK, spb.SessionParameters_RIB_ACK} {
		ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		stream, err := spb.NewGRIBIClient(conn).Modify(ctx)
		if err != nil {
			t.Fatalf("Modify: %v", err)
		}
		if err := stream.Send(&spb.ModifyRequest{Params: &spb.SessionParameters{
			Redundancy: spb.SessionParameters_SINGLE_PRIMARY, Persistence: spb.SessionParameters_PRESERVE, AckType: ack,
		}}); err != nil {
			t.Fatalf("Send: %v", err)
		}
		_, err = stream.Recv()
		got := status.Code(err)
		cancel()
		switch ack {
		case spb.SessionParameters_RIB_AND_FIB_ACK:
			if got != codes.FailedPrecondition || !strings.Contains(err.Error(), "suppress-fib-pending") {
				t.Errorf("RIB_AND_FIB_ACK session: %v, want FailedPrecondition naming the reason", err)
			}
		default:
			if err != nil {
				t.Errorf("RIB_ACK session refused: %v", err)
			}
		}
	}
}

func TestFIBFailedOnError(t *testing.T) {
	r := newRig(t, 30*time.Second)
	ctx := context.Background()
	c := r.client(t, ctx, true, 1)
	c.Modify().AddEntry(t, topology("10.20.0.0/16")...)
	r.waitSent(t, "10.20.0.0/16")
	r.tracker.Done("10.20.0.0/16", errors.New("SWSS_RC_INVALID_PARAM: [OrchAgent] bad interface"))
	if err := await(t, c, 5*time.Second); err != nil {
		t.Fatalf("Await: %v", err)
	}
	var got string
	for _, res := range c.Results(t) {
		if res.Details != nil && res.Details.IPv4Prefix == "10.20.0.0/16" && res.ProgrammingResult == spb.AFTResult_FIB_FAILED {
			got = res.ServerError
		}
	}
	if !strings.Contains(got, "SWSS_RC_INVALID_PARAM") {
		t.Fatalf("no FIB_FAILED carrying orchagent's error; results: %+v", c.Results(t))
	}
	if hasRouteFIB(c, t, "10.20.0.0/16") {
		t.Fatal("a failed route was also reported FIB_PROGRAMMED")
	}
}

func TestFIBFailedOnTimeout(t *testing.T) {
	r := newRig(t, 300*time.Millisecond)
	ctx := context.Background()
	c := r.client(t, ctx, true, 1)
	c.Modify().AddEntry(t, topology("10.20.0.0/16")...)
	if err := await(t, c, 5*time.Second); err != nil {
		t.Fatalf("Await: %v", err)
	}
	var got string
	for _, res := range c.Results(t) {
		if res.Details != nil && res.Details.IPv4Prefix == "10.20.0.0/16" && res.ProgrammingResult == spb.AFTResult_FIB_FAILED {
			got = res.ServerError
		}
	}
	if !strings.Contains(got, "no response from orchagent") {
		t.Fatalf("want a timeout FIB_FAILED, results: %+v", c.Results(t))
	}
}

func TestNextHopAndGroupAckImmediately(t *testing.T) {
	r := newRig(t, 30*time.Second)
	ctx := context.Background()
	c := r.client(t, ctx, true, 1)
	c.Modify().AddEntry(t,
		fluent.NextHopEntry().WithNetworkInstance(ni).WithIndex(1).WithIPAddress("10.0.0.57"),
		fluent.NextHopGroupEntry().WithNetworkInstance(ni).WithID(10).AddNextHop(1, 1),
	)
	if err := await(t, c, 5*time.Second); err != nil {
		t.Fatalf("Await: %v", err)
	}
	chk.HasResult(t, c.Results(t), fluent.OperationResult().WithNextHopOperation(1).WithOperationType(constants.Add).WithProgrammingResult(fluent.InstalledInFIB).AsResult(), chk.IgnoreOperationID())
	chk.HasResult(t, c.Results(t), fluent.OperationResult().WithNextHopGroupOperation(10).WithOperationType(constants.Add).WithProgrammingResult(fluent.InstalledInFIB).AsResult(), chk.IgnoreOperationID())
	if n := len(r.sent); n != 0 {
		t.Fatalf("hook announced %d route writes for a next-hop-only batch", n)
	}
}

func TestRIBAckSessionUnaffected(t *testing.T) {
	r := newRig(t, 30*time.Second)
	ctx := context.Background()
	c := r.client(t, ctx, false, 1)
	c.Modify().AddEntry(t, topology("10.20.0.0/16")...)
	// Nobody plays orchagent here; a RIB_ACK session must converge anyway.
	if err := await(t, c, 5*time.Second); err != nil {
		t.Fatalf("Await: %v", err)
	}
	wantRoute(t, c, "10.20.0.0/16", constants.Add, fluent.InstalledInRIB)
	for _, res := range c.Results(t) {
		if res.ProgrammingResult == spb.AFTResult_FIB_PROGRAMMED || res.ProgrammingResult == spb.AFTResult_FIB_FAILED {
			t.Fatalf("RIB_ACK session received a FIB result: %+v", res)
		}
	}
}

func TestConcurrentSends(t *testing.T) {
	r := newRig(t, 30*time.Second)
	ctx := context.Background()
	c := r.client(t, ctx, true, 1)

	const n = 60
	entries := []fluent.GRIBIEntry{
		fluent.NextHopEntry().WithNetworkInstance(ni).WithIndex(1).WithIPAddress("10.0.0.57"),
		fluent.NextHopGroupEntry().WithNetworkInstance(ni).WithID(10).AddNextHop(1, 1),
	}
	for i := 0; i < n; i++ {
		entries = append(entries, fluent.IPv4Entry().WithNetworkInstance(ni).WithPrefix(fmt.Sprintf("10.%d.0.0/16", 20+i)).WithNextHopGroup(10))
	}
	c.Modify().AddEntry(t, entries...)

	// Answer each route as soon as the hook announces it, from another
	// goroutine, so FIB results race gribigo's own RIB results on the stream.
	done := make(chan struct{})
	go func() {
		defer close(done)
		for i := 0; i < n; i++ {
			select {
			case k := <-r.sentCh:
				r.tracker.Done(k, nil)
			case <-time.After(10 * time.Second):
				return
			}
		}
	}()
	if err := await(t, c, 20*time.Second); err != nil {
		t.Fatalf("Await: %v", err)
	}
	<-done
	for i := 0; i < n; i++ {
		wantRoute(t, c, fmt.Sprintf("10.%d.0.0/16", 20+i), constants.Add, fluent.InstalledInFIB)
	}
}

func TestGetAndFlushDelegate(t *testing.T) {
	r := newRig(t, 30*time.Second)
	ctx := context.Background()
	c := r.client(t, ctx, true, 1)
	c.Modify().AddEntry(t, topology("10.20.0.0/16")...)
	r.waitSent(t, "10.20.0.0/16")
	r.tracker.Done("10.20.0.0/16", nil)
	if err := await(t, c, 5*time.Second); err != nil {
		t.Fatalf("Await: %v", err)
	}

	got, err := c.Get().WithNetworkInstance(ni).WithAFT(fluent.AllAFTs).Send()
	if err != nil {
		t.Fatalf("Get: %v", err)
	}
	chk.GetResponseHasEntries(t, got, topology("10.20.0.0/16")...)

	if _, err := c.Flush().WithElectionOverride().WithAllNetworkInstances().Send(); err != nil {
		t.Fatalf("Flush: %v", err)
	}
	r.waitSent(t, "10.20.0.0/16")
	r.mu.Lock()
	deleted := append([]string(nil), r.deleted...)
	r.mu.Unlock()
	if len(deleted) != 1 || deleted[0] != "10.20.0.0/16" {
		t.Fatalf("Flush did not fire a Delete for the route: %v", deleted)
	}
	got, err = c.Get().WithNetworkInstance(ni).WithAFT(fluent.AllAFTs).Send()
	if err != nil {
		t.Fatalf("Get after Flush: %v", err)
	}
	if n := len(got.GetEntry()); n != 0 {
		t.Fatalf("Get after Flush returned %d entries", n)
	}
}
