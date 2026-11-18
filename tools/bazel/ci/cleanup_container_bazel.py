#!/usr/bin/env python3
"""Reclaim job-owned Bazel state after cache saves and artifact uploads."""

import argparse
from contextlib import ExitStack
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

from .cleanup_runner_bazel_cache import CACHE_NAMES
from .cleanup_vs_workspace import check_container_users, check_mounts
from .repair_bazel_startup import (
    DIRECTORY_FLAGS, mount_id, open_directory, remove_install_directory,
)


LOCAL_CACHES = (*CACHE_NAMES, "bazelisk")
UPLOAD_PREFIX = "bazel_upload_"
AUTHORIZATION = "bazel-cleanup-authorization.json"
IDENTITY = "job-identity.json"
CONTAINER_JOBS = {"archive", "gearbox_archive", "dash_engine_archive", "radv_native", "native", "otel_archive"}
SUPPORTED_JOBS = CONTAINER_JOBS | {"package"}


def owner(workspace):
    if (not workspace.is_absolute() or not workspace.is_dir() or
            any(character in str(workspace) for character in "\r\n")):
        raise ValueError("Cleanup requires an existing absolute checkout")
    identity = {
        "workspace": str(workspace.resolve(strict=True)),
        "run_id": os.environ["GITHUB_RUN_ID"],
        "run_attempt": os.environ["GITHUB_RUN_ATTEMPT"],
        "job": os.environ["GITHUB_JOB"],
    }
    if (identity["job"] not in SUPPORTED_JOBS or
            not all(re.fullmatch(r"[1-9][0-9]*", identity[key])
                    for key in ("run_id", "run_attempt"))):
        raise ValueError("Cleanup requires a supported Bazel job and numeric run identity")
    return identity


def prepare(workspace, runner_temp):
    identity = owner(workspace)
    if (not runner_temp.is_absolute() or ".." in runner_temp.parts or
            any(character in str(runner_temp) for character in "\r\n")):
        raise ValueError("Runner temporary directory must be an absolute single-line path")
    with ExitStack() as descriptors:
        open_directory(workspace, descriptors)
        open_directory(runner_temp, descriptors)
    prefix = f"sonic-bazel-job-{identity['run_id']}-{identity['run_attempt']}-{identity['job']}-"
    directory = Path(tempfile.mkdtemp(prefix=prefix, dir=runner_temp))
    stat = directory.stat()
    (directory / IDENTITY).write_text(json.dumps({
        "schema": 1, **identity, "device": stat.st_dev, "inode": stat.st_ino,
    }, sort_keys=True) + "\n")
    return directory


def job_identity(workspace, runner_temp, directory):
    identity = owner(workspace)
    prefix = f"sonic-bazel-job-{identity['run_id']}-{identity['run_attempt']}-{identity['job']}-"
    if (not directory.is_absolute() or directory.parent != runner_temp or
            not re.fullmatch(re.escape(prefix) + r"[a-z0-9_]{8}", directory.name)):
        raise ValueError("Refusing cleanup outside the registered runner job directory")
    with ExitStack() as descriptors:
        descriptor = open_directory(directory, descriptors)
        marker = os.open(IDENTITY, os.O_RDONLY | os.O_NOFOLLOW, dir_fd=descriptor)
        with os.fdopen(marker) as source:
            report = json.load(source)
        current = os.fstat(descriptor)
        if (not isinstance(report, dict) or report.get("schema") != 1 or
                any(report.get(key) != value for key, value in identity.items()) or
                (report.get("device"), report.get("inode")) != (current.st_dev, current.st_ino)):
            raise ValueError("Runner job directory identity changed")
    return current.st_dev, current.st_ino


def publication_status(steps):
    if not isinstance(steps, dict):
        raise ValueError("Cleanup requires the GitHub steps object")
    uploads = [name for name in steps if name.startswith(UPLOAD_PREFIX)]
    if not uploads:
        raise ValueError("No declared Bazel artifact uploads in cleanup status")
    succeeded = False
    for upload in uploads:
        publisher = upload.removeprefix(UPLOAD_PREFIX)
        for name in (publisher, upload):
            record = steps.get(name)
            if not isinstance(record, dict) or record.get("outcome") not in {
                    "success", "skipped", "failure", "cancelled"}:
                raise ValueError(f"Missing or invalid publication status: {name}")
        outcomes = (steps[publisher]["outcome"], steps[upload]["outcome"])
        if outcomes not in (("success", "success"), ("skipped", "skipped")):
            return False, f"Publication incomplete: {publisher}, {upload}"
        succeeded |= outcomes == ("success", "success")
    return succeeded, "Uploads completed" if succeeded else "No artifact upload succeeded"


def mark_artifacts(workspace, runner_temp, directory, steps):
    eligible, reason = publication_status(steps)
    job_identity(workspace, runner_temp, directory)
    with ExitStack() as descriptors:
        artifact_identities = {}
        for relative in artifact_directories():
            path = workspace / relative
            try:
                artifacts = open_directory(path, descriptors)
            except FileNotFoundError:
                continue
            check_mounts(path)
            stat = os.fstat(artifacts)
            artifact_identities[relative] = [stat.st_dev, stat.st_ino]
        report = {
            "schema": 1, **owner(workspace), "eligible": eligible, "reason": reason,
            "artifacts": artifact_identities,
        }
        target = open_directory(directory, descriptors)
        descriptor = os.open(AUTHORIZATION, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW,
                             0o600, dir_fd=target)
        with os.fdopen(descriptor, "w") as output:
            output.write(json.dumps(report, sort_keys=True) + "\n")
    print(f"Artifact cleanup authorization: {reason}")


def artifact_directories():
    if os.environ["GITHUB_JOB"] == "dash_engine_archive":
        # DASH evidence lives entirely inside the registered job directory.
        return ()
    if os.environ["GITHUB_JOB"] == "otel_archive":
        return ("artifacts/otel",)
    if os.environ["GITHUB_JOB"] == "package":
        return ("src/sonic-yang-models/artifacts",)
    return ("artifacts",)


def servers(workspace, home, directory, private_home=True):
    selected = []
    for process in Path("/proc").iterdir():
        if not process.name.isdecimal():
            continue
        try:
            arguments = (process / "cmdline").read_bytes().split(b"\0")
        except (FileNotFoundError, ProcessLookupError):
            continue
        except PermissionError:
            if process.stat().st_uid == os.geteuid():
                raise
            continue
        if not arguments[0].startswith(b"bazel("):
            continue
        settings = {}
        for argument in arguments[1:]:
            for flag in (b"--workspace_directory=", b"--output_base=", b"--output_user_root="):
                if argument.startswith(flag):
                    settings[flag] = Path(os.fsdecode(argument[len(flag):]))
        server_workspace = settings.get(b"--workspace_directory=")
        base = settings.get(b"--output_base=")
        root = settings.get(b"--output_user_root=")
        if not (
            server_workspace is not None and
            (server_workspace.is_relative_to(workspace) or
             server_workspace.is_relative_to(directory)) or
            private_home and root is not None and root.is_relative_to(home / ".cache/bazel")
        ):
            continue
        if any(path is None or not path.is_absolute() or ".." in path.parts
               for path in (base, root)):
            raise ValueError(f"Cannot identify checkout-owned Bazel server {process.name}")
        if server_workspace is not None and (
                not server_workspace.is_absolute() or ".." in server_workspace.parts):
            raise ValueError(f"Cannot identify Bazel server workspace {process.name}")
        selected.append((base, root, server_workspace))
    return selected


def stop_servers(workspace, home, directory, bazel, private_home=True):
    selected = servers(workspace, home, directory, private_home=private_home)
    for base, root, server_workspace in selected:
        print(f"Stopping checkout-owned Bazel server: {base}", flush=True)
        current = workspace
        if server_workspace is not None:
            if server_workspace.is_dir():
                current = server_workspace
            else:
                print(f"::warning::Server workspace no longer exists: {server_workspace}; "
                      "shutting down its explicit output base from the checkout")
        subprocess.run(
            [bazel, "--ignore_all_rc_files", f"--output_user_root={root}",
             f"--output_base={base}", "shutdown"],
            cwd=current, stdin=subprocess.DEVNULL, check=True, timeout=120)
    if servers(workspace, home, directory, private_home=private_home):
        raise ValueError("Checkout-owned Bazel servers remain active; refusing cleanup")
    return selected


def remove_selected(parent_path, names, expected=None):
    with ExitStack() as descriptors:
        parent = open_directory(parent_path, descriptors)
        device = os.fstat(parent).st_dev
        mount = mount_id(parent)
        selected = []
        for name in names:
            try:
                descriptor = os.open(name, DIRECTORY_FLAGS, dir_fd=parent)
            except FileNotFoundError:
                continue
            descriptors.callback(os.close, descriptor)
            identity = os.fstat(descriptor)
            if expected is not None and (identity.st_dev, identity.st_ino) != expected[name]:
                raise ValueError(f"Authorized cleanup target changed: {parent_path / name}")
            if identity.st_dev != device or mount_id(descriptor) != mount:
                raise ValueError(f"Refusing cleanup of a mounted directory: {parent_path / name}")
            check_mounts(parent_path / name)
            selected.append((name, descriptor, identity))
        current_parent = open_directory(parent_path, descriptors)
        if (os.fstat(current_parent).st_dev, os.fstat(current_parent).st_ino) != (
                os.fstat(parent).st_dev, os.fstat(parent).st_ino):
            raise ValueError("Cleanup parent changed during checks")
        for name, descriptor, identity in selected:
            current = os.stat(name, dir_fd=parent, follow_symlinks=False)
            if (current.st_dev, current.st_ino) != (identity.st_dev, identity.st_ino):
                raise ValueError(f"Cleanup target changed during checks: {parent_path / name}")
            print(f"Removing job-local generated directory: {parent_path / name}", flush=True)
            remove_install_directory(descriptor, device, mount)
            os.rmdir(name, dir_fd=parent)


def cleanup_artifacts(workspace, runner_temp, directory, host=False):
    identity = job_identity(workspace, runner_temp, directory)
    with ExitStack() as descriptors:
        parent = open_directory(directory, descriptors)
        try:
            descriptor = os.open(AUTHORIZATION, os.O_RDONLY | os.O_NOFOLLOW, dir_fd=parent)
        except FileNotFoundError:
            print("::warning::Preserving job artifacts: no upload-cleanup authorization")
            return
        with os.fdopen(descriptor) as source:
            report = json.load(source)
        if (not isinstance(report, dict) or report.get("schema") != 1 or
                any(report.get(key) != value for key, value in owner(workspace).items()) or
                type(report.get("eligible")) is not bool):
            raise ValueError("Artifact cleanup authorization does not belong to this job")
        if not report["eligible"]:
            print("::warning::Preserving artifacts: " + report["reason"])
            return
    artifacts = report.get("artifacts")
    if (not isinstance(artifacts, dict) or
            any(relative not in artifact_directories() or not isinstance(expected, list) or
                len(expected) != 2 or any(type(value) is not int for value in expected)
                for relative, expected in artifacts.items())):
        raise ValueError("Artifact authorization includes an unexpected cleanup directory")
    for relative, expected in artifacts.items():
        path = workspace / relative
        if host:
            check_container_users(path)
        remove_selected(path.parent, (path.name,), expected={path.name: tuple(expected)})
    if host:
        check_container_users(directory)
    remove_selected(runner_temp, (directory.name,), expected={directory.name: identity})


def cleanup_host_bases(home, workspace, bazel):
    for current in (workspace, workspace / "src/sonic-yang-models"):
        if not current.is_dir():
            continue
        result = subprocess.run(
            [bazel, "--batch", "--ignore_all_rc_files", "info", "output_base"],
            cwd=current, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
            text=True, check=True, timeout=120)
        base = Path(result.stdout.strip())
        if (not base.is_absolute() or base.parent.parent != home / ".cache/bazel" or
                not base.parent.name.startswith("_bazel_") or
                not re.fullmatch(r"[0-9a-f]{32}", base.name) or ".." in base.parts):
            raise ValueError("Bazel reported an unexpected host output base")
        check_container_users(base)
        remove_selected(base.parent, (base.name,))


def cleanup(home, workspace, runner_temp, directory, bazel="bazel", host=False):
    job_identity(workspace, runner_temp, directory)
    stop_servers(workspace.resolve(strict=True), home, directory, bazel, private_home=not host)
    if host:
        cleanup_host_bases(home, workspace, bazel)
    else:
        cache = home / ".cache"
        if cache.exists() or cache.is_symlink():
            remove_selected(cache, LOCAL_CACHES)
        else:
            print("No container-local Bazel caches to remove")
    if os.environ["GITHUB_JOB"] == "archive":
        # Source probes may be interrupted before an artifact upload is eligible.
        remove_selected(directory, ("archive-bazel",))
    cleanup_artifacts(workspace, runner_temp, directory, host=host)


def host_paths(workspace, runner_temp):
    if (not workspace.is_absolute() or not runner_temp.is_absolute() or
            workspace.parent.parent != runner_temp.parent or
            any(".." in path.parts or any(character in str(path) for character in "\r\n")
                for path in (workspace, runner_temp))):
        raise ValueError("Cleanup requires the GitHub host workspace layout")
    work_root = runner_temp.parent.resolve(strict=True)
    workspace = work_root / workspace.relative_to(runner_temp.parent)
    runner_temp = work_root / runner_temp.name
    with ExitStack() as descriptors:
        open_directory(workspace, descriptors)
        open_directory(runner_temp, descriptors)
    return workspace, runner_temp


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--prepare", action="store_true")
    mode.add_argument("--mark-artifacts", action="store_true")
    parser.add_argument("--job-directory", type=Path)
    args = parser.parse_args()
    try:
        workspace = Path(os.environ["GITHUB_WORKSPACE"])
        runner_temp = Path(os.environ["RUNNER_TEMP"])
        home = Path(os.environ["HOME"])
        container = home == Path("/github/home") and os.geteuid() == 0 and Path("/.dockerenv").is_file()
        host = (not Path("/.dockerenv").is_file() and os.environ["GITHUB_JOB"] == "package" and
                workspace.parent.parent == runner_temp.parent and
                workspace.name == os.environ.get("GITHUB_REPOSITORY", "").rsplit("/", 1)[-1])
        if os.environ.get("GITHUB_ACTIONS") != "true" or not (container or host):
            raise ValueError("Cleanup requires a GitHub Bazel job container or the YANG host job")
        if host:
            workspace, runner_temp = host_paths(workspace, runner_temp)
        if args.prepare:
            print(json.dumps({"directory": str(prepare(workspace, runner_temp))}))
        elif args.mark_artifacts:
            mark_artifacts(workspace, runner_temp, Path(os.environ["SONIC_BAZEL_JOB_TEMP"]),
                           json.loads(os.environ["BAZEL_CLEANUP_STEPS"]))
        else:
            if args.job_directory is None:
                raise ValueError("Cleanup requires its registered job directory")
            cleanup(home, workspace, runner_temp, args.job_directory, host=host)
    except (KeyError, OSError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(1, f"Container Bazel cleanup failed: {error}\n")


if __name__ == "__main__":
    main()
