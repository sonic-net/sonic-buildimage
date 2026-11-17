"""Keep Make's remote-cache credentials private and its cache access read-only."""

import base64
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]
HELPER = ROOT / "tools/bazel/cache/credentials.py"
OPTIONS = ROOT / "tools/bazel/build_helpers.py"
HOST = "lgh-bazel-87263989.eastus.cloudapp.azure.com"
AUTH = "Basic " + base64.b64encode(b"test-client:private-test-password").decode()


def environment(auth):
    result = dict(os.environ)
    result.pop("BAZEL_REMOTE_AUTH", None)
    result.pop("MAKEFLAGS", None)
    result.pop("MFLAGS", None)
    result.pop("SONIC_BAZEL_REMOTE_CACHE", None)
    result.pop("SONIC_BAZEL_REMOTE_UPLOAD", None)
    result.pop("SONIC_BAZEL_KERNEL_REMOTE_CACHE", None)
    result.pop("SONIC_BAZEL_KERNEL_REMOTE_UPLOAD", None)
    if auth is not None:
        result["BAZEL_REMOTE_AUTH"] = auth
    return result


class RemoteCacheTest(unittest.TestCase):
    def options(self, auth, *arguments, **cache_environment):
        return subprocess.run(
            [sys.executable, "-B", str(OPTIONS), "options", *arguments],
            cwd=ROOT, env=environment(auth) | cache_environment, text=True, capture_output=True,
        )

    def assert_no_credentials(self, text):
        self.assertNotIn(AUTH, text)
        self.assertNotIn(AUTH[6:], text)
        self.assertNotIn("private-test-password", text)

    def test_absent_or_empty_auth_preserves_existing_options(self):
        for auth in (None, ""):
            with self.subTest(auth=auth):
                result = self.options(auth, "--arguments=--jobs=3 --disk_cache=")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.splitlines(), ["--jobs=3", "--disk_cache="])

    def test_private_builds_share_the_selected_cache_and_upload_policy(self):
        for policy, expected in (("", "false"), ("n", "false"), ("y", "true")):
            with self.subTest(policy=policy):
                result = self.options(None, SONIC_BAZEL_REMOTE_CACHE="grpc://cache.example:9092",
                                      SONIC_BAZEL_REMOTE_UPLOAD=policy)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.splitlines(), [
                    "--remote_cache=grpc://cache.example:9092",
                    "--remote_upload_local_results=" + expected])

    def test_remote_access_does_not_require_a_local_cache(self):
        result = self.options(AUTH, "--cache-directory=", "--arguments=--disk_cache=")
        self.assertEqual(result.returncode, 0, result.stderr)
        options = result.stdout.splitlines()
        self.assertIn("--remote_cache=https://" + HOST + "/", options)
        self.assertIn("--credential_helper=" + HOST + "=" + str(HELPER), options)
        self.assertIn("--disk_cache=", options)
        self.assertFalse(any(option.startswith("--repository_cache=") for option in options))
        self.assert_no_credentials(result.stdout + result.stderr)

    def test_read_only_policy_is_applied_after_caller_options(self):
        result = self.options(
            AUTH, "--arguments=--remote_upload_local_results=true "
            "--remote_accept_cached=false --remote_cache=https://other.example/ "
            "--remote_executor=grpcs://other.example --jobs=3",
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        effective = {}
        for option in result.stdout.splitlines():
            key, _, value = option.partition("=")
            effective[key] = value
        self.assertEqual(effective["--remote_upload_local_results"], "false")
        self.assertEqual(effective["--remote_accept_cached"], "true")
        self.assertEqual(effective["--remote_cache"], "https://" + HOST + "/")
        self.assertEqual(effective["--remote_executor"], "")
        self.assertEqual(effective["--jobs"], "3")
        self.assert_no_credentials(result.stdout + result.stderr)

    def test_invalid_auth_fails_without_printing_the_value(self):
        for auth in ("Bearer private-test-password", "Basic !!", AUTH + "\n"):
            with self.subTest(kind=auth.split()[0]):
                result = self.options(auth)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, "")
                self.assertEqual(result.stderr, "Bazel remote cache access failed.\n")
                self.assert_no_credentials(result.stderr)

    def helper(self, uri, auth=AUTH):
        return subprocess.run(
            [sys.executable, "-B", str(HELPER), "get"],
            input=json.dumps({"uri": uri}), env=environment(auth),
            text=True, capture_output=True,
        )

    def test_credentials_are_returned_only_to_the_configured_https_origin(self):
        for uri in ("https://" + HOST + "/ac/0123", "https://" + HOST + ":443/cas/0123"):
            with self.subTest(uri=uri):
                result = self.helper(uri)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(json.loads(result.stdout), {"headers": {"Authorization": [AUTH]}})
                self.assertEqual(result.stderr, "")

    def test_other_origins_and_missing_auth_fail_without_disclosing_credentials(self):
        cases = [(uri, AUTH) for uri in (
            "https://other.example/cas/0", "http://" + HOST,
            "https://" + HOST + ".other.example/", "https://" + HOST + ":444/",
            "https://user:private-test-password@" + HOST,
            "https://" + HOST + ":invalid/",
        )] + [("https://" + HOST + "/cas/0", None)]
        for uri, auth in cases:
            with self.subTest(uri=uri):
                result = self.helper(uri, auth)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, "")
                self.assertEqual(result.stderr, "Bazel remote cache access failed.\n")
                self.assert_no_credentials(result.stderr)

    def test_make_passes_auth_by_name_even_when_local_cache_is_disabled(self):
        work = (ROOT / "Makefile.work").read_text()
        start = work.index("# Forward cache authentication by name")
        end = work.index('# User name and tag for "docker-*" images', start)
        with tempfile.TemporaryDirectory() as directory:
            fake_docker = Path(directory) / "docker.py"
            fake_docker.write_text(
                "import json, os, sys\n"
                "print(json.dumps({'args':sys.argv[1:],"
                "'auth_present':bool(os.environ.get('BAZEL_REMOTE_AUTH'))}))\n"
            )
            makefile = (
                "SONIC_BAZEL_CACHE_SOURCE :=\n"
                f"DOCKER_RUN = {sys.executable} {fake_docker}\n"
                + work[start:end]
                + "\ncheck:\n\t$(DOCKER_RUN) builder-image\n"
            )
            for auth in (None, AUTH):
                with self.subTest(auth_present=auth is not None):
                    result = subprocess.run(
                        ["make", "--no-print-directory", "-f", "-", "check"],
                        input=makefile, cwd=ROOT, env=environment(auth),
                        text=True, capture_output=True,
                    )
                    self.assertEqual(result.returncode, 0, result.stderr)
                    record = json.loads(result.stdout.splitlines()[-1])
                    self.assertEqual(record["args"], [
                        "-e", "BAZEL_REMOTE_AUTH",
                        "-e", "SONIC_BAZEL_REMOTE_CACHE=",
                        "-e", "SONIC_BAZEL_REMOTE_UPLOAD=",
                        "-e", "SONIC_BAZEL_KERNEL_REMOTE_CACHE=",
                        "-e", "SONIC_BAZEL_KERNEL_REMOTE_UPLOAD=",
                        "builder-image"])
                    self.assertEqual(record["auth_present"], auth is not None)
                    self.assert_no_credentials(result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
