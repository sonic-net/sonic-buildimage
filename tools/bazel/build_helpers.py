"""Shared cache settings and atomic archive publication for Make/Bazel builds."""

import argparse
import filecmp
import os
from pathlib import Path
import shlex
import shutil
import subprocess
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


def boot_layout_labels(bazel, labels):
    """Archive owners can opt into the native layout handoff; legacy aliases still export archives."""
    candidates = {label + ".boot-oci" for label in labels if label.startswith("//") and ":" in label}
    if not candidates:
        return set()
    packages = sorted({label.rsplit(":", 1)[0] + ":all" for label in candidates})
    expression = 'filter("[.]boot-oci$", set(' + " ".join(packages) + "))"
    result = subprocess.run([bazel, "query", "--output=label", expression], check=True,
                            stdout=subprocess.PIPE, text=True)
    return candidates.intersection(result.stdout.splitlines())


def matching_archive(source, destination):
    """Identify the already exported file by its open descriptor during comparison."""
    try:
        current = destination.open("rb")
    except FileNotFoundError:
        return None
    with current, source.open("rb") as incoming:
        original = os.fstat(current.fileno())
        if original.st_size == os.fstat(incoming.fileno()).st_size:
            while chunk := incoming.read(1024 * 1024):
                if chunk != current.read(len(chunk)):
                    return None
            # A replacement cannot bind another inode to our layout. An in-place
            # write invalidates this pre-comparison ctime.
            return original
    return None


def export_archive(source, destination, *, bind_identity=False):
    """Atomically publish an archive, optionally identifying the exact file exported."""
    if bind_identity:
        original = matching_archive(source, destination)
        if original is not None:
            return original
    elif destination.is_file() and filecmp.cmp(source, destination, shallow=False):
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=destination.parent, prefix=destination.name + ".",
                                     delete=False) as output:
        temporary = Path(output.name)
        try:
            shutil.copyfile(source, temporary)
            temporary.chmod(0o644)
            if bind_identity:
                before = os.fstat(output.fileno())
            temporary.replace(destination)
            if bind_identity:
                published = os.fstat(output.fileno())
                if (published.st_size, published.st_mtime_ns) == (before.st_size, before.st_mtime_ns):
                    return published
        finally:
            temporary.unlink(missing_ok=True)


def export_image(source, destination, layout=None):
    """Export Make's archive and hand off only its matching configured OCI output."""
    handoff = os.environ.get("SONIC_VS_BOOT_HANDOFF") if layout is not None else None
    published = export_archive(source, destination, bind_identity=bool(handoff))
    if layout is not None:
        from tools.bazel.oci.boot_layout import publish_layout
        publish_layout(layout, source, destination, handoff=handoff, published=published)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    layouts = commands.add_parser("boot-layouts", help="Print available OCI layout targets for archive labels")
    layouts.add_argument("--bazel", required=True)
    layouts.add_argument("--label", action="append", required=True)
    options = commands.add_parser("options", help="Print one Bazel option per line")
    options.add_argument("--cache-directory")
    options.add_argument("--arguments", default="", help="Additional shell-quoted Bazel options")
    export = commands.add_parser("export", help="Publish the single archive reported by cquery")
    export.add_argument("--query-output", required=True)
    export.add_argument("--output", type=Path, required=True)
    export.add_argument("--execution-root", type=Path)
    export.add_argument("--layout-query-output", type=Path)
    args = parser.parse_args()
    if args.command == "boot-layouts":
        for label in sorted(boot_layout_labels(args.bazel, args.label)):
            print(label)
    elif args.command == "options":
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
        export_image(source, args.output, args.layout_query_output)


if __name__ == "__main__":
    main()
