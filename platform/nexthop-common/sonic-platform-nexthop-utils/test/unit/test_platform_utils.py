#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the platform_utils module.
"""

import subprocess

import pytest

from nexthop_utils.platform_utils import CommandError, run_cmd


class TestRunCmd:
    def test_failure_str_includes_stderr_and_stdout(self):
        cmd = "sh -c 'echo progress; echo \"the error\" >&2; exit 3'"

        with pytest.raises(CommandError) as excinfo:
            run_cmd(cmd)

        message = str(excinfo.value)
        assert message.startswith(
            f"Command '{cmd}' returned non-zero exit status 3."
        )
        assert "stderr: the error" in message
        assert "stdout: progress" in message

    def test_failure_str_omits_empty_streams(self):
        with pytest.raises(CommandError) as excinfo:
            run_cmd("false")

        assert str(excinfo.value) == "Command 'false' returned non-zero exit status 1."

    def test_failure_str_output_tail_is_bounded(self):
        tail_chars = CommandError.TAIL_CHARS
        error = CommandError(1, "cmd", stderr="x" * (tail_chars * 2) + "the actual error")

        message = str(error)
        assert message.endswith("the actual error")
        assert len(message) < tail_chars + 200

    def test_failure_is_a_called_process_error(self):
        with pytest.raises(subprocess.CalledProcessError):
            run_cmd("false")

    def test_check_exit_false_returns_stdout(self):
        assert run_cmd("sh -c 'echo ok; exit 1'", check_exit=False) == "ok"
