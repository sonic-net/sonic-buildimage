"""Run the installed BMP receiver and configuration service from the VS archive."""

import subprocess
import uuid
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import validate_runtime, vs_archive_reuse


PROBE_SCRIPT = """
set -eu
test -x /usr/local/bin/bmpcfgd && test -x /usr/bin/bmp.sh
test -s /etc/bmp/openbmpd.conf
grep -Fx '[program:openbmpd]' /etc/supervisor/conf.d/supervisord.conf
grep -Fx 'program:bmpcfgd' /etc/supervisor/critical_processes
/usr/bin/openbmpd -v | grep -Fx 'openbmpd (www.openbmp.org) version : 0.14.0-0'
python3 -c 'from importlib.metadata import distribution; from bmpcfgd.bmpcfgd import main; assert callable(main); assert distribution("sonic-bmpcfgd-services").version == "1.0"'
test "$(dpkg-query -W -f='${Version}' sonic-bmp)" = 0.1
"""


def main():
    root = Path.cwd()
    image = "docker-sonic-bmp"
    archive = root / "target" / (image + ".gz")
    _, manifest = vs_archive_reuse.profile(root, image)
    snapshot = vs_archive_reuse.archive_snapshot(
        archive, image + ":latest", manifest, entrypoint=["/usr/local/bin/supervisord"]
    )
    tag = "sonic-bmp-probe:" + uuid.uuid4().hex
    try:
        loaded, config, _ = validate_runtime.load_archive(archive, tag, snapshot["config_digest"])
        validate_runtime.verify_loaded_image(loaded, config, snapshot["config_digest"])
        validate_runtime.docker(
            "run", "--rm", "--network=none", "--read-only", "--cap-drop=ALL",
            "--security-opt=no-new-privileges", "--env", "PYTHONDONTWRITEBYTECODE=1",
            "--entrypoint", "/bin/sh", tag, "-ec", PROBE_SCRIPT,
        )
    finally:
        subprocess.run(["docker", "image", "rm", tag], capture_output=True, check=False)


if __name__ == "__main__":
    main()
