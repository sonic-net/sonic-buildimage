"""Verify the two DHCP relay package archives emitted by the native VS build."""
import json
from pathlib import Path
import subprocess
import sys
import uuid

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import validate_runtime, vs_archive_reuse

PROBE = """
set -eu
for bin in dhcrelay dhcp4relay dhcp6relay dhcpmon; do
    path=/usr/sbin/$bin
    test -x "$path"
    linked=$(ldd "$path")
    if echo "$linked" | grep -F 'not found'; then exit 1; fi
done
for package in isc-dhcp-relay sonic-dhcp4relay sonic-dhcp6relay sonic-dhcpmon; do
    test "$(dpkg-query -W -f='${db:Status-Status}' "$package")" = installed
done
test -r /etc/default/isc-dhcp-relay
. /etc/default/isc-dhcp-relay
test -z "$SERVERS$INTERFACES$OPTIONS"
for link in /etc/rc[016].d/K01isc-dhcp-relay /etc/rc[2345].d/S01isc-dhcp-relay; do
    test "$(readlink "$link")" = ../init.d/isc-dhcp-relay
done
test -x /usr/bin/start.sh
test -r /etc/supervisor/critical_processes
test -r /usr/share/sonic/templates/docker-dhcp-relay.supervisord.conf.j2
test -r /cli/show/plugins/show_dhcp_relay.py
test -r /cli/config/plugins/dhcp_relay.py
test -r /cli/clear/plugins/clear_dhcp_relay.py
test -n "$IMAGE_VERSION"
python3 -c 'import freezegun,psutil; from dhcp_utilities.dhcprelayd.dhcprelayd import main'
test -x /usr/local/bin/dhcprelayd
"""


def main():
    target = ROOT / "target"
    runtime = None
    for name in ("docker-dhcp-relay", "docker-dhcp-relay-dbg"):
        archive = target / f"{name}.gz"
        manifest = json.loads((target / "bazel-manifests" / name / "manifest.json").read_text())
        snapshot = vs_archive_reuse.archive_snapshot(
            archive, name + ":latest", manifest, entrypoint=["/usr/bin/docker_init.sh"])
        if runtime is None:
            runtime = snapshot["rootfs"]
        elif snapshot["rootfs"][:len(runtime)] != runtime or len(snapshot["rootfs"]) == len(runtime):
            raise ValueError("The debug archive must extend the runtime layers")
        tag = "sonic-dhcp-relay-validation:" + uuid.uuid4().hex
        try:
            image, config, _ = validate_runtime.load_archive(archive, tag, snapshot["config_digest"])
            validate_runtime.verify_loaded_image(image, config, snapshot["config_digest"])
            validate_runtime.docker("run", "--rm", "--network", "none", "--read-only",
                                    "--entrypoint", "/bin/sh", tag, "-ec", PROBE)
        finally:
            subprocess.run(["docker", "image", "rm", tag], check=False)
        print(f"Verified DHCP relay package, loader and daemon imports in {name}")


if __name__ == "__main__":
    main()
