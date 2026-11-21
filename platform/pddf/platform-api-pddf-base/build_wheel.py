"""Build the PDDF source wheel with the declared setuptools backend."""

import os
from pathlib import Path
import shutil
import sys
from tempfile import TemporaryDirectory

from setuptools import build_meta


def main():
    output, setup, *sources = map(Path, sys.argv[1:])
    output = output.absolute()
    os.umask(0o022)
    with TemporaryDirectory(dir=output.parent) as tmp:
        work = Path(tmp)
        for source in [setup, *sources]:
            destination = work / source.relative_to(setup.parent)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
            destination.chmod(0o755 if source.stat().st_mode & 0o111 else 0o644)
        os.chdir(work)
        os.environ["SOURCE_DATE_EPOCH"] = "315532800"
        wheel = build_meta.build_wheel(str(output.parent))
        if wheel != output.name:
            raise ValueError(f"setup.py produced {wheel}, expected {output.name}")


if __name__ == "__main__":
    main()
