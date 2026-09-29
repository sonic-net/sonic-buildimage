// Package gribi wraps gribigo's server so that FIB_PROGRAMMED means what it
// says.
//
// gribigo answers a RIB_AND_FIB_ACK client with FIB_PROGRAMMED the moment an
// entry lands in its RIB (server.go, modifyEntry: "we just say everything that
// was RIB programmed was FIB programmed"). This wrapper sits between gRPC and
// gribigo on the Modify stream: it watches requests go by to learn which
// operation ids are routes, and on the way back it holds each route's
// FIB_PROGRAMMED until orchagent has answered for that route key, then sends
// FIB_PROGRAMMED or FIB_FAILED. Next hops and next-hop groups have no SONiC
// object of their own and pass through untouched, as does everything for a
// RIB_ACK-only session.
package gribi

import (
	"context"
	"errors"
	"fmt"
	"io"
	"log/slog"
	"sync"
	"time"

	"github.com/openconfig/gribigo/server"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	spb "github.com/openconfig/gribi/v1/proto/service"

	"github.com/sonic-net/sonic-gribi/fibtrack"
	"github.com/sonic-net/sonic-gribi/routekey"
)

// Service implements spb.GRIBIServer over a gribigo server.
type Service struct {
	spb.UnimplementedGRIBIServer

	srv        *server.Server
	tracker    *fibtrack.Tracker
	ackTimeout time.Duration
	log        *slog.Logger
	// noFIBAck, when set, is why RIB_AND_FIB_ACK sessions are refused.
	noFIBAck string
}

// New wraps srv. ackTimeout bounds how long a route waits for orchagent
// before the client is told FIB_FAILED.
func New(srv *server.Server, tracker *fibtrack.Tracker, ackTimeout time.Duration, log *slog.Logger) *Service {
	return &Service{srv: srv, tracker: tracker, ackTimeout: ackTimeout, log: log}
}

// RefuseFIBAck makes Modify reject RIB_AND_FIB_ACK sessions with reason,
// for switches where orchagent does not report route results at all.
func (s *Service) RefuseFIBAck(reason string) { s.noFIBAck = reason }

// Modify runs gribigo's Modify over an intercepting stream.
//
// gribigo returns as soon as the client half-closes, which would end the RPC
// before held-back FIB results are sent. A client that sends its operations
// and then closes its side still gets every result, each within ackTimeout.
func (s *Service) Modify(stream spb.GRIBI_ModifyServer) error {
	ms := &modifyStream{GRIBI_ModifyServer: stream, svc: s, ops: map[uint64]string{}, unanswered: map[uint64]struct{}{}}
	ms.idle = sync.NewCond(&ms.mu)
	err := s.srv.Modify(ms)
	if ms.refused != nil {
		// gribigo re-wraps Recv errors as Unknown; keep the precondition code.
		return ms.refused
	}
	if err == nil {
		ms.drain()
	}
	return err
}

// Get delegates to gribigo.
func (s *Service) Get(req *spb.GetRequest, stream spb.GRIBI_GetServer) error {
	return s.srv.Get(req, stream)
}

// Flush delegates to gribigo. The RIB hook fires a Delete per entry, so the
// routes leave the switch too.
func (s *Service) Flush(ctx context.Context, req *spb.FlushRequest) (*spb.FlushResponse, error) {
	return s.srv.Flush(ctx, req)
}

// modifyStream is one client's Modify stream.
type modifyStream struct {
	spb.GRIBI_ModifyServer
	svc *Service

	mu sync.Mutex
	// ops holds the ROUTE_TABLE key of every route operation whose final
	// result has not been sent yet.
	ops    map[uint64]string
	fibAck bool
	// pending counts awaitFIB goroutines still to send their result; idle
	// is signalled when it reaches zero.
	pending int
	idle    *sync.Cond
	// unanswered holds every operation gribigo has not yet reported a final
	// result for; idle is also signalled when it empties.
	unanswered map[uint64]struct{}
	// replies counts session-parameter and election replies gribigo has yet
	// to send; idle is also signalled when it reaches zero.
	replies int
	refused error

	// sendMu serialises gribigo's own result goroutine with the goroutines
	// that deliver held-back FIB results.
	sendMu sync.Mutex
}

// Recv learns the session's ack mode and which operations are routes.
//
// gribigo stops sending results the moment Recv reports the client's
// half-close, even if the last operation's result is still on its way, so
// io.EOF is held back until every operation has had its result passed to
// Send, or ackTimeout has passed (an entry with an unresolved dependency is
// never answered).
func (m *modifyStream) Recv() (*spb.ModifyRequest, error) {
	req, err := m.GRIBI_ModifyServer.Recv()
	if err == io.EOF {
		m.awaitAnswers()
	}
	if err != nil {
		return req, err
	}
	m.mu.Lock()
	defer m.mu.Unlock()
	if p := req.GetParams(); p != nil {
		m.fibAck = p.GetAckType() == spb.SessionParameters_RIB_AND_FIB_ACK
		if m.fibAck && m.svc.noFIBAck != "" {
			m.refused = status.Errorf(codes.FailedPrecondition, "RIB_AND_FIB_ACK is unavailable: %s", m.svc.noFIBAck)
			return nil, m.refused
		}
		m.replies++
	}
	if req.GetElectionId() != nil {
		m.replies++
	}
	for _, op := range req.GetOperation() {
		m.unanswered[op.GetId()] = struct{}{}
		var prefix string
		switch e := op.GetEntry().(type) {
		case *spb.AFTOperation_Ipv4:
			prefix = e.Ipv4.GetPrefix()
		case *spb.AFTOperation_Ipv6:
			prefix = e.Ipv6.GetPrefix()
		default:
			continue
		}
		m.ops[op.GetId()] = routekey.For(op.GetNetworkInstance(), prefix)
	}
	return req, nil
}

type held struct {
	id  uint64
	key string
}

// Send forwards gribigo's response minus any FIB_PROGRAMMED for a route,
// which is re-issued once orchagent has answered.
//
// Operations and replies count as answered only once the response carrying
// them has been written, so a held-back EOF is never released while a
// response is still in flight.
func (m *modifyStream) Send(res *spb.ModifyResponse) error {
	if res == nil || len(res.GetResult()) == 0 {
		err := m.send(res)
		if res.GetSessionParamsResult() != nil || res.GetElectionId() != nil {
			m.answered(nil, 1)
		}
		return err
	}

	m.mu.Lock()
	keep := make([]*spb.AFTResult, 0, len(res.Result))
	var gated []held
	var final []uint64
	for _, r := range res.Result {
		switch st := r.GetStatus(); {
		case st == spb.AFTResult_FAILED, st == spb.AFTResult_FIB_FAILED, st == spb.AFTResult_FIB_PROGRAMMED,
			st == spb.AFTResult_RIB_PROGRAMMED && !m.fibAck:
			final = append(final, r.GetId())
		}
		key, isRoute := m.ops[r.GetId()]
		if !isRoute {
			keep = append(keep, r)
			continue
		}
		switch r.GetStatus() {
		case spb.AFTResult_FIB_PROGRAMMED:
			if m.fibAck {
				gated = append(gated, held{id: r.GetId(), key: key})
				m.pending++
				continue
			}
		case spb.AFTResult_FAILED, spb.AFTResult_FIB_FAILED:
			delete(m.ops, r.GetId())
		case spb.AFTResult_RIB_PROGRAMMED:
			if !m.fibAck {
				delete(m.ops, r.GetId())
			}
		}
		keep = append(keep, r)
	}
	m.mu.Unlock()
	defer m.answered(final, 0)

	// RIB_PROGRAMMED goes out before the FIB result can, even when orchagent
	// has already answered.
	if len(keep) > 0 {
		if len(keep) != len(res.Result) {
			res = &spb.ModifyResponse{Result: keep}
		}
		if err := m.send(res); err != nil {
			for range gated {
				m.done()
			}
			return err
		}
	}
	for _, g := range gated {
		go m.awaitFIB(g.id, g.key)
	}
	return nil
}

func (m *modifyStream) awaitFIB(id uint64, key string) {
	defer m.done()
	ctx, cancel := context.WithTimeout(m.Context(), m.svc.ackTimeout)
	defer cancel()
	err := m.svc.tracker.Await(ctx, key)

	m.mu.Lock()
	delete(m.ops, id)
	m.mu.Unlock()

	if m.Context().Err() != nil {
		return // the client is gone; nobody to tell
	}

	res := &spb.AFTResult{Id: id, Status: spb.AFTResult_FIB_PROGRAMMED}
	if err != nil {
		msg := err.Error()
		if errors.Is(err, context.DeadlineExceeded) {
			msg = fmt.Sprintf("no response from orchagent for %s within %s: next hop unresolved, route dropped without a response, or acknowledgement lost", key, m.svc.ackTimeout)
		}
		res.Status = spb.AFTResult_FIB_FAILED
		res.ErrorDetails = &spb.AFTErrorDetails{ErrorMessage: msg}
		m.svc.log.Warn("FIB programming failed", "op", id, "key", key, "error", msg)
	} else {
		m.svc.log.Debug("FIB programmed", "op", id, "key", key)
	}
	if err := m.send(&spb.ModifyResponse{Result: []*spb.AFTResult{res}}); err != nil {
		m.svc.log.Debug("could not deliver FIB result", "op", id, "key", key, "error", err)
	}
}

// answered records ops that have had their final result sent by gribigo,
// and replies that have been sent.
func (m *modifyStream) answered(ops []uint64, replies int) {
	m.mu.Lock()
	defer m.mu.Unlock()
	for _, id := range ops {
		delete(m.unanswered, id)
	}
	m.replies -= replies
	if len(m.unanswered) == 0 && m.replies <= 0 {
		m.idle.Broadcast()
	}
}

func (m *modifyStream) done() {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.pending--
	if m.pending == 0 {
		m.idle.Broadcast()
	}
}

// awaitAnswers blocks until gribigo has answered every operation and sent
// every reply, or ackTimeout has passed.
func (m *modifyStream) awaitAnswers() {
	deadline := time.Now().Add(m.svc.ackTimeout)
	t := time.AfterFunc(m.svc.ackTimeout, func() {
		m.mu.Lock()
		m.idle.Broadcast()
		m.mu.Unlock()
	})
	defer t.Stop()
	m.mu.Lock()
	defer m.mu.Unlock()
	for (len(m.unanswered) > 0 || m.replies > 0) && time.Now().Before(deadline) {
		m.idle.Wait()
	}
}

// drain blocks until every held-back FIB result has been sent.
func (m *modifyStream) drain() {
	m.mu.Lock()
	defer m.mu.Unlock()
	for m.pending > 0 {
		m.idle.Wait()
	}
}

func (m *modifyStream) send(res *spb.ModifyResponse) error {
	m.sendMu.Lock()
	defer m.sendMu.Unlock()
	return m.GRIBI_ModifyServer.Send(res)
}
