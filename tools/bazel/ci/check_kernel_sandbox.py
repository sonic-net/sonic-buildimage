#!/usr/bin/env python3
"""Check the kernel's pinned Linux sandbox as the native builder user."""

import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


VERSION_FILE = Path(__file__).resolve().parents[1] / "kernel/.bazelversion"
DIAGNOSTIC_PATHS = (
    "/proc/self/attr/current",
    "/proc/sys/kernel/unprivileged_userns_clone",
    "/proc/sys/kernel/apparmor_restrict_unprivileged_userns",
    "/proc/sys/kernel/apparmor_restrict_unprivileged_unconfined",
    "/proc/sys/user/max_user_namespaces",
)


def diagnostics():
    """Print only sandbox-relevant process identity and kernel controls."""
    print(f"uid={os.getuid()} euid={os.geteuid()} gid={os.getgid()} egid={os.getegid()}", file=sys.stderr)
    for name in DIAGNOSTIC_PATHS:
        try:
            value = Path(name).read_text().strip()
        except OSError as error:
            value = f"unavailable ({error.strerror})"
        print(f"{name}: {value}", file=sys.stderr)
    try:
        for line in Path("/proc/self/status").read_text().splitlines():
            if line.split(":", 1)[0] in {"CapEff", "NoNewPrivs", "Seccomp"}:
                print(line, file=sys.stderr)
    except OSError as error:
        print(f"process security status unavailable ({error.strerror})", file=sys.stderr)


def check_sandbox(bazel=None):
    if os.getuid() == 0 or os.geteuid() == 0:
        raise RuntimeError("Run the kernel sandbox check as the unprivileged native build user")
    version = VERSION_FILE.read_text().strip()
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version):
        raise RuntimeError("Invalid pinned kernel Bazel version")
    bazel = bazel or os.environ.get("BAZEL", "bazel")
    environment = dict(os.environ, USE_BAZEL_VERSION=version)
    # Isolate Bazel's server/install state. Do not clear or repair the mounted
    # Bazelisk, repository, action, or Make package caches.
    with tempfile.TemporaryDirectory(prefix="sonic-kernel-sandbox-") as directory:
        workspace = Path(directory)
        (workspace / "WORKSPACE").write_text("")

        def run(command, timeout=120):
            return subprocess.run(command, cwd=workspace, env=environment,
                                  capture_output=True, text=True, timeout=timeout)

        release = run([bazel, "--version"])
        if release.returncode or release.stdout.strip() != f"bazel {version}":
            raise RuntimeError(f"Kernel sandbox check requires Bazel {version}; version check failed: "
                               + (release.stderr or release.stdout).strip())
        result = run([bazel, "--batch", "--nosystem_rc", "--nohome_rc", "--noworkspace_rc",
                      f"--output_user_root={workspace / 'output'}", "info",
                      "--enable_bzlmod=false", "--enable_workspace=true", "install_base"])
        if result.returncode:
            raise RuntimeError("Cannot locate the pinned Bazel sandbox: " + result.stderr.strip())
        install = Path(result.stdout.strip())
        if not install.is_absolute() or not install.is_dir():
            raise RuntimeError("Bazel returned an invalid sandbox install directory")
        helper = install / "linux-sandbox"
        if not helper.is_file() or not os.access(helper, os.X_OK):
            raise RuntimeError("Pinned Bazel linux-sandbox helper is missing or not executable")
        result = run([str(helper), "-W", str(workspace), "--", "/bin/true"], timeout=15)
        if result.returncode:
            # Namespace creation can succeed while AppArmor denies the later
            # sandbox mount. Report that transition without using it as a pass.
            try:
                namespace = run(["unshare", "--user", "--", "/bin/cat",
                                 "/proc/self/attr/current"], timeout=5)
                detail = f"exit {namespace.returncode}: " + (namespace.stdout or namespace.stderr).strip()
            except (OSError, subprocess.TimeoutExpired) as error:
                detail = f"unavailable ({error})"
            print("AppArmor after user-namespace creation: " + detail[:512], file=sys.stderr)
            raise RuntimeError(f"Bazel {version} linux-sandbox failed ({result.returncode}): "
                               + result.stderr.strip())
    print(f"Kernel linux-sandbox ready: Bazel {version}, uid={os.getuid()}, gid={os.getgid()}")


def main():
    try:
        check_sandbox()
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"Kernel sandbox readiness failed: {error}", file=sys.stderr)
        diagnostics()
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
