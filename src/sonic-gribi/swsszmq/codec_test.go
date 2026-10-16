package swsszmq

import (
	"bytes"
	"encoding/binary"
	"strings"
	"testing"

	"github.com/google/go-cmp/cmp"
)

func le(n int) []byte { return binary.LittleEndian.AppendUint64(nil, uint64(n)) }

func rawPair(a, b string) []byte {
	var out []byte
	out = append(out, le(len(a))...)
	out = append(out, a...)
	out = append(out, le(len(b))...)
	out = append(out, b...)
	return out
}

// TestEncodeGolden pins the byte layout against a hand-built frame so a codec
// change cannot silently drift from binaryserializer.h.
func TestEncodeGolden(t *testing.T) {
	tuples := []Tuple{
		{Key: "10.20.0.0/16", Fields: []FieldValue{
			{Field: "nexthop", Value: "10.0.0.57"},
			{Field: "protocol", Value: "gribi"},
		}},
		{Key: "10.30.0.0/16"},
	}

	var want []byte
	want = append(want, le(5)...) // (db,table) + (key,"2") + 2 fields + (key,"0")
	want = append(want, rawPair("APPL_DB", "ROUTE_TABLE")...)
	want = append(want, rawPair("10.20.0.0/16", "2")...)
	want = append(want, rawPair("nexthop", "10.0.0.57")...)
	want = append(want, rawPair("protocol", "gribi")...)
	want = append(want, rawPair("10.30.0.0/16", "0")...)

	got, err := Encode("APPL_DB", "ROUTE_TABLE", tuples)
	if err != nil {
		t.Fatalf("Encode: %v", err)
	}
	if !bytes.Equal(got, want) {
		t.Fatalf("frame mismatch\n got % x\nwant % x", got, want)
	}
}

func TestRoundTrip(t *testing.T) {
	cases := []struct {
		name   string
		db     string
		table  string
		tuples []Tuple
	}{
		{"del", "APPL_DB", "ROUTE_TABLE", []Tuple{{Key: "10.20.0.0/16"}}},
		{"batch", "APPL_DB", "ROUTE_TABLE", []Tuple{
			{Key: "10.20.0.0/16", Fields: []FieldValue{{Field: "nexthop", Value: "10.0.0.57,10.0.0.59"}, {Field: "ifname", Value: "PortChannel101,PortChannel102"}, {Field: "weight", Value: "1,1"}, {Field: "protocol", Value: "gribi"}}},
			{Key: "10.21.0.0/16"},
			{Key: "10.22.0.0/16", Fields: []FieldValue{{Field: "nexthop", Value: "10.0.0.61"}}},
		}},
		{"empty values", "APPL_DB", "ROUTE_TABLE", []Tuple{{Key: "k", Fields: []FieldValue{{Field: "weight", Value: ""}, {Field: "", Value: ""}}}}},
		{"ipv6 key", "APPL_DB", "ROUTE_TABLE", []Tuple{{Key: "2001:db8:20::/64", Fields: []FieldValue{{Field: "nexthop", Value: "fc00::72"}}}}},
		{"vrf key with slash", "APPL_DB", "ROUTE_TABLE", []Tuple{{Key: "Vrfblue:10.20.0.0/16", Fields: []FieldValue{{Field: "protocol", Value: "gribi"}}}}},
		{"no tuples", "APPL_DB", "LABEL_ROUTE_TABLE", nil},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			frame, err := Encode(tc.db, tc.table, tc.tuples)
			if err != nil {
				t.Fatalf("Encode: %v", err)
			}
			db, table, tuples, err := Decode(frame)
			if err != nil {
				t.Fatalf("Decode: %v", err)
			}
			if db != tc.db || table != tc.table {
				t.Fatalf("got db/table %q/%q, want %q/%q", db, table, tc.db, tc.table)
			}
			if diff := cmp.Diff(tc.tuples, tuples); diff != "" {
				t.Fatalf("tuples differ (-want +got):\n%s", diff)
			}
		})
	}
}

func TestDecodeTruncated(t *testing.T) {
	frame, err := Encode("APPL_DB", "ROUTE_TABLE", []Tuple{
		{Key: "10.20.0.0/16", Fields: []FieldValue{{Field: "nexthop", Value: "10.0.0.57"}}},
	})
	if err != nil {
		t.Fatal(err)
	}
	// Every strict prefix of a valid frame must be rejected, never misread.
	for n := 0; n < len(frame); n++ {
		if _, _, _, err := Decode(frame[:n]); err == nil {
			t.Errorf("Decode accepted a %d-byte prefix of a %d-byte frame", n, len(frame))
		}
	}
	if _, _, _, err := Decode(append(append([]byte{}, frame...), 0)); err == nil {
		t.Error("Decode accepted a frame with a trailing byte")
	}

	// A field count that promises more pairs than the frame holds.
	var lying []byte
	lying = append(lying, le(2)...)
	lying = append(lying, rawPair("APPL_DB", "ROUTE_TABLE")...)
	lying = append(lying, rawPair("10.20.0.0/16", "3")...)
	if _, _, _, err := Decode(lying); err == nil {
		t.Error("Decode accepted a field count larger than the remaining pairs")
	}

	var bad []byte
	bad = append(bad, le(2)...)
	bad = append(bad, rawPair("APPL_DB", "ROUTE_TABLE")...)
	bad = append(bad, rawPair("10.20.0.0/16", "x")...)
	if _, _, _, err := Decode(bad); err == nil {
		t.Error("Decode accepted a non-numeric field count")
	}
}

func TestEncodeOversize(t *testing.T) {
	big := strings.Repeat("x", MaxFrameSize)
	_, err := Encode("APPL_DB", "ROUTE_TABLE", []Tuple{{Key: "k", Fields: []FieldValue{{Field: "f", Value: big}}}})
	if err == nil {
		t.Fatal("Encode accepted a frame above MaxFrameSize")
	}
}
