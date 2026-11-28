"""The host root must expose the Debian merged-/usr loader and shell paths."""
import sys
import tarfile

with tarfile.open(sys.argv[1]) as archive:
    entries = {member.name.removeprefix("./").rstrip("/"): member for member in archive}
for name, target in [("bin", "usr/bin"), ("sbin", "usr/sbin"), ("lib", "usr/lib"), ("lib64", "usr/lib64"), ("var/run", "/run"), ("var/lock", "/run/lock")]:
    assert entries[name].issym(), name
    assert entries[name].linkname == target, (name, entries[name].linkname)
for name in ["usr", "usr/bin", "usr/sbin", "usr/lib", "usr/lib64", "run", "var"]:
    assert entries[name].isdir(), name
assert entries["run/lock"].isdir()
assert entries["run/lock"].mode == 0o1777
assert "usr/run/var/lib" not in entries
