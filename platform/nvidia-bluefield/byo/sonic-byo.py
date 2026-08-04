#!/usr/bin/python
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

import argparse
import docker
import subprocess
import os
import sys

DOCKER_TIMEOUT = 600
CONTAINER_NAME = 'byo-app-container'

client = docker.DockerClient(timeout=DOCKER_TIMEOUT)
api_client = docker.APIClient(timeout=DOCKER_TIMEOUT)


def get_args():
    parser = argparse.ArgumentParser(description='Utility for SONiC BYO configuration')
    mode = parser.add_subparsers(dest='mode')
    mode.required = True

    mode.add_parser('disable', help='Stop BYO application, restart SONiC services')

    enable_parser = mode.add_parser('enable', help='Stop SONiC services and run BYO application')
    group = enable_parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--pull', help='Docker image URL to pull')
    group.add_argument('--file', help='Docker image gz file to load')
    group.add_argument('--image', help='Docker image name to run')

    return parser.parse_args()


def run_cmd(cmd, verbose=False):
    if verbose:
        print(' '.join(cmd))
    subprocess.run(cmd)


def sonic_services_ctl(start, verbose):
    services = [
        'featured',
        'swss',
        'syncd',
        'pmon',
        'snmp',
        'lldp',
        'gnmi',
        'bgp',
        'eventd'
    ]

    systemctlops_start = ['unmask', 'enable', 'start']
    systemctlops_stop = ['stop', 'disable', 'mask']

    # Start featured last
    if start:
        services = services[1:] + services[:1]

    print('#', 'Starting' if start else 'Stopping', ', '.join(services))

    ops = systemctlops_start if start else systemctlops_stop
    for op in ops:
        run_cmd(["systemctl", op] + services, verbose=verbose)


def prepare_sonic(verbose=False):
    print('# Preparing sonic..')

    sonic_services_ctl(start=False, verbose=verbose)

    print('# Loading mlx5_core driver')
    run_cmd(["modprobe", "mlx5_core"], verbose=verbose)


def restore_sonic(verbose=False):
    print('# Restoring sonic..')
    sonic_services_ctl(start=True, verbose=verbose)


def pull_image(name):
    try:
        print(f'# Pulling image {name}')
        for line in api_client.pull(name, stream=True, decode=True):
            status = line.get('status', '')
            progress = line.get('progress', '')
            if progress:
                sys.stdout.write(f'\r{status}: {progress}')
            else:
                sys.stdout.write(f'\r{status}')
            sys.stdout.flush()
        print()
        return api_client.inspect_image(name)['Id']
    except docker.errors.APIError as e:
        print(f'Error pulling image: {e}')
        return None


def load_gz(file):
    def chunked_file(f):
        loaded = 0
        total = os.path.getsize(file)
        chunk_size = max(8192, int(total / 1000))
        with open(file, 'rb') as f:
            while True:
                chunk = f.read(chunk_size)
                if not chunk:
                    print()
                    break
                loaded += len(chunk)
                progress = loaded / total * 100
                sys.stdout.write(f'\rLoading.. {progress:.2f}%')
                sys.stdout.flush()
                yield chunk

    try:
        print(f'# Loading image {file}')
        return client.images.load(chunked_file(file))[0].id

    except Exception as e:
        print(f'Failed to load: {e}')
        return None


def run_container(image):
    print(f'# Running image {image}')
    config = {
        'image': image,
        'name': CONTAINER_NAME,
        'detach': True,
        'tty': True,
        'privileged': True,
        'network_mode': 'host',
        'auto_remove': True
    }

    container = client.containers.run(**config) # nosemgrep: python.docker.security.audit.docker-arbitrary-container-run.docker-arbitrary-container-run
    print(f'Container name: {container.name}')


def stop_container():
    try:
        container = client.containers.get(CONTAINER_NAME)
        container.stop()
        print(f'Container {CONTAINER_NAME} stopped and removed successfully')

    except docker.errors.NotFound:
        print(f'Container {CONTAINER_NAME} not found')
    except Exception as e:
        print(f'Docker error occurred: {str(e)}')


def byo_enable(args):
    prepare_sonic(verbose=True)
    if args.pull:
        image_name = pull_image(args.pull)
    elif args.file:
        image_name = load_gz(args.file)
    else:
        image_name = args.image
    run_container(image_name)


def byo_disable():
    stop_container()
    restore_sonic(verbose=True)


def main():
    args = get_args()
    if args.mode == 'enable':
        byo_enable(args)
    if args.mode == 'disable':
        byo_disable()


if __name__ == "__main__":
    main()
