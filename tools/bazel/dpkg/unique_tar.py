"""Retain the final member for each image path, preserving TAR payload metadata."""

from pathlib import PurePosixPath
import sys
import tarfile


def unique_tar(source, destination):
    with tarfile.open(source, "r:*") as archive:
        members = archive.getmembers()
        final = {str(PurePosixPath(member.name)): index for index, member in enumerate(members)}
        with tarfile.open(destination, "w", format=tarfile.PAX_FORMAT) as output:
            for index, member in enumerate(members):
                if final[str(PurePosixPath(member.name))] == index:
                    output.addfile(member, archive.extractfile(member) if member.isfile() else None)


if __name__ == "__main__":
    unique_tar(*sys.argv[1:])
