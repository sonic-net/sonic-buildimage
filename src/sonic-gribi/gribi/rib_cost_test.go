package gribi

import (
	"fmt"
	"testing"

	"github.com/openconfig/gribigo/rib"

	aftpb "github.com/openconfig/gribi/v1/proto/gribi_aft"
)

// These cover patches/gribigo-rib-add-o1.patch, which vendor/ carries and
// whose tests `go mod vendor` leaves out.

// ribWithIPv4Entries returns a RIBHolder holding n IPv4 entries.
func ribWithIPv4Entries(tb testing.TB, n int) *rib.RIBHolder {
	tb.Helper()
	r := rib.NewRIBHolder("DEFAULT")
	for i := 0; i < n; i++ {
		if _, _, err := r.AddIPv4(ipv4EntryN(i), false); err != nil {
			tb.Fatalf("cannot add entry %d: %v", i, err)
		}
	}
	return r
}

// ipv4EntryN returns the IPv4 entry for the nth /32 in 10.0.0.0/8.
func ipv4EntryN(i int) *aftpb.Afts_Ipv4EntryKey {
	return &aftpb.Afts_Ipv4EntryKey{
		Prefix:    fmt.Sprintf("10.%d.%d.%d/32", i>>16&255, i>>8&255, i&255),
		Ipv4Entry: &aftpb.Afts_Ipv4Entry{},
	}
}

// TestAddCostIndependentOfRIBSize guards against an add that walks the
// installed RIB, as gribigo's MergeStructInto did. Allocations are
// deterministic where timings are not, and that merge allocated in
// proportion to the RIB's size.
func TestAddCostIndependentOfRIBSize(t *testing.T) {
	allocs := func(n int) float64 {
		r := ribWithIPv4Entries(t, n)
		i := n
		return testing.AllocsPerRun(100, func() {
			if _, _, err := r.AddIPv4(ipv4EntryN(i), false); err != nil {
				t.Fatalf("cannot add entry %d: %v", i, err)
			}
			i++
		})
	}
	small, large := allocs(100), allocs(10000)
	if large > small*1.5 {
		t.Fatalf("allocations per IPv4 add grew with the RIB: %.0f at 100 entries, %.0f at 10000", small, large)
	}
}

func BenchmarkAddIPv4(b *testing.B) {
	for _, n := range []int{0, 10000, 100000} {
		b.Run(fmt.Sprintf("rib=%d", n), func(b *testing.B) {
			r := ribWithIPv4Entries(b, n)
			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				if _, _, err := r.AddIPv4(ipv4EntryN(n+i), false); err != nil {
					b.Fatalf("cannot add entry: %v", err)
				}
			}
		})
	}
}
