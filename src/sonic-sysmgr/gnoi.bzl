load("@rules_cc//cc:defs.bzl", "cc_library")
load("@sonic_build_infra//proto:extract_proto_headers.bzl", "extract_proto_headers")

def gnoi_proto_lib(name, subdir):
    cc_proto_target = "@com_github_openconfig_gnoi//{subdir}:{subdir}_cc_proto".format(subdir = subdir)
    extract_proto_headers(
        name = "{subdir}_cc_proto_headers".format(subdir = subdir),
        cc_proto_target = cc_proto_target,
        outdir = subdir,
    )

    cc_library(
        name = name,
        hdrs = [":{subdir}_cc_proto_headers".format(subdir = subdir)],
        deps = [cc_proto_target],
    )
