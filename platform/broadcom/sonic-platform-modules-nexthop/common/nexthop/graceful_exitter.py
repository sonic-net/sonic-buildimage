# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import signal
import threading

try:
    from sonic_py_common import syslogger
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class GracefulExitter:
    """
    This class provides a way to query whether the program has received a SIGTERM signal to exit.

    Should be used by a long-running thread that wants to exit gracefully, like this:

    exitter = GracefulExitter()
    while not exitter.should_exit():
        # do work
        exitter.sleep_respect_exit(1)
    """

    exit_event = threading.Event()

    def __init__(self, syslog_identifier: str):
        signal.signal(signal.SIGTERM, self._handle_exit)

        self.logger_instance = syslogger.SysLogger(syslog_identifier)

    def _handle_exit(self, signum, frame):
        self.logger_instance.log_notice(
            f"Caught {signal.Signals(signum).name} - exiting..."
        )
        self.exit_event.set()

    def should_exit(self) -> bool:
        return self.exit_event.is_set()

    def sleep_respect_exit(self, seconds: float):
        self.exit_event.wait(seconds)
