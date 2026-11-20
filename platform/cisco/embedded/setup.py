#!/usr/bin/env python3

import os
from pathlib import Path
import shutil
import subprocess

from setuptools import find_packages, setup

deb_path = Path(os.environ["CISCO_EMBEDDED_PLATFORM_MODULE_DEB"])
if not deb_path.is_file():
    raise RuntimeError(f"Cisco platform module deb not found: {deb_path}")

stage = deb_path.parent / ".cisco-platform-wheel-src"
shutil.rmtree(stage, ignore_errors=True)
stage.mkdir(parents=True)
try:
    subprocess.run(["dpkg-deb", "-x", str(deb_path), str(stage)], check=True)
    package_root = stage / "usr/lib/python3/dist-packages"
    packages = find_packages(
        where=str(package_root), include=("sonic_platform", "sonic_platform.*")
    )
    if not packages:
        raise RuntimeError(f"No sonic_platform packages found under {package_root}")

    setup(
        name="sonic-platform",
        version="1.0",
        description="SONiC platform API implementation on Cisco 8000 platforms",
        license="Apache-2.0",
        packages=packages,
        package_dir={"": str(package_root)},
    )
finally:
    shutil.rmtree(stage, ignore_errors=True)
