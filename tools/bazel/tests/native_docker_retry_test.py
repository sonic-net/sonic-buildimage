"""Exercise native Docker retries and real APT index/download failure handling."""

import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import threading
import unittest


ROOT = Path(__file__).resolve().parents[3]
APT_CONFIG = ROOT / "files/apt/apt.conf.d/apt-timeout-n-retries"


class NativeDockerRetryTest(unittest.TestCase):
    def test_host_workflow_uses_default_native_build_policy(self):
        workflow = (ROOT / ".github/workflows/bazel-oci.yml").read_text()
        job = workflow.split("\n  vs_container:\n", 1)[1].split("\n    steps:\n", 1)[0]
        for name in ("DOCKER_BUILD", "SONIC_BUILD_RETRY_COUNT",
                     "SONIC_BUILD_RETRY_INTERVAL", "SONIC_DOCKER_APT_RETRIES"):
            self.assertNotRegex(job, r"(?m)^      " + name + ":")
        self.assertIn("DOCKER_BUILD ?= docker build", (ROOT / "slave.mk").read_text())

    def test_retry_helper_preserves_arguments_and_failure(self):
        settings = {"SONIC_BUILD_RETRY_COUNT": "2", "SONIC_BUILD_RETRY_INTERVAL": "10"}
        retry_count = int(settings["SONIC_BUILD_RETRY_COUNT"])
        interval = int(settings["SONIC_BUILD_RETRY_INTERVAL"])
        self.assertGreater(retry_count, 0)
        self.assertLessEqual(retry_count, 3)
        self.assertGreater(interval, 0)
        self.assertLessEqual(interval, 30)
        command = [str(ROOT / "scripts/run_with_retry"), "docker", "build"]
        arguments = ["--file", "fixture Dockerfile", "context with spaces"]
        for failures, expected_code, expected_calls in (
                (0, 0, 1), (1, 0, 2), (retry_count + 1, 42, retry_count + 1)):
            with self.subTest(failures=failures), tempfile.TemporaryDirectory() as directory:
                work = Path(directory)
                docker = work / "docker"
                docker.write_text("#!" + sys.executable + "\n" + '''
import json, os, pathlib, sys
path = pathlib.Path("calls.json")
calls = json.loads(path.read_text()) if path.exists() else []
calls.append(sys.argv[1:])
path.write_text(json.dumps(calls))
sys.exit(42 if len(calls) <= int(os.environ["FIXTURE_FAILURES"]) else 0)
''')
                docker.chmod(0o755)
                sleep = work / "sleep"
                sleep.write_text('#!/bin/sh\nprintf "%s\\n" "$1" >> sleeps.txt\n')
                sleep.chmod(0o755)
                environment = dict(os.environ, **settings, FIXTURE_FAILURES=str(failures))
                environment["PATH"] = str(work) + os.pathsep + environment["PATH"]
                result = subprocess.run(command + arguments, cwd=work, env=environment,
                                        capture_output=True, text=True, timeout=10)
                self.assertEqual(result.returncode, expected_code, result.stdout + result.stderr)
                calls = json.loads((work / "calls.json").read_text())
                self.assertEqual(calls, [["build", *arguments]] * expected_calls)
                sleeps = work / "sleeps.txt"
                observed = sleeps.read_text().splitlines() if sleeps.exists() else []
                self.assertEqual(observed, [str(interval)] * (expected_calls - 1))


@unittest.skipUnless(shutil.which("apt-get"), "requires the native Debian/Ubuntu APT client")
class AptIndexRefreshTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="apt-index-refresh-")
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)
        for path in ("etc/apt.conf.d", "etc/sources.list.d", "state/lists/partial",
                     "cache/archives/partial", "log"):
            (self.work / path).mkdir(parents=True, exist_ok=True)
        (self.work / "state/status").touch()
        configuration = self.work / "isolated.conf"
        configuration.write_text(
            f'Dir::Etc "{self.work / "etc"}";\n'
            f'Dir::State "{self.work / "state"}";\n'
            f'Dir::State::status "{self.work / "state/status"}";\n'
            f'Dir::Cache "{self.work / "cache"}";\n'
            f'Dir::Log "{self.work / "log"}";\n'
            'Acquire::http::Proxy "DIRECT";\n')
        self.environment = {key: value for key, value in os.environ.items()
                            if not key.lower().endswith("_proxy")}
        self.environment.update(APT_CONFIG=str(configuration), LC_ALL="C")

    def update(self, strict):
        command = [shutil.which("apt-get")]
        if strict:
            command += ["-c", str(APT_CONFIG)]
        command += ["-o", "Acquire::Retries=0", "-o", "Acquire::http::Timeout=1", "update"]
        return subprocess.run(command, env=self.environment, capture_output=True,
                              text=True, timeout=15)

    def test_unreachable_index_cannot_be_a_successful_docker_layer(self):
        # A bound, non-listening loopback socket refuses connections and keeps
        # the port reserved. No external endpoint or host APT state is used.
        with socket.socket() as endpoint:
            endpoint.bind(("127.0.0.1", 0))
            port = endpoint.getsockname()[1]
            (self.work / "etc/sources.list").write_text(
                f"deb [trusted=yes] http://127.0.0.1:{port} fixture main\n")
            baseline = self.update(strict=False)
            self.assertEqual(baseline.returncode, 0, baseline.stdout + baseline.stderr)
            self.assertIn("Failed to fetch", baseline.stderr)
            strict = self.update(strict=True)
            self.assertEqual(strict.returncode, 100, strict.stdout + strict.stderr)
            self.assertIn("Failed to fetch", strict.stderr)

    def test_valid_local_index_still_succeeds(self):
        repository = self.work / "repository"
        distribution = repository / "dists/fixture"
        packages = distribution / "main/binary-amd64/Packages"
        packages.parent.mkdir(parents=True)
        packages.write_bytes(b"")
        (distribution / "Release").write_text(
            "Suite: fixture\nCodename: fixture\nArchitectures: amd64\nComponents: main\n"
            "SHA256:\n"
            " e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 0 main/binary-amd64/Packages\n")
        (self.work / "etc/sources.list").write_text(
            f"deb [trusted=yes arch=amd64] {repository.as_uri()} fixture main\n")
        result = self.update(strict=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("Failed to fetch", result.stderr)

    def download_fixture(self, failures, failure_status=503, native_docker=False):
        if not shutil.which("dpkg-deb"):
            self.skipTest("requires dpkg-deb to read the existing fixture metadata")
        fixture = ROOT / "tools/bazel/tests/fixtures/oci-debs/example-gz_1.0-1_all.deb"
        payload = fixture.read_bytes()
        control = subprocess.check_output(
            ["dpkg-deb", "--field", str(fixture)], text=True, timeout=10)
        index = (control.rstrip() + f"\nFilename: pool/{fixture.name}\n"
                 f"Size: {len(payload)}\nSHA256: {hashlib.sha256(payload).hexdigest()}\n\n").encode()
        release = ("Suite: fixture\nCodename: fixture\nSHA256:\n "
                   f"{hashlib.sha256(index).hexdigest()} {len(index)} Packages\n").encode()
        endpoints = {"/Release": release, "/Packages": index}
        requests = []

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *unused):
                pass

            def do_GET(self):
                # Fixed in-memory endpoints only; never serve filesystem paths.
                path = str(PurePosixPath(self.path))
                if path == "/pool/" + fixture.name:
                    status = failure_status if len(requests) < failures else 200
                    requests.append(status)
                    body = payload if status == 200 else b"controlled fixture failure\n"
                else:
                    status = 200 if path in endpoints else 404
                    body = endpoints.get(path, b"fixture endpoint unavailable\n")
                self.send_response(status)
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        thread = threading.Thread(target=server.serve_forever,
                                  kwargs={"poll_interval": 0.05}, daemon=True)
        thread.start()
        try:
            (self.work / "etc/sources.list").write_text(
                f"deb [trusted=yes] http://127.0.0.1:{server.server_port} ./\n")
            update = self.update(strict=True)
            self.assertEqual(update.returncode, 0, update.stdout + update.stderr)
            if native_docker:
                result = self.download_with_native_docker_retry()
            else:
                # Exercise the runtime image's acquisition retry count. Disable
                # delays only in this isolated fixture to keep the test short.
                result = subprocess.run(
                    [shutil.which("apt-get"), "-c", str(APT_CONFIG),
                     "-o", "Acquire::Retries::Delay=false", "download", "example-gz=1.0-1"],
                    cwd=self.work, env=self.environment, capture_output=True, text=True, timeout=15)
            return result, requests, list(self.work.glob("*.deb")), payload
        finally:
            server.shutdown()
            server.server_close()
            thread.join(timeout=5)
            self.assertFalse(thread.is_alive(), "fixture server must stop")

    def download_with_native_docker_retry(self):
        """Exercise the opt-in APT retry mitigation without starting Docker."""
        settings = {"SONIC_BUILD_RETRY_COUNT": "2", "SONIC_BUILD_RETRY_INTERVAL": "10"}
        command = [str(ROOT / "scripts/run_with_retry"), "docker", "build"]
        # Match normal APT configuration loading: the build-only file overrides
        # the runtime policy in apt.conf.d. A late -c argument would reverse it.
        configuration = self.work / "etc/apt.conf.d"
        shutil.copyfile(APT_CONFIG, configuration / APT_CONFIG.name)
        (configuration / "zzzz-sonic-ci-acquire-retries").write_text(
            '// Temporary SONiC native Docker CI override; Debian bug #1117797.\n'
            'Acquire::Retries "0";\n')
        docker = self.work / "docker"
        docker.write_text("#!" + sys.executable + "\n" + '''
import json, pathlib, subprocess, sys
path = pathlib.Path("docker-calls.json")
calls = json.loads(path.read_text()) if path.exists() else []
calls.append(sys.argv[1:])
path.write_text(json.dumps(calls))
sys.exit(subprocess.run([APT_GET, "download", "example-gz=1.0-1"]).returncode)
'''.replace("APT_GET", repr(shutil.which("apt-get"))))
        docker.chmod(0o755)
        sleep = self.work / "sleep"
        sleep.write_text('#!/bin/sh\nprintf "%s\\n" "$1" >> sleeps.txt\n')
        sleep.chmod(0o755)
        environment = dict(self.environment, **settings)
        environment["PATH"] = str(self.work) + os.pathsep + environment["PATH"]
        arguments = command + ["fixture-context"]
        process = subprocess.Popen(arguments, cwd=self.work, env=environment,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   text=True, start_new_session=True)
        try:
            stdout, stderr = process.communicate(timeout=15)
        except subprocess.TimeoutExpired:
            # A regression must not leave the wrapper's APT descendants behind.
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            stdout, stderr = process.communicate(timeout=5)
            self.fail("native Docker APT retry fixture timed out: " + stdout + stderr)
        result = subprocess.CompletedProcess(arguments, process.returncode, stdout, stderr)
        self.docker_calls = json.loads((self.work / "docker-calls.json").read_text())
        sleeps = self.work / "sleeps.txt"
        self.docker_sleeps = sleeps.read_text().splitlines() if sleeps.exists() else []
        return result

    def test_native_docker_retries_after_one_failed_package_attempt(self):
        result, requests, downloads, payload = self.download_fixture(
            failures=1, native_docker=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(requests, [503, 200])
        self.assertEqual(self.docker_calls, [["build", "fixture-context"]] * 2)
        self.assertEqual(self.docker_sleeps, ["10"])
        self.assertEqual(len(downloads), 1)
        self.assertEqual(hashlib.sha256(downloads[0].read_bytes()).hexdigest(),
                         hashlib.sha256(payload).hexdigest())

    def test_native_docker_package_failure_exhausts_three_whole_build_attempts(self):
        result, requests, downloads, _ = self.download_fixture(
            failures=100, native_docker=True)
        self.assertEqual(result.returncode, 100, result.stdout + result.stderr)
        self.assertEqual(requests, [503] * 3)
        self.assertEqual(self.docker_calls, [["build", "fixture-context"]] * 3)
        self.assertEqual(self.docker_sleeps, ["10", "10"])
        self.assertEqual(downloads, [])

    def test_package_download_recovers_after_four_transient_failures(self):
        result, requests, downloads, payload = self.download_fixture(failures=4)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(requests, [503] * 4 + [200])
        self.assertEqual(len(downloads), 1)
        self.assertEqual(hashlib.sha256(downloads[0].read_bytes()).hexdigest(),
                         hashlib.sha256(payload).hexdigest())

    def test_package_download_exhaustion_is_bounded_and_fails(self):
        result, requests, downloads, _ = self.download_fixture(failures=100)
        self.assertEqual(result.returncode, 100, result.stdout + result.stderr)
        self.assertEqual(requests, [503] * 9)  # Initial attempt plus eight retries.
        self.assertEqual(downloads, [])

    def test_package_download_permanent_forbidden_is_not_retried(self):
        result, requests, downloads, _ = self.download_fixture(failures=100, failure_status=403)
        self.assertEqual(result.returncode, 100, result.stdout + result.stderr)
        self.assertEqual(requests, [403])
        self.assertEqual(downloads, [])


if __name__ == "__main__":
    unittest.main()
