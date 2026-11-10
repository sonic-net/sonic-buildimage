import hashlib
import os
import shutil
import subprocess
import tempfile
import threading
import unittest
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

from generate_downloader_config import generate_config


class ConfigTests(unittest.TestCase):
    def test_rejects_invalid_pin(self):
        with self.assertRaisesRegex(ValueError, "line 1"):
            generate_config(
                "https://example.com/archive==not-a-checksum",
                "https://mirror.example.com",
            )

    def test_rejects_invalid_mirror(self):
        with self.assertRaises(ValueError):
            generate_config("", "https://mirror.example.com/path?query=value")

    def test_directory_metadata_url_uses_existing_mirror_naming(self):
        config = generate_config(
            "https://go.dev/dl/?mode=json==" + "a" * 32,
            "https://mirror.example.com",
        )
        self.assertIn("https://mirror.example.com/-" + "a" * 32, config)


class DownloadTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bazel = os.environ.get("BAZEL") or shutil.which("bazel")
        if not cls.bazel:
            raise RuntimeError(
                "Bazel is required to verify mirror and cache downloads"
            )

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(
            prefix="sonic-bazel-mirror-"
        )
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.requests = []
        self.responses = {}
        owner = self

        class Handler(BaseHTTPRequestHandler):
            def do_GET(self):
                owner.requests.append(self.path)
                status, body = owner.responses.get(
                    self.path, (404, b"not found")
                )
                self.send_response(status)
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

            def log_message(self, *_):
                pass

        self.server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        self.thread = threading.Thread(target=self.server.serve_forever)
        self.thread.start()
        self.addCleanup(self.stop_server)
        base = "http://127.0.0.1:{}".format(self.server.server_port)
        self.payload = b"SONiC Bazel mirror regression test\n"
        self.sha256 = hashlib.sha256(self.payload).hexdigest()
        checksum = hashlib.md5(self.payload).hexdigest()
        self.upstream_path = "/upstream/payload?source=a.b"
        self.mirror_path = "/mirror/payload-" + checksum
        url = base + self.upstream_path
        self.workspace = self.root / "workspace"
        self.workspace.mkdir()
        (self.workspace / "BUILD.bazel").touch()
        (self.workspace / "download.bzl").write_text(
            'def _download(ctx):\n'
            '    ctx.download(url=ctx.attr.url, output="payload",\n'
            '                 sha256=ctx.attr.sha256)\n'
            '    ctx.file("BUILD.bazel", \'exports_files(["payload"])\\n\')\n'
            'download = repository_rule(implementation=_download, attrs={\n'
            '    "url": attr.string(), "sha256": attr.string(),\n'
            '})\n'
        )
        (self.workspace / "MODULE.bazel").write_text(
            'download = use_repo_rule("//:download.bzl", "download")\n'
            'download(name="downloaded", url="{}", sha256="{}")\n'.format(
                url, self.sha256
            )
        )
        self.config = self.root / "downloader.config"
        self.config.write_text(
            generate_config(url + "==" + checksum, base + "/mirror")
        )
        tools = Path(__file__).resolve().parent
        self.cache_rc = self.root / "cache.bazelrc"
        self.cache_rc.write_text(
            (tools / "cache.bazelrc").read_text().replace(
                "/bazel_cache", str(self.root / "cache")
            )
        )
        self.rc = self.root / "bazelrc"
        self.rc.write_text(
            (tools / "slave.bazelrc").read_text()
            .replace(
                "/usr/local/share/buildinfo/bazel-downloader.config",
                str(self.config),
            )
            .replace("/etc/bazel-cache.bazelrc", str(self.cache_rc))
        )
        self.run_number = 0

    def stop_server(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()

    def fetch(self):
        self.run_number += 1
        env = dict(os.environ, USE_BAZEL_VERSION="8.4.1")
        result = subprocess.run(
            [
                self.bazel, "--batch", "--nosystem_rc", "--noworkspace_rc",
                "--nohome_rc",
                "--output_user_root=" + str(self.root / "bazel"),
                "--output_base=" + str(
                    self.root / ("output-" + str(self.run_number))
                ),
                "--bazelrc=" + str(self.rc), "query", "@downloaded//:payload",
                "--lockfile_mode=off",
                "--experimental_repository_downloader_retries=0",
                "--noshow_progress",
            ],
            cwd=self.workspace, env=env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=120,
        )
        return result

    def test_cold_mirror_and_warm_repository_cache(self):
        self.responses[self.mirror_path] = (200, self.payload)
        result = self.fetch()
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn(self.mirror_path, self.requests)
        self.assertNotIn(self.upstream_path, self.requests)
        print(
            "Bazel provenance: cold fetch requested mirror; "
            "upstream requests=0",
            flush=True,
        )

        requests_before = list(self.requests)
        self.responses.clear()
        result = self.fetch()
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.requests, requests_before, result.stdout)
        print(
            "Bazel provenance: fresh output base reused repository cache; "
            "HTTP requests=0",
            flush=True,
        )

    def test_unpublished_mirror_falls_back_to_upstream(self):
        self.cache_rc.unlink()
        self.responses[self.upstream_path] = (200, self.payload)
        result = self.fetch()
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn(self.mirror_path, self.requests)
        self.assertIn(self.upstream_path, self.requests)
        print(
            "Bazel provenance: missing mirror exercised upstream fallback",
            flush=True,
        )

    def test_corrupt_mirror_is_rejected(self):
        self.responses[self.mirror_path] = (200, b"corrupt")
        result = self.fetch()
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn(self.mirror_path, self.requests)
        self.assertIn("Checksum", result.stdout)


if __name__ == "__main__":
    unittest.main()
