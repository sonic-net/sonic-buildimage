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
import ctypes
import functools
import subprocess
from sonic_py_common.logger import Logger

logger = Logger()


def read_only_cache():
    """Decorator to cache return value for a method/function once.
       This decorator should be used for method/function when:
       1. Executing the method/function takes time. e.g. reading sysfs.
       2. The return value of this method/function never changes.
    """
    def decorator(method):
        method.return_value = None

        @functools.wraps(method)
        def _impl(*args, **kwargs):
            if not method.return_value:
                method.return_value = method(*args, **kwargs)
            return method.return_value
        return _impl
    return decorator


@read_only_cache()
def is_host():
    """
    Test whether current process is running on the host or an docker
    return True for host and False for docker
    """
    try:
        proc = subprocess.Popen("docker --version 2>/dev/null",
                                stdout=subprocess.PIPE,
                                shell=True,
                                stderr=subprocess.STDOUT,
                                universal_newlines=True)
        stdout = proc.communicate()[0]
        proc.wait()
        result = stdout.rstrip('\n')
        return result != ''
    except OSError as e:
        return False


def default_return(return_value, log_func=logger.log_debug):
    def wrapper(method):
        @functools.wraps(method)
        def _impl(*args, **kwargs):
            try:
                return method(*args, **kwargs)
            except Exception as e:
                if log_func:
                    log_func('Faield to execute method {} - {}'.format(method.__name__, repr(e)))
                return return_value
        return _impl
    return wrapper


def read_from_file(file_path, target_type, default='', raise_exception=False, log_func=logger.log_error):
    """
    Read content from file and convert to target type
    :param file_path: File path
    :param target_type: target type
    :param default: Default return value if any exception occur
    :param raise_exception: Raise exception to caller if True else just return default value
    :param log_func: function to log the error
    :return: String content of the file
    """
    try:
        with open(file_path, 'r') as f:
            value = f.read()
            if value is None:
                # None return value is not allowed in any case, so we log error here for further debug.
                logger.log_error('Failed to read from {}, value is None, errno is {}'.format(file_path, ctypes.get_errno()))
                # Raise ValueError for the except statement to handle this as a normal exception
                raise ValueError('File content of {} is None'.format(file_path))
            else:
                value = target_type(value.strip())
    except (ValueError, IOError) as e:
        if log_func:
            log_func('Failed to read from file {} - {}'.format(file_path, repr(e)))
        if not raise_exception:
            value = default
        else:
            raise e

    return value


def read_float_from_file(file_path, default=0.0, raise_exception=False, log_func=logger.log_error):
    """
    Read content from file and cast it to integer
    :param file_path: File path
    :param default: Default return value if any exception occur
    :param raise_exception: Raise exception to caller if True else just return default value
    :param log_func: function to log the error
    :return: Integer value of the file content
    """
    return read_from_file(file_path=file_path, target_type=float, default=default, raise_exception=raise_exception, log_func=log_func)


def write_file(file_path, content, raise_exception=False, log_func=logger.log_error):
    """
    Write the given value to a file
    :param file_path: File path
    :param content: Value to write to the file
    :param raise_exception: Raise exception to caller if True
    :return: True if write success else False
    """
    try:
        with open(file_path, 'w') as f:
            f.write(str(content))
    except (ValueError, IOError) as e:
        if log_func:
            log_func('Failed to write {} to file {} - {}'.format(content, file_path, repr(e)))
        if not raise_exception:
            return False
        else:
            raise e
    return True
