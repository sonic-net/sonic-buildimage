#!/usr/bin/env python3
"""Configure the legacy cache or verify the K3s worker's remote-only cache."""

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import stat
import ssl
import subprocess
import sys
import urllib.error
import urllib.request


RUNTIME_ROOT = Path("/opt/sonic-runner")
NATIVE_HELPER = Path("/opt/sonic-native-runtime/cache_credentials.py")
NATIVE_HELPER_SHA256 = "32ea5af5245984f9fac59f5507bc1a1b6d3272968d3b2edd52b0a03f82b1dc49"
PROBE_TIMEOUT = 8
FALLBACK_BEGIN = "# BEGIN SONiC runner cache connectivity fallback"
FALLBACK_END = "# END SONiC runner cache connectivity fallback"
RESOURCE_BEGIN = "# BEGIN SONiC native CI Bazel resources"
RESOURCE_END = "# END SONiC native CI Bazel resources"


class ConfigurationFailure(Exception):
    def __init__(self, error_class, http_status=None):
        super().__init__(error_class)
        self.error_class = error_class
        self.http_status = http_status


def load_credentials():
    # Use the image's reviewed origin and header validation, including when
    # PYTHONSAFEPATH excludes the script directory from module discovery.
    spec = importlib.util.spec_from_file_location(
        "sonic_runner_cache_credentials", RUNTIME_ROOT / "cache_credentials.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def response_mode(status):
    if status == 200:
        return {"mode": "local_and_remote", "error_class": None, "http_status": status}
    if 500 <= status <= 599:
        return {"mode": "local_only", "error_class": "remote_unavailable", "http_status": status}
    if status in (401, 403):
        raise ConfigurationFailure("authentication_rejected", status)
    if 300 <= status <= 399:
        raise ConfigurationFailure("redirect_refused", status)
    raise ConfigurationFailure("unexpected_http_status", status)


def check_remote(credentials, authorization):
    # Do not read the response body or follow redirects with the job's header.
    request = urllib.request.Request(
        credentials.ENDPOINT.rstrip("/") + "/status", method="GET",
        headers={"Authorization": authorization})
    opener = urllib.request.build_opener(credentials.NoRedirects())
    try:
        with opener.open(request, timeout=PROBE_TIMEOUT) as response:
            return response_mode(response.status)
    except urllib.error.HTTPError as error:
        status = error.code
        error.close()
        return response_mode(status)
    except urllib.error.URLError as error:
        code = "tls_error" if isinstance(error.reason, ssl.SSLError) else "transport_error"
    except ssl.SSLError:
        code = "tls_error"
    except OSError:
        code = "transport_error"
    return {"mode": "local_only", "error_class": code, "http_status": None}


def set_local_fallback(rc, enabled):
    contents = rc.read_text()
    if FALLBACK_BEGIN in contents or FALLBACK_END in contents:
        if (contents.count(FALLBACK_BEGIN) != 1 or contents.count(FALLBACK_END) != 1
                or contents.index(FALLBACK_BEGIN) > contents.index(FALLBACK_END)):
            raise ConfigurationFailure("configuration_failed")
        start = contents.index(FALLBACK_BEGIN)
        end = contents.index(FALLBACK_END) + len(FALLBACK_END)
        contents = contents[:start] + contents[end:].lstrip("\n")
    if enabled:
        # This last value overrides the public remote URL while keeping the
        # existing disk/repository caches, origin helper, and trust settings.
        contents = contents.rstrip("\n") + "\n\n" + "\n".join([
            FALLBACK_BEGIN,
            "# The remote cache was unavailable during this job's preflight.",
            "common --remote_cache=",
            FALLBACK_END,
        ]) + "\n"
    rc.write_text(contents)


def set_resource_limits(rc, jobs, local_memory_mb):
    contents = rc.read_text()
    if RESOURCE_BEGIN in contents or RESOURCE_END in contents:
        if (contents.count(RESOURCE_BEGIN) != 1 or contents.count(RESOURCE_END) != 1
                or contents.index(RESOURCE_BEGIN) > contents.index(RESOURCE_END)):
            raise ConfigurationFailure("configuration_failed")
        start = contents.index(RESOURCE_BEGIN)
        end = contents.index(RESOURCE_END) + len(RESOURCE_END)
        contents = contents[:start] + contents[end:].lstrip("\n")
    # Build options also apply to tests. The legacy memory flag works with both
    # sonic-pins' Bazel 6.4 and the newer image builds; 6.4 lacks local_resources.
    block = "\n".join([
        RESOURCE_BEGIN,
        f"build --jobs={jobs}",
        f"build --local_ram_resources={local_memory_mb}",
        RESOURCE_END,
    ])
    rc.write_text(contents.rstrip("\n") + ("\n\n" if contents.strip() else "") + block + "\n")


def positive_integer(value):
    try:
        result = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError("must be a positive integer") from None
    if result <= 0:
        raise argparse.ArgumentTypeError("must be a positive integer")
    return result


def configure_remote_only(rc, resource_limits=None):
    """Verify the host-selected pool configuration without legacy fallback."""
    try:
        metadata = NATIVE_HELPER.resolve(strict=True).lstat()
        if (not stat.S_ISREG(metadata.st_mode) or metadata.st_uid != 0
                or metadata.st_mode & 0o022
                or hashlib.sha256(NATIVE_HELPER.read_bytes()).hexdigest() != NATIVE_HELPER_SHA256):
            raise ValueError
        spec = importlib.util.spec_from_file_location("native_cache_credentials", NATIVE_HELPER)
        helper = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(helper)
        if helper.host_configuration() is None:
            raise ValueError
        selected = helper.settings()
        if os.environ.get("GITHUB_ACTIONS") == "true":
            # Register masking before any Bazel command or transport error can
            # include the host-selected private endpoint. Credentials stay in
            # the scoped helper and are never read or emitted here.
            for value in (selected["endpoint"], selected["host"]):
                value = value.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
                print("::add-mask::" + value, flush=True)
        required = {
            "remote_cache": selected["endpoint"],
            "credential_helper": selected["host"] + "=" + str(NATIVE_HELPER),
            "remote_executor": "", "disk_cache": "", "repository_cache": "",
        }
        managed = Path(os.environ["SONIC_BAZEL_REMOTE_ONLY_RC"])
        metadata = managed.stat()
        if (not managed.is_absolute() or not stat.S_ISREG(metadata.st_mode)
                or metadata.st_uid != os.geteuid() or metadata.st_mode & 0o077):
            raise ValueError
        # Both files are generated by the worker. Accept their small, known
        # format rather than approximating Bazel's import/command precedence.
        # The only supported home overrides are private startup state and the
        # resource bounds this function can append across repeated calls.
        imports = 0
        for line in rc.read_text().splitlines():
            fields = line.split()
            if not fields or fields[0].startswith("#"):
                continue
            if fields == ["import", str(managed)]:
                imports += 1
            elif len(fields) == 2 and fields[1].partition("=")[0] in {
                "startup": {"--output_user_root", "--host_jvm_args"},
                "build": {"--jobs", "--local_ram_resources"},
            }.get(fields[0], set()) and "=" in fields[1]:
                continue
            else:
                raise ValueError
        if imports != 1:
            raise ValueError

        switches = {"remote_accept_cached", "remote_upload_local_results"}
        required.update(dict.fromkeys(switches, True))
        effective = {}
        for line in managed.read_text().splitlines():
            fields = line.split()
            if not fields or fields[0].startswith("#"):
                continue
            if len(fields) != 2 or fields[0] != "common" or not fields[1].startswith("--"):
                raise ValueError
            name, separator, value = fields[1][2:].partition("=")
            if name in switches:
                value = not separator or value in {"1", "true"}
            elif not separator or name not in {*required, "distdir"}:
                raise ValueError
            if name in effective or (name == "distdir" and value):
                raise ValueError
            effective[name] = value
        if any(effective.get(name) != value for name, value in required.items()):
            raise ValueError
        # Also proves the optional host credential works, without ever emitting
        # its value or accidentally selecting the helper's legacy endpoint.
        helper.probe_access()
        if resource_limits is not None:
            set_resource_limits(rc, **resource_limits)
        os.chmod(rc, 0o600)
    except Exception:
        raise ConfigurationFailure("host_remote_configuration_failed") from None
    return {"status": "configured", "mode": "remote_only", "host_selected": True,
            "disk_cache": False, "repository_cache": False,
            "remote_reads": True, "remote_uploads": True, "resource_limits": resource_limits}


def configure(cache_dir, rc, resource_limits=None):
    try:
        credentials = load_credentials()
    except Exception:
        raise ConfigurationFailure("runtime_helper_unavailable") from None
    try:
        authorization = credentials.authorization()
    except ValueError:
        raise ConfigurationFailure("credentials_invalid") from None
    result = check_remote(credentials, authorization)
    try:
        # The child inherits the step-scoped secret in its environment. Never
        # put it in arguments, configuration, captured logs, or the receipt.
        subprocess.run(
            [sys.executable, str(RUNTIME_ROOT / "cache_probe.py"), "configure",
             "--cache-dir", str(cache_dir), "--rc", str(rc)],
            check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            timeout=30)
        set_local_fallback(rc, result["mode"] == "local_only")
        if resource_limits is not None:
            set_resource_limits(rc, **resource_limits)
    except (OSError, ValueError, subprocess.SubprocessError):
        raise ConfigurationFailure("configuration_failed") from None
    receipt = {"status": "configured", **result}
    if resource_limits is not None:
        receipt["resource_limits"] = resource_limits
    return receipt


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache-dir", type=Path, default=Path("/bazel_cache"))
    parser.add_argument("--remote-only", action="store_true",
                        help="Require the K3s pool's host-selected remote cache; disable local cache reuse")
    parser.add_argument("--rc", type=Path, default=Path.home() / ".bazelrc")
    parser.add_argument("--output", type=Path, default=Path("artifacts/vs/cache-configuration.json"))
    parser.add_argument("--jobs", type=positive_integer,
                        help="Job-scoped Bazel build/test concurrency (requires --local-memory-mb)")
    parser.add_argument("--local-memory-mb", type=positive_integer,
                        help="Job-scoped Bazel local action memory budget (requires --jobs)")
    args = parser.parse_args(argv)
    if (args.jobs is None) != (args.local_memory_mb is None):
        parser.error("--jobs and --local-memory-mb must be supplied together")
    resource_limits = None if args.jobs is None else {
        "jobs": args.jobs, "local_memory_mb": args.local_memory_mb}
    try:
        receipt = (configure_remote_only(args.rc, resource_limits) if args.remote_only
                   else configure(args.cache_dir, args.rc, resource_limits))
    except ConfigurationFailure as error:
        receipt = {"status": "failure", "mode": "unconfigured",
                   "error_class": error.error_class, "http_status": error.http_status}
    except Exception:
        # Exception messages can contain request headers or helper output.
        receipt = {"status": "failure", "mode": "unconfigured",
                   "error_class": "configuration_failed", "http_status": None}
    try:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    except OSError:
        receipt = {"status": "failure", "mode": "unconfigured",
                   "error_class": "receipt_write_failed", "http_status": None}
    print(json.dumps(receipt, sort_keys=True))
    return 0 if receipt["status"] == "configured" else 1


if __name__ == "__main__":
    raise SystemExit(main())
