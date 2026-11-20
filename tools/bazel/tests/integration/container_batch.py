"""Check batched configured archives against individual builds with real Bazel."""

import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.cache_reuse import json_records


IMAGE_VERSION = "--//tools/bazel/oci:image_version="
CASES = [
    ("base.gz", "base", [], "VS normal.sai normal.sai"),
    ("versioned.gz", "versioned", [], "VS normal.sai normal.sai"),
    ("syncd.gz", "syncd", ["--config=syncd-dash"], "DASH dash.sai dash.meta"),
    ("syncd-dbg.gz", "syncd_dbg", ["--config=syncd-dash"], "DASH dash.sai dash.meta"),
]


def module(workspace, name, files):
    directory = workspace / name
    directory.mkdir()
    (directory / "MODULE.bazel").write_text(f'module(name="{name}", version="1.0")\n')
    for path, contents in files.items():
        dest = directory / path
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(contents)
    return f'bazel_dep(name="{name}", version="1.0")\nlocal_path_override(module_name="{name}", path="{name}")\n'


def fixture(workspace):
    # The real Syncd profile selects a boolean and two label flags in external
    # modules. Tiny inputs let CI verify the configuration without compiling DASH.
    flags = """
def _flag(ctx):
    return []

boolean = rule(implementation = _flag, build_setting = config.bool(flag = True))
"""
    public = 'package(default_visibility = ["//visibility:public"])\n'
    dependencies = module(workspace, "sonic_build_infra", {
        "platforms/BUILD": public + 'platform(name="x86_64_trixie")\nplatform(name="aarch64_trixie")\n',
    })
    dependencies += module(workspace, "sonic_dash", {
        "BUILD": public + 'filegroup(name="sdk", srcs=["dash.sai"])\n'
                          'filegroup(name="runtime_packages.json", srcs=["dash.meta"])\n',
        "dash.sai": "sdk", "dash.meta": "metadata",
    })
    dependencies += module(workspace, "sonic_sairedis", {
        "flags.bzl": flags,
        "BUILD": 'load(":flags.bzl", "boolean")\n' + public
                 + 'boolean(name="enable_dashsai", build_setting_default=False)\n'
                   'filegroup(name="standard", srcs=["normal.sai"])\n'
                   'label_flag(name="dash_sai", build_setting_default=":standard")\n'
                   'label_flag(name="dash_runtime_metadata", build_setting_default=":standard")\n',
        "normal.sai": "vs",
    })
    dependencies += 'bazel_dep(name="bazel_skylib", version="1.9.2")\n'
    (workspace / "MODULE.bazel").write_text('module(name="batch_test")\n' + dependencies)
    (workspace / "probe.bzl").write_text("""
def _shared_child(ctx):
    out = ctx.actions.declare_file("ordinary-package.txt")
    ctx.actions.run(
        executable = "/bin/sh",
        arguments = ["-c", 'printf %s "$1" > "$1"', "package", out.path],
        outputs = [out],
        mnemonic = "OrdinaryPackage",
    )
    return [DefaultInfo(files = depset([out]))]

shared_child = rule(implementation = _shared_child)

def _emit(ctx):
    out = ctx.actions.declare_file("probe.txt")
    content = " ".join([ctx.attr.mode, ctx.file.sdk.basename, ctx.file.meta.basename])
    ctx.actions.run(
        executable = "/bin/sh",
        arguments = ["-c", 'echo "$2" > "$3" && cat "$1" >> "$3" && echo >> "$3" && cat "$4" >> "$3"',
                     "archive", ctx.file.package.path, content, out.path, ctx.file.labels.path],
        inputs = [ctx.file.package, ctx.file.labels],
        outputs = [out],
        mnemonic = "VersionedArchive",
    )
    return [DefaultInfo(files = depset([out]))]

probe = rule(implementation = _emit, attrs = {
    "package": attr.label(default = "//:ordinary", allow_single_file = True),
    "labels": attr.label(allow_single_file = True, mandatory = True),
    "mode": attr.string(default = "VS"),
    "sdk": attr.label(default = "@sonic_sairedis//:standard", allow_single_file = True),
    "meta": attr.label(default = "@sonic_sairedis//:standard", allow_single_file = True),
})
""")
    (workspace / "BUILD.bazel").write_text('''
load(":probe.bzl", "shared_child")
package(default_visibility = ["//visibility:public"])
shared_child(name = "ordinary")
config_setting(name = "dash", flag_values = {"@sonic_sairedis//:enable_dashsai": "true"})
''')
    # Only actual consumers inspect each flag. All image families embed the
    # ordinary parent's exec path, so a changed Starlark output tree changes bytes.
    for _, package, flags, _ in CASES:
        attrs = ['labels=":labels"']
        if "--config=syncd-dash" in flags:
            attrs.extend(['mode=select({"//:dash": "DASH", "//conditions:default": "VS"})',
                          'sdk="@sonic_sairedis//:dash_sai"', 'meta="@sonic_sairedis//:dash_runtime_metadata"'])
        owner = workspace / package
        owner.mkdir()
        old_flag = ('alias(name="image_version", actual="//tools/bazel/oci:image_version")\n'
                    if package == "versioned" else "")
        (owner / "manifest.json").write_text("{}")
        (owner / "BUILD.bazel").write_text(
            'load("//:probe.bzl", "probe")\n'
            'load("//tools/bazel/oci:manifest_labels.bzl", "manifest_labels")\n' + public
            + 'manifest_labels(name="labels", manifest="manifest.json")\n'
            + old_flag + 'probe(name="archive", ' + ", ".join(attrs) + ')\n')
    tools = workspace / "tools/bazel"
    oci = tools / "oci"
    oci.mkdir(parents=True)
    for name in ("manifest_labels.bzl", "manifest_labels.py"):
        shutil.copy(ROOT / "tools/bazel/oci" / name, oci)
    (oci / "BUILD.bazel").write_text(
        'load("@bazel_skylib//rules:common_settings.bzl", "string_flag")\n' + public
        + 'string_flag(name="image_version", build_setting_default="")\n'
          'genrule(name="manifest_labels", srcs=["manifest_labels.py"], outs=["label-writer"], '
          'cmd="cp $< $@", executable=True)\n')
    metadata = tools / "image/vs/metadata"
    metadata.mkdir(parents=True)
    (metadata / "BUILD.bazel").write_text(
        'load("@bazel_skylib//rules:common_settings.bzl", "bool_flag", "string_flag")\n' + public
        + 'bool_flag(name="use_make_metadata", build_setting_default=False)\n'
          'string_flag(name="version", build_setting_default="local")\n')
    shutil.copy(ROOT / "tools/bazel/common.bazelrc", tools)
    shutil.copy(ROOT / ".bazelversion", workspace)
    profile = [line for line in (tools / "common.bazelrc").read_text().splitlines()
               if line.startswith("build:syncd-dash ")]
    (workspace / ".bazelrc").write_text("\n".join(profile) + "\n")


def check_shared_cache(workspace, bazel, command, output):
    # A clean between builds discards local action state and outputs. Only the
    # execution cache can supply the version-independent package to v17.
    for version in ("v16", "v17"):
        if version == "v17":
            subprocess.run([bazel, "clean"], cwd=workspace, check=True)
        setting = IMAGE_VERSION + version
        log = workspace / (version + ".json")
        options = ["--remote_cache=", "--remote_upload_local_results", "--disk_cache=" + str(workspace / "cache"),
                   "--execution_log_json_file=" + str(log)]
        env = dict(os.environ, BAZEL_CONTAINER_ARGS=shlex.join(options))
        subprocess.run([*command, "--archive=shared.gz", "--label=//versioned:archive",
                        "--owner-args=", "--shared-args=" + setting], cwd=workspace, env=env, check=True)
        if (output / "shared.gz").read_text().splitlines()[-1] != "Tag=" + version:
            raise AssertionError(f"Shared version {version} did not reach the archive")
        records = list(json_records(log))
        for mnemonic in ("OrdinaryPackage", "ContainerManifestLabel", "VersionedArchive"):
            runners = [record["runner"] for record in records if record["mnemonic"] == mnemonic]
            want_hit = version == "v17" and mnemonic == "OrdinaryPackage"
            if len(runners) != 1 or (runners == ["disk cache hit"]) != want_hit:
                raise AssertionError(f"{version} {mnemonic}: expected disk cache hit={want_hit}, got {runners}")


def check(workspace, architecture):
    bazel = os.environ.get("BAZEL", "bazel")
    platform = "@sonic_build_infra//platforms:" + {"amd64": "x86_64_trixie", "arm64": "aarch64_trixie"}[architecture]
    output = workspace / "target/exports"
    command = [sys.executable, str(ROOT / "tools/bazel/build_container_batch.py"),
               "--architecture", architecture, "--target-path", str(output)]
    env = dict(os.environ, BAZEL_CONTAINER_ARGS="--remote_download_minimal")
    try:
        check_shared_cache(workspace, bazel, command, output)
        command.append("--shared-args=" + IMAGE_VERSION + "v16")
        for name, package, flags, _ in CASES:
            arguments = shlex.join(flags)
            command.extend(["--archive=" + name, f"--label=//{package}:archive",
                            "--owner-args=" + arguments, "--shared-args=" + arguments])
        command.extend(["--archive=alias.gz", "--label=//base:archive", "--owner-args=",
                        "--archive=external.gz", "--label=@sonic_dash//:sdk", "--owner-args="])
        subprocess.run(command, cwd=workspace, env=env, check=True)
        mtimes = {path.name: path.stat().st_mtime_ns for path in output.glob("*.gz")}
        subprocess.run(command, cwd=workspace, env=env, check=True)
        if mtimes != {path.name: path.stat().st_mtime_ns for path in output.glob("*.gz")}:
            raise AssertionError("Rebuilding unchanged archives modified their timestamps")
        parent = (output / "base.gz").read_bytes().splitlines()[1]
        if not parent.endswith(b"/bin/ordinary-package.txt"):
            raise AssertionError(f"Missing embedded parent output path: {parent!r}")
        if (output / "external.gz").read_text() != "sdk" or (output / "alias.gz").read_bytes() != (output / "base.gz").read_bytes():
            raise AssertionError("External or duplicate source labels exported incorrect bytes")
        for name, package, flags, expected in CASES:
            # Existing owner CLI labels resolve to the same shared setting.
            setting = "--//versioned:image_version=" if package == "versioned" else IMAGE_VERSION
            direct = [bazel, "build", f"//{package}:archive", "--platforms=" + platform,
                      "--host_platform=" + platform, setting + "v16", *flags]
            subprocess.run(direct, cwd=workspace, check=True)
            exported = (output / name).read_bytes()
            if exported != (workspace / "bazel-bin" / package / "probe.txt").read_bytes():
                raise AssertionError(f"{name} differs from its standalone build")
            if exported.splitlines() != [expected.encode(), parent, b"com.azure.sonic.manifest={}", b"Tag=v16"]:
                raise AssertionError(f"{name} differs from expected configuration or shared parent: {exported!r}")
    finally:
        subprocess.run([bazel, "shutdown"], cwd=workspace, check=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--architecture", choices=("amd64", "arm64"), required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="sonic-container-batch-test-") as directory:
        workspace = Path(directory)
        fixture(workspace)
        check(workspace, args.architecture)
