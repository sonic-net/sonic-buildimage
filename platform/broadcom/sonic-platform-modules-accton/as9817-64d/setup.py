#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
    name='as9817_64d',
    version='1.0',
    description='Module to initialize Accton AS9817-64D platforms',

    packages=['as9817_64d'],
    package_dir={'as9817_64d': 'as9817-64d/classes'},
)
