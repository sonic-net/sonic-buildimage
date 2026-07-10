#!/usr/bin/env python3
#
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES.
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

import sys
import argparse
import docker
import config.plugins.nvidia_bluefield as nvda_bf

def main():
    parser = argparse.ArgumentParser(description='NASA CLI Helper for NVIDIA BlueField')
    parser.add_argument('command', choices=['get_packet_debug_mode', 'get_sai_debug_mode'],
                       help='Command to execute')
    parser.add_argument('-f', '--filename', action='store_true',
                       help='Show filename instead of status')

    args = parser.parse_args()

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
