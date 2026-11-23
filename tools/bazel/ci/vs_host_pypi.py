"""Install a temporary, digest-verified pip wheelhouse in the stock VS seed."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import os
from pathlib import Path
import re
import shutil
import time
from urllib.parse import urlsplit
from urllib.request import urlopen

LOCK = Path(__file__).with_suffix(".lock")
CONSTRAINTS = Path("files/build/versions/host-image/versions-py3")
STOCK_CONFIG = Path("files/image_config/pip/pip.conf")
STAGING = Path("var/lib/sonic-configured-host-pip")
# pyangbind requires enum34 during installation. The host extension explicitly
# uninstalls it immediately afterwards, so the final host constraints omit it.
TRANSIENT_PINS = {"enum34==1.1.10"}


def archives(lock, constraints):
    pins = set(constraints.read_text().splitlines()) | TRANSIENT_PINS
    seen = set()
    for line in lock.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        requirement, digest, url = line.split()
        parsed = urlsplit(url)
        filename = parsed.path.rsplit("/", 1)[-1]
        if (requirement not in pins or requirement in seen or parsed.scheme != "https"
                or parsed.netloc != "files.pythonhosted.org" or parsed.query or parsed.fragment
                or not re.fullmatch(r"[0-9a-f]{64}", digest)
                or not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.+-]*\.(whl|tar\.gz|zip)", filename)):
            raise ValueError(f"invalid or stale configured-host Python lock: {requirement}")
        seen.add(requirement)
        yield filename, digest, url
    missing = TRANSIENT_PINS - seen
    if missing:
        raise ValueError(f"missing transient configured-host Python archive: {', '.join(sorted(missing))}")


def download(directory, archive):
    filename, expected, url = archive
    target_path = directory / filename
    for attempt in range(3):
        digest = hashlib.sha256()
        try:
            with urlopen(url, timeout=60) as source, target_path.open("wb") as target:
                for block in iter(lambda: source.read(1024 * 1024), b""):
                    digest.update(block)
                    target.write(block)
            break
        except OSError:
            if attempt == 2:
                raise
            time.sleep(2 ** attempt)
    if digest.hexdigest() != expected:
        raise ValueError(f"configured-host Python archive digest mismatch: {filename}")


def install(root):
    config = root / "etc/pip.conf"
    if config.is_symlink() or config.read_bytes() != STOCK_CONFIG.read_bytes():
        raise ValueError("expected the stock pip configuration in the unpersonalized VS host")
    sources = list(archives(LOCK, CONSTRAINTS))
    directory = root / STAGING
    directory.mkdir(mode=0o755)
    try:
        with ThreadPoolExecutor(max_workers=6) as pool:
            list(pool.map(lambda item: download(directory, item), sources))
        # Keep the original inode and its metadata for removal before sealing.
        staged_config = directory / "pip.conf"
        staged_config.write_bytes(config.read_bytes().rstrip() + (
            f"\nno-index = true\nfind-links = file:///{STAGING}\nno-cache-dir = true\n"
        ).encode())
        os.link(config, directory / "original-pip.conf")
        os.replace(staged_config, config)
    except BaseException:
        shutil.rmtree(directory)
        raise


def remove(root):
    directory = root / STAGING
    os.replace(directory / "original-pip.conf", root / "etc/pip.conf")
    shutil.rmtree(directory)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("--remove", action="store_true")
    args = parser.parse_args()
    (remove if args.remove else install)(args.root)
