"""Exercise the two NAT archives exported by the native AMD64/Trixie VS build."""

from pathlib import Path
import subprocess
import sys
import uuid

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from tools.bazel.ci import validate_runtime
from tools.bazel.ci.vs_archive_reuse import archive_snapshot, profile


def verify(workspace, name):
    _, manifest = profile(workspace, name)
    path = workspace / "target" / (name + ".gz")
    snapshot = archive_snapshot(path, name + ":latest", manifest,
                                entrypoint=["/usr/local/bin/supervisord"])
    tag = "sonic-nat-validation:" + uuid.uuid4().hex
    probe = workspace / "dockers/docker-nat/verify-installed.sh"
    try:
        image, config, _ = validate_runtime.load_archive(path, tag, snapshot["config_digest"])
        validate_runtime.verify_loaded_image(image, config, snapshot["config_digest"])
        with probe.open("rb") as script:
            subprocess.run(["docker", "run", "--rm", "--network=none", "--read-only",
                            "--entrypoint", "/bin/sh", "-i", tag, "-s", "--", name],
                           stdin=script, check=True, timeout=120)
    finally:
        subprocess.run(["docker", "image", "rm", tag], capture_output=True, timeout=60)


if __name__ == "__main__":
    workspace = Path.cwd()
    for name in ("docker-nat", "docker-nat-dbg"):
        verify(workspace, name)
