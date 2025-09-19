#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
    name='as9737_32db',
    version='1.0',
    description='Module to initialize Accton AS9737-32DB platforms',

    packages=['as9737_32db'],
    package_dir={'as9737_32db': 'as9737-32db/classes'},
)
