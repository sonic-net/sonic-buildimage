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
Unit tests for mellanox_bfb_installer.bfb_file module.
"""

import os
import shutil
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))


class TestBfbFile(unittest.TestCase):
    """Tests for bfb_file module."""

    def test_is_url_true_for_http_and_https(self):
        from mellanox_bfb_installer import bfb_file

        self.assertTrue(bfb_file._is_url("http://example.com/file.bfb"))
        self.assertTrue(bfb_file._is_url("https://example.com/file.bfb"))

    def test_is_url_false_for_path(self):
        from mellanox_bfb_installer import bfb_file

        self.assertFalse(bfb_file._is_url("/path/to/file.bfb"))
        self.assertFalse(bfb_file._is_url("file.bfb"))

    def test_maybe_download_bfb_returns_path_when_not_url(self):
        from mellanox_bfb_installer import bfb_file

        self.assertEqual(
            bfb_file._maybe_download_bfb("/local/file.bfb", "/tmp/work"),
            "/local/file.bfb",
        )

    def test_maybe_download_bfb_downloads_when_url(self):
        from mellanox_bfb_installer import bfb_file

        mock_log = mock.MagicMock()
        with tempfile.TemporaryDirectory() as work_dir:
            with (
                mock.patch.object(bfb_file, "logger", mock_log),
                mock.patch.object(bfb_file.subprocess, "run") as mock_run,
            ):
                mock_run.return_value = mock.MagicMock(returncode=0)
                out = bfb_file._maybe_download_bfb("https://example.com/image.bfb", work_dir)
                self.assertEqual(out, os.path.join(work_dir, bfb_file.DOWNLOAD_FILENAME))
                mock_log.debug.assert_any_call("Detected URL. Downloading file")
                mock_run.assert_called_once()
                call_args = mock_run.call_args[0][0]
                self.assertEqual(call_args, ["curl", "-L", "-o", out, "https://example.com/image.bfb"])

    def test_maybe_download_bfb_exits_on_curl_failure(self):
        from mellanox_bfb_installer import bfb_file

        mock_log = mock.MagicMock()
        with tempfile.TemporaryDirectory() as work_dir:
            with (
                mock.patch.object(bfb_file, "logger", mock_log),
                mock.patch.object(bfb_file.subprocess, "run") as mock_run,
            ):
                mock_run.return_value = mock.MagicMock(returncode=1)
                with self.assertRaises(SystemExit) as ctx:
                    bfb_file._maybe_download_bfb("https://x/y.bfb", work_dir)
                self.assertEqual(ctx.exception.code, 1)
                mock_log.error.assert_called_once()
                self.assertIn("curl command failed", mock_log.error.call_args[0][0])

    def test_extract_bfb_raises_when_file_missing(self):
        from mellanox_bfb_installer import bfb_file

        mock_log = mock.MagicMock()
        with tempfile.TemporaryDirectory() as work_dir:
            with mock.patch.object(bfb_file, "logger", mock_log):
                with self.assertRaises(SystemExit) as ctx:
                    bfb_file._extract_bfb("/nonexistent/file.bfb", work_dir)
                self.assertEqual(ctx.exception.code, 1)
                mock_log.error.assert_called_once()
                self.assertIn("BFB file not found", mock_log.error.call_args[0][0])

    def test_extract_bfb_raises_when_not_tar(self):
        from mellanox_bfb_installer import bfb_file

        mock_log = mock.MagicMock()
        with tempfile.NamedTemporaryFile(suffix=".bfb", delete=False) as f:
            try:
                f.write(b"not a tar")
                f.flush()
                with (
                    mock.patch.object(bfb_file, "logger", mock_log),
                    mock.patch.object(bfb_file.subprocess, "run") as mock_run,
                ):
                    mock_run.return_value = mock.MagicMock(returncode=0, stdout="ASCII text")
                    with self.assertRaises(SystemExit) as ctx:
                        bfb_file._extract_bfb(f.name, os.path.dirname(f.name))
                    self.assertEqual(ctx.exception.code, 1)
                    mock_log.error.assert_called()
                    self.assertIn("not a tar archive", mock_log.error.call_args[0][0])
            finally:
                os.unlink(f.name)

    def test_extract_bfb_extracts_and_returns_paths(self):
        from mellanox_bfb_installer import bfb_file

        mock_log = mock.MagicMock()
        with tempfile.TemporaryDirectory() as work_dir:
            bfb_file_path = os.path.join(work_dir, "image.bfb")
            with open(bfb_file_path, "wb") as f:
                f.write(b"fake tar content")
            extracted_path = os.path.join(work_dir, "image.bfb-intermediate")
            with open(extracted_path, "w") as f:
                f.write("data")
            sha_path = extracted_path + ".sha256"
            with open(sha_path, "w") as f:
                f.write("abc123")
            with (
                mock.patch.object(bfb_file, "logger", mock_log),
                mock.patch.object(bfb_file.subprocess, "run") as mock_run,
            ):

                def run_side_effect(cmd, *args, **kwargs):
                    if cmd[0] == "file":
                        return mock.MagicMock(returncode=0, stdout="tar archive\n")
                    if cmd[0] == "tar":
                        return mock.MagicMock(returncode=0)
                    return mock.MagicMock(returncode=1)

                mock_run.side_effect = run_side_effect
                bfb_path, checksum_path = bfb_file._extract_bfb(bfb_file_path, work_dir)
            self.assertEqual(bfb_path, extracted_path)
            self.assertEqual(checksum_path, sha_path)
            mock_log.info.assert_any_call("Detected tar archive extracting BFB and SHA256 hash...")
            mock_log.info.assert_any_call("Extracted BFB file: %s", extracted_path)
            mock_log.info.assert_any_call("Found SHA256 hash file: %s", sha_path)

    def test_validate_bfb_sha256_success(self):
        from mellanox_bfb_installer import bfb_file

        mock_log = mock.MagicMock()
        with tempfile.NamedTemporaryFile(mode="wb", suffix=".bfb", delete=False) as bfb_f:
            content = b"bfb-content"
            bfb_f.write(content)
            bfb_f.flush()
            bfb_path = bfb_f.name
        try:
            with tempfile.NamedTemporaryFile(mode="w", suffix=".sha256", delete=False) as h_f:
                h_f.write("14ddd86eb3a5ba510279e487ae986fa66014cb2ac03af4555a6fd5b4f2936fe8")
                h_f.flush()
                hash_path = h_f.name
            try:
                with mock.patch.object(bfb_file, "logger", mock_log):
                    bfb_file._validate_bfb_sha256(bfb_path, hash_path)
                mock_log.info.assert_any_call("Verifying SHA256 checksum...")
                mock_log.info.assert_any_call("SHA256 checksum verification successful")
            finally:
                os.unlink(hash_path)
        finally:
            os.unlink(bfb_path)

    def test_validate_bfb_sha256_mismatch_exits(self):
        from mellanox_bfb_installer import bfb_file

        mock_log = mock.MagicMock()
        with tempfile.NamedTemporaryFile(mode="wb", delete=False) as bfb_f:
            bfb_f.write(b"content")
            bfb_path = bfb_f.name
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as h_f:
            h_f.write("wrong-hash")
            hash_path = h_f.name
        try:
            with mock.patch.object(bfb_file, "logger", mock_log):
                with self.assertRaises(SystemExit) as ctx:
                    bfb_file._validate_bfb_sha256(bfb_path, hash_path)
            self.assertEqual(ctx.exception.code, 1)
            mock_log.error.assert_any_call("SHA256 checksum mismatch!")
        finally:
            os.unlink(bfb_path)
            os.unlink(hash_path)

    def test_validate_bfb_sha256_missing_checksum_exits(self):
        from mellanox_bfb_installer import bfb_file

        mock_log = mock.MagicMock()
        with tempfile.NamedTemporaryFile(delete=False) as bfb_f:
            bfb_path = bfb_f.name
        try:
            with mock.patch.object(bfb_file, "logger", mock_log):
                with self.assertRaises(SystemExit) as ctx:
                    bfb_file._validate_bfb_sha256(bfb_path, "/nonexistent/sha256")
            self.assertEqual(ctx.exception.code, 1)
            mock_log.error.assert_called()
            self.assertIn("SHA256 hash file not found", mock_log.error.call_args[0][0])
        finally:
            os.unlink(bfb_path)

    def test_prepare_bfb_skip_extract_returns_path_without_extract_or_validate(self):
        """With skip_extract=True, returns maybe_download_bfb result; no extract or validate."""
        from mellanox_bfb_installer import bfb_file

        work_dir = tempfile.mkdtemp()
        try:
            local_path = os.path.join(work_dir, "local.bfb")
            with open(local_path, "w") as f:
                f.write("x")
            with (
                mock.patch.object(
                    bfb_file,
                    "_maybe_download_bfb",
                    return_value=local_path,
                ),
                mock.patch.object(bfb_file, "_extract_bfb") as mock_extract,
                mock.patch.object(
                    bfb_file,
                    "_validate_bfb_sha256",
                ) as mock_validate,
            ):
                result = bfb_file.prepare_bfb(local_path, work_dir, skip_extract=True)
            self.assertEqual(result, local_path)
            mock_extract.assert_not_called()
            mock_validate.assert_not_called()
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)

    def test_prepare_bfb_full_flow_calls_download_extract_validate(self):
        """With skip_extract=False, calls _maybe_download_bfb, _extract_bfb, _validate_bfb_sha256."""
        from mellanox_bfb_installer import bfb_file

        work_dir = tempfile.mkdtemp()
        try:
            bfb_path = os.path.join(work_dir, "in.bfb")
            extracted_path = os.path.join(work_dir, "out.bfb-intermediate")
            checksum_path = extracted_path + ".sha256"
            with (
                mock.patch.object(
                    bfb_file,
                    "_maybe_download_bfb",
                    return_value=bfb_path,
                ) as mock_download,
                mock.patch.object(
                    bfb_file,
                    "_extract_bfb",
                    return_value=(extracted_path, checksum_path),
                ) as mock_extract,
                mock.patch.object(
                    bfb_file,
                    "_validate_bfb_sha256",
                ) as mock_validate,
            ):
                result = bfb_file.prepare_bfb(bfb_path, work_dir, skip_extract=False)
            self.assertEqual(result, extracted_path)
            mock_download.assert_called_once_with(bfb_path, work_dir)
            mock_extract.assert_called_once_with(bfb_path, work_dir)
            mock_validate.assert_called_once_with(extracted_path, checksum_path)
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)

    def test_prepare_bfb_reraises_system_exit_from_inner_call(self):
        """SystemExit from maybe_download_bfb, extract_bfb, or validate is re-raised."""
        from mellanox_bfb_installer import bfb_file

        work_dir = tempfile.mkdtemp()
        try:
            with mock.patch.object(
                bfb_file,
                "_maybe_download_bfb",
                side_effect=SystemExit(1337),
            ):
                with self.assertRaises(SystemExit) as ctx:
                    bfb_file.prepare_bfb("/some.bfb", work_dir, skip_extract=True)
            self.assertEqual(ctx.exception.code, 1337)
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)

    def test_prepare_bfb_logs_and_exits_on_other_exception(self):
        """Other exceptions are logged and converted to SystemExit(1)."""
        from mellanox_bfb_installer import bfb_file

        mock_log = mock.MagicMock()
        work_dir = tempfile.mkdtemp()
        try:
            with (
                mock.patch.object(bfb_file, "logger", mock_log),
                mock.patch.object(
                    bfb_file,
                    "_maybe_download_bfb",
                    side_effect=RuntimeError("download failed"),
                ),
            ):
                with self.assertRaises(SystemExit) as ctx:
                    bfb_file.prepare_bfb("http://x/y.bfb", work_dir, skip_extract=True)
            self.assertEqual(ctx.exception.code, 1)
            mock_log.error.assert_called_once()
            self.assertIn("BFB handling failed", mock_log.error.call_args[0][0])
            self.assertIn("download failed", str(mock_log.error.call_args[0][1]))
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)


if __name__ == "__main__":
    unittest.main()
