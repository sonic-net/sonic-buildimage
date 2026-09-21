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

"""Run independent installer tasks in parallel."""

import logging
from concurrent.futures import ThreadPoolExecutor, as_completed
from typing import Callable

logger = logging.getLogger(__name__)


def run_parallel(
    task_count: int,
    task_fn: Callable[[int], int],
) -> int:
    """Run indexed tasks in parallel and return the number that failed.

    Signal and subprocess supervision belong to the caller that owns the installation
    transaction. A running task cannot be cancelled; executor shutdown waits for every worker.

    Args:
        task_count: Number of tasks to run.
        task_fn: Task receiving its index and returning a per-task status.

    Returns:
        Number of tasks that returned a non-zero status or raised an exception.
    """
    failed = 0
    with ThreadPoolExecutor(max_workers=task_count) as executor:
        futures = [executor.submit(task_fn, index) for index in range(task_count)]
        for future in as_completed(futures):
            try:
                if future.result():
                    failed += 1
            except Exception as error:
                logger.error("Parallel task failed: %s", error)
                failed += 1
    return failed
