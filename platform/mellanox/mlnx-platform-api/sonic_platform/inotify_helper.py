#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
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

"""Helper code for Inotify Implementation for reading file until timeout"""
import os
import errno
# Inotify causes an exception when DEBUG env variable is set to a non-integer convertible
# https://github.com/dsoprea/PyInotify/blob/0.2.10/inotify/adapters.py#L37
os.environ['DEBUG'] = '0'
import inotify.adapters

try:
    from sonic_py_common.syslogger import SysLogger
    from . import utils
except ImportError as e:
    raise ImportError(str(e) + '- required module not found') from e

logger = SysLogger()


class InotifyHelper():
    """Helper Code for Inotify Implmentation"""
    def __init__(self, file_path):
        self.file_path = file_path
        self.inotify_obj = inotify.adapters.Inotify()
        if not self.inotify_obj:
            logger.log_error("INOTIFY adapter error!")
            raise RuntimeError("INOTIFY is not present!")
        if not os.path.exists(self.file_path):
            logger.log_error(f"{self.file_path} does not exist")
            raise FileNotFoundError(errno.ENOENT,
                                    os.strerror(errno.ENOENT),
                                    self.file_path)

    def wait_watch(self, timeout, expected_value):
        """Waits for changes in file until specified time and
          compares written value to expected value"""
        self.inotify_obj.add_watch(self.file_path,
                                   mask=inotify.constants.IN_CLOSE_WRITE)
        for event in self.inotify_obj.event_gen(timeout_s=timeout,
                                                yield_nones=False):
            read_value = utils.read_int_from_file(self.file_path,
                                                  raise_exception=True)
            if read_value == expected_value:
                return read_value
        read_value = utils.read_int_from_file(self.file_path,
                                              raise_exception=True)
        if read_value != expected_value:
            return None
        return read_value
