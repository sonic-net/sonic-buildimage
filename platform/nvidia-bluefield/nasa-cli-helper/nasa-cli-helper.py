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
CT_TCP_AGING_IN_SECONDS_KEY = 'CT_TCP_AGING_IN_SECONDS'
CT_UDP_AGING_IN_SECONDS_KEY = 'CT_UDP_AGING_IN_SECONDS'
CT_TCP_AGING_DEFAULT_SECONDS = 300
CT_UDP_AGING_DEFAULT_SECONDS = 120


def get_sai_profile_path():
    """Return path to sai.profile under device tree for current platform/HwSKU."""
    if device_info is None:
        return None
    platform, hwsku = device_info.get_platform_and_hwsku()
    if not platform or not hwsku:
        return None
    return f'/usr/share/sonic/device/{platform}/{hwsku}/{SAI_PROFILE_FILENAME}'


def _aging_key_and_default(use_udp):
    """Return (profile_key, default_seconds) for TCP or UDP aging."""
    if use_udp:
        return CT_UDP_AGING_IN_SECONDS_KEY, CT_UDP_AGING_DEFAULT_SECONDS
    return CT_TCP_AGING_IN_SECONDS_KEY, CT_TCP_AGING_DEFAULT_SECONDS


def get_aging_interval(use_udp):
    """Read aging key from sai.profile. If file is missing, unreadable, or key absent, use TCP/UDP defaults."""
    key, default_seconds = _aging_key_and_default(use_udp)
    sai_profile_path = get_sai_profile_path()
    if not sai_profile_path:
        print("Error: Could not get platform/HwSKU", file=sys.stderr)
        sys.exit(1)
    try:
        with open(sai_profile_path, 'r') as f:
            content = f.read()
    except (FileNotFoundError, OSError):
        return default_seconds

    pattern = re.compile(r'^' + re.escape(key) + r'=(\d+)', re.MULTILINE)
    match = pattern.search(content)
    if match:
        return int(match.group(1))
    return default_seconds


def set_aging_interval(seconds, use_udp):
    """Update TCP or UDP aging key in sai.profile. Add line if missing, else replace.

    If sai.profile does not exist yet, it is created (same as starting from an empty file).
    """
    key, _ = _aging_key_and_default(use_udp)
    sai_profile_path = get_sai_profile_path()
    if not sai_profile_path:
        print("Error: Could not get platform/HwSKU (sonic_py_common.device_info not available or returned empty).", file=sys.stderr)
        sys.exit(1)
    content = ''
    try:
        with open(sai_profile_path, 'r') as f:
            content = f.read()
    except FileNotFoundError:
        pass
    except OSError as e:
        print(f"Error: Failed to read sai.profile: {e}", file=sys.stderr)
        sys.exit(1)

    new_line = f'{key}={seconds}'
    pattern = re.compile(r'^.*' + re.escape(key) + r'=.*$', re.MULTILINE)
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

    print(f"{key} has been set to {seconds}.", file=sys.stderr)
    print("Please run config reload or reboot for the change to take effect.", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description='NASA CLI Helper for NVIDIA BlueField')
    parser.add_argument('command', choices=['get_packet_debug_mode', 'get_sai_debug_mode', 'get_aging_interval', 'set_aging_interval'],
                        help='Command to execute')
    parser.add_argument('-f', '--filename', action='store_true',
                        help='Show filename instead of status (for get_packet_debug_mode / get_sai_debug_mode)')
    parser.add_argument('-u', '--udp', action='store_true',
                        help='Use UDP (default is TCP for aging commands)')
    parser.add_argument('value', nargs='?', type=int, default=None,
                        help='Aging interval in seconds (for set_aging_interval; default 300 TCP / 180 UDP)')

    args = parser.parse_args()

    if args.command == 'get_aging_interval':
        seconds = get_aging_interval(args.udp)
        print(f"{seconds}")
        return

    if args.command == 'set_aging_interval':
        _, default_seconds = _aging_key_and_default(args.udp)
        seconds = args.value if args.value is not None else default_seconds
        if seconds < 1:
            print("Error: aging interval must be >= 1 second.", file=sys.stderr)
            sys.exit(1)
        set_aging_interval(seconds, args.udp)
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
