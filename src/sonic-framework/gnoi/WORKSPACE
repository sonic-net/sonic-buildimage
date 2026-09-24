workspace(name = "com_github_openconfig_gnoi")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

### Bazel rules for many languages to compile PROTO into gRPC libraries
http_archive(
    name = "rules_proto_grpc",
    sha256 = "c0d718f4d892c524025504e67a5bfe83360b3a982e654bc71fed7514eb8ac8ad",
    strip_prefix = "rules_proto_grpc-4.6.0",
    urls = ["https://github.com/rules-proto-grpc/rules_proto_grpc/archive/4.6.0.tar.gz"],
)

http_archive(
    name = "com_google_googleapis",
    sha256 = "9fc03150d86501d7da35eefa989d5553bdd77a95cfe4373cdafe8eee92f6bfb1",
    strip_prefix = "googleapis-870a5ed7e141b4faf70e2a0858854e9b5bb18612",
    urls = ["https://github.com/googleapis/googleapis/archive/870a5ed7e141b4faf70e2a0858854e9b5bb18612.tar.gz"],
)

load("@com_google_googleapis//:repository_rules.bzl", "switched_rules_by_language")

switched_rules_by_language(
    name = "com_google_googleapis_imports",
    cc = True,
    go = True,
)

load(
    "@rules_proto_grpc//:repositories.bzl",
    "bazel_gazelle",
    "io_bazel_rules_go",
    "rules_proto_grpc_repos",
    "rules_proto_grpc_toolchains",
)

rules_proto_grpc_toolchains()

rules_proto_grpc_repos()

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

### Golang
io_bazel_rules_go()

load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains", "go_rules_dependencies")

go_rules_dependencies()

go_register_toolchains(go_version = "1.20")

# gazelle:repo bazel_gazelle
bazel_gazelle()

load("@bazel_gazelle//:deps.bzl", "gazelle_dependencies")
load("//:gnoi_deps.bzl", "gnoi_deps")

# gazelle:repository_macro gnoi_deps.bzl%gnoi_deps
gnoi_deps()

load("@rules_proto_grpc//go:repositories.bzl", rules_proto_grpc_go_repos = "go_repos")

rules_proto_grpc_go_repos()

# Load gazelle_dependencies last, so that the newer version of org_golang_google_grpc is used.
# see https://github.com/rules-proto-grpc/rules_proto_grpc/issues/160
gazelle_dependencies()

### C++
load("@rules_proto_grpc//cpp:repositories.bzl", rules_proto_grpc_cpp_repos = "cpp_repos")

rules_proto_grpc_cpp_repos()

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

# open-config YANG files
http_archive(
    name = "github_openconfig_yang",
    build_file_content = """exports_files(glob(["release/models/**/*.yang"]), visibility = ["//visibility:public"])""",
    sha256 = "f6b2b6c0ffe0b66881287bcd43241a57583f353cc5cc41cba973601c32232f45",
    strip_prefix = "public-bf737a5567ec248456cb528efcd63cab15e8fc69",
    urls = [
        "https://github.com/openconfig/public/archive/bf737a5567ec248456cb528efcd63cab15e8fc69.zip",
    ],
)

# YANG files from other standard bodies.
http_archive(
    name = "github_yang",
    build_file_content = """exports_files(glob(["standard/**/*.yang"]), visibility = ["//visibility:public"])""",
    sha256 = "55913058f64a1ec7fe9e6e70d7128f08e66b20c859803b1fb02dbaf7eef2c64d",
    strip_prefix = "yang-2fa291d6bdb4b281d4e1b3dfa3254ffa7257d800",
    urls = [
        "https://github.com/YangModels/yang/archive/2fa291d6bdb4b281d4e1b3dfa3254ffa7257d800.zip",
    ],
)
