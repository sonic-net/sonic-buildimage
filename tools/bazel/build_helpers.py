"""Shared cache settings and atomic archive publication for Make/Bazel builds."""

import argparse
import filecmp
import os
from pathlib import Path
import shlex
import shutil
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from tools.bazel.cache import credentials


def cache_options(cache_directory):
    """Share content-addressed caches, keeping each Bazel output base private."""
    if not cache_directory:
        return []
    root = Path(cache_directory).expanduser().resolve()
    options = []
    for name, flag in (("repository_cache", "repository_cache"), ("disk_cache", "disk_cache")):
        path = root / name
        path.mkdir(parents=True, exist_ok=True)
        options.append(f"--{flag}={path}")
    return options


def remote_cache_options():
    """Read the shared cache without exposing authentication in Bazel options."""
    if not os.environ.get("BAZEL_REMOTE_AUTH"):
        endpoint = os.environ.get("SONIC_BAZEL_REMOTE_CACHE")
        if not endpoint:
            return []
        upload = "true" if os.environ.get("SONIC_BAZEL_REMOTE_UPLOAD") == "y" else "false"
        return [f"--remote_cache={endpoint}", f"--remote_upload_local_results={upload}"]
    credentials.authorization()
    helper = Path(__file__).resolve().parent / "cache/credentials.py"
    return [
        "--remote_cache=" + credentials.ENDPOINT + "/",
        "--credential_helper=" + credentials.HOST + "=" + str(helper),
        "--remote_executor=",
        "--remote_accept_cached=true",
        "--remote_upload_local_results=false",
    ]


def export_archive(source, destination):
    """Leave the previous image intact if copying a new archive fails."""
    if destination.is_file() and filecmp.cmp(source, destination, shallow=False):
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=destination.parent, prefix=destination.name + ".",
                                     delete=False) as output:
        temporary = Path(output.name)
    try:
        shutil.copyfile(source, temporary)
        temporary.chmod(0o644)
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    options = commands.add_parser("options", help="Print one Bazel option per line")
    options.add_argument("--cache-directory")
    options.add_argument("--arguments", default="", help="Additional shell-quoted Bazel options")
    export = commands.add_parser("export", help="Publish the single archive reported by cquery")
    export.add_argument("--query-output", required=True)
    export.add_argument("--output", type=Path, required=True)
    export.add_argument("--execution-root", type=Path)
    args = parser.parse_args()
    if args.command == "options":
        arguments = cache_options(args.cache_directory) + shlex.split(args.arguments)
        try:
            # Apply the shared cache policy after caller options so Make image
            # builds cannot accidentally upload results or enable remote execution.
            arguments += remote_cache_options()
        except ValueError:
            parser.exit(2, credentials.ERROR + "\n")
        if any(not value or any(char in value for char in "\r\n\0") for value in arguments):
            parser.error("Bazel options must be nonempty single-line arguments")
        for value in arguments:
            print(value)
    else:
        files = args.query_output.splitlines()
        if len(files) != 1:
            parser.error("expected exactly one Bazel archive output")
        source = Path(files[0])
        if args.execution_root is not None:
            if not args.execution_root.is_absolute() or source.is_absolute() or ".." in source.parts:
                parser.error("expected a Bazel exec path relative to the producing execution root")
            source = args.execution_root / source
        if not source.is_file() or not source.stat().st_size:
            parser.error("Bazel did not produce a nonempty archive")
        export_archive(source, args.output)


if __name__ == "__main__":
    main()
