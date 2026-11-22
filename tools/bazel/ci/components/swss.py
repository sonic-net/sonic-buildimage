"""Check SWSS package declarations and native link arguments against the VS runtime."""

import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import tempfile

from tools.bazel.ci import swss_native_closure
from tools.bazel.oci import package_contract
from tools.bazel.tests.integration.oci_source_layers_test import verify_layers


IMAGES = ("docker-orchagent",)
# The AMD64 package job has the selected source checkout, Bazel and binutils.
REUSABLE_WORKFLOW = True
RUNTIME = {
    "swss.tar": "@sonic_swss//dist:swss_pkg",
    "protobuf.tar": "@sonic_dash_api//:protobuf_runtime_pkg",
    "libteam.tar": "@libteam//:libteam_runtime",
    "libteamdctl.tar": "@libteam//:libteamdctl_runtime",
    "rdeps.tar": "//dockers/docker-orchagent:rdeps",
    "config.tar": "//dockers/docker-orchagent/config:files",
    "policy.json": "//dockers/docker-orchagent:package_policy",
    "source-excludes.txt": "//tools/bazel/dpkg:dpkg_excludes.txt",
    "source-includes.txt": "//tools/bazel/dpkg:oci_includes",
}
DEBUG = {
    "swss-symbols.tar": "@sonic_swss//dist:swss_pkg.debug_symbols",
    "dependency-symbols.tar": "//dockers/docker-orchagent:rdeps_debug_symbols",
}


def source_root(location):
    """Locate the checked-out owner from Bazel's single source-file location record."""
    match = re.fullmatch(
        r"(/[^\n]+/fpmsyncd/bgp_eoiu_marker\.py):[0-9]+:[0-9]+: source file "
        r"@[^\s]+//fpmsyncd:bgp_eoiu_marker\.py\n?", location)
    if match is None:
        raise ValueError("Expected one absolute Bazel location for the SWSS source file")
    return Path(match[1]).resolve(strict=True).parents[1]


def verify(directory, *, debug):
    manifest = json.loads((directory / "manifest.json").read_bytes())
    bazel = os.environ["BAZEL"]
    platform = "@sonic_build_infra//platforms:x86_64_trixie"
    options = [f"--{key}={value}" for key, value in sorted(manifest["build_settings"].items())]
    options += ["--platforms=" + platform, "--host_platform=" + platform]
    location = subprocess.check_output(
        [bazel, "query", "--output=location", "@sonic_swss//fpmsyncd:bgp_eoiu_marker.py"], text=True)
    source = source_root(location)
    with tempfile.NamedTemporaryFile() as graph:
        subprocess.run([bazel, "aquery", *options, "--include_param_files", "--output=jsonproto",
                        'mnemonic("CppLink", deps(//dockers/docker-orchagent:rdeps))'], check=True, stdout=graph)
        closure = swss_native_closure.verify(source, directory / "swss.tar", directory / "swss-symbols.tar" if debug else None,
                                             directory / "policy.json", Path(graph.name))
    libraries = {f: directory / f for f in ("protobuf.tar", "libteam.tar", "libteamdctl.tar")}
    contract = verify_layers(
        directory / "swss.tar", directory / "rdeps.tar", directory / "config.tar",
        directory / "dependency-symbols.tar" if debug else None, libraries, source,
        package_contract.read_policy(directory / "policy.json"), source_layer_excludes=directory / "source-excludes.txt",
        source_layer_includes=directory / "source-includes.txt", readelf=shutil.which("readelf"), objcopy=shutil.which("objcopy"))
    out = Path("artifacts/owners")
    out.mkdir(parents=True, exist_ok=True)
    (out / "native-closure.json").write_text(json.dumps(closure, indent=2) + "\n")
    (out / "swss-package.json").write_text(json.dumps(contract, indent=2) + "\n")
