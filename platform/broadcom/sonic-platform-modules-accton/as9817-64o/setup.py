#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
    name='as9817_64o',
    version='1.0',
    description='Module to initialize Accton AS9817-64O platforms',

    packages=['as9817_64o'],
    package_dir={'as9817_64o': 'as9817-64o/classes'},
)
