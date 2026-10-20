"""Run native consumers with Common's declared transitive runtime payload."""

load("@rules_shell//shell:sh_test.bzl", "sh_test")

def common_runtime_test(name, binary, test_args = [], **kwargs):
    """Wrap a native test without relying on libraries installed on the host."""
    runtime = "@sonic_swss_common//tools/bazel/runtime:test_runtime"
    sh_test(
        name = name,
        srcs = ["@sonic_swss_common//tools/bazel/runtime:run_with_runtime.py"],
        args = ["$(rootpath " + runtime + ")", "-"] + select({
            "@sonic_swss_common//tools/bazel:yang_modules_enabled": ["enabled"],
            "//conditions:default": ["disabled"],
        }) + ["$(rootpath " + binary + ")"] + test_args,
        data = [runtime, binary],
        **kwargs
    )
