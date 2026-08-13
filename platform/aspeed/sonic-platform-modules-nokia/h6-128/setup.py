#!/usr/bin/env python3

from setuptools import setup

setup(
    name='sonic-platform',
    version='1.0',
    description='Module to initialize Nokia H6-128 BMC AST2720',
    packages=['sonic_platform'],
    package_dir={'sonic_platform': 'sonic_platform'},
)

