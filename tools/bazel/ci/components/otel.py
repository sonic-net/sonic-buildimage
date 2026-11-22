"""Run the collector release's source, package and golden telemetry suites on VS outputs."""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from tools.bazel.ci.installed_package_test import test_script


IMAGES = ("docker-sonic-otel",)
OWNER = "@otelcol_contrib//"
DIST = OWNER + "distributions/otelcol-contrib:"
INSTALLED = {
    "etc/otelcol-contrib/config.yaml": "config.yaml",
    "etc/otelcol-contrib/otelcol-contrib.conf": "otelcol-contrib.conf",
    "lib/systemd/system/otelcol-contrib.service": "otelcol-contrib.service",
    "usr/share/doc/otelcol-contrib/copyright": "LICENSE",
}
GENERATED = ("components.go", "main.go", "main_others.go")
SOURCES = (*GENERATED, "manifest.yaml", "go.mod", "go.sum")
RUNTIME = {
    "runtime.tar": OWNER + ":runtime",
    "sources.tar": OWNER + ":sources",
    "collector": OWNER + ":otelcol-contrib",
    "metadata.json": OWNER + ":runtime_packages.json",
    "golden": OWNER + "bazel/golden:golden",
    "buildinfo": OWNER + "bazel/buildinfo:buildinfo",
    "default-config.yaml": OWNER + ":tests/docker-tests/default-config.yaml",
    "golden-config.yaml": OWNER + ":tests/golden/config.yaml",
    "golden-compose.yaml": OWNER + ":tests/golden/docker-compose.yaml",
    "golden-expected.yaml": OWNER + ":tests/golden/data/expected.yaml",
    "LICENSE": OWNER + ":LICENSE",
    **{name: DIST + name for name in ("config.yaml", "otelcol-contrib.conf", "otelcol-contrib.service",
                                      "manifest.yaml", "go.mod", "go.sum", "components.bzl")},
    **{name: DIST + "generated/" + name for name in GENERATED},
    **{name: OWNER + "bazel/testdata:" + name for name in
       ("release-buildinfo.json", "release-version.txt", "release-components.txt")},
    **{name: OWNER + "bazel:" + name for name in
       ("package_test.py", "source_contract_test.py", "components_output_test.py")},
}
DEBUG = {"symbols.tar": OWNER + ":runtime.debug_symbols"}


def verify(directory, *, debug):
    # All executables and independently generated expectations come from the
    # producer's configured targets, including the upstream OTLP golden receiver.
    report = Path("artifacts/owners/otel").resolve()
    report.mkdir(parents=True, exist_ok=True)
    # Runner instrumentation must not change the collector's test resources or OTLP endpoints.
    env = {key: value for key, value in os.environ.items() if not key.startswith("OTEL_")}
    env["TEST_UNDECLARED_OUTPUTS_DIR"] = str(report)
    with tempfile.TemporaryDirectory(prefix="otel-owner-") as temporary:
        scratch = Path(temporary)
        # The release output test imports bazel.package_test from its upstream package.
        scripts = scratch / "bazel"
        scripts.mkdir()
        (scripts / "__init__.py").touch()
        script = test_script("otel", directory, scripts, debug=debug)
        shutil.copyfile(directory / "components_output_test.py", scripts / "components_output_test.py")
        for name in ("golden", "buildinfo"):
            executable = scratch / name
            shutil.copyfile(directory / name, executable)
            executable.chmod(0o755)  # GitHub does not restore executable bits from artifact ZIPs.
        subprocess.run([sys.executable, "-B", directory / "source_contract_test.py",
                        "--manifest", directory / "manifest.yaml", "--go-mod", directory / "go.mod",
                        "--dependencies", directory / "components.bzl",
                        *(arg for name in GENERATED for arg in ("--source", directory / name))], check=True)
        subprocess.run([sys.executable, "-B", "-m", "bazel.components_output_test",
                        directory / "release-components.txt"], cwd=scratch, env={**env, "PYTHONPATH": str(scratch)}, check=True)
        inputs = {
            "runtime": "runtime.tar", "sources": "sources.tar", "binary": "collector", "metadata": "metadata.json",
            "baseline-version": "release-version.txt", "baseline-components": "release-components.txt",
            "baseline-buildinfo": "release-buildinfo.json", "default-config": "default-config.yaml",
            "golden-config": "golden-config.yaml", "golden-compose": "golden-compose.yaml",
            "golden-expected": "golden-expected.yaml",
        }
        if debug:
            inputs["debug"] = "symbols.tar"
        command = [sys.executable, "-B", script, "--golden", scratch / "golden", "--buildinfo", scratch / "buildinfo"]
        for flag, filename in inputs.items():
            command.extend(("--" + flag, directory / filename))
        for tool in ("readelf", "objcopy", "gdb"):
            command.extend(("--" + tool, shutil.which(tool) or tool))
        for path, name in INSTALLED.items():
            command.extend(("--runtime-file", f"{path}={directory / name}"))
        for name in SOURCES:
            command.extend(("--source-file", f"{name}={directory / name}"))
        subprocess.run(command, cwd=scratch, env=env, check=True)
