"""Exercise the NAT archives selected for the native AMD64/Trixie VS build."""

import argparse
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


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--debug-layer", choices=("n", "y"), default="y",
                        help="also validate the debug archive when the build enables debug layers")
    args = parser.parse_args(argv)
    workspace = Path.cwd()
    verify(workspace, "docker-nat")
    if args.debug_layer == "y":
        verify(workspace, "docker-nat-dbg")


if __name__ == "__main__":
    main()
