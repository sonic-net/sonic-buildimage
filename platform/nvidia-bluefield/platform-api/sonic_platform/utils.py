#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
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
