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

# Retain Debian's pkg_resources API and rebuild its installed and seed copies.
source_version=$(dpkg-query -W -f='${source:Version}' python3-pkg-resources)
apt-get build-dep -y "setuptools=$source_version"
apt-get source --download-only "setuptools=$source_version"
dpkg-source -x setuptools_*.dsc source

wget -O setuptools-normalization.patch \
    https://github.com/pypa/setuptools/commit/dd9f436a36486b4cb8a4c70a2321548b0be09b8f.patch
echo '54a7c1728f70cfa7dafcb61ad09edae224dab5cd73725e383ee644314019b03c  setuptools-normalization.patch' | sha256sum -c -
patch --batch --fuzz=0 -p1 --directory source < setuptools-normalization.patch

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
    'Backport CVE-2026-59890 and refresh vendors for CVE-2026-23949 and CVE-2026-24049.'
dpkg-buildpackage -b -us -uc
cd ..

wget -O python3-wheel.deb \
    https://snapshot.debian.org/archive/debian/20260204T023029Z/pool/main/w/wheel/python3-wheel_0.46.3-1_all.deb
echo '6278ab7d12e99e8e896476d7b8cf5b54e62b5d685a893cd4cfa1d4a5a06ecec2  python3-wheel.deb' | sha256sum -c -
apt-get install -y ./python3-pkg-resources_*.deb ./python3-setuptools_*.deb \
    ./python3-setuptools-whl_*.deb ./python3-wheel.deb
