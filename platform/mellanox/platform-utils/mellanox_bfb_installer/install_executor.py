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
Parallel install executor with signal handling.
"""

import logging
import os
import signal
import sys
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed
from typing import Callable

logger = logging.getLogger(__name__)


class PidCollection:
    """Thread-safe list of child process PIDs."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._pids: list[int] = []

    def append(self, pid: int) -> None:
        with self._lock:
            self._pids.append(pid)

    def remove_if_contains(self, pid: int) -> None:
        with self._lock:
            try:
                self._pids.remove(pid)
            except ValueError:
                pass

    def copy_to_list(self) -> list[int]:
        """Return a copy of the PID list for safe iteration (e.g. in a signal handler)."""
        with self._lock:
            return list(self._pids)


def run_parallel(
    task_count: int,
    task_fn: Callable[[int, PidCollection], int],
) -> int:
    """
    Run task_fn(0, child_pids), task_fn(1, child_pids), ... in parallel via ThreadPoolExecutor.

    The task_fn is expected to install a bfb image to a single device, by forking child processes.
    The task_fn must append the child process PIDs to the child_pids collection that is passed to
    it as the second argument. This parallel executor will install signal handlers that kill all
    child processes on SIGINT/SIGTERM/SIGHUP.

    Returns the number of tasks that exited with a non-zero status or raised an exception.
    """
    child_pids = PidCollection()

    def _kill_child_procs(_signum=None, _frame=None):
        logger.warning("Installation interrupted. Killing all child procs.")
        pids = child_pids.copy_to_list()
        for pid in pids:
            try:
                logger.debug("Killing child proc PID %s.", pid)
                os.kill(pid, signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
        raise SystemExit(1)

    signal.signal(signal.SIGINT, _kill_child_procs)
    signal.signal(signal.SIGTERM, _kill_child_procs)
    signal.signal(signal.SIGHUP, _kill_child_procs)

    failed = 0
    with ThreadPoolExecutor(max_workers=task_count) as executor:
        futures = {executor.submit(task_fn, i, child_pids): i for i in range(task_count)}
        for future in as_completed(futures):
            try:
                if future.result() != 0:
                    failed += 1
            except Exception as e:
                logger.error("Install task failed: %s", e)
                failed += 1
    return failed
