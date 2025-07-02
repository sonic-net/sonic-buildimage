#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
    name='as9736_64d',
    version='1.0',
    description='Module to initialize Accton AS9736_64D platforms',

    packages=['as9736_64d'],
    package_dir={'as9736_64d': 'as9736-64d/classes'},
)
