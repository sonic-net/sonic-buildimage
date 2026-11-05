load("@io_bazel_rules_go//proto:compiler.bzl", "go_proto_compiler")
load("@io_bazel_rules_go//proto/wkt:well_known_types.bzl", "PROTO_RUNTIME_DEPS", "WELL_KNOWN_TYPES_APIV2")

def use_new_compilers():
    go_proto_compiler(
        name = "go_protoc_gen_go",
        options = [
            "paths=source_relative",
        ],
        plugin = "@org_golang_google_protobuf//cmd/protoc-gen-go",
        suffix = ".pb.go",
        visibility = ["//visibility:public"],
        deps = PROTO_RUNTIME_DEPS + WELL_KNOWN_TYPES_APIV2,
    )
    go_proto_compiler(
        name = "go_protoc_gen_go_grpc",
        options = [
            "paths=source_relative",
        ],
        plugin = "@org_golang_google_grpc_cmd_protoc_gen_go_grpc//:protoc-gen-go-grpc",
        suffix = "_grpc.pb.go",
        visibility = ["//visibility:public"],
        deps = PROTO_RUNTIME_DEPS + [
            "@org_golang_google_grpc//:go_default_library",
            "@org_golang_google_grpc//codes:go_default_library",
            "@org_golang_google_grpc//status:go_default_library",
        ],
    )

