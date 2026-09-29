package fibtrack

import (
	"context"
	"errors"
	"sync"
	"testing"
	"time"
)

const key = "10.20.0.0/16"

func TestAwaitAfterDone(t *testing.T) {
	tr := New()
	tr.Sent(key)
	got := make(chan error, 1)
	go func() {
		ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		got <- tr.Await(ctx, key)
	}()
	time.Sleep(20 * time.Millisecond)
	tr.Done(key, nil)
	if err := <-got; err != nil {
		t.Fatalf("Await = %v, want nil", err)
	}
}

func TestDoneBeforeAwait(t *testing.T) {
	tr := New()
	tr.Sent(key)
	want := errors.New("SWSS_RC_INVALID_PARAM: bad interface")
	tr.Done(key, want)
	if err := tr.Await(context.Background(), key); !errors.Is(err, want) {
		t.Fatalf("Await = %v, want %v", err, want)
	}
}

func TestCoalescedSendsShareOutcome(t *testing.T) {
	tr := New()
	tr.Sent(key)
	tr.Sent(key)
	tr.Done(key, nil)
	for i := 0; i < 2; i++ {
		if err := tr.Await(context.Background(), key); err != nil {
			t.Fatalf("Await %d = %v, want nil", i, err)
		}
	}
}

func TestSentAfterDoneStartsNewSlot(t *testing.T) {
	tr := New()
	tr.Sent(key)
	tr.Done(key, nil)
	tr.Sent(key)
	ctx, cancel := context.WithTimeout(context.Background(), 50*time.Millisecond)
	defer cancel()
	if err := tr.Await(ctx, key); !errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("Await on a fresh slot = %v, want deadline exceeded", err)
	}
	tr.Done(key, nil)
	if err := tr.Await(context.Background(), key); err != nil {
		t.Fatalf("Await after second Done = %v", err)
	}
}

func TestAwaitTimeout(t *testing.T) {
	tr := New()
	tr.Sent(key)
	ctx, cancel := context.WithTimeout(context.Background(), 50*time.Millisecond)
	defer cancel()
	if err := tr.Await(ctx, key); !errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("Await = %v, want deadline exceeded", err)
	}
}

func TestAwaitWithoutSentFails(t *testing.T) {
	tr := New()
	if err := tr.Await(context.Background(), key); err == nil {
		t.Fatal("Await with nothing sent returned nil")
	}
}

func TestDoneWithoutSlotIgnored(t *testing.T) {
	tr := New()
	tr.Done("10.99.0.0/16", nil) // an fpmsyncd route answered on the shared channel
	tr.Sent(key)
	ctx, cancel := context.WithTimeout(context.Background(), 50*time.Millisecond)
	defer cancel()
	if err := tr.Await(ctx, key); !errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("a stray Done resolved an unrelated key: %v", err)
	}
}

func TestKeysIndependent(t *testing.T) {
	tr := New()
	tr.Sent("a")
	tr.Sent("b")
	tr.Done("b", errors.New("boom"))
	if err := tr.Await(context.Background(), "b"); err == nil {
		t.Fatal("b should have failed")
	}
	ctx, cancel := context.WithTimeout(context.Background(), 50*time.Millisecond)
	defer cancel()
	if err := tr.Await(ctx, "a"); !errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("a resolved from b's response: %v", err)
	}
}

func TestDeleteOfUninstalledKeyResolvesImmediately(t *testing.T) {
	tr := New()
	// Never programmed: routeorch will not answer the DEL.
	tr.SentDelete(key)
	if err := tr.Await(context.Background(), key); err != nil {
		t.Fatalf("delete of a never-installed key = %v, want nil", err)
	}

	// Programmed but rejected: still uninstalled.
	tr.Sent(key)
	tr.Done(key, errors.New("SWSS_RC_INVALID_PARAM"))
	tr.SentDelete(key)
	if err := tr.Await(context.Background(), key); err != nil {
		t.Fatalf("delete after a failed SET = %v, want nil", err)
	}
}

func TestDeleteOfInstalledKeyWaitsForResponse(t *testing.T) {
	tr := New()
	tr.Sent(key)
	tr.Done(key, nil)
	tr.SentDelete(key)
	ctx, cancel := context.WithTimeout(context.Background(), 50*time.Millisecond)
	defer cancel()
	if err := tr.Await(ctx, key); !errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("delete of an installed key resolved without a response: %v", err)
	}
	tr.Done(key, nil)
	if err := tr.Await(context.Background(), key); err != nil {
		t.Fatalf("Await after DEL response = %v", err)
	}
	// The key is gone now, so a second DEL needs no response either.
	tr.SentDelete(key)
	if err := tr.Await(context.Background(), key); err != nil {
		t.Fatalf("second delete = %v, want nil", err)
	}
}

func TestFailedReplaceKeepsInstalled(t *testing.T) {
	tr := New()
	tr.Sent(key)
	tr.Done(key, nil)
	// A replace the adapter could not render never reached orchagent; the
	// old route is still in the FIB, so a later DEL must wait for its ack.
	tr.Sent(key)
	tr.Done(key, errors.New("unresolved next hop"))
	tr.SentDelete(key)
	ctx, cancel := context.WithTimeout(context.Background(), 50*time.Millisecond)
	defer cancel()
	if err := tr.Await(ctx, key); !errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("delete after failed replace resolved without a response: %v", err)
	}
}

func TestConcurrentUse(t *testing.T) {
	tr := New()
	const n = 64
	var wg sync.WaitGroup
	for i := 0; i < n; i++ {
		k := string(rune('a'+i%26)) + "/" + string(rune('0'+i%10))
		wg.Add(3)
		go func() { defer wg.Done(); tr.Sent(k) }()
		go func() { defer wg.Done(); tr.Done(k, nil) }()
		go func() {
			defer wg.Done()
			ctx, cancel := context.WithTimeout(context.Background(), 200*time.Millisecond)
			defer cancel()
			_ = tr.Await(ctx, k)
		}()
	}
	wg.Wait()
}
