"""Verify that the OTEL download branch does not guess an unsupported ABI."""

import json
from pathlib import Path
import re
import subprocess

from jinja2 import Environment, StrictUndefined
import pytest


ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("architecture, package_arch", [
    ("amd64", "amd64"), ("arm64", "arm64"), ("armhf", "armv7"), ("unsupported", None),
])
def test_otel_architecture_selection_fails_closed(tmp_path, architecture, package_arch):
    source = (ROOT / "dockers/docker-sonic-otel/Dockerfile.j2").read_text()
    section = source.split("# Download and install official .deb package with architecture-specific logic", 1)[1]
    section = section.split('COPY ["start.sh"', 1)[0]
    rendered = Environment(undefined=StrictUndefined).from_string(section).render(
        CONFIGURED_ARCH=architecture,
    )
    command = rendered.split("RUN ", 1)[1]
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    for name in ("wget", "dpkg", "apt-get", "ln", "rm"):
        tool = bin_dir / name
        tool.write_text(
            "#!/usr/bin/python3\nimport json, os, sys\n"
            "with open(os.environ['DOWNLOAD_TEST_LOG'], 'a') as log:\n"
            "    log.write(json.dumps(sys.argv) + '\\n')\n"
            "sys.exit(37)\n",
        )
        tool.chmod(0o755)
    log = tmp_path / "calls.jsonl"
    version = re.search(r"^ARG OTEL_VERSION=(.+)$", source, re.MULTILINE)[1]
    result = subprocess.run(
        ["bash", "-c", command], cwd=tmp_path, text=True,
        env={"PATH": f"{bin_dir}:/usr/bin:/bin", "OTEL_VERSION": version,
             "DOWNLOAD_TEST_LOG": str(log)},
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10,
    )
    if package_arch is None:
        assert result.returncode != 0
        assert not log.exists()
    else:
        assert result.returncode == 37
        calls = [json.loads(line) for line in log.read_text().splitlines()]
        assert len(calls) == 1
        assert Path(calls[0][0]).name == "wget"
        assert calls[0][-1].endswith(f"_linux_{package_arch}.deb")
