#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

import re
import sys
import argparse
import docker
import config.plugins.nvidia_bluefield as nvda_bf

try:
    from sonic_py_common import device_info
except ImportError:
    device_info = None

SAI_PROFILE_FILENAME = 'sai.profile'
CT_AGING_IN_SECONDS_KEY = 'CT_AGING_IN_SECONDS'
CT_AGING_DEFAULT_SECONDS = 1


def get_sai_profile_path():
    """Return path to sai.profile under device tree for current platform/HwSKU."""
    if device_info is None:
        return None
    platform, hwsku = device_info.get_platform_and_hwsku()
    if not platform or not hwsku:
        return None
    return f'/usr/share/sonic/device/{platform}/{hwsku}/{SAI_PROFILE_FILENAME}'


def get_aging_interval():
    """Read CT_AGING_IN_SECONDS from sai.profile. If not present, return default 1."""
    sai_profile_path = get_sai_profile_path()
    if not sai_profile_path:
        print("Error: Could not get platform/HwSKU", file=sys.stderr)
        sys.exit(1)
    try:
        with open(sai_profile_path, 'r') as f:
            content = f.read()
    except (FileNotFoundError, OSError):
        return CT_AGING_DEFAULT_SECONDS

    pattern = re.compile(r'^' + re.escape(CT_AGING_IN_SECONDS_KEY) + r'=(\d+)', re.MULTILINE)
    match = pattern.search(content)
    if match:
        return int(match.group(1))
    return CT_AGING_DEFAULT_SECONDS


def set_aging_interval(seconds):
    """Update CT_AGING_IN_SECONDS in sai.profile. Add line if missing, else replace."""
    sai_profile_path = get_sai_profile_path()
    if not sai_profile_path:
        print("Error: Could not get platform/HwSKU (sonic_py_common.device_info not available or returned empty).", file=sys.stderr)
        sys.exit(1)
    try:
        with open(sai_profile_path, 'r') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Error: {sai_profile_path} not found.", file=sys.stderr)
        sys.exit(1)
    except OSError as e:
        print(f"Error: Failed to read sai.profile: {e}", file=sys.stderr)
        sys.exit(1)

    new_line = f'{CT_AGING_IN_SECONDS_KEY}={seconds}'
    pattern = re.compile(r'^.*' + re.escape(CT_AGING_IN_SECONDS_KEY) + r'=.*$', re.MULTILINE)
    if pattern.search(content):
        content = pattern.sub(new_line, content, count=1)
    else:
        content = content.rstrip()
        if content and not content.endswith('\n'):
            content += '\n'
        content += new_line + '\n'

    try:
        with open(sai_profile_path, 'w') as f:
            f.write(content)
    except OSError as e:
        print(f"Error: Failed to write sai.profile: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"CT_AGING_IN_SECONDS has been set to {seconds}.", file=sys.stderr)
    print("Please run config reload or reboot for the change to take effect.", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description='NASA CLI Helper for NVIDIA BlueField')
    parser.add_argument('command', choices=['get_packet_debug_mode', 'get_sai_debug_mode', 'get_aging_interval', 'set_aging_interval'],
                        help='Command to execute')
    parser.add_argument('-f', '--filename', action='store_true',
                        help='Show filename instead of status (for get_* commands)')
    parser.add_argument('value', nargs='?', type=int, default=None,
                        help='Aging interval in seconds (for set_aging_interval; default 1)')

    args = parser.parse_args()

    if args.command == 'get_aging_interval':
        seconds = get_aging_interval()
        print(f"{seconds}")
        return

    if args.command == 'set_aging_interval':
        seconds = args.value if args.value is not None else CT_AGING_DEFAULT_SECONDS
        if seconds < 1:
            print("Error: aging interval must be >= 1 second.", file=sys.stderr)
            sys.exit(1)
        set_aging_interval(seconds)
        return

    try:
        docker_client = docker.from_env()
    except docker.errors.DockerException as e:
        print(f"Error: Unable to connect to Docker. {e}", file=sys.stderr)
        sys.exit(1)

    try:
        if args.command == 'get_packet_debug_mode':
            status, filename = nvda_bf.get_packet_debug_mode(docker_client)
        elif args.command == 'get_sai_debug_mode':
            status, filename = nvda_bf.get_sai_debug_mode(docker_client)
    except Exception as e:
        print(f"Error: Failed to execute '{args.command}': {e}", file=sys.stderr)
        sys.exit(1)

    if args.filename:
        print(filename)
    else:
        print(status)


if __name__ == '__main__':
    main()
