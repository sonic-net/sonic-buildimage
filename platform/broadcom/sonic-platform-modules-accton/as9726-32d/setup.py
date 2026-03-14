#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
    name='as9726_32d',
    version='1.0',
    description='Module to initialize Accton AS9726_32D platforms',

    packages=['as9726_32d'],
    package_dir={'as9726_32d': 'as9726-32d/classes'},
)
