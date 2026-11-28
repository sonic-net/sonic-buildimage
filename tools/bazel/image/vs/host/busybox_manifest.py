"""Export the selected native Debian BusyBox applet contract for initramfs."""
import argparse
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import tarfile


def build(busybox, control, output):
    with tarfile.open(control, "r:*") as archive:
        controls = [m for m in archive if m.name.removeprefix("./") == "control"]
        if len(controls) != 1:
            raise ValueError("BusyBox control archive must contain one control file")
        fields = dict(line.split(": ", 1) for line in archive.extractfile(controls[0]).read().decode().splitlines() if ": " in line and not line[0].isspace())
    if fields.get("Package") != "busybox" or fields.get("Architecture") != "amd64":
        raise ValueError("Applet export requires the selected native AMD64 busybox package")
    version = fields["Version"]
    version_output = subprocess.run([busybox], capture_output=True, text=True, check=False)
    # BusyBox with no applet prints its banner and exits 0 or 1 by configuration.
    banner = version_output.stdout + version_output.stderr
    upstream = version.split(":", 1)[-1].split("-", 1)[0]
    if not re.search(r"BusyBox v" + re.escape(upstream) + r"(?:\s|\()", banner):
        raise ValueError("BusyBox executable version differs from the package control")
    applets = subprocess.run([busybox, "--list-full"], capture_output=True, text=True, check=True).stdout.splitlines()
    if not applets or len(set(applets)) != len(applets):
        raise ValueError("BusyBox applet list is empty or contains duplicates")
    if any(PurePosixPath(a).is_absolute() or ".." in PurePosixPath(a).parts or not a or any(c.isspace() for c in a) for a in applets):
        raise ValueError("BusyBox --list-full returned an invalid installed applet path")
    Path(output).write_text(json.dumps({"version": re.sub(r"\+b[0-9]+$", "", version), "applets": applets}, indent=2) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--busybox", required=True)
    parser.add_argument("--control", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    build(args.busybox, args.control, args.out)
