#
# Copyright (c) 2019-2024 NVIDIA CORPORATION & AFFILIATES.
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
from setuptools import setup

setup(
    name='mlnx-platform-api',
    version='1.0',
    description='SONiC platform API implementation on Mellanox platform',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='linuxnetdev@microsoft.com',
    url='https://github.com/Azure/sonic-buildimage',
    maintainer='Kevin Wang',
    maintainer_email='kevinw@mellanox.com',
    packages=[
        'sonic_platform',
        'tests',
        'smart_switch.dpuctl'
    ],
    setup_requires= [
        'pytest-runner'
    ],
    install_requires= [
        'inotify'
    ],
    tests_require = [
        'pytest',
        'mock>=2.0.0'
    ],
    entry_points={
    'console_scripts': [
            'dpuctl = smart_switch.dpuctl.main:dpuctl',
    ]
    },
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Environment :: Plugins',
        'Intended Audience :: Developers',
        'Intended Audience :: Information Technology',
        'Intended Audience :: System Administrators',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.7',
        'Topic :: Utilities',
    ],
    keywords='sonic SONiC platform PLATFORM',
    test_suite='setup.get_test_suite'
)

