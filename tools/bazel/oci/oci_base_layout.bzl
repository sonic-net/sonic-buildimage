"""Expose Make's declared OCI files as a platform-checked rules_oci base."""

load("@aspect_bazel_lib//lib:copy_to_directory.bzl", "copy_to_directory")
load("@aspect_bazel_lib//lib:run_binary.bzl", "run_binary")

def oci_base_layout(name, srcs, root, marker, expected_platform, **kwargs):
    """Copy a complete OCI layout into a TreeArtifact without format conversion.

    srcs declares every source file (or a generated directory). root is its
    workspace-relative root; marker is an execpath expression for oci-layout.
    """
    checked = name + "_checked"
    run_binary(
        name = checked,
        srcs = srcs,
        args = [
            "--marker",
            marker,
            "--expected-platform",
            expected_platform,
            "--out",
            "$@",
        ],
        outs = [checked + "/oci-layout"],
        tool = Label("//tools/bazel/oci:validate_oci_layout"),
        mnemonic = "ValidateOciBase",
        progress_message = "Checking OCI base %{label}",
    )
    copy_to_directory(
        name = name,
        srcs = srcs + [":" + checked],
        root_paths = [root, "/".join([part for part in [native.package_name(), checked] if part])],
        out = name + "_layout",
        # Only the unchanged, validated marker replaces its source counterpart.
        # Requiring that output prevents consumers from bypassing validation.
        allow_overwrites = True,
        **kwargs
    )
