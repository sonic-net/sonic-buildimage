#!/usr/bin/env python3
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Unit tests for mellanox_bfb_installer.dump_receiver.

The HTTP-level tests bind a real ``_ReuseAddrThreadingHTTPServer`` to
``127.0.0.1:0`` (ephemeral port) so we exercise the actual request handler
and ``http.server``/``socketserver`` plumbing, not a mock of them. Each test
gets a fresh temp output directory so file IO is isolated and cleaned up.
"""

import http.client
import os
import shutil
import socket
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))


# ---------------------------------------------------------------------------
# Pure-function tests for _parse_target_path
# ---------------------------------------------------------------------------


class TestParseTargetPath(unittest.TestCase):
    """Validates the URL parser that the request handler uses to figure out
    where on disk an upload should go.

    The handler relies on this function to reject anything that isn't a
    canonical ``/<dpuN>/<safe-basename>`` URL before any FS I/O happens, so
    it's important the rejection rules are tight.
    """

    def test_valid_simple_path(self):
        from mellanox_bfb_installer import dump_receiver

        self.assertEqual(
            dump_receiver._parse_target_path("/dpu0/x.tar.gz"),
            ("dpu0", "x.tar.gz"),
        )

    def test_valid_higher_dpu_id_and_complex_basename(self):
        from mellanox_bfb_installer import dump_receiver

        self.assertEqual(
            dump_receiver._parse_target_path("/dpu12/install-recovery-dpu12-20260516T231500Z.tar.gz"),
            ("dpu12", "install-recovery-dpu12-20260516T231500Z.tar.gz"),
        )

    def test_strips_query_string(self):
        """``?foo=1`` must not appear in the saved filename."""
        from mellanox_bfb_installer import dump_receiver

        self.assertEqual(
            dump_receiver._parse_target_path("/dpu0/x.bin?foo=1&bar=2"),
            ("dpu0", "x.bin"),
        )

    def test_strips_fragment(self):
        """Fragments are client-side only but we still defend against them."""
        from mellanox_bfb_installer import dump_receiver

        self.assertEqual(
            dump_receiver._parse_target_path("/dpu0/x.bin#frag"),
            ("dpu0", "x.bin"),
        )

    def test_must_start_with_slash(self):
        from mellanox_bfb_installer import dump_receiver

        with self.assertRaises(ValueError):
            dump_receiver._parse_target_path("dpu0/x.tar.gz")

    def test_wrong_segment_count_rejected(self):
        """We require exactly two non-empty segments: dpu name and filename."""
        from mellanox_bfb_installer import dump_receiver

        # 0 segments / 1 segment
        for bad in ["/", "/dpu0"]:
            with self.assertRaises(ValueError, msg=f"should reject {bad!r}"):
                dump_receiver._parse_target_path(bad)
        # 3+ segments (includes path-traversal attempts after we URL-decode)
        for bad in ["/dpu0/sub/file", "/a/b/c/d", "/dpu0/../sneaky"]:
            with self.assertRaises(ValueError, msg=f"should reject {bad!r}"):
                dump_receiver._parse_target_path(bad)

    def test_empty_filename_rejected(self):
        from mellanox_bfb_installer import dump_receiver

        with self.assertRaises(ValueError):
            dump_receiver._parse_target_path("/dpu0/")

    def test_invalid_dpu_names_rejected(self):
        """Anything not matching ``dpu<digits>``."""
        from mellanox_bfb_installer import dump_receiver

        for bad in ["DPU0", "rshim0", "dpu", "dpux", "dpu0a", "dpu-0", "0dpu"]:
            with self.assertRaises(ValueError, msg=f"should reject {bad!r}"):
                dump_receiver._parse_target_path(f"/{bad}/x.tar.gz")

    def test_invalid_filenames_rejected(self):
        """Filenames with disallowed chars or that start with a dot."""
        from mellanox_bfb_installer import dump_receiver

        for bad in [".hidden", "with spaces", "weird!", "%2F", "tab\there", "back\\slash"]:
            with self.assertRaises(ValueError, msg=f"should reject {bad!r}"):
                dump_receiver._parse_target_path(f"/dpu0/{bad}")

    def test_filename_length_cap(self):
        """The regex caps filenames at 255 chars (1 leading + 254 trailing)."""
        from mellanox_bfb_installer import dump_receiver

        ok_name = "a" + "b" * 254
        self.assertEqual(
            dump_receiver._parse_target_path(f"/dpu0/{ok_name}"),
            ("dpu0", ok_name),
        )
        too_long = "a" + "b" * 255
        with self.assertRaises(ValueError):
            dump_receiver._parse_target_path(f"/dpu0/{too_long}")


# ---------------------------------------------------------------------------
# _build_server / _ReuseAddrThreadingHTTPServer
# ---------------------------------------------------------------------------


class TestBuildServer(unittest.TestCase):
    """Tests for the factory that constructs the receiver's HTTP server."""

    def test_creates_missing_output_dir(self):
        """_build_server should mkdir -p the configured output directory."""
        from mellanox_bfb_installer import dump_receiver

        with tempfile.TemporaryDirectory(prefix="dr_build_") as base:
            output_dir = os.path.join(base, "new", "nested", "dir")
            server = dump_receiver._build_server(
                bind_ip="127.0.0.1", port=0, output_dir=output_dir, max_bytes=1024
            )
            try:
                self.assertTrue(os.path.isdir(output_dir))
            finally:
                server.server_close()

    def test_server_uses_ipv4_and_reuse(self):
        """Server class binds IPv4 with SO_REUSEADDR for predictable rebinds."""
        from mellanox_bfb_installer import dump_receiver

        with tempfile.TemporaryDirectory(prefix="dr_attrs_") as out:
            server = dump_receiver._build_server(
                bind_ip="127.0.0.1", port=0, output_dir=out, max_bytes=1024
            )
            try:
                self.assertEqual(server.address_family, socket.AF_INET)
                self.assertTrue(server.allow_reuse_address)
            finally:
                server.server_close()

    def test_handler_class_has_attached_config(self):
        """The dynamically-built handler class carries a per-server config."""
        from mellanox_bfb_installer import dump_receiver

        with tempfile.TemporaryDirectory(prefix="dr_cfg_") as out:
            server = dump_receiver._build_server(
                bind_ip="127.0.0.1", port=0, output_dir=out, max_bytes=4321
            )
            try:
                handler_cls = server.RequestHandlerClass
                self.assertIsNotNone(handler_cls.receiver_config)
                self.assertEqual(handler_cls.receiver_config.output_dir, out)
                self.assertEqual(handler_cls.receiver_config.max_bytes, 4321)
            finally:
                server.server_close()


# ---------------------------------------------------------------------------
# HTTP-level integration tests against a real in-process server
# ---------------------------------------------------------------------------


class _ReceiverServerFixture(unittest.TestCase):
    """Common setUp/tearDown that brings up a real receiver on 127.0.0.1.

    Subclasses can override ``max_bytes`` via the class attribute to test the
    upload-size cap branch.
    """

    max_bytes = 1024 * 1024  # 1 MiB default
    SOCKET_TIMEOUT_SEC = 5.0

    def setUp(self):
        from mellanox_bfb_installer import dump_receiver

        self.tmpdir = tempfile.mkdtemp(prefix="dump_receiver_test_")
        self.server = dump_receiver._build_server(
            bind_ip="127.0.0.1", port=0, output_dir=self.tmpdir, max_bytes=self.max_bytes
        )
        self.host, self.port = self.server.server_address[0], self.server.server_address[1]
        self.thread = threading.Thread(
            target=self.server.serve_forever, kwargs={"poll_interval": 0.05}, daemon=True
        )
        self.thread.start()

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=5)
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    # -- helpers --

    def _conn(self):
        """Return a fresh HTTPConnection. The receiver advertises HTTP/1.0
        with Connection: close, so each request uses a fresh socket."""
        return http.client.HTTPConnection(self.host, self.port, timeout=self.SOCKET_TIMEOUT_SEC)

    def _raw_request(self, raw_bytes: bytes) -> bytes:
        """Send a hand-crafted HTTP request and return the full response.

        Needed for cases ``http.client`` won't let us produce, like a PUT
        without Content-Length or a Content-Length that mismatches the body.
        """
        with socket.create_connection((self.host, self.port), timeout=self.SOCKET_TIMEOUT_SEC) as s:
            s.sendall(raw_bytes)
            # The handler sets Connection: close, so server closes after replying.
            chunks = []
            while True:
                try:
                    chunk = s.recv(4096)
                except socket.timeout:
                    break
                if not chunk:
                    break
                chunks.append(chunk)
            return b"".join(chunks)


class TestDumpReceiverHttpHappyPath(_ReceiverServerFixture):
    """201 paths: PUT / POST / GET liveness."""

    def test_put_writes_file_under_output_dir(self):
        body = b"hello dpu world"
        conn = self._conn()
        conn.request(
            "PUT",
            "/dpu0/upload.tar.gz",
            body=body,
            headers={"Content-Length": str(len(body))},
        )
        resp = conn.getresponse()
        resp_body = resp.read()
        conn.close()
        self.assertEqual(resp.status, 201, resp_body)
        dest = os.path.join(self.tmpdir, "dpu0", "upload.tar.gz")
        self.assertTrue(os.path.isfile(dest), f"missing dest: {dest}")
        with open(dest, "rb") as f:
            self.assertEqual(f.read(), body)
        # The .part file must be gone after atomic rename.
        self.assertFalse(os.path.exists(dest + ".part"))

    def test_post_writes_file(self):
        """POST and PUT share the upload handler."""
        body = b"posted-bytes"
        conn = self._conn()
        conn.request("POST", "/dpu1/posted.bin", body=body)
        resp = conn.getresponse()
        conn.close()
        self.assertEqual(resp.status, 201)
        dest = os.path.join(self.tmpdir, "dpu1", "posted.bin")
        self.assertTrue(os.path.isfile(dest))

    def test_get_root_returns_liveness_ok(self):
        conn = self._conn()
        conn.request("GET", "/")
        resp = conn.getresponse()
        body = resp.read()
        conn.close()
        self.assertEqual(resp.status, 200)
        self.assertIn(b"ok", body)

    def test_get_non_root_returns_404(self):
        """GET to anything other than ``/`` must 404, including valid upload paths."""
        conn = self._conn()
        conn.request("GET", "/dpu0/x.tar.gz")
        resp = conn.getresponse()
        conn.close()
        self.assertEqual(resp.status, 404)

    def test_response_includes_connection_close(self):
        """HTTP/1.0 + Connection: close so curl with --retry can re-establish cleanly."""
        conn = self._conn()
        conn.request("PUT", "/dpu0/one.bin", body=b"x")
        resp = conn.getresponse()
        resp.read()
        conn.close()
        self.assertEqual(resp.getheader("Connection"), "close")


class TestDumpReceiverHttpRejections(_ReceiverServerFixture):
    """400 / 411 / 413 / 500 paths."""

    def test_invalid_dpu_name_returns_400(self):
        conn = self._conn()
        conn.request("PUT", "/notadpu/x.bin", body=b"x")
        resp = conn.getresponse()
        body = resp.read()
        conn.close()
        self.assertEqual(resp.status, 400)
        self.assertIn(b"bad target path", body)

    def test_invalid_filename_dot_hidden_returns_400(self):
        """Filename starting with a dot is rejected (no creation of hidden files)."""
        conn = self._conn()
        conn.request("PUT", "/dpu0/.hidden", body=b"x")
        resp = conn.getresponse()
        body = resp.read()
        conn.close()
        self.assertEqual(resp.status, 400, body)
        self.assertFalse(
            os.path.exists(os.path.join(self.tmpdir, "dpu0", ".hidden")),
            "bad filename must not have created a file on disk",
        )

    def test_invalid_filename_with_space_returns_400(self):
        """Filenames with spaces (or any byte outside the allowlist) are
        rejected by the server. We send via raw socket because http.client
        refuses spaces in the URL client-side."""
        raw = (
            b"PUT /dpu0/with%20space HTTP/1.0\r\n"
            b"Content-Length: 1\r\n"
            b"\r\n"
            b"x"
        )
        # We intentionally URL-encode the space because http.client and many
        # other clients would reject the raw form. The receiver doesn't decode
        # percent-encoding, so the regex still rejects ``%`` in the basename.
        resp = self._raw_request(raw)
        first_line = resp.split(b"\r\n", 1)[0]
        self.assertIn(b" 400 ", first_line, resp)

    def test_missing_content_length_returns_411(self):
        """Without Content-Length the handler refuses to read a body."""
        raw = (
            b"PUT /dpu0/x.bin HTTP/1.0\r\n"
            b"Host: localhost\r\n"
            b"\r\n"
        )
        resp = self._raw_request(raw)
        # Status line is the first line of the response.
        first_line = resp.split(b"\r\n", 1)[0]
        self.assertIn(b" 411 ", first_line, resp)

    def test_invalid_content_length_returns_400(self):
        raw = (
            b"PUT /dpu0/x.bin HTTP/1.0\r\n"
            b"Content-Length: notanint\r\n"
            b"\r\n"
        )
        resp = self._raw_request(raw)
        first_line = resp.split(b"\r\n", 1)[0]
        self.assertIn(b" 400 ", first_line, resp)

    def test_negative_content_length_returns_400(self):
        """``http.client`` won't let us produce a negative CL, so use raw bytes."""
        raw = (
            b"PUT /dpu0/x.bin HTTP/1.0\r\n"
            b"Content-Length: -5\r\n"
            b"\r\n"
        )
        resp = self._raw_request(raw)
        first_line = resp.split(b"\r\n", 1)[0]
        self.assertIn(b" 400 ", first_line, resp)

    def test_mkdir_failure_returns_500(self):
        """If os.makedirs for ``<output_dir>/<dpuN>`` fails, the handler must
        return HTTP 500 with a clear ``mkdir failed`` body and write no file.
        The base ``output_dir`` itself was already created in _build_server,
        so we only patch makedirs for the duration of the request."""
        from mellanox_bfb_installer import dump_receiver

        with mock.patch.object(
            dump_receiver.os, "makedirs", side_effect=OSError("EACCES")
        ):
            conn = self._conn()
            conn.request("PUT", "/dpu0/x.bin", body=b"x")
            resp = conn.getresponse()
            body = resp.read()
            conn.close()

        self.assertEqual(resp.status, 500, body)
        self.assertIn(b"mkdir failed", body)
        # And: the file definitely didn't make it to disk.
        self.assertFalse(os.path.exists(os.path.join(self.tmpdir, "dpu0", "x.bin")))

    def test_short_body_returns_500_and_cleans_part_file(self):
        """If the client sends fewer bytes than Content-Length says, the
        handler must respond 500 and remove the partial .part file so the
        next retry from the DPU starts clean."""
        # Send Content-Length: 100 but only 5 bytes of body, then close.
        raw = (
            b"PUT /dpu8/short.bin HTTP/1.0\r\n"
            b"Content-Length: 100\r\n"
            b"\r\n"
            b"short"
        )
        resp = self._raw_request(raw)
        # We may or may not get a response back; both behaviours mean the
        # save failed. Crucially the .part file must not be left around.
        if resp:
            first_line = resp.split(b"\r\n", 1)[0]
            self.assertIn(b" 500 ", first_line, resp)
        # Give the handler a moment to unlink the .part on the io error path.
        for _ in range(20):
            if not os.path.exists(os.path.join(self.tmpdir, "dpu8", "short.bin.part")):
                break
            time.sleep(0.05)
        self.assertFalse(
            os.path.exists(os.path.join(self.tmpdir, "dpu8", "short.bin.part")),
            "stale .part file left behind on short-body upload",
        )
        # The final file must not exist either.
        self.assertFalse(os.path.exists(os.path.join(self.tmpdir, "dpu8", "short.bin")))


class TestDumpReceiverHttpSizeCap(_ReceiverServerFixture):
    """413 path: enforce ``max_bytes`` cap before reading the body."""

    max_bytes = 16  # tiny cap to make the test obvious

    def test_oversize_content_length_returns_413(self):
        body = b"x" * 64
        conn = self._conn()
        conn.request("PUT", "/dpu0/too-big.bin", body=body)
        resp = conn.getresponse()
        msg = resp.read()
        conn.close()
        self.assertEqual(resp.status, 413, msg)
        self.assertIn(b"too large", msg)
        # Nothing must have been written.
        self.assertFalse(os.path.exists(os.path.join(self.tmpdir, "dpu0", "too-big.bin")))
        self.assertFalse(os.path.exists(os.path.join(self.tmpdir, "dpu0", "too-big.bin.part")))


# ---------------------------------------------------------------------------
# main() CLI entrypoint smoke tests
# ---------------------------------------------------------------------------


class TestDumpReceiverMain(unittest.TestCase):
    """Exercise the ``main()`` CLI wrapper. We mock the server so the test
    doesn't bind a real socket or block in serve_forever."""

    def _patch_argv(self, args):
        return mock.patch.object(sys, "argv", ["dump_receiver"] + list(args))

    def test_main_returns_zero_when_server_runs_and_stops(self):
        from mellanox_bfb_installer import dump_receiver

        fake_server = mock.MagicMock()
        # serve_forever returns immediately as if a signal was received.
        fake_server.serve_forever.return_value = None

        with tempfile.TemporaryDirectory(prefix="dr_main_") as out, \
             self._patch_argv([
                 "--bind-ip", "127.0.0.1",
                 "--port", "0",
                 "--output-dir", out,
                 "--max-bytes", "1024",
             ]), \
             mock.patch.object(dump_receiver, "_build_server", return_value=fake_server), \
             mock.patch.object(dump_receiver.signal, "signal"):
            rc = dump_receiver.main()

        self.assertEqual(rc, 0)
        fake_server.serve_forever.assert_called_once()
        fake_server.server_close.assert_called_once()

    def test_main_returns_one_when_bind_fails(self):
        """``_build_server`` may raise OSError if the bind IP is unreachable
        or the port is already in use; main() must surface this as rc=1."""
        from mellanox_bfb_installer import dump_receiver

        with tempfile.TemporaryDirectory(prefix="dr_main_err_") as out, \
             self._patch_argv([
                 "--bind-ip", "127.0.0.1",
                 "--port", "0",
                 "--output-dir", out,
             ]), \
             mock.patch.object(
                 dump_receiver, "_build_server", side_effect=OSError("EADDRINUSE")
             ), \
             mock.patch.object(dump_receiver.signal, "signal"):
            rc = dump_receiver.main()

        self.assertEqual(rc, 1)


class TestDumpReceiverSymlinkEscape(_ReceiverServerFixture):
    """Belt-and-suspenders test for the realpath check.

    The URL regex already blocks ``..`` and ``/`` in filenames, but the
    handler also checks ``realpath(dest)`` is under ``realpath(output_dir)``.
    To exercise that branch we plant a symlink at ``<output_dir>/dpu0``
    pointing outside the configured output directory and confirm the handler
    refuses to write.
    """

    def test_symlinked_dpu_dir_rejected_with_400(self):
        outside = tempfile.mkdtemp(prefix="dr_escape_target_")
        self.addCleanup(shutil.rmtree, outside, True)

        # Note: makedirs(...exist_ok=True) tolerates an existing path even if
        # it's a symlink, so we can place the symlink first and the handler
        # will not clobber it.
        os.symlink(outside, os.path.join(self.tmpdir, "dpu0"))

        conn = self._conn()
        conn.request("PUT", "/dpu0/escape.bin", body=b"should-not-land")
        resp = conn.getresponse()
        body = resp.read()
        conn.close()

        self.assertEqual(resp.status, 400)
        self.assertIn(b"invalid destination", body)
        # Nothing must have appeared in the escape-target directory.
        self.assertFalse(
            os.listdir(outside),
            f"symlink escape produced files in {outside}: {os.listdir(outside)}",
        )


if __name__ == "__main__":
    unittest.main()
