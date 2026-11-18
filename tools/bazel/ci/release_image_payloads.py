"""Release a published image's staged payloads while preserving its CI receipts."""

import argparse
import json
from pathlib import Path
import stat


# The shared SWSS build also produces private comparison TARs.
SWSS_UPLOAD = {"docker-swss-layer-trixie.gz", "source-runtime.tar", "source-debug.tar", "image-debug.tar"}


def release(artifacts, family):
    if Path(family).name != family or family in ("", ".", ".."):
        raise ValueError("Expected a single image family name")
    directory = artifacts / family
    if artifacts.is_symlink() or directory.is_symlink() or not directory.is_dir():
        raise ValueError("Expected an image directory inside the checkout artifacts")
    receipt = directory / ("receipt.json" if family == "swss-layer" else "build-receipt.json")
    if receipt.is_symlink() or not receipt.is_file():
        raise ValueError("Missing image build receipt")
    exports = json.loads(receipt.read_text())["artifacts"]
    directories = [directory]
    if family == "eventd":
        public = directory / "public"
        if public.is_symlink() or not public.is_dir():
            raise ValueError("Expected the Eventd publication directory")
        directories.append(public)

    payloads = []
    for name in exports:
        if Path(name).name != name or name in ("", ".", ".."):
            raise ValueError("Invalid exported filename in image build receipt")
        if family == "swss-layer" and name not in SWSS_UPLOAD:
            continue
        if not name.endswith((".tar", ".gz", ".whl")):
            continue
        for parent in directories:
            path = parent / name
            try:
                info = path.lstat()
            except FileNotFoundError:
                continue
            if not stat.S_ISREG(info.st_mode):
                raise ValueError("Expected a regular staged image payload: " + str(path))
            payloads.append((path, info.st_size))

    for path, _ in payloads:
        path.unlink()
    size = sum(length for _, length in payloads)
    print(f"Released {len(payloads)} staged {family} payloads ({size:,} logical bytes)")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("family")
    release(Path("artifacts"), parser.parse_args().family)
