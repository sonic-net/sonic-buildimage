#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
    name='as4630_54npe',
    version='1.0',
    description='Module to initialize Accton AS4630-54NPE platforms',

    packages=['as4630_54npe'],
    package_dir={'as4630_54npe': 'as4630-54npe/classes'},
)
