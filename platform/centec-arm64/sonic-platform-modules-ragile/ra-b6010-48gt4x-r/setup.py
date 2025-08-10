#!/usr/bin/env python

import os
import sys
from setuptools import setup

setup(
    name='sonic_platform',
    version='1.0',
    description='Module to initialize centec ra-b6010-48gt4x platforms',

    packages=['sonic_platform'], 
    package_dir={'sonic_platform': 'sonic_platform'},
)

