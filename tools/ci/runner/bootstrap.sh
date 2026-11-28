#!/usr/bin/env bash
# Run from a reviewed checkout, outside an Actions job.
set -euo pipefail
version=2.337.0
sha256=70920811a4f8ad4328818682bca5c6469c1c942fab52448868071d0063816613
runner_home=/data/sonic-runner
tool_root=/opt/sonic-runner-tools

if [[ ${1:-} == --help || ${1:-} == --dry-run ]]; then
    cat <<'EOF'
Usage: sudo bash tools/ci/runner/bootstrap.sh
Requires Linux x86_64, systemd, mounted /data, Docker + buildx, git, make,
curl, wget, Python 3 with venv, and working /dev/kvm with a kvm group.
Creates the non-sudo sonic-runner account, downloads and verifies runner
2.337.0, installs jinjanator 25.3.1 in /opt/sonic-runner-tools, and enables
sonic-vs-runner.service. It does not register a runner or start a build.
Creates a self-bind mount at /data/sonic-runner and a startup service that
enables dev,suid,exec there for chroot/image construction; /data is unchanged.
If the host has an enabled gs AppArmor profile, preserves its local rules,
adds owner access only to /sonic/**/*.ps and *.pdf, and reloads that profile.
Loads iptable_nat and persists it for nested Docker's legacy IPv4 NAT support.
--dry-run and --help only print this plan; they change nothing.
EOF
    exit 0
fi
[[ $# == 0 && $EUID == 0 && $(uname -m) == x86_64 ]] || { echo 'Run as root on Linux x86_64.' >&2; exit 1; }
mountpoint -q /data || { echo '/data must be mounted before provisioning.' >&2; exit 1; }
for command in docker git make curl wget python3 systemctl; do command -v "$command" > /dev/null; done
docker info > /dev/null
docker buildx version > /dev/null
getent group docker > /dev/null
getent group kvm > /dev/null
[[ -c /dev/kvm ]] || { echo 'Enable host KVM before provisioning.' >&2; exit 1; }
if id sonic-runner > /dev/null 2>&1; then
    [[ $(getent passwd sonic-runner | cut -d: -f6) == "$runner_home" ]] || { echo 'Existing sonic-runner has a different home.' >&2; exit 1; }
    if pgrep -u sonic-runner -f 'Runner\.(Listener|Worker)' > /dev/null; then
        echo 'An existing runner is active; wait for it to finish.' >&2; exit 1
    fi
else
    useradd --create-home --user-group --home-dir "$runner_home" --shell /bin/bash sonic-runner
    passwd --lock sonic-runner
fi
usermod --append --groups docker,kvm sonic-runner
if id -nG sonic-runner | tr ' ' '\n' | grep -Eq '^(sudo|wheel|admin)$'; then
    echo 'Remove sonic-runner from administrator groups before provisioning.' >&2; exit 1
fi
modprobe iptable_nat
install -d -m 0755 /etc/modules-load.d
cat > /etc/modules-load.d/sonic-vs-runner.conf <<'EOF'
# SONiC's nested Docker uses iptables-legacy; load host NAT support at boot.
iptable_nat
EOF
chmod 0644 /etc/modules-load.d/sonic-vs-runner.conf
chown root:root /etc/modules-load.d/sonic-vs-runner.conf
python3 "$(dirname "$0")/apparmor_gs.py" --install
install -d -m 0700 -o sonic-runner -g sonic-runner "$runner_home"
install -d -m 0700 -o sonic-runner -g sonic-runner "$runner_home/attempts"
# A child bind mount confines SONiC's device/setuid requirements to its home.
# mount's initial bind can retain restrictive parent flags: the oneshot below
# explicitly remounts the bind after systemd creates it on every boot.
python3 - <<'PY'
from pathlib import Path
import shutil
fstab = Path('/etc/fstab')
entry = '/data/sonic-runner /data/sonic-runner none bind,dev,suid,exec,nofail,x-systemd.requires-mounts-for=/data 0 0'
text = fstab.read_text()
existing = [line for line in text.splitlines() if not line.lstrip().startswith('#')
            and len(line.split()) > 1 and line.split()[1] == '/data/sonic-runner']
if existing and existing != [entry]:
    raise SystemExit('Existing /data/sonic-runner fstab entry differs; review it before proceeding')
if not existing:
    backup = Path('/etc/fstab.before-sonic-runner')
    if not backup.exists():
        shutil.copy2(fstab, backup)
    with fstab.open('a') as stream:
        stream.write('\n' + entry + '\n')
PY
if ! mountpoint -q "$runner_home"; then mount --bind "$runner_home" "$runner_home"; fi
cat > /etc/systemd/system/sonic-vs-workspace-permissions.service <<'EOF'
[Unit]
Description=Enable SONiC chroot devices and setuid on the workspace bind mount
RequiresMountsFor=/data/sonic-runner
Before=sonic-vs-runner.service

[Service]
Type=oneshot
ExecStart=/usr/bin/mount -o remount,bind,dev,suid,exec /data/sonic-runner
RemainAfterExit=yes
EOF
install -d -m 0755 -o root -g root "$tool_root/releases"
archive="$tool_root/releases/actions-runner-linux-x64-$version.tar.gz"
download=$(mktemp "$tool_root/releases/.download.XXXXXX")
trap 'rm -f "$download"' EXIT
curl --fail --location --retry 3 --proto '=https' --tlsv1.2 \
    "https://github.com/actions/runner/releases/download/v$version/actions-runner-linux-x64-$version.tar.gz" -o "$download"
printf '%s  %s\n' "$sha256" "$download" | sha256sum --check --status
chmod 0644 "$download"
mv "$download" "$archive"
python3 -m venv "$tool_root"
"$tool_root/bin/python" -m pip install --disable-pip-version-check 'jinjanator==25.3.1'
if [[ -e /usr/local/bin/j2 || -L /usr/local/bin/j2 ]]; then
    [[ $(readlink -f /usr/local/bin/j2) == "$tool_root/bin/j2" ]] || { echo '/usr/local/bin/j2 already belongs to another installation.' >&2; exit 1; }
else
    ln -s "$tool_root/bin/j2" /usr/local/bin/j2
fi
install -d -m 0755 -o root -g root "$tool_root/runner"
install -m 0755 -o root -g root "$(dirname "$0")/preflight.py" "$(dirname "$0")/rearm.py" "$(dirname "$0")/apparmor_gs.py" "$tool_root/runner/"
cat > /etc/systemd/system/sonic-vs-runner.service <<'EOF'
[Unit]
Description=One-job SONiC VS GitHub Actions runner
Wants=network-online.target systemd-modules-load.service
After=network-online.target systemd-modules-load.service docker.service sonic-vs-workspace-permissions.service
Requires=docker.service sonic-vs-workspace-permissions.service
RequiresMountsFor=/data/sonic-runner
ConditionPathExists=/data/sonic-runner/current/.runner

[Service]
User=sonic-runner
Group=sonic-runner
SupplementaryGroups=docker kvm
WorkingDirectory=/data/sonic-runner/current
Environment=HOME=/data/sonic-runner
Environment=PATH=/usr/local/bin:/usr/bin:/bin
ExecStart=/data/sonic-runner/current/run.sh
Restart=no
KillMode=control-group
TimeoutStopSec=5min
UMask=0022

[Install]
WantedBy=multi-user.target
EOF
systemctl daemon-reload
systemctl restart sonic-vs-workspace-permissions.service
systemctl enable sonic-vs-runner.service
echo 'Provisioned. Run preflight.py as sonic-runner, then rearm.py as the GitHub-authenticated operator.'
