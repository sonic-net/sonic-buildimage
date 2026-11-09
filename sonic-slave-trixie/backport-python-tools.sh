#!/bin/bash
set -euo pipefail

mkdir /tmp/python-build-tools
cd /tmp/python-build-tools

set --
case ",${SONIC_VERSION_CONTROL_COMPONENTS:-}," in
    *,all,*|*,py3,*) set -- --constraint /usr/local/share/buildinfo/versions/versions-py3 ;;
esac
pip3 download --only-binary=:all: --no-deps "$@" --dest wheels \
    'jaraco.context>=6.1.0' 'wheel>=0.46.3'

# Rebuild the configured Trixie package, retaining its Debian integration and
# pkg_resources API while updating both the installed and seed-wheel vendors.
apt-get build-dep -y setuptools
apt-get source --download-only setuptools
dpkg-source -x setuptools_*.dsc source

python3 - <<'PY'
from email.parser import Parser
from pathlib import Path
import shutil
import zipfile

vendor = Path("source/setuptools/_vendor")
for path in (vendor / "wheel", vendor / "jaraco/context",
             vendor / "jaraco/context.py"):
    if path.is_dir():
        shutil.rmtree(path)
    elif path.exists():
        path.unlink()
for metadata in vendor.glob("*.dist-info"):
    info = Parser().parsestr((metadata / "METADATA").read_text())
    if info["Name"].lower().replace("_", ".") in ("jaraco.context", "wheel"):
        shutil.rmtree(metadata)
for wheel in Path("wheels").glob("*.whl"):
    with zipfile.ZipFile(wheel) as archive:
        archive.extractall(vendor)
PY

cd source
dch --local +0sonic --distribution trixie --maintmaint \
    'Refresh vendored jaraco.context and wheel for CVE-2026-23949 and CVE-2026-24049.'
dpkg-buildpackage -b -us -uc
cd ..

# This pure-Python Debian package needs no newer-suite APT source or libraries.
wget -O python3-wheel.deb \
    https://snapshot.debian.org/archive/debian/20260204T023029Z/pool/main/w/wheel/python3-wheel_0.46.3-1_all.deb
echo '6278ab7d12e99e8e896476d7b8cf5b54e62b5d685a893cd4cfa1d4a5a06ecec2  python3-wheel.deb' | sha256sum -c -
apt-get install -y ./python3-pkg-resources_*.deb ./python3-setuptools_*.deb \
    ./python3-setuptools-whl_*.deb ./python3-wheel.deb
