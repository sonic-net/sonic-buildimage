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
BFB image file: URL download, extract, SHA256 checksum checking.
"""

import glob
import hashlib
import logging
import os
import sys
import subprocess
from typing import Optional, Tuple

logger = logging.getLogger(__name__)

DOWNLOAD_FILENAME = "sonic-nvidia-bluefield.bfb"


def _is_url(url_or_path: str) -> bool:
    return url_or_path.startswith("http://") or url_or_path.startswith("https://")


def _maybe_download_bfb(url_or_path: str, work_dir: str) -> str:
    """If bfb_arg is a URL, download to work_dir and return the local path.

    If url_or_path is a local path, return it unchanged.
    """
    if not _is_url(url_or_path):
        return url_or_path
    logger.debug("Detected URL. Downloading file")
    filename = os.path.join(work_dir, DOWNLOAD_FILENAME)
    try:
        result = subprocess.run(
            ["curl", "-L", "-o", filename, url_or_path],
        )
        if result.returncode != 0:
            logger.error("the curl command failed with: %s", result.returncode)
            sys.exit(1)
    except FileNotFoundError as err:
        logger.error("curl command not found: %s", err)
        sys.exit(1)
    logger.debug("bfb path changed to %s", filename)
    return filename


def _extract_bfb(bfb_file: str, work_dir: str) -> Tuple[str, Optional[str]]:
    """Extract the raw BFB file and sha256 file from a tar-archive image file.

    Fail if not a tar-archive.
    Does not verify the SHA256 of the extracted BFB file.
    """
    if not os.path.isfile(bfb_file):
        logger.error("BFB file not found: %s", bfb_file)
        sys.exit(1)

    file_result = subprocess.run(
        ["file", "-b", bfb_file],
        capture_output=True,
        text=True,
        timeout=5,
    )
    if file_result.returncode != 0:
        logger.error("the file command failed with: %s", file_result.returncode)
        sys.exit(1)
    file_type = file_result.stdout.strip()

    if "tar archive" not in file_type:
        logger.error(
            "File is not a tar archive: %s! Please provide a tar archive with .bfb extension"
            " containing BFB and SHA256 hash.",
            bfb_file,
        )
        sys.exit(1)

    logger.info("Detected tar archive extracting BFB and SHA256 hash...")

    extract_result = subprocess.run(
        ["tar", "-xf", bfb_file, "-C", work_dir],
        capture_output=True,
        timeout=60,
    )
    if extract_result.returncode != 0:
        logger.error("Failed to extract tar archive: %s", bfb_file)
        sys.exit(1)

    bfb_basename = os.path.basename(bfb_file)
    candidates = glob.glob(os.path.join(work_dir, "*bfb-intermediate"))
    extracted_bfb = None
    for p in candidates:
        if bfb_basename in p:
            extracted_bfb = p
            break
    if not extracted_bfb and candidates:
        extracted_bfb = candidates[0]
    if not extracted_bfb:
        logger.error("No BFB file found in tar archive")
        sys.exit(1)

    logger.info("Extracted BFB file: %s", extracted_bfb)
    os.chmod(extracted_bfb, 0o644)

    extracted_sha256 = extracted_bfb + ".sha256"
    if os.path.isfile(extracted_sha256):
        logger.info("Found SHA256 hash file: %s", extracted_sha256)
    else:
        logger.warning("SHA256 hash file not found in tar archive")
        extracted_sha256 = None

    return (extracted_bfb, extracted_sha256)


def _validate_bfb_sha256(
    extracted_bfb_path: str, extracted_checksum_path: Optional[str]
) -> None:
    if not extracted_checksum_path or not os.path.isfile(extracted_checksum_path):
        logger.error("SHA256 hash file not found: %s", extracted_checksum_path or "")
        sys.exit(1)

    logger.info("Verifying SHA256 checksum...")

    try:
        with open(extracted_checksum_path, "r", encoding="utf-8") as f:
            expected_hash = f.read().strip()
    except Exception as e:
        logger.error("Failed to read SHA256 hash file: %s", e)
        sys.exit(1)

    sha256 = hashlib.sha256()
    with open(extracted_bfb_path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            sha256.update(chunk)
    actual_hash = sha256.hexdigest()

    if expected_hash != actual_hash:
        logger.error("SHA256 checksum mismatch!")
        logger.error("Expected: %s", expected_hash)
        logger.error("Actual:   %s", actual_hash)
        logger.error("BFB file may be corrupted or tampered with.")
        sys.exit(1)

    logger.info("SHA256 checksum verification successful")


def prepare_bfb(bfb: Optional[str], work_dir: str, skip_extract: bool) -> str:
    """Prepare BFB image for installation.

    Includes potential download, extraction, and SHA256 validation.
    Returns the local path of the extracted raw BFB image file.
    """
    try:
        bfb_path = _maybe_download_bfb(bfb, work_dir)
        if skip_extract:
            extracted_bfb_path = bfb_path
        else:
            extracted_bfb_path, extracted_checksum_path = _extract_bfb(
                bfb_path, work_dir
            )
            _validate_bfb_sha256(extracted_bfb_path, extracted_checksum_path)
    except SystemExit:
        raise
    except Exception as e:
        logger.error("BFB handling failed: %s", e)
        sys.exit(1)
    return extracted_bfb_path
