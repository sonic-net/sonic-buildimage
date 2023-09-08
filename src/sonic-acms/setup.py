from __future__ import print_function
import glob
import sys

from setuptools import setup
import pkg_resources
from packaging import version

# sonic_dependencies, version requirement only supports '>='
sonic_dependencies = ['sonic-py-common']

dependencies = [
]

py_modules = [
    'dSMS_config_modifier',
]

dependencies += sonic_dependencies
for package in sonic_dependencies:
    try:
        package_dist = pkg_resources.get_distribution(package.split(">=")[0])
    except pkg_resources.DistributionNotFound:
        print(package + " is not found!", file=sys.stderr)
        print("Please build and install SONiC python wheels dependencies from sonic-buildimage", file=sys.stderr)
        exit(1)
    if ">=" in package:
        if version.parse(package_dist.version) >= version.parse(package.split(">=")[1]):
            continue
        print(package + " version not match!", file=sys.stderr)
        exit(1)

setup(
    name = 'sonic-acms',
    version = '1.0',
    description = 'Utilities for ACMS',
    author = 'Gang Lv',
    author_email = 'ganglv@microsoft.com',
    url = 'https://github.com/sonic-net/sonic-buildimage',
    py_modules = py_modules,
    scripts = [
        'CA_cert_downloader.py',
        'cert_converter.py',
        'start.py',
    ],
    install_requires = dependencies,
    data_files = [
    ],
    setup_requires= [
        'pytest-runner',
        'wheel'
    ],
    tests_require=[
        'pytest',
    ],
    classifiers = [
        'Intended Audience :: Developers',
        'Natural Language :: English',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
    ],
    keywords = 'SONiC sonic-acms PYTHON python'
)
