#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
    name='as4630_54te',
    version='1.0',
    description='Module to initialize Accton AS4630-54TE platforms',

    packages=['as4630_54te'],
    package_dir={'as4630_54te': 'as4630-54te/classes'},
)
