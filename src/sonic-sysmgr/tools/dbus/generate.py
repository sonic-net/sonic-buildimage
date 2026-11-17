"""Run dbusxx-xml2cpp with its declared Trixie execution libraries."""

import argparse
import os
from pathlib import Path, PurePosixPath
import posixpath
import shutil
import subprocess
import tarfile
import tempfile


def installed_member(member, destination):
    # Resolve Debian absolute links inside the private execution filesystem.
    if member.linkname.startswith("/") and (member.issym() or member.islnk()):
        target = posixpath.normpath("/" + member.linkname.lstrip("/")).lstrip("/")
        if member.issym():
            target = posixpath.relpath(target, str(PurePosixPath(member.name).parent))
        member = member.replace(linkname=target)
    return tarfile.data_filter(member, destination)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--xml", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--mode", choices=("proxy", "adaptor"), required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="sysmgr-dbus-generator-") as directory:
        root = Path(directory).resolve()
        with tarfile.open(args.runtime) as archive:
            archive.extractall(root, filter=installed_member)
        loaders = {p.resolve() for p in root.rglob("ld-linux-x86-64.so.2") if p.is_file()}
        if len(loaders) != 1:
            raise ValueError("The AMD64 generator requires one declared loader")
        libraries = {p.parent.resolve() for p in root.rglob("lib*.so*") if p.is_file()}
        if not all(p.is_relative_to(root) for p in loaders | libraries):
            raise ValueError("Generator runtime escapes its declared root")
        environment = dict(os.environ, LC_ALL="C")
        for name in ("LD_LIBRARY_PATH", "LD_PRELOAD", "LD_AUDIT"):
            environment.pop(name, None)
        # xml2cpp derives its include guard from the output argument. A sandbox
        # absolute path would make otherwise identical generated headers differ.
        work = root / "generation"
        work.mkdir()
        shutil.copyfile(args.xml, work / "input.xml")
        command = [str(loaders.pop()), "--library-path", ":".join(map(str, sorted(libraries))),
                   str(root / "usr/bin/dbusxx-xml2cpp"), "input.xml",
                   "--" + args.mode + "=" + args.output.name]
        subprocess.run(command, cwd=work, env=environment, check=True, timeout=60)
        shutil.copyfile(work / args.output.name, args.output)


if __name__ == "__main__":
    main()
