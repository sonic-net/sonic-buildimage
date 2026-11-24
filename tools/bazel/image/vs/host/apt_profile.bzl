"""VS host package names derived from the same reviewed APT lock as payloads."""
load("@orchagent_apt_inputs//:apt_inputs.bzl", "APT_INPUTS")

APT_PACKAGES = sorted({
    target.removeprefix("@vs_host_debian//"): True
    for target in APT_INPUTS["vs_host_debian"]["amd64"].values()
}.keys())
