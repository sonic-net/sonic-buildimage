import hashlib
import importlib.util
import io
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import threading
import unittest
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from unittest.mock import Mock, call, patch

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


class PublisherTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        script = (
            Path(__file__).resolve().parents[2]
            / "scripts" / "populate_file_web_server.py"
        )
        spec = importlib.util.spec_from_file_location(
            "mirror_publisher", script
        )
        cls.publisher = importlib.util.module_from_spec(spec)
        with patch.dict(sys.modules, {"requests": Mock()}):
            spec.loader.exec_module(cls.publisher)

    def test_publishes_legacy_archive_and_bazel_alias(self):
        resource = self.publisher.Resource(
            "https://example.com/archive.tar.gz==" + "a" * 32, "versions-web"
        )
        legacy = resource.get_unique_name()
        with patch.object(
            self.publisher, "upload_resource_to_server", return_value=0
        ) as upload:
            result = self.publisher.publish_resource(
                "cached-file", resource, "user", "key", "https://mirror"
            )
        self.assertEqual(result, 0)
        self.assertEqual(upload.call_args_list, [
            call("cached-file", name, "user", "key", "https://mirror",
                 checksum=resource.hash)
            for name in (legacy, legacy + ".tar.gz")
        ])

    def test_alias_publication_failure_is_reported(self):
        resource = self.publisher.Resource(
            "https://example.com/archive.tar.gz==" + "a" * 32, "versions-web"
        )
        failure = self.publisher.RET_CODE_HTTP_SERVER_ERROR
        with patch.object(
            self.publisher, "upload_resource_to_server",
            side_effect=[0, failure],
        ):
            result = self.publisher.publish_resource(
                "cached-file", resource, "user", "key", "https://mirror"
            )
        self.assertEqual(result, failure)

    def test_alias_checksum_is_not_parsed_from_archive_extension(self):
        response = Mock(status_code=201)
        response.json.return_value = {"checksums": {"md5": "a" * 32}}
        with tempfile.TemporaryDirectory() as temporary:
            payload = Path(temporary) / "payload"
            payload.write_bytes(b"archive")
            with patch.object(self.publisher.requests, "put",
                              return_value=response), patch.object(
                                  self.publisher, "print_msg"
                              ) as log:
                result = self.publisher.upload_resource_to_server(
                    str(payload), "archive-" + "a" * 32 + ".tar.gz",
                    "user", "key", "https://mirror", checksum="a" * 32,
                )
        self.assertEqual(result, 0)
        log.assert_not_called()


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
        self.configure_download(base, checksum)
        self.run_number = 0

    def configure_download(self, base, checksum, archive=False):
        url = base + self.upstream_path
        self.workspace = self.root / "workspace"
        self.workspace.mkdir(exist_ok=True)
        (self.workspace / "BUILD.bazel").touch()
        download_call = (
            '    ctx.download_and_extract(url=ctx.attr.url,\n'
            if archive else
            '    ctx.download(url=ctx.attr.url, output="payload",\n'
        )
        (self.workspace / "download.bzl").write_text(
            'def _download(ctx):\n'
            + download_call
            + '                 sha256=ctx.attr.sha256)\n'
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
        self.downloader_rc = self.root / "downloader.bazelrc"
        self.downloader_rc.write_text(
            "common --experimental_downloader_config={}\n".format(self.config)
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
                "/usr/local/share/buildinfo/bazel-downloader.bazelrc",
                str(self.downloader_rc),
            )
            .replace("/etc/bazel-cache.bazelrc", str(self.cache_rc))
        )

    def configure_archive(self):
        body = io.BytesIO()
        with tarfile.open(fileobj=body, mode="w:gz") as archive:
            info = tarfile.TarInfo("payload")
            info.size = len(self.payload)
            archive.addfile(info, io.BytesIO(self.payload))
        self.payload = body.getvalue()
        self.sha256 = hashlib.sha256(self.payload).hexdigest()
        checksum = hashlib.md5(self.payload).hexdigest()
        self.upstream_path = "/upstream/payload.tar.gz?source=a.b"
        self.mirror_path = "/mirror/payload.tar.gz-" + checksum + ".tar.gz"
        base = "http://127.0.0.1:{}".format(self.server.server_port)
        self.configure_download(base, checksum, archive=True)

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

    def test_bazel_6_accepts_slave_configuration(self):
        (self.workspace / "WORKSPACE.bazel").touch()
        result = subprocess.run(
            [
                os.environ.get("BAZEL_LEGACY") or self.bazel,
                "--batch", "--nosystem_rc", "--noworkspace_rc", "--nohome_rc",
                "--output_user_root=" + str(self.root / "legacy-bazel"),
                "--bazelrc=" + str(self.rc), "info", "release",
            ],
            cwd=self.workspace,
            env=dict(os.environ, USE_BAZEL_VERSION="6.4.0"),
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            timeout=120,
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("release 6.4.0", result.stdout)

    def test_cold_mirror_and_warm_repository_cache(self):
        self.check_cold_mirror_and_warm_repository_cache()

    def test_archive_cold_mirror_and_warm_repository_cache(self):
        self.configure_archive()
        self.responses[self.mirror_path.removesuffix(".tar.gz")] = (
            200, self.payload
        )
        self.check_cold_mirror_and_warm_repository_cache()

    def check_cold_mirror_and_warm_repository_cache(self):
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
        self.check_unpublished_mirror_falls_back_to_upstream()

    def test_unpublished_archive_mirror_falls_back_to_upstream(self):
        self.configure_archive()
        self.check_unpublished_mirror_falls_back_to_upstream()

    def check_unpublished_mirror_falls_back_to_upstream(self):
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

    def test_uninitialized_slave_keeps_native_upstream_download(self):
        self.downloader_rc.unlink()
        self.config.unlink()
        self.responses[self.upstream_path] = (200, self.payload)
        result = self.fetch()
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn(self.upstream_path, self.requests)
        self.assertNotIn(self.mirror_path, self.requests)


if __name__ == "__main__":
    unittest.main()
