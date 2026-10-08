package main

import (
	"fmt"
	"sync"

	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
	rv1 "google.golang.org/grpc/reflection/grpc_reflection_v1"
	rv1alpha "google.golang.org/grpc/reflection/grpc_reflection_v1alpha"
	"google.golang.org/protobuf/reflect/protodesc"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/reflect/protoregistry"
)

// ygot's generated Go registers ywrapper.proto and yext.proto under their
// bare names, but gribi_aft.proto imports them by full path. Stock reflection
// then cannot serve the gRIBI service's dependencies, and clients such as
// grpcurl fail to resolve it.
var protoAliases = map[string]string{
	"github.com/openconfig/ygot/proto/ywrapper/ywrapper.proto": "ywrapper.proto",
	"github.com/openconfig/ygot/proto/yext/yext.proto":         "yext.proto",
}

// registerReflection serves gRPC reflection (v1 and v1alpha) from the global
// registry, with protoAliases answered under the paths gRIBI imports.
func registerReflection(s *grpc.Server) {
	opts := reflection.ServerOptions{Services: s, DescriptorResolver: &aliasResolver{Resolver: protoregistry.GlobalFiles}}
	rv1.RegisterServerReflectionServer(s, reflection.NewServerV1(opts))
	rv1alpha.RegisterServerReflectionServer(s, reflection.NewServer(opts))
}

type aliasResolver struct {
	protodesc.Resolver

	mu      sync.Mutex
	renamed map[string]protoreflect.FileDescriptor
}

func (r *aliasResolver) FindFileByPath(path string) (protoreflect.FileDescriptor, error) {
	registered, ok := protoAliases[path]
	if !ok {
		return r.Resolver.FindFileByPath(path)
	}
	r.mu.Lock()
	defer r.mu.Unlock()
	if fd, ok := r.renamed[path]; ok {
		return fd, nil
	}
	orig, err := r.Resolver.FindFileByPath(registered)
	if err != nil {
		return nil, err
	}
	fdp := protodesc.ToFileDescriptorProto(orig)
	fdp.Name = &path
	fd, err := protodesc.NewFile(fdp, r.Resolver)
	if err != nil {
		return nil, fmt.Errorf("rename %s to %s: %w", registered, path, err)
	}
	if r.renamed == nil {
		r.renamed = map[string]protoreflect.FileDescriptor{}
	}
	r.renamed[path] = fd
	return fd, nil
}
