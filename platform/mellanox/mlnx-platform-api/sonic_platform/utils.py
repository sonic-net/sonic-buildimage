#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2020-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
import ctypes
import functools
import subprocess
import json
import queue
import sys
import threading
import time
import os
import re

# Inotify causes an exception when DEBUG env variable is set to a non-integer convertible
# https://github.com/dsoprea/PyInotify/blob/0.2.10/inotify/adapters.py#L37
os.environ['DEBUG'] = '0'
import inotify.adapters
import inotify.constants
from sonic_py_common import device_info
from sonic_py_common.logger import Logger

HWSKU_JSON = 'hwsku.json'

PORT_INDEX_KEY = "index"
PORT_TYPE_KEY = "port_type"
RJ45_PORT_TYPE = "RJ45"

_OE_DEVICE_ID_RE = re.compile(r'OE(\d+)')
_ELS_DEVICE_ID_RE = re.compile(r'ELS(\d+)')

logger = Logger()


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


def read_str_from_file(file_path, default='', raise_exception=False, log_func=logger.log_error):
    """
    Read string content from file
    :param file_path: File path
    :param default: Default return value if any exception occur
    :param raise_exception: Raise exception to caller if True else just return default value
    :param log_func: function to log the error
    :return: String content of the file
    """
    return read_from_file(file_path=file_path, target_type=str, default=default, raise_exception=raise_exception, log_func=log_func)


def read_int_from_file(file_path, default=0, raise_exception=False, log_func=logger.log_error):
    """
    Read content from file and cast it to integer
    :param file_path: File path
    :param default: Default return value if any exception occur
    :param raise_exception: Raise exception to caller if True else just return default value
    :param log_func: function to log the error
    :return: Integer value of the file content
    """
    return read_from_file(file_path=file_path, target_type=int, default=default, raise_exception=raise_exception, log_func=log_func)


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


def _key_value_converter(content, delimeter):
    ret = {}
    for line in content.splitlines():
        k,v = line.split(delimeter, 1)
        ret[k.strip()] = v.strip()
    return ret


def read_key_value_file(file_path, default={}, raise_exception=False, log_func=logger.log_error, delimeter=':'):
    """Read file content and parse the content to a dict. The file content should like:
       key1:value1
       key2:value2

    Args:
        file_path (str): file path
        default (dict, optional): default return value. Defaults to {}.
        raise_exception (bool, optional): If exception should be raised or hiden. Defaults to False.
        log_func (optional): logger function.. Defaults to logger.log_error.
    """
    converter = lambda content: _key_value_converter(content, delimeter)
    return read_from_file(file_path=file_path, target_type=converter, default=default, raise_exception=raise_exception, log_func=log_func)


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


def pre_initialize(init_func):
    def decorator(method):
        @functools.wraps(method)
        def _impl(self, *args, **kwargs):
            init_func(self)
            return method(self, *args, **kwargs)
        return _impl
    return decorator


def pre_initialize_one(init_func):
    def decorator(method):
        @functools.wraps(method)
        def _impl(self, index):
            init_func(self, index)
            return method(self, index)
        return _impl
    return decorator


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
    docker_env_file = '/.dockerenv'
    return os.path.exists(docker_env_file) is False


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


def run_command(command):
    """
    Utility function to run an shell command and return the output.
    :param command: Shell command string.
    :return: Output of the shell command.
    """
    try:
        process = subprocess.Popen(command, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return process.communicate()[0].strip()
    except Exception:
        return None


def load_json_file(filename, log_func=logger.log_error):
    # load 'platform.json' or 'hwsku.json' file
    data = None
    try:
        with open(filename) as fp:
            try:
                data = json.load(fp)
            except json.JSONDecodeError:
                if log_func:
                    log_func("failed to decode Json file.")
        return data
    except Exception as e:
        if log_func:
            log_func("error occurred while parsing json file: {}".format(sys.exc_info()[1]))
        return None


def _extract_ports_index_by_type(port_type, num_of_asics=1):
    platform_file = os.path.join(device_info.get_path_to_platform_dir(), device_info.PLATFORM_JSON_FILE)
    if not os.path.exists(platform_file):
        return None

    platform_dict = load_json_file(platform_file)['interfaces']
    port_name_to_index_map_dict = {}
    port_index_list = []

    # Compose a interface name to index mapping from 'platform.json'
    for i, (key, value) in enumerate(platform_dict.items()):
        if PORT_INDEX_KEY in value:
            index_raw = value[PORT_INDEX_KEY]
            # The index could be "1" or "1, 1, 1, 1"
            index = index_raw.split(',')[0]
            port_name_to_index_map_dict[key] = index

    if not bool(port_name_to_index_map_dict):
        return None

    hwsku_jsons = get_path_list_to_asic_hwsku_dir(num_of_asics)
    hwsku_dict = {}
    for hwsku_json in hwsku_jsons:
        if not os.path.exists(hwsku_json):
            continue
        hwsku_dict.update(load_json_file(hwsku_json)['interfaces'])

    # Check platform has no hwsku.json(s)
    if not hwsku_dict:
        return None

    # Check if "port_type" matches, if yes, add the port index to the list.
    for i, (key, value) in enumerate(hwsku_dict.items()):
        if key in port_name_to_index_map_dict and PORT_TYPE_KEY in value and value[PORT_TYPE_KEY] == port_type:
            port_index_list.append(int(port_name_to_index_map_dict[key]) - 1)

    # Remove duplicates
    port_index_list = list(dict.fromkeys(port_index_list))

    return port_index_list if port_index_list else None


def get_path_list_to_asic_hwsku_dir(num_of_asics):
    platform_path = device_info.get_path_to_platform_dir()
    hwsku = device_info.get_hwsku()
    if num_of_asics == 1:
        return [os.path.join(platform_path, hwsku, HWSKU_JSON)]
    else:
        return [os.path.join(platform_path, hwsku, str(asic_id), HWSKU_JSON) for asic_id in range(num_of_asics)]


def extract_RJ45_ports_index(num_of_asics=1):
    return _extract_ports_index_by_type(RJ45_PORT_TYPE, num_of_asics)


def build_cpo_port_map():
    """Build the CPO port map used by chassis as cpo_port_map.

    Reads optical_devices.json (interfaces only). Each interface may list OE/ELS
    devices; the first time a (oe_id, els_id, bank_id) triple appears in file order,
    it is assigned the next chassis index 0, 1, 2, ... Duplicate triples on later
    interfaces (e.g. multiple Ethernet names for the same bank) are skipped.

    Returns:
        dict[int, tuple[int, int, int]]: chassis_index -> (oe_id, els_id, bank_id)
        for CpoPort construction, e.g. {0: (0, 0, 0), 1: (0, 0, 1)}.
        None if the file is missing, empty, or has no parseable interfaces.
    """
    platform_path = device_info.get_path_to_platform_dir()
    optical_devices_path = os.path.join(platform_path, device_info.OPTICAL_DEVICES_JSON_FILE)
    if not os.path.exists(optical_devices_path):
        return None
    optical_devices_data = load_json_file(optical_devices_path)
    if not optical_devices_data:
        return None
    interfaces = optical_devices_data.get('interfaces')

    seen = set()
    cpo_port_map = {}
    index = 0
    for interface_name, interface_data in interfaces.items():
        interface_associated_devices = interface_data.get('associated_devices') or []
        oe_id, els_id, bank_id = _parse_oe_els_bank_for_interface(interface_associated_devices, interface_name)
        if oe_id is None:
            continue
        oe_els_bank = (oe_id, els_id, bank_id)
        if oe_els_bank in seen:
            continue
        seen.add(oe_els_bank)
        cpo_port_map[index] = oe_els_bank
        index += 1

    return cpo_port_map if cpo_port_map else None


def _parse_oe_els_bank_for_interface(interface_associated_devices, interface_name):
    """
    Parse OE / ELS numeric ids and OE bank from one interface's associated_devices list.

    Returns:
        tuple: (oe_id, els_id, bank_id) with integers, or (None, None, None) if OE or ELS is missing.
    """
    if not interface_associated_devices:
        return None, None, None
    oe_id = els_id = None
    bank_id = 0
    for device in interface_associated_devices:
        device_id = device.get('device_id') or ''
        oe_match = _OE_DEVICE_ID_RE.match(device_id)
        if oe_match:
            oe_id = int(oe_match.group(1))
            bank_id = int(device.get('bank', 0))
            continue
        els_match = _ELS_DEVICE_ID_RE.match(device_id)
        if els_match:
            els_id = int(els_match.group(1))
    if oe_id is None or els_id is None:
        logger.log_error(
            "Interface {} is missing OE or ELS in associated_devices: {}"
            .format(interface_name, interface_associated_devices))
        return None, None, None
    return oe_id, els_id, bank_id


def build_sfp_port_map():
    """Build the service port map used by chassis as sfp_port_map.

    Reads optical_devices.json (sfps section). Maps module_index to
    SDK module_index for the service port on CPO platforms, e.g. {64: 16}.

    Returns:
        dict[int, int] or None if optical_devices.json is missing or has no
        parseable sfps entries.
    """
    platform_path = device_info.get_path_to_platform_dir()
    optical_devices_path = os.path.join(platform_path, device_info.OPTICAL_DEVICES_JSON_FILE)
    if not os.path.exists(optical_devices_path):
        return None
    optical_devices_data = load_json_file(optical_devices_path)
    if not optical_devices_data:
        return None
    service_ports_data = optical_devices_data.get('sfps')
    if not isinstance(service_ports_data, dict):
        return None
    sfp_port_map = {}
    for _interface_name, interface_data in service_ports_data.items():
        raw_index = interface_data.get('module_index')
        raw_sdk_index = interface_data.get('sdk_module_index')
        if raw_index is None or raw_sdk_index is None:
            continue
        try:
            module_index = int(raw_index)
            sdk_module_index = int(raw_sdk_index)
        except (TypeError, ValueError):
            continue
        sfp_port_map[module_index] = sdk_module_index
    return sfp_port_map if sfp_port_map else None


# Use this function only for files that have user read permission.
def wait_for_file_creation(file_path, timeout):
    """
    Wait for a file to be created using inotify

    Args:
        file_path: Path to the file to wait for
        timeout: Timeout in seconds

    Returns:
        True if file was created/copied from a temporary file, and is readable, False otherwise
    """
    # If file already exists and is readable, return immediately
    if os.access(file_path, os.R_OK):
        return True

    dir_path = os.path.dirname(file_path)
    file_name = os.path.basename(file_path)

    if not os.path.exists(dir_path):
        logger.log_debug("Directory {} does not exist".format(dir_path))
        return False

    try:
        notifier = inotify.adapters.Inotify()
        notifier.add_watch(dir_path,
                           mask=(inotify.constants.IN_CREATE
                                 | inotify.constants.IN_CLOSE_WRITE
                                 | inotify.constants.IN_MOVED_TO))

        for event in notifier.event_gen(timeout_s=timeout, yield_nones=False):
            (_, type_names, path, filename) = event
            if filename == file_name:
                if "IN_CREATE" in type_names or "IN_CLOSE_WRITE" in type_names or "IN_MOVED_TO" in type_names:
                    if os.access(file_path, os.R_OK):
                        logger.log_info("File {} created and readable".format(file_path))
                        return True

    except Exception as e:
        logger.log_error("Inotify error while waiting for {}: {}".format(file_path, repr(e)))

    if os.access(file_path, os.R_OK):
        return True

    return False


def extract_asic_id_map(num_of_asics=1):
    asic_id_map = {}

    hwsku_jsons = get_path_list_to_asic_hwsku_dir(num_of_asics)
    interface2asic = {}
    for asic_id, hwsku_json in enumerate(hwsku_jsons):
        interface2asic.update({interface: asic_id for interface in load_json_file(hwsku_json)['interfaces'].keys()})

    platform_file = os.path.join(device_info.get_path_to_platform_dir(), device_info.PLATFORM_JSON_FILE)
    platform_dict = load_json_file(platform_file)['interfaces']

    for inteface, value in platform_dict.items():
        if PORT_INDEX_KEY in value:
            index_raw = value[PORT_INDEX_KEY]
            # The index could be "1" or "1, 1, 1, 1"
            index = index_raw.split(',')[0]
            asic_id_map[int(index)-1] = interface2asic[inteface]
    return asic_id_map


def get_path_to_hwsku_directory(asic_id=None):
    platform_path = device_info.get_path_to_platform_dir()
    hwsku = device_info.get_hwsku()
    if asic_id is not None:
        return os.path.join(platform_path, hwsku, str(asic_id))
    else:
        return os.path.join(platform_path, hwsku)


def get_path_list_to_asic_hwsku_dir(num_of_asics):
    if num_of_asics == 1:
        return [os.path.join(get_path_to_hwsku_directory(), HWSKU_JSON)]
    else:
        return [os.path.join(get_path_to_hwsku_directory(asic_id), HWSKU_JSON) for asic_id in range(num_of_asics)]


def wait_until(predict, timeout, interval=1, *args, **kwargs):
    """Wait until a condition become true

    Args:
        predict (object): a callable such as function, lambda
        timeout (int): wait time in seconds
        interval (int, optional): interval to check the predict. Defaults to 1.

    Returns:
        _type_: _description_
    """
    if predict(*args, **kwargs):
        return True
    while timeout > 0:
        time.sleep(interval)
        timeout -= interval
        if predict(*args, **kwargs):
            return True
    return False


def wait_until_conditions(conditions, timeout, interval=1):
    """
    Wait until all the conditions become true
    Args:
        conditions (list): a list of callable which generate True|False
        timeout (int): wait time in seconds
        interval (int, optional):  interval to check the predict. Defaults to 1.

    Returns:
        bool: True if wait success else False
    """
    while timeout > 0:
        pending_conditions = []
        for condition in conditions:
            if not condition():
                pending_conditions.append(condition)
        if not pending_conditions:
            return True
        conditions = pending_conditions
        time.sleep(interval)
        timeout -= interval
    return False

  
class TimerEvent:
    def __init__(self, interval, cb, repeat):
        self.interval = interval
        self._cb = cb
        self.repeat = repeat

    def execute(self):
        self._cb()


class Timer(threading.Thread):
    def __init__(self):
        super(Timer, self).__init__()
        self._timestamp_queue = queue.PriorityQueue()
        self._wait_event = threading.Event()
        self._stop_event = threading.Event()
        self._min_timestamp = None

    def schedule(self, interval, cb, repeat=True, run_now=True):
        timer_event = TimerEvent(interval, cb, repeat)
        self.add_timer_event(timer_event, run_now)

    def add_timer_event(self, timer_event, run_now=True):
        timestamp = time.monotonic()
        if not run_now:
            timestamp += timer_event.interval

        self._timestamp_queue.put_nowait((timestamp, timer_event))
        if self._min_timestamp is not None and timestamp < self._min_timestamp:
            self._wait_event.set()

    def stop(self):
        if self.is_alive():
            self._wait_event.set()
            self._stop_event.set()
            self.join()

    def run(self):
        while not self._stop_event.is_set():
            now = time.monotonic()
            item = self._timestamp_queue.get()
            self._min_timestamp = item[0]
            if self._min_timestamp > now:
                self._wait_event.wait(self._min_timestamp - now)
                self._wait_event.clear()
                self._timestamp_queue.put(item)
                continue

            timer_event = item[1]
            timer_event.execute()
            if timer_event.repeat:
                self.add_timer_event(timer_event, False)


class DbUtils:
    lock = threading.Lock()
    db_instances = threading.local()

    @classmethod
    def get_db_instance(cls, db_name, **kargs):
        try:
            if not hasattr(cls.db_instances, 'data'):
                with cls.lock:
                    if not hasattr(cls.db_instances, 'data'):
                        cls.db_instances.data = {}

            if db_name not in cls.db_instances.data:
                from swsscommon.swsscommon import ConfigDBConnector
                db = ConfigDBConnector(use_unix_socket_path=True)
                db.db_connect(db_name)
                cls.db_instances.data[db_name] = db
            return cls.db_instances.data[db_name]
        except Exception as e:
            logger.log_error(f'Failed to get DB instance for DB {db_name} - {e}')
            raise e
