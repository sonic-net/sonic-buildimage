"""Small declared inputs for the real shared-layer Bazel action."""

import io
import sys
import tarfile


for output, value in zip(sys.argv[1:], (b"first", b"second")):
    with tarfile.open(output, "w") as archive:
        for name in ("usr", "usr/lib", "usr/bin"):
            member = tarfile.TarInfo(name)
            member.type = tarfile.DIRTYPE
            member.mode = 0o755
            member.uid, member.gid = 42, 43
            archive.addfile(member)
        for name, data in (("usr/lib/common", b"shared content"), ("usr/bin/tool", value)):
            member = tarfile.TarInfo(name)
            member.size = len(data)
            member.mode = 0o755
            member.uid, member.gid = 42, 43
            archive.addfile(member, io.BytesIO(data))
