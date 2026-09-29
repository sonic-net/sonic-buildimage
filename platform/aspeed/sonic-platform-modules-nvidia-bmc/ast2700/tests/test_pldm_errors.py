#
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

"""Tests for pldm_errors — DSP0267 error code annotation."""

import pytest

from sonic_platform import pldm_errors
from sonic_platform.pldm_errors import (
    COMPLETION_CODES,
    TRANSFER_RESULT_CODES,
    VERIFY_RESULT_CODES,
    APPLY_RESULT_CODES,
    COMPONENT_RESPONSE_CODES,
    annotate,
)


class TestCodeTableCompleteness:

    def test_completion_codes_include_base_codes(self):
        assert 0x00 in COMPLETION_CODES  # SUCCESS
        assert 0x05 in COMPLETION_CODES  # ERROR_UNSUPPORTED_PLDM_CMD

    def test_completion_codes_include_fw_update_range(self):
        # DSP0267 firmware-update-specific codes run 0x80–0x96
        for code in range(0x80, 0x97):
            assert code in COMPLETION_CODES, f"0x{code:02x} missing from COMPLETION_CODES"

    def test_transfer_result_has_success_and_corrupt(self):
        assert 0x00 in TRANSFER_RESULT_CODES
        assert 0x01 in TRANSFER_RESULT_CODES

    def test_verify_result_has_success_and_failure(self):
        assert 0x00 in VERIFY_RESULT_CODES
        assert 0x01 in VERIFY_RESULT_CODES

    def test_apply_result_has_success_and_success_with_modification(self):
        assert 0x00 in APPLY_RESULT_CODES
        assert 0x01 in APPLY_RESULT_CODES

    def test_component_response_has_can_be_updated_and_not_supported(self):
        assert 0x00 in COMPONENT_RESPONSE_CODES
        assert 0x06 in COMPONENT_RESPONSE_CODES

    def test_all_entries_are_two_tuples_of_strings(self):
        for table in (
            COMPLETION_CODES,
            TRANSFER_RESULT_CODES,
            VERIFY_RESULT_CODES,
            APPLY_RESULT_CODES,
            COMPONENT_RESPONSE_CODES,
        ):
            for code, entry in table.items():
                assert isinstance(entry, tuple) and len(entry) == 2, \
                    f"code 0x{code:02x}: expected (name, desc) tuple, got {entry!r}"
                name, desc = entry
                assert isinstance(name, str) and name, \
                    f"code 0x{code:02x}: name must be non-empty string"
                assert isinstance(desc, str) and desc, \
                    f"code 0x{code:02x}: desc must be non-empty string"


class TestAnnotate:

    def test_known_completion_code_gets_annotated(self):
        result = annotate("completion code 0x84")
        assert "0x84" in result
        assert "INVALID_STATE_FOR_COMMAND" in result
        assert "not in a state" in result.lower()

    def test_unknown_code_left_unchanged(self):
        text = "error code 0xFF"
        assert annotate(text) == text

    def test_success_code_annotated(self):
        result = annotate("returned 0x00")
        assert "SUCCESS" in result

    def test_multiple_codes_all_annotated(self):
        result = annotate("cmd 0x84 and 0x85")
        assert "INVALID_STATE_FOR_COMMAND" in result
        assert "INCOMPLETE_UPDATE" in result

    def test_no_codes_returns_unchanged(self):
        text = "no hex codes here"
        assert annotate(text) == text

    def test_transfer_context_picks_transfer_table(self):
        # "transfer" keyword within the context window should route 0x01 to
        # TRANSFER_RESULT_CODES (TRANSFER_IMAGE_CORRUPT) not COMPLETION_CODES (ERROR).
        result = annotate("firmware transfer error: result 0x01")
        assert "TRANSFER_IMAGE_CORRUPT" in result

    def test_verify_context_picks_verify_table(self):
        result = annotate("verify failure: result 0x01")
        assert "VERIFY_FAILURE" in result

    def test_apply_context_picks_apply_table(self):
        result = annotate("apply complete: result 0x02")
        assert "APPLY_MEMORY_WRITE_FAILURE" in result

    def test_no_context_falls_back_to_completion_codes(self):
        # 0x01 with no context keyword → completion code ERROR
        result = annotate("failed: 0x01")
        assert "ERROR" in result
        assert "TRANSFER_IMAGE_CORRUPT" not in result
        assert "VERIFY_FAILURE" not in result

    def test_context_keyword_outside_window_not_used(self):
        # Put the keyword far enough before the code that it falls outside
        # the 60-char context window; should fall back to completion codes.
        prefix = "transfer " + "x" * 80
        result = annotate(f"{prefix} 0x01")
        assert "TRANSFER_IMAGE_CORRUPT" not in result

    def test_three_byte_hex_not_matched(self):
        # annotate() only handles byte-sized (two-hex-digit) codes.
        text = "address 0x8400"
        assert annotate(text) == text

    def test_real_pldm_fw_error_format(self):
        # Matches the exact format emitted by pldm-fw ua.rs:
        # "PLDM command (0xNN) failed with 0xNN"
        result = annotate("PLDM command (0x10) failed with 0x84")
        # 0x10 (RequestUpdate) is a command byte, not a completion code,
        # so it will get the base SUCCESS annotation or none if not in table.
        # 0x84 should definitely get annotated.
        assert "INVALID_STATE_FOR_COMMAND" in result
