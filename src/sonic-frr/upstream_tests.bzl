"""Declare the original FRR checks with explicit native executable inputs."""

load("@rules_python//python:defs.bzl", "py_test")

def frr_upstream_check(name, binaries, sources):
    """Preserve pytest and each selected standalone check in one receipt."""
    py_test(
        name = name,
        srcs = ["@sonic-buildimage//src/sonic-frr:upstream_tests.py"],
        main = "@sonic-buildimage//src/sonic-frr:upstream_tests.py",
        deps = ["@rules_python//python/runfiles"],
        data = sources + [binary for binary, _ in binaries] + [
            "@sonic_build_infra//tools/build_tools:python3",
            "@sonic-buildimage//src/sonic-frr:upstream_cli_reference",
            "@sonic-buildimage//src/sonic-frr:upstream_config_status",
        ],
        args = [
            "--python=$(rlocationpath @sonic_build_infra//tools/build_tools:python3)",
            "--cli-reference=$(rlocationpath @sonic-buildimage//src/sonic-frr:upstream_cli_reference)",
            "--config-status=$(rlocationpath @sonic-buildimage//src/sonic-frr:upstream_config_status)",
        ] + [
            "--source=$(rlocationpath " + source + ")"
            for source in sources
        ] + [
            "--binary=" + logical + "=$(rlocationpath " + binary + ")"
            for binary, logical in binaries
        ] + [
            "--standalone=" + program
            for _, program in binaries
            if program.startswith("pceplib/test/")
        ],
        target_compatible_with = select({
            "@platforms//cpu:x86_64": [],
            "@platforms//cpu:aarch64": [],
            "//conditions:default": ["@platforms//:incompatible"],
        }),
        tags = ["manual"],
        timeout = "long",
    )
