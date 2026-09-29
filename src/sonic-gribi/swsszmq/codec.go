// Package swsszmq speaks swsscommon's ZeroMQ producer/consumer protocol.
//
// orchagent's ZmqServer receives one ZeroMQ frame per send and hands it to
// BinarySerializer::deserializeBuffer (sonic-swss-common/common/binaryserializer.h).
// The frame is a flat list of length-prefixed string pairs:
//
//	u64 pairCount
//	pairCount × { u64 len, bytes, u64 len, bytes }
//
// Lengths are size_t written in host byte order, which on every SONiC x86_64
// box is 8-byte little-endian. Pair 0 is (dbName, tableName). Each tuple then
// contributes (key, decimal fieldCount) followed by fieldCount (field, value)
// pairs; a fieldCount of zero is a DEL.
package swsszmq

import (
	"encoding/binary"
	"errors"
	"fmt"
	"strconv"
)

// MaxFrameSize mirrors MQ_RESPONSE_MAX_COUNT in zmqserver.h: the receive buffer
// orchagent allocates, and the bound ZmqClient::sendMsg enforces on its side.
const MaxFrameSize = 16 * 1024 * 1024

const lenSize = 8

// FieldValue is one hash field of a SET.
type FieldValue struct {
	Field string
	Value string
}

// Tuple is one table operation. No fields means DEL, exactly as the wire
// format has it; there is no separate op code.
type Tuple struct {
	Key    string
	Fields []FieldValue
}

// IsDelete reports whether the tuple deletes its key.
func (t Tuple) IsDelete() bool { return len(t.Fields) == 0 }

// Encode serialises tuples for db/table into one frame.
func Encode(db, table string, tuples []Tuple) ([]byte, error) {
	pairs := 1
	size := lenSize + pairSize(db, table)
	for _, t := range tuples {
		count := strconv.Itoa(len(t.Fields))
		pairs += 1 + len(t.Fields)
		size += pairSize(t.Key, count)
		for _, fv := range t.Fields {
			size += pairSize(fv.Field, fv.Value)
		}
	}
	if size >= MaxFrameSize {
		return nil, fmt.Errorf("swsszmq: frame of %d bytes exceeds the %d byte limit", size, MaxFrameSize)
	}

	buf := make([]byte, 0, size)
	buf = binary.LittleEndian.AppendUint64(buf, uint64(pairs))
	buf = appendPair(buf, db, table)
	for _, t := range tuples {
		buf = appendPair(buf, t.Key, strconv.Itoa(len(t.Fields)))
		for _, fv := range t.Fields {
			buf = appendPair(buf, fv.Field, fv.Value)
		}
	}
	return buf, nil
}

func pairSize(a, b string) int { return 2*lenSize + len(a) + len(b) }

func appendPair(buf []byte, a, b string) []byte {
	buf = binary.LittleEndian.AppendUint64(buf, uint64(len(a)))
	buf = append(buf, a...)
	buf = binary.LittleEndian.AppendUint64(buf, uint64(len(b)))
	buf = append(buf, b...)
	return buf
}

// Decode parses one frame back into its db, table and tuples.
func Decode(frame []byte) (db, table string, tuples []Tuple, err error) {
	r := reader{buf: frame}
	pairs, err := r.u64()
	if err != nil {
		return "", "", nil, err
	}
	if pairs == 0 {
		return "", "", nil, errors.New("swsszmq: frame has no pairs")
	}
	if db, table, err = r.pair(); err != nil {
		return "", "", nil, err
	}
	for i := uint64(1); i < pairs; {
		key, countStr, err := r.pair()
		if err != nil {
			return "", "", nil, err
		}
		i++
		count, err := strconv.Atoi(countStr)
		if err != nil || count < 0 {
			return "", "", nil, fmt.Errorf("swsszmq: bad field count %q for key %q", countStr, key)
		}
		if uint64(count) > pairs-i {
			return "", "", nil, fmt.Errorf("swsszmq: key %q declares %d fields but only %d pairs remain", key, count, pairs-i)
		}
		t := Tuple{Key: key}
		for j := 0; j < count; j++ {
			f, v, err := r.pair()
			if err != nil {
				return "", "", nil, err
			}
			t.Fields = append(t.Fields, FieldValue{Field: f, Value: v})
		}
		i += uint64(count)
		tuples = append(tuples, t)
	}
	if r.pos != len(r.buf) {
		return "", "", nil, fmt.Errorf("swsszmq: %d trailing bytes after the last pair", len(r.buf)-r.pos)
	}
	return db, table, tuples, nil
}

type reader struct {
	buf []byte
	pos int
}

func (r *reader) u64() (uint64, error) {
	if len(r.buf)-r.pos < lenSize {
		return 0, fmt.Errorf("swsszmq: truncated length at offset %d", r.pos)
	}
	v := binary.LittleEndian.Uint64(r.buf[r.pos:])
	r.pos += lenSize
	return v, nil
}

func (r *reader) str() (string, error) {
	n, err := r.u64()
	if err != nil {
		return "", err
	}
	if n > uint64(len(r.buf)-r.pos) {
		return "", fmt.Errorf("swsszmq: string of %d bytes at offset %d overruns the frame", n, r.pos)
	}
	s := string(r.buf[r.pos : r.pos+int(n)])
	r.pos += int(n)
	return s, nil
}

func (r *reader) pair() (string, string, error) {
	a, err := r.str()
	if err != nil {
		return "", "", err
	}
	b, err := r.str()
	if err != nil {
		return "", "", err
	}
	return a, b, nil
}
