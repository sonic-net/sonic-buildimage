// sonic-gribi: a gRIBI server whose backend is SONiC's ZeroMQ
// route channel into orchagent.
//
// gribigo / gribi / grpc track platform/alpinevs/src/libsai-grpc/lemming/go.mod;
// go-redis / miniredis track src/sonic-gnmi/go.mod, so upstreaming stays boring.
module github.com/sonic-net/sonic-gribi

go 1.24

require (
	github.com/alicebob/miniredis/v2 v2.35.0
	github.com/go-zeromq/zmq4 v0.17.0
	github.com/google/go-cmp v0.7.0
	github.com/openconfig/gribi v1.9.1
	github.com/openconfig/gribigo v0.1.3
	github.com/openconfig/ygot v0.32.0
	github.com/redis/go-redis/v9 v9.14.1
	google.golang.org/grpc v1.70.0
	google.golang.org/protobuf v1.36.5
)

require (
	github.com/cespare/xxhash/v2 v2.3.0 // indirect
	github.com/dgryski/go-rendezvous v0.0.0-20200823014737-9f7001d12a5f // indirect
	github.com/go-zeromq/goczmq/v4 v4.2.2 // indirect
	github.com/golang/glog v1.2.5 // indirect
	github.com/google/uuid v1.6.0 // indirect
	github.com/kylelemons/godebug v1.1.0 // indirect
	github.com/openconfig/gnmi v0.13.0 // indirect
	github.com/openconfig/goyang v1.6.0 // indirect
	github.com/yuin/gopher-lua v1.1.1 // indirect
	go.uber.org/atomic v1.10.0 // indirect
	golang.org/x/exp v0.0.0-20250218142911-aa4b98e5adaa // indirect
	golang.org/x/net v0.38.0 // indirect
	golang.org/x/sync v0.12.0 // indirect
	golang.org/x/sys v0.31.0 // indirect
	golang.org/x/text v0.23.0 // indirect
	google.golang.org/genproto/googleapis/rpc v0.0.0-20250218202821-56aae31c358a // indirect
	lukechampine.com/uint128 v1.3.0 // indirect
)
