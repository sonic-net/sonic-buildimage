package main

import (
	"testing"

	"google.golang.org/protobuf/reflect/protoregistry"

	spb "github.com/openconfig/gribi/v1/proto/service"
)

// Reflection clients fetch each import by the path it is imported under, so
// every file the gRIBI service depends on must resolve by that path.
func TestReflectionResolvesGRIBIImports(t *testing.T) {
	r := &aliasResolver{Resolver: protoregistry.GlobalFiles}
	root := spb.File_v1_proto_service_gribi_proto.Path()
	seen := map[string]bool{}
	queue := []string{root}
	for len(queue) > 0 {
		path := queue[0]
		queue = queue[1:]
		if seen[path] {
			continue
		}
		seen[path] = true
		fd, err := r.FindFileByPath(path)
		if err != nil {
			t.Errorf("FindFileByPath(%q): %v", path, err)
			continue
		}
		if fd.Path() != path {
			t.Errorf("FindFileByPath(%q) returned a file named %q", path, fd.Path())
		}
		for i := 0; i < fd.Imports().Len(); i++ {
			queue = append(queue, fd.Imports().Get(i).Path())
		}
	}
	for alias := range protoAliases {
		if !seen[alias] {
			t.Errorf("alias %q is no longer imported by the gRIBI service; drop it", alias)
		}
	}
}
